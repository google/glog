// Copyright (c) 2023, Google Inc.
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
// Author: Sergey Ioffe

// The common part of the striplog tests.

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iosfwd>
#include <string>

#include "base/commandlineflags.h"
#include "config.h"
#include "glog/logging.h"

GLOG_DEFINE_bool(check_mode, false, "Prints 'opt' or 'dbg'");

using std::string;
using namespace google;

int CheckNoReturn(bool b) {
  string s;
  if (b) {
    LOG(FATAL) << "Fatal";
    return 0;  // Workaround for MSVC warning C4715
  } else {
    return 0;
  }
}

struct A {};
std::ostream& operator<<(std::ostream& str, const A&) { return str; }

namespace {
void handle_abort(int /*code*/) { std::exit(EXIT_FAILURE); }
}  // namespace

int main(int, char* argv[]) {
#if defined(_MSC_VER)
  // Avoid presenting an interactive dialog that will cause the test to time
  // out.
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif  // defined(_MSC_VER)
  std::signal(SIGABRT, handle_abort);

  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);
  if (FLAGS_check_mode) {
    printf("%s\n", DEBUG_MODE ? "dbg" : "opt");
    return 0;
  }
  LOG(INFO) << "TESTMESSAGE INFO";
  LOG(WARNING) << 2 << "something"
               << "TESTMESSAGE WARNING" << 1 << 'c' << A() << std::endl;
  LOG(ERROR) << "TESTMESSAGE ERROR";
  bool flag = true;
  (flag ? LOG(INFO) : LOG(ERROR)) << "TESTMESSAGE COND";
  LOG(FATAL) << "TESTMESSAGE FATAL";
}
