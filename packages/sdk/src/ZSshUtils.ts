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

import * as fs from "node:fs";
import * as path from "node:path";
import { promisify } from "node:util";
import { ImperativeError, type IProfile, Logger } from "@zowe/imperative";
import { type ISshSession, SshSession } from "@zowe/zos-uss-for-zowe-sdk";
import { NodeSSH, type Config as NodeSSHConfig } from "node-ssh";
import * as semver from "semver";
import type { ConnectConfig, SFTPWrapper } from "ssh2";
import { PrivateKeyFailurePatterns, SshErrors } from "./SshErrors";
import { ZSshClient } from "./ZSshClient";
import { BUNDLED_SSH_SERVER_VERSION } from "./ZSshConstants";

export interface ISshCallbacks {
    onProgress?: (increment: number) => void; // Callback to report incremental progress
    onError?: (error: Error, context: string) => Promise<boolean>; // Callback to handle errors, returns true to continue/retry
}

export interface IServerOnPathDetails {
    /**
     * The absolute path of the zowex instance on the user's path, if any was detected.
     */
    serverPath?: string;
    /**
     * If serverPath is defined, this will be true if the user is able to execute the program
     * found at serverPath.
     */
    hasExecutePermission: boolean;
    /**
     * If serverPath is defined, this will be the version reported by the output of the
     * server binary's -v flag.
     */
    version?: string;
}
type SftpError = Error & { code?: number };

// biome-ignore lint/complexity/noStaticOnlyClass: Utilities class has static methods
export class ZSshUtils {
    private static readonly SERVER_PAX_FILE = "server.pax.Z";
    private static readonly EXPIRED_PASSWORD_CODES = ["FOTS1668", "FOTS1669"];

    /**
     * Routes an expired-password error through `onError` when a callback is registered,
     * or throws an `ImperativeError` directly as a fallback (preserving CLI behavior).
     * Returns `true` if the text contained an expired-password indicator (i.e. the caller
     * should abort the current operation), `false` otherwise.
     */
    private static async routeExpiredPasswordError(
        text: string,
        context: string,
        options?: Pick<ISshCallbacks, "onError">,
    ): Promise<boolean> {
        if (!ZSshUtils.EXPIRED_PASSWORD_CODES.some((code) => text.includes(code))) {
            return false;
        }
        const err = new ImperativeError({
            msg: SshErrors.FOTS1668.summary,
            errorCode: "EPASSWD_EXPIRED",
            additionalDetails: text,
        });
        if (options?.onError) {
            await options.onError(err, context);
            return true;
        }
        throw err;
    }

    /**
     * Checks if an error message indicates a private key authentication failure
     * @param errorMessage The error message to check
     * @param hasPrivateKey Optional flag to indicate if a private key is configured (for more accurate detection)
     * @returns True if the error indicates a private key authentication failure
     */
    public static isPrivateKeyAuthFailure(errorMessage: string, hasPrivateKey?: boolean): boolean {
        // If no private key is configured, this can't be a private key failure
        if (hasPrivateKey === false) {
            return false;
        }

        return PrivateKeyFailurePatterns.some((pattern) => errorMessage.includes(pattern));
    }

    public static buildSession(args: IProfile): SshSession {
        const sshSessCfg: ISshSession = {
            hostname: args.host,
            port: args.port ?? 22,
            user: args.user,
            privateKey: args.privateKey,
            keyPassphrase: args.privateKey ? args.keyPassphrase : undefined,
            password: args.privateKey ? undefined : args.password,
            handshakeTimeout: args.handshakeTimeout,
        };
        return new SshSession(sshSessCfg);
    }

    public static buildSshConfig(session: SshSession, configProps?: ConnectConfig): ConnectConfig {
        return {
            host: session.ISshSession.hostname,
            port: session.ISshSession.port,
            username: session.ISshSession.user,
            password: session.ISshSession.password,
            privateKey: session.ISshSession.privateKey
                ? fs.readFileSync(session.ISshSession.privateKey, "utf-8")
                : undefined,
            passphrase: session.ISshSession.keyPassphrase,
            readyTimeout: session.ISshSession.handshakeTimeout,
            // ssh2 debug messages are extremely verbose so log at TRACE level
            debug: (msg) => Logger.getAppLogger().trace(msg),
            ...configProps,
        };
    }

