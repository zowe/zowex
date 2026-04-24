---
name: zowe-conformance-check
description: Validate VS Code extensions against Zowe V3 Conformance Criteria. Audit package.json, settings, commands, menus, profile usage, and API registration for conformance. Use when checking Zowe conformance, preparing for conformance submission, auditing an extension against Zowe criteria, or when the user mentions conformance.
---

# Zowe V3 Conformance Check

Validate a VS Code extension against the Zowe Explorer V3 Conformance Criteria (30 items). Identifies which criteria are met, which are violated, and which need manual verification.

## Quick Start

1. Identify which conformance sections apply (use the decision tree below)
2. Copy the applicable checklist from [CRITERIA.md](CRITERIA.md) (or fetch live criteria if requested, see "Full Criteria Reference")
3. Audit the extension against each item
4. Report findings using the output format

## Determining Applicable Sections

Every extension must pass **General Extension** (items 1-9).

Then determine which additional sections apply:

```
Does the extension interact with mainframe content from DS/USS/Jobs views?
  YES → Section [a] (item 11)

Does the extension access Zowe profiles?
  YES → Section [b] (items 12-17)

Does the extension register a new data provider (MVS/USS/JES API)?
  YES → Section [c] (items 18-22)

Does the extension add commands or menus to Zowe Explorer views?
  YES → Section [d] (items 23-30)
```

At least one of [a], [b], [c], or [d] must apply (item 10).

## Audit Workflow

### Step 1: Gather Evidence

Read these files from the extension under review:

- `package.json` — dependencies, extensionDependencies, contributes (commands, menus, configuration)
- `README.md` — Zowe references, tags, support documentation
- Extension entry point (`activate()` function) — API registration, profile initialization
- Any profile/config handling code

### Step 2: Automated Checks

For each applicable criterion, verify programmatically where possible:

**package.json checks:**
- `keywords` or `tags` includes `"zowe"` (item 3)
- No settings starting with `zowe.` prefix (item 6)
- Command `category` values do not use `"Zowe Explorer"` (item 26)
- Command `category` contains the extension's own name (item 25)

**Code checks:**
- Calls `getExplorerExtenderApi()` for profile access (item 13)
- Calls `initialize(profileTypeName)` for custom profile types (item 14)
- Calls `register{Mvs|Uss|Jes}Api()` for data providers (item 19)
- No reads/writes of v1 profiles (item 17)
- Does not replace existing Zowe Explorer commands (items 24, 29)

### Step 3: Manual Verification Items

Some criteria require human judgment:

- Zowe trademark usage compliance (item 1)
- Support documentation quality (item 4)
- Error message format consistency (item 7)
- SDK usage justification if not using Zowe SDKs (item 8)
- API implementation completeness / meaningful error messages (item 21)
- Test suite coverage (item 22)
- Menu placement below Zowe groups (item 27)
- Command naming consistency with Zowe conventions (item 28)

### Step 4: Report Findings

Use this output format:

```markdown
## Zowe V3 Conformance Audit

**Extension:** [name]
**Applicable sections:** General + [a/b/c/d]
**Date:** [date]

### Summary

| Status | Count |
|--------|-------|
| PASS   | X     |
| FAIL   | X     |
| MANUAL | X     |
| N/A    | X     |

### General Extension (items 1-9)

| # | Criterion | Status | Evidence |
|---|-----------|--------|----------|
| 1 | Zowe naming/trademark | PASS/FAIL/MANUAL | [details] |
| ... | ... | ... | ... |

### [Section Name] (items X-Y)

| # | Criterion | Status | Evidence |
|---|-----------|--------|----------|
| ... | ... | ... | ... |

### Required Actions

🔴 **FAIL** items (must fix for conformance):
- [item]: [what needs to change]

🟡 **MANUAL** items (needs human verification):
- [item]: [what to verify]
```

## Status Definitions

| Status | Meaning |
|--------|---------|
| **PASS** | Criterion met, evidence confirmed in code/config |
| **FAIL** | Criterion violated, specific issue identified |
| **MANUAL** | Cannot be verified programmatically, needs human review |
| **N/A** | Criterion does not apply to this extension |

## Key V3 Changes from V2

- **Item 17 (new):** Extension must NOT read or write v1 profiles (previously required backward compatibility)
- Several items reworded for clarity but requirements unchanged

## Full Criteria Reference

For the complete text of all 30 conformance criteria with version info and required/best-practice classification, see [CRITERIA.md](CRITERIA.md).

> **Live Audit Option:** By default, use the local `CRITERIA.md`. If the user explicitly asks for a "live audit" or "latest criteria", use your web browsing or fetching capabilities to read the latest criteria directly from the official documentation: https://docs.zowe.org/stable/whats-new/zowe-v3-conformance-criteria/
