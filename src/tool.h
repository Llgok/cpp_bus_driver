/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 17:58:03
 * @LastEditTime: 2026-04-30 13:47:39
 * @License: GPL 3.0
 */
#pragma once

#include "config.h"

namespace cpp_bus_driver {
namespace safe_convert {

/**
 * @brief 安全地将字符串转换为整数
 * @param input 输入字符串
 * @param output 输出转换后的整数
 * @return 转换成功返回 true
 */
bool SafeStringToLong(const std::string& input, long* output);

/**
 * @brief 安全地将字符串转换为浮点数
 * @param input 输入字符串
 * @param output 输出转换后的浮点数
 * @return 转换成功返回 true
 */
bool SafeStringToFloat(const std::string& input, float* output);

/**
 * @brief 安全地将字符串转换为双精度浮点数
 * @param input 输入字符串
 * @param output 输出转换后的双精度浮点数
 * @return 转换成功返回 true
 */
bool SafeStringToDouble(const std::string& input, double* output);

}  // namespace safe_convert

class Tool {
 public:
  enum class LogLevel {
    kDebug,  // debug信息
    kInfo,   // 普通信息

    kBus,   // 总线错误
    kChip,  // 芯片错误
  };

  enum class InterruptMode {
    kDisable,
    kRising,
    kFalling,
    kChange,
    kOnLow,
    kOnHigh,
  };

  enum class GpioMode {
    kDisable,
    kInput,          // input only
    kOutput,         // output only mode
    kOutputOd,       // output only with open-drain mode
    kInputOutputOd,  // output and input with open-drain mode
    kInputOutput,    // output and input mode
  };

  enum class GpioStatus {
    kDisable,
    kPullup,
    kPulldown,
  };

  Tool() = default;
  virtual ~Tool() = default;

  void LogMessage(LogLevel level, const char* file_name, size_t line_number,
      const char* format, ...);

  /**
   * @brief 搜索函数
   * @param *search_library 需要使用的搜索库
   * @param search_library_length 需要使用的搜索库长度
   * @param *search_sample 需要搜索的样本
   * @param sample_length 需要搜索的样本长度
   * @param *search_index 搜索成功的的样本中第一个搜索成功字符位置的引索
   * @return 成功返回 true，失败返回 false
   */
  bool Search(const uint8_t* search_library, size_t search_library_length,
      const char* search_sample, size_t sample_length,
      size_t* search_index = nullptr);
  bool Search(const char* search_library, size_t search_library_length,
      const char* search_sample, size_t sample_length,
      size_t* search_index = nullptr);

  bool SetGpioMode(
      uint32_t pin, GpioMode mode, GpioStatus status = GpioStatus::kDisable);
  bool GpioWrite(uint32_t pin, bool value);
  bool GpioRead(uint32_t pin);
  bool ResetGpio(int32_t pin);

  void DelayMs(uint32_t value);
  void DelayUs(uint32_t value);

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  int64_t GetSystemTimeUs();
  int64_t GetSystemTimeMs();

  bool InitGpioInterrupt(uint32_t pin, InterruptMode mode,
      void (*interrupt)(void* arg), void* args = nullptr);
  bool DeinitGpioInterrupt(uint32_t pin);
#endif

 protected:
  enum class InitSequenceFormat {
    kDelayMs,
    kWriteC8,
    kWriteC8ByteData,
    kWriteC8D8,
    kWriteC8R24,
    kWriteC8R24D8,
    kWriteC16D8,
  };

  enum class Endian {
    kBig,
    kLittle,
  };

 private:
  static constexpr uint16_t kMaxLogBufferSize = 1024;
};

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
class Pwm : public virtual Tool {
 public:
  explicit Pwm(int32_t pin) : pin_(pin) {}

  bool Init(ledc_timer_t timer_num, ledc_channel_t channel, uint32_t freq_hz,
      uint32_t duty = 0,
      ledc_mode_t speed_mode = ledc_mode_t::LEDC_LOW_SPEED_MODE,
      ledc_timer_bit_t duty_resolution = ledc_timer_bit_t::LEDC_TIMER_10_BIT,
      ledc_sleep_mode_t sleep_mode =
          ledc_sleep_mode_t::LEDC_SLEEP_MODE_NO_ALIVE_NO_PD);

