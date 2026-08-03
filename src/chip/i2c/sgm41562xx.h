/*
 * @Description: SGM41562 系列电池充电管理芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-08-03 16:11:27
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Sgm41562xx final : public ChipI2cGuide {
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

  struct IrqStatus {
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
    bool charge_enabled = false;                  // 充电使能状态
    bool high_impedance_enabled = false;          // 输入高阻模式状态
    bool input_current_limit_enabled = true;      // 输入限流是否生效
    uint16_t input_voltage_limit_mv = 0;          // 最低输入电压限制
    uint16_t input_current_limit_ma = 0;          // 输入电流限制
    uint16_t fast_charge_current_ma = 0;          // 快充电流
    uint16_t termination_current_ma = 0;          // 充电终止电流
    uint16_t charge_voltage_limit_mv = 0;         // 充电目标电压
    uint16_t system_voltage_regulation_mv = 0;    // 系统调节电压
    uint16_t input_overvoltage_threshold_mv = 0;  // 输入过压阈值
    bool watchdog_enabled = false;                // 看门狗使能状态
    uint16_t watchdog_timeout_s = 0;              // 看门狗超时时间
    bool charge_termination_enabled = false;      // 充电终止使能状态
    bool safety_timer_enabled = false;            // 安全定时器使能状态
    uint8_t safety_timer_hours = 0;               // 安全定时器时长
    bool safety_timer_extended_in_ppm = false;    // PPM模式下定时器倍增状态
    bool ntc_enabled = false;                     // NTC检测使能状态
    uint8_t thermal_regulation_threshold_c = 0;   // 热调节阈值
    bool input_voltage_loop_enabled = false;      // 输入电压环路使能状态
    bool pcb_overtemperature_protection_enabled = false;  // PCB过温保护状态
  };

  /**
   * @brief 创建SGM41562系列芯片对象
   * @param bus I2C总线对象
   * @param address I2C设备地址
   * @param rst 复位引脚，使用kDefaultValue时不控制复位引脚
   */
  explicit Sgm41562xx(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault, int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  /**
   * @brief 初始化芯片并根据型号执行对应寄存器初始化序列
   * @param freq_hz I2C总线频率，使用kDefaultValue时采用总线默认频率
   * @return 初始化成功返回true，失败返回false
   */
  bool Init(int32_t freq_hz = kDefaultValue) override;

  /**
   * @brief 反初始化芯片
   * @param delete_bus true：同时反初始化总线，false：保留总线
   * @return 反初始化成功返回true，失败返回false
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取REG0B中的设备ID
   * @param device_id 返回读取到的设备ID
   * @return 读取成功返回true，失败返回false
   */
  bool GetDeviceId(uint8_t& device_id);

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
  bool GetIrqStatus(IrqStatus& status);

  /**
   * @brief 设置充电使能
   * @param enable true：开启充电，false：关闭充电
   * @return 设置成功返回true，失败返回false
   */
  bool SetChargeEnable(bool enable);

  /**
   * @brief 读取并解析REG08中的芯片状态
   * @param status 返回解析后的芯片状态
   * @return 读取成功返回true，失败返回false
   */
  bool GetChipStatus(ChipStatus& status);

  /**
   * @brief 读取测试所需的关键充电与保护配置
   * @param config 返回解析后的关键配置
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
  enum class Cmd {
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
    kDeviceId = 0x0B,
    kExtendedInputCurrentControl = 0x0C,
    kExtendedCurrentControl = 0x0D,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x03;
  static constexpr uint8_t kDeviceIdSgm41562BAndSa = 0x00;
  static constexpr uint8_t kDeviceIdSgm41562A = 0x02;
  static constexpr uint8_t kDeviceIdSgm41562 = 0x04;
  static constexpr uint8_t kDeviceIdSgm41562S = 0x09;

  // SGM41562、SGM41562A和SGM41562B寄存器初始化序列
  static constexpr uint8_t kInitSequenceAb[] = {
      // 禁用PCB过温保护，保持输入电压环路和默认系统调节参数
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kSystemVoltageRegulation),
      0xB7,

      // 禁用NTC，保留默认的两倍安全定时器功能
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kMiscellaneousOperationControl),
      0x40,

      // 禁用看门狗，保留充电终止功能和5小时安全定时器
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kChargeTerminationTimerControl),
      0x1A,

      // 解除输入电流限制
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kSystemStatus),
      0x40,

      // 完成其他配置后开启充电
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kPowerOnConfiguration),
      0xA4,
  };

  // SGM41562S和SGM41562SA寄存器初始化序列
  static constexpr uint8_t kInitSequenceS[] = {
      // 保持输入电压环路和默认热调节、系统调节参数
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kSystemVoltageRegulation),
      0x73,

      // 禁用NTC，保留默认的两倍安全定时器功能
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kMiscellaneousOperationControl),
      0x40,

      // 禁用看门狗，保留充电终止功能和5小时安全定时器
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kChargeTerminationTimerControl),
      0x1A,

      // 禁用PCB过温保护，保持默认输入过压阈值
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kSystemStatus),
      0x40,

      // 将输入电流限制设置为800mA
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kExtendedInputCurrentControl),
      0xCA,

      // 完成其他配置后开启充电
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Cmd::kPowerOnConfiguration),
      0xA4,
  };

  /**
   * @brief 根据设备ID识别芯片型号
   * @param device_id REG0B中的原始设备ID
   * @return 返回识别出的芯片型号，无法识别时返回ChipType::kUnknown
   */
  ChipType DetectChipType(uint8_t device_id);

  /**
   * @brief 区分设备ID同为0x00的SGM41562B和SGM41562SA
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
   * @param cmd 需要更新的寄存器命令
   * @param mask 需要更新的位掩码
   * @param value 写入掩码范围内的目标值
   * @return 更新成功返回true，失败返回false
   */
  bool UpdateRegisterBits(Cmd cmd, uint8_t mask, uint8_t value);

  /**
   * @brief 读取指定寄存器
   * @param cmd 需要读取的寄存器命令
   * @param value 返回读取到的寄存器值
   * @param name 寄存器名称，用于输出错误日志
   * @return 读取成功返回true，失败返回false
   */
  bool ReadRegister(Cmd cmd, uint8_t& value, const char* name);

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
   * @brief 解析REG09寄存器值，不将0xFF视为读取失败
   * @param irq_status REG09寄存器值
   * @param status 返回解析后的故障状态
   */
  static void ParseIrqStatus(uint8_t irq_status, IrqStatus& status);

  /**
   * @brief 解析REG08寄存器值，不将0xFF视为读取失败
   * @param chip_status REG08寄存器值
   * @param status 返回解析后的芯片状态
   */
  static void ParseChipStatus(uint8_t chip_status, ChipStatus& status);

  int32_t rst_;
  ChipType chip_type_ = ChipType::kUnknown;
};
}  // namespace cpp_bus_driver
