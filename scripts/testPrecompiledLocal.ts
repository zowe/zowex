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

import * as childProcess from "node:child_process";

/**
 * Local convenience runner that mirrors what the CI workflow does on the
 * precompiled (no-swig) path: fetch the latest "Precompiled Python bindings"
 * bundle posted to a PR, apply it to z/OS, then run the Python tests against it.
 *
 * Usage: npm run z:test:python:precompiled -- <PR_NUMBER>
 */

/**
 * Runs another npm script in this package. Spawns via the absolute node path
 * (process.execPath) and npm's own entry point (npm_execpath) rather than a
 * bare command name, so execution never depends on a hijackable PATH.
 */
function npmRun(step: string, stepArgs: string[] = []) {
    const npmExec = process.env.npm_execpath;
    if (!npmExec) {
        console.error("This script must be run via npm, e.g. `npm run z:test:python:precompiled -- <PR_NUMBER>`.");
        process.exit(1);
    }
    const args = [npmExec, "run", step, ...(stepArgs.length > 0 ? ["--", ...stepArgs] : [])];
    childProcess.execFileSync(process.execPath, args, { stdio: "inherit" });
}

function main() {
    const prNumber = process.argv[2];
    if (!prNumber || !/^\d+$/.test(prNumber)) {
        console.error("Usage: npm run z:test:python:precompiled -- <PR_NUMBER>");
        process.exit(1);
    }

    try {
        npmRun("z:fetch:python", [prNumber]);
    } catch {
        console.error(`\nNo precompiled bindings could be fetched for PR #${prNumber}; nothing to test.`);
        process.exit(1);
    }

    npmRun("z:apply:python");
    npmRun("z:test:python");
    console.log(`\n✅ Tested precompiled bindings from PR #${prNumber} on z/OS.`);
}

main();
