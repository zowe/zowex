/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 */

import { posix } from "node:path";
import type { Stream } from "node:stream";
import { ImperativeError, Logger } from "@zowe/imperative";
import type { SshSession } from "@zowe/zos-uss-for-zowe-sdk";
import type {
    ClientOptions,
    CommandRequest,
    CommandResponse,
    ExistingClientRequest,
    RpcNotification,
    RpcRequest,
    RpcResponse,
    StatusMessage,
} from "./doc";
import { RpcClientApi } from "./RpcClientApi";
import { RpcStreamManager } from "./RpcStreamManager";
import { type Client, type ClientChannel, createClient } from "./ssh-rs";
import { ZSshUtils } from "./ZSshUtils";

export class ZSshClient extends RpcClientApi implements Disposable {
    public static readonly DEFAULT_SERVER_PATH = "~/.zowe-server";
    private mErrHandler: ClientOptions["onError"];
    private mResponseTimeout: number;
    private mServerInfo: { checksums?: Record<string, string> };
    private mSshClient: Client;
    private mSshStream: ClientChannel;
    private mStreamMgr: RpcStreamManager;
    private mPartialStderr = "";
    private mPartialStdout = "";
    private readonly mRequestMap: Map<number, ExistingClientRequest> = new Map();
    // biome-ignore lint/correctness/noUnusedPrivateClassMembers: Linter Error: this.mRequestId is used
    private mRequestId = 0;

    private constructor() {
        super();
    }

    private static defaultErrHandler(err: Error): void {
        const errMsg = err.toString();
        console.error(errMsg);
        Logger.getAppLogger().error(errMsg);
    }

    public static async create(session: SshSession, opts: ClientOptions = {}): Promise<ZSshClient> {
        Logger.getAppLogger().debug("Starting SSH client");
        const client = new ZSshClient();
        client.mErrHandler = opts.onError ?? ZSshClient.defaultErrHandler;
        client.mResponseTimeout = opts.responseTimeout ? opts.responseTimeout * 1000 : 60e3;
        client.mSshClient = createClient(opts.useNativeSsh);
        client.mSshStream = await new Promise((resolve, reject) => {
            client.mSshClient.on("error", (err) => {
                Logger.getAppLogger().error(`Error connecting to SSH: ${err}`);
                reject(err);
            });
            client.mSshClient.on("ready", async () => {
                const zowexBin = posix.join(opts.serverPath ?? ZSshClient.DEFAULT_SERVER_PATH, "zowex");
                const serverArgs = ["server"];
                if (opts.numWorkers != null) {
                    serverArgs.push("--num-workers", `${opts.numWorkers}`);
                }
                if (opts.requestTimeout != null) {
                    serverArgs.push("--request-timeout", `${opts.requestTimeout}`);
                }
                if (opts.verbose) {
                    serverArgs.push("--verbose");
                }
                client.execAsync(zowexBin, ...serverArgs).then(resolve, reject);
            });
            client.mSshClient.on("close", () => {
                Logger.getAppLogger().debug("Client disconnected");
                opts.onClose?.();
            });
            const keepAliveMsec = opts.keepAliveInterval != null ? opts.keepAliveInterval * 1000 : 30e3;
            client.mSshClient.connect(ZSshUtils.buildSshConfig(session, { keepaliveInterval: keepAliveMsec }));
        });
        client.mStreamMgr = new RpcStreamManager(client.mSshClient);

        if (opts.requests != null) {
            for (const req of opts.requests) {
                client.request(req.command).then(req.rpc.resolve, req.rpc.reject);
            }
        }

        return client;
    }

    /**
     * Collects all currently pending RPC requests. Can be used to pause, replay, or inspect inflight requests.
     * * @param silence - If true, silences the requests. They will no longer trigger their resolve or reject methods in normal operation (timeout, server response). The caller assumes responsibility for these resolve and rejects.
     * @returns A set of existing client requests currently pending resolution.
     */
    public collectAllRequests(silence: boolean = false): Set<ExistingClientRequest> {
        const replayRequests: Set<ExistingClientRequest> = new Set();
        this.mRequestMap.forEach((req) => {
            req.silenced = silence;
            replayRequests.add(req);
            if (req.timeoutId) {
                // don't wait for timeout
                clearTimeout(req.timeoutId);
            }
        });
        return replayRequests;
    }

