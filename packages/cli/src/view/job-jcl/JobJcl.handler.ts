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
import type { jobs, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class ViewJobJclHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<jobs.GetJclResponse> {
        const response = await client.jobs.getJcl({
            jobId: params.arguments.jobId,
        });
        params.response.data.setMessage(
            "Successfully downloaded %d bytes of content from %s",
            response.data.length,
            params.arguments.jobId,
        );
        params.response.data.setObj(response.data);
        params.response.console.log(response.data);
        return response;
    }
}
