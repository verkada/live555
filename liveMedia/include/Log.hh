#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <stdlib.h>

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
public:
  _Log() : logToStdout(false), logLevel(LogLevelDebug) {
    strcpy(logFilePath, "/mnt/ramdisk/live555.log");
  }

  ~_Log() {
  }

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