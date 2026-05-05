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

export const ViewSyslogDefinition: ICommandDefinition = {
    handler: `${__dirname}/ViewSyslog.handler`,
    description: "View the z/OS system log (syslog)",
    type: "command",
    name: "view-syslog",
    aliases: ["vs"],
    summary: "View system log",
    examples: [
        {
            description: "View the last 30 seconds of syslog (default)",
            options: "",
        },
        {
            description: "View the last 5 minutes of syslog",
            options: "--seconds-ago 300",
        },
        {
            description: "View syslog for a specific date and time",
            options: "--date 2026-03-23 --time 10:41:00 --max-lines 100",
        },
    ],
    options: [
        {
            name: "seconds-ago",
            aliases: ["s"],
            description:
                "Relative offset in seconds from the current z/OS time. " + "For example, 300 for the last 5 minutes.",
            type: "number",
            conflictsWith: ["date", "time"],
        },
        {
            name: "date",
            aliases: ["d"],
            description: "Start date in yyyy-mm-dd format.",
            type: "string",
        },
        {
            name: "time",
            aliases: ["t"],
            description: "Start time in hh:mm:ss format.",
            type: "string",
        },
        {
            name: "max-lines",
            aliases: ["ml"],
            description: "Maximum number of syslog lines to return (1-10000).",
            type: "number",
        },
    ],
    profile: { optional: ["ssh"] },
};
