// Copyright (c) 2024, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Routines to extract the current stack trace.  These functions are
// thread-safe.

#ifndef GLOG_INTERNAL_STACKTRACE_H
#define GLOG_INTERNAL_STACKTRACE_H

#include "glog/platform.h"

#if defined(GLOG_USE_GLOG_EXPORT)
#  include "glog/export.h"
#endif

#if !defined(GLOG_NO_EXPORT)
#  error "stacktrace.h" was not included correctly.
#endif

#include "config.h"
#if defined(HAVE_LIBUNWIND)
#  define STACKTRACE_H "stacktrace_libunwind-inl.h"
#elif defined(HAVE_UNWIND)
#  define STACKTRACE_H "stacktrace_unwind-inl.h"
#elif !defined(NO_FRAME_POINTER)
#  if defined(__i386__) && __GNUC__ >= 2
#    define STACKTRACE_H "stacktrace_x86-inl.h"
#  elif (defined(__ppc__) || defined(__PPC__)) && __GNUC__ >= 2
#    define STACKTRACE_H "stacktrace_powerpc-inl.h"
#  elif defined(GLOG_OS_WINDOWS)
#    define STACKTRACE_H "stacktrace_windows-inl.h"
#  endif
#endif

#if !defined(STACKTRACE_H) && defined(HAVE_EXECINFO_BACKTRACE)
#  define STACKTRACE_H "stacktrace_generic-inl.h"
#endif

#if defined(STACKTRACE_H)
#  define HAVE_STACKTRACE
#endif

namespace google {
inline namespace glog_internal_namespace_ {

#if defined(HAVE_STACKTRACE)

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
// "result" must not be nullptr.
GLOG_NO_EXPORT int GetStackTrace(void** result, int max_depth, int skip_count);

#endif  // defined(HAVE_STACKTRACE)

}  // namespace glog_internal_namespace_
}  // namespace google

#endif  // GLOG_INTERNAL_STACKTRACE_H
