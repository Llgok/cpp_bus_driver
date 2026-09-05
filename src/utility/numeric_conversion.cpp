/*
 * @Description: cpp_bus_driver 有界数值转换实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 16:32:00
 * @LastEditTime: 2026-09-05 14:56:39
 * @License: GPL 3.0
 */
#include "utility/numeric_conversion.h"

#include <cmath>
#include <limits>

namespace cpp_bus_driver {
namespace numeric_conversion {
namespace {

// 浮点字段允许的最大字符数，GNSS 协议中的合法数值远小于该上限。
constexpr size_t kMaxFloatingPointLength = 63;

/**
 * @brief 将十进制数字字符转换为数值
 * @param input 输入字符
 * @param output 数值输出指针
 * @return 输入为十进制数字时返回 true
 */
bool DecimalDigitToValue(char input, uint8_t* output) {
  if (output == nullptr || input < '0' || input > '9') {
    return false;
  }
  *output = static_cast<uint8_t>(input - '0');
  return true;
}

/**
 * @brief 将十六进制数字字符转换为数值
 * @param input 输入字符
 * @param output 数值输出指针
 * @return 输入为十六进制数字时返回 true
 */
bool HexDigitToValue(char input, uint8_t* output) {
  if (output == nullptr) {
    return false;
  }
  if (input >= '0' && input <= '9') {
    *output = static_cast<uint8_t>(input - '0');
    return true;
  }
  if (input >= 'A' && input <= 'F') {
    *output = static_cast<uint8_t>(input - 'A' + 10);
    return true;
  }
  if (input >= 'a' && input <= 'f') {
    *output = static_cast<uint8_t>(input - 'a' + 10);
    return true;
  }
  return false;
}

/**
 * @brief 在指定进制和上限内解析无符号整数
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param base 数值进制，仅支持 10 和 16
 * @param maximum 允许的最大值
 * @param output 解析结果输出指针
 * @return 输入完整有效且未溢出时返回 true
 */
bool ParseUnsigned(const char* data, size_t length, uint8_t base,
    uint64_t maximum, uint64_t* output) {
  if (data == nullptr || output == nullptr || length == 0 ||
      (base != 10 && base != 16)) {
    return false;
  }

  size_t index = 0;
  if (data[index] == '+') {
    ++index;
    if (index == length) {
      return false;
    }
  } else if (data[index] == '-') {
    return false;
  }

  uint64_t value = 0;
  for (; index < length; ++index) {
    uint8_t digit = 0;
    const bool digit_valid = base == 10
                                 ? DecimalDigitToValue(data[index], &digit)
                                 : HexDigitToValue(data[index], &digit);
    if (!digit_valid || digit >= base || digit > maximum ||
        value > (maximum - digit) / base) {
      return false;
    }
    value = value * base + digit;
  }

  *output = value;
  return true;
}

/**
 * @brief 在指定上下限内解析有符号十进制整数
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param minimum 允许的最小值
 * @param maximum 允许的最大值
 * @param output 解析结果输出指针
 * @return 输入完整有效且未溢出时返回 true
 */
bool ParseSigned(const char* data, size_t length, int64_t minimum,
    int64_t maximum, int64_t* output) {
  if (data == nullptr || output == nullptr || length == 0 || minimum > 0 ||
      maximum < 0) {
    return false;
  }

  bool negative = false;
  size_t index = 0;
  if (data[index] == '-' || data[index] == '+') {
    negative = data[index] == '-';
    ++index;
    if (index == length) {
      return false;
    }
  }

  const uint64_t positive_limit = static_cast<uint64_t>(maximum);
  const uint64_t negative_limit = static_cast<uint64_t>(-(minimum + 1)) + 1U;
  uint64_t magnitude = 0;
  // 符号只允许出现在首位，避免内部无符号解析再次接受正号。
  if (data[index] < '0' || data[index] > '9') {
    return false;
  }
  if (!ParseUnsigned(data + index, length - index, 10,
          negative ? negative_limit : positive_limit, &magnitude)) {
    return false;
  }

  if (negative) {
    if (magnitude == negative_limit) {
      *output = minimum;
    } else {
      *output = -static_cast<int64_t>(magnitude);
    }
  } else {
    *output = static_cast<int64_t>(magnitude);
  }
  return true;
}

/**
 * @brief 检查字段是否为普通十进制浮点语法
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @return 仅包含符号、十进制数字、小数点和合法指数时返回 true
 */
bool IsDecimalFloatingPoint(const char* data, size_t length) {
  if (data == nullptr || length == 0) {
    return false;
  }
  size_t index = 0;
  if (data[index] == '+' || data[index] == '-') {
    ++index;
  }
  bool mantissa_digit_found = false;
  bool decimal_point_found = false;
  for (; index < length; ++index) {
    const char value = data[index];
    if (value >= '0' && value <= '9') {
      mantissa_digit_found = true;
      continue;
    }
    if (value == '.' && !decimal_point_found) {
      decimal_point_found = true;
      continue;
    }
    if ((value == 'e' || value == 'E') && mantissa_digit_found) {
      ++index;
      if (index < length && (data[index] == '+' || data[index] == '-')) {
        ++index;
      }
      if (index == length) {
        return false;
      }
      for (; index < length; ++index) {
        if (data[index] < '0' || data[index] > '9') {
          return false;
        }
      }
      return true;
    }
    return false;
  }
  return mantissa_digit_found;
}

/**
 * @brief 使用有界十进制运算转换浮点数，不调用可能申请临时内存的文本转换函数
 * @param data 字符序列首地址
 * @param length 字符序列长度
 * @param output 浮点结果输出指针
 * @return 完整语法有效且结果处于目标类型正常范围时返回 true
 */
template <typename Float>
bool ParseDecimal(const char* data, size_t length, Float* output) {
  if (output == nullptr || length > kMaxFloatingPointLength ||
      !IsDecimalFloatingPoint(data, length)) {
    return false;
  }
  size_t index = 0;
  const bool negative = data[0] == '-';
  if (data[0] == '-' || data[0] == '+') {
    ++index;
  }
  long double value = 0.0L;
  int fractional_digits = 0;
  bool after_point = false;
  for (; index < length && data[index] != 'e' && data[index] != 'E'; ++index) {
    if (data[index] == '.') {
      after_point = true;
      continue;
    }
    value = value * 10.0L + static_cast<unsigned int>(data[index] - '0');
    if (after_point) {
      ++fractional_digits;
    }
  }
  int exponent = 0;
  if (index < length) {
    ++index;
    const bool exponent_negative = data[index] == '-';
    if (data[index] == '-' || data[index] == '+') {
      ++index;
    }
    for (; index < length; ++index) {
      // 超大指数只需保留饱和值，避免整数溢出和无界运算。
      if (exponent < 10000) {
        exponent = exponent * 10 + (data[index] - '0');
        if (exponent > 10000) {
          exponent = 10000;
        }
      }
    }
    if (exponent_negative) {
      exponent = -exponent;
    }
  }
  if (value == 0.0L) {
    *output = negative ? -static_cast<Float>(0) : static_cast<Float>(0);
    return true;
  }
  exponent -= fractional_digits;
  // 字段最多 63 字符，尾数的数量级不会超过该长度。
  if (exponent > std::numeric_limits<Float>::max_exponent10 + 1 ||
      exponent < std::numeric_limits<Float>::min_exponent10 -
                     static_cast<int>(kMaxFloatingPointLength) - 1) {
    return false;
  }
  // 分块缩放限制运算次数，不生成超大中间幂，也不使用区域设置或堆内存。
  while (exponent >= 16) {
    value *= 1.0e16L;
    exponent -= 16;
  }
  while (exponent <= -16) {
    value /= 1.0e16L;
    exponent += 16;
  }
  while (exponent > 0) {
    value *= 10.0L;
    --exponent;
  }
  while (exponent < 0) {
    value /= 10.0L;
    ++exponent;
  }
  if (!std::isfinite(value) ||
      value > static_cast<long double>(std::numeric_limits<Float>::max()) ||
      value < static_cast<long double>(std::numeric_limits<Float>::min())) {
    return false;
  }
  const Float converted = static_cast<Float>(value);
  if (!std::isfinite(converted) || converted == static_cast<Float>(0)) {
    return false;
  }
  *output = negative ? -converted : converted;
  return true;
}

}  // namespace

bool ParseInt8(const char* data, size_t length, int8_t* output) {
  if (output == nullptr) {
    return false;
  }
  int64_t value = 0;
  if (!ParseSigned(data, length, std::numeric_limits<int8_t>::min(),
          std::numeric_limits<int8_t>::max(), &value)) {
    return false;
  }
  *output = static_cast<int8_t>(value);
  return true;
}

bool ParseInt32(const char* data, size_t length, int32_t* output) {
  if (output == nullptr) {
    return false;
  }
  int64_t value = 0;
  if (!ParseSigned(data, length, std::numeric_limits<int32_t>::min(),
          std::numeric_limits<int32_t>::max(), &value)) {
    return false;
  }
  *output = static_cast<int32_t>(value);
  return true;
}

bool ParseInt64(const char* data, size_t length, int64_t* output) {
  return ParseSigned(data, length, std::numeric_limits<int64_t>::min(),
      std::numeric_limits<int64_t>::max(), output);
}

bool ParseUint8(const char* data, size_t length, uint8_t* output) {
  if (output == nullptr) {
    return false;
  }
  uint64_t value = 0;
  if (!ParseUnsigned(
          data, length, 10, std::numeric_limits<uint8_t>::max(), &value)) {
    return false;
  }
  *output = static_cast<uint8_t>(value);
  return true;
}

bool ParseUint16(const char* data, size_t length, uint16_t* output) {
  if (output == nullptr) {
    return false;
  }
  uint64_t value = 0;
  if (!ParseUnsigned(
          data, length, 10, std::numeric_limits<uint16_t>::max(), &value)) {
    return false;
  }
  *output = static_cast<uint16_t>(value);
  return true;
}

bool ParseUint32(const char* data, size_t length, uint32_t* output) {
  if (output == nullptr) {
    return false;
  }
  uint64_t value = 0;
  if (!ParseUnsigned(
          data, length, 10, std::numeric_limits<uint32_t>::max(), &value)) {
    return false;
  }
  *output = static_cast<uint32_t>(value);
  return true;
}

bool ParseHexUint8(const char* data, size_t length, uint8_t* output) {
  if (output == nullptr) {
    return false;
  }
  uint64_t value = 0;
  if (!ParseUnsigned(
          data, length, 16, std::numeric_limits<uint8_t>::max(), &value)) {
    return false;
  }
  *output = static_cast<uint8_t>(value);
  return true;
}

bool ParseFloat(const char* data, size_t length, float* output) {
  return ParseDecimal(data, length, output);
}

bool ParseDouble(const char* data, size_t length, double* output) {
  return ParseDecimal(data, length, output);
}

}  // namespace numeric_conversion
}  // namespace cpp_bus_driver
