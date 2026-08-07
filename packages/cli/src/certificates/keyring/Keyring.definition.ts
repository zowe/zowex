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

import type { ICommandDefinition, ICommandOptionDefinition, ICommandPositionalDefinition } from "@zowe/imperative";

const OWNER_POSITIONAL: ICommandPositionalDefinition = {
    name: "owner",
    description: "The key ring owner (user ID).",
    type: "string",
    required: true,
};

const KEYRING_POSITIONAL: ICommandPositionalDefinition = {
    name: "keyring",
    description: "The key ring name.",
    type: "string",
    required: true,
};

const CreateKeyringDefinition: ICommandDefinition = {
    handler: `${__dirname}/create/Create.handler`,
    type: "command",
    name: "create",
    aliases: ["cre"],
    summary: "Create a new key ring",
    description: "Create a new ESM key ring owned by the specified user.",
    examples: [{ description: "Create a key ring", options: "USER01 RING02" }],
    positionals: [OWNER_POSITIONAL, KEYRING_POSITIONAL],
    profile: { optional: ["ssh"] },
};

const DeleteKeyringDefinition: ICommandDefinition = {
    handler: `${__dirname}/delete/Delete.handler`,
    type: "command",
    name: "delete",
    aliases: ["del"],
    summary: "Delete a key ring",
    description: "Delete an ESM key ring. The certificates it referenced remain in the ESM database.",
    examples: [{ description: "Delete a key ring", options: "USER01 RING02" }],
    positionals: [OWNER_POSITIONAL, KEYRING_POSITIONAL],
    profile: { optional: ["ssh"] },
};

const ListMaxEntriesOption: ICommandOptionDefinition = {
    name: "max-entries",
    aliases: ["me"],
    description: "Maximum number of certificates to return. Defaults to 10; use 0 to return all certificates.",
    type: "number",
};

const ListKeyringDefinition: ICommandDefinition = {
    handler: `${__dirname}/list/List.handler`,
    type: "command",
    name: "list",
    aliases: ["ls"],
    summary: "List certificates connected to a key ring",
    description:
        "List the certificates connected to an ESM key ring, showing each certificate's label, owner, " +
        "usage, trust status, and whether it is the ring's default certificate.",
    examples: [
        { description: "List certificates in a ring", options: "USER01 RING02" },
        {
            description: "List only the labels of PERSONAL certificates",
            options: "USER01 RING02 -u PERSONAL --label-only",
        },
        { description: "List every certificate (no page limit)", options: "USER01 RING02 --max-entries 0" },
    ],
    positionals: [OWNER_POSITIONAL, KEYRING_POSITIONAL],
    options: [
        { name: "label", aliases: ["l"], description: "Filter by certificate label.", type: "string" },
        {
            name: "usage",
            aliases: ["u"],
            description: "Filter by usage.",
            type: "string",
            allowableValues: { values: ["PERSONAL", "CERTAUTH", "OTHER"] },
        },
        { name: "label-only", description: "Print only certificate labels.", type: "boolean" },
        { name: "owner-only", description: "Print only certificate owners.", type: "boolean" },
        ListMaxEntriesOption,
    ],
    profile: { optional: ["ssh"] },
    outputFormatOptions: true,
};

const ListRingsDefinition: ICommandDefinition = {
    handler: `${__dirname}/list-rings/ListRings.handler`,
    type: "command",
    name: "list-rings",
    aliases: ["lr"],
    summary: "List a user's key rings and their certificates",
    description:
        "List all of a user's key rings (R_datalib GetRingInfo) and, for each ring, the certificates " +
        "connected to it.",
    examples: [
        { description: "List all of a user's key rings", options: "USER01" },
        { description: "Narrow to a single ring", options: "USER01 RING02" },
    ],
    positionals: [
        OWNER_POSITIONAL,
        { name: "keyring", description: "Optional key ring name to narrow the result.", type: "string" },
    ],
    profile: { optional: ["ssh"] },
    outputFormatOptions: true,
};

const CountRingDefinition: ICommandDefinition = {
    handler: `${__dirname}/count/Count.handler`,
    type: "command",
    name: "count",
    summary: "Count the certificates connected to a key ring",
    description:
        "Count the certificates connected to a key ring (R_datalib GetRingInfo returns the count in a single " +
        "call). Use '*' for the key ring to count all of the owner's certificates.",
    examples: [{ description: "Count certificates in a ring", options: "USER01 RING02" }],
    positionals: [
        OWNER_POSITIONAL,
        {
            name: "keyring",
            description: "The key ring name, or '*' for the owner's virtual key ring.",
            type: "string",
            required: true,
        },
    ],
    profile: { optional: ["ssh"] },
};

export const KeyringDefinition: ICommandDefinition = {
    name: "keyring",
    aliases: ["ring"],
    summary: "Create, delete, list, and count key rings",
    description: "Key ring operations: create and delete key rings, and list or count the certificates they contain.",
    type: "group",
    children: [
        CreateKeyringDefinition,
        DeleteKeyringDefinition,
        ListKeyringDefinition,
        ListRingsDefinition,
        CountRingDefinition,
    ],
};
