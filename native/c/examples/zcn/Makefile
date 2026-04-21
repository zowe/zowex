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

all: $(OUT_DIR) extattr run 

#
# Output directories
#
$(OUT_DIR):
	@echo 'Creating $(OUT_DIR)'
	mkdir -p $(OUT_DIR)

run: $(OUT_DIR)/demozcn
	@echo 'Running ./$(OUT_DIR)/demozcn'
	export _BPXK_JOBLOG=STDERR && ./$(OUT_DIR)/demozcn

$(OUT_DIR)/demozcn.o: demozcn.cpp
	@echo 'Building $(OUT_DIR)/demozcn.o'
	cp ../../$(OUT_DIR)/*.a $(OUT_DIR)
	$(CXX) $(CPP_FLAGS) -c $^ -o $@

$(OUT_DIR)/demozcn: $(OUT_DIR)/demozcn.o $(OUT_DIR)/libzcn.a $(OUT_DIR)/libzut.a
	@echo 'Building demozcn'
	$(CXX) $(CPP_BND_FLAGS_AUTH) -o $@ $^ > $*.bind.lst

extattr: $(OUT_DIR)/demozcn
	extattr +ap $(OUT_DIR)/demozcn

clean:
	rm -f *.o
	rm -f *.dbg
	rm -f *.so
	rm -f *.a
	rm -f *.u
	rm -f *.x
	rm -f *.lst
	rm -f CEEDUMP.*
	rm -rf $(OUT_DIR)
