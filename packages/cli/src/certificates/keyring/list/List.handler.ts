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

export default class ListKeyringHandler extends SshBaseHandler {
    public async processWithClient(
        params: IHandlerParameters,
        client: ZSshClient,
    ): Promise<certificates.ListCertificatesResponse> {
        const { owner, keyring } = params.arguments;
        const response = await client.certificates.listCertificates({
            owner,
            keyring,
            label: params.arguments.label,
            usage: params.arguments.usage,
            labelOnly: params.arguments.labelOnly,
            ownerOnly: params.arguments.ownerOnly,
            maxEntries: params.arguments.maxEntries,
        });

        params.response.data.setMessage(
            "Listed %d certificate(s) in key ring '%s/%s'",
            response.returnedRows,
            owner,
            keyring,
        );

        if (params.arguments.labelOnly) {
            params.response.format.output({
                output: response.items.map((c) => ({ label: c.label })),
                format: "list",
                fields: ["label"],
            });
        } else if (params.arguments.ownerOnly) {
            params.response.format.output({
                output: response.items.map((c) => ({ owner: c.owner })),
                format: "list",
                fields: ["owner"],
            });
        } else {
            params.response.format.output({
                output: response.items.map((c) => ({ ...c, default: c.default ? "YES" : "NO" })),
                format: "table",
                fields: ["label", "owner", "usage", "status", "default"],
                header: true,
            });
        }

        if (response.moreAvailable) {
            params.response.console.log(
                "\nNote: more certificates are available. Use --max-entries 0 (or a higher limit) to see them.",
            );
        }
        return response;
    }
}
