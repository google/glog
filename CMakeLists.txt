cmake_minimum_required (VERSION 3.21)
project (glog
  VERSION 0.7.0
  DESCRIPTION "C++ implementation of the Google logging module"
  HOMEPAGE_URL https://github.com/google/glog
  LANGUAGES CXX
)

set (CPACK_PACKAGE_NAME glog)
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Google logging library")
set (CPACK_PACKAGE_VERSION_MAJOR ${glog_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${glog_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${glog_VERSION_PATCH})
set (CPACK_PACKAGE_VERSION ${glog_VERSION})

list (APPEND CMAKE_MODULE_PATH ${glog_SOURCE_DIR}/cmake)

include (CheckCXXSourceCompiles)
include (CheckCXXSourceRuns)
include (CheckCXXSymbolExists)
include (CheckFunctionExists)
include (CheckIncludeFileCXX)
include (CheckStructHasMember)
include (CheckTypeSize)
include (CMakeDependentOption)
include (CMakePackageConfigHelpers)
include (CMakePushCheckState)
include (CPack)
include (CTest)
include (DetermineGflagsNamespace)
include (GenerateExportHeader)
include (GetCacheVariables)
include (GNUInstallDirs)

option (BUILD_SHARED_LIBS "Build shared libraries" ON)
option (PRINT_UNSYMBOLIZED_STACK_TRACES
  "Print file offsets in traces instead of symbolizing" OFF)
option (WITH_GFLAGS "Use gflags" ON)
option (WITH_GTEST "Use Google Test" ON)
option (WITH_PKGCONFIG "Enable pkg-config support" ON)
option (WITH_SYMBOLIZE "Enable symbolize module" ON)
option (WITH_THREADS "Enable multithreading support" ON)

set (WITH_UNWIND libunwind CACHE STRING "unwind driver")
set_property (CACHE WITH_UNWIND PROPERTY STRINGS none unwind libunwind)

cmake_dependent_option (WITH_GMOCK "Use Google Mock" ON WITH_GTEST OFF)
cmake_dependent_option (WITH_TLS "Enable Thread Local Storage (TLS) support" ON WITH_THREADS OFF)

set (WITH_FUZZING none CACHE STRING "Fuzzing engine")
set_property (CACHE WITH_FUZZING PROPERTY STRINGS none libfuzzer ossfuzz)

if (WITH_UNWIND STREQUAL none)
  set (CMAKE_DISABLE_FIND_PACKAGE_Unwind ON)
endif (WITH_UNWIND STREQUAL none)

if (NOT WITH_GTEST)
  set (CMAKE_DISABLE_FIND_PACKAGE_GTest ON)
endif (NOT WITH_GTEST)

if (NOT WITH_THREADS)
  set (CMAKE_DISABLE_FIND_PACKAGE_Threads ON)
endif (NOT WITH_THREADS)

set (CMAKE_C_VISIBILITY_PRESET hidden)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set (CMAKE_DEBUG_POSTFIX d)
set (CMAKE_THREAD_PREFER_PTHREAD 1)

find_package (GTest NO_MODULE)

if (GTest_FOUND)
  set (HAVE_LIB_GTEST 1)
endif (GTest_FOUND)

if (WITH_GMOCK AND TARGET GTest::gmock)
  set (HAVE_LIB_GMOCK 1)
endif (WITH_GMOCK AND TARGET GTest::gmock)

if (WITH_GFLAGS)
  find_package (gflags 2.2.2)

  if (gflags_FOUND)
    set (HAVE_LIB_GFLAGS 1)
    determine_gflags_namespace (gflags_NAMESPACE)
  endif (gflags_FOUND)
endif (WITH_GFLAGS)

find_package (Threads)
find_package (Unwind)

if (Unwind_FOUND)
  cmake_push_check_state (RESET)
  set (CMAKE_REQUIRED_LIBRARIES unwind::unwind)

  # Check whether linking actually succeeds. ARM toolchains of LLVM unwind
  # implementation do not necessarily provide the _Unwind_Backtrace function
  # which causes the previous check to succeed but the linking to fail.
  check_cxx_symbol_exists (_Unwind_Backtrace unwind.h HAVE__UNWIND_BACKTRACE)
  check_cxx_symbol_exists (_Unwind_GetIP unwind.h HAVE__UNWIND_GETIP)

  check_cxx_symbol_exists (unw_get_reg libunwind.h HAVE_UNW_GET_REG)
  check_cxx_symbol_exists (unw_getcontext libunwind.h HAVE_UNW_GETCONTEXT)
  check_cxx_symbol_exists (unw_init_local libunwind.h HAVE_UNW_INIT_LOCAL)
  check_cxx_symbol_exists (unw_step libunwind.h HAVE_UNW_STEP)

  if (HAVE__UNWIND_BACKTRACE AND HAVE__UNWIND_GETIP)
    set (_HAVE_UNWIND 1)
  endif (HAVE__UNWIND_BACKTRACE AND HAVE__UNWIND_GETIP)

  if (HAVE_UNW_GET_REG AND HAVE_UNW_GETCONTEXT AND HAVE_UNW_INIT_LOCAL AND HAVE_UNW_STEP)
    set (_HAVE_LIBUNWIND 1)
  endif (HAVE_UNW_GET_REG AND HAVE_UNW_GETCONTEXT AND HAVE_UNW_INIT_LOCAL AND HAVE_UNW_STEP)

  if (WITH_UNWIND STREQUAL unwind)
    if (_HAVE_UNWIND)
      set (HAVE_UNWIND 1)
    endif (_HAVE_UNWIND)
  elseif (WITH_UNWIND STREQUAL libunwind)
    if (_HAVE_LIBUNWIND)
      set (HAVE_LIBUNWIND 1)
    endif (_HAVE_LIBUNWIND)
  endif (WITH_UNWIND STREQUAL unwind)

  unset (_HAVE_LIBUNWIND)
  unset (_HAVE_UNWIND)

  cmake_pop_check_state ()
endif (Unwind_FOUND)

check_include_file_cxx (dlfcn.h HAVE_DLFCN_H)
check_include_file_cxx (glob.h HAVE_GLOB_H)
check_include_file_cxx (pwd.h HAVE_PWD_H)
check_include_file_cxx (sys/syscall.h HAVE_SYS_SYSCALL_H)
check_include_file_cxx (sys/time.h HAVE_SYS_TIME_H)
check_include_file_cxx (sys/types.h HAVE_SYS_TYPES_H)
check_include_file_cxx (sys/utsname.h HAVE_SYS_UTSNAME_H)
check_include_file_cxx (sys/wait.h HAVE_SYS_WAIT_H)
check_include_file_cxx (syscall.h HAVE_SYSCALL_H)
check_include_file_cxx (syslog.h HAVE_SYSLOG_H)
check_include_file_cxx (ucontext.h HAVE_UCONTEXT_H)
check_include_file_cxx (unistd.h HAVE_UNISTD_H)

check_type_size (mode_t HAVE_MODE_T LANGUAGE CXX)
check_type_size (ssize_t HAVE_SSIZE_T LANGUAGE CXX)

check_function_exists (dladdr HAVE_DLADDR)
check_function_exists (fcntl HAVE_FCNTL)
check_function_exists (pread HAVE_PREAD)
check_function_exists (pwrite HAVE_PWRITE)
check_function_exists (sigaction HAVE_SIGACTION)
check_function_exists (sigaltstack HAVE_SIGALTSTACK)

check_cxx_symbol_exists (backtrace execinfo.h HAVE_EXECINFO_BACKTRACE)
check_cxx_symbol_exists (backtrace_symbols execinfo.h
  HAVE_EXECINFO_BACKTRACE_SYMBOLS)
check_cxx_symbol_exists (_chsize_s io.h HAVE__CHSIZE_S)

cmake_push_check_state (RESET)

set (CMAKE_REQUIRED_DEFINITIONS -D_XOPEN_SOURCE=500)

if (Threads_FOUND)
  set (CMAKE_REQUIRED_LIBRARIES Threads::Threads)
endif (Threads_FOUND)

check_cxx_symbol_exists (pthread_rwlock_destroy pthread.h HAVE_RWLOCK_DESTROY)
check_cxx_symbol_exists (pthread_rwlock_init pthread.h HAVE_RWLOCK_INIT)
check_cxx_symbol_exists (pthread_rwlock_rdlock pthread.h HAVE_RWLOCK_RDLOCK)
check_cxx_symbol_exists (pthread_rwlock_unlock pthread.h HAVE_RWLOCK_UNLOCK)
check_cxx_symbol_exists (pthread_rwlock_wrlock pthread.h HAVE_RWLOCK_WRLOCK)
check_cxx_symbol_exists (pthread_threadid_np pthread.h HAVE_PTHREAD_THREADID_NP)

cmake_pop_check_state ()

if (HAVE_RWLOCK_INIT AND HAVE_RWLOCK_RDLOCK AND HAVE_RWLOCK_WRLOCK AND
    HAVE_RWLOCK_UNLOCK AND HAVE_RWLOCK_DESTROY)
  set (HAVE_RWLOCK TRUE)
endif (HAVE_RWLOCK_INIT AND HAVE_RWLOCK_RDLOCK AND HAVE_RWLOCK_WRLOCK AND
    HAVE_RWLOCK_UNLOCK AND HAVE_RWLOCK_DESTROY)

cmake_push_check_state (RESET)
set (CMAKE_REQUIRED_LIBRARIES dbghelp)
check_cxx_symbol_exists (UnDecorateSymbolName "windows.h;dbghelp.h" HAVE_DBGHELP)
cmake_pop_check_state ()

check_cxx_source_compiles ("
int main(void)
{
  int a; if (__sync_val_compare_and_swap(&a, 0, 1)) return 1; return 0;
}
" HAVE___SYNC_VAL_COMPARE_AND_SWAP)

check_cxx_source_compiles ("
__declspec(selectany) int a;
int main(void) { return 0; }
" HAVE___DECLSPEC)

if (WITH_TLS)
  set (GLOG_THREAD_LOCAL_STORAGE 1)
endif (WITH_TLS)

set (_PC_FIELDS
  "uc_mcontext.gregs[REG_PC]"          # Solaris x86 (32 + 64 bit)
  "uc_mcontext.gregs[REG_EIP]"         # Linux (i386)
  "uc_mcontext.gregs[REG_RIP]"         # Linux (x86_64)
  "uc_mcontext.sc_ip"                  # Linux (ia64)
  "uc_mcontext.pc"                     # Linux (mips)
  "uc_mcontext.uc_regs->gregs[PT_NIP]" # Linux (ppc)
  "uc_mcontext.gregs[R15]"             # Linux (arm old [untested])
  "uc_mcontext.arm_pc"                 # Linux (arm arch 5)
  "uc_mcontext.gp_regs[PT_NIP]"        # Suse SLES 11 (ppc64)
  "uc_mcontext.mc_eip"                 # FreeBSD (i386)
  "uc_mcontext.mc_rip"                 # FreeBSD (x86_64 [untested])
  "uc_mcontext.__gregs[_REG_EIP]"      # NetBSD (i386)
  "uc_mcontext.__gregs[_REG_RIP]"      # NetBSD (x86_64)
  "uc_mcontext->ss.eip"                # OS X (i386, <=10.4)
  "uc_mcontext->__ss.__eip"            # OS X (i386, >=10.5)
  "uc_mcontext->ss.rip"                # OS X (x86_64)
  "uc_mcontext->__ss.__rip"            # OS X (>=10.5 [untested])
  "uc_mcontext->ss.srr0"               # OS X (ppc, ppc64 [untested])
  "uc_mcontext->__ss.__srr0"           # OS X (>=10.5 [untested])
)

if (HAVE_UCONTEXT_H AND NOT DEFINED PC_FROM_UCONTEXT)
  cmake_push_check_state (RESET)

  set (CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  set (_PC_HEADERS ucontext.h signal.h)

  foreach (_PC_FIELD IN LISTS _PC_FIELDS)
    foreach (_PC_HEADER IN LISTS _PC_HEADERS)
      # Replace non-alphanumeric characters by underscores since the name will be
      # used as preprocessor definition.
      string (REGEX REPLACE "[^a-zA-Z0-9]" "_" HAVE_UCONTEXT_FIELD_NAME
        "HAVE_PC_FROM_UCONTEXT_${_PC_FIELD}")
      # Strip trailing underscores for readability
      string (REGEX REPLACE "_+$" "" HAVE_UCONTEXT_FIELD_NAME
        "${HAVE_UCONTEXT_FIELD_NAME}")

      check_struct_has_member (ucontext_t ${_PC_FIELD} ${_PC_HEADER}
        ${HAVE_UCONTEXT_FIELD_NAME} LANGUAGE CXX)

      if (${HAVE_UCONTEXT_FIELD_NAME})
        set (PC_FROM_UCONTEXT ${_PC_FIELD} CACHE STRING
          "<${_PC_HEADER}> ucontext_t PC member")
        mark_as_advanced (PC_FROM_UCONTEXT)
        break ()
      endif (${HAVE_UCONTEXT_FIELD_NAME})
    endforeach (_PC_HEADER)

    if (${HAVE_UCONTEXT_FIELD_NAME})
      break ()
    endif (${HAVE_UCONTEXT_FIELD_NAME})
  endforeach (_PC_FIELD)

  cmake_pop_check_state ()
endif (HAVE_UCONTEXT_H AND NOT DEFINED PC_FROM_UCONTEXT)

if (HAVE_EXECINFO_BACKTRACE AND HAVE_EXECINFO_BACKTRACE_SYMBOLS)
  set (HAVE_STACKTRACE 1)
endif (HAVE_EXECINFO_BACKTRACE AND HAVE_EXECINFO_BACKTRACE_SYMBOLS)

if (WITH_SYMBOLIZE)
  if (WIN32 OR CYGWIN)
    cmake_push_check_state (RESET)
    set (CMAKE_REQUIRED_LIBRARIES DbgHelp)

    check_cxx_source_runs ([=[
    #include <windows.h>
    #include <dbghelp.h>
    #include <cstdlib>

    void foobar() { }

    int main()
    {
        HANDLE process = GetCurrentProcess();

        if (!SymInitialize(process, NULL, TRUE))
            return EXIT_FAILURE;

        char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(buf);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        void* const pc = reinterpret_cast<void*>(&foobar);
        BOOL ret = SymFromAddr(process, reinterpret_cast<DWORD64>(pc), 0, symbol);

        return ret ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    ]=] HAVE_SYMBOLIZE)

    cmake_pop_check_state ()

    if (HAVE_SYMBOLIZE)
      set (HAVE_STACKTRACE 1)
    endif (HAVE_SYMBOLIZE)
  elseif (UNIX)
    cmake_push_check_state (RESET)
    check_cxx_symbol_exists (__ELF__ "" HAVE_SYMBOLIZE)
    cmake_pop_check_state ()
  elseif (APPLE AND HAVE_DLADDR)
    set (HAVE_SYMBOLIZE 1)
  endif (WIN32 OR CYGWIN)
endif (WITH_SYMBOLIZE)

# CMake manages symbolize availability. The definition is necessary only when
# building the library.
add_compile_definitions (GLOG_NO_SYMBOLIZE_DETECTION)

check_cxx_symbol_exists (gmtime_r "cstdlib;ctime" HAVE_GMTIME_R)
check_cxx_symbol_exists (localtime_r "cstdlib;ctime" HAVE_LOCALTIME_R)

set (SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})

if (WITH_THREADS AND Threads_FOUND)
  if (CMAKE_USE_PTHREADS_INIT)
    set (HAVE_PTHREAD 1)
  endif (CMAKE_USE_PTHREADS_INIT)
else (WITH_THREADS AND Threads_FOUND)
  set (NO_THREADS 1)
endif (WITH_THREADS AND Threads_FOUND)

# fopen/open on Cygwin can not handle unix-type paths like /home/....
# therefore we translate TEST_SRC_DIR to windows-path.
if (CYGWIN)
  execute_process (COMMAND cygpath.exe -m ${glog_SOURCE_DIR}
                   OUTPUT_STRIP_TRAILING_WHITESPACE
                   OUTPUT_VARIABLE TEST_SRC_DIR)
  set (TEST_SRC_DIR \"${TEST_SRC_DIR}\")
else (CYGWIN)
  set (TEST_SRC_DIR \"${glog_SOURCE_DIR}\")
endif (CYGWIN)

configure_file (src/config.h.cmake.in config.h)

set (_glog_CMake_BINDIR ${CMAKE_INSTALL_BINDIR})
set (_glog_CMake_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set (_glog_CMake_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set (_glog_CMake_INSTALLDIR ${_glog_CMake_LIBDIR}/cmake/glog)

set (_glog_CMake_DIR glog/cmake)
set (_glog_CMake_DATADIR ${CMAKE_INSTALL_DATAROOTDIR}/${_glog_CMake_DIR})
set (_glog_BINARY_CMake_DATADIR
  ${glog_BINARY_DIR}/${_glog_CMake_DATADIR})

# Add additional CMake find modules here.
set (_glog_CMake_MODULES)

if (Unwind_FOUND)
  # Copy the module only if libunwind is actually used.
  list (APPEND _glog_CMake_MODULES ${glog_SOURCE_DIR}/cmake/FindUnwind.cmake)
endif (Unwind_FOUND)

# Generate file name for each module in the binary directory
foreach (_file ${_glog_CMake_MODULES})
  get_filename_component (_module "${_file}" NAME)

  list (APPEND _glog_BINARY_CMake_MODULES
    ${_glog_BINARY_CMake_DATADIR}/${_module})
endforeach (_file)

if (_glog_CMake_MODULES)
  # Copy modules to binary directory during the build
  add_custom_command (OUTPUT ${_glog_BINARY_CMake_MODULES}
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${_glog_BINARY_CMake_DATADIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${_glog_CMake_MODULES}
    ${_glog_BINARY_CMake_DATADIR}
    DEPENDS ${_glog_CMake_MODULES}
    COMMENT "Copying find modules..."
  )
endif (_glog_CMake_MODULES)

set (GLOG_PUBLIC_H
  ${glog_BINARY_DIR}/glog/export.h
  src/glog/log_severity.h
  src/glog/logging.h
  src/glog/platform.h
  src/glog/raw_logging.h
  src/glog/stl_logging.h
  src/glog/vlog_is_on.h
)

set (GLOG_SRCS
  ${GLOG_PUBLIC_H}
  src/base/commandlineflags.h
  src/base/googleinit.h
  src/base/mutex.h
  src/demangle.cc
  src/demangle.h
  src/logging.cc
  src/raw_logging.cc
  src/signalhandler.cc
  src/symbolize.cc
  src/symbolize.h
  src/utilities.cc
  src/utilities.h
  src/vlog_is_on.cc
)

if (CYGWIN OR WIN32)
  list (APPEND GLOG_SRCS
    src/windows/port.cc
    src/windows/port.h
  )
endif (CYGWIN OR WIN32)

add_library (glog_internal OBJECT
  ${_glog_BINARY_CMake_MODULES}
  ${GLOG_SRCS}
)
target_compile_features (glog_internal PUBLIC $<TARGET_PROPERTY:glog,COMPILE_FEATURES>)
set_target_properties (glog_internal PROPERTIES DEFINE_SYMBOL GOOGLE_GLOG_IS_A_DLL)

# Some generators (such as Xcode) do not generate any output if the target does
# not reference at least one source file.
set (_glog_EMPTY_SOURCE ${glog_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/glog.cc)

add_custom_command (
  OUTPUT ${_glog_EMPTY_SOURCE}
  COMMAND ${CMAKE_COMMAND} -E touch ${_glog_EMPTY_SOURCE}
)

add_library (glog
  $<TARGET_OBJECTS:glog_internal>
  ${_glog_EMPTY_SOURCE}
)
target_compile_features (glog PUBLIC cxx_std_14)

add_library (glog::glog ALIAS glog)

set (glog_libraries_options_for_static_linking)

# CMake always uses the generated export header
target_compile_definitions (glog PUBLIC GLOG_USE_GLOG_EXPORT)

if (HAVE_LIB_GFLAGS)
  target_compile_definitions (glog PUBLIC GLOG_USE_GFLAGS)
endif (HAVE_LIB_GFLAGS)

if (Unwind_FOUND)
  target_link_libraries (glog PRIVATE unwind::unwind)
  set (glog_libraries_options_for_static_linking "${glog_libraries_options_for_static_linking} -lunwind")
  set (Unwind_DEPENDENCY "find_dependency (Unwind ${Unwind_VERSION})")
endif (Unwind_FOUND)

if (HAVE_DBGHELP)
  target_link_libraries (glog PRIVATE dbghelp)
  set (glog_libraries_options_for_static_linking "${glog_libraries_options_for_static_linking} -ldbghelp")
endif (HAVE_DBGHELP)

if (HAVE_PTHREAD)
  target_link_libraries (glog PRIVATE Threads::Threads)

  set (Threads_DEPENDENCY "find_dependency (Threads)")

  if (CMAKE_THREAD_LIBS_INIT)
    set (glog_libraries_options_for_static_linking "${glog_libraries_options_for_static_linking} ${CMAKE_THREAD_LIBS_INIT}")
  endif (CMAKE_THREAD_LIBS_INIT)
endif (HAVE_PTHREAD)

if (gflags_FOUND)
  # Prefer the gflags target that uses double colon convention
  if (TARGET gflags::gflags)
    target_link_libraries (glog PUBLIC gflags::gflags)
  else (TARGET gflags::gflags)
    target_link_libraries (glog PUBLIC gflags)
  endif (TARGET gflags::gflags)

  set (glog_libraries_options_for_static_linking "${glog_libraries_options_for_static_linking} -lgflags")
endif (gflags_FOUND)

if (ANDROID)
  target_link_libraries (glog PRIVATE log)
  set (glog_libraries_options_for_static_linking "${glog_libraries_options_for_static_linking} -llog")
endif (ANDROID)

set_target_properties (glog PROPERTIES VERSION ${glog_VERSION})
set_target_properties (glog PROPERTIES SOVERSION 1)

if (CYGWIN OR WIN32)
  target_compile_definitions (glog PUBLIC GLOG_NO_ABBREVIATED_SEVERITIES)
endif (CYGWIN OR WIN32)

set_target_properties (glog PROPERTIES PUBLIC_HEADER "${GLOG_PUBLIC_H}")

target_include_directories (glog BEFORE PUBLIC
  "$<BUILD_INTERFACE:${glog_BINARY_DIR}>"
  "$<BUILD_INTERFACE:${glog_SOURCE_DIR}/src>"
  "$<INSTALL_INTERFACE:${_glog_CMake_INCLUDE_DIR}>"
  PRIVATE ${glog_BINARY_DIR}
  PRIVATE ${glog_SOURCE_DIR}/src)

if (CYGWIN OR WIN32)
  target_include_directories (glog_internal PUBLIC
    "$<BUILD_INTERFACE:${glog_SOURCE_DIR}/src/windows>"
    PRIVATE ${glog_SOURCE_DIR}/src/windows)

  target_include_directories (glog PUBLIC
    "$<BUILD_INTERFACE:${glog_SOURCE_DIR}/src/windows>"
    PRIVATE ${glog_SOURCE_DIR}/src/windows)
endif (CYGWIN OR WIN32)

set_target_properties (glog PROPERTIES DEFINE_SYMBOL GOOGLE_GLOG_IS_A_DLL)

target_include_directories (glog_internal PUBLIC
  $<TARGET_PROPERTY:glog,INCLUDE_DIRECTORIES>)
target_compile_definitions (glog_internal PUBLIC
  $<TARGET_PROPERTY:glog,COMPILE_DEFINITIONS>
  PRIVATE GOOGLE_GLOG_IS_A_DLL)

generate_export_header (glog
  EXPORT_MACRO_NAME GLOG_EXPORT
  EXPORT_FILE_NAME ${glog_BINARY_DIR}/glog/export.h)

string (STRIP "${glog_libraries_options_for_static_linking}" glog_libraries_options_for_static_linking)

if (WITH_PKGCONFIG)
  set (VERSION ${glog_VERSION})
  set (prefix ${CMAKE_INSTALL_PREFIX})
  set (exec_prefix ${CMAKE_INSTALL_FULL_BINDIR})
  set (libdir ${CMAKE_INSTALL_FULL_LIBDIR})
  set (includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})

  configure_file (
    "${glog_SOURCE_DIR}/libglog.pc.in"
    "${glog_BINARY_DIR}/libglog.pc"
    @ONLY
  )

  unset (VERSION)
  unset (prefix)
  unset (exec_prefix)
  unset (libdir)
  unset (includedir)
endif (WITH_PKGCONFIG)

# Unit testing

if (NOT WITH_FUZZING STREQUAL "none")
  add_executable (fuzz_demangle
    src/fuzz_demangle.cc
  )

  if (WITH_FUZZING STREQUAL "ossfuzz")
    set (LIB_FUZZING_ENGINE $ENV{LIB_FUZZING_ENGINE})
    target_link_libraries (fuzz_demangle PRIVATE glog ${LIB_FUZZING_ENGINE})
  elseif (WITH_FUZZING STREQUAL "libfuzzer")
    target_compile_options (fuzz_demangle PRIVATE -fsanitize=fuzzer)
    target_link_libraries (fuzz_demangle PRIVATE glog)
  else (WITH_FUZZING STREQUAL "libfuzzer")
    message (FATAL_ERROR "Unsupported fuzzing engine ${WITH_FUZZING}")
  endif (WITH_FUZZING STREQUAL "ossfuzz")
endif (NOT WITH_FUZZING STREQUAL "none")

if (BUILD_TESTING)
  add_library (glog_test INTERFACE)
  target_link_libraries (glog_test INTERFACE $<TARGET_OBJECTS:glog_internal> $<TARGET_PROPERTY:glog,LINK_LIBRARIES>)
  target_compile_definitions (glog_test INTERFACE GLOG_STATIC_DEFINE $<TARGET_PROPERTY:glog,COMPILE_DEFINITIONS>)
  target_include_directories (glog_test INTERFACE $<TARGET_PROPERTY:glog,INCLUDE_DIRECTORIES>)

  if (HAVE_LIB_GTEST)
    target_link_libraries (glog_test INTERFACE GTest::gtest)
  endif (HAVE_LIB_GTEST)

  if (HAVE_LIB_GMOCK)
    target_link_libraries (glog_test INTERFACE GTest::gmock)
  endif (HAVE_LIB_GMOCK)

  add_executable (logging_unittest
    src/logging_unittest.cc
  )

  target_link_libraries (logging_unittest PRIVATE glog_test)

  add_executable (stl_logging_unittest
    src/stl_logging_unittest.cc
  )

  target_link_libraries (stl_logging_unittest PRIVATE glog_test)

  if (HAVE_SYMBOLIZE)
    add_executable (symbolize_unittest
      src/symbolize_unittest.cc
    )

    target_link_libraries (symbolize_unittest PRIVATE glog_test)
  endif (HAVE_SYMBOLIZE)

  add_executable (demangle_unittest
    src/demangle_unittest.cc
  )

  target_link_libraries (demangle_unittest PRIVATE glog_test)

  if (HAVE_STACKTRACE)
    add_executable (stacktrace_unittest
      src/stacktrace_unittest.cc
    )

    target_link_libraries (stacktrace_unittest PRIVATE glog_test)
  endif (HAVE_STACKTRACE)

  add_executable (utilities_unittest
    src/utilities_unittest.cc
  )

  target_link_libraries (utilities_unittest PRIVATE glog_test)

  if (HAVE_STACKTRACE AND HAVE_SYMBOLIZE)
    add_executable (signalhandler_unittest
      src/signalhandler_unittest.cc
    )

    target_link_libraries (signalhandler_unittest PRIVATE glog_test)
  endif (HAVE_STACKTRACE AND HAVE_SYMBOLIZE)

  add_test (NAME demangle COMMAND demangle_unittest)
  add_test (NAME logging COMMAND logging_unittest)

  set_tests_properties (logging PROPERTIES TIMEOUT 30)
  # MacOS diff is not deterministic: use the output to determine whether the
  # test passed.
  set_tests_properties (logging PROPERTIES PASS_REGULAR_EXPRESSION ".*\nPASS\n.*")

  # FIXME: Skip flaky test
  set_tests_properties (logging PROPERTIES SKIP_REGULAR_EXPRESSION
    "Check failed: time_ns within LogTimes::LOG_PERIOD_TOL_NS of LogTimes::LOG_PERIOD_NS")

  if (APPLE)
    # FIXME: Skip flaky test
    set_property (TEST logging APPEND PROPERTY SKIP_REGULAR_EXPRESSION
      "unexpected new.*PASS\nTest with golden file failed. We'll try to show the diff:")
  endif (APPLE)

  if (TARGET signalhandler_unittest)
    add_test (NAME signalhandler COMMAND signalhandler_unittest)
  endif (TARGET signalhandler_unittest)

  if (TARGET stacktrace_unittest)
    add_test (NAME stacktrace COMMAND stacktrace_unittest)
    set_tests_properties (stacktrace PROPERTIES TIMEOUT 30)
  endif (TARGET stacktrace_unittest)

  add_test (NAME stl_logging COMMAND stl_logging_unittest)

  if (TARGET symbolize_unittest)
    add_test (NAME symbolize COMMAND symbolize_unittest)

    # FIXME: Skip flaky test when compiled in C++20 mode
    set_tests_properties (symbolize PROPERTIES SKIP_REGULAR_EXPRESSION
      [=[Check failed: streq\("nonstatic_func"\, TrySymbolize\(\(void \*\)\(&nonstatic_func\)\)\)]=])
  endif (TARGET symbolize_unittest)

  if (HAVE_LIB_GMOCK)
    add_executable (mock-log_unittest
      src/mock-log_unittest.cc
      src/mock-log.h
    )

    target_link_libraries (mock-log_unittest PRIVATE glog_test)

    add_test (NAME mock-log COMMAND mock-log_unittest)
  endif (HAVE_LIB_GMOCK)

  # Generate an initial cache

  get_cache_variables (_CACHEVARS)

  set (_INITIAL_CACHE
    ${glog_BINARY_DIR}/test_package_config/glog_package_config_initial_cache.cmake)

  # Package config test

  add_test (NAME cmake_package_config_init COMMAND ${CMAKE_COMMAND}
    -DTEST_BINARY_DIR=${glog_BINARY_DIR}/test_package_config
    -DINITIAL_CACHE=${_INITIAL_CACHE}
    -DCACHEVARS=${_CACHEVARS}
    -P ${glog_SOURCE_DIR}/cmake/TestInitPackageConfig.cmake
  )

  add_test (NAME cmake_package_config_generate COMMAND ${CMAKE_COMMAND}
    -DGENERATOR=${CMAKE_GENERATOR}
    -DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
    -DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
    -DINITIAL_CACHE=${_INITIAL_CACHE}
    -DPACKAGE_DIR=${glog_BINARY_DIR}
    -DPATH=$ENV{PATH}
    -DSOURCE_DIR=${glog_SOURCE_DIR}/src/package_config_unittest/working_config
    -DTEST_BINARY_DIR=${glog_BINARY_DIR}/test_package_config/working_config
    -P ${glog_SOURCE_DIR}/cmake/TestPackageConfig.cmake
  )

  add_test (NAME cmake_package_config_build COMMAND
    ${CMAKE_COMMAND} --build ${glog_BINARY_DIR}/test_package_config/working_config
                     --config $<CONFIG>
  )

  add_test (NAME cmake_package_config_cleanup COMMAND ${CMAKE_COMMAND} -E
    remove_directory
    ${glog_BINARY_DIR}/test_package_config
  )

  # Fixtures setup
  set_tests_properties (cmake_package_config_init PROPERTIES FIXTURES_SETUP
    cmake_package_config)
  set_tests_properties (cmake_package_config_generate PROPERTIES FIXTURES_SETUP
    cmake_package_config_working)

  # Fixtures cleanup
  set_tests_properties (cmake_package_config_cleanup PROPERTIES FIXTURES_CLEANUP
    cmake_package_config)

  # Fixture requirements
  set_tests_properties (cmake_package_config_generate PROPERTIES
    FIXTURES_REQUIRED cmake_package_config)
  set_tests_properties (cmake_package_config_build PROPERTIES
    FIXTURES_REQUIRED "cmake_package_config;cmake_package_config_working")

  add_executable (cleanup_immediately_unittest
    src/cleanup_immediately_unittest.cc)

  target_link_libraries (cleanup_immediately_unittest PRIVATE glog_test)

  add_executable (cleanup_with_absolute_prefix_unittest
    src/cleanup_with_absolute_prefix_unittest.cc)

  target_link_libraries (cleanup_with_absolute_prefix_unittest PRIVATE glog_test)

  add_executable (cleanup_with_relative_prefix_unittest
    src/cleanup_with_relative_prefix_unittest.cc)

  target_link_libraries (cleanup_with_relative_prefix_unittest PRIVATE glog_test)

  set (CLEANUP_LOG_DIR ${glog_BINARY_DIR}/cleanup_tests)

  add_test (NAME cleanup_init COMMAND
    ${CMAKE_COMMAND} -E make_directory ${CLEANUP_LOG_DIR})
  add_test (NAME cleanup_logdir COMMAND
    ${CMAKE_COMMAND} -E remove_directory ${CLEANUP_LOG_DIR})
  add_test (NAME cleanup_immediately COMMAND
    ${CMAKE_COMMAND}
    -DLOGCLEANUP=$<TARGET_FILE:cleanup_immediately_unittest>
    # NOTE The trailing slash is important
    -DTEST_DIR=${CLEANUP_LOG_DIR}/
    -P ${glog_SOURCE_DIR}/cmake/RunCleanerTest1.cmake
    WORKING_DIRECTORY ${glog_BINARY_DIR})
  add_test (NAME cleanup_with_absolute_prefix COMMAND
    ${CMAKE_COMMAND}
    -DLOGCLEANUP=$<TARGET_FILE:cleanup_with_absolute_prefix_unittest>
    -DTEST_DIR=${glog_BINARY_DIR}/
    -P ${glog_SOURCE_DIR}/cmake/RunCleanerTest2.cmake
    WORKING_DIRECTORY ${glog_BINARY_DIR})
  add_test (NAME cleanup_with_relative_prefix COMMAND
    ${CMAKE_COMMAND}
    -DLOGCLEANUP=$<TARGET_FILE:cleanup_with_relative_prefix_unittest>
    -DTEST_DIR=${glog_BINARY_DIR}/
    -DTEST_SUBDIR=test_subdir/
    -P ${glog_SOURCE_DIR}/cmake/RunCleanerTest3.cmake
    WORKING_DIRECTORY ${glog_BINARY_DIR})

  # Fixtures setup
  set_tests_properties (cleanup_init PROPERTIES FIXTURES_SETUP logcleanuptest)
  ## Fixtures cleanup
  set_tests_properties (cleanup_logdir PROPERTIES FIXTURES_CLEANUP logcleanuptest)
  # Fixture requirements
  set_tests_properties (cleanup_immediately PROPERTIES FIXTURES_REQUIRED logcleanuptest)
  set_tests_properties (cleanup_with_absolute_prefix PROPERTIES FIXTURES_REQUIRED logcleanuptest)
  set_tests_properties (cleanup_with_relative_prefix PROPERTIES FIXTURES_REQUIRED logcleanuptest)

  add_executable (striplog0_unittest
    src/striplog_unittest.cc
  )
  target_compile_definitions (striplog0_unittest PRIVATE GOOGLE_STRIP_LOG=0)
  target_link_libraries (striplog0_unittest PRIVATE glog_test)

  add_test (NAME striplog0 COMMAND striplog0_unittest)

  add_executable (striplog2_unittest
    src/striplog_unittest.cc
  )
  target_compile_definitions (striplog2_unittest PRIVATE GOOGLE_STRIP_LOG=2)
  target_link_libraries (striplog2_unittest PRIVATE glog_test)

  add_test (NAME striplog2 COMMAND striplog2_unittest)

  add_executable (striplog10_unittest
    src/striplog_unittest.cc
  )
  target_compile_definitions (striplog10_unittest PRIVATE GOOGLE_STRIP_LOG=10)
  target_link_libraries (striplog10_unittest PRIVATE glog_test)

  add_test (NAME striplog10 COMMAND striplog10_unittest)

  set_tests_properties (
    striplog0
    striplog2
    striplog10
    PROPERTIES WILL_FAIL ON
  )
endif (BUILD_TESTING)

install (TARGETS glog
  EXPORT glog-targets
  RUNTIME DESTINATION ${_glog_CMake_BINDIR}
  PUBLIC_HEADER DESTINATION ${_glog_CMake_INCLUDE_DIR}/glog
  LIBRARY DESTINATION ${_glog_CMake_LIBDIR}
  ARCHIVE DESTINATION ${_glog_CMake_LIBDIR})

if (WITH_PKGCONFIG)
  install (
    FILES "${glog_BINARY_DIR}/libglog.pc"
    DESTINATION "${_glog_CMake_LIBDIR}/pkgconfig"
  )
endif (WITH_PKGCONFIG)

set (glog_CMake_VERSION 3.0)

if (gflags_FOUND)
  # Ensure clients locate only the package config and not third party find
  # modules having the same name. This avoid cmake_policy PUSH/POP errors.
  if (CMAKE_VERSION VERSION_LESS 3.9)
    set (gflags_DEPENDENCY "find_dependency (gflags ${gflags_VERSION})")
  else (CMAKE_VERSION VERSION_LESS 3.9)
    # Passing additional find_package arguments to find_dependency is possible
    # starting with CMake 3.9.
    set (glog_CMake_VERSION 3.9)
    set (gflags_DEPENDENCY "find_dependency (gflags ${gflags_VERSION} NO_MODULE)")
  endif (CMAKE_VERSION VERSION_LESS 3.9)
endif (gflags_FOUND)

configure_package_config_file (glog-config.cmake.in
  ${glog_BINARY_DIR}/glog-config.cmake
  INSTALL_DESTINATION ${_glog_CMake_INSTALLDIR}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file (
  ${glog_BINARY_DIR}/glog-config-version.cmake
  COMPATIBILITY SameMajorVersion)

export (TARGETS glog NAMESPACE glog:: FILE glog-targets.cmake)
export (PACKAGE glog)

get_filename_component (_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)

# Directory containing the find modules relative to the config install
# directory.
file (RELATIVE_PATH glog_REL_CMake_MODULES
  ${_PREFIX}/${_glog_CMake_INSTALLDIR}
  ${_PREFIX}/${_glog_CMake_DATADIR}/glog-modules.cmake)

get_filename_component (glog_REL_CMake_DATADIR ${glog_REL_CMake_MODULES}
  DIRECTORY)

set (glog_FULL_CMake_DATADIR
  ${glog_BINARY_DIR}/${_glog_CMake_DATADIR})

configure_file (glog-modules.cmake.in
  ${glog_BINARY_DIR}/glog-modules.cmake @ONLY)

install (CODE
"
set (glog_FULL_CMake_DATADIR \"\\\${CMAKE_CURRENT_LIST_DIR}/${glog_REL_CMake_DATADIR}\")
set (glog_DATADIR_DESTINATION ${_glog_CMake_INSTALLDIR})

if (NOT IS_ABSOLUTE ${_glog_CMake_INSTALLDIR})
  set (glog_DATADIR_DESTINATION \"\${CMAKE_INSTALL_PREFIX}/\${glog_DATADIR_DESTINATION}\")
endif (NOT IS_ABSOLUTE ${_glog_CMake_INSTALLDIR})

configure_file (\"${glog_SOURCE_DIR}/glog-modules.cmake.in\"
  \"${glog_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/glog-modules.cmake\" @ONLY)
file (INSTALL
  \"${glog_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/glog-modules.cmake\"
  DESTINATION
  \"\${glog_DATADIR_DESTINATION}\")
"
  COMPONENT Development
)

install (FILES
  ${glog_BINARY_DIR}/glog-config.cmake
  ${glog_BINARY_DIR}/glog-config-version.cmake
  DESTINATION ${_glog_CMake_INSTALLDIR})

# Find modules in share/glog/cmake
install (DIRECTORY ${_glog_BINARY_CMake_DATADIR}
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/glog
  COMPONENT Development
  FILES_MATCHING PATTERN "*.cmake"
)

install (EXPORT glog-targets NAMESPACE glog:: DESTINATION
  ${_glog_CMake_INSTALLDIR})
