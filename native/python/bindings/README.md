# Zowe Remote SSH Python Bindings Distributions

This directory contains utility scripts to bundle and distribute the Python bindings for the Zowe Remote SSH (ZRS) project on z/OS. 

We provide two distinct packaging scripts, tailored for different deployment needs. Both bundles are completely system agnostic on z/OS and do **NOT** require SWIG to be installed on the target machine.

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
