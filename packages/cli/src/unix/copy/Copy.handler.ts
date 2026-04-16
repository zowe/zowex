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
import type { uss, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class CopyHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<uss.CopyUssResponse> {
        const response = await client.uss.copyUss({
            recursive: params.arguments.recursive ?? false,
            followSymlinks: params.arguments.followSymlinks ?? false,
            preserveAttributes: params.arguments.preserveAttributes ?? false,
            force: params.arguments.force ?? false,
            srcFsPath: params.arguments.srcFsPath,
            dstFsPath: params.arguments.dstFsPath,
        });

        params.response.data.setMessage(
            "Successfully copied %s to %s.",
            params.arguments.srcFsPath,
            params.arguments.dstFsPath,
        );
        params.response.format.output({
            output: response,
            format: "table",
            fields: ["success", "srcFsPath", "dstFsPath"],
        });
        return response;
    }
}
