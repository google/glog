// Copyright (c) 2008, Google Inc.
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

#include "config.h"
#include "utilities.h"

#include <cstdio>
#include <cstdlib>

#include <csignal>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <ctime>
#if defined(HAVE_SYSCALL_H)
#include <syscall.h>                 // for syscall()
#elif defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>                 // for syscall()
#endif
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>  // For geteuid.
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "base/googleinit.h"

using std::string;

_START_GOOGLE_NAMESPACE_

static const char* g_program_invocation_short_name = nullptr;

bool IsGoogleLoggingInitialized() {
  return g_program_invocation_short_name != nullptr;
}

_END_GOOGLE_NAMESPACE_

// The following APIs are all internal.
#ifdef HAVE_STACKTRACE

#include "stacktrace.h"
#include "symbolize.h"
#include "base/commandlineflags.h"

GLOG_DEFINE_bool(symbolize_stacktrace, true,
                 "Symbolize the stack trace in the tombstone");

_START_GOOGLE_NAMESPACE_

using DebugWriter = void(const char*, void*);

// The %p field width for printf() functions is two characters per byte.
// For some environments, add two extra bytes for the leading "0x".
static const int kPrintfPointerFieldWidth = 2 + 2 * sizeof(void*);

static void DebugWriteToStderr(const char* data, void *) {
  // This one is signal-safe.
  if (write(STDERR_FILENO, data, strlen(data)) < 0) {
    // Ignore errors.
  }
#if defined(__ANDROID__)
  // ANDROID_LOG_FATAL as fatal error occurred and now is dumping call stack.
  __android_log_write(ANDROID_LOG_FATAL,
                      glog_internal_namespace_::ProgramInvocationShortName(),
                      data);
#endif
}

static void DebugWriteToString(const char* data, void *arg) {
  reinterpret_cast<string*>(arg)->append(data);
}

#ifdef HAVE_SYMBOLIZE
// Print a program counter and its symbol name.
static void DumpPCAndSymbol(DebugWriter *writerfn, void *arg, void *pc,
                            const char * const prefix) {
  char tmp[1024];
  const char *symbol = "(unknown)";
  // Symbolizes the previous address of pc because pc may be in the
  // next function.  The overrun happens when the function ends with
  // a call to a function annotated noreturn (e.g. CHECK).
  if (Symbolize(reinterpret_cast<char *>(pc) - 1, tmp, sizeof(tmp))) {
      symbol = tmp;
  }
  char buf[1024];
  snprintf(buf, sizeof(buf), "%s@ %*p  %s\n",
           prefix, kPrintfPointerFieldWidth, pc, symbol);
  writerfn(buf, arg);
}
#endif

static void DumpPC(DebugWriter *writerfn, void *arg, void *pc,
                   const char * const prefix) {
  char buf[100];
  snprintf(buf, sizeof(buf), "%s@ %*p\n",
           prefix, kPrintfPointerFieldWidth, pc);
  writerfn(buf, arg);
}

// Dump current stack trace as directed by writerfn
static void DumpStackTrace(int skip_count, DebugWriter *writerfn, void *arg) {
  // Print stack trace
  void* stack[32];
  int depth = GetStackTrace(stack, ARRAYSIZE(stack), skip_count+1);
  for (int i = 0; i < depth; i++) {
#if defined(HAVE_SYMBOLIZE)
    if (FLAGS_symbolize_stacktrace) {
      DumpPCAndSymbol(writerfn, arg, stack[i], "    ");
    } else {
      DumpPC(writerfn, arg, stack[i], "    ");
    }
#else
    DumpPC(writerfn, arg, stack[i], "    ");
#endif
  }
}

#ifdef __GNUC__
__attribute__((noreturn))
#endif
static void
DumpStackTraceAndExit() {
  DumpStackTrace(1, DebugWriteToStderr, nullptr);

  // TODO(hamaji): Use signal instead of sigaction?
  if (IsFailureSignalHandlerInstalled()) {
    // Set the default signal handler for SIGABRT, to avoid invoking our
    // own signal handler installed by InstallFailureSignalHandler().
#ifdef HAVE_SIGACTION
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_handler = SIG_DFL;
    sigaction(SIGABRT, &sig_action, nullptr);
#elif defined(GLOG_OS_WINDOWS)
    signal(SIGABRT, SIG_DFL);
#endif  // HAVE_SIGACTION
  }

  abort();
}

_END_GOOGLE_NAMESPACE_

#endif  // HAVE_STACKTRACE

_START_GOOGLE_NAMESPACE_

namespace glog_internal_namespace_ {

const char* ProgramInvocationShortName() {
  if (g_program_invocation_short_name != nullptr) {
    return g_program_invocation_short_name;
  } else {
    // TODO(hamaji): Use /proc/self/cmdline and so?
    return "UNKNOWN";
  }
}

#ifdef GLOG_OS_WINDOWS
struct timeval {
  long tv_sec, tv_usec;
};

// Based on: http://www.google.com/codesearch/p?hl=en#dR3YEbitojA/os_win32.c&q=GetSystemTimeAsFileTime%20license:bsd
// See COPYING for copyright information.
static int gettimeofday(struct timeval *tv, void* /*tz*/) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
#endif
#define EPOCHFILETIME (116444736000000000ULL)
  FILETIME ft;
  ULARGE_INTEGER li;
  uint64 tt;

  GetSystemTimeAsFileTime(&ft);
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  tt = (li.QuadPart - EPOCHFILETIME) / 10;
  tv->tv_sec = tt / 1000000;
  tv->tv_usec = tt % 1000000;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

  return 0;
}
#endif

