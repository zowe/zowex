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

import { readFileSync, statSync, unlinkSync, writeFileSync } from "node:fs";
import { parse } from "node:path";
import { B64String, ZSshClient, ZSshUtils } from "zowex-sdk/src";
import * as utils from "./utils";
const userConfig = require("./config.json");
const testPrefix = parse(__filename).name;

async function main() {
    // Set up
    const sshSession = ZSshUtils.buildSession(utils.getSshConfig());
    using sshClient = await ZSshClient.create(sshSession, { serverPath: userConfig.serverPath });
    const { localFile, remoteFile, tempFile } = utils.getFilenames(userConfig);
    try {
        await sshClient.uss.createFile({ fspath: remoteFile });
    } catch (e) {}

    for (let i = 0; i < userConfig.testCount; i++) {
        // 1. Upload
        console.time(`${testPrefix}:upload`);
        await sshClient.uss.writeFile({
            data: B64String.encode(readFileSync(localFile)),
            fspath: remoteFile,
            encoding: "binary",
        });
        console.timeEnd(`${testPrefix}:upload`);

        // 2. Download
        console.time(`${testPrefix}:download`);
        const { data } = await sshClient.uss.readFile({
            fspath: remoteFile,
            encoding: "binary",
        });
        writeFileSync(tempFile, B64String.decodeBytes(data));
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
    unlinkSync(tempFile);
}

main();
