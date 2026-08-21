# zoweax: installing and securing the APF-authorized console binary

`zoweax` is the APF-authorized alternative to `zowex`. It contains only
the `console` command group (plus `version`/help) and exists because z/OS
extended MCS console services (MCSOPER/MGCRE/MCSOPMSG) require an authorized
caller. Everything else — data sets, jobs, USS, TSO, the JSON-RPC server, and
plug-in loading — lives in the unauthorized `zowex` binary, which never holds
APF authorization. When a client issues the `consoleCommand` JSON-RPC method,
`zowex server` spawns `zoweax console issue ...` in a separate address space
per request.

`zoweax` is **never installed automatically**. The SDK / VS Code / MCP
auto-deploy path ships only `zowex` (to `~/.zowe-server`), because marking a
file APF-authorized is deliberately a system-programmer action that automated
tooling cannot and should not perform. Installing `zoweax` is a manual,
audited step described below.

## Security model at a glance

| Layer | Enforced by | What it guarantees |
| --- | --- | --- |
| Who can run it | UNIX owner/group/mode on the `zoweax` file | Execute permission is the first gate |
| APF authorization | `extattr +ap` set by a sysprog; `TESTAUTH` at every console entry point | Without the attribute, console commands fail closed (`Not authorized`) |
| Address-space isolation | `extattr -s` (set at build) + kernel APF-state rules | `zoweax` never runs in a shared address space, regardless of `_BPX_SHAREAS` |
| Console activation | ESM `MVS.MCSOPER.<console>` profiles (OPERCMDS class) | Sites control who may activate a console, per user |
| Command authority | ESM OPERCMDS profiles (`MVS.*`) checked under the **invoking user's identity** | Each command is individually authorized and audited (SMF) |
| Fallback authority | Console `AUTH` attribute from the user's OPERPARM segment (default `INFO`) | With no OPERCMDS decision, only informational commands succeed |
| Privilege drop | Pre-command hook (`ZUTNOAUT`) | Non-console commands (`version`, help, interactive) relinquish authorization before running, fail closed |


## Installation

Prerequisites (system programmer):

- READ access to `BPX.FILEATTR.APF` in the FACILITY class (to run `extattr +ap`).
- The target filesystem must not be mounted `NOSETUID` (extended attributes
  are ignored on NOSETUID mounts).

Steps:

1. **Obtain the binary.** `zoweax` ships inside the release `server.pax.Z`
   from [GitHub releases](https://github.com/zowe/zowex/releases) (the release
   pax contains both `zowex` and `zoweax`; the SDK-embedded pax contains only
   `zowex`). Or build from source: `npm run z:rebuild` produces both in
   `c/build-out` with the extended attributes already set.

2. **Install to a protected directory** — for example `/usr/lpp/zowex/bin`.
   Never install `zoweax` in a user-writable location such as
   `~/.zowe-server`: anyone who can replace the file contents of an
   APF-authorized binary can run arbitrary authorized code.

   ```sh
   cp zoweax /usr/lpp/zowex/bin/zoweax
   chown SYSPROG:ZOWECONS /usr/lpp/zowex/bin/zoweax
   chmod 750 /usr/lpp/zowex/bin/zoweax     # group membership = who may issue console commands
   ```

3. **Set the extended attributes:**

   ```sh
   extattr +ap /usr/lpp/zowex/bin/zoweax   # APF-authorized
   extattr -s  /usr/lpp/zowex/bin/zoweax   # never run in a shared address space
   ```

4. **Verify:**

   ```sh
   ls -E /usr/lpp/zowex/bin/zoweax          # expect 'ap' in the attribute flags, no 's'
   /usr/lpp/zowex/bin/zoweax console issue "D T"
   ```

   `IEE136I` output means APF and console activation work. `Not authorized`
   means the attribute is missing or the mount is NOSETUID.

**Upgrading:** extended attributes are cleared whenever the file contents are
replaced. Re-run step 3 after every upgrade — a missed `extattr` fails closed
(console commands stop working) rather than open, but it will look like an
outage.

**Uninstalling:** delete the file. Nothing else is registered anywhere.

## How `zowex server` finds zoweax

Resolution order for each `consoleCommand` request:

1. `ZOWEAX_PATH` environment variable (full path to the binary) — most reliable.
2. A `zoweax` file next to the running `zowex` binary — appropriate only when
   `zowex` itself is in a protected admin-managed directory.
3. `zoweax` on the server process's `PATH`.

Note that the server runs under an SSH exec channel — a **non-login shell** —
so `/etc/profile` and `~/.profile` are typically not sourced and `PATH` is the
sshd default. A directory added to `PATH` by a profile script will not be
visible. Verify from the server's perspective:

```sh
ssh user@host 'echo $PATH; command -v zoweax'
```

