/*
 * @Description: BQ2589x 系列电池充电与电源路径管理驱动接口
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {

class Bq2589x final : public I2cChipBase {
 public:
  // REG14 的 PN 与 DEV_REV 联合识别出的芯片型号。
  enum class ChipModel : uint8_t {
    kUnknown,   // 未识别或尚未读取器件信息。
    kBq25890,   // PN=3，DEV_REV=1，地址 0x6A。
    kBq25890h,  // PN=3，DEV_REV=3，地址 0x6A。
    kBq25892,   // PN=0，DEV_REV=1，地址 0x6B；与 BQ25898 重合。
    kBq25895,   // PN=7，DEV_REV=1，地址 0x6A。
    kBq25895m,  // PN=7，DEV_REV=2，地址 0x6A。
    kBq25896,   // PN=0，DEV_REV=2，地址 0x6B。
    kBq25898,   // PN=0，DEV_REV=1，地址 0x6B；与 BQ25892 重合。
    kBq25898c,  // PN=1，DEV_REV=1，地址 0x6B，辅助充电器。
    kBq25898d,  // PN=2，DEV_REV=1，地址 0x6A。
  };

  // 型号相关功能。BQ25898C 不具备下列可选功能，公共充电/ADC/DPM 接口仍可用。
  enum class Feature : uint8_t {
    kIlimPin,                // ILIM 引脚限流。
    kInputCurrentOptimizer,  // ICO 输入电流优化。
    kOtg,                    // OTG 升压、升压电压和开关频率配置。
    kCurrentPulseControl,    // PumpX 电流脉冲控制。
    kIrCompensation,         // BAT_COMP 与 VCLAMP。
    kBatfetControl,          // BATFET_DIS 写入；98C 手册存在冲突，暂禁止写入。
    kBatfetDelay,            // BATFET_DLY 延迟配置。
    kBatfetReset,            // BATFET_RST_EN 系统复位允许位。
    kTsAdc,                  // TS ADC 与 NTC 状态。
    kBoostTemperatureThresholds,  // BHOT/BCOLD：90、92、95、95M、96。
    kDpDmDac,                     // DP/DM DAC：90H、98D。
    k12VoltDetection,             // EN_12V：90H、98D。
    kHvdcp,                       // HVDCP_EN：90、90H、95、95M、98D。
    kMaxCharge,                   // MAXC_EN：90、90H、95、95M、98D。
    kUsbInputDetection,           // D+/D- 输入检测；其余型号使用 PSEL。
    kForceDsel,                   // FORCE_DSEL：90H、98D。
    kVokOtg,                      // VOK_OTG_EN：98。
    kBoostMinimumBatteryVoltage,  // MIN_VBAT_SEL：90H、95M、96、98、98D。
    kBoostPfm,                    // PFM_OTG_DIS：90H、95M、96、98、98D。
    kJeita,              // JEITA 电流/电压配置：除 95、95M、98C 之外的型号。
    kBoostCurrentLimit,  // BOOST_LIM：除 95、95M、98C 之外的型号。
    kSdpStatus,          // SDP_STAT：仅 95，区分 USB100/USB500。
    kBatteryLoad,        // BAT_LOADEN：90、92、95、95M、96。
  };

  // 从同一次 REG14 读取中解析的型号、修订号和温度配置。
  struct ChipInfo {
    // 联合 PN 与 DEV_REV 得到的型号；未知组合不推测为其他型号。
    ChipModel model = ChipModel::kUnknown;
    // PN[2:0] 字段，范围 0-7，不是完整 REG14 寄存器值。
    uint8_t part_number = 0;
    // DEV_REV[1:0] 修订号，范围 0-3。
    uint8_t device_revision = 0;
    // TS_PROFILE 实读位；仅 jeita_profile_valid 为 true
    // 时有效，不用于型号判定。
    bool jeita_profile = false;
    // TS_PROFILE 是否有定义；BQ25898C 的该位保留，不能用来判断温度配置。
    bool jeita_profile_valid = false;
    // 原始身份是否存在型号重合；显式选择 92/98 后仍为 true。
    bool model_is_ambiguous = false;
  };

  // DP_DAC/DM_DAC（98D 手册称 DPLUS_DAC/DMINUS_DAC）的规范化输出档位。
  enum class DpDmVoltage : uint8_t {
    kHighImpedance = 0,  // 高阻态。
    k0Mv = 1,            // 0 V。
    k600Mv = 2,          // 0.6 V。
    k1200Mv = 3,         // 1.2 V。
    k2000Mv = 4,         // 2.0 V。
    k2700Mv = 5,         // 2.7 V。
    k3300Mv = 6,         // 3.3 V。
    kShortDpDm = 7,      // 仅 98D 的 DP DAC：短接 D+/D-，关闭双方驱动。
  };

  // BHOT 升压高温阈值，数值为 VTS/VREGN 百分比而非摄氏温度。
  enum class BoostHotThreshold : uint8_t {
    k34p75Percent = 0,  // 34.75%，手册默认档位。
    k37p75Percent = 1,  // 37.75%。
    k31p25Percent = 2,  // 31.25%。
    kDisabled = 3,      // 禁用升压温度保护。
  };

  // BCOLD 升压低温阈值，数值为 VTS/VREGN 百分比。
  enum class BoostColdThreshold : uint8_t {
    k77Percent = 0,  // 77%，手册默认档位。
    k80Percent = 1,  // 80%。
  };

  // CONV_RATE 对应的 ADC 单次或连续转换模式。
  enum class AdcConversionMode : uint8_t {
    kOneShot = 0,     // 单次模式，通过 CONV_START 启动转换。
    kContinuous = 1,  // 连续转换，手册标称周期为 1 秒。
  };

  // BOOST_FREQ 对应的升压开关频率。
  enum class BoostFrequency : uint8_t {
    k1500Khz = 0,  // 1500 kHz；默认频率因型号而异。
    k500Khz = 1,   // 500 kHz。
  };

  // JEITA 低温区间使用的充电电流比例，相对于 ICHG 设置值。
  enum class JeitaLowTemperatureCurrent : uint8_t {
    k50Percent = 0,  // 使用 ICHG 的 50%。
    k20Percent = 1,  // 使用 ICHG 的 20%，手册默认档位。
  };

  // JEITA 高温区间使用的充电电压。
  enum class JeitaHighTemperatureVoltage : uint8_t {
    kVregMinus200Mv = 0,  // 使用 VREG 减 200 mV，手册默认档位。
    kVreg = 1,            // 使用原 VREG，不降低高温充电电压。
  };

  // 设置 BATFET_DIS 后关闭 BATFET 的延迟选择。
  enum class BatfetTurnOffDelay : uint8_t {
    kImmediate = 0,      // 立即关闭。
    k10To15Seconds = 1,  // 按手册 tSM_DLY 延迟约 10-15 秒。
  };

  // 输入电压动态功率管理阈值的计算方式。
  enum class VindpmMode : uint8_t {
    kRelative = 0,  // 芯片根据输入空载电压和 VINDPM_OS 计算阈值。
    kAbsolute = 1,  // 使用主机写入的绝对 VINDPM 阈值。
  };

  // 规范化输入来源；PSEL 的适配器码 2 与 USB CDP 码 2 分开表示。
  enum class VbusStatus : uint8_t {
    kNoInput = 0,                   // 未检测到输入。
    kUsbHostSdp = 1,                // USB 主机标准下行端口。
    kUsbCdp = 2,                    // USB 充电下行端口。
    kUsbDcp = 3,                    // USB 专用充电端口。
    kAdjustableHighVoltageDcp = 4,  // 可调高压充电适配器。
    kUnknownAdapter = 5,            // 检测到输入，但无法确定适配器类型。
    kNonStandardAdapter = 6,        // 非标准分压式适配器。
    kOtg = 7,                       // 升压 OTG 输出模式。
    kAdapter = 8,     // PSEL 适配器；原始 VBUS_STAT 为 2，不表示 USB CDP。
    kUnknown = 0xFF,  // 未定义的状态编码，原值可从 ChipStatus::raw 检查。
  };

  // CHRG_STAT 当前充电阶段。
  enum class ChargeStatus : uint8_t {
    kNotCharging = 0,      // 未充电。
    kPrecharge = 1,        // 预充电阶段。
    kFastCharge = 2,       // 快速充电阶段。
    kTerminationDone = 3,  // 充电终止完成。
  };

  // CHRG_FAULT 充电故障类型。
  enum class ChargeFault : uint8_t {
    kNormal = 0,              // 无充电故障。
    kInputFault = 1,          // 输入过压或输入电压不足等输入故障。
    kThermalShutdown = 2,     // 芯片过温关断。
    kSafetyTimerExpired = 3,  // 充电安全定时器到期。
  };

  // 规范化 NTC 状态；95/95M 的充电模式编码 1/2 被转换为 kCold/kHot。
  enum class NtcFault : uint8_t {
    kNormal = 0,      // 正常温度区间。
    kWarm = 2,        // JEITA 偏热区间。
    kCool = 3,        // JEITA 偏冷区间。
    kCold = 5,        // 低于允许温度区间。
    kHot = 6,         // 高于允许温度区间。
    kUnknown = 0xFF,  // 未定义的 NTC 编码，原值可从 FaultStatus::raw 检查。
  };

  // 一次 REG0B 读取获得的芯片状态，读取不会清除故障锁存。
  struct ChipStatus {
    // VBUS_STAT：当前输入来源或 OTG 状态。
    VbusStatus vbus_status = VbusStatus::kNoInput;
    // CHRG_STAT：当前充电阶段。
    ChargeStatus charge_status = ChargeStatus::kNotCharging;
    // PG_STAT：true 表示输入电源有效。
    bool power_good = false;
    // 仅 BQ25895 且当前 VBUS_STAT=SDP 时为 true。
    bool sdp_status_valid = false;
    // SDP_STAT 对应 USB100/USB500，单位 mA；无效时为 0，不是实际 IINLIM。
    uint16_t usb_sdp_current_limit_ma = 0;
    // VSYS_STAT：true 表示正处于系统最低电压调节状态。
    bool system_minimum_voltage_regulation = false;
    // 完整 REG0B 原始字节，包含未单独解释的保留位。
    uint8_t raw = 0;
  };

  // 一次 REG0C 读取获得的故障信息，除 NTC 外具有读取清除锁存语义。
  struct FaultStatus {
    // WATCHDOG_FAULT：看门狗曾到期。
    bool watchdog_expired = false;
    // BOOST_FAULT：升压过载、VBUS 过压或升压时电池电压过低。
    bool boost_fault = false;
    // BQ25898C 没有升压故障字段，此时为 false。
    bool boost_fault_valid = false;
    // CHRG_FAULT：输入故障、热关断或安全定时器故障。
    ChargeFault charge_fault = ChargeFault::kNormal;
    // BAT_FAULT：电池过压。
    bool battery_overvoltage = false;
    // NTC_FAULT：有效时反映 TS 当前状态；95M 未定义的升压编码返回 kUnknown。
    NtcFault ntc_fault = NtcFault::kNormal;
    // BQ25898C 没有 TS 引脚，此时为 false，ntc_fault 为 kUnknown。
    bool ntc_fault_valid = false;
    // 完整 REG0C 原始字节，用于保留未定义故障编码。
    uint8_t raw = 0;
  };

  // REG13 的输入动态功率管理状态。
  struct DpmStatus {
    // VDPM_STAT：true 表示输入电压限制正在生效。
    bool vindpm_active = false;
    // IDPM_STAT：true 表示输入电流限制正在生效。
    bool iindpm_active = false;
    // REG13 IDPM_LIM 原码，范围 0-63。
    uint8_t input_current_limit_code = 0;
    // 可转换为 mA 时为 true；98C 手册未给出编码偏移，此值为 false。
    bool input_current_limit_valid = false;
    // 有效时为 100-3250 mA，50 mA/步；无效时为 0，ICO 型号可报告优化限流。
    uint16_t input_current_limit_ma = 0;
  };

  // REG09 中 PumpX 升压和降压脉冲序列的执行状态。
  struct PumpxStatus {
    // PUMPX_UP：true 表示升压脉冲序列尚未完成。
    bool voltage_increase_active = false;
    // PUMPX_DN：true 表示降压脉冲序列尚未完成。
    bool voltage_decrease_active = false;
  };

  // 最近 ADC 结果；98C 跳过未定义的 REG10，不保证各字段来自同一采样时刻。
  struct AdcMeasurements {
    // BATV：电池电压，单位 mV；2304 + code * 20，范围 2304-4844。
    uint16_t battery_voltage_mv = 0;
    // SYSV：系统电压，单位 mV；2304 + code * 20，范围 2304-4844。
    uint16_t system_voltage_mv = 0;
    // TSPCT：VTS/VREGN * 100，21 + code * 0.465，不是摄氏温度。
    float ts_voltage_percentage = 0.0f;
    // BQ25898C 没有 REG10，此值为 false，ts_voltage_percentage 保持 0。
    bool ts_voltage_valid = false;
    // VBUSV：输入电压，单位 mV；2600 + code * 100，需同时检查 vbus_attached。
    uint16_t vbus_voltage_mv = 0;
    // ICHGR：充电电流，单位 mA，范围 0-6350；不是放电电流。
    uint16_t charge_current_ma = 0;
    // THERM_STAT：true 表示芯片正在进行热调节。
    bool thermal_regulation_active = false;
    // VBUS_GD：true 表示检测到 VBUS 接入。
    bool vbus_attached = false;
  };

  // BQ25896 默认 7 位 I2C 地址，不包含读写方向位。
  static constexpr uint8_t kDeviceI2cAddressDefault = 0x6B;
  // 默认 I2C 频率，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;
  // BQ25896 的 REG14 PN 字段，需同时检查修订号。
  static constexpr uint8_t kBq25896PartNumber = 0x00;
  // 当前适配的 BQ25896 REG14 DEV_REV 字段。
  static constexpr uint8_t kBq25896DeviceRevision = 0x02;

  /**
   * @brief 构造充电芯片对象；不访问总线、不改变芯片供电配置。
   * @param bus 该 I2C 地址专用的总线设备包装，可与其他设备共享物理总线。
   * @param address 7 位 I2C 地址，默认 0x6B；0x6A
   * 型号须显式传入或使用型号构造函数。
   * @param model 板级指定的型号；kUnknown 尝试识别当前地址，不扫描另一地址。
   * @return 无返回值；需继续调用 Init() 完成探测与型号检查。
   * @note 同一器件的调用必须串行，包括通过其他对象进行的访问；配置更新包含
   * 多次总线操作，不能仅依赖单次 I2C 事务锁保护读改写过程。
   */
  explicit Bq2589x(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kDeviceI2cAddressDefault,
      ChipModel model = ChipModel::kUnknown)
      : I2cChipBase(bus, address),
        requested_model_(model),
        device_address_(address) {}

  /**
   * @brief 按型号选用手册的固定 I2C 地址构造对象，不访问硬件。
   * @param bus 该器件专用的 I2C 总线设备包装。
   * @param model 板级指定型号；kUnknown 使用默认 0x6B 地址并尝试自动识别。
   * @return 无返回值；继续调用 Init() 验证读取到的 PN、REV 和指定型号。
   */
  Bq2589x(std::shared_ptr<I2cBusBase> bus, ChipModel model)
      : Bq2589x(bus, GetDefaultAddress(model), model) {}

  /**
   * @brief 获取型号在手册中规定的 7 位 I2C 地址，不访问硬件。
   * @param model 器件型号，kUnknown 使用 BQ25896 的默认地址。
   * @return 90/90H/95/95M/98D 返回 0x6A；92/96/98/98C/未知值返回 0x6B。
   */
  static uint8_t GetDefaultAddress(ChipModel model);

  /**
   * @brief 初始化总线并识别器件，执行对应型号的寄存器初始化序列。
   * @param freq_hz I2C 频率，单位 Hz，范围 1-400000，默认 100000。
   * @return 型号、地址确认且初始化序列成功后返回 true；参数无效、总线失败、
   * 型号不符、重合未消除或初始化序列失败返回 false。
   * @note 不支持型号的已读 ID 保留在 chip_info() 中；其寄存器接口不可使用。
   * 按断电重启后的默认状态初始化，具体配置以各型号初始化序列为准。
   * 实际输入限流仍受 ILIM 电阻和 ICO 约束；输入检测可能自动改写 IINLIM，
   * 电源重新接入后需要应用按供电能力重新配置。
   * 不复位芯片。
   * QON 不作为普通硬件复位引脚处理。
   * 重复初始化已就绪对象不会重新配置频率或覆盖应用设置的充电电流。
   * BQ25892/98 的 PN=0、REV=1、地址=0x6B 相同，必须在构造时显式指定型号。
   * 只接受手册记录的 PN/REV；TS_PROFILE 只报告，不作为强制识别条件。
   * 不支持的型号功能返回
   * false，不写保留位；线性参数范围内向下量化，离散档位精确匹配。
   * 除有特别说明外，读取失败保持输出参数原值；ADC 读取不会自动启动转换。
   * 驱动不创建后台喂狗任务；开启看门狗后，须周期调用 ResetWatchdogTimer()。
   * 看门狗到期会恢复部分寄存器默认值，读取接口仍读取硬件当前配置。
   */
  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;

  /**
   * @brief 解除总线设备初始化并清除驱动状态，不复位充电芯片或关闭电源。
   * @param delete_bus true 请求释放自有物理总线；false 仅解除设备并保留总线。
   * @return 释放成功返回 true；总线释放失败返回 false，可稍后重试。
   * @note Deinit(false) 后保留总线清理标志，可再调用 Deinit(true) 完成清理。
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 获取 Init() 缓存的器件识别信息，不访问总线。
   * @return 内部 ChipInfo 的常量引用；未识别时 model 为 kUnknown。
   */
  const ChipInfo& chip_info() const { return chip_info_; }

  /**
   * @brief 判断对象是否已初始化且型号已确认。
   * @return 已就绪且型号受支持返回 true，否则返回 false。
   */
  bool IsSupported() const;

  /**
   * @brief 判断当前型号是否支持指定的可选功能，不访问硬件。
   * @param feature 待查询功能；仅表示驱动支持，不表示功能当前已开启。
   * @return 型号确认且功能受支持返回 true；未初始化、不支持或非法枚举返回
   * false。
   */
  bool HasFeature(Feature feature) const;

  /**
   * @brief 读取 REG14 并解析型号、PN、修订号及 TS_PROFILE，不更新初始化缓存。
   * @param info 保存识别结果的结构体；读取失败时保持原值。
   * @return 总线已初始化且读取成功返回 true，否则返回 false。
   */
  bool GetChipInfo(ChipInfo& info);

  /**
   * @brief 读取 REG14 的 PN[2:0]，不将整个寄存器当作芯片标识。
   * @param part_number 保存 PN 字段，范围 0-7；失败时保持原值。
   * @return 读取成功返回 true；总线未初始化或读取失败返回 false。
   */
  bool GetChipId(uint8_t& part_number);

  /**
   * @brief 设置 REG14 REG_RST 并等待清零，恢复硬件默认值及安全定时器。
   * @param timeout_ms 轮询预算，单位 ms，默认 100；不是 TI 规定的复位时长。
   * @return 复位命令写入且清零返回 true；未就绪、总线失败或等待超时返回 false。
   * @note WATCHDOG 会恢复为 40 秒。超时不会撤销已发出的命令，单次 I2C
   * 访问也可能使实际等待时间超过轮询预算。
   */
  bool ResetRegisters(uint32_t timeout_ms = 100);

  /**
   * @brief 读取 REG14 REG_RST，检查寄存器复位命令是否尚未完成。
   * @param active true 表示复位位仍为 1；读取失败时保持原值。
   * @return 总线已初始化且读取成功返回 true，否则返回 false。
   */
  bool IsRegisterResetActive(bool& active);

  /**
   * @brief 设置 REG00 EN_HIZ 输入高阻态模式。
   * @param enabled true 开启高阻态，false 退出高阻态。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool SetHighImpedanceEnabled(bool enabled);

  /**
   * @brief 读取 REG00 EN_HIZ 输入高阻态配置。
   * @param enabled true 表示已配置高阻态；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetHighImpedanceEnabled(bool& enabled);

  /**
   * @brief 设置 REG00 EN_ILIM，控制 ILIM 引脚是否参与输入限流。
   * @param enabled true 启用 ILIM 引脚限流，false 禁用该引脚限流功能。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。引脚启用时取 IINLIM 与 ILIM
   * 限值的较小值。
   */
  bool SetIlimPinEnabled(bool enabled);

  /**
   * @brief 读取 REG00 EN_ILIM 配置。
   * @param enabled true 表示 ILIM 引脚限流已启用；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetIlimPinEnabled(bool& enabled);

  /**
   * @brief 设置 REG00 IINLIM 输入电流限制，按 50 mA 步长向下取整。
   * @param current_ma 输入电流上限，单位 mA，范围 100-3250。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   * @note 输入检测可能自动改写此字段；具有且启用 ILIM 的型号还受引脚限流约束。
   */
  bool SetInputCurrentLimit(uint16_t current_ma);

  /**
   * @brief 读取 REG00 IINLIM 设置值，不测量实际输入电流。
   * @param current_ma 保存限流值，单位 mA，范围 100-3250；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetInputCurrentLimit(uint16_t& current_ma);

  /**
   * @brief 设置 REG01 BHOT 升压高温监控阈值。
   * @param threshold VTS/VREGN 的 34.75%、37.75%、31.25% 档位或禁用保护。
   * @return 写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note 90H/98/98C/98D 不支持，返回 false。kDisabled 关闭升压温度保护。
   */
  bool SetBoostHotThreshold(BoostHotThreshold threshold);

  /**
   * @brief 读取 REG01 BHOT 升压高温阈值档位。
   * @param threshold 保存比例阈值或禁用档位；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 90H/98/98C/98D 不支持，返回 false。
   */
  bool GetBoostHotThreshold(BoostHotThreshold& threshold);

  /**
   * @brief 设置 REG01 BCOLD 升压低温监控阈值。
   * @param threshold VTS/VREGN 的 77% 或 80% 档位，不是摄氏温度。
   * @return 写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note 90H/98/98C/98D 不支持，返回 false。
   */
  bool SetBoostColdThreshold(BoostColdThreshold threshold);

  /**
   * @brief 读取 REG01 BCOLD 升压低温阈值档位。
   * @param threshold 保存比例阈值档位；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 90H/98/98C/98D 不支持，返回 false。
   */
  bool GetBoostColdThreshold(BoostColdThreshold& threshold);

  /**
   * @brief 设置 REG01 DP_DAC（98D 为 DPLUS_DAC）的 D+ 输出档位。
   * @param voltage 高阻或 0/600/1200/2000/2700/3300 mV；98D 额外支持
   * kShortDpDm。
   * @return 写入成功返回 true；型号不支持、档位无效、检测未完成或总线失败返回
   * false。
   * @note 仅支持 90H/98D；要求输入有效且 FORCE_DPDM
   * 空闲。新输入插入会恢复默认值。 98D 的短接模式关闭 D+ 与 D-
   * 驱动，可能改变高压适配器的输出电压。
   */
  bool SetDpDac(DpDmVoltage voltage);

  /**
   * @brief 读取 REG01 DP_DAC/DPLUS_DAC 的 D+ 输出配置。
   * @param voltage 保存档位，读取失败时保持原值；98D 的码 7 为 kShortDpDm。
   * @return 90H/98D 读取有效编码成功返回 true；其他型号、保留编码或总线失败返回
   * false。
   */
  bool GetDpDac(DpDmVoltage& voltage);

  /**
   * @brief 设置 REG01 DM_DAC（98D 为 DMINUS_DAC）的 D- 输出档位。
   * @param voltage 高阻或 0/600/1200/2000/2700/3300 mV；不接受 kShortDpDm。
   * @return 写入成功返回 true；型号不支持、档位无效、检测未完成或总线失败返回
   * false。
   * @note 仅支持 90H/98D；要求输入有效且 FORCE_DPDM
   * 空闲。新输入插入会恢复默认值。
   */
  bool SetDmDac(DpDmVoltage voltage);

  /**
   * @brief 读取 REG01 DM_DAC/DMINUS_DAC 的 D- 输出配置。
   * @param voltage 保存规范化档位；98D 的码 6/7 均返回
   * k3300Mv，失败时保持原值。
   * @return 90H/98D 读取有效编码成功返回 true；其他型号、保留编码或总线失败返回
   * false。
   */
  bool GetDmDac(DpDmVoltage& voltage);

  /**
   * @brief 设置 REG01 EN_12V，允许 HVDCP/MaxCharge 检测 12 V 档位。
   * @param enabled true 允许 12 V 检测，false 禁止；不会自行触发输入检测。
   * @return 90H/98D 写入成功返回 true；未就绪、不支持或总线失败返回 false。
   */
  bool Set12VoltDetectionEnabled(bool enabled);

  /**
   * @brief 读取 REG01 EN_12V 的 12 V 检测允许配置。
   * @param enabled 保存配置，失败时保持原值；不表示当前 VBUS 为 12 V。
   * @return 90H/98D 读取成功返回 true；未就绪、不支持或总线失败返回 false。
   */
  bool Get12VoltDetectionEnabled(bool& enabled);

  /**
   * @brief 设置 REG01 VINDPM_OS 相对输入电压限制偏移。
   * @param offset_mv 单位 mV；90H/98/98C/98D 仅支持 400 或 600，精确匹配；
   *     90/92/95/95M/96 支持 0-3100，按 100 mV 步长向下取整。
   * @return 写入成功返回 true；范围或档位无效、未就绪或总线访问失败返回 false。
   * @note 当输入空载电压大于 6 V 时，芯片使用此偏移的 2 倍计算阈值；
   * 最终 VINDPM 阈值限制在 3900-15300 mV。
   */
  bool SetInputVoltageLimitOffset(uint16_t offset_mv);

  /**
   * @brief 读取 REG01 VINDPM_OS 的原始偏移设置，不包含大于 6 V 时的倍乘。
   * @param offset_mv 保存偏移，单位 mV；90H/98/98C/98D 为 400 或 600，
   *     其他型号为 0-3100；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetInputVoltageLimitOffset(uint16_t& offset_mv);

  /**
   * @brief 设置 REG02 CONV_RATE，选择单次或连续 ADC 转换模式。
   * @param mode kOneShot 为单次模式，kContinuous 开启约 1 秒周期的连续转换。
   * @return 写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note 切换到单次模式不会自动启动一次转换，需调用 StartAdcConversion()。
   */
  bool SetAdcConversionMode(AdcConversionMode mode);

  /**
   * @brief 读取 REG02 CONV_RATE 对应的 ADC 转换模式。
   * @param mode 保存当前模式；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetAdcConversionMode(AdcConversionMode& mode);

  /**
   * @brief 设置 REG02 CONV_START 启动一次 ADC 转换，不等待转换完成。
   * @return 命令写入成功返回 true；非单次模式、转换或输入检测忙、未就绪或
   * 总线访问失败返回 false。
   * @note 单次转换可能耗时 1 秒；仅电池供电时 ADC 还要求 VBAT 高于 SYS_MIN。
   */
  bool StartAdcConversion();

  /**
   * @brief 读取 REG02 CONV_START，查询转换启动位是否仍为 1。
   * @param active true 表示该位为 1；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 输入来源检测期间此位也可能为 1，不能只据此判断有效采样已完成。
   */
  bool IsAdcConversionActive(bool& active);

  /**
   * @brief 在单次模式下等待 REG02 CONV_START 清零，不自动启动转换。
   * @param timeout_ms 轮询预算，单位 ms，默认 1100；单次转换可能耗时 1 秒。
   * @return 检测到清零返回 true；连续模式、未就绪、总线失败或超时返回 false。
   * @note 单次 I2C 访问可能使实际等待时间超过轮询预算；超时不会取消转换。
   */
  bool WaitForAdcConversion(uint32_t timeout_ms = 1100);

  /**
   * @brief 设置 REG02 BOOST_FREQ 升压开关频率。
   * @param frequency k1500Khz 为 1.5 MHz，k500Khz 为 500 kHz。
   * @return 写入成功返回 true；枚举无效、OTG 已开启、未就绪或总线失败返回
   * false。
   * @note BQ25898C 不支持，返回 false。OTG 已开启时硬件忽略写入，驱动提前拒绝。
   */
  bool SetBoostFrequency(BoostFrequency frequency);

  /**
   * @brief 读取 REG02 BOOST_FREQ 升压频率配置。
   * @param frequency 保存频率档位；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetBoostFrequency(BoostFrequency& frequency);

  /**
   * @brief 设置 REG02 ICO_EN 输入电流优化算法使能。
   * @param enabled true 启用 ICO，false 禁用 ICO。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool SetInputCurrentOptimizerEnabled(bool enabled);

  /**
   * @brief 读取 REG02 ICO_EN 输入电流优化配置。
   * @param enabled true 表示 ICO 已启用；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetInputCurrentOptimizerEnabled(bool& enabled);

  /**
   * @brief 设置 REG02 HVDCP_EN，允许高压 DCP 适配器握手。
   * @param enabled true 允许 HVDCP 握手，false 禁止；不自动重新检测输入。
   * @return 90/90H/95/95M/98D 写入成功返回 true；不支持、未就绪或总线失败返回
   * false。
   * @note 高压握手可能使适配器改变 VBUS 电压，板级电源路径须能承受目标电压。
   */
  bool SetHvdcpEnabled(bool enabled);

  /**
   * @brief 读取 REG02 HVDCP_EN 高压握手允许配置。
   * @param enabled 保存配置，失败时保持原值；不表示握手已完成。
   * @return 90/90H/95/95M/98D 读取成功返回 true；不支持、未就绪或总线失败返回
   * false。
   */
  bool GetHvdcpEnabled(bool& enabled);

  /**
   * @brief 设置 REG02 MAXC_EN，允许 MaxCharge 适配器握手。
   * @param enabled true 允许 MaxCharge 握手，false 禁止；不自动重新检测输入。
   * @return 90/90H/95/95M/98D 写入成功返回 true；不支持、未就绪或总线失败返回
   * false。
   * @note 允许握手可能使兼容适配器改变 VBUS 电压。
   */
  bool SetMaxChargeEnabled(bool enabled);

  /**
   * @brief 读取 REG02 MAXC_EN 的 MaxCharge 握手允许配置。
   * @param enabled 保存配置，失败时保持原值；不表示握手已完成。
   * @return 90/90H/95/95M/98D 读取成功返回 true；不支持、未就绪或总线失败返回
   * false。
   */
  bool GetMaxChargeEnabled(bool& enabled);

  /**
   * @brief 设置 REG02 FORCE_DPDM，强制执行输入来源检测。
   * @return 命令写入成功返回 true；检测已在进行、未就绪或总线失败返回 false。
   * @note 92/96/98/98C 使用 PSEL，其他型号使用 D+/D-；检测结果可能改写 IINLIM。
   */
  bool ForceInputDetection();

  /**
   * @brief 读取 REG02 FORCE_DPDM，查询 PSEL 或 D+/D- 输入检测是否尚未完成。
   * @param active true 表示 FORCE_DPDM 仍为 1；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool IsInputDetectionActive(bool& active);

  /**
   * @brief 设置 REG02 AUTO_DPDM_EN，控制 VBUS 插入时是否自动检测输入来源。
   * @param enabled true 开启自动检测，false 关闭自动检测。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note 92/96/98/98C 检测 PSEL，其他型号检测 D+/D-。
   */
  bool SetAutomaticInputDetectionEnabled(bool enabled);

  /**
   * @brief 读取 REG02 AUTO_DPDM_EN 的自动输入检测配置。
   * @param enabled true 表示开启自动检测；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetAutomaticInputDetectionEnabled(bool& enabled);

  /**
   * @brief 设置 REG03 BAT_LOADEN，控制内部电池负载功能。
   * @param enabled true 开启内部电池负载，false 关闭负载。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note 仅 90/92/95/95M/96 支持；90H/98/98C/98D 返回 false。
   */
  bool SetBatteryLoadEnabled(bool enabled);

  /**
   * @brief 读取 REG03 BAT_LOADEN 内部电池负载配置。
   * @param enabled true 表示已启用负载；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 仅 90/92/95/95M/96 支持；90H/98/98C/98D 返回 false。
   */
  bool GetBatteryLoadEnabled(bool& enabled);

  /**
   * @brief 设置 REG03 FORCE_DSEL，控制 DSEL 引脚的强制高电平配置。
   * @param forced_high true 强制为高，false 允许芯片按输入检测状态控制输出。
   * @return 90H/98D 写入成功返回 true；不支持、未就绪或总线失败返回 false。
   * @note 配置不等于实时引脚电平，98D 仅电池供电等状态仍受手册的引脚逻辑约束。
   */
  bool SetDselForcedHigh(bool forced_high);

  /**
   * @brief 读取 REG03 FORCE_DSEL 强制配置，不采样 DSEL 引脚电平。
   * @param forced_high 保存配置，失败时保持原值。
   * @return 90H/98D 读取成功返回 true；不支持、未就绪或总线失败返回 false。
   */
  bool GetDselForcedHigh(bool& forced_high);

  /**
   * @brief 设置 BQ25898 REG03 VOK_OTG_EN，允许 OTG 时产生有效 VOK 状态。
   * @param enabled true 允许 OTG 时 VOK=1，false 使 OTG 时 VOK=0。
   * @return BQ25898 写入成功返回 true；其他型号、未就绪或总线失败返回 false。
   * @note 不开启 OTG，也不改变适配器插入和普通电池供电时的 VOK 判定规则。
   */
  bool SetVokOtgEnabled(bool enabled);

  /**
   * @brief 读取 BQ25898 REG03 VOK_OTG_EN 配置，不直接读取 VOK 状态。
   * @param enabled 保存配置，失败时保持原值。
   * @return BQ25898 读取成功返回 true；其他型号、未就绪或总线失败返回 false。
   */
  bool GetVokOtgEnabled(bool& enabled);

  /**
   * @brief 写入 REG03 WD_RST 喂看门狗，不复位其他充电寄存器。
   * @return 命令写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note WD_RST 由硬件自动清零；看门狗时限由 SetWatchdogTimer() 设置。
   */
  bool ResetWatchdogTimer();

  /**
   * @brief 设置 REG03 OTG_CONFIG 升压输出使能。
   * @param enabled true 请求开启 OTG，false 请求关闭 OTG。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。实际输出还要求 OTG
   * 引脚及电池、输入条件满足。
   */
  bool SetOtgEnabled(bool enabled);

  /**
   * @brief 读取 REG03 OTG_CONFIG，不直接确认升压输出电压已经建立。
   * @param enabled true 表示寄存器已配置 OTG 开启；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetOtgEnabled(bool& enabled);

  /**
   * @brief 设置 REG03 CHG_CONFIG 充电使能。
   * @param enabled true 允许充电，false 关闭充电。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note 实体 CE 引脚还必须为低电平，其他充电及保护条件也须满足。
   */
  bool SetChargeEnabled(bool enabled);

  /**
   * @brief 读取 REG03 CHG_CONFIG 充电使能，不代表当前一定有充电电流。
   * @param enabled true 表示寄存器允许充电；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetChargeEnabled(bool& enabled);

  /**
   * @brief 设置 REG03 SYS_MIN 系统最低电压，按 100 mV 步长向下取整。
   * @param voltage_mv 电压值，单位 mV，范围 3000-3700。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   */
  bool SetSystemMinimumVoltage(uint16_t voltage_mv);

  /**
   * @brief 读取 REG03 SYS_MIN 系统最低电压设置。
   * @param voltage_mv 保存电压，单位 mV，范围 3000-3700；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetSystemMinimumVoltage(uint16_t& voltage_mv);

  /**
   * @brief 设置 REG03 MIN_VBAT_SEL，选择电池下降到何电压时退出升压模式。
   * @param voltage_mv 电压阈值，单位 mV，仅支持 2900 或 2500。
   * @return 写入成功返回 true；不是支持档位、未就绪或总线失败返回 false。
   * @note 90/92/95/98C 不支持，返回 false。
   */
  bool SetBoostMinimumBatteryVoltage(uint16_t voltage_mv);

  /**
   * @brief 读取 REG03 MIN_VBAT_SEL 对应的升压退出电池电压阈值。
   * @param voltage_mv 保存阈值，单位 mV，为 2900 或 2500；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 90/92/95/98C 不支持，返回 false。
   */
  bool GetBoostMinimumBatteryVoltage(uint16_t& voltage_mv);

  /**
   * @brief 设置 REG04 EN_PUMPX 电流脉冲控制使能。
   * @param enabled true 允许 PumpX 脉冲控制，false 禁用并停止正在进行的序列。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool SetCurrentPulseControlEnabled(bool enabled);

  /**
   * @brief 读取 REG04 EN_PUMPX 电流脉冲控制配置。
   * @param enabled true 表示已启用 PumpX；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetCurrentPulseControlEnabled(bool& enabled);

  /**
   * @brief 设置 REG04 ICHG 快速充电电流上限，按 64 mA 步长向下取整。
   * @param current_ma 单位 mA；96/98C 为 0-3008，98/98D 为 0-4032，
   *     90/90H/92/95/95M 为 0-5056。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   * @note 量化为 0 的设置会禁止充电，输入 1-63 mA 也会量化为 0。
   */
  bool SetFastChargeCurrentLimit(uint16_t current_ma);

  /**
   * @brief 读取 REG04 ICHG 的充电电流配置，不测量实际充电电流。
   * @param current_ma 保存限制值，单位 mA；96/98C 为 0-3008，98/98D 为 0-4032，
   *     90/90H/92/95/95M 为 0-5056；读取失败时保持原值。
   * @return 读取成功返回 true；编码超出型号范围、未就绪或总线失败返回 false。
   */
  bool GetFastChargeCurrentLimit(uint16_t& current_ma);

  /**
   * @brief 设置 REG05 IPRECHG 预充电电流，按 64 mA 步长向下取整。
   * @param current_ma 电流值，单位 mA，范围 64-1024。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   */
  bool SetPrechargeCurrentLimit(uint16_t current_ma);

  /**
   * @brief 读取 REG05 IPRECHG 预充电电流设置。
   * @param current_ma 保存电流值，单位 mA，范围 64-1024；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetPrechargeCurrentLimit(uint16_t& current_ma);

  /**
   * @brief 设置 REG05 ITERM 充电终止电流，按 64 mA 步长向下取整。
   * @param current_ma 电流值，单位 mA，范围 64-1024。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   */
  bool SetTerminationCurrentLimit(uint16_t current_ma);

  /**
   * @brief 读取 REG05 ITERM 充电终止电流设置。
   * @param current_ma 保存电流值，单位 mA，范围 64-1024；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetTerminationCurrentLimit(uint16_t& current_ma);

  /**
   * @brief 设置 REG06 VREG 充电电压上限，按 16 mV 步长向下取整。
   * @param voltage_mv 充电电压，单位 mV，范围 3840-4608。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   * @note 电压须符合电池规格；具备 JEITA 或 IR 补偿的型号可能据此调整实际电压。
   */
  bool SetChargeVoltageLimit(uint16_t voltage_mv);

  /**
   * @brief 读取 REG06 VREG 的充电电压上限设置。
   * @param voltage_mv 保存电压，单位 mV，范围 3840-4608；失败时保持原值。
   * @return 读取成功返回 true；编码超范围、未就绪或总线访问失败返回 false。
   */
  bool GetChargeVoltageLimit(uint16_t& voltage_mv);

  /**
   * @brief 设置 REG06 BATLOWV，选择从预充电切换到快速充电的电池电压阈值。
   * @param voltage_mv 阈值，单位 mV，仅支持 2800 或 3000。
   * @return 写入成功返回 true；档位不支持、未就绪或总线访问失败返回 false。
   */
  bool SetPrechargeToFastChargeThreshold(uint16_t voltage_mv);

  /**
   * @brief 读取 REG06 BATLOWV 对应的充电阶段切换阈值。
   * @param voltage_mv 保存阈值，单位 mV，为 2800 或 3000；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetPrechargeToFastChargeThreshold(uint16_t& voltage_mv);

  /**
   * @brief 设置 REG06 VRECHG，选择电池低于 VREG 多少电压时重新充电。
   * @param offset_mv 相对 VREG 的下降偏移，单位 mV，仅支持 100 或 200。
   * @return 写入成功返回 true；档位不支持、未就绪或总线访问失败返回 false。
   */
  bool SetRechargeThresholdOffset(uint16_t offset_mv);

  /**
   * @brief 读取 REG06 VRECHG 再充电阈值偏移。
   * @param offset_mv 保存偏移，单位 mV，为 100 或 200；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetRechargeThresholdOffset(uint16_t& offset_mv);

  /**
   * @brief 设置 REG07 EN_TERM 充电终止功能使能。
   * @param enabled true 允许达到终止条件后结束充电，false 禁用终止功能。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool SetChargeTerminationEnabled(bool enabled);

  /**
   * @brief 读取 REG07 EN_TERM 充电终止功能配置。
   * @param enabled true 表示允许充电终止；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetChargeTerminationEnabled(bool& enabled);

  /**
   * @brief 设置 REG07 STAT_DIS，以正向使能语义控制 STAT 引脚功能。
   * @param enabled true 启用 STAT 功能并清零 STAT_DIS，false 禁用该功能。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool SetStatPinEnabled(bool enabled);

  /**
   * @brief 读取 REG07 STAT_DIS 并转换为 STAT 引脚正向使能状态。
   * @param enabled true 表示 STAT 功能已启用；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetStatPinEnabled(bool& enabled);

  /**
   * @brief 设置 REG07 WATCHDOG 看门狗时限，不执行喂狗操作。
   * @param timeout_s 时限，单位秒，仅支持 0、40、80、160；0 表示禁用。
   * @return 写入成功返回 true；档位不支持、未就绪或总线访问失败返回 false。
   * @note 喂狗使用 ResetWatchdogTimer()；看门狗到期会恢复部分寄存器配置。
   */
  bool SetWatchdogTimer(uint16_t timeout_s);

  /**
   * @brief 读取 REG07 WATCHDOG 配置的时限，不返回剩余倒计时。
   * @param timeout_s 保存时限，单位秒，为 0、40、80 或 160；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetWatchdogTimer(uint16_t& timeout_s);

  /**
   * @brief 设置 REG07 EN_TIMER 充电安全定时器使能。
   * @param enabled true 启用充电超时保护，false 禁用该保护。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool SetSafetyTimerEnabled(bool enabled);

  /**
   * @brief 读取 REG07 EN_TIMER 充电安全定时器配置。
   * @param enabled true 表示充电安全定时器已开启；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetSafetyTimerEnabled(bool& enabled);

  /**
   * @brief 设置 REG07 CHG_TIMER 快速充电安全时限。
   * @param duration_hours 时限，单位小时，仅支持 5、8、12 或 20。
   * @return 写入成功返回 true；档位不支持、未就绪或总线访问失败返回 false。
   * @note 预充电安全时限固定为 4 小时，不由该字段修改。
   */
  bool SetFastChargeTimer(uint16_t duration_hours);

  /**
   * @brief 读取 REG07 CHG_TIMER 快速充电时限，不返回剩余时间。
   * @param duration_hours 保存时限，单位小时，为 5、8、12 或
   * 20；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetFastChargeTimer(uint16_t& duration_hours);

  /**
   * @brief 设置 REG07 JEITA_ISET 低温区间的充电电流比例。
   * @param setting 使用 ICHG 的 50% 或 20%，对应手册 JEITA 约 0-10 摄氏度区间。
   * @return 写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note 95/95M/98C 不支持，返回 false。
   */
  bool SetJeitaLowTemperatureCurrent(JeitaLowTemperatureCurrent setting);

  /**
   * @brief 读取 REG07 JEITA_ISET 低温充电电流比例。
   * @param setting 保存 50% 或 20% 档位；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 95/95M/98C 不支持，返回 false。
   */
  bool GetJeitaLowTemperatureCurrent(JeitaLowTemperatureCurrent& setting);

  /**
   * @brief 设置 REG08 BAT_COMP 的 IR 补偿电阻，按 20 毫欧步长向下取整。
   * @param resistance_mohm 电阻，单位毫欧，范围 0-140；0 表示无 IR 电阻补偿。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool SetIrCompensationResistance(uint16_t resistance_mohm);

  /**
   * @brief 读取 REG08 BAT_COMP IR 补偿电阻。
   * @param resistance_mohm 保存电阻，单位毫欧，范围 0-140；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetIrCompensationResistance(uint16_t& resistance_mohm);

  /**
   * @brief 设置 REG08 VCLAMP 的 IR 补偿电压钳位，按 32 mV 步长向下取整。
   * @param voltage_mv 允许高于 VREG 的补偿电压，单位 mV，范围 0-224。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool SetIrCompensationVoltageClamp(uint16_t voltage_mv);

  /**
   * @brief 读取 REG08 VCLAMP 相对于 VREG 的补偿电压上限。
   * @param voltage_mv 保存上限，单位 mV，范围 0-224；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetIrCompensationVoltageClamp(uint16_t& voltage_mv);

  /**
   * @brief 设置 REG08 TREG 芯片热调节温度阈值。
   * @param temperature_c 温度，单位摄氏度，仅支持 60、80、100 或 120。
   * @return 写入成功返回 true；档位不支持、未就绪或总线访问失败返回 false。
   * @note 此参数表示芯片热调节阈值，不是 TS 引脚测得的电池温度。
   */
  bool SetThermalRegulationThreshold(uint16_t temperature_c);

  /**
   * @brief 读取 REG08 TREG 芯片热调节温度阈值。
   * @param temperature_c 保存温度，单位摄氏度，为 60、80、100 或 120。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false，输出保持原值。
   */
  bool GetThermalRegulationThreshold(uint16_t& temperature_c);

  /**
   * @brief 设置 REG09 FORCE_ICO，强制开始输入电流优化。
   * @return 命令写入成功返回 true；ICO 未开启、未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。FORCE_ICO 启动后清零，完成结果见
   * ICO_OPTIMIZED。
   */
  bool ForceInputCurrentOptimization();

  /**
   * @brief 读取 REG14 ICO_OPTIMIZED，检查输入电流优化是否完成。
   * @param complete true 表示已检测到最大输入电流；读取失败时保持原值。
   * @return 型号已就绪且读取成功返回 true，否则返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool IsInputCurrentOptimizationComplete(bool& complete);

  /**
   * @brief 设置 REG09 TMR2X_EN 的安全定时器减速功能。
   * @param enabled true 在输入 DPM 或热调节期间将计时速度减半，false 正常计时。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool SetSafetyTimerSlowdownEnabled(bool enabled);

  /**
   * @brief 读取 REG09 TMR2X_EN 安全定时器减速配置。
   * @param enabled true 表示允许在 DPM 或热调节时减速计时；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetSafetyTimerSlowdownEnabled(bool& enabled);

  /**
   * @brief 设置 REG09 BATFET_DIS，以正向使能语义控制电池到系统的 BATFET。
   * @param enabled true 清零 BATFET_DIS 并允许导通，false 强制关闭 BATFET。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 的字段访问属性存在手册冲突，驱动禁止写入并返回 false。
   *     其他型号设置 false 可切断仅电池供电的 SYS；关闭延迟由 BATFET_DLY 决定。
   */
  bool SetBatfetEnabled(bool enabled);

  /**
   * @brief 读取 REG09 BATFET_DIS 并转换为 BATFET 允许导通配置。
   * @param enabled true 表示未强制关闭
   * BATFET，不保证当前实际导通；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 允许读取该字段，但驱动不允许修改其 BATFET 配置。
   */
  bool GetBatfetEnabled(bool& enabled);

  /**
   * @brief 设置 REG09 JEITA_VSET 高温区间充电电压。
   * @param setting 使用 VREG 减 200 mV 或原 VREG，适用于手册约 45-60
   * 摄氏度区间。
   * @return 写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note 95/95M/98C 不支持，返回 false。
   */
  bool SetJeitaHighTemperatureVoltage(JeitaHighTemperatureVoltage setting);

  /**
   * @brief 读取 REG09 JEITA_VSET 高温充电电压档位。
   * @param setting 保存相对于 VREG 的电压档位；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 95/95M/98C 不支持，返回 false。
   */
  bool GetJeitaHighTemperatureVoltage(JeitaHighTemperatureVoltage& setting);

  /**
   * @brief 设置 REG09 BATFET_DLY，仅配置后续 BATFET 关闭动作的延迟。
   * @param delay kImmediate 立即关闭，k10To15Seconds 延迟约 10-15 秒。
   * @return 写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool SetBatfetTurnOffDelay(BatfetTurnOffDelay delay);

  /**
   * @brief 读取 REG09 BATFET_DLY 的关闭延迟档位。
   * @param delay 保存立即或延迟关闭档位；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetBatfetTurnOffDelay(BatfetTurnOffDelay& delay);

  /**
   * @brief 设置 REG09 BATFET_RST_EN，控制 QON 全系统复位功能，不触发复位。
   * @param enabled true 允许 QON 触发 BATFET 全系统复位，false 禁用该功能。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool SetBatfetResetEnabled(bool enabled);

  /**
   * @brief 读取 REG09 BATFET_RST_EN 的 QON 全系统复位使能。
   * @param enabled true 表示该功能已允许；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetBatfetResetEnabled(bool& enabled);

  /**
   * @brief 同次写入 REG09 BATFET_DLY 和 BATFET_DIS，关闭 BATFET 进入运输模式。
   * @param delay 立即关闭或约 10-15 秒后关闭，默认立即关闭。
   * @return 命令写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。仅电池供电时 SYS
   * 可能断电，成功不表示延迟动作完成。
   */
  bool EnterShipMode(BatfetTurnOffDelay delay = BatfetTurnOffDelay::kImmediate);

  /**
   * @brief 设置 REG09 PUMPX_UP，开始向兼容适配器请求升压的电流脉冲序列。
   * @return 命令写入成功返回 true；EN_PUMPX 未启用、序列忙、未就绪或总线失败
   * 返回 false。
   * @note BQ25898C 不支持，返回 false。不等待适配器电压改变，序列状态见
   * GetPumpxStatus()。
   */
  bool StartPumpxVoltageIncrease();

  /**
   * @brief 设置 REG09 PUMPX_DN，开始向兼容适配器请求降压的电流脉冲序列。
   * @return 命令写入成功返回 true；EN_PUMPX 未启用、序列忙、未就绪或总线失败
   * 返回 false。
   * @note BQ25898C 不支持，返回 false。不等待适配器电压改变，序列状态见
   * GetPumpxStatus()。
   */
  bool StartPumpxVoltageDecrease();

  /**
   * @brief 一次读取 REG09，取得 PUMPX_UP 和 PUMPX_DN 执行状态。
   * @param status 保存两个脉冲序列状态；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetPumpxStatus(PumpxStatus& status);

  /**
   * @brief 设置 REG0A BOOSTV 升压输出电压，按 64 mV 步长向下取整。
   * @param voltage_mv 电压，单位 mV，范围 4550-5510。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool SetBoostVoltage(uint16_t voltage_mv);

  /**
   * @brief 读取 REG0A BOOSTV 输出电压设置，不测量实际 VBUS 电压。
   * @param voltage_mv 保存设置电压，单位 mV，范围 4550-5510；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 不支持，返回 false。
   */
  bool GetBoostVoltage(uint16_t& voltage_mv);

  /**
   * @brief 设置 REG0A PFM_OTG_DIS，以正向使能语义控制升压 PFM 模式。
   * @param enabled true 允许 PFM，false 禁止 PFM 并仅使用 PWM。
   * @return 写入成功返回 true；未就绪或总线访问失败返回 false。
   * @note 90/92/95/98C 不支持，返回 false。
   */
  bool SetBoostPfmEnabled(bool enabled);

  /**
   * @brief 读取 REG0A PFM_OTG_DIS 并转换为 PFM 允许状态。
   * @param enabled true 表示允许 PFM，不表示正在使用 PFM；失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 90/92/95/98C 不支持，返回 false。
   */
  bool GetBoostPfmEnabled(bool& enabled);

  /**
   * @brief 设置 REG0A BOOST_LIM 升压输出电流限制。
   * @param current_ma 限流值，单位 mA，精确匹配型号档位：96 为
   *     500/750/1200/1400/1650/1875/2150；90/90H/92 额外支持 2450；
   *     98/98D 为 500/800/1000/1200/1500/1800/2100/2400。
   * @return 写入成功返回 true；档位不支持、未就绪或总线访问失败返回 false。
   * @note 95/95M/98C 不支持，返回 false。96 的编码 7 保留。
   */
  bool SetBoostCurrentLimit(uint16_t current_ma);

  /**
   * @brief 按型号档位表读取 REG0A BOOST_LIM 升压限流设置。
   * @param current_ma 保存限流，单位 mA；档位见
   * SetBoostCurrentLimit()，失败时保持原值。
   * @return 读取成功返回 true；型号不支持、保留编码、未就绪或总线失败返回
   * false。
   * @note 95/95M/98C 不支持；96 的编码 7 保留，90/90H/92 和 98/98D
   * 使用各自八档表。
   */
  bool GetBoostCurrentLimit(uint16_t& current_ma);

  /**
   * @brief 读取 REG0B 输入来源、充电阶段、电源有效和系统调节状态。
   * @param status 保存解析结果及原始字节；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note PSEL 与 USB CDP 的原始码 2 分别解析为 kAdapter 与 kUsbCdp。
   *     SDP_STAT 仅 95 且输入为 SDP 时有效；不消耗 REG0C 锁存，未知编码返回
   * kUnknown。
   */
  bool GetChipStatus(ChipStatus& status);

  /**
   * @brief 读取 BQ25895 SDP_STAT，取得当前 USB SDP 的 USB100/USB500 检测结果。
   * @param current_ma 保存 100 或 500，单位 mA，失败时保持原值；不是 IINLIM
   * 设置值。
   * @return 型号为 95 且 VBUS_STAT=SDP、读取成功返回 true，否则返回 false。
   * @note 其他输入类型下 SDP_STAT 固定为 1，不能据此判断是否检测到 SDP。
   */
  bool GetUsbSdpCurrentLimit(uint16_t& current_ma);

  /**
   * @brief 单次读取 REG0C，取得并消耗锁存故障；NTC_FAULT 始终为实时状态。
   * @param status 保存故障信息及原始字节；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 98C 的 boost_fault_valid 和 ntc_fault_valid 为
   * false，不将保留位解释为故障。 95M 未定义的升压 NTC 编码保留
   * kUnknown；不要拆成多次字段读取。
   */
  bool GetFaultStatus(FaultStatus& status);

  /**
   * @brief 连续执行两次 REG0C 单字节读取，取得历史锁存和当前故障。
   * @param latched 第一次读取的锁存故障；必须与 current 为不同对象。
   * @param current 第二次读取的当前故障；两个输出在整体失败时均保持原值。
   * @return 两次读取均成功返回 true；对象重复、未就绪或总线失败返回 false。
   * @note 第二次读取失败不能撤销第一次对锁存的消耗。两次均为单字节读取；
   *     NTC 有效性和未知编码处理同单输出版本，有效 NTC 字段始终反映当前状态。
   */
  bool GetFaultStatus(FaultStatus& latched, FaultStatus& current);

  /**
   * @brief 设置 REG0D FORCE_VINDPM，相对或绝对输入电压阈值模式。
   * @param mode kRelative 由芯片计算阈值，kAbsolute 由主机设置阈值。
   * @return 写入成功返回 true；枚举无效、未就绪或总线访问失败返回 false。
   * @note VBUS 重新插入可能将模式和 VINDPM 阈值恢复为默认值。
   */
  bool SetVindpmMode(VindpmMode mode);

  /**
   * @brief 读取 REG0D FORCE_VINDPM 对应的阈值设置模式。
   * @param mode 保存相对或绝对模式；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool GetVindpmMode(VindpmMode& mode);

  /**
   * @brief 同次写入 REG0D，选择绝对模式并设置 VINDPM，按 100 mV 向下取整。
   * @param voltage_mv 绝对输入电压阈值，单位 mV，范围 3900-15300。
   * @return 写入成功返回 true；超出范围、未就绪或总线访问失败返回 false。
   * @note 编码偏移是 2600 mV，不是最小允许阈值 3900 mV。
   */
  bool SetAbsoluteVindpmThreshold(uint16_t voltage_mv);

  /**
   * @brief 读取 REG0D VINDPM 的当前阈值，适用于相对模式和绝对模式。
   * @param voltage_mv 保存电压，单位 mV，范围 3900-15300；失败时保持原值。
   * @return 读取成功返回 true；编码低于有效阈值、未就绪或总线失败返回 false。
   */
  bool GetVindpmThreshold(uint16_t& voltage_mv);

  /**
   * @brief 读取 REG0E BATV 的最近电池电压 ADC 结果，不启动转换。
   * @param voltage_mv 保存电压，单位 mV，按 2304 + code * 20 换算，范围
   * 2304-4844。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false，输出保持原值。
   * @note 手册表格上限 4848 mV 与位权公式不一致，此处使用公式所得 4844 mV。
   */
  bool GetBatteryVoltage(uint16_t& voltage_mv);

  /**
   * @brief 读取 REG0F SYSV 的最近系统电压 ADC 结果，不启动转换。
   * @param voltage_mv 保存电压，单位 mV，按 2304 + code * 20 换算，范围
   * 2304-4844。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false，输出保持原值。
   */
  bool GetSystemVoltage(uint16_t& voltage_mv);

  /**
   * @brief 读取 REG10 TSPCT 的最近 TS 电压比例，不直接转换为摄氏温度。
   * @param percentage 保存 VTS/VREGN * 100，按 21 + code * 0.465 换算，
   *     编码范围对应约 21%-80.055%；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note BQ25898C 没有 TS ADC，不访问 REG10 并返回 false。
   */
  bool GetTsVoltagePercentage(float& percentage);

  /**
   * @brief 读取 REG11 VBUSV 的最近输入电压 ADC 结果，不启动转换。
   * @param voltage_mv 保存电压，单位 mV，按 2600 + code * 100 换算，范围
   * 2600-15300。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false，输出保持原值。
   * @note 未接入 VBUS 时也返回寄存器编码值，须用 IsVbusAttached()
   * 判断是否接入。
   */
  bool GetVbusVoltage(uint16_t& voltage_mv);

  /**
   * @brief 读取 REG12 ICHGR 的最近充电电流 ADC 结果，不测量放电电流。
   * @param current_ma 保存电流，单位 mA，按 code * 50 换算，范围 0-6350。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false，输出保持原值。
   * @note VBAT 低于手册 VBATSHORT 时硬件返回 0；该结果不是 ICHG 限流配置。
   */
  bool GetChargeCurrent(uint16_t& current_ma);

  /**
   * @brief 读取 REG0E THERM_STAT 芯片热调节状态。
   * @param active true 表示热调节正在进行；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool IsThermalRegulationActive(bool& active);

  /**
   * @brief 读取 REG11 VBUS_GD，判断是否检测到 VBUS 接入。
   * @param attached true 表示已接入 VBUS；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   */
  bool IsVbusAttached(bool& attached);

  /**
   * @brief 读取最近 ADC 电压、电流、可用的 TS 比例及相关状态。
   * @param measurements 保存 ADC 结果，单位和换算见
   * AdcMeasurements；失败时保持原值。
   * @return 整组读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 98C 分别读取 REG0E-REG0F 和 REG11-REG12，跳过未定义的 REG10，
   *     ts_voltage_valid 为 false；其他型号突发读取 REG0E-REG12。
   *     不读取 REG0C，不启动转换，也不保证各字段来自同一采样时刻。
   */
  bool GetAdcMeasurements(AdcMeasurements& measurements);

  /**
   * @brief 读取 REG13 的电压/电流 DPM 状态和输入限流字段。
   * @param status 保存 DPM 状态和 IDPM_LIM 原码；限流有效时为 100-3250 mA，
   *     无效时为 0；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪或总线访问失败返回 false。
   * @note 有 ICO 的型号在 ICO 启用时报告优化限流；98C 无 ICO，且手册未给出
   *     IDPM_LIM 编码偏移，因此仅保留原码，input_current_limit_valid 为 false。
   */
  bool GetDpmStatus(DpmStatus& status);

 private:
  // 隐藏会绕过型号检查和自清零命令位处理的原始写序列入口。
  using I2cChipBase::InitSequence;

  // BQ2589x 系列寄存器地址，具体字段由型号决定；保留字段不提供配置接口。
  enum class Register : uint8_t {
    kReg00 = 0x00,  // 输入高阻态、ILIM 引脚和输入电流限制。
    kReg01 = 0x01,  // VINDPM 偏移及型号相关的升压温度、DP/DM DAC 和 EN_12V。
    kReg02 = 0x02,  // ADC、升压频率、ICO 与 PSEL 或 D+/D- 输入检测。
    kReg03 =
        0x03,  // 喂狗、OTG、充电、SYS_MIN 及 BAT_LOADEN/FORCE_DSEL/VOK_OTG_EN。
    kReg04 = 0x04,  // PumpX 使能和快速充电电流。
    kReg05 = 0x05,  // 预充电电流和终止电流。
    kReg06 = 0x06,  // 充电电压、预充电切换阈值和再充电偏移。
    kReg07 = 0x07,  // 终止、STAT、看门狗、安全定时器与 JEITA 低温电流。
    kReg08 = 0x08,  // IR 补偿和热调节阈值。
    kReg09 = 0x09,  // ICO 命令、BATFET、JEITA 高温电压和 PumpX 命令。
    kReg0a = 0x0A,  // 升压电压、PFM 和电流限制。
    kReg0b = 0x0B,  // 普通芯片状态，只读。
    kReg0c = 0x0C,  // 故障状态，只读且具有读取清除锁存语义。
    kReg0d = 0x0D,  // VINDPM 模式和绝对电压阈值。
    kReg0e = 0x0E,  // 热调节状态和电池电压 ADC。
    kReg0f = 0x0F,  // 系统电压 ADC。
    kReg10 = 0x10,  // TS 电压比例 ADC，BQ25898C 未定义此地址。
    kReg11 = 0x11,  // VBUS 接入状态和电压 ADC。
    kReg12 = 0x12,  // 充电电流 ADC。
    kReg13 = 0x13,  // 输入动态功率管理状态。
    kReg14 = 0x14,  // 寄存器复位、ICO 完成与器件身份。
  };

  // 各型号的身份和可选功能；量程及字段编码由型号函数实现。
  struct ModelConfig {
    ChipModel model;           // 具体芯片型号。
    uint8_t address;           // 固定的 7 位 I2C 地址。
    uint8_t part_number;       // REG14 PN 编码。
    uint8_t device_revision;   // REG14 DEV_REV 编码。
    uint32_t feature_mask;     // Feature 枚举对应的能力位。
    bool jeita_profile_valid;  // REG14 TS_PROFILE 是否有定义。
  };

  // 内部型号接口；固定信息在构造时保存，实际操作由各型号的 .cpp 实现。
  class ModelDriver {
   public:
    /**
     * @brief 保存当前型号的只读身份和可选功能，不访问硬件。
     * @param model 具体芯片型号。
     * @param address 固定的 7 位 I2C 地址。
     * @param part_number REG14 PN 编码。
     * @param device_revision REG14 DEV_REV 编码。
     * @param features 型号支持的功能列表，构造时转换为位掩码，不保存列表引用。
     * @param jeita_profile_valid TS_PROFILE 是否有效，默认 true；98C 传入
     * false。
     */
    ModelDriver(ChipModel model, uint8_t address, uint8_t part_number,
        uint8_t device_revision, std::initializer_list<Feature> features,
        bool jeita_profile_valid = true);

    /**
     * @brief 销毁内部型号接口，不访问硬件。
     */
    virtual ~ModelDriver() = default;

    // 仅供内部读取的型号固定信息，在型号实例整个生命周期内保持不变。
    const ModelConfig config_;

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    virtual bool Init(Bq2589x& chip) const = 0;

    /**
     * @brief 按当前型号的 VINDPM_OS 或 VDPM_OS 编码设置输入电压偏移。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param offset_mv 输入电压偏移，单位 mV，允许范围由型号决定。
     * @return 设置成功返回 true；参数无效或总线失败返回 false。
     */
    virtual bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const = 0;

    /**
     * @brief 按当前型号的编码读取输入电压偏移。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param offset_mv 保存偏移量，单位 mV；失败时保持原值。
     * @return 读取和解码成功返回 true，否则返回 false。
     */
    virtual bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const = 0;

    /**
     * @brief 按当前型号的 ICHG 掩码和上限设置快速充电电流。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param current_ma 电流上限，单位 mA，按 64 mA 步长向下取整。
     * @return 设置成功返回 true；参数越界或总线失败返回 false。
     */
    virtual bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const = 0;

    /**
     * @brief 按当前型号的 ICHG 掩码和上限读取快速充电电流。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param current_ma 保存电流上限，单位 mA；失败时保持原值。
     * @return 读取成功且编码有效返回 true，否则返回 false。
     */
    virtual bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const = 0;

    /**
     * @brief 按当前型号的 BOOST_LIM 档位设置升压电流限制。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param current_ma 电流上限，单位 mA，必须精确匹配有效档位。
     * @return 设置成功返回 true；不支持、档位无效或总线失败返回 false。
     * @note 默认实现不访问硬件并返回 false，支持此功能的型号覆写。
     */
    virtual bool SetBoostCurrentLimit(Bq2589x& chip, uint16_t current_ma) const;

    /**
     * @brief 按当前型号的 BOOST_LIM 档位读取升压电流限制。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param current_ma 保存电流上限，单位 mA；失败时保持原值。
     * @return 读取成功返回 true；不支持、保留编码或总线失败返回 false。
     * @note 默认实现不访问硬件并返回 false，支持此功能的型号覆写。
     */
    virtual bool GetBoostCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const;

    /**
     * @brief 检查当前型号的档位和输入状态后配置 DP/DM DAC。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param dplus true 配置 DP，false 配置 DM。
     * @param voltage 规范化电压档位，短接档位由型号和引脚决定。
     * @return 设置成功返回 true；不支持、条件不满足或总线失败返回 false。
     * @note 默认实现不访问硬件并返回 false，90H 和 98D 覆写。
     */
    virtual bool SetDpDmDac(
        Bq2589x& chip, bool dplus, DpDmVoltage voltage) const;

    /**
     * @brief 读取当前型号的 DP/DM DAC 并规范化电压档位。
     * @param chip 已确认型号并初始化的芯片实例。
     * @param dplus true 读取 DP，false 读取 DM。
     * @param voltage 保存电压档位；失败时保持原值。
     * @return 读取成功返回 true；不支持、保留编码或总线失败返回 false。
     * @note 默认实现不访问硬件并返回 false，90H 和 98D 覆写。
     */
    virtual bool GetDpDmDac(
        Bq2589x& chip, bool dplus, DpDmVoltage& voltage) const;

    /**
     * @brief 解析当前型号的 REG0B 芯片状态，不访问总线。
     * @param value REG0B 完整原始字节。
     * @return 规范化状态；未知输入类型表示为 kUnknown。
     * @note 默认使用 USB 输入类型编码；PSEL 型号和 95 的 SDP 状态由型号覆写。
     */
    virtual ChipStatus DecodeChipStatus(uint8_t value) const;

    /**
     * @brief 解析当前型号的 REG13 DPM 状态，不访问总线。
     * @param value REG13 完整原始字节。
     * @return DPM 状态及输入限流原码，物理量有效性由型号决定。
     * @note 默认使用 100 mA 偏移和 50 mA 步长；98C 仅报告原码。
     */
    virtual DpmStatus DecodeDpmStatus(uint8_t value) const;

    /**
     * @brief 读取当前型号的 ADC 寄存器，不启动转换。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param data 按 REG0E-REG12 顺序保存原始字节；失败时可能部分更新。
     * @return 所需寄存器全部读取成功返回 true，否则返回 false。
     * @note 默认连续读取五字节；辅助充电器覆写此函数以跳过未定义寄存器。
     */
    virtual bool ReadAdcRegisters(Bq2589x& chip, uint8_t (&data)[5]) const;

    /**
     * @brief 按当前型号的 NTC_FAULT 编码解释温度故障。
     * @param code REG0C NTC_FAULT 原始编码，范围 0-7。
     * @return 规范化故障状态；未定义的编码返回 kUnknown。
     * @note 默认使用 JEITA 编码，其他温度监测方案由具体型号覆写。
     */
    virtual NtcFault DecodeNtcFault(uint8_t code) const;
  };

  /**
   * @brief 将型号支持的功能列表转换为能力掩码。
   * @param features 当前型号手册明确支持的 Feature 列表。
   * @return 可由 HasFeature 查询的功能位掩码。
   */
  static uint32_t MakeFeatureMask(std::initializer_list<Feature> features);

  /**
   * @brief 获取全部已实现型号的静态注册表，不访问总线。
   * @param count 返回注册表中的型号数量。
   * @return 生命周期覆盖整个程序的只读型号接口数组。
   */
  static const ModelDriver* const* GetModelDrivers(size_t& count);

  /**
   * @brief 根据 ChipModel 查找内部型号实现，不读取硬件身份。
   * @param model 要查找的具体型号。
   * @return 已实现型号的静态接口指针；未知或无效型号返回 nullptr。
   */
  static const ModelDriver* GetModelDriver(ChipModel model);

  // BQ25890 的型号实现，定义位于 bq25890.cpp。
  class Bq25890Driver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25890 的身份和可选功能，不访问硬件。
     */
    Bq25890Driver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VINDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，范围 0-3100，按 100 mV 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VINDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，范围 0-3100；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-5056，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-5056；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 按本型号的档位表设置 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，必须精确匹配 kBoostCurrentLimitsMa 中的档位。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetBoostCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 按本型号的档位表读取 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存限流，单位 mA；读取失败或保留编码时保持原值。
     * @return 读取成功且编码有效返回 true，否则返回 false。
     */
    bool GetBoostCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;
  };

  // BQ25890H 的型号实现，定义位于 bq25890h.cpp。
  class Bq25890hDriver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25890H 的身份和可选功能，不访问硬件。
     */
    Bq25890hDriver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VINDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，仅支持 400 或 600，精确匹配。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VINDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，为 400 或 600；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-5056，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-5056；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 按本型号的档位表设置 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，必须精确匹配 kBoostCurrentLimitsMa 中的档位。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetBoostCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 按本型号的档位表读取 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存限流，单位 mA；读取失败或保留编码时保持原值。
     * @return 读取成功且编码有效返回 true，否则返回 false。
     */
    bool GetBoostCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 验证输入状态后设置 REG01 DP_DAC 或 DM_DAC。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param dplus true 写入 D+ 档位，false 写入 D- 档位。
     * @param voltage 高阻或 0/600/1200/2000/2700/3300 mV 档位；不支持编码 7。
     * @return 编码有效、输入检测就绪且写入成功返回 true，否则返回 false。
     */
    bool SetDpDmDac(
        Bq2589x& chip, bool dplus, DpDmVoltage voltage) const override;

    /**
     * @brief 读取 REG01 DP_DAC 或 DM_DAC 的输出配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param dplus true 读取 D+ 档位，false 读取 D- 档位。
     * @param voltage 保存规范化档位；失败时保持原值。
     * @return 读取成功且编码不为保留值 7 时返回 true，否则返回 false。
     */
    bool GetDpDmDac(
        Bq2589x& chip, bool dplus, DpDmVoltage& voltage) const override;
  };

  // BQ25892 的型号实现，定义位于 bq25892.cpp。
  class Bq25892Driver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25892 的身份和可选功能，不访问硬件。
     */
    Bq25892Driver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VINDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，范围 0-3100，按 100 mV 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VINDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，范围 0-3100；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-5056，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-5056；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 按本型号的档位表设置 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，必须精确匹配 kBoostCurrentLimitsMa 中的档位。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetBoostCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 按本型号的档位表读取 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存限流，单位 mA；读取失败或保留编码时保持原值。
     * @return 读取成功且编码有效返回 true，否则返回 false。
     */
    bool GetBoostCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 解析 REG0B 芯片状态，按 PSEL 输入编码解释 VBUS_STAT。
     * @param value 完整 REG0B 原始字节。
     * @return 公共状态及 PSEL 输入来源；编码 2 为适配器，3-6 为未知。
     */
    ChipStatus DecodeChipStatus(uint8_t value) const override;
  };

  // BQ25895 的型号实现，定义位于 bq25895.cpp。
  class Bq25895Driver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25895 的身份和可选功能，不访问硬件。
     */
    Bq25895Driver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VINDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，范围 0-3100，按 100 mV 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VINDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，范围 0-3100；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-5056，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-5056；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 解析 REG0B 芯片状态和 BQ25895 专有 SDP_STAT 字段。
     * @param value 完整 REG0B 原始字节。
     * @return 公共状态；仅 SDP 输入时报告 USB100/USB500 的检测结果。
     */
    ChipStatus DecodeChipStatus(uint8_t value) const override;

    /**
     * @brief 按 SLUSC88C 解码充电和升压模式的 NTC_FAULT。
     * @param code REG0C NTC_FAULT 原码，范围 0-7。
     * @return 规范化状态；未定义编码返回 kUnknown。
     */
    NtcFault DecodeNtcFault(uint8_t code) const override;
  };

  // BQ25895M 的型号实现，定义位于 bq25895m.cpp。
  class Bq25895mDriver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25895M 的身份和可选功能，不访问硬件。
     */
    Bq25895mDriver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VINDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，范围 0-3100，按 100 mV 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VINDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，范围 0-3100；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-5056，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-5056；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 按 SLUSCC8C 已定义的充电模式编码解析 NTC_FAULT。
     * @param code REG0C NTC_FAULT 原码，范围 0-7。
     * @return 规范化状态；手册未定义的升压等编码返回 kUnknown。
     */
    NtcFault DecodeNtcFault(uint8_t code) const override;
  };

  // BQ25896 的型号实现，定义位于 bq25896.cpp。
  class Bq25896Driver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25896 的身份和可选功能，不访问硬件。
     */
    Bq25896Driver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VINDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，范围 0-3100，按 100 mV 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VINDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，范围 0-3100；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-3008，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-3008；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 按本型号的档位表设置 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，必须精确匹配 kBoostCurrentLimitsMa 中的档位。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetBoostCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 按本型号的档位表读取 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存限流，单位 mA；读取失败或保留编码时保持原值。
     * @return 读取成功且编码有效返回 true，否则返回 false。
     */
    bool GetBoostCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 解析 REG0B 芯片状态，按 PSEL 输入编码解释 VBUS_STAT。
     * @param value 完整 REG0B 原始字节。
     * @return 公共状态及 PSEL 输入来源；编码 2 为适配器，3-6 为未知。
     */
    ChipStatus DecodeChipStatus(uint8_t value) const override;
  };

  // BQ25898 的型号实现，定义位于 bq25898.cpp。
  class Bq25898Driver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25898 的身份和可选功能，不访问硬件。
     */
    Bq25898Driver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，仅支持 400 或 600，精确匹配。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，为 400 或 600；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-4032，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-4032；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 按本型号的档位表设置 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，必须精确匹配 kBoostCurrentLimitsMa 中的档位。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetBoostCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 按本型号的档位表读取 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存限流，单位 mA；读取失败或保留编码时保持原值。
     * @return 读取成功且编码有效返回 true，否则返回 false。
     */
    bool GetBoostCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 解析 REG0B 芯片状态，按 PSEL 输入编码解释 VBUS_STAT。
     * @param value 完整 REG0B 原始字节。
     * @return 公共状态及 PSEL 输入来源；编码 2 为适配器，3-6 为未知。
     */
    ChipStatus DecodeChipStatus(uint8_t value) const override;
  };

  // BQ25898C 的型号实现，定义位于 bq25898c.cpp。
  class Bq25898cDriver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25898C 的身份和可选功能，不访问硬件。
     */
    Bq25898cDriver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，仅支持 400 或 600，精确匹配。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，为 400 或 600；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-3008，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-3008；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 解析 REG0B 芯片状态，按 PSEL 输入编码解释 VBUS_STAT。
     * @param value 完整 REG0B 原始字节。
     * @return 公共状态及 PSEL 输入来源；编码 2 为适配器，3-7 为未知。
     */
    ChipStatus DecodeChipStatus(uint8_t value) const override;

    /**
     * @brief 解析 BQ25898C REG13 状态，保留 IDPM_LIM 原码。
     * @param value 完整 REG13 原始字节。
     * @return DPM 状态和原码；手册未给出限流编码偏移，mA 无效且保持 0。
     */
    DpmStatus DecodeDpmStatus(uint8_t value) const override;

    /**
     * @brief 分段读取 BQ25898C ADC 结果，跳过未定义的 REG10。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param data REG0E-REG12 对应的五字节缓冲区，索引 2 保持原值。
     * @return 两段读取均成功返回 true；失败时缓冲区可能已部分更新。
     */
    bool ReadAdcRegisters(Bq2589x& chip, uint8_t (&data)[5]) const override;

    /**
     * @brief 保持 BQ25898C 未定义的 NTC 字段为未知状态。
     * @param code 未使用；此型号没有 TS 引脚，REG0C 低三位保留。
     * @return 始终返回 kUnknown。
     */
    NtcFault DecodeNtcFault(uint8_t code) const override;
  };

  // BQ25898D 的型号实现，定义位于 bq25898d.cpp。
  class Bq25898dDriver final : public ModelDriver {
   public:
    /**
     * @brief 初始化 BQ25898D 的身份和可选功能，不访问硬件。
     */
    Bq25898dDriver();

    /**
     * @brief 执行当前型号的寄存器初始化序列。
     * @param chip 已初始化总线并确认型号的芯片实例。
     * @return 初始化序列成功返回 true，寄存器写入失败返回 false。
     */
    bool Init(Bq2589x& chip) const override;

    /**
     * @brief 设置 REG01 VDPM_OS 相对输入电压限制偏移。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 单位 mV，仅支持 400 或 600，精确匹配。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t offset_mv) const override;

    /**
     * @brief 读取 REG01 VDPM_OS 的原始偏移设置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param offset_mv 保存偏移，单位 mV，为 400 或 600；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetInputVoltageLimitOffset(
        Bq2589x& chip, uint16_t& offset_mv) const override;

    /**
     * @brief 设置 REG04 ICHG 快速充电电流上限。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，范围 0-4032，按 64 mA 步长向下取整。
     * @return 范围有效且写入成功返回 true，否则返回 false。
     */
    bool SetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 读取 REG04 ICHG 快速充电电流配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存电流上限，单位 mA，范围 0-4032；失败时保持原值。
     * @return 读取成功且编码在型号范围内返回 true，否则返回 false。
     */
    bool GetFastChargeCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 按本型号的档位表设置 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 单位 mA，必须精确匹配 kBoostCurrentLimitsMa 中的档位。
     * @return 档位匹配且写入成功返回 true，否则返回 false。
     */
    bool SetBoostCurrentLimit(
        Bq2589x& chip, uint16_t current_ma) const override;

    /**
     * @brief 按本型号的档位表读取 REG0A BOOST_LIM 升压限流。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param current_ma 保存限流，单位 mA；读取失败或保留编码时保持原值。
     * @return 读取成功且编码有效返回 true，否则返回 false。
     */
    bool GetBoostCurrentLimit(
        Bq2589x& chip, uint16_t& current_ma) const override;

    /**
     * @brief 验证输入状态后设置 REG01 DPLUS_DAC 或 DMINUS_DAC。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param dplus true 写入 D+ 档位，false 写入 D- 档位。
     * @param voltage 高阻或 0/600/1200/2000/2700/3300 mV 档位；D+ 另支持短接
     * D+/D-。
     * @return 编码有效、输入检测就绪且写入成功返回 true，否则返回 false。
     */
    bool SetDpDmDac(
        Bq2589x& chip, bool dplus, DpDmVoltage voltage) const override;

    /**
     * @brief 读取 REG01 DPLUS_DAC 或 DMINUS_DAC 的输出配置。
     * @param chip 已初始化并完成型号检查的充电芯片对象。
     * @param dplus true 读取 D+ 档位，false 读取 D- 档位。
     * @param voltage 保存规范化档位，D- 编码 7 转换为 k3300Mv；失败时保持原值。
     * @return 读取成功返回 true，否则返回 false。
     */
    bool GetDpDmDac(
        Bq2589x& chip, bool dplus, DpDmVoltage& voltage) const override;
  };

  /**
   * @brief 读取单字节寄存器，REG14 可在型号确认前用于识别。
   * @param reg 目标寄存器地址。
   * @param value 保存原始字节；读取失败时保持原值。
   * @return 读取成功返回 true；未就绪、总线失败或访问 98C 未定义的 REG10 返回
   * false。
   */
  bool ReadRegister(Register reg, uint8_t& value);

  /**
   * @brief 连续读取寄存器，并记录访问失败信息
   * @param reg 起始寄存器地址
   * @param data 接收缓冲区；失败时可能部分更新
   * @param length 读取字节数，不得跨越未定义寄存器
   * @return 读取成功返回true，否则返回false
   */
  bool ReadRegister(Register reg, uint8_t* data, size_t length);

  /**
   * @brief 读取寄存器并提取掩码覆盖的位字段。
   * @param reg 目标寄存器地址。
   * @param mask 字段在寄存器内的位掩码，由内部调用者保证有效。
   * @param shift 掩码最低有效位位置，范围 0-7。
   * @param value 保存右移对齐后的字段值；读取失败时保持原值。
   * @return 寄存器读取成功返回 true，否则返回 false。
   */
  bool ReadField(Register reg, uint8_t mask, uint8_t shift, uint8_t& value);

  /**
   * @brief 读取寄存器标志并按需反转为正向使能语义。
   * @param reg 目标寄存器地址。
   * @param mask 标志位掩码，由内部调用者保证有效。
   * @param value 保存布尔状态；读取失败时保持原值。
   * @param inverted true 反转原位值，false 保持原位值。
   * @return 寄存器读取成功返回 true，否则返回 false。
   */
  bool ReadFlag(Register reg, uint8_t mask, bool& value, bool inverted = false);

  /**
   * @brief 对可写字段执行读改写，保留无关位并避免重放未清零的命令位。
   * @param reg 由功能函数按型号选择的目标寄存器；REG14 用于写入 REG_RST。
   * @param mask 由功能函数按字段定义指定的非零位掩码。
   * @param value 已移位的数据，不能包含 mask 以外的位。
   * @return 写入成功返回 true；未就绪、掩码或数据无效、总线失败返回 false。
   * @note REG14 直接构造复位命令，不把身份和 ICO 只读状态写回；其他寄存器的
   * 自清零命令位在未被本次明确设置时写 0，不因读到 1 而再次触发。
   */
  bool UpdateRegisterBits(Register reg, uint8_t mask, uint8_t value);

  /**
   * @brief 检查物理量范围并向下量化为线性寄存器字段后写入。
   * @param reg 目标寄存器地址。
   * @param mask 目标字段位掩码。
   * @param shift 字段左移位数，范围 0-7。
   * @param value 要设置的物理量，单位由具体字段决定。
   * @param minimum 最小值，同时作为编码偏移。
   * @param maximum 最大允许值，与 minimum、value 使用相同单位。
   * @param step 非零量化步长，由内部调用者保证有效。
   * @return 写入成功返回 true；value 越界或寄存器更新失败返回 false。
   */
  bool SetLinearField(Register reg, uint8_t mask, uint8_t shift, uint16_t value,
      uint16_t minimum, uint16_t maximum, uint16_t step);

  /**
   * @brief 读取线性字段并按 minimum + code * step 还原物理量。
   * @param reg 目标寄存器地址。
   * @param mask 字段位掩码。
   * @param shift 字段右移位数，范围 0-7。
   * @param minimum 编码为 0 时对应的物理量。
   * @param maximum 最大有效物理量。
   * @param step 每个编码步长对应的物理量。
   * @param value 保存换算结果；读取或范围检查失败时保持原值。
   * @return 读取成功且换算值未超过 maximum 返回 true，否则返回 false。
   */
  bool GetLinearField(Register reg, uint8_t mask, uint8_t shift,
      uint16_t minimum, uint16_t maximum, uint16_t step, uint16_t& value);

  /**
   * @brief 精确匹配离散物理量表，将表下标写入目标字段。
   * @param reg 目标寄存器地址。
   * @param mask 字段位掩码。
   * @param shift 表下标写入前左移的位数，范围 0-7。
   * @param value 要设置的物理量，单位与 table 一致。
   * @param table 有效离散档位表，内部调用者保证非空且生命周期覆盖调用。
   * @param count 表内有效元素个数，编码必须能装入目标字段。
   * @return 找到精确档位且写入成功返回 true；无匹配或寄存器更新失败返回 false。
   */
  bool SetTableField(Register reg, uint8_t mask, uint8_t shift, uint16_t value,
      const uint16_t* table, size_t count);

  /**
   * @brief 读取字段编码，以其为下标从离散档位表取得物理量。
   * @param reg 目标寄存器地址。
   * @param mask 字段位掩码。
   * @param shift 字段右移位数，范围 0-7。
   * @param table 有效档位表，内部调用者保证非空。
   * @param count 表内有效元素个数，不包含保留编码。
   * @param value 保存表中的物理量；读取失败或编码越界时保持原值。
   * @return 读取成功且编码小于 count 返回 true，否则返回 false。
   */
  bool GetTableField(Register reg, uint8_t mask, uint8_t shift,
      const uint16_t* table, size_t count, uint16_t& value);

  /**
   * @brief 轮询等待寄存器指定掩码中的全部位清零。
   * @param reg 目标寄存器地址。
   * @param mask 要等待清零的位掩码。
   * @param timeout_ms 轮询预算，单位 ms；0 表示只进行立即状态检查。
   * @return 观察到清零返回 true；寄存器读取失败或超时返回 false。
   * @note 兼容底层计时器回绕；单次 I2C 访问可能超过轮询预算，不会取消硬件命令。
   */
  bool WaitForClear(Register reg, uint8_t mask, uint32_t timeout_ms);

  /**
   * @brief 检查 EN_PUMPX 与执行状态后启动一个方向的 PumpX 脉冲序列。
   * @param increase true 触发 PUMPX_UP，false 触发 PUMPX_DN。
   * @return 命令写入成功返回 true；功能未开启、序列忙或总线失败返回 false。
   */
  bool StartPumpx(bool increase);

  /**
   * @brief 从 REG14 原始字节解析身份信息，不访问总线。
   * @param value 完整 REG14 原始字节。
   * @return 包含 PN、修订号、温度配置及识别型号的结构体；未知组合保持
   * kUnknown。
   */
  ChipInfo DecodeChipInfo(uint8_t value) const;

  /**
   * @brief 从 REG0C 原始字节解析故障信息，不读取或清除硬件锁存。
   * @param value 已读取的完整 REG0C 字节。
   * @return 故障结构体，保留原始字节；未知 NTC 编码表示为 kUnknown。
   */
  FaultStatus DecodeFaultStatus(uint8_t value) const;

  /**
   * @brief 检查型号和输入检测状态后写入 DP 或 DM 输出电压档位。
   * @param dplus true 写 DP_DAC，false 写 DM_DAC。
   * @param voltage 规范化档位；短接模式仅适用于 BQ25898D 的 DP_DAC。
   * @return 写入成功返回 true；不支持、档位无效、输入未就绪或总线失败返回
   * false。
   */
  bool SetDpDmDac(bool dplus, DpDmVoltage voltage);

  /**
   * @brief 读取输入检测和电源状态，判断当前是否允许主机写入 DP/DM DAC。
   * @return 输入检测已结束、PG 有效且输入既非断开也非 OTG 时返回 true；
   * 条件不满足或总线失败返回 false。
   * @note 由支持 DAC 的型号在检查目标档位有效后调用，不修改寄存器。
   */
  bool IsDpDmDacReady();

  // 板级指定型号；kUnknown 表示使用自动识别结果。
  ChipModel requested_model_;
  // 此对象探测的固定地址，不在初始化时扫描其他器件地址。
  int16_t device_address_;
  // Init() 缓存的识别结果，不代替后续硬件状态读取。
  ChipInfo chip_info_;
  // 型号确认后选择的静态实现；初始化前及解除初始化后为 nullptr。
  const ModelDriver* model_driver_ = nullptr;
  // 型号已确认且驱动初始化完成。
  bool initialized_ = false;
  // 总线设备包装已初始化，允许读取身份寄存器。
  bool bus_initialized_ = false;
  // 总线仍需清理；Deinit(false) 后可在后续 Deinit(true) 中释放自有总线。
  bool bus_cleanup_required_ = false;
};

}  // namespace cpp_bus_driver
