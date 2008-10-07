// Copyright 2000 - 2007 Google Inc.
// All rights reserved.
//
// Produce stack trace.
//
// There are three different ways we can try to get the stack trace:
//
// 1) Our hand-coded stack-unwinder.  This depends on a certain stack
//    layout, which is used by gcc (and those systems using a
//    gcc-compatible ABI) on x86 systems, at least since gcc 2.95.
//    It uses the frame pointer to do its work.
//
// 2) The libunwind library.  This is still in development, and as a
//    separate library adds a new dependency, abut doesn't need a frame
//    pointer.  It also doesn't call malloc.
//
// 3) The gdb unwinder -- also the one used by the c++ exception code.
//    It's obviously well-tested, but has a fatal flaw: it can call
//    malloc() from the unwinder.  This is a problem because we're
//    trying to use the unwinder to instrument malloc().
//
// Note: if you add a new implementation here, make sure it works
// correctly when GetStackTrace() is called with max_depth == 0.
// Some code may do that.

#include "config.h"

// First, the i386 case.
#if defined(__i386__) && __GNUC__ >= 2
# if defined(HAVE_EXECINFO_H)
#   include "stacktrace_generic-inl.h"
# elif !defined(NO_FRAME_POINTER)
#   include "stacktrace_x86-inl.h"
# endif

// Now, the x86_64 case.
#elif defined(__x86_64__) && __GNUC__ >= 2
# if defined(HAVE_EXECINFO_H)
#   include "stacktrace_generic-inl.h"
# elif !defined(NO_FRAME_POINTER)
#   include "stacktrace_x86-inl.h"
# elif 1
    // This is the unwinder used by gdb, which can call malloc (see above).
#   include "stacktrace_x86_64-inl.h"
# elif 0     // We assume libunwind is installed on this machine
    // Use the libunwind library.
    // There's no way to enable it except for manually
    // editing this file (by replacing this "elif 0" with "elif 1", e.g.).
#   define UNW_LOCAL_ONLY
#   include "stacktrace_libunwind-inl.h"
# elif defined(__linux)
#   error Cannnot calculate stack trace: need either libunwind or frame-pointers
# else
#   error Cannnot calculate stack trace: need libunwind
# endif

// The PowerPC case
#elif (defined(__ppc__) || defined(__PPC__)) && __GNUC__ >= 2
# if defined(HAVE_EXECINFO_H)
#   include "stacktrace_generic-inl.h"
# elif defined(STACKTRACE_WITH_FRAME_POINTER)
#   include "stacktrace_powerpc-inl.h"
# endif

// OK, those are all the processors we know how to deal with.
#else
# error Cannot calculate stack trace: will need to write for your environment
#endif
