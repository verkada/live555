#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>  // gettimeofday
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <mutex>

#include <Log.hh>

using namespace std;

// Max log size 1MB
// Currently we're logging to ramdisk so keep that in mind
// when changing this. 
#define MAX_LOG_FILE_SIZE (1 * 1024 * 1024)

#ifdef LOG_FILE_TMP_DIR
  _Log Log(LOG_FILE_TMP_DIR);
#else
  _Log Log;
#endif

/**
 * Return current timestamp as string in syslog-ng format. Not re-entrant.
 */
static const char* timestamp()
{
  static char buffer[33];

  struct timeval now;
  struct tm *nowlocal;

  // get and format timestamp
  gettimeofday(&now, NULL);
  nowlocal = localtime(&now.tv_sec);
  strftime(buffer, 33, "%Y-%m-%dT%H:%M:%S.000000%z ", nowlocal);

  // populate microseconds
  char plusminus = buffer[26];
  sprintf(buffer+20, "%06d", now.tv_usec);
  buffer[26] = plusminus;

  // convert from HHMM to HH:MM
  buffer[31] = buffer[30];
  buffer[30] = buffer[29];
  buffer[29] = ':';

  return buffer;
}

static string LogLevelAsString(LogLevel logLevel);

string LogLevelAsString(LogLevel logLevel) {
  switch(logLevel) {
    case LogLevelPanic: return "PANIC";
    case LogLevelFatal: return "FATAL";
    case LogLevelError: return "ERROR";
    case LogLevelWarning: return "WARN ";
    case LogLevelInfo: return "INFO ";
    case LogLevelDebug: return "DEBUG";
    case LogLevelTrace: return "TRACE";
  }
}

void _Log::OutputToFile(const string filePath) {
  currLogFilePath = filePath;
}

void _Log::OutputToStdout(bool toStdout) {
  logToStdout = toStdout;
}

_Log::_Log(const string logDirectory) : logFileDir(logDirectory), logToStdout(false), logLevel(LogLevelDebug) {
  if (kMaxLogs == 0) {  // Use stdout
    currLogFilePath = "";
    logToStdout = true;
    return;
  }

  // Set log file to 0
  SetLogFileNum(0);

  // If log files already exist from previouse run, remove them.
  while (access(currLogFilePath.c_str(), F_OK) == 0) {
    remove(currLogFilePath.c_str());
    NextLogFile();
  }

  // Set log file to 0 again
  SetLogFileNum(0);
}

// Caller assumed to be handling logMutex properly
FILE* _Log::AcquireFile() {
  if (currLogFilePath.empty()) {
    if (logToStdout) {
      return stdout;
    } else {
      return stderr;
    }
  } else {
    FILE* logFileHandle = fopen(currLogFilePath.c_str(), "a");
    if (logFileHandle == NULL) {
      fprintf(stderr, "Couldn't acquire log number %u.%s\n", logFileNum, strerror(errno));
    }
    return logFileHandle;
  }
}

void _Log::SetLogFileNum(unsigned int logNum) {
  string newLogFilePath;
  logFileNum = logNum % kMaxLogs;
  printf("Setting log to %u\n", logFileNum);
  newLogFilePath = logFileDir + "/live555-" + to_string(logFileNum) + ".log";
  currLogFilePath = newLogFilePath;
}

void _Log::PrepNextLog() {
  NextLogFile();

  printf("Now writing to next log.\n");
  if (access(currLogFilePath.c_str(), F_OK) == 0) {
    if (remove(currLogFilePath.c_str()) < 0) {
      perror("Couldn't remove old log while moving to next writing log.");
    }
    return;
  }
}

void _Log::NextLogFile() {
  SetLogFileNum(logFileNum + 1);
}

