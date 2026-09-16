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

import { DeferredPromise } from "@zowe/imperative";
import { type ISshSession, SshSession } from "@zowe/zos-uss-for-zowe-sdk";
import { NodeSSH } from "node-ssh";
import { SessionContext } from "../src/utils";

describe("SessionContext", () => {
    const fakeSession: ISshSession = {
        hostname: "example.com",
        port: 22,
        user: "admin",
        password: "pass",
    };

    afterEach(() => {
        vi.restoreAllMocks();
    });

    describe("acquire with a plain SshSession", () => {
        it("opens a fresh connection", async () => {
            const connectSpy = vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValue({} as any);
            const session = new SshSession(fakeSession);

            const handle = await SessionContext.acquire(session);

            expect(connectSpy).toHaveBeenCalledTimes(1);
            expect(handle.ssh).toBeInstanceOf(NodeSSH);
        });

        it("disposes the connection when the handle is disposed", async () => {
            vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValue({} as any);
            const disposeSpy = vi.spyOn(NodeSSH.prototype, "dispose").mockImplementation(() => {});
            const session = new SshSession(fakeSession);

            {
                using handle = await SessionContext.acquire(session);
                expect(handle.ssh).toBeDefined();
            }

            expect(disposeSpy).toHaveBeenCalledTimes(1);
        });

        it("opens a new, independent connection for every call", async () => {
            const connectSpy = vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValue({} as any);
            const session = new SshSession(fakeSession);

            const first = await SessionContext.acquire(session);
            const second = await SessionContext.acquire(session);

            expect(connectSpy).toHaveBeenCalledTimes(2);
            expect(first.ssh).not.toBe(second.ssh);
        });
    });

    describe("acquire with a SessionContext", () => {
        it("connects once and reuses the connection across calls", async () => {
            const connectSpy = vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValue({} as any);
            const ctx = new SessionContext(new SshSession(fakeSession));

            const first = await SessionContext.acquire(ctx);
            const second = await SessionContext.acquire(ctx);

            expect(connectSpy).toHaveBeenCalledTimes(1);
            expect(first.ssh).toBe(second.ssh);
        });

        it("does not dispose the shared connection when the handle is disposed", async () => {
            vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValue({} as any);
            const disposeSpy = vi.spyOn(NodeSSH.prototype, "dispose").mockImplementation(() => {});
            const ctx = new SessionContext(new SshSession(fakeSession));

            {
                using handle = await SessionContext.acquire(ctx);
                expect(handle.ssh).toBeDefined();
            }

            expect(disposeSpy).not.toHaveBeenCalled();
        });

        it("blocks a second concurrent caller until the shared connect completes", async () => {
            // Regression test: connect() must be memoized as a promise, not just the eventual
            // NodeSSH instance. Memoizing only the instance lets a second caller's synchronous
            // pre-await check see a non-null (but not-yet-connected) value and return immediately
            // with an unconnected ssh, instead of waiting for the handshake like the first caller.
            const gate = new DeferredPromise<void>();
            const connectSpy = vi.spyOn(NodeSSH.prototype, "connect").mockImplementation(() => gate.promise as any);
            const ctx = new SessionContext(new SshSession(fakeSession));

            let secondResolved = false;
            const first = SessionContext.acquire(ctx);
            const second = SessionContext.acquire(ctx).then((handle) => {
                secondResolved = true;
                return handle;
            });

            // Drain pending microtasks without resolving the connect: the second caller must not
            // have resolved yet, since the shared connection hasn't finished connecting.
            await Promise.resolve();
            await Promise.resolve();
            expect(secondResolved).toBe(false);
            expect(connectSpy).toHaveBeenCalledTimes(1);

            gate.resolve();
            const [firstHandle, secondHandle] = await Promise.all([first, second]);

            expect(secondResolved).toBe(true);
            expect(connectSpy).toHaveBeenCalledTimes(1);
            expect(firstHandle.ssh).toBe(secondHandle.ssh);
        });
    });

    describe("[Symbol.dispose]", () => {
        it("disposes the underlying connection once it has connected", async () => {
            vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValue({} as any);
            const disposeSpy = vi.spyOn(NodeSSH.prototype, "dispose").mockImplementation(() => {});
            const ctx = new SessionContext(new SshSession(fakeSession));

            await SessionContext.acquire(ctx);
            ctx[Symbol.dispose]();

            expect(disposeSpy).toHaveBeenCalledTimes(1);
        });

        it("does nothing if a connection was never established", () => {
            const disposeSpy = vi.spyOn(NodeSSH.prototype, "dispose").mockImplementation(() => {});
            const ctx = new SessionContext(new SshSession(fakeSession));

            expect(() => ctx[Symbol.dispose]()).not.toThrow();
            expect(disposeSpy).not.toHaveBeenCalled();
        });

        it("defers disposal until an in-flight connect settles, then disposes it", async () => {
            const gate = new DeferredPromise<void>();
            vi.spyOn(NodeSSH.prototype, "connect").mockImplementation(() => gate.promise as any);
            const disposeSpy = vi.spyOn(NodeSSH.prototype, "dispose").mockImplementation(() => {});
            const ctx = new SessionContext(new SshSession(fakeSession));

            const acquiring = SessionContext.acquire(ctx);
            ctx[Symbol.dispose](); // dispose while the connect is still pending

            expect(disposeSpy).not.toHaveBeenCalled();

            gate.resolve();
            await acquiring;
            // The dispose-on-settle continuation runs as a microtask after connect resolves.
            await Promise.resolve();

            expect(disposeSpy).toHaveBeenCalledTimes(1);
        });

        it("does not throw or leave an unhandled rejection when the in-flight connect fails", async () => {
            const unhandled = vi.fn();
            process.once("unhandledRejection", unhandled);

            const gate = new DeferredPromise<void>();
            vi.spyOn(NodeSSH.prototype, "connect").mockImplementation(() => gate.promise as any);
            const disposeSpy = vi.spyOn(NodeSSH.prototype, "dispose").mockImplementation(() => {});
            const ctx = new SessionContext(new SshSession(fakeSession));

            const acquiring = SessionContext.acquire(ctx).catch(() => undefined);
            expect(() => ctx[Symbol.dispose]()).not.toThrow();

            gate.reject(new Error("connection refused"));
            await acquiring;
            // Give the unhandledRejection event a chance to fire before asserting it didn't.
            await new Promise((r) => setImmediate(r));

            expect(disposeSpy).not.toHaveBeenCalled();
            expect(unhandled).not.toHaveBeenCalled();
        });
    });

    describe("ISshSession", () => {
        it("delegates to the wrapped session", () => {
            const session = new SshSession(fakeSession);
            const ctx = new SessionContext(session);

            expect(ctx.ISshSession).toEqual(session.ISshSession);
        });
    });
});
