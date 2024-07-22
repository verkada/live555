#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>                   // gettimeofday
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include <Log.hh>

_Log Log;

using namespace std;

#define MAX_INT_BUFFER 30
#define MAX_LOG_FILE_SIZE (1 * 1024 * 1024) // Max log size 1MB
#define MAX_LOG_LEN 200

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
    case LogLevelPanic: return "PANIC";
    case LogLevelFatal: return "FATAL";
    case LogLevelError: return "ERROR";
    case LogLevelWarning: return "WARN";
    case LogLevelInfo: return "INFO";
    case LogLevelDebug: return "DEBUG";
    case LogLevelTrace: return "TRACE";
  }
}

void _Log::OutputToFile(const char* filePath) {
  strcpy(logFilePath, filePath);
}

void _Log::OutputToStdout(bool toStdout) {
  logToStdout = toStdout;
}

FILE* _Log::AcquireFile() {
  if( strcmp(logFilePath, "") == 0 ) {
    if (logToStdout) {
      return stdout;
    } else {
      return stderr;
    }
  } else {
    FILE* logFileHandle = fopen(logFilePath, "a");
    if( logFileHandle == NULL ) {
#ifdef LOG_FILE_TMP_DIR
      strcpy(logFilePath, LOG_FILE_TMP_DIR "/live555.log");
      logFileHandle = fopen(logFilePath, "a");
#endif
    }

    return logFileHandle;
  }
}

void _Log::RemoveOldBackup() {
#ifdef LOG_FILE_BACKUP_DIR
  DIR* backupDir;
  struct dirent* dirInfo;

  // Look for old backup log file and remove it.
  backupDir = opendir(LOG_FILE_BACKUP_DIR);
  if( backupDir ) {
    while( (dirInfo = readdir(backupDir)) != NULL ) {
      if( strncmp(dirInfo->d_name, "live555", 7) == 0 ) {
        char rmBuf[PATH_MAX];

        // Attempt to remove using system call (unlink() and remove() complain /mnt/config is Read-Only)
        fprintf(stdout, "Attempting to remove old backup log.\n");
        snprintf(rmBuf, sizeof(rmBuf), "rm " LOG_FILE_BACKUP_DIR "/%s", dirInfo->d_name);
        system(rmBuf);
        break;
      }
    }
  }
  closedir(backupDir);
#endif
}

void _Log::BackupLog() {
#if defined(LOG_FILE_BACKUP_DIR) && defined(LOG_FILE_TMP_DIR)
  RemoveOldBackup();  // Remove old backup log if it exists (flash memory should never be at risk of getting filled up)
  fprintf(stdout, "Attempting to backup log.\n");

  // Backup new log file.
  char newPath[PATH_MAX];
  FILE* backupLogFile;
  FILE* tmpLogFile;
  size_t logFileSize;
  char* fileBuffer;

  // Formulate new path
  snprintf(newPath, sizeof(newPath), LOG_FILE_BACKUP_DIR "/live555-%lu.log", time(NULL));
  fprintf(stdout, "renaming %s to %s\n", logFilePath, newPath);

  // Open backup file and current logging file
  backupLogFile = fopen(newPath, "w");
  tmpLogFile = fopen(logFilePath, "r");

  // Get file size and create buffer for copying
  fseek(tmpLogFile, 0, SEEK_END);
  logFileSize = ftell(tmpLogFile);
  fileBuffer = (char *)malloc(logFileSize);
  rewind(tmpLogFile);

  if (fileBuffer == NULL) {
    fprintf(stderr, "Couldn't create temp file read buffer.\n");
    goto _clean_up_file_copy;
  }

  {
    // Read temp log file contents
    size_t bytesRead = fread(fileBuffer, sizeof(char), logFileSize, tmpLogFile);
    if (bytesRead != logFileSize) {
      perror("Read incorrect number of bytes from tmp log file.");
      goto _clean_up_file_copy;
    }
    {
      // Write temp log file contents to backup log file
      size_t bytesWritten = fwrite(fileBuffer, sizeof(char), logFileSize, backupLogFile);
      if (bytesWritten != logFileSize) {
        perror("Wrote incorrect number of bytes to backup log file.");
        goto _clean_up_file_copy;
      }
      {
        // Give everyone rw permissions for this backup file
        mode_t filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        if (chmod(newPath, filePerms) != 0) {
          perror("Error setting file permissions");
          goto _clean_up_file_copy;
        }
      }
    }
  }
_clean_up_file_copy:
  // Flush all streams
  fflush(stderr);
  fflush(stdout);
  fflush(backupLogFile);

  // Close all files / free memory
  fclose(tmpLogFile);
  fclose(backupLogFile);
  free(fileBuffer);
#endif
}

