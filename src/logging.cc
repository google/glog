#define _GNU_SOURCE 1 // needed for O_NOFOLLOW and pread()/pwrite()

#include "utilities.h"

#include <assert.h>
#include <pthread.h>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <cstdio>
#include <iostream>
#include <stdarg.h>
#include <stdlib.h>
#include <pwd.h>
#include <syslog.h>
#include <vector>
#include <errno.h>                   // for errno
#include <sstream>
#include "base/commandlineflags.h"        // to get the program name
#include "glog/logging.h"
#include "glog/raw_logging.h"
#include "base/googleinit.h"

using std::string;
using std::vector;
using std::ostrstream;
using std::setw;
using std::hex;
using std::dec;
using std::min;
using std::ostream;
using std::ostringstream;
using std::strstream;

DEFINE_bool(logtostderr, false,
            "log messages go to stderr instead of logfiles");
DEFINE_bool(alsologtostderr, false,
            "log messages go to stderr in addition to logfiles");

// By default, errors (including fatal errors) get logged to stderr as
// well as the file.
//
// The default is ERROR instead of FATAL so that users can see problems
// when they run a program without having to look in another file.
DEFINE_int32(stderrthreshold,
             GOOGLE_NAMESPACE::ERROR,
             "log messages at or above this level are copied to stderr in "
             "addition to logfiles.  This flag obsoletes --alsologtostderr.");

DEFINE_string(alsologtoemail, "",
              "log messages go to these email addresses "
              "in addition to logfiles");
DEFINE_bool(log_prefix, true,
            "Prepend the log prefix to the start of each log line");
DEFINE_int32(minloglevel, 0, "Messages logged at a lower level than this don't "
             "actually get logged anywhere");
DEFINE_int32(logbuflevel, 0,
             "Buffer log messages logged at this level or lower"
             " (-1 means don't buffer; 0 means buffer INFO only;"
             " ...)");
DEFINE_int32(logbufsecs, 30,
             "Buffer log messages for at most this many seconds");
DEFINE_int32(logemaillevel, 999,
             "Email log messages logged at this level or higher"
             " (0 means email all; 3 means email FATAL only;"
             " ...)");
DEFINE_string(logmailer, "/bin/mail",
              "Mailer used to send logging email");

// Compute the default value for --log_dir
static const char* DefaultLogDir() {
  const char* env;
  env = getenv("GOOGLE_LOG_DIR");
  if (env != NULL && env[0] != '\0') {
    return env;
  }
  env = getenv("TEST_TMPDIR");
  if (env != NULL && env[0] != '\0') {
    return env;
  }
  return "";
}

DEFINE_string(log_dir, DefaultLogDir(),
              "If specified, logfiles are written into this directory instead "
              "of the default logging directory.");
DEFINE_string(log_link, "", "Put additional links to the log "
              "files in this directory");

DEFINE_int32(max_log_size, 1800,
             "approx. maximum log file size (in MB)");

DEFINE_bool(stop_logging_if_full_disk, false,
            "Stop attempting to log to disk if the disk is full.");

// TODO(hamaji): consider windows
#define PATH_SEPARATOR '/'

_START_GOOGLE_NAMESPACE_

// A mutex that allows only one thread to log at a time, to keep things from
// getting jumbled.  Some other very uncommon logging operations (like
// changing the destination file for log messages of a given severity) also
// lock this mutex.  Please be sure that anybody who might possibly need to
// lock it does so.
static Mutex log_mutex;

// Number of messages sent at each severity.  Under log_mutex.
int64 LogMessage::num_messages_[NUM_SEVERITIES] = {0, 0, 0, 0};

// Globally disable log writing (if disk is full)
static bool stop_writing = false;

const char*const LogSeverityNames[NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "FATAL"
};

const char* GetLogSeverityName(LogSeverity severity) {
  return LogSeverityNames[severity];
}

static bool SendEmailInternal(const char*dest, const char *subject,
                              const char*body, bool use_logging);

base::Logger::~Logger() {
}

// Encapsulates all file-system related state
class LogFileObject : public base::Logger {
 public:
  LogFileObject(LogSeverity severity, const char* base_filename);
  ~LogFileObject();

  virtual void Write(bool force_flush, // Should we force a flush here?
                     time_t timestamp,  // Timestamp for this entry
                     const char* message,
                     int message_len);

  // Configuration options
  void SetBasename(const char* basename);
  void SetExtension(const char* ext);
  void SetSymlinkBasename(const char* symlink_basename);

  // Normal flushing routine
  virtual void Flush();

  // It is the actual file length for the system loggers,
  // i.e., INFO, ERROR, etc.
  virtual uint32 LogSize() {
    MutexLock l(&lock_);
    return file_length_;
  }

  // Internal flush routine.  Exposed so that FlushLogFilesUnsafe()
  // can avoid grabbing a lock.  Usually Flush() calls it after
  // acquiring lock_.
  void FlushUnlocked();

 private:
  static const uint32 kRolloverAttemptFrequency = 0x20;

  Mutex lock_;
  bool base_filename_selected_;
  string base_filename_;
  string symlink_basename_;
  string filename_extension_;     // option users can specify (eg to add port#)
  FILE* file_;
  LogSeverity severity_;
  uint32 bytes_since_flush_;
  uint32 file_length_;
  unsigned int rollover_attempt_;
  int64 next_flush_time_;         // cycle count at which to flush log

  // Actually create a logfile using the value of base_filename_ and the
  // supplied argument time_pid_string
  // REQUIRES: lock_ is held
  bool CreateLogfile(const char* time_pid_string);
};

LogFileObject::LogFileObject(LogSeverity severity,
                             const char* base_filename)
  : base_filename_selected_(base_filename != NULL),
    base_filename_((base_filename != NULL) ? base_filename : ""),
    symlink_basename_(ProgramInvocationShortName()),
    filename_extension_(),
    file_(NULL),
    severity_(severity),
    bytes_since_flush_(0),
    file_length_(0),
    rollover_attempt_(kRolloverAttemptFrequency-1),
    next_flush_time_(0) {
  assert(severity >= 0);
  assert(severity < NUM_SEVERITIES);
}

LogFileObject::~LogFileObject() {
  MutexLock l(&lock_);
  if (file_ != NULL) {
    fclose(file_);
    file_ = NULL;
  }
}

void LogFileObject::SetBasename(const char* basename) {
  MutexLock l(&lock_);
  base_filename_selected_ = true;
  if (base_filename_ != basename) {
    // Get rid of old log file since we are changing names
    if (file_ != NULL) {
      fclose(file_);
      file_ = NULL;
      rollover_attempt_ = kRolloverAttemptFrequency-1;
    }
    base_filename_ = basename;
  }
}

void LogFileObject::SetExtension(const char* ext) {
  MutexLock l(&lock_);
  if (filename_extension_ != ext) {
    // Get rid of old log file since we are changing names
    if (file_ != NULL) {
      fclose(file_);
      file_ = NULL;
      rollover_attempt_ = kRolloverAttemptFrequency-1;
    }
    filename_extension_ = ext;
  }
}

