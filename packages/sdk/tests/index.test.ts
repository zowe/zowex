/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 * This test file covers the index and utils exports.
 */

import { describe, expect, it } from "vitest";
import * as index from "../src/index";
import * as utilsIndex from "../src/utils/index";

describe("index exports", () => {
    it("should export all classes and functions", () => {
        expect(index.ZSshClient).toBeDefined();
        expect(index.ZSshUtils).toBeDefined();
        expect(index.UtilsApi).toBeDefined();
        expect(index.SshConfigUtils).toBeDefined();
        expect(index.ConfigFileUtils).toBeDefined();
    });
});

describe("utils index exports", () => {
    it("should export search parser", () => {
        expect(utilsIndex.parseSearchOutput).toBeDefined();
    });
});
