# Google Logging Library

Google Logging (glog) is a C++14 library that implements application-level
logging. The library provides logging APIs based on C++-style streams and
various helper macros.

# How to Use

You can log a message by simply streaming things to `LOG`(<a particular
[severity level](logging.md#severity-levels)\>), e.g.,

``` cpp title="main.cpp"
#include <glog/logging.h>

int main(int argc, char* argv[]) {
    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);

    // ...
    LOG(INFO) << "Found " << num_cookies << " cookies";
}
```

The library can be installed using various [package managers](packages.md) or
compiled [from source](build.md). For a detailed overview of glog features and
their usage, please refer to the [user guide](logging.md).

!!! warning
    The above example requires further [Bazel](build.md#bazel) or
    [CMake](usage.md) setup for use in own projects.
