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

export default class ChmodHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<uss.ChmodFileResponse> {
        const response = await client.uss.chmodFile({
            recursive: params.arguments.recursive,
            mode: params.arguments.mode,
            fspath: params.arguments.path,
        });
        params.response.data.setMessage(
            "Successfully changed permissions of %s to %s",
            params.arguments.path,
            params.arguments.mode,
        );
        params.response.format.output({
            output: response,
            format: "table",
            fields: ["success", "fspath"],
        });
        return response;
    }
}
