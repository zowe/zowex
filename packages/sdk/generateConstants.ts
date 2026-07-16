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

import * as fs from "node:fs";
import * as path from "node:path";

const main = () => {
    const constantsClassTemplate = `
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
// Generated via generateConstants.ts 
export const BUNDLED_SSH_SERVER_VERSION = "{{version}}";
`;
    try {
        const packageJsonContent = fs.readFileSync(path.resolve(__dirname, "package.json"));
        const packageJsonObj = JSON.parse(packageJsonContent.toString());

        if (!packageJsonObj.version) {
            throw new Error("No version field found in package.json");
        }
        const output = constantsClassTemplate.replace("{{version}}", packageJsonObj.version);
        const outputPath = path.resolve(__dirname, "src", "ZSshConstants.ts");
        fs.writeFileSync(outputPath, output);
    } catch (e) {
        console.error(`Encountered an error trying to write out ZSshConstants.ts: ${e}`);
    }
};
main();
