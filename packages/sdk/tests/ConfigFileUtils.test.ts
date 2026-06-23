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

import type { Config } from "@zowe/imperative";
import type { CommentToken } from "comment-json";
import { cloneDeep } from "es-toolkit/compat";
import { ConfigFileUtils } from "../src/ConfigFileUtils";

const AFTER_PROPERTIES_SYMBOL = Symbol.for("after:properties");

interface MockProfile {
    type: string;
    properties: {
        host: string;
        user: string;
        privateKey?: string;
    };
    [AFTER_PROPERTIES_SYMBOL]?: CommentToken[];
}

interface MockConfigLayer {
    user: boolean;
    global: boolean;
    path: string;
    properties: {
        profiles: {
            [profileName: string]: MockProfile;
        };
    };
}

describe("ConfigFileUtils", () => {
    let mockTeamConfig: Config;
    let mockLayerJson: MockConfigLayer;
    let writtenLayerJson: MockConfigLayer | null;

    const EXAMPLE_PRIVATE_KEY_PATH = "/u/users/example/.ssh/id_XXX";
    const EXAMPLE_PRIVATE_KEY_COMMENT = `privateKey was moved to a comment as the value is invalid. Original value: "${EXAMPLE_PRIVATE_KEY_PATH}"`;
    const MOCK_LAYER_PATH = "/mock/path/config.json";

    beforeEach(() => {
        // Reset the written layer JSON for each test
        writtenLayerJson = null;
    });

    const createMockConfigWithPrivateKey = (): MockConfigLayer => {
        const testConfig: MockConfigLayer = {
            user: false,
            global: false,
            path: MOCK_LAYER_PATH,
            properties: {
                profiles: {
                    testprofile: {
                        type: "ssh",
                        properties: {
                            host: "example.com",
                            user: "testuser",
                            privateKey: EXAMPLE_PRIVATE_KEY_PATH,
                        },
                    },
                },
            },
        };
        mockLayerJson = cloneDeep(testConfig);
        return testConfig;
    };

    const createMockConfigWithCommentedKey = (): MockConfigLayer => {
        const testConfig: MockConfigLayer = {
            user: false,
            global: false,
            path: MOCK_LAYER_PATH,
            properties: {
                profiles: {
                    testprofile: {
                        type: "ssh",
                        properties: {
                            host: "example.com",
                            user: "testuser",
                        },
                    },
                },
            },
        };
        // Add comment for private key that was removed
        testConfig.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL] = [
            {
                type: "LineComment",
                value: ` ${EXAMPLE_PRIVATE_KEY_COMMENT}`,
                inline: false,
            } as CommentToken,
        ];
        mockLayerJson = cloneDeep(testConfig);
        // Manually copy the symbol since cloneDeep won't preserve it
        mockLayerJson.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL] =
            testConfig.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL];
        return testConfig;
    };

    const createMockTeamConfig = (testConfig: MockConfigLayer): Config => {
        return {
            findLayer: (_user: boolean, _global: boolean) => mockLayerJson,
            api: {
                profiles: {
                    get: (profileName: string, _failNotFound = true) => {
                        const profile = testConfig.properties?.profiles?.[profileName];
                        return profile
                            ? {
                                  host: profile.properties.host,
                                  user: profile.properties.user,
                                  privateKey: profile.properties.privateKey,
                              }
                            : undefined;
                    },
                    getProfilePathFromName: (profileName: string) => {
                        return `profiles.${profileName}`;
                    },
                },
                layers: {
                    find: (_profileName: string) => ({
                        user: testConfig.user,
                        global: testConfig.global,
                    }),
                    write: (layerJson: MockConfigLayer) => {
                        writtenLayerJson = cloneDeep(layerJson);
                        // Preserve symbols when capturing written data
                        const preserveSymbols = (source: MockConfigLayer, target: MockConfigLayer) => {
                            if (typeof source === "object" && source !== null) {
                                for (const key of Object.getOwnPropertySymbols(source)) {
                                    (target as Record<symbol, unknown>)[key] = (source as Record<symbol, unknown>)[key];
                                }
                                for (const [key, value] of Object.entries(source)) {
                                    if (
                                        typeof value === "object" &&
                                        value !== null &&
                                        target[key as keyof MockConfigLayer]
                                    ) {
                                        if (key === "properties" && "profiles" in value) {
                                            for (const [profileKey, profileValue] of Object.entries(value.profiles)) {
                                                if (typeof profileValue === "object" && profileValue !== null) {
                                                    preserveSymbols(
                                                        profileValue as MockConfigLayer,
                                                        target.properties.profiles[profileKey] as MockConfigLayer,
                                                    );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        };
                        if (writtenLayerJson) {
                            preserveSymbols(layerJson, writtenLayerJson);
                        }
                    },
                },
            },
        } as Config;
    };

    describe("commentOutProperty", () => {
        it("should comment out a property in JSON file using Config API and comment-json", () => {
            // Start with a config that has the privateKey property
            const testConfig = createMockConfigWithPrivateKey();
            mockTeamConfig = createMockTeamConfig(testConfig);

            const result = ConfigFileUtils.getInstance().commentOutProperty(
                mockTeamConfig,
                "testprofile",
                "privateKey",
            );

            expect(result).toBeDefined();
            expect(result?.propertyPath).toBe("properties.privateKey");
            expect(result?.originalValue).toBe(EXAMPLE_PRIVATE_KEY_PATH);
            expect(result?.layerPath).toBe(MOCK_LAYER_PATH);
            expect(result?.commentText).toContain(EXAMPLE_PRIVATE_KEY_COMMENT);

            // Verify the written config has the comment and no privateKey property
            expect(writtenLayerJson).toBeDefined();
            expect(writtenLayerJson?.properties.profiles.testprofile.properties.privateKey).toBeUndefined();

            const afterPropertiesSymbol = Symbol.for("after:properties");
            const comments = writtenLayerJson?.properties.profiles.testprofile[afterPropertiesSymbol];
            expect(comments).toBeDefined();
            expect(comments).toHaveLength(1);
            expect(comments?.[0].value).toContain(EXAMPLE_PRIVATE_KEY_COMMENT);
        });

        it("should return undefined for non-existent property", () => {
            // Start with a config that doesn't have the privateKey property
            const testConfig: MockConfigLayer = {
                user: false,
                global: false,
                path: MOCK_LAYER_PATH,
                properties: {
                    profiles: {
                        testprofile: {
                            type: "ssh",
                            properties: {
                                host: "example.com",
                                user: "testuser",
                                // No privateKey property
                            },
                        },
                    },
                },
            };
            mockLayerJson = cloneDeep(testConfig);
            mockTeamConfig = createMockTeamConfig(testConfig);

            const result = ConfigFileUtils.getInstance().commentOutProperty(
                mockTeamConfig,
                "testprofile",
                "privateKey",
            );
            expect(result).toBeUndefined();
        });
    });

    describe("uncommentProperty", () => {
        it("should uncomment a previously commented property", () => {
            // Start with a config that has a commented privateKey
            const testConfig = createMockConfigWithCommentedKey();
            mockTeamConfig = createMockTeamConfig(testConfig);

            const success = ConfigFileUtils.getInstance().uncommentProperty(mockTeamConfig, "testprofile", {
                layerPath: MOCK_LAYER_PATH,
                propertyPath: "properties.privateKey",
                originalValue: EXAMPLE_PRIVATE_KEY_PATH,
                commentText: EXAMPLE_PRIVATE_KEY_COMMENT,
            });
            expect(success).toBe(true);

            // Verify the property was restored
            expect(writtenLayerJson).toBeDefined();
            expect(writtenLayerJson?.properties.profiles.testprofile.properties.privateKey).toBe(
                EXAMPLE_PRIVATE_KEY_PATH,
            );

            // Verify the comment was removed
            const afterPropertiesSymbol = Symbol.for("after:properties");
            const comments = writtenLayerJson?.properties.profiles.testprofile[afterPropertiesSymbol];
            expect(comments).toBeUndefined();
        });

        it("should return false when profile is not found in layerJson during uncommentProperty", () => {
            const testConfig = createMockConfigWithCommentedKey();
            mockTeamConfig = createMockTeamConfig(testConfig);

            const success = ConfigFileUtils.getInstance().uncommentProperty(mockTeamConfig, "nonexistentprofile", {
                layerPath: MOCK_LAYER_PATH,
                propertyPath: "properties.privateKey",
                originalValue: EXAMPLE_PRIVATE_KEY_PATH,
                commentText: EXAMPLE_PRIVATE_KEY_COMMENT,
            });
            expect(success).toBe(false);
        });

        it("should return false and log error when uncommentProperty throws", () => {
            const badTeamConfig = {
                api: {
                    layers: {
                        find: () => {
                            throw new Error("Simulated error in layers.find");
                        },
                    },
                },
            } as any;

            const consoleErrorSpy = vi.spyOn(console, "error").mockImplementation(() => {});

            const success = ConfigFileUtils.getInstance().uncommentProperty(badTeamConfig, "testprofile", {
                layerPath: MOCK_LAYER_PATH,
                propertyPath: "properties.privateKey",
                originalValue: EXAMPLE_PRIVATE_KEY_PATH,
                commentText: EXAMPLE_PRIVATE_KEY_COMMENT,
            });

            expect(success).toBe(false);
            expect(consoleErrorSpy).toHaveBeenCalledWith("Error uncommenting property:", expect.any(Error));
            consoleErrorSpy.mockRestore();
        });
    });

    describe("deleteCommentedLine", () => {
        it("should delete comment lines", () => {
            // Start with a config that has a commented privateKey
            const testConfig = createMockConfigWithCommentedKey();
            mockTeamConfig = createMockTeamConfig(testConfig);

            const success = ConfigFileUtils.getInstance().deleteCommentedLine(mockTeamConfig, "testprofile", {
                layerPath: MOCK_LAYER_PATH,
                propertyPath: "properties.privateKey",
                originalValue: EXAMPLE_PRIVATE_KEY_PATH,
                commentText: EXAMPLE_PRIVATE_KEY_COMMENT,
            });
            expect(success).toBe(true);

            // Verify the comment was removed
            expect(writtenLayerJson).toBeDefined();
            const afterPropertiesSymbol = Symbol.for("after:properties");
            const comments = writtenLayerJson?.properties.profiles.testprofile[afterPropertiesSymbol];
            expect(comments).toBeUndefined();
        });

        it("should return false for invalid property path", () => {
            // Start with a basic config without the profile we're trying to access
            const testConfig: MockConfigLayer = {
                user: false,
                global: false,
                path: MOCK_LAYER_PATH,
                properties: {
                    profiles: {
                        // No testprofile here
                    },
                },
            };
            mockLayerJson = cloneDeep(testConfig);
            mockTeamConfig = createMockTeamConfig(testConfig);

            const invalidCommentInfo = {
                layerPath: MOCK_LAYER_PATH,
                propertyPath: "invalid.path",
                originalValue: "test",
            };

            const success = ConfigFileUtils.getInstance().deleteCommentedLine(
                mockTeamConfig,
                "invalidprofile",
                invalidCommentInfo,
            );
            expect(success).toBe(false);
        });

        it("should return false and log error when deleteCommentedLine throws", () => {
            const badTeamConfig = {
                api: {
                    layers: {
                        find: () => {
                            throw new Error("Simulated error in layers.find");
                        },
                    },
                },
            } as any;

            const consoleErrorSpy = vi.spyOn(console, "error").mockImplementation(() => {});

            const success = ConfigFileUtils.getInstance().deleteCommentedLine(badTeamConfig, "testprofile", {
                layerPath: MOCK_LAYER_PATH,
                propertyPath: "properties.privateKey",
                originalValue: EXAMPLE_PRIVATE_KEY_PATH,
                commentText: EXAMPLE_PRIVATE_KEY_COMMENT,
            });

            expect(success).toBe(false);
            expect(consoleErrorSpy).toHaveBeenCalledWith("Error deleting comment lines:", expect.any(Error));
            consoleErrorSpy.mockRestore();
        });

        it("should return undefined in commentOutProperty when layerFromProfName is not found", () => {
            const testConfig = createMockConfigWithPrivateKey();
            const configWithNoLayer = createMockTeamConfig(testConfig);
            configWithNoLayer.api.layers.find = vi.fn().mockReturnValue(undefined);

            const consoleWarnSpy = vi.spyOn(console, "warn").mockImplementation(() => {});

            const result = ConfigFileUtils.getInstance().commentOutProperty(
                configWithNoLayer,
                "testprofile",
                "privateKey",
            );

            expect(result).toBeUndefined();
            expect(consoleWarnSpy).toHaveBeenCalledWith("Could not determine active config layer path");
            consoleWarnSpy.mockRestore();
        });

        it("should unshift comment when comments array already exists in commentOutProperty", () => {
            const testConfig = createMockConfigWithPrivateKey();
            // Pre-initialize comments array
            testConfig.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL] = [
                { type: "LineComment", value: " pre-existing comment", inline: false } as CommentToken,
            ];
            mockLayerJson = cloneDeep(testConfig);
            mockLayerJson.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL] =
                testConfig.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL];
            mockTeamConfig = createMockTeamConfig(testConfig);

            const result = ConfigFileUtils.getInstance().commentOutProperty(
                mockTeamConfig,
                "testprofile",
                "privateKey",
            );

            expect(result).toBeDefined();
            expect(writtenLayerJson).toBeDefined();
            const comments = writtenLayerJson?.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL];
            expect(comments).toHaveLength(2);
            expect(comments?.[0].value).toContain(EXAMPLE_PRIVATE_KEY_COMMENT);
            expect(comments?.[1].value).toBe(" pre-existing comment");
        });

        it("should handle throw in commentOutProperty and return undefined", () => {
            const badTeamConfig = {
                api: {
                    layers: {
                        find: () => {
                            throw new Error("Simulated find error");
                        },
                    },
                },
            } as any;

            const consoleErrorSpy = vi.spyOn(console, "error").mockImplementation(() => {});

            const result = ConfigFileUtils.getInstance().commentOutProperty(badTeamConfig, "testprofile", "privateKey");

            expect(result).toBeUndefined();
            expect(consoleErrorSpy).toHaveBeenCalled();
            consoleErrorSpy.mockRestore();
        });

        it("should return early in removeCommentsInObject when comments are null", () => {
            const obj = {};
            // Call the private method directly
            (ConfigFileUtils.getInstance() as any).removeCommentsInObject(
                obj,
                "some text",
                ConfigFileUtils.AFTER_PROPERTIES_SYMBOL,
            );
            expect(obj[ConfigFileUtils.AFTER_PROPERTIES_SYMBOL]).toBeUndefined();
        });

        it("should retain other comments when matching comment is removed in removeCommentsInObject", () => {
            const testConfig = createMockConfigWithCommentedKey();
            // Add a second non-matching comment
            testConfig.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL].push({
                type: "LineComment",
                value: " another comment",
                inline: false,
            } as CommentToken);
            mockLayerJson = cloneDeep(testConfig);
            mockLayerJson.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL] =
                testConfig.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL];
            mockTeamConfig = createMockTeamConfig(testConfig);

            const success = ConfigFileUtils.getInstance().uncommentProperty(mockTeamConfig, "testprofile", {
                layerPath: MOCK_LAYER_PATH,
                propertyPath: "properties.privateKey",
                originalValue: EXAMPLE_PRIVATE_KEY_PATH,
                commentText: EXAMPLE_PRIVATE_KEY_COMMENT,
            });

            expect(success).toBe(true);
            expect(writtenLayerJson).toBeDefined();
            const comments = writtenLayerJson?.properties.profiles.testprofile[AFTER_PROPERTIES_SYMBOL];
            expect(comments).toHaveLength(1);
            expect(comments?.[0].value).toBe(" another comment");
        });
    });
});
