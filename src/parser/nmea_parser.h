/*
 * @Description: GNSS NMEA 流式解析工具接口
 * @Author: LILYGO_L
 * @Date: 2026-09-04 16:45:00
 * @LastEditTime: 2026-09-04 20:17:00
 * @License: GPL 3.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace cpp_bus_driver {

/**
 * @brief 面向 UART 字节流的有界 NMEA 0183 与 PUBX 输出解析器
 */
class NmeaParser {
 public:
  // 单条输入语句的最大尺寸，包含 '$'、校验和，不包含行结束符。
  static constexpr size_t kMaxSentenceLength = 2048;
  // 单条语句允许的最大字段数量，覆盖 MIA-M10Q 的 PUBX,03 卫星状态输出。
  static constexpr size_t kMaxFieldCount = 256;
  // 单次 Feed 调用为重复语句保留的最大更新数量。
  static constexpr size_t kMaxRepeatedUpdates = 16;

  // TXT 单组正文上限，以及单次更新共享的文本存储上限（包含终止符）。
  static constexpr size_t kMaxTextAssemblyBytes = 4096;
  static constexpr size_t kMaxTextSentenceCount = 99;
  static constexpr size_t kTextBufferSize = 8192;

  // NMEA 短字段的固定字符存储，不申请堆内存。
  template <size_t Capacity>
  class FixedText {
   public:
    /**
     * @brief 复制有界字段，超长时拒绝而不截断
     * @param data 输入字符地址，长度为零时允许为空
     * @param length 不包含终止符的字符数量
     * @return 参数和容量有效时返回 true，失败时原内容不变
     */
    bool Assign(const char* data, size_t length) noexcept {
      if ((data == nullptr && length != 0) || length > Capacity) {
        return false;
      }
      if (length != 0) {
        std::memmove(data_.data(), data, length);
      }
      data_[length] = '\0';
      size_ = length;
      return true;
    }
    void clear() noexcept {
      size_ = 0;
      data_[0] = '\0';
    }
    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    const char* c_str() const noexcept { return data_.data(); }
    const char* begin() const noexcept { return data_.data(); }
    const char* end() const noexcept { return data_.data() + size_; }

    /**
     * @brief 与空字符结尾文本比较，不分配内存
     * @param text 待比较文本
     * @return 字符数量和内容相同时返回 true
     */
    bool operator==(const char* text) const noexcept {
      if (text == nullptr) {
        return false;
      }
      for (size_t index = 0; index < size_; ++index) {
        if (text[index] == '\0' || data_[index] != text[index]) {
          return false;
        }
      }
      return text[size_] == '\0';
    }

   private:
    // 固定缓冲区始终保留一个结尾空字符。
    std::array<char, Capacity + 1> data_{};
    size_t size_ = 0;
  };

  // 固定数组与有效数量的组合，迭代只访问有效元素。
  template <typename T, size_t Capacity>
  class FixedList {
   public:
    /**
     * @brief 将完整元素复制到数组末尾
     * @param value 待保存元素
     * @return 数组已满时返回 false，已有元素不变
     */
    bool PushBack(const T& value) {
      if (size_ == Capacity) {
        return false;
      }
      data_[size_++] = value;
      return true;
    }
    void clear() noexcept { size_ = 0; }
    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    // 下标访问要求 index < size()；解析器在访问前检查数量。
    const T& operator[](size_t index) const noexcept { return data_[index]; }
    T& operator[](size_t index) noexcept { return data_[index]; }
    const T* begin() const noexcept { return data_.data(); }
    const T* end() const noexcept { return data_.data() + size_; }
    T* begin() noexcept { return data_.data(); }
    T* end() noexcept { return data_.data() + size_; }

   private:
    // clear 只清除有效数量，后续写入会覆盖旧元素。
    std::array<T, Capacity> data_{};
    size_t size_ = 0;
  };

  using Text = FixedText<32>;

  // TXT 正文在所属更新或聚合槽的共享缓冲区中的位置。
  struct TextRange {
    size_t offset = 0;
    size_t length = 0;
  };

  // 明确表示字段是否存在，避免用 -1 填充无符号类型。
  template <typename T>
  struct OptionalValue {
    T value{};
    bool valid = false;
  };

  // 已通过校验和检查的 NMEA 语句标识。
  struct SentenceMetadata {
    FixedText<5> address;
    FixedText<2> talker_id;
    FixedText<4> formatter;
  };

  // UTC 时间，second 保留 NMEA 小数秒。
  struct UtcTime {
    uint8_t hour = 0;
    uint8_t minute = 0;
    double second = 0.0;
    bool valid = false;
  };

