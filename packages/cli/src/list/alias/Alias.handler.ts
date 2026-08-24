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

import { type IHandlerParameters, ImperativeError, TextUtils } from "@zowe/imperative";
import type { ds, ZSshClient } from "@zowe/zowex-for-zowe-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class ListAliasHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.ResolveDsAliasResponse> {
        let response: ds.ResolveDsAliasResponse;
        try {
            response = await client.ds.resolveDsAlias({
                dsname: params.arguments.aliasName,
            });
        } catch (err) {
            const errText = err.toString().replace("Error: ", "");
            params.response.data.setExitCode(1);
            params.response.console.errorHeader(TextUtils.chalk.red("Response from Service"));
            params.response.console.error(errText);
            params.response.data.setMessage(errText);
            return { success: false, targetDsn: undefined };
        }
        const msg = `Alias '${params.arguments.aliasName}' resolved to data set '${response.targetDsn}'.`;
        params.response.data.setMessage(msg);
        params.response.console.log(msg);
        return response;
    }
}
