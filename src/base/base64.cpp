#include <lim/base/base64.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace lim {
  std::string Base64Encode(const uint8_t *bytes_to_encode, int bytes_length_to_encode) {
	  const char kPaddingChar = '=';
	  const char kBase64CharSet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	  
    if (bytes_to_encode == NULL) {
      return "";
    }

    std::string bytes_encoded;
	  int i = 0, padding_char_length = 0;
	  while (i < bytes_length_to_encode) {
		  // encode 3 bytes at a time
      int chunk = 0;
		  chunk |= int(bytes_to_encode[i++]) << 16;
      if (i == bytes_length_to_encode) {
			  padding_char_length = 2;
      } else {
			  chunk |= int(bytes_to_encode[i++]) << 8;
        if (i == bytes_length_to_encode)
				  padding_char_length = 1;
        else
				  chunk |= int(bytes_to_encode[i++]);
      }

		  int j = (chunk & 0x00fc0000) >> 18;
      int k = (chunk & 0x0003f000) >> 12;
      int l = (chunk & 0x00000fc0) >> 6;
      int m = (chunk & 0x0000003f);
      bytes_encoded += kBase64CharSet[j];
      bytes_encoded += kBase64CharSet[k];

		  if (padding_char_length > 1)
        bytes_encoded += kPaddingChar;
		  else 
        bytes_encoded += kBase64CharSet[l];

		  if (padding_char_length > 0)
        bytes_encoded += kPaddingChar;
		  else 
        bytes_encoded += kBase64CharSet[m];
	  }

    bytes_encoded += '\0';
    return bytes_encoded;
}

  std::string Base64Decode(const std::string &bytes_to_decode) {
    std::string bytes_decoded;
    unsigned int byte = 0;
    int bits_length = 0;
    for (size_t i = 0; i < bytes_to_decode.length(); ++i) {
      int decoded_char;
      if (bytes_to_decode[i] >= 'A' && bytes_to_decode[i] <= 'Z')
        decoded_char = bytes_to_decode[i] - 'A';
      else if (bytes_to_decode[i] >= 'a' && bytes_to_decode[i] <= 'z')
        decoded_char = bytes_to_decode[i] - 'a' + 26;
      else if (bytes_to_decode[i] >= '0' && bytes_to_decode[i] <= '9')
        decoded_char = bytes_to_decode[i] - '0' + 52;
      else if (bytes_to_decode[i] == '+')
        decoded_char = 62;
      else if (bytes_to_decode[i] == '/')
        decoded_char = 63;
      else
        decoded_char = -1;

      if (decoded_char != -1) {
        byte = (byte << 6) | decoded_char;
        bits_length += 6;
        if (bits_length >= 8) {
          bits_length -= 8;
          bytes_decoded += (byte >> bits_length);
          byte &= (1 << bits_length) - 1;
        }
      }
    }
    return bytes_decoded;
  }
}

