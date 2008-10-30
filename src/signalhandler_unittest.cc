// Copyright 2008 Google Inc. All Rights Reserved.
// Author: satorux@google.com (Satoru Takabayashi)
//
// This is a helper binary for testing signalhandler.cc.  The actual test
// is done in signalhandler_unittest.sh.

#include "utilities.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "glog/logging.h"

using namespace GOOGLE_NAMESPACE;

void* DieInThread(void*) {
  fprintf(stderr, "0x%lx is dying\n", pthread_self());
  // Use volatile to prevent from these to be optimized away.
  volatile int a = 0;
  volatile int b = 1 / a;
}

void WriteToStdout(const char* data, int size) {
  write(STDOUT_FILENO, data, size);
}

int main(int argc, char **argv) {
#ifdef HAVE_STACKTRACE
  InitGoogleLogging(argv[0]);
  InstallFailureSignalHandler();
  const std::string command = argc > 1 ? argv[1] : "none";
  if (command == "segv") {
    // We assume 0xDEAD is not writable.
    int *a = (int*)0xDEAD;
    *a = 0;
  } else if (command == "loop") {
    fprintf(stderr, "looping\n");
    while (true);
  } else if (command == "die_in_thread") {
    pthread_t thread;
    pthread_create(&thread, NULL, &DieInThread, NULL);
    pthread_join(thread, NULL);
  } else if (command == "dump_to_stdout") {
    InstallFailureWriter(WriteToStdout);
    abort();
  } else {
    // Tell the shell script
    puts("OK");
  }
#endif
  return 0;
}
