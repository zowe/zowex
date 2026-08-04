#!/bin/sh
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
# Dump the symbol names exported by the Language Environment C/C++ side decks on THIS system.
#
# Run this on a system at the minimum supported z/OS release (see compat/zos-min-level.txt) and
# commit the output as compat/CRTEQCXE.<level>.exports, so `make check-compat` can verify on a newer
# build host that zowex only imports symbols the floor release actually provides.
#
#   sh tools/dump_sidedeck_exports.sh > compat/CRTEQCXE.zosv2r5.exports
#
# Record the source system's z/OS release and LE PTF level in the header comment afterwards -- a
# snapshot without a maintenance level is not reproducible.
#
# Override the data set with SCEELIB2=<name> if your site does not use CEE.SCEELIB2.

SCEELIB2=${SCEELIB2:-CEE.SCEELIB2}

# LE C base, libc++ (EBCDIC/ASCII x 31/64-bit variants plus helpers), and unwind side decks.
MEMBERS="CELQS001 CELQS003 CRTDQCXE CRTDQCXG CRTDQCXH CRTDQCXS CRTDQCXP CRTDQCXA CRTDQXLA CRTDQUNW"

echo "# Language Environment C/C++ side-deck exports"
echo "#"
echo "# Source data set: $SCEELIB2"
echo "# Captured on:     $(uname -a)"
echo "# z/OS release:    TODO record the marketing release (e.g. z/OS 2.5)"
echo "# LE PTF level:    TODO record the applied Language Environment maintenance level"
echo "#"

for member in $MEMBERS; do
  # Side decks are plain binder control statements:
  #   IMPORT CODE,'dllname','symbolname'
  cat "//'$SCEELIB2($member)'" 2>/dev/null
done | sed -n "s/.*IMPORT[ ,]*\(CODE\|DATA\)[ ,]*'[^']*'[ ,]*'\([^']*\)'.*/\2/p" | sort -u
