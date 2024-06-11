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
// Author: Sergiu Deitsch
//

#include <glog/logging.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>

namespace {

struct MyLogSink : google::LogSink {  // (1)!
  void send(google::LogSeverity severity, const char* /*full_filename*/,
            const char* base_filename, int line,
            const google::LogMessageTime& /*time*/, const char* message,
            std::size_t message_len) override {
    std::cout << google::GetLogSeverityName(severity) << ' ' << base_filename
              << ':' << line << ' ';
    std::copy_n(message, message_len,
                std::ostreambuf_iterator<char>{std::cout});
    std::cout << '\n';
  }
};

}  // namespace

int main(int /*argc*/, char** argv) {
  google::InitGoogleLogging(argv[0]);

  MyLogSink sink;
  google::AddLogSink(&sink);  // (2)!

  LOG(INFO) << "logging to MySink";

  google::RemoveLogSink(&sink);  // (3)!

  // We can directly log to a sink without registering it
  LOG_TO_SINK(&sink, INFO) << "direct logging";  // (4)!
  LOG_TO_SINK_BUT_NOT_TO_LOGFILE(&sink, INFO)
      << "direct logging but not to file";
}