    /**
     * Check the user's $PATH for our server binary.
     * @param session Pre-established SSH session
     * @returns object describing the details of any located zowex program
     */
    public static async detectServerOnPath(session: SshSession): Promise<IServerOnPathDetails> {
        Logger.getAppLogger().debug(`[ZSshUtils] enter detectServerOnPath()`);
        return ZSshUtils.withSsh(session, async (ssh) => {
            const details: IServerOnPathDetails = {
                serverPath: undefined,
                hasExecutePermission: false,
                version: undefined,
            };
            const notFoundMessage = "Zowe binary not found";

            let commandVOutput = "";

            const findBinInOutput = () => {
                let foundBin: string | undefined;
                commandVOutput.split("\n").forEach((line: string) => {
                    if (line.trim().endsWith(ZSshClient.BIN_NAME)) {
                        // the output may be prefixed with unprintable characters such as \u001b,
                        // so consider the bin path to start with the first '/'
                        const firstSlashIndex = line.indexOf("/");
                        if (firstSlashIndex >= 0) {
                            foundBin = line.substring(firstSlashIndex).trim();
                        }
                    }
                });
                return foundBin;
            };
            const findNotFoundMessageInOutput = () => {
                let foundMsg: string | undefined;
                commandVOutput.split("\n").forEach((line: string) => {
                    if (line.trim().endsWith(notFoundMessage)) {
                        foundMsg = line.trim();
                    }
                });
                return foundMsg;
            };
            await ssh.withShell((shellChannel) => {
                return new Promise((resolve, reject) => {
                    shellChannel.on("error", reject);
                    shellChannel.on("exit", () => {
                        Logger.getAppLogger().debug(
                            `[ZSshUtils] detectServerOnPath(): SSH shell exited. Checking shell stdout...`,
                        );

                        if (shellChannel.stdout.readableLength > 0) {
                            commandVOutput += shellChannel.stdout.read().toString();
                            Logger.getAppLogger().debug(`[ZSshUtils] Final output: '${commandVOutput}'..`);
                        }
                        resolve();
                    });
                    shellChannel.on("data", (output: string | Buffer) => {
                        commandVOutput += output.toString();
                        Logger.getAppLogger().debug(`[ZSshUtils] Received command -v output: '${output}'..`);
                        if (
                            shellChannel.closed ||
                            shellChannel.stdout.readableEnded ||
                            findBinInOutput() ||
                            findNotFoundMessageInOutput()
                        ) {
                            resolve();
                        }
                    });
                    shellChannel.write(`(command -v ${ZSshClient.BIN_NAME} || echo ${notFoundMessage}) && exit\n`);
                    shellChannel.end();
                });
            });
            Logger.getAppLogger().debug(
                `[ZSshUtils] Returned from shell executing 'command -v ${ZSshClient.BIN_NAME}'`,
            );

            const foundBin = findBinInOutput();
            if (foundBin) {
                details.serverPath = foundBin;
                Logger.getAppLogger().info(`[ZSshUtils] Found zowex executable at ${details.serverPath}`);
                const testExecuteCmd = await ssh.execCommand(`${details.serverPath} -v`);
                details.hasExecutePermission = testExecuteCmd.code === 0;
                details.version = details.hasExecutePermission ? testExecuteCmd.stdout.trim() : undefined;
            } else {
                Logger.getAppLogger().info(
                    `[ZSshUtils] Did not detect any ${ZSshClient.BIN_NAME} program on the user's PATH.`,
                );
            }

            return details;
        });
    }

    /**
     * A value of `true` will prevent us from trying to deploy the server.
     * @param session Pre-established SSH session
     * @param path The path to test for write access
     * @returns A promise resolving to true if the user is denied write access.
     *          If the path does not exist, false will be returned.
     */
    public static async lacksWriteAccess(session: SshSession, testPath: string): Promise<boolean> {
        return ZSshUtils.withSsh(session, async (ssh) => {
            Logger.getAppLogger().info(`[ZSshUtils] Testing lacksWriteAccess to path '%s'`, testPath);

            // See: https://www.man7.org/linux/man-pages/man1/test.1.html
            const testExistsCmd = await ssh.execCommand(`test -e ${testPath}`);
            Logger.getAppLogger().debug(
                `[ZSshUtils] test -e %s, code %d, stdout: '%s', stderr: '%s'`,
                testPath,
                testExistsCmd.code,
                testExistsCmd.stdout,
                testExistsCmd.stderr,
            );
            const testWriteCmd = await ssh.execCommand(`test -w ${testPath}`);
            Logger.getAppLogger().debug(
                `[ZSshUtils] test -w %s, code %d, stdout: '%s', stderr: '%s'`,
                testPath,
                testWriteCmd.code,
                testWriteCmd.stdout,
                testWriteCmd.stderr,
            );

            return (
                testExistsCmd.code === 0 && // 0 : the file exists
                testWriteCmd.code !== 0
            ); // non-zero: lacks access
        });
    }

