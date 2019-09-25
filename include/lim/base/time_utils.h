#ifndef LIM_TIME_UTILS_H
#define LIM_TIME_UTILS_H
#include <stdint.h>
#include <string>

namespace lim {
  int64_t CurrentMilliTime();
  std::string TimeToString(uint64_t millisec, const char *format = NULL);
  std::string GetCurrentTimeString(const char* format = NULL);
}
#endif
