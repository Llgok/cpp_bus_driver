/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-02-03 15:06:34
 * @LastEditTime: 2026-04-30 11:04:20
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Axp517 final : public ChipI2cGuide {
 public:
  enum class ChargeStatus {
    kTrickleCharge,
    kPrecharge,
    kConstantCurrent,
    kConstantVoltage,
    kChargeDone,
    kNotCharging,
    kInvalid,
  };

  enum class BatteryCurrentDirection {
    kStandby,
    kCharge,
    kDischarge,
    kInvalid,
  };

  enum class NtcFaultStatus {
    kNormal = 0,
    kTsColdCharge = 1,
    kTsHotCharge = 2,
    kTsColdWork = 5,
    kTsHotWork = 6,
  };

  enum class BcDetectResult {
    kSdp = 1,  // 标准下行端口
    kCdp = 2,  // 充电下行端口
    kDcp = 3,  // 专用充电端口
  };

  // struct IrqStatus0
  // {
  //     bool vbus_fault_flag = false;
  //     bool vbus_over_voltage_flag = false;
  //     bool boost_over_voltage_flag = false;
  //     bool charge_to_normal_flag = false;
  //     bool gauge_new_soc_flag = false;
  //     bool soc_drop_to_shutdown_level_flag = false;
  //     bool soc_drop_to_warning_level_flag = false;
  // };

  // struct IrqStatus1
  // {
  //     bool pwr_on_positive_edge_flag = false;
  //     bool pwr_on_negative_edge_flag = false;
  //     bool pwr_on_long_press_flag = false;
  //     bool pwr_on_short_press_flag = false;
  //     bool battery_remove_flag = false;
  //     bool battery_insert_flag = false;
  //     bool vbus_remove_flag = false;
  //     bool vbus_insert_flag = false;
  // };

  // struct IrqStatus2
  // {
  //     bool battery_over_voltage_flag = false;
  //     bool charger_safety_timer_expire_flag = false;
  //     bool die_over_temperature_level1_flag = false;
  //     bool charger_start_flag = false;
  //     bool battery_charge_done_flag = false;
  //     bool batfet_over_current_flag = false;
  //     bool watchdog_expire_flag = false;
  // };

  // struct IrqStatus3
  // {
  //     bool battery_under_temperature_work_flag = false;
  //     bool battery_over_temperature_work_flag = false;
  //     bool battery_under_temperature_charge_flag = false;
  //     bool battery_over_temperature_charge_flag = false;
  //     bool battery_over_temperature_quit_flag = false;
  //     bool bc1_2_detect_result_change_flag = false;
  //     bool bc1_2_detect_finished_flag = false;
  // };

  enum class AdcData {
    kChipTemperatureCelsius = 0,
    kSystemVoltage,

    kChargingCurrent = 6,
    kDischargeCurrent,
  };

  enum class GpioSource {
    kByReg = 0,  // 通过寄存器控制
    kPdIrq,      // kPdIrq
  };

  enum class GpioMode {
    kInput,   // 输入模式
    kOutput,  // 输出模式
  };

  enum class GpioStatus {
    kHiz = 0,  // 高阻态
    kLow,      // 低电平
    kHigh,     // 高电平
    kInvalid,  // 无效
  };

  enum class ForceBatfet {
    kAuto,
    kOn,
    kOff,
  };

  struct ChipStatus0 {
    bool current_limit_status = false;
    bool thermal_regulation_status = false;
    bool battery_in_active_mode = false;
    bool battery_present_status = false;
    bool batfet_status = false;
    bool vbus_good_indication = false;
  };

  struct ChipStatus1 {
    ChargeStatus charging_status = ChargeStatus::kNotCharging;
    bool vindpm_status = false;
    bool system_status_indication = false;
    BatteryCurrentDirection battery_current_direction =
        BatteryCurrentDirection::kStandby;
  };

  struct AdcChannel {
    bool vbus_current_measure = false;
    bool battery_discharge_current_measure = false;
    bool battery_charge_current_measure = false;
    bool chip_temperature_measure = false;
    bool system_voltage_measure = false;
    bool vbus_voltage_measure = false;
    bool ts_value_measure = false;
    bool battery_voltage_measure = false;
  };

  explicit Axp517(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;
  bool Deinit(bool delete_bus = false) override;

  uint8_t GetDeviceId();

  /**
   * @brief 获取芯片状态0
   * @param &status 使用Chip_Status_0::配置
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool GetChipStatus0(ChipStatus0& status);

  /**
   * @brief 获取芯片状态1
   * @param &status 使用Chip_Status_1::配置
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool GetChipStatus1(ChipStatus1& status);

  // /**
  //  * @brief 获取所有中断状态
  //  * @param &status0 中断状态0
  //  * @param &status1 中断状态1
  //  * @param &status2 中断状态2
  //  * @param &status3 中断状态3
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool GetIrqStatus(IrqStatus0& status0, IrqStatus1& status1,
  //                   IrqStatus2& status2, IrqStatus3& status3);

  // /**
  //  * @brief 清除所有中断标志
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool ClearAllIrq(void);

  /**
   * @brief 设置充电使能
   * @param enable [true]：开启充电 [false]：关闭充电
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetChargeEnable(bool enable);

  /**
   * @brief 设置充电电流
   * @param current_ma 充电电流值(mA)，范围0-5120mA，64mA/步进
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetChargeCurrent(uint16_t current_ma);

  /**
   * @brief 设置充电电压
   * @param voltage_mv 充电电压值(mV)，支持4000, 4100, 4200, 4350, 4400, 3800,
   * 3600, 5000mV
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetChargeVoltage(uint16_t voltage_mv);

  /**
   * @brief 设置输入电流限制
   * @param limit_ma 输入电流限制值(mA)，范围100-3250mA，50mA/步进
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetInputCurrentLimit(uint16_t limit_ma);

  /**
   * @brief 设置输入电压限制
   * @param limit_mv 输入电压限制值(mV)，范围3600-16200mV，100mV/步进
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetInputVoltageLimit(uint16_t limit_mv);

  /**
   * @brief 获取电池电量百分比
   * @return 电池电量百分比(0-100)
   * @Date 2026-02-03 15:06:34
   */
  uint8_t GetBatteryLevel();

  /**
   * @brief 获取电池健康度
   * @return 电池健康度(0-100)int16_t
   * @Date 2026-02-03 15:06:34
   */
  uint8_t GetBatteryHealth();

  /**
   * @brief 获取电池温度，使用前需要开启对应ADC通道（REG 90H）
   * @return 电池温度(℃)
   * @Date 2026-02-03 15:06:34
   */
  int8_t GetBatteryTemperatureCelsius();

  /**
   * @brief 设置ADC通道
   * @param channel 使用Adc_Channel::配置
   * @return
   * @Date 2026-02-04 10:16:10
   */
  bool SetAdcChannel(AdcChannel channel);

  /**
   * @brief 获取电池电压，使用前需要开启对应ADC通道（REG 90H）
   * @return 电池电压(mV)
   * @Date 2026-02-03 15:06:34
   */
  uint16_t GetBatteryVoltage();

  /**
   * @brief 获取电池电流，使用前需要开启对应ADC通道（REG 90H）
   * @return 电池电流(mA)，正值为充电，负值为放电，读取失败返回-32768
   * @Date 2026-02-03 15:06:34
   */
  float GetBatteryCurrent();

  /**
   * @brief 获取TS引脚电压值，使用前需要开启对应ADC通道（REG 90H）
   * @return TS引脚电压值(mV)
   * @Date 2026-02-03 15:06:34
   */
  float GetTsVoltage();

  /**
   * @brief 获取VBUS电流，使用前需要开启对应ADC通道（REG 90H）
   * @return VBUS电压(mA)
   * @Date 2026-02-03 15:06:34
   */
  uint16_t GetVbusCurrent();

  /**
   * @brief 获取VBUS电压，使用前需要开启对应ADC通道（REG 90H）
   * @return VBUS电压(mV)
   * @Date 2026-02-03 15:06:34
   */
  uint16_t GetVbusVoltage();

  /**
   * @brief 设置adc数据输出选择
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetAdcDataSelect(AdcData data_select);

  /**
   * @brief 获取ADC数据值
   * @return
   * @Date 2026-02-03 15:06:34
   */
  uint16_t GetAdcData();

  /**
   * @brief 获取芯片结温温度，使用前需要开启ADC数据选择（REG
   * 9BH）中的Tdie和对应ADC通道（REG 90H）
   * @return 芯片结温温度(℃)
   * @Date 2026-02-03 15:06:34
   */
  float GetChipDieJunctionTemperatureCelsius();

  /**
   * @brief 获取系统电压，使用前需要开启ADC数据选择（REG
   * 9BH）中的Vsys和对应ADC通道（REG 90H）
   * @return 系统电压(mV)
   * @Date 2026-02-03 15:06:34
   */
  uint16_t GetSystemVoltage();

  /**
   * @brief 获取充电电流，使用前需要开启ADC数据选择（REG
   * 9BH）中的Ichg和对应ADC通道（REG 90H）
   * @return 充电电流(mV)
   * @Date 2026-02-03 15:06:34
   */
  float GetChargingCurrent();

  /**
   * @brief 获取放电电流，使用前需要开启ADC数据选择（REG
   * 9BH）中的Idischg和对应ADC通道（REG 90H）
   * @return 放电电流(mV)
   * @Date 2026-02-03 15:06:34
   */
  float GetDischargingCurrent();

  /**
   * @brief 设置Boost模式使能
   * @param enable [true]：开启Boost模式 [false]：关闭Boost模式
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetBoostEnable(bool enable);

  /**
   * @brief 设置GPIO输出源选择
   * @param source 使用Gpio_Source::配置
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetGpioSource(GpioSource source);

  /**
   * @brief 设置GPIO模式
   * @param mode 使用Gpio_Mode::配置
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetGpioMode(GpioMode mode);

  /**
   * @brief 写GPIO状态
   * @param config 使用Gpio_Status::配置
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool GpioWrite(GpioStatus status);

  /**
   * @brief 读取GPIO状态
   * @return 使用Gpio_Status::配置
   * @Date 2026-02-03 15:06:34
   */
  GpioStatus GpioRead();

  /**
   * @brief 设置开启运输模式
   * @param enable [true]：开启运输模式 [false]：关闭运输模式
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetShippingModeEnable(bool enable);

  /**
   * @brief 强制设置batfet（电池开关）启动或者关闭
   * @param mode 使用Force_Batfet::配置
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetForceBatfetMode(ForceBatfet mode);

  /**
   * @brief 强制设置rbfet（vbus反向供电开关）启动或者关闭
   * @param enable enable [true]：强制开启 [false]：强制关闭
   * @return
   * @Date 2026-02-03 15:06:34
   */
  bool SetForceRbfetEnable(bool enable);

  // /**
  //  * @brief 设置Boost输出电压
  //  * @param voltage_mv 输出电压(mV)，范围4550-5510mV，64mV/步进
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool SetBoostVoltage(uint16_t voltage_mv);

  // /**
  //  * @brief 设置看门狗
  //  * @param enable [true]：开启看门狗 [false]：关闭看门狗
  //  * @param timeout_s 超时时间(秒)，支持1,2,4,8,16,32,64,128秒
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool SetWatchdog(bool enable, uint8_t timeout_s);

  // /**
  //  * @brief 喂狗
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool FeedWatchdog();

  // /**
  //  * @brief 设置JEITA标准使能
  //  * @param enable [true]：开启JEITA标准 [false]：关闭JEITA标准
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool SetJeitaEnable(bool enable);

  // /**
  //  * @brief 设置BC1.2检测使能
  //  * @param enable [true]：开启BC1.2检测 [false]：关闭BC1.2检测
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool SetBc12DetectEnable(bool enable);

  // /**
  //  * @brief 获取BC1.2检测结果
  //  * @param &result BC检测结果
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool GetBc12DetectResult(BcDetectResult& result);

  // /**
  //  * @brief 设置PD角色
  //  * @param is_source [true]：源模式 [false]：汇模式
  //  * @param is_drp [true]：双角色模式 [false]：固定角色
  //  * @return
  //  * @Date 2026-02-03 15:06:34
  //  */
  // bool SetPdRole(bool is_source, bool is_drp);

 private:
  enum class Cmd {
    kRoDeviceId = 0xCE,

    // 状态寄存器
    kRoBmuStatus0 = 0x00,  // BMU状态0
    kRoBmuStatus1,         // BMU状态1

    kRoBcDetect = 0x05,  // BC检测结果
    kRoBmuFault0,        // BMU故障0

    kRoBmuFault1 = 0x08,  // BMU故障1

    // 模块使能控制
    kRwModuleEnableControl0 = 0x0B,  // 模块使能控制0

    // 通用配置
    kRwCommonConfigure = 0x10,  // 通用配置
    kRwGpioConfigure,           // GPIO配置
    kRwBatfetControl,           // BATFET控制
    kRwRbfetControl,            // RBFET控制

    // 充电相关
    kRwMinimumSystemVoltageControl = 0x15,  // 最小系统电压控制
    kRwInputVoltageLimitControl,            // 输入电压限制控制
    kRwInputCurrentLimitControl,            // 输入电流限制控制

    kRwModuleEnableControl1 = 0x19,  // 模块使能控制1
    kRwWatchdogControl,              // 看门狗控制

    kRwBoostConfigure = 0x1E,  // Boost配置

    // 温度传感器
    kRwTsPinConfigure = 0x50,  // TS引脚配置

    kRwVltfChgSetting = 0x54,  // 充电低温阈值设置
    kRwVhtfChgSetting,         // 充电高温阈值设置
    kRwVltfWorkSetting,        // 工作低温阈值设置
    kRwVhtfWorkSetting,        // 工作高温阈值设置

    // JEITA标准
    kRwJeitaStandardEnableControl = 0x58,  // JEITA标准使能控制
    kRwJeitaCurrentVoltageConfiguration,   // JEITA电流/电压配置

    // 充电控制
    kRwIprechgItrichgSetting = 0x61,       // 预充电/涓流充电设置
    kRwIccSetting,                         // 恒流充电设置
    kRwItermSettingAndControl,             // 终止电流设置和控制
    kRwCvChargerVoltageSetting,            // 恒压充电电压设置
    kRwThermalRegulationThresholdSetting,  // 热调节阈值设置

    kRwChargerTimerConfigure = 0x67,  // 充电定时器配置

    // 电量计
    kRwFuelGaugeControl = 0x71,  // 电量计控制
    kRoBatteryTemperature,       // 电池温度
    kRoBatterySoh,               // 电池健康度
    kRoBatteryPercentage,        // 电池百分比

    // ADC相关
    kRwAdcChannelEnableControl = 0x90,  // ADC通道使能控制
    kRoVbatH,                           // 电池电压高字节
    kRoVbatL,                           // 电池电压低字节
    kRoIbatH,                           // 电池电流高字节
    kRoIbatL,                           // 电池电流低字节
    kRoTsH,                             // TS电压高字节
    kRoTsL,                             // TS电压低字节
    kRoVbusCurrentH,                    // VBUS电流高字节
    kRoVbusCurrentL,                    // VBUS电流低字节
    kRoVbusVoltageH,                    // VBUS电压高字节
    kRoVbusVoltageL,                    // VBUS电压低字节
    kRwAdcDataSelect,                   // ADC数据选择
    kRoAdcDataH,                        // ADC数据高字节
    kRoAdcDataL,                        // ADC数据低字节

    // 中断
    kRwIrqEnable0 = 0x40,  // 中断使能0
    kRwIrqEnable1,         // 中断使能1
    kRwIrqEnable2,         // 中断使能2
    kRwIrqEnable3,         // 中断使能3

    kRwIrqStatus0 = 0x48,  // 中断状态0
    kRwIrqStatus1,         // 中断状态1
    kRwIrqStatus2,         // 中断状态2
    kRwIrqStatus3,         // 中断状态3

    // PD相关
    kRwTcpcControl = 0xB9,  // TCPC控制
    kRwRoleControl,         // 角色控制
    kRwCommand = 0xC3,      // 命令寄存器
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x34;
  static constexpr uint8_t kDeviceId = 0x02;
  static constexpr uint8_t kInitSequence[] = {

      // 输入电流限制修改为最大
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwInputCurrentLimitControl), 0B11111100,

      // 输入电压限制修改为4.7v
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwInputVoltageLimitControl), 0B00001100,

      // 设置充电电流为512mA
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kRwIccSetting), 0B00001000};

  int32_t rst_;
};
}  // namespace cpp_bus_driver