    public static async installServer(
        session: SshSession,
        serverPath: string,
        options?: ISshCallbacks,
    ): Promise<boolean> {
        Logger.getAppLogger().info(
            `[ZSshUtils] installServer to ${session.ISshSession.hostname} at path: ${serverPath}`,
        );
        const localDir = ZSshUtils.getBinDir(__dirname);
        const remoteDir = serverPath.replace(/^~/, ".");

        return ZSshUtils.sftp(session, async (sftp, ssh) => {
            Logger.getAppLogger().info(`[ZSshUtils] Step 1/4: Creating remote directory ${remoteDir}`);
            const execReturn = await ssh.execCommand(`mkdir -p ${remoteDir}`);
            if (await ZSshUtils.routeExpiredPasswordError(execReturn.stderr ?? "", "deploy", options)) return false;
            if (execReturn.code !== 0) {
                const technical = `mkdir -p ${remoteDir} RC=${execReturn.code}: ${execReturn.stderr}`;
                Logger.getAppLogger().error(`[ZSshUtils] Step 1 FAILED: ${technical}`);
                const err = new ImperativeError({
                    msg: "Failed to create the server directory on the remote system.",
                    errorCode: "EDEPLOYFAIL",
                    additionalDetails: technical,
                });
                if (options?.onError) {
                    const shouldRetry = await options.onError(err, "deploy");
                    if (!shouldRetry) {
                        return false;
                    }
                    return ZSshUtils.installServer(session, serverPath, options);
                }
                return false;
            }

            const localPaxPath = path.join(localDir, ZSshUtils.SERVER_PAX_FILE);
            const remotePaxPath = path.posix.join(remoteDir, ZSshUtils.SERVER_PAX_FILE);

            let previousPercentage = 0;
            const progressCallback = options?.onProgress
                ? (progress: number, _chunk: number, total: number) => {
                      const percentage = Math.floor((progress / total) * 100);
                      const increment = percentage - previousPercentage;
                      if (increment > 0) {
                          options.onProgress!(increment);
                          previousPercentage = percentage;
                      }
                  }
                : undefined;

            Logger.getAppLogger().info(
                `[ZSshUtils] Step 2/4: Uploading ${ZSshUtils.SERVER_PAX_FILE} to ${remotePaxPath}`,
            );
            try {
                await promisify(sftp.fastPut.bind(sftp))(localPaxPath, remotePaxPath, { step: progressCallback });
            } catch (err) {
                if (await ZSshUtils.routeExpiredPasswordError(String(err), "upload", options)) return false;
                const codePart = (err as SftpError).code == null ? "" : ` RC=${(err as SftpError).code}`;
                const technical = `Upload ${ZSshUtils.SERVER_PAX_FILE}${codePart}: ${err}`;
                Logger.getAppLogger().error(`[ZSshUtils] Step 2 FAILED: ${technical}`);
                const uploadErr = new ImperativeError({
                    msg: "Failed to upload the server binary to the remote system.",
                    errorCode: "EDEPLOYFAIL",
                    additionalDetails: technical,
                });
                if (options?.onError) {
                    const shouldRetry = await options.onError(uploadErr, "upload");
                    if (!shouldRetry) {
                        return false;
                    }
                    return ZSshUtils.installServer(session, serverPath, options);
                }
                return false;
            }

            Logger.getAppLogger().info(`[ZSshUtils] Step 3/4: Extracting PAX archive in ${remoteDir}`);
            const result = await ssh.execCommand(`pax -rzf ${ZSshUtils.SERVER_PAX_FILE}`, { cwd: remoteDir });
            if (await ZSshUtils.routeExpiredPasswordError(result.stderr ?? "", "extract", options)) return false;
            if (result.code === 0) {
                Logger.getAppLogger().info(`[ZSshUtils] Step 3 OK: Extracted server binaries`);
            } else {
                const technical = `pax -rzf RC=${result.code}: ${result.stderr}`;
                Logger.getAppLogger().error(`[ZSshUtils] Step 3 FAILED: ${technical}`);
                const paxErr = new ImperativeError({
                    msg: "Failed to extract the server archive on the remote system.",
                    errorCode: "EDEPLOYFAIL",
                    additionalDetails: technical,
                });
                if (options?.onError) {
                    const shouldContinue = await options.onError(paxErr, "extract");
                    if (!shouldContinue) {
                        return false;
                    }
                } else {
                    return false;
                }
            }

            Logger.getAppLogger().info(`[ZSshUtils] Step 4/4: Cleaning up ${remotePaxPath}`);
            try {
                await promisify(sftp.unlink.bind(sftp))(remotePaxPath);
            } catch (err) {
                const technical = `${err}`;
                Logger.getAppLogger().warn(`[ZSshUtils] Step 4 WARNING: cleanup failed: ${technical}`);
                const cleanupErr = new ImperativeError({
                    msg: "Failed to clean up the upload archive on the remote system.",
                    errorCode: "ECLEANUPFAIL",
                    additionalDetails: technical,
                });
                if (options?.onError) {
                    await options.onError(cleanupErr, "cleanup");
                } else {
                    Logger.getAppLogger().debug("Cleanup error is non-fatal, continuing...");
                }
            }
            Logger.getAppLogger().info("[ZSshUtils] installServer completed successfully");
            return true;
        });
    }

