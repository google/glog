Google Logging Library
======================

|Linux Github actions| |Windows Github actions| |macOS Github actions| |Codecov|

Google Logging (glog) is a C++14 library that implements application-level
logging. The library provides logging APIs based on C++-style streams and
various helper macros.

.. role:: cmake(code)
   :language: cmake

.. role:: cmd(code)
   :language: bash

.. role:: cpp(code)
   :language: cpp

.. role:: bazel(code)
   :language: starlark


Getting Started
---------------

You can log a message by simply streaming things to ``LOG``\ (<a
particular `severity level <#severity-levels>`__>), e.g.,

.. code:: cpp

   #include <glog/logging.h>

   int main(int argc, char* argv[]) {
       // Initialize Google’s logging library.
       google::InitGoogleLogging(argv[0]);

       // ...
       LOG(INFO) << "Found " << num_cookies << " cookies";
   }


The library can be installed using various package managers or compiled from
`source <#building-from-source>`__. For a detailed overview of glog features and
their usage, please refer to the `user guide <#user-guide>`__.

.. pull-quote::
   [!IMPORTANT]

   The above example requires further `Bazel <#bazel>`__ or
   `CMake <#usage-in-projects>`__ setup for use in own projects.


.. contents:: Table of Contents


Usage in Projects
~~~~~~~~~~~~~~~~~

Assuming that glog was previously `built using CMake <#cmake>`__ or installed
using a package manager, you can use the CMake command :cmake:`find_package` to
build against glog in your CMake project as follows:

.. code:: cmake

   cmake_minimum_required (VERSION 3.16)
   project (myproj VERSION 1.0)

   find_package (glog 0.7.0 REQUIRED)

   add_executable (myapp main.cpp)
   target_link_libraries (myapp glog::glog)


Compile definitions and options will be added automatically to your
target as needed.

Alternatively, glog can be incorporated into using the CMake command
:cmake:`add_subdirectory` to include glog directly from a subdirectory of your
project by replacing the :cmake:`find_package` call from the previous snippet by
:cmake:`add_subdirectory`. The :cmake:`glog::glog` target is in this case an
:cmake:`ALIAS` library target for the ``glog`` library target.

Building from Source
~~~~~~~~~~~~~~~~~~~~

Bazel
^^^^^

To use glog within a project which uses the
`Bazel <https://bazel.build/>`__ build tool, add the following lines to
your ``WORKSPACE`` file:

.. code:: bazel

   load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

   http_archive(
       name = "com_github_gflags_gflags",
       sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
       strip_prefix = "gflags-2.2.2",
       urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
   )

   http_archive(
       name = "com_github_google_glog",
       sha256 = "122fb6b712808ef43fbf80f75c52a21c9760683dae470154f02bddfc61135022",
       strip_prefix = "glog-0.6.0",
       urls = ["https://github.com/google/glog/archive/v0.6.0.zip"],
   )

You can then add :bazel:`@com_github_google_glog//:glog` to the deps section
of a :bazel:`cc_binary` or :bazel:`cc_library` rule, and :code:`#include <glog/logging.h>`
to include it in your source code. Here’s a simple example:

.. code:: bazel

   cc_binary(
       name = "main",
       srcs = ["main.cc"],
       deps = ["@com_github_google_glog//:glog"],
   )

CMake
^^^^^

glog can be compiled using `CMake <http://www.cmake.org>`__ on a wide range of
platforms. The typical workflow for building glog  on a Unix-like system with
GNU Make as build tool is as follows:

1. Clone the repository and change into source directory.

  .. code:: bash

     git clone https://github.com/google/glog.git
     cd glog

2. Run CMake to configure the build tree.

  .. code:: bash

     cmake -S . -B build -G "Unix Makefiles"

  CMake provides different generators, and by default will pick the most
  relevant one to your environment. If you need a specific version of Visual
  Studio, use :cmd:`cmake . -G <generator-name>`, and see :cmd:`cmake --help`
  for the available generators. Also see :cmd:`-T <toolset-name>`, which can
  be used to request the native x64 toolchain with :cmd:`-T host=x64`.

