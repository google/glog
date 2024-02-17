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
// Author: Shinichiro Hamaji
//         Sergiu Deitsch
//
// Define utilities for glog internal usage.

#ifndef GLOG_INTERNAL_UTILITIES_H
#define GLOG_INTERNAL_UTILITIES_H

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

// printf macros for size_t, in the style of inttypes.h
#ifdef _LP64
#  define __PRIS_PREFIX "z"
#else
#  define __PRIS_PREFIX
#endif

// Use these macros after a % in a printf format string
// to get correct 32/64 bit behavior, like this:
// size_t size = records.size();
// printf("%"PRIuS"\n", size);

#define PRIdS __PRIS_PREFIX "d"
#define PRIxS __PRIS_PREFIX "x"
#define PRIuS __PRIS_PREFIX "u"
#define PRIXS __PRIS_PREFIX "X"
#define PRIoS __PRIS_PREFIX "o"

#include "config.h"
#include "glog/platform.h"
#if defined(GLOG_USE_WINDOWS_PORT)
#  include "port.h"
#endif
#if defined(HAVE_UNISTD_H)
#  include <unistd.h>
#endif
#if !defined(HAVE_SSIZE_T)
#  if defined(GLOG_OS_WINDOWS)
#    include <basetsd.h>
using ssize_t = SSIZE_T;
#  else
using ssize_t = std::ptrdiff_t;
#  endif
#endif

#include "glog/log_severity.h"
#include "glog/types.h"

// There are three different ways we can try to get the stack trace:
//
// 1) The libunwind library.  This is still in development, and as a
//    separate library adds a new dependency, but doesn't need a frame
//    pointer.  It also doesn't call malloc.
//
// 2) Our hand-coded stack-unwinder.  This depends on a certain stack
//    layout, which is used by gcc (and those systems using a
//    gcc-compatible ABI) on x86 systems, at least since gcc 2.95.
//    It uses the frame pointer to do its work.
//
// 3) The gdb unwinder -- also the one used by the c++ exception code.
//    It's obviously well-tested, but has a fatal flaw: it can call
//    malloc() from the unwinder.  This is a problem because we're
//    trying to use the unwinder to instrument malloc().
//
// 4) The Windows API CaptureStackTrace.
//
// Note: if you add a new implementation here, make sure it works
// correctly when GetStackTrace() is called with max_depth == 0.
// Some code may do that.

#ifndef ARRAYSIZE
// There is a better way, but this is good enough for our purpose.
#  define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

namespace google {

namespace logging {
namespace internal {

struct CrashReason {
  CrashReason() = default;

  const char* filename{nullptr};
  int line_number{0};
  const char* message{nullptr};

