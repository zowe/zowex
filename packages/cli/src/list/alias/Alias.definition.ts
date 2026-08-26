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

export const ListAliasDefinition: ICommandDefinition = {
    handler: `${__dirname}/Alias.handler`,
    type: "command",
    name: "alias",
    aliases: ["a"],
    summary: "Resolve a data set alias",
    description:
        "Resolve a data set alias to find the target data set it points to. " +
        "Uses IDCAMS LISTCAT to look up the alias in the catalog and return the associated data set name.",
    examples: [
        {
            description: `Resolve the alias "SHARE.ALIAS.NAME" to find its target data set`,
            options: `"SHARE.ALIAS.NAME"`,
        },
    ],
    positionals: [
        {
            name: "aliasName",
            description: "The name of the data set alias that you want to resolve",
            type: "string",
            required: true,
        },
    ],
    options: [],
    profile: { optional: ["ssh"] },
};
