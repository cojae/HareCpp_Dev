#include "Logger.hpp"
#include <stdio.h>
#include <time.h>

const char* COL_NORM = "\x1B[0m";     // Normal
const char* COL_FATAL = "\x1B[31m";   // Red
const char* COL_ERROR = "\x1B[91m";   // Light Red
const char* COL_INFO = "\x1B[37m";    // White
const char* COL_WARN = "\x1B[33m";    // Yellow
const char* COL_DETAIL = "\x1B[32m";  // Green
const char* COL_TEST = "\x1B[34m";    // Blue

namespace HareCpp {
static int dbgLevel = LOG_TEST;

void SET_DEBUG_LEVEL(int debugLevel) {
  if (debugLevel > LOG_NONE || debugLevel < LOG_FATAL) {
    LOG_SIMPLE(LOG_FATAL, "Invalid Log Level");
  } else {
    dbgLevel = debugLevel;
  }
}

void GET_LOG_DISPLAY(int logLevel, const char*& logColor, std::string& level) {
  // Get log color
  switch (logLevel) {
    case LOG_FATAL:
      logColor = COL_FATAL;
      level = "FATAL";
      break;
    case LOG_ERROR:
      logColor = COL_ERROR;
      level = "ERROR";
      break;
    case LOG_WARN:
      logColor = COL_WARN;
      level = "WARN";
      break;
    case LOG_INFO:
      logColor = COL_INFO;
      level = "INFO";
      break;
    case LOG_DETAILED:
      logColor = COL_DETAIL;
      level = "DETAIL";
      break;
    case LOG_TEST:
      logColor = COL_TEST;
      level = "TEST";
      break;
    case LOG_NONE:
      return;
    default:
      return;
  }
}

void LOG_SIMPLE(int logLevel, const char* message) {
  if (logLevel <= dbgLevel && dbgLevel != LOG_NONE) {
    const char* logColor;
    std::string level;
    GET_LOG_DISPLAY(logLevel, logColor, level);
    printf("\e[1;34mHareCpp:\e[0m\t%s%s: %s\e[0m\n", logColor, level.c_str(),
           message);
  }
}
void LOG_FULL(int logLevel, const char* message, int line, const char* file) {
  /**
   * Are we in the proper debug level set by user or default (LOG_INFO)
   */
  if (logLevel <= dbgLevel && dbgLevel != LOG_NONE) {
    const char* logColor;
    std::string level;
    GET_LOG_DISPLAY(logLevel, logColor, level);
    char callLocation[32];
    snprintf(callLocation, 32, "%d:%s", line, file);
    printf("\e[1;34mHareCpp: %-32s\e[0m %s%s: %s\e[0m\n", callLocation,
           logColor, level.c_str(), message);
  }
}

void LOG_FULL(int logLevel, std::string message, int line, const char* file) {
  LOG_FULL(logLevel, message.c_str(), line, file);
}
void LOG_SIMPLE(int logLevel, std::string message) {
  LOG_SIMPLE(logLevel, message.c_str());
}

}  // namespace HareCpp
