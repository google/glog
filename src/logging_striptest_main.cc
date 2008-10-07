// Copyright 2007 Google Inc. All Rights Reserved.
// Author: Sergey Ioffe

// The common part of the striplog tests.

#include <stdio.h>
#include <string>
#include <iosfwd>
#include "glog/logging.h"
#include "base/commandlineflags.h"
#include "config.h"

DECLARE_bool(logtostderr);

using std::string;
using namespace GOOGLE_NAMESPACE;

int CheckNoReturn(bool b) {
  string s;
  if (b) {
    LOG(FATAL) << "Fatal";
  } else {
    return 0;
  }
}

struct A { };
std::ostream &operator<<(std::ostream &str, const A&) {return str;}

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);
  LOG(INFO) << "TESTMESSAGE INFO";
  LOG(WARNING) << 2 << "something" << "TESTMESSAGE WARNING"
               << 1 << 'c' << A() << std::endl;
  LOG(ERROR) << "TESTMESSAGE ERROR";
  bool flag = true;
  (flag ? LOG(INFO) : LOG(ERROR)) << "TESTMESSAGE COND";
  LOG(FATAL) << "TESTMESSAGE FATAL";
}
