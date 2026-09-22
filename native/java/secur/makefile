# makefile boilerplate to build the JNI implementation (C++), metal C code, and
# bind into a "shared object" called at REST API runtime via the /wto endpoint
#
# This program and the accompanying materials are made available and may be used, at your option, under either:
# * Eclipse Public License v2.0, available at https://www.eclipse.org/legal/epl-v20.html, OR
# * Apache License, version 2.0, available at http://www.apache.org/licenses/LICENSE-2.0
# 
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
# 
# Copyright Contributors to the Zowe Project.

CXX=xlc++
CC=xlc

JAVA_HOME=/usr/lpp/java/J8.0_64
JAVA_SIDEDECK_DIR=bin/j9vm

DLL_CPP_FLAGS=-W "c,lp64,langlvl(extended),dll,xplink,exportall" \
 -qsearch=$(JAVA_HOME)/include \
 -qsource \
 -g \
 -c

DLL_BND_FLAGS=-W "l,lp64,dll,dynam=dll,xplink,map,list" \
 -g \
 -qsource

SIDEDECKPATH = $(JAVA_HOME)/$(JAVA_SIDEDECK_DIR)
SIDEDECK = libjvm

PREFIX := "../lib/"

LIB_SECUR = "libzowe-commons-secur.so"

all: $(LIB_SECUR)

install:
	mkdir -p $(PREFIX)
	cp -vp *.so $(PREFIX)
	ls -E $(PREFIX)

$(LIB_SECUR): secur.o jnitools.o
	$(CXX) $(DLL_BND_FLAGS) -o $@ $(SIDEDECKPATH)/$(SIDEDECK).x jnitools.o $^ > $*.bind.lst
	extattr +p $@

secur.o: secur.c
	$(CC) $(DLL_CPP_FLAGS) -qlist=$*.cpp.lst -o $@ $^

jnitools.o: jnitools.c
	$(CC) $(DLL_CPP_FLAGS) -qlist=$*.cpp.lst -o $@ $^

clean:
	rm *.o
	rm *.lst
	rm *.x
	rm *.so
