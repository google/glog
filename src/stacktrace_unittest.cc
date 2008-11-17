#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "utilities.h"
#include "base/commandlineflags.h"
#include "glog/logging.h"
#include "stacktrace.h"

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
#endif

using namespace GOOGLE_NAMESPACE;

#ifdef HAVE_STACKTRACE

// Obtain a backtrace, verify that the expected callers are present in the
// backtrace, and maybe print the backtrace to stdout.

//-----------------------------------------------------------------------//
void CheckStackTraceLeaf();
void CheckStackTrace4(int i);
void CheckStackTrace3(int i);
void CheckStackTrace2(int i);
void CheckStackTrace1(int i);
void CheckStackTrace(int i);
//-----------------------------------------------------------------------//

// The sequence of functions whose return addresses we expect to see in the
// backtrace.
const int BACKTRACE_STEPS = 6;
void * expected_stack[BACKTRACE_STEPS] = {
  (void *) &CheckStackTraceLeaf,
  (void *) &CheckStackTrace4,
  (void *) &CheckStackTrace3,
  (void *) &CheckStackTrace2,
  (void *) &CheckStackTrace1,
  (void *) &CheckStackTrace,
};

// Depending on the architecture/compiler/libraries, (not sure which)
// the current function may or may not appear in the backtrace.
// For gcc-2:
//
// stack[0] is ret addr within CheckStackTrace4
// stack[1] is ret addr within CheckStackTrace3
// stack[2] is ret addr within CheckStackTrace2
// stack[3] is ret addr within CheckStackTrace1
// stack[4] is ret addr within CheckStackTrace
//
// For gcc3-k8:
//
// stack[0] is ret addr within CheckStackTraceLeaf
// stack[1] is ret addr within CheckStackTrace4
// ...
// stack[5] is ret addr within CheckStackTrace

//-----------------------------------------------------------------------//

const int kMaxFnLen = 0x40; // assume relevant functions are only this long

void CheckRetAddrIsInFunction( void * ret_addr, void * function_start_addr)
{
  CHECK_GE(ret_addr, function_start_addr);
  CHECK_LE(ret_addr, (void *) ((char *) function_start_addr + kMaxFnLen));
}

//-----------------------------------------------------------------------//

void CheckStackTraceLeaf(void) {
  const int STACK_LEN = 10;
  void *stack[STACK_LEN];
  int size;

  size = GetStackTrace(stack, STACK_LEN, 0);
  printf("Obtained %d stack frames.\n", size);
  CHECK_LE(size, STACK_LEN);

  if (1) {
#ifdef HAVE_EXECINFO_H
    char **strings = backtrace_symbols(stack, size);
    printf("Obtained %d stack frames.\n", size);
    for (int i = 0; i < size; i++)
      printf("%s %p\n", strings[i], stack[i]);
    printf("CheckStackTrace() addr: %p\n", &CheckStackTrace);
    free(strings);
#endif
  }
  for (int i = 0; i < BACKTRACE_STEPS; i++) {
    printf("Backtrace %d: expected: %p..%p  actual: %p ... ",
           i, expected_stack[i],
           reinterpret_cast<char*>(expected_stack[i]) + kMaxFnLen, stack[i]);
    CheckRetAddrIsInFunction(stack[i], expected_stack[i]);
    printf("OK\n");
  }

  // Check if the second stacktrace returns the same size.
  CHECK_EQ(size, GetStackTrace(stack, STACK_LEN, 0));
}

//-----------------------------------------------------------------------//

/* Dummy functions to make the backtrace more interesting. */
void CheckStackTrace4(int i) { for (int j = i; j >= 0; j--) CheckStackTraceLeaf(); }
void CheckStackTrace3(int i) { for (int j = i; j >= 0; j--) CheckStackTrace4(j); }
void CheckStackTrace2(int i) { for (int j = i; j >= 0; j--) CheckStackTrace3(j); }
void CheckStackTrace1(int i) { for (int j = i; j >= 0; j--) CheckStackTrace2(j); }
void CheckStackTrace(int i)  { for (int j = i; j >= 0; j--) CheckStackTrace1(j); }

//-----------------------------------------------------------------------//

int main(int argc, char ** argv) {
  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);
  
  CheckStackTrace(0);
  
  printf("PASS\n");
  return 0;
}

#else
int main() {
  printf("PASS (no stacktrace support)\n");
  return 0;
}
#endif  // HAVE_STACKTRACE
