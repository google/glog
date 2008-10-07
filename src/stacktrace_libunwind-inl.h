// Copyright 2005 - 2007 Google Inc.
// All rights reserved.
//
// Author: Arun Sharma
//
// Produce stack trace using libunwind

extern "C" {
#include <libunwind.h>
}
#include "base/stacktrace.h"
#include "base/raw_logging.h"
#include "base/spinlock.h"

_START_GOOGLE_NAMESPACE_

// Sometimes, we can try to get a stack trace from within a stack
// trace, because libunwind can call mmap/sbrk (maybe indirectly via
// malloc), and that mmap gets trapped and causes a stack-trace
// request.  If were to try to honor that recursive request, we'd end
// up with infinite recursion or deadlock.  Luckily, it's safe to
// ignore those subsequent traces.  In such cases, we return 0 to
// indicate the situation.
static SpinLock libunwind_lock(SpinLock::LINKER_INITIALIZED);

// If you change this function, also change GetStackFrames below.
int GetStackTrace(void** result, int max_depth, int skip_count) {
  void *ip;
  int n = 0;
  unw_cursor_t cursor;
  unw_context_t uc;

  if (!libunwind_lock.TryLock()) {
    return 0;
  }

  unw_getcontext(&uc);
  RAW_CHECK(unw_init_local(&cursor, &uc) >= 0, "unw_init_local failed");
  skip_count++;         // Do not include the "GetStackTrace" frame

  while (n < max_depth) {
    int ret = unw_get_reg(&cursor, UNW_REG_IP, (unw_word_t *) &ip);
    if (ret < 0)
      break;
    if (skip_count > 0) {
      skip_count--;
    } else {
      result[n++] = ip;
    }
    ret = unw_step(&cursor);
    if (ret <= 0)
      break;
  }

  libunwind_lock.Unlock();
  return n;
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
  void *ip;
  int n = 0;
  unw_cursor_t cursor;
  unw_context_t uc;
  unw_word_t sp = 0, next_sp = 0;

  if (!libunwind_lock.TryLock()) {
    return 0;
  }

  unw_getcontext(&uc);
  RAW_CHECK(unw_init_local(&cursor, &uc) >= 0, "unw_init_local failed");
  skip_count++;         // Do not include the "GetStackFrames" frame

  while (skip_count--) {
    if (unw_step(&cursor) <= 0 ||
        unw_get_reg(&cursor, UNW_REG_SP, &next_sp) < 0) {
      goto out;
    }
  }
  while (n < max_depth) {
    sp = next_sp;
    if (unw_get_reg(&cursor, UNW_REG_IP, (unw_word_t *) &ip) < 0)
      break;
    if (unw_step(&cursor) <= 0 ||
        unw_get_reg(&cursor, UNW_REG_SP, &next_sp)) {
      // We couldn't step any further (possibly because we reached _start).
      // Provide the last good PC we've got, and get out.
      sizes[n] = 0;
      pcs[n++] = ip;
      break;
    }
    sizes[n] = next_sp - sp;
    pcs[n++] = ip;
  }
 out:
  libunwind_lock.Unlock();
  return n;
}

_END_GOOGLE_NAMESPACE_
