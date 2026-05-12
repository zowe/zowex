---
name: zowe-conformance
description: Validate VS Code extensions and CLI plug-ins against Zowe V3 Conformance Criteria. Audit package.json, settings, commands, menus, profile usage, and API registration for conformance. Use when checking Zowe conformance, preparing for conformance submission, auditing an extension/plug-in against Zowe criteria, or when the user mentions conformance.
---

# Zowe V3 Conformance Check

Validate a VS Code extension or CLI plug-in against the Zowe V3 Conformance Criteria. Identifies which criteria are met, which are violated, and which need manual verification.

## Quick Start

1. Determine if the target is a CLI Plug-in.
2. Identify which conformance sections apply (see below).
3. Copy the applicable checklist from [CLI_CRITERIA.md](CLI_CRITERIA.md) (or fetch live criteria if requested, see "Full Criteria Reference").
4. Audit the extension/plug-in against each item.
5. Report findings using the output format.

## Determining Applicable Sections

### For CLI Plug-ins

Every CLI plug-in must pass all applicable sections in [CLI_CRITERIA.md](CLI_CRITERIA.md), which include:

- Infrastructure (items 1-8)
- Installation (items 9-13)
- Naming (items 14-16)
- Profiles (items 17-30)
- Support (item 31)
- Documentation (item 32)

## Audit Workflow

### Step 1: Gather Evidence

Read these files from the extension/plug-in under review:

- `package.json` — dependencies, extensionDependencies, contributes (commands, menus, configuration)
- `README.md` — Zowe references, tags, support documentation
- Extension/Plug-in entry point (`activate()` function or `imperative` configuration) — API registration, profile initialization
- Any profile/config handling code

### Step 2: Automated Checks

For each applicable criterion, verify programmatically where possible.

**CLI checks:**

- `package.json` has no `@zowe/cli` peer dependency (item 5)
- Uses `@zowe/imperative` RestClient instead of `axios` or `request` (item 7)
- Option names do not use reserved aliases like `--rfj`, `--help`, etc. (item 16)
- Profile properties use exact names like `host`, `port`, `user`, `password` (item 20)
- No reads/writes of v1 profiles (item 30)

### Step 3: Manual Verification Items

Some criteria require human judgment:

- Zowe trademark usage compliance
- Support documentation quality
- Error message format consistency
- SDK usage justification if not using Zowe SDKs
- API implementation completeness / meaningful error messages
- Test suite coverage
- Menu placement below Zowe groups
- Command naming consistency with Zowe conventions

### Step 4: Report Findings

Use this output format:

```markdown
## Zowe V3 Conformance Audit

**Target:** [name] (CLI)
**Applicable sections:** [list sections]
**Date:** [date]

### Summary

| Status | Count |
| ------ | ----- |
| PASS   | X     |
| FAIL   | X     |
| MANUAL | X     |
| N/A    | X     |

### [Section Name] (items X-Y)

| #   | Criterion | Status           | Evidence  |
| --- | --------- | ---------------- | --------- |
| 1   | [Summary] | PASS/FAIL/MANUAL | [details] |
| ... | ...       | ...              | ...       |

### Required Actions

🔴 **FAIL** items (must fix for conformance):

- [item]: [what needs to change]

🟡 **MANUAL** items (needs human verification):

- [item]: [what to verify]
```

## Status Definitions

| Status     | Meaning                                                 |
| ---------- | ------------------------------------------------------- |
| **PASS**   | Criterion met, evidence confirmed in code/config        |
| **FAIL**   | Criterion violated, specific issue identified           |
| **MANUAL** | Cannot be verified programmatically, needs human review |
| **N/A**    | Criterion does not apply to this extension/plug-in      |

## Key V3 Changes from V2

- **CLI Item 30 (new):** Must NOT read or write v1 profiles (previously required backward compatibility)
- **CLI Item 8 (new best practice):** Exit codes should be 0 for success, non-zero for failure
- **CLI Item 13 (new best practice):** Plug-ins should be installable from a local archive
- Several items reworded for clarity but requirements unchanged

## Full Criteria Reference

For the complete text of all conformance criteria with version info and required/best-practice classification, see [CLI_CRITERIA.md](CLI_CRITERIA.md) for CLI plug-ins.

> **Live Audit Option:** By default, use the local criteria files. If the user explicitly asks for a "live audit" or "latest criteria", use your web browsing or fetching capabilities to read the latest criteria directly from the official documentation: https://docs.zowe.org/stable/whats-new/zowe-v3-conformance-criteria/
