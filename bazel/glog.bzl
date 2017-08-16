# Derived from the sample BUILD file provided here:
#       https://github.com/google/glog/issues/61
#
# Derived from https://github.com/google/glog/files/393474/BUILD.txt

def glog_library():
    internal_headers = [
        ':config_h',
        ':logging_h',
        ':raw_logging_h',
        ':stl_logging_h',
        ':vlog_is_on_h',
    ]

    if PACKAGE_NAME:
        native.cc_library(
            name = 'internal_headers',
            hdrs = internal_headers,
            includes = [
	    		PACKAGE_NAME,
            ],
        )
    else:
        native.cc_library(
            name = 'internal_headers',
            hdrs = internal_headers,
        )

    native.genrule(
        name = 'gen_sh',
        outs = [
            'gen.sh',
        ],
        cmd = '''
#! /bin/sh
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
EOF''')

    native.genrule(
        name = 'config_h',
        srcs = [
            'src/config.h.cmake.in',
        ],
        outs = [
            '/'.join([PACKAGE_NAME, 'config.h']) if PACKAGE_NAME else 'config.h',
        ],
        cmd = "awk '{ gsub(/^#cmakedefine/, \"//cmakedefine\"); print; }' $(<) > $(@)",
    )

    native.genrule(
        name = 'logging_h',
        srcs = [
            'src/glog/logging.h.in',
        ],
        outs = [
            '/'.join([PACKAGE_NAME, 'glog/logging.h']) if PACKAGE_NAME else 'glog/logging.h',
        ],
        cmd = '$(location :gen_sh) < $(<) > $(@)',
        tools = [':gen_sh'],
    )

    native.genrule(
        name = 'raw_logging_h',
        srcs = [
            'src/glog/raw_logging.h.in',
        ],
        outs = [
            '/'.join([PACKAGE_NAME, 'glog/raw_logging.h']) if PACKAGE_NAME else 'glog/raw_logging.h',
        ],
        cmd = '$(location :gen_sh) < $(<) > $(@)',
        tools = [':gen_sh'],
    )

    native.genrule(
        name = 'stl_logging_h',
        srcs = [
            'src/glog/stl_logging.h.in',
        ],
        outs = [
            '/'.join([PACKAGE_NAME, 'glog/stl_logging.h']) if PACKAGE_NAME else 'glog/stl_logging.h',
        ],
        cmd = '$(location :gen_sh) < $(<) > $(@)',
        tools = [':gen_sh'],
    )

    native.genrule(
        name = 'vlog_is_on_h',
        srcs = [
            'src/glog/vlog_is_on.h.in',
        ],
        outs = [
            '/'.join([PACKAGE_NAME, 'glog/vlog_is_on.h']) if PACKAGE_NAME else 'glog/vlog_is_on.h',
        ],
        cmd = '$(location :gen_sh) < $(<) > $(@)',
        tools = [':gen_sh'],
    )
