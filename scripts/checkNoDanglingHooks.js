/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 */

// Rejects ztest afterAll() hooks that reference suite-lambda locals by reference.
//
// describe() invokes afterAll hooks *after* the suite lambda returns -- see the execute_hooks call
// in ztest.hpp, which runs once suite() is already off the stack. Variables declared directly in
// the suite body are destroyed at that point, so a hook holding a reference to one reads freed
// stack memory. A captured std::string typically reads back empty, which silently turns a cleanup
// step into a no-op: the test still passes while leaving data sets behind on the system.
//
// beforeAll / beforeEach / afterEach are unaffected -- they run from inside it(), while the suite
// lambda is still on the stack.
//
// Only variables declared *directly in the suite body* are reported. Locals of the enclosing
// function (a common and safe pattern, e.g. a created_dsns vector owned by zds_tests()) outlive
// describe() and are fine, as are statics.
//
// Fix by capturing what the hook needs by copy -- afterAll([dsn]() { ... }) instead of
// afterAll([&]() { ... }) -- and drop captures the hook body never uses.

const fs = require("node:fs");
const path = require("node:path");

const ROOT = path.resolve(__dirname, "../native/c/test");
const EXTENSIONS = new Set([".cpp", ".hpp"]);

function collect(dir, out) {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
        const full = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            collect(full, out);
        } else if (EXTENSIONS.has(path.extname(entry.name))) {
            out.push(full);
        }
    }
    return out;
}

// Blank out comments and string/char literals so brace matching is not thrown off by punctuation
// inside them. Replaced in place with spaces to keep every offset and line number intact.
function blankNonCode(text) {
    const out = text.split("");
    const blank = (start, end) => {
        for (let i = start; i < end && i < out.length; i++) {
            if (out[i] !== "\n") out[i] = " ";
        }
    };
    for (let i = 0; i < text.length; i++) {
        const two = text.slice(i, i + 2);
        if (two === "//") {
            let end = text.indexOf("\n", i);
            if (end === -1) end = text.length;
            blank(i, end);
            i = end;
        } else if (two === "/*") {
            const end = text.indexOf("*/", i + 2);
            const stop = end === -1 ? text.length : end + 2;
            blank(i, stop);
            i = stop;
        } else if (text[i] === '"' || text[i] === "'") {
            const quote = text[i];
            let j = i + 1;
            while (j < text.length && text[j] !== quote) {
                if (text[j] === "\\") j++;
                j++;
            }
            blank(i + 1, j);
            i = j;
        }
    }
    return out.join("");
}

function matchBrace(text, openIdx) {
    let depth = 0;
    for (let i = openIdx; i < text.length; i++) {
        if (text[i] === "{") depth++;
        else if (text[i] === "}" && --depth === 0) return i;
    }
    return -1;
}

// Body of the lambda that starts at the first '{' at or after `from`.
function lambdaBody(text, from) {
    const open = text.indexOf("{", from);
    if (open === -1) return null;
    const close = matchBrace(text, open);
    return close === -1 ? null : { open, close };
}

// Variables declared directly in `body` -- brace depth 0 and paren depth 0 relative to it. Anything
// nested inside a hook/it lambda or a call argument belongs to another scope and is skipped.
function suiteLocals(text, body) {
    let braces = 0;
    let parens = 0;
    let top = "";
    for (let i = body.open + 1; i < body.close; i++) {
        const c = text[i];
        if (c === "{") braces++;
        else if (c === "}") braces--;
        else if (c === "(") parens++;
        else if (c === ")") parens--;
        else if (braces === 0 && parens === 0) top += c;
    }

    const names = new Set();
    const DECL = /^\s*((?:const|constexpr|static|volatile|unsigned|signed)\s+)*([A-Za-z_][\w:]*(?:\s*<[^<>]*>)?|auto)\s*[*&]?\s+([A-Za-z_]\w*)\s*(?:=|$)/;
    for (const statement of top.split(";")) {
        const match = statement.match(DECL);
        // Statics have static storage duration, so a reference to one stays valid.
        if (match && !/\bstatic\b/.test(match[1] ?? "")) names.add(match[3]);
    }
    return names;
}

const findings = [];
for (const file of collect(ROOT, [])) {
    const raw = fs.readFileSync(file, "utf8");
    const text = blankNonCode(raw);

    // Innermost enclosing describe() suite body for a given offset.
    const suites = [];
    for (const match of text.matchAll(/\bdescribe\s*\(/g)) {
        const body = lambdaBody(text, match.index);
        if (body) suites.push(body);
    }
    const enclosingSuite = (idx) =>
        suites
            .filter((s) => s.open < idx && idx < s.close)
            .sort((a, b) => b.open - a.open)[0];

    for (const match of text.matchAll(/\bafterAll\s*\(\s*\[([^\]]*)\]/g)) {
        const captures = match[1];
        if (!captures.includes("&")) continue;

        const suite = enclosingSuite(match.index);
        if (!suite) continue;
        const hook = lambdaBody(text, match.index + match[0].length);
        if (!hook) continue;

        const locals = suiteLocals(text, suite);
        const hookBody = text.slice(hook.open, hook.close + 1);
        const byRefDefault = /^\s*&/.test(captures);

        const dangling = [...locals].filter((name) =>
            byRefDefault
                // [&] grabs whatever the body actually touches.
                ? new RegExp(`\\b${name}\\b`).test(hookBody)
                // Otherwise only the explicitly by-reference names matter.
                : new RegExp(`&\\s*${name}\\b`).test(captures),
        );
        if (dangling.length === 0) continue;

        const line = raw.slice(0, match.index).split(/\r?\n/).length;
        findings.push(
            `${path.relative(process.cwd(), file)}:${line}: afterAll([${captures}]) holds suite-lambda local(s) by reference: ${dangling.join(", ")}`,
        );
    }
}

if (findings.length > 0) {
    console.error("afterAll hook(s) referencing destroyed suite-lambda locals:\n");
    for (const finding of findings) console.error(`  ${finding}`);
    console.error(
        [
            "",
            "describe() runs afterAll hooks after the suite lambda has returned, so variables declared",
            "in the suite body are already destroyed. Reading one through a captured reference is",
            "undefined behavior; a std::string usually reads back empty, silently turning cleanup into",
            "a no-op that leaves resources behind while the test still passes.",
            "",
            "Capture by value instead -- afterAll([dsn]() { ... }) -- and drop captures the body does",
            "not use. beforeAll / beforeEach / afterEach run inside the suite lambda and are unaffected.",
            "",
        ].join("\n"),
    );
    process.exit(1);
}

console.log("No afterAll hooks referencing destroyed suite-lambda locals.");
