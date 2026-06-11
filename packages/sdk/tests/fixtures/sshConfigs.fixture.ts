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

import type { IProfileLoaded } from "@zowe/imperative";
import type { ISshConfigExt } from "../../src/SshConfigUtils";

export const sshProfiles: IProfileLoaded[] = [
    {
        message: "",
        name: "sshKey",
        type: "ssh",
        failNotFound: false,
        profile: { host: "lpar1.com", port: 22, user: "zoweUser1", privateKey: "/path/to/id_rsa" },
    },
    {
        message: "",
        name: "sshBasic",
        type: "ssh",
        failNotFound: false,
        profile: { host: "lpar2.com", port: 22, user: "zoweUser2", password: "password" },
    },
];

export const migratedSshConfigs: ISshConfigExt[] = [
    {
        hostname: "lpar3.com",
        name: "SSHlpar3",
        port: 22,
        privateKey: "/Users/users/.ssh/id_rsa",
        user: "zoweUser3",
    },
    {
        hostname: "lpar4.com",
        name: "SSHlpar4",
        port: 22,
        privateKey: "/Users/users/.ssh/id_rsa",
        user: "zoweUser4",
    },
];

export const migratedSshConfigsValidation: ISshConfigExt[] = [
    {
        hostname: "lpar3.com",
        name: "SSHlpar3",
        port: 22,
        privateKey: "/Users/users/.ssh/id_rsa",
        user: "zoweUser3",
    },
    {
        hostname: "lpar4.com",
        name: "SSHlpar4",
        port: 22,
        privateKey: "/Users/users/.ssh/id_rsa",
        user: "zoweUser4",
    },
    {
        hostname: "lpar1.com",
        name: "SSHlpar4",
        port: 22,
        privateKey: "/Users/users/.ssh/id_rsa",
        user: "user1",
    },
    {
        hostname: "lpar1.com",
        name: "SSHlpar4",
        port: 22,
        privateKey: "/Users/users/.ssh/id_rsa",
        user: "user2",
    },
];
