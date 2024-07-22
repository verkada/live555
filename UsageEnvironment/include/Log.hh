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
#include <limits.h>

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
  char      logFilePath[256];
  bool      logToStdout;
  LogLevel  logLevel;

  FILE* AcquireFile();
  void  ReleaseFile(FILE* logFileHandle);
  void  BackupLog();
  void  RemoveOldBackup();
public:
  _Log() : logToStdout(false), logLevel(LogLevelDebug) {
# ifdef LOG_FILE_TMP_DIR
    strcpy(logFilePath, LOG_FILE_TMP_DIR "/live555.log");
# else
  strcpy(logFilePath, "");
# endif
#ifdef ERROR_MESSAGE_BACKUP_TRIGGER
    fprintf(stdout, "Checking for error %s", ERROR_MESSAGE_BACKUP_TRIGGER);
#endif
  }

  ~_Log() {}

  void OutputToFile(const char* filePath);
  void OutputToStdout(bool toStdout);
  void SetLevel(LogLevel level);

  void Panic(const char* filePath, int line, const char * format, ...);
  void Fatal(const char* filePath, int line, const char * format, ...);
  void Error(const char* filePath, int line, const char * format, ...);
  void Warning(const char* filePath, int line, const char * format, ...);
  void Info(const char* filePath, int line, const char * format, ...);
  void Debug(const char* filePath, int line, const char * format, ...);
  void Trace(const char* filePath, int line, const char * format, ...);
};

extern _Log Log;

#endif