# Implement a macro glog_library() that the BUILD file can load.

# By default, glog is built with gflags support.  You can change this behavior
# by using glog_library(with_gflags=0)
#
# This file is inspired by the following sample BUILD files:
#       https://github.com/google/glog/issues/61
#       https://github.com/google/glog/files/393474/BUILD.txt
#
# Known issue: the namespace parameter is not supported on Win32.

def glog_library(namespace = "google", with_gflags = 1, **kwargs):
    if native.repository_name() != "@":
        repo_name = native.repository_name().lstrip("@")
        gendir = "$(GENDIR)/external/" + repo_name
        src_windows = "external/%s/src/windows" % repo_name
    else:
        gendir = "$(GENDIR)"
        src_windows = "src/windows"

    # Config setting for WebAssembly target.
    native.config_setting(
        name = "wasm",
        values = {"cpu": "wasm"},
    )

    common_copts = [
        "-DGLOG_BAZEL_BUILD",
        "-DHAVE_STDINT_H",
        "-DHAVE_STRING_H",
        "-DHAVE_UNWIND_H",
    ] + (["-DHAVE_LIB_GFLAGS"] if with_gflags else [])

    wasm_copts = [
        # Disable warnings that exists in glog.
        "-Wno-sign-compare",
        "-Wno-unused-function",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-variable",
        # Inject a C++ namespace.
        "-DGOOGLE_NAMESPACE='%s'" % namespace,
        # Allows src/base/mutex.h to include pthread.h.
        "-DHAVE_PTHREAD",
        # Allows src/logging.cc to determine the host name.
        "-DHAVE_SYS_UTSNAME_H",
        # For src/utilities.cc.
        "-DHAVE_SYS_TIME_H",
        # Enable dumping stacktrace upon sigaction.
        "-DHAVE_SIGACTION",
        # For logging.cc.
        "-DHAVE_PREAD",
        "-DHAVE___ATTRIBUTE__",
        "-I%s/glog_internal" % gendir,
    ]

    linux_or_darwin_copts = wasm_copts + [
        # For src/utilities.cc.
        "-DHAVE_SYS_SYSCALL_H",
    ]

    freebsd_only_copts = [
        # Enable declaration of _Unwind_Backtrace
        "-D_GNU_SOURCE",
    ]

    darwin_only_copts = [
        # For stacktrace.
        "-DHAVE_DLADDR",
    ]

    windows_only_copts = [
        "-DHAVE_SNPRINTF",
        "-I" + src_windows,
    ]

    windows_only_srcs = [
        "src/glog/log_severity.h",
        "src/windows/config.h",
        "src/windows/port.cc",
        "src/windows/port.h",
    ]

    gflags_deps = ["@com_github_gflags_gflags//:gflags"] if with_gflags else []

    native.cc_library(
        name = "glog",
        visibility = ["//visibility:public"],
        srcs = [
            "src/base/commandlineflags.h",
            "src/base/googleinit.h",
            "src/base/mutex.h",
            "src/demangle.cc",
            "src/demangle.h",
            "src/logging.cc",
            "src/raw_logging.cc",
            "src/signalhandler.cc",
            "src/stacktrace.h",
            "src/stacktrace_generic-inl.h",
            "src/stacktrace_libunwind-inl.h",
            "src/stacktrace_powerpc-inl.h",
            "src/stacktrace_windows-inl.h",
            "src/stacktrace_x86-inl.h",
            "src/stacktrace_x86_64-inl.h",
            "src/symbolize.cc",
            "src/symbolize.h",
            "src/utilities.cc",
            "src/utilities.h",
            "src/vlog_is_on.cc",
        ] + select({
            "@bazel_tools//src/conditions:windows": windows_only_srcs,
            "//conditions:default": [":config_h"],
        }),
        copts =
            select({
                "@bazel_tools//src/conditions:windows": common_copts + windows_only_copts,
                "@bazel_tools//src/conditions:darwin": common_copts + linux_or_darwin_copts + darwin_only_copts,
                "@bazel_tools//src/conditions:freebsd": common_copts + linux_or_darwin_copts + freebsd_only_copts,
                ":wasm": common_copts + wasm_copts,
                "//conditions:default": common_copts + linux_or_darwin_copts,
            }),
        deps = [
            ":glog_headers",
        ] + gflags_deps,
        **kwargs
    )

    # glog headers vary depending on the os.
    native.cc_library(
        name = "glog_headers",
        deps = select({
            "@bazel_tools//src/conditions:windows": [":windows_glog_headers"],
            "//conditions:default": [":default_glog_headers"],
        }),
    )

    native.cc_library(
        name = "windows_glog_headers",
        hdrs = native.glob(["src/windows/glog/*.h"]),
        strip_include_prefix = "src/windows",
        # We need to override the default GOOGLE_GLOG_DLL_DECL from
        # src/windows/glog/*.h to match src/windows/config.h.
        defines = ["GOOGLE_GLOG_DLL_DECL=__declspec(dllexport)"],
        deps = [":strip_include_prefix_hack"],
    )

    # Workaround https://github.com/bazelbuild/bazel/issues/6337 by declaring
    # the dependencies without strip_include_prefix.
    native.cc_library(
        name = "strip_include_prefix_hack",
        hdrs = native.glob(["src/windows/*.h"]),
    )

    native.cc_library(
        name = "default_glog_headers",
        strip_include_prefix = "src",
        hdrs = [
            "src/glog/log_severity.h",
            ":logging_h",
            ":raw_logging_h",
            ":stl_logging_h",
            ":vlog_is_on_h",
        ],
    )
    native.genrule(
        name = "config_h",
        srcs = [
            "src/config.h.cmake.in",
        ],
        outs = [
            "glog_internal/config.h",
        ],
        cmd = "awk '{ gsub(/^#cmakedefine/, \"//cmakedefine\"); print; }' $< > $@",
    )

    native.genrule(
        name = "gen_sh",
        outs = [
            "gen.sh",
        ],
        cmd = r'''\
#!/bin/sh
cat > $@ <<"EOF"
sed -e 's/@ac_cv_cxx_using_operator@/1/g' \
    -e 's/@ac_cv_have_unistd_h@/1/g' \
    -e 's/@ac_cv_have_stdint_h@/1/g' \
    -e 's/@ac_cv_have_systypes_h@/1/g' \
    -e 's/@ac_cv_have_libgflags@/{}/g' \
    -e 's/@ac_cv_have_uint16_t@/1/g' \
    -e 's/@ac_cv_have___builtin_expect@/1/g' \
    -e 's/@ac_cv_have_.*@/0/g' \
    -e 's/@ac_google_start_namespace@/namespace google {{/g' \
    -e 's/@ac_google_end_namespace@/}}/g' \
    -e 's/@ac_google_namespace@/google/g' \
    -e 's/@ac_cv___attribute___noinline@/__attribute__((noinline))/g' \
    -e 's/@ac_cv___attribute___noreturn@/__attribute__((noreturn))/g' \
    -e 's/@ac_cv___attribute___printf_4_5@/__attribute__((__format__ (__printf__, 4, 5)))/g'
EOF
'''.format(int(with_gflags)),
    )

    [
        native.genrule(
            name = "%s_h" % f,
            srcs = [
                "src/glog/%s.h.in" % f,
            ],
            outs = [
                "src/glog/%s.h" % f,
            ],
            cmd = "$(location :gen_sh) < $< > $@",
            tools = [":gen_sh"],
        )
        for f in [
            "vlog_is_on",
            "stl_logging",
            "raw_logging",
            "logging",
        ]
    ]
