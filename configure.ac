## Process this file with autoconf to produce configure.
## In general, the safest way to proceed is to run the following:
##    % aclocal -I . -I `pwd`/../autoconf && autoheader && autoconf && automake

# make sure we're interpreted by some minimal autoconf
AC_PREREQ(2.57)

AC_INIT(glog, 0.1.1, opensource@google.com)
# The argument here is just something that should be in the current directory
# (for sanity checking)
AC_CONFIG_SRCDIR(README)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(src/config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AM_CONDITIONAL(GCC, test "$GCC" = yes)   # let the Makefile know if we're gcc

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# Check whether some low-level functions/files are available
AC_HEADER_STDC

# These are tested for by AC_HEADER_STDC, but I check again to set the var
AC_CHECK_HEADER(stdint.h, ac_cv_have_stdint_h=1, ac_cv_have_stdint_h=0)
AC_CHECK_HEADER(sys/types.h, ac_cv_have_systypes_h=1, ac_cv_have_systypes_h=0)
AC_CHECK_HEADER(inttypes.h, ac_cv_have_inttypes_h=1, ac_cv_have_inttypes_h=0)
AC_CHECK_HEADERS(syscall.h)
AC_CHECK_HEADERS(sys/syscall.h)
# For backtrace with glibc.
AC_CHECK_HEADERS(execinfo.h)
# For backtrace with libunwind.
AC_CHECK_HEADERS(libunwind.h, ac_cv_have_libunwind_h=1, ac_cv_have_libunwind_h=0)
AC_CHECK_HEADERS(ucontext.h)

AC_CHECK_SIZEOF(void *)

# These are the types I need.  We look for them in either stdint.h,
# sys/types.h, or inttypes.h, all of which are part of the default-includes.
AC_CHECK_TYPE(uint16_t, ac_cv_have_uint16_t=1, ac_cv_have_uint16_t=0)
AC_CHECK_TYPE(u_int16_t, ac_cv_have_u_int16_t=1, ac_cv_have_u_int16_t=0)
AC_CHECK_TYPE(__uint16, ac_cv_have___uint16=1, ac_cv_have___uint16=0)

AC_CHECK_FUNC(sigaltstack,
              AC_DEFINE(HAVE_SIGALTSTACK, 1,
                        [Define if you have the `sigaltstack' function]))
AC_CHECK_FUNC(dladdr,
              AC_DEFINE(HAVE_DLADDR, 1,
                        [Define if you have the `dladdr' function]))

AX_C___ATTRIBUTE__
# We only care about these two attributes.
if test x"$ac_cv___attribute__" = x"yes"; then
  ac_cv___attribute___noreturn="__attribute__ ((noreturn))"
  ac_cv___attribute___printf_4_5="__attribute__((__format__ (__printf__, 4, 5)))"
else
  ac_cv___attribute___noreturn=
  ac_cv___attribute___printf_4_5=
fi

AX_C___BUILTIN_EXPECT
if test x"$ac_cv___builtin_expect" = x"yes"; then
  ac_cv_have___builtin_expect=1
else
  ac_cv_have___builtin_expect=0
fi

AX_C___SYNC_VAL_COMPARE_AND_SWAP

# On x86_64, instead of libunwind, we can choose to compile with frame-pointers
# (This isn't needed on i386, where -fno-omit-frame-pointer is the default).
AC_ARG_ENABLE(frame_pointers,
              AS_HELP_STRING([--enable-frame-pointers],
                             [On x86_64 systems, compile with -fno-omit-frame-pointer (see INSTALL)]),
              enable_frame_pointers=no)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(, [return __x86_64__ == 1 ? 0 : 1])],
                  [is_x86_64=yes], [is_x86_64=no])
AM_CONDITIONAL(ENABLE_FRAME_POINTERS, test "$enable_frame_pointers" = yes)
AM_CONDITIONAL(X86_64, test "$is_x86_64" = yes)

# Some of the code in this directory depends on pthreads
ACX_PTHREAD

# Check if there is google-gflags library installed.
AC_CHECK_LIB(gflags, main, ac_cv_have_libgflags=1, ac_cv_have_libgflags=0)
if test x"$ac_cv_have_libgflags" = x"1"; then
  GFLAGS_LIBS=-lgflags
  AC_DEFINE(HAVE_LIB_GFLAGS, 1, [define if you have google gflags library])
else
  GFLAGS_LIBS=
fi

# We want to link in libunwind if it exists
UNWIND_LIBS=
# Unfortunately, we need to check the header file in addition to the
# lib file to check if libunwind is available since libunwind-0.98
# doesn't install all necessary header files.
if test x"$ac_cv_have_libunwind_h" = x"1"; then
 AC_CHECK_LIB(unwind, backtrace, UNWIND_LIBS=-lunwind)
fi
AC_SUBST(UNWIND_LIBS)
if test x"$UNWIND_LIBS" != x""; then
  AC_DEFINE(HAVE_LIB_UNWIND, 1, [define if you have libunwind])
fi

# We'd like to use read/write locks in several places in the code.
# See if our pthreads support extends to that.  Note: for linux, it
# does as long as you define _XOPEN_SOURCE appropriately.
AC_RWLOCK

# Find out what namespace 'normal' STL code lives in, and also what namespace
# the user wants our classes to be defined in
AC_CXX_STL_NAMESPACE
AC_DEFINE_GOOGLE_NAMESPACE(google)

AC_CXX_USING_OPERATOR

# We want to access the "PC" (Program Counter) register from a struct
# ucontext.  Every system has its own way of doing that.  We try all the
# possibilities we know about.  Note REG_PC should come first (REG_RIP
# is also defined on solaris, but does the wrong thing).
AC_MSG_CHECKING([how to access the program counter from a struct ucontext])
pc_fields="           uc_mcontext.gregs[[REG_PC]]"  # Solaris x86 (32 + 64 bit)
pc_fields="$pc_fields uc_mcontext.gregs[[REG_EIP]]" # Linux (i386)
pc_fields="$pc_fields uc_mcontext.gregs[[REG_RIP]]" # Linux (x86_64)
pc_fields="$pc_fields uc_mcontext.sc_ip"            # Linux (ia64)
pc_fields="$pc_fields uc_mcontext.uc_regs->gregs[[PT_NIP]]" # Linux (ppc)
pc_fields="$pc_fields uc_mcontext.gregs[[R15]]"     # Linux (arm old [untested])
pc_fields="$pc_fields uc_mcontext.arm_pc"           # Linux (arm new [untested])
pc_fields="$pc_fields uc_mcontext.mc_eip"           # FreeBSD (i386)
pc_fields="$pc_fields uc_mcontext.mc_rip"           # FreeBSD (x86_64 [untested])
pc_fields="$pc_fields uc_mcontext->ss.eip"          # OS X (i386, <=10.4)
pc_fields="$pc_fields uc_mcontext->__ss.__eip"      # OS X (i386, >=10.5)
pc_fields="$pc_fields uc_mcontext->ss.rip"          # OS X (x86_64)
pc_fields="$pc_fields uc_mcontext->__ss.__rip"      # OS X (>=10.5 [untested])
pc_fields="$pc_fields uc_mcontext->ss.srr0"         # OS X (ppc, ppc64 [untested])
pc_fields="$pc_fields uc_mcontext->__ss.__srr0"     # OS X (>=10.5 [untested])
pc_field_found=false
for pc_field in $pc_fields; do
  if ! $pc_field_found; then
    AC_TRY_COMPILE([#define _GNU_SOURCE 1
                    #include <ucontext.h>],
                   [ucontext_t u; return u.$pc_field == 0;],
                   AC_DEFINE_UNQUOTED(PC_FROM_UCONTEXT, $pc_field,
                                      How to access the PC from a struct ucontext)
                   AC_MSG_RESULT([$pc_field])
  		 pc_field_found=true)
  fi
done
if ! $pc_field_found; then
  AC_MSG_WARN(Could not find the PC.  Will not output failed addresses...)
fi

# These are what's needed by logging.h.in and raw_logging.h.in
AC_SUBST(ac_google_start_namespace)
AC_SUBST(ac_google_end_namespace)
AC_SUBST(ac_google_namespace)
AC_SUBST(ac_cv_cxx_using_operator)
AC_SUBST(ac_cv___attribute___noreturn)
AC_SUBST(ac_cv___attribute___printf_4_5)
AC_SUBST(ac_cv_have___builtin_expect)
AC_SUBST(ac_cv_have_stdint_h)
AC_SUBST(ac_cv_have_systypes_h)
AC_SUBST(ac_cv_have_inttypes_h)
AC_SUBST(ac_cv_have_uint16_t)
AC_SUBST(ac_cv_have_u_int16_t)
AC_SUBST(ac_cv_have___uint16)
AC_SUBST(ac_cv_have_libgflags)
AC_SUBST(GFLAGS_LIBS)

# Write generated configuration file
AC_CONFIG_FILES([Makefile src/glog/logging.h src/glog/raw_logging.h src/glog/vlog_is_on.h src/glog/stl_logging.h])
AC_OUTPUT
