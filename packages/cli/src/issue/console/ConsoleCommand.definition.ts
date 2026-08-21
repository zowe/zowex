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

export const ConsoleCommand: ICommandDefinition = {
    handler: `${__dirname}/ConsoleCommand.handler`,
    description: "Issue a console command",
    type: "command",
    name: "console-command",
    aliases: ["console", "cc"],
    summary: "Issue a console command",
    examples: [
        {
            description: "Issue a console command",
            options: '"D IPLINFO"',
        },
    ],
    positionals: [
        {
            name: "command",
            description: "The command to issue.",
            type: "string",
            required: true,
        },
    ],
    options: [
        {
            name: "console-name",
            aliases: ["cn"],
            description:
                "Specify the console name of the console to issue the command to. Defaults to the current user ID.",
            type: "string",
            required: false,
        },
    ],
    profile: { optional: ["ssh"] },
};