void LogFileObject::SetSymlinkBasename(const char* symlink_basename) {
  MutexLock l(&lock_);
  symlink_basename_ = symlink_basename;
}

class LogDestination {
 public:
  friend class LogMessage;
  friend void ReprintFatalMessage();
  friend base::Logger* base::GetLogger(LogSeverity);
  friend void base::SetLogger(LogSeverity, base::Logger*);

  // These methods are just forwarded to by their global versions.
  static void SetLogDestination(LogSeverity severity,
				const char* base_filename);
  static void SetLogSymlink(LogSeverity severity,
                            const char* symlink_basename);
  static void AddLogSink(LogSink *destination);
  static void RemoveLogSink(LogSink *destination);
  static void SetLogFilenameExtension(const char* filename_extension);
  static void SetStderrLogging(LogSeverity min_severity);
  static void SetEmailLogging(LogSeverity min_severity, const char* addresses);
  static void LogToStderr();
  // Flush all log files that are at least at the given severity level
  static void FlushLogFiles(int min_severity);
  static void FlushLogFilesUnsafe(int min_severity);

  // we set the maximum size of our packet to be 1400, the logic being
  // to prevent fragmentation.
  // Really this number is arbitrary.
  static const int kNetworkBytes = 1400;

  static const string& hostname();
 private:

  LogDestination(LogSeverity severity, const char* base_filename);
  ~LogDestination() { }

  // Take a log message of a particular severity and log it to stderr
  // iff it's of a high enough severity to deserve it.
  static void MaybeLogToStderr(LogSeverity severity, const char* message,
			       size_t len);

  // Take a log message of a particular severity and log it to email
  // iff it's of a high enough severity to deserve it.
  static void MaybeLogToEmail(LogSeverity severity, const char* message,
			      size_t len);
  // Take a log message of a particular severity and log it to a file
  // iff the base filename is not "" (which means "don't log to me")
  static void MaybeLogToLogfile(LogSeverity severity,
                                time_t timestamp,
				const char* message, size_t len);
  // Take a log message of a particular severity and log it to the file
  // for that severity and also for all files with severity less than
  // this severity.
  static void LogToAllLogfiles(LogSeverity severity,
                               time_t timestamp,
                               const char* message, size_t len);

  // Send logging info to all registered sinks.
  static void LogToSinks(LogSeverity severity,
                         const char *full_filename,
                         const char *base_filename,
                         int line,
                         const struct ::tm* tm_time,
                         const char* message,
                         size_t message_len);

  // Wait for all registered sinks via WaitTillSent
  // including the optional one in "data".
  static void WaitForSinks(LogMessage::LogMessageData* data);

  static LogDestination* log_destination(LogSeverity severity);

  LogFileObject fileobject_;
  base::Logger* logger_;      // Either &fileobject_, or wrapper around it

  static LogDestination* log_destinations_[NUM_SEVERITIES];
  static LogSeverity email_logging_severity_;
  static string addresses_;
  static string hostname_;

  // arbitrary global logging destinations.
  static vector<LogSink*>* sinks_;

  // Protects the vector sinks_,
  // but not the LogSink objects its elements reference.
  static Mutex sink_mutex_;

  // Disallow
  LogDestination(const LogDestination&);
  LogDestination& operator=(const LogDestination&);
};

// Errors do not get logged to email by default.
LogSeverity LogDestination::email_logging_severity_ = 99999;

string LogDestination::addresses_ = "";
string LogDestination::hostname_ = "";

vector<LogSink*>* LogDestination::sinks_ = NULL;
Mutex LogDestination::sink_mutex_;

/* static */
const string& LogDestination::hostname() {
  if (hostname_.empty()) {
    struct utsname buf;
    if (0 == uname(&buf)) {
      hostname_ = buf.nodename;
    } else {
      hostname_ = "(unknown)";
    }
  }
  return hostname_;
}

LogDestination::LogDestination(LogSeverity severity,
                               const char* base_filename)
  : fileobject_(severity, base_filename),
    logger_(&fileobject_) {
}

void LogFileObject::Flush() {
  MutexLock l(&lock_);
  FlushUnlocked();
}

void LogFileObject::FlushUnlocked(){
  if (file_ != NULL) {
    fflush(file_);
    bytes_since_flush_ = 0;
  }
  // Figure out when we are due for another flush.
  const int64 next = (FLAGS_logbufsecs
                      * static_cast<int64>(1000000));  // in usec
  next_flush_time_ = CycleClock_Now() + UsecToCycles(next);
}

inline void LogDestination::FlushLogFilesUnsafe(int min_severity) {
  // assume we have the log_mutex or we simply don't care
  // about it
  for (int i = min_severity; i < NUM_SEVERITIES; i++) {
    LogDestination* log = log_destination(i);
    if (log != NULL) {
      // Flush the base fileobject_ logger directly instead of going
      // through any wrappers to reduce chance of deadlock.
      log->fileobject_.FlushUnlocked();
    }
  }
}

inline void LogDestination::FlushLogFiles(int min_severity) {
  // Prevent any subtle race conditions by wrapping a mutex lock around
  // all this stuff.
  MutexLock l(&log_mutex);
  for (int i = min_severity; i < NUM_SEVERITIES; i++) {
    LogDestination* log = log_destination(i);
    if (log != NULL) {
      log->logger_->Flush();
    }
  }
}

inline void LogDestination::SetLogDestination(LogSeverity severity,
					      const char* base_filename) {
  assert(severity >= 0 && severity < NUM_SEVERITIES);
  // Prevent any subtle race conditions by wrapping a mutex lock around
  // all this stuff.
  MutexLock l(&log_mutex);
  log_destination(severity)->fileobject_.SetBasename(base_filename);
}

inline void LogDestination::SetLogSymlink(LogSeverity severity,
                                          const char* symlink_basename) {
  CHECK_GE(severity, 0);
  CHECK_LT(severity, NUM_SEVERITIES);
  MutexLock l(&log_mutex);
  log_destination(severity)->fileobject_.SetSymlinkBasename(symlink_basename);
}

inline void LogDestination::AddLogSink(LogSink *destination) {
  // Prevent any subtle race conditions by wrapping a mutex lock around
  // all this stuff.
  MutexLock l(&sink_mutex_);
  if (!sinks_)  sinks_ = new vector<LogSink*>;
  sinks_->push_back(destination);
}

inline void LogDestination::RemoveLogSink(LogSink *destination) {
  // Prevent any subtle race conditions by wrapping a mutex lock around
  // all this stuff.
  MutexLock l(&sink_mutex_);
  // This doesn't keep the sinks in order, but who cares?
  if (sinks_) {
    for (int i = sinks_->size() - 1; i >= 0; i--) {
      if ((*sinks_)[i] == destination) {
        (*sinks_)[i] = (*sinks_)[sinks_->size() - 1];
        sinks_->pop_back();
        break;
      }
    }
  }
}

