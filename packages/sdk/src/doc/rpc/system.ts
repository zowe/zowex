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

export interface ViewSyslogRequest extends common.CommandRequest<"viewSyslog"> {
    /**
     * Date in yyyy-mm-dd. Mutually exclusive with secondsAgo.
     */
    date?: string;
    /**
     * Time in hh:mm:ss. Mutually exclusive with secondsAgo.
     */
    time?: string;
    /**
     * Relative offset: start from (now - secondsAgo) on z/OS.
     * Mutually exclusive with date/time.
     */
    secondsAgo?: number;
    /**
     * Maximum syslog lines to return.
     */
    maxLines?: number;
}

export interface ViewSyslogResponse extends common.CommandResponse {
    /**
     * Raw syslog text, newline-separated lines.
     */
    data?: string;
    /**
     * Actual start date used for the read (yyyy-mm-dd).
     */
    startDate?: string;
    /**
     * Actual start time used for the read (hh:mm:ss).
     */
    startTime?: string;
    /**
     * Date of the last record returned (yyyy-mm-dd).
     */
    endDate?: string;
    /**
     * Time of the last record returned (hh:mm:ss).
     */
    endTime?: string;
    /**
     * Number of lines returned in data.
     */
    returnedLines?: number;
    /**
     * True when the syslog contained more lines than maxLines
     * and the response was truncated.
     */
    hasMore?: boolean;
}
