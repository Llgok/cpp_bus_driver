/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:47:17
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class L76k final : public ChipUartGuide, public Gnss {
 public:
  // 更新频率（定位频率）
  enum class UpdateFreq {
    kFreq1Hz,
    kFreq2Hz,
    kFreq5Hz,
  };

  // 波特率
  enum class BaudRate {
    kBr4800Bps,
    kBr9600Bps,
    kBr19200Bps,
    kBr38400Bps,
    kBr57600Bps,
    kBr115200Bps,
  };

  // 重启模式
  enum class RestartMode {
    kHotStart,
    kWarmStart,
    kColdStart,
    kColdStartFactoryReset,
  };

  // kGnss 星系
  enum class GnssConstellation {
    kGps,     // 美国全球定位系统
    kBeidou,  // 中国的全球卫星导航系统
    kGpsBeidou,
    kGlonass,  // 俄罗斯的全球卫星导航系统
    kGpsGlonass,
    kBeidouGlonass,
    kGpsBeidouGlonass,
  };

  explicit L76k(const std::shared_ptr<BusUartGuide> bus, const int32_t wake_up,
      const int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipUartGuide(bus), wake_up_(wake_up), rst_(rst) {}

  explicit L76k(const std::shared_ptr<BusUartGuide> bus,
      const std::function<bool(bool)>& wake_up_callback,
      const int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipUartGuide(bus), wake_up_callback_(wake_up_callback), rst_(rst) {}

  bool Init(int32_t baud_rate = 9600) override;
  bool Deinit() override;

  bool GetDeviceId(size_t* search_index = nullptr);

  /**
   * @brief 启动睡眠
   * @param enable [true]：进入睡眠，[false]：退出睡眠
   * @return
   * @Date 2025-02-14 16:18:00
   */
  bool Sleep(bool enable);

  /**
   * @brief 直接读取数据
   * @param *data 读取数据的指针
   * @param length 要读取数据的长度
   * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
   * @Date 2025-02-13 18:04:11
   */
  uint32_t ReadData(uint8_t* data, uint32_t length = 0);

  /**
   * @brief 获取接收缓存数据的长度
   * @return
   * @Date 2025-02-13 18:22:46
   */
  size_t GetRxBufferLength();

  /**
   * @brief 清除接收缓存中的所有数据
   * @return
   * @Date 2025-02-13 18:22:46
   */
  bool ClearRxBufferData();

  /**
   * @brief 获取信息数据
   * @param &data 获取数据的指针
   * @param *length 获取长度的指针
   * @param max_length 获取数据的最大长度
   * @param timeout_count 超时计数
   * @return
   * @Date 2025-03-24 10:03:31
   */
  bool GetInfoData(std::unique_ptr<uint8_t[]>& data, uint32_t* length,
      uint32_t max_length = kMaxReceiveSize,
      uint8_t timeout_count = kGetInformationTimeoutCount);

  /**
   * @brief 设置定位频率
   * @param freq 使用 UpdateFreq::配置，频率设定
   * @return
   * @Date 2025-02-14 18:18:57
   */
  bool SetUpdateFrequency(UpdateFreq freq);

  /**
   * @brief 设置模块和系统的波特率
   * @param baud_rate 使用 BaudRate::配置，波特率设定
   * @return
   * @Date 2025-02-17 13:45:17
   */
  bool SetBaudRate(BaudRate baud_rate);

  /**
   * @brief 获取系统的波特率
   * @return 波特率数据
   * @Date 2025-02-17 13:45:58
   */
  uint32_t GetBaudRate();

  /**
   * @brief 设置重启模式
   * @param mode 使用 RestartMode::配置
   * @return
   * @Date 2025-02-17 14:27:01
   */
  bool SetRestartMode(RestartMode mode);

  /**
   * @brief 设置GNSS的星系
   * @param constellation 使用 GnssConstellation::配置
   * @return
   * @Date 2025-02-17 14:45:15
   */
  bool SetGnssConstellation(GnssConstellation constellation);

 private:
  enum class Cmd {
    kRoDeviceId = 0x00,

  };

  static constexpr uint8_t kGetInformationTimeoutCount = 3;  // 获取信息超时计数
  static constexpr uint16_t kMaxReceiveSize = 1024 * 2;      // 最大接收尺寸

  int32_t wake_up_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  std::function<bool(bool)> wake_up_callback_ = nullptr;
  int32_t rst_;
  uint16_t update_freq_ = 1000;  // 默认更新频率为 1000ms（1Hz）
};
}  // namespace cpp_bus_driver
