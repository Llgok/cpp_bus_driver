/*
 * @Description: GNSS NMEA 流式解析工具实现
 * @Author: LILYGO_L
 * @Date: 2026-09-04 16:45:00
 * @LastEditTime: 2026-09-05 14:56:32
 * @License: GPL 3.0
 */
#include "parser/nmea_parser.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <new>
#include <utility>

#include "utility/numeric_conversion.h"

namespace cpp_bus_driver {

// 指向当前语句缓冲区的非拥有字段视图。
struct NmeaParser::FieldView {
  const char* data = nullptr;
  size_t length = 0;
  // 字段超出固定存储容量时通知当前语句，区分容量错误和格式错误。
  bool* capacity_error = nullptr;

  /**
   * @brief 创建空字段视图
   */
  FieldView() = default;

  /**
   * @brief 创建指向原始语句缓冲区的字段视图
   * @param field_data 字段首地址
   * @param field_length 字段长度
   * @param error_flag 容量错误输出标志，可为空
   */
  FieldView(
      const char* field_data, size_t field_length, bool* error_flag = nullptr)
      : data(field_data), length(field_length), capacity_error(error_flag) {}

  /**
   * @brief 判断字段是否为空
   * @return 字段长度为零时返回 true
   */
  bool empty() const { return length == 0; }
};

// 完成校验并拆分后的单条 NMEA 语句。
struct NmeaParser::SentenceView {
  FieldView address;
  FieldView talker_id;
  FieldView formatter;
  std::array<FieldView, kMaxFieldCount> fields{};
  size_t field_count = 0;
  bool proprietary_pubx = false;
};

// 跨 UART 输入保存的 GSV 多语句聚合槽。
struct NmeaParser::GsvAssembly {
  Gsv value;
  // Talker ID 与 Signal ID 组成的聚合键。
  uint32_t key = 0;
  // 下一条期望接收的 GSV 语句编号。
  uint8_t next_sentence_number = 0;
  bool active = false;
};

// 跨 UART 输入保存的 TXT 多语句聚合槽。
struct NmeaParser::TxtAssembly {
  Txt value;
  // Talker ID 与 Text ID 组成的聚合键。
  uint32_t key = 0;
  // 已聚合文本的字节总数。
  size_t total_text_bytes = 0;
  // 正文与终止符集中存储，value.messages 只保存偏移和长度。
  std::array<char, kMaxTextAssemblyBytes + kMaxTextSentenceCount> text_buffer{};
  size_t text_buffer_used = 0;
  // 下一条期望接收的 TXT 语句编号。
  uint8_t next_sentence_number = 0;
  bool active = false;
};

// 解析器的大容量可变状态，集中放在堆上以降低调用任务的栈占用。
struct NmeaParser::State {
  // 大容量结果与暂存对象和状态一起分配，不放在 Feed 调用栈上。
  Update update;
  Rlm rlm_scratch;
  Gsv gsv_scratch;
  PubxSatelliteStatus pubx_satellite_scratch;
  // 当前正在收集的 NMEA 语句，不包含 CR/LF。
  std::array<char, kMaxSentenceLength + 1> sentence_buffer{};
  // 当前语句已保存的字节数。
  size_t sentence_length = 0;
  // 是否已经接收到当前语句的 '$' 起始符。
  bool receiving_sentence = false;
  // 当前语句是否触发过有界容器容量限制。
  bool capacity_error_pending = false;

