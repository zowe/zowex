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

import { createReadStream, createWriteStream, statSync, unlinkSync } from "node:fs";
import { dirname, parse } from "node:path";
import { pipeline } from "node:stream/promises";
import { Base64Decode, Base64Encode } from "base64-stream";
import { Client } from "ssh2";
import * as utils from "./utils";
const userConfig = require("./config.json");
const testPrefix = parse(__filename).name;

async function main() {
    // Set up
    const sshConfig = utils.getSshConfig();
    const sshClient = new Client();
    const { localFile, remoteFile, tempFile } = utils.getFilenames(userConfig);
    sshClient.connect({ ...sshConfig, debug: userConfig.verboseSsh ? console.debug : undefined });
    await new Promise<void>((resolve, reject) => {
        sshClient.on("ready", resolve);
        sshClient.on("error", reject);
    });

    for (let i = 0; i < userConfig.testCount; i++) {
        // 1. Upload
        console.time(`${testPrefix}:upload`);
        const srcStream = createReadStream(localFile, { highWaterMark: userConfig.chunkSize });
        await new Promise<void>((resolve, reject) => {
            sshClient.exec(
                `${dirname(remoteFile)}/testb64 upload ${remoteFile} ${userConfig.chunkSize}`,
                (err, stream) => {
                    if (err) {
                        reject(err);
                        return;
                    }
                    pipeline(srcStream, new Base64Encode(), stream.stdin).then(resolve, reject);
                },
            );
        });
        console.timeEnd(`${testPrefix}:upload`);

        // 2. Download
        console.time(`${testPrefix}:download`);
        const destStream = createWriteStream(tempFile);
        await new Promise<void>((resolve, reject) => {
            sshClient.exec(
                `${dirname(remoteFile)}/testb64 download ${remoteFile} ${userConfig.chunkSize}`,
                (err, stream) => {
                    if (err) {
                        reject(err);
                        return;
                    }
                    pipeline(stream.stdout, new Base64Decode(), destStream).then(resolve, reject);
                },
            );
        });
        console.timeEnd(`${testPrefix}:download`);

        // 3. Verify
        const success = await utils.compareChecksums(localFile, tempFile);
        if (success) {
            console.log(`✅ Checksums match (${i + 1}/${userConfig.testCount})`);
        } else {
            console.error(`❌ Checksums do not match (${i + 1}/${userConfig.testCount}): ${statSync(tempFile).size}`);
        }
    }

    // Tear down
    sshClient.end();
    unlinkSync(tempFile);
}

main();
