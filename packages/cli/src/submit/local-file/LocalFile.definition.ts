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

export const SubmitLocalFileDefinition: ICommandDefinition = {
    handler: `${__dirname}/LocalFile.handler`,
    description: "Submit a job from JCL written in a local file.",
    type: "command",
    name: "local-file",
    aliases: ["lf"],
    summary: "Submit a job from local files",
    examples: [
        {
            description: 'Submit a job from the file "my_jcl.txt"',
            options: '"my_jcl.txt"',
        },
    ],
    positionals: [
        {
            name: "fspath",
            description: "The path to the JCL to submit",
            type: "string",
            required: true,
        },
    ],
    options: [
        {
            name: "encoding",
            aliases: ["ec"],
            description: "The encoding of the local JCL file. Defaults to UTF-8 if not specified.",
            type: "string",
            required: false,
        },
    ],
    profile: { optional: ["ssh"] },
    outputFormatOptions: true,
};
