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

export default class ListJobsHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<jobs.ListJobsResponse> {
        const response = await client.jobs.listJobs({
            owner: params.arguments.owner,
            prefix: params.arguments.prefix,
            status: params.arguments.status,
        });
        params.response.data.setMessage("Successfully listed %d matching jobs", response.items.length);
        params.response.format.output({
            output: response.items,
            format: "table",
            fields: ["id", "name", "owner", "status", "retcode"],
        });
        return response;
    }
}
