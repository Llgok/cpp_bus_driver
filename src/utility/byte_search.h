/*
 * @Description: 字节缓冲区搜索工具
 * @Author: LILYGO_L
 * @Date: 2026-09-04 15:08:17
 * @LastEditTime: 2026-09-05 14:56:37
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace cpp_bus_driver {
namespace byte_search {

constexpr size_t kNotFound = static_cast<size_t>(-1);

/**
 * @brief 在字节缓冲区中查找指定文本
 * @param data 待搜索的字节缓冲区
 * @param data_size 字节缓冲区长度
 * @param text 待查找文本
 * @param text_size 待查找文本长度
 * @return 匹配文本的起始索引；参数无效或未找到时返回 kNotFound
 */
size_t FindText(
    const uint8_t* data, size_t data_size, const char* text, size_t text_size);

/**
 * @brief 检查字节缓冲区是否包含指定文本
 * @param data 待搜索的字节缓冲区
 * @param data_size 字节缓冲区长度
 * @param text 待查找文本
 * @param text_size 待查找文本长度
 * @return 找到文本时返回 true，否则返回 false
 */
inline bool ContainsText(
    const uint8_t* data, size_t data_size, const char* text, size_t text_size) {
  return FindText(data, data_size, text, text_size) != kNotFound;
}

/**
 * @brief 在字节缓冲区中查找以空字符结尾的字符数组
 * @tparam TextSize 字符数组长度，包含末尾空字符
 * @param data 待搜索的字节缓冲区
 * @param data_size 字节缓冲区长度
 * @param text 待查找字符数组
 * @return 匹配文本的起始索引；文本为空、数组未终止或未找到时返回 kNotFound
 */
template <size_t TextSize>
size_t FindText(
    const uint8_t* data, size_t data_size, const char (&text)[TextSize]) {
  // 数组容量可能大于实际文本长度，只搜索首个空字符之前的内容。
  size_t text_size = 0;
  while (text_size < TextSize && text[text_size] != '\0') {
    ++text_size;
  }
  if (text_size == TextSize) {
    return kNotFound;
  }
  return FindText(data, data_size, text, text_size);
}

/**
 * @brief 检查字节缓冲区是否包含以空字符结尾的字符数组
 * @tparam TextSize 字符数组长度，包含末尾空字符
 * @param data 待搜索的字节缓冲区
 * @param data_size 字节缓冲区长度
 * @param text 待查找字符数组
 * @return 找到文本时返回 true，否则返回 false
 */
template <size_t TextSize>
bool ContainsText(
    const uint8_t* data, size_t data_size, const char (&text)[TextSize]) {
  return FindText(data, data_size, text) != kNotFound;
}

}  // namespace byte_search
}  // namespace cpp_bus_driver
