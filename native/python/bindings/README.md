# Zowe Remote SSH Python Bindings Distributions

This directory contains utility scripts to bundle and distribute the Python bindings for the Zowe Remote SSH (ZRS) project on z/OS. 

We provide two distinct packaging scripts, tailored for different deployment needs. Both bundles are completely system agnostic on z/OS and do **NOT** require SWIG to be installed on the target machine.

## Available Functions

The bindings are split across three modules, each generated from the corresponding `*_py.hpp` header. All functions raise a Python exception (`RuntimeError`) on failure instead of returning an error code.

### `zds_py` — Data Sets

| Function | Description |
| --- | --- |
| `create_data_set(dsn: str, attributes: DS_ATTRIBUTES)` | Create a new dataset with the specified attributes. |
| `list_data_sets(dsn: str) -> list[ZDSEntry]` | List datasets matching the given pattern. |
| `read_data_set(dsn: str, codepage: str = "") -> str` | Read content from a dataset with optional encoding. |
| `write_data_set(dsn: str, data: str, codepage: str = "", etag: str = "") -> str` | Write data to a dataset with optional encoding and etag validation. |
| `delete_data_set(dsn: str)` | Delete the specified dataset. |
| `create_member(dsn: str)` | Create a new member in a partitioned dataset. |
| `list_members(dsn: str) -> list[ZDSMem]` | List all members in a partitioned dataset. |

**Supporting types:**

- `DS_ATTRIBUTES`: `alcunit`, `blksize`, `dirblk`, `dsorg`, `primary`, `recfm`, `lrecl`, `dataclass`, `unit`, `dsntype`, `mgntclass`, `dsname`, `avgblk`, `secondary`, `size`, `storclass`, `vol`
- `ZDSEntry`: `name`, `dsorg`, `volser`, `recfm`, `migrated`
- `ZDSMem`: `name`

### `zjb_py` — Jobs

| Function | Description |
| --- | --- |
| `list_jobs_by_owner(owner_name: str) -> list[ZJob]` | List all jobs owned by the specified user. |
| `list_jobs_by_owner(owner_name: str, prefix: str, status: str) -> list[ZJob]` | List jobs owned by the specified user, filtered by job name prefix and status. |
| `get_job_status(jobid: str) -> ZJob` | Get the current status of a job by job ID. |
| `list_spool_files(jobid: str) -> list[ZJobDD]` | List all spool files (DD statements) for a job. |
| `read_spool_file(jobid: str, key: int) -> str` | Read the content of a specific spool file by job ID and key. |
| `get_job_jcl(jobid: str) -> str` | Retrieve the JCL content for a job. |
| `submit_job(jcl_content: str) -> str` | Submit JCL content and return the assigned job ID. |
| `delete_job(jobid: str) -> bool` | Delete a job from the system and return success status. |

**Supporting types:**

- `ZJob`: `jobname`, `jobid`, `owner`, `status`, `full_status`, `retcode`, `correlator`
- `ZJobDD`: `jobid`, `ddn`, `dsn`, `stepname`, `procstep`, `key`

### `zusf_py` — USS (Unix System Services)

| Function | Description |
| --- | --- |
| `create_uss_file(file: str, mode: str)` | Create a new USS file with the specified permissions. |
| `create_uss_dir(file: str, mode: str)` | Create a new USS directory with the specified permissions. |
| `list_uss_dir(path: str, options: ListOptions = ListOptions(False, False)) -> str` | List contents of a USS directory. |
| `move_uss_file_or_dir(source: str, destination: str)` | Move a USS file or directory. |
| `read_uss_file(file: str, codepage: str = "") -> str` | Read content from a USS file with optional encoding. |
| `read_uss_file_streamed(file: str, pipe: str, codepage: str = "", content_len: size_t* = None)` | Read USS file content to a pipe in streaming mode. |
| `write_uss_file(file: str, data: str, codepage: str = "", etag: str = "") -> str` | Write data to a USS file with optional encoding and etag validation. |
| `write_uss_file_streamed(file: str, pipe: str, codepage: str = "", etag: str = "", content_len: size_t* = None) -> str` | Write data from a pipe to a USS file in streaming mode. |
| `chmod_uss_item(file: str, mode: str, recursive: bool = False)` | Change permissions of a USS file or directory. |
| `delete_uss_item(file: str, recursive: bool = False)` | Delete a USS file or directory, with optional recursion. |
| `chown_uss_item(file: str, owner: str, recursive: bool = False)` | Change ownership of a USS file or directory. |
| `chtag_uss_item(file: str, tag: str, recursive: bool = False)` | Change the file tag of a USS file or directory. |