  /**
   * @brief 设置pwm占空比
   * @param duty 值范围 0~100
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDuty(uint8_t duty);

  /**
   * @brief 设置pwm频率
   * @param freq_hz pwm频率
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetFrequency(uint32_t freq_hz);

  /**
   * @brief 设置渐变占空比
   * @param target_duty 值范围 0~100
   * @param time_ms 渐变花费时间
   * @return 操作成功返回 true，失败返回 false
   */
  bool StartGradientTime(uint8_t target_duty, int32_t time_ms);

  /**
   * @brief 停止pwm
   * @param idle_level pwm停止后空闲状态下pwm的输出电平
   * @return 操作成功返回 true，失败返回 false
   */
  bool Stop(uint32_t idle_level = 0);

 private:
  int32_t pin_;
  ledc_timer_t timer_num_;
  ledc_channel_t channel_;
  uint32_t freq_hz_;
  uint32_t duty_;
  ledc_mode_t speed_mode_;
  ledc_timer_bit_t duty_resolution_;
  ledc_sleep_mode_t sleep_mode_;
};

#endif

class GnssParser : public virtual Tool {
 public:
  // UTC 时间
  struct Utc {
    uint8_t hour = -1;
    uint8_t minute = -1;
    float second = -1;
    bool update_flag = false;
  };

  // UTC 日期
  struct Date {
    uint8_t day = -1;
    uint8_t month = -1;
    uint16_t year = -1;
    bool update_flag = false;
  };

  // 经纬度坐标，degrees_minutes 为十进制度
  struct Coordinate {
    uint8_t degrees = -1;
    float minutes = -1;
    double degrees_minutes = -1;
    std::string direction;
    bool update_flag = false;
    bool direction_update_flag = false;
  };

  // 纬度和经度组合
  struct Location {
    Coordinate lat;
    Coordinate lon;
  };

  struct Rmc {
    Utc utc;

    // 定位系统状态。
    // A = 数据有效
    // V = 无效
    // D = 差分
    std::string location_status;
    bool location_status_update_flag = false;

    Location location;

    float speed_over_ground_knots = -1;
    float course_over_ground_degree = -1;
    float magnetic_variation = -1;
    std::string magnetic_variation_direction;
    std::string mode_indicator;
    std::string navigational_status;

    Date data;
  };

  struct Gga {
    Utc utc;
    Location location;

    // kGps 定位模式/状态指示
    // 0 = 定位不可用或无效
    // 1 = kGps kSps 模式，定位有效
    // 2 = 差分 kGps、kSps 模式或 SBAS定位有效
    // 6 = 估算（航位推算）模式
    uint8_t gps_mode_status = -1;
    uint8_t online_satellite_count = -1;  // 在线的卫星数量

    float hdop = -1;
    float altitude = -1;
    std::string altitude_unit;
    float geoid_separation = -1;
    std::string geoid_separation_unit;
    float differential_age = -1;
    std::string differential_station_id;
  };

  struct Gsv {
    // GSV 中单颗可见卫星的信息
    struct Satellite {
      uint16_t id = -1;
      int16_t elevation = -1;
      int16_t azimuth = -1;
      int16_t cn0 = -1;
      uint8_t signal_id = -1;
      std::string talker_id;
    };

    uint8_t total_sentence_count = -1;
    uint8_t sentence_number = -1;
    uint8_t total_satellite_count = -1;
    uint8_t signal_id = -1;
    std::string talker_id;
    std::vector<Satellite> satellites;
    bool update_flag = false;
  };

  struct Gsa {
    // 单条 GSA 语句信息，不同星系可能输出多条
    struct Sentence {
      std::string talker_id;
      std::string selection_mode;
      uint8_t fix_mode = -1;
      std::vector<uint16_t> satellite_ids;
      float pdop = -1;
      float hdop = -1;
      float vdop = -1;
      uint8_t system_id = -1;
    };

    std::vector<Sentence> sentences;
    bool update_flag = false;
  };

