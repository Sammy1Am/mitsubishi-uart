#pragma once

#include "mitsubishi_uart.h"

namespace esphome {
namespace mitsubishi_uart {

class MUARTUtils {
 public:
  static std::string DecodeNBitString(const uint8_t data[], size_t dataLength, size_t wordSize) {
    auto resultLength = (dataLength / wordSize) + (dataLength % wordSize != 0);
    auto result = std::string();

    for (int i = 0; i < resultLength; i++) {
      auto bits = BitSlice(data, i * wordSize, ((i + 1) * wordSize) - 1);
      if (bits <= 0x1F) bits += 0x40;
      result += (char)bits;
    }

    return result;
  }

 private:
  static uint64_t BitSlice(const uint8_t ds[], size_t start, size_t end) {
    // Lazies! https://stackoverflow.com/a/25297870/1817097
    uint64_t s = 0;
    size_t i, n = (end - 1) / 8;
    for(i = 0; i <= n; ++i)
      s = (s << 8) + ds[i];
    s >>= (n+1) * 8 - end;
    uint64_t mask = (((uint64_t)1) << (end - start + 1))-1;//len = end - start + 1
    s &= mask;
    return s;
  }
};

}  // namespace mitsubishi_uart
}  // namespace esphome
