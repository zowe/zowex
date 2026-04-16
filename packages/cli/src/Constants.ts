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

import type { ICommandOptionDefinition } from "@zowe/imperative";
import { ZSshClient } from "zowex-sdk";

// biome-ignore lint/complexity/noStaticOnlyClass: Constants class has static properties
export class Constants {
    public static readonly OPT_SERVER_PATH: ICommandOptionDefinition = {
        name: "server-path",
        aliases: ["sp"],
        description: `The path to the deployed Zowe Remote SSH server instance. Defaults to '${ZSshClient.DEFAULT_SERVER_PATH}'.`,
        type: "string",
        required: false,
    };
}
