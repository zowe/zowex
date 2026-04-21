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

        sshClient.exec(
            remoteBinary,
            { pty: { modes: { ECHO: 0, ECHONL: 0, ICANON: 0, IEXTEN: 0, ISIG: 0 } } },
            async (err, stream) => {
                if (err) {
                    console.error("Error starting SSH shell:", err);
                    sshClient.end();
                    return;
                }

                stream.stderr.on("data", (data: Buffer) => {
                    console.error("STDERR:", data.toString());
                });
                stream.stdout.on("data", (data: Buffer) => {
                    console.error("STDOUT:", data);
                    require("fs").writeFileSync("stdout.txt", data);
                });
                stream.on("error", (error: Error) => {
                    console.error("Stream error:", error);
                    sshClient.end();
                });
                stream.on("close", (code: number, signal: string) => {
                    console.log(`Stream closed with code: ${code}, signal: ${signal}`);
                    sshClient.end();
                });
                await new Promise((resolve) => setTimeout(resolve, 2000));
                stream.stdin.write(Buffer.from([...Array(256).keys()]));
                setTimeout(() => sshClient.end(), 5000);
            },
        );
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
