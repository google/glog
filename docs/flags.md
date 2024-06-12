# Adjusting Output

Several flags influence glog's output behavior.

## Using Command-line Parameters and Environment Variables

If the [Google gflags
library](https://github.com/gflags/gflags) is installed on your machine,
the build system will automatically detect and use it, allowing you to
pass flags on the command line.

!!! example "Activate `--logtostderr` in an application from the command line"
    A binary `you_application` that uses glog can be started using
    ``` bash
    ./your_application --logtostderr=1
    ```
    to log to `stderr` instead of writing the output to a log file.

!!! tip
    You can set boolean flags to `true` by specifying `1`, `true`, or `yes`. To
    set boolean flags to `false`, specify `0`, `false`, or `no`. In either case
    the spelling is case-insensitive.


If the Google gflags library isn't installed, you set flags via
environment variables, prefixing the flag name with `GLOG_`, e.g.,

!!! example "Activate `logtostderr` without gflags"
    ``` bash
    GLOG_logtostderr=1 ./your_application
    ```

The following flags are most commonly used:

`logtostderr` (`bool`, default=`false`)

:   Log messages to `stderr` instead of logfiles.

`stderrthreshold` (`int`, default=2, which is `ERROR`)

:   Copy log messages at or above this level to `stderr` in addition to
    logfiles. The numbers of severity levels `INFO`, `WARNING`, `ERROR`,
    and `FATAL` are 0, 1, 2, and 3, respectively.

`minloglevel` (`int`, default=0, which is `INFO`)

:   Log messages at or above this level. Again, the numbers of severity
    levels `INFO`, `WARNING`, `ERROR`, and `FATAL` are 0, 1, 2, and 3,
    respectively.

`log_dir` (`string`, default="")

:   If specified, logfiles are written into this directory instead of
    the default logging directory.

`v` (`int`, default=0)

:   Show all `#!cpp VLOG(m)` messages for `m` less or equal the value of this
    flag. Overridable by `#!bash --vmodule`. Refer to [verbose
    logging](logging.md#verbose-logging) for more detail.

`vmodule` (`string`, default="")

:   Per-module verbose level. The argument has to contain a
    comma-separated list of `<module name>=<log level>`. `<module name>` is a
    glob pattern (e.g., `gfs*` for all modules whose name starts with "gfs"),
    matched against the filename base (that is, name ignoring .cc/.h./-inl.h).
    `<log level>` overrides any value given by `--v`. See also [verbose
    logging](logging.md#verbose-logging) for more details.

Additional flags are defined in
[flags.cc](https://github.com/google/glog/blob/master/src/flags.cc). Please see
the source for their complete list.

## Modifying Flags Programmatically

You can also modify flag values in your program by modifying global variables
`FLAGS_*`. Most settings start working immediately after you update `FLAGS_*`.
The exceptions are the flags related to destination files. For instance, you
might want to set `FLAGS_log_dir` before calling `google::InitGoogleLogging`.

!!! example "Setting `log_dir` at runtime"
    ``` cpp
    LOG(INFO) << "file";
    // Most flags work immediately after updating values.
    FLAGS_logtostderr = 1;
    LOG(INFO) << "stderr";
    FLAGS_logtostderr = 0;
    // This won’t change the log destination. If you want to set this
    // value, you should do this before google::InitGoogleLogging .
    FLAGS_log_dir = "/some/log/directory";
    LOG(INFO) << "the same file";
    ```
