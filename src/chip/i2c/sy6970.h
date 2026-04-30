/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-30 13:44:34
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Sy6970 final : public ChipI2cGuide {
 public:
  enum class ChargeStatus {
    kNotCharging = 0,
    kPrecharge,
    kFastCharging,
    kChargeDone,
  };

  enum class BusStatus {
    kNoInput = 0,
    kUsbHostSdp,
    kUsbCdp,
    kUsbDcp,
    kHvdcp,
    kUnknownAdapter,
    kNonStandardAdapter,
    kOtgMode,
  };

  enum class ChargeFault {
    kNormal = 0,
    kInputFault,
    kThermalShutdown,
    kSafetyTimerExpiration,
  };

  enum class NtcFault {
    kNormal = 0,

    kWarm = 2,
    kCool,

    kCold = 5,
    kHot,
  };

  struct IrqStatus {
    bool watchdog_expiration_flag = false;
    bool boost_fault_flag = false;
    ChargeFault charge_fault_status = ChargeFault::kNormal;
    bool battery_over_voltage_fault_flag = false;
    NtcFault ntc_fault_status = NtcFault::kNormal;
  };

  struct ChipStatus {
    BusStatus bus_status = BusStatus::kNoInput;
    ChargeStatus charge_status = ChargeStatus::kNotCharging;
    bool power_good_status = false;
    bool usb_status = false;
    bool system_voltage_regulation_status = false;
  };

  explicit Sy6970(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  uint8_t GetDeviceId();

  /**
   * @brief 开始ADC转换
   * @param enable [true]：开启ADC转换，[false]：关闭ADC转换
   * @return
   * @Date 2025-11-17 17:18:44
   */
  bool SetAdcConversionEnable(bool enable);

  /**
   * @brief 获取电池电压
   * @return 电池电压值 (mV)
   * @Date 2025-11-17 17:22:27
   */
  uint16_t GetBatteryVoltage();

  /**
       * @brief 读取系统电压
       * @return 系统电压值 (mV)
  @Date 2025-11-17 17:28:24
       */
  uint16_t GetSystemVoltage();

  /**
       * @brief 读取总线电压
       * @return 总线电压值 (mV)
  @Date 2025-11-17 17:28:24
       */
  uint16_t GetBusVoltage();

  /**
   * @brief 设置运输模式
   * @param enable [true]：开启运输模式，[false]：关闭运输模式
   * @return
   * @Date 2025-11-18 16:30:58
   */
  bool SetShippingModeEnable(bool enable);

  /**
   * @brief 获取充电电流
   * @return 充电电流值 (mA)
   * @Date 2025-11-18 16:30:58
   */
  uint16_t GetChargingCurrent();

  /**
   * @brief 获取中断标志
   * @return 中断标志寄存器值
   * @Date 2025-11-18 16:38:56
   */
  uint8_t GetIrqFlag();

  /**
   * @brief 中断状态解析
   * @param irq_flag 中断标志寄存器值
   * @param status 中断状态结构体引用
   * @return
   * @Date 2025-11-18 16:38:56
   */
  bool ParseIrqStatus(uint8_t irq_flag, IrqStatus& status);

  /**
   * @brief 获取芯片状态
   * @return 芯片状态寄存器值
   * @Date 2025-11-18 16:38:56
   */
  uint8_t GetChipStatus();

  /**
   * @brief 芯片状态解析
   * @param chip_flag 芯片状态寄存器值
   * @param status 使用ChipStatus::配置
   * @return
   * @Date 2025-11-18 16:38:56
   */

  bool ParseChipStatus(uint8_t chip_flag, ChipStatus& status);

  // /**
  //  * @brief 设置充电使能
  //  * @param enable true:使能充电, false:禁用充电
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetChargeEnable(bool enable);

  // /**
  //  * @brief 设置高阻态模式
  //  * @param enable true:使能高阻态, false:禁用高阻态
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetHizModeEnable(bool enable);

  // /**
  //  * @brief 设置ILIM引脚使能
  //  * @param enable true:使能ILIM引脚, false:禁用ILIM引脚
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetIlimPinEnable(bool enable);

  // /**
  //  * @brief 设置输入电流限制
  //  * @param current_ma 输入电流限制值 (100-3250mA)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetInputCurrentLimit(uint16_t current_ma);

  // /**
  //  * @brief 设置升压模式热温度监控阈值
  //  * @param threshold 阈值选择 (0:37.75%, 1:34.37%, 2/3:31.25%)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBoostHotThreshold(uint8_t threshold);

  // /**
  //  * @brief 设置升压模式冷温度监控阈值
  //  * @param threshold 阈值选择 (0:77%, 1:80%)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBoostColdThreshold(bool threshold);

  // /**
  //  * @brief 设置输入电压限制偏移
  //  * @param offset_mv 偏移电压值 (0-3100mV)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetVindpmOffset(uint16_t offset_mv);

  // /**
  //  * @brief 设置ADC转换速率
  //  * @param continuous true:连续转换, false:单次转换
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetAdcConversionRate(bool continuous);

  // /**
  //  * @brief 设置升压模式频率
  //  * @param high_freq true:1.5MHz, false:500kHz
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBoostFrequency(bool high_freq);

  // /**
  //  * @brief 设置自适应输入电流限制使能
  //  * @param enable true:使能AICL, false:禁用AICL
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetAdaptiveCurrentLimitEnable(bool enable);

  // /**
  //  * @brief 设置HVDCP使能
  //  * @param enable true:使能HVDCP, false:禁用HVDCP
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetHvdcpEnable(bool enable);

  // /**
  //  * @brief 设置HVDCP电压类型
  //  * @param high_voltage true:12V, false:9V
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetHvdcpVoltageType(bool high_voltage);

  // /**
  //  * @brief 强制DP/DM检测
  //  * @param force true:强制检测, false:不强制
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetForceDpdmDetection(bool force);

  // /**
  //  * @brief 设置自动DP/DM检测使能
  //  * @param enable true:使能自动检测, false:禁用自动检测
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetAutoDpdmDetectionEnable(bool enable);

  // /**
  //  * @brief 设置电池负载使能
  //  * @param enable true:使能电池负载, false:禁用电池负载
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBatteryLoadEnable(bool enable);

  // /**
  //  * @brief 看门狗定时器重置
  //  * @param reset true:重置看门狗, false:正常操作
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetWatchdogReset(bool reset);

  // /**
  //  * @brief 设置OTG模式
  //  * @param enable true:使能OTG, false:禁用OTG
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetOtgEnable(bool enable);

  // /**
  //  * @brief 设置最小系统电压限制
  //  * @param voltage_mv 系统电压值 (3000-3700mV)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetMinSystemVoltageLimit(uint16_t voltage_mv);

  // /**
  //  * @brief 设置快速充电电流限制
  //  * @param current_ma 充电电流值 (0-5056mA)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetFastChargeCurrentLimit(uint16_t current_ma);

  // /**
  //  * @brief 设置预充电电流限制
  //  * @param current_ma 预充电电流值 (64-1024mA)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetPrechargeCurrentLimit(uint16_t current_ma);

  // /**
  //  * @brief 设置终止充电电流限制
  //  * @param current_ma 终止充电电流值 (64-1024mA)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetTerminationCurrentLimit(uint16_t current_ma);

  // /**
  //  * @brief 设置充电电压限制
  //  * @param voltage_mv 充电电压值 (3840-4608mV)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetChargeVoltageLimit(uint16_t voltage_mv);

  // /**
  //  * @brief 设置电池低压阈值
  //  * @param high_threshold true:3.0V, false:2.8V
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBatteryLowVoltageThreshold(bool high_threshold);

  // /**
  //  * @brief 设置电池再充电阈值
  //  * @param high_threshold true:200mV, false:100mV
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBatteryRechargeThreshold(bool high_threshold);

  // /**
  //  * @brief 设置充电终止使能
  //  * @param enable true:使能充电终止, false:禁用充电终止
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetChargeTerminationEnable(bool enable);

  // /**
  //  * @brief 设置STAT引脚使能
  //  * @param enable true:禁用STAT引脚, false:使能STAT引脚
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetStatPinDisable(bool enable);

  // /**
  //  * @brief 设置看门狗定时器
  //  * @param timer_s 定时器时间 (0,40,80,160秒)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetWatchdogTimer(uint16_t timer_s);

  // /**
  //  * @brief 设置充电安全定时器使能
  //  * @param enable true:使能安全定时器, false:禁用安全定时器
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetChargeSafetyTimerEnable(bool enable);

  // /**
  //  * @brief 设置快速充电定时器
  //  * @param timer_hr 定时器时间 (5,8,12,20小时)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetFastChargeTimer(uint8_t timer_hr);

  // /**
  //  * @brief 设置JEITA低温电流设置
  //  * @param low_current true:20%, false:50%
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetJeitaLowTempCurrent(bool low_current);

  // /**
  //  * @brief 设置电池补偿电阻
  //  * @param resistance_mohm 补偿电阻值 (0-140mΩ)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBatteryCompensationResistance(uint8_t resistance_mohm);

  // /**
  //  * @brief 设置IR补偿电压钳位
  //  * @param voltage_mv 钳位电压值 (0-224mV)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetIrCompensationVoltageClamp(uint8_t voltage_mv);

  // /**
  //  * @brief 设置热调节阈值
  //  * @param temperature 温度值 (60,80,100,120°C)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetThermalRegulationThreshold(uint8_t temperature);

  // /**
  //  * @brief 强制自适应输入电流限制
  //  * @param force true:强制AICL, false:不强制
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetForceAdaptiveCurrentLimit(bool force);

  // /**
  //  * @brief 设置安全定时器减速
  //  * @param enable true:2倍减速, false:正常速度
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetSafetyTimerSlowdown(bool enable);

  // /**
  //  * @brief 设置JEITA高温电压设置
  //  * @param normal_voltage true:正常电压, false:电压降低150mV
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetJeitaHighTempVoltage(bool normal_voltage);

  // /**
  //  * @brief 设置BATFET关闭延迟
  //  * @param delay true:延迟关闭, false:立即关闭
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBatfetTurnoffDelay(bool delay);

  // /**
  //  * @brief 设置BATFET重置使能
  //  * @param enable true:使能BATFET重置, false:禁用BATFET重置
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBatfetResetEnable(bool enable);

  // /**
  //  * @brief 设置泵升压控制
  //  * @param up true:请求更高电压, false:禁用
  //  * @param down true:请求更低电压, false:禁用
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetPumpControl(bool up, bool down);

  // /**
  //  * @brief 设置升压模式电压
  //  * @param voltage_mv 升压电压值 (4550-5510mV)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBoostVoltage(uint16_t voltage_mv);

  // /**
  //  * @brief 设置升压模式电流限制
  //  * @param current_ma 升压电流限制值
  //  (500,750,1200,1400,1650,1875,2150,2450mA)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetBoostCurrentLimit(uint16_t current_ma);

  // /**
  //  * @brief 设置输入电压限制模式
  //  * @param absolute true:绝对VINDPM, false:相对VINDPM
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetVindpmMode(bool absolute);

  // /**
  //  * @brief 设置绝对输入电压限制
  //  * @param voltage_mv 输入电压限制值 (3900-15300mV)
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetAbsoluteVindpmThreshold(uint16_t voltage_mv);

  // /**
  //  * @brief 读取总线状态
  //  * @return 总线状态枚举值
  //  * @Date 2025-11-17 16:48:12
  //  */
  // BusStatus ReadBusStatus();

  // /**
  //  * @brief 读取充电状态
  //  * @return 充电状态枚举值
  //  * @Date 2025-11-17 16:48:12
  //  */
  // ChargeStatus ReadChargeStatus();

  // /**
  //  * @brief 读取电源良好状态
  //  * @return true:电源良好, false:电源不良
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadPowerGoodStatus();

  // /**
  //  * @brief 读取USB状态
  //  * @return true:USB500输入, false:USB100输入
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadUsbStatus();

  // /**
  //  * @brief 读取系统电压调节状态
  //  * @return true:在VSYSMIN调节中, false:不在VSYSMIN调节中
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadSystemVoltageRegulationStatus();

  // /**
  //  * @brief 读取热调节状态
  //  * @return true:在热调节中, false:正常
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadThermalRegulationStatus();

  // /**
  //  * @brief 读取NTC电压百分比
  //  * @return NTC电压百分比值 (需要除以1000)
  //  * @Date 2025-11-17 16:48:12
  //  */
  // uint16_t ReadNtcVoltagePercentage();

  // /**
  //  * @brief 读取总线连接状态
  //  * @return true:总线已连接, false:总线未连接
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadBusConnectionStatus();

  // /**
  //  * @brief 读取VINDPM状态
  //  * @return true:在VINDPM中, false:不在VINDPM中
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadVindpmStatus();

  // /**
  //  * @brief 读取IINDPM状态
  //  * @return true:在IINDPM中, false:不在IINDPM中
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadIindpmStatus();

  // /**
  //  * @brief 读取输入电流限制设置
  //  * @return 输入电流限制值 (mA)
  //  * @Date 2025-11-17 16:48:12
  //  */
  // uint16_t ReadInputCurrentLimitSetting();

  // /**
  //  * @brief 重置所有寄存器
  //  * @param reset true:重置寄存器, false:保持当前设置
  //  * @return
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool SetRegisterReset(bool reset);

  // /**
  //  * @brief 读取AICL优化状态
  //  * @return true:最大输入电流已检测, false:检测进行中
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadAiclOptimizedStatus();

  // /**
  //  * @brief 读取设备配置
  //  * @return 设备配置值
  //  * @Date 2025-11-17 16:48:12
  //  */
  // uint8_t ReadDeviceConfiguration();

  // /**
  //  * @brief 读取温度配置文件
  //  * @return true:JEITA配置文件, false:冷/热窗口配置文件
  //  * @Date 2025-11-17 16:48:12
  //  */
  // bool ReadTemperatureProfile();

  // /**
  //  * @brief 读取设备版本
  //  * @return 设备版本号
  //  * @Date 2025-11-17 16:48:12
  //  */
  // uint8_t ReadDeviceRevision();

 private:
  enum class Cmd {
    kRoDeviceId = 0x14,

    kRwInputSourceControl = 0x00,      // 输入源控制寄存器
    kRwTemperatureMonitorControl,      // 温度监控控制寄存器
    kRwSystemControl,                  // 系统控制寄存器
    kRwPowerOnConfiguration,           // 上电配置寄存器
    kRwChargeCurrentControl,           // 充电电流控制寄存器
    kRwPrechrgTermCurrentControl,      // 预充电/终止电流控制寄存器
    kRwChargeVoltageControl,           // 充电电压控制寄存器
    kRwChargeTerminationTimerControl,  // 充电终止/定时器控制寄存器
    kRwIrCompensationControl,          // IR补偿控制寄存器
    kRwMiscellaneousOperationControl,  // 杂项操作控制寄存器
    kRwBoostModeControl,               // 升压模式控制寄存器
    kRdSystemStatus,                   // 系统状态寄存器
    kRdFaultStatus,                    // 故障状态寄存器
    kRwVindpmControl,                  // 输入电压限制控制寄存器
    kRdBatteryVoltage,                 // 电池电压寄存器
    kRdSystemVoltage,                  // 系统电压寄存器
    kRdNtcVoltage,                     // NTC电压寄存器
    kRdBusVoltageStatus,               // 总线电压状态寄存器
    kRdChargeCurrent,                  // 充电电流寄存器
    kRdInputCurrentLimitStatus,        // 输入电流限制状态寄存器
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x6A;
  static constexpr uint8_t kDeviceId = 0x01;
  static constexpr uint8_t kInitSequence[] = {

      // 关闭 ILIM引脚，输入电流限制修改为最大
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwInputSourceControl), 0B00111111,

      // 禁用看门狗定时喂狗功能
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwChargeTerminationTimerControl), 0B10001101,

      // 快速充电电流限制设置为512ma
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwChargeCurrentControl), 0B00001000};

  int32_t rst_;
};
}  // namespace cpp_bus_driver