  // UTC 日期；RMC/PUBX,04 的两位年份会将 two_digit_year 置为 true。
  struct UtcDate {
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;
    bool two_digit_year = false;
    bool valid = false;
  };

  // 经纬度坐标，decimal_degrees 已根据 S/W 方向保存为负数。
  struct Coordinate {
    uint16_t degrees = 0;
    double minutes = 0.0;
    double decimal_degrees = 0.0;
    char direction = '\0';
    bool valid = false;
  };

  // 纬度与经度组合。
  struct Position {
    Coordinate latitude;
    Coordinate longitude;
  };

  // DTM：当前大地基准相对参考基准的偏移。
  struct Dtm {
    SentenceMetadata metadata;
    Text local_datum;
    Text local_datum_subdivision;
    OptionalValue<double> latitude_offset_minutes;
    OptionalValue<char> latitude_direction;
    OptionalValue<double> longitude_offset_minutes;
    OptionalValue<char> longitude_direction;
    OptionalValue<double> altitude_offset_meters;
    Text reference_datum;
    bool valid = false;
  };

  // GBS：RAIM 卫星故障检测结果。
  struct Gbs {
    SentenceMetadata metadata;
    UtcTime utc;
    OptionalValue<double> latitude_error_meters;
    OptionalValue<double> longitude_error_meters;
    OptionalValue<double> altitude_error_meters;
    OptionalValue<uint16_t> failed_satellite_id;
    OptionalValue<double> missed_detection_probability;
    OptionalValue<double> estimated_bias_meters;
    OptionalValue<double> bias_standard_deviation_meters;
    OptionalValue<uint8_t> system_id;
    OptionalValue<uint8_t> signal_id;
    bool valid = false;
  };

  // GGA：定位时间、位置和定位质量。
  struct Gga {
    SentenceMetadata metadata;
    UtcTime utc;
    Position position;
    OptionalValue<uint8_t> fix_quality;
    OptionalValue<uint8_t> satellites_used;
    OptionalValue<float> hdop;
    OptionalValue<double> altitude_meters;
    OptionalValue<double> geoid_separation_meters;
    OptionalValue<double> differential_age_seconds;
    OptionalValue<Text> differential_station_id;
    bool valid = false;
  };

  // GLL：位置、定位时间和有效状态。
  struct Gll {
    SentenceMetadata metadata;
    Position position;
    UtcTime utc;
    OptionalValue<char> data_status;
    OptionalValue<char> positioning_mode;
    bool valid = false;
  };

  // GNS：多星系定位时间、位置和解算状态。
  struct Gns {
    SentenceMetadata metadata;
    UtcTime utc;
    Position position;
    OptionalValue<Text> positioning_modes;
    OptionalValue<uint8_t> satellites_used;
    OptionalValue<float> hdop;
    OptionalValue<double> altitude_meters;
    OptionalValue<double> geoid_separation_meters;
    OptionalValue<double> differential_age_seconds;
    OptionalValue<Text> differential_station_id;
    OptionalValue<char> navigational_status;
    bool valid = false;
  };

  // GRS：与同历元 GSA 卫星顺序对应的伪距残差。
  struct Grs {
    SentenceMetadata metadata;
    UtcTime utc;
    OptionalValue<uint8_t> residual_mode;
    std::array<OptionalValue<float>, 12> residuals_meters{};
    OptionalValue<uint8_t> system_id;
    OptionalValue<uint8_t> signal_id;
    bool valid = false;
  };

  // GSA：定位模式、参与解算的卫星和精度因子。
  struct Gsa {
    SentenceMetadata metadata;
    OptionalValue<char> operation_mode;
    OptionalValue<uint8_t> navigation_mode;
    FixedList<uint16_t, 12> satellite_ids;
    OptionalValue<float> pdop;
    OptionalValue<float> hdop;
    OptionalValue<float> vdop;
    OptionalValue<uint8_t> system_id;
    bool valid = false;
  };

  // GST：伪距误差统计和位置误差标准差。
  struct Gst {
    SentenceMetadata metadata;
    UtcTime utc;
    OptionalValue<double> range_rms_meters;
    OptionalValue<double> semi_major_deviation_meters;
    OptionalValue<double> semi_minor_deviation_meters;
    OptionalValue<double> semi_major_orientation_degrees;
    OptionalValue<double> latitude_deviation_meters;
    OptionalValue<double> longitude_deviation_meters;
    OptionalValue<double> altitude_deviation_meters;
    bool valid = false;
  };

