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
import { B64String, type jobs, type ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class DownloadJobJclHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<jobs.ReadSpoolResponse> {
        const localFilePath: string = path.join(
            params.arguments.directory ?? process.cwd(),
            `${params.arguments.jobId}.txt`,
        );
        params.response.console.log(
            "Downloading spool '%s' from job ID '%s' to local file '%s'",
            params.arguments.dsnKey,
            params.arguments.jobId,
            localFilePath,
        );

        const response = await client.jobs.readSpool({
            spoolId: params.arguments.dsnKey,
            jobId: params.arguments.jobId,
            encoding: params.arguments.encoding,
            localEncoding: params.arguments.localEncoding,
        });

        const content = B64String.decode(response.data);
        IO.createDirsSyncFromFilePath(localFilePath);
        fs.writeFileSync(localFilePath, content, params.arguments.binary ? "binary" : "utf8");

        params.response.data.setMessage("Successfully downloaded content to %s", localFilePath);
        return response;
    }
}
