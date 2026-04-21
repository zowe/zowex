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

import type { EventEmitter } from "node:events";
import type { Readable, Writable } from "node:stream";
import type { ClientCallback, ConnectConfig } from "ssh2";

export interface ClientChannel extends EventEmitter {
    stdin: Writable;
    stdout: Readable;
    stderr: Readable;
    resume(): this;
}

export interface Client extends EventEmitter {
    connect(config: ConnectConfig): void;
    exec(command: string, callback: ClientCallback): void;
    end(): void;
}

export function createClient(useNative?: boolean): Client {
    if (useNative) {
        const { Client: NativeClient } = require("./Client");
        return new NativeClient();
    } else {
        const { Client: Ssh2Client } = require("ssh2");
        return new Ssh2Client();
    }
}
