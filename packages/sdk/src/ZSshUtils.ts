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
import { isEqual } from "es-toolkit";
import { NodeSSH, type Config as NodeSSHConfig } from "node-ssh";
import type { ConnectConfig, SFTPWrapper } from "ssh2";
import { PrivateKeyFailurePatterns, SshErrors } from "./SshErrors";

export interface ISshCallbacks {
    onProgress?: (increment: number) => void; // Callback to report incremental progress
    onError?: (error: Error, context: string) => Promise<boolean>; // Callback to handle errors, returns true to continue/retry
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
     * Formats a detailed message for error returned by an SFTP operation.
     */
    private static formatSftpErrorMessage(err: SftpError, prefix: string): string {
        const codePart = err.code == null ? "" : ` RC=${err.code}`;
        return `${prefix}${codePart}: ${err}`;
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
            Logger.getAppLogger().info(`[ZSshUtils] Step 1/5: Creating remote directory ${remoteDir}`);
            const mkdirResult = await ssh.execCommand(`mkdir -p ${remoteDir}`);
            if (await ZSshUtils.routeExpiredPasswordError(mkdirResult.stderr ?? "", "deploy", options)) return false;
            if (mkdirResult.code !== 0) {
                const details = `mkdir -p ${remoteDir} RC=${mkdirResult.code}: ${mkdirResult.stderr}`;
                Logger.getAppLogger().error(`[ZSshUtils] Step 1 FAILED: ${details}`);
                const mkdirErr = new ImperativeError({
                    msg: "Failed to create the server directory on the remote system.",
                    errorCode: "EDEPLOYFAIL",
                    additionalDetails: details,
                });
                if (options?.onError) {
                    const shouldRetry = await options.onError(mkdirErr, "deploy");
                    if (!shouldRetry) {
                        return false;
                    }
                    return ZSshUtils.installServer(session, serverPath, options);
                }
                return false;
            }

            // Some implementations of SFTP for z/OS default to text mode and require explicitly switching to binary mode
            Logger.getAppLogger().info(`[ZSshUtils] Step 2/5: Verifying that binary mode is enabled for SFTP`);
            try {
                await promisify(sftp.readdir.bind(sftp))("/+mode=binary");
            } catch (err) {
                if (await ZSshUtils.routeExpiredPasswordError(String(err), "binary", options)) return false;
                const details = ZSshUtils.formatSftpErrorMessage(err, "Set mode=binary");
                Logger.getAppLogger().info(
                    `[ZSshUtils] Step 2 INFO: switch to binary mode failed (may be unsupported by server): ${details}`,
                );
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
                `[ZSshUtils] Step 3/5: Uploading ${ZSshUtils.SERVER_PAX_FILE} to ${remotePaxPath}`,
            );
            try {
                await promisify(sftp.fastPut.bind(sftp))(localPaxPath, remotePaxPath, { step: progressCallback });
            } catch (err) {
                if (await ZSshUtils.routeExpiredPasswordError(String(err), "upload", options)) return false;
                const details = ZSshUtils.formatSftpErrorMessage(err, `Upload ${ZSshUtils.SERVER_PAX_FILE}`);
                Logger.getAppLogger().error(`[ZSshUtils] Step 3 FAILED: ${details}`);
                const uploadErr = new ImperativeError({
                    msg: "Failed to upload the server binary to the remote system.",
                    errorCode: "EDEPLOYFAIL",
                    additionalDetails: details,
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

            Logger.getAppLogger().info(`[ZSshUtils] Step 4/5: Extracting PAX archive in ${remoteDir}`);
            const paxResult = await ssh.execCommand(`pax -rzf ${ZSshUtils.SERVER_PAX_FILE}`, { cwd: remoteDir });
            if (await ZSshUtils.routeExpiredPasswordError(paxResult.stderr ?? "", "extract", options)) return false;
            if (paxResult.code === 0) {
                Logger.getAppLogger().info(`[ZSshUtils] Step 4 OK: Extracted server binaries`);
            } else {
                const details = `pax -rzf RC=${paxResult.code}: ${paxResult.stderr}`;
                Logger.getAppLogger().error(`[ZSshUtils] Step 4 FAILED: ${details}`);
                const paxErr = new ImperativeError({
                    msg: "Failed to extract the server archive on the remote system.",
                    errorCode: "EDEPLOYFAIL",
                    additionalDetails: details,
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

            Logger.getAppLogger().info(`[ZSshUtils] Step 5/5: Cleaning up ${remotePaxPath}`);
            try {
                await promisify(sftp.unlink.bind(sftp))(remotePaxPath);
            } catch (err) {
                const details = ZSshUtils.formatSftpErrorMessage(err, `Delete ${remotePaxPath}`);
                Logger.getAppLogger().warn(`[ZSshUtils] Step 5 WARNING: cleanup failed: ${details}`);
                const cleanupErr = new ImperativeError({
                    msg: "Failed to clean up the upload archive on the remote system.",
                    errorCode: "ECLEANUPFAIL",
                    additionalDetails: details,
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
                const details = `rm -rf ${serverPath} RC=${result.code}: ${result.stderr}`;
                Logger.getAppLogger().error(`[ZSshUtils] Uninstall FAILED: ${details}`);
                const rmErr = new ImperativeError({
                    msg: "Failed to uninstall the server from the remote system.",
                    errorCode: "EUNINSTALLFAIL",
                    additionalDetails: details,
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

    public static async checkIfOutdated(remoteChecksums?: Record<string, string>): Promise<boolean> {
        if (remoteChecksums == null) {
            Logger.getAppLogger().warn("Checksums not found, could not verify server");
            return false;
        }
        const localFile = path.join(ZSshUtils.getBinDir(__dirname), "checksums.asc");
        const localChecksums: Record<string, string> = {};
        for (const line of fs.readFileSync(localFile, "utf-8").trimEnd().split("\n")) {
            const [checksum, file] = line.split(/\s+/);
            localChecksums[file] = checksum;
        }
        return !isEqual(localChecksums, remoteChecksums);
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
}
