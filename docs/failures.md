# Failure Signal Handler

## Stacktrace as Default Failure Handler

The library provides a convenient signal handler that will dump useful
information when the program crashes on certain signals such as `SIGSEGV`. The
signal handler can be installed by `#!cpp
google::InstallFailureSignalHandler()`. The following is an example of output
from the signal handler.

    *** Aborted at 1225095260 (unix time) try "date -d @1225095260" if you are using GNU date ***
    *** SIGSEGV (@0x0) received by PID 17711 (TID 0x7f893090a6f0) from PID 0; stack trace: ***
    PC: @           0x412eb1 TestWaitingLogSink::send()
        @     0x7f892fb417d0 (unknown)
        @           0x412eb1 TestWaitingLogSink::send()
        @     0x7f89304f7f06 google::LogMessage::SendToLog()
        @     0x7f89304f35af google::LogMessage::Flush()
        @     0x7f89304f3739 google::LogMessage::~LogMessage()
        @           0x408cf4 TestLogSinkWaitTillSent()
        @           0x4115de main
        @     0x7f892f7ef1c4 (unknown)
        @           0x4046f9 (unknown)


## Customizing Handler Output

By default, the signal handler writes the failure dump to the standard error.
However, it is possible to customize the destination by installing a callback
using the `#!cpp google::InstallFailureWriter()` function. The function expects
a pointer to a function with the following signature:

``` cpp
void YourFailureWriter(const char* message/* (1)! */, std::size_t length/* (2)! */);
```

1. The pointer references the start of the failure message.

    !!! danger
        The string is **not null-terminated**.

2. The message length in characters.

!!! warning "Possible overflow errors"
    Users should not expect the `message` string to be null-terminated.

## User-defined Failure Function

`FATAL` severity level messages or unsatisfied `CHECK` condition
terminate your program. You can change the behavior of the termination
by `google::InstallFailureFunction`.

``` cpp
void YourFailureFunction() {
  // Reports something...
  exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
  google::InstallFailureFunction(&YourFailureFunction);
}
```

By default, glog tries to dump the stacktrace and calls `#!cpp std::abort`. The
stacktrace is generated only when running the application on a system
supported[^1] by glog.

[^1]: To extract the stack trace, glog currently supports the following targets:

    * x86, x86_64,
    * PowerPC architectures,
    * `libunwind`,
    * and the Debug Help Library (`dbghelp`) on Windows.

