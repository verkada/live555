#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <stdlib.h>

using namespace std;

enum LogLevel {
  LogLevelPanic,
  LogLevelFatal,
  LogLevelError,
  LogLevelWarning,
  LogLevelInfo,
  LogLevelDebug,
  LogLevelTrace
};

#define MAX_INT_BUFFER 30
string itoa(int integer) {
  char buffer[MAX_INT_BUFFER]; // Buffer to hold the formatted string
   snprintf(buffer, sizeof(buffer), "%d", integer);
   return string(buffer);
}

static string LogLevelAsString(LogLevel logLevel);
static string FormatLog(const char* fileName, int line, LogLevel logLevel, const char * format) {
  
  string _format = string(format);
  if( fileName != NULL ) {
    _format += " [" + string(fileName) + string(":") + itoa(line) + "]";
  }

  return LogLevelAsString(LogLevelInfo) + string(" ") + string(_format) + "\n"; 
}

class _Log {
private:
  FILE* logFile;
  bool  logToStdout;

  FILE* GetFile();
public:
  _Log() : logFile(NULL), logToStdout(false) {    
  }

  ~_Log() {
    if( logFile != NULL ) {
      fclose(logFile);
    }    
  }

  void OutputToFile(const char* filePath);
  void OutputToStdout(bool toStdout);

  void Info(const char * format, ...);
  void Debug(const char * format, ...);

  void Info(const char* filePath, int line, const char * format, ...);
  void Debug(const char* filePath, int line, const char * format, ...);
};

string LogLevelAsString(LogLevel logLevel) {
  switch(logLevel) {
    case LogLevelPanic: return "PANI";
    case LogLevelFatal: return "FATA";
    case LogLevelError: return "ERRO";
    case LogLevelWarning: return "WARN";
    case LogLevelInfo: return "INFO";
    case LogLevelDebug: return "DEBU";
    case LogLevelTrace: return "TRAC";
  }
}

void _Log::OutputToFile(const char* filePath) {
  logFile = fopen(filePath, "w");
}

void _Log::OutputToStdout(bool toStdout) {
  logToStdout = toStdout;
}

FILE* _Log::GetFile() {
  if( logFile != NULL ) {    
    return logFile;
  }  else if (logToStdout) {
    return stdout;
  } else {
    return stderr;
  }
}

/***
 * Simple logging functions
***/

void _Log::Info(const char * format, ...) {
  va_list argptr; va_start(argptr, format);

  vfprintf(GetFile(), FormatLog(NULL, 0, LogLevelInfo, format).c_str(), argptr);
  fflush(GetFile());

  va_end(argptr);
}

void _Log::Debug(const char * format, ...) {
  va_list argptr; va_start(argptr, format);

  vfprintf(GetFile(), FormatLog(NULL, 0, LogLevelDebug, format).c_str(), argptr);
  fflush(GetFile());

  va_end(argptr);
}

/***
 * Logging functions that take in file path and line number
***/

void _Log::Info(const char* filePath, int line, const char * format, ...) {
  va_list argptr; va_start(argptr, format);

  vfprintf(GetFile(), FormatLog(filePath, line, LogLevelInfo, format).c_str(), argptr);
  fflush(GetFile());

  va_end(argptr);
}

void _Log::Debug(const char* filePath, int line, const char * format, ...) {
  va_list argptr; va_start(argptr, format);

  vfprintf(GetFile(), FormatLog(filePath, line, LogLevelDebug, format).c_str(), argptr);
  fflush(GetFile());

  va_end(argptr);
}


_Log Log;