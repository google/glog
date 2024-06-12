# Notes for Windows Users

glog defines the severity level `ERROR`, which is also defined by `windows.h`.
You can make glog not define `INFO`, `WARNING`, `ERROR`, and `FATAL` by defining
`GLOG_NO_ABBREVIATED_SEVERITIES` before including `glog/logging.h`. Even with
this macro, you can still use the iostream like logging facilities:

``` cpp
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <windows.h>
#include <glog/logging.h>

// ...

LOG(ERROR) << "This should work";
LOG_IF(ERROR, x > y) << "This should be also OK";
```

However, you cannot use `INFO`, `WARNING`, `ERROR`, and `FATAL` anymore for
functions defined in `glog/logging.h`.

``` cpp
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <windows.h>
#include <glog/logging.h>

// ...

// This won’t work.
// google::FlushLogFiles(google::ERROR);

// Use this instead.
google::FlushLogFiles(google::GLOG_ERROR);
```

If you don't need `ERROR` defined by `windows.h`, there are a couple of more
workarounds which sometimes don't work[^1]:

-  `#!cpp #define WIN32_LEAN_AND_MEAN` or `NOGDI` **before**
   `#!cpp #include <windows.h>`.
-  `#!cpp #undef ERROR` **after** `#!cpp #include <windows.h>`.

[^1]: For more information refer to [this
      issue](http://code.google.com/p/google-glog/issues/detail?id=33).
