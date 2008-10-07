// Copyright 2006 Google Inc. All Rights Reserved.
// Author: Satoru Takabayashi
//
// Unit tests for functions in demangle.c.

#include <iostream>
#include <fstream>
#include <string>
#include "glog/logging.h"
#include "demangle.h"
#include "googletest.h"
#include "config.h"

DEFINE_bool(demangle_filter, false, "Run demangle_unittest in filter mode");

using namespace std;
using namespace GOOGLE_NAMESPACE;

// A wrapper function for Demangle() to make the unit test simple.
static const char *DemangleIt(const char * const mangled) {
  static char demangled[4096];
  if (Demangle(mangled, demangled, sizeof(demangled))) {
    return demangled;
  } else {
    return mangled;
  }
}

// Test corner cases of bounary conditions.
TEST(Demangle, CornerCases) {
  char tmp[10];
  EXPECT_TRUE(Demangle("_Z6foobarv", tmp, sizeof(tmp)));
  // sizeof("foobar()") == 9
  EXPECT_STREQ("foobar()", tmp);
  EXPECT_TRUE(Demangle("_Z6foobarv", tmp, 9));
  EXPECT_STREQ("foobar()", tmp);
  EXPECT_FALSE(Demangle("_Z6foobarv", tmp, 8));  // Not enough.
  EXPECT_FALSE(Demangle("_Z6foobarv", tmp, 1));
  EXPECT_FALSE(Demangle("_Z6foobarv", tmp, 0));
  EXPECT_FALSE(Demangle("_Z6foobarv", NULL, 0));  // Should not cause SEGV.
}

TEST(Demangle, FromFile) {
  string test_file = FLAGS_test_srcdir + "/src/demangle_unittest.txt";
  ifstream f(test_file.c_str());  // The file should exist.
  EXPECT_FALSE(f.fail());

  string line;
  while (getline(f, line)) {
    // Lines start with '#' are considered as comments.
    if (line.empty() || line[0] == '#') {
      continue;
    }
    // Each line should contain a mangled name and a demangled name
    // separated by '\t'.  Example: "_Z3foo\tfoo"
    string::size_type tab_pos = line.find('\t');
    EXPECT_NE(string::npos, tab_pos);
    string mangled = line.substr(0, tab_pos);
    string demangled = line.substr(tab_pos + 1);
    EXPECT_EQ(demangled, DemangleIt(mangled.c_str()));
  }
}

int main(int argc, char **argv) {
#ifdef HAVE_LIB_GFLAGS
  ParseCommandLineFlags(&argc, &argv, true);
#endif
  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);
  if (FLAGS_demangle_filter) {
    // Read from cin and write to cout.
    string line;
    while (getline(cin, line, '\n')) {
      cout << DemangleIt(line.c_str()) << endl;
    }
    return 0;
  } else if (argc > 1) {
    cout << DemangleIt(argv[1]) << endl;
    return 0;
  } else {
    return RUN_ALL_TESTS();
  }
}
