# API Matrix

### Legend

- ✅ Supported
- 🚧 Partially supported
- ❌ Not supported
- ➖ Not applicable
- _italic_ Target for MVP

## Data Sets

| Operation                | z/OSMF          | FTP | Backend         | Middleware | SDK | CLI | VSCE | Python |
| ------------------------ | --------------- | --- | --------------- | ---------- | --- | --- | ---- | ------ |
| _List data sets_         | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _List data set members_  | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Read data set_          | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Read data set member_   | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Write data set_         | ✅              | ✅  | 🚧 <sup>1</sup> | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Write data set member_  | ✅              | ✅  | 🚧 <sup>1</sup> | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Create data set_        | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Create data set member_ | ✅              | ✅  | 🚧 <sup>2</sup> | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Delete data set_        | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Delete data set member_ | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| Recall data set          | 🚧 <sup>3</sup> | ❌  | ✅              | ✅         | ✅  | ✅  | ✅   | ❌     |
| Migrate data set         | ✅              | ❌  | ❌              | ❌         | ❌  | ❌  | ❌   | ❌     |
| Delete migrated data set | ✅              | ❌  | ❌              | ❌         | ❌  | ❌  | ❌   | ❌     |
| Rename data set          | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ❌     |
| Rename data set member   | ✅              | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ❌     |
| Copy data set            | ✅              | ❌  | ✅              | ❌         | ❌  | ❌  | ❌   | ❌     |
| Compress data set        | ➖              | ❌  | ✅              | ❌         | ❌  | ❌  | ❌   | ❌     |
| Search data sets         | 🚧 <sup>4</sup> | ❌  | 🚧 <sup>4</sup> | ❌         | ✅  | ❌  | ❌   | ❌     |
| Invoke AMS (VSAM)        | ✅              | ❌  | ❌              | ❌         | ❌  | ❌  | ➖   | ❌     |

1. RECFM=U (undefined record format) data sets are read-only; write attempts return an error.
2. If the member already exists, this operation causes member contents to be overwritten.
3. Does not support some migration utilities like CA Disk.
4. Limited options compared to ISPF `srchfor`.

## USS Files

| Operation                | z/OSMF | FTP | Backend         | Middleware | SDK | CLI | VSCE | Python |
| ------------------------ | ------ | --- | --------------- | ---------- | --- | --- | ---- | ------ |
| _List files/directories_ | ✅     | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Read USS file_          | ✅     | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Write USS file_         | ✅     | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Create file/directory_  | ✅     | ✅  | 🚧 <sup>1</sup> | ✅         | ✅  | ✅  | ✅   | ✅     |
| _Delete file/directory_  | ✅     | ✅  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| Copy file/directory      | ✅     | ❌  | ✅              | ✅         | ✅  | ✅  | ✅   | ❌     |
| Move file/directory      | ✅     | ✅  | ✅              | ✅         | ✅  | ❌  | ✅   | ✅     |
| _`chmod` file/directory_ | ✅     | ❌  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _`chown` file/directory_ | ✅     | ❌  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| _`chtag` USS file_       | ✅     | ❌  | ✅              | ✅         | ✅  | ✅  | ✅   | ✅     |
| Issue UNIX command       | ✅     | ✅  | ✅              | ❌         | ✅  | ❌  | ❌   | ❌     |
| Invoke `extattr`         | ✅     | ❌  | ❌              | ❌         | ❌  | ➖  | ➖   | ❌     |
| Get ACL entries          | ✅     | ❌  | ❌              | ❌         | ❌  | ➖  | ➖   | ❌     |
| Set ACL entries          | ✅     | ❌  | ❌              | ❌         | ❌  | ➖  | ➖   | ❌     |
| Link file/directory      | ✅     | ❌  | ❌              | ❌         | ❌  | ➖  | ➖   | ❌     |
| Unlink file/directory    | ✅     | ❌  | ❌              | ❌         | ❌  | ➖  | ➖   | ❌     |
| List z/OS file systems   | ✅     | ❌  | ❌              | ❌         | ❌  | ❌  | ➖   | ❌     |
| Create z/OS file system  | ✅     | ❌  | ❌              | ❌         | ❌  | ❌  | ➖   | ❌     |
| Delete z/OS file system  | ✅     | ❌  | ❌              | ❌         | ❌  | ❌  | ➖   | ❌     |
| Mount file system        | ✅     | ❌  | ❌              | ❌         | ❌  | ❌  | ➖   | ❌     |
| Unmount file system      | ✅     | ❌  | ❌              | ❌         | ❌  | ❌  | ➖   | ❌     |

