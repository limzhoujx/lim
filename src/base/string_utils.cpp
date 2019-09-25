#include <lim/base/string_utils.h>
#include <iostream>
#include <vector>
#include <iterator>
#include <regex>
#include <algorithm> 
#include <functional>
#include <cctype>
#include <locale>
#include <string.h>

namespace lim {
#ifdef _WIN32
  char *strcasestr(const char *haystack, const char *needle) {
    if (haystack == NULL || needle == NULL)
      return NULL;

    if (strlen(haystack) == 0)
      return (char*)haystack;

    while (strncasecmp(haystack, needle, strlen(needle))) {
      if (*haystack++ == '\0')
        return (char*)NULL;
    }
    return (char*)haystack;
  }

  void *memmem(const void *haystack, size_t haystack_length, const void *needle, size_t needle_length) {
    const unsigned char *y = (const unsigned char *)haystack;
    const unsigned char *x = (const unsigned char *)needle;

    if (needle_length > haystack_length)
      return NULL;

    size_t j, k, l;
    if (x[0] == x[1]) {
      k = 2;
      l = 1;
    } else {
      k = 1;
      l = 2;
    }

    j = 0;
    while (j <= haystack_length - needle_length) {
      if (x[1] != y[j + 1]) {
        j += k;
      } else {
        if (!memcmp(x + 2, y + j + 2, needle_length - 2) && x[0] == y[j])
          return (void *)&y[j];
        j += l;
      }
    }
    return NULL;
  }
#endif

  std::string &ltrim(std::string &str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return str;
  }

  std::string &rtrim(std::string &str) {
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());
    return str;
  }

  std::string &trim(std::string &str) {
    return ltrim(rtrim(str));
  }

	std::string &toupper(std::string &str) {
		std::transform(str.begin(), str.end(), str.begin(), std::ptr_fun<int, int>(std::toupper));
		return str;
	}
	
	std::string &tolower(std::string &str) {
		std::transform(str.begin(), str.end(), str.begin(), std::ptr_fun<int, int>(std::tolower));
		return str;
	}
	
  std::vector<std::string> split(const std::string& str, const std::string& delim) {
    /*std::regex reg{ delim };
    return std::vector<std::string> {
      std::sregex_token_iterator(str.begin(), str.end(), reg, -1),
        std::sregex_token_iterator()
    };*/
    std::vector<std::string> splits;
    int start_position = 0;
    int end_position = str.find(delim);
    while (end_position != -1) {
			splits.push_back(str.substr(start_position, end_position - start_position));
      start_position = end_position + delim.length();
      end_position = str.find(delim, start_position);
    }
		splits.push_back(str.substr(start_position, end_position));
		return splits;
  }

  std::string &replace(std::string &str, const std::string &src, const std::string &dest) {
    while (true) {
      int index;
      if ((index = str.find(src)) != -1)
        str.replace(index, src.length(), dest);
      else
        break;
    }
    return str;
  }
}

