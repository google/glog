licenses(['notice'])

namespace = 'google'
with_gflags = 1
with_libunwind = 1

cc_library(
    name = 'glog',
    visibility = [ '//visibility:public' ],
    srcs = glob([
        'src/base/commandlineflags.h',
        'src/base/googleinit.h',
        'src/demangle.cc',
        'src/logging.cc',
        'src/raw_logging.cc',
        'src/signalhandler.cc',
        'src/stacktrace_*-inl.h',
        'src/symbolize.cc',
        'src/utilities.cc',
        'src/vlog_is_on.cc',
    ]),
    hdrs = [
        'src/base/mutex.h',
        'src/demangle.h',
        'src/stacktrace.h',
        'src/symbolize.h',
        'src/utilities.h',
        'src/glog/log_severity.h',
    ],
    includes = [
        'src',
    ],
    copts = [
        # Disable warnings that exists in glog.
        '-Wno-invalid-noreturn',
        '-Wno-sign-compare',
        '-Wno-unused-const-variable',
        '-Wno-unused-function',
        '-Wno-unused-local-typedefs',
        '-Wno-unused-variable',
        # Inject a C++ namespace.
        "-D_START_GOOGLE_NAMESPACE_='namespace %s {'" % namespace,
        "-D_END_GOOGLE_NAMESPACE_='}'",
        "-DGOOGLE_NAMESPACE='%s'" % namespace,
        # Allows src/base/mutex.h to include pthread.h.
        '-DHAVE_PTHREAD',
        # Allows src/logging.cc to determine the host name.
        '-DHAVE_SYS_UTSNAME_H',
        # For src/utilities.cc.
        '-DHAVE_SYS_SYSCALL_H',
        '-DHAVE_SYS_TIME_H',
        '-DHAVE_STDINT_H',
        '-DHAVE_STRING_H',
        # Enable dumping stacktrace upon sigaction.
        '-DHAVE_SIGACTION',
        # For logging.cc.
        '-DHAVE_PREAD',
    ] + ([
        # Use gflags to parse CLI arguments.
        # NOTE: These parenthesis are necessary.
        '-DHAVE_LIB_GFLAGS',
    ] if with_gflags else []) + ([
        # Use linunwind to get stacktrace.
        '-DHAVE_LIB_UNWIND',
    ] if with_libunwind else []),
    deps = [
        ':internal_headers',
    ] + ([
        '//third_party/gflags',
    ] if with_gflags else []) + ([
        '//third_party/libunwind',
    ] if with_libunwind else []),
)


cc_library(
    name = 'internal_headers',
    hdrs = [
        ':config_h',
        ':logging_h',
        ':raw_logging_h',
        ':stl_logging_h',
        ':vlog_is_on_h',
    ],
    includes = [
		PACKAGE_NAME,
    ] if PACKAGE_NAME else [],
)


genrule(
    name = 'gen_sh',
    outs = [
        'gen.sh',
    ],
    cmd = r'''\
#!/bin/sh
cat > $@ <<"EOF"
sed -e 's/@ac_cv_have_unistd_h@/1/g' \
    -e 's/@ac_cv_have_stdint_h@/1/g' \
    -e 's/@ac_cv_have_systypes_h@/1/g' \
    -e 's/@ac_cv_have_libgflags_h@/1/g' \
    -e 's/@ac_cv_have_uint16_t@/1/g' \
    -e 's/@ac_cv_have___builtin_expect@/1/g' \
    -e 's/@ac_cv_have_.*@/0/g' \
    -e 's/@ac_google_start_namespace@/namespace google {/g' \
    -e 's/@ac_google_end_namespace@/}/g' \
    -e 's/@ac_google_namespace@/google/g' \
    -e 's/@ac_cv___attribute___noinline@/__attribute__((noinline))/g' \
    -e 's/@ac_cv___attribute___noreturn@/__attribute__((noreturn))/g' \
    -e 's/@ac_cv___attribute___printf_4_5@/__attribute__((__format__ (__printf__, 4, 5)))/g'
EOF
''',
)


genrule(
    name = 'config_h',
    srcs = [
        'src/config.h.cmake.in',
    ],
    outs = [
        '/'.join([PACKAGE_NAME, 'config.h']) if PACKAGE_NAME else 'config.h',
    ],
    cmd = "awk '{ gsub(/^#cmakedefine/, \"//cmakedefine\"); print; }' $(<) > $(@)",
)


[genrule(
    name = '%s_h' % f,
    srcs = [
        'src/glog/%s.h.in' % f,
    ],
    outs = [
        '/'.join([PACKAGE_NAME, 'glog/%s.h' % f]) \
                if PACKAGE_NAME else 'glog/%s.h' % f,
    ],
    cmd = '$(location :gen_sh) < $(<) > $(@)',
    tools = [':gen_sh'],
) for f in [
        'vlog_is_on',
        'stl_logging',
        'raw_logging',
        'logging',
    ]
]