inline void LogDestination::SetLogFilenameExtension(const char* ext) {
  // Prevent any subtle race conditions by wrapping a mutex lock around
  // all this stuff.
  MutexLock l(&log_mutex);
  for ( int severity = 0; severity < NUM_SEVERITIES; ++severity ) {
    log_destination(severity)->fileobject_.SetExtension(ext);
  }
}

inline void LogDestination::SetStderrLogging(LogSeverity min_severity) {
  assert(min_severity >= 0 && min_severity < NUM_SEVERITIES);
  // Prevent any subtle race conditions by wrapping a mutex lock around
  // all this stuff.
  MutexLock l(&log_mutex);
  FLAGS_stderrthreshold = min_severity;
}

inline void LogDestination::LogToStderr() {
  // *Don't* put this stuff in a mutex lock, since SetStderrLogging &
  // SetLogDestination already do the locking!
  SetStderrLogging(0);            // thus everything is "also" logged to stderr
  for ( int i = 0; i < NUM_SEVERITIES; ++i ) {
    SetLogDestination(i, "");     // "" turns off logging to a logfile
  }
}

inline void LogDestination::SetEmailLogging(LogSeverity min_severity,
					    const char* addresses) {
  assert(min_severity >= 0 && min_severity < NUM_SEVERITIES);
  // Prevent any subtle race conditions by wrapping a mutex lock around
  // all this stuff.
  MutexLock l(&log_mutex);
  LogDestination::email_logging_severity_ = min_severity;
  LogDestination::addresses_ = addresses;
}

static void WriteToStderr(const char* message, size_t len) {
  // Avoid using cerr from this module since we may get called during
  // exit code, and cerr may be partially or fully destroyed by then.
  fwrite(message, len, 1, stderr);
}

inline void LogDestination::MaybeLogToStderr(LogSeverity severity,
					     const char* message, size_t len) {
  if ((severity >= FLAGS_stderrthreshold) || FLAGS_alsologtostderr) {
    WriteToStderr(message, len);
#ifdef OS_WINDOWS
    // On Windows, also output to the debugger
    ::OutputDebugStringA(string(message,len).c_str());
#endif
  }
}


inline void LogDestination::MaybeLogToEmail(LogSeverity severity,
					    const char* message, size_t len) {
  if (severity >= email_logging_severity_ ||
      severity >= FLAGS_logemaillevel) {
    string to(FLAGS_alsologtoemail);
    if (!addresses_.empty()) {
      if (!to.empty()) {
        to += ",";
      }
      to += addresses_;
    }
    const string subject(string("[LOG] ") + LogSeverityNames[severity] + ": " +
                         ProgramInvocationShortName());
    string body(hostname());
    body += "\n\n";
    body.append(message, len);

    // should NOT use SendEmail().  The caller of this function holds the
    // log_mutex and SendEmail() calls LOG/VLOG which will block trying to
    // acquire the log_mutex object.  Use SendEmailInternal() and set
    // use_logging to false.
    SendEmailInternal(to.c_str(), subject.c_str(), body.c_str(), false);
  }
}


inline void LogDestination::MaybeLogToLogfile(LogSeverity severity,
                                              time_t timestamp,
					      const char* message,
					      size_t len) {
  const bool should_flush = severity > FLAGS_logbuflevel;
  LogDestination* destination = log_destination(severity);
  destination->logger_->Write(should_flush, timestamp, message, len);
}

inline void LogDestination::LogToAllLogfiles(LogSeverity severity,
                                             time_t timestamp,
                                             const char* message,
                                             size_t len) {

  if ( FLAGS_logtostderr )            // global flag: never log to file
    WriteToStderr(message, len);
  else
    for (int i = severity; i >= 0; --i)
      LogDestination::MaybeLogToLogfile(i, timestamp, message, len);

}

inline void LogDestination::LogToSinks(LogSeverity severity,
                                       const char *full_filename,
                                       const char *base_filename,
                                       int line,
                                       const struct ::tm* tm_time,
                                       const char* message,
                                       size_t message_len) {
  ReaderMutexLock l(&sink_mutex_);
  if (sinks_) {
    for (int i = sinks_->size() - 1; i >= 0; i--) {
      (*sinks_)[i]->send(severity, full_filename, base_filename,
                         line, tm_time, message, message_len);
    }
  }
}

inline void LogDestination::WaitForSinks(LogMessage::LogMessageData* data) {
  ReaderMutexLock l(&sink_mutex_);
  if (sinks_) {
    for (int i = sinks_->size() - 1; i >= 0; i--) {
      (*sinks_)[i]->WaitTillSent();
    }
  }
  if (data->send_method_ == &LogMessage::SendToSinkAndLog &&
      data->sink_ != NULL) {
    data->sink_->WaitTillSent();
  }
}

bool LogFileObject::CreateLogfile(const char* time_pid_string) {
  string string_filename = base_filename_+filename_extension_+
                           time_pid_string;
  const char* filename = string_filename.c_str();
  int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0664);
  if (fd == -1) return false;
  // Mark the file close-on-exec. We don't really care if this fails
  fcntl(fd, F_SETFD, FD_CLOEXEC);

  file_ = fdopen(fd, "a");  // Make a FILE*.
  if (file_ == NULL) {  // Man, we're screwed!
    close(fd);
    unlink(filename);  // Erase the half-baked evidence: an unusable log file
    return false;
  }

  // We try to create a symlink called <program_name>.<severity>,
  // which is easier to use.  (Every time we create a new logfile,
  // we destroy the old symlink and create a new one, so it always
  // points to the latest logfile.)  If it fails, we're sad but it's
  // no error.
  if (!symlink_basename_.empty()) {
    // take directory from filename
    const char* slash = strrchr(filename, PATH_SEPARATOR);
    const string linkname =
      symlink_basename_ + '.' + LogSeverityNames[severity_];
    string linkpath;
    if ( slash ) linkpath = string(filename, slash-filename+1);  // get dirname
    linkpath += linkname;
    unlink(linkpath.c_str());                    // delete old one if it exists

    // Make the symlink be relative (in the same dir) so that if the
    // entire log directory gets relocated the link is still valid.
    const char *linkdest = slash ? (slash + 1) : filename;
    symlink(linkdest, linkpath.c_str());         // silently ignore failures

    // Make an additional link to the log file in a place specified by
    // FLAGS_log_link, if indicated
    if (!FLAGS_log_link.empty()) {
      linkpath = FLAGS_log_link + "/" + linkname;
      unlink(linkpath.c_str());                  // delete old one if it exists
      symlink(filename, linkpath.c_str());       // silently ignore failures
    }
  }

  return true;  // Everything worked
}

