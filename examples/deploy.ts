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

import * as fs from "node:fs";
import * as path from "node:path";
import { pipeline } from "node:stream";
import { promisify } from "node:util";
import { type IProfile, ProfileInfo } from "@zowe/imperative";
import { Client, type SFTPWrapper } from "ssh2";
import { AsciiToEbcdicTransform } from "../scripts/buildTools";

/**
 * Deploy and build C++ examples on z/OS.
 *
 * Uses ssh2 Client.shell() for remote commands (same approach as buildTools.ts)
 * so the user's login environment is available.
 *
 * Usage:
 *   npx tsx examples/deploy.ts <ssh-profile> <deploy-dir> [example...]
 *
 * Arguments:
 *   ssh-profile   Name of a Zowe SSH profile
 *   deploy-dir    USS directory for the deploy (e.g. ~/zowex)
 *   example...    Optional list of examples to deploy (default: all)
 *
 * Examples:
 *   npx tsx examples/deploy.ts myssh ~/zowex
 *   npx tsx examples/deploy.ts myssh ~/zowex pong-server jsonrpc-ssh
 */

const ALL_EXAMPLES = ["pong-server", "jsonrpc-ssh", "binary-ssh"];
const EXAMPLES_DIR = __dirname;
const REPO_ROOT = path.resolve(__dirname, "..");

interface ExtraFile {
    localPath: string;
    remoteRelPath: string;
}

interface BuildStep {
    cwd: string;
    cmd: string;
}

function getExtraFiles(example: string): ExtraFile[] {
    switch (example) {
        case "jsonrpc-ssh":
            return [{ localPath: path.join(REPO_ROOT, "native/c/zjson.hpp"), remoteRelPath: "zjson.hpp" }];
        default:
            return [];
    }
}

function getBuildSteps(example: string, remoteDir: string): BuildStep[] {
    switch (example) {
        case "pong-server":
            return [{ cwd: remoteDir, cmd: "ibm-clang++64 -std=c++17 -o server server.cpp" }];
        case "jsonrpc-ssh":
            return [{ cwd: remoteDir, cmd: "ibm-clang++64 -std=c++17 -o server server.cpp" }];
        case "binary-ssh":
            return [
                { cwd: remoteDir, cmd: "ibm-clang++64 -std=c++17 -o server server.cpp" },
                {
                    cwd: `${remoteDir}/tests`,
                    cmd: "for f in testb64 testb85 testraw; do ibm-clang++64 -std=c++17 -o $f $f.cpp; done",
                },
            ];
        default:
            return [];
    }
}

function collectCppFiles(localDir: string, prefix = ""): string[] {
    const files: string[] = [];
    for (const entry of fs.readdirSync(localDir, { withFileTypes: true })) {
        const rel = prefix ? `${prefix}/${entry.name}` : entry.name;
        if (entry.isDirectory()) {
            files.push(...collectCppFiles(path.join(localDir, entry.name), rel));
        } else if (entry.name.endsWith(".cpp")) {
            files.push(rel);
        }
    }
    return files;
}

async function loadSshProfile(profileName: string): Promise<IProfile> {
    const profInfo = new ProfileInfo("zowe");
    await profInfo.readProfilesFromDisk();
    const profAttrs = profInfo.getAllProfiles("ssh").find((prof) => prof.profName === profileName);
    if (!profAttrs) {
        console.error(`SSH profile "${profileName}" not found. Available profiles:`);
        for (const p of profInfo.getAllProfiles("ssh")) {
            console.error(`  - ${p.profName}`);
        }
        process.exit(1);
    }
    const mergedArgs = profInfo.mergeArgsForProfile(profAttrs, { getSecureVals: true });
    return Object.fromEntries(mergedArgs.knownArgs.map((arg) => [arg.argName, arg.argValue]));
}

function buildSshClient(sshProfile: IProfile): Promise<Client> {
    const client = new Client();
    return new Promise((resolve, reject) => {
        client.on("error", (err) => reject(err));
        client.on("ready", () => resolve(client));
        client.connect({
            ...sshProfile,
            username: sshProfile.user,
            privateKey: sshProfile.privateKey ? fs.readFileSync(sshProfile.privateKey) : undefined,
        });
    });
}

function runInShell(connection: Client, command: string): Promise<{ stdout: string; stderr: string; code: number }> {
    return new Promise((resolve, reject) => {
        connection.shell(false, (err, stream) => {
            if (err) return reject(err);

            let stdout = "";
            let stderr = "";

            stream.on("data", (data: Buffer) => {
                stdout += data.toString();
            });
            stream.stderr.on("data", (data: Buffer) => {
                stderr += data.toString();
            });
            stream.on("exit", (code: number) => {
                resolve({ stdout, stderr, code: code ?? 0 });
            });
            stream.on("close", () => {
                resolve({ stdout, stderr, code: 0 });
            });

            stream.end(`${command}\nexit $?\n`);
        });
    });
}

