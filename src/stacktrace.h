// Copyright 2000 - 2007 Google Inc.
// All rights reserved.
//
// Routines to extract the current stack trace.  These functions are
// thread-safe.

#ifndef BASE_STACKTRACE_H_
#define BASE_STACKTRACE_H_

#include "config.h"

_START_GOOGLE_NAMESPACE_

// Skips the most recent "skip_count" stack frames (also skips the
// frame generated for the "GetStackFrames" routine itself), and then
// records the pc values for up to the next "max_depth" frames in
// "pcs", and the corresponding stack frame sizes in "sizes".  Returns
// the number of values recorded in "pcs"/"sizes".
//
// Example:
//      main() { foo(); }
//      foo() { bar(); }
//      bar() {
//        void* pcs[10];
//        int sizes[10];
//        int depth = GetStackFrames(pcs, sizes, 10, 1);
//      }
//
// The GetStackFrames call will skip the frame for "bar".  It will
// return 2 and will produce pc values that map to the following
// procedures:
//      pcs[0]       foo
//      pcs[1]       main
// (Actually, there may be a few more entries after "main" to account for
// startup procedures.)
// And corresponding stack frame sizes will also be recorded:
//    sizes[0]       16
//    sizes[1]       16
// (Stack frame sizes of 16 above are just for illustration purposes.)
// Stack frame sizes of 0 or less indicate that those frame sizes couldn't
// be identified.
//
// This routine may return fewer stack frame entries than are
// available. Also note that "pcs" and "sizes" must both be non-NULL.
extern int GetStackFrames(void** pcs, int* sizes, int max_depth,
                          int skip_count);

// This is similar to the GetStackFrames routine, except that it returns
// the stack trace only, and not the stack frame sizes as well.
// Example:
//      main() { foo(); }
//      foo() { bar(); }
//      bar() {
//        void* result[10];
//        int depth = GetStackFrames(result, 10, 1);
//      }
//
// This produces:
//      result[0]       foo
//      result[1]       main
//           ....       ...
//
// "result" must not be NULL.
extern int GetStackTrace(void** result, int max_depth, int skip_count);

_END_GOOGLE_NAMESPACE_

#endif  // BASE_STACKTRACE_H_
