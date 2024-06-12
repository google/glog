# Logging

glog defines a series of macros that simplify many common logging tasks. You can
log messages by [severity level](#severity-levels), [control logging](flags.md)
behavior from the command line, log based on
[conditionals](#conditional-occasional-logging), abort the program when
[expected conditions](#runtime-checks) are not met, introduce your [own logging
levels](#verbose-logging), [customize the prefix](#format-customization)
attached to log messages, and more.


## Severity Levels

You can specify one of the following severity levels (in increasing order of
severity):

1.  `INFO`,
2.  `WARNING`,
3.  `ERROR`, and
4.  `FATAL`.

Logging a `FATAL` message terminates the program (after the message is logged).

!!! note
    Messages of a given severity are logged not only to corresponding severity
    logfile but also to other logfiles of lower severity. For instance, a
    message of severity `FATAL` will be logged to logfiles of severity `FATAL`,
    `ERROR`, `WARNING`, and `INFO`.

The `DFATAL` severity logs a `FATAL` error in [debug mode](#debugging-support)
(i.e., there is no `NDEBUG` macro defined), but avoids halting the program in
production by automatically reducing the severity to `ERROR`.

## Log Files

Unless otherwise specified, glog uses the format

    <tmp>/<program name>.<hostname>.<user name>.log.<severity level>.<date>-<time>.<pid>

for log filenames written to a directory designated as `<tmp>` and
determined according to the following rules.

**Windows**

:   glog uses the
    [GetTempPathA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-gettemppatha)
    API function to retrieve the directory for temporary files with a
    fallback to

    1.  `C:\TMP\`
    2.  `C:\TEMP\`

    (in the order given.)

**non-Windows**

:   The directory is determined by referencing the environment variables

    1.  `TMPDIR`
    2.  `TMP`

    if set with a fallback to `/tmp/`.

The default path to a log file on Linux, for instance, could be

    /tmp/hello_world.example.com.hamaji.log.INFO.20080709-222411.10474

By default, glog echos `ERROR` and `FATAL` messages to standard error in
addition to log files.

## Log Line Prefix Format

Log lines have this form:

    Lyyyymmdd hh:mm:ss.uuuuuu threadid file:line] msg...

where the fields are defined as follows:

  | Placeholder         | Meaning                                                               |
  | ------------------- | ----------------------------------------------------------------------|
  | `L`                 | A single character, representing the log level (e.g., `I` for `INFO`) |
  | `yyyy`              | The year                                                              |
  | `mm`                | The month (zero padded; i.e., May is `05`)                            |
  | `dd`                | The day (zero padded)                                                 |
  | `hh:mm:ss.uuuuuu`   | Time in hours, minutes and fractional seconds                         |
  | `threadid`          | The space-padded thread ID                                            |
  | `file`              | The file name                                                         |
  | `line`              | The line number                                                       |
  | `msg`               | The user-supplied message                                             |

!!! example "Default log line prefix format"

    ```
    I1103 11:57:31.739339 24395 google.cc:2341] Command line: ./some_prog
    I1103 11:57:31.739403 24395 google.cc:2342] Process id 24395
    ```

!!! note
    Although microseconds are useful for comparing events on a single machine,
    clocks on different machines may not be well synchronized. Hence, use with
    caution when comparing the low bits of timestamps from different machines.

### Format Customization

The predefined log line prefix can be replaced using a user-provided callback
that formats the corresponding output.

For each log entry, the callback will be invoked with a reference to a
`google::LogMessage` instance containing the severity, filename, line
number, thread ID, and time of the event. It will also be given a
reference to the output stream, whose contents will be prepended to the actual
message in the final log line.

To enable the use of a prefix formatter, use the

``` cpp
google::InstallPrefixFormatter(&MyPrefixFormatter);
```

function to pass a pointer to the corresponding `MyPrefixFormatter` callback
during initialization. `InstallPrefixFormatter` takes a second optional argument
of type `#!cpp void*` that allows supplying user data to the callback.

!!! example "Custom prefix formatter"
    The following function outputs a prefix that matches glog's default format.
    The third parameter `data` can be used to access user-supplied data which
    unless specified defaults to `#!cpp nullptr`.

    ``` cpp
    void MyPrefixFormatter(std::ostream& s, const google::LogMessage& m, void* /*data*/) {
       s << google::GetLogSeverityName(m.severity())[0]
       << setw(4) << 1900 + m.time().year()
       << setw(2) << 1 + m.time().month()
       << setw(2) << m.time().day()
       << ' '
       << setw(2) << m.time().hour() << ':'
       << setw(2) << m.time().min()  << ':'
       << setw(2) << m.time().sec() << "."
       << setw(6) << m.time().usec()
       << ' '
       << setfill(' ') << setw(5)
       << m.thread_id() << setfill('0')
       << ' '
       << m.basename() << ':' << m.line() << "]";
    }
    ```


## Conditional / Occasional Logging

Sometimes, you may only want to log a message under certain conditions.
You can use the following macros to perform conditional logging:

``` cpp
LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
```

The "Got lots of cookies" message is logged only when the variable
`num_cookies` exceeds 10. If a line of code is executed many times, it may be
useful to only log a message at certain intervals. This kind of logging is most
useful for informational messages.

``` cpp
LOG_EVERY_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";
```

The above line outputs a log messages on the 1st, 11th, 21st, ... times
it is executed.

!!! note
    The placeholder `#!cpp google::COUNTER` identifies the recurring repetition.

You can combine conditional and occasional logging with the following
macro.

``` cpp
LOG_IF_EVERY_N(INFO, (size > 1024), 10) << "Got the " << google::COUNTER
                                        << "th big cookie";
```

Instead of outputting a message every nth time, you can also limit the
output to the first n occurrences:

``` cpp
LOG_FIRST_N(INFO, 20) << "Got the " << google::COUNTER << "th cookie";
```

Outputs log messages for the first 20 times it is executed. The `#!cpp
google::COUNTER` identifier indicates which repetition is happening.

Other times, it is desired to only log a message periodically based on a
time. For instance, to log a message every 10ms:

``` cpp
LOG_EVERY_T(INFO, 0.01) << "Got a cookie";
```

Or every 2.35s:

``` cpp
LOG_EVERY_T(INFO, 2.35) << "Got a cookie";
```

## Verbose Logging

When you are chasing difficult bugs, thorough log messages are very
useful. However, you may want to ignore too verbose messages in usual
development. For such verbose logging, glog provides the `VLOG` macro, which
allows you to define your own numeric logging levels.

The `#!bash --v` command line option controls which verbose messages are logged:

``` cpp
VLOG(1) << "I’m printed when you run the program with --v=1 or higher";
VLOG(2) << "I’m printed when you run the program with --v=2 or higher";
```

With `VLOG`, the lower the verbose level, the more likely messages are to be
logged. For example, if `#!bash --v==1`, `#!cpp VLOG(1)` will log, but `#!cpp
VLOG(2)` will not log.

!!! warning
    The `VLOG` behavior is opposite of the severity level logging, where
    `INFO`, `ERROR`, etc. are defined in increasing order and thus
    `#!bash --minloglevel` of 1 will only log `WARNING` and above.

Though you can specify any integers for both `VLOG` macro and `--v` flag, the
common values for them are small positive integers. For example, if you write
`#!cpp VLOG(0)`, you should specify `--v=-1` or lower to silence it. This is less
useful since we may not want verbose logs by default in most cases. The `VLOG`
macros always log at the `INFO` log level (when they log at all).

Verbose logging can be controlled from the command line on a per-module basis:

``` bash
--vmodule=mapreduce=2,file=1,gfs*=3 --v=0
```

Specifying these options will specifically:

1.  Print `#!cpp VLOG(2)` and lower messages from mapreduce.{h,cc}
2.  Print `#!cpp VLOG(1)` and lower messages from file.{h,cc}
3.  Print `#!cpp VLOG(3)` and lower messages from files prefixed with "gfs"
4.  Print `#!cpp VLOG(0)` and lower messages from elsewhere

The wildcarding functionality 3. supports both `*` (matches 0 or more
characters) and `?` (matches any single character) wildcards. Please also refer
to [command line flags](flags.md) for more information.

There's also `#!cpp VLOG_IS_ON(n)` "verbose level" condition macro. This macro
returns `#!cpp true` when the `--v` is equal to or greater than `n`. The macro can be
used as follows:

``` cpp
if (VLOG_IS_ON(2)) {
    // (1)
}
```

1. Here we can perform some logging preparation and logging that can’t be
   accomplished with just `#!cpp VLOG(2) << "message ...";`

Verbose level condition macros `VLOG_IF`, `VLOG_EVERY_N` and `VLOG_IF_EVERY_N`
behave analogous to `LOG_IF`, `LOG_EVERY_N`, `LOG_IF_EVERY_N`, but accept a
numeric verbosity level as opposed to a severity level.

``` cpp
VLOG_IF(1, (size > 1024))
   << "I’m printed when size is more than 1024 and when you run the "
      "program with --v=1 or more";
VLOG_EVERY_N(1, 10)
   << "I’m printed every 10th occurrence, and when you run the program "
      "with --v=1 or more. Present occurrence is " << google::COUNTER;
VLOG_IF_EVERY_N(1, (size > 1024), 10)
   << "I’m printed on every 10th occurrence of case when size is more "
      " than 1024, when you run the program with --v=1 or more. ";
      "Present occurrence is " << google::COUNTER;
```


!!! info "Performance"
    The conditional logging macros provided by glog (e.g., `CHECK`, `LOG_IF`,
    `VLOG`, etc.) are carefully implemented and don't execute the right hand
    side expressions when the conditions are false. So, the following check may
    not sacrifice the performance of your application.

    ``` cpp
    CHECK(obj.ok) << obj.CreatePrettyFormattedStringButVerySlow();
    ```

## Debugging Support

Special debug mode logging macros only have an effect in debug mode and are
compiled away to nothing for non-debug mode compiles. Use these macros to avoid
slowing down your production application due to excessive logging.

``` cpp
DLOG(INFO) << "Found cookies";
DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
DLOG_EVERY_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";
DLOG_FIRST_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";
DLOG_EVERY_T(INFO, 0.01) << "Got a cookie";
```

## Runtime Checks

It is a good practice to check expected conditions in your program
frequently to detect errors as early as possible. The `CHECK` macro
provides the ability to abort the application when a condition is not met,
similar to the `assert` macro defined in the standard C library.

`CHECK` aborts the application if a condition is not true. Unlike
`assert`, it is **not** controlled by `NDEBUG`, so the check will be executed
regardless of compilation mode. Therefore, `fp->Write(x)` in the following
example is always executed:

``` cpp
CHECK(fp->Write(x) == 4) << "Write failed!";
```

There are various helper macros for equality/inequality checks
-`CHECK_EQ`, `CHECK_NE`, `CHECK_LE`, `CHECK_LT`, `CHECK_GE`, and
`CHECK_GT`. They compare two values, and log a `FATAL` message including the two
values when the result is not as expected. The values must have
`#!cpp operator<<(ostream, ...)` defined.

You may append to the error message like so:

``` cpp
CHECK_NE(1, 2) << ": The world must be ending!";
```

We are very careful to ensure that each argument is evaluated exactly
once, and that anything which is legal to pass as a function argument is legal
here. In particular, the arguments may be temporary expressions which will end
up being destroyed at the end of the apparent statement, for example:

``` cpp
CHECK_EQ(string("abc")[1], ’b’);
```

The compiler reports an error if one of the arguments is a pointer and the other
is `#!cpp nullptr`. To work around this, simply `#!cpp static_cast` `#!cpp
nullptr` to the type of the desired pointer.

``` cpp
CHECK_EQ(some_ptr, static_cast<SomeType*>(nullptr));
```

Better yet, use the `CHECK_NOTNULL` macro:

``` cpp
CHECK_NOTNULL(some_ptr);
some_ptr->DoSomething();
```

Since this macro returns the given pointer, this is very useful in
constructor initializer lists.

``` cpp
struct S {
    S(Something* ptr) : ptr_(CHECK_NOTNULL(ptr)) {}
    Something* ptr_;
};
```

!!! warning
    Due to the argument forwarding, `CHECK_NOTNULL` cannot be used to
    simultaneously stream an additional custom message. To provide a custom
    message, one can use the macro `CHECK_EQ` prior to the failing check.

If you are comparing C strings (`#!cpp char *`), a handy set of macros performs
both case sensitive and insensitive comparisons - `CHECK_STREQ`, `CHECK_STRNE`,
`CHECK_STRCASEEQ`, and `CHECK_STRCASENE`. The `CHECK_*CASE*` macro variants are
case-insensitive. You can safely pass `#!cpp nullptr` pointers to this macro.
They treat `#!cpp nullptr` and any non-`#!cpp nullptr` string as not equal. Two
`#!cpp nullptr`s are equal.

!!! note
    Both arguments may be temporary objects which are destructed at the
    end of the current *full expression*, such as

    ``` cpp
    CHECK_STREQ(Foo().c_str(), Bar().c_str());
    ```

    where `Foo` and `Bar` return `std::string`.

The `CHECK_DOUBLE_EQ` macro checks the equality of two floating point values,
accepting a small error margin. `CHECK_NEAR` accepts a third floating point
argument, which specifies the acceptable error margin.


## Raw Logging

The header file `<glog/raw_logging.h>` can be used for thread-safe logging,
which does not allocate any memory or acquire any locks. Therefore, the macros
defined in this header file can be used by low-level memory allocation and
synchronization code. Please check
[src/glog/raw_logging.h](https://github.com/google/glog/blob/master/src/glog/raw_logging.h)
for detail.

## Google Style `perror()`

`PLOG()` and `PLOG_IF()` and `PCHECK()` behave exactly like their `LOG*` and
`CHECK` equivalents with the addition that they append a description of the
current state of `errno` to their output lines. E.g.

``` cpp
PCHECK(write(1, nullptr, 2) >= 0) << "Write nullptr failed";
```

This check fails with the following error message.

    F0825 185142 test.cc:22] Check failed: write(1, nullptr, 2) >= 0 Write nullptr failed: Bad address [14]

## Syslog

`SYSLOG`, `SYSLOG_IF`, and `SYSLOG_EVERY_N` macros are available. These log to
syslog in addition to the normal logs. Be aware that logging to syslog can
drastically impact performance, especially if syslog is configured for remote
logging! Make sure you understand the implications of outputting to syslog
before you use these macros. In general, it's wise to use these macros
sparingly.
