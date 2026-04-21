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

export default class ListUssFilesHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<uss.ListFilesResponse> {
        // const directory = UssUtils.normalizeUnixPath(params.arguments.directory);
        const directory = params.arguments.directory;
        const response = await client.uss.listFiles({
            fspath: directory,
            all: params.arguments.all,
            long: params.arguments.long,
            depth: params.arguments.depth,
        });
        params.response.data.setObj(response);
        if (params.arguments.long) {
            params.response.format.output({
                output: response.items,
                format: "table",
                fields: ["filetag", "mode", "links", "user", "group", "size", "mtime", "name"],
            });
        } else {
            params.response.format.output({
                output: response.items,
                format: "table",
                fields: ["name"],
            });
        }
        return response;
    }
}
