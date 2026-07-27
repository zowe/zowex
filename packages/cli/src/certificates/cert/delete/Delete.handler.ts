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

export default class DeleteCertHandler extends SshBaseHandler {
    public async processWithClient(
        params: IHandlerParameters,
        client: ZSshClient,
    ): Promise<certificates.DeleteCertificateResponse> {
        const { owner, keyring, label, skipRefresh, database } = params.arguments;
        const response = await client.certificates.deleteCertificate({ owner, keyring, label, skipRefresh, database });

        const target = database ? "the RACF database" : `${owner}/${keyring}`;
        params.response.data.setMessage("Certificate '%s' removed from %s", label, target);
        params.response.console.log("Certificate '%s' removed from %s", label, target);
        if (response.warning) {
            params.response.console.log("Note: %s", response.warning);
        }
        return response;
    }
}
