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
import { SshErrors } from "../src/SshErrors";
import { ZSshUtils } from "../src/ZSshUtils";

vi.mock("node:fs", { spy: true });

function setupSftpMocks(
    sftpMock: object,
    sshMock: { execCommand: ReturnType<typeof vi.fn> },
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

        it("should return false for matching checksums with different order", async () => {
            const readFileSyncSpy = vi.spyOn(fs, "readFileSync").mockReturnValueOnce("123 abc\n456 def");
            const isOutdated = await ZSshUtils.checkIfOutdated({ def: "456", abc: "123" });
            expect(isOutdated).toBe(false);
            expect(readFileSyncSpy).toHaveBeenCalled();
        });

        it("should return true for different checksums", async () => {
            const readFileSyncSpy = vi.spyOn(fs, "readFileSync").mockReturnValueOnce("123 abc\n456 def");
            const isOutdated = await ZSshUtils.checkIfOutdated({ abc: "789", def: "456" });
            expect(isOutdated).toBe(true);
            expect(readFileSyncSpy).toHaveBeenCalled();
        });

        it("should return true for same checksums and local file added", async () => {
            const readFileSyncSpy = vi.spyOn(fs, "readFileSync").mockReturnValueOnce("123 abc\n456 def\n789 ghi");
            const isOutdated = await ZSshUtils.checkIfOutdated({ abc: "123", def: "456" });
            expect(isOutdated).toBe(true);
            expect(readFileSyncSpy).toHaveBeenCalled();
        });

        it("should return true for same checksums and local file removed", async () => {
            const readFileSyncSpy = vi.spyOn(fs, "readFileSync").mockReturnValueOnce("123 abc");
            const isOutdated = await ZSshUtils.checkIfOutdated({ abc: "123", def: "456" });
            expect(isOutdated).toBe(true);
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
            const sshMock = { execCommand: vi.fn().mockResolvedValue({ code: 0, stdout: "", stderr: "" }) };
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
            const fastPutMock = vi.fn((_local: string, _remote: string, _opts: any, cb: (err?: Error) => void) =>
                cb(new Error("upload failed")),
            );
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
    });
});
