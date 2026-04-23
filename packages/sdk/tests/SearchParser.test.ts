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

import { describe, expect, it } from "vitest";
import { parseSearchOutput } from "../src/utils/tools/SearchParser";

describe("parseSearchOutput", () => {
    it("properly parses PDS member search", () => {
        const output = [
            "\f  ASMFSUPC    -     MVS FILE/LINE/WORD/BYTE/SFOR COMPARE UTILITY- V1R6M0  (2021/11/01)  2026/04/21   8.49    PAGE     1",
            " LINE-#  SOURCE SECTION                    SRCH DSN: SYS1.SAMPLIB(IEANTCOB)",
            "",
            "      *        ***************************************************************** 00050000",
            "      5        *  Descriptive Name: Name/Token Service COBOL Declares          * 00250000",
            "     18        *    IEANTCOB defines Name/Token Service constants and external * 00500000",
            "     75        *  End of Name/Token Service Declares                           * 02650000",
            "\f  ASMFSUPC    -     MVS FILE/LINE/WORD/BYTE/SFOR COMPARE UTILITY- V1R6M0  (2021/11/01)  2026/04/21   8.49    PAGE     2",
            "     SEARCH-FOR SUMMARY SECTION            SRCH DSN: SYS1.SAMPLIB(IEANTCOB)",
            "",
            "LINES-FOUND  LINES-PROC  DATASET-W/LNS  DATASET-WO/LNS  COMPARE-COLS  LONGEST-LINE",
            "        3           76            1              0           1:80           80",
            "",
            "PROCESS OPTIONS USED: ANYC LPSF",
            "",
            "THE FOLLOWING PROCESS STATEMENTS (USING COLUMNS 1:72) WERE PROCESSED:",
            "    SRCHFOR 'Name/Token Service'",
            "",
            "",
        ].join("\n");

        const result = parseSearchOutput(output);

        expect(result.dataset).toBe("SYS1.SAMPLIB(IEANTCOB)");
        expect(result.members).toHaveLength(1);
        expect(result.members[0].name).toBe("IEANTCOB");
        expect(result.members[0].matches.map((m) => m.lineNumber)).toEqual([5, 18, 75]);
        expect(result.summary.linesFound).toBe(3);
        expect(result.summary.membersWithLines).toBe(1);
    });

    it("should parse PDS search with multiple members", () => {
        const output = [
            "\f  ASMFSUPC    -     MVS FILE/LINE/WORD/BYTE/SFOR COMPARE UTILITY- V1R6M0  (2021/11/01)  2026/04/21   8.53    PAGE     1",
            " LINE-#  SOURCE SECTION                    SRCH DSN: IBMUSER.JCL",
            "",
            "",
            " GDG                         --------- STRING(S) FOUND -------------------",
            "",
            "      1  //IEFBR14$ JOB (IZUACCT),'mainframer',REGION=0M                        JOB00730",
            "      *  //*",
            " IEFBR14                     --------- STRING(S) FOUND -------------------",
            "",
            "      4  //EXEC     EXEC PGM=IEFBR14",
            "\f  ASMFSUPC    -     MVS FILE/LINE/WORD/BYTE/SFOR COMPARE UTILITY- V1R6M0  (2021/11/01)  2026/04/21   8.53    PAGE     3",
            "     SEARCH-FOR SUMMARY SECTION            SRCH DSN: IBMUSER.JCL",
            "",
            "LINES-FOUND  LINES-PROC  MEMBERS-W/LNS  MEMBERS-WO/LNS  COMPARE-COLS  LONGEST-LINE",
            "       10          937            8             37           1:80           80",
            "",
            "THE FOLLOWING PROCESS STATEMENTS (USING COLUMNS 1:72) WERE PROCESSED:",
            "    SRCHFOR 'ief'",
            "",
            "",
        ].join("\n");

        const result = parseSearchOutput(output);

        expect(result.members.map((m) => m.name)).toEqual(["GDG", "IEFBR14"]);
        expect(result.dataset).toBe("IBMUSER.JCL");
        expect(result.summary.membersWithLines).toEqual(8);
        expect(result.members[0].matches.map((m) => m.lineNumber)).toEqual([1]);
        expect(result.members[1].matches.map((m) => m.lineNumber)).toEqual([4]);
        expect(result.summary.linesFound).toBe(10);
    });
});