void LogFileObject::Write(bool force_flush,
                          time_t timestamp,
                          const char* message,
                          int message_len) {
  MutexLock l(&lock_);

  // We don't log if the base_name_ is "" (which means "don't write")
  if (base_filename_selected_ && base_filename_.empty()) {
    return;
  }

  if ((file_length_ >> 20) >= FLAGS_max_log_size) {
    fclose(file_);
    file_ = NULL;
    file_length_ = bytes_since_flush_ = 0;
    rollover_attempt_ = kRolloverAttemptFrequency-1;
  }

  // If there's no destination file, make one before outputting
  if (file_ == NULL) {
    // Try to rollover the log file every 32 log messages.  The only time
    // this could matter would be when we have trouble creating the log
    // file.  If that happens, we'll lose lots of log messages, of course!
    if (++rollover_attempt_ != kRolloverAttemptFrequency) return;
    rollover_attempt_ = 0;

    struct ::tm tm_time;
    localtime_r(&timestamp, &tm_time);

    // The logfile's filename will have the date/time & pid in it
    char time_pid_string[256];  // More than enough chars for time, pid, \0
    ostrstream time_pid_stream(time_pid_string, sizeof(time_pid_string));
    time_pid_stream.fill('0');
    time_pid_stream << 1900+tm_time.tm_year
		    << setw(2) << 1+tm_time.tm_mon
		    << setw(2) << tm_time.tm_mday
		    << '-'
		    << setw(2) << tm_time.tm_hour
		    << setw(2) << tm_time.tm_min
		    << setw(2) << tm_time.tm_sec
		    << '.'
		    << GetMainThreadPid()
		    << '\0';

    if (base_filename_selected_) {
      if (!CreateLogfile(time_pid_string)) {
        perror("Could not create log file");
        fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n", time_pid_string);
        return;
      }
    } else {
      // If no base filename for logs of this severity has been set, use a
      // default base filename of
      // "<program name>.<hostname>.<user name>.log.<severity level>.".  So
      // logfiles will have names like
      // webserver.examplehost.root.log.INFO.19990817-150000.4354, where
      // 19990817 is a date (1999 August 17), 150000 is a time (15:00:00),
      // and 4354 is the pid of the logging process.  The date & time reflect
      // when the file was created for output.
      //
      // Where does the file get put?  Successively try the directories
      // "/tmp", and "."
      string stripped_filename(ProgramInvocationShortName());  // in cmdlineflag
      struct utsname buf;
      if (0 != uname(&buf)) {
        *buf.nodename = '\0';  // ensure null termination on failure
      }

      string uidname = MyUserName();
      // We should not call CHECK() here because this function can be
      // called after holding on to log_mutex. We don't want to
      // attempt to hold on to the same mutex, and get into a
      // deadlock. Simply use a name like invalid-user.
      if (uidname.empty()) uidname = "invalid-user";

      stripped_filename = stripped_filename+'.'+buf.nodename+'.'
                          +uidname+".log."
                          +LogSeverityNames[severity_]+'.';
      // We're going to (potentially) try to put logs in several different dirs
      const vector<string> & log_dirs = GetLoggingDirectories();

      // Go through the list of dirs, and try to create the log file in each
      // until we succeed or run out of options
      bool success = false;
      for (vector<string>::const_iterator dir = log_dirs.begin();
           dir != log_dirs.end();
           ++dir) {
        base_filename_ = *dir + "/" + stripped_filename;
        if ( CreateLogfile(time_pid_string) ) {
          success = true;
          break;
        }
      }
      // If we never succeeded, we have to give up
      if ( success == false ) {
        perror("Could not create logging file");
        fprintf(stderr, "COULD NOT CREATE A LOGGINGFILE %s!", time_pid_string);
        return;
      }
    }

    // Write a header message into the log file
    char file_header_string[512];  // Enough chars for time and binary info
    ostrstream file_header_stream(file_header_string,
                                  sizeof(file_header_string));
    file_header_stream.fill('0');
    file_header_stream << "Log file created at: "
                       << 1900+tm_time.tm_year << '/'
                       << setw(2) << 1+tm_time.tm_mon << '/'
                       << setw(2) << tm_time.tm_mday
                       << ' '
                       << setw(2) << tm_time.tm_hour << ':'
                       << setw(2) << tm_time.tm_min << ':'
                       << setw(2) << tm_time.tm_sec << '\n'
                       << "Running on machine: "
                       << LogDestination::hostname() << '\n'
                       << '\0';
    int header_len = strlen(file_header_string);
    fwrite(file_header_string, 1, header_len, file_);
    file_length_ += header_len;
    bytes_since_flush_ += header_len;
  }

  // Write to LOG file
  if ( !stop_writing ) {
    // fwrite() doesn't return an error when the disk is full, for
    // messages that are less than 4096 bytes. When the disk is full,
    // it returns the message length for messages that are less than
    // 4096 bytes. fwrite() returns 4096 for message lengths that are
    // greater than 4096, thereby indicating an error.
    errno = 0;
    fwrite(message, 1, message_len, file_);
    if ( FLAGS_stop_logging_if_full_disk &&
         errno == ENOSPC ) {  // disk full, stop writing to disk
      stop_writing = true;  // until the disk is
      return;
    } else {
      file_length_ += message_len;
      bytes_since_flush_ += message_len;
    }
  } else {
    if ( CycleClock_Now() >= next_flush_time_ )
      stop_writing = false;  // check to see if disk has free space.
    return;  // no need to flush
  }

  // See important msgs *now*.  Also, flush logs at least every 10^6 chars,
  // or every "FLAGS_logbufsecs" seconds.
  if ( force_flush ||
       (bytes_since_flush_ >= 1000000) ||
       (CycleClock_Now() >= next_flush_time_) ) {
    FlushUnlocked();
  }
}

LogDestination* LogDestination::log_destinations_[NUM_SEVERITIES];

inline LogDestination* LogDestination::log_destination(LogSeverity severity) {
  assert(severity >=0 && severity < NUM_SEVERITIES);
  if (!log_destinations_[severity]) {
    log_destinations_[severity] = new LogDestination(severity, NULL);
  }
  return log_destinations_[severity];
}

// Get the part of filepath after the last path separator.
// (Doesn't modify filepath, contrary to basename() in libgen.h.)
static const char* const_basename(const char* filepath) {
  const char* base = strrchr(filepath, '/');
#ifdef OS_WINDOWS  // Look for either path separator in Windows
  if (!base)
    base = strrchr(filepath, '\\');
#endif
  return base ? (base+1) : filepath;
}

//
// LogMessage's constructor starts each message with a string like:
// I1018 160715 logging.cc:1153]
// (1st letter of severity level, GMT month, date, & time;
// thread id (if not 0x400); basename of file and line of the logging command)
// We ignore thread id 0x400 because it seems to be the default for single-
// threaded programs.

// An arbitrary limit on the length of a single log message.  This
// is so that streaming can be done more efficiently.
const size_t LogMessage::kMaxLogMessageLen = 30000;

// Need to reserve some space for FATAL messages because
// we usually do LOG(FATAL) when we ran out of heap memory.
// However, since LogMessage() also calls new[], in this case,
// it will recusively call LOG(FATAL), which then call new[] ... etc.
// Eventually, the program will run out of stack memory and no message
// will get logged.   Note that we will not be protecting this buffer
// with a lock because the chances are very small that there will
// be a contention during LOG(FATAL) which can only happen at most
// once per program.
static char fatal_message_buffer[LogMessage::kMaxLogMessageLen+1];

