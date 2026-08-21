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

import type * as common from "./common";

export interface IssueConsoleCmdRequest extends common.CommandRequest<"consoleCommand"> {
    /**
     * Console command to execute
     */
    commandText: string;
    /**
     * Name of the console
     */
    consoleName?: string;
    /**
     * Timeout in seconds when waiting for responses
     */
    timeout?: number;
    /**
     * Whether to wait for command responses (default: true)
     */
    wait?: boolean;
}

export interface IssueConsoleCmdResponse extends common.CommandResponse {
    /**
     * Data returned from the console command
     */
    data: string;
}
