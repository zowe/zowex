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

import { statSync, unlinkSync } from "node:fs";
import { parse } from "node:path";
import { promisify } from "node:util";
import { NodeSSH, type Config as NodeSSHConfig } from "node-ssh";
import { ZSshUtils } from "zowex-sdk/src";
import * as utils from "./utils";
const userConfig = require("./config.json");
const testPrefix = parse(__filename).name;

async function main() {
    // Set up
    const sshSession = ZSshUtils.buildSession(utils.getSshConfig());
    const sshClient = new NodeSSH();
    await sshClient.connect(ZSshUtils.buildSshConfig(sshSession) as NodeSSHConfig);
    const { localFile, remoteFile, tempFile } = utils.getFilenames(userConfig);
    const sftp = await sshClient.requestSFTP();

    for (let i = 0; i < userConfig.testCount; i++) {
        // 1. Upload
        console.time(`${testPrefix}:upload`);
        await promisify(sftp.fastPut.bind(sftp))(localFile, remoteFile);
        console.timeEnd(`${testPrefix}:upload`);

        // 2. Download
        console.time(`${testPrefix}:download`);
        await promisify(sftp.fastGet.bind(sftp))(remoteFile, tempFile);
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
    sshClient.dispose();
    unlinkSync(tempFile);
}

main();
