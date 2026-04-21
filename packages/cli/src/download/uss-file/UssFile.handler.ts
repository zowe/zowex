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
import { type IHandlerParameters, IO, type ITaskWithStatus, TaskStage } from "@zowe/imperative";
import type { uss, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class DownloadUssFileHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<uss.ReadFileResponse> {
        const task: ITaskWithStatus = {
            percentComplete: 0,
            statusMessage: "Downloading...",
            stageName: TaskStage.IN_PROGRESS,
        };
        const baseName = path.posix.basename(params.arguments.filePath);
        const localFilePath: string =
            params.arguments.file ?? path.join(params.arguments.directory ?? process.cwd(), baseName);
        IO.createDirsSyncFromFilePath(localFilePath);

        params.response.progress.startBar({ task });
        const response = await client.uss.readFile(
            {
                stream: () => fs.createWriteStream(localFilePath),
                fspath: params.arguments.filePath,
                encoding: params.arguments.binary ? "binary" : params.arguments.encoding,
                localEncoding: params.arguments.localEncoding,
            },
            (percent: number): void => {
                task.percentComplete = percent;
            },
        );

        task.stageName = TaskStage.COMPLETE;
        params.response.progress.endBar();

        params.response.data.setMessage("Successfully downloaded content to %s", localFilePath);
        return response;
    }
}
