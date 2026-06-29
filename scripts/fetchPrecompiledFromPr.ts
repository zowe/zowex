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

import * as childProcess from "node:child_process";
import * as fs from "node:fs";
import * as path from "node:path";

/**
 * Locates the most recent "Precompiled Python bindings" comment on a pull
 * request (any author), parses the release-asset link it contains, and
 * downloads that asset to dist/zbind_bin_dist.tar.gz so it can be applied to
 * z/OS for testing.
 *
 * Exit codes: 0 = asset downloaded; 2 = no usable comment/asset found (caller
 * may skip tests); other non-zero = unexpected error.
 *
 * Usage: tsx scripts/fetchPrecompiledFromPr.ts <PR_NUMBER>
 */

const STICKY_MARKER = "Precompiled Python bindings";
const OUTPUT = path.resolve(__dirname, "../dist/zbind_bin_dist.tar.gz");

// Constrain values parsed out of (untrusted) comment text before they touch the
// filesystem or are passed to gh, so a crafted comment cannot traverse paths.
const SAFE_TAG = /^[\w.-]+$/;
const SAFE_ASSET = /^zbind_bin_dist[\w.-]*\.tar\.gz$/;

/**
 * Resolves an executable to an absolute path within a fixed set of trusted,
 * system-owned directories, so spawns never rely on a hijackable PATH.
 */
function resolveExecutable(name: string): string | null {
    const trustedDirs =
        process.platform === "win32"
            ? [path.join(process.env.SystemRoot ?? "C:\\Windows", "System32")]
            : ["/opt/homebrew/bin", "/usr/local/bin", "/usr/bin", "/bin"];
    const candidates = process.platform === "win32" ? [`${name}.exe`, `${name}.cmd`] : [name];
    for (const dir of trustedDirs) {
        for (const file of candidates) {
            const full = path.join(dir, file);
            if (fs.existsSync(full)) {
                return full;
            }
        }
    }
    return null;
}

const GH_BIN = resolveExecutable("gh");
if (GH_BIN == null) {
    console.error('Required executable "gh" was not found in a trusted system directory.');
    process.exit(1);
}

/** Runs `gh` with the given args, returning trimmed stdout. */
function gh(args: string[]): string {
    return childProcess.execFileSync(GH_BIN, args, { encoding: "utf-8" }).trim();
}

function main() {
    const prNumber = process.argv[2];
    if (!prNumber || !/^\d+$/.test(prNumber)) {
        console.error("Usage: npm run z:fetch:python -- <PR_NUMBER>");
        process.exit(1);
    }

    // Most recent matching comment on this PR, regardless of author.
    const comments = JSON.parse(gh(["pr", "view", prNumber, "--json", "comments"])).comments as Array<{
        body: string;
        createdAt: string;
    }>;
    const latest = comments
        .filter((c) => c.body.includes(STICKY_MARKER))
        .sort((a, b) => a.createdAt.localeCompare(b.createdAt))
        .at(-1);
    if (!latest) {
        console.warn(`No "${STICKY_MARKER}" comment found on PR #${prNumber}.`);
        process.exit(2);
    }

    // Pull the release-asset URL out of the markdown download link.
    const linkMatch = latest.body.match(/]\((https?:\/\/[^)]+\/releases\/download\/[^)]+)\)/);
    if (!linkMatch) {
        console.warn(`Found a "${STICKY_MARKER}" comment on PR #${prNumber} but no release-asset download link.`);
        process.exit(2);
    }
    const urlMatch = linkMatch[1].match(/\/releases\/download\/([^/]+)\/([^/]+)$/);
    const tag = urlMatch ? decodeURIComponent(urlMatch[1]) : "";
    const assetName = urlMatch ? decodeURIComponent(urlMatch[2]) : "";
    if (!SAFE_TAG.test(tag) || !SAFE_ASSET.test(assetName)) {
        console.warn(`Asset link did not match the expected pattern (tag="${tag}", asset="${assetName}"); skipping.`);
        process.exit(2);
    }

    // Download from the current repo's release by tag + asset name. Using
    // `gh release download` (rather than the raw URL) keeps the fetch scoped to
    // this repository regardless of what the comment URL points at.
    const outDir = path.dirname(OUTPUT);
    fs.mkdirSync(outDir, { recursive: true });
    console.log(`Downloading "${assetName}" from release "${tag}"...`);
    gh(["release", "download", tag, "--pattern", assetName, "--dir", outDir, "--clobber"]);

    const downloaded = path.resolve(outDir, assetName);
    if (path.dirname(downloaded) !== path.resolve(outDir) || !fs.existsSync(downloaded)) {
        console.error(`Expected downloaded asset at ${downloaded} was not found.`);
        process.exit(1);
    }
    if (downloaded !== OUTPUT) {
        fs.copyFileSync(downloaded, OUTPUT);
        fs.rmSync(downloaded, { force: true });
    }
    console.log(`Fetched precompiled bindings -> ${OUTPUT}`);
}

main();
