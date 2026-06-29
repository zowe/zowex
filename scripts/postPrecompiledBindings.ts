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
 * Posts the precompiled Python bindings tarball (built on z/OS via
 * `npm run z:python:pack`) to a pull request as a downloadable link.
 *
 * GitHub does not allow attaching binaries directly to PR comments, and gists
 * corrupt binary archives, so the tarball is hosted as an asset on a single
 * persistent prerelease that acts as a dev-artifacts bucket. Each run uploads a
 * uniquely named asset (per PR + commit hash), so a PR can have multiple builds
 * without creating extra releases.
 *
 * Usage: tsx scripts/postPrecompiledBindings.ts <PR_NUMBER>
 */

// Marker used to identify (and update) the script's own sticky comment on a PR.
const COMMENT_HEADER = "🐍 Precompiled Python bindings";
const STICKY_MARKER = "Precompiled Python bindings";

// Single persistent prerelease used as a host for dev artifacts (never published to users).
const RELEASE_TAG = "py-bindings-dev";
const RELEASE_TITLE = "Python Bindings (Dev Artifacts)";
const RELEASE_NOTES =
    "Persistent host for precompiled Python bindings built from pull requests. Not for distribution — assets here are dev artifacts linked from PR comments.";

const TARBALL = path.resolve(__dirname, "../dist/zbind_bin_dist.tar.gz");

/**
 * Resolves an executable to an absolute path within a fixed set of trusted,
 * system-owned directories. Spawning by bare name lets the OS search a possibly
 * attacker-controlled PATH; resolving against a hardcoded directory list keeps
 * execution restricted to fixed, unwriteable locations.
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

/** Resolves a required executable to an absolute path, exiting if it is missing. */
function requireExecutable(name: string): string {
    const bin = resolveExecutable(name);
    if (bin == null) {
        console.error(`Required executable "${name}" was not found in a trusted system directory.`);
        process.exit(1);
    }
    return bin;
}

// Resolve tooling once, to absolute paths, so no spawn below relies on PATH.
const GH_BIN = requireExecutable("gh");

/** Runs `gh` with the given args, returning trimmed stdout. */
function gh(args: string[]): string {
    return childProcess.execFileSync(GH_BIN, args, { encoding: "utf-8" }).trim();
}

/** Returns the short git commit hash, or "local" if it cannot be determined. */
function getShortHash(): string {
    const gitBin = resolveExecutable("git");
    if (gitBin == null) {
        return "local";
    }
    try {
        const hash = childProcess.execFileSync(gitBin, ["rev-parse", "--short", "HEAD"], { encoding: "utf-8" }).trim();
        // Only accept a real short hash; anything else falls back to a safe literal.
        return /^[0-9a-f]{4,40}$/i.test(hash) ? hash : "local";
    } catch {
        return "local";
    }
}

/**
 * Finds the REST comment id of the most recent comment on the PR that was
 * authored by the current user and contains the sticky marker. Returns null if
 * no such comment exists (so a fresh one should be created).
 */
function findStickyCommentId(prNumber: string): number | null {
    const comments = JSON.parse(gh(["pr", "view", prNumber, "--json", "comments"])).comments as Array<{
        body: string;
        url: string;
        viewerDidAuthor: boolean;
        createdAt: string;
    }>;
    const mine = comments
        .filter((c) => c.viewerDidAuthor && c.body.includes(STICKY_MARKER))
        .sort((a, b) => a.createdAt.localeCompare(b.createdAt));
    const latest = mine.at(-1);
    if (!latest) {
        return null;
    }
    // gh exposes a GraphQL node id; the numeric REST id needed for PATCH lives in the URL.
    const match = latest.url.match(/#issuecomment-(\d+)/);
    return match ? Number(match[1]) : null;
}

function main() {
    const prNumber = process.argv[2];
    if (!prNumber || !/^\d+$/.test(prNumber)) {
        console.error("Usage: npm run z:python:post -- <PR_NUMBER>");
        process.exit(1);
    }

    if (!fs.existsSync(TARBALL)) {
        console.error(`Tarball not found: ${TARBALL}\nRun "npm run z:python:pack" first to build it on z/OS.`);
        process.exit(1);
    }

    const hash = getShortHash();
    const assetName = `zbind_bin_dist-pr${prNumber}-${hash}.tar.gz`;

    // Defense-in-depth: prNumber and hash are validated above, but re-check that the
    // resolved staging path stays directly inside dist/ before any filesystem access,
    // so a crafted argument can never escape the intended directory.
    const distDir = path.resolve(path.dirname(TARBALL));
    const stagedPath = path.resolve(distDir, assetName);
    if (path.dirname(stagedPath) !== distDir) {
        console.error(`Refusing to stage outside the dist directory: ${stagedPath}`);
        process.exit(1);
    }
    fs.copyFileSync(TARBALL, stagedPath);

    try {
        const repo = gh(["repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner"]);

        // Ensure the shared prerelease exists; create it only the first time.
        let releaseExists = true;
        try {
            childProcess.execFileSync(GH_BIN, ["release", "view", RELEASE_TAG], { stdio: "ignore" });
        } catch {
            releaseExists = false;
        }
        if (!releaseExists) {
            console.log(`Creating prerelease "${RELEASE_TAG}"...`);
            gh(["release", "create", RELEASE_TAG, "--prerelease", "--title", RELEASE_TITLE, "--notes", RELEASE_NOTES]);
        }

        console.log(`Uploading asset "${assetName}"...`);
        gh(["release", "upload", RELEASE_TAG, stagedPath, "--clobber"]);

        // Resolve the asset's download URL from the release metadata.
        const assets = JSON.parse(gh(["release", "view", RELEASE_TAG, "--json", "assets"])).assets as Array<{
            name: string;
            url: string;
        }>;
        const url =
            assets.find((a) => a.name === assetName)?.url ??
            `https://github.com/${repo}/releases/download/${RELEASE_TAG}/${assetName}`;

        const body = [
            `### ${COMMENT_HEADER}`,
            "",
            `Built from \`${hash}\` on z/OS.`,
            "",
            `📦 **[Download \`${assetName}\`](${url})**`,
            "",
            `<sub>Hosted as an asset on the \`${RELEASE_TAG}\` prerelease (dev artifact, not for distribution).</sub>`,
        ].join("\n");

        // Treat the comment as sticky: update our existing one if present, else create it.
        const existingCommentId = findStickyCommentId(prNumber);
        if (existingCommentId != null) {
            console.log(`Updating existing comment ${existingCommentId} on PR #${prNumber}...`);
            gh([
                "api",
                "--method",
                "PATCH",
                `repos/${repo}/issues/comments/${existingCommentId}`,
                "-f",
                `body=${body}`,
            ]);
        } else {
            console.log(`Posting comment to PR #${prNumber}...`);
            gh(["pr", "comment", prNumber, "--body", body]);
        }

        console.log(`\n✅ ${existingCommentId != null ? "Updated" : "Posted"} precompiled bindings on PR #${prNumber}`);
        console.log(`   Asset: ${url}`);
    } finally {
        fs.rmSync(stagedPath, { force: true });
    }
}

main();