3. Afterwards, generated files can be used to compile the project.

  .. code:: bash

     cmake --build build

4. Test the build software (optional).

  .. code:: bash

     cmake --build build --target test

5. Install the built files (optional).

  .. code:: bash

     cmake --build build --target install


Once successfully built, glog can be
`integrated into own projects <#usage-in-projects>`__.


conan
~~~~~

You can download and install glog using the `conan
<https://conan.io>`__ package manager:

.. code:: bash

   pip install conan
   conan install -r conancenter glog/<glog-version>@

The glog recipe in conan center is kept up to date by conan center index community
contributors. If the version is out of date, please create an
issue or pull request on the `conan-center-index
<https://github.com/conan-io/conan-center-index>`__ repository.

vcpkg
~~~~~

You can download and install glog using the `vcpkg
<https://github.com/Microsoft/vcpkg>`__ dependency manager:

.. code:: bash

   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh
   ./vcpkg integrate install
   ./vcpkg install glog

The glog port in vcpkg is kept up to date by Microsoft team members and
community contributors. If the version is out of date, please create an
issue or pull request on the vcpkg repository.

User Guide
----------

glog defines a series of macros that simplify many common logging tasks.
You can log messages by severity level, control logging behavior from
the command line, log based on conditionals, abort the program when
expected conditions are not met, introduce your own verbose logging
levels, customize the prefix attached to log messages, and more.

Following sections describe the functionality supported by glog. Please note
this description may not be complete but limited to the most useful ones. If you
want to find less common features, please check header files under `src/glog
<src/glog>`__ directory.

Severity Levels
~~~~~~~~~~~~~~~

You can specify one of the following severity levels (in increasing
order of severity):

1. ``INFO``,
2. ``WARNING``,
3. ``ERROR``, and
4. ``FATAL``.

Logging a ``FATAL`` message terminates the program (after the message is
logged).

.. pull-quote::
   [!NOTE]

   Messages of a given severity are logged not only to corresponding severity
   logfile but also to other logfiles of lower severity. For instance, a message
   of severity ``FATAL`` will be logged to logfiles of severity ``FATAL``,
   ``ERROR``, ``WARNING``, and ``INFO``.

The ``DFATAL`` severity logs a ``FATAL`` error in debug mode (i.e.,
there is no ``NDEBUG`` macro defined), but avoids halting the program in
production by automatically reducing the severity to ``ERROR``.

Unless otherwise specified, glog uses the format

::

    <tmp>/<program name>.<hostname>.<user name>.log.<severity level>.<date>-<time>.<pid>

for log filenames written to a directory designated as ``<tmp>`` and determined
according to the following rules.

**Windows**
    glog uses the
    `GetTempPathA <https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-gettemppatha>`__
    API function to retrieve the directory for temporary files with a fallback to

    1. ``C:\TMP\``
    2. ``C:\TEMP\``

    (in the order given.)

**non-Windows**
    The directory is determined by referencing the environment variables

    1. ``TMPDIR``
    2. ``TMP``

    if set with a fallback to ``/tmp/``.

The default path to a log file on Linux, for instance, could be

::

    /tmp/hello_world.example.com.hamaji.log.INFO.20080709-222411.10474

By default, glog echos ``ERROR`` and ``FATAL`` messages to standard error in
addition to log files.


Log Line Prefix Format
~~~~~~~~~~~~~~~~~~~~~~

Log lines have this form:

::

    Lyyyymmdd hh:mm:ss.uuuuuu threadid file:line] msg...

where the fields are defined as follows:

==================== =========================================================================
Placeholder          Meaning
==================== =========================================================================
``L``                A single character, representing the log level (e.g., ``I`` for ``INFO``)
``yyyy``             The year
``mm``               The month (zero padded; i.e., May is ``05``)
``dd``               The day (zero padded)
``hh:mm:ss.uuuuuu``  Time in hours, minutes and fractional seconds
``threadid``         The space-padded thread ID
``file``             The file name
``line``             The line number
``msg``              The user-supplied message
==================== =========================================================================

