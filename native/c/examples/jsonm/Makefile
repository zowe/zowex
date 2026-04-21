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

OUT_DIR=build-out

.INCLUDE: ../examples_toolchain.mk

all: $(OUT_DIR) run

run: $(OUT_DIR)/json
	@echo 'Running ./$(OUT_DIR)/json'
	export _BPXK_JOBLOG=STDERR && ./$(OUT_DIR)/json

$(OUT_DIR):
	@echo 'Creating $(OUT_DIR)'
	mkdir -p $(OUT_DIR)

copy-mtl:
	cp ../../zmetal.h zmetal.h
	cp ../../zwto.h zwto.h
	cp ../../zjsonm31.h zjsonm31.h

$(OUT_DIR)/json.s: json.c
	@echo 'Building $(OUT_DIR)/json.s'
	$(CC) $(MTL_FLAGS64) -qlist=$*.mtl.lst $(MTL_HEADERS) -o $@ $^

$(OUT_DIR)/json.o: $(OUT_DIR)/json.s
	@echo 'Building $(OUT_DIR)/json.s'
	$(ASM) $(ASM_FLAGS) -a=$*.s.lst $(MACLIBS) -o $@ $^

$(OUT_DIR)/json: $(OUT_DIR)/json.o
	@echo 'Building $(OUT_DIR)/json'
	$(BIND) $(BIND_FLAGS_64) -V -eMAIN -o $@ $^ > $*.bind.lst

clean:
	rm -f *.o
	rm -f *.dbg
	rm -f *.so
	rm -f *.a
	rm -f *.x
	rm -f *.lst
	rm -f CEEDUMP.*
	rm -rf $(OUT_DIR)
