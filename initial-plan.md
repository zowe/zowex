# Add data-set (DSN) support to `system cert import` / `export`

## Context

`zowex system cert import` and `zowex system cert export` can only read/write **USS
files** today. On z/OS the native idiom for certificate material is a *data set* —
`RACDCERT ID(x) EXPORT(...) DSN('MY.CERT.P12') FORMAT(PKCS12DER)` writes straight
to one, and that is how certificates arrive from other systems and from RACF admins.
Today a user has to bounce the file through USS with `cp -B "//'DSN'" /tmp/x.p12`
before `zowex` can touch it. That hop is real friction:

- The repo's own test fixture does exactly this dance —
  [zkr.test.cpp:86-130](native/c/test/zkr.test.cpp#L86-L130) generates a cert,
  `RACDCERT EXPORT ... DSN(...)`, then `cp -B` into `/tmp`, then `DELETE` the DSN.
- The Zowe MCP server has **no binary-safe download**: its `downloadUssFileToFile` /
  `downloadDatasetToFile` are text-mode and silently corrupt a PKCS#12 blob
  (2683 bytes → 3621 bytes, `openssl` then rejects it). Documented in
  [testbed-client-cert-auth--with-mcp.md:324-346](doc/testbed-client-cert-auth--with-mcp.md#L324-L346).

**Outcome:** `--dsn` on both commands, plumbed through every layer — native arg
parser → `certificates.cpp` → `zkr.hpp` → generated RPC schemas → SDK RPC types →
`Cert.definition.ts` + handlers. Byte-exact for binary PKCS#12; EBCDIC text records
for PEM. Member writes go through the existing **BPAM** access-method layer.

### Decisions locked in (confirmed with the user)

| Decision | Choice |
|---|---|
| Surface | A **new `--dsn` option**, mutually exclusive with `--file`. No `//'DSN'` overload, no auto-detection. |
| DSN kinds | **Sequential data sets *and* PDS/E members** (`MY.CERTS.LIB(CERT01)`). |
| Formats | **Both `pem` and `p12`** on export. Import stays p12-only (all `zkr_import_cert` accepts). |
| Write path | **BPAM (`zds_*_output_bpam`) for members**; sequential falls back to the existing `zds_write` binary path — see step 1 for why BPAM cannot cover sequential. |

Assumed default, flag if you disagree: **the target data set is created when it does
not exist**, with RACDCERT-parity attributes (`PS`/`VB`/`LRECL=84`/`BLKSIZE=27998`)
— matching what `RACDCERT ... FORMAT(PKCS12DER) DSN(...)` produces, verified live on
`FERNANDO.CLIENTCR.FERNANDO.P12`. An existing data set is written in place and its
attributes are left alone.

---

## Environment (verified live via MCP this session)

- `lpar.1`, user `fernando` / RACF `FERNANDO`; MVS encoding `IBM-037`, USS `IBM-1047`.
- Cert `Certificate for FERNANDO` — owner `FERNANDO`, `PERSONAL`, `TRUST`, RSA 2048,
  valid 2026-08-28 → 2027-08-29. `keyring: "*"` (virtual ring) works for export, so
  no real ring is needed on the export side.
- Real PKCS#12-in-a-data-set fixtures, all `PS / VB / LRECL 84 / BLKSIZE 27998`:
  `FERNANDO.CLIENTCR.FERNANDO.P12`, `FERNANDO.PUBLIC.CERT`,
  `FERNANDO.PUBLIC.CERT1.PKCS12`, `FERNANDO.PUBLIC.CERT2.PKCS12`.
- **Authority caveat:** `refreshCertificateClass` fails for FERNANDO (`SAF 8 / ESM 8
  / rsn 92` — plain authorization failure), and FERNANDO owns no key rings. Probe
  `zowex system keyring create` before relying on the import path; if it fails, run
  import verification as an authorized user or against a pre-existing ring.
- **`SYS1.PASSWORD` does not exist on lpar.1** — legacy MVS data-set password
  protection is not active there. Relevant to step 5.

---

## Step 1 — Access-method choice (read this before writing any I/O code)

A PKCS#12 blob is **binary**: byte-exact in, byte-exact out, no code-page
conversion, no newline interpretation. Findings from reading the existing layers,
all of which constrain the design:

**BPAM (`zds_open_output_bpam` / `zds_write_output_bpam` / `zds_close_output_bpam`,
[zds.cpp:1988-2085](native/c/zds.cpp#L1988-L2085)) is the right writer for members,
and is binary-safe.** `zds_write_output_bpam` → `ZDSWBPAM` → `write_output_bpam`
writes **one record of exactly `length` raw bytes** — no iconv, no line splitting
([zam.c:676-706](native/c/zam.c#L676-L706)). It also gives us three things `fopen`
does not:
- an ENQ on `SPFEDIT` plus a device RESERVE ([zam.c:97-117, 158](native/c/zam.c#L97-L117)),
  so a concurrent ISPF edit can't interleave;
- `STOW` of the member directory entry on close ([zam.c:913](native/c/zam.c#L913));
- coverage by the **DCB abend exit** (`ZAMDEXIT`, `gioc`, see
  [doc/dcb-abend-exit.md](doc/dcb-abend-exit.md)), so an OPEN/WRITE abend becomes
  `ZDS_RTNCD_DCB_ABEND_ERROR` instead of killing the long-running `zowex server`
  process. This matters directly for step 5.

**Three caller obligations BPAM imposes — all silent-corruption risks:**

1. **BPAM writes members only.** `open_output_bpam` hard-codes
   `dcb.dcbdsrg1 = dcbdsgpo` (DSORG=PO,
   [zam.c:503](native/c/zam.c#L503)), and `validate_jfcb_attributes` rejects a
   non-PDS (`jfcbind1 != jfcpds`) **and** requires a non-blank member name
   (`jfcbelnm[0] == ' '` → fail, [zam.c:75-95](native/c/zam.c#L75-L95)). There is
   **no BSAM sequential writer anywhere in `zam.c`** — see the full entry-point list;
   sequential output in this repo is always `fopen`.
   → **Sequential targets must use `zds_write`'s existing binary branch**
   (`fopen("wb")`, [zds.cpp:1500-1502](native/c/zds.cpp#L1500-L1502)), which is
   already-shipped, already-tested code, not new `fopen` plumbing.
2. **`write_output_bpam` silently truncates.** For V-format it caps `length` at
   `lrecl - sizeof(RDW)` with no error ([zam.c:685-693](native/c/zam.c#L685-L693)).
   → The caller **must** chunk the blob into records of at most `lrecl - 4` bytes.
   Getting this wrong loses data with a zero return code.
3. **Fixed-format pads with EBCDIC blanks.** `handle_fixed_record` does
   `memset(free_location, ' ', lrecl)` before the copy
   ([zam.c:543](native/c/zam.c#L543)), so a short final record comes back padded and
   the byte count changes.
   → **`RECFM=FB` is unusable for binary; VB is mandatory.** This is why the
   creation attributes below specify `V,B` — and it matches what RACDCERT itself
   produces.

**Reading needs nothing new.** `zds_read` with
`encoding_opts.data_type == eDataTypeBinary` opens `"rb"` and concatenates chunks
with no conversion ([zds.cpp:871-874, 930-939](native/c/zds.cpp#L871-L874)), and its
`zds_resolve_dsname` yields `//'DSN(MEM)'`, so **the same call covers sequential
data sets and members**. Same semantics as `cp -B`, which the existing test fixture
already depends on successfully.

### The one new shared function

Put the write logic in `zds`, not in `certificates.cpp` — it is generic data-set
I/O, it is unit-testable in `zds.test.cpp`, and it gives the repo a correct binary
data-set writer that `data-set write --encoding binary` can adopt later.

`native/c/zds.hpp`, declared next to `zds_write` ([zds.hpp:179](native/c/zds.hpp#L179)):

```cpp
/**
 * @brief Write raw bytes to a data set with no code-page conversion or line
 *        splitting. Members go through BPAM (ENQ/RESERVE/STOW + DCB abend exit);
 *        sequential data sets use the binary fopen path, since BPAM output is
 *        PDS-only. The target must be V-format: fixed-format records are blank
 *        padded and would change the byte count.
 * @param opts write options; opts.dsname may name a member, e.g. MY.LIB(MEM)
 * @param data raw bytes to write
 * @return RTNCD_SUCCESS on success; RTNCD_FAILURE otherwise (details in zds->diag)
 */
int zds_write_binary(const ZDSWriteOpts &opts, const std::string &data);
```

`native/c/zds.cpp` implementation outline:

1. `validate_dataset_exists(dsn, zds->diag)` — reuse
   [zds.cpp:1777-1785](native/c/zds.cpp#L1777-L1785).
2. `const DscbAttributes attrs = zds_get_dscb_attributes(dsn);` — reuse
   [zds.cpp:661-682](native/c/zds.cpp#L661-L682). **Reject** `recfm` without `V`
   with `ZDS_RTNCD_UNSUPPORTED_RECFM` and a message naming the padding problem;
   this is the guard that makes obligation 3 above impossible to trip.
3. `const size_t chunk = attrs.lrecl - 4;` (obligation 2). Guard `attrs.lrecl > 4`.
4. Member (`zds_has_member(dsn)`, [zds.cpp:1463](native/c/zds.cpp#L1463)) → BPAM:
   ```cpp
   IO_CTRL *ioc = nullptr;
   rc = zds_open_output_bpam(zds, dsn, ioc);
   for (size_t off = 0; off < data.size(); off += chunk) {
     std::string rec = data.substr(off, chunk);
     rc = zds_write_output_bpam(zds, ioc, rec);   // asa_char defaults to '\0'
     if (rc != RTNCD_SUCCESS) break;
   }
   const int close_rc = zds_close_output_bpam(zds, ioc);   // always close
   memset(zds->ddname, 0, sizeof(zds->ddname));            // DD was freed
   ```
   Close unconditionally even after a write failure, mirroring
   `zds_write_member_bpam` ([zds.cpp:1589, 1606](native/c/zds.cpp#L1589)), or the
   ENQ/RESERVE leaks. Note `zds_write_output_bpam` takes `std::string &` (non-const),
   so pass an lvalue.
5. Sequential → `zds_write_sequential(zds, zds_resolve_write_target(opts), data, attrs)`
   with `zut_prepare_encoding("binary", &zds->encoding_opts)` already applied by the
   caller (or set it here).

**`zds_write` itself is left untouched.** Calling BPAM directly means we do *not*
have to change how the shared `zds_write` routes member writes, so no existing
caller's behavior shifts. (Separately worth an issue, not this change:
`zds_write`'s member path is line-oriented — it splits on the newline character at
[zds.cpp:1619-1621](native/c/zds.cpp#L1619-L1621) — so `data-set write --encoding
binary` into a member is silently corrupting today. `zds_write_binary` is the fix
when someone picks that up.)

## Step 2 — `native/c/zkr.hpp` / `zkr.cpp`: let the caller supply the bytes

`zkr_export_cert` **already** returns the exported bytes via
`std::string &data` ([zkr.hpp:213](native/c/zkr.hpp#L213)) and does no file I/O —
all writing happens in `certificates.cpp`. So **export needs no `zkr` change**.

Import is asymmetric: `zkr_import_cert` reads the file itself via `load_pkcs12_file`
([zkr.cpp:268-300](native/c/zkr.cpp#L268-L300), a `stat` + `fopen`/`fread` on a USS
path, called at [zkr.cpp:625](native/c/zkr.cpp#L625)). Make it symmetric with export
by letting the caller pass bytes:

1. [`ZKRImportOptions`](native/c/zkr.hpp#L138-L147) — add one field after `p12_path`:
   ```cpp
     std::string p12_data;      // pre-loaded PKCS#12 bytes; when non-empty, used instead of p12_path
   ```
   Keep `p12_path`: existing callers and
   [zkr.test.cpp:722, 769](native/c/test/zkr.test.cpp#L722) keep working unchanged.
2. Update the `zkr_import_cert` doc comment
   ([zkr.hpp:215-220](native/c/zkr.hpp#L215-L220)) to say bytes may be supplied
   directly, so `zkr` stays free of data-set knowledge.
3. In `zkr_import_cert`, replace the unconditional `load_pkcs12_file` call at
   [zkr.cpp:625](native/c/zkr.cpp#L625) with a branch on `opts.p12_data`.
   **Ownership gotcha:** the existing code later calls `gsk_free_buffer(&buff_in)`,
   which `free()`s `buff_in.data`. So the `p12_data` branch must `malloc` +
   `memcpy` — do **not** point `buff_in.data` at the `std::string`'s storage, or
   GSK will free memory it does not own. Mirror `load_pkcs12_file`'s
   out-of-memory diagnostic via `record_message(zkr, "IMPORT", ...)`.

## Step 3 — `native/c/commands/certificates.cpp`: the feature

### 3a. Two local static helpers

Keep these file-local — they encode cert-specific *policy* (format → binary vs text,
RACDCERT-parity creation attributes), while the generic I/O now lives in
`zds_write_binary`. Place them with the other statics near `reject_virtual_ring` /
`report_error`.

```cpp
// Read a PKCS#12 blob out of a sequential data set or PDS/E member. Binary mode so
// LE concatenates V-format record data with no RDWs and no code-page conversion --
// byte-identical to `cp -B "//'DSN'"`.
static int read_cert_dsn(const std::string &dsn, std::string &data, std::string &err);

// Write exported certificate bytes to a data set, creating it when absent.
// is_binary=true  (p12): raw bytes, chunked into V-format records (zds_write_binary).
// is_binary=false (pem): the EBCDIC PEM text is written as records via zds_write in
//                        text mode -- one base64 line per record.
static int write_cert_dsn(const std::string &dsn, const std::string &data,
                          bool is_binary, std::string &err);
```

`read_cert_dsn`:
- Pre-check `zds_dataset_exists(dsn)` ([zds.hpp:140](native/c/zds.hpp#L140) — it
  strips the member and checks the base library) so a typo produces a clear message
  instead of a bare fopen failure, matching `load_pkcs12_file`'s diagnostics.
- `zut_prepare_encoding("binary", &zds.encoding_opts)`
  ([zut.cpp:917-931](native/c/zut.cpp#L917-L931)) then
  `zds_read(ZDSReadOpts{.zds = &zds, .dsname = dsn}, data)`.
- Reject empty content: `"PKCS#12 data set is empty: <dsn>"`.

`write_cert_dsn`:
- If `!zds_dataset_exists(dsn)`, create it first — both `zds_write_binary` and
  `zds_write` hard-require an existing target. Use `zds_create_dsn`
  ([zds.hpp:238](native/c/zds.hpp#L238)) with **explicitly set** attributes. Do
  **not** use `zds_create_dsn_vb`, which builds a *PDSE* with `LRECL=255`
  ([zds.cpp:2222-2234](native/c/zds.cpp#L2222-L2234)) — wrong shape here:

  ```cpp
  DS_ATTRIBUTES a{};              // zero-init; see gotcha below
  a.dsorg     = "PS";             // member target: "PO" + a.dirblk = 5 + a.dsntype = ZDS_DSNTYPE_LIBRARY
  a.recfm     = "V,B";            // MANDATORY -- FB pads records with blanks (step 1, obligation 3)
  a.lrecl     = 84;               // RACDCERT FORMAT(PKCS12DER) parity -> 80 data bytes per record
  a.blksize   = 27998;
  a.alcunit   = "TRK";
  a.primary   = 5;
  a.secondary = 5;
  ```

  **`DS_ATTRIBUTES` gotcha:** `zds_create_dsn` treats *negative* as "unset" but
  **preserves 0** ([zds.cpp:2132-2145](native/c/zds.cpp#L2132-L2145)), and
  `DS_ATTRIBUTES a{}` zero-inits. Set every field you care about explicitly or you
  get `LRECL(0)`/`BLKSIZE(0)`. (This is the known `DS_ATTRIBUTES` quirk already on
  the deferred-issues list.)

  For a member target, create the *library* from the base name (`PO` / `LIBRARY` /
  `dirblk`), then write the member. Validate the member name with the existing
  [`zds_is_valid_member_name`](native/c/zds.cpp#L638-L655).
- `is_binary` → `zds_write_binary`. Otherwise `zds_write` with a **zeroed `ZDS`**:
  `eDataTypeText` + empty codepage means `zds_use_codepage` returns false → **no
  iconv**, so the already-EBCDIC PEM passes straight through as text records.
- Leave `zds.etag` empty so `validate_etag_if_present` is a no-op.

**Security note:** the USS path deliberately uses `zut_write_file_private` to force
`0600` on a file that may contain a private key
([certificates.cpp:240-248](native/c/commands/certificates.cpp#L240-L248)). Data
sets have no mode bits — protection is the RACF DATASET profile. Say so in the
`--dsn` help text and the CLI `description`; it is a real difference in the security
posture of the two sinks. See also step 5.

### 3b. `handle_cert_export` ([certificates.cpp:199-280](native/c/commands/certificates.cpp#L199-L280))

- Read `const std::string dsn = context.get<std::string>("dsn", "");` beside the
  existing `file` at line 206.
- Validate before any ESM call, in the style of the existing checks:
  - both `--file` and `--dsn` → `"Error: specify --file or --dsn, not both"`
    (reuse the exact `"not both"` wording from the `cert delete` check, which
    [zkr.test.cpp:198-206](native/c/test/zkr.test.cpp#L198-L206) already asserts on).
  - `format p12` with neither → extend today's message at line 256 to
    `"Error: PKCS#12 output is binary; specify --file or --dsn"`.
- After `zkr_export_cert` succeeds, add a `dsn` branch beside the `file` branch:
  `write_cert_dsn(dsn, data, is_p12, err)`, print
  `"Certificate written to data set <dsn> (<n> bytes)"`, set
  `result->set("dsn", str(dsn))` plus the existing `bytesWritten`.
- The base64 `data` field at line 277 is unchanged, including the
  EBCDIC→ISO8859-1 conversion for PEM.

### 3c. `handle_cert_import` ([certificates.cpp:282-330](native/c/commands/certificates.cpp#L282-L330))

- Read `dsn`; keep `opts.p12_path = context.get<std::string>("file", "")`.
- Replace the "`--file` is required" check at
  [certificates.cpp:303-307](native/c/commands/certificates.cpp#L303-L307) with:
  - both set → `"Error: specify --file or --dsn, not both"`
  - neither set → `"Error: --file or --dsn is required (source PKCS#12)"`
- When `dsn` is set, call `read_cert_dsn(dsn, opts.p12_data, err)` before
  `zkr_import_cert` — **after** the label/usage/password checks and the existing
  `reject_virtual_ring` call (line 313), so cheap validation still fails fast
  without I/O.

### 3d. Argument registration ([certificates.cpp:746-771](native/c/commands/certificates.cpp#L746-L771))

Add to both commands in the file's existing `add_keyword_arg` style. **No short
alias** — `-f` is `--file`, `-F` is `--format`; the file's convention for newer
options is long-only (`--skip-refresh`, `--from-database`, `--label-only`):

```cpp
  export_cmd->add_keyword_arg("dsn", make_aliases("--dsn"),
      "output data set (sequential or PDS/E member), created if absent; mutually "
      "exclusive with --file. Protection is the RACF DATASET profile, not file permissions.",
      ArgType_Single, false);
```
```cpp
  import_cmd->add_keyword_arg("dsn", make_aliases("--dsn"),
      "source PKCS#12 data set (sequential or PDS/E member); mutually exclusive with --file",
      ArgType_Single, false);
```

Also **drop `required` on import's `--file`**
([certificates.cpp:765](native/c/commands/certificates.cpp#L765) — currently `true`),
since `--dsn` can satisfy it, and add an `add_example` per command using a realistic
DSN.

The parser does have a `conflicts_with` builder
([parser.hpp:292-302](native/c/parser.hpp#L292-L302)) but **no command in the repo
uses it**; handler-level validation is consistent with this file and is the only
option that also covers the JSON-RPC path, where there is no arg parser.

## Step 4 — SDK RPC types (the source of truth) and the CLI

`packages/sdk/src/doc/rpc/*.ts` is the **only** hand-written definition; the C++
validation schemas are generated *from* it by
[scripts/generateTypes.ts](scripts/generateTypes.ts) — see
[add-new-command.md:222-253](doc/add-new-command.md#L222-L253). A `?` in TS becomes
`FIELD_OPTIONAL`; no `?` becomes `FIELD_REQUIRED`.

In [packages/sdk/src/doc/rpc/certificates.ts](packages/sdk/src/doc/rpc/certificates.ts),
following the file's convention of a multi-line JSDoc on **every** field:

1. `ExportCertificateRequest` (lines 163-188) — add `dsn?: string` after
   `file?: string`, documented as "… Mutually exclusive with `file`." (the same
   prose-only mutual-exclusion style as `DeleteCertificateRequest.keyring`/`database`
   at lines 94-102).
2. `ExportCertificateResponse` (lines 190-219) — add `dsn?: string`, "Output data
   set name, when written to a data set".
3. `ImportCertificateRequest` (lines 221-251) — add `dsn?: string` **and relax
   `file: string` → `file?: string`** (line 241).

**`file` becoming optional is the one consequence to be deliberate about.** It
regenerates [requests.hpp:90](native/c/server/schemas/requests.hpp#L90) from
`FIELD_REQUIRED(file, STRING)` to `FIELD_OPTIONAL`, so the server no longer rejects
a `file`-less import at the schema layer — enforcement moves to
`handle_cert_import`. That is exactly the pattern already used for export's
`password`, and
[zowex.cert.server.test.cpp:73-83](native/c/test/zowex.cert.server.test.cpp#L73-L83)
documents the reasoning verbatim. Backward compatible at runtime: every existing
caller sends `file`.

**Field name `dsn`, not `dsname`.** The rest of the RPC layer uses `dsname`
(`ds.ts`, `jobs.ts`), but there the data set *is* the subject; here it is one of two
interchangeable I/O sinks, so `file` / `dsn` reads better and matches both the CLI
option and the native arg name — meaning **no `.rename_arg` is needed** in
[rpc_commands.cpp:344-349](native/c/server/rpc_commands.cpp#L344-L349). If you
prefer `dsname`, add `.rename_arg("dsname", "dsn")` to both cert registrations.

Then **regenerate and commit** — never hand-edit the generated `.hpp` files
(`// Code generated by generateTypes.ts. DO NOT EDIT.`). Skipping this makes the
server answer `"Request validation failed"` for any request carrying `dsn`:

```bash
npm run build:types   # rewrites native/c/server/schemas/{requests,responses}.hpp
```

**`packages/sdk/src/RpcClientApi.ts` needs no change.** Its cert block
([lines 34-70](packages/sdk/src/RpcClientApi.ts#L34-L70)) is a thin generic mapping,
so the new field is picked up structurally.

### CLI

[Cert.definition.ts](packages/cli/src/certificates/cert/Cert.definition.ts):

- `ExportCertDefinition` (lines 29-73): add a `dsn` option after `file` (lines
  59-64). Use Imperative's `conflictsWith` — unlike the native parser, this **is**
  used in this package (e.g.
  [DataSet.definition.ts:75](packages/cli/src/download/data-set/DataSet.definition.ts#L75)):
  ```ts
  {
      name: "dsn",
      description:
          "Output data set on the z/OS server (sequential or PDS/E member), created if it does not " +
          "exist. Mutually exclusive with --file. Note that a data set is protected by its RACF " +
          "DATASET profile, not by file permissions.",
      type: "string",
      conflictsWith: ["file"],
  },
  ```
- `ImportCertDefinition` (lines 75-122): add the same-shaped `dsn` option and
  **remove `required: true` from `file`** (line 112).
- Update both `description` strings (lines 34-37, 80-83) to mention data sets, and
  add one `examples` entry each using a realistic DSN.

Handlers:

- [Import.handler.ts:21-30](packages/cli/src/certificates/cert/import/Import.handler.ts#L21-L30)
  — destructure `dsn` and forward it (`--dsn` arrives as `params.arguments.dsn`; no
  case conversion needed).
- [Export.handler.ts:21-36](packages/cli/src/certificates/cert/export/Export.handler.ts#L21-L36)
  — forward `dsn`, **and widen the output branch**. It currently keys on
  `if (response.file)`; a successful DSN export would fall into the `else` and
  base64-decode a binary PKCS#12 blob onto stdout. Change to
  `if (response.file || response.dsn)` and build the message from whichever is set:
  `Certificate '<label>' exported to data set <dsn> on the server (<n> bytes, p12).`

## Step 5 — Protected / encrypted data sets (confirm, then guard)

Three independent protection mechanisms can sit on a data set holding a
certificate. **None of them is the same thing as `--password`**, which is the
PKCS#12 passphrase handed to `gsk_decode_import_key` / `gsk_export_key` to decrypt
the *bundle*. `--password` is unchanged by this work; the mechanisms below guard the
*container*. Both can apply at once, and the error messages must not conflate them.

**(a) Legacy MVS password protection — the one real hazard.** The JFCB header the
repo already vendors defines the bits, in `jfcbind2`
([chdsect/jfcb.h:308-309](native/c/chdsect/jfcb.h#L308-L309)):

```c
#define jfcbrwpw 0x30 /* PASSWORD IS REQUIRED TO WRITE BUT NOT TO READ */
#define jfcsecur 0x10 /* PASSWORD IS REQUIRED TO READ OR TO WRITE      */
```

The hazard is **not** a failure — it is a **hang**. OPEN on a password-protected
data set issues a WTOR to the operator console and waits for a reply. `zowex server`
is a long-lived RPC server over SSH with no console: the request would block
indefinitely rather than return an error.

Guard it **pre-OPEN**, which is nearly free because `open_output_bpam` already reads
the JFCB (`read_output_jfcb`, [zam.c:453](native/c/zam.c#L453)) and already inspects
`jfcbind1` in `validate_jfcb_attributes` ([zam.c:75-95](native/c/zam.c#L75-L95)).
Add a check for `jfcb.jfcbind2 & (jfcsecur | jfcbrwpw)` right there, failing with
`ZDS_RTNCD_UNSUPPORTED_DATA_SET` and a message that names password protection and
tells the user to copy the data set to an unprotected one. This protects every BPAM
caller, not just certificates.

For the **read/import** side (which uses `zds_read`'s `fopen`, no JFCB), the
equivalent indicator is the DSCB `DS1DSIND` byte — already captured but never parsed
as `DSCBFormat1.ds1dsind` ([zdstype.h:156](native/c/zdstype.h#L156)). Surface it as
a `bool password_protected` on `ZDSEntry`, populated in the DSCB parsing that
`zds_list_data_sets(..., show_attributes=true)` already performs, right beside the
existing `entry.encrypted = (dscb->ds1flag1 & DS1ENCRP_MASK) != 0;`
([zds.cpp:3116-3118](native/c/zds.cpp#L3116-L3118)) and its mask defines
([zds.cpp:2748-2751](native/c/zds.cpp#L2748-L2751)). That also makes the flag
available to `data-set list`.

**Confirmation checklist — do this before writing the guard, since one bit value is
not yet repo-verified:**

- [ ] Confirm the `DS1DSIND` password bit values against the DFSMS DSCB reference
      (*DFSMSdfp Advanced Services*, format-1 DSCB / `IECSDSL1`). The `jfcbind2`
      constants above **are** repo-verified; `DS1DSIND` is not — do not guess a mask.
- [ ] Confirm empirically that OPEN on a password-protected data set issues a WTOR
      rather than failing. **This cannot be tested on lpar.1**: `SYS1.PASSWORD` does
      not exist there (verified read-only via MCP), so legacy password protection is
      inactive. Either test on a system that has it, or unit-test the bit logic on a
      synthetic JFCB/DSCB and record in the plan that the end-to-end path is
      unverified.
- [ ] Confirm **DFSMS data set encryption** is transparent. Expected: with access to
      the ICSF key label, reads/writes just work and no code change is needed; the
      flag is already reported (`encrypted` in `getDatasetAttributes` / `ZDSEntry`).
      Needs an encrypted fixture — a DATACLAS carrying a key label. None of
      FERNANDO's 197 data sets is currently encrypted.
- [ ] Confirm a **RACF-denied** data set produces a clean error, not a process kill.
      Expected clean: the BPAM path is covered by the DCB abend exit
      ([doc/dcb-abend-exit.md](doc/dcb-abend-exit.md)) and the sequential path gets a
      NULL from `fopen`. Test by pointing `--dsn` at a data set you cannot read
      (e.g. another user's, or `SYS1.*`) on both import and export.
- [ ] Decide the follow-up: if legacy password protection turns out to matter for a
      real consumer, a `--dsn-password` option would be the shape — explicitly
      **out of scope** here, and only worth it with a concrete requester.

## Step 6 — Docs and changelogs

- [doc/certificates-test-plan.md](doc/certificates-test-plan.md): update the
  `exportCertificate` / `importCertificate` rows (lines 38-39), add `--dsn` items to
  the manual checklist (lines 137-145), and simplify the Item 2 fixture recipe
  (lines 82-86) — the `cp -B` and `DELETE '<dsn>'` steps go away once import reads a
  DSN directly.
- [doc/testbed-client-cert-auth--with-cli.md:233-237](doc/testbed-client-cert-auth--with-cli.md#L233-L237)
  and
  [doc/testbed-client-cert-auth--with-mcp.md:376-380](doc/testbed-client-cert-auth--with-mcp.md#L376-L380):
  both close with "the upcoming `zowex system cert import/export --dsn` work" — flip
  those to the shipped syntax and record the byte-parity result from verification
  step 1.
- Changelogs, under `## Recent Changes` with a PR link (existing format:
  `- <sentence>. [#NNNN](https://github.com/zowe/zowex/pull/NNNN)`):
  - [native/CHANGELOG.md](native/CHANGELOG.md) — a `c:` entry for `--dsn`, one for
    `zds_write_binary`, one for the password-protection guard.
  - [packages/sdk/CHANGELOG.md](packages/sdk/CHANGELOG.md) — the new `dsn` field and
    `file` becoming optional on `importCertificate`.
  - [packages/cli/CHANGELOG.md](packages/cli/CHANGELOG.md) — the new `--dsn` option.

## Step 7 — Tests

There are **no CLI unit tests, no help-output snapshots, and no `.snap` files**
anywhere in this repo, so nothing to regenerate. The real coverage is native.

1. **[zds.test.cpp](native/c/test/zds.test.cpp)** — unit-test `zds_write_binary`
   directly, since it is the new shared primitive:
   - byte-exact round trip through a **sequential** `VB` data set;
   - byte-exact round trip through a **PDS/E member** (the BPAM path);
   - payload longer than one record, and a payload whose length is an exact multiple
     of `lrecl - 4` (the chunking boundary from step 1, obligation 2);
   - payload containing `0x00`, `0x15` (EBCDIC NL) and `0x40` (EBCDIC blank) bytes —
     the three that a text-mode or fixed-format path would mangle;
   - an `FB` target is **rejected** with `ZDS_RTNCD_UNSUPPORTED_RECFM`.

   Build the payload with `std::string(ptr, len)`, **not** a bare literal: the
   existing binary tests at
   [zds.test.cpp:2106-2126, 2298-2320](native/c/test/zds.test.cpp#L2106-L2126) use
   `std::string binary_data = "Binary\x00\x01..."`, which truncates at the first NUL
   and asserts almost nothing. These are the repo's first real byte-exact binary
   data-set tests.
2. **[zkr.test.cpp](native/c/test/zkr.test.cpp)** — CLI validation cases in the
   unconditional `"CLI input validation (no authority required)"` block (lines
   151-238), which needs no ESM authority. Copy the `cert delete` mutual-exclusion
   shape at lines 198-206:
   - `cert import` rejects `--file` and `--dsn` together → asserts `"not both"`;
   - `cert import` rejects neither → asserts `"--dsn"`;
   - `cert export -F p12` with neither → asserts `"--dsn"`.
3. **`zkr.test.cpp` gated round trip** — extend the p12 test at lines 701-806 (it
   already exports a p12 and re-imports it): export to a scratch DSN with `--dsn`,
   re-import from that DSN, and assert byte-equality between the `--dsn` bytes and
   the `--file` bytes for the same certificate. Cover **both** a sequential DSN and a
   member. Register scratch DSNs in the existing `created_dsns`/`afterAll` cleanup so
   a failed expectation still cleans up.
4. **`zkr_generate_p12_fixture`** (lines 86-130) — drop the
   `cp -B "//'<dsn>'" <uss>` hop and the `DELETE '<dsn>'`; import the DSN directly.
   Net simplification, and it exercises the new path on every gated run.
5. **[zowex.cert.server.test.cpp](native/c/test/zowex.cert.server.test.cpp)** — in
   `"request validation (no authority required)"`, assert that `importCertificate`
   with `dsn` and no `file` is **not** rejected by schema validation
   (`Expect(response).Not().ToContain("Request validation failed")`), proving the
   step-4 `FIELD_OPTIONAL` change and that `dsn` reaches the handler. Model on lines
   64-83.
6. **[packages/sdk/tests/RpcClientApi.test.ts](packages/sdk/tests/RpcClientApi.test.ts)**
   — this file covers `console`/`ds`/`jobs`/`uss`/… but has **zero `certificates`
   coverage**. Add `it("should route certificate commands correctly")` exercising
   `importCertificate({ …, dsn })` / `exportCertificate({ …, dsn })`, following the
   `client.request` assertion style at lines 39-44. New coverage, not a modification.

---

## Verification

Build and deploy:

```bash
npm run build:types      # MUST run first -- regenerates the RPC schemas
npm run z:upload
npm run z:build
```

**1. Byte parity — the acid test.** Export once, write it both ways, prove the bytes
are identical:

```bash
zowe zowex system cert export FERNANDO '*' -l 'Certificate for FERNANDO' \
    -F p12 -p changeit --dsn "FERNANDO.ZNPDSN.PARITY.P12"
# on z/OS:
cp -B "//'FERNANDO.ZNPDSN.PARITY.P12'" /tmp/parity-dsn.p12
ls -l /tmp/parity-dsn.p12        # must equal the "(N bytes)" the command printed
openssl pkcs12 -in /tmp/parity-dsn.p12 -passin pass:changeit -info -noout
#   -> 2 cert bags + 1 shrouded key bag, as in the testbed doc
```
Note: PKCS#12 encryption is salted, so two *separate* `gsk_export_key` calls differ
— never diff two independent exports. Compare a single export's DSN bytes against
its reported byte count and against `openssl` parsing, or against `--file` output
from the same invocation.

**2. Created-data-set shape** — read-only, via MCP:
`mcp__zowe__getDatasetAttributes { dsn: "FERNANDO.ZNPDSN.PARITY.P12" }`
→ expect `PS / VB / lrecl 84 / blksz 27998`, matching
`FERNANDO.CLIENTCR.FERNANDO.P12`.

**3. PDS/E member — the BPAM path.** This is the case step 1 exists for, so do
**not** skip it:
```bash
zowe zowex system cert export FERNANDO '*' -l 'Certificate for FERNANDO' \
    -F p12 -p changeit --dsn "FERNANDO.ZNPDSN.P12LIB(CERT01)"
```
`mcp__zowe__listMembers { dsn: "FERNANDO.ZNPDSN.P12LIB" }` to confirm the member,
then repeat the step-1 `cp -B` + `openssl` check on `//'…P12LIB(CERT01)'`. Also
confirm no ENQ is left behind (a subsequent export to the same member must succeed,
not hang).

**4. Import from an existing data set.** Probe authority first —
`zowe zowex system keyring create FERNANDO ZNPDSN.RING1`; if that fails, use an
existing ring or an authorized user (see the authority caveat above).

```bash
zowe zowex system cert import FERNANDO ZNPDSN.RING1 -l ZNPDSN01 -u PERSONAL \
    -p changeit --dsn "FERNANDO.CLIENTCR.FERNANDO.P12"
```
Verify with `mcp__zowe__showCertificate { owner: "FERNANDO", keyring: "ZNPDSN.RING1",
label: "..." }`. Expect the already-exists warning — the ESM already holds this
certificate, so the supplied label is ignored and the existing record is connected.

**5. PEM to a data set:** `... -F pem --dsn "FERNANDO.ZNPDSN.CERT.PEM"`, then
`mcp__zowe__readDataset { dsn: "FERNANDO.ZNPDSN.CERT.PEM" }` — should render as
readable `-----BEGIN CERTIFICATE-----` text (this one *is* text, so MCP's text-mode
read is correct here).

**6. Protection behavior** (step 5 confirmations): point `--dsn` at a data set you
cannot read on both import and export — expect a clean diagnostic and a live server,
never an abend or a hang.

**7. Validation errors** (no authority needed):
```bash
zowe zowex system cert import FERNANDO RING01 -l L -u PERSONAL -p x \
    -f /tmp/a.p12 --dsn "A.B.C"      # -> "not both"
zowe zowex system cert import FERNANDO RING01 -l L -u PERSONAL -p x   # -> "--file or --dsn is required"
```

**8. Test suites:**
```bash
npm run z:test -- certificates    # zkr + cert-server suites
npm run z:test -- ds              # zds_write_binary unit tests + regression
npm test                          # SDK vitest
```
Reminder from memory: unset `_BPXK_JOBLOG=STDERR` before `npm run z:test` on
lpar.1 — it fakes ~12 jobs-server RPC failures.

**9. Cleanup:** `mcp__zowe__deleteDataset` on every `FERNANDO.ZNPDSN.**` scratch data
set, `zowe zowex system cert delete FERNANDO -l ZNPDSN01 --database`, and
`zowe zowex system keyring delete FERNANDO ZNPDSN.RING1`.

### Out of scope

The **Zowe MCP server lives in a different repo**, so `mcp__zowe__importCertificate`
/ `exportCertificate` will not accept `dsn` until that repo follows. Worth a
follow-up issue there — combined with MCP having no binary-safe download, a `dsn`
parameter on those two tools would finally let a PKCS#12 round-trip happen entirely
through MCP. The **Python bindings need no change**: `native/python/bindings` covers
only `zds`/`zjb`/`zusf`; there is no `zkr` binding.