  // We'll also store a bit of stack trace context at the time of crash as
  // it may not be available later on.
  void* stack[32];
  int depth{0};
};

}  // namespace internal
}  // namespace logging

inline namespace glog_internal_namespace_ {

#if defined(__has_attribute)
#  if __has_attribute(noinline)
#    define ATTRIBUTE_NOINLINE __attribute__((noinline))
#    define HAVE_ATTRIBUTE_NOINLINE
#  endif
#endif

#if !defined(HAVE_ATTRIBUTE_NOINLINE)
#  if defined(GLOG_OS_WINDOWS)
#    define ATTRIBUTE_NOINLINE __declspec(noinline)
#    define HAVE_ATTRIBUTE_NOINLINE
#  endif
#endif

#if !defined(HAVE_ATTRIBUTE_NOINLINE)
#  define ATTRIBUTE_NOINLINE
#endif

void AlsoErrorWrite(LogSeverity severity, const char* tag,
                    const char* message) noexcept;

const char* ProgramInvocationShortName();

int32 GetMainThreadPid();
bool PidHasChanged();

const std::string& MyUserName();

// Get the part of filepath after the last path separator.
// (Doesn't modify filepath, contrary to basename() in libgen.h.)
const char* const_basename(const char* filepath);

void SetCrashReason(const logging::internal::CrashReason* r);

void InitGoogleLoggingUtilities(const char* argv0);
void ShutdownGoogleLoggingUtilities();

template <class Functor>
class ScopedExit final {
 public:
  template <class F, std::enable_if_t<
                         std::is_constructible<Functor, F&&>::value>* = nullptr>
  constexpr explicit ScopedExit(F&& functor) noexcept(
      std::is_nothrow_constructible<Functor, F&&>::value)
      : functor_{std::forward<F>(functor)} {}
  ~ScopedExit() noexcept(noexcept(std::declval<Functor&>()())) { functor_(); }
  ScopedExit(const ScopedExit& other) = delete;
  ScopedExit& operator=(const ScopedExit& other) = delete;
  ScopedExit(ScopedExit&& other) noexcept = delete;
  ScopedExit& operator=(ScopedExit&& other) noexcept = delete;

 private:
  Functor functor_;
};

// Thin wrapper around a file descriptor so that the file descriptor
// gets closed for sure.
class GLOG_NO_EXPORT FileDescriptor final {
  static constexpr int InvalidHandle = -1;

 public:
  constexpr FileDescriptor() noexcept : FileDescriptor{nullptr} {}
  constexpr explicit FileDescriptor(int fd) noexcept : fd_{fd} {}
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr FileDescriptor(std::nullptr_t) noexcept : fd_{InvalidHandle} {}

  FileDescriptor(const FileDescriptor& other) = delete;
  FileDescriptor& operator=(const FileDescriptor& other) = delete;

  FileDescriptor(FileDescriptor&& other) noexcept : fd_{other.release()} {}
  FileDescriptor& operator=(FileDescriptor&& other) noexcept {
    // Close the file descriptor being held and assign a new file descriptor
    // previously held by 'other' without closing it.
    reset(other.release());
    return *this;
  }

  constexpr explicit operator bool() const noexcept {
    return fd_ != InvalidHandle;
  }

  constexpr int get() const noexcept { return fd_; }

  int release() noexcept { return std::exchange(fd_, InvalidHandle); }
  void reset(std::nullptr_t) noexcept { safe_close(); }
  void reset() noexcept { reset(nullptr); }
  void reset(int fd) noexcept {
    reset();
    fd_ = fd;
  }

  int close() noexcept { return unsafe_close(); }

  ~FileDescriptor() { safe_close(); }

 private:
  int unsafe_close() noexcept { return ::close(release()); }
  void safe_close() noexcept {
    if (*this) {
      unsafe_close();
    }
  }

  int fd_;
};

// Provide variants of (in)equality comparison operators to avoid constructing
// temporaries.

constexpr bool operator==(const FileDescriptor& lhs, int rhs) noexcept {
  return lhs.get() == rhs;
}

constexpr bool operator==(int lhs, const FileDescriptor& rhs) noexcept {
  return rhs == lhs;
}

constexpr bool operator!=(const FileDescriptor& lhs, int rhs) noexcept {
  return !(lhs == rhs);
}

constexpr bool operator!=(int lhs, const FileDescriptor& rhs) noexcept {
  return !(lhs == rhs);
}

constexpr bool operator==(const FileDescriptor& lhs, std::nullptr_t) noexcept {
  return !lhs;
}

constexpr bool operator==(std::nullptr_t, const FileDescriptor& rhs) noexcept {
  return !rhs;
}

constexpr bool operator!=(const FileDescriptor& lhs, std::nullptr_t) noexcept {
  return static_cast<bool>(lhs);
}

constexpr bool operator!=(std::nullptr_t, const FileDescriptor& rhs) noexcept {
  return static_cast<bool>(rhs);
}

}  // namespace glog_internal_namespace_

}  // namespace google

template <>
struct std::default_delete<std::FILE> {
  void operator()(FILE* p) const noexcept { fclose(p); }
};

#endif  // GLOG_INTERNAL_UTILITIES_H
