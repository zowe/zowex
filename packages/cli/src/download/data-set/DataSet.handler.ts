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

import * as fs from "node:fs";
import * as path from "node:path";
import type { IHandlerParameters } from "@zowe/imperative";
import { IO } from "@zowe/imperative";
import type { ds, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class DownloadDataSetHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.ReadDatasetResponse> {
        const match = params.arguments.dataSet.match(/\(([^)]+)\)/);
        const localFilePath: string =
            params.arguments.file ??
            path.join(
                params.arguments.directory ?? process.cwd(),
                `${match ? match[1] : params.arguments.dataSet}.txt`,
            );
        IO.createDirsSyncFromFilePath(localFilePath);

        params.response.console.log(
            "Downloading data set '%s' to local file '%s'",
            params.arguments.dataSet,
            localFilePath,
        );
        const response = await client.ds.readDataset({
            stream: () => fs.createWriteStream(localFilePath),
            dsname: params.arguments.dataSet,
            encoding: params.arguments.binary ? "binary" : params.arguments.encoding,
            localEncoding: params.arguments.localEncoding,
            volume: params.arguments.volumeSerial,
        });

        params.response.data.setMessage("Successfully downloaded content to %s", localFilePath);
        return response;
    }
}
