# ISPF Member Statistics

This document summarizes how ISPF statistics are stored in Partitioned Data Set (PDS) directory entries on z/OS and how `zowex` validates them.

## PDS Directory Structure

PDS directories are comprised of 256-byte blocks containing variable-length member entries. Each entry starts with a 12-byte header (`NAME`, `TTR`, and control flag `C`), followed by up to 62 bytes of `USER_DATA`.

The control byte `C` specifies the `USER_DATA` length in halfwords. ISPF uses this space to store member statistics of either:

- **30 bytes** (15 halfwords) for standard statistics.
- **40 bytes** (20 halfwords) for extended statistics (used when line counts exceed 65,535).

## Member Statistics Validation

To avoid displaying corrupted attributes from non-standard user data (e.g., load module metadata or custom SCM attributes), `zowex` validates directory entries in `zds.cpp` using two heuristics:

1. **Size and Flag Check:** Standard stats must be exactly 30 bytes with trailing spaces in the `extended` field. Extended stats must be exactly 40 bytes with the extended flag (`0x20`) set in `flags`.
2. **Century Check:** Creation and modification century bytes must be `0x00` (1900s) or `0x01` (2000s). This will break in the year 2100.

If validation fails, `zowex` skips parsing stats and leaves the member's attributes blank instead of rendering garbled dates or line counts.

## Manual Testing

1. **Locate Corrupted Member:** Find a PDS containing a member with invalid user data in its directory entry (e.g., custom SCM metadata or link-edited load modules).
2. **Verify in ISPF:** View the member list in **ISPF Option 3.4**. It should display blank attributes, warning symbols, or question marks (`??/??/??`) for the corrupted member.
3. **Query via CLI:** Run the `zowex` command with attributes enabled:
   ```bash
   zowex ds list-members "MY.CORRUPTED.PDS" --attributes
   ```
4. **Confirm Safe Handling:** Verify the member is listed successfully, but its stats (dates and line counts) are cleanly omitted rather than displaying garbled strings.