**Supporting types:**

- `ListOptions(all_files: bool = False, long_format: bool = False, max_depth: int = 1)`

## Distribution Types

### 1. Precompiled Binary Distribution (`zbind_bin_dist.tar.gz`)
*Designed for end-users who just want to use the Python bindings immediately without compiling anything.*

- **Compiled on:** The build mainframe (e.g. `lpar.1`).
- **Dependencies needed on target:** Only Python (e.g. Python 3.11, 3.12, etc.). **No compiler, SWIG, C++ sources, or headers are required.**
- **How to Build:**
  ```bash
  python package_precompiled.py
  ```
  This creates `zbind_bin_dist.tar.gz` containing precompiled shared libraries (`_*.so`), wrapper modules (`*.py`), and package metadata.

- **How to Extract & Use on Target Machine (e.g. `lpar.2`):**
  1. Transfer `zbind_bin_dist.tar.gz` to the target machine (binary mode).
  2. Unpack the tarball safely (disabling C-runtime autoconversion):
     ```bash
     chtag -b zbind_bin_dist.tar.gz
     python3 -c "import gzip; f_in = gzip.open('zbind_bin_dist.tar.gz', 'rb'); f_out = open('zbind_bin_dist.tar', 'wb'); f_out.writelines(f_in); f_in.close(); f_out.close()"
     chtag -b zbind_bin_dist.tar
     tar -xf zbind_bin_dist.tar && rm zbind_bin_dist.tar
     ```
  3. Untag the files inside the extracted folder:
     ```bash
     chtag -r zbind_bin_dist/*
     ```
  4. Import instantly in your scripts using `sys.path`:
     ```python
     import sys
     sys.path.insert(0, "/path/to/zbind_bin_dist")
     
     from zusf_py import list_uss_dir
     print(list_uss_dir("/tmp"))
     ```

### 2. Self-Contained Source Distribution (`zbind_src_dist.tar.gz`)
*Designed for environments where you need to build and install the bindings locally from source (e.g. target machines with different or multiple Python versions), but without SWIG.*

- **Dependencies needed on target:** Python and `ibm-clang`. **SWIG is NOT required.**
- **How to Build:**
  ```bash
  python package_zbind.py
  ```
  This creates `zbind_src_dist.tar.gz` containing all required C++ sources, headers, precompiled Metal C object files, and a flat-layout optimized `setup.py`.

- **How to Extract & Install on Target Machine:**
  1. Transfer `zbind_src_dist.tar.gz` to the target machine (binary mode).
  2. Unpack the tarball safely:
     ```bash
     chtag -b zbind_src_dist.tar.gz
     python3 -c "import gzip; f_in = gzip.open('zbind_src_dist.tar.gz', 'rb'); f_out = open('zbind_src_dist.tar', 'wb'); f_out.writelines(f_in); f_in.close(); f_out.close()"
     chtag -b zbind_src_dist.tar
     tar -xf zbind_src_dist.tar && rm zbind_src_dist.tar
     ```
  3. Untag the C++ files inside the extracted directory so that autoconversion works:
     ```bash
     cd zbind_src_dist
     chtag -r *.cpp *.hpp *.h *.cxx chdsect/*.h
     ```
  4. Build and install inside your target Python environment:
     ```bash
     export CC=ibm-clang64
     export CXX=ibm-clang++64
     
     # Option A: Build in-place
     python setup.py build_ext --inplace
     
     # Option B: Install via pip
     pip install .
     ```
