# Change Log

All notable changes to the Client code for "zowex-cli" are documented in this file.

Check [Keep a Changelog](http://keepachangelog.com/) for recommendations on how to structure this file.

## `0.4.0`

- Added an `--attributes` flag to list ISPF statistics for member attributes. [#630](https://github.com/zowe/zowex/issues/630)
- Added the `zssh uss copy` command to the CLI. [#379](https://github.com/zowe/zowex/pull/379).
- Updated `stream` fields to match new requirements from the SDK. [#548](https://github.com/zowe/zowex/issues/548)

## `0.3.0`

- Used new SDK groupings that align with zowex. [#807](https://github.com/zowe/zowex/pull/808)
- Added the `pattern` option to the `list data-set-members` command to filter the returned members. [#817](https://github.com/zowe/zowex/pull/817)

## `0.2.4`

- Added the `zssh rename data-set-member` command to the CLI. [#765] (https://github.com/zowe/zowex/pull/765).
- Added `+` suffix after volser for multi-volume data sets when listing data sets with attributes. [#782](https://github.com/zowe/zowex/pull/782)

## `0.2.3`

- Added the `zssh rename data-set` command to the CLI. [#376](https://github.com/zowe/zowex/issues/376).
- Added `owner` field to the table returned by the `zssh list jobs` and `zssh view job-status` commands. [#749](https://github.com/zowe/zowex/pull/749)
- Updated the `zssh server install` command to locate server PAX bundled in the SDK package. [#760](https://github.com/zowe/zowex/pull/760)
- The user is now warned if their data is truncated when uploading to a data set. Users can avoid truncation by keeping records within the maximum record length. [#751](https://github.com/zowe/zowex/pull/751)

## `0.2.1`

- When listing data sets with attributes, a comprehensive set is now retrieved that is similar to what ISPF displays. [#629](https://github.com/zowe/zowex/issues/629)

## `0.2.0`

- Updated `CliPromptApi` class to gracefully handle unused functions from the `AbstractConfigManager` class. [#605](https://github.com/zowe/zowex/pull/605)
- Added a `--depth` option to the `zssh list uss` command for listing USS directories recursively. [#575](https://github.com/zowe/zowex/pull/575)
- Added `zssh issue tso-command` command. [#595](https://github.com/zowe/zowex/pull/595)

## `0.1.10`

- Added a `storeServerPath` function as placeholder for future CLI functionality to persist deploy directory paths from user input. [#527](https://github.com/zowe/zowex/issues/527)

## `0.1.9`

- Added support for the `local-encoding` option to data set, USS file, and job file commands (view, download, upload) to specify the source encoding of content (defaults to UTF-8). [#511](https://github.com/zowe/zowex/issues/511)
- Added support for `--volume-serial` option when uploading/downloading data sets. [#439](https://github.com/zowe/zowex/issues/439)
- Fixed an issue where the `zssh config setup` command did not prompt the user for a password if the given private key was not recognized by the host. [#524](https://github.com/zowe/zowex/issues/524)
- Added a new prompt that shows if the user has an invalid private key on an existing profile when running the `zssh config setup` command. Now, if an invalid private key is detected, it is moved to a new comment in the JSON file and the user is given options to proceed. They can undo the comment action, delete the comment entirely, or preserve the comment and succeed with setup. [#524](https://github.com/zowe/zowex/issues/524)
- Added a `translateCliError` function to provide more user-friendly messages for an encountered error. If a user-friendly translation is unavailable, the original error is returned. [#533](https://github.com/zowe/zowex/pull/533)
- Updated the `zssh server install` command to display more user-friendly error messages if an issue was encountered during the installation process. [#228](https://github.com/zowe/zowex/issues/228)

## `0.1.5`

- Added support for progress messages for USS files downloaded and uploaded via the CLI plug-in. [#426](https://github.com/zowe/zowex/pull/426)
- Added support for listing the long format for USS files with the `long` request option. When provided, the CLI plug-in prints additional file attributes, including permissions, file tag, file size, number of links, owner, and group. [#346](https://github.com/zowe/zowex/issues/346)
- Added support for listing all USS files with the `all` request option. When provided, the CLI plug-in includes hidden files in the list response. [#421](https://github.com/zowe/zowex/pull/421)
- Added support for the `encoding` option to the `zssh upload uss` and the `zssh upload ds` commands. When the `encoding` option is provided, the command uses the option value as the target encoding for the uploaded content. [#427](https://github.com/zowe/zowex/issues/427)
- Added support for the `file` option to the `zssh download uss` and `zssh download ds` commands. When the `file` option is provided, the command uses the option value as the destination file path for the downloaded content. [#428](https://github.com/zowe/zowex/issues/428)
- Adopted streaming for commands that upload/download data sets and USS files. [#358](https://github.com/zowe/zowex/pull/358)

## `0.1.1`

- Fixed issue where a jobs list request returns unexpected results whenever a search query does not match any jobs. [#217](https://github.com/zowe/zowex/pull/217)

## `0.1.0`

- Added `zssh restore dataset` command. [#38](https://github.com/zowe/zowex/pull/38)
- Added `zssh view uss-file` command. [#38](https://github.com/zowe/zowex/pull/38)
- Added `zssh list spool-files` command. [#38](https://github.com/zowe/zowex/pull/38)
- Added support for cancelling jobs. [#138](https://github.com/zowe/zowex/pull/138)
- Added support for holding and releasing jobs. [#182](https://github.com/zowe/zowex/pull/182)
- Updated error handling for listing data sets. [#185](https://github.com/zowe/zowex/pull/185)
- Added `zssh submit uss-file|local-file|data-set` commands. [#184](https://github.com/zowe/zowex/pull/184)
- Added support for conflict detection through use of e-tags. When a data set or USS file is viewed, the e-tag is displayed to the user and can be passed for future write requests to prevent overwriting new changes on the target system. [#144](https://github.com/zowe/zowex/issues/144)

## [Unreleased]

- Initial release
