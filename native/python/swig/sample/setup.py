#!/usr/bin/env python3

"""
setup.py file for SWIG example
"""

from distutils.core import setup, Extension


example_module = Extension('_example',
                           sources=['example_wrap.cxx', 'example.cpp'],
                           language='c++',
                           extra_objects=['hello.o', 'zmetal.o'],
                           )

setup (name = 'example',
       version = '0.1',
       author      = "Zowe",
       description = """SWIG example that invokes Metal C from Python""",
       ext_modules = [example_module],
       py_modules = ["example"],
       )
