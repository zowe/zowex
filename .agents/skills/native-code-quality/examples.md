# Code Quality Examples

Real-world refactoring examples from the Zowe native codebase.

## Example 1: RAII Pattern for Resource Management

From `native/c/zds.cpp` - DiagMsgGuard preserves diagnostic messages:

```cpp
class DiagMsgGuard
{
  ZDS *zds_;
  char saved_msg_[256];
  int saved_msg_len_;

public:
  explicit DiagMsgGuard(ZDS *zds)
      : zds_(zds), saved_msg_len_(zds->diag.e_msg_len)
  {
    memcpy(saved_msg_, zds->diag.e_msg, sizeof(saved_msg_));
  }

  ~DiagMsgGuard()
  {
    memcpy(zds_->diag.e_msg, saved_msg_, sizeof(saved_msg_));
    zds_->diag.e_msg_len = saved_msg_len_;
  }

  // Non-copyable
  DiagMsgGuard(const DiagMsgGuard &) = delete;
  DiagMsgGuard &operator=(const DiagMsgGuard &) = delete;
};
```

**Why it's good:**

- Eliminates manual save/restore boilerplate
- Exception-safe resource handling
- Clear ownership semantics

---

## Example 2: Helper Struct for Complex State

From `native/c/zds.cpp` - TruncationTracker compresses line ranges:

```cpp
struct TruncationTracker
{
  int count;
  int range_start;
  int range_end;
  string lines_str;

  TruncationTracker()
      : count(0), range_start(-1), range_end(-1), lines_str("")
  {
    lines_str.reserve(128);
  }

  // Methods to add lines, flush ranges, etc.
};
```

**Why it's good:**

- Encapsulates related state
- Pre-allocates string buffer
- Clear initialization

---

## Example 3: DRY - Extracting Common Validation

**Before (duplicated across handlers):**

```cpp
// In handler A
if (name == nullptr || strlen(name) == 0) {
  set_error(ctx, "Name is required");
  return -1;
}
if (strlen(name) > MAX_NAME_LENGTH) {
  set_error(ctx, "Name exceeds maximum length");
  return -1;
}

// In handler B (same code repeated)
if (name == nullptr || strlen(name) == 0) {
  set_error(ctx, "Name is required");
  return -1;
}
if (strlen(name) > MAX_NAME_LENGTH) {
  set_error(ctx, "Name exceeds maximum length");
  return -1;
}
```

**After (single validation function):**

```cpp
static int validate_name(Context *ctx, const char *name) {
  if (name == nullptr || strlen(name) == 0) {
    set_error(ctx, "Name is required");
    return -1;
  }
  if (strlen(name) > MAX_NAME_LENGTH) {
    set_error(ctx, "Name exceeds maximum length");
    return -1;
  }
  return 0;
}

// In handlers
if (validate_name(ctx, name) != 0) return -1;
```

---

## Example 4: Reducing Complexity with Early Returns

**Before:**

```cpp
int process_member(Dataset *ds, const char *member) {
  int result = -1;
  if (ds != nullptr) {
    if (member != nullptr) {
      if (strlen(member) <= 8) {
        if (ds->is_pds) {
          // actual processing (30+ lines)
          result = 0;
        } else {
          set_error("Dataset is not a PDS");
        }
      } else {
        set_error("Member name too long");
      }
    } else {
      set_error("Member name required");
    }
  } else {
    set_error("Dataset is null");
  }
  return result;
}
```

**After:**

```cpp
int process_member(Dataset *ds, const char *member) {
  if (ds == nullptr) {
    set_error("Dataset is null");
    return -1;
  }
  if (member == nullptr) {
    set_error("Member name required");
    return -1;
  }
  if (strlen(member) > 8) {
    set_error("Member name too long");
    return -1;
  }
  if (!ds->is_pds) {
    set_error("Dataset is not a PDS");
    return -1;
  }

  // actual processing (30+ lines)
  return 0;
}
```

---

## Example 5: YAGNI - Removing Unused Code

**Indicators to look for:**

```cpp
// TODO: might need this later
// int unused_feature(void) { ... }

// Legacy code from old implementation
#if 0
void old_approach(void) { ... }
#endif

// Parameter always passed as same value
int func(int required, int always_zero) {
  // always_zero is never anything but 0
}
```

**Action:** Delete and let git preserve history.

---

## Example 6: SRP - Splitting a God Function

**Before (single function doing too much):**

```cpp
int handle_request(Request *req) {
  // 1. Parse input (20 lines)
  // 2. Validate (15 lines)
  // 3. Authenticate (10 lines)
  // 4. Business logic (40 lines)
  // 5. Format response (20 lines)
  // 6. Send response (10 lines)
}
```

**After (separate concerns):**

```cpp
int handle_request(Request *req) {
  ParsedInput *input = parse_request(req);
  if (input == nullptr) return -1;

  if (validate_input(input) != 0) return -1;
  if (authenticate(req) != 0) return -1;

  Result *result = process_business_logic(input);
  if (result == nullptr) return -1;

  return send_response(req, result);
}
```

Each extracted function can now:

- Be tested independently
- Be reused elsewhere
- Be modified without affecting others
