# Test bed: client-certificate auth to z/OSMF as FERNANDO (lpar.1)

This documents how a personal RACF certificate was generated for user `FERNANDO`
on `lpar.1`, exported to a data set, and turned into a local PKCS#12 → PEM/key
pair used to authenticate Zowe CLI (`zowe files ...`) to z/OSMF via mutual TLS
instead of userid/password.

Motivation: we're about to add data-set support to `zowex system cert
import/export` (today `native/c/zkr.cpp` / `native/c/commands/certificates.cpp`
only read/write USS files, e.g. `zkr_import_cert`'s `load_pkcs12_file` does a
plain `fopen`/`fread` against a USS path). Before touching that code, this
gives a real personal cert, a real PKCS#12-in-a-data-set (`FERNANDO.PUBLIC.CERT*`
already existed as fixtures; `FERNANDO.CLIENTCR.FERNANDO.P12` is a fresh one),
and a working end-to-end auth path to validate against later.

## 0. Investigation (read-only, via the Zowe MCP server and `zowex`)

Before mutating anything, checked what already existed:

```
# Confirms the RACF userid for the SSH connection, active system, etc.
mcp__zowe__getContext

# FERNANDO owned zero certificates and zero key rings before this exercise
zowe zowex system keyring list FERNANDO "*" --max-entries 0 --response-format-json
zowe zowex system keyring list-rings FERNANDO --response-format-json   # errors: no rings (ESM rsn 32)

