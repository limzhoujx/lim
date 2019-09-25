#ifndef LIM_STRING_H
#define LIM_STRING_H
#include <string>
#include <vector>
#include <stdint.h>

namespace lim {
#ifdef _WIN32
  #define strcasecmp stricmp
  #define strncasecmp strnicmp

  char *strcasestr(const char *haystack, const char *needle);
  void *memmem(const void *haystack, size_t haystack_length, const void *needle, size_t needle_length);

  #define fseeko64 _fseeki64
  #define ftello64 _ftelli64
#endif
  std::string &ltrim(std::string &str);
  std::string &rtrim(std::string &str);
  std::string &trim(std::string &str);

	std::string &toupper(std::string &str);
	std::string &tolower(std::string &str);
	
  std::vector<std::string> split(const std::string& str, const std::string& delim);
  std::string &replace(std::string &str, const std::string &src, const std::string &dest);
}
#endif

