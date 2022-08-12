load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

licenses(["notice"])

cc_binary(
    name = "stacktrace",
    srcs = ["stacktrace.cpp"],
    deps = ["//:glog"],
)

cc_test(
    name = "stacktrace_test",
    srcs = ["test.cpp"],
    env = {"TARGET_APP": "$(location :stacktrace)"},
    deps = [":stacktrace"],
)
