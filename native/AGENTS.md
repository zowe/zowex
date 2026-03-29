# Native Development — AI Agent Guide

C/C++ code that compiles and runs on z/OS USS. The `zowex` binary serves as both a CLI tool and an RPC server (`zowex server`).

## z/OS Process Creation

On z/OS USS, `fork()` creates a **new address space** — expensive and slow. Prefer `spawn()` from `<spawn.h>` with `_BPX_SHAREAS=YES` to run child processes as a new TCB in the **same address space**.

- Use `zut_spawn_shell_command()` for shell commands (uses `spawn()` + `_BPX_SHAREAS=YES`)
- `zut_run_program()` exists but uses `fork()`/`execvp()` — avoid for new command execution paths

**Identity-changing commands (newgrp, su, sg):** Commands that change real/effective UID or GID are not supported in a shared address space ([IBM newgrp doc](https://www.ibm.com/docs/en/zos/3.2.0?topic=descriptions-newgrp-change-new-group)). `zut_spawn_shell_command()` detects when the user's command is one of `newgrp`, `su`, or `sg` (by parsing the first token) and in that case does **not** pass `_BPX_SHAREAS=YES` to the child, so the child runs in its own address space. To support additional such commands, add the command basename to `ZUT_NOSHAREAS_COMMANDS` in `zut.cpp`.

## Adding a New Native Command

1. **Handler**: Implement in `c/commands/<domain>.cpp`, declare in the matching `.hpp`
2. **CLI registration**: Add subcommand in the `register_commands()` function of the domain's `.cpp`
3. **RPC registration**: Register in `c/server/rpc_commands.cpp` using `CommandBuilder` with `.validate<Req, Resp>()`, `.rename_arg()`, and `.read_stdout()` as needed
4. **SDK types**: Define request/response interfaces in `packages/sdk/src/doc/rpc/<domain>.ts`
5. **SDK binding**: Wire up in `packages/sdk/src/RpcClientApi.ts`

## Build & Test Workflow

All `npm run z:*` commands work from the repo root and target the remote z/OS system configured in `config.yaml`.

| Command                    | Purpose                                  |
| -------------------------- | ---------------------------------------- |
| `npm run z:upload`         | Upload source files to z/OS              |
| `npm run z:build`          | Compile on z/OS                          |
| `npm run z:rebuild`        | Clean + build                            |
| `npm run z:test`           | Build all + run native tests             |
| `npm run z:make all tests` | Build everything including test binaries |

After modifying native source or test files, run `npm run z:upload` before `npm run z:test` to ensure the latest code is compiled on z/OS.

The `zowex` binary on z/OS is at `<deployDir>/c/build-out/zowex` (deployDir is in `config.yaml`, typically `~/zowe-native-proto`).

## Checking for Compiler Warnings

Compiler warnings from z/OS builds are now surfaced by the build scripts. To check for warnings:

```sh
npm run z:rebuild          # main binary — shows warnings from native/c
npm run z:make tests       # test binaries — shows warnings from native/c/test
```

Warnings appear on stderr after the build step completes.

### Recognizing pre-existing warnings

Pre-existing warnings come from files that were **not changed by the current PR**. Cross-reference warnings with the PR's changed files:

```sh
git diff --name-only main..HEAD
```

A warning is **pre-existing** if its source file does not appear in that list. A warning is **new** (must be fixed) if its source file was added or modified by the PR.

## Logging

Use `ZLogger` macros (`ZLOG_DEBUG`, `ZLOG_INFO`, `ZLOG_WARN`, `ZLOG_ERROR`) for all diagnostic output in native C++ source and test files. Never use `std::fprintf(stderr, ...)` or `std::fflush(stderr)` for logging.

- Include `"zlogger.hpp"` (or `"../zlogger.hpp"` from the `test/` subdirectory) where needed.
- Choose the level by severity: `ZLOG_DEBUG` for trace/progress messages, `ZLOG_WARN` for unexpected-but-recoverable conditions, `ZLOG_ERROR` for failures.
- Macros are no-ops when `ZLOG_ENABLE` is not defined, and are gated by `ZOWEX_LOG_LEVEL` at runtime (default: INFO, so DEBUG messages are silent unless the env var is set).

### Checking for forgotten fprintf debug statements

Before finalizing a PR, verify no raw `fprintf(stderr` calls were introduced in any changed native file:

```sh
git diff main..HEAD -- 'native/c/**/*.cpp' 'native/c/**/*.hpp' | grep '^+.*fprintf'
```

Any hit that is not a removal (i.e. lines starting with `+` rather than `-`) must be converted to the appropriate `ZLOG_*` macro.

## Test Framework

- CLI tests: `c/test/zowex.<domain>.test.cpp` — use `execute_command_with_output()`
- RPC server tests: `c/test/zowex.server.test.cpp` — use `start_server()`, `write_to_server()`, `read_line_from_server()`
- SDK tests: `packages/sdk/tests/` — use `vitest` with mocked SSH layer