  // GSV：同一发送设备和信号 ID 的完整多语句卫星集合。
  struct Gsv {
    struct Satellite {
      OptionalValue<uint16_t> id;
      OptionalValue<int16_t> elevation_degrees;
      OptionalValue<uint16_t> azimuth_degrees;
      OptionalValue<uint8_t> carrier_to_noise_db_hz;
    };

    SentenceMetadata metadata;
    uint8_t total_sentence_count = 0;
    uint8_t received_sentence_count = 0;
    uint8_t total_satellite_count = 0;
    OptionalValue<uint8_t> signal_id;
    FixedList<Satellite, 36> satellites;
    bool complete = false;
    bool valid = false;
  };

  // RLM：Galileo 搜救返回链路消息。
  struct Rlm {
    SentenceMetadata metadata;
    Text beacon_id;
    UtcTime utc;
    OptionalValue<char> message_code;
    FixedText<kMaxSentenceLength> message_body;
    bool valid = false;
  };

  // RMC：推荐的最小导航数据。
  struct Rmc {
    SentenceMetadata metadata;
    UtcTime utc;
    OptionalValue<char> data_status;
    Position position;
    OptionalValue<float> speed_over_ground_knots;
    OptionalValue<float> course_over_ground_degrees;
    UtcDate date;
    OptionalValue<float> magnetic_variation_degrees;
    OptionalValue<char> magnetic_variation_direction;
    OptionalValue<char> positioning_mode;
    OptionalValue<char> navigational_status;
    bool valid = false;
  };

  // TXT：一次完整文本传输；messages 按语句编号排序。
  struct Txt {
    SentenceMetadata metadata;
    uint8_t total_sentence_count = 0;
    OptionalValue<uint8_t> text_id;
    FixedList<TextRange, kMaxTextSentenceCount> messages;
    bool complete = false;
    bool valid = false;
  };

  // VLW：累计和复位后的对地/对水里程。
  struct Vlw {
    SentenceMetadata metadata;
    OptionalValue<double> total_water_distance_nautical_miles;
    OptionalValue<double> water_distance_nautical_miles;
    OptionalValue<double> total_ground_distance_nautical_miles;
    OptionalValue<double> ground_distance_nautical_miles;
    bool valid = false;
  };

  // VTG：对地航向和速度。
  struct Vtg {
    SentenceMetadata metadata;
    OptionalValue<float> true_course_degrees;
    OptionalValue<float> magnetic_course_degrees;
    OptionalValue<float> speed_knots;
    OptionalValue<float> speed_kilometers_per_hour;
    OptionalValue<char> positioning_mode;
    bool valid = false;
  };

  // ZDA：UTC 日期时间和本地时区。
  struct Zda {
    SentenceMetadata metadata;
    UtcTime utc;
    UtcDate date;
    OptionalValue<int8_t> local_zone_hours;
    OptionalValue<uint8_t> local_zone_minutes;
    bool valid = false;
  };

  // PUBX,00：u-blox 经纬度定位数据。
  struct PubxPosition {
    SentenceMetadata metadata;
    UtcTime utc;
    Position position;
    OptionalValue<double> altitude_above_user_datum_meters;
    OptionalValue<Text> navigation_status;
    OptionalValue<float> horizontal_accuracy_meters;
    OptionalValue<float> vertical_accuracy_meters;
    OptionalValue<float> speed_kilometers_per_hour;
    OptionalValue<float> course_over_ground_degrees;
    OptionalValue<float> vertical_velocity_meters_per_second;
    OptionalValue<float> differential_age_seconds;
    OptionalValue<float> hdop;
    OptionalValue<float> vdop;
    OptionalValue<float> tdop;
    OptionalValue<uint8_t> satellites_used;
    OptionalValue<bool> dead_reckoning_used;
    bool valid = false;
  };

  // PUBX,03：u-blox 卫星跟踪状态。
  struct PubxSatelliteStatus {
    struct Satellite {
      OptionalValue<uint16_t> id;
      OptionalValue<char> status;
      OptionalValue<uint16_t> azimuth_degrees;
      OptionalValue<int16_t> elevation_degrees;
      OptionalValue<uint8_t> carrier_to_noise_db_hz;
      OptionalValue<uint8_t> carrier_lock_time_seconds;
    };

    SentenceMetadata metadata;
    uint8_t reported_satellite_count = 0;
    FixedList<Satellite, 32> satellites;
    bool valid = false;
  };

