---
name: data-integrity
description: Prevent data loss and unsafe operations in data set and USS workflows, including cross-profile, cross-LPAR, and overwrites. Use when performing move, copy, upload, rename, or delete operations on z/OS resources.
---

# Data Integrity Prevention

Prevent data loss during z/OS data set and USS file operations.

## Pre-Operation Checklist

Before any destructive operation:

- [ ] **Source verification**: Confirm source exists and is readable
- [ ] **Target collision**: Check if target already exists
- [ ] **Cross-profile check**: Verify different profiles don't reference same resource
- [ ] **Backup strategy**: Ensure source preserved until target verified

## High-Risk Operations

**Dataset operations:**
- `copyDatasetOrMember` with `overwrite: true`
- `renameDataset` or `renameMember`
- Any operation between different profile connections

**USS operations:**
- `moveFile` (defaults `force: true` - source deleted on success)
- `copyUss` with `force: true`
- `deleteFile` (irreversible)

## Safety Patterns

**Two-phase operations:**
```
1. Copy source → target (preserve source)
2. Verify target integrity
3. Only then remove source (if move)
```

**Profile validation:**
```typescript
// Check if profiles might reference same system
if (sourceProfile.host === targetProfile.host && 
    sourceProfile.user === targetProfile.user) {
  // Same LPAR - extra caution needed
}
```

## Warning Signs

Stop and confirm with user if:
- Target path already exists without explicit overwrite
- Cross-profile operation detected
- Batch operation affects >10 resources
- Production profile involved (profile name contains 'prod', 'production')

## Quick Verification

For moves/renames, verify target created before considering source removal:
```typescript
const targetExists = await api.listFiles(targetPath);
if (!targetExists) throw new Error('Target verification failed');
```