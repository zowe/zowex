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
# Verify that every external symbol zowex/zoweax import is available on the minimum supported z/OS
# release, so a CEE3561S load failure is caught here instead of on a customer system.
#
#   sh tools/check_runtime_symbols.sh            # verify
#   sh tools/check_runtime_symbols.sh --update   # refresh the baseline after a reviewed change
#
# Run from native/c after a build. See compat/README.md.

OUT_DIR=${OUT_DIR:-build-out}
COMPAT=compat
BASELINE=$COMPAT/runtime-imports.baseline
REPORT=$OUT_DIR/runtime-imports.txt

if [ ! -f "$COMPAT/zos-min-level.txt" ]; then
  echo "check_runtime_symbols: missing $COMPAT/zos-min-level.txt" >&2
  exit 1
fi
MIN_LEVEL=$(sed -e 's/#.*//' -e '/^[[:space:]]*$/d' "$COMPAT/zos-min-level.txt" | head -n 1)
FLOOR=$COMPAT/CRTEQCXE.$MIN_LEVEL.exports

# Every symbol the program object imports from a DLL starts life as an undefined external in one of
# our own object files, so {undefined} - {defined} across all inputs is exactly the external
# dependency set. Objects only, not the lib*.a archives: nm emits per-member headers for archives.
OBJS=$(ls $OUT_DIR/*.o $OUT_DIR/commands/*.o $OUT_DIR/server/*.o $OUT_DIR/extend/*.o 2>/dev/null)
if [ -z "$OBJS" ]; then
  echo "check_runtime_symbols: no object files under $OUT_DIR; build first (make all)" >&2
  exit 1
fi

# POSIX nm output is "name type value size"; undefined externals have type U. Note -g and -u are
# mutually exclusive in POSIX nm, so filter on the type column instead of combining them.
if ! nm -P $OBJS > $OUT_DIR/.nm.raw 2>$OUT_DIR/.nm.err; then
  echo "check_runtime_symbols: nm failed" >&2
  cat $OUT_DIR/.nm.err >&2
  exit 1
fi

awk '$2 == "U" { print $1 }' $OUT_DIR/.nm.raw | sort -u > $OUT_DIR/.nm.undef
awk '$2 != "U" { print $1 }' $OUT_DIR/.nm.raw | sort -u > $OUT_DIR/.nm.defined
comm -23 $OUT_DIR/.nm.undef $OUT_DIR/.nm.defined > $OUT_DIR/.imports

{
  echo "# External symbols imported by zowex / zoweax"
  echo "#"
  echo "# Minimum supported z/OS level: $MIN_LEVEL"
  echo "# Compiler: $(ibm-clang++64 --version 2>&1 | head -n 1)"
  echo "#"
  cat $OUT_DIR/.imports
} > $REPORT

if [ "$1" = "--update" ]; then
  {
    echo "# Allow-list of external symbols imported by zowex / zoweax."
    echo "#"
    echo "# Every symbol below must be exported by the Language Environment runtime on the z/OS"
    echo "# release named in zos-min-level.txt ($MIN_LEVEL). A symbol that only exists on newer LE"
    echo "# levels causes CEE3561S at load time on customer systems. See README.md and"
    echo "# https://github.com/zowe/zowex/issues/871."
    echo "#"
    echo "# Regenerate with: make check-compat-update"
    echo "# Compiler: $(ibm-clang++64 --version 2>&1 | head -n 1)"
    echo "#"
    cat $OUT_DIR/.imports
  } > $BASELINE
  echo "Baseline refreshed: $(wc -l < $OUT_DIR/.imports) symbols. Review the diff before committing."
  exit 0
fi

# Strip comments so the reference files can carry provenance headers.
strip() { sed -e 's/#.*//' -e '/^[[:space:]]*$/d' "$1" | sort -u; }

rc=0

if [ -f "$BASELINE" ]; then
  strip "$BASELINE" > $OUT_DIR/.baseline
else
  : > $OUT_DIR/.baseline
fi
if [ ! -s $OUT_DIR/.baseline ]; then
  echo "WARNING: $BASELINE is not seeded, so imported symbols are not being checked against it."
  echo "         Seed it on a build at $MIN_LEVEL with: make check-compat-update"
elif ! diff -u $OUT_DIR/.baseline $OUT_DIR/.imports > $OUT_DIR/runtime-imports.diff 2>&1; then
  echo "=============================================================================="
  echo "ERROR: the set of runtime symbols imported by zowex changed."
  echo ""
  sed -n '/^[+-][^+-]/p' $OUT_DIR/runtime-imports.diff
  echo ""
  echo "Every '+' symbol must be exported by the Language Environment runtime on $MIN_LEVEL,"
  echo "the minimum supported z/OS release. A symbol that only exists on newer LE levels causes"
  echo "CEE3561S at load time on customer systems (zowex#871)."
  echo ""
  echo "  1. On a $MIN_LEVEL system:  grep -i '<symbol>' \"//'CEE.SCEELIB2(CRTDQCXE)'\""
  echo "  2. Found     -> refresh the baseline:  make check-compat-update"
  echo "  3. Not found -> change the code to avoid it. Do NOT update the baseline."
  echo "=============================================================================="
  rc=8
fi

# Independent of the baseline diff on purpose: a reflexive `--update` can defeat that check, but it
# cannot make a symbol appear on the floor release. This is the check that catches a CEE3561S.
if [ -f "$FLOOR" ]; then
  strip "$FLOOR" > $OUT_DIR/.floor
  # CELQ* are LE C base entry points resolved from the always-present base side decks.
  comm -23 $OUT_DIR/.imports $OUT_DIR/.floor | grep -v '^CELQ' > $OUT_DIR/.missing
  if [ -s $OUT_DIR/.missing ]; then
    echo "=============================================================================="
    echo "ERROR: these symbols are NOT exported by the LE runtime on $MIN_LEVEL:"
    sed 's/^/  /' $OUT_DIR/.missing
    echo ""
    echo "The bind succeeds on this build host and the binary will fail with CEE3561S on"
    echo "$MIN_LEVEL. Reference: $FLOOR"
    echo "=============================================================================="
    rc=8
  fi
else
  echo "WARNING: $FLOOR not found, so imports are not being checked against the floor release."
  echo "         Capture it on a $MIN_LEVEL system with: sh tools/dump_sidedeck_exports.sh"
fi

if [ $rc -eq 0 ]; then
  echo "Runtime symbol check passed ($(wc -l < $OUT_DIR/.imports) imports, floor $MIN_LEVEL)."
fi
exit $rc
