// Copyright (c) 1999, Google Inc.
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

#include <cstring>

#include <glog/platform.h>

#include "base.h"

namespace google {

Mutex log_mutex;
bool exit_on_dfatal = true;

namespace base {
namespace internal {

bool GetExitOnDFatal() {
  MutexLock l(&log_mutex);
  return exit_on_dfatal;
}

// Determines whether we exit the program for a LOG(DFATAL) message in
// debug mode.  It does this by skipping the call to Fail/FailQuietly.
// This is intended for testing only.
//
// This can have some effects on LOG(FATAL) as well.  Failure messages
// are always allocated (rather than sharing a buffer), the crash
// reason is not recorded, the "gwq" status message is not updated,
// and the stack trace is not recorded.  The LOG(FATAL) *will* still
// exit the program.  Since this function is used only in testing,
// these differences are acceptable.
void SetExitOnDFatal(bool value) {
  MutexLock l(&log_mutex);
  exit_on_dfatal = value;
}

}  // namespace internal
}  // namespace base


namespace glog_internal_namespace_ {

const char* const_basename(const char* filepath) {
  const char* base = std::strrchr(filepath, '/');
#ifdef GLOG_OS_WINDOWS  // Look for either path separator in Windows
  if (!base)
    base = std::strrchr(filepath, '\\');
#endif
  return base ? (base+1) : filepath;
}

}  // namespace glog_internal_namespace_

}  // namespace google
