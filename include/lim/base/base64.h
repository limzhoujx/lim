#ifndef _LIM_BASE_64_H_
#define _LIM_BASE_64_H_
#include <string>

namespace lim {
  std::string Base64Encode(const uint8_t *bytes_to_encode, int bytes_length_to_encode);
  std::string Base64Decode(const std::string &bytes_to_decode);
}
#endif

