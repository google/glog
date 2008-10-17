#include "utilities.h"

#include <sys/time.h>
#include <time.h>

#include "base/googleinit.h"
#include "stacktrace.h"
#include "symbolize.h"

using std::string;

_START_GOOGLE_NAMESPACE_

static const char* g_program_invocation_short_name = NULL;
static pthread_t g_main_thread_id;

// The following APIs are all internal.
#ifdef HAVE_STACKTRACE

#include "stacktrace.h"
#include "symbolize.h"
#include "base/commandlineflags.h"

DEFINE_bool(symbolize_stacktrace, true,
            "Symbolize the stack trace in the tombstone");

typedef void DebugWriter(const char*, void*);

// The %p field width for printf() functions is two characters per byte.
// For some environments, add two extra bytes for the leading "0x".
static const int kPrintfPointerFieldWidth = 2 + 2 * sizeof(void*);

static void DebugWriteToStderr(const char* data, void *unused) {
  // This one is signal-safe.
  write(STDERR_FILENO, data, strlen(data));
}

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

// Print a program counter and the corresponding stack frame size.
static void DumpPCAndFrameSize(DebugWriter *writerfn, void *arg, void *pc,
                               int framesize, const char * const prefix) {
  char buf[100];
  if (framesize <= 0) {
    snprintf(buf, sizeof(buf), "%s@ %*p  (unknown)\n",
             prefix, kPrintfPointerFieldWidth, pc);
  } else {
    snprintf(buf, sizeof(buf), "%s@ %*p  %9d\n",
             prefix, kPrintfPointerFieldWidth, pc, framesize);
  }
  writerfn(buf, arg);
}

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
  int depth = GetStackTrace(stack, sizeof(stack)/sizeof(*stack), skip_count+1);
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

static void DumpStackTraceAndExit() {
  DumpStackTrace(1, DebugWriteToStderr, NULL);
  abort();
}
#endif

namespace glog_internal_namespace_ {

const char* ProgramInvocationShortName() {
  if (g_program_invocation_short_name != NULL) {
    return g_program_invocation_short_name;
  } else {
    // TODO(hamaji): Use /proc/self/cmdline and so?
    return "UNKNOWN";
  }
}

bool IsGoogleLoggingInitialized() {
  return g_program_invocation_short_name != NULL;
}

bool is_default_thread() {
  if (g_program_invocation_short_name == NULL) {
    // InitGoogleLogging() not yet called, so unlikely to be in a different
    // thread
    return true;
  } else {
    return pthread_equal(pthread_self(), g_main_thread_id);
  }
}

int64 CycleClock_Now() {
  // TODO(hamaji): temporary impementation - it might be too slow.
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<int64>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

int64 UsecToCycles(int64 usec) {
  return usec;
}

static int32 g_main_thread_pid = getpid();
int32 GetMainThreadPid() {
  return g_main_thread_pid;
}

static string g_my_user_name;
const string& MyUserName() {
  return g_my_user_name;
}
static void MyUserNameInitializer() {
  // TODO(hamaji): Probably this is not portable.
  const char* user = getenv("USER");
  if (user != NULL) {
    g_my_user_name = user;
  } else {
    g_my_user_name = "invalid-user";
  }
}
REGISTER_MODULE_INITIALIZER(utilities, MyUserNameInitializer());

}  // namespace glog_internal_namespace_

void InitGoogleLogging(const char* argv0) {
  const char* slash = strrchr(argv0, '/');
#ifdef OS_WINDOWS
  if (!slash)  slash = strrchr(argv0, '\\');
#endif
  g_program_invocation_short_name = slash ? slash + 1 : argv0;
  g_main_thread_id = pthread_self();

#ifdef HAVE_STACKTRACE
  InstallFailureFunction(&DumpStackTraceAndExit);
#endif
}

_END_GOOGLE_NAMESPACE_
