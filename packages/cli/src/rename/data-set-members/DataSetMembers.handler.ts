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
import type { ds, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class RenameMemberHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.RenameMemberResponse> {
        const dsname = params.arguments.dsname;
        const memberBefore = params.arguments.memberBefore;
        const memberAfter = params.arguments.memberAfter;

        const response = await client.ds.renameMember({ dsname, memberBefore, memberAfter });

        const message = `Dataset member "${memberBefore}" renamed to "${memberAfter}"`;
        params.response.data.setMessage(message);
        params.response.data.setObj(response);
        if (response.success) {
            params.response.console.log(message);
        }
        return response;
    }
}