# Found z/OSMF's own keyring/owner by probing likely started-task IDs
zowe zowex system keyring list-rings IZUSVR --response-format-json
```

That last command revealed the ring `IZUSVR/IZUKeyring.IZUDFLT`, holding:

| label                       | owner     | usage    | purpose                                  |
|------------------------------|-----------|----------|-------------------------------------------|
| `DefaultzOSMFCert.IZUDFLT`   | `IZUSVR`  | PERSONAL | z/OSMF's own HTTPS server identity cert   |
| `zOSMFCA2`                   | `irrcerta`| CERTAUTH | local CA that signed the server cert      |
| `DigiCert Global Root CA`    | `irrcerta`| CERTAUTH | public root (reference only, no key)      |
| `DigiCert CA`                | `irrcerta`| CERTAUTH | public intermediate (reference only)      |
| `ROOTSTAR`                   | `irrsitec`| —        | site cert                                  |

**Key point:** `IZUKeyring.IZUDFLT` holds the *server's* identity, not
anything usable as a client cert for a user. To log in to z/OSMF as FERNANDO
via mutual TLS, RACF needs a certificate it can look up and map back to the
`FERNANDO` userid — that has to be generated and owned by FERNANDO.

`zowe zowex system cert show IZUSVR IZUKeyring.IZUDFLT -l zOSMFCA2` showed
`keySize: 2048, keyType: 1` — i.e. RACF actually holds `zOSMFCA2`'s *private*
signing key (it was originally created locally via `RACDCERT CERTAUTH
GENCERT`, not imported from a real CA). That means a new cert can be signed
with it and z/OSMF's TLS listener will already trust the result, since
`zOSMFCA2` is already connected to `IZUSVR/IZUKeyring.IZUDFLT` and TRUSTed.

`zowex` has no `GENCERT` RPC (only import/export/connect/delete/list/show/
trust/rename — see `native/c/zkr.hpp`), so creating the new certificate had to
go through RACF directly via TSO, not through `zowex`.

## 1. Generate FERNANDO's personal certificate

Run via `zowe zos-tso issue command "..."` (uses the `zvdt.1` zosmf profile /
`lpar.1`). This is the same recipe as the team's existing "Testing z/OSMF
Certificates" doc, minus the CA-creation step (`zOSMFCA2` already existed,
trusted, and connected — see above; that step is only needed when standing up
a brand new zVDT from scratch).

```
RACDCERT ID(FERNANDO) GENCERT SUBJECTSDN(CN('User FERNANDO') O('Broadcom') OU('IZUDFLT') C('US')) WITHLABEL('Certificate for FERNANDO') SIGNWITH(CERTAUTH LABEL('zOSMFCA2'))
```

- `ID(FERNANDO)` — the new certificate is owned by FERNANDO in the RACF
  database, which is how RACF maps an incoming client cert back to a userid
  during TLS client auth (no `RACMAP` needed for this path).
- `SIGNWITH(CERTAUTH LABEL('zOSMFCA2'))` — chains it to a CA z/OSMF's own
  keyring already trusts, instead of a self-signed cert the TLS handshake
  would reject.
- `RACDCERT GENCERT` defaults new certs to `TRUST` status, so no separate
  `ALTER ... TRUST` was actually needed here (confirmed after the fact via
  `zowex system keyring list FERNANDO "*"` — status came back `TRUST`). The
  older doc's `RACDCERT ID(<USER>) ALTER(LABEL(...)) TRUST` step is there for
  belt-and-suspenders / in case a site's defaults differ; it's redundant when
  you can see the cert already came back TRUST.
- RACF printed `IRRD175I ... will not be in effect until a SETROPTS REFRESH`.
  This turned out not to block anything end-to-end here, but if client-cert
  login is ever flaky after regenerating a cert, run:
  ```
  SETROPTS RACLIST(DIGTCERT) REFRESH
  ```

Verified the cert existed with a **read-only** `zowex` call (avoids trusting
the TSO JSON API's output, which was observed to occasionally return a
previous command's response on the next call — a timing/ordering quirk of
`zowe zos-tso issue command` worth knowing about, not a RACF issue):

```
zowe zowex system keyring list FERNANDO "*" --max-entries 0 --response-format-json
# -> label: "Certificate for FERNANDO", owner: FERNANDO, usage: PERSONAL, status: TRUST
```

## 2. Export the certificate + private key to a data set

```
RACDCERT ID(FERNANDO) EXPORT(LABEL('Certificate for FERNANDO')) DSN('FERNANDO.CLIENTCR.FERNANDO.P12') FORMAT(PKCS12DER) PASSWORD('changeit')
```

- `FORMAT(PKCS12DER)` bundles the cert + private key + CA chain into one
  PKCS#12 blob, written directly to the named data set (`RECFM=VB LRECL=84` —
  same shape as the pre-existing `FERNANDO.PUBLIC.CERT*` fixtures, confirming
  this is the standard shape RACDCERT produces for this format).
- `PASSWORD('changeit')` is a throwaway PKCS#12 passphrase used only to
  encrypt the bundle in transit; matches the convention in the team's
  existing cert-testing doc. Treat it as public/well-known — fine for a lab
  cert, not for anything real.
- Confirmed the data set materialized via the **read-only** MCP tool instead
  of trusting TSO command echo:
  ```
  mcp__zowe__listDatasets  dsnPattern="FERNANDO.CLIENTCR.**" detail="full"
  ```

## 3. Download the PKCS#12 data set to the laptop

```
zowe zos-files download data-set "FERNANDO.CLIENTCR.FERNANDO.P12" --binary --file ./fernando_lpar1.p12
```

`--binary` is required — this is a binary PKCS#12 blob stored in a `RECFM=VB`
data set; a binary transfer strips the per-record RDWs and concatenates the
raw bytes, an ASCII/text transfer would corrupt it.

Sanity-checked the download without extracting anything:

```
openssl pkcs12 -in fernando_lpar1.p12 -passin pass:changeit -info -noout
```

(showed 2 cert bags + 1 shrouded key bag — i.e. a valid, complete PKCS#12 file)

## 4. Split the PKCS#12 into a PEM cert + unencrypted key (run locally)

```
openssl pkcs12 -in fernando_lpar1.p12 -clcerts -nokeys -out fernando_lpar1.crt
openssl pkcs12 -in fernando_lpar1.p12 -nocerts -nodes -out fernando_lpar1.key
```

- `-clcerts -nokeys` → just the leaf (client) certificate, PEM.
- `-nocerts -nodes` → just the private key, PEM, **unencrypted** (`-nodes`) so
  Zowe CLI can read it without prompting for a key passphrase.

Both commands prompt for the PKCS#12 import password (`changeit`).

## 5. Wire it into a Zowe CLI team config for cert-based auth

Working example (`~/gh/delme/delme/zowe.config.json`):

```jsonc
{
    "$schema": "./zowe.schema.json",
    "profiles": {
        "zosmf": {
            "type": "zosmf",
            "properties": {
                "port": 1443,
                "certFile": "./fernando_lpar1.crt",
                "certKeyFile": "./fernando_lpar1.key"
            },
            "secure": []
        },
        "tso": {
            "type": "tso",
            "properties": { "account": "IZUACCT", "codePage": "1047", "logonProcedure": "IZUFPROC" },
            "secure": []
        },
        "ssh": { "type": "ssh", "properties": { "port": 22 }, "secure": [] },
        "project_base": {
            "type": "base",
            "properties": { "host": "lpar.1", "rejectUnauthorized": false }
        }
    },
    "defaults": { "zosmf": "zosmf", "tso": "tso", "ssh": "ssh", "base": "project_base" },
    "autoStore": true
}
```

Notes:
- `certFile` / `certKeyFile` on a `zosmf`-type profile are all that's needed —
  no `user`/`password`/`token` and no explicit `authOrder` required; Zowe CLI
  picks cert auth automatically when those two properties are present.
- `rejectUnauthorized: false` on the `base` profile is only there because this
  is a lab LPAR with a self-signed/local CA chain; don't carry that into a
  profile pointed at anything real.

## 6. Verified working

```
cd ~/gh/delme/delme
zowe files list ds "SYS1.PARMLIB"
```

Confirmed by the user: this listed data sets successfully — i.e. FERNANDO is
now authenticating to z/OSMF on lpar.1 purely via the client certificate, no
password involved.

## Recap: full command sequence, in order

```bash
# --- one-time RACF setup (via zowe zos-tso issue command against lpar.1) ---
RACDCERT ID(FERNANDO) GENCERT SUBJECTSDN(CN('User FERNANDO') O('Broadcom') OU('IZUDFLT') C('US')) WITHLABEL('Certificate for FERNANDO') SIGNWITH(CERTAUTH LABEL('zOSMFCA2'))
RACDCERT ID(FERNANDO) EXPORT(LABEL('Certificate for FERNANDO')) DSN('FERNANDO.CLIENTCR.FERNANDO.P12') FORMAT(PKCS12DER) PASSWORD('changeit')

