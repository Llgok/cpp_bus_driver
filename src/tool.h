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
   * @return
   * @Date 2025-03-26 16:41:35
   */
  bool Search(const uint8_t* search_library, size_t search_library_length,
      const char* search_sample, size_t sample_length,
      size_t* search_index = nullptr);
  bool Search(const char* search_library, size_t search_library_length,
      const char* search_sample, size_t sample_length,
      size_t* search_index = nullptr);

  bool SafeStoi(const std::string& input, long* output);
  bool SafeStof(const std::string& input, float* output);
  bool SafeStod(const std::string& input, double* output);

  bool SetGpioMode(
      uint32_t pin, GpioMode mode, GpioStatus status = GpioStatus::kDisable);
  bool GpioWrite(uint32_t pin, bool value);
  bool GpioRead(uint32_t pin);

  void DelayMs(uint32_t value);
  void DelayUs(uint32_t value);

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
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

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
class Pwm : public Tool {
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
   * @return
   * @Date 2025-03-19 10:02:18
   */
  bool SetDuty(uint8_t duty);

  /**
   * @brief 设置pwm频率
   * @param freq_hz pwm频率
   * @return
   * @Date 2026-04-16 10:58:15
   */
  bool SetFrequency(uint32_t freq_hz);

  /**
   * @brief 设置渐变占空比
   * @param target_duty 值范围 0~100
   * @param time_ms 渐变花费时间
   * @return
   * @Date 2025-03-19 10:03:07
   */
  bool StartGradientTime(uint8_t target_duty, int32_t time_ms);

  /**
   * @brief 停止pwm
   * @param idle_level pwm停止后空闲状态下pwm的输出电平
   * @return
   * @Date 2026-04-16 11:05:24
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

class Gnss : public Tool {
 public:
  struct Rmc {
    struct {
      uint8_t hour = -1;    // 小时
      uint8_t minute = -1;  // 分钟
      float second = -1;    // 秒

      bool update_flag = false;
    } utc;

    // 定位系统状态。
    // A = 数据有效
    // V = 无效
    // D = 差分
    std::string location_status;

    struct {
      struct {
        uint8_t degrees = -1;         // 度
        float minutes = -1;           // 分
        double degrees_minutes = -1;  // 带小数的度分转度

        // 纬度方向
        // N = 北
        // S = 南
        std::string direction;

        bool update_flag = false;            // 度分更新标志
        bool direction_update_flag = false;  // 方向更新标志
      } lat;

      struct {
        uint8_t degrees = -1;
        float minutes = -1;
        double degrees_minutes = -1;

        // 经度方向
        // E = 东
        // W = 西
        std::string direction;

        bool update_flag = false;
        bool direction_update_flag = false;
      } lon;

    } location;

    struct {
      uint8_t day = -1;    // 日
      uint8_t month = -1;  // 月
      uint8_t year = -1;   // 年

      bool update_flag = false;
    } data;
  };

  struct Gga {
    struct {
      uint8_t hour = -1;    // 小时
      uint8_t minute = -1;  // 分钟
      float second = -1;    // 秒

      bool update_flag = false;
    } utc;

    struct {
      struct {
        uint8_t degrees = -1;         // 度
        float minutes = -1;           // 分
        double degrees_minutes = -1;  // 带小数的度分转度

        // 纬度方向
        // N = 北
        // S = 南
        std::string direction;

        bool update_flag = false;            // 度分更新标志
        bool direction_update_flag = false;  // 方向更新标志
      } lat;

      struct {
        uint8_t degrees = -1;
        float minutes = -1;
        double degrees_minutes = -1;

        // 经度方向
        // E = 东
        // W = 西
        std::string direction;

        bool update_flag = false;
        bool direction_update_flag = false;
      } lon;

    } location;

    // kGps 定位模式/状态指示
    // 0 = 定位不可用或无效
    // 1 = kGps kSps 模式，定位有效
    // 2 = 差分 kGps、kSps 模式或 SBAS定位有效
    // 6 = 估算（航位推算）模式
    uint8_t gps_mode_status = -1;
    uint8_t online_satellite_count = -1;  // 在线的卫星数量

    float hdop = -1;
  };

  Gnss() = default;

  /**
   * @brief 解析rmc信息
   * @param data 要解析的数据
   * @param length 要解析的数据长度
   * @param &rmc 返回的解析结构体
   * @return
   * @Date 2025-02-18 11:54:34
   */
  bool ParseRmcInfo(const uint8_t* data, size_t length, Rmc& rmc);

  /**
   * @brief 解析gga信息
   * @param data 要解析的数据
   * @param length 要解析的数据长度
   * @param &gga 返回的解析结构体
   * @return
   * @Date 2025-02-18 11:54:34
   */
  bool ParseGgaInfo(const uint8_t* data, size_t length, Gga& gga);
};

}  // namespace cpp_bus_driver