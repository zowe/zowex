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
    handler: `${__dirname}/CopyDatasetOrMember.handler`,
    description: "Copy a data set or member",
    type: "command",
    name: "data-set",
    aliases: ["ds"],
    summary: "Copy a data set or member to a data set or member",
    examples: [
        {
            description: "Copy a data set or member",
            options: "'ibmuser.test.cntl",
        },
    ],
};