// Similarly we reserve space for a LogMessageData struct to be used
// for FATAL messages.
LogMessage::LogMessageData LogMessage::fatal_message_data_(0, FATAL, 0);

LogMessage::LogMessageData::LogMessageData(int preserved_errno,
                                           LogSeverity severity,
                                           int ctr) :
    // ORDER DEPENDENCY: buf_ comes before message_text_ comes before stream_
    preserved_errno_(preserved_errno),
    // Use static buffer for LOG(FATAL)
    buf_((severity != FATAL) ? new char[kMaxLogMessageLen+1] : NULL),
    message_text_((severity != FATAL) ? buf_ : fatal_message_buffer),
    stream_(message_text_, kMaxLogMessageLen, ctr),
    severity_(severity) {
}

LogMessage::LogMessageData::~LogMessageData() {
  delete[] buf_;
}

LogMessage::LogMessageData* LogMessage::GetMessageData(int preserved_errno,
                                                       LogSeverity severity,
                                                       int ctr) {
  if (severity != FATAL) {
    return allocated_;
  } else {
    fatal_message_data_.preserved_errno_ = preserved_errno;
    fatal_message_data_.stream_.set_ctr(ctr);
    return &fatal_message_data_;
  }
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
		       int ctr, void (LogMessage::*send_method)()) :
    allocated_((severity != FATAL) ?
               new LogMessageData(errno, severity, ctr) : NULL),
    data_(GetMessageData(errno, severity, ctr)) {
  Init(file, line, severity, send_method);
}

LogMessage::LogMessage(const char* file, int line, const CheckOpString& result)
    : allocated_(NULL),
      data_(GetMessageData(errno, FATAL, 0)) {
  Init(file, line, FATAL, &LogMessage::SendToLog);
  stream() << "Check failed: " << (*result.str_) << " ";
}

LogMessage::LogMessage(const char* file, int line) :
    allocated_(new LogMessageData(errno, INFO, 0)),
    data_(GetMessageData(errno, INFO, 0)) {
  Init(file, line, INFO, &LogMessage::SendToLog);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity) :
    allocated_((severity != FATAL) ?
               new LogMessageData(errno, severity, 0) : NULL),
    data_(GetMessageData(errno, severity, 0)) {
  Init(file, line, severity, &LogMessage::SendToLog);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       LogSink* sink) :
    allocated_((severity != FATAL) ?
               new LogMessageData(errno, severity, 0) : NULL),
    data_(GetMessageData(errno, severity, 0)) {
  Init(file, line, severity, &LogMessage::SendToSinkAndLog);
  data_->sink_ = sink;  // override Init()'s setting to NULL
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       vector<string> *outvec) :
    allocated_((severity != FATAL) ?
               new LogMessageData(errno, severity, 0) : NULL),
    data_(GetMessageData(errno, severity, 0)) {
  Init(file, line, severity, &LogMessage::SaveOrSendToLog);
  data_->outvec_ = outvec; // override Init()'s setting to NULL
}

void LogMessage::Init(const char* file, int line, LogSeverity severity,
                      void (LogMessage::*send_method)()) {
  data_->line_ = line;
  data_->send_method_ = send_method;

  data_->outvec_ = NULL;
  data_->sink_ = NULL;
  data_->timestamp_ = time(NULL);
  localtime_r(&data_->timestamp_, &data_->tm_time_);
  RawLog__SetLastTime(data_->tm_time_);
  data_->basename_ = const_basename(file);
  data_->fullname_ = file;
  data_->stream_.fill('0');

  // In some cases, we use logging like a print mechanism and
  // the prefixes get in the way
  if (FLAGS_log_prefix && (line != kNoLogPrefix)) {
    if ( is_default_thread() ) {
      stream() << LogSeverityNames[severity][0]
               << setw(2) << 1+data_->tm_time_.tm_mon
               << setw(2) << data_->tm_time_.tm_mday
               << ' '
               << setw(2) << data_->tm_time_.tm_hour
               << setw(2) << data_->tm_time_.tm_min
               << setw(2) << data_->tm_time_.tm_sec
               << ' ' << data_->basename_ << ':' << data_->line_ << "] ";
    } else {
      stream() << LogSeverityNames[severity][0]
               << setw(2) << 1+data_->tm_time_.tm_mon
               << setw(2) << data_->tm_time_.tm_mday
               << ' '
               << setw(2) << data_->tm_time_.tm_hour
               << setw(2) << data_->tm_time_.tm_min
               << setw(2) << data_->tm_time_.tm_sec
               << ' ' << setw(8) << std::hex << pthread_self() << std::dec
               << ' ' << data_->basename_ << ':' << data_->line_ << "] ";
    }
  }

  data_->num_prefix_chars_ = data_->stream_.pcount();
  data_->has_been_flushed_ = false;
}

LogMessage::~LogMessage() {
  Flush();
  delete allocated_;
}

// Flush buffered message, called by the destructor, or any other function
// that needs to synchronize the log.
void LogMessage::Flush() {
  if (data_->has_been_flushed_ || data_->severity_ < FLAGS_minloglevel)
    return;

  data_->num_chars_to_log_ = data_->stream_.pcount();
  data_->num_chars_to_syslog_ =
    data_->num_chars_to_log_ - data_->num_prefix_chars_;

  // Do we need to add a \n to the end of this message?
  bool append_newline =
    (data_->message_text_[data_->num_chars_to_log_-1] != '\n');
  char original_final_char = '\0';

  // If we do need to add a \n, we'll do it by violating the memory of the
  // ostrstream buffer.  This is quick, and we'll make sure to undo our
  // modification before anything else is done with the ostrstream.  It
  // would be preferable not to do things this way, but it seems to be
  // the best way to deal with this.
  if (append_newline) {
    original_final_char = data_->message_text_[data_->num_chars_to_log_];
    data_->message_text_[data_->num_chars_to_log_++] = '\n';
  }

  // Prevent any subtle race conditions by wrapping a mutex lock around
  // the actual logging action per se.
  {
    MutexLock l(&log_mutex);
    (this->*(data_->send_method_))();
    ++num_messages_[static_cast<int>(data_->severity_)];
  }
  LogDestination::WaitForSinks(data_);

  if (append_newline) {
    // Fix the ostrstream back how it was before we screwed with it.
    // It's 99.44% certain that we don't need to worry about doing this.
    data_->message_text_[data_->num_chars_to_log_-1] = original_final_char;
  }

  // If errno was already set before we enter the logging call, we'll
  // set it back to that value when we return from the logging call.
  // It happens often that we log an error message after a syscall
  // failure, which can potentially set the errno to some other
  // values.  We would like to preserve the original errno.
  if (data_->preserved_errno_ != 0) {
    errno = data_->preserved_errno_;
  }

  // Note that this message is now safely logged.  If we're asked to flush
  // again, as a result of destruction, say, we'll do nothing on future calls.
  data_->has_been_flushed_ = true;
}