  // GSV 按 Talker ID 与 Signal ID 独立聚合。
  std::array<GsvAssembly, kMaxRepeatedUpdates> gsv_assemblies{};
  // TXT 按 Talker ID 与 Text ID 独立聚合。
  std::array<TxtAssembly, 4> txt_assemblies{};
  // 字段视图随解析器复用，避免每条超长语句额外占用任务栈。
  SentenceView sentence;
  // 解析器生命周期累计诊断数据。
  Statistics statistics{};
};

namespace {

// 单个完整 GSV 星系/信号集合允许保存的最大卫星数量。
constexpr size_t kMaxGsvSatellites = 36;
// MIA-M10Q 可并发跟踪并由 PUBX,03 报告的最大卫星数量。
constexpr size_t kMaxPubxSatellites = 32;

/**
 * @brief 原地重建固定存储对象，避免在任务栈上生成同尺寸临时对象
 * @param value 已构造的固定存储对象，不得为空
 */
template <typename T>
void ResetInPlace(T* value) {
  value->~T();
  new (value) T{};
}

/**
 * @brief 判断字段是否与指定空字符结尾文本完全一致
 * @param field 非拥有字段视图
 * @param text 用于比较的文本
 * @return 长度和内容均一致时返回 true
 */
template <typename Field>
bool FieldEquals(const Field& field, const char* text) {
  if (text == nullptr) {
    return false;
  }
  size_t length = 0;
  while (text[length] != '\0') {
    ++length;
  }
  if (field.length == 0) {
    return length == 0;
  }
  return field.data != nullptr && field.length == length &&
         std::equal(field.data, field.data + field.length, text);
}

/**
 * @brief 将有界字段复制到字符串
 * @param field 非拥有字段视图
 * @param output 字符串输出指针
 * @return 输入和输出有效时返回 true
 */
template <typename Field, size_t Capacity>
bool CopyField(const Field& field, NmeaParser::FixedText<Capacity>* output) {
  if (output == nullptr) {
    return false;
  }
  if (field.length == 0) {
    output->clear();
    return true;
  }
  if (field.data == nullptr) {
    return false;
  }
  if (!output->Assign(field.data, field.length)) {
    if (field.capacity_error != nullptr) {
      *field.capacity_error = true;
    }
    return false;
  }
  return true;
}

/**
 * @brief 解析允许为空且带范围限制的 uint8_t 字段
 * @param field 非拥有字段视图
 * @param output 可选值输出指针
 * @param minimum 允许的最小值
 * @param maximum 允许的最大值
 * @return 空字段或有效数值返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalUint8(const Field& field, Optional* output,
    uint8_t minimum = 0, uint8_t maximum = 0xFF) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  uint8_t value = 0;
  if (!numeric_conversion::ParseUint8(field.data, field.length, &value) ||
      value < minimum || value > maximum) {
    return false;
  }
  output->value = value;
  output->valid = true;
  return true;
}

/**
 * @brief 解析允许为空的十六进制 uint8_t 字段
 * @param field 非拥有字段视图
 * @param output 可选值输出指针
 * @param maximum 允许的最大值
 * @return 空字段或有效数值返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalHexUint8(
    const Field& field, Optional* output, uint8_t maximum = 0xFF) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  uint8_t value = 0;
  if (!numeric_conversion::ParseHexUint8(field.data, field.length, &value) ||
      value > maximum) {
    return false;
  }
  output->value = value;
  output->valid = true;
  return true;
}

/**
 * @brief 解析允许为空且带范围限制的 uint16_t 字段
 * @param field 非拥有字段视图
 * @param output 可选值输出指针
 * @param minimum 允许的最小值
 * @param maximum 允许的最大值
 * @return 空字段或有效数值返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalUint16(const Field& field, Optional* output,
    uint16_t minimum = 0, uint16_t maximum = 0xFFFF) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  uint16_t value = 0;
  if (!numeric_conversion::ParseUint16(field.data, field.length, &value) ||
      value < minimum || value > maximum) {
    return false;
  }
  output->value = value;
  output->valid = true;
  return true;
}

/**
 * @brief 解析允许为空且带范围限制的 int8_t 字段
 * @param field 非拥有字段视图
 * @param output 可选值输出指针
 * @param minimum 允许的最小值
 * @param maximum 允许的最大值
 * @return 空字段或有效数值返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalInt8(const Field& field, Optional* output,
    int8_t minimum = std::numeric_limits<int8_t>::min(),
    int8_t maximum = std::numeric_limits<int8_t>::max()) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  int8_t value = 0;
  if (!numeric_conversion::ParseInt8(field.data, field.length, &value) ||
      value < minimum || value > maximum) {
    return false;
  }
  output->value = value;
  output->valid = true;
  return true;
}

/**
 * @brief 解析允许为空且带范围限制的 float 字段
 * @param field 非拥有字段视图
 * @param output 可选值输出指针
 * @param minimum 允许的最小值
 * @param maximum 允许的最大值
 * @return 空字段或有效有限数值返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalFloat(const Field& field, Optional* output,
    float minimum = -std::numeric_limits<float>::max(),
    float maximum = std::numeric_limits<float>::max()) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  float value = 0.0F;
  if (!numeric_conversion::ParseFloat(field.data, field.length, &value) ||
      value < minimum || value > maximum) {
    return false;
  }
  output->value = value;
  output->valid = true;
  return true;
}

/**
 * @brief 解析允许为空且带范围限制的 double 字段
 * @param field 非拥有字段视图
 * @param output 可选值输出指针
 * @param minimum 允许的最小值
 * @param maximum 允许的最大值
 * @return 空字段或有效有限数值返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalDouble(const Field& field, Optional* output,
    double minimum = -std::numeric_limits<double>::max(),
    double maximum = std::numeric_limits<double>::max()) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  double value = 0.0;
  if (!numeric_conversion::ParseDouble(field.data, field.length, &value) ||
      value < minimum || value > maximum) {
    return false;
  }
  output->value = value;
  output->valid = true;
  return true;
}

/**
 * @brief 解析允许为空且属于指定集合的单字符字段
 * @param field 非拥有字段视图
 * @param allowed 允许字符组成的空字符结尾文本
 * @param output 可选值输出指针
 * @return 空字段或允许的单字符返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalCharacter(
    const Field& field, const char* allowed, Optional* output) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  if (field.length != 1 || allowed == nullptr) {
    return false;
  }
  bool found = false;
  for (size_t index = 0; allowed[index] != '\0'; ++index) {
    found |= field.data[0] == allowed[index];
  }
  if (!found) {
    return false;
  }
  output->value = field.data[0];
  output->valid = true;
  return true;
}

/**
 * @brief 解析允许为空的字符串字段
 * @param field 非拥有字段视图
 * @param output 可选值输出指针
 * @return 字段复制成功时返回 true
 */
template <typename Field, typename Optional>
bool ParseOptionalString(const Field& field, Optional* output) {
  if (output == nullptr) {
    return false;
  }
  *output = Optional{};
  if (field.empty()) {
    return true;
  }
  if (!CopyField(field, &output->value)) {
    return false;
  }
  output->valid = true;
  return true;
}

/**
 * @brief 检查固定整数位数的无符号十进制字段
 * @param field 非拥有字段视图
 * @param integer_digits 整数部分必须包含的数字数量
 * @param allow_fraction 是否允许小数点及至少一位小数
 * @return 位数和字符均符合要求时返回 true，不接受符号或指数
 */
template <typename Field>
bool IsFixedDecimal(
    const Field& field, size_t integer_digits, bool allow_fraction) {
  if (field.data == nullptr || field.length < integer_digits ||
      integer_digits == 0) {
    return false;
  }
  for (size_t index = 0; index < integer_digits; ++index) {
    if (field.data[index] < '0' || field.data[index] > '9') {
      return false;
    }
  }
  if (field.length == integer_digits) {
    return true;
  }
  if (!allow_fraction || field.data[integer_digits] != '.' ||
      field.length - integer_digits < 2) {
    return false;
  }
  for (size_t index = integer_digits + 1; index < field.length; ++index) {
    if (field.data[index] < '0' || field.data[index] > '9') {
      return false;
    }
  }
  return true;
}

/**
 * @brief 解析 hhmmss.ss 格式的 UTC 时间字段
 * @param field 非拥有字段视图
 * @param output UTC 时间输出指针
 * @return 空字段或合法时间返回 true
 */
template <typename Field, typename Time>
bool ParseUtcTime(const Field& field, Time* output) {
  if (output == nullptr) {
    return false;
  }
  *output = Time{};
  if (field.empty()) {
    return true;
  }
  if (!IsFixedDecimal(field, 6, true)) {
    return false;
  }
  uint8_t hour = 0;
  uint8_t minute = 0;
  double second = 0.0;
  if (!numeric_conversion::ParseUint8(field.data, 2, &hour) || hour > 23 ||
      !numeric_conversion::ParseUint8(field.data + 2, 2, &minute) ||
      minute > 59 ||
      !numeric_conversion::ParseDouble(
          field.data + 4, field.length - 4, &second) ||
      second < 0.0 || second >= 60.0) {
    return false;
  }
  output->hour = hour;
  output->minute = minute;
  output->second = second;
  output->valid = true;
  return true;
}

/**
 * @brief 判断完整年份是否为闰年
 * @param year 完整年份
 * @return 闰年返回 true
 */
bool IsLeapYear(uint16_t year) {
  return (year % 4U == 0U && year % 100U != 0U) || year % 400U == 0U;
}

/**
 * @brief 获取指定月份的合法天数
 * @param month 月份
 * @param year 两位或四位年份
 * @param two_digit_year 年份是否来自两位 NMEA 字段
 * @return 月份有效时返回天数，否则返回 0
 */
uint8_t DaysInMonth(uint8_t month, uint16_t year, bool two_digit_year) {
  static constexpr uint8_t kDays[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 0 || month > 12) {
    return 0;
  }
  const uint16_t full_year = two_digit_year ? 2000U + year : year;
  if (month == 2 && IsLeapYear(full_year)) {
    return 29;
  }
  return kDays[month - 1];
}

/**
 * @brief 解析 ddmmyy 紧凑 UTC 日期
 * @param field 非拥有字段视图
 * @param output UTC 日期输出指针
 * @return 空字段或真实日历日期返回 true
 */
template <typename Field, typename Date>
bool ParseCompactDate(const Field& field, Date* output) {
  if (output == nullptr) {
    return false;
  }
  *output = Date{};
  if (field.empty()) {
    return true;
  }
  if (!IsFixedDecimal(field, 6, false)) {
    return false;
  }
  uint8_t day = 0;
  uint8_t month = 0;
  uint8_t year = 0;
  if (!numeric_conversion::ParseUint8(field.data, 2, &day) ||
      !numeric_conversion::ParseUint8(field.data + 2, 2, &month) ||
      !numeric_conversion::ParseUint8(field.data + 4, 2, &year) || day == 0 ||
      day > DaysInMonth(month, year, true)) {
    return false;
  }
  output->day = day;
  output->month = month;
  output->year = year;
  output->two_digit_year = true;
  output->valid = true;
  return true;
}

/**
 * @brief 解析由日、月、四位年组成的 UTC 日期
 * @param day_field 日字段
 * @param month_field 月字段
 * @param year_field 年字段
 * @param output UTC 日期输出指针
 * @return 三个字段均为空或组成真实日历日期时返回 true
 */
template <typename Field, typename Date>
bool ParseExpandedDate(const Field& day_field, const Field& month_field,
    const Field& year_field, Date* output) {
  if (output == nullptr) {
    return false;
  }
  *output = Date{};
  if (day_field.empty() && month_field.empty() && year_field.empty()) {
    return true;
  }
  if (!IsFixedDecimal(day_field, 2, false) ||
      !IsFixedDecimal(month_field, 2, false) ||
      !IsFixedDecimal(year_field, 4, false)) {
    return false;
  }
  uint8_t day = 0;
  uint8_t month = 0;
  uint16_t year = 0;
  if (!numeric_conversion::ParseUint8(day_field.data, day_field.length, &day) ||
      !numeric_conversion::ParseUint8(
          month_field.data, month_field.length, &month) ||
      !numeric_conversion::ParseUint16(
          year_field.data, year_field.length, &year) ||
      year == 0 || year > 9999 || day == 0 ||
      day > DaysInMonth(month, year, false)) {
    return false;
  }
  output->day = day;
  output->month = month;
  output->year = year;
  output->two_digit_year = false;
  output->valid = true;
  return true;
}

/**
 * @brief 解析 NMEA 度分格式坐标并换算为带符号十进制度
 * @param value_field ddmm.mmmm 或 dddmm.mmmm 字段
 * @param direction_field N/S 或 E/W 方向字段
 * @param degree_digits 度数部分的字符数量
 * @param output 坐标输出指针
 * @return 两字段均为空或组成合法坐标时返回 true
 */
template <typename Field, typename Coordinate>
bool ParseCoordinate(const Field& value_field, const Field& direction_field,
    size_t degree_digits, Coordinate* output) {
  if (output == nullptr) {
    return false;
  }
  *output = Coordinate{};
  if (value_field.empty() && direction_field.empty()) {
    return true;
  }
  if ((degree_digits != 2 && degree_digits != 3) ||
      direction_field.data == nullptr || direction_field.length != 1 ||
      !IsFixedDecimal(value_field, degree_digits + 2, true)) {
    return false;
  }

  uint16_t degrees = 0;
  double minutes = 0.0;
  if (!numeric_conversion::ParseUint16(
          value_field.data, degree_digits, &degrees) ||
      !numeric_conversion::ParseDouble(value_field.data + degree_digits,
          value_field.length - degree_digits, &minutes) ||
      minutes < 0.0 || minutes >= 60.0) {
    return false;
  }

  const char direction = direction_field.data[0];
  const bool latitude = degree_digits == 2;
  const uint16_t maximum_degrees = latitude ? 90 : 180;
  if (degrees > maximum_degrees ||
      (degrees == maximum_degrees && minutes != 0.0) ||
      (latitude && direction != 'N' && direction != 'S') ||
      (!latitude && direction != 'E' && direction != 'W')) {
    return false;
  }

  double decimal_degrees = degrees + minutes / 60.0;
  if (direction == 'S' || direction == 'W') {
    decimal_degrees = -decimal_degrees;
  }
  output->degrees = degrees;
  output->minutes = minutes;
  output->decimal_degrees = decimal_degrees;
  output->direction = direction;
  output->valid = true;
  return true;
}

/**
 * @brief 从语句的指定字段解析纬度和经度
 * @param sentence 已校验语句
 * @param latitude_index 纬度字段索引
 * @param latitude_direction_index 纬度方向字段索引
 * @param longitude_index 经度字段索引
 * @param longitude_direction_index 经度方向字段索引
 * @param output 位置输出指针
 * @return 两个坐标均满足成对出现和范围约束时返回 true
 */
template <typename Sentence, typename Position>
bool ParsePosition(const Sentence& sentence, size_t latitude_index,
    size_t latitude_direction_index, size_t longitude_index,
    size_t longitude_direction_index, Position* output) {
  if (output == nullptr || sentence.field_count <= longitude_direction_index) {
    return false;
  }
  Position value;
  if (!ParseCoordinate(sentence.fields[latitude_index],
          sentence.fields[latitude_direction_index], 2, &value.latitude) ||
      !ParseCoordinate(sentence.fields[longitude_index],
          sentence.fields[longitude_direction_index], 3, &value.longitude)) {
    return false;
  }
  if (value.latitude.valid != value.longitude.valid) {
    return false;
  }
  *output = std::move(value);
  return true;
}

/**
 * @brief 将语句地址、发送设备和 formatter 复制到输出结构
 * @param sentence 已校验语句
 * @param output 语句结构输出指针
 * @return 元数据复制成功时返回 true
 */
template <typename Sentence, typename Output>
bool CopyMetadata(const Sentence& sentence, Output* output) {
  return output != nullptr &&
         CopyField(sentence.address, &output->metadata.address) &&
         CopyField(sentence.talker_id, &output->metadata.talker_id) &&
         CopyField(sentence.formatter, &output->metadata.formatter);
}

/**
 * @brief 检查字段是否只包含十六进制字符
 * @param field 非拥有字段视图
 * @param required_length 要求的长度，0 表示仅要求非空
 * @return 内容和长度符合要求时返回 true
 */
template <typename Field>
bool IsHexString(const Field& field, size_t required_length = 0) {
  if (field.empty() ||
      (required_length != 0 && field.length != required_length)) {
    return false;
  }
  for (size_t index = 0; index < field.length; ++index) {
    const char value = field.data[index];
    if (!((value >= '0' && value <= '9') || (value >= 'A' && value <= 'F') ||
            (value >= 'a' && value <= 'f'))) {
      return false;
    }
  }
  return true;
}

/**
 * @brief 判断语句类型是否属于解析器支持的接收机输出
 * @param sentence 已校验语句
 * @return 支持该输出类型时返回 true
 */
template <typename Sentence>
bool IsSupportedSentence(const Sentence& sentence) {
  if (sentence.proprietary_pubx) {
    return sentence.field_count > 2 &&
           (FieldEquals(sentence.fields[1], "00") ||
               FieldEquals(sentence.fields[1], "03") ||
               FieldEquals(sentence.fields[1], "04"));
  }
  // 两份目标协议手册中定义的全部标准 NMEA 输出类型。
  static constexpr const char* kSupported[] = {"DTM", "GBS", "GGA", "GLL",
      "GNS", "GRS", "GSA", "GST", "GSV", "RLM", "RMC", "TXT", "VLW", "VTG",
      "ZDA"};
  for (const char* formatter : kSupported) {
    if (FieldEquals(sentence.formatter, formatter)) {
      return true;
    }
  }
  return false;
}

/**
 * @brief 将发送设备标识和可选编号编码为无分配聚合键
 * @param talker_id 两字符发送设备标识
 * @param identifier 信号编号或文本编号，保留空值与零值的区别
 * @return 用于查找多语句聚合槽的整数键
 */
template <typename Field, typename Optional>
uint32_t MakeAssemblyKey(const Field& talker_id, const Optional& identifier) {
  const uint32_t first = static_cast<uint8_t>(talker_id.data[0]);
  const uint32_t second = static_cast<uint8_t>(talker_id.data[1]);
  return (first << 17) | (second << 9) |
         (identifier.valid ? 0x100U | identifier.value : 0U);
}

}  // namespace

constexpr size_t NmeaParser::kMaxSentenceLength;
constexpr size_t NmeaParser::kMaxFieldCount;
constexpr size_t NmeaParser::kMaxRepeatedUpdates;
constexpr size_t NmeaParser::kMaxTextAssemblyBytes;
constexpr size_t NmeaParser::kMaxTextSentenceCount;
constexpr size_t NmeaParser::kTextBufferSize;

void NmeaParser::Update::Clear() {
  dtm = Dtm{};
  gbs = Gbs{};
  gga = Gga{};
  gll = Gll{};
  gns = Gns{};
  grs.clear();
  gsa.clear();
  gst = Gst{};
  gsv.clear();
  ResetInPlace(&rlm);
  rmc = Rmc{};
  txt.clear();
  vlw = Vlw{};
  vtg = Vtg{};
  zda = Zda{};
  pubx_position = PubxPosition{};
  ResetInPlace(&pubx_satellite_status);
  pubx_time = PubxTime{};
  text_buffer_used_ = 0;
}

const char* NmeaParser::Update::GetTextMessage(
    const Txt& text, size_t index) const noexcept {
  if (!text.valid || index >= text.messages.size()) {
    return nullptr;
  }
  const TextRange& range = text.messages[index];
  if (range.offset >= text_buffer_used_ ||
      range.length >= text_buffer_used_ - range.offset ||
      text_buffer_[range.offset + range.length] != '\0') {
    return nullptr;
  }
  return text_buffer_.data() + range.offset;
}

bool NmeaParser::Update::HasData() const {
  return dtm.valid || gbs.valid || gga.valid || gll.valid || gns.valid ||
         !grs.empty() || !gsa.empty() || gst.valid || !gsv.empty() ||
         rlm.valid || rmc.valid || !txt.empty() || vlw.valid || vtg.valid ||
         zda.valid || pubx_position.valid || pubx_satellite_status.valid ||
         pubx_time.valid;
}

NmeaParser::NmeaParser() : state_(new (std::nothrow) State) { Reset(); }

NmeaParser::~NmeaParser() = default;

NmeaParser::NmeaParser(NmeaParser&& other) noexcept = default;

NmeaParser& NmeaParser::operator=(NmeaParser&& other) noexcept = default;

bool NmeaParser::IsReady() const noexcept { return state_ != nullptr; }

const NmeaParser::Update* NmeaParser::update() const noexcept {
  return state_ != nullptr ? &state_->update : nullptr;
}

NmeaParser::FeedResult NmeaParser::Feed(const uint8_t* data, size_t length) {
  FeedResult result;
  result.input_bytes = length;
  if (state_ == nullptr) {
    result.capacity_errors = 1;
    return result;
  }
  Update* update = &state_->update;
  update->Clear();
  state_->capacity_error_pending = false;
  if (data == nullptr && length != 0) {
    result.format_errors = 1;
    state_->statistics.input_bytes += length;
    ++state_->statistics.format_errors;
    return result;
  }

  for (size_t index = 0; index < length; ++index) {
    const char value = static_cast<char>(data[index]);
    if (value == '$') {
      if (state_->receiving_sentence && state_->sentence_length != 0) {
        ++result.format_errors;
      }
      state_->receiving_sentence = true;
      state_->sentence_length = 0;
      state_->sentence_buffer[state_->sentence_length++] = value;
      continue;
    }
    if (!state_->receiving_sentence) {
      continue;
    }
    if (value == '\n') {
      ParseBufferedSentence(update, &result);
      state_->receiving_sentence = false;
      state_->sentence_length = 0;
      continue;
    }
    if (value == '\r') {
      continue;
    }
    if (static_cast<unsigned char>(value) < 0x20U ||
        static_cast<unsigned char>(value) > 0x7EU) {
      state_->receiving_sentence = false;
      state_->sentence_length = 0;
      ++result.format_errors;
      continue;
    }
    if (state_->sentence_length >= kMaxSentenceLength) {
      state_->receiving_sentence = false;
      state_->sentence_length = 0;
      ++result.overflow_errors;
      continue;
    }
    state_->sentence_buffer[state_->sentence_length++] = value;
  }

  state_->statistics.input_bytes += result.input_bytes;
  state_->statistics.completed_sentences += result.completed_sentences;
  state_->statistics.parsed_sentences += result.parsed_sentences;
  state_->statistics.checksum_errors += result.checksum_errors;
  state_->statistics.format_errors += result.format_errors;
  state_->statistics.unsupported_sentences += result.unsupported_sentences;
  state_->statistics.overflow_errors += result.overflow_errors;
  state_->statistics.capacity_errors += result.capacity_errors;
  return result;
}

void NmeaParser::Reset() {
  if (state_ == nullptr) {
    return;
  }
  state_->sentence_buffer.fill('\0');
  state_->sentence_length = 0;
  state_->receiving_sentence = false;
  state_->capacity_error_pending = false;
  for (auto& assembly : state_->gsv_assemblies) {
    ResetInPlace(&assembly);
  }
  for (auto& assembly : state_->txt_assemblies) {
    ResetInPlace(&assembly);
  }
  state_->sentence.address = FieldView{};
  state_->sentence.talker_id = FieldView{};
  state_->sentence.formatter = FieldView{};
  state_->sentence.fields.fill(FieldView{});
  state_->sentence.field_count = 0;
  state_->sentence.proprietary_pubx = false;
  state_->statistics = Statistics{};
  state_->update.Clear();
}

const NmeaParser::Statistics& NmeaParser::GetStatistics() const {
  static const Statistics kEmptyStatistics;
  return state_ != nullptr ? state_->statistics : kEmptyStatistics;
}

void NmeaParser::ParseBufferedSentence(Update* update, FeedResult* result) {
  if (state_ == nullptr || update == nullptr || result == nullptr ||
      state_->sentence_length == 0) {
    return;
  }
  ++result->completed_sentences;
  SentenceView& sentence = state_->sentence;
  bool checksum_error = false;
  if (!BuildSentenceView(&sentence, &checksum_error)) {
    if (checksum_error) {
      ++result->checksum_errors;
    } else {
      ++result->format_errors;
    }
    return;
  }
  const bool parsed = DispatchSentence(sentence, update);
  if (state_->capacity_error_pending) {
    ++result->capacity_errors;
    state_->capacity_error_pending = false;
    // 资源不足后丢弃未完成聚合，避免后续语句使用不完整的中间状态。
    for (auto& assembly : state_->gsv_assemblies) {
      ResetInPlace(&assembly);
    }
    for (auto& assembly : state_->txt_assemblies) {
      ResetInPlace(&assembly);
    }
  } else if (parsed) {
    ++result->parsed_sentences;
  } else if (IsSupportedSentence(sentence)) {
    ++result->format_errors;
  } else {
    ++result->unsupported_sentences;
  }
}

bool NmeaParser::BuildSentenceView(
    SentenceView* sentence, bool* checksum_error) {
  if (sentence == nullptr || checksum_error == nullptr || state_ == nullptr ||
      state_->sentence_length < 7 || state_->sentence_buffer[0] != '$') {
    return false;
  }
  sentence->address = FieldView{};
  sentence->talker_id = FieldView{};
  sentence->formatter = FieldView{};
  sentence->fields.fill(FieldView{});
  sentence->field_count = 0;
  sentence->proprietary_pubx = false;
  *checksum_error = false;

  size_t star_index = state_->sentence_length;
  for (size_t index = 1; index < state_->sentence_length; ++index) {
    if (state_->sentence_buffer[index] == '*') {
      star_index = index;
      break;
    }
  }
  if (star_index == state_->sentence_length ||
      star_index + 3 != state_->sentence_length) {
    return false;
  }

  uint8_t expected_checksum = 0;
  const FieldView checksum_field(
      state_->sentence_buffer.data() + star_index + 1, 2);
  if (!IsHexString(checksum_field, 2) ||
      !numeric_conversion::ParseHexUint8(
          state_->sentence_buffer.data() + star_index + 1, 2,
          &expected_checksum)) {
    return false;
  }
  uint8_t calculated_checksum = 0;
  for (size_t index = 1; index < star_index; ++index) {
    calculated_checksum ^= static_cast<uint8_t>(state_->sentence_buffer[index]);
  }
  if (calculated_checksum != expected_checksum) {
    *checksum_error = true;
    return false;
  }

  size_t field_start = 1;
  for (size_t index = 1; index <= star_index; ++index) {
    if (index != star_index && state_->sentence_buffer[index] != ',') {
      continue;
    }
    if (sentence->field_count >= kMaxFieldCount) {
      return false;
    }
    sentence->fields[sentence->field_count++] =
        FieldView(state_->sentence_buffer.data() + field_start,
            index - field_start, &state_->capacity_error_pending);
    field_start = index + 1;
  }
  if (sentence->field_count == 0 || sentence->fields[0].empty()) {
    return false;
  }

  sentence->address = sentence->fields[0];
  if (FieldEquals(sentence->address, "PUBX")) {
    sentence->proprietary_pubx = true;
    sentence->formatter = sentence->address;
    return true;
  }
  if (sentence->address.length >= 4 && sentence->address.data[0] == 'P') {
    sentence->formatter = sentence->address;
    return true;
  }
  if (sentence->address.length != 5) {
    return false;
  }
  for (size_t index = 0; index < sentence->address.length; ++index) {
    const char value = sentence->address.data[index];
    if (value < 'A' || value > 'Z') {
      return false;
    }
  }
  sentence->talker_id =
      FieldView(sentence->address.data, 2, &state_->capacity_error_pending);
  sentence->formatter =
      FieldView(sentence->address.data + 2, 3, &state_->capacity_error_pending);
  return true;
}

bool NmeaParser::DispatchSentence(
    const SentenceView& sentence, Update* update) {
  if (update == nullptr) {
    return false;
  }
  if (sentence.proprietary_pubx) {
    if (sentence.field_count <= 1) {
      return false;
    }
    if (FieldEquals(sentence.fields[1], "00")) {
      return ParsePubxPosition(sentence, &update->pubx_position);
    }
    if (FieldEquals(sentence.fields[1], "03")) {
      return ParsePubxSatelliteStatus(sentence, &update->pubx_satellite_status);
    }
    if (FieldEquals(sentence.fields[1], "04")) {
      return ParsePubxTime(sentence, &update->pubx_time);
    }
    return false;
  }
  if (FieldEquals(sentence.formatter, "DTM")) {
    return ParseDtm(sentence, &update->dtm);
  }
  if (FieldEquals(sentence.formatter, "GBS")) {
    return ParseGbs(sentence, &update->gbs);
  }
  if (FieldEquals(sentence.formatter, "GGA")) {
    return ParseGga(sentence, &update->gga);
  }
  if (FieldEquals(sentence.formatter, "GLL")) {
    return ParseGll(sentence, &update->gll);
  }
  if (FieldEquals(sentence.formatter, "GNS")) {
    return ParseGns(sentence, &update->gns);
  }
  if (FieldEquals(sentence.formatter, "GRS")) {
    Grs value;
    if (!ParseGrs(sentence, &value)) {
      return false;
    }
    if (!update->grs.PushBack(std::move(value))) {
      state_->capacity_error_pending = true;
      return false;
    }
    return true;
  }
  if (FieldEquals(sentence.formatter, "GSA")) {
    Gsa value;
    if (!ParseGsa(sentence, &value)) {
      return false;
    }
    if (!update->gsa.PushBack(std::move(value))) {
      state_->capacity_error_pending = true;
      return false;
    }
    return true;
  }
  if (FieldEquals(sentence.formatter, "GST")) {
    return ParseGst(sentence, &update->gst);
  }
  if (FieldEquals(sentence.formatter, "GSV")) {
    return ParseGsv(sentence, update);
  }
  if (FieldEquals(sentence.formatter, "RLM")) {
    return ParseRlm(sentence, &update->rlm);
  }
  if (FieldEquals(sentence.formatter, "RMC")) {
    return ParseRmc(sentence, &update->rmc);
  }
  if (FieldEquals(sentence.formatter, "TXT")) {
    return ParseTxt(sentence, update);
  }
  if (FieldEquals(sentence.formatter, "VLW")) {
    return ParseVlw(sentence, &update->vlw);
  }
  if (FieldEquals(sentence.formatter, "VTG")) {
    return ParseVtg(sentence, &update->vtg);
  }
  if (FieldEquals(sentence.formatter, "ZDA")) {
    return ParseZda(sentence, &update->zda);
  }
  return false;
}

bool NmeaParser::ParseDtm(const SentenceView& sentence, Dtm* output) const {
  if (output == nullptr || sentence.field_count != 9) {
    return false;
  }
  Dtm value;
  if (!CopyMetadata(sentence, &value) ||
      !CopyField(sentence.fields[1], &value.local_datum) ||
      !CopyField(sentence.fields[2], &value.local_datum_subdivision) ||
      !ParseOptionalDouble(
          sentence.fields[3], &value.latitude_offset_minutes) ||
      !ParseOptionalCharacter(
          sentence.fields[4], "NS", &value.latitude_direction) ||
      !ParseOptionalDouble(
          sentence.fields[5], &value.longitude_offset_minutes) ||
      !ParseOptionalCharacter(
          sentence.fields[6], "EW", &value.longitude_direction) ||
      !ParseOptionalDouble(sentence.fields[7], &value.altitude_offset_meters) ||
      !CopyField(sentence.fields[8], &value.reference_datum) ||
      value.local_datum.empty() || value.reference_datum.empty()) {
    return false;
  }
  if (value.latitude_offset_minutes.valid != value.latitude_direction.valid ||
      value.longitude_offset_minutes.valid != value.longitude_direction.valid) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGbs(const SentenceView& sentence, Gbs* output) const {
  if (output == nullptr || sentence.field_count < 9 ||
      sentence.field_count > 11) {
    return false;
  }
  Gbs value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[1], &value.utc) ||
      !ParseOptionalDouble(
          sentence.fields[2], &value.latitude_error_meters, 0.0) ||
      !ParseOptionalDouble(
          sentence.fields[3], &value.longitude_error_meters, 0.0) ||
      !ParseOptionalDouble(
          sentence.fields[4], &value.altitude_error_meters, 0.0) ||
      !ParseOptionalUint16(sentence.fields[5], &value.failed_satellite_id) ||
      !ParseOptionalDouble(
          sentence.fields[6], &value.missed_detection_probability, 0.0) ||
      !ParseOptionalDouble(sentence.fields[7], &value.estimated_bias_meters) ||
      !ParseOptionalDouble(
          sentence.fields[8], &value.bias_standard_deviation_meters, 0.0)) {
    return false;
  }
  if (sentence.field_count > 9 &&
      !ParseOptionalHexUint8(sentence.fields[9], &value.system_id)) {
    return false;
  }
  if (sentence.field_count > 10 &&
      !ParseOptionalHexUint8(sentence.fields[10], &value.signal_id)) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGga(const SentenceView& sentence, Gga* output) const {
  if (output == nullptr || sentence.field_count != 15) {
    return false;
  }
  Gga value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[1], &value.utc) ||
      !ParsePosition(sentence, 2, 3, 4, 5, &value.position) ||
      !ParseOptionalUint8(sentence.fields[6], &value.fix_quality, 0, 9) ||
      !ParseOptionalUint8(sentence.fields[7], &value.satellites_used, 0, 99) ||
      !ParseOptionalFloat(sentence.fields[8], &value.hdop, 0.0F) ||
      !ParseOptionalDouble(sentence.fields[9], &value.altitude_meters) ||
      (!sentence.fields[10].empty() &&
          !FieldEquals(sentence.fields[10], "M")) ||
      !ParseOptionalDouble(
          sentence.fields[11], &value.geoid_separation_meters) ||
      (!sentence.fields[12].empty() &&
          !FieldEquals(sentence.fields[12], "M")) ||
      !ParseOptionalDouble(
          sentence.fields[13], &value.differential_age_seconds, 0.0) ||
      !ParseOptionalString(
          sentence.fields[14], &value.differential_station_id)) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGll(const SentenceView& sentence, Gll* output) const {
  if (output == nullptr || sentence.field_count < 7 ||
      sentence.field_count > 8) {
    return false;
  }
  Gll value;
  if (!CopyMetadata(sentence, &value) ||
      !ParsePosition(sentence, 1, 2, 3, 4, &value.position) ||
      !ParseUtcTime(sentence.fields[5], &value.utc) ||
      !ParseOptionalCharacter(sentence.fields[6], "AV", &value.data_status) ||
      (sentence.field_count == 8 && !ParseOptionalCharacter(sentence.fields[7],
                                        "ADEFNR", &value.positioning_mode))) {
    return false;
  }
  if (!value.data_status.valid) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGns(const SentenceView& sentence, Gns* output) const {
  if (output == nullptr || sentence.field_count < 13 ||
      sentence.field_count > 14) {
    return false;
  }
  Gns value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[1], &value.utc) ||
      !ParsePosition(sentence, 2, 3, 4, 5, &value.position) ||
      !ParseOptionalString(sentence.fields[6], &value.positioning_modes) ||
      !ParseOptionalUint8(sentence.fields[7], &value.satellites_used, 0, 99) ||
      !ParseOptionalFloat(sentence.fields[8], &value.hdop, 0.0F) ||
      !ParseOptionalDouble(sentence.fields[9], &value.altitude_meters) ||
      !ParseOptionalDouble(
          sentence.fields[10], &value.geoid_separation_meters) ||
      !ParseOptionalDouble(
          sentence.fields[11], &value.differential_age_seconds, 0.0) ||
      !ParseOptionalString(
          sentence.fields[12], &value.differential_station_id) ||
      (sentence.field_count == 14 &&
          !ParseOptionalCharacter(
              sentence.fields[13], "VSCU", &value.navigational_status))) {
    return false;
  }
  if (value.positioning_modes.valid) {
    if (value.positioning_modes.value.size() > 4) {
      return false;
    }
    static constexpr char kPositioningModes[] = {'A', 'D', 'E', 'F', 'N', 'R'};
    for (char mode : value.positioning_modes.value) {
      if (std::find(std::begin(kPositioningModes), std::end(kPositioningModes),
              mode) == std::end(kPositioningModes)) {
        return false;
      }
    }
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGrs(const SentenceView& sentence, Grs* output) const {
  if (output == nullptr || sentence.field_count < 15 ||
      sentence.field_count > 17) {
    return false;
  }
  Grs value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[1], &value.utc) ||
      !ParseOptionalUint8(sentence.fields[2], &value.residual_mode, 0, 1)) {
    return false;
  }
  for (size_t index = 0; index < value.residuals_meters.size(); ++index) {
    if (!ParseOptionalFloat(
            sentence.fields[index + 3], &value.residuals_meters[index])) {
      return false;
    }
  }
  if (sentence.field_count > 15 &&
      !ParseOptionalHexUint8(sentence.fields[15], &value.system_id)) {
    return false;
  }
  if (sentence.field_count > 16 &&
      !ParseOptionalHexUint8(sentence.fields[16], &value.signal_id)) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGsa(const SentenceView& sentence, Gsa* output) const {
  if (output == nullptr || sentence.field_count < 18 ||
      sentence.field_count > 19) {
    return false;
  }
  Gsa value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseOptionalCharacter(
          sentence.fields[1], "AM", &value.operation_mode) ||
      !ParseOptionalUint8(sentence.fields[2], &value.navigation_mode, 1, 3)) {
    return false;
  }
  for (size_t index = 3; index <= 14; ++index) {
    OptionalValue<uint16_t> satellite_id;
    if (!ParseOptionalUint16(sentence.fields[index], &satellite_id)) {
      return false;
    }
    if (satellite_id.valid &&
        !value.satellite_ids.PushBack(std::move(satellite_id.value))) {
      state_->capacity_error_pending = true;
      return false;
    }
  }
  if (!ParseOptionalFloat(sentence.fields[15], &value.pdop, 0.0F, 99.0F) ||
      !ParseOptionalFloat(sentence.fields[16], &value.hdop, 0.0F, 99.0F) ||
      !ParseOptionalFloat(sentence.fields[17], &value.vdop, 0.0F, 99.0F) ||
      (sentence.field_count == 19 &&
          !ParseOptionalHexUint8(sentence.fields[18], &value.system_id))) {
    return false;
  }
  if (!value.operation_mode.valid || !value.navigation_mode.valid) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGst(const SentenceView& sentence, Gst* output) const {
  if (output == nullptr || sentence.field_count != 9) {
    return false;
  }
  Gst value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[1], &value.utc) ||
      !ParseOptionalDouble(sentence.fields[2], &value.range_rms_meters, 0.0) ||
      !ParseOptionalDouble(
          sentence.fields[3], &value.semi_major_deviation_meters, 0.0) ||
      !ParseOptionalDouble(
          sentence.fields[4], &value.semi_minor_deviation_meters, 0.0) ||
      !ParseOptionalDouble(sentence.fields[5],
          &value.semi_major_orientation_degrees, 0.0, 360.0) ||
      !ParseOptionalDouble(
          sentence.fields[6], &value.latitude_deviation_meters, 0.0) ||
      !ParseOptionalDouble(
          sentence.fields[7], &value.longitude_deviation_meters, 0.0) ||
      !ParseOptionalDouble(
          sentence.fields[8], &value.altitude_deviation_meters, 0.0)) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseGsv(const SentenceView& sentence, Update* update) {
  if (update == nullptr || sentence.field_count < 4) {
    return false;
  }
  uint8_t total_sentence_count = 0;
  uint8_t sentence_number = 0;
  uint8_t total_satellite_count = 0;
  if (!numeric_conversion::ParseUint8(sentence.fields[1].data,
          sentence.fields[1].length, &total_sentence_count) ||
      total_sentence_count < 1 || total_sentence_count > 9 ||
      !numeric_conversion::ParseUint8(sentence.fields[2].data,
          sentence.fields[2].length, &sentence_number) ||
      sentence_number < 1 || sentence_number > total_sentence_count ||
      !numeric_conversion::ParseUint8(sentence.fields[3].data,
          sentence.fields[3].length, &total_satellite_count) ||
      total_satellite_count > kMaxGsvSatellites) {
    return false;
  }

  const size_t payload_field_count = sentence.field_count - 4;
  const bool has_signal_id = payload_field_count % 4 == 1;
  if (payload_field_count % 4 != 0 && !has_signal_id) {
    return false;
  }
  size_t satellite_field_end = sentence.field_count;
  OptionalValue<uint8_t> signal_id;
  if (has_signal_id) {
    --satellite_field_end;
    if (!ParseOptionalHexUint8(
            sentence.fields[satellite_field_end], &signal_id)) {
      return false;
    }
  }
  const size_t satellite_group_count = (satellite_field_end - 4U) / 4U;
  if (satellite_group_count > 4U) {
    return false;
  }

  Gsv& sentence_value = state_->gsv_scratch;
  ResetInPlace(&sentence_value);
  if (!CopyMetadata(sentence, &sentence_value)) {
    return false;
  }
  sentence_value.total_sentence_count = total_sentence_count;
  sentence_value.received_sentence_count = sentence_number;
  sentence_value.total_satellite_count = total_satellite_count;
  sentence_value.signal_id = signal_id;

  for (size_t index = 4; index + 3 < satellite_field_end; index += 4) {
    Gsv::Satellite satellite;
    if (!ParseOptionalUint16(sentence.fields[index], &satellite.id) ||
        !ParseOptionalInt8(
            sentence.fields[index + 1], &satellite.elevation_degrees, 0, 90) ||
        !ParseOptionalUint16(
            sentence.fields[index + 2], &satellite.azimuth_degrees, 0, 359) ||
        !ParseOptionalUint8(sentence.fields[index + 3],
            &satellite.carrier_to_noise_db_hz, 0, 99)) {
      return false;
    }
    if (satellite.id.valid || satellite.elevation_degrees.valid ||
        satellite.azimuth_degrees.valid ||
        satellite.carrier_to_noise_db_hz.valid) {
      if (!satellite.id.valid) {
        return false;
      }
      if (!sentence_value.satellites.PushBack(std::move(satellite))) {
        state_->capacity_error_pending = true;
        return false;
      }
    }
  }

  const uint32_t key = MakeAssemblyKey(sentence.talker_id, signal_id);

  GsvAssembly* assembly = nullptr;
  for (auto& candidate : state_->gsv_assemblies) {
    if (candidate.active && candidate.key == key) {
      assembly = &candidate;
      break;
    }
  }
  if (sentence_number == 1) {
    if (assembly == nullptr) {
      for (auto& candidate : state_->gsv_assemblies) {
        if (!candidate.active) {
          assembly = &candidate;
          break;
        }
      }
    }
    if (assembly == nullptr) {
      state_->capacity_error_pending = true;
      return false;
    }
    ResetInPlace(assembly);
    assembly->active = true;
    assembly->key = key;
    assembly->value.metadata = std::move(sentence_value.metadata);
    assembly->value.total_sentence_count = total_sentence_count;
    assembly->value.total_satellite_count = total_satellite_count;
    assembly->value.signal_id = signal_id;
    assembly->next_sentence_number = 1;
  }
  if (assembly == nullptr || !assembly->active ||
      assembly->next_sentence_number != sentence_number ||
      assembly->value.total_sentence_count != total_sentence_count ||
      assembly->value.total_satellite_count != total_satellite_count) {
    if (assembly != nullptr) {
      ResetInPlace(assembly);
    }
    return false;
  }

  for (auto& satellite : sentence_value.satellites) {
    if (!assembly->value.satellites.PushBack(std::move(satellite))) {
      ResetInPlace(assembly);
      state_->capacity_error_pending = true;
      return false;
    }
  }
  assembly->value.received_sentence_count = sentence_number;
  ++assembly->next_sentence_number;
  if (sentence_number == total_sentence_count) {
    if (assembly->value.satellites.size() != total_satellite_count) {
      ResetInPlace(assembly);
      return false;
    }
    assembly->value.complete = true;
    assembly->value.valid = true;
    if (!update->gsv.PushBack(std::move(assembly->value))) {
      state_->capacity_error_pending = true;
      return false;
    }
    ResetInPlace(assembly);
  }
  return true;
}

