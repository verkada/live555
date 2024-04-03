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
  FILE*     logFile;
  bool      logToStdout;
  LogLevel  logLevel;

  FILE* GetFile();
public:
  _Log() : logFile(NULL), logToStdout(false), logLevel(LogLevelDebug) {
    OutputToFile("/tmp/rtsp.log");
  }

  ~_Log() {
    if( logFile != NULL ) {
      fclose(logFile);
    }    
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