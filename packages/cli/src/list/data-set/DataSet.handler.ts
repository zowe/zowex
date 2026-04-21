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

import { type IHandlerParameters, ImperativeError, TextUtils } from "@zowe/imperative";
import type { Dataset, ds, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";

export default class ListDataSetsHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.ListDatasetsResponse> {
        let response: ds.ListDatasetsResponse;
        try {
            response = await client.ds.listDatasets({
                pattern: params.arguments.pattern,
                attributes: params.arguments.attributes,
            });
        } catch (err) {
            const errText = (err instanceof ImperativeError ? err.additionalDetails : err.toString()).replace(
                "Error: ",
                "",
            );
            params.response.data.setExitCode(1);
            params.response.console.errorHeader(TextUtils.chalk.red("Response from Service"));
            params.response.console.error(errText);
            params.response.data.setMessage(errText);
            return { success: false, items: [], returnedRows: 0 };
        }

        // Sort object keys by "name" first, then alphabetically
        response.items = response.items.map((item) => {
            const sortedKeys = (Object.keys(item) as Array<keyof Dataset>).sort((a, b) =>
                a === "name" ? -1 : b === "name" ? 1 : a.localeCompare(b),
            );
            return sortedKeys.reduce((obj, key) => {
                // biome-ignore lint/performance/noAccumulatingSpread: Spreading ds attrs is safe
                return { ...obj, [key]: item[key] } as Dataset;
            }, {} as Dataset);
        });

        params.response.data.setMessage(
            "Successfully listed %d matching data sets for pattern '%s'",
            response.returnedRows,
            params.arguments.pattern,
        );
        params.response.format.output({
            output: response.items.map((item) => ({
                ...item,
                migrated: item.migrated ? "YES" : "NO",
                volser: item.multivolume ? `${item.volser}+` : item.volser,
            })),
            format: "table",
            fields: [
                "name",
                "volser",
                "devtype",
                "dsorg",
                "recfm",
                "lrecl",
                "blksize",
                "primary",
                "secondary",
                "dsntype",
                "migrated",
            ],
            header: true,
        });
        return response;
    }
}
