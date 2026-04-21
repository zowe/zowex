# Pong Server Demo

Demonstrates z/OS SSH communication with EBCDIC to ASCII conversion.

## Deploy & Build

```bash
npx tsx examples/deploy.ts <ssh-profile> <deploy-dir> pong-server
```

## Client

```bash
cd examples/pong-server && npm install
npx tsx client.ts <user>@<host> <deploy-dir>/examples/pong-server/server
```