Example:

::

  I1103 11:57:31.739339 24395 google.cc:2341] Command line: ./some_prog
  I1103 11:57:31.739403 24395 google.cc:2342] Process id 24395

.. pull-quote::
   [!NOTE]

   Although microseconds are useful for comparing events on a single machine,
   clocks on different machines may not be well synchronized. Hence, use with
   caution when comparing the low bits of timestamps from different machines.


Customizing the Log Line Prefix
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The predefined log line prefix can be replaced using a user-provided callback
that formats the corresponding output.

For each log entry, the callback will be invoked with a reference to a
``google::LogMessage`` instance containing the severity, filename, line number,
thread ID, and time of the event. It will also be given a reference to the
output stream, whose contents will be prepended to the actual message in the
final log line.

For example, the following function outputs a prefix that matches glog's default
format. The third parameter ``data`` can be used to access user-supplied data
which unless specified defaults to :cpp:`nullptr`.

.. code:: cpp

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


To enable the use of a prefix formatter, use the

.. code:: cpp

    google::InstallPrefixFormatter(&MyPrefixFormatter);

function to pass a pointer to the corresponding :cpp:`MyPrefixFormatter`
callback during initialization. :cpp:`InstallPrefixFormatter` takes a second
optional argument of type  :cpp:`void*` that allows supplying user data to the
callback.


Setting Flags
~~~~~~~~~~~~~

Several flags influence glog’s output behavior. If the `Google gflags library
<https://github.com/gflags/gflags>`__ is installed on your machine, the build
system will automatically detect and use it, allowing you to pass flags on the
command line. For example, if you want to activate :cmd:`--logtostderr`, you can
start your application with the following command line:

.. code:: bash

   ./your_application --logtostderr=1

If the Google gflags library isn’t installed, you set flags via
environment variables, prefixing the flag name with ``GLOG_``, e.g.,

.. code:: bash

   GLOG_logtostderr=1 ./your_application

The following flags are most commonly used:

``logtostderr`` (``bool``, default=\ ``false``)
   Log messages to ``stderr`` instead of logfiles.

   .. pull-quote::
      [!TIP]

      You can set boolean flags to :cpp:`true` by specifying ``1``, :cpp:`true`,
      or ``yes``. To set boolean flags to :cpp:`false`, specify ``0``,
      ``false``, or ``no``. In either case the spelling is case-insensitive.

``stderrthreshold`` (``int``, default=2, which is ``ERROR``)
   Copy log messages at or above this level to stderr in addition to
   logfiles. The numbers of severity levels ``INFO``, ``WARNING``,
   ``ERROR``, and ``FATAL`` are 0, 1, 2, and 3, respectively.

``minloglevel`` (``int``, default=0, which is ``INFO``)
   Log messages at or above this level. Again, the numbers of severity
   levels ``INFO``, ``WARNING``, ``ERROR``, and ``FATAL`` are 0, 1, 2,
   and 3, respectively.

``log_dir`` (``string``, default="")
   If specified, logfiles are written into this directory instead of the
   default logging directory.

``v`` (``int``, default=0)
   Show all ``VLOG(m)`` messages for ``m`` less or equal the value of this flag.
   Overridable by :cmd:`--vmodule`. Refer to `verbose logging <#verbose-logging>`__
   for more detail.

``vmodule`` (``string``, default="")
   Per-module verbose level. The argument has to contain a
   comma-separated list of ``<module name>=<log level>``. ``<module name>`` is a
   glob pattern (e.g., ``gfs*`` for all modules whose name starts with
   "gfs"), matched against the filename base (that is, name ignoring
   .cc/.h./-inl.h). ``<log level>`` overrides any value given by :cmd:`--v`.
   See also `verbose logging <#verbose-logging>`__ for more details.

Additional flags are defined in `flags.cc <src/flags.cc>`__. Please see the
source for their complete list.

