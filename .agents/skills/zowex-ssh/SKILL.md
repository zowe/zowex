---
name: zowex-ssh
description: Deploy and operate the zowex JSON-RPC server on a *remote* z/OS host over SSH, for end-user dataset/job/USS operations when that host has no working z/OSMF endpoint. Use when the user asks to "connect to <host> over ssh", "deploy zowex to <lpar>", or "use zowex on <lpar>" to work with a remote system's datasets, jobs, or USS files. Not for building/testing this repo's own native code — that uses `npm run z:rebuild` / `npm run z:test`.
---

# zowex over SSH — deploy and operate

This skill is an alternative to the `zowe` CLI (z/OSMF REST): it uses the **zowex** native backend driven over SSH stdio instead. Use it when the target z/OS system has SSH but no z/OSMF. It's for operating on a remote target host's datasets/jobs/USS — unrelated to building or testing this repo's own native code, which uses `npm run z:rebuild` / `npm run z:test`.

Everything below uses two shell variables you should set once per target:

```bash
ZX_HOST=user@host          # ssh target (z/OS USS)
ZX_DIR=/u/$USER/zowex      # remote deploy dir; pick anything the user can write
```

If the user gives only a hostname, ask for (or infer) the SSH user and a writable USS directory.

**If you are an LLM/agent running this skill, always set your own unique `ZX_STATE` first — never rely on the default.** All `zx` state (config, persistent-session pid, FIFOs, `ControlMaster` socket) lives under `$ZX_STATE`, which defaults to one shared directory per UID (`/tmp/zx.$UID`). That default is meant for a human's interactive shell. If an agent run uses it too, it collides with any persistent session the user has open — `zx use`/`zx deploy` overwrites their saved host config, `zx start`/`zx stop`/`zx reset` clobbers their session, and vice versa. It also lets two concurrent agent runs stomp on each other the same way.

```bash
export ZX_STATE=/tmp/zx-agent-$$.$UID   # unique per agent run; keep it short (see §6 ControlPath note)
```

Do this at the very start of every session, before the first `zx`/`$zx` call — deploy included. Never call `zx reset` under a `ZX_STATE` a human or another agent might be using (see §7).