// Copy of FATAL log message so that we can print it out again after
// all the stack traces.
// We cannot simply use fatal_message_buffer, because two or more FATAL log
// messages may happen in a row. This is a real possibility given that
// FATAL log messages are often associated with corrupted process state.
// In this case, we still want to reprint the first FATAL log message, so
// we need to save away the first message in a separate buffer.
static time_t fatal_time;
static char fatal_message[256];

void ReprintFatalMessage() {
  if (fatal_message[0]) {
    const int n = strlen(fatal_message);
    if (!FLAGS_logtostderr) {
      // Also write to stderr
      WriteToStderr(fatal_message, n);
    }
    LogDestination::LogToAllLogfiles(ERROR, fatal_time, fatal_message, n);
  }
}

// L >= log_mutex (callers must hold the log_mutex).
void LogMessage::SendToLog() {
  static bool already_warned_before_initgoogle = false;

  log_mutex.AssertHeld();

  RAW_DCHECK(data_->num_chars_to_log_ > 0 &&
             data_->message_text_[data_->num_chars_to_log_-1] == '\n', "");

  // Messages of a given severity get logged to lower severity logs, too

  if (!already_warned_before_initgoogle && !IsGoogleLoggingInitialized()) {
    const char w[] = "WARNING: Logging before InitGoogleLogging() is "
                     "written to STDERR\n";
    WriteToStderr(w, strlen(w));
    already_warned_before_initgoogle = true;
  }

  // global flag: never log to file if set.  Also -- don't log to a
  // file if we haven't parsed the command line flags to get the
  // program name.
  if (FLAGS_logtostderr || !IsGoogleLoggingInitialized()) {
    WriteToStderr(data_->message_text_, data_->num_chars_to_log_);

    // this could be protected by a flag if necessary.
    LogDestination::LogToSinks(data_->severity_,
                               data_->fullname_, data_->basename_,
                               data_->line_, &data_->tm_time_,
                               data_->message_text_ + data_->num_prefix_chars_,
                               (data_->num_chars_to_log_ -
                                data_->num_prefix_chars_ - 1));
  } else {

    // log this message to all log files of severity <= severity_
    LogDestination::LogToAllLogfiles(data_->severity_, data_->timestamp_,
                                     data_->message_text_,
                                     data_->num_chars_to_log_);

    LogDestination::MaybeLogToStderr(data_->severity_, data_->message_text_,
                                     data_->num_chars_to_log_);
    LogDestination::MaybeLogToEmail(data_->severity_, data_->message_text_,
                                    data_->num_chars_to_log_);
    LogDestination::LogToSinks(data_->severity_,
                               data_->fullname_, data_->basename_,
                               data_->line_, &data_->tm_time_,
                               data_->message_text_ + data_->num_prefix_chars_,
                               (data_->num_chars_to_log_
                                - data_->num_prefix_chars_ - 1));
    // NOTE: -1 removes trailing \n
  }

  // If we log a FATAL message, flush all the log destinations, then toss
  // a signal for others to catch. We leave the logs in a state that
  // someone else can use them (as long as they flush afterwards)
  if (data_->severity_ == FATAL) {
    // save away the fatal message so we can print it again later
    const int copy = min<int>(data_->num_chars_to_log_,
                              sizeof(fatal_message)-1);
    memcpy(fatal_message, data_->message_text_, copy);
    fatal_message[copy] = '\0';
    fatal_time = data_->timestamp_;

    if (!FLAGS_logtostderr) {
      for (int i = 0; i < NUM_SEVERITIES; ++i) {
        if ( LogDestination::log_destinations_[i] )
          LogDestination::log_destinations_[i]->logger_->Write(true, 0, "", 0);
      }
    }
    
    // release the lock that our caller (directly or indirectly)
    // LogMessage::~LogMessage() grabbed so that signal handlers
    // can use the logging facility. Alternately, we could add
    // an entire unsafe logging interface to bypass locking
    // for signal handlers but this seems simpler.
    log_mutex.Unlock();
    LogDestination::WaitForSinks(data_);

    Fail();
  }
}

static void logging_fail() {
#if defined _DEBUG && defined COMPILER_MSVC
  // When debugging on windows, avoid the obnoxious dialog and make
  // it possible to continue past a LOG(FATAL) in the debugger
  _asm int 3
#else
  abort();
#endif
}

#ifdef HAVE___ATTRIBUTE__
void (*g_logging_fail_func)() __attribute__((noreturn)) = &logging_fail;
#else
void (*g_logging_fail_func)() = &logging_fail;
#endif

void InstallFailureFunction(void (*fail_func)()) {
  g_logging_fail_func = fail_func;
}

void LogMessage::Fail() {
  g_logging_fail_func();
}

// L >= log_mutex (callers must hold the log_mutex).
void LogMessage::SendToSinkAndLog() {
  if (data_->sink_ != NULL) {
    RAW_DCHECK(data_->num_chars_to_log_ > 0 &&
               data_->message_text_[data_->num_chars_to_log_-1] == '\n', "");
    data_->sink_->send(data_->severity_, data_->fullname_, data_->basename_,
                       data_->line_, &data_->tm_time_,
                       data_->message_text_ + data_->num_prefix_chars_,
                       (data_->num_chars_to_log_ -
                        data_->num_prefix_chars_ - 1));
  }
  SendToLog();
}

// L >= log_mutex (callers must hold the log_mutex).
void LogMessage::SaveOrSendToLog() {
  if (data_->outvec_ != NULL) {
    RAW_DCHECK(data_->num_chars_to_log_ > 0 &&
               data_->message_text_[data_->num_chars_to_log_-1] == '\n', "");
    // Omit prefix of message and trailing newline when recording in outvec_.
    const char *start = data_->message_text_ + data_->num_prefix_chars_;
    int len = data_->num_chars_to_log_ - data_->num_prefix_chars_ - 1;
    data_->outvec_->push_back(string(start, len));
  } else {
    SendToLog();
  }
}

// L >= log_mutex (callers must hold the log_mutex).
void LogMessage::SendToSyslogAndLog() {
  // Before any calls to syslog(), make a single call to openlog()
  static bool openlog_already_called = false;
  if (!openlog_already_called) {
    openlog(ProgramInvocationShortName(), LOG_CONS | LOG_NDELAY | LOG_PID,
            LOG_USER);
    openlog_already_called = true;
  }

  // This array maps Google severity levels to syslog levels
  const int SEVERITY_TO_LEVEL[] = { LOG_INFO, LOG_WARNING, LOG_ERR, LOG_EMERG };
  syslog(LOG_USER | SEVERITY_TO_LEVEL[static_cast<int>(data_->severity_)], "%.*s",
         int(data_->num_chars_to_syslog_),
         data_->message_text_ + data_->num_prefix_chars_);
  SendToLog();
}

base::Logger* base::GetLogger(LogSeverity severity) {
  MutexLock l(&log_mutex);
  return LogDestination::log_destination(severity)->logger_;
}

