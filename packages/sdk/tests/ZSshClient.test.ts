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

import { EventEmitter, Readable } from "node:stream";
import { Logger } from "@zowe/imperative";
import { type ISshSession, SshSession } from "@zowe/zos-uss-for-zowe-sdk";
import { Client, type ClientCallback, type ConnectConfig } from "ssh2";
import type { ReadableStreamRpc } from "../lib";
import type { CommandRequest, RpcRequest, RpcResponse } from "../src/doc";
import * as sshRs from "../src/ssh-rs";
import { ZSshClient } from "../src/ZSshClient";
import { ZSshUtils } from "../src/ZSshUtils";

vi.mock("ssh2");
vi.mock("../src/ssh-rs", () => {
    return { createClient: () => new Client() };
});

const fakeSession: ISshSession = {
    hostname: "example.com",
    port: 22,
    user: "admin",
    privateKey: "~/.ssh/id_rsa",
};

describe("ZSshClient", () => {
    const readyMessage = JSON.stringify({ status: "ready", data: { checksums: "sha256" } });
    const rpcRequest: RpcRequest = {
        jsonrpc: "2.0",
        method: "ping",
        params: {},
        id: 1,
    };
    const rpcResponseBad: RpcResponse = {
        jsonrpc: "2.0",
        error: { code: 0, message: "bad rpc" },
        id: 1,
    };
    const rpcResponseGood: RpcResponse = {
        jsonrpc: "2.0",
        result: { success: true },
        id: 1,
    };

    beforeAll(() => {
        vi.useFakeTimers();
    });

    beforeEach(() => {
        vi.spyOn(ZSshUtils, "buildSshConfig").mockReturnValue({});
    });

    describe("create function", () => {
        it("should start SSH client with default options", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            const client = await ZSshClient.create(new SshSession(fakeSession));
            expect(client).toBeInstanceOf(ZSshClient);
        });

        it("should handle error thrown by SSH client", async () => {
            const sshError = new Error("bad ssh");
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("error", sshError);
                return this;
            });
            await expect(ZSshClient.create(new SshSession(fakeSession))).rejects.toThrow(sshError);
        });

        it("should handle error in SSH exec callback", async () => {
            const sshError = new Error("bad ssh");
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            vi.spyOn(Client.prototype, "exec").mockImplementation(function (
                _command: string,
                callback: ClientCallback,
            ) {
                callback(sshError, undefined as any);
                return this;
            });
            await expect(ZSshClient.create(new SshSession(fakeSession))).rejects.toThrow(sshError);
        });

        it("should handle error in SSH data handler", async () => {
            const sshError = new Error("bad ssh");
            const sshStream = { stderr: new EventEmitter(), stdout: new EventEmitter() };
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            vi.spyOn(Client.prototype, "exec").mockImplementation(function (
                _command: string,
                callback: ClientCallback,
            ) {
                callback(undefined, sshStream as any);
                sshStream.stderr.emit("data", "");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "getServerStatus").mockImplementation(() => {
                throw sshError;
            });
            await expect(ZSshClient.create(new SshSession(fakeSession))).rejects.toThrow(sshError);
        });

        it("should invoke onClose callback", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                this.emit("close");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            const onCloseMock = vi.fn();
            await ZSshClient.create(new SshSession(fakeSession), {
                onClose: onCloseMock,
            });
            expect(onCloseMock).toHaveBeenCalledTimes(1);
        });

        it("should invoke onError callback", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            const onErrorMock = vi.fn();
            const client = await ZSshClient.create(new SshSession(fakeSession), {
                onError: onErrorMock,
            });
            (client as any).onErrData("bad ssh");
            expect(onErrorMock).toHaveBeenCalledTimes(1);
        });

        it("should respect keepAliveInterval option", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            const buildSshConfigMock = vi.spyOn(ZSshUtils, "buildSshConfig");
            await ZSshClient.create(new SshSession(fakeSession), {
                keepAliveInterval: 5,
            });
            expect(buildSshConfigMock).toHaveBeenCalledTimes(1);
            expect(buildSshConfigMock.mock.calls[0][buildSshConfigMock.mock.calls[0].length - 1]).toEqual({
                keepaliveInterval: 5e3,
            });
        });

        it("should respect numWorkers option", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            const execAsyncMock = vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            await ZSshClient.create(new SshSession(fakeSession), {
                numWorkers: 1,
            });
            expect(execAsyncMock).toHaveBeenCalledTimes(1);
            expect(execAsyncMock.mock.calls[0].slice(1)).toEqual(["server", "--num-workers", "1"]);
        });

        it("should respect requestTimeout option", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            const execAsyncMock = vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            await ZSshClient.create(new SshSession(fakeSession), {
                requestTimeout: 300,
            });
            expect(execAsyncMock).toHaveBeenCalledTimes(1);
            expect(execAsyncMock.mock.calls[0].slice(1)).toEqual(["server", "--request-timeout", "300"]);
        });

        it("should respect responseTimeout option", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            const client = await ZSshClient.create(new SshSession(fakeSession), {
                responseTimeout: 300,
            });
            expect((client as any).mResponseTimeout).toBe(3e5);
        });

        it("should respect verbose option", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            const execAsyncMock = vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            await ZSshClient.create(new SshSession(fakeSession), {
                verbose: true,
            });
            expect(execAsyncMock).toHaveBeenCalledTimes(1);
            expect(execAsyncMock.mock.calls[0].slice(1)).toEqual(["server", "--verbose"]);
        });

        it("should respect serverPath option", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            const execAsyncMock = vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            await ZSshClient.create(new SshSession(fakeSession), {
                serverPath: "/tmp/zowe-server",
            });
            expect(execAsyncMock).toHaveBeenCalledTimes(1);
            expect(execAsyncMock.mock.calls[0][0]).toBe("/tmp/zowe-server/zowex");
        });

        it("should respect useNativeSsh option", async () => {
            const sshStream = new EventEmitter();
            vi.spyOn(Client.prototype, "connect").mockImplementation(function (_config: ConnectConfig) {
                this.emit("ready");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(sshStream);
            const createClientMock = vi.spyOn(sshRs, "createClient").mockReturnValue(new Client() as any);
            await ZSshClient.create(new SshSession(fakeSession), { useNativeSsh: true });
            expect(createClientMock).toHaveBeenCalledWith(true);
        });
    });

    describe("dispose function", () => {
        it("should end SSH connection", () => {
            const endMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshClient = { end: endMock };
            client.dispose();
            expect(endMock).toHaveBeenCalledTimes(1);
        });

        it("should end SSH connection 2", () => {
            const endMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshClient = { end: endMock };
            client[Symbol.dispose]();
            expect(endMock).toHaveBeenCalledTimes(1);
        });
    });

    describe("request function", () => {
        it("should send request that succeeds", async () => {
            const request: CommandRequest = { command: "ping" };
            const writeMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = { stdin: { write: writeMock } };
            const response = client.request(request);
            (client as any).processResponses(`${JSON.stringify(rpcResponseGood)}\n`);
            expect(await response).toEqual({ success: true });
            expect(writeMock.mock.calls[0]).toEqual([`${JSON.stringify(rpcRequest)}\n`]);
        });

        it("should send request that fails", async () => {
            const request: CommandRequest = { command: "ping" };
            const writeMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = { stdin: { write: writeMock } };
            const response = client.request(request);
            (client as any).processResponses(`${JSON.stringify(rpcResponseBad)}\n`);
            await expect(response).rejects.toMatchObject({ message: rpcResponseBad.error?.message });
            expect(writeMock.mock.calls[0]).toEqual([`${JSON.stringify(rpcRequest)}\n`]);
        });

        it("should send request that times out", async () => {
            const request: CommandRequest = { command: "ping" };
            const writeMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = { stdin: { write: writeMock } };
            (client as any).mErrHandler = (_err: Error) => {};
            const response = client.request(request);
            vi.runAllTimers();
            await expect(response).rejects.toMatchObject({ errorCode: "ETIMEDOUT" });
            expect(writeMock.mock.calls[0]).toEqual([`${JSON.stringify(rpcRequest)}\n`]);
        });

        it("should skip empty response lines", async () => {
            const request: CommandRequest = { command: "ping" };
            const fakeStdout = new EventEmitter();
            const sshStream = { stdin: { write: vi.fn() }, stdout: fakeStdout, stderr: { on: vi.fn() } };
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = sshStream;
            (client as any).getServerStatus(sshStream, readyMessage, "zowex server");

            const response = client.request(request);
            // Send response with empty lines
            fakeStdout.emit("data", `\n\n${JSON.stringify(rpcResponseGood)}\n`);

            expect(await response).toEqual({ success: true });
        });
    });

    describe("callbacks", () => {
        it("should receive ready message from Zowe server", async () => {
            const sshStream = { stderr: new EventEmitter(), stdout: new EventEmitter() };
            const onStderrMock = vi.spyOn(sshStream.stderr, "on");
            const onStdoutMock = vi.spyOn(sshStream.stdout, "on");
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshClient = {
                exec: function (_command: string, callback: ClientCallback) {
                    callback(undefined, sshStream as any);
                    sshStream.stdout.emit("data", readyMessage);
                    return this;
                },
            };
            await (client as any).execAsync();
            expect(onStderrMock).toHaveBeenCalledTimes(2);
            expect(onStdoutMock).toHaveBeenCalledTimes(2);
            expect(client.serverChecksums).not.toBeNull();
        });

        it("should handle not found error from Zowe server", async () => {
            const sshStream = { stderr: new EventEmitter(), stdout: new EventEmitter() };
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshClient = {
                exec: function (_command: string, callback: ClientCallback) {
                    callback(undefined, sshStream as any);
                    sshStream.stderr.emit("data", "FSUM7351 not found");
                    return this;
                },
            };
            await expect((client as any).execAsync()).rejects.toMatchObject({ errorCode: "ENOTFOUND" });
        });

        it("should handle startup error from Zowe server", async () => {
            const sshStream = { stderr: new EventEmitter(), stdout: new EventEmitter() };
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshClient = {
                exec: function (_command: string, callback: ClientCallback) {
                    callback(undefined, sshStream as any);
                    sshStream.stderr.emit("data", "bad json");
                    return this;
                },
            };
            await expect((client as any).execAsync()).rejects.toThrow("Error starting Zowe server");
        });

        it("should process stderr data from Zowe server", async () => {
            const request: CommandRequest = { command: "ping" };
            const fakeStderr = new EventEmitter();
            const sshStream = { stdin: { write: vi.fn() }, stdout: { on: vi.fn() }, stderr: fakeStderr };
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = sshStream;
            (client as any).getServerStatus(sshStream, readyMessage, "zowex server");
            const response = client.request(request);
            fakeStderr.emit("data", `${JSON.stringify(rpcResponseBad)}\n`);
            await expect(response).rejects.toMatchObject({ message: rpcResponseBad.error?.message });
        });

        it("should process stdout data from Zowe server", async () => {
            const request: CommandRequest = { command: "ping" };
            const fakeStdout = new EventEmitter();
            const sshStream = { stdin: { write: vi.fn() }, stdout: fakeStdout, stderr: { on: vi.fn() } };
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = sshStream;
            (client as any).getServerStatus(sshStream, readyMessage, "zowex server");
            const response = client.request(request);
            fakeStdout.emit("data", `${JSON.stringify(rpcResponseGood)}\n`);
            expect(await response).toEqual({ success: true });
        });

        it("should handle chdir error from Zowe server without throwing", async () => {
            const sshStream = { stderr: new EventEmitter(), stdout: new EventEmitter() };
            const onErrorMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mErrHandler = onErrorMock;
            (client as any).mSshClient = {
                exec: function (_command: string, callback: ClientCallback) {
                    callback(undefined, sshStream as any);
                    sshStream.stderr.emit("data", "FOTS1681 chdir error");
                    // Send ready message after the non-fatal error
                    sshStream.stdout.emit("data", readyMessage);
                    return this;
                },
            };

            await (client as any).execAsync();
            expect(onErrorMock).toHaveBeenCalledTimes(1);
            expect(onErrorMock.mock.calls[0][0]).toBeInstanceOf(Error);
        });

        it("should handle invalid response from Zowe server", async () => {
            const request: CommandRequest = { command: "ping" };
            const fakeStdout = new EventEmitter();
            const sshStream = { stdin: { write: vi.fn() }, stdout: fakeStdout, stderr: { on: vi.fn() } };
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mErrHandler = (err: Error) => {
                if (!err.toString().includes("Request timed out")) throw err; // we're expecting a request timed out error for the handler
            };
            (client as any).mSshStream = sshStream;
            (client as any).getServerStatus(sshStream, readyMessage, "zowex server");
            const response = client.request(request);
            expect(() => fakeStdout.emit("data", "bad json\n")).toThrow("Invalid JSON response");
            vi.runAllTimers();
            await expect(response).rejects.toBeDefined();
        });

        it("should handle unmapped response from Zowe server", async () => {
            const request: CommandRequest = { command: "ping" };
            const fakeStdout = new EventEmitter();
            const sshStream = { stdin: { write: vi.fn() }, stdout: fakeStdout, stderr: { on: vi.fn() } };
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mErrHandler = (err: Error) => {
                if (!err.toString().includes("Request timed out")) throw err; // we're expecting a request timed out error for the handler
            };
            (client as any).mSshStream = sshStream;
            (client as any).getServerStatus(sshStream, readyMessage, "zowex server");
            const response = client.request(request);
            expect(() => fakeStdout.emit("data", `${JSON.stringify({ ...rpcResponseGood, id: -1 })}\n`)).toThrow(
                "Missing promise for response ID",
            );
            vi.runAllTimers();
            await expect(response).rejects.toBeDefined();
        });

        it("should log message with default error handler", () => {
            const logErrorMock = vi.fn();
            vi.spyOn(Logger, "getAppLogger").mockReturnValue({
                error: logErrorMock,
            } as any);
            const testError = new Error("test error");
            (ZSshClient as any).defaultErrHandler(testError);
            expect(logErrorMock).toHaveBeenCalledTimes(1);
            expect(logErrorMock.mock.calls[0][0]).toBe(`Error: ${testError.message}`);
        });
    });

    describe("uss.issueCmd", () => {
        it("should send unixCommand request and return command output", async () => {
            const writeMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = { stdin: { write: writeMock } };

            const response = client.uss.issueCmd({ commandText: "whoami" });
            const rpcResponse: RpcResponse = {
                jsonrpc: "2.0",
                result: { success: true, data: "admin" },
                id: 1,
            };
            (client as any).processResponses(`${JSON.stringify(rpcResponse)}\n`);

            const result = await response;
            expect(result.success).toBe(true);
            expect(result.data).toBe("admin");

            const sentRequest = JSON.parse(writeMock.mock.calls[0][0]);
            expect(sentRequest.method).toBe("unixCommand");
            expect(sentRequest.params.commandText).toBe("whoami");
        });
    });

    describe("request with stream", () => {
        it("should register stream function when request contains stream", async () => {
            const mockStream = new Readable({ read() {} });
            const streamFn = () => mockStream;
            const request: CommandRequest & ReadableStreamRpc = {
                command: "ping",
                stream: streamFn,
            };
            const writeMock = vi.fn();
            const registerStreamMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mSshStream = { stdin: { write: writeMock } };
            (client as any).mStreamMgr = { registerStream: registerStreamMock };

            const response = client.request(request);

            const mappedRequest = (client as any).mRequestMap.get(1);
            expect(mappedRequest).toBeDefined();
            expect(mappedRequest.command).toBe(request);

            (client as any).processResponses(`${JSON.stringify(rpcResponseGood)}\n`);
            await response;

            expect(registerStreamMock).toHaveBeenCalledTimes(1);
            expect(registerStreamMock.mock.calls[0][1]).toBe(streamFn);
        });
    });

    describe("notification handling", () => {
        it("should handle RPC notifications", async () => {
            const notification = {
                jsonrpc: "2.0",
                method: "sendStream",
                params: { id: 1, pipePath: "/dev/null" },
            };
            const fakeStdout = new EventEmitter();
            const sshStream = { stdin: { write: vi.fn() }, stdout: fakeStdout, stderr: { on: vi.fn() } };
            const client: ZSshClient = new (ZSshClient as any)();
            const linkStreamToPromiseMock = vi.fn();
            (client as any).mStreamMgr = { linkStreamToPromise: linkStreamToPromiseMock };
            (client as any).mSshStream = sshStream;
            (client as any).getServerStatus(sshStream, readyMessage, "zowex server");

            (client as any).mRequestMap.set(1, {
                rpc: { resolve: vi.fn(), reject: vi.fn() },
                command: { command: "test" },
                silenced: false,
            });

            fakeStdout.emit("data", `${JSON.stringify(notification)}\n`);

            expect(linkStreamToPromiseMock).toHaveBeenCalledTimes(1);
            expect(linkStreamToPromiseMock.mock.calls[0][1]).toEqual(notification);
        });

        it("should handle errors in notification processing", async () => {
            const notification = {
                jsonrpc: "2.0",
                method: "invalidMethod",
                params: { id: 1 },
            };
            const fakeStdout = new EventEmitter();
            const sshStream = { stdin: { write: vi.fn() }, stdout: fakeStdout, stderr: { on: vi.fn() } };
            const onErrorMock = vi.fn();
            const client: ZSshClient = new (ZSshClient as any)();
            (client as any).mErrHandler = onErrorMock;
            (client as any).mSshStream = sshStream;
            (client as any).getServerStatus(sshStream, readyMessage, "zowex server");

            (client as any).mRequestMap.set(1, {
                rpc: { resolve: vi.fn(), reject: vi.fn() },
                command: { command: "test" },
                silenced: false,
            });

            fakeStdout.emit("data", `${JSON.stringify(notification)}\n`);

            expect(onErrorMock).toHaveBeenCalledTimes(1);
            expect(onErrorMock.mock.calls[0][0].message).toContain("unknown method invalidMethod");
        });
    });

    describe("collectAllRequests function", () => {
        it("should harvest requests, apply silence flag, and clear timeouts", () => {
            const client: ZSshClient = new (ZSshClient as any)();
            const clearTimeoutSpy = vi.spyOn(globalThis, "clearTimeout");

            const timeout1 = setTimeout(() => {}, 10000);
            const timeout2 = setTimeout(() => {}, 10000);

            const req1 = {
                command: { command: "ping" },
                rpc: { resolve: vi.fn(), reject: vi.fn() },
                silenced: false,
                timeoutId: timeout1,
            };
            const req2 = {
                command: { command: "echo" },
                rpc: { resolve: vi.fn(), reject: vi.fn() },
                silenced: false,
                timeoutId: timeout2,
            };

            (client as any).mRequestMap.set(1, req1);
            (client as any).mRequestMap.set(2, req2);

            const harvested = client.collectAllRequests(true);
            expect(harvested.size).toBe(2);
            expect(harvested.has(req1 as any)).toBe(true);
            expect(req1.silenced).toBe(true);
            expect(req2.silenced).toBe(true);
            expect(clearTimeoutSpy).toHaveBeenCalledWith(timeout1);
            expect(clearTimeoutSpy).toHaveBeenCalledWith(timeout2);
        });
    });

    describe("dispose function", () => {
        it("should reject open requests with standard message on normal dispose", () => {
            const client: ZSshClient = new (ZSshClient as any)();
            const rejectMock = vi.fn();
            (client as any).mSshClient = { end: vi.fn() };
            (client as any).mRequestMap.set(1, { rpc: { resolve: vi.fn(), reject: rejectMock } });

            // Default dispose: isRestart = false, closeOpenRequests = true
            client.dispose();

            expect(rejectMock).toHaveBeenCalledTimes(1);
            expect(rejectMock.mock.calls[0][0].message).toContain("Shutting down ZRS. No action is required.");
            expect((client as any).mRequestMap.size).toBe(0);
        });

        it("should reject open requests with restart message when isRestart is true", () => {
            const client: ZSshClient = new (ZSshClient as any)();
            const rejectMock = vi.fn();
            (client as any).mSshClient = { end: vi.fn() };
            (client as any).mRequestMap.set(1, { rpc: { resolve: vi.fn(), reject: rejectMock } });

            // Restarting, not retrying requests
            client.dispose(true);

            expect(rejectMock).toHaveBeenCalledTimes(1);
            expect(rejectMock.mock.calls[0][0].message).toContain("Restarting ZRS");
            expect((client as any).mRequestMap.size).toBe(0);
        });

        it("should NOT reject open requests when requests are silenced", () => {
            const client: ZSshClient = new (ZSshClient as any)();
            const rejectMock = vi.fn();
            (client as any).mSshClient = { end: vi.fn() };
            (client as any).mRequestMap.set(1, { rpc: { resolve: vi.fn(), reject: rejectMock }, silenced: true });

            client.dispose(true);

            expect(rejectMock).not.toHaveBeenCalled();
            expect((client as any).mRequestMap.size).toBe(0);
        });

        it("should end SSH connection", () => {
            const client: ZSshClient = new (ZSshClient as any)();
            const endMock = vi.fn();
            (client as any).mSshClient = { end: endMock };
            client[Symbol.dispose]();
            expect(endMock).toHaveBeenCalledTimes(1);
        });
    });

    describe("handleResponse with silenced requests", () => {
        it("should silently delete the request and ignore the response if silenced is true", () => {
            const client: ZSshClient = new (ZSshClient as any)();
            const resolveMock = vi.fn();
            const rejectMock = vi.fn();

            // Seed a silenced request
            (client as any).mRequestMap.set(99, {
                command: { command: "ping" },
                rpc: { resolve: resolveMock, reject: rejectMock },
                silenced: true,
            });

            const fakeResponse: RpcResponse = { jsonrpc: "2.0", id: 99, result: { success: true } };

            (client as any).handleResponse(fakeResponse);

            // Promise handlers should NOT have been called
            expect(resolveMock).not.toHaveBeenCalled();
            expect(rejectMock).not.toHaveBeenCalled();

            expect((client as any).mRequestMap.has(99)).toBe(false);
        });
    });

    describe("create function (Replay Requests)", () => {
        it("should automatically replay harvested requests upon creation", async () => {
            const fakeResolve = vi.fn();
            const fakeReject = vi.fn();
            const harvestedRequest = {
                command: { command: "harvested-command" },
                rpc: { resolve: fakeResolve, reject: fakeReject },
                silenced: true,
            };

            const requestSpy = vi.spyOn(ZSshClient.prototype, "request").mockResolvedValue({ success: true } as any);

            vi.spyOn(Client.prototype, "connect").mockImplementation(function () {
                this.emit("ready");
                return this;
            });
            vi.spyOn(ZSshClient.prototype as any, "execAsync").mockResolvedValue(new EventEmitter());

            await ZSshClient.create(new SshSession(fakeSession), {
                requests: new Set([harvestedRequest] as any),
            });

            expect(requestSpy).toHaveBeenCalledWith(harvestedRequest.command);

            expect(fakeResolve).toHaveBeenCalledWith({ success: true });
            expect(fakeReject).not.toHaveBeenCalled();
        });
    });
});

