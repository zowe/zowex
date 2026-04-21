# DCB Abend Exit Implementation

## Overview

This document describes the process for implementing the DCB abend exit in the native code, specifically in `native/c/zam.c` (`ZAMDEXIT`), and how the exit works with higher-level operations in `native/c/zds.cpp`.

A DCB abend exit is used to intercept and handle abnormal endings (abends) that occur during data set I/O operations (such as `OPEN`, `CLOSE`, or writing to a data set). This prevents the entire process from terminating abruptly and allows the framework to report the error gracefully.

## Implementation in `zam.c`

- **The Exit Routine (`ZAMDEXIT`)**:
   - `ZAMDEXIT` is a C function defined with `#pragma prolog` and `#pragma epilog` directives for proper linkage. It takes a `DCB_ABEND_PL` parameter list.
   - When an abend triggers the exit, it sets a flag `gioc->dcb_abend = 1;` on the global/register variable `gioc` to record that an abend occurred.
   - It evaluates the parameter list's options mask (`plist->option_mask`). 
      - If `DCB_ABEND_OPT_OK_TO_IGNORE` is set, it returns `DCB_ABEND_RC_IGNORE` to tell the system that it is safe to ignore the abend (for example, an out-of-space SE37 abend). 
      - Some abends cannot be ignored but they can be delayed. For instance, if `DCB_ABEND_OPT_OK_TO_DELAY` is set, it returns `DCB_ABEND_RC_DELAY` and delays the abend until all the DCBs in the same OPEN or CLOSE macro are opened or closed.
      - The default return code for the DCB abend exit is `DCB_ABEND_RC_TERMINATE`, which tells the system to continue with terminating the process as a result of the abend.
   - *Note on End-of-Volume errors*: Requests to ignore an abend are disregarded if the abend occurs during end-of-volume processing and the end-of-volume routines were called by the `CLOSE` routines. In this situation, an `ABEND` macro is issued even though the `IGNORE` option has been selected. If the end-of-volume error is successfully ignored, the DCB is closed, and control returns to the application. The application must then check if the DCB is still open before issuing any macros other than `CLOSE` or `FREEPOOL`.

- **The Context Pointer (`gioc`)**:
   - The system-provided DCB abend exit interface does not support passing a user context pointer (such as `IO_CTRL`). To work around this, we utilize a dedicated register variable mapped to register `r8` (`register IO_CTRL *gioc ASMREG("r8");`).
   - Prior to beginning any I/O operation (for example, `OPEN` or `CLOSE`), `gioc` is pointed to the active `IO_CTRL` structure.

- **Registering the Exit (`setup_exit_list`)**:
   - In `setup_exit_list`, 24-bit storage is allocated (`storage_obtain24`) for `zam24`.
   - A piece of stub logic (`ZAM24`, an assembly routine which handles AMODE switching and returning to the system) is copied to this memory. This stub eventually calls `ZAMDEXIT`.
   - The address of this stub is added to the exit list (`ioc->exlst`) with code `exldcbab` (indicating a DCB abend exit), and an end-of-list delimiter is appended.

## Interaction with `zds.cpp`

High-level APIs in `zds.cpp` (like `zds_open_output_bpam`, `zds_write_output_bpam`, and `zds_close_output_bpam`) wrap the underlying operations from `zam.c`.
When a DCB abend occurs during an operation, such as an SE37 out-of-space abend while writing to a PDS, the execution flows as follows:

1. The underlying macro (for example, `write_sync`, `stow`, or `close_dcb`) encounters a condition that triggers `ZAMDEXIT`.
2. `ZAMDEXIT` marks `gioc->dcb_abend = 1` and, if allowed, instructs the system to ignore the abend and return control.
3. The calling function in `zam.c` detects the `dcb_abend` flag, sets `zds->diag.detail_rc` to `ZDS_RTNCD_DCB_ABEND_ERROR`, and returns `RTNCD_FAILURE`.
4. `zds.cpp` then bubbles up this failure code back to the caller instead of terminating the process.

This design enables consumers to handle error scenarios during BPAM operations without crashing the application and performing improper data set processing. **Note:** If an abend occurs during end-of-volume processing, the ignore request might be discarded and the abend could still propagate up to ESTAEX recovery.
