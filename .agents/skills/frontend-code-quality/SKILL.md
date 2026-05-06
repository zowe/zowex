---
name: frontend-code-quality
description: Improve TypeScript code quality in CLI, SDK, and VS Code extension packages. Apply DRY, YAGNI, SRP principles to reduce duplication and complexity. Use when refactoring, deduplicating, simplifying, or improving code in packages/cli/, packages/sdk/, or packages/vsce/.
---

# Frontend Code Quality

Identify and fix code quality issues in the client-side TypeScript codebase (`packages/`).

## Environment Constraints

**Client Components:** Node.js LTS, TypeScript.
**Packages:**
- `packages/cli`: Zowe CLI plug-in (`@zowe/imperative` handlers)
- `packages/vsce`: Zowe Explorer extender (`@zowe/zowe-explorer-api` implementations)
- `packages/sdk`: Node.js SDK (`ZSshClient` and RPC typings)

## Quality Checklist

Before refactoring, evaluate code against:

- [ ] DRY: No duplicated logic (e.g. repetitive authentication fallbacks in `SshBaseHandler`)
- [ ] YAGNI: No unused/speculative code
- [ ] SRP: Each function/class has one responsibility
- [ ] Complexity: Functions < 50 lines, cyclomatic complexity < 10 (treat as a warning/suggestion, not a strict requirement)
- [ ] Naming: Clear names, minimal comments needed
- [ ] Error handling: Consistent patterns (e.g. bubbling errors up to Base Handlers instead of repeating `try/catch` in every CLI handler)
- [ ] Types: Proper use of strict typing, avoid `any`

---

## TypeScript Best Practices

### Guidelines

- **Types vs Interfaces:** Use `interface` for public APIs and contracts, `type` for unions/intersections and internal utilities.
- **Avoid `any`:** Always strive to provide accurate types. For example, replacing `any` in API responses with actual Zowe Explorer interfaces.
- **Async/Await vs Raw Promises:** Prefer `async`/`await` over raw Promises, avoiding Promise chains.
- **Consistent Error Handling:** Centralize `ImperativeError` formatting and CLI output styling (e.g. `TextUtils.chalk.red`) rather than duplicating it across individual handlers.

---

## DRY (Don't Repeat Yourself)

### Detection

Look for:

- Duplicated private key fallback logic across command methods in `packages/cli/src/SshBaseHandler.ts`.
- Repetitive REST API call constructs inside `updateAttributes` and similar methods in `packages/vsce/src/api/SshUssApi.ts`.
- Repeated `catch(err)` blocks that set `params.response.console.errorHeader` across different CLI handlers.

### Refactoring Patterns

**Extract error handling to Base Class:**

```typescript
// Before: duplicated in multiple handlers (e.g. ListDataSetsHandler)
try {
    response = await client.ds.listDatasets({...});
} catch (err) {
    const errText = (err instanceof ImperativeError ? err.additionalDetails : err.toString());
    params.response.data.setExitCode(1);
    params.response.console.errorHeader(TextUtils.chalk.red("Response from Service"));
    params.response.console.error(errText);
    return { success: false, items: [], returnedRows: 0 };
}

// After: bubble it up and let SshBaseHandler format and print errors uniformly
const response = await client.ds.listDatasets({...});
```

---

## YAGNI (You Ain't Gonna Need It)

### Detection

Look for:

- Handlers checking for configuration properties that are already validated by Imperative framework profiles.
- Unused properties in Zowe config mappings or SDK payload definitions.
- Boilerplate properties that always take the same static value across all callers.

### Actions

- **Delete** dead code rather than commenting it out (use version control)
- **Simplify** over-engineered abstractions

---

## SRP (Single Responsibility Principle)

### Detection

Signs a function does too much:

- RPC connection initialization, timeout scheduling, error event binding, and JSON payload building happening inside a single function block (e.g., inside `ZSshClient.ts`).
- CLI handlers (`packages/cli/src/**/*.handler.ts`) doing data formatting, SSH command building, AND printing console output.

### Refactoring

**Split by responsibility:**

- **SDK (`packages/sdk`)**: Strictly handles RPC JSON-RPC formulation, `stdin`/`stdout` streaming, and SSH connection lifecycle.
- **CLI Handlers (`packages/cli`)**: Gathers CLI `IHandlerParameters`, passes them to SDK, and formats the output `params.response.format.output`.
- **VS Code Extension (`packages/vsce`)**: Adapts Zowe SDK outputs to `IZosFilesResponse` structures for Zowe Explorer.

---

## Complexity Reduction

### Metrics

- **Function length**: Target < 50 lines
- **Function parameters**: Target <= 3, Max 5.
- **Cyclomatic complexity**: Target < 10 branches (use as a warning/suggestion to guide refactoring, rather than a strict requirement)
- **Nesting depth**: Target < 4 levels

### Patterns

**Early return:**

```typescript
// Before
if (attributes.tag) {
    const response = await client.uss.chtagFile({...});
    success &&= response.success;
}

if (attributes.uid || attributes.owner) {
    const response = await client.uss.chownFile({...});
    success &&= response.success;
}

// After: Break into specialized functions or array loops rather than repeated if-blocks.
```

---

## Naming Conventions

### Classes and Interfaces

- `PascalCase` regardless of language: e.g., `SshBaseHandler`, `DatasetInfo`.

### Functions and Variables

- `camelCase` for client-side Node.js/TypeScript components: e.g., `processWithClient`, `translateCliError`.

---

## Workflow

When improving code quality:

1. **Identify scope**: Single file or related files (e.g. `cli`, `vsce`, `sdk`)
2. **Search for patterns**: Use grep to find similar code blocks
3. **Categorize issues** by type (DRY, YAGNI, SRP, complexity)
4. **Propose changes** with rationale
5. **Verify no breaking changes** to public APIs before implementing

### Commands

Run formatter/linter (Biome):

```bash
npm run lint
```

Run tests (Vitest) to ensure no regressions:

```bash
npm run test
```

Find potential duplicates:

```bash
rg -n "pattern" packages/cli/ packages/sdk/ packages/vsce/
```

Analyze function length and complexity:
*Note: Use structural code search tools or read the target files directly to evaluate function lengths and branches. Avoid brittle brace-matching regex.*

---

## Anti-Patterns to Flag

| Pattern            | Issue                | Fix                              |
| ------------------ | -------------------- | -------------------------------- |
| Manual Promise     | Hard to debug/trace  | Use async/await when possible    |
| Inconsistent Err   | Different CLI output | Use `SshBaseHandler` for errors  |
| Deep nesting       | Hard to follow       | Early returns, extract functions |
| > 5 parameters     | Hard to maintain     | Use options/config object        |
| Copy-paste auth    | Maintenance burden   | Extract shared fallback logic    |
