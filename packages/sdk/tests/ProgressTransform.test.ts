/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 * This test file covers the ProgressTransform stream class.
 */

import { pipeline, Readable, Writable } from "node:stream";
import { promisify } from "node:util";
import type { CallbackInfo } from "../src/doc/types";
import { ProgressTransform } from "../src/ProgressTransform";

const pipelineAsync = promisify(pipeline);

describe("ProgressTransform", () => {
    it("should initialize without arguments", () => {
        const transform = new ProgressTransform();
        expect(transform.bytesProcessed).toBe(0);
    });

    it("should call callback(0) immediately when CallbackInfo is provided", () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback, totalBytes: 100 };
        const transform = new ProgressTransform(callbackInfo);

        expect(callback).toHaveBeenCalledTimes(1);
        expect(callback).toHaveBeenCalledWith(0);
        expect(transform.bytesProcessed).toBe(0);
    });

    it("should process Buffer chunks and update bytesProcessed", async () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback, totalBytes: 100 };
        const onDataCallback = vi.fn();
        const transform = new ProgressTransform(callbackInfo, onDataCallback);

        const chunk1 = Buffer.alloc(30);
        const chunk2 = Buffer.alloc(40);

        // First chunk
        await new Promise<void>((resolve) => {
            transform.write(chunk1, () => resolve());
        });
        expect(transform.bytesProcessed).toBe(30);
        expect(callback).toHaveBeenLastCalledWith(30);
        expect(onDataCallback).toHaveBeenCalledTimes(1);

        // Second chunk
        await new Promise<void>((resolve) => {
            transform.write(chunk2, () => resolve());
        });
        expect(transform.bytesProcessed).toBe(70);
        expect(callback).toHaveBeenLastCalledWith(70);
        expect(onDataCallback).toHaveBeenCalledTimes(2);
    });

    it("should process string chunks with different encodings", async () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback, totalBytes: 100 };
        const transform = new ProgressTransform(callbackInfo);

        const text = "Hello World!"; // 12 bytes in UTF-8
        const expectedBytes = Buffer.byteLength(text, "utf-8");

        await new Promise<void>((resolve) => {
            transform.write(text, "utf-8", () => resolve());
        });

        expect(transform.bytesProcessed).toBe(expectedBytes);
    });

    it("should process string chunks with no encoding specified", async () => {
        const transform = new ProgressTransform();
        await new Promise<void>((resolve) => {
            transform.write("Hello", () => resolve());
        });
        expect(transform.bytesProcessed).toBe(5);
    });

    it("should process string chunk with undefined encoding directly", () => {
        const transform = new ProgressTransform();
        transform._transform("hello", undefined as any, () => {});
        expect(transform.bytesProcessed).toBe(5);
    });

    it("should deduplicate progress callback calls when percentage does not change", async () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback, totalBytes: 1000 };
        const transform = new ProgressTransform(callbackInfo);

        // 0% (called in constructor)
        expect(callback).toHaveBeenCalledTimes(1);

        // Write 3 bytes -> 0.3% -> Math.round is 0% (should not call callback again)
        await new Promise<void>((resolve) => {
            transform.write(Buffer.alloc(3), () => resolve());
        });
        expect(callback).toHaveBeenCalledTimes(1);

        // Write 10 bytes -> 1.3% -> Math.round is 1% (should call callback)
        await new Promise<void>((resolve) => {
            transform.write(Buffer.alloc(10), () => resolve());
        });
        expect(callback).toHaveBeenCalledTimes(2);
        expect(callback).toHaveBeenLastCalledWith(1);
    });

    it("should cap percentage at 100", async () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback, totalBytes: 50 };
        const transform = new ProgressTransform(callbackInfo);

        // Write 60 bytes (more than totalBytes)
        await new Promise<void>((resolve) => {
            transform.write(Buffer.alloc(60), () => resolve());
        });

        expect(transform.bytesProcessed).toBe(60);
        expect(callback).toHaveBeenLastCalledWith(100);
    });

    it("should handle totalBytes of 0 without dividing by zero", async () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback, totalBytes: 0 };
        const transform = new ProgressTransform(callbackInfo);

        await new Promise<void>((resolve) => {
            transform.write(Buffer.alloc(10), () => resolve());
        });

        expect(transform.bytesProcessed).toBe(10);
        // Should only be called once with 0 from constructor
        expect(callback).toHaveBeenCalledTimes(1);
        expect(callback).toHaveBeenCalledWith(0);
    });

    it("should handle CallbackInfo with undefined totalBytes", async () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback };
        const transform = new ProgressTransform(callbackInfo);

        await new Promise<void>((resolve) => {
            transform.write(Buffer.alloc(10), () => resolve());
        });

        expect(transform.bytesProcessed).toBe(10);
        // Should only be called once with 0 from constructor
        expect(callback).toHaveBeenCalledTimes(1);
        expect(callback).toHaveBeenCalledWith(0);
    });

    it("should handle null chunk gracefully", () => {
        const transform = new ProgressTransform();
        const callback = vi.fn();

        // Directly call _transform with null chunk (simulating stream end or edge cases)
        transform._transform(null as any, "utf-8", callback);

        expect(transform.bytesProcessed).toBe(0);
        expect(callback).toHaveBeenCalledTimes(1);
        expect(callback).toHaveBeenCalledWith(null, null);
    });

    it("should work correctly in a stream pipeline", async () => {
        const callback = vi.fn();
        const callbackInfo: CallbackInfo = { callback, totalBytes: 100 };
        const transform = new ProgressTransform(callbackInfo);

        const source = Readable.from([Buffer.alloc(50), Buffer.alloc(50)]);
        let bytesReceived = 0;
        const destination = new Writable({
            write(chunk, _encoding, cb) {
                bytesReceived += chunk.length;
                cb();
            },
        });

        await pipelineAsync(source, transform, destination);

        expect(bytesReceived).toBe(100);
        expect(transform.bytesProcessed).toBe(100);
        expect(callback).toHaveBeenLastCalledWith(100);
    });
});
