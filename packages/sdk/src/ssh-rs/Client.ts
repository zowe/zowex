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
import {
    type AuthenticatedSSHClient,
    type AuthFailure,
    KeyPair,
    SSHClient,
    type SshPublicKey,
    SshTransport,
} from "russh";
import type { Subscription } from "rxjs";
import type { ConnectConfig } from "ssh2";
import { ClientChannel } from "./ClientChannel";

type DebugFn = (message: string) => void;
const noop: DebugFn = () => {};

export class Client extends EventEmitter {
    private sshClient: AuthenticatedSSHClient | null = null;
    private disconnectSub: Subscription | null = null;
    private closed = false;
    private debug: DebugFn = noop;

    public connect(config: ConnectConfig): void {
        if (this.closed) {
            throw new Error("Cannot reuse a closed SSH client — create a new instance");
        }
        this.debug = config.debug ?? noop;
        this.connectAsync(config).catch((err) => {
            this.emit("error", err);
        });
    }

    public exec(command: string, callback: (err: Error | undefined, channel: ClientChannel) => void): void {
        if (!this.sshClient) {
            callback(new Error("Not connected"), undefined!);
            return;
        }
        this.execAsync(command)
            .then((ch) => callback(undefined, ch))
            .catch((err) => callback(err, undefined!));
    }

    public end(): void {
        this.debug("russh: end() called, disconnecting");
        this.cleanup();
        this.emitClose();
    }

    private cleanup(): void {
        this.disconnectSub?.unsubscribe();
        this.disconnectSub = null;
        const client = this.sshClient;
        this.sshClient = null;
        if (client) {
            client.disconnect().catch(() => {});
        }
    }

    private emitClose(): void {
        if (this.closed) return;
        this.closed = true;
        this.debug("russh: emitting close");
        this.emit("close");
    }

    private async connectAsync(config: ConnectConfig): Promise<void> {
        const address = `${config.host ?? "localhost"}:${config.port ?? 22}`;
        this.debug(`russh: opening TCP connection to ${address}`);
        const transport = await SshTransport.newSocket(address);

        this.debug("russh: starting SSH handshake");
        const unauthClient = await SSHClient.connect(
            transport,
            async (key: SshPublicKey) => {
                this.debug(`russh: server key ${key.algorithm()} fingerprint=${key.fingerprint()}`);
                return true;
            },
            {
                connectionTimeoutSeconds: config.readyTimeout != null ? config.readyTimeout / 1000 : undefined,
                keepaliveIntervalSeconds:
                    config.keepaliveInterval != null ? config.keepaliveInterval / 1000 : undefined,
            },
        );
        this.debug("russh: SSH handshake complete");

        this.disconnectSub = unauthClient.disconnect$.subscribe(() => {
            this.debug("russh: disconnect event received from server");
            this.cleanup();
            this.emitClose();
        });

        let authClient: AuthenticatedSSHClient;
        try {
            const username = config.username ?? "root";
            authClient = await this.authenticate(unauthClient, username, config);
        } catch (err) {
            this.debug("russh: post-handshake error, tearing down connection");
            this.disconnectSub.unsubscribe();
            this.disconnectSub = null;
            unauthClient.disconnect().catch(() => {});
            throw err;
        }

        this.sshClient = authClient;
        this.debug("russh: emitting ready");
        this.emit("ready");
    }

    private async authenticate(
        unauthClient: SSHClient,
        username: string,
        config: ConnectConfig,
    ): Promise<AuthenticatedSSHClient> {
        if (config.privateKey) {
            this.debug(`russh: authenticating with public key for ${username}`);
            const keyData =
                typeof config.privateKey === "string"
                    ? config.privateKey
                    : Buffer.from(config.privateKey).toString("utf-8");
            const passphrase = typeof config.passphrase === "string" ? config.passphrase : undefined;
            const keyPair = await KeyPair.parse(keyData, passphrase);

            const result = await unauthClient.authenticateWithKeyPair(username, keyPair, null);
            return this.assertAuthSuccess(result, "Public key");
        }

        if (config.password) {
            this.debug(`russh: authenticating with password for ${username}`);
            const result = await unauthClient.authenticateWithPassword(username, config.password);
            return this.assertAuthSuccess(result, "Password");
        }

        throw new Error("No authentication method available: provide privateKey or password");
    }

    private assertAuthSuccess(result: AuthenticatedSSHClient | AuthFailure, method: string): AuthenticatedSSHClient {
        if ("remainingMethods" in result) {
            const remaining = (result as AuthFailure).remainingMethods.join(", ");
            throw new Error(`${method} authentication failed. Remaining methods: ${remaining}`);
        }
        this.debug(`russh: ${method.toLowerCase()} authentication succeeded`);
        return result as AuthenticatedSSHClient;
    }

    private async execAsync(command: string): Promise<ClientChannel> {
        this.debug(`russh: opening channel for exec: ${command}`);
        const newCh = await this.sshClient!.openSessionChannel();
        const channel = await this.sshClient!.activateChannel(newCh);
        await channel.requestExec(command);
        this.debug(`russh: [ch ${channel.id}] exec channel opened`);
        return new ClientChannel(channel, this.debug);
    }
}
