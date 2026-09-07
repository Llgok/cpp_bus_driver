/*
 * @Description: SGM38121 多通道 LDO 稳压器驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-08-03 16:11:24
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class Sgm38121 final : public I2cChipBase {
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

  explicit Sgm38121(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault)
      : I2cChipBase(bus, address) {}

  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取 SGM38121 芯片标识。
   * @return 芯片标识；读取失败返回 0xFF。
   */
  uint8_t GetChipId();

  /**
   * @brief 设置输出电压
   * @param channel 使用Channel::配置
   * @param voltage DVDD_1和DVDD_2取值528~1504，AVDD_1和AVDD_2取值1504~3424
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetOutputVoltage(Channel channel, uint16_t voltage);

  /**
   * @brief 设置通道状态
   * @param channel 使用Channel::配置
   * @param status 使用Status::配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetChannelStatus(Channel channel, Status status);

 private:
  // 默认 I2C 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  enum class Register {
    kRoChipId = 0x00,
    kRwDischargeResistorSelection = 0X02,
    kRwDvdd1OutputVoltageLevel = 0x03,
    kRwDvdd2OutputVoltageLevel = 0x04,
    kRwAvdd1OutputVoltageLevel = 0x05,
    kRwAvdd2OutputVoltageLevel = 0x06,
    kRwFunction = 0x07,
    kRwPowerSequenceSetting1 = 0X0A,
    kRwPowerSequenceSetting2 = 0x0B,
    kRwEnableControl = 0X0E,
    kRwSequenceControl = 0x0F,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x28;
  static constexpr uint8_t kChipId = 0x80;
};
}  // namespace cpp_bus_driver
