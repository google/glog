def glog_internals():
    # Below are the generation rules that generates the necessary header files
    # for glog. Originally they are generated by CMAKE configure_file() command,
    # which replaces certain template placeholders in the .in files with
    # provided values.

    # gen_sh is a bash script that provides the values for generated header
    # files. Under the hood it is just a wrapper over sed.
    native.genrule(
        name = 'gen_sh',
        outs = [
            'gen.sh',
        ],
        cmd = '''
cat > $@ <<"EOF"
#! /bin/sh
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
            'config.h',
        ],
        cmd = "awk '{ gsub(/^#cmakedefine/, \"//cmakedefine\"); print; }' $(<) > $(@)",
    )

    native.genrule(
        name = 'logging_h',
        srcs = [
            'src/glog/logging.h.in',
        ],
        outs = [
            'glog/logging.h',
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
            'glog/raw_logging.h',
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
            'glog/stl_logging.h',
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
            'glog/vlog_is_on.h',
        ],
        cmd = '$(location :gen_sh) < $(<) > $(@)',
        tools = [':gen_sh'],
    )
