/*
 * @Description: SGM41562 系列电池充电管理芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-09-03 18:00:00
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {
class Sgm41562xx final : public I2cChipBase {
 public:
  enum class ChipType {
    kUnknown = 0,
    kSgm41562,
    kSgm41562A,
    kSgm41562B,
    kSgm41562S,
    kSgm41562Sa,
  };

  enum class ChargeStatus {
    kNotCharging = 0,
    kPrecharge,
    kCharging,
    kChargeComplete,
  };

  enum class ShippingModeDelay {
    k1Second = 0,
    k2Seconds,
    k4Seconds,
    k8Seconds,
  };

  enum class InterruptType : uint8_t {
    kInputPowerGood = 0x10,
    kChargeComplete = 0x08,
    kChargeStatus = 0x04,
    kNtc = 0x02,
    kBatteryOvervoltage = 0x01,
  };

  struct FaultStatus {
    bool input_power_fault = false;
    bool thermal_shutdown = false;
    bool battery_overvoltage_fault = false;
    bool safety_timer_expired = false;
    bool ntc_hot = false;
    bool ntc_cold = false;
  };

  struct ChipStatus {
    bool watchdog_expired = false;
    ChargeStatus charge_status = ChargeStatus::kNotCharging;
    bool power_path_management_active = false;
    bool input_power_good = false;
    bool thermal_regulation_active = false;
  };

  struct ChargerConfig {
    uint8_t i2c_address = 0;
    bool charge_enabled = false;
    bool high_impedance_enabled = false;
    uint8_t reset_pull_down_time_s = 0;
    uint8_t battery_fet_off_time_s = 0;
    uint16_t battery_undervoltage_threshold_mv = 0;
    uint16_t minimum_input_voltage_limit_mv = 0;
    uint16_t input_current_limit_ma = 0;
    bool input_current_limit_enabled = true;
    bool input_current_limit_200_ma_offset_enabled = false;
    uint16_t fast_charge_current_ma = 0;
    bool quarter_charge_current_scale_enabled = false;
    uint16_t precharge_current_ma = 0;
    uint16_t termination_current_ma = 0;
    uint16_t discharge_current_limit_ma = 0;
    uint16_t charge_voltage_limit_mv = 0;
    uint16_t precharge_to_fast_charge_threshold_mv = 0;
    uint16_t recharge_threshold_mv = 0;
    bool watchdog_in_discharge_enabled = false;
    bool watchdog_enabled = false;
    uint16_t watchdog_timeout_s = 0;
    bool charge_termination_enabled = false;
    bool safety_timer_enabled = false;
    uint8_t safety_timer_hours = 0;
    bool charge_after_termination_enabled = false;
    bool safety_timer_extended_in_ppm = false;
    bool ntc_enabled = false;
    bool shipping_mode_enabled = false;
    bool input_power_good_interrupt_enabled = false;
    bool charge_complete_interrupt_enabled = false;
    bool charge_status_interrupt_enabled = false;
    bool ntc_interrupt_enabled = false;
    bool battery_overvoltage_interrupt_enabled = false;
    uint8_t thermal_regulation_threshold_c = 0;
    uint16_t system_voltage_regulation_mv = 0;
    bool input_voltage_loop_enabled = false;
    bool pcb_overtemperature_protection_enabled = false;
    uint16_t input_overvoltage_threshold_mv = 0;
    uint8_t shipping_mode_delay_s = 0;
    bool force_power_path_switch_enabled = false;
    bool battery_power_enabled = false;
    bool input_overvoltage_protection_enabled = false;
    uint16_t exit_shipping_mode_interrupt_delay_ms = 0;
    uint16_t exit_shipping_mode_input_delay_ms = 0;
    uint16_t termination_deglitch_time_ms = 0;
    bool shipping_mode_interrupt_enabled = false;
    bool precharge_current_multiplier_six_enabled = false;
    bool termination_current_multiplier_six_enabled = false;
  };

  /**
   * @brief 创建SGM41562系列芯片对象
   * @param bus I2C总线对象
   * @param address I2C设备地址
   * @param rst 复位引脚，使用 kPinNotConnected 时不控制复位引脚
   */
  explicit Sgm41562xx(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kPinNotConnected)
      : I2cChipBase(bus, address), rst_(rst) {}

  /**
   * @brief 初始化芯片并根据型号执行对应寄存器初始化序列
   * @param freq_hz I2C总线频率，默认使用 kDefaultFrequencyHz
   * @return 初始化成功返回true，失败返回false
   */
  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;

  /**
   * @brief 反初始化芯片
   * @param delete_bus true：同时反初始化总线，false：保留总线
   * @return 反初始化成功返回true，失败返回false
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取REG0B中的芯片ID
   * @param chip_id 返回读取到的芯片ID
   * @return 读取成功返回true，失败返回false
   */
  bool GetChipId(uint8_t& chip_id);

  /**
   * @brief 获取Init函数确认的芯片型号
   * @return 返回芯片型号，尚未成功初始化时返回ChipType::kUnknown
   */
  ChipType GetChipType() const;

  /**
   * @brief 将芯片型号转换为字符串
   * @param chip_type 芯片型号
   * @return 返回芯片型号字符串
   */
  static const char* ChipTypeToString(ChipType chip_type);

  /**
   * @brief 读取并解析REG09中的故障状态
   * @param status 返回解析后的故障状态
   * @return 读取成功返回true，失败返回false
   */
  bool GetFaultStatus(FaultStatus& status);

  /**
   * @brief 设置充电使能
   * @param enable true：开启充电，false：关闭充电
   * @return 设置成功返回true，失败返回false
   */
  bool SetChargeEnable(bool enable);

  /**
   * @brief 设置输入高阻模式
   * @param enable true：关闭Qbypass，false：开启Qbypass
   * @return 设置成功返回true，失败返回false
   */
  bool SetHighImpedanceModeEnable(bool enable);

  /**
   * @brief 设置最低输入电压限制
   * @param voltage_mv 范围3880mV至5080mV，步进80mV
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetMinimumInputVoltageLimit(uint16_t voltage_mv);

  /**
   * @brief 设置输入电流限制
   * @param current_ma A/B系列上限500mA，S/SA系列上限980mA，步进30mA
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetInputCurrentLimit(uint16_t current_ma);

  /**
   * @brief 设置电池欠压锁定阈值
   * @param voltage_mv 范围2400mV至3030mV，步进90mV
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetBatteryUndervoltageThreshold(uint16_t voltage_mv);

  /**
   * @brief 设置快速充电电流限制
   * @param current_ma 电流必须符合芯片型号与精细比例对应的范围和步进
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetFastChargeCurrentLimit(uint16_t current_ma);

  /**
   * @brief 设置BAT到SYS放电电流限制
   * @param current_ma 范围400mA至3200mA，步进200mA
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetDischargeCurrentLimit(uint16_t current_ma);

  /**
   * @brief 设置充电终止电流并关闭六倍比例
   * @param current_ma 范围1mA至31mA，步进2mA
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetTerminationCurrentLimit(uint16_t current_ma);

  /**
   * @brief 设置S/SA型号预充电电流并关闭六倍比例
   * @param current_ma 范围1mA至31mA，步进2mA
   * @return 设置成功返回true，型号或参数无效及通信失败返回false
   */
  bool SetPrechargeCurrentLimit(uint16_t current_ma);

  /**
   * @brief 设置充电目标电压限制
   * @param voltage_mv A/B系列步进15mV，S/SA系列步进10mV
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetChargeVoltageLimit(uint16_t voltage_mv);

  /**
   * @brief 设置系统调节电压
   * @param voltage_mv A/B与S/SA系列均为50mV步进
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetSystemRegulationVoltage(uint16_t voltage_mv);

  /**
   * @brief 设置预充电转快速充电的电压阈值
   * @param voltage_mv 可设置为2800mV或3000mV
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetPrechargeToFastChargeThreshold(uint16_t voltage_mv);

  /**
   * @brief 设置充电完成后的再充电压差阈值
   * @param voltage_mv 可设置为100mV或200mV
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetRechargeThreshold(uint16_t voltage_mv);

  /**
   * @brief 设置看门狗超时时间
   * @param timeout_s 设置为0关闭，其他有效值随芯片型号为40倍或64倍
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetWatchdogTimer(uint16_t timeout_s);

  /**
   * @brief 复位I2C看门狗计数器
   * @return 复位成功返回true，失败返回false
   */
  bool ResetWatchdogTimer();

  /**
   * @brief 设置放电模式下的看门狗功能
   * @param enable true：启用，false：禁用
   * @return 设置成功返回true，失败返回false
   */
  bool SetWatchdogInDischargeEnable(bool enable);

  /**
   * @brief 设置充电终止功能
   * @param enable true：启用，false：禁用
   * @return 设置成功返回true，失败返回false
   */
  bool SetChargeTerminationEnable(bool enable);

  /**
   * @brief 设置充电安全定时器
   * @param enable true：启用，false：禁用
   * @return 设置成功返回true，失败返回false
   */
  bool SetSafetyTimerEnable(bool enable);

  /**
   * @brief 设置充电安全定时器时长
   * @param duration_hours 可设置为3、5、8或12小时
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetSafetyTimerDuration(uint8_t duration_hours);

  /**
   * @brief 设置充电终止后继续保持充电电流的定时功能
   * @param enable true：终止后继续，false：终止后暂停
   * @return 设置成功返回true，失败返回false
   */
  bool SetChargeAfterTerminationEnable(bool enable);

  /**
   * @brief 设置NTC温度检测功能
   * @param enable true：启用，false：禁用
   * @return 设置成功返回true，失败返回false
   */
  bool SetNtcEnable(bool enable);

  /**
   * @brief 设置PPM模式下安全定时器两倍延长功能
   * @param enable true：启用两倍延长，false：禁用
   * @return 设置成功返回true，失败返回false
   */
  bool SetPpmSafetyTimerExtensionEnable(bool enable);

  /**
   * @brief 设置指定中断源使能
   * @param interrupt_type 中断源
   * @param enable true：启用中断，false：屏蔽中断
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetInterruptEnable(InterruptType interrupt_type, bool enable);

  /**
   * @brief 设置输入电压环路功能
   * @param enable true：启用，false：禁用
   * @return 设置成功返回true，失败返回false
   */
  bool SetInputVoltageLoopEnable(bool enable);

  /**
   * @brief 设置PCB过温保护功能
   * @param enable true：启用，false：禁用
   * @return 设置成功返回true，失败返回false
   */
  bool SetPcbOvertemperatureProtectionEnable(bool enable);

  /**
   * @brief 设置热调节温度阈值
   * @param temperature_c 可设置为60、80、100或120摄氏度
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetThermalRegulationThreshold(uint8_t temperature_c);

  /**
   * @brief 设置A/B系列输入限流释放功能
   * @param enable true：释放限流，false：使用输入限流设置
   * @return 设置成功返回true，型号无效或通信失败返回false
   */
  bool SetInputCurrentLimitReleaseEnable(bool enable);

  /**
   * @brief 设置A/B系列输入限流额外增加200mA功能
   * @param enable true：额外增加200mA，false：不增加
   * @return 设置成功返回true，型号无效或通信失败返回false
   */
  bool SetInputCurrentLimitOffsetEnable(bool enable);

  /**
   * @brief 设置S/SA型号输入过压阈值
   * @param voltage_mv 可设置为6000mV或19000mV
   * @return 设置成功返回true，型号或参数无效及通信失败返回false
   */
  bool SetInputOvervoltageThreshold(uint16_t voltage_mv);

  /**
   * @brief 设置放电模式下强制开启Qswitch
   * @param enable true：强制开启，false：使用正常电源路径
   * @return 设置成功返回true，失败返回false
   */
  bool SetForcePowerPathSwitchEnable(bool enable);

  /**
   * @brief 设置移除VIN后的电池供电功能
   * @param enable true：允许电池供电，false：关闭电池供电
   * @return 设置成功返回true，失败返回false
   */
  bool SetBatteryPowerEnable(bool enable);

  /**
   * @brief 设置输入过压锁定检测功能
   * @param enable true：启用检测，false：禁用检测
   * @return 设置成功返回true，失败返回false
   */
  bool SetInputOvervoltageProtectionEnable(bool enable);

  /**
   * @brief 设置充电电流四分之一精细比例
   * @param enable true：使用四分之一比例，false：使用正常比例
   * @return 设置成功返回true，失败返回false
   */
  bool SetQuarterChargeCurrentScaleEnable(bool enable);

  /**
   * @brief 读取并解析REG08中的芯片状态
   * @param status 返回解析后的芯片状态
   * @return 读取成功返回true，失败返回false
   */
  bool GetChipStatus(ChipStatus& status);

  /**
   * @brief 读取官方寄存器定义的全部常用充电与保护配置
   * @param config 返回解析后的完整常用配置
   * @return 读取成功返回true，失败返回false
   */
  bool GetChargerConfig(ChargerConfig& config);

  /**
   * @brief 设置运输模式使能
   * @param enable true：进入运输模式，false：取消进入运输模式
   * @return 设置成功返回true，失败返回false
   */
  bool SetShippingModeEnable(bool enable);

  /**
   * @brief 设置进入运输模式前的延迟时间
   * @param delay 运输模式延迟时间
   * @return 设置成功返回true，失败返回false
   */
  bool SetShippingModeDelay(ShippingModeDelay delay);

 private:
  // 默认 I2C 总线时钟，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  enum class Register {
    kInputSourceControl = 0x00,
    kPowerOnConfiguration = 0x01,
    kChargeCurrentControl = 0x02,
    kDischargeTerminationCurrent = 0x03,
    kChargeVoltageControl = 0x04,
    kChargeTerminationTimerControl = 0x05,
    kMiscellaneousOperationControl = 0x06,
    kSystemVoltageRegulation = 0x07,
    kSystemStatus = 0x08,
    kFaultAndShippingControl = 0x09,
    kI2cAddressMiscellaneousConfiguration = 0x0A,
    kChipId = 0x0B,
    kExtendedInputCurrentControl = 0x0C,
    kExtendedCurrentControl = 0x0D,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x03;
  static constexpr uint8_t kChipIdSgm41562BAndSa = 0x00;
  static constexpr uint8_t kChipIdSgm41562A = 0x02;
  static constexpr uint8_t kChipIdSgm41562 = 0x04;
  static constexpr uint8_t kChipIdSgm41562S = 0x09;

  // SGM41562、SGM41562A和SGM41562B寄存器初始化序列
  static constexpr uint8_t kInitSequenceAb[] = {
      // 禁用看门狗
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kChargeTerminationTimerControl),
      0x1A,

      // 解除输入电流限制
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kSystemStatus),
      0x40,

      // 完成其他配置后开启充电
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kPowerOnConfiguration),
      0xA4,
  };

  // SGM41562S和SGM41562SA寄存器初始化序列
  static constexpr uint8_t kInitSequenceS[] = {
      // 禁用看门狗
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kChargeTerminationTimerControl),
      0x1A,

      // 将输入电流限制设置为芯片可配置的最高值980mA
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kExtendedInputCurrentControl),
      0xFA,

      // 完成其他配置后开启充电
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kPowerOnConfiguration),
      0xA4,
  };

  /**
   * @brief 根据芯片ID识别芯片型号
   * @param chip_id REG0B中的原始芯片ID
   * @return 返回识别出的芯片型号，无法识别时返回ChipType::kUnknown
   */
  ChipType DetectChipType(uint8_t chip_id);

  /**
   * @brief 区分芯片ID同为0x00的SGM41562B和SGM41562SA
   * @return 返回识别出的芯片型号，无法可靠区分时返回ChipType::kUnknown
   */
  ChipType DetectIdZeroChipType();

  /**
   * @brief 使用REG02中的REG_RST位复位寄存器
   * @return 复位成功返回true，失败返回false
   */
  bool ResetRegisters();

  /**
   * @brief 通过读改写方式更新寄存器中的指定位
   * @param register_id 需要更新的寄存器
   * @param mask 需要更新的位掩码
   * @param value 写入掩码范围内的目标值
   * @return 更新成功返回true，失败返回false
   */
  bool UpdateRegisterBits(Register register_id, uint8_t mask, uint8_t value);

  /**
   * @brief 读取指定寄存器
   * @param register_id 需要读取的寄存器
   * @param value 返回读取到的寄存器值
   * @param name 寄存器名称，用于输出错误日志
   * @return 读取成功返回true，失败返回false
   */
  bool ReadRegister(Register register_id, uint8_t& value, const char* name);

  /**
   * @brief 读取并解析输入与电源路径关键配置
   * @param config 保存解析后的关键配置
   * @return 读取成功返回true，失败返回false
   */
  bool ReadInputConfig(ChargerConfig& config);

  /**
   * @brief 读取并解析充电电流与电压关键配置
   * @param config 保存解析后的关键配置
   * @return 读取成功返回true，失败返回false
   */
  bool ReadChargeConfig(ChargerConfig& config);

  /**
   * @brief 读取并解析定时器与温度保护关键配置
   * @param config 保存解析后的关键配置
   * @return 读取成功返回true，失败返回false
   */
  bool ReadProtectionConfig(ChargerConfig& config);

  /**
   * @brief 检查当前型号是否使用S或SA扩展寄存器布局
   * @return 使用扩展寄存器布局返回true，否则返回false
   */
  bool HasExtendedRegisterMap() const;

  /**
   * @brief 检查芯片是否已成功初始化并完成型号识别
   * @return 已初始化返回true，否则返回false
   */
  bool IsInitialized();

  /**
   * @brief 解析REG09故障状态寄存器值
   * @param fault_status REG09寄存器值
   * @param status 返回解析后的故障状态
   */
  static void ParseFaultStatus(uint8_t fault_status, FaultStatus& status);

  /**
   * @brief 解析REG08芯片状态寄存器值
   * @param chip_status REG08寄存器值
   * @param status 返回解析后的芯片状态
   */
  static void ParseChipStatus(uint8_t chip_status, ChipStatus& status);

  int32_t rst_;
  ChipType chip_type_ = ChipType::kUnknown;
};
}  // namespace cpp_bus_driver