describe("ZSshUtils", () => {
    describe("uninstall server", () => {
        const serverPath = "/faketmp/fakeserver";
        const fakeSshSession = new SshSession(fakeSession);
        it("should return code 0 when uninstalling server succeeds", async () => {
            const fakeLogger = { debug: vi.fn(), error: vi.fn() };
            vi.spyOn(Logger, "getAppLogger").mockReturnValue(fakeLogger as any);
            vi.spyOn(ZSshUtils as any, "sftp").mockImplementation(
                async (_session: any, callback: (sftp: any, ssh: any) => Promise<void>) => {
                    const sshMock = {
                        execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "deleted", stderr: "" }),
                    };
                    return callback({}, sshMock);
                },
            );
            await ZSshUtils.uninstallServer(fakeSshSession, serverPath);
            expect(fakeLogger.debug).toHaveBeenCalledWith(`Deleted directory ${serverPath} with response: deleted`);
            expect(fakeLogger.error).not.toHaveBeenCalled();
        });

        it("should return code 1 when uninstalling server fails", async () => {
            const fakeLogger = { debug: vi.fn(), error: vi.fn() };
            vi.spyOn(Logger, "getAppLogger").mockReturnValue(fakeLogger as any);
            vi.spyOn(ZSshUtils as any, "sftp").mockImplementation(
                async (_session: any, callback: (sftp: any, ssh: any) => Promise<void>) => {
                    const sshMock = {
                        execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: "" }),
                    };
                    return callback({}, sshMock);
                },
            );

            await expect(ZSshUtils.uninstallServer(fakeSshSession, serverPath)).rejects.toThrow();
            expect(fakeLogger.error).toHaveBeenCalled();
        });
    });

    describe("isPrivateKeyAuthFailure", () => {
        it("should return true for common private key failure patterns", () => {
            const testCases = [
                "All configured authentication methods failed",
                "Cannot parse privateKey: Malformed OpenSSH private key",
                "but no passphrase given",
                "integrity check failed",
                "Permission denied (publickey,password)",
                "Permission denied (publickey)",
                "Authentication failed",
                "Invalid private key",
                "privateKey value does not contain a (valid) private key",
                "Cannot parse privateKey",
            ];

            testCases.forEach((errorMessage) => {
                expect(ZSshUtils.isPrivateKeyAuthFailure(errorMessage, true)).toBe(true);
            });
        });

        it("should return false for non-private key error messages", () => {
            const testCases = [
                "Connection timed out",
                "Network is unreachable",
                "Connection refused",
                "Host key verification failed",
                "Some other random error",
            ];

            testCases.forEach((errorMessage) => {
                expect(ZSshUtils.isPrivateKeyAuthFailure(errorMessage, true)).toBe(false);
            });
        });

        it("should return false when hasPrivateKey is explicitly false", () => {
            const privateKeyErrorMessage = "All configured authentication methods failed";
            expect(ZSshUtils.isPrivateKeyAuthFailure(privateKeyErrorMessage, false)).toBe(false);
        });

        it("should return true when hasPrivateKey is undefined but error matches pattern", () => {
            const privateKeyErrorMessage = "All configured authentication methods failed";
            expect(ZSshUtils.isPrivateKeyAuthFailure(privateKeyErrorMessage)).toBe(true);
        });

        it("should handle partial matches in error messages", () => {
            const errorMessage =
                "SSH Error: All configured authentication methods failed. Please check your credentials.";
            expect(ZSshUtils.isPrivateKeyAuthFailure(errorMessage, true)).toBe(true);
        });
    });
});
