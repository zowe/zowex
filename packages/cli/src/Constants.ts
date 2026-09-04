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
import { ZSshClient } from "@zowe/zowex-for-zowe-sdk";

// biome-ignore lint/complexity/noStaticOnlyClass: Constants class has static properties
export class Constants {
    public static readonly OPT_IDENTITY_AGENT: ICommandOptionDefinition = {
        name: "identity-agent",
        aliases: ["agent"],
        description:
            "Path to the ssh-agent's UNIX socket (or named pipe on Windows) to use for ssh-agent-based " +
            "user authentication, or 'pageant' to use Pageant on Windows. Takes priority over a private key " +
            "or password configured on the profile.",
        type: "string",
        required: false,
    };

    public static readonly OPT_SERVER_PATH: ICommandOptionDefinition = {
        name: "server-path",
        aliases: ["sp"],
        description: `The path to the deployed Zowe Remote SSH server instance. Defaults to '${ZSshClient.DEFAULT_SERVER_PATH}'.`,
        type: "string",
        required: false,
    };

    /**
     * Options appended to every zowex command definition on top of SshSession.SSH_CONNECTION_OPTIONS.
     */
    public static readonly ZSSH_EXTRA_OPTIONS: ICommandOptionDefinition[] = [
        Constants.OPT_IDENTITY_AGENT,
        Constants.OPT_SERVER_PATH,
    ];
}
