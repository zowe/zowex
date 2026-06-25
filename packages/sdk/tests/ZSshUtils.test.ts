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
import { Logger } from "@zowe/imperative";
import { type ISshSession, SshSession } from "@zowe/zos-uss-for-zowe-sdk";
import { NodeSSH } from "node-ssh";
import { ZSshClient } from "../src";
import { SshErrors } from "../src/SshErrors";
import { ZSshUtils } from "../src/ZSshUtils";

vi.mock("node:fs", { spy: true });

function setupSftpMocks(
    sftpMock: object,
    sshMock: { execCommand: ReturnType<typeof vi.fn>; withShell?: ReturnType<typeof vi.fn> },
    options?: { mockGetBinDir?: boolean; mockExistsSync?: boolean },
): void {
    vi.spyOn(ZSshUtils as any, "sftp").mockImplementation(
        async (_session: any, callback: (sftp: any, ssh: any) => Promise<any>) => callback(sftpMock, sshMock),
    );
    if (options?.mockGetBinDir !== false) {
        vi.spyOn(ZSshUtils as any, "getBinDir").mockReturnValue("/fake/bin");
    }
    if (options?.mockExistsSync !== false) {
        vi.spyOn(fs, "existsSync").mockReturnValue(true);
    }
}

describe("ZSshUtils", () => {
    describe("checkIfOutdated", () => {
        it("should return false for dev builds without remote checksums", async () => {
            const readFileSyncSpy = vi.spyOn(fs, "readFileSync");
            const isOutdated = await ZSshUtils.checkIfOutdated();
            expect(isOutdated).toBe(false);
            expect(readFileSyncSpy).not.toHaveBeenCalled();
        });

        it.each([
            {
                desc: "matching checksums with different order",
                fileContent: "123 abc\n456 def",
                remote: { def: "456", abc: "123" },
                expected: false,
            },
            {
                desc: "different checksums",
                fileContent: "123 abc\n456 def",
                remote: { abc: "789", def: "456" },
                expected: true,
            },
            {
                desc: "same checksums and local file added",
                fileContent: "123 abc\n456 def\n789 ghi",
                remote: { abc: "123", def: "456" },
                expected: true,
            },
            {
                desc: "same checksums and local file removed",
                fileContent: "123 abc",
                remote: { abc: "123", def: "456" },
                expected: true,
            },
        ])("should return $expected for $desc", async ({ fileContent, remote, expected }) => {
            const readFileSyncSpy = vi.spyOn(fs, "readFileSync").mockReturnValueOnce(fileContent);
            const isOutdated = await ZSshUtils.checkIfOutdated(remote);
            expect(isOutdated).toBe(expected);
            expect(readFileSyncSpy).toHaveBeenCalled();
        });
    });

    describe("installServer / uninstallServer", () => {
        const fakeSession: ISshSession = {
            hostname: "example.com",
            port: 22,
            user: "admin",
            password: "pass",
        };

        beforeEach(() => {
            vi.spyOn(Logger, "getAppLogger").mockReturnValue({
                debug: vi.fn(),
                error: vi.fn(),
                warn: vi.fn(),
                info: vi.fn(),
                trace: vi.fn(),
            } as any);
        });

        it("should upload via SFTP and extract server", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const unlinkMock = vi.fn((_path: string, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock, unlink: unlinkMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server");
            expect(result).toBe(true);
            expect(fastPutMock).toHaveBeenCalledTimes(1);
            expect(unlinkMock).toHaveBeenCalledTimes(1);
        });

        it("should report progress during SFTP upload", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, opts: any, cb: (err?: Error) => void) => {
                if (opts?.step) {
                    opts.step(50, 50, 100);
                    opts.step(100, 50, 100);
                }
                cb();
            });
            const unlinkMock = vi.fn((_path: string, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock, unlink: unlinkMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const progressMock = vi.fn();
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", {
                onProgress: progressMock,
            });
            expect(result).toBe(true);
            expect(progressMock).toHaveBeenCalled();
            const totalProgress = progressMock.mock.calls.reduce((sum: number, call: number[]) => sum + call[0], 0);
            expect(totalProgress).toBe(100);
        });

        it("should return false when SFTP upload fails and no onError callback", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => {
                const err = new Error("upload failed");
                (err as any).code = "PERMISSION_DENIED";
                cb(err);
            });
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server");
            expect(result).toBe(false);
        });

        it("should pass a user-friendly message to onError when a step fails", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) =>
                cb(new Error("disk full")),
            );
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(false);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(false);
            expect(onError).toHaveBeenCalledOnce();
            const [err] = onError.mock.calls[0] as [{ message: string; details: { additionalDetails: string } }];
            expect(err.message).toBe("Failed to upload the server binary to the remote system.");
            expect(err.details.additionalDetails).toContain("disk full");
        });

        it("should throw ImperativeError when password is expired (FOTS1668)", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const sftpMock = { fastPut: vi.fn() };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: expiredStderr, stdout: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            await expect(ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server")).rejects.toMatchObject({
                message: SshErrors.FOTS1668.summary,
                errorCode: "EPASSWD_EXPIRED",
            });
        });

        it("should call onError with FOTS1668 summary when password is expired and onError is provided", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const sftpMock = { fastPut: vi.fn() };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: expiredStderr, stdout: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(false);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(false);
            expect(onError).toHaveBeenCalledOnce();
            const [err, context] = onError.mock.calls[0] as [{ message: string; errorCode: string }, string];
            expect(err.message).toBe(SshErrors.FOTS1668.summary);
            expect(err.errorCode).toBe("EPASSWD_EXPIRED");
            expect(context).toBe("deploy");
        });

        it("should throw ImperativeError on uninstall when password is expired", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const sftpMock = {};
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: expiredStderr, stdout: "" }) };
            setupSftpMocks(sftpMock, sshMock, { mockGetBinDir: false, mockExistsSync: false });

            await expect(
                ZSshUtils.uninstallServer(new SshSession(fakeSession), "~/.zowe-server"),
            ).rejects.toMatchObject({
                message: SshErrors.FOTS1668.summary,
                errorCode: "EPASSWD_EXPIRED",
            });
        });

        it("should return false when mkdir fails and no onError callback is provided", async () => {
            const sftpMock = {};
            const sshMock = {
                execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: "permission denied", stdout: "" }),
            };
            setupSftpMocks(sftpMock, sshMock);

            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server");
            expect(result).toBe(false);
        });

        it("should call onError when mkdir fails and handle retry if onError returns true", async () => {
            const sftpMock = { fastPut: vi.fn((_l, _r, _o, cb) => cb()), unlink: vi.fn((_p, cb) => cb()) };
            let callCount = 0;
            const sshMock = {
                execCommand: vi.fn().mockImplementation(async () => {
                    callCount++;
                    if (callCount === 1) {
                        return { code: 1, stderr: "temporary error", stdout: "" };
                    }
                    return { code: 0, stderr: "", stdout: "" };
                }),
            };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(true);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(true);
            expect(onError).toHaveBeenCalledOnce();
            expect(sshMock.execCommand).toHaveBeenCalledTimes(3);
        });

        it("should call onError when mkdir fails and return false if onError returns false", async () => {
            const sftpMock = {};
            const sshMock = {
                execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: "permanent error", stdout: "" }),
            };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(false);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(false);
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should call onError when unlink fails during cleanup", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const unlinkMock = vi.fn((_path: string, cb: (err?: Error) => void) => cb(new Error("unlink failed")));
            const sftpMock = { fastPut: fastPutMock, unlink: unlinkMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(false);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(true); // cleanup is non-fatal for installServer
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should log warning when unlink fails and no onError callback is provided", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const unlinkMock = vi.fn((_path: string, cb: (err?: Error) => void) => cb(new Error("unlink failed")));
            const sftpMock = { fastPut: fastPutMock, unlink: unlinkMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server");
            expect(result).toBe(true);
        });

        it("should throw rmErr when rm -rf fails during uninstall and no onError callback is provided", async () => {
            const sftpMock = {};
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: "rm failed", stdout: "" }) };
            setupSftpMocks(sftpMock, sshMock, { mockGetBinDir: false, mockExistsSync: false });

            await expect(ZSshUtils.uninstallServer(new SshSession(fakeSession), "~/.zowe-server")).rejects.toThrow(
                "Failed to uninstall the server from the remote system.",
            );
        });

        it("should call onError when rm -rf fails during uninstall and handle retry if onError returns true", async () => {
            const sftpMock = {};
            let callCount = 0;
            const sshMock = {
                execCommand: vi.fn().mockImplementation(async () => {
                    callCount++;
                    if (callCount === 1) {
                        return { code: 1, stderr: "temporary error", stdout: "" };
                    }
                    return { code: 0, stdout: "" };
                }),
            };
            setupSftpMocks(sftpMock, sshMock, { mockGetBinDir: false, mockExistsSync: false });

            const onError = vi.fn().mockResolvedValue(true);
            await ZSshUtils.uninstallServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(onError).toHaveBeenCalledOnce();
            expect(sshMock.execCommand).toHaveBeenCalledTimes(2);
        });

        it("should call onError when fastPut fails and handle retry if onError returns true", async () => {
            let fastPutCallCount = 0;
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => {
                fastPutCallCount++;
                if (fastPutCallCount === 1) {
                    cb(new Error("fastPut temp error"));
                } else {
                    cb();
                }
            });
            const unlinkMock = vi.fn((_path: string, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock, unlink: unlinkMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(true);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(true);
            expect(onError).toHaveBeenCalledOnce();
            expect(fastPutMock).toHaveBeenCalledTimes(2);
        });

        it("should return false when pax extraction fails and no onError callback is provided", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = {
                execCommand: vi.fn().mockImplementation(async (cmd) => {
                    if (cmd.startsWith("pax")) {
                        return { code: 1, stderr: "pax failed", stdout: "" };
                    }
                    return { code: 0, stderr: "", stdout: "" };
                }),
            };
            setupSftpMocks(sftpMock, sshMock);

            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server");
            expect(result).toBe(false);
        });

        it("should call onError when pax extraction fails and return false if onError returns false", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = {
                execCommand: vi.fn().mockImplementation(async (cmd) => {
                    if (cmd.startsWith("pax")) {
                        return { code: 1, stderr: "pax failed", stdout: "" };
                    }
                    return { code: 0, stderr: "", stdout: "" };
                }),
            };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(false);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(false);
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should call onError when pax extraction fails and continue if onError returns true", async () => {
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const unlinkMock = vi.fn((_path: string, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock, unlink: unlinkMock };
            const sshMock = {
                execCommand: vi.fn().mockImplementation(async (cmd) => {
                    if (cmd.startsWith("pax")) {
                        return { code: 1, stderr: "pax failed", stdout: "" };
                    }
                    return { code: 0, stderr: "", stdout: "" };
                }),
            };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(true);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(true);
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should throw ImperativeError when password is expired during SFTP upload (fastPut)", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) =>
                cb(new Error(expiredStderr)),
            );
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            await expect(ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server")).rejects.toMatchObject({
                message: SshErrors.FOTS1668.summary,
                errorCode: "EPASSWD_EXPIRED",
            });
        });

        it("should return false when password is expired during upload and onError is provided", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) =>
                cb(new Error(expiredStderr)),
            );
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(false);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(false);
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should throw ImperativeError when password is expired during PAX extraction", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = {
                execCommand: vi.fn().mockImplementation(async (cmd) => {
                    if (cmd.startsWith("pax")) {
                        return { code: 1, stderr: expiredStderr, stdout: "" };
                    }
                    return { code: 0, stderr: "", stdout: "" };
                }),
            };
            setupSftpMocks(sftpMock, sshMock);

            await expect(ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server")).rejects.toMatchObject({
                message: SshErrors.FOTS1668.summary,
                errorCode: "EPASSWD_EXPIRED",
            });
        });

        it("should return false when password is expired during PAX extraction and onError is provided", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) => cb());
            const sftpMock = { fastPut: fastPutMock };
            const sshMock = {
                execCommand: vi.fn().mockImplementation(async (cmd) => {
                    if (cmd.startsWith("pax")) {
                        return { code: 1, stderr: expiredStderr, stdout: "" };
                    }
                    return { code: 0, stderr: "", stdout: "" };
                }),
            };
            setupSftpMocks(sftpMock, sshMock);

            const onError = vi.fn().mockResolvedValue(false);
            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(result).toBe(false);
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should abort on uninstall when password is expired and routeExpiredPasswordError handles it with onError returning void", async () => {
            const expiredStderr =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            const sftpMock = {};
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: expiredStderr, stdout: "" }) };
            setupSftpMocks(sftpMock, sshMock, { mockGetBinDir: false, mockExistsSync: false });

            const onError = vi.fn().mockResolvedValue(true);
            await ZSshUtils.uninstallServer(new SshSession(fakeSession), "~/.zowe-server", { onError });
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should call onError when rm -rf fails during uninstall and throw rmErr if onError returns false", async () => {
            const sftpMock = {};
            const sshMock = {
                execCommand: vi.fn().mockResolvedValue({ code: 1, stderr: "permanent error", stdout: "" }),
            };
            setupSftpMocks(sftpMock, sshMock, { mockGetBinDir: false, mockExistsSync: false });

            const onError = vi.fn().mockResolvedValue(false);
            await expect(
                ZSshUtils.uninstallServer(new SshSession(fakeSession), "~/.zowe-server", { onError }),
            ).rejects.toThrow("Failed to uninstall the server from the remote system.");
            expect(onError).toHaveBeenCalledOnce();
        });

        it("should execute real sftp helper method successfully", async () => {
            // Restore sftp mock to test the real sftp method
            vi.restoreAllMocks();

            // Re-mock fs.existsSync and Logger
            vi.spyOn(fs, "existsSync").mockReturnValue(true);
            vi.spyOn(Logger, "getAppLogger").mockReturnValue({
                debug: vi.fn(),
                error: vi.fn(),
                warn: vi.fn(),
                info: vi.fn(),
                trace: vi.fn(),
            } as any);

            // We can spy on NodeSSH.prototype.connect and requestSFTP
            const connectSpy = vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValue({} as any);
            const requestSftpSpy = vi.spyOn(NodeSSH.prototype, "requestSFTP").mockResolvedValue({
                fastPut: vi.fn((_l, _r, _o, cb) => cb()),
                unlink: vi.fn((_p, cb) => cb()),
            } as any);
            const execCommandSpy = vi.spyOn(NodeSSH.prototype, "execCommand").mockResolvedValue({
                code: 0,
                stdout: "",
                stderr: "",
            });
            const disposeSpy = vi.spyOn(NodeSSH.prototype, "dispose").mockImplementation(() => {});

            const result = await ZSshUtils.installServer(new SshSession(fakeSession), "~/.zowe-server");
            expect(result).toBe(true);
            expect(connectSpy).toHaveBeenCalled();
            expect(requestSftpSpy).toHaveBeenCalled();
            expect(execCommandSpy).toHaveBeenCalled();
            expect(disposeSpy).toHaveBeenCalled();
        });
    });

    describe("buildSession", () => {
        it("should build session with password when private key is not provided", () => {
            const profile = {
                host: "example.com",
                port: 2222,
                user: "testuser",
                password: "mypassword",
            };
            const session = ZSshUtils.buildSession(profile);
            expect(session.ISshSession.hostname).toBe("example.com");
            expect(session.ISshSession.port).toBe(2222);
            expect(session.ISshSession.user).toBe("testuser");
            expect(session.ISshSession.password).toBe("mypassword");
            expect(session.ISshSession.privateKey).toBeUndefined();
        });

        it("should build session with private key and passphrase, and undefined password", () => {
            const profile = {
                host: "example.com",
                user: "testuser",
                privateKey: "/path/to/key",
                keyPassphrase: "passphrase",
                password: "mypassword", // should be ignored
            };
            const session = ZSshUtils.buildSession(profile);
            expect(session.ISshSession.hostname).toBe("example.com");
            expect(session.ISshSession.port).toBe(22); // default
            expect(session.ISshSession.user).toBe("testuser");
            expect(session.ISshSession.privateKey).toBe("/path/to/key");
            expect(session.ISshSession.keyPassphrase).toBe("passphrase");
            expect(session.ISshSession.password).toBeUndefined();
        });
    });

    describe("buildSshConfig", () => {
        it("should build basic config without private key", () => {
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
                password: "mypassword",
            });
            const config = ZSshUtils.buildSshConfig(session);
            expect(config.host).toBe("example.com");
            expect(config.port).toBe(22);
            expect(config.username).toBe("testuser");
            expect(config.password).toBe("mypassword");
            expect(config.privateKey).toBeUndefined();
        });

        it("should read private key file and build config", () => {
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
                privateKey: "/path/to/key",
                keyPassphrase: "passphrase",
            });
            const readFileSyncSpy = vi.spyOn(fs, "readFileSync").mockReturnValue("key-content");
            const config = ZSshUtils.buildSshConfig(session);
            expect(config.privateKey).toBe("key-content");
            expect(config.passphrase).toBe("passphrase");
            expect(readFileSyncSpy).toHaveBeenCalledWith("/path/to/key", "utf-8");
        });

        it("should merge configProps", () => {
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const config = ZSshUtils.buildSshConfig(session, { readyTimeout: 5000, keepaliveInterval: 1000 });
            expect(config.readyTimeout).toBe(5000);
            expect(config.keepaliveInterval).toBe(1000);
        });

        it("should trace log in debug callback", () => {
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const traceMock = vi.fn();
            vi.spyOn(Logger, "getAppLogger").mockReturnValue({
                trace: traceMock,
            } as any);

            const config = ZSshUtils.buildSshConfig(session);
            expect(config.debug).toBeDefined();
            config.debug!("test debug message");
            expect(traceMock).toHaveBeenCalledWith("test debug message");
        });
    });

    describe("isPrivateKeyAuthFailure", () => {
        it("should return false if hasPrivateKey is false", () => {
            const result = ZSshUtils.isPrivateKeyAuthFailure("Permission denied (publickey)", false);
            expect(result).toBe(false);
        });

        it("should return true for private key failure patterns", () => {
            const result = ZSshUtils.isPrivateKeyAuthFailure("Permission denied (publickey)");
            expect(result).toBe(true);
        });

        it("should return false for other error messages", () => {
            const result = ZSshUtils.isPrivateKeyAuthFailure("Connection timed out");
            expect(result).toBe(false);
        });
    });

    describe("getBinDir error path", () => {
        it("should throw error when bin directory is not found", () => {
            vi.spyOn(fs, "existsSync").mockReturnValue(false);
            expect(() => (ZSshUtils as any).getBinDir("/")).toThrow("Failed to find bin directory in path");
        });
    });

    describe("detectServerOnPath", () => {
        const mockDetectServerSSH = (
            expectedServerPathOutput: string,
            expectedVersion: string | undefined,
            shouldHaveExecuteAccess: boolean,
        ) => {
            const sftpMock = {};
            let exitEventHandler: () => void;
            let dataEventHandler: (strOrBuf: string | Buffer) => void;
            const mockChannel = vi.mockObject({
                on: vi.fn().mockImplementation((eventName: string, eventHandler) => {
                    if (eventName === "exit") {
                        exitEventHandler = eventHandler;
                    } else if (eventName === "data") {
                        dataEventHandler = eventHandler;
                    }
                }),
                write: vi.fn().mockImplementation((_writeData: string) => {}),
                end: vi.fn(),
                closed: false,
                stdout: { readableEnded: false },
            });

            const sshMock = {
                execCommand: vi.fn().mockImplementation((cmd) => {
                    if (cmd.indexOf("-v") > 0) {
                        return shouldHaveExecuteAccess
                            ? { code: 0, stderr: "", stdout: expectedVersion }
                            : { code: 1, stderr: "No such command, buddy", stdout: "" };
                    }

                    throw new Error("Unknown command passed to execCommand");
                }),
                withShell: vi.fn().mockImplementation(async (shellCallback) => {
                    const callbackPromise = shellCallback(mockChannel);
                    dataEventHandler(`user@host$ blahblah\n${expectedServerPathOutput} \n`);

                    if (exitEventHandler) {
                        exitEventHandler();
                    }
                    await callbackPromise;
                }),
            };
            setupSftpMocks(sftpMock, sshMock);
        };
        it("Should return hasExecutePermission=true and version number if zowex command succeeds", async () => {
            const expectedServerPath = `/u/users/user/mybins/${ZSshClient.BIN_NAME}`;
            const expectedVersion = "1.0.0-aefbab";
            mockDetectServerSSH(expectedServerPath, expectedVersion, true);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const detectResult = await ZSshUtils.detectServerOnPath(session);
            expect(detectResult).toBeDefined();
            expect(detectResult.serverPath).toEqual(expectedServerPath);
            expect(detectResult.hasExecutePermission).toEqual(true);
            expect(detectResult.version).toEqual(expectedVersion);
        });
        it("Should return hasExecutePermission=false if zowex command fails", async () => {
            const expectedServerPath = `/u/users/user/mybins/${ZSshClient.BIN_NAME}`;
            mockDetectServerSSH(expectedServerPath, undefined, false);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const detectResult = await ZSshUtils.detectServerOnPath(session);
            expect(detectResult).toBeDefined();
            expect(detectResult.serverPath).toEqual(expectedServerPath);
            expect(detectResult.hasExecutePermission).toEqual(false);
            expect(detectResult.version).toBeUndefined();
        });
        it("Should detect binary not found in output of shell command", async () => {
            mockDetectServerSSH("Zowe binary not found", undefined, false);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const detectResult = await ZSshUtils.detectServerOnPath(session);
            expect(detectResult).toBeDefined();
            expect(detectResult.serverPath).toBeUndefined();
            expect(detectResult.hasExecutePermission).toEqual(false);
            expect(detectResult.version).toBeUndefined();
        });
        it("Should handle unexpected error message in output of shell command", async () => {
            mockDetectServerSSH("Eek! Some kinda error that's unexpected", undefined, false);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const detectResult = await ZSshUtils.detectServerOnPath(session);
            expect(detectResult).toBeDefined();
            expect(detectResult.serverPath).toBeUndefined();
            expect(detectResult.hasExecutePermission).toEqual(false);
            expect(detectResult.version).toBeUndefined();
        });
    });

    describe("lacksWriteAccess", () => {
        const mockLacksWriteAccessSsh = (shouldExist: boolean, shouldHaveWriteAccess: boolean) => {
            const sftpMock = {};

            const sshMock = {
                execCommand: vi.fn().mockImplementation((cmd) => {
                    if (cmd.indexOf("-e") > 0) {
                        return shouldExist ? { code: 0, stderr: "", stdout: "" } : { code: 1, stderr: "", stdout: "" };
                    } else if (cmd.indexOf("-w") > 0) {
                        return shouldHaveWriteAccess
                            ? { code: 0, stderr: "", stdout: "" }
                            : { code: 1, stderr: "", stdout: "" };
                    }

                    throw new Error("Unknown command passed to execCommand");
                }),
            };
            setupSftpMocks(sftpMock, sshMock);
        };

        it("Should return true if the path does exist but the user does NOT have write access to it", async () => {
            mockLacksWriteAccessSsh(true, false);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const lacksAccess = await ZSshUtils.lacksWriteAccess(session, "fakePath");
            expect(lacksAccess).toEqual(true);
        });
        it("Should return false if the path does exist but the user does INDEED have write access to it", async () => {
            mockLacksWriteAccessSsh(true, true);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const lacksAccess = await ZSshUtils.lacksWriteAccess(session, "fakePath");
            expect(lacksAccess).toEqual(false);
        });
        it("Should return false if the path does NOT exist even if (for some reason) write access check returns 0", async () => {
            mockLacksWriteAccessSsh(false, true);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const lacksAccess = await ZSshUtils.lacksWriteAccess(session, "fakePath");
            expect(lacksAccess).toEqual(false);
        });
        it("Should return false if the path does NOT exist and the write access command also fails", async () => {
            // since if the path does not exist, we don't know whether the user can create the directory without
            // additional checks
            mockLacksWriteAccessSsh(false, false);
            const session = new SshSession({
                hostname: "example.com",
                port: 22,
                user: "testuser",
            });
            const lacksAccess = await ZSshUtils.lacksWriteAccess(session, "fakePath");
            expect(lacksAccess).toEqual(false);
        });
    });
});
