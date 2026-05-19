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
import { SshSession } from "@zowe/zos-uss-for-zowe-sdk";
import { Constants } from "../Constants";
import { ViewSyslogDefinition } from "./view-syslog/ViewSyslog.definition";

const SystemDefinition: ICommandDefinition = {
    name: "system",
    aliases: ["sys"],
    summary: "System-level operations",
    description: "Perform system-level operations such as viewing the syslog",
    type: "group",
    children: [ViewSyslogDefinition],
    passOn: [
        {
            property: "options",
            value: [...SshSession.SSH_CONNECTION_OPTIONS, Constants.OPT_SERVER_PATH],
            merge: true,
            ignoreNodes: [{ type: "group" }],
        },
    ],
};

export = SystemDefinition;