void _Log::BackupLog(const string backupFilePath) {
  lock_guard<mutex> backupLockGuard(logMutex);
  if (access(backupFilePath.c_str(), F_OK) == 0 && remove(backupFilePath.c_str()) < 0) {
    fprintf(stderr, "Couldn't remove old backup file in %s%s\n", backupFilePath, strerror(errno));
    return;
  }

  printf("Backing up rtspd log.\n");
  syslog(LOG_ERR, "RTSP issue triggered. Backing up logs in %s.", backupFilePath);

  // Backup new log file.
  ofstream backupLogFile(backupFilePath);

  if (!backupLogFile.is_open()) {
    perror("Couldn't open backup log file.");
    return;
  }

  unsigned int originalLogNum = logFileNum;

  NextLogFile();
  if (access(currLogFilePath.c_str(), F_OK) != 0) {
    // No wrap-around has ocurred yet
    SetLogFileNum(0);  // Start from log 0
  }

  // Copy all logs into backup log file
  for (int i = 0; i < kMaxLogs; i++) {
    if (access(currLogFilePath.c_str(), F_OK) == 0) {
      ifstream tmpLogFile(currLogFilePath);

      if (!tmpLogFile.is_open()) {
        fprintf(stderr, "Couldn't open temp log file %s in live555.%s", currLogFilePath, strerror(errno));
        break;
      }

      // Read temp log file contents
      string line;
      while (getline(tmpLogFile, line)) {
        backupLogFile << line << endl;
      }

      // Add new line to end of file contents to prevent line-chaining
      backupLogFile << '\n';

      tmpLogFile.close();
      NextLogFile();
    }
  }

  SetLogFileNum(originalLogNum);

  // Give everyone rw permissions for this backup file
  mode_t filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  if (chmod(backupFilePath.c_str(), filePerms) != 0) {
    perror("Error setting new backup log file permissions");
  }

  // Flush backup and close it
  backupLogFile.flush();
  backupLogFile.close();

  // Flush all stdout and stderr
  fflush(stderr);
  fflush(stdout);
}

void _Log::ReleaseFile(FILE* logFileHandle) {
  if (logFileHandle != NULL) {
    fflush(logFileHandle);
    struct stat file_stat;
    if (!currLogFilePath.empty()) {
      // The file is not stdout or stderr
      if (stat(currLogFilePath.c_str(), &file_stat) < 0) {
        perror("Error getting live555 log file stat.");
      }
      fflush(stdout);
      fflush(stderr);
      fclose(logFileHandle);
      if (file_stat.st_size >= (MAX_LOG_FILE_SIZE / kMaxLogs)) {
        PrepNextLog();
      }
    }
  }
}

void _Log::SetLevel(LogLevel level) {
  logLevel = level;
}

/***
 * Logging functions that take in file path and line number
***/

static string FormatLog(string fileName, int line, LogLevel logLevel, const char* format) {
  string _format = string(timestamp()) + string(" ") + string(format);
  _format += " [" + string(fileName) + string(":") + to_string(line) + "]";
  return LogLevelAsString(logLevel) + string(" ") + string(_format) + "\n";
}

void _Log::LogWithLevel(LogLevel level, const std::string filePath, int line, const char* format, va_list argptr) {
  lock_guard<mutex> logLockGuard(logMutex);
  if (logLevel >= level) {
    FILE* fileHandle = AcquireFile();
    if (fileHandle == NULL) {
      return;
    }
    vfprintf(fileHandle, FormatLog(filePath, line, level, format).c_str(), argptr);
    ReleaseFile(fileHandle);
    va_end(argptr);
  }
}

void _Log::Panic(const string filePath, int line, const char* format, ...) {
  va_list argptr; va_start(argptr, format);
  LogWithLevel(LogLevelPanic, filePath, line, format, argptr);
}

void _Log::Fatal(const string filePath, int line, const char* format, ...) {
  va_list argptr; va_start(argptr, format);
  LogWithLevel(LogLevelFatal, filePath, line, format, argptr);
}

void _Log::Error(const string filePath, int line, const char* format, ...) {
  va_list argptr; va_start(argptr, format);
  LogWithLevel(LogLevelError, filePath, line, format, argptr);
}

void _Log::Warning(const string filePath, int line, const char* format, ...) {
  va_list argptr; va_start(argptr, format);
  LogWithLevel(LogLevelWarning, filePath, line, format, argptr);
}

void _Log::Info(const string filePath, int line, const char* format, ...) {
  va_list argptr; va_start(argptr, format);
  LogWithLevel(LogLevelInfo, filePath, line, format, argptr);
}

void _Log::Debug(const string filePath, int line, const char* format, ...) {
  va_list argptr; va_start(argptr, format);
  LogWithLevel(LogLevelDebug, filePath, line, format, argptr);
}

void _Log::Trace(const string filePath, int line, const char* format, ...) {
  va_list argptr; va_start(argptr, format);
  LogWithLevel(LogLevelTrace, filePath, line, format, argptr);
}


