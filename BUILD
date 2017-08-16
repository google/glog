licenses(['notice'])

load(':bazel/glog.bzl', 'glog_library')
glog_library()

cc_library(
    name = 'glog',
    visibility = [ '//visibility:public' ],
    srcs = [
        'src/base/commandlineflags.h',
        'src/base/googleinit.h',
        'src/demangle.cc',
        'src/logging.cc',
        'src/raw_logging.cc',
        'src/symbolize.cc',
        'src/utilities.cc',
        'src/vlog_is_on.cc',
    ],
    hdrs = [
        'src/base/mutex.h',
        'src/demangle.h',
        'src/symbolize.h',
        'src/utilities.h',
        'src/glog/log_severity.h',
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
    ],
    deps = [
        ':internal_headers',
    ]
)
