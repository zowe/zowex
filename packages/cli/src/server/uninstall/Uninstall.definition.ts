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

export const ServerUninstallDefinition: ICommandDefinition = {
    handler: `${__dirname}/Uninstall.handler`,
    type: "command",
    name: "uninstall",
    aliases: ["rm"],
    summary: "Uninstall Zowe Remote SSH server on z/OS host",
    description: "Uninstall Zowe Remote SSH server on z/OS host",
    examples: [
        {
            description: "Uninstall Zowe Remote SSH server from the default location",
            options: "",
        },
        {
            description: 'Uninstall Zowe Remote SSH server from the directory "/tmp/zowe-server"',
            options: '--server-path "/tmp/zowe-server"',
        },
    ],
    profile: { optional: ["ssh"] },
};
