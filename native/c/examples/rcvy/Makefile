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

run: $(OUT_DIR)/testrcvy
	@echo 'Running ./$(OUT_DIR)/testrcvy'
	export _BPXK_JOBLOG=STDERR && ./$(OUT_DIR)/testrcvy

$(OUT_DIR):
	@echo 'Creating $(OUT_DIR)'
	mkdir -p $(OUT_DIR)

$(OUT_DIR)/testrcvy.s: testrcvy.c
	@echo 'Building $(OUT_DIR)/testrcvy.s'
	$(CC) $(MTL_FLAGS64) -qlist=$*.mtl.lst $(MTL_HEADERS) -o $@ $^

$(OUT_DIR)/testrcvy.o: $(OUT_DIR)/testrcvy.s
	@echo 'Building $(OUT_DIR)/testrcvy.o'
	$(ASM) $(ASM_FLAGS) -a=$*.s.lst $(MACLIBS) -o $@ $^

$(OUT_DIR)/testrcvy: $(OUT_DIR)/testrcvy.o
	@echo 'Building testrcvy'
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
