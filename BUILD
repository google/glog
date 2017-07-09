# Derived from the sample BUILD file provided here:
#       https://github.com/google/glog/issues/61
#
# Derived from https://github.com/google/glog/files/393474/BUILD.txt
#
# Currently gflags is not included, since having deps
# of ['//external:gflags'] does not seem to let it find
# 'gflags/gflags.h'.
#
# Currently we need to do
#     export GLOG_logtostderr=1
#
# To allow logging to stderr.

licenses(['notice'])

load(':bazel/glog.bzl', 'glog_internals')
glog_internals()

cc_library(
    name = 'glog',
    visibility = [
        '//visibility:public',
    ],
    srcs = [
        'src/base/commandlineflags.h',
        'src/base/googleinit.h',
        'src/demangle.cc',
        'src/logging.cc',
        'src/symbolize.cc',
        'src/vlog_is_on.cc',
        'src/raw_logging.cc',
        'src/utilities.cc'
    ],
    hdrs = [
        'src/base/mutex.h',
        'src/utilities.h',
        'src/demangle.h',
        'src/symbolize.h',
        'src/glog/log_severity.h',
        ':stl_logging_h',
        ':config_h',
        ':logging_h',
        ':vlog_is_on_h',
        ':raw_logging_h'
    ],
    includes = [
        'src',
    ],
    copts = [
        # Disable warnings that exists in glog
        '-Wno-sign-compare',
        '-Wno-unused-local-typedefs',
        ## Inject google namespace as 'google'
        "-D_START_GOOGLE_NAMESPACE_='namespace google {'",
        "-D_END_GOOGLE_NAMESPACE_='}'",
        "-DGOOGLE_NAMESPACE='google'",
        # Allows src/base/mutex.h to include pthread.h.
        '-DHAVE_PTHREAD',
        # Allows src/logging.cc to determine the host name.
        '-DHAVE_SYS_UTSNAME_H',
        # System header files enabler for src/utilities.cc
        # Enable system calls from syscall.h
        '-DHAVE_SYS_SYSCALL_H',
        # Enable system calls from sys/time.h
        '-DHAVE_SYS_TIME_H',
        '-DHAVE_STDINT_H',
        '-DHAVE_STRING_H',
        # For logging.cc
        '-DHAVE_PREAD',
        # Add the include paths the the generated .h files.
        '-I$(GENDIR)/%s' % PACKAGE_NAME,
    ],
)
