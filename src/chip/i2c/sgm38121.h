
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-17 14:02:46
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Sgm38121 final : public ChipI2cGuide {
 public:
  enum class Channel {
    kDvdd1,
    kDvdd2,
    kAvdd1,
    kAvdd2,
  };

  enum class Status {
    kOff = 0,
    kOn,
  };

  explicit Sgm38121(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  uint8_t GetDeviceId();

  /**
   * @brief 设置输出电压
   * @param channel 使用Channel::配置
   * @param voltage DVDD_1和DVDD_2取值528~1504，AVDD_1和AVDD_2取值1504~3424
   * @return
   * @Date 2025-07-17 10:27:58
   */
  bool SetOutputVoltage(Channel channel, uint16_t voltage);

  /**
   * @brief 设置通道状态
   * @param channel 使用Channel::配置
   * @param status 使用Status::配置
   * @return
   * @Date 2025-07-17 10:29:45
   */
  bool SetChannelStatus(Channel channel, Status status);

 private:
  enum class Cmd {
    kRoDeviceId = 0x00,

    kRwDischargeResistorSelection = 0X02,
    kRwDvdd1OutputVoltageLevel,
    kRwDvdd2OutputVoltageLevel,
    kRwAvdd1OutputVoltageLevel,
    kRwAvdd2OutputVoltageLevel,
    kRwFunction,

    kRwPowerSequenceSetting1 = 0X0A,
    kRwPowerSequenceSetting2,

    kRwEnableControl = 0X0E,
    kRwSequenceControl,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x28;
  static constexpr uint8_t kDeviceId = 0x80;

  int32_t rst_;
};
}  // namespace cpp_bus_driver