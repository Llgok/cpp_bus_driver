
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-28 15:50:06
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Bq27220xxxx final : public ChipI2cGuide {
 public:
  enum class TemperatureMode {
    kInternal,
    kExternalNtc,
  };

  struct BatteryStatus {
    struct {
      // 检测到完全放电，该标志根据选择的 Soc Flag Config B 选项进行设置和清除
      bool fd = false;
      // Ocv 测量更新已完成，设置时为真
      bool ocvcomp = false;
      // 指示 Ocv 读取因电流而失败的状态位，该位只能在接收到 OCV_CMD()
      // 后在电池存在的情况下进行设置，设置时为真
      bool ocvfail = false;
      // 设置时器件在 Sleep 模式下运行，该位将在 Sleep 模式下的 AD
      // 测量期间暂时清除
      bool sleep = false;
      //  检测到充电条件下的过热，如果 Operation Config B [IntOt]  位 = 1，则
      //  SocInt 引脚会在 [Otc] 位被设置时切换一次
      bool otc = false;
      // 检测到放电条件下的过热，设置时为真，如果 Operation Config B [IntOt] 位
      // = 1，则 SocInt 引脚会在 [Otd] 位被设置时切换一次
      bool otd = false;
      // 检测到充满电，该标志根据选择的 Soc Flag Config A 和 Soc Flag Config B
      // 选项进行设置和清除
      bool fc = false;
      // 充电禁止：如果设置，则表示不应开始充电，因为 Temperature() 超出范围
      // [Charge Inhibit Temp Low, Charge Inhibit Temp High]，设置时为真
      bool chginh = false;
      // 终止充电警报，该标志根据选择的 Soc Flag Config A 选项进行设置和清除
      bool tca = false;
      // 进行了良好的 Ocv 测量，设置时为真
      bool ocvgd = false;
      // 检测插入的电池，设置时为真
      bool auth_gd = false;
      // 检测到电池存在，设置时为真
      bool battpres = false;
      // 终止放电警报，该标志根据选择的 Soc Flag Config A 选项进行设置和清除
      bool tda = false;
      // 指示系统应关闭的系统关闭位，设置时为真，如果设置，SocInt
      // 引脚会切换一次
      bool sysdwn = false;
      // 设置时，器件处于 Discharge 模式；清除时，器件处于 Charging 或
      // Relaxation 模式
      bool dsg = false;

    } flag;
  };

  struct OperationStatus {
    // 定义当前安全访问
    uint8_t sec = 0;
    struct {
      // 电量监测计处于 Config Update 模式，电量监测暂停
      bool config_update = false;
      // 指示已超过 Btp 阈值的标志
      bool btp_int = false;
      // 指示 RemainingCapacity() 累积当前正在由平滑处理引擎进行调节
      bool smth = false;
      // 指示电量监测计初始化是否完成。该位只能在电池存在时被设置。设置时为真
      bool init_comp = false;
      // 指示当前的放电周期是符合还是不符合 Fcc 更新的要求。会设置对 Fcc
      // 更新有效的放电周期
      bool vdq = false;
      // 指示测量的电池电压是高于还是低于 Edv2 阈值。设置时表示低于
      bool edv2 = false;
      // 使用 0x2D 命令进行切换，以启用 / 禁用 Calibration 模式
      bool calmd = false;
    } flag;
  };

  explicit Bq27220xxxx(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  uint16_t GetDeviceId();

  /**
   * @brief 获取设置的电池容量，该值在电池的插拔过程中恢复初始状态
   * @return
   * @Date 2025-02-26 10:39:26
   */
  uint16_t GetDesignCapacity();

  /**
   * @brief 获取电池包电压，以 mV 为单位，范围为 0 至 6000mV
   * @return
   * @Date 2025-02-26 16:04:44
   */
  uint16_t GetVoltage();

  /**
   * @brief 获取流过感测电阻的瞬时电流，它每秒更新一次，单位为 mA
   * @return
   * @Date 2025-02-26 16:05:21
   */
  int16_t GetCurrent();

  /**
   * @brief 获取电池剩余容量，当 Cedv Smoothing Config [Smen]
   * 被设置时，这将是平滑处理引擎的结果，否则，会返回未过滤的剩余容量，单位为
   * mAh
   * @return
   * @Date 2025-02-26 16:06:35
   */
  uint16_t GetRemainingCapacity();

  /**
   * @brief 获取充满电的电池的补偿容量，单位为 mAh，FullChargeCapacity() 按照
   * Cedv 算法的规定定期更新（需要电池充放电才更新）
   * @return
   * @Date 2025-02-26 16:07:12
   */
  uint16_t GetFullChargeCapacity();

  /**
   * @brief 获取放电速率，默认值是 0，AtRate 的值会被 AtRateTimeToEmpty()
   * 函数使用，用于计算剩余运行时间， 如果 AtRate 设置为 0，AtRateTimeToEmpty()
   * 将返回 65,535，表示无法预测剩余时间（或者剩余时间非常长） 只能在 Normal
   * 模式下使用
   * @return 放电或者充电电流值，负号代表放电
   * @Date 2025-02-26 10:53:31
   */
  int16_t GetAtRate();

  /**
   * @brief 设置放电速率，默认值是 0，AtRate 的值会被 AtRateTimeToEmpty()
   * 函数使用，用于计算剩余运行时间， 如果 AtRate 设置为 0，AtRateTimeToEmpty()
   * 将返回 65,535，表示无法预测剩余时间（或者剩余时间非常长） 只能在 Normal
   * 模式下使用
   * @param rate 放电或者充电电流值，负号代表放电
   * @return
   * @Date 2025-02-26 10:55:46
   */
  bool SetAtRate(int16_t rate);

  /**
   * @brief
   * 在set_at_rate()函数设置的放电电流速度的条件下，预测的剩余电池还能工作的时间（以分钟为单位），在系统设置
   * AtRate() 值后， AtRateTimeToEmpty() 会在 1 秒内更新，此外，燃料计（fuel
   * gauge）会每秒自动更新 AtRateTimeToEmpty()，基于当前的 AtRate()
   * 值和电池状态， 只能在 Normal 模式下使用，值 65535 表示 AtRate() = 0
   * @return 值的范围是 0 到 65534 分钟
   * @Date 2025-02-26 10:56:42
   */
  uint16_t GetAtRateTimeToEmpty();

  /**
   * @brief 获取温度，可以获取芯片内部温度或者外部NTC温度，由Operation
   * Config中的[Wrtemp]和[Temps]值决定，默认读取芯片内部温度
   * @return 电量监测计测量的温度的浮点数，以°K为单位
   * @Date 2025-02-26 11:35:54
   */
  float GetTemperatureKelvin();

  /**
   * @brief 获取温度，可以获取芯片内部温度或者外部NTC温度，由Operation
   * Config中的[Wrtemp]和[Temps]值决定，默认读取芯片内部温度
   * @return 电量监测计测量的温度的浮点数，以°C为单位
   * @Date 2025-02-26 11:35:54
   */
  float GetTemperatureCelsius();

  /**
   * @brief 设置获取温度的模式，从内部获取或外部NTC获取
   * @param mode 使用 TemperatureMode::配置
   * @return
   * @Date 2025-02-26 15:35:10
   */
  bool SetTemperatureMode(TemperatureMode mode);

  /**
   * @brief 获取电池状态
   * @param &status 使用 BatteryStatus::获取
   * @return
   * @Date 2025-02-26 16:03:21
   */
  bool GetBatteryStatus(BatteryStatus& status);

  /**
   * @brief 获取操作状态
   * @param &status 使用 OperationStatus::获取
   * @return
   * @Date 2025-02-26 16:03:49
   */
  bool GetOperationStatus(OperationStatus& status);

  /**
   * @brief 设置电池容量
   * @param capacity 电池容量
   * @return
   * @Date 2025-02-26 10:39:54
   */
  bool SetDesignCapacity(uint16_t capacity);

  /**
   * @brief 获取当前放电率下预测的剩余电池还能工作的时间（以分钟为单位），值
   * 65535 表示电池未在放电
   * @return 值的范围是 0 到 65534 分钟
   * @Date 2025-02-26 16:22:42
   */
  uint16_t GetTimeToEmpty();

  /**
   * @brief 获取当前电池充电的所需时间，根据 AverageCurrent()
   * 预测电池达到充满电状态的剩余时间（以分钟为单位）， 该计算考虑了基于固定
   * AverageCurrent() 电荷累积速率的线性 Ttf 计算的收尾电流时间扩展，值 65535
   * 表示电池未在充电
   * @return 值的范围是 0 到 65534 分钟
   * @Date 2025-02-26 16:22:42
   */
  uint16_t GetTimeToFull();

  /**
   * @brief 获取通过检测电阻的待机电流，StandbyCurrent()
   * 是自适应测量值，最初它会报告在 Initial Standby 中编程的待机电流，
   * 在待机模式下经过几秒钟后会报告测得的待机电流，当测量的电流高于 Deadband
   * 且小于或等于 2 × Initial Standby 时，寄存器值每秒更新一次，
   * 符合这些标准的第一个值和最后一个值不会包含在内，因为它们可能不是稳定的值。为了接近
   * 1 分钟的时间常数， 每个新的 StandbyCurrent() 值通过取最后一个待机电流的大约
   * 93% 的权重和当前测量的平均电流的大约 7% 来计算
   * @return
   * @Date 2025-02-26 16:44:07
   */
  int16_t GetStandbyCurrent();

  /**
   * @brief
   * 获取待机放电率下预测的剩余电池还能工作的时间（以分钟为单位），该计算使用
   * NominalAvailableCapacity() (Nac)（未经补偿的剩余容量） 值 65535
   * 表示电池未在放电
   * @return
   * @Date 2025-02-26 16:48:14
   */
  uint16_t GetStandbyTimeToEmpty();

  /**
   * @brief 获取最大负载情况下的电流，以 mA 为单位，MaxLoadCurrent()
   * 是自适应测量值，最初报告为在 Initial Max Load Current
   * 中编程的最大负载电流， 如果测量的电流始终大于 Initial Max Load
   * Current，则只要在前一次放电至 Soc 低于 50% 后电池充满电，Max Load Current
   * () 就会减小至前一个值和 Initial Max Load Current 的平均值，
   * 这可以防止报告的值保持在异常高的水平
   * @return
   * @Date 2025-02-26 16:56:09
   */
  int16_t GetMaxLoadCurrent();

  /**
   * @brief
   * 获取最大负载电流放电率下预测的剩余电池还能工作的时间（以分钟为单位），值
   * 65535 表示电池未在放电
   * @return
   * @Date 2025-02-26 16:59:30
   */
  uint16_t GetMaxLoadTimeToEmpty();

  /**
   * @brief
   * 获取在充电/放电期间从电池中转移的库仑量，计数器在放电期间递增，在充电期间递减，充电期间，当
   * FC 位被设置（表示充满电）时，计数器清零， IgnoreSd
   * 位提供忽略自放电的功能，IgnoreSd =
   * 0（默认值）（常规放电或自放电期间库仑计递增），IgnoreSd =
   * 1（库仑计仅在真正放电时才递增）
   * @return
   * @Date 2025-02-26 17:08:21
   */
  uint16_t GetRawCoulombCount();

  /**
   * @brief
   * 该值表示电池充电和放电期间的平均功率，放电期间值为负，充电期间值为正，值 0
   * 表示电池未在放电，报告该值时采用的单位为 mW
   * @return
   * @Date 2025-02-26 17:12:11
   */
  int16_t GetAveragePower();

  /**
   * @brief 获取芯片温度，获取电量监测计测量的内部温度传感器的浮点数值
   * @return 电量监测计测量的温度的浮点数，以°K为单位
   * @return
   * @Date 2025-02-26 15:56:06
   */
  float GetChipTemperatureKelvin();

  /**
   * @brief 获取芯片温度，获取电量监测计测量的内部温度传感器的浮点数值
   * @return 电量监测计测量的温度的浮点数，以°C为单位
   * @return
   * @Date 2025-02-26 15:56:06
   */
  float GetChipTemperatureCelsius();

  /**
   * @brief 获取活动电池已经历的周期数，其范围为 0 至 65535，当累积放电 ≥
   * 周期阈值时，会产生一个周期， 周期阈值的计算方法为 Cycle Count Percentage
   * 乘以 FullChargeCapacity()（当CEDV Gauging Configuration [Cct] = 1 时）或
   * DesignCapacity()（当 [Cct] = 0 时）
   * @return
   * @Date 2025-02-26 17:18:35
   */
  uint16_t GetCycleCount();

  /**
   * @brief 获取预测的剩余电池电量百分比，实际为RemainingCapacity() 占
   * FullChargeCapacity() 的百分比，范围为 0 至 100%， StatusOfCharge() =
   * RemainingCapacity() ÷ FullChargeCapacity()，四舍五入至最接近的整数百分点
   * @return
   * @Date 2025-02-26 17:23:27
   */
  uint16_t GetStatusOfCharge();

  /**
   * @brief 获取预测的电池健康百分比，实际为FullChargeCapacity() 占
   * DesignCapacity() 的百分比，范围为 0 至 100%， StatusOfHealth() =
   * FullChargeCapacity() ÷ DesignCapacity()，四舍五入至最接近的整数百分点
   * @return
   * @Date 2025-02-26 17:23:42
   */
  uint16_t GetStatusOfHealth();

  /**
   * @brief 获取电池充电电压，值 65535 表示电池请求电池充电器提供最大电压
   * @return
   * @Date 2025-02-26 17:34:03
   */
  uint16_t GetChargingVoltage();

  /**
   * @brief 获取电池充电电流，值 65535 表示电池请求电池充电器提供最大电流
   * @return
   * @Date 2025-02-26 17:34:52
   */
  uint16_t GetChargingCurrent();

  /**
   * @brief
   * 设置睡眠电流阈值，单位mA默认值为10mA，当设置sleep使能并且电流低于设置的这个阈值时自动进入睡眠模式
   * @param threshold （0 ~ 100mA）睡眠电流阈值
   * @return
   * @Date 2025-02-26 18:08:31
   */
  bool SetSleepCurrentThreshold(uint16_t threshold);

 private:
  enum class Cmd {
    // 读写寄存器命令
    // kWoWriteRegister = 0xAA,
    // kWoReadRegister,
    // RAM寄存器地址命令
    kRwRamRegister = 0x3E,

    kRwControlStatusStart = 0x00,

    kRwAtRateStart = 0x02,
    kRoAtRateTimeToEmptyStart = 0x04,
    kRoTemperatureStart = 0x06,
    kRoVoltageStart = 0x08,
    kRoBatteryStatusStart = 0x0A,
    kRoCurrentStart = 0x0C,
    kRoRemainingCapacityStart = 0x10,
    kRoFullChargeCapacityStart = 0x12,
    kRoTimeToEmptyStart = 0x16,
    kRoTimeToFullStart = 0x18,
    kRoStandbyCurrentStart = 0x1A,
    kRoStandbyTimeToEmptyStart = 0x1C,
    kRoMaxLoadCurrentStart = 0x1E,
    kRoMaxLoadTimeToEmptyStart = 0x20,
    kRoRawCoulombCountStart = 0x22,
    kRoAveragePowerStart = 0x24,
    kRoInternalTemperatureStart = 0x28,
    kRoCycleCountStart = 0x2A,
    kRoStatusOfChargeStart = 0x2C,
    kRoStatusOfHealthStart = 0x2E,
    kRoChargingVoltageStart = 0x30,
    kRoChargingCurrentStart = 0x32,
    kRoOperationStatusStart = 0x3A,
    kRoDesignCapacityStart = 0x3C,
    kRwMacDataStart = 0x40,
    kRoMacDataSumStart = 0x60,
    kRoMacDataLenStart,
  };

  // 访问寄存器需要通过前置读写命令来访问
  // 采用小端先发的规则发送（0x0001 先发0x01后发0x00）
  // 凡是写寄存器都需要延时10ms
  enum class ControlStatusReg {
    kRoDeviceId = 0x0001,
    kWoEnterConfigUpdate = 0x0090,
    kWoExitConfigUpdateReinit,
    kWoExitConfigUpdate,
  };

  // 凡是写寄存器都需要延时10ms
  enum class ConfigurationReg {
    kRwOperationConfigA = 0x9206,
    kRwSleepCurrent = 0x9217,
  };

  // 凡是写寄存器都需要延时10ms
  enum class GasGaugingReg {
    kWoDesignCapacity = 0x929F,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x55;
  static constexpr uint16_t kDeviceId = 0x0220;

  /**
   * @brief 进入配置更新模式
   * @return
   * @Date 2025-02-26 15:33:17
   */
  bool EnterConfigUpdate();
  /**
   * @brief 退出配置更新模式
   * @return
   * @Date 2025-02-26 15:33:17
   */
  bool ExitConfigUpdate();

  int32_t rst_;
};
}  // namespace cpp_bus_driver