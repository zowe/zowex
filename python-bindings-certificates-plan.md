# Add certificate / key ring support to the Python bindings (`zkr_py`)

## Context

[initial-plan.md](initial-plan.md#L633-L640) closed with:

> The **Python bindings need no change**: `native/python/bindings` covers only
> `zds`/`zjb`/`zusf`; there is no `zkr` binding.

That was accurate as a *scope* statement for `--dsn`, and it is exactly the gap this
plan closes. Today the certificate surface exists at four layers and is missing from
the fifth:

| Layer | Certificate support | Where |
|---|---|---|
| Service (C++) | 13 entry points | [zkr.hpp:154-268](native/c/zkr.hpp#L154-L268) |
| CLI (`zowex`) | 14 commands | [certificates.cpp](native/c/commands/certificates.cpp) |
| JSON-RPC | 14 methods | [rpc_commands.cpp:327-370](native/c/server/rpc_commands.cpp#L327-L370) |
| SDK / CLI (TS) | 14 request/response types | [packages/sdk/src/doc/rpc/certificates.ts](packages/sdk/src/doc/rpc/certificates.ts) |
| **Python bindings** | **none** | [native/python/bindings/__init__.py](native/python/bindings/__init__.py#L1-L5) |

**Outcome:** a fourth binding module, `zkr_py`, that covers all 14 certificate
operations in-process (no `zowex server`, no JSON-RPC, no subprocess), with
byte-exact PKCS#12 handling via Python `bytes`, structured SAF/ESM diagnostics on
exceptions, and the `--file` / `--dsn` sinks and sources that the CLI already has.

### Why this is not just "one more module like `zusf_py`"

Four things make `zkr` structurally different from the three existing modules, and
each one is a step below:

1. **Binary payloads.** PKCS#12 is arbitrary bytes. Every existing binding returns
   `std::string`, which SWIG's `std_string.i` hands to Python as `str`. A p12 blob is
   not valid UTF-8 — this is the single highest-risk item in the plan (Step 4).
2. **No `extern "C"` block.** [zkr.hpp](native/c/zkr.hpp) has no
   `#ifdef SWIG extern "C"` section, unlike [zds.hpp:101](native/c/zds.hpp#L101),
   [zjb.hpp:77](native/c/zjb.hpp#L77) and [zusf.hpp:77](native/c/zusf.hpp#L77). Without
   one, nothing links (Step 1).
3. **Extra link dependency.** `zkr` needs the System SSL side deck
   `/usr/lib/GSKCMS64.x` ([makefile:17](native/c/makefile#L17)) — the first binding to
   need anything beyond the Metal C objects (Step 5).
4. **File/data-set I/O lives in the command layer.** `read_cert_dsn` /
   `write_cert_dsn` sit in an anonymous namespace in
   [certificates.cpp:116-198](native/c/commands/certificates.cpp#L116-L198), reachable
   from nothing else (Step 2).

---

## Decisions

Each row is a recommendation with its rationale; the ones marked ⚠️ are worth a
second opinion before implementation starts.

| # | Decision | Choice | Why |
|---|---|---|---|
| D1 | Naming | RPC method names in `snake_case`: `create_keyring`, `list_certificates`, `refresh_digtcert`, … | The RPC names are the canonical cross-surface vocabulary (SDK, CLI, MCP all use them). Gives a clean 14:14 mapping and a reviewable checklist. |
| D2 | Parameter style | **Flat scalar parameters** with defaults, not the native options structs | `ZKRExportOptions`/`ZKRImportOptions`/`ZKRConnectOptions`/`ZKRAlterOptions` would need four SWIG proxy classes and four-line call sites in Python. The glue builds them locally. `DS_ATTRIBUTES` stays exposed in `zds_py` only because it has 17 fields. |
| D3 | Binary payloads | A `ZkrBytes` typedef with explicit `bytes` typemaps | See Step 4. Base64 (what the RPC layer does) is the wrong answer for Python. |
| D4 | Export/import sinks and sources | Three functions each, not one function with mutually exclusive flags | `export_certificate` → `bytes`; `export_certificate_to_file`; `export_certificate_to_dsn`. Same for import. Removes the "not both / neither" validation entirely and keeps `bytes` off the default-argument path (Step 4, risk R1). |
| D5 | `--database` / `--from-database` | Keep the boolean flags, 1:1 with the CLI/RPC, and replicate the CLI's validation in the glue | Parity beats elegance here: the flags exist to stop `'*'` reaching R_datalib ([certificates.cpp:66-79](native/c/commands/certificates.cpp#L66-L79)) and the same footgun exists in Python. |
| D6 | Errors | `RuntimeError` subclass `ZkrError` carrying `saf_rc`, `esm_rc`, `esm_rsn`, `function_code`, `gsk_rc`, `service` | The RPC layer deliberately exposes `safReturns` programmatically ([certificates.cpp:34-45](native/c/commands/certificates.cpp#L34-L45)). Dropping it in Python would make the bindings the only surface where you cannot tell "not authorized" from "no such ring". |
| D7 | Warnings | Every operation that can produce a non-fatal SAF warning returns it as a `str` (`""` when none) | `import` (already-exists), `delete` (refresh required), `list_rings` (truncation) all set `zkr.diag.warning`, and the CLI prints it as `Note: …`. Swallowing it silently would be a real defect. |
| D8 ⚠️ | Shared cert-material I/O | New `native/c/zkrio.{hpp,cpp}`, called by both `certificates.cpp` and `zkr_py.cpp` | The alternative is duplicating ~70 lines including the RACDCERT-parity attribute constants (PS/VB/84/27998) in the glue, where they would silently drift. Cost: one new translation unit + makefile rules. |
| D9 ⚠️ | `zkr_filter_certs` | Add an out-param sibling `zkr_filter_certs_into` inside the `extern "C"` block; make the existing function delegate to it | `zkr_filter_certs` returns `std::vector<ZKRCertInfo>` **by value**, which the bindings README calls out as the one thing C linkage cannot do ([README.md:214-216](native/python/bindings/README.md#L214-L216)). Re-implementing RACDCERT LABEL parity in the glue would fork logic that has its own unit tests. |
| D10 | Result structs | Reuse the native `ZKRCertInfo`/`ZKRCertDetail`/`ZKRRingCert`/`ZKRRingEntry`, re-declared in the `.i` | Matches the established `zds_py.i`/`zjb_py.i` pattern. Only two new structs, for the native out-params: `ZkrCertList` and `ZkrRingList`. |
| D11 | PEM encoding | Returned `bytes` are ISO8859-1; bytes written to a **file or data set** stay EBCDIC | Mirrors the CLI exactly: it converts only for the programmatic `data` field and keeps file/DSN output "byte-identical to keyring-util" ([certificates.cpp:371-385](native/c/commands/certificates.cpp#L371-L385)). Must be documented loudly — it is surprising. |
| D12 | GIL | Not released | No existing module does. Certificate calls are short. Non-goal. |

### The API surface

```python
import zkr_py as kr

# key rings
kr.create_keyring(owner, keyring)                                    -> str   # warning
kr.delete_keyring(owner, keyring)                                    -> str
kr.list_rings(owner, keyring="")                                     -> ZkrRingList
kr.count_ring(owner, keyring)                                        -> int
kr.refresh_digtcert()                                                -> str

# certificates in a ring
kr.list_certificates(owner, keyring, label="", usage="",
                     max_entries=10)                                 -> ZkrCertList
kr.show_certificate(owner, keyring, label)                           -> ZKRCertDetail
kr.set_default_certificate(owner, keyring, label)                    -> str
kr.connect_certificate(owner, keyring, label, from_ring="",
                       from_database=False, usage="",
                       make_default=False)                           -> str
kr.delete_certificate(owner, keyring, label,
                      database=False, skip_refresh=False)            -> str

# certificate records (DataAlter; no ring involved)
kr.trust_certificate(owner, label, status)                           -> str
kr.rename_certificate(owner, label, new_label)                       -> str

# export -- bytes, or straight to a sink
kr.export_certificate(owner, keyring, label,
                      format="pem", password="")                     -> bytes
kr.export_certificate_to_file(owner, keyring, label, file,
                              format="pem", password="")             -> int   # bytes written
kr.export_certificate_to_dsn(owner, keyring, label, dsn,
                             format="pem", password="")              -> int

# import -- from bytes, or from a source
kr.import_certificate(owner, keyring, label, usage, password,
                      data, skip_refresh=False)                      -> str   # warning
kr.import_certificate_from_file(owner, keyring, label, usage,
                                password, file, skip_refresh=False)  -> str
kr.import_certificate_from_dsn(owner, keyring, label, usage,
                               password, dsn, skip_refresh=False)    -> str
```

New structs:

```cpp
struct ZkrCertList  { std::vector<ZKRCertInfo>  items; bool more_available; };
struct ZkrRingList  { std::vector<ZKRRingEntry> items; std::string warning; };
```

---

## Step 1 — C linkage for `zkr` (`native/c/zkr.hpp`)

Wrap the declarations at [zkr.hpp:150-268](native/c/zkr.hpp#L150-L268) in the same
guard the other three service headers use, copying the comment verbatim from
[zjb.hpp:74-80](native/c/zjb.hpp#L74-L80):

```cpp
// The bindings compile this header EBCDIC and their SWIG wrappers ASCII. libc++ uses a distinct
// inline namespace per char mode (std::__1 vs std::__1_a), so a mangled name is unresolvable
// across that boundary -- everything the bindings call needs C linkage.
#ifdef SWIG
extern "C"
{
#endif
  ... zkr_new_ring ... zkr_alter_cert ...
#ifdef SWIG
}
#endif
```

Obligations:

1. **Every function the glue calls goes inside**, or the bind fails with `IEW2456E`.
   That is all of them except `zkr_filter_certs`.
2. **`zkr_filter_certs` stays outside** (D9) and gains a sibling inside the block:

   ```cpp
   void zkr_filter_certs_into(const std::vector<ZKRCertInfo> &certs,
                              const std::string &label, const std::string &usage,
                              size_t max_entries, bool *more_available,
                              std::vector<ZKRCertInfo> &out);
   ```

   `zkr_filter_certs` becomes a two-line wrapper around it, so
   [certificates.cpp:235-236](native/c/commands/certificates.cpp#L235-L236) and the
   existing `zkr_filter_certs` unit tests
   ([zkr.test.cpp:290-388](native/c/test/zkr.test.cpp#L290-L388)) keep working
   unchanged and both paths share one implementation.
3. **No overloads.** There are none in `zkr.hpp` today — keep it that way, or the
   `#ifndef SWIG` dance from [zjb.hpp:47-72](native/c/zjb.hpp#L47-L72) is needed.
4. **Default arguments are fine** inside the block (`skip_refresh = false`,
   `max_entries = 0`, `more_available = nullptr`) — they are resolved at the call
   site, not in the symbol.

There is no `-DSWIG` in the `zowex` build ([toolchain.mk:84-85](native/c/toolchain.mk#L84-L85)),
so this change is a no-op for the CLI and RPC server.

## Step 2 — Shared cert-material I/O (`native/c/zkrio.{hpp,cpp}`)

`zkr.hpp` documents that the service layer is deliberately free of data-set
knowledge — that is why `ZKRImportOptions.p12_data` exists at all
([zkr.hpp:216-224](native/c/zkr.hpp#L216-L224)). Keep that boundary; give the I/O its
own unit so both the command layer and the bindings use one copy.

New `native/c/zkrio.hpp`, all three inside an `#ifdef SWIG extern "C"` block:

```cpp
int zkrio_read_dsn(const std::string &dsn, std::string &data, std::string &err);
int zkrio_write_dsn(const std::string &dsn, const std::string &data, bool is_binary, std::string &err);
int zkrio_write_file(const std::string &path, const std::string &data, std::string &err);
```

`zkrio.cpp` is a straight move of
[`read_cert_dsn`](native/c/commands/certificates.cpp#L116-L142) and
[`write_cert_dsn`](native/c/commands/certificates.cpp#L144-L196), plus a one-line
`zkrio_write_file` delegating to `zut_write_file_private`
([zut.hpp:480](native/c/zut.hpp#L480)).

Why `zkrio_write_file` matters: **`zut.hpp` has no `extern "C"` block at all**, so the
ASCII glue cannot call `zut_write_file_private` (or `zut_prepare_encoding`, or
`zut_encode`) directly. Routing the private-file write through `zkrio` gets the glue
the 0600-from-creation guarantee without touching `zut.hpp`.

Then:

- [certificates.cpp](native/c/commands/certificates.cpp) drops the two helpers from
  its anonymous namespace, `#include "../zkrio.hpp"`, and calls
  `zkrio_read_dsn` / `zkrio_write_dsn` / `zkrio_write_file` (the last replacing the
  direct `zut_write_file_private` call at
  [certificates.cpp:341](native/c/commands/certificates.cpp#L341)). Pure refactor —
  no behavior change, so the existing gated `--dsn` tests are the regression net.
- [native/c/makefile](native/c/makefile): add a `$(OUT_DIR)/zkrio.o` rule next to
  `zkr.o` ([makefile:204-220](native/c/makefile#L204-L220)) and add that object to the
  `zowex` link line at [makefile:385](native/c/makefile#L385) — **not** to
  `libzkr.{so,a}`. `libzkr.so` binds today against nothing but the GSKCMS side deck;
  folding a `zds`-dependent object into it would break that standalone bind.
  Optionally add `$(OUT_DIR_SWIG)/zkr.o` and `$(OUT_DIR_SWIG)/zkrio.o` to
  `SWIG_EXTENDER_OBJS` ([makefile:92](native/c/makefile#L92)) for consistency — note
  those objects are not consumed by the Python build (see Step 5).

**Fallback if D8 is rejected:** duplicate both helpers inside `zkr_py.cpp` and add a
comment on each pointing at the original, plus a test asserting the created data set's
DSORG/RECFM/LRECL/BLKSIZE so drift is caught.

## Step 3 — The binding glue (`native/python/bindings/zkr_py.{hpp,cpp}`)

Model on [zusf_py.cpp](native/python/bindings/zusf_py.cpp) — it is the cleanest of the
three (it has the `set_encoding_opts`/`get_etag` helper pattern the other two are
missing).

### Conversion contract

`zkr.cpp` is compiled EBCDIC ([setup.py:25-41](native/python/bindings/setup.py#L25-L41))
and compares its inputs against EBCDIC literals — `usage == "PERSONAL"`
([zkr.cpp:304-306](native/c/zkr.cpp#L304-L306)), `format == "p12"`
([zkr.cpp:550](native/c/zkr.cpp#L550)), `status == "TRUST"`
([zkr.cpp:1113-1117](native/c/zkr.cpp#L1113-L1117)) — and `memcpy`s `owner`/`ring`
into R_datalib control blocks, which require EBCDIC
([zkr.cpp:61-78](native/c/zkr.cpp#L61-L78)).

| What crosses | Direction |
|---|---|
| `owner`, `keyring`, `label`, `new_label`, `usage`, `status`, `format`, `from_ring`, `password`, `file`, `dsn` | `a2e` on the way in |
| `ZKRCertInfo` / `ZKRCertDetail` / `ZKRRingEntry` / `ZKRRingCert` string fields | `e2a` on the way out |
| `diag.e_msg`, `diag.service`, `diag.warning` | `e2a` on the way out |
| Exported **PEM** bytes | `e2a` — but only for the value returned to Python (D11) |
| Exported **PKCS#12** bytes | never converted, in either direction |
| Imported PKCS#12 `data` | never converted |

Three ordering rules that are easy to get wrong:

1. **Filter before converting.** `list_certificates` must call `zkr_list_ring`, then
   `zkr_filter_certs_into` with the `a2e`'d `label`/`usage` (EBCDIC compared against
   EBCDIC), and only then `e2a` the survivors. Filtering after conversion would
   compare ASCII against EBCDIC and always miss.
2. **Write before converting.** `export_certificate_to_file` / `_to_dsn` must hand
   `zkrio_*` the raw bytes `zkr_export_cert` produced, and convert only if they also
   return them. In the recommended shape they return a byte count, so they never
   convert at all.
3. **Replicate, do not call, the `zkr_list_ring` cap logic.**
   [certificates.cpp:207-241](native/c/commands/certificates.cpp#L207-L241) passes
   `max_entries` to `zkr_list_ring` only when there is *no* filter, and applies it to
   matching rows otherwise. Same in the glue, or `--max-entries` semantics diverge.

### Error handling

```cpp
struct ZkrError : std::runtime_error
{
  int function_code, saf_rc, esm_rc, esm_rsn, gsk_rc;
  std::string service;
};

// One helper, used by every entry point:
void zkr_raise(const ZKR &ctx, int rc);   // throws ZkrError when rc != 0
```

`zkr_raise` copies `ctx.diag`, `e2a`s the three strings, and throws. The `.i` turns it
into a Python exception with attributes (Step 4).

**Guard against an empty message.** `zds_py.cpp` had exactly this bug — a
substring-constructed `diag` that reported failures with no text (see
`[[python-bindings-known-deferred-issues]]`). `ZKRDiag::e_msg` is a `std::string`, not
a `char[]`, so the substring hazard does not apply, but a fallback is still worth it:
if `e_msg` comes back empty, synthesize `"<service> failed (SAF <rc>/<esm>/<rsn>)"`.

### Cross-mode `std::string` members

`ZKR zkr{}` is value-initialized in the ASCII glue and its `diag.e_msg` is then
assigned by EBCDIC-compiled code. This is already a proven pattern in the repo —
`zjb_py.cpp` does the same with `ZJob`, which also holds `std::string` members
([zjb_py.cpp:78-81](native/python/bindings/zjb_py.cpp#L78-L81)) — but note the comment
there: use `ZKR zkr{}`, **never `= {0}`**, which builds a `std::string` from a null
pointer.

### Validation to replicate from the command layer

The glue is the only validation layer for Python callers, so port these (raising
`std::invalid_argument` → Python `ValueError`, added to the `.i`'s `%exception`):

- `label` required — export, import, delete, show, connect, set-default, trust, rename
- `usage` required on import; `password` required on import and on `format="p12"` export
- `keyring == "*"` rejected for `create_keyring`, `delete_keyring`,
  `import_certificate*`, `connect_certificate`, `set_default_certificate`
  ([certificates.cpp:66-79](native/c/commands/certificates.cpp#L66-L79))
- `delete_certificate`: `keyring == "*"` rejected in favor of `database=True`;
  `database=True` with a non-empty `keyring` rejected; neither rejected
- `connect_certificate`: same three checks for `from_ring` / `from_database`
- `max_entries < 0` rejected
- `list_rings` normalizes `keyring == "*"` to `""`
  ([certificates.cpp:645-647](native/c/commands/certificates.cpp#L645-L647))

## Step 4 — The SWIG interface (`native/python/bindings/zkr_py.i`)

Start from [zds_py.i](native/python/bindings/zds_py.i) — same `%module`, `%exception`,
`%include "std_string.i"` / `"std_vector.i"`, `%feature("docstring")` per function,
`%include "zkr_py.hpp"`, then the re-declared structs and `%template`s.

### 4a. Binary payloads — the highest-risk item

`%include "std_string.i"` maps `std::string` → Python `str` via
`SWIG_FromCharPtrAndSize`, which on Python 3 calls
`PyUnicode_DecodeUTF8(..., "surrogateescape")`. A PKCS#12 blob is not UTF-8; it would
come back as a `str` full of lone surrogates that only round-trips if the caller knows
to re-encode with `surrogateescape`. That is not an API anyone should ship.

Introduce a distinguished type in `zkr_py.hpp`:

```cpp
typedef std::string ZkrBytes;
```

and typemap it in `zkr_py.i`, **before** `%include "zkr_py.hpp"`:

```swig
%typemap(out) ZkrBytes {
  $result = PyBytes_FromStringAndSize($1.data(), static_cast<Py_ssize_t>($1.size()));
}

%typemap(in) const ZkrBytes & (std::string temp) {
  char *buf = nullptr;
  Py_ssize_t len = 0;
  if (PyBytes_AsStringAndSize($input, &buf, &len) != 0) {
    SWIG_exception_fail(SWIG_TypeError, "expected bytes for argument $argnum");
  }
  temp.assign(buf, static_cast<size_t>(len));
  $1 = &temp;
}

%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING) const ZkrBytes & {
  $1 = PyBytes_Check($input) ? 1 : 0;
}
```

Then declare `export_certificate` as returning `ZkrBytes` and `import_certificate` as
taking `const ZkrBytes &data`.

**Verify, do not assume, that the typedef wins the typemap match.** SWIG matches
typemaps on the type as written before reducing typedefs, so `%typemap(out) ZkrBytes`
should beat `std_string.i`'s `%typemap(out) std::string` — but if it does not, SWIG
falls through *silently* to the `str` typemap. Two guards:

- Grep the generated `zkr_py_wrap.cxx` for `PyBytes_FromStringAndSize` after the first
  `make swig-wrappers`.
- The Step 6 byte-exactness test (a p12 containing `0x00`) fails loudly if it did not.

Fallbacks, in order of preference, if the typedef does not match: a one-field wrapper
struct (`struct ZkrBytes { std::string v; };`) typemapped on the real type; or
`%apply` / an explicitly named `%typemap(out) std::string export_certificate`.

Do **not** reach for `SWIG_PYTHON_STRICT_BYTE_CHAR` — it would turn every `std::string`
in the module into `bytes`, including labels and error messages.

### 4b. `ZkrError` with SAF attributes (D6)

```swig
%{ static PyObject *g_zkr_error = nullptr; %}

%init %{
  g_zkr_error = PyErr_NewException("zkr_py.ZkrError", PyExc_RuntimeError, nullptr);
  Py_INCREF(g_zkr_error);
  PyModule_AddObject(m, "ZkrError", g_zkr_error);
%}

%exception {
  try { $action }
  catch (const ZkrError &e) {
    PyObject *exc = PyObject_CallFunction(g_zkr_error, "s", e.what());
    if (exc) {
      PyObject_SetAttrString(exc, "service",       PyUnicode_FromString(e.service.c_str()));
      PyObject_SetAttrString(exc, "function_code", PyLong_FromLong(e.function_code));
      PyObject_SetAttrString(exc, "saf_rc",        PyLong_FromLong(e.saf_rc));
      PyObject_SetAttrString(exc, "esm_rc",        PyLong_FromLong(e.esm_rc));
      PyObject_SetAttrString(exc, "esm_rsn",       PyLong_FromLong(e.esm_rsn));
      PyObject_SetAttrString(exc, "gsk_rc",        PyLong_FromLong(e.gsk_rc));
      PyErr_SetObject(g_zkr_error, exc);
      Py_DECREF(exc);
    }
    SWIG_fail;
  }
  catch (const std::invalid_argument &e) { SWIG_exception(SWIG_ValueError,   e.what()); }
  catch (const std::exception &e)        { SWIG_exception(SWIG_RuntimeError, e.what()); }
  catch (...)                            { SWIG_exception(SWIG_RuntimeError, "Unknown exception"); }
}
```

`ZkrError` derives from `RuntimeError`, so `except RuntimeError:` in existing-style
code still catches it. `%init`'s module variable is `m` in SWIG 4.x Python — confirm
against the generated wrapper; older templates use `d` (the module dict) instead.

**If this proves fiddly, fall back to message-only** (append
`" (SAF <fc>/<rc>/<esm>/<rsn>)"` to `e_msg` in `zkr_raise` and keep the plain
`%exception` from `zds_py.i`) and file the structured version as a follow-up. Keep 4b
as its own commit so it can be dropped without touching anything else.

### 4c. Struct and template declarations

Mirror [zds_py.i:34-67](native/python/bindings/zds_py.i#L34-L67):

```swig
%template(ZKRCertInfoVector) std::vector<ZKRCertInfo>;
%template(ZKRRingCertVector) std::vector<ZKRRingCert>;
%template(ZKRRingEntryVector) std::vector<ZKRRingEntry>;

struct ZKRCertInfo { std::string label; std::string owner; std::string usage;
                     std::string status; bool is_default; };
struct ZKRCertDetail { /* all 13 fields */ };
struct ZKRRingCert { std::string owner; std::string label; };
struct ZKRRingEntry { std::string owner; std::string name;
                      std::vector<ZKRRingCert> certs; };
```

Note `zds_py.i` and `zjb_py.i` both put `%template` *before* the `struct`
declarations and SWIG accepts it. Mirror the existing order rather than inventing a
new one, but **check the generated `zkr_py.py` actually exposes the members** — if
SWIG treated a struct as an opaque pointer the failure mode is a missing attribute at
runtime, not a build error. The Step 6 tests assert on every field for exactly this
reason.

## Step 5 — Build wiring

### `native/python/bindings/setup.py`

```python
zkr_py_module = Extension("_zkr_py",
                          sources=["zkr_py_wrap.cxx", "zkr_py.cpp",
                                   f"{C_PATH}/zkr.cpp", f"{C_PATH}/zkrio.cpp",
                                   f"{C_PATH}/zds.cpp", f"{C_PATH}/zut.cpp"],
                          language="c++",
                          extra_objects=[
                              f"{build_out_path}/zdsm.o",
                              f"{build_out_path}/zutm.o",
                              f"{build_out_path}/zam.o",
                              f"{build_out_path}/zam24.o",
                              f"{build_out_path}/zutm31.o",
                              f"{build_out_path}/zutcall24.o",
                              "/usr/lib/GSKCMS64.x",   # System SSL GSKCMS 64-bit side deck
                          ],
                          include_dirs=[chdsect, ztype],
                          extra_compile_args=["-D_EXT", "-D_OPEN_SYS_FILE_EXT=1"],
                          )
```

- `BuildExtMixedCharMode` ([setup.py:25-41](native/python/bindings/setup.py#L25-L41))
  keys on `os.path.abspath(src).startswith(ztype + os.sep)`, so all four `native/c`
  sources get `-fzos-le-char-mode=ebcdic` with no change needed. `zkr_py.cpp` and the
  wrapper stay ASCII.
- `zds.cpp` + `zut.cpp` + the six Metal C objects are there for `zkrio`; without
  Step 2's DSN/file support, `_zkr_py` needs only `zkr.cpp` and the side deck.
- **Do not add `libraries=["zut"]`.** The `zusf` extension both compiles `zut.cpp`
  with `-DSWIG` and links the non-`SWIG` `libzut.so`
  ([setup.py:43-51](native/python/bindings/setup.py#L43-L51)) — two copies of the same
  symbols with different linkage. Do not propagate that.
- Add `'zkr'` to the default set in `get_modules_to_build`
  ([setup.py:99](native/python/bindings/setup.py#L99)) and the
  `ext_modules`/`py_modules` blocks.
- `IRRSDL64` needs no side deck: `libzkr.so` is bound today with `-shared` and nothing
  but GSKCMS ([makefile:210-212](native/c/makefile#L210-L212)), and `_zkr_py.so` is
  bound the same way. If that turns out to be wrong, it surfaces as an `IEW2456E` for
  `IRRSDL64` at the extension link — immediate and unambiguous.

### `native/python/bindings/Makefile`

Add `$(SWIG) -python -c++ zkr_py.i` to `swig-wrappers`
([Makefile:32-36](native/python/bindings/Makefile#L32-L36)), a `build-zkr` target
alongside `build-zds`, and the `help` line.

### `native/python/bindings/__init__.py`

```python
from . import zkr_py
__all__ = ['zusf_py', 'zds_py', 'zjb_py', 'zkr_py']
```

### `package_zbind.py` (source distribution)

- `bindings_files` ([package_zbind.py:24-29](native/python/bindings/package_zbind.py#L24-L29)):
  add `zkr_py.cpp`, `zkr_py.hpp`, `zkr_py.py`, `zkr_py_wrap.cxx`.
- `core_sources` ([:63](native/python/bindings/package_zbind.py#L63)): add `zkr.cpp`,
  `zkrio.cpp`. (Headers are already copied wholesale by the loop at
  [:73-77](native/python/bindings/package_zbind.py#L73-L77), so `zkr.hpp`, `zkrtype.h`
  and `zkrio.hpp` come along for free.)
- `object_files` ([:88](native/python/bindings/package_zbind.py#L88)): add
  `zutcall24.o` — **it is missing today**, and the bundled `setup.py`'s `zds`/`zjb`
  extensions omit it from `extra_objects` too, unlike the in-tree `setup.py`. Worth a
  separate look; if the bundle builds without it today, add it only for `_zkr_py`.
- The embedded `setup_py_content` ([:100-186](native/python/bindings/package_zbind.py#L100-L186)):
  add `"zkr.cpp", "zkrio.cpp"` to `CORE_SOURCES`, add the `_zkr_py` extension with
  `/usr/lib/GSKCMS64.x` in `extra_objects`, and register it in `ext_modules` /
  `py_modules`.
- The embedded `readme_content` ([:201-224](native/python/bindings/package_zbind.py#L201-L224)):
  note the new target prerequisite — System SSL (`/usr/lib/GSKCMS64.x`) must be
  installed. It is a base z/OS element, so this is a documentation point, not a
  blocker.

### `package_precompiled.py` (binary distribution)

- `py_files` ([package_precompiled.py:39](native/python/bindings/package_precompiled.py#L39)):
  add `zkr_py.py`. The `_*.so` glob at
  [:41-46](native/python/bindings/package_precompiled.py#L41-L46) picks up
  `_zkr_py*.so` automatically.
- `init_content` ([:60-63](native/python/bindings/package_precompiled.py#L60-L63)): add
  `from .zkr_py import *`.
- Its README block ([:69-100](native/python/bindings/package_precompiled.py#L69-L100)):
  add a `zkr_py` import example.

## Step 6 — Tests

### `native/python/bindings/test/test_zkr.py`

Mirror the three-tier gating from
[zkr.test.cpp:12-21](native/c/test/zkr.test.cpp#L12-L21) so the suite passes for any
user, using `pytest.mark.skipif` / `pytest.skip` where the C++ suite uses `itif`.

**Tier A — no authority.** Deterministic; these are the ones that will actually run in
CI.

- module imports and exposes all 18 callables plus `ZkrError`
- every validation rule from Step 3 raises `ValueError` with the expected substring —
  the direct analogue of the C++ "CLI input validation" block
  ([zkr.test.cpp:149-238](native/c/test/zkr.test.cpp#L149-L238))
- `import_certificate(..., data="not bytes")` raises `TypeError` (proves the `in`
  typemap is wired, not the `str` fallback)
- `list_certificates` against a nonexistent ring raises `ZkrError` with a non-empty
  message, `saf_rc >= 8`, and integer `esm_rc`/`esm_rsn`/`function_code` — proves both
  the diagnostic conversion (no EBCDIC leakage, no empty message) and 4b
- `show_certificate`/`list_rings` field access on the returned proxies (guards against
  the silent opaque-pointer failure from 4c)

**Tier B — `can_mutate` probe.** `create_keyring` on a scratch ring succeeds; on
failure record the reason and skip the rest, exactly as
[zkr.test.cpp:395-424](native/c/test/zkr.test.cpp#L395-L424) does. Then: ring
lifecycle (create → `list_rings` → `list_certificates` empty → `count_ring` == 0 →
delete), and delete-nonexistent produces a clean `ZkrError`.

**Tier C — authority + PKCS#12 fixture.** Fixture from `ZKR_TEST_P12` /
`ZKR_TEST_P12_PASS` (accept the `ZNP_`-prefixed form too — `buildTools.ts` forwards
local `ZNP_*` vars verbatim, see [zkr.test.cpp:76-83](native/c/test/zkr.test.cpp#L76-L83)),
otherwise self-provision via RACDCERT the way
[zkr_generate_p12_fixture](native/c/test/zkr.test.cpp#L86-L130) does. Two notes for the
Python port:

- `tsocmd` output is EBCDIC. Use `subprocess.run(..., capture_output=True)` and
  `.decode("cp1047")`, not `text=True`.
- The `cp -B "//'DSN'"` hop is no longer needed — `import_certificate_from_dsn` reads
  the exported data set directly. Drop it, exactly as the initial plan does for the
  C++ fixture.

Then the lifecycle, and the assertions that are new coverage for this work:

1. **Byte-exactness.** `data = export_certificate(..., format="p12", password=...)`
   → `isinstance(data, bytes)`, `data[0] == 0x30` (DER `SEQUENCE`), `b"\x00" in data`
   (a UTF-8 or text-mode path could not produce this), `len(data) > 1000`.
2. **`bytes` round trip.** `import_certificate(..., data=data)` succeeds; the returned
   warning is either `""` or mentions the already-exists case.
3. **DSN parity.** `n = export_certificate_to_dsn(..., dsn=scratch)` where
   `n == len(data)`, then `import_certificate_from_dsn(..., dsn=scratch)` succeeds.
   Cover a **sequential** data set *and* a **PDS/E member** (`LIB(CERT01)`) — the BPAM
   path. Register both in the teardown.
4. **PEM is ASCII.** `export_certificate(..., format="pem")` starts with
   `b"-----BEGIN CERTIFICATE-----"` and ends with `b"-----END CERTIFICATE-----\n"`.
   This is the assertion that catches a missing `e2a` on the PEM path (D11).
5. **PEM on disk stays EBCDIC.** `export_certificate_to_file(..., format="pem")`, then
   read the file back with `open(path, "rb")` and assert it does **not** start with the
   ASCII banner — locking in the deliberate keyring-util parity so nobody "fixes" it
   later. Also assert `oct(os.stat(path).st_mode & 0o777) == "0o600"`.
6. **Filter parity.** `list_certificates(..., label=<exact>)` finds it;
   `label=<lowercased>` does not (RACDCERT LABEL is case-sensitive) — proves the
   filter ran on EBCDIC, before conversion.
7. `show_certificate` → `subject`, `serial_number`, `not_before`/`not_after` are
   readable ASCII; `set_default_certificate` → `is_default` flips in
   `list_certificates`; `trust_certificate` NOTRUST↔TRUST; `rename_certificate` and
   back; `connect_certificate` from a second ring; `delete_certificate(database=True)`.

Teardown must be registered up front (rings, labels, DSNs) so a mid-flow failure still
cleans up — same shape as
[zkr.test.cpp:598-620](native/c/test/zkr.test.cpp#L598-L620).

### `native/python/bindings/test/Makefile`

Add `TEST_ZKR`, `JUNIT_ZKR`, a `test-zkr` target, the `$(MAKE) test-zkr` line in
`test`, `$(JUNIT_ZKR)` in `merge-results` and `clean`, and the `help` entry
([Makefile:1-81](native/python/bindings/test/Makefile#L1-L81)). The `%_utf8.py` iconv
pattern rule picks the new file up automatically.

### Fixtures and CI

- [test/fixtures/env.example.yml](native/python/bindings/test/fixtures/env.example.yml)
  and `env.yml`: add `KEYRING_PREFIX` (scratch ring name prefix) and document the
  optional `ZKR_TEST_P12` / `ZKR_TEST_P12_PASS` environment variables.
- [.github/workflows/zos-py-build.yml](.github/workflows/zos-py-build.yml): add
  `KEYRING_PREFIX` to the generated `env.yml`
  ([:67-71](.github/workflows/zos-py-build.yml#L67-L71)). Expect Tiers B and C to skip
  — the CI user has no certificate authority, the same state
  [doc/certificates-test-plan.md](doc/certificates-test-plan.md#L21-L26) records for
  the C++ suite. Item 1 of that plan (granting the CI user authority) unlocks both
  suites at once; this plan does not depend on it.
- **The workflow's `paths:` filter is `native/python/**`**
  ([:5-7](.github/workflows/zos-py-build.yml#L5-L7)), which will not fire on Steps 1
  and 2. Add `native/c/zkr.hpp`, `native/c/zkr.cpp`, `native/c/zkrio.hpp`,
  `native/c/zkrio.cpp` to the list rather than the whole of `native/c/**` (the job
  shares the `zos-build` concurrency group and takes ~20 minutes).

### Existing suites

`npm run z:test -- certificates` and `npm run z:test -- ds` must still pass unchanged —
Steps 1 and 2 are meant to be behavior-neutral for the CLI. That is the regression net
for the `certificates.cpp` refactor.

## Step 7 — Docs and changelogs

- [native/python/bindings/README.md](native/python/bindings/README.md):
  - "Available Functions" gains a `### zkr_py — Certificates and key rings` table
    (18 rows) plus supporting types, matching the existing format
    ([README.md:7-66](native/python/bindings/README.md#L7-L66)).
  - "Architecture": the build-pipeline diagram
    ([:81-103](native/python/bindings/README.md#L81-L103)) needs a note that `zkr` adds
    a side deck and no Metal C of its own; the "Why calls across the line need
    `extern C`" section ([:180-216](native/python/bindings/README.md#L180-L216)) should
    cite `zkr.hpp` alongside the other three and use `zkr_filter_certs` as the live
    example of the "cannot return a C++ type" limit (today it cites
    `zusf_format_file_entry`, which nothing calls).
  - "Which strings get converted" ([:217-238](native/python/bindings/README.md#L217-L238)):
    add a `zkr_py` column, and a short subsection on `bytes` — the one payload that
    crosses the line with **no** conversion in either direction, and the EBCDIC-PEM-on-disk
    rule (D11).
  - Both distribution sections: System SSL is a new target prerequisite.
- [doc/certificates-test-plan.md](doc/certificates-test-plan.md): add the Python
  bindings to the tier table in §1 and a row per method in the §2 coverage matrix.
  Explicitly distinguish this from "Item 4 — RPC-layer tests"
  ([:126-138](doc/certificates-test-plan.md#L126-L138)), which also proposes a Python
  test but drives `zowex server` over JSON-RPC out-of-process; `zkr_py` is in-process
  and bypasses the RPC layer entirely. They are complementary, not substitutes.
- [doc/apis.md](doc/apis.md): there is **no Certificates section at all** today. Add
  one with the 14 operations and the `Backend / SDK / CLI / Python` columns filled in.
- [native/CHANGELOG.md](native/CHANGELOG.md) under `## Recent Changes`, matching the
  existing `- <sentence>. [#NNNN](...)` format:
  - a `python:` entry for the `zkr_py` module
  - a `python:` entry for binary-safe `bytes` on export/import
  - a `c:` entry for `zkrio` (shared certificate-material I/O) if Step 2 lands
- No `packages/*/CHANGELOG.md` entries — the TypeScript surface does not change.

## Step 8 — (optional) Flask app route

[native/python/app](native/python/app) is a demo APIML-onboarded Flask service with one
blueprint per binding module. A `routes/zkr.py` would complete the pattern, and would
need `pythonSwagger.json` and `app/test/test_zkr.py` updates too.

**Recommendation: defer.** The app is a sample, not a shipped surface, and exposing
certificate *mutation* and PKCS#12 *export* over HTTP is a security design question
(who authenticates, where does the passphrase go, is the response body a private key)
that deserves its own review rather than a footnote in a bindings plan. If it is
wanted, the defensible minimum is read-only: `listRings`, `listCertificates`,
`countRing`, `showCertificate`.

---

## Verification

Order matters. Step 1 and 2 touch headers, and
`[[python-bindings-known-deferred-issues]]` §4 records that header changes do **not**
trigger rebuilds of dependent objects — `npm run z:rebuild` will not save you.

```bash
npm run z:make -- clean          # mandatory: header-only changes do not rebuild dependents
npm run z:upload
npm run z:build                  # zowex + libzkr + the Metal C objects
npm run z:python:build           # make swig-wrappers && python setup.py build_ext
```

**1. The wrapper actually emits `bytes`.** Before running anything:

```bash
grep -c PyBytes_FromStringAndSize native/python/bindings/zkr_py_wrap.cxx   # must be >= 1
grep -c PyBytes_AsStringAndSize   native/python/bindings/zkr_py_wrap.cxx   # must be >= 1
```

Zero means the `ZkrBytes` typedef did not win the typemap match — go to the 4a
fallbacks before going further.

**2. The module loads.** This is the cheap `IRRSDL64` / `GSKCMS` link check; a missing
symbol fails at import, not at first call:

```bash
cd native/python/bindings && python -c "import zkr_py; print(zkr_py.refresh_digtcert.__doc__)"
```

**3. Diagnostics cross the line intact** — no authority needed:

```python
import zkr_py as kr
try:
    kr.list_certificates("FERNANDO", "NO.SUCH.RING.ZKRPY")
except kr.ZkrError as e:
    print(repr(str(e)), e.service, e.saf_rc, e.esm_rc, e.esm_rsn)
```
Expect readable ASCII text (not mojibake, not empty) and `saf_rc >= 8`.

**4. Round trip against a real certificate** (needs authority — probe with
`kr.create_keyring("FERNANDO", "ZKRPY.RING1")` first; if it raises, use an existing
ring):

```python
data = kr.export_certificate("FERNANDO", "*", "Certificate for FERNANDO",
                             format="p12", password="changeit")
assert isinstance(data, bytes) and data[0] == 0x30 and b"\x00" in data
open("/tmp/zkrpy.p12", "wb").write(data)
n = kr.export_certificate_to_dsn("FERNANDO", "*", "Certificate for FERNANDO",
                                 "FERNANDO.ZKRPY.PARITY.P12",
                                 format="p12", password="changeit")
assert n == len(data)
```

Then, off-platform, confirm `openssl pkcs12 -in zkrpy.p12 -passin pass:changeit -info
-noout` reports 2 cert bags + 1 shrouded key bag, as in
[doc/testbed-client-cert-auth--with-cli.md:132](doc/testbed-client-cert-auth--with-cli.md#L132).

**Do not diff two independent exports** — PKCS#12 encryption is salted, so two
`gsk_export_key` calls on the same certificate differ. Compare a single export's bytes
against its own DSN/file copy, or against `openssl` parsing.

**5. Data-set shape**, read-only via MCP:
`mcp__zowe__getDatasetAttributes { dsn: "FERNANDO.ZKRPY.PARITY.P12" }` → expect
`PS / VB / lrecl 84 / blksz 27998`. Then repeat with a member target
(`FERNANDO.ZKRPY.P12LIB(CERT01)`) and confirm via `mcp__zowe__listMembers` that a
second export to the same member succeeds rather than hanging on a stale ENQ.

**6. PEM both ways:**
```python
pem = kr.export_certificate("FERNANDO", "*", "Certificate for FERNANDO")
assert pem.startswith(b"-----BEGIN CERTIFICATE-----")          # returned bytes: ASCII
kr.export_certificate_to_file("FERNANDO", "*", "Certificate for FERNANDO", "/tmp/zkrpy.pem")
assert not open("/tmp/zkrpy.pem", "rb").read().startswith(b"-----BEGIN")  # on disk: EBCDIC
```
And `mcp__zowe__readDataset` on a `--dsn` PEM export should render as readable
`-----BEGIN CERTIFICATE-----` text (MCP's text-mode read is correct for PEM).

**7. No secret leakage.** Trigger a bad-password import and confirm the passphrase
appears nowhere in `str(e)`. `zkr.cpp` only ever puts the *label* in a GSK diagnostic
([zkr.cpp:595-600](native/c/zkr.cpp#L595-L600)), so this should hold — verify, and add
a test.

**8. Suites:**
```bash
npm run z:python:test                 # includes the new zkr suite
npm run z:test -- certificates        # Step 1/2 must be behavior-neutral
npm run z:test -- ds
npm run lint:cpp                      # the glob covers native/python/bindings/*.{cpp,hpp}
```
Reminder from memory: unset `_BPXK_JOBLOG=STDERR` before `npm run z:test` on lpar.1 —
it fakes ~12 jobs-server RPC failures (`[[joblog-breaks-native-tests]]`).

**9. Cleanup:** `mcp__zowe__deleteDataset` on every `FERNANDO.ZKRPY.**` scratch data
set, `kr.delete_certificate("FERNANDO", "", "<label>", database=True)`,
`kr.delete_keyring("FERNANDO", "ZKRPY.RING1")`, and `rm /tmp/zkrpy.*`.

---

## Risks

| # | Risk | Detection | Mitigation |
|---|---|---|---|
| R1 | `ZkrBytes` typemap silently loses to `std_string.i` and p12 comes back as `str` | Verification step 1 (grep the wrapper); Step 6 test 1 | 4a fallbacks: wrapper struct, or a named `%typemap(out)` |
| R2 | Unresolved symbols at extension link (`IEW2456E`) | The extension link fails, or `import zkr_py` fails | Step 1's `extern "C"` block must cover **every** function the glue calls; the GSKCMS side deck must be in `extra_objects` |
| R3 | `gskcms.h` will not compile under Python's `sysconfig` CFLAGS | `zkr.cpp` compile error in `z:python:build` | `zkr.cpp` sets `_XOPEN_SOURCE_EXTENDED` itself ([zkr.cpp:12](native/c/zkr.cpp#L12)); if that is not enough add `-D_ALL_SOURCE` to `extra_compile_args` for this extension only |
| R4 | `-Wreturn-type-c-linkage` on a C-linkage function returning a C++ type | Build warning | D9 avoids it entirely by adding an out-param sibling instead |
| R5 | Stale objects after the header changes in Steps 1–2 | Symptoms that make no sense (old behavior, phantom link errors) | `npm run z:make -- clean` first, every time (`[[python-bindings-known-deferred-issues]]` §4) |
| R6 | The `certificates.cpp` refactor changes CLI behavior | `npm run z:test -- certificates` | Pure move; keep the diff mechanical and reviewable |
| R7 | Tiers B/C skip everywhere, so the interesting paths get no CI coverage | Skip counts in `pybi_results.xml` | Same condition the C++ suite is already in; Tier A is deliberately broad so the ASCII/EBCDIC and `bytes` boundaries are covered without authority |
| R8 | Cross-mode `std::string` members in `ZKR`/`ZKRCertInfo` | Garbage strings or a crash on first error | Already proven in-repo (`ZJob` in `zjb_py.cpp`); use `ZKR zkr{}`, never `= {0}` |

---

## Suggested commit order

Each commit builds and its tests pass on its own.

1. `feat(native): add C linkage and an out-param filter to zkr for the bindings` —
   `native/c/zkr.hpp`, `native/c/zkr.cpp` (Step 1)
2. `refactor(native): extract certificate material I/O into zkrio` —
   `native/c/zkrio.{hpp,cpp}`, `native/c/commands/certificates.cpp`,
   `native/c/makefile` (Step 2)
3. `feat(python): add zkr_py certificate and key ring bindings` —
   `native/python/bindings/zkr_py.{hpp,cpp,i}`, `__init__.py` (Steps 3–4a, 4c)
4. `feat(python): expose SAF diagnostics on certificate binding errors` —
   `zkr_py.i`, `zkr_py.{hpp,cpp}` (Step 4b; droppable on its own)
5. `build(python): wire the zkr_py extension into both distributions` — `setup.py`,
   `Makefile`, `package_zbind.py`, `package_precompiled.py` (Step 5)
6. `test(python): add zkr_py coverage` — `test/test_zkr.py`, `test/Makefile`,
   `test/fixtures/env*.yml`, `.github/workflows/zos-py-build.yml` (Step 6)
7. `docs: document the zkr_py certificate bindings` — the four docs and the changelog
   (Step 7)

Commits 3 and 5 are mutually dependent for a *working* build (3 adds files 5 compiles);
if strict bisectability matters more than reviewability, squash them.

---

## Out of scope

- **The Zowe MCP server** lives in a different repo. It is a JSON-RPC consumer and gains
  nothing from `zkr_py`; the `dsn`-parameter follow-up noted in
  [initial-plan.md:633-640](initial-plan.md#L633-L640) still stands there.
- **The `zds_py` codepage / etag defects** (`[[python-bindings-known-deferred-issues]]`
  §2): explicit codepages return empty and etags are converted asymmetrically. `zkr_py`
  is designed to avoid the same trap — it never passes a codepage to `zds` and never
  converts a `bytes` payload — but fixing `zds_py` is a separate change with its own
  tests. Worth doing next; it is the reason the `bytes` typemaps in Step 4 must not be
  hand-waved into "just use `str`".
- **Certificate creation.** `zkr` has no `RACDCERT GENCERT` equivalent, so generating a
  new certificate still needs TSO. `zkr_py` inherits that limit; the test fixture works
  around it with `tsocmd`.
- **Releasing the GIL** around R_datalib/GSK calls (D12).
- **Streaming export/import.** `zusf_py` has `*_streamed` variants for large files;
  certificates are kilobytes, so there is no case for it.
- **The Flask demo app** (Step 8) — deferred, with a security rationale.
