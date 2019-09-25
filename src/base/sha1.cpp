/*
Copyright (c) 2011, Micael Hildenborg
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Micael Hildenborg nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY Micael Hildenborg ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Micael Hildenborg BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include<lim/base/sha1.h>

namespace lim {
  inline const unsigned int Rol(const unsigned int value, const unsigned int steps) {
    return ((value << steps) | (value >> (32 - steps)));
  }

  // Sets the first 16 integers in the buffert to zero.
  // Used for clearing the W buffert.
  inline void ClearWBuffert(unsigned int* buffer) {
    for (int pos = 16; --pos >= 0;) {
      buffer[pos] = 0;
    }
  }

  void InnerHash(unsigned int* result, unsigned int* w) {
    unsigned int a = result[0];
    unsigned int b = result[1];
    unsigned int c = result[2];
    unsigned int d = result[3];
    unsigned int e = result[4];

    int round = 0;

#define sha1macro(func,val) \
		{ \
              const unsigned int t = Rol(a, 5) + (func) + e + val + w[round]; \
			e = d; \
			d = c; \
			c = Rol(b, 30); \
			b = a; \
			a = t; \
		}

    while (round < 16) {
      sha1macro((b & c) | (~b & d), 0x5a827999)
        ++round;
    }
    while (round < 20) {
      w[round] = Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
      sha1macro((b & c) | (~b & d), 0x5a827999)
        ++round;
    }
    while (round < 40) {
      w[round] = Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
      sha1macro(b ^ c ^ d, 0x6ed9eba1)
        ++round;
    }
    while (round < 60) {
      w[round] = Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
      sha1macro((b & c) | (b & d) | (c & d), 0x8f1bbcdc)
        ++round;
    }
    while (round < 80) {
      w[round] = Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
      sha1macro(b ^ c ^ d, 0xca62c1d6)
        ++round;
    }

#undef sha1macro

    result[0] += a;
    result[1] += b;
    result[2] += c;
    result[3] += d;
    result[4] += e;
  }

  void Sha1(const void* src, const int byte_length, unsigned char* hash) {
    // Init the result array.
    unsigned int result[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };

    // Cast the void src pointer to be the byte array we can work with.
    const unsigned char* sarray = (const unsigned char*)src;

    // The reusable round buffer
    unsigned int w[80];

    // Loop through all complete 64byte blocks.
    const int end_of_full_blocks = byte_length - 64;
    int end_current_block;
    int current_block = 0;

    while (current_block <= end_of_full_blocks){
      end_current_block = current_block + 64;

      // Init the round buffer with the 64 byte block data.
      for (int round_pos = 0; current_block < end_current_block; current_block += 4) {
        // This line will swap endian on big endian and keep endian on little endian.
        w[round_pos++] = (unsigned int)sarray[current_block + 3]
          | (((unsigned int)sarray[current_block + 2]) << 8)
          | (((unsigned int)sarray[current_block + 1]) << 16)
          | (((unsigned int)sarray[current_block]) << 24);
      }
      InnerHash(result, w);
    }

    // Handle the last and not full 64 byte block if existing.
    end_current_block = byte_length - current_block;
    ClearWBuffert(w);
    int last_block_bytes = 0;
    for (; last_block_bytes < end_current_block; ++last_block_bytes) {
      w[last_block_bytes >> 2] |= (unsigned int)sarray[last_block_bytes + current_block] << ((3 - (last_block_bytes & 3)) << 3);
    }
    w[last_block_bytes >> 2] |= 0x80 << ((3 - (last_block_bytes & 3)) << 3);
    if (end_current_block >= 56) {
      InnerHash(result, w);
      ClearWBuffert(w);
    }
    w[15] = byte_length << 3;
    InnerHash(result, w);

    // Store hash in result pointer, and make sure we get in in the correct order on both endian models.
    for (int hash_byte = 20; --hash_byte >= 0;) {
      hash[hash_byte] = (result[hash_byte >> 2] >> (((3 - hash_byte) & 0x3) << 3)) & 0xff;
    }
  }

  void ToHexString(const unsigned char* hash, char* hexstring){
    const char hex_digits[] = { "0123456789abcdef" };

    for (int hash_byte = 20; --hash_byte >= 0;) {
      hexstring[hash_byte << 1] = hex_digits[(hash[hash_byte] >> 4) & 0xf];
      hexstring[(hash_byte << 1) + 1] = hex_digits[hash[hash_byte] & 0xf];
    }
    hexstring[40] = 0;
  }
}
