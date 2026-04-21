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
import type { tso, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class TsoCommandHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<tso.IssueTsoCmdResponse> {
        const commandText = params.arguments.command;
        const response = await client.tso.issueCmd({ commandText });

        params.response.data.setMessage("TSO response: %s", commandText);
        params.response.console.log(response.data);
        return response;
    }
}