  struct Vtg {
    float course_true_degree = -1;
    float course_magnetic_degree = -1;
    float speed_knots = -1;
    float speed_kmh = -1;
    std::string mode_indicator;
    bool update_flag = false;
  };

  struct Gll {
    Location location;
    Utc utc;
    std::string location_status;
    std::string mode_indicator;
    bool update_flag = false;
  };

  struct Txt {
    struct Sentence {
      uint8_t total_sentence_count = -1;
      uint8_t sentence_number = -1;
      uint8_t text_id = -1;
      std::string text;
    };

    std::vector<Sentence> sentences;
    bool update_flag = false;
  };

  struct Zda {
    Utc utc;
    Date date;
    int8_t local_hour = -1;
    int8_t local_minute = -1;
    bool update_flag = false;
  };

  struct Info {
    Rmc rmc;
    Gga gga;
    Gsv gsv;
    Gsa gsa;
    Vtg vtg;
    Gll gll;
    Txt txt;
    Zda zda;
  };

  GnssParser() = default;

  /**
   * @brief 解析rmc信息
   * @param data 要解析的数据
   * @param length 要解析的数据长度
   * @param &rmc 返回的解析结构体
   * @return 解析成功返回 true，失败返回 false
   */
  bool ParseRmcInfo(const uint8_t* data, size_t length, Rmc& rmc);

  /**
   * @brief 解析gga信息
   * @param data 要解析的数据
   * @param length 要解析的数据长度
   * @param &gga 返回的解析结构体
   * @return 解析成功返回 true，失败返回 false
   */
  bool ParseGgaInfo(const uint8_t* data, size_t length, Gga& gga);

  /**
   * @brief 解析 GSV 可见卫星信息
   * @param data 要解析的 NMEA 数据
   * @param length 数据长度
   * @param gsv 返回解析后的可见卫星信息
   * @return 成功解析到至少一个字段返回 true
   */
  bool ParseGsvInfo(const uint8_t* data, size_t length, Gsv& gsv);

  /**
   * @brief 解析 GSA DOP 和参与定位的卫星信息
   * @param data 要解析的 NMEA 数据
   * @param length 数据长度
   * @param gsa 返回解析后的 GSA 信息
   * @return 成功解析到至少一个字段返回 true
   */
  bool ParseGsaInfo(const uint8_t* data, size_t length, Gsa& gsa);

  /**
   * @brief 解析 VTG 地面航向和速度信息
   * @param data 要解析的 NMEA 数据
   * @param length 数据长度
   * @param vtg 返回解析后的 VTG 信息
   * @return 成功解析到至少一个字段返回 true
   */
  bool ParseVtgInfo(const uint8_t* data, size_t length, Vtg& vtg);

  /**
   * @brief 解析 GLL 经纬度、UTC 和定位状态信息
   * @param data 要解析的 NMEA 数据
   * @param length 数据长度
   * @param gll 返回解析后的 GLL 信息
   * @return 成功解析到至少一个字段返回 true
   */
  bool ParseGllInfo(const uint8_t* data, size_t length, Gll& gll);

  /**
   * @brief 解析 TXT 文本消息
   * @param data 要解析的 NMEA 数据
   * @param length 数据长度
   * @param txt 返回解析后的文本消息集合
   * @return 成功解析到至少一个字段返回 true
   */
  bool ParseTxtInfo(const uint8_t* data, size_t length, Txt& txt);

  /**
   * @brief 解析 ZDA UTC 日期时间信息
   * @param data 要解析的 NMEA 数据
   * @param length 数据长度
   * @param zda 返回解析后的 ZDA 信息
   * @return 成功解析到至少一个字段返回 true
   */
  bool ParseZdaInfo(const uint8_t* data, size_t length, Zda& zda);

  /**
   * @brief 一次解析 RMC、GGA、GSV、GSA、VTG、GLL、TXT 和 ZDA 信息
   * @param data 要解析的 NMEA 数据
   * @param length 数据长度
   * @param info 返回解析后的完整 GNSS 信息
   * @return 成功解析到任意一种信息返回 true
   */
  bool ParseInfo(const uint8_t* data, size_t length, Info& info);
};

}  // namespace cpp_bus_driver