You can also modify flag values in your program by modifying global
variables ``FLAGS_*`` . Most settings start working immediately after
you update ``FLAGS_*`` . The exceptions are the flags related to
destination files. For example, you might want to set ``FLAGS_log_dir``
before calling :cpp:`google::InitGoogleLogging` . Here is an example:

.. code:: cpp

   LOG(INFO) << "file";
   // Most flags work immediately after updating values.
   FLAGS_logtostderr = 1;
   LOG(INFO) << "stderr";
   FLAGS_logtostderr = 0;
   // This won’t change the log destination. If you want to set this
   // value, you should do this before google::InitGoogleLogging .
   FLAGS_log_dir = "/some/log/directory";
   LOG(INFO) << "the same file";

Conditional / Occasional Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sometimes, you may only want to log a message under certain conditions.
You can use the following macros to perform conditional logging:

.. code:: cpp

   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";

The "Got lots of cookies" message is logged only when the variable
``num_cookies`` exceeds 10. If a line of code is executed many times, it
may be useful to only log a message at certain intervals. This kind of
logging is most useful for informational messages.

.. code:: cpp

   LOG_EVERY_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";

The above line outputs a log messages on the 1st, 11th, 21st, ... times
it is executed.

.. pull-quote::
   [!NOTE]

   The placeholder ``google::COUNTER`` identifies the recurring repetition.

You can combine conditional and occasional logging with the following
macro.

.. code:: cpp

   LOG_IF_EVERY_N(INFO, (size > 1024), 10) << "Got the " << google::COUNTER
                                           << "th big cookie";

Instead of outputting a message every nth time, you can also limit the
output to the first n occurrences:

.. code:: cpp

   LOG_FIRST_N(INFO, 20) << "Got the " << google::COUNTER << "th cookie";

Outputs log messages for the first 20 times it is executed. The
``google::COUNTER`` identifier indicates which repetition is happening.

Other times, it is desired to only log a message periodically based on a time.
For instance, to log a message every 10ms:

.. code:: cpp

   LOG_EVERY_T(INFO, 0.01) << "Got a cookie";

Or every 2.35s:

.. code:: cpp

   LOG_EVERY_T(INFO, 2.35) << "Got a cookie";

Debug Mode Support
~~~~~~~~~~~~~~~~~~

Special "debug mode" logging macros only have an effect in debug mode
and are compiled away to nothing for non-debug mode compiles. Use these
macros to avoid slowing down your production application due to
excessive logging.

.. code:: cpp

   DLOG(INFO) << "Found cookies";
   DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
   DLOG_EVERY_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";
   DLOG_FIRST_N(INFO, 10) << "Got the " << google::COUNTER << "th cookie";
   DLOG_EVERY_T(INFO, 0.01) << "Got a cookie";


``CHECK`` Macros
~~~~~~~~~~~~~~~~

It is a good practice to check expected conditions in your program
frequently to detect errors as early as possible. The ``CHECK`` macro
provides the ability to abort the application when a condition is not
met, similar to the ``assert`` macro defined in the standard C library.

``CHECK`` aborts the application if a condition is not true. Unlike
``assert``, it is \*not\* controlled by ``NDEBUG``, so the check will be
executed regardless of compilation mode. Therefore, ``fp->Write(x)`` in
the following example is always executed:

.. code:: cpp

   CHECK(fp->Write(x) == 4) << "Write failed!";

There are various helper macros for equality/inequality checks -
``CHECK_EQ``, ``CHECK_NE``, ``CHECK_LE``, ``CHECK_LT``, ``CHECK_GE``,
and ``CHECK_GT``. They compare two values, and log a ``FATAL`` message
including the two values when the result is not as expected. The values
must have :cpp:`operator<<(ostream, ...)` defined.

You may append to the error message like so:

.. code:: cpp

   CHECK_NE(1, 2) << ": The world must be ending!";

