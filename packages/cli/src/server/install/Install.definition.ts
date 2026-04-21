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

export const ServerInstallDefinition: ICommandDefinition = {
    handler: `${__dirname}/Install.handler`,
    type: "command",
    name: "install",
    aliases: ["up"],
    summary: "Install Zowe Remote SSH server on z/OS host",
    description: "Install Zowe Remote SSH server on z/OS host",
    examples: [
        {
            description: "Install Zowe Remote SSH server in the default location",
            options: "",
        },
        {
            description: 'Install Zowe Remote SSH server in the directory "/tmp/zowe-server"',
            options: '--server-path "/tmp/zowe-server"',
        },
    ],
    profile: { optional: ["ssh"] },
};
