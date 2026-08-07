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

// bumpVersion.ts - Bumps the version across package.json, CHANGELOG.md, and ZSshConstants.ts files
import { execSync } from "node:child_process";
import * as fs from "node:fs";
import * as path from "node:path";

const ROOT_DIR = path.resolve(__dirname, "..");
const BUMP_TYPES = ["patch", "minor", "major"] as const;
type BumpType = (typeof BUMP_TYPES)[number];

const PACKAGE_JSON_PATHS = [
    path.join(ROOT_DIR, "package.json"),
    path.join(ROOT_DIR, "packages", "cli", "package.json"),
    path.join(ROOT_DIR, "packages", "sdk", "package.json"),
];
const CHANGELOG_PATHS = [
    path.join(ROOT_DIR, "native", "CHANGELOG.md"),
    path.join(ROOT_DIR, "packages", "cli", "CHANGELOG.md"),
    path.join(ROOT_DIR, "packages", "sdk", "CHANGELOG.md"),
];
const SSH_CONSTANTS_PATH = path.join(ROOT_DIR, "packages", "sdk", "src", "ZSshConstants.ts");
const SDK_PACKAGE_NAME = "@zowe/zowex-for-zowe-sdk";

function nextVersion(currentVersion: string, bumpType: BumpType): string {
    const [major, minor, patch] = currentVersion.split(".").map(Number);
    switch (bumpType) {
        case "major":
            return `${major + 1}.0.0`;
        case "minor":
            return `${major}.${minor + 1}.0`;
        case "patch":
            return `${major}.${minor}.${patch + 1}`;
    }
}

function updatePackageJson(filePath: string, newVersion: string) {
    const pkg = JSON.parse(fs.readFileSync(filePath, "utf-8"));
    pkg.version = newVersion;
    if (pkg.dependencies?.[SDK_PACKAGE_NAME] != null) {
        pkg.dependencies[SDK_PACKAGE_NAME] = newVersion;
    }
    fs.writeFileSync(filePath, `${JSON.stringify(pkg, null, 2)}\n`);
}

function updateChangelog(filePath: string, newVersion: string) {
    const content = fs.readFileSync(filePath, "utf-8");
    if (!content.includes("## Recent Changes")) {
        return;
    }
    fs.writeFileSync(filePath, content.replace("## Recent Changes", `## \`${newVersion}\``));
}

function updateSshConstants(filePath: string, oldVersion: string, newVersion: string) {
    const content = fs.readFileSync(filePath, "utf-8");
    fs.writeFileSync(
        filePath,
        content.replace(
            `export const BUNDLED_SSH_SERVER_VERSION = "${oldVersion}";`,
            `export const BUNDLED_SSH_SERVER_VERSION = "${newVersion}";`,
        ),
    );
}

function main() {
    const bumpType = process.argv[2] as BumpType;
    if (!BUMP_TYPES.includes(bumpType)) {
        console.error(`Usage: npm run version:bump -- <${BUMP_TYPES.join("|")}>`);
        process.exit(1);
    }

    const rootPackageJson = JSON.parse(fs.readFileSync(PACKAGE_JSON_PATHS[0], "utf-8"));
    const oldVersion = rootPackageJson.version;
    const newVersion = nextVersion(oldVersion, bumpType);

    for (const packageJsonPath of PACKAGE_JSON_PATHS) {
        updatePackageJson(packageJsonPath, newVersion);
    }
    for (const changelogPath of CHANGELOG_PATHS) {
        updateChangelog(changelogPath, newVersion);
    }
    updateSshConstants(SSH_CONSTANTS_PATH, oldVersion, newVersion);

    execSync("npm install --package-lock-only", { cwd: ROOT_DIR, stdio: "inherit" });

    console.log(`Bumped version: ${oldVersion} -> ${newVersion}`);
}

main();