# --- pull it down to the laptop ---
zowe zos-files download data-set "FERNANDO.CLIENTCR.FERNANDO.P12" --binary --file ./fernando_lpar1.p12

# --- split locally ---
openssl pkcs12 -in fernando_lpar1.p12 -clcerts -nokeys -out fernando_lpar1.crt
openssl pkcs12 -in fernando_lpar1.p12 -nocerts -nodes  -out fernando_lpar1.key

# --- point a zosmf profile at certFile/certKeyFile (see zowe.config.json above) ---
# --- then just use the CLI normally ---
zowe files list ds "SYS1.PARMLIB"
```

## Appendix: doing this via the Zowe MCP server instead

The Zowe MCP server was later extended with certificate/keyring, USS, and
TSO-command tools (`exportCertificate`, `importCertificate`,
`trustCertificate`, `connectCertificate`, `refreshCertificateClass`,
`runSafeTsoCommand`, `downloadUssFileToFile`, `downloadDatasetToFile`, etc.).
Below is what re-running the steps above through those tools actually looks
like — re-verified live against `FERNANDO`/lpar.1, not just inferred from the
tool descriptions, including the parts that didn't work.

### Investigation (unchanged — already MCP-only)

Section 0 above was already done entirely through MCP (`getContext`,
`listDatasets`, `showCertificate`, plus `zowex system keyring list*` for the
one gap MCP didn't cover at the time). With the new tool set there is still
no `listKeyRings`/`createKeyRing` MCP tool, so enumerating/creating rings
still requires `zowex system keyring list-rings|create` via the CLI.

### 1. Generate the certificate — still no MCP tool for this

There is no `gencertCertificate`-type MCP tool (the cert tools cover
export/import/connect/delete/rename/trust/refresh/setDefault — not
creation). The closest option is `runSafeTsoCommand`, but its own
description says: *"Only allowlisted (safe) commands run automatically;
unknown commands require user confirmation via elicitation."* `RACDCERT
GENCERT` mutates the security database, so it's not going to be on a "safe"
read-oriented allowlist — expect an interactive approval prompt (or an
outright block, depending on how the allowlist is configured) rather than a
silent run. This step still realistically has to go through
`zowe zos-tso issue command` (or an admin session), as done above.

### 2. Trust the certificate — now a direct MCP call, no ring needed

```
mcp__zowe__trustCertificate({ owner: "FERNANDO", label: "Certificate for FERNANDO", status: "TRUST" })
```

Real response:
```json
{
  "owner": "FERNANDO",
  "label": "Certificate for FERNANDO",
  "status": "TRUST",
  "warning": "the change succeeded, but the DIGTCERT class must be refreshed for it to take effect (run 'system cert refresh') (IRRSDL64 DATAALTER: SAF rc 4, ESM rc 4, reason 4)",
  "safReturnCodes": { "functionCode": 12, "safReturnCode": 4, "productReturnCode": 4, "productReasonCode": 4 }
}
```
This is a clean, direct replacement for `RACDCERT ID(...) ALTER(...) TRUST` —
no ring, no TSO quoting to get right. (As noted above, it was already a
no-op here since `GENCERT` defaults to `TRUST`; this just confirms the tool
works and surfaces the same "class needs refreshing" warning `zkr_alter_cert`
produces natively.)

### 3. Refresh the DIGTCERT class — call exists, but FERNANDO isn't authorized

```
mcp__zowe__refreshCertificateClass({})
```

Real response: **failed**.
```
Error: IRRSDL64 REFRESH failed: SAF rc: 8, ESM rc: 8, ESM rsn: 92
```
Reason code 92 is a straight authorization failure — issuing `REFRESH` for
the `DIGTCERT` class needs elevated RACF authority FERNANDO doesn't have on
this LPAR. Worth knowing before relying on this tool: it's not that the tool
is broken, it's that not every userid can do this (same restriction would
apply to `zowex system cert refresh` or `SETROPTS RACLIST(DIGTCERT) REFRESH`
run as FERNANDO). In practice this didn't block anything in this exercise —
see the loose-ends note below.

### 4. Export to PKCS#12 — works, and confirms `keyring: "*"` is valid here

```
mcp__zowe__exportCertificate({
  owner: "FERNANDO", keyring: "*", label: "Certificate for FERNANDO",
  format: "p12", password: "changeit",
  file: "/u/users/fernando/fernando_lpar1_mcp_test.p12"
})
```

Real response:
```json
{ "label": "Certificate for FERNANDO", "owner": "FERNANDO", "keyring": "*", "format": "p12",
  "file": "/u/users/fernando/fernando_lpar1_mcp_test.p12", "bytesWritten": 2683 }
