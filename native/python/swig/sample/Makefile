#
#  This program and the accompanying materials are
#  made available under the terms of the Eclipse Public License v2.0
#  which accompanies this distribution, and is available at
#  https://www.eclipse.org/legal/epl-v20.html
#
#  SPDX-License-Identifier: EPL-2.0
#
#  Copyright Contributors to the Zowe Project.
#

CC=xlc
CXX=xlclang++
ASM=as

_CC_CCMODE=1
_CXX_CCMODE=1
_C89_CCMODE=1
_CC_EXTRA_ARGS=1
_CXX_EXTRA_ARGS=1
_C89_EXTRA_ARGS=1
.EXPORT: _CC_CCMODE _CXX_CCMODE _C89_CCMODE _CC_EXTRA_ARGS _CXX_EXTRA_ARGS _C89_EXTRA_ARGS

CC_OPTS=metal,langlvl(extended),sscom,nolongname,inline,genasm,inlrpt,csect,nose,lp64,list,warn64,optimize(2),list,showinc,showmacro,source,aggregate
CXX_OPTS=lp64,langlvl(extended0x),dll,xplink,exportall

all: build-ext

#
# Metal C
#
zmetal.s: zmetal.c zmetal.h
	@echo 'Building zmetal.s'
	$(CC) -S -W"c,$(CC_OPTS)" -qlist=zmetal.c.lst -I/usr/include/metal -o $@ zmetal.c

zmetal.o: zmetal.s
	@echo 'Building zmetal.o'
	$(ASM) -mrent -a=zmetal.s.lst -ISYS1.MACLIB -ICBC.SCCNSAM -o $@ zmetal.s

#
# C++ wrapper around the Metal C function
#
hello.o: hello.cpp hello.h zmetal.h
	@echo 'Building hello.o'
	$(CXX) -W "c,$(CXX_OPTS)" -c -qlist=hello.cpp.lst -o $@ hello.cpp

#
# SWIG wrapper generation
#
example_wrap.cxx: example.i example.h
	@echo 'Generating SWIG wrapper'
	swig -c++ -python example.i

#
# Python extension module
#
build-ext: example_wrap.cxx example.cpp example.h setup.py zmetal.o hello.o
	@echo 'Building Python extension'
	python setup.py build_ext --inplace

clean:
	rm -f *.o
	rm -f *.s
	rm -f *.lst
	rm -f *.cxx
	rm -f *.pyc
	rm -f example.py
	rm -f _example*.so
	rm -rf build

.PHONY: all build-ext clean