We are very careful to ensure that each argument is evaluated exactly
once, and that anything which is legal to pass as a function argument is
legal here. In particular, the arguments may be temporary expressions
which will end up being destroyed at the end of the apparent statement,
for example:

.. code:: cpp

   CHECK_EQ(string("abc")[1], ’b’);

The compiler reports an error if one of the arguments is a pointer and the other
is :cpp:`nullptr`. To work around this, simply :cpp:`static_cast` :cpp:`nullptr` to
the type of the desired pointer.

.. code:: cpp

   CHECK_EQ(some_ptr, static_cast<SomeType*>(nullptr));

Better yet, use the ``CHECK_NOTNULL`` macro:

.. code:: cpp

   CHECK_NOTNULL(some_ptr);
   some_ptr->DoSomething();

Since this macro returns the given pointer, this is very useful in
constructor initializer lists.

.. code:: cpp

   struct S {
       S(Something* ptr) : ptr_(CHECK_NOTNULL(ptr)) {}
       Something* ptr_;
   };


.. pull-quote::
   [!WARNING]

   Due to the argument forwarding, ``CHECK_NOTNULL`` cannot be used to
   simultaneously stream an additional custom message. To provide a custom
   message, one can use the macro ``CHECK_EQ`` prior to the failing check.

If you are comparing C strings (:cpp:`char *`), a handy set of macros performs
both case sensitive and insensitive comparisons - ``CHECK_STREQ``,
``CHECK_STRNE``, ``CHECK_STRCASEEQ``, and ``CHECK_STRCASENE``. The
``CHECK_*CASE*`` macro variants are case-insensitive. You can safely pass
:cpp:`nullptr` pointers to this macro. They treat :cpp:`nullptr` and any
non-:cpp:`nullptr` string as not equal. Two :cpp:`nullptr`\ s are equal.

.. pull-quote::
   [!NOTE]

   Both arguments may be temporary objects which are destructed at the end of
   the current "full expression", such as

   .. code:: cpp

      CHECK_STREQ(Foo().c_str(), Bar().c_str());

   where ``Foo`` and ``Bar`` return :cpp:`std::string`.

The ``CHECK_DOUBLE_EQ`` macro checks the equality of two floating point
values, accepting a small error margin. ``CHECK_NEAR`` accepts a third
floating point argument, which specifies the acceptable error margin.

Verbose Logging
~~~~~~~~~~~~~~~

When you are chasing difficult bugs, thorough log messages are very useful.
However, you may want to ignore too verbose messages in usual development. For
such verbose logging, glog provides the ``VLOG`` macro, which allows you to
define your own numeric logging levels. The :cmd:`--v` command line option
controls which verbose messages are logged:

.. code:: cpp

   VLOG(1) << "I’m printed when you run the program with --v=1 or higher";
   VLOG(2) << "I’m printed when you run the program with --v=2 or higher";

With ``VLOG``, the lower the verbose level, the more likely messages are to be
logged. For example, if :cmd:`--v==1`, ``VLOG(1)`` will log, but ``VLOG(2)``
will not log.

.. pull-quote::
   [!CAUTION]

   The ``VLOG`` behavior is opposite of the severity level logging, where
   ``INFO``, ``ERROR``, etc. are defined in increasing order and thus
   :cmd:`--minloglevel` of 1 will only log ``WARNING`` and above.

Though you can specify any integers for both ``VLOG`` macro and :cmd:`--v` flag,
the common values for them are small positive integers. For example, if you
write ``VLOG(0)``, you should specify :cmd:`--v=-1` or lower to silence it. This
is less useful since we may not want verbose logs by default in most cases. The
``VLOG`` macros always log at the ``INFO`` log level (when they log at all).

Verbose logging can be controlled from the command line on a per-module
basis:

.. code:: bash

   --vmodule=mapreduce=2,file=1,gfs*=3 --v=0

Specifying these options will specficially:

1. Print ``VLOG(2)`` and lower messages from mapreduce.{h,cc}
2. Print ``VLOG(1)`` and lower messages from file.{h,cc}
3. Print ``VLOG(3)`` and lower messages from files prefixed with "gfs"
4. Print ``VLOG(0)`` and lower messages from elsewhere

