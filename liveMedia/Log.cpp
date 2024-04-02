#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <stdlib.h>
#include <sys/time.h>                   // gettimeofday

#include "Log.h"

_Log Log;

using namespace std;

#define MAX_INT_BUFFER 30
string itoa(int integer) {
  char buffer[MAX_INT_BUFFER]; // Buffer to hold the formatted string
   snprintf(buffer, sizeof(buffer), "%d", integer);
   return string(buffer);
}

/**
 * Return current timestamp as string in syslog-ng format. Not re-entrant.
 */
static const char * timestamp()
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

void _Log::SetLevel(LogLevel level) {
  logLevel = level;
}

/***
 * Logging functions that take in file path and line number
***/

static string FormatLog(string fileName, int line, LogLevel logLevel, const char * format) {
  
  string _format = string(timestamp()) + string(" ") + string(format);
  _format += " [" + string(fileName) + string(":") + itoa(line) + "]";
  return LogLevelAsString(logLevel) + string(" ") + string(_format) + "\n"; 
}

void _Log::Panic(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelPanic  ) {
    va_list argptr; va_start(argptr, format);
    vfprintf(GetFile(), FormatLog(filePath, line, LogLevelPanic, format).c_str(), argptr);
    fflush(GetFile());
    va_end(argptr);
  }
}

void _Log::Fatal(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelFatal  ) {
    va_list argptr; va_start(argptr, format);
    vfprintf(GetFile(), FormatLog(filePath, line, LogLevelFatal, format).c_str(), argptr);
    fflush(GetFile());
    va_end(argptr);
  }
}

void _Log::Error(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelError  ) {
    va_list argptr; va_start(argptr, format);
    vfprintf(GetFile(), FormatLog(filePath, line, LogLevelError, format).c_str(), argptr);
    fflush(GetFile());
    va_end(argptr);
  }
}

void _Log::Warning(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelWarning  ) {
    va_list argptr; va_start(argptr, format);
    vfprintf(GetFile(), FormatLog(filePath, line, LogLevelWarning, format).c_str(), argptr);
    fflush(GetFile());
    va_end(argptr);
  }
}

void _Log::Info(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelInfo  ) {
    va_list argptr; va_start(argptr, format);
    vfprintf(GetFile(), FormatLog(filePath, line, LogLevelInfo, format).c_str(), argptr);
    fflush(GetFile());
    va_end(argptr);
  }
}

void _Log::Debug(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelDebug  ) {
    va_list argptr; va_start(argptr, format);
    vfprintf(GetFile(), FormatLog(filePath, line, LogLevelDebug, format).c_str(), argptr);
    fflush(GetFile());
    va_end(argptr);
  }
}

void _Log::Trace(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelTrace  ) {
    va_list argptr; va_start(argptr, format);
    vfprintf(GetFile(), FormatLog(filePath, line, LogLevelTrace, format).c_str(), argptr);
    fflush(GetFile());
    va_end(argptr);
  }
}

