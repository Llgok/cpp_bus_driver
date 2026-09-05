/*
 * @Description: cpp_bus_driver 有界数值转换接口
 * @Author: LILYGO_L
 * @Date: 2026-09-04 16:32:00
 * @LastEditTime: 2026-09-04 16:32:00
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace cpp_bus_driver {
namespace numeric_conversion {

// 浮点转换最多接受 63 个字符，不分配内存、不依赖区域设置。
// 拒绝非有限数、溢出和非零下溢（含非正规数）；失败不修改输出。
// 使用有界十进制运算，面向传感器数据，不承诺任意小数的严格正确舍入。

/**
 * @brief 将有界字符序列解析为 int8_t
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果未溢出时返回 true
 */
bool ParseInt8(const char* data, size_t length, int8_t* output);

/**
 * @brief 将有界字符序列解析为 int32_t
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果未溢出时返回 true
 */
bool ParseInt32(const char* data, size_t length, int32_t* output);

/**
 * @brief 将有界字符序列解析为 int64_t
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果未溢出时返回 true
 */
bool ParseInt64(const char* data, size_t length, int64_t* output);

/**
 * @brief 将有界字符序列解析为 uint8_t
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果未溢出时返回 true
 */
bool ParseUint8(const char* data, size_t length, uint8_t* output);

/**
 * @brief 将有界字符序列解析为 uint16_t
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果未溢出时返回 true
 */
bool ParseUint16(const char* data, size_t length, uint16_t* output);

/**
 * @brief 将有界字符序列解析为 uint32_t
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果未溢出时返回 true
 */
bool ParseUint32(const char* data, size_t length, uint32_t* output);

/**
 * @brief 将有界十六进制字符序列解析为 uint8_t
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果未溢出时返回 true
 */
bool ParseHexUint8(const char* data, size_t length, uint8_t* output);

/**
 * @brief 将有界字符序列解析为 float
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果为有限值时返回 true
 */
bool ParseFloat(const char* data, size_t length, float* output);

/**
 * @brief 将有界字符序列解析为 double
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果为有限值时返回 true
 */
bool ParseDouble(const char* data, size_t length, double* output);

/**
 * @brief 将完整字符串解析为 float
 * @param input 待解析字符串
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果为有限值时返回 true
 */
inline bool ParseFloat(const std::string& input, float* output) {
  return ParseFloat(input.data(), input.size(), output);
}

/**
 * @brief 将完整字符串解析为 double
 * @param input 待解析字符串
 * @param output 解析结果输出指针
 * @return 输入完整有效且结果为有限值时返回 true
 */
inline bool ParseDouble(const std::string& input, double* output) {
  return ParseDouble(input.data(), input.size(), output);
}

}  // namespace numeric_conversion
}  // namespace cpp_bus_driver
