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

import { imperative, ZoweVsCodeExtension } from "@zowe/zowe-explorer-api";
import * as vscode from "vscode";
import { SshCommandApi, SshJesApi, SshMvsApi, SshUssApi } from "./api";
import { watchNativeSshSetting } from "./NativeSshHelper";
import { SshClientCache } from "./SshClientCache";
import { registerSshErrorCorrelations } from "./SshErrorCorrelations";
import { initLogger, registerCommands } from "./Utilities";

// This method is called when your extension is activated
export function activate(context: vscode.ExtensionContext) {
    initLogger(context);
    const zoweExplorerApi = ZoweVsCodeExtension.getZoweExplorerApi();
    if (zoweExplorerApi == null) {
        const errMsg =
            "Could not access Zowe Explorer API. Please check that the latest version of Zowe Explorer is installed.";
        imperative.Logger.getAppLogger().fatal(errMsg);
        vscode.window.showErrorMessage(errMsg);
        return;
    }

    context.subscriptions.push(...registerCommands(context));
    context.subscriptions.push(SshClientCache.inst);
    context.subscriptions.push(watchNativeSshSetting(context));
    zoweExplorerApi.registerMvsApi(new SshMvsApi(zoweExplorerApi.getDataSetAttrProvider?.()));
    zoweExplorerApi.registerUssApi(new SshUssApi());
    zoweExplorerApi.registerJesApi(new SshJesApi());
    zoweExplorerApi.registerCommandApi(new SshCommandApi());

    registerSshErrorCorrelations();

    zoweExplorerApi.getExplorerExtenderApi().reloadProfiles("ssh");
}

// This method is called when your extension is deactivated
export function deactivate() {}
