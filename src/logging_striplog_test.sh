#! /bin/sh
# Copyright 2007 Google, Inc.
# All Rights Reserved. 
#
# Author: Sergey Ioffe

get_strings () {
    if test -e ".libs/$1"; then
        binary=".libs/$1"
    elif test -e "$1.exe"; then
        binary="$1.exe"
    else
        echo "We coundn't find $1 binary."
        exit 1
    fi
    
    strings -n 10 $binary | sort | awk '/TESTMESSAGE/ {printf "%s ", $2}'
}

# Die if "$1" != "$2", print $3 as death reason
check_eq () {
    if [ "$1" != "$2" ]; then
        echo "Check failed: '$1' == '$2' ${3:+ ($3)}"
        exit 1
    fi
}

die () {
    echo $1
    exit 1
}

# Check that the string literals are appropriately stripped. This will
# not be the case in debug mode.

check_eq "`get_strings logging_striptest0`" "COND ERROR FATAL INFO WARNING "
check_eq "`get_strings logging_striptest2`" "COND ERROR FATAL "
check_eq "`get_strings logging_striptest10`" ""

# Check that LOG(FATAL) aborts even for large STRIP_LOG

./logging_striptest2 2>/dev/null && die "Did not abort for STRIP_LOG=2"
./logging_striptest10 2>/dev/null && die "Did not abort for STRIP_LOG=10"

echo "PASS"
