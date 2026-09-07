/*
 * @Description: SGM41562 系列电池充电管理芯片统一驱动接口
 * @Author:
 * LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-09-03 18:00:00
 * @License: GPL 3.0
 */
#pragma once

#include <cstdint>
#include <initializer_list>
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

  // 型号支持的可选配置功能，不表示该功能当前已开启。
  enum class Feature : uint8_t {
    kInputCurrentLimitRelease,   // 解除输入电流限制，基础型号支持。
    kInputCurrentLimitOffset,    // 输入限流增加200mA，基础型号支持。
    kPrechargeCurrent,           // 设置预充电电流，S/SA型号支持。
    kInputOvervoltageThreshold,  // 选择输入过压阈值，S/SA型号支持。
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
   */
  explicit Sgm41562xx(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault)
      : I2cChipBase(bus, address) {}

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
   * @brief 查询当前型号是否支持指定的可选配置功能，不访问硬件
   * @param feature 要查询的功能，不表示对应使能位的状态
   * @return 已初始化且型号支持时返回true，未就绪或功能无效时返回false
   */
  bool HasFeature(Feature feature) const;

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

  // 线性编码寄存器字段及对应物理量范围。
  struct RegisterField {
    Register register_id;
    uint8_t mask;
    uint8_t shift;
    uint16_t minimum;
    uint16_t maximum;
    uint16_t step;
  };

  class ModelDriver {
   public:
    /**
     * @brief 保存当前型号组的可选功能，不访问硬件
     * @param features 支持的功能列表，构造时转换为位掩码保存
     */
    explicit ModelDriver(std::initializer_list<Feature> features);

    /**
     * @brief 销毁内部型号实现
     */
    virtual ~ModelDriver() = default;

    // 型号组支持的可选功能，在静态实现整个生命周期内保持不变。
    const uint32_t feature_mask_;

    /**
     * @brief 按型号执行寄存器初始化序列
     * @param chip 需要操作的芯片实例
     * @return 初始化成功返回true，通信失败返回false
     */
    virtual bool Init(Sgm41562xx& chip) const = 0;

    /**
     * @brief 按当前型号的REG04字段设置充电电压
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 充电电压，单位mV，范围和步进由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetChargeVoltageLimit(
        Sgm41562xx& chip, uint16_t voltage_mv) const = 0;

    /**
     * @brief 按当前型号的REG07字段设置系统调节电压
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 系统电压，单位mV，范围和步进由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetSystemRegulationVoltage(
        Sgm41562xx& chip, uint16_t voltage_mv) const = 0;

    /**
     * @brief 读取电流比例并按当前型号的REG02字段设置快速充电电流
     * @param chip 已初始化并确认型号的芯片实例
     * @param current_ma 充电电流，单位mA，范围和步进随型号及四分之一比例变化
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetFastChargeCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const = 0;

    /**
     * @brief 按当前型号的REG07字段设置热调节阈值
     * @param chip 已初始化并确认型号的芯片实例
     * @param temperature_c 温度，支持60、80、100和120摄氏度
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetThermalRegulationThreshold(
        Sgm41562xx& chip, uint8_t temperature_c) const = 0;

    /**
     * @brief 按当前型号的时间档位设置REG05看门狗
     * @param chip 已初始化并确认型号的芯片实例
     * @param timeout_s 超时时间，单位s，0表示关闭，其余档位由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetWatchdogTimer(
        Sgm41562xx& chip, uint16_t timeout_s) const = 0;

    /**
     * @brief 按当前型号的寄存器设置预充电切换阈值
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 阈值，调用前已检查为2800或3000mV
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetPrechargeToFastChargeThreshold(
        Sgm41562xx& chip, uint16_t voltage_mv) const = 0;

    /**
     * @brief 通过当前型号的复位位喂看门狗
     * @param chip 已初始化并确认型号的芯片实例
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool ResetWatchdogTimer(Sgm41562xx& chip) const = 0;

    /**
     * @brief 按当前型号的REG07字段设置输入电压环路
     * @param chip 已初始化并确认型号的芯片实例
     * @param enable 是否开启输入电压环路
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetInputVoltageLoopEnable(
        Sgm41562xx& chip, bool enable) const = 0;

    /**
     * @brief 按当前型号的寄存器设置PCB过温保护
     * @param chip 已初始化并确认型号的芯片实例
     * @param enable 是否开启PCB过温保护
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetPcbOvertemperatureProtectionEnable(
        Sgm41562xx& chip, bool enable) const = 0;

    /**
     * @brief 设置输入电流并处理型号特有的附加控制位
     * @param chip 需要操作的芯片实例
     * @param current_ma 输入电流限制，单位mA
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    virtual bool SetInputCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const = 0;

    /**
     * @brief 完成终止电流设置后清除该型号的六倍比例
     * @param chip 需要操作的芯片实例
     * @return 处理成功返回true，通信失败返回false
     */
    virtual bool FinishTerminationCurrentLimit(Sgm41562xx& chip) const;

    /**
     * @brief 设置支持该功能型号的预充电电流
     * @param chip 需要操作的芯片实例
     * @param current_ma 预充电电流，单位mA
     * @return 设置成功返回true，不支持、参数无效或通信失败返回false
     */
    virtual bool SetPrechargeCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const;

    /**
     * @brief 设置支持该功能型号的输入电流限制解除控制位
     * @param chip 需要操作的芯片实例
     * @param enable 是否解除输入电流限制
     * @return 设置成功返回true，不支持或通信失败返回false
     */
    virtual bool SetInputCurrentLimitReleaseEnable(
        Sgm41562xx& chip, bool enable) const;

    /**
     * @brief 设置支持该功能型号的输入电流200mA偏移
     * @param chip 需要操作的芯片实例
     * @param enable 是否开启偏移
     * @return 设置成功返回true，不支持或通信失败返回false
     */
    virtual bool SetInputCurrentLimitOffsetEnable(
        Sgm41562xx& chip, bool enable) const;

    /**
     * @brief 设置支持该功能型号的输入过压阈值
     * @param chip 需要操作的芯片实例
     * @param voltage_mv 输入过压阈值，单位mV
     * @return 设置成功返回true，不支持、参数无效或通信失败返回false
     */
    virtual bool SetInputOvervoltageThreshold(
        Sgm41562xx& chip, uint16_t voltage_mv) const;

    /**
     * @brief 解析输入配置并读取型号特有的扩展寄存器
     * @param chip 需要操作的芯片实例
     * @param input_source_control REG00寄存器值
     * @param system_status REG08寄存器值
     * @param config 保存解析后的配置
     * @return 读取成功返回true，通信失败返回false
     */
    virtual bool ReadInputConfig(Sgm41562xx& chip, uint8_t input_source_control,
        uint8_t system_status, ChargerConfig& config) const = 0;

    /**
     * @brief 解析充电配置并读取型号特有的扩展寄存器
     * @param chip 需要操作的芯片实例
     * @param charge_current_control REG02寄存器值
     * @param charge_voltage_control REG04寄存器值
     * @param config 保存解析后的配置
     * @return 读取成功返回true，通信失败返回false
     */
    virtual bool ReadChargeConfig(Sgm41562xx& chip,
        uint8_t charge_current_control, uint8_t charge_voltage_control,
        ChargerConfig& config) const = 0;

    /**
     * @brief 解析当前型号的系统电压、热调节、输入环路和看门狗配置
     * @param charge_timer_control REG05寄存器值
     * @param system_voltage_regulation REG07寄存器值
     * @param config 保存解析后的配置
     */
    virtual void ParseProtectionConfig(uint8_t charge_timer_control,
        uint8_t system_voltage_regulation, ChargerConfig& config) const = 0;
  };

  /**
   * @brief 将功能列表转换为位掩码
   * @param features 当前型号组支持的有效功能列表
   * @return 返回对应的功能位掩码
   */
  static uint32_t MakeFeatureMask(std::initializer_list<Feature> features);

  /**
   * @brief 根据已识别的芯片型号选择内部实现，不访问硬件
   * @param chip_type 已识别的具体芯片型号
   * @return 返回静态只读实现指针，未知或无效型号返回nullptr
   */
  static const ModelDriver* GetModelDriver(ChipType chip_type);

  // SGM41562、A和B的基础布局实现，定义位于sgm41562.cpp。
  class Sgm41562Driver final : public ModelDriver {
   public:
    /**
     * @brief 初始化SGM41562、A和B共用的可选功能，不访问硬件
     */
    Sgm41562Driver();

    /**
     * @brief 执行当前型号的寄存器初始化序列
     * @param chip 需要初始化的芯片实例
     * @return 初始化成功返回true，通信失败返回false
     */
    bool Init(Sgm41562xx& chip) const override;

    /**
     * @brief 按当前型号的REG04字段设置充电电压
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 充电电压，单位mV，范围和步进由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetChargeVoltageLimit(
        Sgm41562xx& chip, uint16_t voltage_mv) const override;

    /**
     * @brief 按当前型号的REG07字段设置系统调节电压
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 系统电压，单位mV，范围和步进由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetSystemRegulationVoltage(
        Sgm41562xx& chip, uint16_t voltage_mv) const override;

    /**
     * @brief 读取电流比例并按当前型号的REG02字段设置快速充电电流
     * @param chip 已初始化并确认型号的芯片实例
     * @param current_ma 充电电流，单位mA，范围和步进随型号及四分之一比例变化
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetFastChargeCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const override;

    /**
     * @brief 按当前型号的REG07字段设置热调节阈值
     * @param chip 已初始化并确认型号的芯片实例
     * @param temperature_c 温度，支持60、80、100和120摄氏度
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetThermalRegulationThreshold(
        Sgm41562xx& chip, uint8_t temperature_c) const override;

    /**
     * @brief 按当前型号的时间档位设置REG05看门狗
     * @param chip 已初始化并确认型号的芯片实例
     * @param timeout_s 超时时间，单位s，0表示关闭，其余档位由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetWatchdogTimer(Sgm41562xx& chip, uint16_t timeout_s) const override;

    /**
     * @brief 按当前型号的寄存器设置预充电切换阈值
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 阈值，调用前已检查为2800或3000mV
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetPrechargeToFastChargeThreshold(
        Sgm41562xx& chip, uint16_t voltage_mv) const override;

    /**
     * @brief 通过当前型号的复位位喂看门狗
     * @param chip 已初始化并确认型号的芯片实例
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool ResetWatchdogTimer(Sgm41562xx& chip) const override;

    /**
     * @brief 按当前型号的REG07字段设置输入电压环路
     * @param chip 已初始化并确认型号的芯片实例
     * @param enable 是否开启输入电压环路
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetInputVoltageLoopEnable(
        Sgm41562xx& chip, bool enable) const override;

    /**
     * @brief 按当前型号的寄存器设置PCB过温保护
     * @param chip 已初始化并确认型号的芯片实例
     * @param enable 是否开启PCB过温保护
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetPcbOvertemperatureProtectionEnable(
        Sgm41562xx& chip, bool enable) const override;

    /**
     * @brief 设置输入电流并清除限制解除位和200mA偏移位
     * @param chip 需要操作的芯片实例
     * @param current_ma 输入电流限制，范围50至500mA，步进30mA
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetInputCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const override;

    /**
     * @brief 设置输入电流限制解除控制位
     * @param chip 需要操作的芯片实例
     * @param enable 是否解除输入电流限制
     * @return 设置成功返回true，通信失败返回false
     */
    bool SetInputCurrentLimitReleaseEnable(
        Sgm41562xx& chip, bool enable) const override;

    /**
     * @brief 设置输入电流限制的200mA偏移
     * @param chip 需要操作的芯片实例
     * @param enable 是否开启偏移
     * @return 设置成功返回true，通信失败返回false
     */
    bool SetInputCurrentLimitOffsetEnable(
        Sgm41562xx& chip, bool enable) const override;

    /**
     * @brief 解析输入电流限制及型号固定的输入过压阈值
     * @param chip 当前芯片实例，用于区分A型号
     * @param input_source_control REG00寄存器值
     * @param system_status REG08寄存器值
     * @param config 保存解析后的配置
     * @return 解析完成返回true
     */
    bool ReadInputConfig(Sgm41562xx& chip, uint8_t input_source_control,
        uint8_t system_status, ChargerConfig& config) const override;

    /**
     * @brief 解析基础型号的充电电流、电压和预充电切换阈值
     * @param chip 需要操作的芯片实例，本实现不访问硬件
     * @param charge_current_control REG02寄存器值
     * @param charge_voltage_control REG04寄存器值
     * @param config 保存解析后的配置
     * @return 解析完成返回true
     */
    bool ReadChargeConfig(Sgm41562xx& chip, uint8_t charge_current_control,
        uint8_t charge_voltage_control, ChargerConfig& config) const override;

    /**
     * @brief 解析基础型号的系统电压、热调节、输入环路、PCB保护和看门狗配置
     * @param charge_timer_control REG05寄存器值
     * @param system_voltage_regulation REG07寄存器值
     * @param config 保存解析后的配置
     */
    void ParseProtectionConfig(uint8_t charge_timer_control,
        uint8_t system_voltage_regulation,
        ChargerConfig& config) const override;
  };

  // SGM41562S和SA的扩展布局实现，定义位于sgm41562s.cpp。
  class Sgm41562sDriver final : public ModelDriver {
   public:
    /**
     * @brief 初始化SGM41562S和SA共用的可选功能，不访问硬件
     */
    Sgm41562sDriver();

    /**
     * @brief 执行当前型号的寄存器初始化序列
     * @param chip 需要初始化的芯片实例
     * @return 初始化成功返回true，通信失败返回false
     */
    bool Init(Sgm41562xx& chip) const override;

    /**
     * @brief 按当前型号的REG04字段设置充电电压
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 充电电压，单位mV，范围和步进由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetChargeVoltageLimit(
        Sgm41562xx& chip, uint16_t voltage_mv) const override;

    /**
     * @brief 按当前型号的REG07字段设置系统调节电压
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 系统电压，单位mV，范围和步进由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetSystemRegulationVoltage(
        Sgm41562xx& chip, uint16_t voltage_mv) const override;

    /**
     * @brief 读取电流比例并按当前型号的REG02字段设置快速充电电流
     * @param chip 已初始化并确认型号的芯片实例
     * @param current_ma 充电电流，单位mA，范围和步进随型号及四分之一比例变化
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetFastChargeCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const override;

    /**
     * @brief 按当前型号的REG07字段设置热调节阈值
     * @param chip 已初始化并确认型号的芯片实例
     * @param temperature_c 温度，支持60、80、100和120摄氏度
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetThermalRegulationThreshold(
        Sgm41562xx& chip, uint8_t temperature_c) const override;

    /**
     * @brief 按当前型号的时间档位设置REG05看门狗
     * @param chip 已初始化并确认型号的芯片实例
     * @param timeout_s 超时时间，单位s，0表示关闭，其余档位由型号决定
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetWatchdogTimer(Sgm41562xx& chip, uint16_t timeout_s) const override;

    /**
     * @brief 按当前型号的寄存器设置预充电切换阈值
     * @param chip 已初始化并确认型号的芯片实例
     * @param voltage_mv 阈值，调用前已检查为2800或3000mV
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetPrechargeToFastChargeThreshold(
        Sgm41562xx& chip, uint16_t voltage_mv) const override;

    /**
     * @brief 通过当前型号的复位位喂看门狗
     * @param chip 已初始化并确认型号的芯片实例
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool ResetWatchdogTimer(Sgm41562xx& chip) const override;

    /**
     * @brief 按当前型号的REG07字段设置输入电压环路
     * @param chip 已初始化并确认型号的芯片实例
     * @param enable 是否开启输入电压环路
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetInputVoltageLoopEnable(
        Sgm41562xx& chip, bool enable) const override;

    /**
     * @brief 按当前型号的寄存器设置PCB过温保护
     * @param chip 已初始化并确认型号的芯片实例
     * @param enable 是否开启PCB过温保护
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetPcbOvertemperatureProtectionEnable(
        Sgm41562xx& chip, bool enable) const override;

    /**
     * @brief 设置REG0C中的输入电流限制
     * @param chip 需要操作的芯片实例
     * @param current_ma 输入电流限制，范围50至980mA，步进30mA
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetInputCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const override;

    /**
     * @brief 终止电流写入成功后关闭REG0D中的六倍比例
     * @param chip 需要操作的芯片实例
     * @return 设置成功返回true，通信失败返回false
     */
    bool FinishTerminationCurrentLimit(Sgm41562xx& chip) const override;

    /**
     * @brief 设置预充电电流，然后关闭预充电六倍比例
     * @param chip 需要操作的芯片实例
     * @param current_ma 预充电电流，范围1至31mA，步进2mA
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetPrechargeCurrentLimit(
        Sgm41562xx& chip, uint16_t current_ma) const override;

    /**
     * @brief 设置REG08中的输入过压阈值
     * @param chip 需要操作的芯片实例
     * @param voltage_mv 输入过压阈值，支持6000mV和19000mV
     * @return 设置成功返回true，参数无效或通信失败返回false
     */
    bool SetInputOvervoltageThreshold(
        Sgm41562xx& chip, uint16_t voltage_mv) const override;

    /**
     * @brief 读取REG0C并解析扩展布局的输入与保护配置
     * @param chip 需要操作的芯片实例
     * @param input_source_control REG00寄存器值
     * @param system_status REG08寄存器值
     * @param config 保存解析后的配置
     * @return 读取成功返回true，通信失败返回false
     */
    bool ReadInputConfig(Sgm41562xx& chip, uint8_t input_source_control,
        uint8_t system_status, ChargerConfig& config) const override;

    /**
     * @brief 解析扩展型号的充电电流、电压，并读取REG0D中的电流配置
     * @param chip 需要操作的芯片实例
     * @param charge_current_control REG02寄存器值
     * @param charge_voltage_control REG04寄存器值
     * @param config 保存解析后的配置
     * @return 读取成功返回true，通信失败返回false
     */
    bool ReadChargeConfig(Sgm41562xx& chip, uint8_t charge_current_control,
        uint8_t charge_voltage_control, ChargerConfig& config) const override;

    /**
     * @brief 解析扩展型号的系统电压、热调节、输入环路和看门狗配置
     * @param charge_timer_control REG05寄存器值
     * @param system_voltage_regulation REG07寄存器值
     * @param config 保存解析后的配置
     */
    void ParseProtectionConfig(uint8_t charge_timer_control,
        uint8_t system_voltage_regulation,
        ChargerConfig& config) const override;
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
   * @brief 校验并写入线性编码寄存器字段
   * @param field 字段布局和物理量范围
   * @param value 待设置的物理量
   * @return 设置成功返回true，参数无效或通信失败返回false
   */
  bool SetRegisterField(const RegisterField& field, uint16_t value);

  /**
   * @brief 将寄存器字段解码为物理量并限制到有效上限
   * @param field 字段布局和物理量范围
   * @param register_value 原始寄存器值
   * @return 返回解码后的物理量
   */
  static uint16_t DecodeRegisterField(
      const RegisterField& field, uint8_t register_value);

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

  ChipType chip_type_ = ChipType::kUnknown;
  // Init完成型号识别后绑定静态实现，未初始化时为空。
  const ModelDriver* model_driver_ = nullptr;
};
}  // namespace cpp_bus_driver