void base::SetLogger(LogSeverity severity, base::Logger* logger) {
  MutexLock l(&log_mutex);
  LogDestination::log_destination(severity)->logger_ = logger;
}

// L < log_mutex.  Acquires and releases mutex_.
int64 LogMessage::num_messages(int severity) {
  MutexLock l(&log_mutex);
  return num_messages_[severity];
}

// Output the COUNTER value. This is only valid if ostream is a
// LogStream.
ostream& operator<<(ostream &os, const PRIVATE_Counter&) {
  LogMessage::LogStream *log = dynamic_cast<LogMessage::LogStream*>(&os);
  CHECK(log == log->self());
  os << log->ctr();
  return os;
}

ErrnoLogMessage::ErrnoLogMessage(const char* file, int line,
                                 LogSeverity severity, int ctr,
                                 void (LogMessage::*send_method)())
    : LogMessage(file, line, severity, ctr, send_method) {
}

ErrnoLogMessage::~ErrnoLogMessage() {
  // Don't access errno directly because it may have been altered
  // while streaming the message.
  char buf[100];
  posix_strerror_r(preserved_errno(), buf, sizeof(buf));
  stream() << ": " << buf << " [" << preserved_errno() << "]";
}

void FlushLogFiles(LogSeverity min_severity) {
  LogDestination::FlushLogFiles(min_severity);
}

void FlushLogFilesUnsafe(LogSeverity min_severity) {
  LogDestination::FlushLogFilesUnsafe(min_severity);
}

void SetLogDestination(LogSeverity severity, const char* base_filename) {
  LogDestination::SetLogDestination(severity, base_filename);
}

void SetLogSymlink(LogSeverity severity, const char* symlink_basename) {
  LogDestination::SetLogSymlink(severity, symlink_basename);
}

LogSink::~LogSink() {
}

void LogSink::WaitTillSent() {
  // noop default
}

string LogSink::ToString(LogSeverity severity, const char* file, int line,
                         const struct ::tm* tm_time,
                         const char* message, size_t message_len) {
  ostringstream stream(string(message, message_len));
  stream.fill('0');

  if ( is_default_thread() ) {
    stream << LogSeverityNames[severity][0]
           << setw(2) << 1+tm_time->tm_mon
           << setw(2) << tm_time->tm_mday
           << ' '
           << setw(2) << tm_time->tm_hour
           << setw(2) << tm_time->tm_min
           << setw(2) << tm_time->tm_sec
           << ' ' << file << ':' << line << "] ";
  } else {
    stream << LogSeverityNames[severity][0]
           << setw(2) << 1+tm_time->tm_mon
           << setw(2) << tm_time->tm_mday
           << ' '
           << setw(2) << tm_time->tm_hour
           << setw(2) << tm_time->tm_min
           << setw(2) << tm_time->tm_sec
           << ' ' << setw(8) << std::hex << pthread_self() << std::dec
           << ' ' << file << ':' << line << "] ";
  }

  stream << string(message, message_len);
  return stream.str();
}

void AddLogSink(LogSink *destination) {
  LogDestination::AddLogSink(destination);
}

void RemoveLogSink(LogSink *destination) {
  LogDestination::RemoveLogSink(destination);
}

void SetLogFilenameExtension(const char* ext) {
  LogDestination::SetLogFilenameExtension(ext);
}

void SetStderrLogging(LogSeverity min_severity) {
  LogDestination::SetStderrLogging(min_severity);
}

void SetEmailLogging(LogSeverity min_severity, const char* addresses) {
  LogDestination::SetEmailLogging(min_severity, addresses);
}

void LogToStderr() {
  LogDestination::LogToStderr();
}

// use_logging controls whether the logging functions LOG/VLOG are used
// to log errors.  It should be set to false when the caller holds the
// log_mutex.
static bool SendEmailInternal(const char*dest, const char *subject,
                              const char*body, bool use_logging) {
  if (dest && *dest) {
    if ( use_logging ) {
      VLOG(1) << "Trying to send TITLE:" << subject
              << " BODY:" << body << " to " << dest;
    } else {
      fprintf(stderr, "Trying to send TITLE: %s BODY: %s to %s\n",
              subject, body, dest);
    }

    string cmd(FLAGS_logmailer);
    cmd = cmd + " -s\"" + string(subject) + "\" " + string(dest);
    FILE* pipe = popen(cmd.c_str(), "w");
    if (pipe != NULL) {
      // Add the body if we have one
      if (body)
        fwrite(body, sizeof(char), strlen(body), pipe);
      bool ok = pclose(pipe) != -1;
      if ( !ok ) {
        if ( use_logging ) {
          char buf[100];
          posix_strerror_r(errno, buf, sizeof(buf));
          LOG(ERROR) << "Problems sending mail to " << dest << ": " << buf;
        } else {
          char buf[100];
          posix_strerror_r(errno, buf, sizeof(buf));
          fprintf(stderr, "Problems sending mail to %s: %s\n", dest, buf);
        }
      }
      return ok;
    } else {
      if ( use_logging ) {
        LOG(ERROR) << "Unable to send mail to " << dest;
      } else {
        fprintf(stderr, "Unable to send mail to %s\n", dest);
      }
    }
  }
  return false;
}

bool SendEmail(const char*dest, const char *subject, const char*body){
  return SendEmailInternal(dest, subject, body, true);
}

void GetTempDirectories(vector<string>* list) {
  list->clear();
#ifdef OS_WINDOWS
  // On windows we'll try to find a directory in this order:
  //   C:/Documents & Settings/whomever/TEMP (or whatever GetTempPath() is)
  //   C:/TMP/
  //   C:/TEMP/
  //   C:/WINDOWS/ or C:/WINNT/
  //   .
  char tmp[MAX_PATH];
  if (GetTempPathA(MAX_PATH, tmp))
    list->push_back(tmp);
  list->push_back("C:\\tmp\\");
  list->push_back("C:\\temp\\");
#else
  // Directories, in order of preference. If we find a dir that
  // exists, we stop adding other less-preferred dirs
  const char * candidates[] = {
    // Non-null only during unittest/regtest
    getenv("TEST_TMPDIR"),

    // Explicitly-supplied temp dirs
    getenv("TMPDIR"), getenv("TMP"),

    // If all else fails
    "/tmp",
  };

  for (int i = 0; i < sizeof(candidates) / sizeof(*candidates); i++) {
    const char *d = candidates[i];
    if (!d) continue;  // Empty env var

    // Make sure we don't surprise anyone who's expecting a '/'
    string dstr = d;
    if (dstr[dstr.size() - 1] != '/') {
      dstr += "/";
    }
    list->push_back(dstr);

    struct stat statbuf;
    if (!stat(d, &statbuf) && S_ISDIR(statbuf.st_mode)) {
      // We found a dir that exists - we're done.
      return;
    }
  }

#endif
}

static vector<string>* logging_directories_list;

