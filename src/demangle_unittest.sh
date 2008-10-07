#! /bin/sh
# Copyright 2006 Google, Inc.  All Rights Reserved. 
# Author: Satoru Takabayashi
#
# Unit tests for demangle.c with a real binary.

set -e

die () {
    echo $1
    exit 1
}

BINDIR=".libs"
LIBGLOG="$BINDIR/libglog.so"

DEMANGLER="$BINDIR/demangle_unittest"

if test -e "$DEMANGLER"; then
  # We need shared object.
  export LD_LIBRARY_PATH=$BINDIR
  export DYLD_LIBRARY_PATH=$BINDIR
else
  # For windows
  DEMANGLER="./demangle_unittest.exe"
  if ! test -e "$DEMANGLER"; then
    echo "We coundn't find demangle_unittest binary."
    exit 1
  fi
fi

# Extract C++ mangled symbols from libbase.so.
NM_OUTPUT="demangle.nm"
nm "$LIBGLOG" | perl -nle 'print $1 if /\s(_Z\S+$)/' > "$NM_OUTPUT"

# Check if mangled symbols exist. If there are none, we quit.
# The binary is more likely compiled with GCC 2.95 or something old.
if ! grep --quiet '^_Z' "$NM_OUTPUT"; then
    echo "PASS"
    exit 0
fi

# Demangle the symbols using our demangler.
DM_OUTPUT="demangle.dm"
GLOG_demangle_filter=1 "$DEMANGLER" --demangle_filter < "$NM_OUTPUT" > "$DM_OUTPUT"

# Calculate the numbers of lines.
NM_LINES=`wc -l "$NM_OUTPUT" | awk '{ print $1 }'`
DM_LINES=`wc -l "$DM_OUTPUT" | awk '{ print $1 }'`

# Compare the numbers of lines.  They must be the same.
if test "$NM_LINES" != "$DM_LINES"; then
    die "$NM_OUTPUT and $DM_OUTPUT don't have the same numbers of lines"
fi

# Check if mangled symbols exist.  They must not exist.
if grep --quiet '^_Z' "$DM_OUTPUT"; then
    die "Mangled symbols found in $DM_OUTPUT"
fi

# All C++ symbols are demangled successfully.
echo "PASS"
exit 0
