// Copyright 2008 Google Inc. All Rights Reserved.
// Author: hamaji@google.com (Shinichiro Hamaji)

#include "utilities.h"
#include "googletest.h"
#include "glog/logging.h"

using namespace GOOGLE_NAMESPACE;

TEST(sync_val_compare_and_swap, utilities) {
  bool now_entering = false;
  EXPECT_FALSE(sync_val_compare_and_swap(&now_entering, false, true));
  EXPECT_TRUE(sync_val_compare_and_swap(&now_entering, false, true));
  EXPECT_TRUE(sync_val_compare_and_swap(&now_entering, false, true));
}

int main(int argc, char **argv) {
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);

  CHECK_EQ(RUN_ALL_TESTS(), 0);
}
