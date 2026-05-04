#!/usr/bin/env python3

"""
setup.py file for SWIG example
"""

from setuptools import setup, Extension
import os
import sys

C_PATH = "../../c"
chdsect = os.path.abspath(f"{C_PATH}/chdsect")
ztype = os.path.abspath(C_PATH)
build_out_path = f"{C_PATH}/build-out"
swig_build_path = f"{build_out_path}/swig"

zusf_py_module = Extension("_zusf_py",
                           sources=["zusf_py_wrap.cxx", "zusf_py.cpp",
                                    f"{C_PATH}/zusf.cpp", f"{C_PATH}/zut.cpp"],
                           language="c++",
                           include_dirs=[chdsect],
                           libraries=["zut"],
                           library_dirs=[build_out_path],
                           )

zds_py_module = Extension("_zds_py",
                          sources=["zds_py_wrap.cxx", "zds_py.cpp"],
                          language="c++",
                          extra_objects=[
                              f"{build_out_path}/zdsm.o",
                              f"{build_out_path}/zutm.o",
                              f"{build_out_path}/zam.o",
                              f"{build_out_path}/zam24.o",
                              f"{build_out_path}/zutm31.o",
                              f"{swig_build_path}/zds.o",
                              f"{swig_build_path}/zut.o",
                          ],
                          include_dirs=[chdsect, ztype],
                          )

zjb_py_module = Extension("_zjb_py",
                          sources=["zjb_py_wrap.cxx", "zjb_py.cpp"],
                          language="c++",
                          extra_objects=[
                              f"{build_out_path}/zjbm.o",
                              f"{build_out_path}/zutm.o",
                              f"{build_out_path}/zutm31.o",
                              f"{build_out_path}/zam.o",
                              f"{build_out_path}/zdsm.o",
                              f"{swig_build_path}/zjb.o",
                              f"{swig_build_path}/zut.o",
                              f"{swig_build_path}/zds.o",
                          ],
                          include_dirs=[chdsect, ztype],
                          )

# Parse environment variable for selective building


def get_modules_to_build():
    """Determine which modules to build based on ZBIND_MODULES environment variable."""
    modules_env = os.environ.get('ZBIND_MODULES', '')

    if modules_env:
        modules_to_build = set(modules_env.split(','))
        # Clean up any whitespace
        modules_to_build = {m.strip() for m in modules_to_build if m.strip()}
    else:
        # If no specific modules requested, build all
        modules_to_build = {'zusf', 'zds', 'zjb'}

    return modules_to_build


# Determine which modules to build
modules_to_build = get_modules_to_build()

# Select extensions and py_modules based on what's requested
ext_modules = []
py_modules = []

if 'zusf' in modules_to_build:
    ext_modules.append(zusf_py_module)
    py_modules.append("zusf_py")

if 'zds' in modules_to_build:
    ext_modules.append(zds_py_module)
    py_modules.append("zds_py")

if 'zjb' in modules_to_build:
    ext_modules.append(zjb_py_module)
    py_modules.append("zjb_py")

print(f"Building modules: {', '.join(modules_to_build)}")

setup(name="zbind",
      description="""Simple swig example""",
      ext_modules=ext_modules,
      py_modules=py_modules,
      )
