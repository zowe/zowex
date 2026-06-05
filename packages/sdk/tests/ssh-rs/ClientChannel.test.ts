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

import { Subject } from "rxjs";
import { describe, expect, it, vi } from "vitest";
import { ClientChannel } from "../../src/ssh-rs/ClientChannel";

describe("ClientChannel", () => {
    const createMockInnerChannel = () => {
        return {
            id: 42,
            write: vi.fn().mockResolvedValue(undefined),
            eof: vi.fn().mockResolvedValue(undefined),
            close: vi.fn().mockResolvedValue(undefined),
            data$: new Subject<Uint8Array>(),
            extendedData$: new Subject<[number, Uint8Array]>(),
            eof$: new Subject<void>(),
            closed$: new Subject<void>(),
        };
    };

    it("should initialize streams and write stdin data to inner channel", async () => {
        const mockInner = createMockInnerChannel();
        const debugMock = vi.fn();
        const channel = new ClientChannel(mockInner as any, debugMock);

        expect(channel.stdin).toBeDefined();
        expect(channel.stdout).toBeDefined();
        expect(channel.stderr).toBeDefined();

        // Test stdin write (string chunk)
        await new Promise<void>((resolve, reject) => {
            (channel.stdin as any)._write("test string", "utf8", (err: any) => (err ? reject(err) : resolve()));
        });
        expect(mockInner.write).toHaveBeenCalledWith(new Uint8Array(Buffer.from("test string")));
        expect(debugMock).toHaveBeenCalled();

        // Test stdin write (Buffer chunk)
        const buf = Buffer.from("test buffer");
        await new Promise<void>((resolve, reject) => {
            channel.stdin.write(buf, (err) => (err ? reject(err) : resolve()));
        });
        expect(mockInner.write).toHaveBeenLastCalledWith(new Uint8Array(buf));

        // Test stdin final / EOF
        await new Promise<void>((resolve, reject) => {
            channel.stdin.end((err: unknown) => (err ? reject(err) : resolve()));
        });
        expect(mockInner.eof).toHaveBeenCalled();
    });

    it("should handle error in stdin write", async () => {
        const mockInner = createMockInnerChannel();
        const testError = new Error("write error");
        mockInner.write.mockRejectedValueOnce(testError);

        const channel = new ClientChannel(mockInner as any);

        const errorPromise = new Promise<void>((resolve) => {
            channel.stdin.on("error", (err) => {
                expect(err).toBe(testError);
                resolve();
            });
        });

        channel.stdin.write("test");
        await errorPromise;
    });

    it("should handle error in stdin final", async () => {
        const mockInner = createMockInnerChannel();
        const testError = new Error("eof error");
        mockInner.eof.mockRejectedValueOnce(testError);

        const channel = new ClientChannel(mockInner as any);

        const errorPromise = new Promise<void>((resolve) => {
            channel.stdin.on("error", (err) => {
                expect(err).toBe(testError);
                resolve();
            });
        });

        channel.stdin.end();
        await errorPromise;
    });

    it("should handle resume()", () => {
        const mockInner = createMockInnerChannel();
        const channel = new ClientChannel(mockInner as any);
        const resumeStdoutSpy = vi.spyOn(channel.stdout, "resume");
        const resumeStderrSpy = vi.spyOn(channel.stderr, "resume");

        const result = channel.resume();
        expect(result).toBe(channel);
        expect(resumeStdoutSpy).toHaveBeenCalled();
        expect(resumeStderrSpy).toHaveBeenCalled();
    });

    it("should handle close()", () => {
        const mockInner = createMockInnerChannel();
        const channel = new ClientChannel(mockInner as any);

        channel.close();
        expect(mockInner.close).toHaveBeenCalled();
    });

    it("should handle close() with inner channel error gracefully", () => {
        const mockInner = createMockInnerChannel();
        mockInner.close.mockRejectedValueOnce(new Error("close fail"));
        const channel = new ClientChannel(mockInner as any);

        expect(() => channel.close()).not.toThrow();
    });

    it("should push data from inner observables to stdout and stderr", async () => {
        const mockInner = createMockInnerChannel();
        const channel = new ClientChannel(mockInner as any);

        const stdoutData: Buffer[] = [];
        const stderrData: Buffer[] = [];

        channel.stdout.on("data", (chunk) => stdoutData.push(chunk));
        channel.stderr.on("data", (chunk) => stderrData.push(chunk));

        // Push stdout data
        const outChunk = new Uint8Array([1, 2, 3]);
        mockInner.data$.next(outChunk);
        expect(stdoutData[0]).toEqual(Buffer.from(outChunk));

        // Push stderr data (type 1)
        const errChunk = new Uint8Array([4, 5, 6]);
        mockInner.extendedData$.next([1, errChunk]);
        expect(stderrData[0]).toEqual(Buffer.from(errChunk));

        // Push other extended data type (should be ignored)
        mockInner.extendedData$.next([2, new Uint8Array([9])]);
        expect(stderrData).toHaveLength(1);
    });

    it("should handle remote EOF", () => {
        const mockInner = createMockInnerChannel();
        const channel = new ClientChannel(mockInner as any);
        const cleanupSpy = vi.spyOn(channel as any, "cleanup");

        mockInner.eof$.next();
        expect(cleanupSpy).toHaveBeenCalledTimes(1);

        // Directly call cleanup a second time to cover the "if (this.ended) return;" branch
        (channel as any).cleanup();
        expect(cleanupSpy).toHaveBeenCalledTimes(2);
    });

    it("should handle remote closed and emit close", async () => {
        const mockInner = createMockInnerChannel();
        const channel = new ClientChannel(mockInner as any);

        const closePromise = new Promise<void>((resolve) => {
            channel.on("close", resolve);
        });

        mockInner.closed$.next();
        await closePromise;
    });

    it("should destroy stdout/stderr on observable errors", () => {
        const mockInner = createMockInnerChannel();
        const channel = new ClientChannel(mockInner as any);

        // Add error listeners to prevent uncaught exceptions
        channel.stdout.on("error", () => {});
        channel.stderr.on("error", () => {});

        const stdoutDestroySpy = vi.spyOn(channel.stdout, "destroy");
        const stderrDestroySpy = vi.spyOn(channel.stderr, "destroy");

        const testErr = new Error("stdout fail");
        mockInner.data$.error(testErr);
        expect(stdoutDestroySpy).toHaveBeenCalledWith(testErr);

        const testErr2 = new Error("stderr fail");
        mockInner.extendedData$.error(testErr2);
        expect(stderrDestroySpy).toHaveBeenCalledWith(testErr2);
    });

    it("should complete stdout/stderr when observables complete", () => {
        const mockInner = createMockInnerChannel();
        const channel = new ClientChannel(mockInner as any);

        const stdoutCleanupSpy = vi.spyOn(channel as any, "cleanup");
        mockInner.data$.complete();
        expect(stdoutCleanupSpy).toHaveBeenCalled();
    });
});
