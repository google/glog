# Custom Sinks

Under certain circumstances, it is useful to send the log output to a
destination other than a file, `stderr` and/or `stdout`. In case, the library
provides the `#!cpp google::LogSink` interface whose implementations can be used
to write the log output to arbitrary locations.

## Basic Interface

The sink interface is defined as follows:

``` cpp
class LogSink {
 public:
  virtual void send(LogSeverity severity, const char* full_filename,
                    const char* base_filename, int line,
                    const LogMessageTime& time, const char* message,
                    size_t message_len);
};
```

The user must implement `#!cpp google::LogSink::send`, which is called by the
library every time a message is logged.

!!! warning "Possible deadlock due to nested logging"
    This method can't use `LOG()` or `CHECK()` as logging system mutex(s) are
    held during this call.

## Registering Log Sinks

To use the custom sink and instance of the above interface implementation must
be registered using `google::AddLogSink` which expects a pointer to the
`google::LogSink` instance. To unregister use `google::RemoveLogSink`. Both
functions are thread-safe.

!!! danger "`LogSink` ownership"
    The `google::LogSink` instance must not be destroyed until the referencing
    pointer is unregistered.

## Direct Logging

Instead of registering the sink, we can directly use to log messages. While `#!
LOG_TO_SINK(sink, severity)` allows to log both to the sink and to a global log
registry, e.g., a file, `#!cpp LOG_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)`
will avoid the latter.

!!! example "Using a custom sink"
    ``` cpp title="custom_sink.cc"
    -8<- "examples/custom_sink.cc:33:"
    ```

    1. `MySink` implements a custom sink that sends log messages to `std::cout`.
    2. The custom sink must be registered to for use with existing logging
       macros.
    3. Once the custom sink is no longer needed we remove it from the registry.
    4. A sink does not need to be registered globally. However, then, messages
       must be logged using dedicated macros.

    Running the above example as `#!bash GLOG_log_dir=. ./custom_sink_example`
    will produce

    <div class="annotate" markdown>

    ``` title="Custom sink output"
    INFO custom_sink.cc:63 logging to MySink
    INFO custom_sink.cc:68 direct logging
    INFO custom_sink.cc:69 direct logging but not to file (1)
    ```

    </div>

    1. This line is not present in the log file because we used
       `LOG_TO_SINK_BUT_NOT_TO_LOGFILE` to log the message.

    and the corresponding log file will contain

    ``` title="Log file generated with the custom sink"
    Log file created at: 2024/06/11 13:24:27
    Running on machine: pc
    Running duration (h:mm:ss): 0:00:00
    Log line format: [IWEF]yyyymmdd hh:mm:ss.uuuuuu threadid file:line] msg
    I20240611 13:24:27.476620 126237946035776 custom_sink.cc:63] logging to MySink
    I20240611 13:24:27.476796 126237946035776 custom_sink.cc:68] direct logging
    ```
