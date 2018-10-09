# - Try to find libunwind
# Once done this will define
#
#  Libunwind_FOUND - system has libunwind
#  unwind - cmake target for libunwind

find_library (UNWIND_LIBRARY NAMES unwind DOC "unwind library")
include (CheckIncludeFile)
check_include_file (libunwind.h HAVE_LIBUNWIND_H)
check_include_file (unwind.h HAVE_UNWIND_H)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(LIBUNWIND_ARCH "arm")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
    set(LIBUNWIND_ARCH "aarch64")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR
        CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64" OR
        CMAKE_SYSTEM_PROCESSOR STREQUAL "corei7-64")
    set(LIBUNWIND_ARCH "x86_64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
    set(LIBUNWIND_ARCH "x86")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^ppc64")
    set(LIBUNWIND_ARCH "ppc64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^ppc")
    set(LIBUNWIND_ARCH "ppc32")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
    set(LIBUNWIND_ARCH "mips")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^hppa")
    set(LIBUNWIND_ARCH "hppa")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^ia64")
    set(LIBUNWIND_ARCH "ia64")
endif()

find_library (UNWIND_LIBRARY_PLATFORM NAMES "unwind-${LIBUNWIND_ARCH}" DOC "unwind library platform")
if (UNWIND_LIBRARY_PLATFORM)
    set(HAVE_LIB_UNWIND "1")
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set Libunwind_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Libunwind DEFAULT_MSG
    UNWIND_LIBRARY HAVE_LIBUNWIND_H HAVE_UNWIND_H HAVE_LIB_UNWIND)

mark_as_advanced (UNWIND_LIBRARY UNWIND_LIBRARY_PLATFORM)

if (Libunwind_FOUND)
    add_library(unwind INTERFACE IMPORTED)
    set_target_properties(unwind PROPERTIES
        INTERFACE_LINK_LIBRARIES "${UNWIND_LIBRARY};${UNWIND_LIBRARY_PLATFORM}"
    )
else()
    message("Can't find libunwind library")
endif()

