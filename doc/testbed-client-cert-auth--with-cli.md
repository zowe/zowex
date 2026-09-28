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
  Verification section of the design).
