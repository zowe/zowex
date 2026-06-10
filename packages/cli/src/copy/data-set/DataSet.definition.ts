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

export const CopyDatasetOrMemberDefinition: ICommandDefinition = {
    handler: `${__dirname}/DataSet.handler`,
    description: "Copy a data set or member",
    type: "command",
    name: "data-set",
    aliases: ["ds"],
    summary: "Copy a data set or member to a data set or member",
    examples: [
        {
            description: "Copy a data set or member",
            options: "'ibmuser.test.cntl' 'ibmuser.backup.cntl'",
        },
        {
            description: "Copy a data set with replace option",
            options: "'ibmuser.test.cntl' 'ibmuser.backup.cntl' --replace",
        },
        {
            description: "Copy a data set with overwrite option",
            options: "'ibmuser.test.cntl' 'ibmuser.backup.cntl' --overwrite",
        },
        {
            description: "Copy a member with both replace and overwrite options",
            options: "'ibmuser.test.cntl(member1)' 'ibmuser.backup.cntl(member1)' --replace --overwrite",
        },
    ],
    positionals: [
        {
            name: "fromDataset",
            description: "The data set or member to copy from",
            type: "string",
            required: true,
        },
        {
            name: "toDataset",
            description: "The data set or member to copy to",
            type: "string",
            required: true,
        },
    ],
    options: [
        {
            name: "replace",
            description: "Replace the target dataset or member if it already exists",
            type: "boolean",
            required: false,
        },
        {
            name: "overwrite",
            description: "Overwrite the target dataset or member if it already exists",
            type: "boolean",
            required: false,
        },
    ],
};