**Local prereqs:** `ssh`, `sftp`, `jq`, `base64`, bash ≥3.2, and `curl` or `wget`. Run `.agents/skills/zowex-ssh/zx check` to verify. (`jq` is the only one not stock on macOS — if it's missing, ask the user to install it via their package manager, e.g. Homebrew on macOS, before continuing.)
**Remote prereqs:** SSH login + a writable USS directory. The `zowex` binary is self-contained.
**Bundle:** `zx deploy` will auto-download the latest `server.pax.Z` from [github.com/zowe/zowex/releases](https://github.com/zowe/zowex/releases) if it isn't found locally. Default save path is `~/.local/share/zx/server.pax.Z` (always user-writable, works whether `zx` is run directly or via a PATH symlink). Downloads automatically without prompting. To pin a specific version or path, set `ZX_PAX=/path/to/the.pax.Z`. Set `GITHUB_TOKEN` if the API is rate-limited on a shared corporate IP.

---

## 0 · Add `zx` to your PATH (optional but recommended)

Run once to symlink the helper into `~/.local/bin` (created if absent):

```bash
.agents/skills/zowex-ssh/zx install          # -> ~/.local/bin/zx  (default)
.agents/skills/zowex-ssh/zx install ~/bin    # -> ~/bin/zx          (custom dir)
```

`zx install` prints an `export PATH=...` line if the target directory isn't already on your PATH — paste it into your shell profile (`~/.zshrc`, `~/.bashrc`, etc.).

After that, every command below can be called as `zx ...` directly. To remove the symlink: `zx uninstall`.

---

## 1 · One-time deploy

The easiest path — just run `zx deploy`:

```bash
zx deploy user@host [/remote/dir]
```

If `server.pax.Z` isn't present locally, `zx deploy` will:
1. Hit the GitHub releases API to find the latest release asset (`*.pax.Z`)
2. Show you the filename and destination, then download it automatically
3. Continue with the normal sftp → unpax → verify flow

The release asset may carry a version in its name (e.g. `zowex-0.7.0-server.pax.Z`); `zx` saves it as `server.pax.Z` at the `ZX_PAX` path.

**Manual / pinned-version deploy** — download the `.pax.Z` from [github.com/zowe/zowex/releases](https://github.com/zowe/zowex/releases), then:

```bash
ZX_PAX=/path/to/downloaded.pax.Z
zx deploy user@host [/remote/dir]
```

**Low-level steps** (if you need to do it by hand):

```bash
# 1a. push the bundle (binary mode is sftp default for `put`)
sftp -b - "$ZX_HOST" <<EOF
-mkdir $ZX_DIR
cd $ZX_DIR
put server.pax.Z
bye
EOF

# 1b. unpax on the host (extracts flat: ./zowex)
ssh "$ZX_HOST" "cd $ZX_DIR && pax -rvf server.pax.Z && chmod +x zowex"

ZX_BIN=$ZX_DIR/zowex

# 1c. verify
ssh "$ZX_HOST" "$ZX_BIN --help"
```

If `pax -rvf` fails on the `.Z`, do `uncompress server.pax.Z && pax -rvf server.pax`.

The CLI also exposes `console` / `tso` / `system` / `tool` subcommands — run `$ZX_BIN <cmd> --help` if you need one of those interactively. Only a subset is exposed via JSON-RPC (see §3). This CLI-only gap isn't limited to those four groups: `data-set copy` is CLI-only too (no `copyDataset` RPC — see §3's Datasets table). When in doubt whether a `zowex data-set`/`job`/`uss` verb has an RPC equivalent, check `$ZX_BIN data-set --help` against §3 rather than assuming parity.

---

## 2 · Talking to the server

`zowex server` reads **JSON-RPC 2.0** requests on **stdin** and writes JSON responses on **stdout**, one object per line. Auth is whatever SSH gave you — there is no in-band handshake.

**Wire behavior (verified against v0.6.0):**
- The server emits one **ready banner** line first: `{"status":"ready","message":"...","data":{"version":"..."}}`. Consume and discard it before reading responses.
- With the default worker pool, **responses can arrive out of order** (matched by `id`). For scripted/sequential use, start with `-w 1` to serialize. The `zx` helper does this.
- Unknown methods return `{"error":{"code":-32601,"message":"Unrecognized command <name>"}}`.
- Result shape: `{"id":N,"result":{"success":true, ...},"jsonrpc":"2.0"}`. Common result fields: `data` (b64 for `readDataset`/`readFile`/`readSpool`; **plain text** for `unixCommand`/`tsoCommand`), `items` (lists), `jobId`/`jobName` (`submitJcl`), `etag` (`readDataset`), `returnedRows`.

### 2a · The `zx` helper (preferred)

```bash
zx=.agents/skills/zowex-ssh/zx     # or alias/symlink it onto PATH

$zx deploy user@host [/remote/dir]   # one-time per host (idempotent); also saves config
$zx ds list "SYS1.*"                 # works immediately — auto one-shot
$zx job submit foo.jcl
$zx start                            # optional: persistent session = faster repeated calls
$zx job status JOB01234
$zx stop
```

`zx` remembers `host`+`bin` in `$ZX_STATE/config` (set by `deploy` or `zx use <host> <bin>`; overridable via `ZX_HOST`/`ZX_BIN` env). Every grouped command auto-detects whether a persistent session is live and falls back to a one-shot ssh if not.

**Command groups** (run `zx help` for the full list):

| group | subcommands |
|---|---|
| `zx ds` | `list <pat>` · `members <dsn>` · `read <dsn>` · `write <dsn> <file>` · `get`/`put` (member or whole PDS) · `create` · `delete` · `copy` · `rename` |
| `zx job` | `list [<owner> [<prefix>]]` · `submit <file>` · `status <id>` · `spools <id>` · `spool <id> <n>` · `jcl <id>` · `cancel`/`delete`/`hold`/`release <id>` |
| `zx uss` | `ls` · **`get`/`put`** (sftp, binary-safe) · `read`/`write` (RPC, text) · `rm` · `mkdir` · `mv` · `cp` · `chmod` · `chown` · `chtag` · `sh '<cmd>'` |
| `zx tso` | `'<cmd>'` |
| `zx system` | `apf` · `linklist` · `proclib` · `syslog` (RPC) · `parmlib` · `subsystems` · `symbol <s>` (CLI) |
| `zx tool` | `amblist <dsn> --cs '<stmts>'` · `run <pgm> [opts]` · `search <dsn> <str>` · `dynalloc` · `dsect` (all CLI) |
| `zx console` | `'<cmd>' [--cn <n>] [--timeout <s>] [--no-wait]` (CLI; needs APF — fails `Not authorized - 4` from a plain USS dir) |
| `zx rpc` | `<method> ['<params>']` — raw escape hatch |

**Output:** grouped commands pretty-print by default (lists → one per line, b64 → decoded, status → `key=value`). Add `-j`/`--json` anywhere (before or after the group) for raw JSON: `zx ds list "SYS1.*" -j` or `zx -j ds list "SYS1.*"`.

`zx info` shows config + whether a session is live. `zx reset` is a harder stop: kills the session, closes the ControlMaster socket, and removes the entire `$ZX_STATE` directory — useful when the state gets corrupted or you want a clean slate.

### 2b · One-shot

For a single call without the persistent session (note `-w 1`; first output line is the ready banner):

```bash
printf '%s\n' '{"jsonrpc":"2.0","id":1,"method":"listJobs","params":{"owner":"*","prefix":"XYZ*"}}' \
  | ssh "$ZX_HOST" "$ZX_BIN server -w 1" | sed 1d
```

### 2c · Isolated / parallel connections (second host without disturbing the first)

All `zx` state — config, persistent-session pid, FIFOs, and the SSH `ControlMaster` socket — lives
under `$ZX_STATE`, which defaults to **one shared directory per UID** (`/tmp/zx.$UID`). That means
`zx deploy`/`zx use` for a second host **overwrites** the first host's saved config, and — more
importantly — `_live()` (the check every grouped command uses to decide "route through the
persistent session or go one-shot") only checks *whether a pid exists*, not *which host it's
for*. If a persistent session is already live for host A and you need one-shot calls against host
B without touching it, exporting `ZX_HOST`/`ZX_BIN` alone is **not enough** — the dispatcher will
still shove your request down host A's session.

Fix: give the second host its own `ZX_STATE` so it never sees host A's pid file or socket:

```bash
export ZX_STATE=/tmp/zx-<label>.$UID     # short — see ControlPath length note below
export ZX_HOST=user@hostB
zx deploy "$ZX_HOST" /remote/dir          # writes config under the isolated ZX_STATE only
zx ds list "HLQ.*"                        # one-shot; host A's session/socket is untouched
```

If host B needs password auth and you can't leave an interactive prompt open, prime the
`ControlMaster` once with `sshpass` using **the same `ControlPath` pattern `zx` uses**, then let
`zx` reuse the socket silently for subsequent calls:

```bash
sshpass -p "$PASSWORD" ssh -o ControlMaster=auto -o ControlPath="$ZX_STATE/cm-%C" \
  -o ControlPersist=30m -T "$ZX_HOST" exit
```

Do this under the same isolated `$ZX_STATE` you'll use for the real `zx` calls — the `%C` token
resolves to the same socket path either way, so `zx`'s own `ssh` invocations find it already open
and skip the password prompt. Never run `zx start`/`zx stop`/`zx reset` in this workflow — those
only make sense for the single shared default state and `reset` will nuke whichever `$ZX_STATE`
is currently exported.

### Envelope

```json
{"jsonrpc":"2.0","method":"<name>","params":{...},"id":<int>}
```

- `id` must be unique per request within a session.
- All `data` fields (file/dataset content, JCL text) are **base64**.
- Responses are standard JSON-RPC 2.0: `{"jsonrpc":"2.0","id":N,"result":{...}}` or `{"...","error":{...}}`.

---

## 3 · Method reference

### Datasets

| method | params |
|---|---|
| `listDatasets` | `{"pattern":"HLQ.*"}` |
| `listDsMembers` | `{"dsname":"HLQ.PDS"}` |
| `readDataset` | `{"dsname":"HLQ.PS"}` · optional `"encoding":"ISO8859-1"` |
| `writeDataset` | `{"dsname":"HLQ.PS","data":"<b64>"}` · optional `"encoding"` |
| `createDataset` | `{"dsname":"HLQ.NEW","attributes":{"primary":10,"lrecl":80}}` |
| `deleteDataset` | `{"dsname":"HLQ.OLD"}` |
| `renameDataset` | `{"dsnameBefore":"A","dsnameAfter":"B"}` |

**No `copyDataset` RPC** — at least as of server v0.6.0, `copyDataset` isn't wired over JSON-RPC (returns `-32601 Unrecognized command`) even though it exists as a CLI verb. `zx ds copy` calls the CLI directly instead (`zowex data-set copy <src> <dst> [--ow|-r]`, via SSH passthrough — see §2's CLI-vs-RPC note). If you're issuing raw RPC via `zx rpc` or `zx -j ds copy`-style JSON, don't rely on `copyDataset` — shell out to the CLI form instead.

### Jobs

| method | params |
|---|---|
| `listJobs` | `{}` · optional `"owner"`, `"prefix"` |
| `getJobStatus` | `{"jobId":"JOB01234"}` |
| `listSpools` | `{"jobId":"JOB01234"}` |
| `readSpool` | `{"jobId":"JOB01234","spoolId":2}` |
| `getJcl` | `{"jobId":"JOB01234"}` |
| `submitJcl` | `{"jcl":"<b64 of JCL text>"}` |
| `submitJob` | `{"dsname":"HLQ.JCL(MBR)"}` |
| `submitUss` | `{"fspath":"/path/job.jcl"}` |
| `cancelJob` / `deleteJob` / `holdJob` / `releaseJob` | `{"jobId":"JOB01234"}` |

### USS

| method | params |
|---|---|
| `listFiles` | `{"fspath":"/u/x"}` |
| `readFile` | `{"fspath":"/u/x/f"}` · optional `"encoding"` |
| `writeFile` | `{"fspath":"/u/x/f","data":"<b64>"}` · optional `"encoding"` |
| `createFile` | `{"fspath":"/u/x/f"}` · add `"isDir":true` for mkdir |
| `deleteFile` | `{"fspath":"/u/x/f"}` |
| `moveFile` | `{"source":"...","target":"..."}` |
| `copyUss` | `{"srcFsPath":"...","dstFsPath":"..."}` |
| `chmodFile` | `{"fspath":"...","mode":"755"}` |
| `chownFile` | `{"fspath":"...","owner":"..."}` |
| `chtagFile` | `{"fspath":"...","tag":"UTF-8"}` |
| `unixCommand` | `{"commandText":"<shell cmd>"}` — `result.data` is **plain text** |

### TSO

| method | params |
|---|---|
| `tsoCommand` | `{"commandText":"<tso cmd>"}` — `result.data` is plain text |

`unixCommand` / `tsoCommand` are the escape hatches for anything not covered. There is **no JSON-RPC console method** (the `zowex console` CLI subcommand exists but isn't exposed over RPC) — use `unixCommand` with a host-side `opercmd`-equivalent if you need one, or fall back to `ssh "$ZX_HOST" "$ZX_BIN console issue ..."`.

---

## 4 · zowe-CLI → zx mapping

When porting a workflow that used the `zowe` CLI:

| `zowe` CLI | `zx` | underlying RPC method |
|---|---|---|
| `zowe files ls ds "PAT"` | `zx ds list "PAT"` | `listDatasets` |
| `zowe files ls am "DSN"` | `zx ds members "DSN"` | `listDsMembers` |
| `zowe files view ds "DSN"` | `zx ds read "DSN"` | `readDataset` (b64) |
| `zowe files upload ftds f "DSN"` | `zx ds write "DSN" f` | `writeDataset` |
| `zowe files delete ds "DSN" -f` | `zx ds delete "DSN"` | `deleteDataset` |
| `zowe jobs submit lf f.jcl` | `zx job submit f.jcl` | `submitJcl` (local file) |
| `zowe jobs submit ds "HLQ.JCL(MBR)"` | `zx job submit "HLQ.JCL(MBR)"` | `submitJob` (dataset) |
| `zowe jobs submit uss "/path/job.jcl"` | `zx job submit /path/job.jcl` | `submitUss` (USS path) |
| `zowe jobs view jsbj JOBID` | `zx job status JOBID` | `getJobStatus` → `.status`/`.retcode`/`.phaseName` |
| `zowe jobs list sfbj JOBID` | `zx job spools JOBID` | `listSpools` |
| `zowe jobs view sfbi JOBID N` | `zx job spool JOBID N` | `readSpool` (b64) |
| `zowe jobs cancel job JOBID` | `zx job cancel JOBID` | `cancelJob` |
| (AMBLIST batch job) | `zx tool amblist DSN --cs '<stmts>'` | CLI passthrough |
| (ISRSUPC batch job) | `zx tool search DSN "str" --parms anyc` | CLI passthrough |

Submit-then-poll pattern:

```bash
JID=$(zx job submit f.jcl | grep '^jobId=' | cut -d= -f2)
while [[ $(zx -j job status "$JID" | jq -r .result.status) == ACTIVE ]]; do sleep 2; done
zx job spools "$JID"          # list spool files with their IDs
zx job spool  "$JID" 2        # read spool file #2 (JESMSGLG, etc.)
```

---

## 5 · File transfer

| Command | What | Transport |
|---|---|---|
| `zx uss get <remote> [<local>]` | download one USS file (binary-safe) | sftp |
| `zx uss put <local> <remote>` | upload one USS file (binary-safe) | sftp |
| `zx ds get "<DSN(M)>" <file>` | download one PDS member | RPC `readDataset` (b64) |
| `zx ds get "<DSN>" <localdir>` | download **all** members of a PDS | loops `listDsMembers` + `readDataset` |
| `zx ds put <file> "<DSN(M)>"` | upload one file as a member | RPC `writeDataset` (b64) |
| `zx ds put <localdir> "<DSN>"` | upload directory → PDS (filename → MEMBER, ext stripped, upcased, 8-char) | loops `writeDataset` |

`ds get|put` go through base64-in-JSON, so use them for text members (REXX/JCL/parmlib/source). For load modules or anything large/binary, stage through USS:

```bash
zx uss sh 'cp -B "//'\''SYS1.LINKLIB(IEFBR14)'\''" /tmp/iefbr14.bin'
zx uss get /tmp/iefbr14.bin
```

For whole-PDS `ds get`, run `zx start` first — one persistent session is much faster than N one-shot SSH connects. `ds put` of a directory may hit zowex DEQ contention on rapid same-PDS writes; failures are reported per-member and the command exits non-zero if any failed.

---

## 6 · Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| password prompt on every `zx` call | `zx` already multiplexes its own ssh calls per `$ZX_STATE` (30min `ControlPersist`), so this is usually only about *manual/raw* `ssh` calls outside `zx`. Prefer `ssh-copy-id <host>` (a key). If the user specifically wants a long-lived multiplexed session across *all* ssh tools to that host, the fix is a host-wide, persistent change to `~/.ssh/config` (`ControlMaster auto` / `ControlPath` / `ControlPersist`) — it keeps an authenticated socket open for the persist duration, reusable by any local process as that user. That's a change outside this session's scope: confirm with the user and get their desired `ControlPersist` before editing `~/.ssh/config`, don't add it unprompted. |
| `mkdir: ... EDC5134I Function not implemented` on deploy | parent dir is an automount root — `zx deploy` now `cd`s to the parent first to trigger the mount; if it still fails, the parent genuinely doesn't exist |
| `pax: FSUM7108 cannot open` | wrong dir / no write perms — pick a different `ZX_DIR` |
| `zowex: FSUM7351 not found` | `ZX_BIN` path wrong — re-run the `find` from step 1b |
| `EDC5129I No such file or directory` on `zowex --help` | binary not tagged/executable — `chmod +x $ZX_BIN`; if it's a tag issue, `chtag -b $ZX_BIN` |
| server returns nothing then EOF | request wasn't newline-terminated, or JSON was malformed — `zx rpc` always appends `\n` |
| `CEE3501S module not found` | LE runtime not in LIBPATH — prefix server start with `export LIBPATH=$ZX_DIR/c/build-out:$LIBPATH;` |
| every method returns auth-style errors | the SSH user lacks the needed RACF access; zowex itself does no auth |
| `ControlPath too long ('...' >= 104 bytes)` | Unix domain socket path limit (macOS: 104 bytes) — `$ZX_STATE/cm-%C` overflowed it. Use a short `ZX_STATE`, e.g. `/tmp/zx-<label>.$UID`, not a long nested path like a session scratch dir |
| need password auth against a second host without disturbing an existing `zx` session/socket | give the second host its own `ZX_STATE` and prime its `ControlMaster` with `sshpass` — see §2c |

---

## 7 · Cleanup

```bash
zx stop                              # kill persistent session (ControlMaster socket kept for 30m)
zx reset                             # harder: also kills ControlMaster + wipes $ZX_STATE dir
ssh "$ZX_HOST" "rm -rf $ZX_DIR"     # remove the remote binary (only if you deployed it)
```

Only remove `$ZX_DIR` on the remote side if you created it for this session.