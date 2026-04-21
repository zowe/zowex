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

import { Transform } from "node:stream";
const ascii85 = require("ascii85");

export class Base85Decode extends Transform {
    extra: string;

    constructor() {
        super({ decodeStrings: false });
        this.extra = "";
    }

    _transform(chunk: any, encoding: string, cb: () => void) {
        chunk = "" + chunk;
        chunk = this.extra + chunk.replace(/(\r\n|\n|\r)/gm, "");

        const remaining = chunk.length % 5;
        this.extra = chunk.slice(chunk.length - remaining);
        chunk = chunk.slice(0, chunk.length - remaining);

        const buf = ascii85.decode(chunk);
        this.push(buf);
        cb();
    }

    _flush(cb: () => void) {
        if (this.extra.length) {
            this.push(ascii85.decode(this.extra));
        }
        cb();
    }
}

export class Base85Encode extends Transform {
    extra: Buffer | null;
    lineLength: number | undefined;
    currLineLength: number;

    constructor(options: any) {
        super(options);
        this.extra = null;
        this.lineLength = options && options.lineLength;
        this.currLineLength = 0;
        if (options && options.prefix) {
            this.push(options.prefix);
        }

        const encIn = options && options.inputEncoding;
        this.setDefaultEncoding(encIn || "utf8");

        const encOut = options && options.outputEncoding;
        if (encOut !== null) {
            this.setEncoding(encOut || "utf8");
        }
    }

    _fixLineLength(chunk: string): string {
        if (!this.lineLength) {
            return chunk;
        }

        const size = chunk.length;
        const needed = this.lineLength - this.currLineLength;
        let start: number, end: number;

        let _chunk = "";
        for (start = 0, end = needed; end < size; start = end, end += this.lineLength) {
            _chunk += chunk.slice(start, end);
            _chunk += "\r\n";
        }

        const left = chunk.slice(start);
        this.currLineLength = left.length;

        _chunk += left;

        return _chunk;
    }

    _transform(chunk: any, encoding: string, cb: () => void) {
        if (this.extra) {
            chunk = Buffer.concat([this.extra, chunk]);
            this.extra = null;
        }

        const remaining = chunk.length % 4;

        if (remaining !== 0) {
            this.extra = chunk.slice(chunk.length - remaining);
            chunk = chunk.slice(0, chunk.length - remaining);
        }

        chunk = ascii85.encode(chunk);
        this.push(Buffer.from(this._fixLineLength(chunk)));
        cb();
    }

    _flush(cb: () => void) {
        if (this.extra) {
            this.push(Buffer.from(this._fixLineLength(ascii85.encode(this.extra))));
        }
        cb();
    }
}
