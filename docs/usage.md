# Using glog in a CMake Project

Assuming that glog was previously [built using CMake](build.md#cmake) or
installed using a package manager, you can use the CMake command `#!cmake
find_package` to build against glog in your CMake project as follows:

``` cmake title="CMakeLists.txt"
cmake_minimum_required (VERSION 3.16)
project (myproj VERSION 1.0)

find_package (glog 0.8.0 REQUIRED)

add_executable (myapp main.cpp)
target_link_libraries (myapp glog::glog)
```

Compile definitions and options will be added automatically to your target as
needed.

Alternatively, glog can be incorporated into using the CMake command `#!cmake
add_subdirectory` to include glog directly from a subdirectory of your project
by replacing the `#!cmake find_package` call from the previous snippet by
`add_subdirectory`. The `#!cmake glog::glog` target is in this case an `#!cmake
ALIAS` library target for the `glog` library target.
