import { type ISshSession, SshSession } from "@zowe/zos-uss-for-zowe-sdk";
import { type Config, NodeSSH } from "node-ssh";
import { ZSshUtils } from "../ZSshUtils";

/**
 * A connected NodeSSH handle plus disposal ownership, meant to be acquired with a `using`
 * declaration. Disposing it closes the connection only when it was opened fresh for the call;
 * a connection reused from a SessionContext stays open under the context's ownership.
 */
type SshHandle = Disposable & { ssh: NodeSSH };

export class SessionContext implements Disposable {
    private sshConn?: NodeSSH;
    private connectPromise?: Promise<NodeSSH>;

    constructor(private session: SshSession) {}

    public [Symbol.dispose](): void {
        if (this.sshConn) {
            this.sshConn.dispose();
        } else if (this.connectPromise) {
            // A connect is still in flight (e.g. dispose ran while concurrent calls were still
            // establishing it). Dispose must stay synchronous, so dispose it once it settles
            // instead of leaking the socket; swallow a failed connect since there's nothing to
            // clean up in that case.
            this.connectPromise.then(
                (ssh) => ssh.dispose(),
                () => {},
            );
        }
    }

    public get ISshSession(): ISshSession {
        return this.session.ISshSession;
    }

    /**
     * Connects (or reuses an existing connection) and returns a disposable handle. Acquire it
     * with a `using` declaration: a raw SshSession opens a fresh connection that the handle
     * closes on dispose, while a SessionContext is reused and its disposal is left to the
     * context's own owner.
     */
    public static async acquire(session: SshSession | SessionContext): Promise<SshHandle> {
        if (session instanceof SshSession) {
            const ssh = new NodeSSH();
            await ssh.connect(ZSshUtils.buildSshConfig(session) as Config);
            return { ssh, [Symbol.dispose]: () => ssh.dispose() };
        }
        session.connectPromise ??= (async () => {
            const ssh = new NodeSSH();
            await ssh.connect(ZSshUtils.buildSshConfig(session.session) as Config);
            session.sshConn = ssh; // enables synchronous access from [Symbol.dispose]
            return ssh;
        })();
        return { ssh: await session.connectPromise, [Symbol.dispose]: () => {} };
    }
}
