# Building from Source

## Bazel

To use glog within a project which uses the [Bazel](https://bazel.build/) build
tool, add the following lines to your `WORKSPACE` file:

``` bazel title="WORKSPACE"
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "gflags",
    sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    strip_prefix = "gflags-2.2.2",
    urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
)

http_archive(
    name = "com_github_google_glog",
    sha256 = "c17d85c03ad9630006ef32c7be7c65656aba2e7e2fbfc82226b7e680c771fc88",
    strip_prefix = "glog-0.7.1",
    urls = ["https://github.com/google/glog/archive/v0.7.1.zip"],
)
```

You can then add `@com_github_google_glog//:glog` to
the deps section of a `cc_binary` or
`cc_library` rule, and `#!cpp #include <glog/logging.h>` to
include it in your source code.

!!! example "Using glog in a Bazel project"
    ``` bazel
    cc_binary(
        name = "main",
        srcs = ["main.cc"],
        deps = ["@com_github_google_glog//:glog"],
    )
    ```

## CMake

glog can be compiled using [CMake](http://www.cmake.org) on a wide range of
platforms. The typical workflow for building glog on a Unix-like system with GNU
Make as build tool is as follows:

1.  Clone the repository and change into source directory.
  ``` bash
  git clone https://github.com/google/glog.git
  cd glog
  ```
2.  Run CMake to configure the build tree.
  ``` bash
  cmake -S . -B build -G "Unix Makefiles"
  ```
  CMake provides different generators, and by default will pick the most
  relevant one to your environment. If you need a specific version of Visual
  Studio, use `#!bash cmake . -G <generator-name>`, and see `#!bash cmake
  --help` for the available generators. Also see `-T <toolset-name>`, which can
  be used to request the native x64 toolchain with `-T host=x64`.
3.  Afterwards, generated files can be used to compile the project.
  ``` bash
  cmake --build build
  ```
4.  Test the build software (optional).
  ``` bash
  cmake --build build --target test
  ```
5.  Install the built files (optional).
  ``` bash
  cmake --build build --target install
  ```

Once successfully built, glog can be [integrated into own projects](usage.md).