```

Two useful findings from actually running this:
- **`keyring: "*"` (the owner's virtual key ring) works for export.** The
  certificate never needs to be connected to a real ring first — no
  `keyring create` / `cert connect` detour needed, unlike what the original
  plan assumed. This mirrors how `RACDCERT ... EXPORT` operates directly on
  the security database regardless of ring.
- The response has **no `data` (base64) field**, even though
  `native/c/commands/certificates.cpp`'s `handle_cert_export` always sets
  `result->data` = base64 of the exported bytes in the underlying RPC. The
  MCP tool apparently drops that field when `file` is given (reasonable —
  avoids duplicating a multi-KB blob into the tool response) — but it means
  there is currently **no way to get the p12 bytes back through MCP without
  a working binary file transfer**, which is the next problem:

### 5. Getting the exported file back — binary content gets corrupted

```
mcp__zowe__downloadUssFileToFile({
  path: "/u/users/fernando/fernando_lpar1_mcp_test.p12",
  localPath: "<workspace>/fernando_lpar1_mcp_test.p12"
})
```

This **succeeded but silently corrupted the file**: 2683 bytes on z/OS became
3621 bytes locally, and the result no longer parses:
```
openssl pkcs12 -in fernando_lpar1_mcp_test.p12 -passin pass:changeit -info -noout
# asn1 encoding routines:asn1_check_tlen:wrong tag ... nested asn1 error ... Type=PKCS12
```
Both `downloadUssFileToFile` and `downloadDatasetToFile` are documented as
writing **"UTF-8 text"** — they do an EBCDIC→text conversion pass, which is
correct for source/JCL/config files but destructive for arbitrary binary
data like a PKCS#12 blob. **There is currently no binary-safe download path
through this MCP server**; `zowe zos-files download data-set --binary` (or
the CLI's USS-file binary download) remains the only reliable way to pull a
p12 back to the laptop. This is the same "USS/DSN as bytes vs. USS/DSN as
text" distinction that motivates the `zowex` data-set import/export work in
the first place — worth keeping in mind when we design that feature's own
MCP surface, if it gets one.

(The test file and its local copy were deleted after this check —
`mcp__zowe__deleteUssFile` on the z/OS side, `rm` locally — so nothing from
this appendix's experiment was left behind.)

### Net effect: MCP today covers steps 2–4 of this recipe; 1 and part of 5 still need the CLI

| Step | Via MCP? |
|---|---|
| Investigate rings/certs | ✅ (already used) |
| `GENCERT` | ❌ no tool; `runSafeTsoCommand` would need elicitation approval for a mutating RACF command |
| `ALTER ... TRUST` | ✅ `trustCertificate` |
| `SETROPTS ... REFRESH` | ⚠️ tool exists (`refreshCertificateClass`) but requires RACF authority FERNANDO doesn't have |
| `EXPORT ... FORMAT(PKCS12DER)` | ✅ `exportCertificate`, and confirmed `keyring: "*"` avoids needing a real ring |
| Download the binary p12 | ❌ MCP's download tools are text-mode only and corrupt binary content; use the CLI's `--binary` download |
| Local `openssl` split, profile wiring | unchanged — local machine steps, not applicable to MCP |

## Loose ends / follow-ups

- `SETROPTS RACLIST(DIGTCERT) REFRESH` was never explicitly run; everything
  worked without it. Keep it in your back pocket if a future regenerate/export
  cycle behaves inconsistently.
- `RACDCERT ID(FERNANDO) ALTER(...) TRUST` is unnecessary when `GENCERT`
  already defaults to `TRUST` (true here) — only needed if a site changes
  that default.
- `zowe zos-tso issue command`'s JSON response was observed to sometimes lag
  by one call (echoing the *previous* command's output). When in doubt, don't
  trust the TSO command's stdout for verification — cross-check with a
  read-only `zowex`/MCP call instead, as done above.
- This whole exercise doubles as a concrete test case for
  `zowex system cert import/export --dsn`: `FERNANDO.CLIENTCR.FERNANDO.P12`
  (and the pre-existing `FERNANDO.PUBLIC.CERT*` data sets) are real
  PKCS#12-in-a-data-set fixtures to import/export against. `--dsn` support has
  landed in the native/SDK/CLI layers; end-to-end byte-parity verification
  against these fixtures on a deployed build is still outstanding (see the
  Verification section of the design). This also settles the MCP
  binary-safe-download gap noted above at 324-346: once the follow-up in the
  Zowe MCP server repo adds a `dsn` parameter to `importCertificate`/
  `exportCertificate`, a PKCS#12 round trip will be possible entirely through
  MCP without touching `downloadUssFileToFile`/`downloadDatasetToFile`.