  // PUBX,04：u-blox UTC、周计数和接收机时钟信息。
  struct PubxTime {
    SentenceMetadata metadata;
    UtcTime utc;
    UtcDate date;
    OptionalValue<double> utc_time_of_week_seconds;
    OptionalValue<uint16_t> utc_week_number;
    OptionalValue<int8_t> leap_seconds;
    bool leap_seconds_are_default = false;
    OptionalValue<int64_t> clock_bias_nanoseconds;
    OptionalValue<double> clock_drift_nanoseconds_per_second;
    OptionalValue<int32_t> time_pulse_granularity_nanoseconds;
    bool valid = false;
  };

  // 单次 Feed 调用生成的更新；重复语句均有固定数量上限。
  struct Update {
    Dtm dtm;
    Gbs gbs;
    Gga gga;
    Gll gll;
    Gns gns;
    FixedList<Grs, kMaxRepeatedUpdates> grs;
    FixedList<Gsa, kMaxRepeatedUpdates> gsa;
    Gst gst;
    FixedList<Gsv, kMaxRepeatedUpdates> gsv;
    Rlm rlm;
    Rmc rmc;
    FixedList<Txt, kMaxRepeatedUpdates> txt;
    Vlw vlw;
    Vtg vtg;
    Zda zda;
    PubxPosition pubx_position;
    PubxSatelliteStatus pubx_satellite_status;
    PubxTime pubx_time;

    /**
     * @brief 清除本次更新并保留重复语句容器容量以便复用
     */
    void Clear();

    /**
     * @brief 获取本次更新中一条 TXT 正文
     * @param text 本次更新中的完整 TXT 组，不得使用其他更新的组
     * @param index 组内语句下标
     * @return 空字符结尾正文，参数越界返回 nullptr；下次 Feed 或 Reset 后失效
     */
    const char* GetTextMessage(const Txt& text, size_t index) const noexcept;

    /**
     * @brief 判断本次更新是否包含至少一条已解析语句
     * @return 包含有效更新时返回 true
     */
    bool HasData() const;

   private:
    friend class NmeaParser;
    // 所有完整 TXT 组共享此缓冲区，组内只存偏移和长度。
    std::array<char, kTextBufferSize> text_buffer_{};
    size_t text_buffer_used_ = 0;
  };

  // 单次 Feed 调用的解析结果。
  struct FeedResult {
    size_t input_bytes = 0;
    uint32_t completed_sentences = 0;
    uint32_t parsed_sentences = 0;
    uint32_t checksum_errors = 0;
    uint32_t format_errors = 0;
    uint32_t unsupported_sentences = 0;
    uint32_t overflow_errors = 0;
    uint32_t capacity_errors = 0;

    /**
     * @brief 判断本次输入是否生成有效解析结果
     * @return 至少解析一条支持的语句时返回 true
     */
    bool HasParsedSentence() const { return parsed_sentences != 0; }
  };

  // 解析器生命周期内累计的诊断计数。
  struct Statistics {
    uint64_t input_bytes = 0;
    uint32_t completed_sentences = 0;
    uint32_t parsed_sentences = 0;
    uint32_t checksum_errors = 0;
    uint32_t format_errors = 0;
    uint32_t unsupported_sentences = 0;
    uint32_t overflow_errors = 0;
    uint32_t capacity_errors = 0;
  };

  /**
   * @brief 一次性申请固定存储，创建后需通过 IsReady 检查分配结果
   */
  NmeaParser();

  /**
   * @brief 释放解析器内部流状态
   */
  ~NmeaParser();

  /**
   * @brief 转移另一个解析器的流状态
   * @param other 被转移的解析器
   */
  NmeaParser(NmeaParser&& other) noexcept;

  /**
   * @brief 释放当前状态并接管另一个解析器的流状态
   * @param other 被转移的解析器
   * @return 当前解析器引用
   */
  NmeaParser& operator=(NmeaParser&& other) noexcept;

  /**
   * @brief 禁止复制解析器，避免复制未完成帧和聚合状态
   */
  NmeaParser(const NmeaParser&) = delete;

  /**
   * @brief 禁止复制赋值解析器
   */
  NmeaParser& operator=(const NmeaParser&) = delete;

  /**
   * @brief 将任意长度 UART 数据送入流式 NMEA 解析器
   * @param data 输入字节，可包含分包、粘包和非 NMEA 文本
   * @param length 输入字节长度
   * @return 本次输入的解析统计，初始化失败或容量超限计入 capacity_errors
   */
  FeedResult Feed(const uint8_t* data, size_t length);

  /**
   * @brief 检查构造时是否成功分配全部固定存储
   * @return 存储可用时返回 true，失败时不会在 Feed 中重试分配
   */
  bool IsReady() const noexcept;

