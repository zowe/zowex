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

import { ProfileInfo } from "@zowe/imperative";
import { SshSession, ZSshClient } from "./src";

(async () => {
    const profInfo = new ProfileInfo("zowe");
    await profInfo.readProfilesFromDisk();
    const sshProfAttrs = profInfo.getDefaultProfile("ssh");
    const sshMergedArgs = profInfo.mergeArgsForProfile(sshProfAttrs, { getSecureVals: true });
    const session = new SshSession(ProfileInfo.initSessCfg(sshMergedArgs.knownArgs));
    const serverPathArg = sshMergedArgs.knownArgs.find((arg) => arg.argName === "serverPath");
    using client = await ZSshClient.create(session, { serverPath: serverPathArg?.argValue as string });
    const testUsers = process.argv.slice(2);
    if (!testUsers.length) console.error("No user IDs specified");
    for (const user of testUsers) {
        console.time(`listDatasets:${user}`);
        const response = await client.ds.listDatasets({ pattern: `${user}.**` });
        console.timeEnd(`listDatasets:${user}`);
        console.dir(response.items.map((item) => item.name));
    }
    for (const user of testUsers) {
        console.time(`listFiles:${user}`);
        const response = await client.uss.listFiles({ fspath: `/u/users/${user}` });
        console.timeEnd(`listFiles:${user}`);
        console.dir(response.items.map((item) => item.name));
    }
    for (const user of testUsers) {
        console.time(`listJobs:${user}`);
        const response = await client.jobs.listJobs({ owner: user });
        console.timeEnd(`listJobs:${user}`);
        console.dir(response.items.map((item) => item.id));
    }
})().catch((err) => {
    console.error(err.stack);
    process.exit(1);
});
