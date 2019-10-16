# Sample toolchain file for building with gcc compiler
#
# Typical usage:
#    *) cmake -H. -B_build -DCMAKE_TOOLCHAIN_FILE="${PWD}/toolchains/gcc.cmake"

# this one is important
SET(CMAKE_SYSTEM_NAME Generic)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# set compiler
set(CMAKE_C_COMPILER armr5-none-eabi-gcc)
set(CMAKE_CXX_COMPILER armr5-none-eabi-g++)
set(CMAKE_C_FLAGS "-mfloat-abi=soft -mcpu=cortex-r5 -specs=nosys.specs")
set(CMAKE_CXX_FLAGS "-mfloat-abi=soft -mcpu=cortex-r5 -specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "-specs=nosys.specs")

# set c++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
