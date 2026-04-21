# Opening data sets: Handling abends

## Overview

Manual testing is required for some abends, such as [S913 (Insufficient
Permissions)](https://www.ibm.com/docs/en/zos/2.5.0?topic=messages-iec150i),
because creating an inaccessible data set in an automated test is not trivial.

This document describes how to:

- Create a data set that other users cannot access
- Implement a general error handling pattern around abend codes from LE C/C++
  code

## Creating an inaccessible data set

In IBM's Resource Access Control Facility (RACF), a "private" data set is one
that only the owner can access. All other users are denied access.

### Creating a profile with UACC(NONE)

The most effective way to make a data set private is to create a RACF profile
for it and set the **Universal Access Authority (UACC)** to **NONE**. This
ensures that any user not specifically listed in the data set's access list is
denied access.

- **For a single specific data set (Discrete Profile):**
  Use the `ADDSD` command to protect a specific data set:

  ```text
  ADDSD 'HLQ.DATASET.NAME' UACC(NONE)
  ```

- **For a group of data sets (Generic Profile):**
  Protect multiple data sets that share a common naming pattern using a generic
  profile:

  ```text
  ADDSD 'HLQ.PRIVATE.**' UACC(NONE) GENERIC
  ```

> [!NOTE]
> The double asterisk (`**`) acts as a wildcard for all qualifiers following
> the prefix.

### Testing the S913 abend

To test the S913 (Insufficient Permissions) abend, you need two separate user
IDs:

1. **User A (Creator):** Create a partitioned data set and protect it using the
   commands above.
2. **User B (Tester):** Attempt to list members for the protected data set
   created by User A. This triggers the S913 abend and validates the error
   handling.

When listing members for this data set, you see the following error:

```
Insufficient permissions for opening data set 'HLQ.DATASET.NAME' (S913 abend)
```

## Handling abends from `fopen`

When an abend occurs during data set opening using the `fopen` C function, the
`errno` variable (which contains the current error number) is set to `EABEND`
(92). You can use the `__amrc` structure defined in the `stdio.h` C header to
access the abend code:

```cpp
constexpr auto EABEND = 92;
if (errno == EABEND) // EABEND
{
    __amrc_type save_amrc = *__amrc;
    const auto abend_code = save_amrc.__code.__abend.__syscode;
    if (abend_code == 0x913)
    {
        // Insufficient permissions for this data set
        zds->diag.e_msg_len = snprintf(
            zds->diag.e_msg,
            sizeof(zds->diag.e_msg),
            "Insufficient permissions for opening data set '%s' (S913 abend)",
            dsn.c_str());
    }
}
```

Refer to the IBM article ["Using the __amrc
structure"](https://www.ibm.com/docs/en/zos/3.1.0?topic=dip-using-amrc-structure)
for more details on the diagnostic information in the `__amrc` structure.
