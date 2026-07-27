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

export default class SetDefaultCertHandler extends SshBaseHandler {
    public async processWithClient(
        params: IHandlerParameters,
        client: ZSshClient,
    ): Promise<certificates.SetDefaultCertificateResponse> {
        const { owner, keyring, label } = params.arguments;
        const response = await client.certificates.setDefaultCertificate({ owner, keyring, label });

        params.response.data.setMessage("Certificate '%s' set as the default in %s/%s", label, owner, keyring);
        params.response.console.log("Certificate '%s' set as the default certificate in %s/%s", label, owner, keyring);
        if (response.warning) {
            params.response.console.log("Note: %s", response.warning);
        }
        return response;
    }
}