## ESM configuration

Two resource families control console use, both in the **OPERCMDS** class:

- `MVS.MCSOPER.<console-name>` — checked at console activation. The default
  console name is the user's ID (CLI) or the user's ID plus a digit suffix
  0-9 (JSON-RPC, to allow concurrent requests), so a generic
  `MVS.MCSOPER.<userid>*` profile covers both.
- `MVS.<command>.<qualifier>` — checked per command under the invoking user's
  identity. Profile names and required access levels for every MVS command
  are listed in *z/OS MVS System Commands*, "Command authorization" — e.g.
  DISPLAY commands need READ on `MVS.DISPLAY.**`; WTOR replies need the
  documented access on `MVS.REPLY.*`.

When the ESM returns "no decision" (class inactive or no matching profile),
z/OS falls back to the console's authority attribute, which comes from the
user's **OPERPARM segment** (default `INFO` — informational commands only).
There is deliberately no code-level way to request higher authority; sites
that cannot use OPERCMDS profiles can raise a specific user's fallback
authority via OPERPARM instead.

> The ACF2 and Top Secret examples below follow each product's documented
> command patterns but **must be validated by an administrator of that ESM**
> before use — resource type mappings (CLASMAP) and profile record syntax
> vary by site.

### RACF

```
/* Activate the class (once) */
SETROPTS CLASSACT(OPERCMDS) GENERIC(OPERCMDS) RACLIST(OPERCMDS)

/* Console activation, per user (covers USERID and USERID0-9) */
RDEFINE OPERCMDS MVS.MCSOPER.USERID* UACC(NONE)
PERMIT MVS.MCSOPER.USERID* CLASS(OPERCMDS) ID(USERID) ACCESS(READ)

/* Informational displays */
RDEFINE OPERCMDS MVS.DISPLAY.** UACC(NONE) AUDIT(ALL(READ))
PERMIT MVS.DISPLAY.** CLASS(OPERCMDS) ID(OPERGRP) ACCESS(READ)

/* WTOR replies (check the command table for the required access level) */
RDEFINE OPERCMDS MVS.REPLY.** UACC(NONE) AUDIT(ALL(READ))
PERMIT MVS.REPLY.** CLASS(OPERCMDS) ID(OPERGRP) ACCESS(UPDATE)

/* Fallback console authority for a user without OPERCMDS coverage */
ALTUSER USERID OPERPARM(AUTH(INFO))

/* Installer authority for extattr +ap */
RDEFINE FACILITY BPX.FILEATTR.APF UACC(NONE)
PERMIT BPX.FILEATTR.APF CLASS(FACILITY) ID(SYSPROG) ACCESS(READ)

SETROPTS RACLIST(OPERCMDS) REFRESH
SETROPTS RACLIST(FACILITY) REFRESH
```

### ACF2

Confirm the resource type mapped to the OPERCMDS class first (`SHOW CLASMAP`;
commonly `OPR`) and to FACILITY (commonly `FAC`).

```
ACF
SET RESOURCE(OPR)
RECKEY MVS ADD(MCSOPER.USERID- UID(uidstring-for-USERID) SERVICE(READ) ALLOW)
RECKEY MVS ADD(DISPLAY.- UID(uidstring-for-opergrp) SERVICE(READ) ALLOW)
RECKEY MVS ADD(REPLY.- UID(uidstring-for-opergrp) SERVICE(UPDATE) ALLOW)
F ACF2,REBUILD(OPR)

* Fallback console authority (OPERPARM user profile record)
SET PROFILE(USER) DIVISION(OPERPARM)
INSERT USERID AUTH(INFO)
F ACF2,REBUILD(USR),CLASS(P)

* Installer authority for extattr +ap
SET RESOURCE(FAC)
RECKEY BPX ADD(FILEATTR.APF UID(uidstring-for-sysprog) SERVICE(READ) ALLOW)
F ACF2,REBUILD(FAC)
```

### Top Secret (TSS)

```
* Ownership (once)
TSS ADDTO(opsdept) OPERCMDS(MVS.)

* Console activation, per user
TSS PERMIT(USERID) OPERCMDS(MVS.MCSOPER.USERID) ACCESS(READ)

* Informational displays
TSS PERMIT(opergrp) OPERCMDS(MVS.DISPLAY.) ACCESS(READ)

* WTOR replies (check the command table for the required access level)
TSS PERMIT(opergrp) OPERCMDS(MVS.REPLY.) ACCESS(UPDATE)

* Fallback console authority (OPERPARM segment equivalent)
TSS ADDTO(USERID) OPERPARM(AUTH(INFO))

* Installer authority for extattr +ap
TSS PERMIT(sysprog) IBMFAC(BPX.FILEATTR.APF) ACCESS(READ)
```
