// Copyright 2006 Google Inc. All Rights Reserved.
// Author: Maxim Lifantsev
//
// logging_unittest.cc covers the functionality herein

#include "utilities.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "config.h"
#include "glog/logging.h"          // To pick up flag settings etc.
#include "glog/raw_logging.h"

#if defined(HAVE_SYSCALL_H)
#include <syscall.h>                 // for syscall()
#elif defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>                 // for syscall()
#endif
#include <unistd.h>

#if defined(OS_MACOSX)
#ifndef __DARWIN_UNIX03
#define __DARWIN_UNIX03  // tells libgen.h to define basename()
#endif
#endif  // OS_MACOSX

#include <libgen.h>                      // basename()

_START_GOOGLE_NAMESPACE_

// Data for RawLog__ below. We simply pick up the latest
// time data created by a normal log message to avoid calling
// localtime_r which can allocate memory.
static struct ::tm last_tm_time_for_raw_log;

void RawLog__SetLastTime(const struct ::tm& t) {
  memcpy(&last_tm_time_for_raw_log, &t, sizeof(last_tm_time_for_raw_log));
}

// CAVEAT: vsnprintf called from *DoRawLog below has some (exotic) code paths
// that invoke malloc() and getenv() that might acquire some locks.
// If this becomes a problem we should reimplement a subset of vsnprintf
// that does not need locks and malloc.

// Helper for RawLog__ below.
// *DoRawLog writes to *buf of *size and move them past the written portion.
// It returns true iff there was no overflow or error.
static bool DoRawLog(char** buf, int* size, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = vsnprintf(*buf, *size, format, ap);
  va_end(ap);
  if (n < 0 || n > *size) return false;
  *size -= n;
  *buf += n;
  return true;
}

// Helper for RawLog__ below.
inline static bool VADoRawLog(char** buf, int* size,
                              const char* format, va_list ap) {
  int n = vsnprintf(*buf, *size, format, ap);
  if (n < 0 || n > *size) return false;
  *size -= n;
  *buf += n;
  return true;
}

void RawLog__(LogSeverity severity, const char* file, int line,
              const char* format, ...) {
  if (!(FLAGS_logtostderr || severity >= FLAGS_stderrthreshold ||
        FLAGS_alsologtostderr || !IsGoogleLoggingInitialized())) {
    return;  // this stderr log message is suppressed
  }
  // can't call localtime_r here: it can allocate
  struct ::tm& t = last_tm_time_for_raw_log;
  char buffer[3000];  // 3000 bytes should be enough for everyone... :-)
  char* buf = buffer;
  int size = sizeof(buffer);
  if (is_default_thread()) {
     DoRawLog(&buf, &size, "%c%02d%02d %02d%02d%02d %s:%d] RAW: ",
              LogSeverityNames[severity][0],
              1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
              basename(const_cast<char *>(file)), line);
  } else {
    DoRawLog(&buf, &size, "%c%02d%02d %02d%02d%02d %08x %s:%d] RAW: ",
             LogSeverityNames[severity][0],
             1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
             int(pthread_self()),
             basename(const_cast<char *>(file)), line);
  }
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
#if defined(HAVE_SYSCALL_H) || defined(HAVE_SYS_SYSCALL_H)
  syscall(SYS_write, STDERR_FILENO, buffer, strlen(buffer));
#else
  write(STDERR_FILENO, buffer, strlen(buffer));
#endif
  if (severity == FATAL)  LogMessage::Fail();
}

_END_GOOGLE_NAMESPACE_
