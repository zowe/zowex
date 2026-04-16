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
import { B64String, type jobs, type ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class ViewJobJclHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<jobs.ReadSpoolResponse> {
        const response = await client.jobs.readSpool({
            spoolId: params.arguments.dsnKey,
            jobId: params.arguments.jobId,
            encoding: params.arguments.encoding,
            localEncoding: params.arguments.localEncoding,
        });
        const content = B64String.decode(response.data);
        params.response.data.setMessage(
            "Successfully downloaded %d bytes of content from %s",
            content.length,
            params.arguments.jobId,
        );
        params.response.data.setObj(content);
        params.response.console.log(content);
        return response;
    }
}
