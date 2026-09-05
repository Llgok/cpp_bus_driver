/*
 * @Description: 字节缓冲区搜索工具实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 15:08:17
 * @LastEditTime: 2026-09-05 14:56:35
 * @License: GPL 3.0
 */
#include "utility/byte_search.h"

#include <cstring>

namespace cpp_bus_driver {
namespace byte_search {

size_t FindText(
    const uint8_t* data, size_t data_size, const char* text, size_t text_size) {
  if (data == nullptr || text == nullptr || text_size == 0 ||
      data_size < text_size) {
    return kNotFound;
  }

  const uint8_t* current = data;
  const uint8_t* const last = data + data_size - text_size;
  const uint8_t first_byte = static_cast<uint8_t>(text[0]);

  while (current <= last) {
    const size_t searchable_size = static_cast<size_t>(last - current) + 1;
    const void* found = std::memchr(current, first_byte, searchable_size);
    if (found == nullptr) {
      return kNotFound;
    }

    const uint8_t* candidate = static_cast<const uint8_t*>(found);
    if (text_size == 1 ||
        std::memcmp(candidate + 1, text + 1, text_size - 1) == 0) {
      return static_cast<size_t>(candidate - data);
    }
    current = candidate + 1;
  }

  return kNotFound;
}

}  // namespace byte_search
}  // namespace cpp_bus_driver
