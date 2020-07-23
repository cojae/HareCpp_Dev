#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <time.h>
#include <string>

namespace HareCpp {

#define LOG(a, b) LOG_FULL(a, b, __LINE__, __FILE__);

enum HARE_LOG_E {
  LOG_FATAL = 0,
  LOG_ERROR = 1,
  LOG_WARN = 2,
  LOG_INFO = 3,
  LOG_DETAILED = 4,
  LOG_TEST = 5,
  LOG_NONE = 6
};

extern void SET_DEBUG_LEVEL(int debugLevel);
extern void LOG_SIMPLE(int logLevel, const char* str);
extern void LOG_FULL(int logLevel, const char* str, int line, const char* file);
extern void LOG_FULL(int logLevel, std::string str, int line, const char* file);
extern void LOG_SIMPLE(int logLevel, std::string str);

}  // namespace HareCpp
#endif /*_LOGGER_H*/