bool NmeaParser::ParseRlm(const SentenceView& sentence, Rlm* output) const {
  if (output == nullptr || sentence.field_count != 5 ||
      !IsHexString(sentence.fields[1], 15) ||
      !IsHexString(sentence.fields[4])) {
    return false;
  }
  Rlm& value = state_->rlm_scratch;
  ResetInPlace(&value);
  if (!CopyMetadata(sentence, &value) ||
      !CopyField(sentence.fields[1], &value.beacon_id) ||
      !ParseUtcTime(sentence.fields[2], &value.utc) ||
      !ParseOptionalCharacter(
          sentence.fields[3], "0123456789ABCDEF", &value.message_code) ||
      !CopyField(sentence.fields[4], &value.message_body)) {
    return false;
  }
  if (!value.utc.valid || !value.message_code.valid) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseRmc(const SentenceView& sentence, Rmc* output) const {
  if (output == nullptr || sentence.field_count < 12 ||
      sentence.field_count > 14) {
    return false;
  }
  Rmc value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[1], &value.utc) ||
      !ParseOptionalCharacter(sentence.fields[2], "AVD", &value.data_status) ||
      !ParsePosition(sentence, 3, 4, 5, 6, &value.position) ||
      !ParseOptionalFloat(
          sentence.fields[7], &value.speed_over_ground_knots, 0.0F) ||
      !ParseOptionalFloat(sentence.fields[8], &value.course_over_ground_degrees,
          0.0F, 360.0F) ||
      !ParseCompactDate(sentence.fields[9], &value.date) ||
      !ParseOptionalFloat(
          sentence.fields[10], &value.magnetic_variation_degrees, 0.0F) ||
      !ParseOptionalCharacter(
          sentence.fields[11], "EW", &value.magnetic_variation_direction) ||
      (sentence.field_count > 12 && !ParseOptionalCharacter(sentence.fields[12],
                                        "ADEFNR", &value.positioning_mode)) ||
      (sentence.field_count > 13 && !ParseOptionalCharacter(sentence.fields[13],
                                        "SCUV", &value.navigational_status))) {
    return false;
  }
  if (!value.data_status.valid) {
    return false;
  }
  // 磁偏角及其东西方向必须同时提供，或同时留空。
  if (value.magnetic_variation_degrees.valid !=
      value.magnetic_variation_direction.valid) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseTxt(const SentenceView& sentence, Update* update) {
  if (update == nullptr || sentence.field_count < 5) {
    return false;
  }
  uint8_t total_sentence_count = 0;
  uint8_t sentence_number = 0;
  OptionalValue<uint8_t> text_id;
  if (!numeric_conversion::ParseUint8(sentence.fields[1].data,
          sentence.fields[1].length, &total_sentence_count) ||
      total_sentence_count < 1 ||
      total_sentence_count > kMaxTextSentenceCount ||
      !numeric_conversion::ParseUint8(sentence.fields[2].data,
          sentence.fields[2].length, &sentence_number) ||
      sentence_number < 1 || sentence_number > total_sentence_count ||
      !ParseOptionalUint8(sentence.fields[3], &text_id)) {
    return false;
  }

  // 字段仍指向连续的原始语句，直接保留正文中的逗号，不创建临时字符串。
  const char* text_data = sentence.fields[4].data;
  const FieldView& last_field = sentence.fields[sentence.field_count - 1];
  const size_t text_size =
      static_cast<size_t>(last_field.data + last_field.length - text_data);
  const uint32_t key = MakeAssemblyKey(sentence.talker_id, text_id);

  TxtAssembly* assembly = nullptr;
  for (auto& candidate : state_->txt_assemblies) {
    if (candidate.active && candidate.key == key) {
      assembly = &candidate;
      break;
    }
  }
  if (sentence_number == 1) {
    if (assembly == nullptr) {
      for (auto& candidate : state_->txt_assemblies) {
        if (!candidate.active) {
          assembly = &candidate;
          break;
        }
      }
    }
    if (assembly == nullptr) {
      state_->capacity_error_pending = true;
      return false;
    }
    ResetInPlace(assembly);
    assembly->active = true;
    assembly->key = key;
    if (!CopyMetadata(sentence, &assembly->value)) {
      return false;
    }
    assembly->value.total_sentence_count = total_sentence_count;
    assembly->value.text_id = text_id;
    assembly->total_text_bytes = 0;
    assembly->next_sentence_number = 1;
  }
  if (assembly == nullptr || !assembly->active ||
      assembly->next_sentence_number != sentence_number ||
      assembly->value.total_sentence_count != total_sentence_count) {
    if (assembly != nullptr) {
      ResetInPlace(assembly);
    }
    return false;
  }
  if (assembly->total_text_bytes > kMaxTextAssemblyBytes ||
      text_size > kMaxTextAssemblyBytes - assembly->total_text_bytes ||
      assembly->text_buffer_used >= assembly->text_buffer.size() ||
      text_size >= assembly->text_buffer.size() - assembly->text_buffer_used) {
    ResetInPlace(assembly);
    state_->capacity_error_pending = true;
    return false;
  }
  TextRange range;
  range.offset = assembly->text_buffer_used;
  range.length = text_size;
  if (!assembly->value.messages.PushBack(range)) {
    state_->capacity_error_pending = true;
    return false;
  }
  std::memcpy(
      assembly->text_buffer.data() + range.offset, text_data, text_size);
  assembly->text_buffer[range.offset + text_size] = '\0';
  assembly->text_buffer_used += text_size + 1;
  assembly->total_text_bytes += text_size;
  ++assembly->next_sentence_number;
  if (sentence_number == total_sentence_count) {
    assembly->value.complete = true;
    assembly->value.valid = true;
    if (update->txt.size() >= kMaxRepeatedUpdates ||
        assembly->text_buffer_used >
            update->text_buffer_.size() - update->text_buffer_used_) {
      state_->capacity_error_pending = true;
      return false;
    }
    // 完整组才提交到本次更新，组内偏移统一转换到共享文本区。
    const size_t base = update->text_buffer_used_;
    std::memcpy(update->text_buffer_.data() + base,
        assembly->text_buffer.data(), assembly->text_buffer_used);
    for (auto& message : assembly->value.messages) {
      message.offset += base;
    }
    if (!update->txt.PushBack(assembly->value)) {
      state_->capacity_error_pending = true;
      return false;
    }
    update->text_buffer_used_ += assembly->text_buffer_used;
    ResetInPlace(assembly);
  }
  return true;
}

