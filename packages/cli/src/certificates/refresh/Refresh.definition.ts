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

import type { ICommandDefinition } from "@zowe/imperative";

export const RefreshDefinition: ICommandDefinition = {
    handler: `${__dirname}/Refresh.handler`,
    type: "command",
    name: "refresh",
    summary: "Refresh the DIGTCERT class",
    description: "Refresh the DIGTCERT class so that certificate and key ring changes take effect.",
    examples: [{ description: "Refresh the DIGTCERT class", options: "" }],
    profile: { optional: ["ssh"] },
};
