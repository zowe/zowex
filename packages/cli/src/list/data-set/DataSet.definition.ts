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

import { type ICommandDefinition, TextUtils } from "@zowe/imperative";

export const ListDataSetDefinition: ICommandDefinition = {
    handler: `${__dirname}/DataSet.handler`,
    type: "command",
    name: "data-set",
    aliases: ["ds"],
    summary: "List data sets matching pattern",
    description:
        "List all data sets that match a DSLEVEL pattern (see help below). \n" +
        "The following values can be used with the --response-format-filter (--rff) argument to " +
        "display more data from the data sets: " +
        "volume, unit, referred, ext, used, recfm, lrecl, blksz, dsorg, and dsname.",
    examples: [
        {
            description: 'List all data sets beginning with "ibmuser" and ending in "cntl"',
            options: '"ibmuser.**.cntl"',
        },
    ],
    positionals: [
        {
            name: "pattern",
            description:
                "The pattern or patterns to match data sets against. Also known as 'DSLEVEL', it " +
                "is somewhat similar to the concept of a 'glob' (but not identical)." +
                " The following special sequences can be used in the pattern:\n" +
                TextUtils.chalk.yellow("%") +
                ": Matches any single character\n" +
                TextUtils.chalk.yellow("*") +
                ": Matches any number of characters within" +
                " a data set name qualifier (e.g. " +
                '"ibmuser.j*.old" matches "ibmuser.jcl.old" but not "ibmuser.jcl.very.old")\n' +
                TextUtils.chalk.yellow("**") +
                ": Matches any number of characters within any number" +
                " of data set name qualifiers (e.g. " +
                '"ibmuser.**.old" matches both "ibmuser.jcl.old" and "ibmuser.jcl.very.old")\n' +
                "However, the pattern cannot begin with any of these sequences.\n",
            type: "string",
            required: true,
        },
    ],
    options: [
        {
            name: "attributes",
            aliases: ["a"],
            description: "Fetch attributes of the data sets being listed.",
            type: "boolean",
        },
        {
            name: "exact-match",
            description: "Match the pattern exactly instead of appending a trailing wildcard.",
            type: "boolean",
            defaultValue: false,
        },
    ],
    profile: { optional: ["ssh"] },
    outputFormatOptions: true,
};
