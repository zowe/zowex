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
import type { certificates, ZSshClient } from "@zowe/zowex-for-zowe-sdk";
import { SshBaseHandler } from "../../../SshBaseHandler";

export default class ListRingsHandler extends SshBaseHandler {
    public async processWithClient(
        params: IHandlerParameters,
        client: ZSshClient,
    ): Promise<certificates.ListRingsResponse> {
        const { owner, keyring } = params.arguments;
        const response = await client.certificates.listRings({ owner, keyring });

        params.response.data.setMessage("Listed %d key ring(s) for %s", response.returnedRows, owner);

        // Flatten rings + their certificates into a table (one row per certificate).
        const rows: Array<{ ring: string; label: string; certOwner: string }> = [];
        for (const r of response.items) {
            if (r.certificates.length === 0) {
                rows.push({ ring: `${r.owner}/${r.name}`, label: "", certOwner: "" });
            } else {
                for (const c of r.certificates) {
                    rows.push({ ring: `${r.owner}/${r.name}`, label: c.label, certOwner: c.owner });
                }
            }
        }
        params.response.format.output({
            output: rows,
            format: "table",
            fields: ["ring", "label", "certOwner"],
            header: true,
        });
        if (response.warning) {
            params.response.console.log("Note: %s", response.warning);
        }
        return response;
    }
}
