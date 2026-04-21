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
import { B64String, type uss, type ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class ViewUssFileHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<uss.ReadFileResponse> {
        const response = await client.uss.readFile({
            fspath: params.arguments.filePath,
            encoding: params.arguments.binary ? "binary" : params.arguments.encoding,
            localEncoding: params.arguments.localEncoding,
        });
        const content = B64String.decode(response.data);
        params.response.data.setMessage(
            "Successfully downloaded %d bytes of content from %s",
            content.length,
            params.arguments.filePath,
        );
        params.response.data.setObj(content);
        params.response.console.log(content);
        return response;
    }
}