The wildcarding functionality 3. supports both ``*`` (matches 0
or more characters) and ``?`` (matches any single character) wildcards.
Please also refer to `command line flags <#setting-flags>`__ for more
information.

There’s also ``VLOG_IS_ON(n)`` "verbose level" condition macro. This macro
returns ``true`` when the :cmd:`--v` is equal to or greater than ``n``. The
macro can be used as follows:

.. code:: cpp

   if (VLOG_IS_ON(2)) {
       // do some logging preparation and logging
       // that can’t be accomplished with just VLOG(2) << ...;
   }

Verbose level condition macros ``VLOG_IF``, ``VLOG_EVERY_N`` and
``VLOG_IF_EVERY_N`` behave analogous to ``LOG_IF``, ``LOG_EVERY_N``,
``LOG_IF_EVERY_N``, but accept a numeric verbosity level as opposed to a
severity level.

.. code:: cpp

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



Failure Signal Handler
~~~~~~~~~~~~~~~~~~~~~~

The library provides a convenient signal handler that will dump useful
information when the program crashes on certain signals such as ``SIGSEGV``. The
signal handler can be installed by :cpp:`google::InstallFailureSignalHandler()`.
The following is an example of output from the signal handler.

::

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

By default, the signal handler writes the failure dump to the standard
error. You can customize the destination by :cpp:`InstallFailureWriter()`.

Performance of Messages
~~~~~~~~~~~~~~~~~~~~~~~

The conditional logging macros provided by glog (e.g., ``CHECK``,
``LOG_IF``, ``VLOG``, etc.) are carefully implemented and don’t execute
the right hand side expressions when the conditions are false. So, the
following check may not sacrifice the performance of your application.

.. code:: cpp

   CHECK(obj.ok) << obj.CreatePrettyFormattedStringButVerySlow();

User-defined Failure Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FATAL`` severity level messages or unsatisfied ``CHECK`` condition
terminate your program. You can change the behavior of the termination
by :cpp:`InstallFailureFunction`.

.. code:: cpp

   void YourFailureFunction() {
     // Reports something...
     exit(EXIT_FAILURE);
   }

   int main(int argc, char* argv[]) {
     google::InstallFailureFunction(&YourFailureFunction);
   }

By default, glog tries to dump the stacktrace and calls :cpp:`std::abort`. The
stacktrace is generated only when running the application on a system supported
by glog. Currently, glog supports x86, x86_64, PowerPC architectures,
``libunwind``, and the Debug Help Library (``dbghelp``) on Windows for
extracting the stack trace.


Raw Logging
~~~~~~~~~~~

The header file ``<glog/raw_logging.h>`` can be used for thread-safe logging,
which does not allocate any memory or acquire any locks. Therefore, the macros
defined in this header file can be used by low-level memory allocation and
synchronization code. Please check
`src/glog/raw_logging.h <src/glog/raw_logging.h>`__ for detail.

Google Style ``perror()``
~~~~~~~~~~~~~~~~~~~~~~~~~

``PLOG()`` and ``PLOG_IF()`` and ``PCHECK()`` behave exactly like their
``LOG*`` and ``CHECK`` equivalents with the addition that they append a
description of the current state of errno to their output lines. E.g.

.. code:: cpp

   PCHECK(write(1, nullptr, 2) >= 0) << "Write nullptr failed";

This check fails with the following error message.

::

   F0825 185142 test.cc:22] Check failed: write(1, nullptr, 2) >= 0 Write nullptr failed: Bad address [14]

Syslog
~~~~~~

``SYSLOG``, ``SYSLOG_IF``, and ``SYSLOG_EVERY_N`` macros are available.
These log to syslog in addition to the normal logs. Be aware that
logging to syslog can drastically impact performance, especially if
syslog is configured for remote logging! Make sure you understand the
implications of outputting to syslog before you use these macros. In
general, it’s wise to use these macros sparingly.