  /**
   * @brief 获取解析器拥有的本次更新，避免在任务栈上创建大结果对象
   * @return 初始化失败返回 nullptr；内容在下次 Feed 或 Reset 时更新
   */
  const Update* update() const noexcept;

  /**
   * @brief 清除未完成帧、多语句聚合状态和累计统计
   */
  void Reset();

  /**
   * @brief 获取解析器生命周期内的累计统计
   * @return 累计统计只读引用
   */
  const Statistics& GetStatistics() const;

 private:
  struct FieldView;
  struct SentenceView;
  struct GsvAssembly;
  struct TxtAssembly;
  struct State;

  /**
   * @brief 处理接收缓冲区中的一条完整语句
   * @param update 本次解析更新
   * @param result 本次解析统计
   */
  void ParseBufferedSentence(Update* update, FeedResult* result);

  /**
   * @brief 校验当前语句并创建零拷贝字段视图
   * @param sentence 返回拆分后的语句视图
   * @param checksum_error 返回失败是否由校验和引起
   * @return 语句格式和校验和均有效时返回 true
   */
  bool BuildSentenceView(SentenceView* sentence, bool* checksum_error);

  /**
   * @brief 按标准 formatter 或 PUBX 消息编号分发语句
   * @param sentence 已校验语句
   * @param update 本次解析更新
   * @return 支持并成功解析时返回 true
   */
  bool DispatchSentence(const SentenceView& sentence, Update* update);

  /**
   * @brief 解析 DTM 大地基准偏移语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseDtm(const SentenceView& sentence, Dtm* output) const;
  /**
   * @brief 解析 GBS RAIM 故障检测语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseGbs(const SentenceView& sentence, Gbs* output) const;
  /**
   * @brief 解析 GGA 定位质量语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseGga(const SentenceView& sentence, Gga* output) const;
  /**
   * @brief 解析 GLL 经纬度语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseGll(const SentenceView& sentence, Gll* output) const;
  /**
   * @brief 解析 GNS 多星系定位语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseGns(const SentenceView& sentence, Gns* output) const;
  /**
   * @brief 解析 GRS 伪距残差语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseGrs(const SentenceView& sentence, Grs* output) const;
  /**
   * @brief 解析 GSA 有效卫星与精度因子语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseGsa(const SentenceView& sentence, Gsa* output) const;
  /**
   * @brief 解析 GST 伪距误差统计语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseGst(const SentenceView& sentence, Gst* output) const;
  /**
   * @brief 解析并聚合 GSV 多语句卫星信息
   * @param sentence 已校验语句
   * @param update 本次解析更新
   * @return 当前分段有效并成功写入聚合状态时返回 true
   */
  bool ParseGsv(const SentenceView& sentence, Update* update);
  /**
   * @brief 解析 RLM 搜救返回链路语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseRlm(const SentenceView& sentence, Rlm* output) const;
  /**
   * @brief 解析 RMC 最小导航数据语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseRmc(const SentenceView& sentence, Rmc* output) const;
  /**
   * @brief 解析并聚合 TXT 多语句文本
   * @param sentence 已校验语句
   * @param update 本次解析更新
   * @return 当前分段有效并成功写入聚合状态时返回 true
   */
  bool ParseTxt(const SentenceView& sentence, Update* update);
  /**
   * @brief 解析 VLW 对地和对水里程语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseVlw(const SentenceView& sentence, Vlw* output) const;
  /**
   * @brief 解析 VTG 对地航向和速度语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseVtg(const SentenceView& sentence, Vtg* output) const;
  /**
   * @brief 解析 ZDA UTC 日期时间语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParseZda(const SentenceView& sentence, Zda* output) const;
  /**
   * @brief 解析 PUBX,00 经纬度定位语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParsePubxPosition(
      const SentenceView& sentence, PubxPosition* output) const;
  /**
   * @brief 解析 PUBX,03 卫星状态语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParsePubxSatelliteStatus(
      const SentenceView& sentence, PubxSatelliteStatus* output) const;
  /**
   * @brief 解析 PUBX,04 时间与时钟语句
   * @param sentence 已校验语句
   * @param output 解析结果输出指针
   * @return 字段数量、格式和取值均有效时返回 true
   */
  bool ParsePubxTime(const SentenceView& sentence, PubxTime* output) const;

  // 构造时一次性分配流缓冲、字段视图、聚合槽、解析暂存和更新结果。
  std::unique_ptr<State> state_;
};

}  // namespace cpp_bus_driver
