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

export const SubmitStdinDefinition: ICommandDefinition = {
    handler: `${__dirname}/Stdin.handler`,
    description: "Submit a job from JCL written to the standard input (stdin) of this process.",
    type: "command",
    name: "stdin",
    aliases: ["si", "in"],
    summary: "Submit a job from stdin",
    examples: [
        {
            description: "Submit a job from stdin, redirecting the contents of my_jcl.txt in.",
            options: " < my_jcl.txt",
        },
        {
            description: 'Submit a job from the local file "my_build_jcl.txt" and wait for job complete.',
            options: '"my_build_jcl.txt" --wait 5,12',
        },
    ],
    options: [
        {
            name: "encoding",
            aliases: ["ec"],
            description: "The encoding of the JCL content being piped in. Defaults to UTF-8 if not specified.",
            type: "string",
            required: false,
        },
        // {
        //     name: "wait",
        //     aliases: ["w"],
        //     description:
        //         "Specify job query interval and max times of querying job status. " +
        //         "The format of this option is comma-separated numeric values. For example, " +
        //         "'5,12' means queries job status every 5 seconds for 12 times at most.",
        //     type: "string",
        //     required: false,
        //     conflictsWith: ["wait-for-active", "wait-for-output"],
        // },
        // {
        //     name: "wait-for-output",
        //     aliases: ["wfo"],
        //     description: "Wait for the job to enter OUTPUT status before completing the command.",
        //     type: "boolean",
        //     required: false,
        //     conflictsWith: ["wait-for-active", "wait"],
        // },
        // {
        //     name: "wait-for-active",
        //     aliases: ["wfa"],
        //     description: "Wait for the job to enter ACTIVE status before completing the command.",
        //     type: "boolean",
        //     required: false,
        //     conflictsWith: ["wait-for-output", "wait"],
        // },
    ],
    profile: { optional: ["ssh"] },
    outputFormatOptions: true,
};
