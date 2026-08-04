# Runtime compatibility data

`zowex` is built on one LPAR and the resulting `server.pax.Z` ships to every customer system. It links
dynamically against the C++ runtime (libc++) that IBM ships **inside Language Environment**, in the DLL
`CRTEQCXE`. If the build references a symbol the target system's LE does not export, `zowex` fails at
**load** time, before `main()`:

```
CEE3561S External function _ZNSt5__1_e13__hash_memoryEPKvm was not found in DLL CRTEQCXE
```

The files here exist so that failure is caught by CI on the build host instead of by a user on a
customer system. See [zowe/zowex#871](https://github.com/zowe/zowex/issues/871) and
[`doc/troubleshooting.md`](../../../doc/troubleshooting.md).

## What determines compatibility

Three separate things, often confused:

| Lever | What it controls |
|---|---|
| **Open XL C/C++ level** used to build | Which libc++ symbols the binary references. This is the dominant factor. Open XL 2.1 is supported on z/OS 2.4/2.5/3.1; **Open XL 2.2 requires z/OS 3.1+** and references a newer libc++. |
| **LE maintenance (PTF) level** on the target | Which libc++ symbols the target actually exports. A z/OS release alone does not tell you this. |
| **`-mzos-target`** (`ZosTarget` in `toolchain.mk`) | Only the LE **system-header** API level, via `__TARGET_LIB__`. It makes no link-step change and does **not** restrict libc++. Useful, but not a fix for a missing libc++ export. |

## Files

| File | Meaning |
|---|---|
| `zos-min-level.txt` | The declared minimum supported z/OS release, as an Open XL target name. Single source of truth for the tooling. Changing it changes the floor for every consumer of the published artifact. |
| `runtime-imports.baseline` | Allow-list of the external symbols `zowex`/`zoweax` import. `make check-compat` fails when the built binary imports something not listed. |
| `CRTEQCXE.<level>.exports` | Snapshot of the libc++/LE side-deck exports on a real system at the minimum supported level. `make check-compat` fails when the binary imports something this snapshot does not contain — the check that actually catches a CEE3561S before shipping. |

## Expected build toolchain

- **Open XL C/C++ 2.1** (`/usr/lpp/IBM/cnw/v2r1/openxl/bin`), pinned in
  `.github/workflows/zos-build.yml`. The compiler version is *logged* into the check report rather
  than hard-asserted, so an LPAR upgrade shows up as a reviewable diff instead of a broken build.
- `ZosTarget=zosv2r5` (the default in `native/c/toolchain.mk`).

## Running the check

On z/OS, from the deployed `native/c` directory:

```sh
make check-compat          # verify; fails on a new or unavailable import
make check-compat-update   # refresh runtime-imports.baseline after a reviewed change
```

Or from a workstation, against the configured build system:

```sh
npm run z:check:compat
```

## Refreshing the data

### `runtime-imports.baseline`

Only after you have confirmed each added symbol is available at the minimum supported level:

```sh
make check-compat-update   # then review the diff and commit
```

Do **not** run this to make a red build green. If a symbol is genuinely unavailable on the floor
release, change the code to stop using it.

### `CRTEQCXE.<level>.exports`

Must be produced on a real system at the minimum supported level — the side decks for an older LE
cannot be fabricated on a newer LPAR:

```sh
sh tools/dump_sidedeck_exports.sh > compat/CRTEQCXE.zosv2r5.exports
```

Record the source system's LE PTF level in the file's header comment. **An export snapshot without a
maintenance level is not reproducible**, and a stale snapshot silently degrades this check to no
protection. Re-capture whenever the declared floor moves or the reference system takes LE maintenance.

## Unseeded state

Both `runtime-imports.baseline` and `CRTEQCXE.<level>.exports` need data from real systems. Until a
file is seeded, `check_runtime_symbols.sh` reports what is missing and continues (warning, not
failure), so the build is not blocked by data nobody has captured yet. Once seeded, the corresponding
check is enforced.