    public static async uninstallServer(
        session: SshSession,
        serverPath: string,
        options?: Omit<ISshCallbacks, "onProgress">,
    ): Promise<void> {
        Logger.getAppLogger().debug(`Uninstalling server from ${session.ISshSession.hostname} at path: ${serverPath}`);
        return ZSshUtils.sftp(session, async (_sftp, ssh) => {
            const result = await ssh.execCommand(`rm -rf ${serverPath}`);
            if (await ZSshUtils.routeExpiredPasswordError(result.stderr ?? "", "unlink", options)) return;
            if (result.code === 0) {
                Logger.getAppLogger().debug(`Deleted directory ${serverPath} with response: ${result.stdout}`);
            } else {
                const technical = `rm -rf ${serverPath} RC=${result.code}: ${result.stderr}`;
                Logger.getAppLogger().error(`[ZSshUtils] Uninstall FAILED: ${technical}`);
                const rmErr = new ImperativeError({
                    msg: "Failed to uninstall the server from the remote system.",
                    errorCode: "EUNINSTALLFAIL",
                    additionalDetails: technical,
                });
                if (options?.onError) {
                    const shouldContinue = await options.onError(rmErr, "unlink");
                    if (!shouldContinue) {
                        throw rmErr;
                    }
                    return ZSshUtils.uninstallServer(session, serverPath, options);
                }
                throw rmErr;
            }
        });
    }

    public static async checkIfOutdated(remoteVersion: string): Promise<boolean> {
        Logger.getAppLogger().debug(
            `[ZSshUtils] checkIfOutdated: Comparing remote version '${remoteVersion}' to bundled server version '${BUNDLED_SSH_SERVER_VERSION}'`,
        );
        if (remoteVersion) {
            // older builds of zowex were suffixed with +<gitHash>, but + will not be treated as
            // a prerelease string. So, normalize + to - for consistent behavior.
            remoteVersion = remoteVersion.replace("+", "-");
        }
        if (!semver.valid(remoteVersion)) {
            Logger.getAppLogger().warn(
                `[ZSshUtils] checkIfOutdated: Invalid remote version '${remoteVersion}' passed. Assuming outdated by default`,
            );
            return true;
        }

        const isOutdated = semver.lt(remoteVersion, BUNDLED_SSH_SERVER_VERSION);
        Logger.getAppLogger().debug(`[ZSshUtils] remote version '${remoteVersion}' is outdated? ${isOutdated}`);
        return isOutdated;
    }

    private static getBinDir(dir: string): string {
        if (!dir || fs.existsSync(path.join(dir, "package.json"))) {
            return path.join(dir, "bin");
        }

        const dirUp = path.normalize(path.join(dir, ".."));
        if (path.parse(dirUp).base.length > 0) {
            return ZSshUtils.getBinDir(dirUp);
        }

        throw new Error(`Failed to find bin directory in path ${dir}`);
    }

    private static async sftp<T>(
        session: SshSession,
        callback: (sftp: SFTPWrapper, ssh: NodeSSH) => Promise<T>,
    ): Promise<T> {
        const ssh = new NodeSSH();
        await ssh.connect(ZSshUtils.buildSshConfig(session) as NodeSSHConfig);
        try {
            return await ssh.requestSFTP().then((sftp) => callback(sftp, ssh));
        } finally {
            ssh.dispose();
        }
    }

    private static async withSsh<T>(session: SshSession, callback: (ssh: NodeSSH) => Promise<T>): Promise<T> {
        const ssh = new NodeSSH();
        await ssh.connect(ZSshUtils.buildSshConfig(session) as NodeSSHConfig);
        try {
            return await callback(ssh);
        } finally {
            ssh.dispose();
        }
    }
}
