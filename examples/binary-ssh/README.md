# Binary SSH Demo

Demonstrates bidirectional transfer of raw bytes over a z/OS SSH connection.

## Deploy & Build

```bash
npx tsx examples/deploy.ts <ssh-profile> <deploy-dir> binary-ssh
```

## Client

```bash
cd examples/binary-ssh && npm install
npx tsx client.ts <user>@<host> <deploy-dir>/examples/binary-ssh/server
```

## Transfer Benchmarks

The `tests/` directory contains base64, base85, and raw binary transfer test binaries.
See `tests/setup.sh` to generate test resources and configure the benchmarks.
