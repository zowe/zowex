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

import { type ISshErrorDefinition, SshErrors } from "zowex-sdk";

/**
 * Translates known CLI errors into more user-friendly messages.
 * @param error The error to translate
 * @returns An error with a user-friendly message if a known pattern is found, otherwise the original error.
 */
export function translateCliError(error: Error): Error | ISshErrorDefinition {
    const errorMessage = `${error}`;
    for (const key in SshErrors) {
        const errorDef = SshErrors[key];
        for (const match of errorDef.matches) {
            if (typeof match === "string" && errorMessage.includes(match)) {
                return errorDef;
            } else if (match instanceof RegExp && match.test(errorMessage)) {
                return errorDef;
            }
        }
    }
    return error;
}
