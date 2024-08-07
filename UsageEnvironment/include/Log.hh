/*
 * Logging class for handling live555 logging
 */
#ifndef _LOG_HH
#define _LOG_HH

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <mutex>

enum LogLevel {
  LogLevelPanic,
  LogLevelFatal,
  LogLevelError,
  LogLevelWarning,
  LogLevelInfo,
  LogLevelDebug,
  LogLevelTrace
};

class _Log {
private:
  const unsigned int kMaxLogs = 6;

  std::string    currLogFilePath;
  std::string    logFileDir;
  std::mutex     logMutex;
  unsigned int   logFileNum;
  bool           logToStdout;
  LogLevel       logLevel;

  FILE* AcquireFile();
  void  ReleaseFile(FILE* logFileHandle);
  void  NextLogFile();
  void  PreviousLogFile();
  void  SetLogFileNum(unsigned int logNum);
  void  PrepNextLog();
  void  LogWithLevel(LogLevel level, const std::string filePath, int line, const char* format, va_list argptr);
public:
  _Log(const std::string logDirectory = "");
  ~_Log() {}

  void BackupLog(const std::string backupFileName);
  void SetLevel(LogLevel level);
  void SetTmpLogDir(std::string logDirectory);

  void Panic(const std::string filePath, int line, const char* format, ...);
  void Fatal(const std::string filePath, int line, const char* format, ...);
  void Error(const std::string filePath, int line, const char* format, ...);
  void Warning(const std::string filePath, int line, const char* format, ...);
  void Info(const std::string filePath, int line, const char* format, ...);
  void Debug(const std::string filePath, int line, const char* format, ...);
  void Trace(const std::string filePath, int line, const char* format, ...);
};

extern _Log Log;

#endif