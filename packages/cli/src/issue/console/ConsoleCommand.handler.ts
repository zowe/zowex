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

import type { IHandlerParameters } from "@zowe/imperative";
import type { console, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class ConsoleCommandHandler extends SshBaseHandler {
    public async processWithClient(
        params: IHandlerParameters,
        client: ZSshClient,
    ): Promise<console.IssueConsoleCmdResponse> {
        const commandText = params.arguments.command;
        const consoleName = params.arguments.consoleName;

        const response = await client.console.issueCmd({ commandText, consoleName });

        params.response.data.setMessage("Console: %s command: %s", consoleName, commandText);
        params.response.console.log(response.data);
        return response;
    }
}