Strip Logging Messages
~~~~~~~~~~~~~~~~~~~~~~

Strings used in log messages can increase the size of your binary and
present a privacy concern. You can therefore instruct glog to remove all
strings which fall below a certain severity level by using the
``GOOGLE_STRIP_LOG`` macro:

If your application has code like this:

.. code:: cpp

   #define GOOGLE_STRIP_LOG 1    // this must go before the #include!
   #include <glog/logging.h>

The compiler will remove the log messages whose severities are less than
the specified integer value. Since ``VLOG`` logs at the severity level
``INFO`` (numeric value ``0``), setting ``GOOGLE_STRIP_LOG`` to 1 or
greater removes all log messages associated with ``VLOG``\ s as well as
``INFO`` log statements.

Automatically Remove Old Logs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable the log cleaner:

.. code:: cpp

   using namespace std::chrono_literals;
   google::EnableLogCleaner(24h * 3); // keep your logs for 3 days


In C++20 (and later) this can be shortened to:

.. code:: cpp

   using namespace std::chrono_literals;
   google::EnableLogCleaner(3d); // keep your logs for 3 days

And then glog will check if there are overdue logs whenever a flush is
performed. In this example, any log file from your project whose last
modified time is greater than 3 days will be unlink()ed.

This feature can be disabled at any time (if it has been enabled)

.. code:: cpp

   google::DisableLogCleaner();

Notes for Windows Users
~~~~~~~~~~~~~~~~~~~~~~~

glog defines a severity level ``ERROR``, which is also defined in
``windows.h`` . You can make glog not define ``INFO``, ``WARNING``,
``ERROR``, and ``FATAL`` by defining ``GLOG_NO_ABBREVIATED_SEVERITIES``
before including ``glog/logging.h`` . Even with this macro, you can
still use the iostream like logging facilities:

.. code:: cpp

   #define GLOG_NO_ABBREVIATED_SEVERITIES
   #include <windows.h>
   #include <glog/logging.h>

   // ...

   LOG(ERROR) << "This should work";
   LOG_IF(ERROR, x > y) << "This should be also OK";

However, you cannot use ``INFO``, ``WARNING``, ``ERROR``, and ``FATAL``
anymore for functions defined in ``glog/logging.h`` .

.. code:: cpp

   #define GLOG_NO_ABBREVIATED_SEVERITIES
   #include <windows.h>
   #include <glog/logging.h>

   // ...

   // This won’t work.
   // google::FlushLogFiles(google::ERROR);

   // Use this instead.
   google::FlushLogFiles(google::GLOG_ERROR);

If you don’t need ``ERROR`` defined by ``windows.h``, there are a couple
of more workarounds which sometimes don’t work:

-  :cpp:`#define WIN32_LEAN_AND_MEAN` or :cpp:`NOGDI` **before**
   :cpp:`#include <windows.h>`.
-  :cpp:`#undef ERROR` **after** :cpp:`#include <windows.h>`.

See `this
issue <http://code.google.com/p/google-glog/issues/detail?id=33>`__ for
more detail.


Installation Notes for 64-bit Linux Systems
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The glibc built-in stack-unwinder on 64-bit systems has some problems with glog.
(In particular, if you are using :cpp:`InstallFailureSignalHandler()`, the
signal may be raised in the middle of malloc, holding some malloc-related locks
when they invoke the stack unwinder. The built-in stack unwinder may call malloc
recursively, which may require the thread to acquire a lock it already holds:
deadlock.)

For that reason, if you use a 64-bit system and you need
:cpp:`InstallFailureSignalHandler()`, we strongly recommend you install
``libunwind`` before trying to configure or install google glog.
libunwind can be found
`here <http://download.savannah.nongnu.org/releases/libunwind/libunwind-snap-070410.tar.gz>`__.

Even if you already have ``libunwind`` installed, you will probably
still need to install from the snapshot to get the latest version.

