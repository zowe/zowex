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

import {
    ConfigBuilder,
    ConfigSchema,
    ConfigUtils,
    type IProfileLoaded,
    type IProfileTypeConfiguration,
    type ProfileInfo,
} from "@zowe/imperative";
import type { ISshSession } from "@zowe/zos-uss-for-zowe-sdk";
import { ZoweVsCodeExtension } from "@zowe/zowe-explorer-api";
import { type Config, NodeSSH } from "node-ssh";
import type { ConnectConfig } from "ssh2";
import type { MockInstance } from "vitest";
import type { IDisposable, ProgressCallback } from "../lib";
import { AbstractConfigManager } from "../src/AbstractConfigManager";
import { ConfigFileUtils } from "../src/ConfigFileUtils";
import { type inputBoxOpts, MESSAGE_TYPE, type qpItem, type qpOpts } from "../src/doc";
import { type ISshConfigExt, SshConfigUtils } from "../src/SshConfigUtils";

vi.mock("path", async (importOriginal) => {
    const actual = await importOriginal<typeof import("path")>();
    return {
        ...actual,
        normalize: vi.fn((p: string) => p),
    };
});

vi.mock("ssh2");
vi.mock("node:fs", () => ({
    readFileSync: vi.fn(() => "mocked file content"),
}));
vi.mock("vscode");
vi.mock("@zowe/zowe-explorer-api", () => ({
    ZoweVsCodeExtension: {
        getZoweExplorerApi: () => ({
            getExplorerExtenderApi: () => ({
                getProfilesCache: () => ({
                    getProfileInfo: () =>
                        Promise.resolve({
                            getAllProfiles: (): ProfileInfo => [] as any,
                            getTeamConfig: (): Config => undefined,
                        }),
                }),
            }),
        }),
    },
}));

const sshProfiles: IProfileLoaded[] = [
    {
        message: "",
        name: "ssh1",
        type: "ssh",
        failNotFound: false,
        profile: { host: "lpar1.com", port: 22, user: "zoweUser1", privateKey: "/path/to/id_rsa" },
    },
    {
        message: "",
        name: "ssh2",
        type: "ssh",
        failNotFound: false,
        profile: { host: "lpar2.com", port: 22, user: "zoweUser2", password: "password" },
    },
];

