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

#include <glog/logging.h>

#include "base/mutex.h"

namespace google {

// A mutex that allows only one thread to log at a time, to keep things from
// getting jumbled.  Some other very uncommon logging operations (like
// changing the destination file for log messages of a given severity) also
// lock this mutex.  Please be sure that anybody who might possibly need to
// lock it does so.
GLOG_NO_EXPORT extern Mutex log_mutex;

// Has the user called SetExitOnDFatal(true)?
GLOG_NO_EXPORT extern bool exit_on_dfatal;

namespace base {
namespace internal {

GLOG_NO_EXPORT bool GetExitOnDFatal();
GLOG_NO_EXPORT void SetExitOnDFatal(bool value);

}  // namespace internal
}  // namespace base

namespace glog_internal_namespace_ {

// Get the part of filepath after the last path separator.
// (Doesn't modify filepath, contrary to basename() in libgen.h.)
GLOG_NO_EXPORT const char* const_basename(const char* filepath);

}  // namespace glog_internal_namespace_

}  // namespace google
