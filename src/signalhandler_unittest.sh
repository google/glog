#! /bin/sh
# Copyright 2008 Google, Inc.  All Rights Reserved.
# Author: Satoru Takabayashi
#
# Unit tests for signalhandler.cc.

die () {
    echo $1
    exit 1
}

BINDIR=".libs"
LIBGLOG="$BINDIR/libglog.so"

BINARY="$BINDIR/signalhandler_unittest"
LOG_INFO="./signalhandler_unittest.INFO"

# Remove temporary files.
rm -f signalhandler.out*

if test -e "$BINARY"; then
  # We need shared object.
  export LD_LIBRARY_PATH=$BINDIR
  export DYLD_LIBRARY_PATH=$BINDIR
else
  # For windows
  BINARY="./signalhandler_unittest.exe"
  if ! test -e "$BINARY"; then
    echo "We coundn't find demangle_unittest binary."
    exit 1
  fi
fi

if [ x`$BINARY` != 'xOK' ]; then
  echo "PASS (No stacktrace support. We don't run this test.)"
  exit 0
fi

# The PC cannot be obtained in signal handlers on PowerPC correctly.
# We just skip the test for PowerPC.
if [ x`uname -p` = x"powerpc" ]; then
  echo "PASS (We don't test the signal handler on PowerPC.)"
  exit 0
fi

# Test for a case the program kills itself by SIGSEGV.
GOOGLE_LOG_DIR=. $BINARY segv 2> signalhandler.out1
for pattern in SIGSEGV 0xdead main "Aborted at [0-9]"; do
  if ! grep --quiet "$pattern" signalhandler.out1; then
    die "'$pattern' should appear in the output"
  fi
done
if ! grep --quiet "a message before segv" $LOG_INFO; then
  die "'a message before segv' should appear in the INFO log"
fi
rm -f $LOG_INFO

# Test for a case the program is killed by this shell script.
# $! = the process id of the last command run in the background.
# $$ = the process id of this shell.
$BINARY loop 2> signalhandler.out2 &
# Wait until "looping" is written in the file.  This indicates the program
# is ready to accept signals.
while true; do
  if grep --quiet looping signalhandler.out2; then
    break
  fi
done
kill -TERM $!
wait $!

from_pid=''
# Only linux has the process ID of the signal sender.
if [ x`uname` = "xLinux" ]; then
  from_pid="from PID $$"
fi
for pattern in SIGTERM "by PID $!" "$from_pid" main "Aborted at [0-9]"; do
  if ! grep --quiet "$pattern" signalhandler.out2; then
    die "'$pattern' should appear in the output"
  fi
done

# Test for a case the program dies in a non-main thread.
$BINARY die_in_thread 2> signalhandler.out3
EXPECTED_TID="`sed 's/ .*//' signalhandler.out3`"

for pattern in SIGFPE DieInThread "TID $EXPECTED_TID" "Aborted at [0-9]"; do
  if ! grep --quiet "$pattern" signalhandler.out3; then
    die "'$pattern' should appear in the output"
  fi
done

# Test for a case the program installs a custom failure writer that writes
# stuff to stdout instead of stderr.
$BINARY dump_to_stdout 1> signalhandler.out4
for pattern in SIGABRT main "Aborted at [0-9]"; do
  if ! grep --quiet "$pattern" signalhandler.out4; then
    die "'$pattern' should appear in the output"
  fi
done

echo PASS
