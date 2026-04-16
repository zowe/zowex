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

export default class SubmitDsHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<jobs.SubmitJobResponse> {
        const dsname = params.arguments.name;
        const response = await client.jobs.submitJob({ dsname });

        const msg = `Job submitted: ${response.jobId}`;
        params.response.data.setMessage(msg);
        params.response.data.setObj(response);
        if (response.success) {
            params.response.console.log(msg);
        }
        return response;
    }
}
