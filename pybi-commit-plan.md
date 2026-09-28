# Commit split plan — `zkr_py` certificate/key ring Python bindings

Goal: land the feature as a bisectable sequence where every commit compiles/typechecks
on its own. Native C linkage → shared I/O extraction → binding glue → build wiring →
tests → docs, in that order, because each later layer depends on the one before it.
Mirrors the order proposed in
[python-bindings-certificates-plan.md](python-bindings-certificates-plan.md#suggested-commit-order),
adjusted for how the diff actually landed (see deviations noted per commit).

Legend: `git add <files>` lists are exact — copy/paste them in order.

---

## 1. `feat(native): add C linkage and an out-param filter to zkr for the bindings`

```
git add native/c/zkr.hpp native/c/zkr.cpp
```

- Wraps the `zkr_*` declarations in `#ifdef SWIG extern "C" { ... }`, matching the
  existing pattern in `zds.hpp`/`zjb.hpp`/`zusf.hpp` — without it nothing SWIG-generated
  can call into `zkr`, but there is no `-DSWIG` in the `zowex` build, so this is a
  no-op for the CLI/RPC server.
- Adds `zkr_filter_certs_into` (out-param sibling, inside the `extern "C"` block)
  because C linkage cannot express `zkr_filter_certs`'s `std::vector<ZKRCertInfo>`
  return-by-value. `zkr_filter_certs` becomes a two-line wrapper around it, so its
  existing callers (`certificates.cpp`) and unit tests (`zkr.test.cpp`) are
  unaffected — pure refactor, no behavior change.
- No makefile change needed here: the `SWIG_EXTENDER_OBJS`/`OUT_DIR_SWIG` wiring for
  `zkr.o` lands in commit 2 bundled with `zkrio.o`, since splitting that single
  makefile line by hunk buys nothing (see commit 2's note).
- Verify: `npm run z:test -- zkr` (existing `zkr_filter_certs` tests must still pass
  unchanged) after `npm run z:make -- clean && npm run z:upload && npm run z:build`.

## 2. `refactor(native): extract certificate material I/O into zkrio`

```
git add native/c/zkrio.hpp native/c/zkrio.cpp native/c/commands/certificates.cpp native/c/makefile
```

- New `native/c/zkrio.{hpp,cpp}`: `zkrio_read_dsn` / `zkrio_write_dsn` are a straight
  move of the `read_cert_dsn`/`write_cert_dsn` helpers out of `certificates.cpp`'s
  anonymous namespace; `zkrio_write_file` is a new one-line wrapper around
  `zut_write_file_private` — needed because `zut.hpp` has no `extern "C"` block, so
  the ASCII-compiled binding glue in commit 3 cannot call it directly.
- `certificates.cpp` drops both helpers and calls `zkrio_read_dsn` / `zkrio_write_dsn`
  / `zkrio_write_file` instead (the last replacing the direct `zut_write_file_private`
  call in `handle_cert_export`). Pure refactor, no behavior change — the existing
  gated `--dsn` tests are the regression net.
- `native/c/makefile`: adds the plain `$(OUT_DIR)/zkrio.o` build rule and the `zowex`
  link line update (required — `zowex` now needs `zkrio.o`), **plus**, bundled into
  the same commit for convenience, the `OUT_DIR_SWIG` rules for `zkr.o`/`zkrio.o` and
  their addition to `SWIG_EXTENDER_OBJS`. That second half is cosmetic — the Python
  build in commits 3–4 compiles `zkr.cpp`/`zkrio.cpp` itself via `setup.py`'s
  `Extension(sources=...)`, it never consumes `SWIG_EXTENDER_OBJS` — but it is one
  line touching the same file, so splitting it out gains nothing.
- Depends on commit 1 (`zkr_filter_certs_into`'s declaration must already have moved
  into the `extern "C"` block, though `certificates.cpp` doesn't call it — commit 3
  does).
- Verify: `npm run z:test -- certificates` and `npm run z:test -- ds` unchanged.

## 3. `feat(python): add zkr_py certificate and key ring bindings`

```
git add native/python/bindings/zkr_py.hpp native/python/bindings/zkr_py.cpp \
        native/python/bindings/zkr_py.i native/python/bindings/__init__.py
```

- The binding glue: 18 callables covering all 14 RPC-parity certificate/key-ring
  operations, the `ZkrBytes` typemap (PKCS#12/PEM cross as Python `bytes`, not `str`
  — the highest-risk item per the plan, since `std_string.i`'s default typemap would
  silently mangle binary payloads through UTF-8 surrogateescape), and the `ZkrError`
  exception carrying `saf_rc`/`esm_rc`/`esm_rsn`/`gsk_rc`/`function_code`/`service`.
- **Deviation from the plan:** the plan proposed splitting the `ZkrError` diagnostics
  (its step 4b) into a separate, droppable commit from the `ZkrBytes` typemap (4a) and
  struct declarations (4c). As implemented, `zkr_py.i` interleaves them tightly —
  `%ignore ZkrError`, the `g_zkr_error` module exception, `%init`, and `%exception`
  sit in the same 70-line block as the `ZkrBytes` typemaps (lines 24–95). Splitting
  that by hunk would mean hand-slicing a new file line-by-line for no reviewability
  gain, so both land together here.
- Depends on commits 1 (`zkr_filter_certs_into`) and 2 (`zkrio_*`, called from
  `export_certificate_to_file/_to_dsn` and `import_certificate_from_file/_from_dsn`).
- Does **not** build standalone yet — `setup.py`/`Makefile` don't know about `zkr_py`
  until commit 4. That's an acceptable bisect gap of one commit, same tradeoff the
  plan flagged for its commits 3+5.

## 4. `build(python): wire the zkr_py extension into both distributions`

```
git add native/python/bindings/setup.py native/python/bindings/Makefile \
        native/python/bindings/package_zbind.py native/python/bindings/package_precompiled.py
```

- `setup.py`: new `_zkr_py` `Extension` (sources include `zkr.cpp`/`zkrio.cpp`/
  `zds.cpp`/`zut.cpp` plus the GSKCMS64 side deck in `extra_objects`), added to the
  default module set and `ext_modules`/`py_modules`.
- `Makefile`: `swig-wrappers`/`build-zkr`/`swig-zkr`/`help`/`.PHONY` entries.
- `package_zbind.py` / `package_precompiled.py`: same wiring for the source and
  precompiled distribution bundles (`bindings_files`, `core_sources`, the embedded
  `setup_py_content`, `py_files`, `init_content`).
- **Two pre-existing bugs surfaced and fixed here, unrelated to `zkr_py` itself** —
  worth calling out to a reviewer even though they're bundled rather than split out,
  since untangling them from this diff would mean re-deriving which lines are
  "new for zkr" vs "fixing something that already affected zusf/zds/zjb":
  - `package_zbind.py`'s `object_files` list was missing `zutcall24.o` — the plan
    flagged this in Step 5 ("it is missing today... worth a separate look"). Fixed by
    adding it to the shared `object_files` list, so `zusf_py`/`zds_py`/`zjb_py`'s
    bundled `extra_objects` now include it too, not just `zkr_py`'s.
  - `package_zbind.py`'s step 10 file tagging did `chtag -t` (retag-only, no byte
    conversion) on the SWIG-generated `*_py.py` wrapper files, which are copied
    verbatim from the EBCDIC-compiled build tree — so every prior bundle shipped
    those files mislabeled as ISO8859-1 while still holding IBM-1047 bytes. Fixed by
    `iconv`-converting them before tagging. This silently corrupted `import` of the
    source-distribution bundle for `zusf_py`/`zds_py`/`zjb_py` too, not just the new
    `zkr_py.py` — **consider filing this as its own GitHub issue** the way
    `commit-plan.md` did for the JFCB password-protection bug, since it predates this
    branch and isn't scoped to `zkr_py`.
- Depends on commit 3 (`zkr_py.{hpp,cpp,i}` must exist to be listed as sources).
- Verify: `npm run z:python:build` (after `npm run z:make -- clean` — Steps 1–2
  changed headers, which do not trigger dependent rebuilds), then
  `grep -c PyBytes_FromStringAndSize native/python/bindings/zkr_py_wrap.cxx` (must be
  ≥ 1 — confirms the `ZkrBytes` typemap won, not the `str` fallback), then
  `python -c "import zkr_py"`.

## 5. `test(python): add zkr_py coverage`

```
git add native/python/bindings/test/test_zkr.py native/python/bindings/test/Makefile \
        native/python/bindings/test/fixtures/env.example.yml .github/workflows/zos-py-build.yml
```

- `test_zkr.py`: three-tier gating (no-authority / `can_mutate` probe / full
  PKCS#12 fixture), mirroring `zkr.test.cpp`'s structure — validation errors,
  `ZkrError` diagnostics, byte-exactness (`data[0] == 0x30`, `b"\x00" in data`),
  DSN/file parity (sequential **and** PDS/E member), PEM-is-ASCII-in / stays-EBCDIC
  on disk, RACDCERT-LABEL-style case-sensitive filtering.
- `test/Makefile`: `TEST_ZKR`/`JUNIT_ZKR`, `test-zkr` target wired into `test` and
  `merge-results`.
- `env.example.yml`: `KEYRING_PREFIX`, and documents the optional `ZKR_TEST_P12` /
  `ZKR_TEST_P12_PASS` (and `ZNP_`-prefixed) fixture variables.
- `.github/workflows/zos-py-build.yml`: adds `KEYRING_PREFIX` to the generated
  `env.yml`, and adds `native/c/zkr.{hpp,cpp}` / `native/c/zkrio.{hpp,cpp}`
  individually to the `paths:` filter (not all of `native/c/**` — the job shares the
  `zos-build` concurrency group and takes ~20 minutes). Expect Tiers B/C to skip in
  CI (no certificate authority there), same state the C++ suite is already in.
- Depends on commits 1–4 all being present (needs a buildable, importable `zkr_py`).
- Verify: `npm run z:python:test`; expect Tier A to run and pass, Tiers B/C to skip
  without `KEYRING_PREFIX`/fixture authority. Remember to unset
  `_BPXK_JOBLOG=STDERR` first if testing on `lpar.1` — unrelated to this suite, but it
  fakes ~12 jobs-server RPC failures in the same `npm run z:test` run
  (`[[joblog-breaks-native-tests]]`).

## 6. `docs: document the zkr_py certificate bindings`

```
git add native/python/bindings/README.md doc/apis.md doc/certificates-test-plan.md native/CHANGELOG.md
```

- `README.md`: new `### zkr_py — Certificates and key rings` function table (18 rows)
  and supporting types; updates the "Architecture", "Why calls across the line need
  `extern "C"`" (now citing `zkr_filter_certs`/`zkr_filter_certs_into` as the live
  "cannot return a C++ type" example), and "Which strings get converted" sections to
  include the fourth module.
- `doc/apis.md`: new "Certificates" section — there was none before — with the 14
  operations across the `Backend/SDK/CLI/Python` columns.
- `doc/certificates-test-plan.md`: adds the Python suite to §1's tier description and
  a `zkr_py (in-process, Tier)` column to §2's coverage matrix, explicit about this
  being complementary to (not a substitute for) the out-of-process RPC-layer test
  item already in that plan.
- `native/CHANGELOG.md`: three `## Recent Changes` bullets (the `zkr_py` module, its
  binary-safe `bytes` handling, and `zkrio`), each with a
  `[#NNNN](https://github.com/zowe/zowex/pull/NNNN)` placeholder.
- Last in the sequence so every doc reference (line numbers, function names) points
  at code that already exists in history.
- **Before pushing:** replace the `#NNNN` placeholders in `native/CHANGELOG.md` with
  the real PR number once one is opened (same loose end `commit-plan.md` flagged for
  its own CHANGELOG entries).

---

## Untracked planning docs — do not commit as-is

```
?? python-bindings-certificates-plan.md
?? pybi-commit-plan.md
```

Same treatment `commit-plan.md` recommended for `initial-plan.md`: these are working
design/process docs for this implementation session, not project documentation
someone would want checked out permanently, and their content is now redundant with
the actual code plus `doc/certificates-test-plan.md` / `native/python/bindings/README.md`.
Delete both (or move them out of the repo) once commits 1–6 land. If a permanent
design record is wanted instead, fold the relevant parts of
`python-bindings-certificates-plan.md` into commit 6 as a doc, rather than leaving the
raw planning doc at the repo root.

`commit-plan.md` and `initial-plan.md` themselves belong to the separate, already-landed
`--dsn` feature branch (see git log: `5d61463f`..`be073168`) — not part of this plan,
already handled by `commit-plan.md`'s own recommendations.

---

## Quick reference: commit order vs. dependency

```
1 zkr.hpp/zkr.cpp extern "C" + filter_certs_into ──┐
                                                     ▼
2 zkrio.{hpp,cpp} + certificates.cpp + makefile (dep: 1)
                                                     │
                                                     ▼
3 zkr_py.{hpp,cpp,i} + __init__.py (dep: 1, 2)
                                                     │
                                                     ▼
4 setup.py + Makefile + package_{zbind,precompiled}.py (dep: 3)
                                                     │
                                                     ▼
5 test_zkr.py + test/Makefile + fixtures + workflow yml (dep: 1-4)
                                                     │
                                                     ▼
6 docs + changelog (dep: 1-5 all landed)
```

No commit here needs partial-file (`git add -p`) staging — unlike the plan's original
7-step proposal, every file's changes land wholly within one commit in the sequence
above.
