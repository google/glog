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
// Author: Maxim Lifantsev
//
// logging_unittest.cc covers the functionality herein

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <ostream>
#include <streambuf>
#include <thread>

#include "config.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>  // for close() and write()
#endif
#if defined(HAVE_SYSCALL_H)
#  include <syscall.h>  // for syscall()
#elif defined(HAVE_SYS_SYSCALL_H)
#  include <sys/syscall.h>  // for syscall()
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <fcntl.h>  // for open()

#include "glog/logging.h"
#include "glog/raw_logging.h"
#include "stacktrace.h"
#include "utilities.h"

#if (defined(HAVE_SYSCALL_H) || defined(HAVE_SYS_SYSCALL_H)) &&    \
    (!(defined(GLOG_OS_MACOSX)) && !(defined(GLOG_OS_OPENBSD))) && \
    !defined(GLOG_OS_EMSCRIPTEN)
#  define safe_write(fd, s, len) syscall(SYS_write, fd, s, len)
#else
// Not so safe, but what can you do?
#  define safe_write(fd, s, len) write(fd, s, len)
#endif

namespace google {

#if defined(__GNUC__)
#  define GLOG_ATTRIBUTE_FORMAT(archetype, stringIndex, firstToCheck) \
    __attribute__((format(archetype, stringIndex, firstToCheck)))
#  define GLOG_ATTRIBUTE_FORMAT_ARG(stringIndex) \
    __attribute__((format_arg(stringIndex)))
#else
#  define GLOG_ATTRIBUTE_FORMAT(archetype, stringIndex, firstToCheck)
#  define GLOG_ATTRIBUTE_FORMAT_ARG(stringIndex)
#endif

// CAVEAT: std::vsnprintf called from *DoRawLog below has some (exotic) code
// paths that invoke malloc() and getenv() that might acquire some locks. If
// this becomes a problem we should reimplement a subset of std::vsnprintf that
// does not need locks and malloc.

// Helper for RawLog__ below.
// *DoRawLog writes to *buf of *size and move them past the written portion.
// It returns true iff there was no overflow or error.
GLOG_ATTRIBUTE_FORMAT(printf, 3, 4)
static bool DoRawLog(char** buf, size_t* size, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = std::vsnprintf(*buf, *size, format, ap);
  va_end(ap);
  if (n < 0 || static_cast<size_t>(n) > *size) return false;
  *size -= static_cast<size_t>(n);
  *buf += n;
  return true;
}

// Helper for RawLog__ below.
inline static bool VADoRawLog(char** buf, size_t* size, const char* format,
                              va_list ap) {
#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
  int n = std::vsnprintf(*buf, *size, format, ap);
#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
  if (n < 0 || static_cast<size_t>(n) > *size) return false;
  *size -= static_cast<size_t>(n);
  *buf += n;
  return true;
}

static const int kLogBufSize = 3000;
static std::once_flag crashed;
static logging::internal::CrashReason crash_reason;
static char crash_buf[kLogBufSize + 1] = {0};  // Will end in '\0'

namespace {
template <std::size_t N>
class StaticStringBuf : public std::streambuf {
 public:
  StaticStringBuf() {
    setp(std::begin(data_), std::end(data_));
    setg(std::begin(data_), std::begin(data_), std::end(data_));
  }
  const char* data() noexcept {
    if (pptr() != pbase() && pptr() != epptr() && *(pptr() - 1) != '\0') {
      sputc('\0');
    }
    return data_;
  }

 private:
  char data_[N];
};
}  // namespace

GLOG_ATTRIBUTE_FORMAT(printf, 4, 5)
void RawLog__(LogSeverity severity, const char* file, int line,
              const char* format, ...) {
  if (!(FLAGS_logtostdout || FLAGS_logtostderr ||
        severity >= FLAGS_stderrthreshold || FLAGS_alsologtostderr ||
        !IsGoogleLoggingInitialized())) {
    return;  // this stderr log message is suppressed
  }

  // We do not have any any option other that string streams to obtain the
  // thread identifier as the corresponding value is not convertible to an
  // integer. Use a statically allocated buffer to avoid dynamic memory
  // allocations.
  StaticStringBuf<kLogBufSize> sbuf;
  std::ostream oss(&sbuf);

  oss << std::setw(5) << std::this_thread::get_id();

  // can't call localtime_r here: it can allocate
  char buffer[kLogBufSize];
  char* buf = buffer;
  size_t size = sizeof(buffer);

  // NOTE: this format should match the specification in base/logging.h
  DoRawLog(&buf, &size, "%c00000000 00:00:00.000000 %s %s:%d] RAW: ",
           GetLogSeverityName(severity)[0], sbuf.data(),
           const_basename(const_cast<char*>(file)), line);

  // Record the position and size of the buffer after the prefix
  const char* msg_start = buf;
  const size_t msg_size = size;

  va_list ap;
  va_start(ap, format);
  bool no_chop = VADoRawLog(&buf, &size, format, ap);
  va_end(ap);
  if (no_chop) {
    DoRawLog(&buf, &size, "\n");
  } else {
    DoRawLog(&buf, &size, "RAW_LOG ERROR: The Message was too long!\n");
  }
  // We make a raw syscall to write directly to the stderr file descriptor,
  // avoiding FILE buffering (to avoid invoking malloc()), and bypassing
  // libc (to side-step any libc interception).
  // We write just once to avoid races with other invocations of RawLog__.
  safe_write(fileno(stderr), buffer, strlen(buffer));
  if (severity == GLOG_FATAL) {
    std::call_once(crashed, [file, line, msg_start, msg_size] {
      crash_reason.filename = file;
      crash_reason.line_number = line;
      memcpy(crash_buf, msg_start, msg_size);  // Don't include prefix
      crash_reason.message = crash_buf;
#ifdef HAVE_STACKTRACE
      crash_reason.depth =
          GetStackTrace(crash_reason.stack, ARRAYSIZE(crash_reason.stack), 1);
#else
      crash_reason.depth = 0;
#endif
      SetCrashReason(&crash_reason);
    });
    LogMessage::Fail();  // abort()
  }
}

}  // namespace google
