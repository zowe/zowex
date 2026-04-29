---
name: metalc-code-quality
description: Improve z/OS Metal C code quality by identifying duplication, complexity, and design issues. Apply DRY, YAGNI, SRP principles. Use when refactoring, deduplicating, simplifying, or improving native Metal C code in native/c/.
---

# z/OS Metal C Code Quality

Identify and fix code quality issues in the native Metal C codebase (`native/c/`).

## Environment Constraints

**Compiler:** IBM XL C/C++ or Open XL C/C++ Metal C compiler.
**Runtime:** No standard C library (LE) is available. You cannot use standard `malloc`, `free`, or standard I/O streams directly. Avoid using the following functions in Metal C code that have a pre-requisite of using `__cinit()`: `calloc`, `free`, `malloc`, `__malloc31`, `rand`, `realloc`, `snprintf`, `srand`, `strdup`, `strtok`, `strtok_r`, `vsnprintf`, `vsprintf`, and `vsscanf`.
**Addressing Mode (AMODE):** Mixed AMODE 64 and AMODE 31. Explicit pointer sizing (`PTR32`, `PTR64`) and thunking is required.

## Quality Checklist

Before refactoring, evaluate code against:

- [ ] DRY: No duplicated logic
- [ ] YAGNI: No unused/speculative code
- [ ] SRP: Each function has one responsibility
- [ ] Complexity: Functions < 50 lines, cyclomatic complexity < 10 (treat as a warning/suggestion, not a strict requirement)
- [ ] Memory: Proper use of `storage_obtain` and `storage_release` with correct sizes. **No memory leaks on error paths.**
- [ ] AMODE Safety: Correct pointer types (`PTR32` vs `PTR64`) used for system services.
- [ ] Linkage: Proper `#pragma prolog` and `#pragma epilog` for functions.

---

## Memory Management & AMODE

### Guidelines

- **No `malloc`/`free`:** Use `storage_obtain31`, `storage_obtain24`, `storage_get64` and their corresponding release functions (`storage_release`, `storage_free64`).
- **AMODE Thunking:** When calling a 31-bit z/OS service from 64-bit code, you must allocate 31-bit storage, copy the 64-bit data into it, invoke the service with `PTR32` pointers, and copy the results back.
- **Pointer Declarations:** Always explicitly declare pointer sizes when crossing AMODE boundaries (e.g., `void *PTR32`, `char *PTR64`).
- **Memory Leaks:** Because Metal C lacks C++ RAII, you must manually ensure `storage_release` is called on all exit paths (including early returns for errors).

### Refactoring Patterns

**Ensure memory is freed on early returns:**

```c
// Before: Memory leak on error path
int process_data(const char *PTR64 input) {
  int len = strlen(input) + 1;
  char *PTR32 buf31 = storage_obtain31(len);
  strcpy(buf31, input);
  
  if (some_validation_fails(buf31)) {
    return RTNCD_FAILURE; // LEAK! buf31 is not released
  }
  
  do_work(buf31);
  storage_release(len, buf31);
  return RTNCD_SUCCESS;
}

// After: Proper cleanup
int process_data(const char *PTR64 input) {
  int len = strlen(input) + 1;
  char *PTR32 buf31 = storage_obtain31(len);
  strcpy(buf31, input);
  
  if (some_validation_fails(buf31)) {
    storage_release(len, buf31); // CLEANUP
    return RTNCD_FAILURE;
  }
  
  do_work(buf31);
  storage_release(len, buf31);
  return RTNCD_SUCCESS;
}
```

---

## DRY (Don't Repeat Yourself)

### Detection

Look for:

- Repeated AMODE thunking logic (allocating 31-bit memory, copying, calling, copying back, freeing).
- Repeated error message formatting using `ZDIAG`.
- Copy-paste with minor variations in system macro invocations.

### Refactoring Patterns

**Extract AMODE thunking to a helper:**

If multiple functions need to convert a specific 64-bit struct to 31-bit to call a service, extract that into a single wrapper function rather than duplicating the `storage_obtain31` and `memcpy` logic in every caller.

---

## YAGNI (You Ain't Gonna Need It)

### Detection

Look for:

- Commented-out code blocks or inline assembly.
- Functions with no callers.
- Unused struct members in custom structures (do not remove members from standard z/OS system structures like `DSCBFormat1`).

### Actions

- **Delete** dead code rather than commenting it out (use version control).
- **Remove** unused parameters after verifying no external callers.

---

## SRP (Single Responsibility Principle)

### Detection

Signs a function does too much:

- Mixes complex AMODE thunking with heavy business logic.
- Has deeply nested conditionals (> 3 levels).

### Refactoring

**Split by responsibility:**

Separate the AMODE boundary crossing from the actual logic. Have one function handle the 64-to-31 bit thunking, which then calls a pure 31-bit function that contains the business logic.

---

## Complexity Reduction

### Patterns

**Early return (with cleanup):**

```c
// Before
int process(ZDIAG *diag, Data *d) {
  int rc = RTNCD_SUCCESS;
  if (d != NULL) {
    if (d->valid) {
      // 50 lines of logic
    } else {
      diag->detail_rc = ZDS_RTNCD_INVALID;
      rc = RTNCD_FAILURE;
    }
  } else {
    rc = RTNCD_FAILURE;
  }
  return rc;
}

// After
int process(ZDIAG *diag, Data *d) {
  if (d == NULL) return RTNCD_FAILURE;
  
  if (!d->valid) {
    diag->detail_rc = ZDS_RTNCD_INVALID;
    return RTNCD_FAILURE;
  }
  
  // 50 lines of logic
  return RTNCD_SUCCESS;
}
```

---

## Naming Conventions & Pragmas

### Functions

- Use uppercase module prefixes for main entry points: `ZJSMINIT`, `ZDSDEL`, `ZUTWDYN`.
- Use `snake_case` for internal static helper functions.

### Pragmas

- Every exported Metal C function must have a corresponding `#pragma prolog` and `#pragma epilog` to manage the save area (DSA).
- Ensure the name in the pragma exactly matches the function name.

```c
#pragma prolog(ZMYFUNC, " ZWEPROLG NEWDSA=(YES,4) ")
#pragma epilog(ZMYFUNC, " ZWEEPILG ")
int ZMYFUNC(void *data) {
  // ...
}
```

### Strings and Padding

- When passing names (like module names or DD names) to z/OS services, they often require blank-padding rather than null-termination.
- Use `memset` to pad with spaces, then `memcpy` to copy the string.

```c
char name_truncated[8 + 1] = {0};
memset(name_truncated, ' ', sizeof(name_truncated) - 1); // pad with spaces
memcpy(name_truncated, program, strlen(program) > 8 ? 8 : strlen(program)); // copy and truncate
```

---

## Anti-Patterns to Flag

| Pattern | Issue | Fix |
| --- | --- | --- |
| Missing `storage_release` | Memory leak on error paths | Add cleanup before early returns |
| Missing `PTR32`/`PTR64` | AMODE mismatch / S0C4 ABEND | Explicitly cast and use correct pointer types |
| Missing `#pragma prolog` | Save area chain corruption | Add required pragmas for all entry points |
| Null-terminating z/OS names | Service expects blank-padded | Use `memset` with spaces and `memcpy` |
| Deep nesting | Hard to follow | Early returns (with proper memory cleanup) |
| Standard C library calls | Not available in Metal C | Use z/OS specific macros or custom implementations |
