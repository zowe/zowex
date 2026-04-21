# SSH Benchmark

Minimal benchmarks that measure SSH connection and command execution time.

| Benchmark | Wrapper | File |
|-----------|---------|------|
| Shell | `ssh` CLI | `bench.sh` |
| Node.js | `ssh2` or native `russh` | `bench.mjs` |

## What is measured

All benchmarks measure the same three things:

1. **Phase 1** — Connect + authenticate + run `ls` + disconnect, repeated N times (connection latency)
2. **Phase 2** — Single connection, run 10 `echo hello` commands sequentially (command throughput)
3. **Phase 3** — Single connection, exec `~/pong-server.sh`, send 20 `ping` messages and measure round-trip latency to receive `pong` (persistent channel latency)

The shell script only measures phase 1.

## Setup for Phase 3

Upload `pong-server.sh` to the remote host and make it executable:

```bash
scp examples/ssh-benchmark/pong-server.sh user@host:~/pong-server.sh
ssh user@host chmod +x ~/pong-server.sh
```

## Prerequisites

- SSH access to a remote host with key-based authentication
- Node.js 18+ and `ssh2` package (`npm install ssh2`)

## Usage

All benchmarks use **`user@host`** format and auto-detect a private key from `~/.ssh/` (`id_ed25519`, `id_rsa`, `id_ecdsa` — first found wins). You can override the key path explicitly.

### Shell

```bash
./bench.sh user@host [iterations] [port]
```

Uses `ssh` directly, so it respects your SSH agent and `~/.ssh/config`.

### Node.js

```bash
npm install ssh2
node bench.mjs user@host [node|native] [port] [key_path]
```

The second argument selects the SSH client — `node` (default) uses the `ssh2` npm package, `native` uses the `russh`-backed native client.

## Example output

```
phase 1 — connect + ls + disconnect (10 iterations):
  run  1: 142 ms
  run  2: 128 ms
  ...
  total: 1350 ms  avg: 135 ms

phase 2 — 10x echo (single connection):
  total: 387 ms  avg: 38 ms/cmd

phase 3 — 20x ping/pong (persistent exec channel):
  total: 230 ms  avg: 11 ms/rtt
```
