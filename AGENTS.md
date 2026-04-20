# AGENTS.md

## Project overview

Zowe Remote SSH (ZRS) is a full-stack solution used to interface with z/OS mainframe resources using the Secure Shell protocol.

Client-side components:
- `packages/cli`: Zowe CLI plug-in to enable ZRS mainframe interaction.
- `packages/vsce`: Zowe Explorer extender to enable ZRS mainframe interaction.
- `packages/sdk`: Node.js SDK used by the Zowe CLI plug-in and Zowe Explorer extender to communicate w/ ZRS server-side components.

Server-side components:
- `native/c`: Metal C and LE C/C++ code deployed and built on z/OS systems. Nucleus for mainframe interaction: handles commands, interprets requests, builds responses.
  - `zowex`: C++ binary which uses native APIs. Includes various CLI commands, alternative to ZOAU.
  - `zowex server`: CLI command in `zowex` that initializes a JSON-RPC server instance. Used to communicate with client-side SDK through shell stdin/stdout (e.g. `./zowex server`). Stream-capable read/write functionalities served through z/OS UNIX pipes and SFTP. 
- `native/python`: Proof-of-concept bindings for Python: powered by SWIG, wraps `.cpp` APIs, basic error handling.

## Build and test commands

Remote commands must be used to test server-side changes:
- `npm run z:rebuild` to deploy all files & build.
- `npm run z:upload <files...>` to deploy one or more files (useful for targeted changes).
- `npm run z:test` to build `native/c/test` and run native tests.
  - `npm run z:test <matcher>` to run tests w/ names matching the given string 
- `npm run z:all` rebuilds native code, runs tests, and packages artifacts for client components.
- `npm run z:artifacts` prepares server-side artifacts on the current z/OS system.
- `npm run z:package` prepares the server-side artifacts and downloads them to your local machine for testing.

The following commands are used to evaluate client-side components:
- `npm run build` builds `packages/cli`, `packages/sdk`, `packages/vsce`.
- `npm run build:types` builds matching C++ types that correspond to expected request & response types defined in `packages/sdk/src/doc/rpc`. Used when creating new response and request functions from end-to-end.
- `npm run lint` runs Biome formatter/linter on client-side code.
- `npm run test` runs client-side component tests using Vitest.

`npm run license` applies required license headers to new source files in the repository.

## Code style guidelines

- Metal C source code must use `.c` and `.h`. Compiled using `xlc` compiler from z/OS XL C/C++.
- C++ code should adhere to C++-17 standards. Compiled using Clang-based `ibm-clang` compiler from Open XL C/C++.
- Number of function parameters: Keep parameters to 3 or fewer for new functions. Maximum of 5 parameters. 
- Class naming is `PascalCase` regardless of language.
- Variable naming: `camelCase` for client-side Node.js/TypeScript components, `snake_case` for server-side.

## Testing instructions

- If asked to prepare a local build with end-to-end changes, first prepare the server-side artifacts, then pull them down and build the client pieces: `npm run z:rebuild && npm run z:package && npm run build`

## PR instructions

- Consider using Conventional Commits and isolating file changes by scope.
- Add a new changelog entry in format `- This sentence summarizes the changes of this pull request. [#Issue Number](https://github.com/zowe/zowex/issue/XXXX OR matching PR link)` in the appropriate changelog file:
  - Client-side changes: `CHANGELOG.md` for corresponding package.
  - Server-side changes: `native/CHANGELOG.md`
- `npm run lint` and `npm run build` must pass before PR is ready.

## Security considerations

- Never log user credentials or passwords to the console.
- Never read `config.yaml` or user's Zowe config for more context (`**/zowe.config.json`).
- Always sanitize user inputs before passing to native shell commands.