void _Log::ReleaseFile(FILE* logFileHandle) {
  if( logFileHandle != NULL ) {
    fflush(logFileHandle);
    struct stat file_stat;
    if( strcmp(logFilePath, "") != 0 ) {
      // The file is not stdout or stderr, so we can back it up if needed
      // then close it
      if( stat(logFilePath, &file_stat) < 0 ) {
        perror("Error getting live555 log file stat.");
        goto _dealloc_log_file_struct;
      }
      if( file_stat.st_size >= MAX_LOG_FILE_SIZE ) {
#ifdef LOG_FILE_TMP_DIR
        if( remove(logFilePath) < 0 ) {
          perror("Error removing live555 tmp log in " LOG_FILE_TMP_DIR);
        }
#endif
      }
_dealloc_log_file_struct:
      fclose(logFileHandle);
    }
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
    FILE* fileHandle = AcquireFile();
    vfprintf(fileHandle, FormatLog(filePath, line, LogLevelPanic, format).c_str(), argptr);
    ReleaseFile(fileHandle);
    va_end(argptr);
  }
}

void _Log::Fatal(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelFatal  ) {
    va_list argptr; va_start(argptr, format);
    FILE* fileHandle = AcquireFile();
    vfprintf(fileHandle, FormatLog(filePath, line, LogLevelFatal, format).c_str(), argptr);
    ReleaseFile(fileHandle);
    va_end(argptr);
  }
}

void _Log::Error(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelError ) {
    va_list argptr; va_start(argptr, format);
    FILE* fileHandle = AcquireFile();
    char errorLog[MAX_LOG_LEN];
    vsnprintf(errorLog, MAX_LOG_LEN, FormatLog(filePath, line, LogLevelError, format).c_str(), argptr);
    fprintf(fileHandle, "%s", errorLog);
    ReleaseFile(fileHandle);
#ifdef ERROR_MESSAGE_BACKUP_TRIGGER
    if( strstr(errorLog, ERROR_MESSAGE_BACKUP_TRIGGER) ) {
      syslog(LOG_ERR, "RTSP issue triggered.");
      BackupLog();
    } else {
      fprintf(stdout, "%s - log DID NOT trigger backup.\n", errorLog);
    }
#endif
    va_end(argptr);
  }
}

void _Log::Warning(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelWarning ) {
    va_list argptr; va_start(argptr, format);
    FILE* fileHandle = AcquireFile();
    vfprintf(fileHandle, FormatLog(filePath, line, LogLevelWarning, format).c_str(), argptr);
    ReleaseFile(fileHandle);
    va_end(argptr);
  }
}

void _Log::Info(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelInfo  ) {
    va_list argptr; va_start(argptr, format);
    FILE* fileHandle = AcquireFile();
    vfprintf(fileHandle, FormatLog(filePath, line, LogLevelInfo, format).c_str(), argptr);
    ReleaseFile(fileHandle);
    va_end(argptr);
  }
}

void _Log::Debug(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelDebug ) {
    va_list argptr; va_start(argptr, format);
    FILE* fileHandle = AcquireFile();
    vfprintf(fileHandle, FormatLog(filePath, line, LogLevelDebug, format).c_str(), argptr);
    ReleaseFile(fileHandle);
    va_end(argptr);
  }
}

void _Log::Trace(const char* filePath, int line, const char * format, ...) {
  if( logLevel >= LogLevelTrace ) {
    va_list argptr; va_start(argptr, format);
    FILE* fileHandle = AcquireFile();
    vfprintf(fileHandle, FormatLog(filePath, line, LogLevelTrace, format).c_str(), argptr);
    ReleaseFile(fileHandle);
    va_end(argptr);
  }
}