    /**
     * Cleans up the SSH client by ending the SSH connection and rejecting pending requests
     * that have not been silenced. If you want to silence requests, see {@link collectAllRequests}
     * * @param isRestart - Indicates if the disposal is part of a server restart sequence
     * (changes the rejection message sent to pending requests).
     */
    public dispose(isRestart: boolean = false): void {
        Logger.getAppLogger().debug("Stopping SSH client");
        this.mRequestMap.forEach((req) => {
            let rejMsg = "Shutting down ZRS. No action is required.";
            if (isRestart) {
                rejMsg = "Restarting ZRS";
            }
            if (!req.silenced) {
                req.rpc.reject(new ImperativeError({ msg: rejMsg, suppressDump: true }));
            }
        });
        this.mRequestMap.clear();
        this.mSshClient?.end();
    }

    public [Symbol.dispose](): void {
        this.dispose();
    }

    public get serverChecksums(): Record<string, string> | undefined {
        return this.mServerInfo?.checksums;
    }

    public async request<T extends CommandResponse>(
        request: CommandRequest,
        progressCallback?: (percent: number) => void,
    ): Promise<T> {
        let timeoutId: NodeJS.Timeout;
        return new Promise<T>((resolve, reject) => {
            const { command, ...rest } = request;
            const rpcRequest: RpcRequest = {
                jsonrpc: "2.0",
                method: command,
                params: rest,
                id: ++this.mRequestId,
            };
            timeoutId = setTimeout(() => {
                const thisReq = this.mRequestMap.get(rpcRequest.id);
                if (thisReq) {
                    this.mRequestMap.delete(rpcRequest.id);
                    if (!thisReq.silenced) {
                        this.mErrHandler(new Error("Request timed out")); // our hook provides better error message and action
                        reject(new ImperativeError({ msg: "Request timed out", errorCode: "ETIMEDOUT" }));
                    }
                }
            }, this.mResponseTimeout);
            if ("stream" in request && this.isStreamFunction(request.stream)) {
                this.mStreamMgr.registerStream(
                    rpcRequest,
                    request.stream,
                    timeoutId,
                    progressCallback && {
                        callback: progressCallback,
                        // If stream is a ReadStream use the size of the localFile in bytes
                        // If stream is a WriteStream, set undefined because the size will be provided by a notification
                        totalBytes: "contentLen" in request ? (request.contentLen as number) : undefined,
                    },
                );
            }
            this.mRequestMap.set(rpcRequest.id, {
                command: request,
                rpc: { resolve, reject },
                silenced: false,
                timeoutId,
            });
            const requestStr = JSON.stringify(rpcRequest);
            Logger.getAppLogger().trace(`Sending request: ${requestStr}`);
            this.mSshStream.stdin.write(`${requestStr}\n`);
        }).finally(() => clearTimeout(timeoutId));
    }

    private execAsync(...args: string[]): Promise<ClientChannel> {
        return new Promise((resolve, reject) => {
            this.mSshClient.exec(args.join(" "), (err, stream) => {
                if (err) {
                    Logger.getAppLogger().error(`Error running SSH command: ${err}`);
                    reject(err);
                } else {
                    const onData = (data: Buffer) => {
                        const removeListeners = () => {
                            stream.stderr.removeListener("data", onData);
                            stream.stdout.removeListener("data", onData);
                        };
                        try {
                            this.mServerInfo = this.getServerStatus(stream, data.toString(), args.join(" "));
                            if (this.mServerInfo) {
                                removeListeners();
                                resolve(stream);
                            }
                        } catch (err) {
                            removeListeners();
                            reject(err);
                        }
                    };
                    stream.stderr.on("data", onData);
                    stream.stdout.on("data", onData);
                }
            });
        });
    }

