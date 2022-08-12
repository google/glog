#include <glog/logging.h>

void bar(int x) { CHECK_EQ(x, 1); }

void foo(int x) { bar(x + 3); }

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  foo(1);
}
