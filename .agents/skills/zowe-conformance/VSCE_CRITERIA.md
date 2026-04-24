# Zowe Explorer V3 Conformance Criteria — Full Reference

> Source: Zowe Explorer VSC - V3 Conformance Changes document
> Legend: **Required** = must meet for conformance | **Best Practice** = recommended but not mandatory

---

## General Extension (items 1-9) — All extensions must comply

| # | Type | Criterion |
|---|-----|------|
| 1 | Required | **Naming:** If the extension uses the word "Zowe" in its name, it abides by The Linux Foundation Trademark Usage Guidelines and Branding Guidelines to ensure the word Zowe is used in a way intended by the Zowe community. |
| 2 | Best Practice | **No Zowe CLI plugin installation requirement:** If the extender makes use of a Zowe CLI profile other than the Zowe Explorer default `zosmf` then the extension must not make any assumptions that a matching Zowe CLI plugin has been installed in the Zowe Explorer user's environment. |
| 3 | Required | **Publication tag:** If the extension is published in a public catalog or marketplace such as Npmjs, Open-VSX, or VS Code Marketplace, it uses the tag or keyword "Zowe" so it can be found when searching for Zowe and be listed with other Zowe offerings. |
| 4 | Required | **Support:** Extension has documentation with instructions on how to report problems that are related to the extension and not Zowe Explorer. It needs to explain how users can determine if a problem is related to the extension or Zowe Explorer. |
| 5 | Best Practice | **User settings consistency:** Extender provides a consistent user settings experience. For VS Code extensions, extender follows the recommended naming convention for configuration settings as described in VS Code's configuration contribution documentation. |
| 6 | Required | **Using .zowe in user settings:** Extender avoids starting setting names with the prefix `zowe.`, which is reserved for Zowe Explorer and other extensions maintained by the Zowe Project. |
| 7 | Best Practice | **Error message consistency:** Extension follows the recommended error message format indicated in the Zowe Explorer extensibility documentation to provide a consistent user experience with Zowe Explorer. |
| 8 | Best Practice | **Zowe SDK usage:** Extension utilizes the available Zowe SDKs that standardize z/OS interactions as well as other common capabilities that are used by many other Zowe extensions and tools unless the extension's goal is to provide a new implementation with clearly stated goals. |
| 9 | Required | **Sharing Zowe profiles:** If the extension accesses the same mainframe service as a Zowe CLI plug-in, the connection information should be shared via Zowe Team Config (zowe.config.json). |

---

## Item 10 — Section Applicability

Applicant must complete at least one section below. Mark all that apply:

- **[a]** Extension interacts with mainframe content retrieved via Data Set, USS, or Jobs view
- **[b]** Extension accesses profiles
- **[c]** Extension serves as a data provider
- **[d]** Extension adds menus

---

## [a] Extension Interacts with Mainframe Assets (item 11)

| # |Type | Criterion |
|---|-----|------|
| 11 | Required | **README.MD File:** Extension documents the following in its associated README.MD file (displayed at the appropriate Marketplace): (1) recommends use of Zowe Explorer, (2) describes the relationship to Zowe Explorer, (3) describes the scenario that leverages Zowe Explorer, (4) uses the "zowe" TAG. Sample verbiage: "Recommended for use with Zowe Explorer. [Extension-name] extension uses the Zowe Explorer to access mainframe files and then [complete-your-use-case-here]." |

---

## [b] Extension Accesses Profiles (items 12-17)

