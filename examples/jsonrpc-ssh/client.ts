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

import { JSONRPCClient } from "json-rpc-2.0";
import { Client } from "ssh2";

async function main() {
    const [username, host] = process.argv[2].split("@");
    const privateKeyPath = require("os").homedir() + "/.ssh/id_rsa";
    const sshConfig = {
        host,
        port: 22,
        username,
        privateKey: require("fs").readFileSync(privateKeyPath),
    };

    const remoteBinary = process.argv[3];
    const sshClient = new Client();

    sshClient.on("ready", () => {
        console.log("SSH connection established.");

        sshClient.shell(false, async (err, stream) => {
            if (err) {
                console.error("Error starting SSH shell:", err);
                sshClient.end();
                return;
            }

            stream.write(remoteBinary + "\n");
            console.log("Remote binary started.");
            stream.stderr.on("data", (data: Buffer) => {
                console.error("STDERR:", data.toString());
            });
            stream.on("error", (error: Error) => {
                console.error("Stream error:", error);
                sshClient.end();
            });
            stream.on("close", (code: number, signal: string) => {
                console.log(`Stream closed with code: ${code}, signal: ${signal}`);
                sshClient.end();
            });

            const rpcClient = new JSONRPCClient((request) => {
                console.log("Sending request:", request);
                stream.stdin.write(JSON.stringify(request) + "\n");
            });
            stream.stdout.on("data", (data: Buffer) => {
                const responses = data.toString().trim().split("\n");
                for (const response of responses) {
                    try {
                        const parsedResponse = JSON.parse(response);
                        console.log("Received response:", parsedResponse);
                        rpcClient.receive(parsedResponse);
                    } catch (error) {
                        console.error("Invalid JSON-RPC response:", error);
                    }
                }
            });
            await Promise.all([
                rpcClient.request("Arith.Add", { a: 5, b: 7 }),
                rpcClient.request("Arith.Add", { a: 6, b: 8 }),
                rpcClient.request("Arith.Add", { a: 7, b: 9 }),
            ]);
            sshClient.end();
        });
    });

    sshClient.on("error", (err: Error) => {
        console.error("SSH connection error:", err);
    });

    sshClient.on("close", () => {
        console.log("SSH connection closed.");
    });

    sshClient.connect(sshConfig);
}

main().catch((err) => {
    console.error("Error:", err);
});
