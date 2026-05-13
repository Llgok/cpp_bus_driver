/*
 * @Description: BQ27220 single-cell CEDV fuel gauge driver
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-05-12 18:35:00
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Bq27220 final : public ChipI2cGuide {
 public:
  enum class TemperatureMode {
    kInternal,
    kExternalNtc,
  };

  enum class SecurityMode {
    kFullAccess = 1,
    kUnsealed = 2,
    kSealed = 3,
    kUnknown = 0,
  };

  enum class ControlSubcommand : uint16_t {
    kControlStatus = 0x0000,
    kDeviceNumber = 0x0001,
    kFirmwareVersion = 0x0002,
    kHardwareVersion = 0x0003,
    kBoardOffset = 0x0009,
    kCcOffset = 0x000A,
    kCcOffsetSave = 0x000B,
    kOcvCommand = 0x000C,
    kBatteryInsert = 0x000D,
    kBatteryRemove = 0x000E,
    kSetSnooze = 0x0013,
    kClearSnooze = 0x0014,
    kSetProfile1 = 0x0015,
    kSetProfile2 = 0x0016,
    kSetProfile3 = 0x0017,
    kSetProfile4 = 0x0018,
    kSetProfile5 = 0x0019,
    kSetProfile6 = 0x001A,
    kCalibrationToggle = 0x002D,
    kSeal = 0x0030,
    kReset = 0x0041,
    kExitCalibration = 0x0080,
    kEnterCalibration = 0x0081,
    kEnterConfigUpdate = 0x0090,
    kExitConfigUpdateReinit = 0x0091,
    kExitConfigUpdate = 0x0092,
    kReturnToRom = 0x0F00,
  };

  enum class DataMemoryAddress : uint16_t {
    kOperationConfigA = 0x9206,
    kOperationConfigB = 0x9208,
    kSocDelta = 0x920B,
    kIoConfig = 0x920D,
    kInitialDischargeSet = 0x920E,
    kInitialChargeSet = 0x9210,
    kSleepCurrent = 0x9217,
    kDischargeDetectionThreshold = 0x9228,
    kChargeDetectionThreshold = 0x922A,
    kQuitCurrent = 0x922C,
    kBatteryLowPercent = 0x9251,
    kOverloadCurrent = 0x9264,
    kSelfDischargeRate = 0x9268,
    kNearFull = 0x926B,
    kReserveCapacity = 0x926D,
    kCycleCountPercentage = 0x927D,
    kBatteryId = 0x929A,
    kGaugingConfiguration = 0x929B,
    kFullChargeCapacity = 0x929D,
    kDesignCapacity = 0x929F,
    kDesignVoltage = 0x92A3,
    kChargeTerminationVoltage = 0x92A5,
    kEmf = 0x92A7,
    kC0 = 0x92A9,
    kR0 = 0x92AB,
    kT0 = 0x92AD,
    kR1 = 0x92AF,
    kTc = 0x92B1,
    kC1 = 0x92B2,
    kFixedEdv0 = 0x92B4,
    kFixedEdv1 = 0x92B7,
    kFixedEdv2 = 0x92BA,
    kVoltageDod0 = 0x92BD,
    kVoltageDod10 = 0x92BF,
    kVoltageDod20 = 0x92C1,
    kVoltageDod30 = 0x92C3,
    kVoltageDod40 = 0x92C5,
    kVoltageDod50 = 0x92C7,
    kVoltageDod60 = 0x92C9,
    kVoltageDod70 = 0x92CB,
    kVoltageDod80 = 0x92CD,
    kVoltageDod90 = 0x92CF,
    kVoltageDod100 = 0x92D1,
  };

  struct BatteryStatus {
    struct {
      bool discharging = false;                    // 放电状态，DSG
      bool system_down = false;                    // 系统下电状态，SYSDWN
      bool terminate_discharge_alarm = false;      // 终止放电告警，TDA
      bool battery_present = false;                // 电池存在状态，BATTPRES
      bool authentication_good = false;            // 认证通过状态，AUTH_GD
      bool open_circuit_voltage_good = false;      // 开路电压有效状态，OCVGD
      bool terminate_charge_alarm = false;         // 终止充电告警，TCA
      bool charge_inhibit = false;                 // 禁止充电状态，CHGINH
      bool full_charged = false;                   // 满充状态，FC
      bool over_temperature_discharge = false;     // 放电过温状态，OTD
      bool over_temperature_charge = false;        // 充电过温状态，OTC
      bool sleep_mode = false;                     // 睡眠模式状态，SLEEP
      bool open_circuit_voltage_failed = false;    // 开路电压检测失败，OCVFAIL
      bool open_circuit_voltage_complete = false;  // 开路电压检测完成，OCVCOMP
      bool full_discharged = false;                // 满放状态，FD
    } flag;
  };

  struct OperationStatus {
    uint8_t security_mode_bits = 0;  // 安全模式原始位，SEC
    SecurityMode security = SecurityMode::kUnknown;  // 解析后的安全模式
    struct {
      bool calibration_mode = false;               // 校准模式状态，CALMD
      bool edv2_reached = false;                   // EDV2 阈值到达状态，EDV2
      bool valid_discharge_qualified = false;      // 有效放电合格状态，VDQ
      bool initialization_complete = false;        // 初始化完成状态，INITCOMP
      bool smoothing_active = false;               // 平滑算法激活状态，SMTH
      bool battery_trip_point_interrupt = false;   // 电池阈值中断状态，BTP_INT
      bool config_update_mode = false;             // 配置更新模式状态，CFGUPDATE
    } flag;
  };

  struct GaugingConfig {
    uint16_t raw_value = 0x0D11;  // Gauging Configuration 原始配置值
  };

  struct CedvProfile {
    uint16_t full_charge_capacity = 650;  // 满充容量，mAh
    uint16_t design_capacity = 650;       // 设计容量，mAh
    uint16_t reserve_capacity = 0;        // 保留容量，mAh
    uint16_t near_full = 200;             // 接近满充判断阈值
    uint8_t self_discharge_rate = 20;     // 自放电率
    uint16_t edv0 = 3490;                 // 固定 EDV0 电压，mV
    uint16_t edv1 = 3511;                 // 固定 EDV1 电压，mV
    uint16_t edv2 = 3535;                 // 固定 EDV2 电压，mV
    uint16_t emf = 3670;                  // CEDV EMF 模型参数
    uint16_t c0 = 115;                    // CEDV C0 模型参数
    uint16_t r0 = 968;                    // CEDV R0 模型参数
    uint16_t t0 = 4547;                   // CEDV T0 模型参数
    uint16_t r1 = 4764;                   // CEDV R1 模型参数
    uint8_t tc = 11;                      // CEDV TC 模型参数
    uint8_t c1 = 0;                       // CEDV C1 模型参数
    uint16_t dod0 = 4147;                 // DOD 0% 对应电压，mV
    uint16_t dod10 = 4002;                // DOD 10% 对应电压，mV
    uint16_t dod20 = 3969;                // DOD 20% 对应电压，mV
    uint16_t dod30 = 3938;                // DOD 30% 对应电压，mV
    uint16_t dod40 = 3880;                // DOD 40% 对应电压，mV
    uint16_t dod50 = 3824;                // DOD 50% 对应电压，mV
    uint16_t dod60 = 3794;                // DOD 60% 对应电压，mV
    uint16_t dod70 = 3753;                // DOD 70% 对应电压，mV
    uint16_t dod80 = 3677;                // DOD 80% 对应电压，mV
    uint16_t dod90 = 3574;                // DOD 90% 对应电压，mV
    uint16_t dod100 = 3490;               // DOD 100% 对应电压，mV
  };

  /**
   * @brief 构造 BQ27220 电量计驱动对象
   * @param bus I2C 总线对象
   * @param address BQ27220 7bit I2C 地址
   * @param rst 可选复位引脚，不使用时保持 CPP_BUS_DRIVER_DEFAULT_VALUE
   * @Date 2026-05-12 18:35:00
   */
  explicit Bq27220(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  /**
   * @brief 初始化 BQ27220 并校验 Device ID
   * @param freq_hz I2C 工作频率，默认使用总线默认配置
   * @return 初始化成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  /**
   * @brief 反初始化 BQ27220 驱动
   * @param delete_bus 是否同时删除底层 I2C 总线
   * @return 反初始化成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool Deinit(bool delete_bus = false) override;

  /**
   * @brief 读取芯片 Device ID
   * @return Device ID，BQ27220 正常应为 0x0220
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetDeviceId();

  /**
   * @brief 读取芯片固件版本
   * @return Firmware version 原始值
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetFirmwareVersion();

  /**
   * @brief 读取芯片硬件版本
   * @return Hardware version 原始值
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetHardwareVersion();

  /**
   * @brief 读取设计容量
   * @return 设计容量(mAh)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetDesignCapacity();

  /**
   * @brief 读取电池端电压
   * @return 电压(mV)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetVoltage();

  /**
   * @brief 读取瞬时电流
   * @return 电流(mA)，放电通常为正值
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetCurrent();

  /**
   * @brief 读取平均电流
   * @return 平均电流(mA)，放电通常为正值
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetAverageCurrent();

  /**
   * @brief 读取剩余容量
   * @return 剩余容量(mAh)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetRemainingCapacity();

  /**
   * @brief 读取满充容量估算值
   * @return 满充容量(mAh)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetFullChargeCapacity();

  /**
   * @brief 读取 AtRate 电流
   * @return AtRate 电流(mA)
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetAtRate();

  /**
   * @brief 设置 AtRate 电流用于 AtRateTimeToEmpty 估算
   * @param rate AtRate 电流(mA)
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetAtRate(int16_t rate);

  /**
   * @brief 读取按 AtRate 估算的剩余时间
   * @return 剩余时间(min)，0xFFFF 表示不可用
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetAtRateTimeToEmpty();

  /**
   * @brief 读取电池温度原始值
   * @return 温度原始值(0.1K)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetTemperatureRaw();

  /**
   * @brief 读取电池温度
   * @return 温度(K)
   * @Date 2026-05-12 18:35:00
   */
  float GetTemperatureKelvin();

  /**
   * @brief 读取电池温度
   * @return 温度(摄氏度)
   * @Date 2026-05-12 18:35:00
   */
  float GetTemperatureCelsius();

  /**
   * @brief 设置温度采样模式
   * @param mode 使用芯片内部温度或外部 NTC
   * @return 设置成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetTemperatureMode(TemperatureMode mode);

  /**
   * @brief 读取并解析 BatteryStatus 标志位
   * @param status 输出 BatteryStatus 解析结果
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool GetBatteryStatus(BatteryStatus& status);

  /**
   * @brief 读取并解析 OperationStatus 标志位
   * @param status 输出 OperationStatus 解析结果
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool GetOperationStatus(OperationStatus& status);

  /**
   * @brief 写入设计容量到 Data Memory
   * @param capacity 设计容量(mAh)
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetDesignCapacity(uint16_t capacity);

  /**
   * @brief 读取预计放空时间
   * @return 放空时间(min)，0xFFFF 表示不可用
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetTimeToEmpty();

  /**
   * @brief 读取预计充满时间
   * @return 充满时间(min)，0xFFFF 表示不可用
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetTimeToFull();

  /**
   * @brief 读取待机电流估算值
   * @return 待机电流(mA)
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetStandbyCurrent();

  /**
   * @brief 读取按待机电流估算的放空时间
   * @return 放空时间(min)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetStandbyTimeToEmpty();

  /**
   * @brief 读取最大负载电流估算值
   * @return 最大负载电流(mA)
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetMaxLoadCurrent();

  /**
   * @brief 读取按最大负载电流估算的放空时间
   * @return 放空时间(min)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetMaxLoadTimeToEmpty();

  /**
   * @brief 读取库仑计原始累计值
   * @return 原始累计值，有符号 16bit
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetRawCoulombCount();

  /**
   * @brief 读取平均功率
   * @return 平均功率(mW)
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetAveragePower();

  /**
   * @brief 读取芯片内部温度原始值
   * @return 温度原始值(0.1K)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetChipTemperatureRaw();

  /**
   * @brief 读取芯片内部温度
   * @return 温度(K)
   * @Date 2026-05-12 18:35:00
   */
  float GetChipTemperatureKelvin();

  /**
   * @brief 读取芯片内部温度
   * @return 温度(摄氏度)
   * @Date 2026-05-12 18:35:00
   */
  float GetChipTemperatureCelsius();

  /**
   * @brief 读取循环次数
   * @return CycleCount 原始值
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetCycleCount();

  /**
   * @brief 读取电量百分比
   * @return SOC(%)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetStatusOfCharge();

  /**
   * @brief 读取健康度
   * @return SOH(%)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetStatusOfHealth();

  /**
   * @brief 读取充电电压请求值
   * @return 充电电压(mV)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetChargingVoltage();

  /**
   * @brief 读取充电电流请求值
   * @return 充电电流(mA)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetChargingCurrent();

  /**
   * @brief 设置放电方向 BTP 阈值
   * @param threshold_mah 阈值容量(mAh)
   * @return 设置成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetBtpDischargeThreshold(uint16_t threshold_mah);

  /**
   * @brief 设置充电方向 BTP 阈值
   * @param threshold_mah 阈值容量(mAh)
   * @return 设置成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetBtpChargeThreshold(uint16_t threshold_mah);

  /**
   * @brief 读取 ADC 计数原始值
   * @return ADC count 原始值
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetAnalogCount();

  /**
   * @brief 读取原始电流
   * @return 原始电流(mA)
   * @Date 2026-05-12 18:35:00
   */
  int16_t GetRawCurrent();

  /**
   * @brief 读取原始电压
   * @return 原始电压(mV)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetRawVoltage();

  /**
   * @brief 读取内部温度原始命令值
   * @return 温度原始值(0.1K)
   * @Date 2026-05-12 18:35:00
   */
  uint16_t GetRawInternalTemperature();

  /**
   * @brief 设置进入 Sleep 的电流阈值
   * @param threshold 电流阈值(mA)，芯片支持范围通常为 0-100
   * @return 设置成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetSleepCurrentThreshold(uint16_t threshold);

  /**
   * @brief 发送 Control() 子命令
   * @param subcommand Control 子命令
   * @return 发送成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SendControlSubcommand(ControlSubcommand subcommand);

  /**
   * @brief 发送 Control() 子命令并读取返回值
   * @param subcommand Control 子命令
   * @param value 输出返回值
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadControlSubcommand(ControlSubcommand subcommand, uint16_t* value);

  /**
   * @brief 封存芯片，禁止普通 Data Memory 写入
   * @return 封存成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool Seal();

  /**
   * @brief 解封芯片到 Unsealed 模式
   * @return 解封成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool Unseal();

  /**
   * @brief 进入 Full Access 模式
   * @return 进入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool FullAccess();

  /**
   * @brief 软复位 BQ27220
   * @return 复位后 Device ID 校验成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool Reset();

  /**
   * @brief 设置电池插入或移除状态
   * @param inserted true 表示电池插入，false 表示电池移除
   * @return 设置成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetBatteryInserted(bool inserted);

  /**
   * @brief 选择芯片内部 Profile 编号
   * @param profile Profile 编号，有效范围 1-6
   * @return 设置成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool SetBatteryProfile(uint8_t profile);

  /**
   * @brief 进入校准模式
   * @return 进入命令发送成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool EnterCalibration();

  /**
   * @brief 退出校准模式
   * @return 退出命令发送成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ExitCalibration();

  /**
   * @brief 切换校准模式
   * @return 命令发送成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ToggleCalibration();

  /**
   * @brief 读取 16bit Data Memory 数据
   * @param address Data Memory 地址枚举
   * @param value 输出数据
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadDataMemory(DataMemoryAddress address, uint16_t* value);

  /**
   * @brief 读取 8bit Data Memory 数据
   * @param address Data Memory 地址枚举
   * @param value 输出数据
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadDataMemory(DataMemoryAddress address, uint8_t* value);

  /**
   * @brief 读取 16bit Data Memory 数据
   * @param address Data Memory 绝对地址
   * @param value 输出数据
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadDataMemory(uint16_t address, uint16_t* value);

  /**
   * @brief 读取 8bit Data Memory 数据
   * @param address Data Memory 绝对地址
   * @param value 输出数据
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadDataMemory(uint16_t address, uint8_t* value);

  /**
   * @brief 写入 16bit Data Memory 数据
   * @param address Data Memory 地址枚举
   * @param value 写入数据
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool WriteDataMemory(DataMemoryAddress address, uint16_t value);

  /**
   * @brief 写入 8bit Data Memory 数据
   * @param address Data Memory 地址枚举
   * @param value 写入数据
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool WriteDataMemory(DataMemoryAddress address, uint8_t value);

  /**
   * @brief 写入 16bit Data Memory 数据
   * @param address Data Memory 绝对地址
   * @param value 写入数据
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool WriteDataMemory(uint16_t address, uint16_t value);

  /**
   * @brief 写入 8bit Data Memory 数据
   * @param address Data Memory 绝对地址
   * @param value 写入数据
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool WriteDataMemory(uint16_t address, uint8_t value);

  /**
   * @brief 写入完整 CEDV 电池参数和 Gauging 配置
   * @param profile CEDV 电池模型参数
   * @param config Gauging Configuration 配置
   * @return 写入并退出 Config Update 成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ApplyBatteryProfile(
      const CedvProfile& profile, const GaugingConfig& config);

  /**
   * @brief 仅在芯片参数与目标参数不一致时写入 CEDV 电池参数
   * @param profile CEDV 电池模型参数
   * @param config Gauging Configuration 配置
   * @return 参数已匹配或写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ApplyBatteryProfileIfNeeded(
      const CedvProfile& profile, const GaugingConfig& config);

 private:
  enum class Cmd : uint8_t {
    kControl = 0x00,
    kAtRate = 0x02,
    kAtRateTimeToEmpty = 0x04,
    kTemperature = 0x06,
    kVoltage = 0x08,
    kBatteryStatus = 0x0A,
    kCurrent = 0x0C,
    kRemainingCapacity = 0x10,
    kFullChargeCapacity = 0x12,
    kAverageCurrent = 0x14,
    kTimeToEmpty = 0x16,
    kTimeToFull = 0x18,
    kStandbyCurrent = 0x1A,
    kStandbyTimeToEmpty = 0x1C,
    kMaxLoadCurrent = 0x1E,
    kMaxLoadTimeToEmpty = 0x20,
    kRawCoulombCount = 0x22,
    kAveragePower = 0x24,
    kInternalTemperature = 0x28,
    kCycleCount = 0x2A,
    kStatusOfCharge = 0x2C,
    kStatusOfHealth = 0x2E,
    kChargingVoltage = 0x30,
    kChargingCurrent = 0x32,
    kBtpDischargeSet = 0x34,
    kBtpChargeSet = 0x36,
    kOperationStatus = 0x3A,
    kDesignCapacity = 0x3C,
    kSelectSubclass = 0x3E,
    kMacData = 0x40,
    kMacDataSum = 0x60,
    kMacDataLen = 0x61,
    kAnalogCount = 0x79,
    kRawCurrent = 0x7A,
    kRawVoltage = 0x7C,
    kRawInternalTemperature = 0x7E,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x55;
  static constexpr uint16_t kDeviceId = 0x0220;
  static constexpr uint16_t kUnsealKey1 = 0x0414;
  static constexpr uint16_t kUnsealKey2 = 0x3672;
  static constexpr uint16_t kFullAccessKey = 0xFFFF;

  /**
   * @brief 进入 Config Update 模式
   * @return 进入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool EnterConfigUpdate();

  /**
   * @brief 退出 Config Update 模式
   * @param reinit true 表示退出后重新初始化 gauge
   * @return 退出成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ExitConfigUpdate(bool reinit = true);

  /**
   * @brief 等待 Config Update 标志进入指定状态
   * @param enabled 期望的 Config Update 状态
   * @param timeout_ms 超时时间(ms)
   * @return 等到目标状态返回 true，超时或读取失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool WaitConfigUpdate(bool enabled, uint32_t timeout_ms = 2000);

  /**
   * @brief 读取标准命令 16bit 无符号值
   * @param cmd 标准命令地址
   * @param value 输出数据
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadU16(Cmd cmd, uint16_t* value);

  /**
   * @brief 按小端顺序写入标准命令 16bit 值
   * @param cmd 标准命令地址
   * @param value 写入数据
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool WriteU16(Cmd cmd, uint16_t value);

  /**
   * @brief 读取标准命令 16bit 有符号值
   * @param cmd 标准命令地址
   * @param value 输出数据
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadS16(Cmd cmd, int16_t* value);

  /**
   * @brief 写入 Data Memory 字节块并自动计算 checksum
   * @param address Data Memory 绝对地址
   * @param data 写入数据指针
   * @param length 写入长度，最大 32 字节
   * @return 写入成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool WriteDataMemoryBytes(
      uint16_t address, const uint8_t* data, size_t length);

  /**
   * @brief 读取 Data Memory 字节块
   * @param address Data Memory 绝对地址
   * @param data 输出数据指针
   * @param length 读取长度，最大 32 字节
   * @return 读取成功返回 true，失败返回 false
   * @Date 2026-05-12 18:35:00
   */
  bool ReadDataMemoryBytes(uint16_t address, uint8_t* data, size_t length);

  /**
   * @brief 计算 BQ27220 MACData checksum
   * @param data 数据指针
   * @param length 数据长度
   * @return checksum 值
   * @Date 2026-05-12 18:35:00
   */
  uint8_t CalcChecksum(const uint8_t* data, size_t length);

  int32_t rst_;
};
}  // namespace cpp_bus_driver
