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
import type { IHandlerParameters } from "@zowe/imperative";
import type { ZSshClient } from "zowex-sdk";
import type { ds } from "zowex-sdk/src/doc";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class UploadFileToDataSetHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.WriteDatasetResponse> {
        const response = await client.ds.writeDataset({
            stream: () => fs.createReadStream(params.arguments.file),
            dsname: params.arguments.dataSet,
            encoding: params.arguments.binary ? "binary" : params.arguments.encoding,
            localEncoding: params.arguments.localEncoding,
            volume: params.arguments.volumeSerial,
        });
        const uploadSource = `local file '${params.arguments.file}'`;
        const successMsg = params.response.console.log(
            "Uploaded from %s to %s ",
            uploadSource,
            params.arguments.dataSet,
        );
        if (response.truncationWarning) {
            params.response.console.log("Warning: %s", response.truncationWarning);
        }
        params.response.data.setMessage(successMsg);
        return response;
    }
}
