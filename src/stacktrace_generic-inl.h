// Copyright 2000 - 2007 Google Inc.
// All rights reserved.
//
// Portable implementation - just use glibc
//
// Note:  The glibc implementation may cause a call to malloc.
// This can cause a deadlock in HeapProfiler.
#include <execinfo.h>
#include <string.h>
#include "stacktrace.h"

_START_GOOGLE_NAMESPACE_

// If you change this function, also change GetStackFrames below.
int GetStackTrace(void** result, int max_depth, int skip_count) {
  static const int kStackLength = 64;
  void * stack[kStackLength];
  int size;

  size = backtrace(stack, kStackLength);
  skip_count++;  // we want to skip the current frame as well
  int result_count = size - skip_count;
  if (result_count < 0)
    result_count = 0;
  if (result_count > max_depth)
    result_count = max_depth;
  for (int i = 0; i < result_count; i++)
    result[i] = stack[i + skip_count];

  return result_count;
}

// If you change this function, also change GetStackTrace above:
//
// This GetStackFrames routine shares a lot of code with GetStackTrace
// above. This code could have been refactored into a common routine,
// and then both GetStackTrace/GetStackFrames could call that routine.
// There are two problems with that:
//
// (1) The performance of the refactored-code suffers substantially - the
//     refactored needs to be able to record the stack trace when called
//     from GetStackTrace, and both the stack trace and stack frame sizes,
//     when called from GetStackFrames - this introduces enough new
//     conditionals that GetStackTrace performance can degrade by as much
//     as 50%.
//
// (2) Whether the refactored routine gets inlined into GetStackTrace and
//     GetStackFrames depends on the compiler, and we can't guarantee the
//     behavior either-way, even with "__attribute__ ((always_inline))"
//     or "__attribute__ ((noinline))". But we need this guarantee or the
//     frame counts may be off by one.
//
// Both (1) and (2) can be addressed without this code duplication, by
// clever use of template functions, and by defining GetStackTrace and
// GetStackFrames as macros that expand to these template functions.
// However, this approach comes with its own set of problems - namely,
// macros and  preprocessor trouble - for example,  if GetStackTrace
// and/or GetStackFrames is ever defined as a member functions in some
// class, we are in trouble.
int GetStackFrames(void** pcs, int* sizes, int max_depth, int skip_count) {
  static const int kStackLength = 64;
  void * stack[kStackLength];
  int size;

  size = backtrace(stack, kStackLength);
  skip_count++;  // we want to skip the current frame as well
  int result_count = size - skip_count;
  if (result_count < 0)
    result_count = 0;
  if (result_count > max_depth)
    result_count = max_depth;
  for (int i = 0; i < result_count; i++)
    pcs[i] = stack[i + skip_count];

  // No implementation for finding out the stack frame sizes yet.
  memset(sizes, 0, sizeof(*sizes) * result_count);

  return result_count;
}

_END_GOOGLE_NAMESPACE_