bool NmeaParser::ParseVlw(const SentenceView& sentence, Vlw* output) const {
  if (output == nullptr || sentence.field_count != 9) {
    return false;
  }
  Vlw value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseOptionalDouble(sentence.fields[1],
          &value.total_water_distance_nautical_miles, 0.0) ||
      (!sentence.fields[2].empty() && !FieldEquals(sentence.fields[2], "N")) ||
      !ParseOptionalDouble(
          sentence.fields[3], &value.water_distance_nautical_miles, 0.0) ||
      (!sentence.fields[4].empty() && !FieldEquals(sentence.fields[4], "N")) ||
      !ParseOptionalDouble(sentence.fields[5],
          &value.total_ground_distance_nautical_miles, 0.0) ||
      (!sentence.fields[6].empty() && !FieldEquals(sentence.fields[6], "N")) ||
      !ParseOptionalDouble(
          sentence.fields[7], &value.ground_distance_nautical_miles, 0.0) ||
      (!sentence.fields[8].empty() && !FieldEquals(sentence.fields[8], "N"))) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseVtg(const SentenceView& sentence, Vtg* output) const {
  if (output == nullptr || sentence.field_count < 9 ||
      sentence.field_count > 10) {
    return false;
  }
  Vtg value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseOptionalFloat(
          sentence.fields[1], &value.true_course_degrees, 0.0F, 360.0F) ||
      (!sentence.fields[2].empty() && !FieldEquals(sentence.fields[2], "T")) ||
      !ParseOptionalFloat(
          sentence.fields[3], &value.magnetic_course_degrees, 0.0F, 360.0F) ||
      (!sentence.fields[4].empty() && !FieldEquals(sentence.fields[4], "M")) ||
      !ParseOptionalFloat(sentence.fields[5], &value.speed_knots, 0.0F) ||
      (!sentence.fields[6].empty() && !FieldEquals(sentence.fields[6], "N")) ||
      !ParseOptionalFloat(
          sentence.fields[7], &value.speed_kilometers_per_hour, 0.0F) ||
      (!sentence.fields[8].empty() && !FieldEquals(sentence.fields[8], "K")) ||
      (sentence.field_count == 10 && !ParseOptionalCharacter(sentence.fields[9],
                                         "ADEFNR", &value.positioning_mode))) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParseZda(const SentenceView& sentence, Zda* output) const {
  if (output == nullptr || sentence.field_count != 7) {
    return false;
  }
  Zda value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[1], &value.utc) ||
      !ParseExpandedDate(sentence.fields[2], sentence.fields[3],
          sentence.fields[4], &value.date) ||
      !ParseOptionalInt8(
          sentence.fields[5], &value.local_zone_hours, -13, 13) ||
      !ParseOptionalUint8(
          sentence.fields[6], &value.local_zone_minutes, 0, 59)) {
    return false;
  }
  if (value.local_zone_hours.valid != value.local_zone_minutes.valid) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParsePubxPosition(
    const SentenceView& sentence, PubxPosition* output) const {
  const bool valid_field_count =
      sentence.field_count == 21 ||
      (sentence.field_count == 22 && sentence.fields[21].empty());
  if (output == nullptr || !valid_field_count ||
      !FieldEquals(sentence.fields[1], "00")) {
    return false;
  }
  PubxPosition value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[2], &value.utc) ||
      !ParsePosition(sentence, 3, 4, 5, 6, &value.position) ||
      !ParseOptionalDouble(
          sentence.fields[7], &value.altitude_above_user_datum_meters) ||
      !ParseOptionalString(sentence.fields[8], &value.navigation_status) ||
      !ParseOptionalFloat(
          sentence.fields[9], &value.horizontal_accuracy_meters, 0.0F) ||
      !ParseOptionalFloat(
          sentence.fields[10], &value.vertical_accuracy_meters, 0.0F) ||
      !ParseOptionalFloat(
          sentence.fields[11], &value.speed_kilometers_per_hour, 0.0F) ||
      !ParseOptionalFloat(sentence.fields[12],
          &value.course_over_ground_degrees, 0.0F, 360.0F) ||
      !ParseOptionalFloat(
          sentence.fields[13], &value.vertical_velocity_meters_per_second) ||
      !ParseOptionalFloat(
          sentence.fields[14], &value.differential_age_seconds, 0.0F) ||
      !ParseOptionalFloat(sentence.fields[15], &value.hdop, 0.0F) ||
      !ParseOptionalFloat(sentence.fields[16], &value.vdop, 0.0F) ||
      !ParseOptionalFloat(sentence.fields[17], &value.tdop, 0.0F) ||
      !ParseOptionalUint8(sentence.fields[18], &value.satellites_used, 0, 99)) {
    return false;
  }
  if (value.navigation_status.valid) {
    // u-blox PUBX,00 手册定义的导航状态代码。
    static constexpr const char* kNavigationStatuses[] = {
        "NF", "DR", "G2", "G3", "D2", "D3", "RK", "TT"};
    bool navigation_status_valid = false;
    for (const char* status : kNavigationStatuses) {
      navigation_status_valid |= value.navigation_status.value == status;
    }
    if (!navigation_status_valid) {
      return false;
    }
  }
  uint8_t reserved = 0;
  if (!numeric_conversion::ParseUint8(
          sentence.fields[19].data, sentence.fields[19].length, &reserved) ||
      reserved != 0) {
    return false;
  }

  OptionalValue<uint8_t> dead_reckoning;
  if (!ParseOptionalUint8(sentence.fields[20], &dead_reckoning, 0, 1)) {
    return false;
  }
  if (dead_reckoning.valid) {
    value.dead_reckoning_used.value = dead_reckoning.value != 0;
    value.dead_reckoning_used.valid = true;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParsePubxSatelliteStatus(
    const SentenceView& sentence, PubxSatelliteStatus* output) const {
  if (output == nullptr || sentence.field_count < 3 ||
      !FieldEquals(sentence.fields[1], "03")) {
    return false;
  }
  uint8_t satellite_count = 0;
  if (!numeric_conversion::ParseUint8(sentence.fields[2].data,
          sentence.fields[2].length, &satellite_count) ||
      satellite_count > kMaxPubxSatellites) {
    return false;
  }
  const size_t expected_fields = 3U + static_cast<size_t>(satellite_count) * 6U;
  const bool has_trailing_empty_field =
      sentence.field_count == expected_fields + 1U &&
      sentence.fields[sentence.field_count - 1].empty();
  if (sentence.field_count != expected_fields && !has_trailing_empty_field) {
    return false;
  }

  PubxSatelliteStatus& value = state_->pubx_satellite_scratch;
  ResetInPlace(&value);
  if (!CopyMetadata(sentence, &value)) {
    return false;
  }
  value.reported_satellite_count = satellite_count;
  for (size_t index = 0; index < satellite_count; ++index) {
    const size_t field_index = 3U + index * 6U;
    PubxSatelliteStatus::Satellite satellite;
    if (!ParseOptionalUint16(sentence.fields[field_index], &satellite.id) ||
        !ParseOptionalCharacter(
            sentence.fields[field_index + 1], "-Ue", &satellite.status) ||
        !ParseOptionalUint16(sentence.fields[field_index + 2],
            &satellite.azimuth_degrees, 0, 359) ||
        !ParseOptionalInt8(sentence.fields[field_index + 3],
            &satellite.elevation_degrees, -90, 90) ||
        !ParseOptionalUint8(sentence.fields[field_index + 4],
            &satellite.carrier_to_noise_db_hz, 0, 99) ||
        !ParseOptionalUint8(sentence.fields[field_index + 5],
            &satellite.carrier_lock_time_seconds, 0, 64)) {
      return false;
    }
    if (!satellite.id.valid || !satellite.status.valid) {
      return false;
    }
    if (!value.satellites.PushBack(std::move(satellite))) {
      state_->capacity_error_pending = true;
      return false;
    }
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

bool NmeaParser::ParsePubxTime(
    const SentenceView& sentence, PubxTime* output) const {
  if (output == nullptr || sentence.field_count < 10 ||
      sentence.field_count > 11 || !FieldEquals(sentence.fields[1], "04") ||
      (sentence.field_count == 11 && !sentence.fields[10].empty())) {
    return false;
  }
  PubxTime value;
  if (!CopyMetadata(sentence, &value) ||
      !ParseUtcTime(sentence.fields[2], &value.utc) ||
      !ParseCompactDate(sentence.fields[3], &value.date) ||
      !ParseOptionalDouble(
          sentence.fields[4], &value.utc_time_of_week_seconds, 0.0) ||
      !ParseOptionalUint16(sentence.fields[5], &value.utc_week_number)) {
    return false;
  }

  if (!sentence.fields[6].empty()) {
    size_t leap_second_length = sentence.fields[6].length;
    if (sentence.fields[6].data[leap_second_length - 1] == 'D') {
      value.leap_seconds_are_default = true;
      --leap_second_length;
    }
    int8_t leap_seconds = 0;
    if (leap_second_length == 0 ||
        !numeric_conversion::ParseInt8(
            sentence.fields[6].data, leap_second_length, &leap_seconds)) {
      return false;
    }
    value.leap_seconds.value = leap_seconds;
    value.leap_seconds.valid = true;
  }

  if (!sentence.fields[7].empty()) {
    int64_t clock_bias = 0;
    if (!numeric_conversion::ParseInt64(
            sentence.fields[7].data, sentence.fields[7].length, &clock_bias)) {
      return false;
    }
    value.clock_bias_nanoseconds.value = clock_bias;
    value.clock_bias_nanoseconds.valid = true;
  }
  if (!ParseOptionalDouble(
          sentence.fields[8], &value.clock_drift_nanoseconds_per_second)) {
    return false;
  }
  if (!sentence.fields[9].empty()) {
    int32_t granularity = 0;
    if (!numeric_conversion::ParseInt32(
            sentence.fields[9].data, sentence.fields[9].length, &granularity)) {
      return false;
    }
    value.time_pulse_granularity_nanoseconds.value = granularity;
    value.time_pulse_granularity_nanoseconds.valid = true;
  }
  if (!value.utc.valid || !value.date.valid ||
      !value.utc_time_of_week_seconds.valid || !value.utc_week_number.valid) {
    return false;
  }
  value.valid = true;
  *output = std::move(value);
  return true;
}

}  // namespace cpp_bus_driver