1. Recursive option is not supported.

## Jobs

| Operation          | z/OSMF | FTP | Backend | Middleware | SDK | CLI | VSCE            | Python |
| ------------------ | ------ | --- | ------- | ---------- | --- | --- | --------------- | ------ |
| _Get job status_   | ✅     | ✅  | ✅      | ✅         | ✅  | ✅  | ✅              | ✅     |
| _List jobs_        | ✅     | ✅  | ✅      | ✅         | ✅  | ✅  | ✅              | ✅     |
| _List spool files_ | ✅     | ✅  | ✅      | ✅         | ✅  | ✅  | ✅              | ✅     |
| _Read spool file_  | ✅     | ✅  | ✅      | ✅         | ✅  | ✅  | ✅              | ✅     |
| _Get job JCL_      | ✅     | ❌  | ✅      | ✅         | ✅  | ✅  | ✅              | ✅     |
| _Submit job_       | ✅     | ✅  | ✅      | ✅         | ✅  | ✅  | ✅              | ✅     |
| _Delete job_       | ✅     | ✅  | ✅      | ✅         | ✅  | ✅  | ✅              | ✅     |
| Cancel job         | ✅     | ❌  | ✅      | ✅         | ✅  | ✅  | ✅ <sup>1</sup> | ❌     |
| Hold job           | ✅     | ❌  | ✅      | ✅         | ✅  | ✅  | ➖ <sup>2</sup> | ❌     |
| Release job        | ✅     | ❌  | ✅      | ✅         | ✅  | ✅  | ➖ <sup>2</sup> | ❌     |
| Change job class   | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ➖              | ❌     |

1. Does not support force, restart, dump, or purge.
2. Zowe Explorer does not support the Job Hold/Release operation.

## Console

| Operation                | z/OSMF | FTP | Backend         | Middleware | SDK | CLI | VSCE | Python |
| ------------------------ | ------ | --- | --------------- | ---------- | --- | --- | ---- | ------ |
| Issue MVS command        | ✅     | ❌  | ✅ <sup>1</sup> | ❌         | ✅  | ❌  | ❌   | ❌     |
| Get MVS command response | ✅     | ❌  | ❌              | ❌         | ❌  | ❌  | ❌   | ❌     |
| Get MVS detect result    | ✅     | ❌  | ❌              | ❌         | ❌  | ➖  | ➖   | ❌     |

1. Requires APF authorization.
<!-- 2. You must manually deploy/symlink `zoweax` -->

## TSO

| Operation                | z/OSMF | FTP | Backend | Middleware | SDK | CLI | VSCE | Python |
| ------------------------ | ------ | --- | ------- | ---------- | --- | --- | ---- | ------ |
| Start TSO address space  | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ❌   | ❌     |
| Start TSO app            | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ➖   | ❌     |
| Issue TSO command        | ✅     | ❌  | ✅      | ✅         | ✅  | ✅  | ✅   | ❌     |
| Get TSO command response | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ❌   | ❌     |
| Send TSO message         | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ❌   | ❌     |
| Send TSO app message     | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ➖   | ❌     |
| Ping TSO address space   | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ❌   | ❌     |
| Receive TSO message      | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ❌   | ❌     |
| Receive TSO app message  | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ➖   | ❌     |
| Stop TSO address space   | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ❌   | ❌     |

## System

| Operation       | z/OSMF | FTP | Backend | Middleware | SDK | CLI | VSCE | Python |
| --------------- | ------ | --- | ------- | ---------- | --- | --- | ---- | ------ |
| Display symbol  | ❌     | ❌  | ✅      | ❌         | ❌  | ❌  | ➖   | ❌     |
| List parmlib    | ❌     | ❌  | ✅      | ❌         | ❌  | ❌  | ➖   | ❌     |
| List proclib    | ❌     | ❌  | ✅      | ❌         | ❌  | ❌  | ➖   | ❌     |
| List subsystems | ❌     | ❌  | ✅      | ❌         | ❌  | ❌  | ➖   | ❌     |

## Other

| Operation            | z/OSMF | FTP | Backend | Middleware | SDK | CLI | VSCE | Python |
| -------------------- | ------ | --- | ------- | ---------- | --- | --- | ---- | ------ |
| Read system log      | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ➖   | ❌     |
| Get server status    | ✅     | ❌  | ❌      | ❌         | ❌  | ❌  | ❌   | ❌     |
| Change user password | ✅     | ❌  | ❌      | ❌         | ❌  | ➖  | ➖   | ❌     |
| Issue SSH command    | ❌     | ❌  | ❌      | ❌         | ✅  | ❌  | ❌   | ❌     |
