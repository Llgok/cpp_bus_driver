/*
 * @Description: SGM41562 系列电池充电管理芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:44:29
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Sgm41562xx final : public ChipI2cGuide {
 public:
  enum class ChargeStatus {
    kNotCharging = 0,
    kPrecharge,
    kCharge,
    kChargeDone,
  };

  enum class EnterShippingTime {
    kWait1s = 0,
    kWait2s,
    kWait4s,
    kWait8s,
  };

  // 中断状态
  struct IrqStatus {
    bool InputPowerFaultFlag = false;                 // 输入电源故障标志
    bool thermal_shutdown_flag = false;               // 过热关断标志
    bool battery_over_voltage_fault_flag = false;     // 电池过压故障标志
    bool safety_timer_expiration_fault_flag = false;  // 安全定时器到期故障标志
    bool ntc_exceeding_hot_flag = false;              // ntc过热标志
    bool ntc_exceeding_cold_flag = false;             // ntc过冷标志
  };

  // 芯片状态
  struct ChipStatus {
    bool watchdog_expiration_flag = false;                    // 看门狗超时标志
    ChargeStatus charge_status = ChargeStatus::kNotCharging;  // 充电状态标志
    // 设备在电源路径管理模式标志
    bool device_in_power_path_management_mode_flag = false;
    // 输入电源状态标志（[1] = 电源是好的 [0] = 电源是不好的）
    bool input_power_status_flag = false;
    bool thermal_regulation_status_flag = false;  // 热调节状态
  };

  explicit Sgm41562xx(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;
  uint8_t GetDeviceId();

  /**
   * @brief 获取中断标志
   * @return 返回读取到的数值
   */
  uint8_t GetIrqFlag();

  /**
   * @brief 解析中断状态，详见 SGM41562 手册表 13 的 kReg09 寄存器说明
   * @param irq_flag GetIrqFlag() 返回的中断状态值
   * @param status 用于保存各中断状态位的 IrqStatus 结构体
   * @return 解析成功返回 true，失败返回 false
   */
  bool ParseIrqStatus(uint8_t irq_flag, IrqStatus& status);

  /**
   * @brief 设置充电使能
   * @param enable [true]：打开充电 [false]：关闭充电
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetChargeEnable(bool enable);

  /**
   * @brief 获取芯片状态
   * @return 返回读取到的数值
   */
  uint8_t GetChipStatus();

  /**
   * @brief 解析芯片状态，详见 SGM41562 手册表 12 的 kReg08 寄存器说明
   * @param chip_flag GetChipStatus() 返回的状态值
   * @param status 用于保存各状态位的 ChipStatus 结构体
   * @return 解析成功返回 true，失败返回 false
   */
  bool ParseChipStatus(uint8_t chip_flag, ChipStatus& status);

  /**
   * @brief 设置开启运输模式
   * @param enable [true]：开启运输模式 [false]：关闭运输模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetShippingModeEnable(bool enable);

  /**
   * @brief 设置进入运输模式的时间
   * @param time 进入运输模式前的等待时间
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetEnterShippingTime(EnterShippingTime time);

 private:
  enum class Cmd {
    kRoDeviceId = 0x0B,

    kRwInputSourceControl = 0x00,             // 输入源控制寄存器
    kRwPowerOnConfiguration,                  // 上电配置寄存器
    kRwChargeCurrentControl,                  // 充电电流控制寄存器
    kRwDischargeTerminationCurrent,           // 放电/终止电流寄存器
    kRwChargeVoltageControl,                  // 充电电压控制寄存器
    kRwChargeTerminationTimerControl,         // 充电终止/定时器控制寄存器
    kRwMiscellaneousOperationControl,         // 杂项操作控制寄存器
    kRwSystemVoltageRegulation,               // 系统电压调节寄存器
    kRdSystemStatus,                          // 系统状态寄存器
    kRdFault,                                 // 故障寄存器
    kRwI2cAddressMiscellaneousConfiguration,  // I2c地址及杂项配置寄存器
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x03;
  static constexpr uint8_t kDeviceId = 0x04;
  static constexpr uint8_t kInitSequence[] = {

      // 如需复位，可将充电电流控制寄存器设置为 0B11001111。

      static_cast<uint8_t>(InitSequenceFormat::kDelayMs), 120,

      // 关闭PCB OTP
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwSystemVoltageRegulation), 0B10110111,

      // 关闭NTC
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwMiscellaneousOperationControl), 0B01000000,

      // 如需屏蔽中断，可将杂项操作控制寄存器设置为 0B11011111。

      // 充电电流权重限制可通过 I2C 地址及杂项寄存器的 0B01100001 配置。

      // 关闭看门狗功能
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), 0B00011010,

      // 开启电池充电功能
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwPowerOnConfiguration), 0B10100100,

      // 关闭充电可将上电配置寄存器设置为 0B10101100。

      // 关闭输入电流限制
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRdSystemStatus), 0B01000000

      // 电流限制模式下，0B00100000 可增加 200 mA 输入电流阈值。

  };

  int32_t rst_;
};
}  // namespace cpp_bus_driver
