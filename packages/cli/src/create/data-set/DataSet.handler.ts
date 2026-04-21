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
import type { DatasetAttributes, ds, ZSshClient } from "zowex-sdk";
import { SshBaseHandler } from "../../SshBaseHandler";
import { CreateDefaults } from "../CreateDefaults";

export default class CreateDatasetHandler extends SshBaseHandler {
    public async processWithClient(params: IHandlerParameters, client: ZSshClient): Promise<ds.CreateDatasetResponse> {
        const dsname = params.arguments.name;
        // Defaults such that it can be assigned to attributes in createDataset without a partial
        let attributes: DatasetAttributes = { lrecl: 80, primary: 1 };

        const attrKeys: (keyof DatasetAttributes)[] = [
            "alcunit",
            "blksize",
            "dirblk",
            "dsorg",
            "primary",
            "recfm",
            "lrecl",
            "dataclass",
            "unit",
            "dsntype",
            "mgntclass",
            "dsname",
            "avgblk",
            "secondary",
            "size",
            "storclass",
            "vol",
        ];

        switch (params.arguments.template?.toLocaleUpperCase()) {
            case "PARTITIONED":
                attributes = CreateDefaults.DATA_SET.PARTITIONED;
                break;
            case "SEQUENTIAL":
                attributes = CreateDefaults.DATA_SET.SEQUENTIAL;
                break;
            case "DEFAULT":
                attributes = CreateDefaults.DATA_SET.SEQUENTIAL;
                break;
            case "CLASSIC":
                attributes = CreateDefaults.DATA_SET.CLASSIC;
                break;
            case "C":
                attributes = CreateDefaults.DATA_SET.C;
                break;
            case "BINARY":
                attributes = CreateDefaults.DATA_SET.BINARY;
                break;
            default:
                break;
        }

        const args = params.arguments as Record<string, unknown>;

        for (const key of attrKeys) {
            const value = args[key];
            if (value !== undefined) {
                attributes = { ...attributes, [key]: value };
            }
        }

        const response = await client.ds.createDataset({ dsname, attributes });

        const dsMessage = `Dataset "${dsname}" created`;
        params.response.data.setMessage(dsMessage);
        params.response.data.setObj(response);
        if (response.success) {
            params.response.console.log(dsMessage);
        }
        return response;
    }
}
