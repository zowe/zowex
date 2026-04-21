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

# Assembler program makefile

# Default assembler and linker
AS=as
LD=ld

# Source files
SOURCES = testasm1.s testasm4.s testaut1.s testload.s

# Object files
OBJECTS = $(SOURCES:.s=.o)

# Executables
EXECUTABLES = $(SOURCES:.s=)

# Default target
all: $(EXECUTABLES)

# Rule to build executables from object files
%: %.o
	$(LD) -o $@ $<

# Rule to build object files from source files
%.o: %.s
	$(AS) -a=$*.s.lst -o $@ $<

# Run target (example for testauth)
run-testaut1: testaut1
	./testaut1

# Run target (example for testasm1)
run-testasm1: testasm1
	./testasm1

# Run target (example for testasm4)
run-testasm4: testasm4
	./testasm4

# Clean target
clean:
	rm -f $(OBJECTS) $(EXECUTABLES)
