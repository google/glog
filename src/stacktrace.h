// Copyright 2000 - 2007 Google Inc.
// All rights reserved.
//
// Routines to extract the current stack trace.  These functions are
// thread-safe.

#ifndef BASE_STACKTRACE_H_
#define BASE_STACKTRACE_H_

#include "config.h"

_START_GOOGLE_NAMESPACE_

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