    private getServerStatus(stream: ClientChannel, data: string, command: string): StatusMessage["data"] | undefined {
        Logger.getAppLogger().debug(`Received SSH data: ${data}`);
        let response: StatusMessage;
        try {
            response = JSON.parse(data);
        } catch {
            const errMsg = Logger.getAppLogger().error("Error starting Zowe server: %s\n%s", command, data);
            if (data.includes("FSUM7351")) {
                throw new ImperativeError({
                    msg: "Server not found",
                    errorCode: "ENOTFOUND",
                    additionalDetails: data,
                });
            }
            if (data.includes("FOTS1681")) {
                // non-fatal chdir error, display the error but continue waiting for ready message
                this.mErrHandler(new Error(errMsg));
                return;
            }
            throw new ImperativeError({ msg: `Error starting Zowe server: ${command}`, additionalDetails: data });
        }
        if (response?.status === "ready") {
            stream.stderr.on("data", this.onErrData.bind(this));
            stream.stdout.on("data", this.onOutData.bind(this));
            Logger.getAppLogger().debug("Client is ready");
            return response.data;
        }
    }

    private onErrData(chunk: Buffer) {
        if (this.mRequestMap.size === 0) {
            const errMsg = Logger.getAppLogger().error("Error from server: %s", chunk.toString());
            this.mErrHandler(new Error(errMsg));
            return;
        }
        this.mPartialStderr = this.processResponses(this.mPartialStderr + chunk.toString());
    }

    private onOutData(chunk: Buffer) {
        this.mPartialStdout = this.processResponses(this.mPartialStdout + chunk.toString());
    }

    private processResponses(data: string): string {
        const responses = data.split("\n");
        for (let i = 0; i < responses.length - 1; i++) {
            if (responses[i].length === 0) {
                continue;
            }

            Logger.getAppLogger().trace(`Received response: ${responses[i]}`);
            let response: RpcResponse | RpcNotification;
            try {
                response = JSON.parse(responses[i]);
            } catch {
                const errMsg = Logger.getAppLogger().error("Invalid JSON response: %s", responses[i]);
                this.mErrHandler(new Error(errMsg));
                continue;
            }

            const responseId: number = "id" in response ? response.id : response.params?.id;
            if (!this.mRequestMap.has(responseId)) {
                const errMsg = Logger.getAppLogger().error("Missing promise for response ID: %d", responseId);
                this.mErrHandler(new Error(errMsg));
                continue;
            }

            if ("method" in response) {
                this.handleNotification(response);
            } else {
                this.handleResponse(response);
            }
        }
        return responses[responses.length - 1];
    }

    private handleNotification(notif: RpcNotification): void {
        const rpcPromise = this.mRequestMap.get(notif.params?.id as number);
        try {
            switch (notif.method) {
                case "receiveStream":
                    this.mStreamMgr.linkStreamToPromise(rpcPromise.rpc, notif, "GET");
                    break;
                case "sendStream":
                    this.mStreamMgr.linkStreamToPromise(rpcPromise.rpc, notif, "PUT");
                    break;
                default:
                    throw new Error(`unknown method ${notif.method}`);
            }
        } catch (err) {
            const errMsg = Logger.getAppLogger().error("Failed to handle RPC notification: %s", err.message);
            this.mErrHandler(new Error(errMsg));
        }
    }

    private handleResponse(response: RpcResponse): void {
        const request = this.mRequestMap.get(response.id);

        if (request.silenced) {
            // Requests that are silenced have already been collected.
            this.mRequestMap.delete(response.id);
            return;
        }

        if (response.error != null) {
            Logger.getAppLogger().error(`Error for response ID: ${response.id}\n${JSON.stringify(response.error)}`);
            this.mRequestMap.get(response.id).rpc.reject(
                new ImperativeError({
                    msg: response.error.message,
                    errorCode: response.error.code.toString(),
                    additionalDetails: response.error.data,
                }),
            );
        } else {
            this.mRequestMap.get(response.id).rpc.resolve(response.result);
        }

        this.mRequestMap.delete(response.id);
    }

    /**
     * Type guard to verify if an incoming request parameter contains a function that returns a Node.js Stream.
     * * @param req - The unknown property to inspect.
     * @returns True if `req` is a function, and asserts it as a stream generator function.
     */
    private isStreamFunction(req: unknown): req is () => Stream {
        return typeof req === "function";
    }
}
