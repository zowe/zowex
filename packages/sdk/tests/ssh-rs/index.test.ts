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

import Module from "node:module";
import { Client as Ssh2Client } from "ssh2";
import { afterAll, beforeAll, describe, expect, it, vi } from "vitest";
import { createClient } from "../../src/ssh-rs";

const originalRequire = Module.prototype.require;

class MockClient {
    public end = vi.fn();
}

beforeAll(() => {
    // Stub dynamic require to intercept require("./Client")
    Module.prototype.require = function (id: string, ...args: any[]) {
        if (id === "./Client") {
            return { Client: MockClient };
        }
        return originalRequire.apply(this, [id, ...args] as any);
    };
});

afterAll(() => {
    // Restore original require loader
    Module.prototype.require = originalRequire;
});

describe("ssh-rs index createClient", () => {
    it("should return a native client when useNative is true", () => {
        const client = createClient(true);
        expect(client).toBeDefined();
        expect(client).toBeInstanceOf(MockClient);
    });

    it("should return an ssh2 client when useNative is false/undefined", () => {
        const client = createClient(false);
        expect(client).toBeInstanceOf(Ssh2Client);

        const clientDefault = createClient();
        expect(clientDefault).toBeInstanceOf(Ssh2Client);
    });
});
