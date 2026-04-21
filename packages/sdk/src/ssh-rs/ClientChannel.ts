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

import { EventEmitter } from "node:events";
import { PassThrough, Writable } from "node:stream";
import type { Channel } from "russh";
import type { Subscription } from "rxjs";

type DebugFn = (message: string) => void;
const noop: DebugFn = () => {};

/**
 * ssh2-compatible ClientChannel backed by a russh-napi Channel.
 *
 * Exposes `.stdin` (Writable), `.stdout` (Readable), `.stderr` (Readable)
 * and a `"close"` event — the subset used by the Zowe SDK.
 */
export class ClientChannel extends EventEmitter {
    public readonly stdin: Writable;
    public readonly stdout: PassThrough;
    public readonly stderr: PassThrough;

    private subscriptions: Subscription[] = [];
    private ended = false;
    private stdoutBytes = 0;
    private stderrBytes = 0;
    private stdinBytes = 0;

    constructor(
        private readonly inner: Channel,
        private readonly debug: DebugFn = noop,
    ) {
        super();

        this.stdout = new PassThrough();
        this.stderr = new PassThrough();

        this.stdin = new Writable({
            write: (chunk: Buffer | string, _encoding, callback) => {
                const data = typeof chunk === "string" ? Buffer.from(chunk) : chunk;
                this.stdinBytes += data.length;
                this.debug(`russh: [ch ${this.inner.id}] stdin write ${data.length} bytes (total ${this.stdinBytes})`);
                inner.write(new Uint8Array(data)).then(() => callback(), callback);
            },
            final: (callback) => {
                this.debug(`russh: [ch ${this.inner.id}] stdin EOF sent (total written ${this.stdinBytes} bytes)`);
                inner.eof().then(() => callback(), callback);
            },
        });

        this.wireObservables();
    }

    /**
     * Drain all incoming data without processing (ssh2 compatibility).
     */
    public resume(): this {
        this.stdout.resume();
        this.stderr.resume();
        return this;
    }

    /**
     * Close the channel and clean up subscriptions.
     */
    public close(): void {
        this.debug(`russh: [ch ${this.inner.id}] close requested`);
        this.cleanup();
        this.inner.close().catch(() => {});
    }

    private wireObservables(): void {
        const dataSub = this.inner.data$.subscribe({
            next: (data) => {
                this.stdoutBytes += data.length;
                this.stdout.push(Buffer.from(data));
            },
            complete: () => this.cleanup(),
            error: (err) => {
                this.debug(`russh: [ch ${this.inner.id}] stdout stream error: ${err.message}`);
                this.stdout.destroy(err);
            },
        });

        const extSub = this.inner.extendedData$.subscribe({
            next: ([type, data]) => {
                if (type === 1) {
                    this.stderrBytes += data.length;
                    this.stderr.push(Buffer.from(data));
                }
            },
            complete: () => this.cleanup(),
            error: (err) => {
                this.debug(`russh: [ch ${this.inner.id}] stderr stream error: ${err.message}`);
                this.stderr.destroy(err);
            },
        });

        const eofSub = this.inner.eof$.subscribe(() => {
            this.debug(
                `russh: [ch ${this.inner.id}] remote EOF (stdout ${this.stdoutBytes} bytes, stderr ${this.stderrBytes} bytes)`,
            );
            this.cleanup();
        });

        const closeSub = this.inner.closed$.subscribe(() => {
            this.debug(`russh: [ch ${this.inner.id}] closed by remote`);
            this.cleanup();
            this.emit("close");
        });

        this.subscriptions.push(dataSub, extSub, eofSub, closeSub);
    }

    private cleanup(): void {
        if (this.ended) return;
        this.ended = true;

        for (const sub of this.subscriptions) {
            sub.unsubscribe();
        }
        this.subscriptions = [];

        if (!this.stdout.writableEnded) this.stdout.end();
        if (!this.stderr.writableEnded) this.stderr.end();
    }
}