const migratedSshConfigs: ISshConfigExt[] = [
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

const migratedSshConfigsValidation: ISshConfigExt[] = [
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

export class TestAbstractConfigManager extends AbstractConfigManager {
    // All overridden methods are initialized as spies that do nothing
    public showMessage = vi.fn<(message: string, type: MESSAGE_TYPE) => void>();

    public showInputBox = vi.fn<(opts: inputBoxOpts) => Promise<string | undefined>>().mockResolvedValue(undefined);

    public withProgress = vi
        .fn()
        .mockImplementation(
            <T>(_message: string, task: (progress: ProgressCallback) => Promise<T>): Promise<T> => task(() => {}),
        );

    public showMenu = vi.fn<(opts: qpOpts) => Promise<string | undefined>>().mockResolvedValue(undefined);

    public showCustomMenu = vi.fn<(opts: qpOpts) => Promise<qpItem | undefined>>().mockResolvedValue(undefined);

    public getCurrentDir = vi.fn<() => string | undefined>().mockReturnValue("/mock/dir");

    public getProfileSchemas = vi.fn<() => IProfileTypeConfiguration[]>().mockReturnValue([]);

    protected storeServerPath(_host: string, _path: string): void {}

    protected getClientSetting = vi.fn<(setting: keyof ISshSession) => any>().mockReturnValue(15000);

    protected showStatusBar = vi.fn<() => IDisposable | undefined>().mockReturnValue(undefined);
}

describe("AbstractConfigManager", async () => {
    let profCache: ProfileInfo;
    let testManager: TestAbstractConfigManager;
    beforeAll(async () => {
        profCache = await ZoweVsCodeExtension.getZoweExplorerApi()
            .getExplorerExtenderApi()
            .getProfilesCache()
            .getProfileInfo();
    });
    beforeEach(async () => {
        testManager = new TestAbstractConfigManager(profCache);
        vi.spyOn(testManager, "fetchAllSshProfiles" as any).mockReturnValueOnce(sshProfiles);
        vi.spyOn(SshConfigUtils, "migrateSshConfig").mockResolvedValueOnce(migratedSshConfigs);
    });
    describe("promptForProfile", async () => {
        describe("profile selection and input", () => {
            // Common test data
            const mockProfile = { attrs: "fake" };
            const mockMenuItems = [
                { label: "$(plus) Add New SSH Host..." },
                { description: "lpar1.com", label: "ssh1" },
                { description: "lpar2.com", label: "ssh2" },
                { label: "Migrate From SSH Config", separator: true },
                { description: "lpar3.com", label: "SSHlpar3" },
                { description: "lpar4.com", label: "SSHlpar4" },
            ];

            const mockValidConfig = { privateKey: "/path/to/id_rsa", keyPassphrase: "testKP" };
            const mockNewProfile = { name: "sshProfile", port: 22 };

            // Helper function to setup common mocks
            const setupProfileCreationMocks = () => {
                vi.spyOn(testManager as any, "createZoweSchema").mockImplementation(() => {});
                vi.spyOn(testManager as any, "getNewProfileName").mockReturnValue(undefined);
                testManager.getCurrentDir.mockReturnValue(undefined);
            };

            beforeEach(() => {
                vi.clearAllMocks();
            });

            describe("when profile name is provided", () => {
                it("should return merged attributes", async () => {
                    vi.spyOn(testManager as any, "getMergedAttrs").mockReturnValueOnce(mockProfile);

                    const result = await testManager.promptForProfile("testProfile");

                    expect(result).toEqual({
                        profile: mockProfile,
                        message: "",
                        failNotFound: false,
                        type: "ssh",
                    });
                });
            });

            describe("when no profile name is provided", () => {
                it("should display custom menu with correct options", async () => {
                    const showCustomMenuSpy = vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce(undefined);

                    await testManager.promptForProfile();

                    expect(showCustomMenuSpy).toHaveBeenCalledWith({
                        items: mockMenuItems,
                        placeholder: "Select configured SSH host or enter user@host",
                    });
                });

                describe("profile validation", () => {
                    const mockSelection = { description: "lpar1.com", label: "ssh1" };

                    it("should set profile when validation passes with empty config", async () => {
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce(mockSelection);
                        const validateSpy = vi.spyOn(testManager as any, "validateConfig").mockReturnValue({});
                        const setProfileSpy = vi.spyOn(testManager as any, "setProfile").mockImplementation(() => {});

                        await testManager.promptForProfile();

                        expect(validateSpy).toHaveBeenCalled();
                        expect(setProfileSpy).toHaveBeenCalledWith({}, "ssh1");
                    });

                    it("should set profile when validation returns modified config", async () => {
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce(mockSelection);
                        const validateSpy = vi
                            .spyOn(testManager as any, "validateConfig")
                            .mockReturnValue(mockValidConfig);
                        const setProfileSpy = vi.spyOn(testManager as any, "setProfile").mockImplementation(() => {});

                        const result = await testManager.promptForProfile();

                        expect(result).toBeDefined();
                        expect(validateSpy).toHaveBeenCalled();
                        expect(setProfileSpy).toHaveBeenCalledWith(mockValidConfig, "ssh1");
                    });

                    it("should return undefined when validation fails", async () => {
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce(mockSelection);
                        const validateSpy = vi.spyOn(testManager as any, "validateConfig").mockReturnValue(undefined);
                        const setProfileSpy = vi.spyOn(testManager as any, "setProfile");

                        const result = await testManager.promptForProfile();

                        expect(result).toBeUndefined();
                        expect(validateSpy).toHaveBeenCalled();
                        expect(setProfileSpy).not.toHaveBeenCalled();
                    });
                });

                describe("new profile creation", () => {
                    beforeEach(() => {
                        setupProfileCreationMocks();
                    });

                    it("should handle creating new profile with valid input", async () => {
                        const profileWithName = {
                            user: "user1",
                            name: "nameValue",
                            port: 222,
                            privateKey: "/path/to/id_dsa",
                            hostname: "example1.com",
                        };
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce({
                            label: "$(plus) Add New SSH Host...",
                        });
                        vi.spyOn(testManager, "showInputBox").mockResolvedValue(
                            `ssh ${profileWithName.user}@e${profileWithName.hostname} -p ${profileWithName.port} -i ${profileWithName.privateKey}`,
                        );
                        vi.spyOn(testManager as any, "getNewProfileName").mockReturnValue(profileWithName);
                        vi.spyOn(testManager as any, "attemptConnection").mockResolvedValue(true);
                        const createZoweSchemaSpy = vi.spyOn(testManager as any, "createZoweSchema");
                        const setSpy = vi.spyOn(testManager as any, "setProfile").mockImplementation(() => {});
                        expect(await testManager.promptForProfile()).toStrictEqual({
                            name: profileWithName.name,
                            message: "",
                            failNotFound: false,
                            type: "ssh",
                            profile: {
                                host: profileWithName.hostname,
                                port: profileWithName.port,
                                privateKey: profileWithName.privateKey,
                                user: profileWithName.user,
                                name: profileWithName.name,
                                password: undefined,
                                handshakeTimeout: undefined,
                                keyPassphrase: undefined,
                            },
                        });

                        expect(createZoweSchemaSpy).toHaveBeenCalledWith(true);
                        expect(setSpy).toHaveBeenCalledWith(profileWithName);
                    });

                    it("should use project-level config when prioritizeProjectLevelConfig is true and current directory exists", async () => {
                        const profileWithName = {
                            user: "user1",
                            name: "nameValue",
                            port: 222,
                            privateKey: "/path/to/id_dsa",
                            hostname: "example1.com",
                        };
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce({
                            label: "$(plus) Add New SSH Host...",
                        });
                        vi.spyOn(testManager, "showInputBox").mockResolvedValue(
                            `ssh ${profileWithName.user}@${profileWithName.hostname} -p ${profileWithName.port} -i ${profileWithName.privateKey}`,
                        );
                        vi.spyOn(testManager as any, "getNewProfileName").mockReturnValue(profileWithName);
                        vi.spyOn(testManager as any, "attemptConnection").mockResolvedValue(true);
                        testManager.getCurrentDir.mockReturnValue("/mock/project/dir"); // Ensure current directory exists
                        const createZoweSchemaSpy = vi.spyOn(testManager as any, "createZoweSchema");
                        const setSpy = vi.spyOn(testManager as any, "setProfile").mockImplementation(() => {});

                        await testManager.promptForProfile(undefined, {
                            setExistingProfile: true,
                            prioritizeProjectLevelConfig: true,
                        });

                        // When prioritizeProjectLevelConfig=true and getCurrentDir() returns a directory,
                        // createZoweSchema should be called with false (project-level, not global)
                        expect(createZoweSchemaSpy).toHaveBeenCalledWith(false);
                        expect(setSpy).toHaveBeenCalledWith(profileWithName);
                    });

                    it("should use global config when prioritizeProjectLevelConfig is false", async () => {
                        const profileWithName = {
                            user: "user1",
                            name: "nameValue",
                            port: 222,
                            privateKey: "/path/to/id_dsa",
                            hostname: "example1.com",
                        };
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce({
                            label: "$(plus) Add New SSH Host...",
                        });
                        vi.spyOn(testManager, "showInputBox").mockResolvedValue(
                            `ssh ${profileWithName.user}@${profileWithName.hostname} -p ${profileWithName.port} -i ${profileWithName.privateKey}`,
                        );
                        vi.spyOn(testManager as any, "getNewProfileName").mockReturnValue(profileWithName);
                        vi.spyOn(testManager as any, "attemptConnection").mockResolvedValue(true);
                        testManager.getCurrentDir.mockReturnValue("/mock/project/dir"); // Ensure current directory exists
                        const createZoweSchemaSpy = vi.spyOn(testManager as any, "createZoweSchema");
                        const setSpy = vi.spyOn(testManager as any, "setProfile").mockImplementation(() => {});

                        await testManager.promptForProfile(undefined, {
                            setExistingProfile: true,
                            prioritizeProjectLevelConfig: false,
                        });

                        // When prioritizeProjectLevelConfig=false, createZoweSchema should be called with true (global)
                        expect(createZoweSchemaSpy).toHaveBeenCalledWith(true);
                        expect(setSpy).toHaveBeenCalledWith(profileWithName);
                    });

                    it("should use global config when prioritizeProjectLevelConfig is true but current directory is undefined", async () => {
                        const profileWithName = {
                            user: "user1",
                            name: "nameValue",
                            port: 222,
                            privateKey: "/path/to/id_dsa",
                            hostname: "example1.com",
                        };
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce({
                            label: "$(plus) Add New SSH Host...",
                        });
                        vi.spyOn(testManager, "showInputBox").mockResolvedValue(
                            `ssh ${profileWithName.user}@${profileWithName.hostname} -p ${profileWithName.port} -i ${profileWithName.privateKey}`,
                        );
                        vi.spyOn(testManager as any, "getNewProfileName").mockReturnValue(profileWithName);
                        vi.spyOn(testManager as any, "attemptConnection").mockResolvedValue(true);
                        testManager.getCurrentDir.mockReturnValue(undefined); // No current directory
                        const createZoweSchemaSpy = vi.spyOn(testManager as any, "createZoweSchema");
                        const setSpy = vi.spyOn(testManager as any, "setProfile").mockImplementation(() => {});

                        await testManager.promptForProfile(undefined, {
                            setExistingProfile: true,
                            prioritizeProjectLevelConfig: true,
                        });

                        // When prioritizeProjectLevelConfig=true but getCurrentDir() returns undefined,
                        // createZoweSchema should be called with true (global)
                        expect(createZoweSchemaSpy).toHaveBeenCalledWith(true);
                        expect(setSpy).toHaveBeenCalledWith(profileWithName);
                    });

                    it("should return undefined when profile creation fails", async () => {
                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce({
                            label: "$(plus) Add New SSH Host...",
                        });
                        vi.spyOn(testManager as any, "createNewProfile").mockReturnValueOnce(undefined);
                        const createZoweSchemaSpy = vi.spyOn(testManager as any, "createZoweSchema");

                        const result = await testManager.promptForProfile();

                        expect(createZoweSchemaSpy).not.toHaveBeenCalled();
                        expect(result).toBeUndefined();
                    });
                });

                describe("custom SSH host input", () => {
                    it("should handle custom SSH host entry", async () => {
                        const customHostSelection = {
                            description: "Custom SSH Host",
                            label: "ssh user@example.com",
                        };

                        vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce(customHostSelection);
                        const createNewProfileSpy = vi
                            .spyOn(testManager as any, "createNewProfile")
                            .mockReturnValueOnce(mockNewProfile);
                        setupProfileCreationMocks();

                        await testManager.promptForProfile();

                        expect(createNewProfileSpy).toHaveBeenCalledWith("ssh user@example.com");
                    });
                });
            });
        });
        describe("profile validation sequence", async () => {
            // Common mock data
            const mockProfileData = {
                port: 22,
                host: "lpar1.com",
                user: "zoweUser",
                privateKey: "/path/to/id_rsa",
            };

            const mockProfileWithoutKey = {
                port: 22,
                host: "lpar1.com",
                user: "zoweUser",
            };

            // Helper function to setup common mocks
            const setupCommonMocks = (
                profileData: typeof mockProfileData | typeof mockProfileWithoutKey = mockProfileData,
            ) => {
                vi.spyOn(testManager, "showCustomMenu").mockResolvedValueOnce({
                    label: "$(plus) Add New SSH Host...",
                });
                vi.spyOn(testManager as any, "createNewProfile").mockReturnValueOnce(profileData);
                vi.spyOn(testManager as any, "getNewProfileName").mockReturnValueOnce({
                    name: "newProfileName",
                    ...profileData,
                });
                testManager.getCurrentDir.mockReturnValue(undefined);
            };

            beforeEach(async () => {
                vi.spyOn(testManager as any, "validateConfig").mockReturnValue({});
                vi.spyOn(testManager as any, "setProfile").mockImplementation(() => {});
            });

            describe("SSH config validation scenarios", () => {
                it("should validate migrated SSH config with valid privateKey", async () => {
                    setupCommonMocks();
                    vi.spyOn(testManager as any, "validateConfig").mockReturnValue({});

                    const result = await testManager.promptForProfile();

                    expect(result).toBeDefined();
                });

                it("should find local privateKey when attached key is invalid", async () => {
                    setupCommonMocks();
                    vi.spyOn(testManager as any, "validateConfig").mockReturnValue(undefined);

                    const validatePrivKeySpy = vi
                        .spyOn(testManager as any, "validateFoundPrivateKeys")
                        .mockImplementationOnce(() => {
                            (testManager as any).validationResult = {};
                        });

                    const result = await testManager.promptForProfile();

                    expect(result).toBeDefined();
                    expect(validatePrivKeySpy).toHaveBeenCalledOnce();
                });

                it("should handle invalid privateKey with available local key", async () => {
                    setupCommonMocks();
                    vi.spyOn(testManager as any, "validateConfig")
                        .mockReturnValueOnce(undefined)
                        .mockReturnValueOnce({ privateKey: "/path/to/id_rsa" });

                    const validatePrivKeySpy = vi
                        .spyOn(testManager as any, "validateFoundPrivateKeys")
                        .mockImplementationOnce(() => {});

                    const result = await testManager.promptForProfile();

                    expect(result).toBeDefined();
                    expect(validatePrivKeySpy).toHaveBeenCalledOnce();
                });

                it("should cancel SSH setup when no privateKey is available", async () => {
                    setupCommonMocks(mockProfileWithoutKey);
                    vi.spyOn(testManager as any, "validateConfig").mockReturnValue(undefined);

                    const validatePrivKeySpy = vi
                        .spyOn(testManager as any, "validateFoundPrivateKeys")
                        .mockImplementationOnce(() => {});

                    const result = await testManager.promptForProfile();

                    expect(result).toBeUndefined();
                    expect(validatePrivKeySpy).toHaveBeenCalledOnce();
                    expect(testManager.showMessage).toHaveBeenCalledWith("SSH setup cancelled.", MESSAGE_TYPE.WARNING);
                });
            });
        });
    });
    const defaultServerPath = "/faketmp/fakeserver";
    describe("promptForDeployDirectory", () => {
        const host = "testHost";
        it("returns default path if user presses enter without changing", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValue(defaultServerPath);
            const storeMock = vi.spyOn(testManager as any, "storeServerPath").mockImplementation(() => {});

            const result = await testManager.promptForDeployDirectory(host, defaultServerPath);

            expect(result).toBe(defaultServerPath);
            expect(storeMock).not.toHaveBeenCalled();
        });

        it("returns trimmed user input if valid absolute path is entered", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValue("   /custom/path   ");
            const storeMock = vi.spyOn(testManager as any, "storeServerPath").mockImplementation(() => {});

            const result = await testManager.promptForDeployDirectory(host, defaultServerPath);

            expect(result).toBe("/custom/path");
            expect(storeMock).toHaveBeenCalledWith(host, "/custom/path");
        });

        it("returns undefined if user cancels input", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValue(undefined);
            const showMessageMock = vi.spyOn(testManager, "showMessage").mockImplementation(() => {});
            const storeMock = vi.spyOn(testManager as any, "storeServerPath").mockImplementation(() => {});

            const result = await testManager.promptForDeployDirectory(host, defaultServerPath);

            expect(result).toBeUndefined();
            expect(showMessageMock).toHaveBeenCalled();
            expect(storeMock).not.toHaveBeenCalled();
        });
    });

    describe("validateDeployPath", () => {
        it("should return 'Path cannot be empty' when trimmed path is empty", () => {
            const result = TestAbstractConfigManager.validateDeployPath("   ");
            expect(result).toBe("Path cannot be empty.");
        });
        it("should return invalid path format when path is >1024 chars", () => {
            const result = TestAbstractConfigManager.validateDeployPath("/a".repeat(1025));
            expect(result).toBe("Path is longer than the USS max path length of 1024.");
        });
        it("should return null for paths starting with ~ (valid path)", async () => {
            const result = TestAbstractConfigManager.validateDeployPath("~/.zowe-ssh");
            expect(result).toBeNull();
        });

        it("should return null for paths with ~ in the middle (valid path)", async () => {
            const replaceSpy = vi.spyOn(String.prototype, "replace");
            const multipleTildes = "~/.projects/zowe~ssh";
            const result = TestAbstractConfigManager.validateDeployPath(multipleTildes);
            expect(replaceSpy).toHaveLastReturnedWith(multipleTildes.substring(1));
            expect(result).toBeNull();
        });
    });

    describe("createNewProfile", async () => {
        let showInputBoxSpy: any;

        beforeEach(() => {
            showInputBoxSpy = vi.spyOn(testManager, "showInputBox");
        });

        describe("valid inputs", () => {
            it("should parse basic SSH connection", async () => {
                showInputBoxSpy.mockResolvedValue("ssh user@example.com");

                const result = await (testManager as any).createNewProfile();

                expect(result).toStrictEqual({
                    user: "user",
                    hostname: "example.com",
                });
            });

            it("should parse SSH connection with port and identity file", async () => {
                showInputBoxSpy.mockResolvedValue("ssh user@example.com -p 22 -i /path/to/id_rsa");

                const result = await (testManager as any).createNewProfile();

                expect(result).toStrictEqual({
                    user: "user",
                    hostname: "example.com",
                    port: 22,
                    privateKey: "/path/to/id_rsa",
                });
            });

            it("should accept pre-configured connection string", async () => {
                const result = await (testManager as any).createNewProfile("user@example.com");

                expect(result).toBeDefined();
            });
        });

        describe("invalid inputs", () => {
            const testCases = [
                {
                    name: "should handle cancelled input",
                    input: undefined,
                    expectedMessage: "SSH setup cancelled.",
                    expectedType: MESSAGE_TYPE.WARNING,
                },
                {
                    name: "should reject malformed SSH command",
                    input: "bad ssh input",
                    expectedMessage: "Invalid SSH command format. Ensure it matches the expected pattern.",
                    expectedType: MESSAGE_TYPE.ERROR,
                },
                {
                    name: "should reject invalid port",
                    input: "ssh user@example.com -p badPort",
                    expectedMessage: "Invalid value for flag -p. Port must be a valid number.",
                    expectedType: MESSAGE_TYPE.ERROR,
                },
                {
                    name: "should reject missing port value",
                    input: "ssh user@example.com -p",
                    expectedMessage: "Missing value for flag -p.",
                    expectedType: MESSAGE_TYPE.ERROR,
                },
                {
                    name: "should reject unquoted paths with spaces",
                    input: "ssh user@example.com -i /path/with spaces/keyfile",
                    expectedMessage: "Invalid value for flag -i. Values with spaces must be quoted.",
                    expectedType: MESSAGE_TYPE.ERROR,
                    setup: () => {
                        vi.spyOn((testManager as any).flagRegex, "exec").mockReturnValueOnce([
                            "-i /path/with spaces/keyfile",
                            "i",
                            "/path/with spaces/keyfile",
                        ]);
                    },
                },
            ];

            testCases.forEach(({ name, input, expectedMessage, expectedType, setup }) => {
                it(name, async () => {
                    if (setup) setup();
                    if (input !== undefined) showInputBoxSpy.mockResolvedValue(input);

                    const result = await (testManager as any).createNewProfile();

                    expect(result).toBeUndefined();
                    expect(testManager.showMessage).toHaveBeenCalledWith(expectedMessage, expectedType);
                });
            });

            it("should reject invalid pre-configured connection string", async () => {
                const result = await (testManager as any).createNewProfile("bad config options");

                expect(result).toBeUndefined();
                expect(testManager.showMessage).toHaveBeenCalledWith(
                    "Invalid custom connection format. Ensure it matches the expected pattern.",
                    MESSAGE_TYPE.ERROR,
                );
            });
        });
    });
    describe("createZoweSchema", async () => {
        let testManager: TestAbstractConfigManager;
        let mockTeamConfig: any;
        let mockConfigApi: any;
        let mockLayers: any;

        const baseSchema = {
            type: "base",
            schema: { title: "Base Profile", description: "Base connection info", type: "object", properties: {} },
        };
        const zosmfSchema = {
            type: "zosmf",
            schema: { title: "z/OSMF Profile", description: "z/OSMF connection info", type: "object", properties: {} },
        };
        const defaultSchemas = [baseSchema, zosmfSchema];
        const homeDir = "/home/user/.zowe";
        const workspaceDir = "/mock/dir";

        beforeEach(async () => {
            mockLayers = { activate: vi.fn(), merge: vi.fn(), write: vi.fn() };
            mockConfigApi = { layers: mockLayers };
            mockTeamConfig = { layerExists: vi.fn(), api: mockConfigApi, setSchema: vi.fn() };

            const profCache = await ZoweVsCodeExtension.getZoweExplorerApi()
                .getExplorerExtenderApi()
                .getProfilesCache()
                .getProfileInfo();

            testManager = new TestAbstractConfigManager(profCache);
            vi.spyOn((testManager as any).mProfilesCache, "getTeamConfig").mockReturnValue(mockTeamConfig);
            testManager.getProfileSchemas.mockReturnValue(defaultSchemas);
        });

        async function runCreateSchema(globalFlag: boolean, layerExists: boolean, profileSchemas = defaultSchemas) {
            vi.spyOn(ConfigUtils, "getZoweDir").mockReturnValue(homeDir);
            if (!globalFlag) testManager.getCurrentDir.mockReturnValue(workspaceDir);
            testManager.getProfileSchemas.mockReturnValue(profileSchemas);
            mockTeamConfig.layerExists.mockReturnValue(layerExists);

            const mockNewConfig = { api: mockConfigApi, exists: true, layers: [] as any };
            vi.spyOn(ConfigSchema, "buildSchema").mockReturnValue({
                type: "object",
                description: "Test Schema",
                properties: {},
            } as any);
            vi.spyOn(ConfigBuilder, "build").mockResolvedValue(mockNewConfig as any);

            await (testManager as any).createZoweSchema(globalFlag);
            return { mockNewConfig };
        }

        describe.each`
            globalFlag | expectedDir
            ${true}    | ${homeDir}
            ${false}   | ${workspaceDir}
        `("layer exists (global=$globalFlag)", ({ globalFlag, expectedDir }) => {
            it("should return early if layer already exists", async () => {
                vi.spyOn(ConfigUtils, "getZoweDir").mockReturnValue(homeDir);
                if (!globalFlag) testManager.getCurrentDir.mockReturnValue(workspaceDir);
                mockTeamConfig.layerExists.mockReturnValue(true);

                await (testManager as any).createZoweSchema(globalFlag);

                expect(mockTeamConfig.layerExists).toHaveBeenCalledWith(expectedDir);
                expect(mockLayers.activate).not.toHaveBeenCalled();
                expect(mockTeamConfig.setSchema).not.toHaveBeenCalled();
            });
        });

        describe.each`
            globalFlag | expectedDesc
            ${true}    | ${"Test Schema"}
            ${false}   | ${"Test Schema"}
        `("schema creation success (global=$globalFlag)", ({ globalFlag, expectedDesc }) => {
            it("should create schema when layer doesn't exist", async () => {
                const { mockNewConfig } = await runCreateSchema(globalFlag, false);
                expect(mockLayers.activate).toHaveBeenCalledWith(false, globalFlag);
                expect(ConfigSchema.buildSchema).toHaveBeenCalledWith(defaultSchemas);
                expect(mockTeamConfig.setSchema).toHaveBeenCalledWith(
                    expect.objectContaining({ description: expectedDesc }),
                );
                expect(mockLayers.merge).toHaveBeenCalledWith(mockNewConfig);
                expect(mockLayers.write).toHaveBeenCalled();
            });
        });
    });
    describe("validateConfig", () => {
        it("should return an empty object for a valid profile", async () => {
            vi.spyOn(testManager as any, "attemptConnection").mockResolvedValue(true);
            expect(
                await (testManager as any).validateConfig(
                    { name: "ssh1", hostname: "lpar1.com", port: 22, user: "user1", privateKey: "/path/to/id_rsa" },
                    false,
                ),
            ).toStrictEqual({});
        });

        it("should return a password for a partial profile", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValueOnce("user1").mockResolvedValueOnce("password1");
            vi.spyOn(testManager as any, "attemptConnection").mockResolvedValue(true);
            vi.spyOn(testManager as any, "promptForPassword").mockResolvedValue({ password: "password1" });

            expect(
                await (testManager as any).validateConfig({ name: "ssh1", hostname: "lpar1.com", port: 22 }, true),
            ).toStrictEqual({ password: "password1", user: "user1" });
        });

        it("should return undefined for partial profile when no password prompt allowed", async () => {
            vi.spyOn(testManager, "showInputBox");
            vi.spyOn(testManager as any, "attemptConnection").mockResolvedValue(true);
            expect(
                await (testManager as any).validateConfig(
                    { name: "ssh1", hostname: "lpar1.com", port: 22, user: "user1" },
                    false,
                ),
            ).toBeUndefined();
        });

        it("should handle invalid username and return new user", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValueOnce("goodUser");
            vi.spyOn(testManager as any, "attemptConnection")
                .mockRejectedValueOnce("Invalid username")
                .mockResolvedValueOnce(true);

            expect(
                await (testManager as any).validateConfig(
                    {
                        name: "ssh1",
                        hostname: "lpar1.com",
                        port: 22,
                        privateKey: "/path/to/id_rsa",
                        user: "badUser",
                    },
                    true,
                ),
            ).toStrictEqual({ user: "goodUser" });
        });

        it("should return undefined if Invalid username and no new user provided", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValueOnce(undefined);
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValueOnce("Invalid username");

            expect(
                await (testManager as any).validateConfig(
                    {
                        name: "ssh1",
                        hostname: "lpar1.com",
                        port: 22,
                        privateKey: "/path/to/id_rsa",
                        user: "badUser",
                    },
                    true,
                ),
            ).toBeUndefined();
        });

        it("should handle passphrase errors and return keyPassphrase", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValueOnce("goodPass");
            vi.spyOn(testManager as any, "attemptConnection")
                .mockRejectedValueOnce("but no passphrase given")
                .mockResolvedValueOnce(true);

            expect(
                await (testManager as any).validateConfig(
                    { name: "ssh1", hostname: "lpar1.com", port: 22, privateKey: "/path/to/id_rsa", user: "user1" },
                    true,
                ),
            ).toStrictEqual({ keyPassphrase: "goodPass" });
        });

        it("should retry passphrase on integrity check failed", async () => {
            const msgSpy = vi.spyOn(testManager, "showMessage");
            vi.spyOn(testManager, "showInputBox").mockResolvedValueOnce("badPass").mockResolvedValueOnce("goodPass");
            vi.spyOn(testManager as any, "attemptConnection")
                .mockRejectedValueOnce("integrity check failed")
                .mockRejectedValueOnce("integrity check failed")
                .mockResolvedValueOnce(true);

            expect(
                await (testManager as any).validateConfig(
                    { name: "ssh1", hostname: "lpar1.com", port: 22, privateKey: "/path/to/id_rsa", user: "user1" },
                    true,
                ),
            ).toStrictEqual({ keyPassphrase: "goodPass" });

            expect(msgSpy).toHaveBeenCalledWith(
                expect.stringContaining("Passphrase Authentication Failed"),
                expect.anything(),
            );
        });

        it("should handle All configured authentication methods failed with password prompt", async () => {
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValue(
                "All configured authentication methods failed",
            );
            vi.spyOn(testManager as any, "promptForPassword").mockResolvedValue({ password: "pass123" });

            expect(
                await (testManager as any).validateConfig(
                    { name: "ssh1", hostname: "lpar1.com", port: 22, user: "user1" },
                    true,
                ),
            ).toStrictEqual({ password: "pass123" });
        });

        it("should return undefined for handshake timeout", async () => {
            const msgSpy = vi.spyOn(testManager, "showMessage");
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValue(
                new Error("Timed out while waiting for handshake"),
            );

            expect(
                await (testManager as any).validateConfig(
                    { name: "ssh1", hostname: "lpar1.com", port: 22, user: "user1", password: "badPassword" },
                    true,
                ),
            ).toBeUndefined();
            expect(msgSpy).toHaveBeenCalledWith("Timed out while waiting for handshake", MESSAGE_TYPE.ERROR);
        });

        it("should return undefined for malformed private key", async () => {
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValue(
                "Cannot parse privateKey: Malformed OpenSSH private key",
            );
            vi.spyOn(ConfigFileUtils.getInstance(), "commentOutProperty").mockReturnValue(undefined);
            // Skip past autoStore = false check to validate logic below it
            const getTeamConfigMock = vi
                .spyOn((testManager as any).mProfilesCache, "getTeamConfig")
                .mockReturnValueOnce({
                    properties: { autoStore: false },
                });

            expect(
                await (testManager as any).validateConfig(
                    { name: "ssh1", hostname: "lpar1.com", port: 22, user: "user1", privateKey: "/path/to/id_rsa" },
                    true,
                ),
            ).toBeUndefined();
            expect(getTeamConfigMock).toHaveBeenCalledTimes(1);
        });
        it("should return undefined for privateKey with no password if 'All configured authentication methods failed'", async () => {
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValue(
                new Error("All configured authentication methods failed"),
            );
            vi.spyOn(ConfigFileUtils.getInstance(), "commentOutProperty").mockReturnValueOnce(undefined);
            // Skip past autoStore = false check to validate logic below it
            const getTeamConfigMock = vi
                .spyOn((testManager as any).mProfilesCache, "getTeamConfig")
                .mockReturnValueOnce({
                    properties: { autoStore: false },
                });

            const config = {
                name: "ssh1",
                hostname: "lpar1.com",
                port: 22,
                user: "user1",
                privateKey: "/path/to/key",
            };
            const result = await (testManager as any).validateConfig(config, false);
            expect(getTeamConfigMock).toHaveBeenCalledTimes(1);

            expect(result).toBeUndefined();
        });
        it("should return undefined if attemptConnection fails for new user", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValue("newUser");
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValue(new Error("Invalid username"));

            const result = await (testManager as any).validateConfig(
                { name: "ssh1", hostname: "lpar1.com", port: 22, user: "badUser", privateKey: "/path/to/id_rsa" },
                true,
            );

            expect(result).toBeUndefined();
        });

        it("should return undefined if connection attempt is made with an expired password", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValue("newUser");
            const errorText =
                "FOTS1668 WARNING: Your password has expired.\nFOTS1669 Password change required but no TTY available.";
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValue(new Error(errorText));
            const showMessageSpy = vi.spyOn(testManager, "showMessage");
            const result = await (testManager as any).validateConfig(
                { name: "ssh1", hostname: "lpar1.com", port: 22, user: "badUser", password: "expired" },
                true,
            );

            expect(showMessageSpy).toHaveBeenCalledWith(`Error: ${errorText}`, MESSAGE_TYPE.ERROR);
            expect(result).toBeUndefined();
        });

        it("should clear privateKey and keyPassphrase after 3 failed passphrase attempts", async () => {
            vi.spyOn(testManager, "showInputBox").mockResolvedValue("wrongPass"); // always fails
            vi.spyOn(testManager as any, "attemptConnection").mockRejectedValue(new Error("integrity check failed"));
            vi.spyOn(testManager, "showMessage").mockImplementation(() => {});
            const config = {
                name: "ssh1",
                hostname: "lpar1.com",
                port: 22,
                user: "user1",
                privateKey: "/path/to/key",
            };
            const handleInvalidPrivateKeyMock = vi
                .spyOn(testManager as any, "handleInvalidPrivateKey")
                .mockResolvedValue(true);

            const result = await (testManager as any).validateConfig(config, true);

            expect(result).toBeUndefined();
            expect(handleInvalidPrivateKeyMock).toHaveBeenCalledTimes(1);
            expect(config.privateKey).toBeUndefined();
            expect((config as any).keyPassphrase).toBeUndefined();
        });

        it("should call promptForPassword when password missing and askForPassword is true", async () => {
            vi.spyOn(testManager as any, "attemptConnection").mockImplementation(() => {
                throw new Error("All configured authentication methods failed");
            });
            vi.spyOn(testManager as any, "promptForPassword").mockResolvedValue({ password: "pw123" });

            const result = await (testManager as any).validateConfig(
                {
                    name: "ssh1",
                    hostname: "lpar1.com",
                    port: 22,
                    user: "user1",
                    // Setting password to exit check
                    password: "badPw",
                    privateKey: undefined,
                },
                true,
            );

            expect(result).toStrictEqual({ password: "pw123" });
        });
        it("should call promptForPassword when password missing and askForPassword is false", async () => {
            vi.spyOn(testManager as any, "attemptConnection").mockImplementation(() => {
                throw new Error("All configured authentication methods failed");
            });

            const result = await (testManager as any).validateConfig(
                {
                    name: "ssh1",
                    hostname: "lpar1.com",
                    port: 22,
                    user: "user1",
                    // Setting password to exit check
                    password: "badPw",
                    privateKey: undefined,
                },
                false,
            );

            expect(result).toBeUndefined();
        });
        it("should remove privateKey and retry using password when All configured authentication methods failed", async () => {
            const attemptConnectionSpy = vi
                .spyOn(testManager as any, "attemptConnection")
                .mockRejectedValueOnce(new Error("All configured authentication methods failed"))
                .mockResolvedValueOnce(true);
            const handleInvalidPrivateKeySpy = vi
                .spyOn(testManager as any, "handleInvalidPrivateKey")
                .mockResolvedValue(true);

            const config = {
                name: "ssh1",
                hostname: "lpar1.com",
                port: 22,
                user: "user1",
                privateKey: "/path/to/key",
                password: "test",
            };

            const result = await (testManager as any).validateConfig(config, true);
            expect(config.privateKey).toBeUndefined();

            expect(result).toStrictEqual({});

            expect(handleInvalidPrivateKeySpy).toHaveBeenCalledTimes(1);
            expect(attemptConnectionSpy).toHaveBeenCalledTimes(2);
        });
    });
    describe("attemptConnection", async () => {
        it("should attempt connection and have a truthy result", async () => {
            const connectMock = vi.spyOn(NodeSSH.prototype, "connect").mockResolvedValueOnce(undefined);
            const isConnectedMock = vi.spyOn(NodeSSH.prototype, "isConnected").mockReturnValueOnce(true);
            const execCommandMock = vi.spyOn(NodeSSH.prototype, "execCommand").mockImplementation(() => {
                return { stdout: "" } as any;
            });
            await expect(
                (testManager as any).attemptConnection({
                    name: "testProf",
                    privateKey: "/path/to/id_rsa",
                    port: 22,
                    user: "user1",
                }),
            ).resolves.not.toThrow();
            expect(connectMock).toHaveBeenCalledTimes(1);
            expect(isConnectedMock).toHaveBeenCalledTimes(1);
            expect(execCommandMock).toHaveBeenCalledTimes(1);
            connectMock.mockRestore();
            isConnectedMock.mockRestore();
            execCommandMock.mockRestore();
        });

        it("should throw an error on connection attempt", async () => {
            const connectMock = vi
                .spyOn(NodeSSH.prototype, "connect")
                .mockImplementation((_config: ConnectConfig) => undefined);
            const isConnectedMock = vi.spyOn(NodeSSH.prototype, "isConnected").mockReturnValueOnce(true);
            const execCommandMock = vi.spyOn(NodeSSH.prototype, "execCommand").mockImplementation(() => {
                return { stderr: "FOTS1668" } as any;
            });
            await expect(
                (testManager as any).attemptConnection({
                    name: "testProf",
                    privateKey: "/path/to/id_rsa",
                    port: 22,
                    user: "user1",
                }),
            ).rejects.toThrow("FOTS1668");
            expect(connectMock).toHaveBeenCalledTimes(1);
            expect(isConnectedMock).toHaveBeenCalledTimes(1);
            expect(execCommandMock).toHaveBeenCalledTimes(1);
        });
        it("should throw leverage the default handshaketimeout setting if no value is passed on config", async () => {
            const connectMock = vi
                .spyOn(NodeSSH.prototype, "connect")
                .mockImplementation((_config: ConnectConfig) => undefined);
            const isConnectedMock = vi.spyOn(NodeSSH.prototype, "isConnected").mockReturnValueOnce(true);
            const execCommandMock = vi.spyOn(NodeSSH.prototype, "execCommand").mockImplementation(() => {
                return { stderr: "FOTS1668" } as any;
            });
            await expect(
                (testManager as any).attemptConnection({
                    name: "testProf",
                    privateKey: "/path/to/id_rsa",
                    port: 22,
                    user: "user1",
                }),
            ).rejects.toThrow("FOTS1668");
            expect(connectMock).toHaveBeenCalledWith(expect.objectContaining({ readyTimeout: 15000 }));
            expect(connectMock).toHaveBeenCalledTimes(1);
            expect(isConnectedMock).toHaveBeenCalledTimes(1);
            expect(execCommandMock).toHaveBeenCalledTimes(1);
        });
    });
    describe("promptForPassword", async () => {
        it("returns undefined if user cancels input immediately", async () => {
            testManager.showInputBox = vi.fn().mockResolvedValue(undefined);
            (testManager as any).attemptConnection = vi.fn();

            const result = await (testManager as any).promptForPassword({ user: "user1", hostname: "host" }, {});
            expect(result).toBeUndefined();
            expect((testManager as any).attemptConnection).not.toHaveBeenCalled();
        });

        it("returns password if attemptConnection succeeds on first try", async () => {
            testManager.showInputBox = vi.fn().mockResolvedValue("password123");
            (testManager as any).attemptConnection = vi.fn().mockResolvedValue(true);
            testManager.showMessage = vi.fn();

            const result = await (testManager as any).promptForPassword({ user: "user1", hostname: "host" }, {});
            expect(result).toEqual({ password: "password123" });
            expect((testManager as any).attemptConnection).toHaveBeenCalledTimes(1);
            expect(testManager.showMessage).not.toHaveBeenCalled();
        });

        it("retries up to 3 times and shows error messages on failure", async () => {
            testManager.showInputBox = vi.fn().mockResolvedValue("wrongpass");
            (testManager as any).attemptConnection = vi.fn().mockRejectedValue(new Error("Auth Failed"));
            testManager.showMessage = vi.fn();

            const result = await (testManager as any).promptForPassword({ user: "user1", hostname: "host" }, {});
            expect(result).toBeUndefined();
            expect((testManager as any).attemptConnection).toHaveBeenCalledTimes(3);
            // One message after each attempt and one final one after 3 failures
            expect(testManager.showMessage).toHaveBeenCalledTimes(3 + 1);
            expect(testManager.showMessage).toHaveBeenCalledWith(
                "Password Authentication Failed (1/3)",
                MESSAGE_TYPE.ERROR,
            );
        });

        it("returns undefined and shows expired message if error contains FOTS1668", async () => {
            testManager.showInputBox = vi.fn().mockResolvedValue("expiredPass");
            (testManager as any).attemptConnection = vi.fn().mockRejectedValue(new Error("FOTS1668: password expired"));
            testManager.showMessage = vi.fn();

            const result = await (testManager as any).promptForPassword({ user: "user1", hostname: "host" }, {});
            expect(result).toBeUndefined();
            expect(testManager.showMessage).toHaveBeenCalledWith(
                "Password Expired on Target System",
                MESSAGE_TYPE.ERROR,
            );
            expect((testManager as any).attemptConnection).toHaveBeenCalledTimes(1);
        });
    });
    describe("validateFoundPrivateKeys", () => {
        const baseProfile = { port: 22, user: "user1", hostname: "lpar1.com" };
        const expectedProfileWithKey = { ...baseProfile, privateKey: "/path/to/id_dsa" };
        const expectedProfileWithRsaKey = { ...baseProfile, privateKey: "/Users/users/.ssh/id_rsa" };

        let findPrivateKeysSpy: MockInstance;
        let validateConfigSpy: MockInstance;

        beforeEach(() => {
            // Reset test manager state
            (testManager as any).selectedProfile = { ...baseProfile };
            (testManager as any).validationResult = undefined;
            (testManager as any).migratedConfigs = undefined;

            // Create fresh spies for each test
            findPrivateKeysSpy = vi.spyOn(SshConfigUtils, "findPrivateKeys");
            validateConfigSpy = vi.spyOn(testManager as any, "validateConfig");
        });
        it("should modify a profile with a found private key", async () => {
            const mockPrivateKeys = ["/path/to/id_rsa", "/path/to/id_ecdsa", "/path/to/id_dsa"];

            findPrivateKeysSpy.mockResolvedValue(mockPrivateKeys);
            validateConfigSpy
                .mockReturnValueOnce(undefined)
                .mockReturnValueOnce(undefined)
                .mockReturnValueOnce({ privateKey: "/path/to/id_dsa" });

            await (testManager as any).validateFoundPrivateKeys();

            expect((testManager as any).validationResult).toStrictEqual({});
            expect((testManager as any).selectedProfile).toStrictEqual(expectedProfileWithKey);
            expect(validateConfigSpy).toHaveBeenCalledTimes(3);
        });

        it("should validate with private keys found with two matching hostname configs", async () => {
            findPrivateKeysSpy.mockResolvedValue([]);
            (testManager as any).migratedConfigs = migratedSshConfigsValidation;
            validateConfigSpy.mockReturnValueOnce({});

            await (testManager as any).validateFoundPrivateKeys();

            expect((testManager as any).validationResult).toStrictEqual({});
            expect((testManager as any).selectedProfile).toStrictEqual(expectedProfileWithRsaKey);
            expect(validateConfigSpy).toHaveBeenCalledTimes(1);
        });

        it("should validate with private keys found with one matching hostname config", async () => {
            findPrivateKeysSpy.mockResolvedValue([]);
            (testManager as any).migratedConfigs = migratedSshConfigsValidation.slice(0, -1);
            validateConfigSpy.mockReturnValueOnce({ privateKey: "/Users/users/.ssh/id_rsa" });

            await (testManager as any).validateFoundPrivateKeys();

            expect((testManager as any).validationResult).toStrictEqual({});
            expect((testManager as any).selectedProfile).toStrictEqual(expectedProfileWithRsaKey);
            expect(validateConfigSpy).toHaveBeenCalledTimes(1);
        });
    });
    describe("setProfile", () => {
        let testManager: TestAbstractConfigManager;
        let mockTeamConfig: any;
        let mockConfigApi: any;
        let mockLayers: any;
        let mockProfiles: any;

        beforeEach(async () => {
            mockLayers = {
                activate: vi.fn(),
                merge: vi.fn(),
                write: vi.fn(),
                get: vi.fn().mockReturnValue({ properties: { defaults: { ssh: null } } }),
            };
            mockProfiles = {
                defaultGet: vi.fn(),
                defaultSet: vi.fn(),
                set: vi.fn(),
                getProfileNameFromPath: vi.fn(),
            };
            mockConfigApi = { layers: mockLayers, profiles: mockProfiles };
            mockTeamConfig = {
                layerExists: vi.fn(),
                api: mockConfigApi,
                setSchema: vi.fn(),
                save: vi.fn(),
            };
            testManager = new TestAbstractConfigManager(profCache);
            vi.spyOn((testManager as any).mProfilesCache, "getTeamConfig").mockReturnValue(mockTeamConfig);
            // Mock methods that may or may not exist on mProfilesCache
            const mockMergeArgsForProfile = vi.fn().mockReturnValue({ knownArgs: [] });
            const mockUpdateProperty = vi.fn();
            const mockIsSecured = vi.fn().mockReturnValue(false);

            // Add methods to mProfilesCache if they don't exist
            (testManager as any).mProfilesCache.mergeArgsForProfile = mockMergeArgsForProfile;
            (testManager as any).mProfilesCache.updateProperty = mockUpdateProperty;
            (testManager as any).mProfilesCache.isSecured = mockIsSecured;

            vi.spyOn(testManager as any, "fetchDefaultProfile").mockReturnValue({ name: "default" });
            vi.spyOn(testManager as any, "showMenu").mockResolvedValue("Yes");
        });

        it("should create profile with keyPassphrase and mark it as secure", async () => {
            const config = {
                user: "user1",
                host: "example.com",
                keyPassphrase: "passphrase123",
                name: "testProfile",
            };

            await (testManager as any).setProfile(config);

            expect(mockConfigApi.profiles.set).toHaveBeenCalledWith(
                "testProfile",
                expect.objectContaining({
                    secure: ["user", "keyPassphrase"],
                }),
            );
            expect(mockTeamConfig.save).toHaveBeenCalled();
        });

        it("should mark both password and keyPassphrase as secure when both are present", async () => {
            const config = {
                user: "user1",
                host: "example.com",
                password: "secret123",
                keyPassphrase: "passphrase123",
                name: "testProfile",
            };

            await (testManager as any).setProfile(config);

            expect(mockConfigApi.profiles.set).toHaveBeenCalledWith(
                "testProfile",
                expect.objectContaining({
                    secure: ["user", "password", "keyPassphrase"],
                }),
            );
        });

        it("should call layers.write() when no secure properties are present", async () => {
            const config = {
                host: "example.com",
                port: 22,
                name: "testProfile",
            };

            await (testManager as any).setProfile(config);

            expect(mockConfigApi.layers.write).toHaveBeenCalled();
            expect(mockTeamConfig.save).not.toHaveBeenCalled();
        });

        it("should set default SSH profile when no default exists", async () => {
            mockConfigApi.profiles.defaultGet.mockReturnValue(null);
            mockLayers.get.mockReturnValue({ properties: { defaults: { ssh: null } } });

            const config = {
                user: "user1",
                host: "example.com",
                name: "firstProfile",
            };

            await (testManager as any).setProfile(config);

            expect(mockConfigApi.profiles.defaultSet).toHaveBeenCalledWith("ssh", "firstProfile");
        });

        it("should prompt user when modifying shared configuration", async () => {
            const config = {
                user: "updatedUser",
                host: "example.com",
                name: "existingProfile",
            };

            (testManager as any).mProfilesCache.mergeArgsForProfile.mockReturnValue({
                knownArgs: [{ argName: "user", argLoc: { jsonLoc: "profiles.baseProfile.properties.user" } }],
            });
            mockConfigApi.profiles.getProfileNameFromPath.mockReturnValue("baseProfile"); // Different from profile being updated

            await (testManager as any).setProfile(config, "existingProfile");

            expect((testManager as any).showMenu).toHaveBeenCalledWith(
                expect.objectContaining({
                    title: expect.stringContaining('Property: "user" found in a possibly shared configuration'),
                }),
            );
        });

        it("should force update when user chooses 'No' for shared configuration", async () => {
            vi.spyOn(testManager as any, "showMenu").mockResolvedValue("No");

            const config = {
                user: "updatedUser",
                name: "existingProfile",
            };

            (testManager as any).mProfilesCache.mergeArgsForProfile.mockReturnValue({
                knownArgs: [{ argName: "user", argLoc: { jsonLoc: "profiles.baseProfile.properties.user" } }],
            });
            mockConfigApi.profiles.getProfileNameFromPath.mockReturnValue("baseProfile");

            await (testManager as any).setProfile(config, "existingProfile");

            expect((testManager as any).mProfilesCache.updateProperty).toHaveBeenCalledWith(
                expect.objectContaining({
                    forceUpdate: true,
                }),
            );
        });
    });
    describe("getNewProfileName", async () => {
        let testManager: TestAbstractConfigManager;
        let mockTeamConfig: any;
        let mockConfigApi: any;

        beforeEach(async () => {
            // Set up the mock config API
            mockConfigApi = {
                layerExists: vi.fn(),
                layerActive: vi.fn(() => ({
                    properties: {
                        profiles: {
                            testProfile: { host: "host.example.com", user: "testuser" },
                        },
                    },
                })),
                setSchema: vi.fn(),
                save: vi.fn(),
            };

            mockTeamConfig = {
                ...mockConfigApi,
                api: mockConfigApi,
            };

            testManager = new TestAbstractConfigManager(profCache);

            // Mock mProfilesCache methods
            (testManager as any).mProfilesCache.mergeArgsForProfile = vi.fn().mockReturnValue({ knownArgs: [] });
            (testManager as any).mProfilesCache.updateProperty = vi.fn();
            (testManager as any).mProfilesCache.isSecured = vi.fn().mockReturnValue(false);

            // Mock helper methods
            vi.spyOn(testManager as any, "fetchDefaultProfile").mockReturnValue({ name: "default" });
            vi.spyOn(testManager as any, "showMenu").mockResolvedValue("Yes");
        });

        it("should generate name from hostname if name is missing", async () => {
            const profile: ISshConfigExt = { hostname: "host.example.com" };
            const result = await (testManager as any).getNewProfileName(profile, mockTeamConfig);
            expect(result!.name).toBe("host_example_com");
        });

        it("should return same profile if name exists and is unique", async () => {
            const profile: ISshConfigExt = { name: "unique", hostname: "host" };
            const result = await (testManager as any).getNewProfileName(profile, mockTeamConfig);
            expect(result!.name).toBe("unique");
            expect(testManager.showMenu).not.toHaveBeenCalled();
        });

        it("should return profile if existing name found and user confirms overwrite", async () => {
            mockTeamConfig.layerActive = vi.fn(() => ({
                properties: { profiles: { duplicate: {} } },
            }));
            (testManager as any).showMenu.mockResolvedValueOnce("Yes");

            const profile: ISshConfigExt = { name: "duplicate", hostname: "host" };
            const result = await (testManager as any).getNewProfileName(profile, mockTeamConfig);

            expect(testManager.showMenu).toHaveBeenCalled();
            expect(result!.name).toBe("duplicate");
        });

        it("should enter loop if existing name found and user says No, then accept unique name", async () => {
            mockTeamConfig.layerActive = vi.fn(() => ({
                properties: { profiles: { duplicate: {} } },
            }));
            (testManager as any).showMenu.mockResolvedValueOnce("No");
            (testManager as any).showInputBox.mockResolvedValueOnce("newName");

            const profile: ISshConfigExt = { name: "duplicate", hostname: "host" };
            const result = await (testManager as any).getNewProfileName(profile, mockTeamConfig);

            expect(testManager.showMenu).toHaveBeenCalledWith(
                expect.objectContaining({
                    placeholder: expect.stringContaining("already exists"),
                }),
            );
            expect(result!.name).toBe("newName");
        });
        it("should call showMenu and accept overwrite when user selects 'Yes'", async () => {
            // Make the duplicate name exist in the config
            mockTeamConfig.layerActive = vi.fn(() => ({
                properties: { profiles: { duplicate: { host: "something" } } },
            }));
            (testManager as any).showMenu.mockResolvedValueOnce("No").mockResolvedValueOnce("Yes");
            vi.spyOn(testManager, "showInputBox").mockResolvedValue("duplicate");
            const profile: ISshConfigExt = { name: "duplicate", hostname: "host.example.com" };
            const result = await (testManager as any).getNewProfileName(profile, mockTeamConfig);
            expect(testManager.showMenu).toHaveBeenCalledWith(
                expect.objectContaining({ placeholder: expect.stringContaining("duplicate") }),
            );
            expect(result!.name).toBe("duplicate");
        });
    });
    describe("getMergedAttrs", () => {
        let testManager: TestAbstractConfigManager;

        beforeEach(() => {
            testManager = new TestAbstractConfigManager(profCache);

            // Default mocks for mProfilesCache
            (testManager as any).mProfilesCache = {
                mergeArgsForProfile: vi.fn(),
                getAllProfiles: vi.fn(),
            };
        });

        it("should return empty object if prof is null", () => {
            const result = (testManager as any).getMergedAttrs(null);
            expect(result).toEqual({});
            expect((testManager as any).mProfilesCache.mergeArgsForProfile).not.toHaveBeenCalled();
        });

        it("should return empty object when prof is a string but not found", () => {
            (testManager as any).mProfilesCache.getAllProfiles.mockReturnValue([]);
            (testManager as any).mProfilesCache.mergeArgsForProfile.mockReturnValue({ knownArgs: [] });

            const result = (testManager as any).getMergedAttrs("missing");
            expect(result).toEqual({});
        });

        it("should merge attributes when prof is an object", () => {
            const profObj = { profName: "objProfile" };
            const mergedArgs = {
                knownArgs: [{ argName: "port", argValue: 22 }],
            };

            (testManager as any).mProfilesCache.mergeArgsForProfile.mockReturnValue(mergedArgs);

            const result = (testManager as any).getMergedAttrs(profObj);

            expect((testManager as any).mProfilesCache.mergeArgsForProfile).toHaveBeenCalledWith(profObj, {
                getSecureVals: true,
            });
            expect(result).toEqual({ port: 22 });
        });
    });
    describe("fetchAllSshProfiles", () => {
        let testManager: TestAbstractConfigManager;

        beforeEach(() => {
            testManager = new TestAbstractConfigManager(profCache);

            (testManager as any).mProfilesCache = {
                getAllProfiles: vi.fn(),
            };

            vi.spyOn(testManager as any, "getMergedAttrs").mockReturnValue({ host: "example.com" });
        });

        it("should return loaded SSH profiles with merged attributes", () => {
            const mockProfiles = [{ profName: "ssh1" }, { profName: "ssh2" }];
            (testManager as any).mProfilesCache.getAllProfiles.mockReturnValue(mockProfiles);

            const result = (testManager as any).fetchAllSshProfiles();

            expect((testManager as any).mProfilesCache.getAllProfiles).toHaveBeenCalledWith("ssh");
            expect((testManager as any).getMergedAttrs).toHaveBeenCalledTimes(mockProfiles.length);

            expect(result).toEqual([
                {
                    message: "",
                    name: "ssh1",
                    type: "ssh",
                    profile: { host: "example.com" },
                    failNotFound: false,
                },
                {
                    message: "",
                    name: "ssh2",
                    type: "ssh",
                    profile: { host: "example.com" },
                    failNotFound: false,
                },
            ]);
        });
    });
    describe("fetchDefaultProfile", () => {
        let testManager: TestAbstractConfigManager;

        beforeEach(() => {
            testManager = new TestAbstractConfigManager(profCache);

            (testManager as any).mProfilesCache = {
                getDefaultProfile: vi.fn(),
            };

            vi.spyOn(testManager as any, "getMergedAttrs").mockReturnValue({ host: "example.com" });
        });

        it("should return the default SSH profile with merged attributes", () => {
            const mockDefaultProfile = { profName: "default" };
            (testManager as any).mProfilesCache.getDefaultProfile.mockReturnValue(mockDefaultProfile);

            const result = (testManager as any).fetchDefaultProfile();

            expect((testManager as any).mProfilesCache.getDefaultProfile).toHaveBeenCalledWith("ssh");
            expect((testManager as any).getMergedAttrs).toHaveBeenCalledWith(mockDefaultProfile);

            expect(result).toEqual({
                message: "",
                name: "default",
                type: "ssh",
                profile: { host: "example.com" },
                failNotFound: false,
            });
        });
    });
});
