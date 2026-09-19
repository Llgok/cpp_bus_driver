/*
 * @Description: AXP517 电源管理、Fuel Gauge 与 Type-C/PD 控制器驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-09-18 16:30:00
 * @LastEditTime: 2026-09-20 01:24:19
 * @License: GPL 3.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {

class Axp517 final : public I2cChipBase {
 public:
  enum class ChargeStatus : uint8_t {
    kTrickleCharge,
    kPrecharge,
    kConstantCurrent,
    kConstantVoltage,
    kChargeDone,
    kNotCharging,
    kInvalid,
  };

  enum class BatteryCurrentDirection : uint8_t {
    kStandby,
    kCharge,
    kDischarge,
    kInvalid,
  };

  enum class BatteryHealth : uint8_t {
    kUnknown,
    kGood,
    kCold,
    kOverheat,
    kOvervoltage,
    kSafetyTimerExpired,
  };

  enum class CapacityLevel : uint8_t {
    kCritical,
    kLow,
    kNormal,
    kHigh,
    kFull,
  };

  enum class NtcFault : uint8_t {
    kNormal = 0,
    kColdCharge = 1,
    kHotCharge = 2,
    kColdWork = 5,
    kHotWork = 6,
    kUnknown = 7,
  };

  enum class Bc12Result : uint8_t {
    kUnknown = 0,
    kSdp = 1,
    kCdp = 2,
    kDcp = 3,
  };

  enum class AdcInput : uint8_t {
    kDieTemperature = 0,
    kSystemVoltage,
    kTs,
    kTsAverage,
    kBatteryCurrent,
    kBatteryAverageCurrent,
    kChargeCurrent,
    kDischargeCurrent,
  };

  enum class DieTemperatureModel : uint8_t {
    kOfficialDriver,
    kDatasheetV1,
  };

  enum class AdcChannel : uint8_t {
    kBatteryVoltage = 0x01,
    kTs = 0x02,
    kVbusVoltage = 0x04,
    kSystemVoltage = 0x08,
    kDieTemperature = 0x10,
    kChargeCurrent = 0x20,
    kDischargeCurrent = 0x40,
    kVbusCurrent = 0x80,
  };

  enum class BatfetMode : uint8_t {
    kAuto = 0,
    kOn = 1,
    kOff = 4,
  };

  enum class GpioSource : uint8_t {
    kRegister = 0,
    kPdIrq = 1,
  };

  enum class GpioMode : uint8_t {
    kInput,
    kOutput,
  };

  enum class GpioOutput : uint8_t {
    kHighImpedance,
    kLow,
    kHigh,
  };

  enum class ChargeMode : uint8_t {
    kRuntime,
    kSuspend,
    kShutdown,
    kLimited,
    kPaused,
  };

  enum class CcTermination : uint8_t {
    kRa,
    kRp,
    kRd,
    kOpen,
  };

  enum class RpCurrent : uint8_t {
    kDefault,
    k1500Ma,
    k3000Ma,
  };

  enum class CcState : uint8_t {
    kOpen,
    kRa,
    kRd,
    kRpDefault,
    kRp1500Ma,
    kRp3000Ma,
  };

  enum class TypeCRole : uint8_t {
    kSink,
    kSource,
    kDualRole,
  };

  enum class Polarity : uint8_t {
    kCc1,
    kCc2,
  };

  enum class PdRevision : uint8_t {
    kRev10,
    kRev20,
    kRev30,
  };

  enum class PdTransmitType : uint8_t {
    kSop,
    kSopPrime,
    kSopDoublePrime,
    kDebugPrime,
    kDebugDoublePrime,
    kHardReset,
    kCableReset,
    kBistMode2,
  };

  enum class TcpcCommand : uint8_t {
    kWakeI2c = 0x11,
    kDisableVbusDetect = 0x22,
    kEnableVbusDetect = 0x33,
    kDisableSinkVbus = 0x44,
    kSinkVbus = 0x55,
    kDisableSourceVbus = 0x66,
    kSourceVbusDefault = 0x77,
    kSourceVbusHigh = 0x88,
    kLookForConnection = 0x99,
    kReceiveOneMore = 0xAA,
    kI2cIdle = 0xFF,
  };

  enum class Irq : uint64_t {
    kVbusFault = 0x1,
    kVbusOvervoltage = 0x2,
    kBoostOvervoltage = 0x4,
    kGaugeNewSoc = 0x10,
    kGaugeWatchdog = 0x20,
    kSocWarning = 0x40,
    kSocShutdown = 0x80,
    kPowerKeyRising = 0x100,
    kPowerKeyFalling = 0x200,
    kPowerKeyLongPress = 0x400,
    kPowerKeyShortPress = 0x800,
    kBatteryRemoved = 0x1000,
    kBatteryInserted = 0x2000,
    kVbusRemoved = 0x4000,
    kVbusInserted = 0x8000,
    kBatteryOvervoltage = 0x10000,
    kChargeTimerExpired = 0x20000,
    kDieOvertemperature = 0x40000,
    kChargeStarted = 0x80000,
    kChargeDone = 0x100000,
    kBatfetOvercurrent = 0x200000,
    kSmoothingFinished = 0x400000,
    kWatchdogExpired = 0x800000,
    kBatteryColdWork = 0x1000000,
    kBatteryHotWork = 0x2000000,
    kBatteryColdCharge = 0x4000000,
    kBatteryHotCharge = 0x8000000,
    kBatteryTemperatureRecovered = 0x10000000,
    kSocError = 0x20000000,
    kBc12Changed = 0x40000000,
    kBc12Finished = 0x80000000,
    kQcFailed = 0x100000000,
    kQcSucceeded = 0x200000000,
  };

  enum class PdAlert : uint16_t {
    kCcChanged = 0x0001,
    kPowerChanged = 0x0002,
    kRxMessage = 0x0004,
    kRxHardReset = 0x0008,
    kTxFailed = 0x0010,
    kTxDiscarded = 0x0020,
    kTxSuccess = 0x0040,
    kVoltageAlarmHigh = 0x0080,
    kVoltageAlarmLow = 0x0100,
    kFault = 0x0200,
    kRxOverflow = 0x0400,
    kSinkDisconnected = 0x0800,
    kExtendedStatus = 0x2000,
    kExtendedAlert = 0x4000,
    kVendor = 0x8000,
  };

  struct ChipId {
    uint8_t chip_id = 0;
    uint8_t extended_id = 0;
  };

  struct Status {
    bool current_limited = false;
    bool thermal_regulation = false;
    bool battery_active = false;
    bool battery_present = false;
    bool batfet_on = false;
    bool vbus_good = false;
    bool vindpm_active = false;
    bool system_on = false;
    ChargeStatus charge = ChargeStatus::kInvalid;
    BatteryCurrentDirection current_direction =
        BatteryCurrentDirection::kInvalid;
  };

  struct FaultStatus {
    NtcFault ntc = NtcFault::kUnknown;
    bool system_overvoltage = false;
    bool battery_undervoltage = false;
  };

  struct CcStatus {
    CcState cc1 = CcState::kOpen;
    CcState cc2 = CcState::kOpen;
    bool looking_for_connection = false;
    bool sink_attached = false;
    bool source_attached = false;
    bool audio_accessory = false;
    bool debug_accessory = false;
  };

  struct TcpcId {
    uint16_t vendor_id = 0;
    uint16_t product_id = 0;
    uint16_t device_revision = 0;
    uint16_t type_c_revision = 0;
    uint16_t pd_revision = 0;
    uint16_t interface_revision = 0;
  };

  struct TcpcStatus {
    uint8_t power = 0;
    uint8_t fault = 0;
    uint8_t extended_status = 0;
    uint8_t extended_alert = 0;
    bool vbus_present = false;
    bool sourcing_vbus = false;
    bool sinking_vbus = false;
    bool vconn_present = false;
    bool vbus_safe0v = false;
  };

  // PD 报文支持的最大数据对象数量。
  static constexpr size_t kMaxPdDataObjects = 7;

  struct PdMessage {
    PdTransmitType frame_type = PdTransmitType::kSop;
    uint16_t header = 0;
    std::array<uint32_t, kMaxPdDataObjects> data_objects{};
    // 必须与 header[14:12] 一致，不包含消息头本身。
    uint8_t data_object_count = 0;
  };

  struct InterruptStatus {
    uint64_t power = 0;
    uint16_t pd = 0;
    bool Has(Irq irq) const {
      return (power & static_cast<uint64_t>(irq)) != 0;
    }
    bool Has(PdAlert alert) const {
      return (pd & static_cast<uint16_t>(alert)) != 0;
    }
  };

  struct ChargeProfile {
    uint16_t voltage_mv = 4200;
    uint16_t runtime_current_ma = 512;
    uint16_t suspend_current_ma = 512;
    uint16_t shutdown_current_ma = 512;
    uint16_t limited_current_ma = 256;
    uint16_t precharge_current_ma = 128;
    uint16_t termination_current_ma = 64;
    uint8_t warning_percent = 15;
    uint8_t shutdown_percent = 0;
  };

  struct NtcConfig {
    // 必须提供实测/电池规格参数；依次对应官方 para1～para16，单位 mV。
    // 温度点：-25,-15,-10,-5,0,5,10,20,30,40,45,50,55,60,70,80 ℃。
    std::array<uint16_t, 16> voltage_mv{};
    uint8_t current_ua = 60;
    uint16_t charge_cold_mv = 1312;
    uint16_t charge_hot_mv = 176;
    uint16_t work_cold_mv = 1984;
    uint16_t work_hot_mv = 152;
    uint16_t cold_hysteresis_mv = 32;
    uint16_t hot_hysteresis_mv = 4;
    bool compensate_offset = true;
  };

  struct JeitaConfig {
    uint16_t cool_mv = 880;
    uint16_t warm_mv = 240;
    uint8_t cool_current_reduction = 0;  // 寄存器编码，0～3。
    uint8_t warm_current_reduction = 0;
    uint8_t cool_voltage_reduction = 0;
    uint8_t warm_voltage_reduction = 0;
  };

  struct PowerKeyConfig {
    uint16_t on_time_ms = 1000;
    uint16_t long_press_ms = 1500;
    uint16_t off_time_ms = 6000;
    bool off_enabled = true;
    bool irq_wakeup = false;
  };

  struct GaugeDiagnostics {
    uint8_t soc_percent = 0;
    uint8_t calculated_soc_percent = 0;
    uint16_t pct_now = 0;
    bool reset_recommended = false;
  };

  explicit Axp517(std::shared_ptr<I2cBusBase> bus,
                  int16_t address = kDefaultAddress)
      : I2cChipBase(bus, address) {}

  /**
   * @brief 初始化总线并读取芯片标识，不写入电池型号和板级充电参数。
   * @param freq_hz I2C 工作频率。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;

  /**
   * @brief 释放总线资源；不关闭供电。
   * @param delete_bus 是否释放底层总线。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取 0x03/0x0E 芯片标识；官方未提供固定 ID 判定值。
   * @param chip_id 用于接收芯片标识。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetChipId(ChipId& chip_id);

  /**
   * @brief 获取电池、VBUS、充电和系统状态。
   * @param status 用于接收状态信息。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetStatus(Status& status);

  /**
   * @brief 获取 NTC 状态及锁存的系统过压、电池欠压标志。
   * @param status 用于接收状态信息。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetFaultStatus(FaultStatus& status);

  /**
   * @brief 清除指定 BMU 故障，mask 仅允许 0x08 寄存器的 bit2/3。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ClearFaults(uint8_t mask);

  /**
   * @brief 获取 BC1.2 检测结果；未识别端口返回 kUnknown。
   * @param result 用于接收读取结果。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBc12Result(Bc12Result& result);

  /**
   * @brief 配置 BC1.2 检测时钟。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBc12DetectEnable(bool enable);

  /**
   * @brief 设置充电使能，按官方流程切换 Buck；关闭时等待 1000 ms。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetChargeEnable(bool enable);

  /**
   * @brief 设置恒流充电电流，0～5120 mA，64 mA/步进，超范围饱和。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetChargeCurrent(uint16_t current_ma);

  /**
   * @brief 读取当前恒流充电设定，单位 mA。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetChargeCurrent(uint16_t& current_ma);

  /**
   * @brief 按官方边界选择 4.0/4.1/4.2/4.35/4.4/5.0 V 档位。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetChargeVoltage(uint16_t voltage_mv);

  /**
   * @brief 读取实际恒压充电设定，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetChargeVoltage(uint16_t& voltage_mv);

  /**
   * @brief 设置预充电电流，0～960 mA，64 mA/步进。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetPrechargeCurrent(uint16_t current_ma);

  /**
   * @brief 设置涓流充电电流，32～224 mA，32 mA/步进。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTrickleCurrent(uint16_t current_ma);

  /**
   * @brief 设置终止电流及终止使能，64～960 mA，64 mA/步进。
   * @param current_ma 目标电流，单位 mA。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTerminationCurrent(uint16_t current_ma, bool enable = true);

  /**
   * @brief 设置 DPM 状态下是否禁止充电终止。
   * @param disable 是否禁止充电终止。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTerminationDisabledInDpm(bool disable);

  /**
   * @brief 设置输入限流，100～3250 mA；低于 100 mA 时关闭 Buck。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetInputCurrentLimit(uint16_t current_ma);

  /**
   * @brief 读取输入限流，Buck 关闭时返回 0 mA。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetInputCurrentLimit(uint16_t& current_ma);

  /**
   * @brief 设置 VINDPM，3600～16200 mV，100 mV/步进。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetInputVoltageLimit(uint16_t voltage_mv);

  /**
   * @brief 读取 VINDPM，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetInputVoltageLimit(uint16_t& voltage_mv);

  /**
   * @brief 根据已协商的电压配置充电路径；不会主动发起 PD 协商。
   * @param voltage_mv 目标电压，单位 mV。
   * @param default_vindpm_mv 默认 VINDPM 电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ApplyNegotiatedInputVoltage(uint16_t voltage_mv,
                                  uint16_t default_vindpm_mv = 4600);

  /**
   * @brief 应用电池充电参数并保存运行/休眠/关机电流配置。
   * @param profile 充电参数配置。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ConfigureCharging(const ChargeProfile& profile);

  /**
   * @brief 切换运行、休眠、关机、限流或暂停充电电流。
   * @param mode 充电或工作模式。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetChargeMode(ChargeMode mode);

  /**
   * @brief 设置 Buck 模块使能。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBuckEnable(bool enable);

  /**
   * @brief 设置 Boost 模块使能。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBoostEnable(bool enable);

  /**
   * @brief 设置 Boost 电压，4550～5510 mV，64 mV/步进。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBoostVoltage(uint16_t voltage_mv);

  /**
   * @brief 设置最小系统电压，1.0～3.7 V，100 mV/步进。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetMinimumSystemVoltage(uint16_t voltage_mv);

  /**
   * @brief 设置 Boost 关闭阈值，2.6/2.8/3.0/3.2 V。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBoostDisableThreshold(uint16_t voltage_mv);

  /**
   * @brief 设置 Boost 模式 RBFET 限流编码，500/900/1500/2000 mA。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBoostRbfetCurrentLimit(uint16_t current_ma);

  /**
   * @brief 设置重充电压差编码，直接对应官方 RECHG_CFG[2:0]。
   * @param code 寄存器编码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetRechargeThreshold(uint8_t code);

  /**
   * @brief 设置充电开关频率编码，直接对应官方 CHG_FREQ。
   * @param code 寄存器编码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetChargeFrequency(uint8_t code);

  /**
   * @brief 设置安全充电定时器编码和使能。
   * @param enable 是否使能功能。
   * @param code 寄存器编码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetChargeTimer(bool enable, uint8_t code);

  /**
   * @brief 设置电池 IR 补偿电阻和电压补偿编码。
   * @param resistance_code 电阻补偿编码。
   * @param voltage_code 电压补偿编码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBatteryIrCompensation(uint8_t resistance_code, uint8_t voltage_code);

  /**
   * @brief 设置软件充电路径配置寄存器。
   * @param value 用于接收寄存器值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetSwitchChargeConfiguration(uint8_t value);

  /**
   * @brief 读取 Boost 电压设定，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBoostVoltage(uint16_t& voltage_mv);

  /**
   * @brief 设置 RBFET 强制导通；关闭时按官方流程处理 VBUS 中断。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetRbfetForceEnable(bool enable);

  /**
   * @brief 设置 BATFET 自动、强制导通或强制关闭。
   * @param mode 充电或工作模式。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBatfetMode(BatfetMode mode);

  /**
   * @brief 设置运输模式。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetShippingModeEnable(bool enable);

  /**
   * @brief 配置看门狗，超时档位为 1/2/4/8/16/32/64/128 秒。
   * @param enable 是否使能功能。
   * @param timeout_s 看门狗超时时间，单位秒。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetWatchdog(bool enable, uint8_t timeout_s = 1);

  /**
   * @brief 清除看门狗计数。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool FeedWatchdog();

  /**
   * @brief 配置 PWRON 开机、长按、关机时间及 IRQ 唤醒。
   * @param config 功能配置参数。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ConfigurePowerKey(const PowerKeyConfig& config);

  /**
   * @brief 设置按键 16 秒复位功能。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetLongPressResetEnable(bool enable);

  /**
   * @brief 设置芯片热调节阈值，60/80/100/120 ℃。
   * @param celsius 温度阈值，单位摄氏度。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetThermalRegulationThreshold(uint8_t celsius);

  /**
   * @brief 设置芯片过温关机和阈值，115/125/135 ℃。
   * @param enable 是否使能功能。
   * @param celsius 温度阈值，单位摄氏度。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetThermalShutdown(bool enable, uint8_t celsius = 125);

  /**
   * @brief 设置 CHGLED 模块使能和功能编码（0～7）。
   * @param enable 是否使能功能。
   * @param function CHGLED 功能编码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ConfigureChargeLed(bool enable, uint8_t function);

  /**
   * @brief 设置电池连接检测。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBatteryDetectionEnable(bool enable);

  /**
   * @brief 配置 GPIO 信号源、输入/输出方向和输出电平。
   * @param source 是否作为 Source。
   * @param mode 充电或工作模式。
   * @param output 用于接收 GPIO 电平。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ConfigureGpio(GpioSource source, GpioMode mode, GpioOutput output);

  /**
   * @brief 读取 GPIO 电平。
   * @param high 用于接收 GPIO 高电平状态。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ReadGpio(bool& high);

  /**
   * @brief 写入 8 路 ADC 使能掩码，位定义见 AdcChannel。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetAdcChannels(uint8_t mask);

  /**
   * @brief 单独启用或关闭一个 ADC 通道，保留其余通道配置。
   * @param channel ADC 通道。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetAdcChannelEnable(AdcChannel channel, bool enable);

  /**
   * @brief 选通并读取 ADC 原始值，通道切换后等待至少 1 ms。
   * @param input ADC 输入类型。
   * @param raw 用于接收 ADC 原始值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ReadAdc(AdcInput input, uint16_t& raw);

  /**
   * @brief 读取电池电压，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBatteryVoltage(uint16_t& voltage_mv);

  /**
   * @brief 读取电池电流，单位 mA；正值充电、负值放电。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBatteryCurrent(float& current_ma);

  /**
   * @brief 读取电池平均电流，单位 mA。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBatteryAverageCurrent(float& current_ma);

  /**
   * @brief 读取充电 ADC 电流，单位 mA。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetChargingCurrent(float& current_ma);

  /**
   * @brief 读取放电 ADC 电流，单位 mA，保留官方符号。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetDischargingCurrent(float& current_ma);

  /**
   * @brief 读取 TS 引脚电压，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetTsVoltage(float& voltage_mv);

  /**
   * @brief 读取 VBUS 电压，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetVbusVoltage(uint16_t& voltage_mv);

  /**
   * @brief 读取 VBUS 电流，单位 mA。
   * @param current_ma 目标电流，单位 mA。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetVbusCurrent(float& current_ma);

  /**
   * @brief 自动选通并读取 VSYS 电压，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetSystemVoltage(uint16_t& voltage_mv);

  /**
   * @brief 读取结温，默认完整复现官方算法；可选择 V1 手册的换算公式。
   * @param celsius 温度阈值，单位摄氏度。
   * @param model 温度换算模型。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetDieTemperature(
      float& celsius,
      DieTemperatureModel model = DieTemperatureModel::kOfficialDriver);

  /**
   * @brief 配置 NTC 表、偏置电流、保护阈值及滞回；表必须严格递减。
   * @param config 功能配置参数。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ConfigureNtc(const NtcConfig& config);

  /**
   * @brief 设置 TS 是否参与温度保护，关闭时同时禁用 JEITA。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetNtcEnable(bool enable);

  /**
   * @brief 设置 TS 固定 ADC 校准值（0～0x3FFF）及数据源。
   * @param use_fixed_data 是否使用固定校准值。
   * @param raw 用于接收 ADC 原始值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTsCalibration(bool use_fixed_data, uint16_t raw);

  /**
   * @brief 配置 JEITA 冷/暖阈值和电流、电压降档。
   * @param config 功能配置参数。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ConfigureJeita(const JeitaConfig& config);

  /**
   * @brief 设置 JEITA 使能；开启前需启用 NTC。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetJeitaEnable(bool enable);

  /**
   * @brief 依据 NTC 表与官方充放电偏移补偿读取电池温度。
   * @param celsius 温度阈值，单位摄氏度。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBatteryTemperature(float& celsius);

  /**
   * @brief 读取原始 Gauge SOC（0～100）；无电池返回 0。
   * @param percent 电量百分比。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetGaugeSoc(uint8_t& percent);

  /**
   * @brief 读取显示用 SOC，优先采用官方 DATA_BUFF 电量平滑值。
   * @param percent 电量百分比。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBatteryLevel(uint8_t& percent);

  /**
   * @brief 读取电池健康状态，与 SOH 百分比区分。
   * @param health 用于接收电池健康状态。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBatteryHealth(BatteryHealth& health);

  /**
   * @brief 读取 Gauge SOH 百分比，越界值视为无效。
   * @param percent 电量百分比。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetBatterySoh(uint8_t& percent);

  /**
   * @brief 获取电量等级，依据当前 Gauge 阈值分类。
   * @param level 用于接收电量等级。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetCapacityLevel(CapacityLevel& level);

  /**
   * @brief 设置低电量阈值：warning 5～20%，shutdown 0～15%。
   * @param warning_percent 低电量告警百分比。
   * @param shutdown_percent 关机百分比。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetGaugeThresholds(uint8_t warning_percent, uint8_t shutdown_percent);

  /**
   * @brief 读取低电量告警与关机阈值。
   * @param warning_percent 低电量告警百分比。
   * @param shutdown_percent 关机百分比。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetGaugeThresholds(uint8_t& warning_percent, uint8_t& shutdown_percent);

  /**
   * @brief 读取循环次数。
   * @param cycles 用于接收循环次数。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetCycleCount(uint16_t& cycles);

  /**
   * @brief 读取 Gauge 预计放空时间原始值；官方资料未定义时间单位。
   * @param value 用于接收寄存器值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetTimeToEmptyRaw(uint16_t& value);

  /**
   * @brief 读取 Gauge 预计充满时间原始值；官方资料未定义时间单位。
   * @param value 用于接收寄存器值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetTimeToFullRaw(uint16_t& value);

  /**
   * @brief 设置 Gauge 时钟使能。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetGaugeEnable(bool enable);

  /**
   * @brief 读取电池模型更新标记。
   * @param updated 用于接收模型更新结果。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool IsBatteryModelUpdated(bool& updated);

  /**
   * @brief 写入并逐字节校验 1～128 字节模型；失败关闭 BROM 并清除标记。
   * @param data 电池模型数据。
   * @param length 数据长度，单位字节。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool UpdateBatteryModel(const uint8_t* data, size_t length);

  /**
   * @brief 仅在模型未更新时下载；返回 updated 表示本次是否完成下载。
   * @param data 电池模型数据。
   * @param length 数据长度，单位字节。
   * @param updated 用于接收模型更新结果。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ApplyBatteryModelIfNeeded(const uint8_t* data, size_t length,
                                 bool& updated);

  /**
   * @brief 按官方暂停充电、复位 MCU、恢复充电流程重启 Gauge。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ResetGauge();

  /**
   * @brief 读取 FG 间接地址的 16 位数据；默认等待官方规定的 2000 ms。
   * @param address Gauge 间接地址。
   * @param value 用于接收寄存器值。
   * @param settle_ms 操作后的等待时间，单位毫秒。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ReadGaugeWord(uint8_t address, uint16_t& value,
                     uint32_t settle_ms = 2000);

  /**
   * @brief 检查 SOC/PCT 异常，可选择复位；阻塞至少 4 秒。
   * @param diagnostics 用于接收诊断结果。
   * @param recover 是否自动恢复异常 Gauge。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool CheckGauge(GaugeDiagnostics& diagnostics, bool recover = false);

  /**
   * @brief 启动 DATA_BUFF 电量平滑，参数为显示电量 0～100%。
   * @param percent 电量百分比。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool StartSocSmoothing(uint8_t percent);

  /**
   * @brief 执行一次官方电量平滑步骤，调用方每 30 秒调用一次。
   * @param active 用于接收平滑状态。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool StepSocSmoothing(bool& active);

  /**
   * @brief 停止电量平滑，显示切回 Gauge SOC。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool StopSocSmoothing();

  /**
   * @brief 按官方模型估算剩余/满充容量，单位 uAh，结果下限为 0。
   * @param design_capacity_mah 设计容量，单位 mAh。
   * @param cycle_life 设计循环寿命。
   * @param lifetime_loss_percent 寿命衰减百分比。
   * @param remaining_uah 用于接收剩余容量，单位 uAh。
   * @param full_uah 用于接收满充容量，单位 uAh。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool EstimateCapacity(uint16_t design_capacity_mah, uint16_t cycle_life,
                        uint8_t lifetime_loss_percent,
                        uint32_t& remaining_uah, uint32_t& full_uah);

  /**
   * @brief 读取全部 5 组普通 IRQ 与独立 PD Alert，不自动清除。
   * @param status 用于接收状态信息。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetInterruptStatus(InterruptStatus& status);

  /**
   * @brief 读取 5 组普通 IRQ 状态。
   * @param status 用于接收状态信息。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetIrqStatus(uint64_t& status);

  /**
   * @brief 设置指定普通 IRQ 使能；不影响掩码外的位。
   * @param mask 寄存器位掩码。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetIrqEnable(uint64_t mask, bool enable);

  /**
   * @brief 读取普通 IRQ 使能掩码。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetIrqEnable(uint64_t& mask);

  /**
   * @brief 按 W1C 语义清除指定普通 IRQ，建议传入已读取的快照。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ClearIrqs(uint64_t mask);

  /**
   * @brief 清除全部普通 IRQ；不清除 PD 接收队列。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ClearAllIrqs();

  /**
   * @brief 读取 PD Alert。
   * @param alerts 用于接收 PD Alert。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetPdAlerts(uint16_t& alerts);

  /**
   * @brief 写入 PD Alert 使能掩码。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetPdAlertMask(uint16_t mask);

  /**
   * @brief 清除指定 PD Alert；RX 消息应先读取或显式丢弃。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ClearPdAlerts(uint16_t mask);

  /**
   * @brief 读取 TCPC 厂商、产品及协议版本。
   * @param id 用于接收芯片标识。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetTcpcId(TcpcId& id);

  /**
   * @brief 按官方流程初始化 TCPC；enable_pd_irqs 仅在接入协议栈时开启。
   * @param enable_pd_irqs 是否使能 PD 中断。
   * @param self_powered 是否由本芯片供电。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool InitTypeC(bool enable_pd_irqs = false, bool self_powered = true);

  /**
   * @brief 设置 CC 检测时钟，开启后等待 20 ms。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTypeCEnable(bool enable);

  /**
   * @brief 软件复位 TCPC，调用后需重新初始化角色和事件掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ResetTypeC();

  /**
   * @brief 配置 CC1/CC2 端接、Rp 电流及 DRP 开关。
   * @param cc1 CC1 端接类型。
   * @param cc2 CC2 端接类型。
   * @param current Rp 电流。
   * @param dual_role 是否开启双角色。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetCcTerminations(CcTermination cc1, CcTermination cc2,
                         RpCurrent current = RpCurrent::kDefault,
                         bool dual_role = false);

  /**
   * @brief 设置双 CC 端接；VCONN 导通时将非通信 CC 设为开路。
   * @param termination CC 端接类型。
   * @param current Rp 电流。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetCc(CcTermination termination,
              RpCurrent current = RpCurrent::kDefault);

  /**
   * @brief 配置 Sink/Source/DRP 并启动连接检测。
   * @param role Type-C 角色。
   * @param current Rp 电流。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTypeCRole(TypeCRole role, RpCurrent current = RpCurrent::kDefault);

  /**
   * @brief 分别解码 CC1/CC2 的 Rp/Rd/Ra 状态和附件连接类型。
   * @param status 用于接收状态信息。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetCcStatus(CcStatus& status);

  /**
   * @brief 确定通信 CC 极性，停止 DRP 并断开非通信 CC。
   * @param polarity CC 极性。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetPolarity(Polarity polarity);

  /**
   * @brief 按官方 APPLY_RC 流程将非通信 CC 置为开路。
   * @param polarity CC 极性。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ApplyCcResistance(Polarity polarity);

  /**
   * @brief 设置 VCONN 供电。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetVconnEnable(bool enable);

  /**
   * @brief 开启或关闭 TCPC VBUS 检测。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetVbusDetectEnable(bool enable);

  /**
   * @brief 设置 TCPC Source/Sink VBUS 命令；不允许两者同时开启。
   * @param source 是否作为 Source。
   * @param sink 是否作为 Sink。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetVbusPath(bool source, bool sink);

  /**
   * @brief 获取 TCPC 电源、故障、扩展状态与 vSafe0V。
   * @param status 用于接收状态信息。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetTcpcStatus(TcpcStatus& status);

  /**
   * @brief 清除指定 TCPC 故障（单字节 W1C）。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ClearTcpcFaults(uint8_t mask);

  /**
   * @brief 配置电源、故障、扩展状态和扩展事件子掩码。
   * @param power 电源状态掩码。
   * @param fault 故障状态掩码。
   * @param extended_status 扩展状态掩码。
   * @param extended_alert 扩展事件掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTcpcStatusMasks(uint8_t power, uint8_t fault,
                          uint8_t extended_status, uint8_t extended_alert);

  /**
   * @brief 清除扩展 TCPC 事件。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ClearTcpcExtendedAlerts(uint8_t mask);

  /**
   * @brief 发送 TCPC 命令。
   * @param command TCPC 命令。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SendTcpcCommand(TcpcCommand command);

  /**
   * @brief 设置 PD 消息头的电源角色、数据角色和协议版本。
   * @param source 是否作为 Source。
   * @param host 是否为主机数据角色。
   * @param revision PD 协议版本。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetPdMessageHeader(bool source, bool host,
                          PdRevision revision = PdRevision::kRev20);

  /**
   * @brief 配置接收类型位掩码 bit0～5；默认 SOP，硬复位接收需显式开启。
   * @param mask 寄存器位掩码。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetPdReceiveMask(uint8_t mask);

  /**
   * @brief 读取一帧 PD 消息，校验长度后才清除 RX Alert；空队列返回 false。
   * @param message PD 消息。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool ReceivePdMessage(PdMessage& message);

  /**
   * @brief 显式丢弃当前 PD 接收消息，用于处理损坏或不支持的帧。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool DiscardPdMessage();

  /**
   * @brief 向固定地址 TX FIFO 写入并发送 PD 帧或硬复位/BIST 命令。
   * @param type PD 发送类型。
   * @param message PD 消息。
   * @param revision PD 协议版本。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool TransmitPdMessage(PdTransmitType type, const PdMessage* message,
                          PdRevision revision = PdRevision::kRev20);

  /**
   * @brief 设置 BIST 测试模式。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetBistEnable(bool enable);

  /**
   * @brief 设置 VBUS 自动放电及弱放电。
   * @param automatic 是否自动放电。
   * @param bleed 是否弱放电。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetVbusDischarge(bool automatic, bool bleed);

  /**
   * @brief 根据 PD/PPS 合同计算断开阈值；电压为 0 时关闭断开检测。
   * @param pd_contract 是否存在 PD 合同。
   * @param pps 是否为 PPS 合同。
   * @param requested_voltage_mv 请求的 VBUS 电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetAutoDischargeThreshold(bool pd_contract, bool pps,
                                  uint16_t requested_voltage_mv);

  /**
   * @brief 设置 Fast Role Swap 及官方规定的断开阈值。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetFastRoleSwapEnable(bool enable);

  /**
   * @brief 设置 VBUS 停止放电阈值和高低报警阈值，单位 mV，25 mV/步进。
   * @param stop_discharge_mv 停止放电电压，单位 mV。
   * @param alarm_low_mv 低电压报警阈值，单位 mV。
   * @param alarm_high_mv 高电压报警阈值，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetVbusThresholds(uint16_t stop_discharge_mv, uint16_t alarm_low_mv,
                          uint16_t alarm_high_mv);

  /**
   * @brief 读取 TCPC VBUS ADC，含量程倍率，单位 mV。
   * @param voltage_mv 目标电压，单位 mV。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool GetTcpcVbusVoltage(uint16_t& voltage_mv);

  /**
   * @brief 设置软件低功耗；仅在 CC 断开时进入。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool EnterTypeCLowPower();

  /**
   * @brief 唤醒 I2C/CC 模块。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool WakeTypeC();

  /**
   * @brief 按官方充电通知流程发送硬复位并关闭/恢复 CC 时钟。
   * @param enable 是否使能功能。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool NotifyTypeCCharging(bool enable);

 private:
  // 保留官方寄存器语义；TCPC 多字节寄存器为小端，ADC 和 Gauge 为大端。
  enum class Register : uint8_t {
    kStatus0 = 0x00,                           // 电源管理状态 0
    kStatus1 = 0x01,                           // 电源管理状态 1
    kChipId = 0x03,                            // 芯片 ID
    kDataBuff = 0x04,                          // 电量显示数据缓冲
    kBcDetect = 0x05,                          // BC1.2 检测结果
    kIlimType = 0x06,                          // 输入限流类型
    kBmuFault1 = 0x08,                         // 电池管理故障状态 1
    kClkEn = 0x0B,                             // 时钟使能
    kChipIdExt = 0x0E,                         // 扩展芯片 ID
    kCommCfg = 0x10,                           // 通信配置
    kGpioCfg = 0x11,                           // GPIO 配置
    kBatfetCtrl = 0x12,                        // 电池开关管控制
    kRbfetCtrl = 0x13,                         // 反向电池开关管控制
    kDieTempCfg = 0x14,                        // 芯片温度配置
    kVsysMin = 0x15,                           // 系统最低电压
    kVindpmCfg = 0x16,                         // 输入电压动态电源管理配置
    kIinLim = 0x17,                            // 输入电流限制
    kResetCfg = 0x18,                          // 复位配置
    kModuleEn = 0x19,                          // 功能模块使能
    kWatchdogCfg = 0x1A,                       // 看门狗配置
    kGaugeThld = 0x1B,                         // 电量计阈值
    kPokSet = 0x1C,                            // 电源键配置
    kBstCfg0 = 0x1E,                           // 升压配置 0
    kBstCfg1 = 0x1F,                           // 升压配置 1
    kChgledCfg = 0x30,                         // 充电指示灯配置
    kIrqEn0 = 0x40,                            // 中断使能 0
    kIrqEn1 = 0x41,                            // 中断使能 1
    kIrqEn2 = 0x42,                            // 中断使能 2
    kIrqEn3 = 0x43,                            // 中断使能 3
    kIrqEn4 = 0x44,                            // 中断使能 4
    kIrq0 = 0x48,                              // 中断状态 0
    kIrq1 = 0x49,                              // 中断状态 1
    kIrq2 = 0x4A,                              // 中断状态 2
    kIrq3 = 0x4B,                              // 中断状态 3
    kIrq4 = 0x4C,                              // 中断状态 4
    kTsCfg = 0x50,                             // 温度检测配置
    kTsHysl2H = 0x52,                          // TS 低温滞回配置
    kTsHysh2L = 0x53,                          // TS 高温滞回配置
    kVltfChg = 0x54,                           // 充电低温保护阈值
    kVhtfChg = 0x55,                           // 充电高温保护阈值
    kVltfWork = 0x56,                          // 工作低温保护阈值
    kVhtfWork = 0x57,                          // 工作高温保护阈值
    kJeitaCfg = 0x58,                          // JEITA 配置
    kJeitaCvCfg = 0x59,                        // JEITA 恒压降额配置
    kJeitaCool = 0x5A,                         // JEITA 冷温区配置
    kJeitaWarm = 0x5B,                         // JEITA 暖温区配置
    kTsCfgDataH = 0x5C,                        // TS 校准数据高字节
    kTsCfgDataL = 0x5D,                        // TS 校准数据低字节
    kRechgCfg = 0x60,                          // 再充电配置
    kIprechgCfg = 0x61,                        // 预充电电流配置
    kIccCfg = 0x62,                            // 恒流充电电流配置
    kItermCfg = 0x63,                          // 终止充电电流配置
    kVtermCfg = 0x64,                          // 终止充电电压配置
    kTreguThld = 0x65,                         // 热调节阈值
    kChgFreq = 0x66,                           // 充电开关频率
    kChgTmrCfg = 0x67,                         // 充电安全定时器配置
    kBatDet = 0x68,                            // 电池检测配置
    kIrComp = 0x69,                            // 电池内阻补偿配置
    kSwChgCfg1 = 0x6B,                         // 软件充电路径配置 1
    kGaugeBrom = 0x70,                         // 电量计模型存储区
    kGaugeConfig = 0x71,                       // 电量计配置
    kGaugeTemperature = 0x72,                  // 电量计温度原始值
    kGaugeSoh = 0x73,                          // 电量计健康度
    kGaugeSoc = 0x74,                          // 电量计电量
    kGaugeTime2EmptyH = 0x76,                  // 预计放空时间高字节
    kGaugeTime2EmptyL = 0x77,                  // 预计放空时间低字节
    kGaugeTime2FullH = 0x78,                   // 预计充满时间高字节
    kGaugeTime2FullL = 0x79,                   // 预计充满时间低字节
    kCycleH = 0x7B,                            // 循环次数高字节
    kCycleL = 0x7C,                            // 循环次数低字节
    kTsSource = 0x82,                          // TS 数据源
    kFgAddr = 0x84,                            // Fuel Gauge 间接地址
    kFgDataH = 0x85,                           // Fuel Gauge 数据高字节
    kFgDataL = 0x86,                           // Fuel Gauge 数据低字节
    kAdcChEn0 = 0x90,                          // ADC 通道使能
    kVbatH = 0x91,                             // 电池电压高字节
    kVbatL = 0x92,                             // 电池电压低字节
    kIbatH = 0x93,                             // 电池电流高字节
    kIbatL = 0x94,                             // 电池电流低字节
    kTsH = 0x95,                               // TS 电压高字节
    kTsL = 0x96,                               // TS 电压低字节
    kIbusH = 0x97,                             // 输入电流高字节
    kIbusL = 0x98,                             // 输入电流低字节
    kVbusH = 0x99,                             // VBUS 电压高字节
    kVbusL = 0x9A,                             // VBUS 电压低字节
    kAdcControl = 0x9B,                        // ADC 控制
    kAdcRes = 0x9C,                            // ADC 转换结果
    kTcpcVendorId = 0xA0,                      // Type-C 控制器厂商 ID
    kTcpcProductId = 0xA2,                     // Type-C 控制器产品 ID
    kTcpcBcdDev = 0xA4,                        // Type-C 控制器设备版本
    kTcpcTcRev = 0xA6,                         // Type-C 协议版本
    kTcpcPdRev = 0xA8,                         // USB PD 协议版本
    kTcpcPdIntRev = 0xAA,                      // USB PD 接口版本
    kIrqPdAlertlStatus = 0xB0,                 // PD Alert 状态低字节
    kIrqPdAlerthStatus = 0xB1,                 // PD Alert 状态高字节
    kIrqPdAlertlEn = 0xB2,                     // PD Alert 使能低字节
    kTcpcAlertMask = 0xB2,                     // Type-C Alert 掩码
    kIrqPdAlerthEn = 0xB3,                     // PD Alert 使能高字节
    kTcpcPowerStatusMask = 0xB4,               // Type-C 电源状态掩码
    kTcpcFaultStatusMask = 0xB5,               // Type-C 故障状态掩码
    kTcpcExtendedStatusMask = 0xB6,            // Type-C 扩展状态掩码
    kTcpcAlertExtendedMask = 0xB7,             // Type-C 扩展 Alert 掩码
    kTcpcConfigStdOutput = 0xB8,               // Type-C 标准输出能力配置
    kTcpcCtrl = 0xB9,                          // Type-C 控制
    kTcpcRoleCtrl = 0xBA,                      // Type-C 角色控制
    kTcpcFaultCtrl = 0xBB,                     // Type-C 故障控制
    kTcpcPowerCtrl = 0xBC,                     // Type-C 电源控制
    kTcpcCcStatus = 0xBD,                      // Type-C CC 状态
    kTcpcPowerStatus = 0xBE,                   // Type-C 电源状态
    kTcpcFaultStatus = 0xBF,                   // Type-C 故障状态
    kTcpcExtendedStatus = 0xC0,                // Type-C 扩展状态
    kTcpcAlertExtended = 0xC1,                 // Type-C 扩展 Alert 状态
    kTcpcCommand = 0xC3,                       // Type-C 命令
    kTcpcDevCap1 = 0xC4,                       // Type-C 设备能力 1
    kTcpcDevCap2 = 0xC6,                       // Type-C 设备能力 2
    kTcpcStdInputCap = 0xC8,                   // Type-C 标准输入能力
    kTcpcStdOutputCap = 0xC9,                  // Type-C 标准输出能力
    kTcpcMsgHdrInfo = 0xCE,                    // PD 消息头配置
    kTcpcRxDetect = 0xCF,                      // PD 接收类型检测
    kTcpcVbusVoltage = 0xD0,                   // Type-C VBUS 电压
    kTcpcVbusSinkDisconnectThresh = 0xD2,      // Type-C Sink 断开电压阈值
    kTcpcVbusStopDischargeThresh = 0xD4,       // Type-C 停止放电电压阈值
    kTcpcVbusVoltageAlarmHiCfg = 0xD6,         // Type-C VBUS 高电压报警配置
    kTcpcVbusVoltageAlarmLoCfg = 0xD8,         // Type-C VBUS 低电压报警配置
    kTcpcRxByteCnt = 0xDA,                     // PD 接收数据长度
    kTcpcTransmit = 0xDB,                      // PD 发送控制
    kTcpcTxByteCnt = 0xDC,                     // PD 发送数据长度
    kAwakeEn = 0xE0,                           // Type-C 唤醒控制
    kPdState = 0xE3,                           // PD 状态控制
    kCcCnntSta = 0xE6,                         // CC 连接状态
    kCcGeneralControl = 0xE8,                  // CC 通用控制
    kPhyBmcTxCtrl = 0xE9,                      // BMC 发送控制
    kVbusCcPeriodFreq = 0xEA,                  // VBUS 与 CC 周期频率
    kTwiAddrStatic = 0xEB,                     // TWI 静态地址
    kTwiAddrExt = 0xFF,                        // TWI 扩展地址
  };

  static constexpr int32_t kDefaultFrequencyHz = 100000;
  static constexpr uint8_t kDefaultAddress = 0x34;

  // 芯片上电后的默认充电参数初始化序列。
  static constexpr uint8_t kInitSequence[] = {
      // 输入电流限制设置为 2000 mA。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kIinLim), 0x98,
      // 输入电压限制设置为 4.7 V。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kVindpmCfg), 0x0C,
      // 充电电流设置为 512 mA。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kIccCfg), 0x08};

  /**
   * @brief 写入 COMM_CFG 的可写配置位。
   * @param value 用于接收寄存器值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetCommonConfiguration(uint8_t value);

  /**
   * @brief 写入 TCPC 标准输入/输出能力配置。
   * @param value 用于接收寄存器值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool SetTcpcStandardConfiguration(uint8_t value);

  /**
   * @brief 读取寄存器，并记录访问失败信息
   * @param reg 寄存器地址
   * @param data 接收缓冲区
   * @param length 读取字节数
   * @return 读取成功返回true，否则返回false
   */
  bool ReadRegister(uint8_t reg, uint8_t* data, size_t length = 1);

  /**
   * @brief 写入寄存器，并记录访问失败信息
   * @param reg 寄存器地址
   * @param value 待写入数据
   * @return 写入成功返回true，否则返回false
   */
  bool WriteRegister(uint8_t reg, uint8_t value);

  /**
   * @brief 更新普通 RW 寄存器位；禁止用于 IRQ/W1C/FIFO。
   * @param reg 寄存器地址。
   * @param mask 寄存器位掩码。
   * @param value 用于接收寄存器值。
   * @return 执行成功返回 true，失败返回 false。
   */
  bool UpdateRegisterBits(Register reg, uint8_t mask, uint8_t value);

  /**
   * @brief 从固定寄存器地址重复读取数据。
   * @param reg 寄存器地址。
   * @param data 接收数据的缓冲区。
   * @param length 读取长度，单位字节。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool ReadNoIncrement(Register reg, uint8_t* data, size_t length);

  /**
   * @brief 连续写入寄存器数据。
   * @param reg 起始寄存器地址。
   * @param data 待写入数据的缓冲区。
   * @param length 写入长度，单位字节。
   * @return 写入成功返回 true，失败返回 false。
   */
  bool WriteBytes(Register reg, const uint8_t* data, size_t length);

  /**
   * @brief 读取大端序 16 位寄存器值。
   * @param reg 起始寄存器地址。
   * @param value 用于接收寄存器值。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool ReadBigEndian16(Register reg, uint16_t& value);

  /**
   * @brief 读取小端序 16 位寄存器值。
   * @param reg 起始寄存器地址。
   * @param value 用于接收寄存器值。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool ReadLittleEndian16(Register reg, uint16_t& value);

  /**
   * @brief 写入小端序 16 位寄存器值。
   * @param reg 起始寄存器地址。
   * @param value 待写入寄存器值。
   * @return 写入成功返回 true，失败返回 false。
   */
  bool WriteLittleEndian16(Register reg, uint16_t value);

  /**
   * @brief 选通 ADC 后读取电流值。
   * @param input ADC 输入类型。
   * @param current_ma 用于接收电流，单位 mA。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool ReadSelectedCurrent(AdcInput input, float& current_ma);

  /**
   * @brief 读取并补偿 TS 引脚电压。
   * @param voltage_mv 用于接收电压，单位 mV。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool ReadCompensatedTs(float& voltage_mv);

  /**
   * @brief 重启 Gauge BROM。
   * @return 重启成功返回 true，失败返回 false。
   */
  bool RestartBrom();

  /**
   * @brief 将 ADC 原始电流值转换为带符号电流。
   * @param raw ADC 原始值。
   * @return 转换后的电流，单位 mA。
   */
  static float DecodeCurrent(uint16_t raw);

  /**
   * @brief 根据 NTC 表插值计算电池温度。
   * @param config NTC 配置。
   * @param voltage_mv TS 电压，单位 mV。
   * @return 计算得到的温度，单位摄氏度。
   */
  static float ConvertNtc(const NtcConfig& config, float voltage_mv);

  /**
   * @brief 解码 CC 引脚状态。
   * @param raw CC 原始编码。
   * @param sink 是否处于 Sink 角色。
   * @return 解码后的 CC 状态。
   */
  static CcState DecodeCc(uint8_t raw, bool sink);

  /**
   * @brief 判断 CC 状态是否为 Rp。
   * @param state CC 状态。
   * @return 是 Rp 返回 true，否则返回 false。
   */
  static bool IsRp(CcState state);

  ChargeProfile charge_profile_{};
  NtcConfig ntc_config_{};
  bool ntc_configured_ = false;
  bool self_powered_ = true;
};

}  // namespace cpp_bus_driver
