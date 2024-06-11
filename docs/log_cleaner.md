# Automatically Remove Old Logs

To enable the log cleaner:

``` cpp
using namespace std::chrono_literals;
google::EnableLogCleaner(24h * 3); // keep your logs for 3 days
```

In C++20 (and later) this can be shortened to:

``` cpp
using namespace std::chrono_literals;
google::EnableLogCleaner(3d); // keep your logs for 3 days
```

And then glog will check if there are overdue logs whenever a flush is
performed. In this example, any log file from your project whose last
modified time is greater than 3 days will be `unlink`()ed.

This feature can be disabled at any time (if it has been enabled) using
``` cpp
google::DisableLogCleaner();
```
