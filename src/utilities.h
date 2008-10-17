// Define utilties for glog internal usage.

#ifndef UTILITIES_H__
#define UTILITIES_H__

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__)
# define OS_WINDOWS
#elif defined(linux) || defined(__linux) || defined(__linux__)
# define OS_LINUX
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
# define OS_MACOSX
#elif defined(__FreeBSD__)
# define OS_FREEBSD
#else
// TODO(hamaji): Add other platforms.
#endif

// printf macros for size_t, in the style of inttypes.h
#ifdef _LP64
#define __PRIS_PREFIX "z"
#else
#define __PRIS_PREFIX
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

#include "base/mutex.h"  // This must go first so we get _XOPEN_SOURCE

#include <string>

#include "config.h"
#include "glog/logging.h"

#if defined(HAVE_EXECINFO_H)
# define HAVE_STACKTRACE
#elif defined(STACKTRACE_WITH_FRAME_POINTER)
# define HAVE_STACKTRACE
#endif

#if defined(__ELF__)  // defined by gcc on Linux
# define HAVE_SYMBOLIZE
#elif defined(OS_MACOSX) && defined(HAVE_DLADDR)
// Use dladdr to symbolize.
# define HAVE_SYMBOLIZE
#endif

_START_GOOGLE_NAMESPACE_

namespace glog_internal_namespace_ {

#ifdef HAVE___ATTRIBUTE__
# define ATTRIBUTE_NOINLINE __attribute__ ((noinline))
# define HAVE_ATTRIBUTE_NOINLINE
#else
# define ATTRIBUTE_NOINLINE
#endif

const char* ProgramInvocationShortName();

bool IsGoogleLoggingInitialized();

bool is_default_thread();

int64 CycleClock_Now();

int64 UsecToCycles(int64 usec);

int32 GetMainThreadPid();

const std::string& MyUserName();

}
_END_GOOGLE_NAMESPACE_

using namespace GOOGLE_NAMESPACE::glog_internal_namespace_;

#endif  // UTILITIES_H__
