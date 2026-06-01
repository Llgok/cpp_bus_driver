/*
 * @Description: AW21009
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-06-01 16:08:45
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Aw21009 final : public ChipI2cGuide {
 public:
  enum class LedChannel : uint8_t {
    kLed1 = 0,
    kLed2,
    kLed3,
    kLed4,
    kLed5,
    kLed6,
    kLed7,
    kLed8,
    kLed9,

    kAll,
  };

  enum class LedGroup : uint8_t {
    kGroup1 = 0,
    kGroup2,
    kGroup3,

    kAll,
  };

  enum class ClockFrequency : uint8_t {
    k16Mhz = 0,
    k8Mhz,
    k1Mhz,
    k512Khz,
    k256Khz,
    k125Khz,
    k62p5Khz,
    k31p25Khz,
  };

  enum class PwmResolution : uint8_t {
    k8Bit = 0,
    k9Bit,
    k12Bit,
    k12BitWithDither,
  };

  enum class OpenShortDetectMode : uint8_t {
    kDisable = 0B00,
    kShort = 0B10,
    kOpen = 0B11,
  };

  enum class OpenThreshold : uint8_t {
    k0p1V = 0,
    k0p2V,
  };

  enum class ShortThreshold : uint8_t {
    k0p5V = 0,
    k1V,
  };

  enum class ThermalRollOffCurrent : uint8_t {
    k100Percent = 0,
    k75Percent,
    k50Percent,
    k25Percent,
  };

  enum class ThermalRollOffThreshold : uint8_t {
    k140C = 0,
    k120C,
    k100C,
    k90C,
  };

  enum class SpreadSpectrumRange : uint8_t {
    k5Percent = 0,
    k15Percent,
    k24Percent,
    k34Percent,
  };

  enum class SpreadSpectrumPeriod : uint8_t {
    k1980Us = 0,
    k1200Us,
    k820Us,
    k660Us,
  };

  enum class RextStatus : uint8_t {
    kNormal = 0,
    kShortOrOcp,
    kOpen,
    kNotExist,
  };

  enum class OcpThreshold : uint8_t {
    k85Ma = 0,
    k55Ma,
  };

  enum class UpdateMode : uint8_t {
    kBrightnessAtPwmBoundary = 0,
    kBrightnessAndScalingAtPwmBoundary,
    kBrightnessFast,
    kBrightnessAndScalingFast,
  };

  enum class SlewRateRising : uint8_t {
    k1Ns = 0,
    k6Ns,
  };

  enum class SlewRateFalling : uint8_t {
    k1Ns = 0,
    k3Ns,
    k6Ns,
    k10Ns,
  };

  enum class PatternMode : uint8_t {
    kManual = 0,
    kAutoBreath,
  };

  enum class PatternTime : uint8_t {
    k0s = 0,
    k0p13s,
    k0p26s,
    k0p38s,
    k0p51s,
    k0p77s,
    k1p04s,
    k1p60s,
    k2p10s,
    k2p60s,
    k3p10s,
    k4p20s,
    k5p20s,
    k6p20s,
    k7p30s,
    k8p30s,
  };

  enum class PatternLoopStart : uint8_t {
    kT0 = 0,
    kT1,
    kT2,
    kT3,
  };

  enum class PatternLoopEnd : uint8_t {
    kT3 = 0,
    kT1,
  };

  struct ThermalStatus {
    bool thermal_roll_off = false;
    bool over_temperature = false;
    uint8_t raw = 0;
  };

  struct UvStatus {
    RextStatus rext_status = RextStatus::kNormal;
    bool uvlo = false;
    bool power_up = false;
    uint8_t raw = 0;
  };

  struct PatternStatus {
    bool loop_over = false;
    bool running = false;
    uint8_t raw = 0;
  };

  struct PatternTiming {
    PatternTime rise = PatternTime::k0s;
    PatternTime on = PatternTime::k0s;
    PatternTime fall = PatternTime::k0s;
    PatternTime off = PatternTime::k0s;
    PatternLoopStart loop_start = PatternLoopStart::kT0;
    PatternLoopEnd loop_end = PatternLoopEnd::kT3;
    uint16_t repeat = 0;
  };

  explicit Aw21009(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault,
      int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 将AW21009全部寄存器复位到默认值。
   * @return 复位命令写入成功返回true。
   */
  bool SoftwareReset();

  /**
   * @brief 从RESET/ID寄存器读取AW21009芯片ID。
   * @return 芯片ID，读取失败返回0xFF。
   */
  uint8_t GetDeviceId();

  /**
   * @brief 一次性配置GCR全局控制寄存器。
   * @param auto_power_save 是否开启自动省电模式。
   * @param clock_frequency PWM时钟频率选项。
   * @param pwm_resolution 亮度PWM分辨率。
   * @param chip_enable 是否进入芯片工作模式。
   * @return 寄存器写入成功返回true。
   */
  bool SetGlobalControl(bool auto_power_save, ClockFrequency clock_frequency,
      PwmResolution pwm_resolution, bool chip_enable);

  /**
   * @brief 开启或关闭自动省电模式。
   * @param enable true表示开启自动省电模式。
   * @return 寄存器更新成功返回true。
   */
  bool SetAutoPowerSave(bool enable);

  /**
   * @brief 设置PWM时钟频率选项。
   * @param clock_frequency 时钟频率选项。
   * @return 寄存器更新成功返回true。
   */
  bool SetClockFrequency(ClockFrequency clock_frequency);

  /**
   * @brief 设置亮度PWM分辨率。
   * @param pwm_resolution PWM分辨率选项。
   * @return 寄存器更新成功返回true。
   */
  bool SetPwmResolution(PwmResolution pwm_resolution);

  /**
   * @brief 开启或关闭AW21009工作模式。
   * @param enable true表示使能芯片。
   * @return 寄存器更新成功返回true。
   */
  bool SetChipEnable(bool enable);

  /**
   * @brief 将待更新的BR和SL寄存器值锁存到输出逻辑。
   * @return UPDATE命令写入成功返回true。
   */
  bool Update();

  /**
   * @brief 设置单个LED或全部LED的12位亮度。
   * @param channel LED通道，或LedChannel::kAll。
   * @param value 亮度值，范围0~4095。
   * @param update true表示写入亮度寄存器后自动写UPDATE。
   * @return 操作成功返回true。
   */
  bool SetBrightness(LedChannel channel, uint16_t value, bool update = true);

  /**
   * @brief 读取单个LED通道的12位亮度。
   * @param channel 单个LED通道。
   * @param value 输出亮度值。
   * @return 读取成功返回true。
   */
  bool GetBrightness(LedChannel channel, uint16_t* value);

  /**
   * @brief 单字节模式下设置8位亮度。
   * @param channel LED通道，或LedChannel::kAll。
   * @param value 8位亮度值。
   * @param update true表示写入亮度寄存器后自动写UPDATE。
   * @return 操作成功返回true。
   */
  bool SetSingleByteBrightness(
      LedChannel channel, uint8_t value, bool update = true);

  /**
   * @brief RGB模式下设置单个RGB组或全部RGB组的12位亮度。
   * @param group LED组，或LedGroup::kAll。
   * @param value 亮度值，范围0~4095。
   * @param update true表示写入亮度寄存器后自动写UPDATE。
   * @return 操作成功返回true。
   */
  bool SetRgbBrightness(LedGroup group, uint16_t value, bool update = true);

  /**
   * @brief 设置单个LED或全部LED的8位电流比例。
   * @param channel LED通道，或LedChannel::kAll。
   * @param value 电流比例值，范围0~255。
   * @param update true表示写入比例寄存器后自动写UPDATE。
   * @return 操作成功返回true。
   */
  bool SetCurrentLimit(LedChannel channel, uint8_t value, bool update = true);

  /**
   * @brief 读取单个LED通道的8位电流比例。
   * @param channel 单个LED通道。
   * @param value 输出电流比例值。
   * @return 读取成功返回true。
   */
  bool GetCurrentLimit(LedChannel channel, uint8_t* value);

  /**
   * @brief 设置全局电流控制寄存器。
   * @param value 全局电流值，范围0~255。
   * @return 寄存器写入成功返回true。
   */
  bool SetGlobalCurrentLimit(uint8_t value);

  /**
   * @brief 读取全局电流控制寄存器。
   * @param value 输出全局电流值。
   * @return 读取成功返回true。
   */
  bool GetGlobalCurrentLimit(uint8_t* value);

  /**
   * @brief 开启或关闭PWM相位延迟。
   * @param enable true表示开启相位延迟。
   * @return 寄存器更新成功返回true。
   */
  bool SetPhaseDelay(bool enable);

  /**
   * @brief 开启或关闭指定LED组的相位反转。
   * @param group LED组，或LedGroup::kAll。
   * @param enable true表示开启相位反转。
   * @return 寄存器更新成功返回true。
   */
  bool SetPhaseInvert(LedGroup group, bool enable);

  /**
   * @brief 配置LED开路或短路检测。开启检测时会按手册要求设置SSCR.DCPWM[1:0]为11；关闭检测时会清除这两个位。
   *       检测电流仍需要用户按硬件条件配置到约1mA。
   * @param mode 检测模式。
   * @param open_threshold 开路检测阈值。
   * @param short_threshold 短路检测阈值。
   * @return 寄存器更新成功返回true。
   */
  bool SetOpenShortDetection(OpenShortDetectMode mode,
      OpenThreshold open_threshold = OpenThreshold::k0p1V,
      ShortThreshold short_threshold = ShortThreshold::k0p5V);

  /**
   * @brief 读取LED开路或短路检测状态位。
   * @param status 输出LED1~LED9对应的9位状态掩码。
   * @return 读取成功返回true。
   */
  bool GetOpenShortStatus(uint16_t* status);

  /**
   * @brief 配置温度折返行为。
   * @param current 温度折返时的输出电流比例。
   * @param threshold 温度折返阈值。
   * @return 寄存器更新成功返回true。
   */
  bool SetThermalRollOff(ThermalRollOffCurrent current,
      ThermalRollOffThreshold threshold);

  /**
   * @brief 开启或关闭过温检测和过温保护。
   * @param detect_enable true表示开启过温检测。
   * @param protect_enable true表示允许过温保护清除CHIPEN。
   * @return 寄存器更新成功返回true。
   */
  bool SetOverTemperatureProtection(bool detect_enable, bool protect_enable);

  /**
   * @brief 读取温度折返和过温状态。
   * @param status 输出解析后的温度状态。
   * @return 读取成功返回true。
   */
  bool GetThermalStatus(ThermalStatus* status);

  /**
   * @brief 强制选中的LED组PWM占空比为100%。
   * @param led1_to_led6_enable true表示LED1~LED6强制满占空比。
   * @param led7_to_led9_enable true表示LED7~LED9强制满占空比。
   * @return 寄存器更新成功返回true。
   */
  bool SetPwmFullDuty(bool led1_to_led6_enable, bool led7_to_led9_enable);

  /**
   * @brief 配置PWM扩频功能。
   * @param enable true表示开启扩频。
   * @param range 扩频范围。
   * @param period 扩频周期。
   * @return 寄存器更新成功返回true。
   */
  bool SetSpreadSpectrum(bool enable, SpreadSpectrumRange range,
      SpreadSpectrumPeriod period);

  /**
   * @brief 开启或关闭欠压检测和欠压保护。
   * @param detect_enable true表示开启欠压检测。
   * @param protect_enable true表示允许欠压保护清除CHIPEN。
   * @return 寄存器更新成功返回true。
   */
  bool SetUvProtection(bool detect_enable, bool protect_enable);

  /**
   * @brief 开启或关闭过流保护。
   * @param enable true表示开启过流保护。
   * @return 寄存器更新成功返回true。
   */
  bool SetOcpProtection(bool enable);

  /**
   * @brief 设置过流保护阈值。
   * @param threshold 过流保护阈值选项。
   * @return 寄存器更新成功返回true。
   */
  bool SetOcpThreshold(OcpThreshold threshold);

  /**
   * @brief 读取欠压、上电和REXT状态。
   * @param status 输出解析后的欠压状态。
   * @return 读取成功返回true。
   */
  bool GetUvStatus(UvStatus* status);

  /**
   * @brief 开启或关闭AW21009广播I2C地址。
   * @param enable true表示开启0x1C广播地址。
   * @return 寄存器更新成功返回true。
   */
  bool SetBroadcastAddressEnable(bool enable);

  /**
   * @brief 设置BR和SL寄存器更新时序模式。
   * @param mode 更新时序模式。
   * @return 寄存器更新成功返回true。
   */
  bool SetUpdateMode(UpdateMode mode);

  /**
   * @brief 开启或关闭单字节亮度模式。
   * @param enable true表示开启单字节模式。
   * @return 寄存器更新成功返回true。
   */
  bool SetSingleByteMode(bool enable);

  /**
   * @brief 开启或关闭RGB模式。
   * @param enable true表示开启RGB模式。
   * @return 寄存器更新成功返回true。
   */
  bool SetRgbMode(bool enable);

  /**
   * @brief 开启或关闭PWMIS0省电行为。
   * @param enable true表示开启PWMIS0。
   * @return 寄存器更新成功返回true。
   */
  bool SetPowerSavePwmIs0(bool enable);

  /**
   * @brief 配置LED输出压摆率。
   * @param rising 上升沿压摆率。
   * @param falling 下降沿压摆率。
   * @return 寄存器更新成功返回true。
   */
  bool SetSlewRate(SlewRateRising rising, SlewRateFalling falling);

  /**
   * @brief 设置组亮度寄存器值。
   * @param value 组亮度值，范围0~4095。
   * @return 寄存器写入成功返回true。
   */
  bool SetGroupBrightness(uint16_t value);

  /**
   * @brief 设置RGB通道的组电流比例寄存器。
   * @param red 红色通道比例值。
   * @param green 绿色通道比例值。
   * @param blue 蓝色通道比例值。
   * @return 寄存器写入成功返回true。
   */
  bool SetGroupScaling(uint8_t red, uint8_t green, uint8_t blue);

  /**
   * @brief 直接配置组控制模式。
   * @param group_mask bit0~bit2分别选择LED组1~3。
   * @param use_individual_scaling true表示组模式下使用SL00~SL08。
   * @return 寄存器更新成功返回true。
   */
  bool SetGroupConfig(uint8_t group_mask, bool use_individual_scaling);

  /**
   * @brief 开启或关闭单个组或全部组的组控制模式。
   * @param group LED组，或LedGroup::kAll。
   * @param enable true表示开启组控制模式。
   * @return 寄存器更新成功返回true。
   */
  bool SetGroupEnable(LedGroup group, bool enable);

  /**
   * @brief 配置pattern控制器。
   * @param enable true表示开启pattern控制器。
   * @param mode 手动模式或自动呼吸模式。
   * @param switch_on 手动模式输出开关。
   * @param ramp_enable true表示开启手动模式平滑过渡。
   * @return 寄存器更新成功返回true。
   */
  bool SetPatternConfig(
      bool enable, PatternMode mode, bool switch_on, bool ramp_enable);

  /**
   * @brief 设置手动pattern输出状态。
   * @param switch_on true表示选择高电平pattern亮度。
   * @param ramp_enable true表示在低亮度和高亮度之间平滑过渡。
   * @return 寄存器更新成功返回true。
   */
  bool SetManualPatternSwitch(bool switch_on, bool ramp_enable);

  /**
   * @brief 配置自动呼吸pattern时序和重复次数。
   * @param timing pattern时序配置。
   * @return 寄存器写入成功返回true。
   */
  bool SetPatternTiming(const PatternTiming& timing);

  /**
   * @brief 设置自动呼吸pattern的高亮度和低亮度。
   * @param max_brightness 最高pattern亮度。
   * @param min_brightness 最低pattern亮度。
   * @return 寄存器写入成功返回true。
   */
  bool SetPatternBrightness(uint8_t max_brightness, uint8_t min_brightness);

  /**
   * @brief 启动pattern控制器。
   * @return RUN位写入成功返回true。
   */
  bool StartPattern();

  /**
   * @brief 停止pattern控制器。
   * @return RUN位清除成功返回true。
   */
  bool StopPattern();

  /**
   * @brief 读取pattern控制器状态。
   * @param status 输出解析后的pattern状态。
   * @return 读取成功返回true。
   */
  bool GetPatternStatus(PatternStatus* status);

  static constexpr uint8_t kDeviceI2cAddress1 = 0x20;
  static constexpr uint8_t kDeviceI2cAddress2 = 0x21;
  static constexpr uint8_t kDeviceI2cAddress3 = 0x24;
  static constexpr uint8_t kDeviceI2cAddress4 = 0x25;
  static constexpr uint8_t kDeviceI2cBroadcastAddress = 0x1C;
  static constexpr uint8_t kDeviceI2cAddressDefault = kDeviceI2cAddress1;
  static constexpr uint8_t kLedCount = 9;
  static constexpr uint16_t kBrightnessMax = 0x0FFF;

 private:
  // 寄存器地址表和仅供内部实现使用的辅助函数放在private区。
  enum class Register : uint8_t {
    kGlobalControl = 0x20,
    kBrightnessStart = 0x21,
    kUpdate = 0x45,
    kScalingStart = 0x46,
    kGlobalCurrentControl = 0x58,
    kPhaseControl = 0x59,
    kOpenShortDetectControl = 0x5A,
    kOpenShortStatus0 = 0x5B,
    kOpenShortStatus1 = 0x5C,
    kOverTemperatureControl = 0x5E,
    kSpreadSpectrumControl = 0x5F,
    kUvControl = 0x60,
    kGlobalControl2 = 0x61,
    kGlobalControl3 = 0x62,
    kReset = 0x70,
    kPatternConfig = 0x80,
    kPatternGo = 0x81,
    kPatternTime0 = 0x82,
    kPatternTime1 = 0x83,
    kPatternTime2 = 0x84,
    kPatternTime3 = 0x85,
    kGroupBrightnessHigh = 0x86,
    kGroupBrightnessLow = 0x87,
    kGroupScalingRed = 0x88,
    kGroupScalingGreen = 0x89,
    kGroupScalingBlue = 0x8A,
    kGroupConfig = 0x8B,
  };

  static constexpr uint8_t kDeviceId = 0x12;

  /**
   * @brief 将寄存器枚举转换为寄存器地址。
   * @param reg 寄存器枚举值。
   * @return 寄存器地址。
   */
  static uint8_t RegisterValue(Register reg);

  /**
   * @brief 将LED通道枚举转换为从0开始的通道序号。
   * @param channel LED通道枚举值。
   * @return 从0开始的通道序号。
   */
  static uint8_t ChannelIndex(LedChannel channel);

  /**
   * @brief 将LED组枚举转换为GCFG/PHCR组掩码。
   * @param group LED组枚举值。
   * @return 组掩码，非法组返回0。
   */
  static uint8_t GroupMask(LedGroup group);

  /**
   * @brief 判断通道枚举是否为真实LED通道。
   * @param channel LED通道枚举值。
   * @return channel为kLed1~kLed9时返回true。
   */
  static bool IsSingleChannel(LedChannel channel);

  /**
   * @brief 判断组枚举是否为真实LED组。
   * @param group LED组枚举值。
   * @return group为kGroup1~kGroup3时返回true。
   */
  static bool IsSingleGroup(LedGroup group);

  /**
   * @brief 读取一个AW21009寄存器。
   * @param reg 寄存器地址。
   * @param value 输出寄存器值。
   * @return 读取成功返回true。
   */
  bool ReadRegister(uint8_t reg, uint8_t* value);

  /**
   * @brief 写入一个AW21009寄存器。
   * @param reg 寄存器地址。
   * @param value 寄存器值。
   * @return 写入成功返回true。
   */
  bool WriteRegister(uint8_t reg, uint8_t value);

  /**
   * @brief 连续写入多个AW21009寄存器。
   * @param start_reg 起始寄存器地址。
   * @param data 写入数据缓冲区。
   * @param length 写入数据长度，单位为字节。
   * @return 写入成功返回true。
   */
  bool WriteRegisters(uint8_t start_reg, const uint8_t* data, size_t length);

  /**
   * @brief 通过读改写更新一个寄存器中的指定bit。
   * @param reg 寄存器地址。
   * @param mask 需要更新的bit掩码。
   * @param value 掩码前的新bit值。
   * @return 读改写成功返回true。
   */
  bool WriteMaskedRegister(uint8_t reg, uint8_t mask, uint8_t value);

  /**
   * @brief 通过从0开始的通道序号写入单个12位亮度值。
   * @param index 从0开始的LED通道序号。
   * @param value 亮度值，范围0~4095。
   * @return 寄存器写入成功返回true。
   */
  bool WriteBrightnessByIndex(uint8_t index, uint16_t value);

  /**
   * @brief 通过通道序号写入单个8位电流比例值。
   * @param index 从0开始的LED通道序号。
   * @param value 电流比例值。
   * @return 寄存器写入成功返回true。
   */
  bool WriteCurrentLimitByIndex(uint8_t index, uint8_t value);

  int32_t rst_;
};
}  // namespace cpp_bus_driver
