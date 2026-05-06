# Zowe CLI V3 Conformance Criteria — Full Reference

> Source: V3 Zowe CLI Conformance Changes document
> Legend: **Required** = must meet for conformance | **Best Practice** = recommended but not mandatory

---

## Infrastructure (items 1-8)

| # | Type | Criterion |
|---|-----|------|
| 1 | Required | Plug-in is constructed on the Imperative CLI Framework. |
| 2 | Required | Plug-in is NOT run as a standalone CLI. |
| 3 | Required | Plug-in commands write to stdout or stderr via Imperative Framework `response.console` APIs. |
| 4 | Required | If plug-in requires gzip decompression support, leverage the Core CLI built-in support -- do NOT opt-out of the built-in gzip decompression support (specifically, the `mDecode` property of the Imperative RestClient must NOT be overridden). |
| 5 | Required | Plug-ins must not have an `@zowe/cli` peer dependency for improved npm@7 compatibility. The only peer dependencies that should be included are packages which are imported in the plug-in's source code (e.g., `@zowe/imperative`). |
| 6 | Best Practice | In their Imperative config file (defined in the `imperative.configurationModule` property of `package.json`), plug-ins should make their imports as few and specific as possible. This can significantly decrease their load time. |
| 7 | Best Practice | HTTP or HTTPS requests to REST APIs should use the `@zowe/imperative` RestClient instead of a direct dependency on a 3rd-party package like request or axios. |
| 8 | Best Practice | Commands contributed by a plug-in should produce exit codes that follow the convention of: <br>- Upon successful execution of the command, return zero; or <br>- If a command fails to execute successfully, return a non-zero value. <br>Plug-in documentation should identify the possible values that may be returned by the plug-in's commands, explaining what each value indicates. *(New for V3)* |

---

## Installation (items 9-13)

| # | Type | Criterion |
|---|-----|------|
| 9 | Required | Plug-in is installable with the `zowe plugins install` command. |
| 10 | Required | Plug-in is installable into the `@zowe-vN-lts` version of the core Zowe CLI and follows semantic versioning (where "N" = the latest "active" LTS release number). |
| 11 | Required | Plug-in is uninstallable via the `zowe plugins uninstall` command. |
| 12 | Required | `@latest` must point to the same version as the most recent zowe lts tag (Note: for V3 it will be `@zowe-v3-lts`). |
| 13 | Best Practice | Plug-ins should be able to be installed from a local archive without having to download dependencies from npm. *(New for V3)* |

---

## Naming (items 14-16)

| # | Type | Criterion |
|---|-----|------|
| 14 | Required | If the plug-in introduces a command group name, it does not conflict with existing conformant plug-in group names. |
| 15 | Best Practice | Names of CLI commands/groups/options must be in kebab case (lower case & hyphens). Names of properties in `zowe.config.json` should be camel case. Only alphanumeric characters should be used - `[a-zA-Z0-9-]+`. |
| 16 | Required | The following option names/aliases are reserved and must not be used: `--response-format-json`, `--rfj`, `--help`, `-h`, `--help-examples`, `--help-web`, `--hw` |

---

## Profiles (items 17-30)

| # | Type | Criterion |
|---|-----|------|
| 17 | Required | If the plug-in has unique connection details, it introduces a profile that lets users store these details for repeated use. |
| 18 | Best Practice | Plug-in users are able to override all profile settings via the command line and/or environment variables. |
| 19 | Required | If the plug-in uses a Zowe API-ML integrated API, it (the plug-in) has an option named `base-path` in the profile to used to house the path of the associated service in the API ML. |
| 20 | Required | If the plug-in connects to a service, it must include the following profile properties AND they MUST be these exact properties (e.g. host, NOT hostname): `host`, `port`, `user`, `password` |
| 21 | Required | If the plug-in connects to a service, and the service supports logging in with a token, it must include the following profile properties AND they MUST be these exact properties: `tokenType`, `tokenValue` |
| 22 | Required | If the plug-in connects to a service, and the service supports logging in with PEM certificates, it must include the following profile properties AND they MUST be these exact properties: `certFile`, `certKeyFile` |
| 23 | Required | If the plug-in connects to a service, and the service supports logging in with PFX/P12 certificates, it must include the following profile properties AND they MUST be these exact properties: `certFile`, `certFilePassphrase` |
| 24 | Required | If the plug-in provides an option to reject untrusted certificates, the property must be named `rejectUnauthorized`. CLI option should be `reject-unauthorized`. |
| 25 | Best Practice | The plug-in specifies options to be pre-filled by default in `zowe.config.json` once `zowe config init` has executed. |
| 26 | Required | The plug-in supports reading profiles stored in the Zowe Team Config format. |
| 27 | Required | The plug-in supports base profiles within a user's Zowe Team Config. |
| 28 | Best Practice | When host, port, user, or password is missing for a particular command and no default value is set, the user is prompted for the argument. Host, user, and password should NOT have default values. |
| 29 | Required | To take advantage of the new `zowe config auto-init` command, a plugin that works with a single-sign-on, APIML-compliant REST service MUST supply a new object within its plugin definition to identify that REST service. The new `IImperative.apimlConnLookup` object must be in the plugin's definition. That object includes the `apiId` and `gatewayUrl` of the corresponding REST service. The related REST service must also supply its `apiId` and `gatewayUrl` in the `apiml` section of its `application.yml` definition. Zowe-CLI automatically handles the `apimlConnLookup` object for the `zosmf` service. Thus an `apimlConnLookup` object for `zosmf` does not have to be specified within a plugin. |
| 30 | Required | The plug-in does not use (read or write) v1 profiles. *(New for V3)* |

---

## Support (item 31)

| # | Type | Criterion |
|---|-----|------|
| 31 | Required | Submitter describes how support is provided and support details are clearly documented. |

---

## Documentation (item 32)

| # | Type | Criterion |
|---|-----|------|
| 32 | Best Practice | Plug-in command help is contributed to this repo for the purpose of hosting the ecosystem web-help on Zowe Docs - https://github.com/zowe/zowe-cli-web-help-generator |