Caution: if you install libunwind from the URL above, be aware that you
may have trouble if you try to statically link your binary with glog:
that is, if you link with ``gcc -static -lgcc_eh ...``. This is because
both ``libunwind`` and ``libgcc`` implement the same C++ exception
handling APIs, but they implement them differently on some platforms.
This is not likely to be a problem on ia64, but may be on x86-64.

Also, if you link binaries statically, make sure that you add
:cmd:`-Wl,--eh-frame-hdr` to your linker options. This is required so that
``libunwind`` can find the information generated by the compiler required for
stack unwinding.

Using :cmd:`-static` is rare, though, so unless you know this will affect you it
probably won’t.

If you cannot or do not wish to install libunwind, you can still try to
use two kinds of stack-unwinder:

glibc built-in stack-unwinder
    As we already mentioned, glibc’s unwinder has a deadlock issue. However, if
    you don’t use :cpp:`InstallFailureSignalHandler()` or you don’t worry about
    the rare possibilities of deadlocks, you can use this stack-unwinder. If you
    specify no options and ``libunwind`` isn’t detected on your system, the
    configure script chooses this unwinder by default.

frame pointer based stack-unwinder
    The frame pointer based stack unwinder requires that your application, the
    glog library, and system libraries like libc, all be compiled with a frame
    pointer. This is *not* the default for x86-64.


How to Contribute
-----------------

We’d love to accept your patches and contributions to this project.
There are a just a few small guidelines you need to follow.

Contributor License Agreement (CLA)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Contributions to any Google project must be accompanied by a Contributor
License Agreement. This is not a copyright **assignment**, it simply
gives Google permission to use and redistribute your contributions as
part of the project.

* If you are an individual writing original source code and you’re sure
  you own the intellectual property, then you’ll need to sign an
  `individual
  CLA <https://developers.google.com/open-source/cla/individual>`__.
* If you work for a company that wants to allow you to contribute your
  work, then you’ll need to sign a `corporate
  CLA <https://developers.google.com/open-source/cla/corporate>`__.

You generally only need to submit a CLA once, so if you’ve already
submitted one (even if it was for a different project), you probably
don’t need to do it again.

Once your CLA is submitted (or if you already submitted one for another
Google project), make a commit adding yourself to the
`AUTHORS <./AUTHORS>`__ and `CONTRIBUTORS <./CONTRIBUTORS>`__ files. This
commit can be part of your first `pull
request <https://help.github.com/articles/creating-a-pull-request>`__.

Submitting a Patch
~~~~~~~~~~~~~~~~~~

1. It’s generally best to start by opening a new issue describing the
   bug or feature you’re intending to fix. Even if you think it’s
   relatively minor, it’s helpful to know what people are working on.
   Mention in the initial issue that you are planning to work on that
   bug or feature so that it can be assigned to you.
2. Follow the normal process of
   `forking <https://help.github.com/articles/fork-a-repo>`__ the
   project, and setup a new branch to work in. It’s important that each
   group of changes be done in separate branches in order to ensure that
   a pull request only includes the commits related to that bug or
   feature.
3. Do your best to have `well-formed commit
   messages <http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html>`__
   for each change. This provides consistency throughout the project,
   and ensures that commit messages are able to be formatted properly by
   various git tools.
4. Finally, push the commits to your fork and submit a `pull
   request <https://help.github.com/articles/creating-a-pull-request>`__.


.. |Linux Github actions| image:: https://github.com/google/glog/actions/workflows/linux.yml/badge.svg
   :target: https://github.com/google/glog/actions
.. |Windows Github actions| image:: https://github.com/google/glog/actions/workflows/windows.yml/badge.svg
   :target: https://github.com/google/glog/actions
.. |macOS Github actions| image:: https://github.com/google/glog/actions/workflows/macos.yml/badge.svg
   :target: https://github.com/google/glog/actions
.. |Codecov| image:: https://codecov.io/gh/google/glog/branch/master/graph/badge.svg?token=8an420vNju
   :target: https://codecov.io/gh/google/glog
