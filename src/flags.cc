// Copyright (c) 2024, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "glog/flags.h"

#include <cstdlib>
#include <cstring>

#include "base/commandlineflags.h"
#include "glog/log_severity.h"

namespace {

// Compute the default value for --log_dir
static const char* DefaultLogDir() {
  constexpr const char* const names[]{"GOOGLE_LOG_DIR", "TEST_TMPDIR"};
  for (const char* const name : names) {
    const char* const env = std::getenv(name);
    if (env != nullptr && env[0] != '\0') {
      return env;
    }
  }
  return "";
}

bool BoolFromEnv(const char* varname, bool defval) {
  const char* const valstr = getenv(varname);
  if (!valstr) {
    return defval;
  }
  return std::memchr("tTyY1\0", valstr[0], 6) != nullptr;
}

}  // namespace

GLOG_DEFINE_bool(timestamp_in_logfile_name,
                 BoolFromEnv("GOOGLE_TIMESTAMP_IN_LOGFILE_NAME", true),
                 "put a timestamp at the end of the log file name");
GLOG_DEFINE_bool(logtostderr, BoolFromEnv("GOOGLE_LOGTOSTDERR", false),
                 "log messages go to stderr instead of logfiles");
GLOG_DEFINE_bool(alsologtostderr, BoolFromEnv("GOOGLE_ALSOLOGTOSTDERR", false),
                 "log messages go to stderr in addition to logfiles");
GLOG_DEFINE_bool(colorlogtostderr, false,
                 "color messages logged to stderr (if supported by terminal)");
GLOG_DEFINE_bool(colorlogtostdout, false,
                 "color messages logged to stdout (if supported by terminal)");
GLOG_DEFINE_bool(logtostdout, BoolFromEnv("GOOGLE_LOGTOSTDOUT", false),
                 "log messages go to stdout instead of logfiles");
#ifdef GLOG_OS_LINUX
GLOG_DEFINE_bool(
    drop_log_memory, true,
    "Drop in-memory buffers of log contents. "
    "Logs can grow very quickly and they are rarely read before they "
    "need to be evicted from memory. Instead, drop them from memory "
    "as soon as they are flushed to disk.");
#endif

// By default, errors (including fatal errors) get logged to stderr as
// well as the file.
//
// The default is ERROR instead of FATAL so that users can see problems
// when they run a program without having to look in another file.
GLOG_DEFINE_int32(
    stderrthreshold, google::GLOG_ERROR,
    "log messages at or above this level are copied to stderr in "
    "addition to logfiles.  This flag obsoletes --alsologtostderr.");

GLOG_DEFINE_string(alsologtoemail, "",
                   "log messages go to these email addresses "
                   "in addition to logfiles");
GLOG_DEFINE_bool(log_file_header, true,
                 "Write the file header at the start of each log file");
GLOG_DEFINE_bool(log_prefix, true,
                 "Prepend the log prefix to the start of each log line");
GLOG_DEFINE_bool(log_year_in_prefix, true,
                 "Include the year in the log prefix");
GLOG_DEFINE_int32(minloglevel, 0,
                  "Messages logged at a lower level than this don't "
                  "actually get logged anywhere");
GLOG_DEFINE_int32(logbuflevel, 0,
                  "Buffer log messages logged at this level or lower"
                  " (-1 means don't buffer; 0 means buffer INFO only;"
                  " ...)");
GLOG_DEFINE_int32(logbufsecs, 30,
                  "Buffer log messages for at most this many seconds");

GLOG_DEFINE_int32(logcleansecs, 60 * 5,  // every 5 minutes
                  "Clean overdue logs every this many seconds");

GLOG_DEFINE_int32(logemaillevel, 999,
                  "Email log messages logged at this level or higher"
                  " (0 means email all; 3 means email FATAL only;"
                  " ...)");
GLOG_DEFINE_string(logmailer, "", "Mailer used to send logging email");

GLOG_DEFINE_int32(logfile_mode, 0664, "Log file mode/permissions.");

GLOG_DEFINE_string(
    log_dir, DefaultLogDir(),
    "If specified, logfiles are written into this directory instead "
    "of the default logging directory.");
GLOG_DEFINE_string(log_link, "",
                   "Put additional links to the log "
                   "files in this directory");

GLOG_DEFINE_uint32(max_log_size, 1800,
                   "approx. maximum log file size (in MB). A value of 0 will "
                   "be silently overridden to 1.");

GLOG_DEFINE_bool(stop_logging_if_full_disk, false,
                 "Stop attempting to log to disk if the disk is full.");

GLOG_DEFINE_string(log_backtrace_at, "",
                   "Emit a backtrace when logging at file:linenum.");

GLOG_DEFINE_bool(log_utc_time, false, "Use UTC time for logging.");

GLOG_DEFINE_int32(v, 0,
                  "Show all VLOG(m) messages for m <= this."
                  " Overridable by --vmodule.");

GLOG_DEFINE_string(
    vmodule, "",
    "per-module verbose level."
    " Argument is a comma-separated list of <module name>=<log level>."
    " <module name> is a glob pattern, matched against the filename base"
    " (that is, name ignoring .cc/.h./-inl.h)."
    " <log level> overrides any value given by --v.");

GLOG_DEFINE_bool(symbolize_stacktrace, true,
                 "Symbolize the stack trace in the tombstone");
