# Implement a macro glog_library() that the BUILD.bazel file can load.

# By default, glog is built with gflags support.  You can change this behavior
# by using glog_library(with_gflags=0)
#
# This file is inspired by the following sample BUILD files:
#       https://github.com/google/glog/issues/61
#       https://github.com/google/glog/files/393474/BUILD.txt
#
# Known issue: the namespace parameter is not supported on Win32.

def expand_template_impl(ctx):
    ctx.actions.expand_template(
        template = ctx.file.template,
        output = ctx.outputs.out,
        substitutions = ctx.attr.substitutions,
    )

expand_template = rule(
    implementation = expand_template_impl,
    attrs = {
        "template": attr.label(mandatory = True, allow_single_file = True),
        "substitutions": attr.string_dict(mandatory = True),
        "out": attr.output(mandatory = True),
    },
)

def dict_union(x, y):
    z = {}
    z.update(x)
    z.update(y)
    return z

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
        # Inject a C++ namespace.
        "-DGOOGLE_NAMESPACE='%s'" % namespace,
        "-DHAVE_CXX11_NULLPTR_T",
        "-DHAVE_STDINT_H",
        "-DHAVE_STRING_H",
        "-DGLOG_CUSTOM_PREFIX_SUPPORT",
        "-I%s/glog_internal" % gendir,
    ] + (["-DHAVE_LIB_GFLAGS"] if with_gflags else [])

    wasm_copts = [
        # Disable warnings that exists in glog.
        "-Wno-sign-compare",
        "-Wno-unused-function",
        "-Wno-unused-local-typedefs",
        "-Wno-unused-variable",
        # Allows src/base/mutex.h to include pthread.h.
        "-DHAVE_PTHREAD",
        # Allows src/logging.cc to determine the host name.
        "-DHAVE_SYS_UTSNAME_H",
        # For src/utilities.cc.
        "-DHAVE_SYS_TIME_H",
        "-DHAVE_UNWIND_H",
        # Enable dumping stacktrace upon sigaction.
        "-DHAVE_SIGACTION",
        # For logging.cc.
        "-DHAVE_PREAD",
        "-DHAVE___ATTRIBUTE__",
    ]

    linux_or_darwin_copts = wasm_copts + [
        # For src/utilities.cc.
        "-DHAVE_SYS_SYSCALL_H",
        # For src/logging.cc to create symlinks.
        "-DHAVE_UNISTD_H",
    ]

    freebsd_only_copts = [
        # Enable declaration of _Unwind_Backtrace
        "-D_GNU_SOURCE",
    ]

    darwin_only_copts = [
        # For stacktrace.
        "-DHAVE_DLADDR",
        # Avoid deprecated syscall().
        "-DHAVE_PTHREAD_THREADID_NP",
    ]

    windows_only_copts = [
        "-DGLOG_NO_ABBREVIATED_SEVERITIES",
        "-DHAVE_SNPRINTF",
        "-I" + src_windows,
    ]

    windows_only_srcs = [
        "src/glog/log_severity.h",
        "src/windows/dirent.h",
        "src/windows/port.cc",
        "src/windows/port.h",
    ]

    gflags_deps = ["@com_github_gflags_gflags//:gflags"] if with_gflags else []

    native.cc_library(
        name = "glog",
        visibility = ["//visibility:public"],
        srcs = [
            ":config_h",
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
            "src/stacktrace_unwind-inl.h",
            "src/stacktrace_windows-inl.h",
            "src/stacktrace_x86-inl.h",
            "src/symbolize.cc",
            "src/symbolize.h",
            "src/utilities.cc",
            "src/utilities.h",
            "src/vlog_is_on.cc",
        ] + select({
            "@bazel_tools//src/conditions:windows": windows_only_srcs,
            "//conditions:default": [],
        }),
        hdrs = [
            "src/glog/log_severity.h",
            "src/glog/platform.h",
            ":logging_h",
            ":raw_logging_h",
            ":stl_logging_h",
            ":vlog_is_on_h",
        ],
        strip_include_prefix = "src",
        defines = select({
            # GOOGLE_GLOG_DLL_DECL is normally set by export.h, but that's not
            # generated for Bazel.
            "@bazel_tools//src/conditions:windows": [
                "GOOGLE_GLOG_DLL_DECL=__declspec(dllexport)",
                "GLOG_DEPRECATED=__declspec(deprecated)",
                "GLOG_NO_ABBREVIATED_SEVERITIES",
            ],
            "//conditions:default": [
                "GLOG_DEPRECATED=__attribute__((deprecated))",
            ],
        }),
        copts =
            select({
                "@bazel_tools//src/conditions:windows": common_copts + windows_only_copts,
                "@bazel_tools//src/conditions:darwin": common_copts + linux_or_darwin_copts + darwin_only_copts,
                "@bazel_tools//src/conditions:freebsd": common_copts + linux_or_darwin_copts + freebsd_only_copts,
                ":wasm": common_copts + wasm_copts,
                "//conditions:default": common_copts + linux_or_darwin_copts,
            }),
        deps = gflags_deps + select({
            "@bazel_tools//src/conditions:windows": [":strip_include_prefix_hack"],
            "//conditions:default": [],
        }),
        **kwargs
    )

    # Workaround https://github.com/bazelbuild/bazel/issues/6337 by declaring
    # the dependencies without strip_include_prefix.
    native.cc_library(
        name = "strip_include_prefix_hack",
        hdrs = [
            "src/glog/log_severity.h",
            ":logging_h",
            ":raw_logging_h",
            ":stl_logging_h",
            ":vlog_is_on_h",
        ],
    )

    expand_template(
        name = "config_h",
        template = "src/config.h.cmake.in",
        out = "glog_internal/config.h",
        substitutions = {"#cmakedefine": "//cmakedefine"},
    )

    common_config = {
        "@ac_cv_cxx11_atomic@": "1",
        "@ac_cv_cxx11_constexpr@": "1",
        "@ac_cv_cxx11_chrono@": "1",
        "@ac_cv_cxx11_nullptr_t@": "1",
        "@ac_cv_cxx_using_operator@": "1",
        "@ac_cv_have_inttypes_h@": "0",
        "@ac_cv_have_u_int16_t@": "0",
        "@ac_cv_have_glog_export@": "0",
        "@ac_google_start_namespace@": "namespace google {",
        "@ac_google_end_namespace@": "}",
        "@ac_google_namespace@": "google",
    }

    posix_config = dict_union(common_config, {
        "@ac_cv_have_unistd_h@": "1",
        "@ac_cv_have_stdint_h@": "1",
        "@ac_cv_have_systypes_h@": "1",
        "@ac_cv_have_uint16_t@": "1",
        "@ac_cv_have___uint16@": "0",
        "@ac_cv_have___builtin_expect@": "1",
        "@ac_cv_have_libgflags@": "1" if with_gflags else "0",
        "@ac_cv___attribute___noinline@": "__attribute__((noinline))",
        "@ac_cv___attribute___noreturn@": "__attribute__((noreturn))",
        "@ac_cv___attribute___printf_4_5@": "__attribute__((__format__(__printf__, 4, 5)))",
    })

    windows_config = dict_union(common_config, {
        "@ac_cv_have_unistd_h@": "0",
        "@ac_cv_have_stdint_h@": "0",
        "@ac_cv_have_systypes_h@": "0",
        "@ac_cv_have_uint16_t@": "0",
        "@ac_cv_have___uint16@": "1",
        "@ac_cv_have___builtin_expect@": "0",
        "@ac_cv_have_libgflags@": "0",
        "@ac_cv___attribute___noinline@": "",
        "@ac_cv___attribute___noreturn@": "__declspec(noreturn)",
        "@ac_cv___attribute___printf_4_5@": "",
    })

    [
        expand_template(
            name = "%s_h" % f,
            template = "src/glog/%s.h.in" % f,
            out = "src/glog/%s.h" % f,
            substitutions = select({
                "@bazel_tools//src/conditions:windows": windows_config,
                "//conditions:default": posix_config,
            }),
        )
        for f in [
            "vlog_is_on",
            "stl_logging",
            "raw_logging",
            "logging",
        ]
    ]