async function uploadFile(sftp: SFTPWrapper, localPath: string, remotePath: string) {
    await new Promise<void>((resolve, reject) => {
        pipeline(
            fs.createReadStream(localPath),
            new AsciiToEbcdicTransform(),
            sftp.createWriteStream(remotePath),
            (err) => (err ? reject(err) : resolve()),
        );
    });
}

async function main() {
    const args = process.argv.slice(2);
    const sshProfileName = args[0];
    const deployDir = args[1];
    const selected = args.length > 2 ? args.slice(2) : ALL_EXAMPLES;

    if (!sshProfileName || !deployDir) {
        console.error("Usage: npx tsx examples/deploy.ts <ssh-profile> <deploy-dir> [example...]");
        console.error("");
        console.error("  ssh-profile   Name of a Zowe SSH profile");
        console.error("  deploy-dir    USS directory for the deploy (e.g. ~/zowex)");
        console.error("  example...    Optional: pong-server, jsonrpc-ssh, binary-ssh (default: all)");
        process.exit(1);
    }

    const remoteDeployDir = deployDir.replace(/^~/, ".");
    const examplesRemote = `${remoteDeployDir}/examples`;

    console.log(`Loading SSH profile "${sshProfileName}"...`);
    const sshProfile = await loadSshProfile(sshProfileName);
    const connection = await buildSshClient(sshProfile);

    const sftp = await promisify(connection.sftp.bind(connection))();

    try {
        console.log(`Connected to ${sshProfile.host}\n`);

        const mkdirResult = await runInShell(connection, `mkdir -p ${examplesRemote}`);
        if (mkdirResult.code !== 0) {
            throw new Error(`Failed to create ${examplesRemote}: ${mkdirResult.stderr}`);
        }

        for (const example of selected) {
            const localDir = path.join(EXAMPLES_DIR, example);
            if (!fs.existsSync(localDir)) {
                console.log(`Skipping ${example} (directory not found)`);
                continue;
            }

            const remoteDir = `${examplesRemote}/${example}`;
            console.log(`==> ${example}`);

            const cppFiles = collectCppFiles(localDir);
            if (cppFiles.length === 0) {
                console.log("    No .cpp files found, skipping");
                continue;
            }

            // Create all remote directories in a single shell invocation
            const remoteDirs = [...new Set(cppFiles.map((f) => path.posix.dirname(`${remoteDir}/${f}`)))];
            await runInShell(connection, `mkdir -p ${remoteDirs.join(" ")}`);

            // Upload .cpp files via SFTP
            for (const file of cppFiles) {
                const localPath = path.join(localDir, file);
                const remotePath = `${remoteDir}/${file}`;
                process.stdout.write(`    Uploading ${file}...`);
                await uploadFile(sftp, localPath, remotePath);
                console.log(" done");
            }

            // Upload extra dependencies (e.g. zjson.hpp for jsonrpc-ssh)
            for (const extra of getExtraFiles(example)) {
                const remotePath = `${remoteDir}/${extra.remoteRelPath}`;
                process.stdout.write(`    Uploading ${extra.remoteRelPath}...`);
                await uploadFile(sftp, extra.localPath, remotePath);
                console.log(" done");
            }

            // Build using shell (login environment is available)
            const steps = getBuildSteps(example, remoteDir);
            for (const step of steps) {
                process.stdout.write(`    Building in ${step.cwd}...`);
                const result = await runInShell(connection, `cd ${step.cwd} && ${step.cmd}`);
                if (result.code !== 0) {
                    console.log(" FAILED");
                    console.error(`    ${result.stderr}`);
                    process.exitCode = 1;
                    break;
                }
                console.log(" done");
            }

            console.log(`    Ready: ${remoteDir}/server\n`);
        }

        console.log("Deploy complete!\n");
        console.log("To test an example, run the client from the corresponding directory:");
        for (const example of selected) {
            console.log(`  cd examples/${example} && npm install`);
            console.log(
                `  npx tsx client.ts ${sshProfile.user}@${sshProfile.host} ${examplesRemote}/${example}/server\n`,
            );
        }
    } finally {
        sftp.end();
        connection.end();
    }
}

main().catch((err) => {
    console.error(err instanceof Error ? err.message : String(err));
    process.exit(1);
});
