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
import type { system, ZSshClient } from "@zowe/zowex-for-zowe-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class ViewSyslogHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<system.ViewSyslogResponse> {
        const response = await client.system.viewSyslog({
            date: params.arguments.date,
            time: params.arguments.time,
            secondsAgo: params.arguments.secondsAgo,
            maxLines: params.arguments.maxLines,
        });

        if (response.data) {
            params.response.console.log(response.data);
        }

        const hasMoreLabel = response.hasMore ? "yes" : "no";
        params.response.console.log(
            `Start: ${response.startDate} ${response.startTime}` +
                ` | End: ${response.endDate} ${response.endTime}` +
                ` | Lines: ${response.returnedLines}` +
                ` | Has more: ${hasMoreLabel}`,
        );

        if (response.hasMore) {
            const nextMaxLines = (response.returnedLines ?? 100) * 5;
            params.response.console.log(
                "To see more, run:\n" +
                    `  zowe zssh system view-syslog --date ${response.endDate} --time ${response.endTime} --max-lines ${nextMaxLines}`,
            );
        }

        params.response.data.setMessage(
            "Syslog from %s %s to %s %s (%d lines)",
            response.startDate,
            response.startTime,
            response.endDate,
            response.endTime,
            response.returnedLines,
        );
        return response;
    }
}
