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

import { accessSync, constants, readFileSync } from "node:fs";
import { homedir } from "node:os";
import * as path from "node:path";
import type { ISshSession } from "@zowe/zos-uss-for-zowe-sdk";
import * as sshConfig from "ssh-config";

export interface ISshConfigExt extends ISshSession {
    name?: string;
}
// biome-ignore lint/complexity/noStaticOnlyClass: Utilities class has static methods
export class SshConfigUtils {
    public static async findPrivateKeys(): Promise<string[]> {
        const keyNames = ["id_ed25519", "id_rsa", "id_ecdsa", "id_dsa"];
        const privateKeyPaths: Set<string> = new Set();
        // Check standard ~/.ssh private keys
        for (const algo of keyNames) {
            const keyPath = path.resolve(homedir(), ".ssh", algo);
            try {
                accessSync(keyPath, constants.R_OK);
                privateKeyPaths.add(keyPath);
            } catch {
                // Ignore missing keys
            }
        }
        return Array.from(privateKeyPaths);
    }

    public static async migrateSshConfig(): Promise<ISshConfigExt[]> {
        const homeDir = homedir();
        const parsedConfig = SshConfigUtils.readSshConfig(homeDir);
        if (parsedConfig == null) {
            return [];
        }

        const SSHConfigs: ISshConfigExt[] = [];
        for (const config of parsedConfig) {
            if (config.type === sshConfig.LineType.DIRECTIVE && config.param === "Host") {
                // If it has multiple names, take the first
                const name = typeof config.value === "object" ? config.value[0].val : (config.value as string);
                // Skip host names that contain wildcard characters
                if (name.includes("*") || name.includes("?")) continue;

                SSHConfigs.push({ name, ...SshConfigUtils.parseHostSection(config as sshConfig.Section, homeDir) });
            }
        }
        return SSHConfigs;
    }

    /**
     * Looks up a single `Host` entry by name in the user's `~/.ssh/config` file, mirroring the
     * field mapping used by {@link migrateSshConfig}.
     * @param hostAlias The exact name following the `Host` keyword to search for
     * @returns The matching session properties, or `undefined` if the host was not found
     */
    public static findSshConfigHost(hostAlias: string): ISshConfigExt | undefined {
        const homeDir = homedir();
        const parsedConfig = SshConfigUtils.readSshConfig(homeDir);
        if (parsedConfig == null) {
            return undefined;
        }

        for (const config of parsedConfig) {
            if (config.type !== sshConfig.LineType.DIRECTIVE || config.param !== "Host") continue;

            const names = typeof config.value === "object" ? config.value.map((val) => val.val) : [config.value];
            if (!names.includes(hostAlias)) continue;

            return { name: hostAlias, ...SshConfigUtils.parseHostSection(config as sshConfig.Section, homeDir) };
        }
        return undefined;
    }

    private static readSshConfig(homeDir: string): sshConfig.default | undefined {
        const filePath = path.join(homeDir, ".ssh", "config");
        try {
            return sshConfig.parse(readFileSync(filePath, "utf-8"));
        } catch {
            return undefined;
        }
    }

    private static parseHostSection(section: sshConfig.Section, homeDir: string): Omit<ISshConfigExt, "name"> {
        const session: Omit<ISshConfigExt, "name"> = {};
        if (!Array.isArray(section.config)) {
            return session;
        }

        for (const subConfig of section.config) {
            if (typeof subConfig === "object" && "param" in subConfig && "value" in subConfig) {
                const param = (subConfig as sshConfig.Directive).param.toLowerCase();
                const value = subConfig.value as string;

                switch (param) {
                    case "hostname":
                        session.hostname = value;
                        break;
                    case "port":
                        session.port = Number.parseInt(value, 10);
                        break;
                    case "user":
                        session.user = value;
                        break;
                    case "identityfile":
                        session.privateKey = path.normalize(
                            value.startsWith("~") ? path.join(homeDir, value.slice(2)) : value,
                        );
                        break;
                    case "connecttimeout":
                        session.handshakeTimeout = Number.parseInt(value, 10) * 1000;
                        break;
                    default:
                        break;
                }
            }
        }
        return session;
    }
}
