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

export default class CountRingHandler extends SshBaseHandler {
    public async processWithClient(
        params: IHandlerParameters,
        client: ZSshClient,
    ): Promise<certificates.CountRingResponse> {
        const { owner, keyring } = params.arguments;
        const response = await client.certificates.countRing({ owner, keyring });

        params.response.data.setMessage("%d certificate(s) in %s/%s", response.count, owner, keyring);
        params.response.console.log("%d certificate(s) in %s/%s", response.count, owner, keyring);
        return response;
    }
}
