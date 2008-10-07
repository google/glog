// Copyright 2007 Google Inc. All Rights Reserved.
// Author: Raksit Ashok
//
// Unit test for the GetStackFrames function in stacktrace.cc.

#include <stdio.h>
#include "glog/logging.h"
#include "base/commandlineflags.h"
#include "stacktrace.h"
#include "config.h"
#include "utilities.h"

using std::min;
using namespace GOOGLE_NAMESPACE;

#ifdef HAVE_STACKTRACE

// Obtain a backtrace of the stack frame sizes, verify that they look sane.

//-----------------------------------------------------------------------//
int CheckFrameSizesLeaf(int32* i);  // 8KB frame size.
int CheckFrameSizes2(int32* i);     // 4KB
int CheckFrameSizes1(int32* i);     // 2KB
int CheckFrameSizes(int32* i);      // 1KB
//-----------------------------------------------------------------------//

// The expected frame-sizes in the backtrace.
const int BACKTRACE_STEPS = 4;
int expected_frame_sizes[BACKTRACE_STEPS] = {
  1 << 13,
  1 << 12,
  1 << 11,
  1 << 10,
};

//-----------------------------------------------------------------------//

void CheckFrameSizeIsOk(int actual_frame_size, int ref_frame_size) {
  // Assume upto 512 bytes of miscellaneous stuff in CheckFrameSizes* frames.
  const int misc_frame_size = 512;
  CHECK_GE(actual_frame_size, ref_frame_size);
  CHECK_LE(actual_frame_size, ref_frame_size + misc_frame_size);
}

//-----------------------------------------------------------------------//

int ATTRIBUTE_NOINLINE CheckFrameSizesLeaf(int32 *i) {
  const int DEPTH = 10;
  void* pcs[DEPTH];
  int frame_sizes[DEPTH];
  int size;
  int32 j[2048];  // 8KB.
  for (int k = 0; k < 2048; k++) j[k] = k + i[k % 1024];

  for (int depth = 0; depth < DEPTH; depth++) {
    size = GetStackFrames(pcs, frame_sizes, depth, 0);
    printf("--- GetStackFrames(..., %d, 0) = %d\n", depth, size);
    CHECK_LE(size, depth);
    CHECK_GE(size, min(depth, BACKTRACE_STEPS));

    for (int k = 0; k < size; k++) {
      if (k < BACKTRACE_STEPS)
    // GetStackFrames doesn't work correctly if we are using glibc's backtrace.
#ifndef HAVE_EXECINFO_H
        CheckFrameSizeIsOk(frame_sizes[k], expected_frame_sizes[k]);
#endif
      printf("frame_sizes[%d] = %d\n", k, frame_sizes[k]);
    }
  }

  int sum = 0;
  for (int k = 0; k < 2048; k++) sum += j[k];
  return sum;
}

//-----------------------------------------------------------------------//

/* Dummy functions to make the frame-size backtrace more interesting. */
int ATTRIBUTE_NOINLINE CheckFrameSizes2(int32* i) {
  int32 j[1024];  // 4KB.
  for (int k = 0; k < 1024; k++) j[k] = k + i[k % 512];
  return CheckFrameSizesLeaf(j) + j[512];
}

int ATTRIBUTE_NOINLINE CheckFrameSizes1(int32* i) {
  int32 j[512];  // 2KB.
  for (int k = 0; k < 512; k++) j[k] = k + i[k % 256];
  return CheckFrameSizes2(j) + j[256];
}

int ATTRIBUTE_NOINLINE CheckFrameSizes(int32* i)  {
  int32 j[256];  // 1KB.
  for (int k = 0; k < 256; k++) j[k] = k + i[k];
  return CheckFrameSizes1(j) + j[128];
}

//-----------------------------------------------------------------------//

int main(int argc, char ** argv) {
  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);

  int32 i[256];  // 1KB.
  for (int j = 0; j < 256; j++) i[j] = j;
  
  int ret = CheckFrameSizes(i);

  printf("CheckFrameSizes returned: %d\n", ret);

  printf("PASS\n");
  return 0;
}

#else
int main() {
  printf("PASS (no stacktrace support)\n");
  return 0;
}
#endif  // HAVE_STACKTRACE
