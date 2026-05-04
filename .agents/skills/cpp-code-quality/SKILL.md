---
name: cpp-code-quality
description: Improve C++ code quality by identifying duplication, complexity, and design issues. Apply DRY, YAGNI, SRP principles. Use when refactoring, deduplicating, simplifying, or improving backend code in native/c/.
---

# Native Code Quality

Identify and fix code quality issues in the native C++ codebase (`native/c/`).

## Compiler Constraints

**Backend and Middleware (ibm-clang):** C++17, Clang 18.

## Quality Checklist

Before refactoring, evaluate code against:

- [ ] DRY: No duplicated logic
- [ ] YAGNI: No unused/speculative code
- [ ] SRP: Each function/class has one responsibility
- [ ] Complexity: Functions < 50 lines, cyclomatic complexity < 10 (treat as a warning/suggestion, not a strict requirement)
- [ ] Naming: Clear names, minimal comments needed
- [ ] Error handling: Consistent patterns
- [ ] Memory: Proper use of RAII/smart pointers vs. legacy C-style allocation

---

## Memory Management

### Guidelines

- **Legacy C APIs:** When interacting with strict C APIs, preserve `malloc`/`free` and raw pointers to avoid ABI issues.
- **C++ Refactoring:** When refactoring internal C++ logic, prefer modern C++ memory management (`std::unique_ptr`, `std::shared_ptr`) to prevent memory leaks.
- **Resource Ownership:** Clearly document or refactor who owns a pointer if it is passed across module boundaries.

---

## DRY (Don't Repeat Yourself)

### Detection

Look for:

- Similar code blocks across files
- Copy-paste with minor variations
- Repeated validation/parsing logic
- Duplicated error message formatting

### Refactoring Patterns

**Extract helper function:**

```cpp
// Before: duplicated in multiple places
if (zds->diag.e_msg_len > 0) {
  memcpy(dest, zds->diag.e_msg, zds->diag.e_msg_len);
}

// After: single helper
static void copy_diag_msg(ZDS *zds, char *dest) {
  if (zds->diag.e_msg_len > 0) {
    memcpy(dest, zds->diag.e_msg, zds->diag.e_msg_len);
  }
}
```

**Use RAII guards (when appropriate for ibm-clang code):**

```cpp
// See DiagMsgGuard pattern in zds.cpp for reference
class ResourceGuard {
  Resource *r_;
public:
  explicit ResourceGuard(Resource *r) : r_(r) {}
  ~ResourceGuard() { release(r_); }
  ResourceGuard(const ResourceGuard &) = delete;
  ResourceGuard &operator=(const ResourceGuard &) = delete;
};
```

**Consolidate similar switch cases:**

```cpp
// Before
case TYPE_A: return handle_type_a(data);
case TYPE_B: return handle_type_b(data);

// After (if logic is similar)
case TYPE_A:
case TYPE_B:
  return handle_common(type, data);
```

---

## YAGNI (You Ain't Gonna Need It)

### Detection

Look for:

- Commented-out code blocks
- Functions with no callers
- Parameters that are always passed the same value
- Unused struct members
- Speculative "future use" code

### Actions

- **Delete** dead code rather than commenting it out (use version control)
- **Remove** unused parameters after verifying no external callers
- **Simplify** over-engineered abstractions

---

## SRP (Single Responsibility Principle)

### Detection

Signs a function does too much:

- Has "and" in what it does ("parses and validates and transforms")
- Takes many boolean flags that change behavior
- Has deeply nested conditionals (> 3 levels)
- Mixes I/O with business logic

### Refactoring

**Split by responsibility:**

```cpp
// Before: one function doing everything
int process_dataset(const char *name, int flags) {
  // validation
  // parsing
  // transformation
  // output
}

// After: separate concerns
int validate_dataset_name(const char *name);
DatasetInfo parse_dataset(const char *name);
TransformResult transform_dataset(DatasetInfo *info, int flags);
int write_output(TransformResult *result);
```

---

## Complexity Reduction

### Metrics

- **Function length**: Target < 50 lines
- **Cyclomatic complexity**: Target < 10 branches (use as a warning/suggestion to guide refactoring, rather than a strict requirement)
- **Nesting depth**: Target < 4 levels

### Patterns

**Early return:**

```cpp
// Before
int process(Data *d) {
  if (d != nullptr) {
    if (d->valid) {
      // 50 lines of logic
    }
  }
  return -1;
}

// After
int process(Data *d) {
  if (d == nullptr) return -1;
  if (!d->valid) return -1;
  // 50 lines of logic
}
```

**Extract conditional logic:**

```cpp
// Before
if (a > 0 && b < 100 && (c == TYPE_A || c == TYPE_B) && d != nullptr) {

// After
static bool is_valid_range(int a, int b) {
  return a > 0 && b < 100;
}
static bool is_supported_type(int c) {
  return c == TYPE_A || c == TYPE_B;
}
if (is_valid_range(a, b) && is_supported_type(c) && d != nullptr) {
```

**Replace nested if-else with lookup:**

```cpp
// Before
const char *get_error_msg(int code) {
  if (code == 1) return "Error A";
  else if (code == 2) return "Error B";
  // ... many more
}

// After
static const char *ERROR_MSGS[] = { nullptr, "Error A", "Error B", ... };
const char *get_error_msg(int code) {
  if (code < 0 || code >= sizeof(ERROR_MSGS)/sizeof(ERROR_MSGS[0])) {
    return "Unknown error";
  }
  return ERROR_MSGS[code];
}
```

---

## Naming Conventions

### Functions

- Use verb_noun pattern: `parse_dataset`, `validate_input`, `get_member_list`
- Prefix with module. For example: `zds_`, `zjb_`, `zut_` for C++ functions in `zds.cpp`, `zjb.cpp`, and `zut.cpp` respectively

### Variables

- Descriptive names over comments: `member_count` not `cnt`
- Boolean variables: `is_valid`, `has_members`, `should_retry`

### Constants

- All caps with underscores: `MAX_DS_LENGTH`, `DEFAULT_TIMEOUT`

---

## Workflow

When improving code quality:

1. **Identify scope**: Single file or related files
2. **Search for patterns**: Use grep to find similar code blocks
3. **Categorize issues** by type (DRY, YAGNI, SRP, complexity)
4. **Propose changes** with rationale
5. **Verify no breaking changes** to public APIs before implementing

### Commands

Find potential duplicates:

```bash
rg -n "pattern" native/c/ native/zowed/
```

Analyze function length and complexity:
*Note: Use structural code search tools or read the target files directly to evaluate function lengths and branches. Avoid brittle brace-matching regex.*

---

## Code Style Reference

From `.clang-format`:

- Allman-style braces
- 2-space indentation
- Pointer alignment: right (`char *ptr`)
- Max line width: 120

---

## Anti-Patterns to Flag

| Pattern            | Issue                | Fix                              |
| ------------------ | -------------------- | -------------------------------- |
| Magic numbers      | Unclear meaning      | Extract to named constants       |
| God functions      | > 100 lines          | Split by responsibility          |
| Deep nesting       | Hard to follow       | Early returns, extract functions |
| Commented code     | Clutters codebase    | Delete (use git history)         |
| Boolean parameters | Unclear at call site | Use enums or separate functions  |
| Copy-paste code    | Maintenance burden   | Extract shared function          |