int64 CycleClock_Now() {
  // TODO(hamaji): temporary impementation - it might be too slow.
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return static_cast<int64>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

int64 UsecToCycles(int64 usec) {
  return usec;
}

WallTime WallTime_Now() {
  // Now, cycle clock is retuning microseconds since the epoch.
  return CycleClock_Now() * 0.000001;
}

static int32 g_main_thread_pid = getpid();
int32 GetMainThreadPid() {
  return g_main_thread_pid;
}

bool PidHasChanged() {
  int32 pid = getpid();
  if (g_main_thread_pid == pid) {
    return false;
  }
  g_main_thread_pid = pid;
  return true;
}

pid_t GetTID() {
  // On Linux and MacOSX, we try to use gettid().
#if defined GLOG_OS_LINUX || defined GLOG_OS_MACOSX
#ifndef __NR_gettid
#ifdef GLOG_OS_MACOSX
#define __NR_gettid SYS_gettid
#elif ! defined __i386__
#error "Must define __NR_gettid for non-x86 platforms"
#else
#define __NR_gettid 224
#endif
#endif
  static bool lacks_gettid = false;
  if (!lacks_gettid) {
#if (defined(GLOG_OS_MACOSX) && defined(HAVE_PTHREAD_THREADID_NP))
    uint64_t tid64;
    const int error = pthread_threadid_np(nullptr, &tid64);
    pid_t tid = error ? -1 : static_cast<pid_t>(tid64);
#else
    auto tid = static_cast<pid_t>(syscall(__NR_gettid));
#endif
    if (tid != -1) {
      return tid;
    }
    // Technically, this variable has to be volatile, but there is a small
    // performance penalty in accessing volatile variables and there should
    // not be any serious adverse effect if a thread does not immediately see
    // the value change to "true".
    lacks_gettid = true;
  }
#endif  // GLOG_OS_LINUX || GLOG_OS_MACOSX

  // If gettid() could not be used, we use one of the following.
#if defined GLOG_OS_LINUX
  return getpid();  // Linux:  getpid returns thread ID when gettid is absent
#elif defined GLOG_OS_WINDOWS && !defined GLOG_OS_CYGWIN
  return static_cast<pid_t>(GetCurrentThreadId());
#elif defined(HAVE_PTHREAD)
  // If none of the techniques above worked, we use pthread_self().
  return (pid_t)(uintptr_t)pthread_self();
#else
  return -1;
#endif
}

const char* const_basename(const char* filepath) {
  const char* base = strrchr(filepath, '/');
#ifdef GLOG_OS_WINDOWS  // Look for either path separator in Windows
  if (!base)
    base = strrchr(filepath, '\\');
#endif
  return base ? (base+1) : filepath;
}

static string g_my_user_name;
const string& MyUserName() {
  return g_my_user_name;
}
static void MyUserNameInitializer() {
  // TODO(hamaji): Probably this is not portable.
#if defined(GLOG_OS_WINDOWS)
  const char* user = getenv("USERNAME");
#else
  const char* user = getenv("USER");
#endif
  if (user != nullptr) {
    g_my_user_name = user;
  } else {
#if defined(HAVE_PWD_H) && defined(HAVE_UNISTD_H)
    struct passwd pwd;
    struct passwd* result = nullptr;
    char buffer[1024] = {'\0'};
    uid_t uid = geteuid();
    int pwuid_res = getpwuid_r(uid, &pwd, buffer, sizeof(buffer), &result);
    if (pwuid_res == 0 && result) {
      g_my_user_name = pwd.pw_name;
    } else {
      snprintf(buffer, sizeof(buffer), "uid%d", uid);
      g_my_user_name = buffer;
    }
#endif
    if (g_my_user_name.empty()) {
      g_my_user_name = "invalid-user";
    }
  }
}
REGISTER_MODULE_INITIALIZER(utilities, MyUserNameInitializer())

#ifdef HAVE_STACKTRACE
void DumpStackTraceToString(string* stacktrace) {
  DumpStackTrace(1, DebugWriteToString, stacktrace);
}
#endif

// We use an atomic operation to prevent problems with calling CrashReason
// from inside the Mutex implementation (potentially through RAW_CHECK).
static const CrashReason* g_reason = nullptr;

void SetCrashReason(const CrashReason* r) {
  sync_val_compare_and_swap(&g_reason,
                            reinterpret_cast<const CrashReason*>(0),
                            r);
}

void InitGoogleLoggingUtilities(const char* argv0) {
  CHECK(!IsGoogleLoggingInitialized())
      << "You called InitGoogleLogging() twice!";
  const char* slash = strrchr(argv0, '/');
#ifdef GLOG_OS_WINDOWS
  if (!slash)  slash = strrchr(argv0, '\\');
#endif
  g_program_invocation_short_name = slash ? slash + 1 : argv0;

#ifdef HAVE_STACKTRACE
  InstallFailureFunction(&DumpStackTraceAndExit);
#endif
}

void ShutdownGoogleLoggingUtilities() {
  CHECK(IsGoogleLoggingInitialized())
      << "You called ShutdownGoogleLogging() without calling InitGoogleLogging() first!";
  g_program_invocation_short_name = nullptr;
#ifdef HAVE_SYSLOG_H
  closelog();
#endif
}

}  // namespace glog_internal_namespace_

_END_GOOGLE_NAMESPACE_

// Make an implementation of stacktrace compiled.
#ifdef STACKTRACE_H
# include STACKTRACE_H
# if 0
// For include scanners which can't handle macro expansions.
#  include "stacktrace_libunwind-inl.h"
#  include "stacktrace_x86-inl.h"
#  include "stacktrace_x86_64-inl.h"
#  include "stacktrace_powerpc-inl.h"
#  include "stacktrace_generic-inl.h"
# endif
#endif
