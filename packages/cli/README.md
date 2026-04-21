# Zowe Remote SSH plug-in for Zowe CLI

The Zowe Remote SSH plug-in for Zowe CLI enables developers to work with mainframe resources over SSH from their command line.

## Features

- **Dataset Operations**: Create, read, write, and delete datasets
- **USS File Access**: Create, read, write, and delete files and directories in Unix System Services
- **Job Management**: Submit, view, cancel, hold, and release jobs on z/OS

## Minimum Requirements

- Zowe CLI v8.0.0 (or newer)

## Installation

Access the latest version of the CLI plug-in from the GitHub Releases page.

Install the CLI plug-in from the tarball:

```
zowe plugins install zowex-cli-*.tgz
```

Once complete, the Zowe CLI plug-in is installed and ready to use.

## Usage

Run the `zowe plugins show-first-steps zowex-cli` command to see the first steps for using the plug-in.

## Building from source

1. From the root of this repository, run `npm install` to install all the dependencies
2. Change to the `packages/cli` folder and run `npm run build` to build the plug-in
3. `npm run package` packages the plug-in

The plug-in is created and saved in the `dist` folder.
Install the plug-in by running the following Zowe CLI command:

```
zowe plugins install dist/zowex-cli-*.tgz
```

Zowe Remote SSH plug-in commands are accessible through the `zowe zssh` command group.

```
zowe zssh --help
```

