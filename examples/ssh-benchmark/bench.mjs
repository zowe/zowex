#!/usr/bin/env node
/**
 * SSH benchmark — measures connection + command execution time.
 * Phase 1: Connect + run "ls" + disconnect, repeated N times (connection latency)
 * Phase 2: Single connection, run 10 "echo hello" commands (command throughput)
 * Phase 3: Single connection, exec pong-server, 20x ping/pong (persistent channel latency)
 *
 * Usage: node bench.mjs user@host [node|native] [port] [key_path]
 */

import { createRequire } from "node:module";
import { readFileSync, existsSync } from "node:fs";
import { homedir } from "node:os";
import { join } from "node:path";
import { performance } from "node:perf_hooks";

const require = createRequire(import.meta.url);
const { createClient } = require("../../packages/sdk/lib/ssh-rs/index.js");

const [dest, clientArg, portArg, keyArg] = process.argv.slice(2);
if (!dest || !dest.includes("@")) {
  console.error("Usage: node bench.mjs user@host [node|native] [port] [key_path]");
  process.exit(1);
}

const [user, host] = dest.split("@");
const useNative = clientArg === "native";
const port = Number(portArg) || 22;
const iterations = 10;

console.log(`client: ${useNative ? "native (russh)" : "node (ssh2)"}`);

function findPrivateKey(explicit) {
  if (explicit) return readFileSync(explicit);
  const sshDir = join(homedir(), ".ssh");
  for (const name of ["id_ed25519", "id_rsa", "id_ecdsa"]) {
    const p = join(sshDir, name);
    if (existsSync(p)) return readFileSync(p);
  }
  return undefined;
}

const privateKey = findPrivateKey(keyArg);
if (!privateKey) {
  console.error("No private key found in ~/.ssh — pass one explicitly as the 4th argument");
  process.exit(1);
}

function connect() {
  return new Promise((resolve, reject) => {
    const conn = createClient(useNative);
    conn
      .on("ready", () => resolve(conn))
      .on("error", reject)
      .connect({ host, port, username: user, privateKey });
  });
}

function execCommand(conn, cmd) {
  return new Promise((resolve, reject) => {
    conn.exec(cmd, (err, stream) => {
      if (err) return reject(err);
      let output = "";
      stream.stdout.on("data", (data) => { output += data.toString(); });
      stream.stdout.on("close", () => resolve(output));
      stream.stderr.resume();
    });
  });
}

function disconnect(conn) {
  return new Promise((resolve) => {
    conn.on("close", resolve);
    conn.end();
  });
}

// Phase 1: connect + "ls" + disconnect, repeated N times
console.log(`\nphase 1 — connect + ls + disconnect (${iterations} iterations):`);
let totalMs = 0;

for (let i = 1; i <= iterations; i++) {
  const t0 = performance.now();
  const conn = await connect();
  await execCommand(conn, "ls");
  await disconnect(conn);
  const elapsed = performance.now() - t0;
  totalMs += elapsed;
  console.log(`  run ${String(i).padStart(2)}: ${Math.round(elapsed)} ms`);
}

const avg = Math.round(totalMs / iterations);
console.log(`  total: ${Math.round(totalMs)} ms  avg: ${avg} ms`);

// Phase 2: single connection, 10x "echo hello"
const ECHO_COUNT = 10;
const conn = await connect();

const t1 = performance.now();
for (let i = 0; i < ECHO_COUNT; i++) {
  await execCommand(conn, "echo hello");
}
const echoMs = performance.now() - t1;

await disconnect(conn);

console.log();
console.log(`phase 2 — ${ECHO_COUNT}x echo (single connection):`);
console.log(`  total: ${Math.round(echoMs)} ms  avg: ${Math.round(echoMs / ECHO_COUNT)} ms/cmd`);

// Phase 3: exec pong-server, measure ping/pong round-trip latency
const PING_COUNT = 20;
const pingConn = await connect();

const pongStream = await new Promise((resolve, reject) => {
  pingConn.exec("~/pong-server.sh", (err, stream) => {
    if (err) return reject(err);
    stream.stderr.resume();
    resolve(stream);
  });
});

const pingLatencies = [];
for (let i = 0; i < PING_COUNT; i++) {
  const t = performance.now();
  pongStream.stdin.write("ping\n");
  await new Promise((resolve) => {
    const onData = (data) => {
      if (data.toString().includes("pong")) {
        pongStream.stdout.removeListener("data", onData);
        resolve();
      }
    };
    pongStream.stdout.on("data", onData);
  });
  pingLatencies.push(performance.now() - t);
}

pongStream.stdin.end();
await disconnect(pingConn);

const totalPing = pingLatencies.reduce((a, b) => a + b, 0);
console.log();
console.log(`phase 3 — ${PING_COUNT}x ping/pong (persistent exec channel):`);
console.log(`  total: ${Math.round(totalPing)} ms  avg: ${Math.round(totalPing / PING_COUNT)} ms/rtt`);