| # | Type | Criterion |
|---|-----|------|
| 12 | Required | **VS Code extension dependency:** If the extension calls the Zowe Explorer API it must declare Zowe Explorer as a VS Code extension dependency by including an `extensionDependencies` entry for Zowe Explorer in its package.json file. This ensures Zowe Explorer and Zowe Explorer API are activated and initialized for proper use by its extenders. |
| 13 | Required | **Zowe Extender access:** Extension accesses the shared Zowe Explorer profiles cache via `ZoweExplorerApi.IApiRegisterClient.getExplorerExtenderApi()` API as documented in the Zowe Explorer extensibility documentation. |
| 14 | Required | **Added Profile Type initialization:** If the extension has a dependency on a new Zowe CLI profile type other than the Zowe Explorer default `zosmf`, it is calling `ZoweExplorerApi.IApiRegisterClient.getExplorerExtenderApi().initialize(profileTypeName)` to ensure that the profile type is supported and managed by the extension without a Zowe CLI plugin installed. |
| 15 | Best Practice | **Base Profile and Tokens:** Extension supports base profiles and tokens. |
| 16 | Required | **Team Configuration File:** Extension supports the Zowe CLI team configuration file format. |
| 17 | Required | **No V1 Profiles:** The plug-in does not use (read or write) v1 profiles. *(New for V3 — replaces the V2 requirement for backward compatibility with v1 profiles.)* |

---

## [c] Extension Serves as a Data Provider (items 18-22)

| # | Type | Criterion |
|---|-----|------|
| 18 | Required | **VS Code extension dependency:** If the extension calls the Zowe Explorer API it must declare Zowe Explorer as a VS Code extension dependency by including an `extensionDependencies` entry for Zowe Explorer in its package.json file. This ensures Zowe Explorer and Zowe Explorer API are activated and initialized for proper use by its extenders. |
| 19 | Required | **New Zowe CLI profile type:** Extension registers its new API instances with a new profile type name for the different Zowe Explorer views via the `ZoweExplorerApi.IApiRegisterClient.register{Mvs\|Uss\|Jes}Api(profileTypeName)` call as indicated from the Zowe Explorer extensibility documentation. |
| 20 | Best Practice | **Matching Zowe CLI Plugin:** Provide a Zowe CLI Plugin for the data provider's new profile type that implements the core capabilities required for the new protocol that users can then also use to interact with the protocol outside of the Zowe Explorer extension using Zowe CLI commands. |
| 21 | Required | **Data provider API implementation:** Extension fully implements and registers to at least one of the three Zowe Explorer interfaces or alternatively throws exceptions that provide meaningful error messages to the end-user in the `Error.message` property that will be displayed in a dialog. |
| 22 | Best Practice | **API test suite implementation:** If the extension implements a Zowe Explorer API, it has a test suite. |

---

## [d] Extension Adds Menus (items 23-30)

| # | Type | Criterion |
|---|-----|------|
| 23 | Best Practice | **VS Code extension dependency:** If the extension calls the Zowe Explorer API it should declare Zowe Explorer as a VS Code extension dependency by including an `extensionDependencies` entry for Zowe Explorer in its package.json file. This ensures Zowe Explorer and Zowe Explorer API are activated and initialized for proper use by its extenders. |
| 24 | Required | **Command operations:** If the extension is adding new commands to Zowe Explorer's tree views, the commands must not replace any existing Zowe Explorer commands. |
| 25 | Best Practice | **Command categories 1:** If the extension adds to `contributes.commands` in package.json, the value assigned to the `category` property contains the extension name. |
| 26 | Required | **Command categories 2:** If the extension assigns values to the `category` property in `contributes.commands` in package.json, the value cannot be "Zowe Explorer". |
| 27 | Required | **Context menu groups:** If contributing commands to Zowe Explorer's context menus, the extension follows the Zowe Explorer extensibility documentation and adds them in new context menu groups that are located below Zowe Explorer's existing context menu groups in the user interface. |
| 28 | Best Practice | **Adding New Menu Items:** If the extension is adding new commands and context menu entries to the Zowe Explorer tree view nodes, the new command name is consistent with the terminology and naming conventions of the existing Zowe Explorer menu entries. More information is provided in the Zowe Explorer extensibility documentation. |
| 29 | Required | **Existing Menu Items:** Extension does not overwrite any existing Zowe Explorer command and context menu entries. |
| 30 | Required | **Context menu items:** If contributing commands to Zowe Explorer's views (such as Data Sets, USS, or Jobs), the extension should only add them to the view's right-click context menus. |