const vector<string>& GetLoggingDirectories() {
  // Not strictly thread-safe but we're called early in InitGoogle().
  if (logging_directories_list == NULL) {
    logging_directories_list = new vector<string>;

    if ( !FLAGS_log_dir.empty() ) {
      // A dir was specified, we should use it
      logging_directories_list->push_back(FLAGS_log_dir.c_str());
    } else {
      GetTempDirectories(logging_directories_list);
#ifdef OS_WINDOWS
      char tmp[MAX_PATH];
      if (GetWindowsDirectoryA(tmp, MAX_PATH))
        logging_directories_list->push_back(tmp);
      logging_directories_list->push_back(".\\");
#else
      logging_directories_list->push_back("./");
#endif
    }
  }
  return *logging_directories_list;
}

void TestOnly_ClearLoggingDirectoriesList() {
  fprintf(stderr, "TestOnly_ClearLoggingDirectoriesList should only be "
          "called from test code.\n");
  delete logging_directories_list;
  logging_directories_list = NULL;
}

void GetExistingTempDirectories(vector<string>* list) {
  GetTempDirectories(list);
  vector<string>::iterator i_dir = list->begin();
  while( i_dir != list->end() ) {
    // zero arg to access means test for existence; no constant
    // defined on windows
    if ( access(i_dir->c_str(), 0) ) {
      i_dir = list->erase(i_dir);
    } else {
      ++i_dir;
    }
  }
}

void TruncateLogFile(const char *path, int64 limit, int64 keep) {

  struct stat statbuf;
  const int kCopyBlockSize = 8 << 10;
  char copybuf[kCopyBlockSize];
  int64 read_offset, write_offset;
  // Don't follow symlinks unless they're our own fd symlinks in /proc
  int flags = O_RDWR;
  const char *procfd_prefix = "/proc/self/fd/";
  if (strncmp(procfd_prefix, path, strlen(procfd_prefix))) flags |= O_NOFOLLOW;

  int fd = open(path, flags);
  if (fd == -1) {
    if (errno == EFBIG) {
      // The log file in question has got too big for us to open. The
      // real fix for this would be to compile logging.cc (or probably
      // all of base/...) with -D_FILE_OFFSET_BITS=64 but that's
      // rather scary.
      // Instead just truncate the file to something we can manage
      if (truncate(path, 0) == -1) {
	PLOG(ERROR) << "Unable to truncate " << path;
      } else {
	LOG(ERROR) << "Truncated " << path << " due to EFBIG error";
      }
    } else {
      PLOG(ERROR) << "Unable to open " << path;
    }
    return;
  }
  
  if (fstat(fd, &statbuf) == -1) {
    PLOG(ERROR) << "Unable to fstat()"; 
    goto out_close_fd; 
  }

  // See if the path refers to a regular file bigger than the
  // specified limit
  if (!S_ISREG(statbuf.st_mode)) goto out_close_fd;
  if (statbuf.st_size <= limit)  goto out_close_fd;
  if (statbuf.st_size <= keep) goto out_close_fd;

  // This log file is too large - we need to truncate it
  LOG(INFO) << "Truncating " << path << " to " << keep << " bytes";
  
  // Copy the last "keep" bytes of the file to the beginning of the file
  read_offset = statbuf.st_size - keep; 
  write_offset = 0;
  int bytesin, bytesout;
  while ((bytesin = pread(fd, copybuf, sizeof(copybuf), read_offset)) > 0) {
    bytesout = pwrite(fd, copybuf, bytesin, write_offset);
    if (bytesout == -1) {
      PLOG(ERROR) << "Unable to write to " << path;
      break;
    } else if (bytesout != bytesin) {
      LOG(ERROR) << "Expected to write " << bytesin << ", wrote " << bytesout;
    }
    read_offset += bytesin;
    write_offset += bytesout;
  }
  if (bytesin == -1) PLOG(ERROR) << "Unable to read from " << path;
  
  // Truncate the remainder of the file. If someone else writes to the
  // end of the file after our last read() above, we lose their latest
  // data. Too bad ...
  if (ftruncate(fd, write_offset) == -1) {
    PLOG(ERROR) << "Unable to truncate " << path;
  }

 out_close_fd:
  close(fd);

}

void TruncateStdoutStderr() {
  int64 limit = FLAGS_max_log_size << 20;
  int64 keep = 1 << 20;
  TruncateLogFile("/proc/self/fd/1", limit, keep);
  TruncateLogFile("/proc/self/fd/2", limit, keep);
}


// Helper functions for string comparisons.
#define DEFINE_CHECK_STROP_IMPL(name, func, expected) \
  string* Check##func##expected##Impl(const char* s1, const char* s2, \
                                      const char* names) { \
    bool equal = s1 == s2 || (s1 && s2 && !func(s1, s2)); \
    if (equal == expected) return NULL; \
    else { \
      strstream ss; \
      ss << #name " failed: " << names << " (" << s1 << " vs. " << s2 << ")"; \
      return new string(ss.str(), ss.pcount()); \
    } \
  }
DEFINE_CHECK_STROP_IMPL(CHECK_STREQ, strcmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRNE, strcmp, false)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASEEQ, strcasecmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASENE, strcasecmp, false)
#undef DEFINE_CHECK_STROP_IMPL

int posix_strerror_r(int err, char *buf, size_t len) {
  // Sanity check input parameters
  if (buf == NULL || len <= 0) {
    errno = EINVAL;
    return -1;
  }

  // Reset buf and errno, and try calling whatever version of strerror_r()
  // is implemented by glibc
  buf[0] = '\000';
  int old_errno = errno;
  errno = 0;
  char *rc = reinterpret_cast<char *>(strerror_r(err, buf, len));

  // Both versions set errno on failure
  if (errno) {
    // Should already be there, but better safe than sorry
    buf[0]     = '\000';
    return -1;
  }
  errno = old_errno;

  // POSIX is vague about whether the string will be terminated, although
  // is indirectly implies that typically ERANGE will be returned, instead
  // of truncating the string. This is different from the GNU implementation.
  // We play it safe by always terminating the string explicitly.
  buf[len-1] = '\000';

  // If the function succeeded, we can use its exit code to determine the
  // semantics implemented by glibc
  if (!rc) {
    return 0;
  } else {
    // GNU semantics detected
    if (rc == buf) {
      return 0;
    } else {
      buf[0] = '\000';
#if defined(OS_MACOSX) || defined(OS_FREEBSD)
      if (reinterpret_cast<int>(rc) < sys_nerr) {
        // This means an error on MacOSX or FreeBSD.
        return -1;
      }
#endif
      strncat(buf, rc, len-1);
      return 0;
    }
  }
}

LogMessageFatal::LogMessageFatal(const char* file, int line) :
    LogMessage(file, line, FATAL) {}

LogMessageFatal::LogMessageFatal(const char* file, int line,
                                 const CheckOpString& result) :
    LogMessage(file, line, result) {}

LogMessageFatal::~LogMessageFatal() {
    Flush();
    LogMessage::Fail();
}

_END_GOOGLE_NAMESPACE_
