# Client Design

## SDK Package

The SDK defines a `ZSshClient` class with a `request` method that communicates with the [server component](../backend/server/architecture.md) over SSH.

For each command supported by the server component, a `Request` and `Response` type is defined in the `doc` folder. For example, to list data sets:

```typescript
export namespace ListDatasets {
  export interface Request extends IRpcRequest {
    command: "listDatasets";
    pattern: string;
    attributes?: boolean;
  }

  export interface Response extends IRpcResponse {
    items: Dataset[];
    returnedRows: number;
  }
}
```

Once these interfaces are defined, the command can be invoked using the generic method `ZSshClient.request`:

```typescript
using client = await ZSshClient.create(session);
const request: ListDatasets.Request = {
  command: "listDatasets",
  pattern: "IBMUSER.*",
};
const response = await client.request<ListDatasets.Response>(request);
```

Each command is also exposed through the `AbstractRpcClient` class, which makes it more convenient and concise to invoke commands. For example, to list data sets there is a `listDatasets` method defined within the `ds` object:

```typescript
  public get ds() {
    return {
      listDatasets: (request: Omit<ListDatasets.Request, "command">): Promise<ListDatasets.Response> =>
        this.request({ command: "listDatasets", ...request }),
      ...
    };
  }
```

Now the example of invoking a command can be simplified as follows:

```typescript
using client = await ZSshClient.create(session);
const response = await client.ds.listDatasets({ pattern: "IBMUSER.*" });
```

> [!NOTE]
> To transmit binary data, it must be Base64 encoded since the z/OS OpenSSH server does not support sending raw bytes without codepage conversion. Use the methods `B64String.encode` before sending binary data and `B64String.decode` after receiving it.

## CLI Plug-in

The Zowe CLI plug-in harnesses the SDK to access mainframe resources over SSH. The `src` directory is structured as follows:

- `imperative.ts` - Imperative configuration for the plug-in
- `SshBaseHandler.ts` - Base command handler that creates an SSH session and processes command output
- `**/*.definition.ts` - Definitions of commands and command groups
- `**/*.handler.ts` - Command handlers that implement functionality using the SDK

## VS Code Extension

This extension replicates the core functionality of Zowe Explorer using the SSH backend. The `src` directory is structured as follows:

- `extension.ts` - Main entry point of the extension that registers VS Code commands and loads SSH profiles in Zowe Explorer
- `SshClientCache.ts` - Manages persistent SSH sessions for multiple z/OS servers so they can be reused for subsequent commands
- `SshConfigUtils.ts` - Defines utility methods to interact with VS Code UI such as prompting for SSH profile
- `api/Ssh*Api.ts` - Classes that extend the Zowe Explorer API to implement functionality for the Data Sets, USS, and Jobs trees
