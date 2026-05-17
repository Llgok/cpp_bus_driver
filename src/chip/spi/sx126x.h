/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-05-14 16:28:41
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Sx126x final : public ChipSpiGuide {
 public:
  enum class ChipType {
    kSx1262 = 0,
    kSx1261,
  };

  enum class StdbyConfig {
    kStdbyRc = 0,  // 13 MHz Resistance-Capacitance Oscillator
    kStdbyXosc,    // 32MHz XTAL
  };
  enum class Dio3TcxoVoltage {
    kOutput1600Mv = 0,
    kOutput1700Mv,
    kOutput1800Mv,
    kOutput2200Mv,
    kOutput2400Mv,
    kOutput2700Mv,
    kOutput3000Mv,
    kOutput3300Mv,
  };

  enum class PacketType {
    kFalse = -1,

    kGfsk = 0,
    kLora,
    kLrFhss = 0x03,
  };

  // 在发送（Tx）或接收（Rx）操作之后，无线电会进入的模式。
  enum class FallbackMode {
    kStdbyRc = 0x20,
    kStdbyXosc = 0x30,
    kFs = 0x40,
  };

  // 设置在多少个符号（symbol）的时间内执行信道活动检测
  enum class CadSymbolNum {
    kOn1Symb = 0,
    kOn2Symb,
    kOn4Symb,
    kOn8Symb,
    kOn16Symb,
  };

  enum class CadExitMode {
    // 芯片在 LoRa
    // 模式下执行信道活动检测（kCad）操作，一旦操作完成，无论信道上是否有活动，
    // 芯片都会返回到 kStdbyRc （待机模式，使用内部 RC 振荡器）模式
    kOnly = 0,

    // 芯片执行信道活动检测（kCad）操作，如果检测到活动，芯片将保持在接收（kRx）模式，
    // 直到检测到数据包或计时器达到CAD超时时间。
    kRx = 0x01,

    // 芯片执行信道活动检测（kCad）操作，如果没有检测到活动，芯片自动进入发送（kTx）模式；
    // 该模式用于官方示例中的 Listen Before
    // Talk（LBT）流程，发送数据需要提前写入FIFO。
    kLbt = 0x10,
  };

  enum class RegulatorMode {
    kLdo = 0,     // 仅使用LDO用于所有模式
    kLdoAndDcdc,  // 在STBY_XOSC、kFs、RX和TX模式中使用DC-DC+kLdo
  };

  enum class Dio2Mode {
    kIrq = 0,  // DIO2被用作IRQ
    // 控制一个射频开关，这种情况：在睡眠模式、待机接收模式、待机外部振荡器模式、频率合成模式和接收模式下，kDio2
    // = 0；在发射模式下，kDio2 = 1
    kRfSwitch,

  };

  enum class RampTime {
    kRamp10Us = 0,
    kRamp20Us,
    kRamp40Us,
    kRamp80Us,
    kRamp200Us,
    kRamp800Us,
    kRamp1700Us,
    kRamp3400Us,
  };

  enum class ImgCalFreq {
    kFreq430_440Mhz,
    kFreq470_510Mhz,
    kFreq779_787Mhz,
    kFreq863_870Mhz,
    kFreq902_928Mhz,
  };

  // 芯片所处的模式
  enum class ChipMode {
    kRx,  // 接收模式
    kTx,  // 发送模式
  };

  // Table 13-29: IRQ registers. 中断掩码和标志共用相同 bit 位，
  // 配置 IRQ 时写 1 表示启用，清除 IRQ 时写 1 表示清除对应标志。
  /**
   * Bit | IRQ               | Description                         | Modulation
   * ----|-------------------|-------------------------------------|-----------
   * 0   | TxDone            | Packet transmission completed       | All
   * 1   | RxDone            | Packet received                     | All
   * 2   | PreambleDetected  | Preamble detected                   | All
   * 3   | SyncWordValid     | Valid sync word detected            | GFSK
   * 4   | HeaderValid       | Valid LoRa header received          | LoRa
   * 5   | HeaderErr         | LoRa header CRC error               | LoRa
   * 6   | CrcErr            | Wrong CRC received                  | All
   * 7   | CadDone           | Channel activity detection finished | LoRa
   * 8   | CadDetected       | Channel activity detected           | LoRa
   * 9   | Timeout           | Rx or Tx timeout                    | All
   * 10-13 | -               | RFU                                 | -
   * 14  | LrFhssHop         | LR-FHSS hop done after PA ramp-up   | LR-FHSS
   * 15  | -                 | RFU                                 | -
   */
  enum class IrqMaskFlag {
    kDisable = 0,                  // 取消中断
    kTxDone = 0B0000000000000001,  // TX完成中断
    kRxDone = 0B0000000000000010,  // RX完成中断
    kPreambleDetected = 0B0000000000000100,
    kSyncWordValid = 0B0000000000001000,
    kHeaderValid = 0B0000000000010000,
    kHeaderError = 0B0000000000100000,
    kCrcError = 0B0000000001000000,
    kCadDone = 0B0000000010000000,
    kCadDetected = 0B0000000100000000,
    kTimeout = 0B0000001000000000,
    kLrfhssHop = 0B0100000000000000,
    kAll = 0B0100001111111111,  // 全部中断
  };

  /**
   * @brief 将 IrqMaskFlag 转换为可组合的IRQ掩码
   * @param flag 使用 IrqMaskFlag:: 配置
   * @return IRQ掩码
   */
  static constexpr uint16_t IrqMask(IrqMaskFlag flag) {
    return static_cast<uint16_t>(flag);
  }

  // Table 13-76: Status Bytes Definition
  // 命令状态
  /**
   * Bit |  Description
   * ----|------------------
   * 0   | Reserved
   * 1-3   |Command Status (
   * [0x0: Reserved],
   * [0x1: kRfu],
   * [0x2: Data is available to host],
   * [0x3: Command timeout],
   * [0x4: Command processing error],
   * [0x5: Failure to execute command],
   * [0x6: Command kTx done])
   * 4-6   | Chip Mode (
   * [0x0: Unused],
   * [0x1: kRfu],
   * [0x2: kStbyRc],
   * [0x3: kStbyXosc],
   * [0x4: kFs],
   * [0x5: kRx],
   * [0x6: kTx])
   * 7   | Reserved
   */
  enum class CmdStatus {
    kFalse = -1,

    kRfu = 0x01,
    kDataIsAvailableToHost,
    kCmdTimeout,
    kCmdProcessingError,
    kFailToExecuteCmd,
    kCmdTxDone,
  };

  // 芯片模式状态
  enum class ChipModeStatus {
    kFalse = -1,

    kStbyRc = 0x02,
    kStbyXosc,
    kFs,
    kRx,
    kTx,
  };

  enum class SleepMode {
    kColdStart = 0B00000000,  // 冷启动，关闭RTC唤醒

    // 冷启动，开启RTC唤醒（RTC唤醒来自RC64k）
    kColdStartWakeUpOnRtcTimeout = 0B00000001,
    kWarmStart = 0B00000100,  // 热启动，关闭RTC唤醒
    // 热启动，开启RTC唤醒（RTC唤醒来自RC64k）
    kWarmStartWakeUpOnRtcTimeout = 0B00000101,
  };

  // 扩频因子
  enum class Sf {
    kSf5 = 0x05,
    kSf6,
    kSf7,
    kSf8,
    kSf9,
    kSf10,
    kSf11,
    kSf12,
  };

  // LoRa带宽
  enum class LoraBw {
    kBw7810Hz = 0,
    kBw15630Hz,
    kBw31250Hz,
    kBw62500Hz,
    kBw125000Hz,
    kBw250000Hz,
    kBw500000Hz,

    kBw10420Hz = 0x08,
    kBw20830Hz,
    kBw41670Hz,
  };

  // 纠错编码级别
  enum class Cr {
    kCr45 = 0x01,
    kCr46,
    kCr47,
    kCr48,
  };

  // 低数据速率优化
  enum class Ldro {
    kLdroOff = 0,
    kLdroOn,
  };

  enum class LoraHeaderType {
    kVariableLengthPacket = 0,  // 可变长度
    kFixedLengthPacket,         // 固定长度
  };

  enum class LoraCrcType {
    kOff = 0,  // 关闭
    kOn,       // 打开
  };

  enum class InvertIq {
    kStandardIqSetup = 0,  // 使用标准的IQ极性
    kInvertedIqSetup,      // 使用反转的IQ极性
  };

  // kGfsk 的高斯滤波器的滚降因子
  enum class PulseShape {
    kNoFilter = 0,
    kGaussianBt03 = 0x08,
    kGaussianBt05,
    kGaussianBt07,
    kGaussianBt1,
  };

  // GFSK带宽
  enum class GfskBw {
    kBw467000Hz = 0x09,
    kBw234300Hz,
    kBw117300Hz,
    kBw58600Hz,
    kBw29300Hz,
    kBw14600Hz,
    kBw7300Hz,

    kBw373600Hz = 0x11,
    kBw187200Hz,
    kBw93800Hz,
    kBw46900Hz,
    kBw23400Hz,
    kBw11700Hz,
    kBw5800Hz,

    kBw312000Hz = 0x19,
    kBw156200Hz,
    kBw78200Hz,
    kBw39000Hz,
    kBw19500Hz,
    kBw9700Hz,
    kBw4800Hz,
  };

  // 检测接收到的信号中的前导码
  enum class PreambleDetector {
    kLengthOff = 0,
    kLength8bit = 0x04,
    kLength16bit,
    kLength24bit,
    kLength32bit,
  };

  // 比较接收到的数据包地址与设备预设的地址（节点地址和广播地址）的机制
  enum class AddrComp {
    kFilteringDisable = 0,
    kFilteringActivatedNode,
    kFilteringActivatedNodeBroadcast,
  };

  enum class GfskHeaderType {
    // 数据包的长度在双方都是已知的，有效载荷的大小没有被添加到数据包中
    kKnownPacket = 0,
    kVariablePacket,  // 数据包是可变长度的，有效载荷的第一个字节将是数据包的大小
  };

  enum class GfskCrcType {
    kCrc1Byte = 0,  // CRC在1字节计算
    kCrcOff,        // 关闭 kCrc
    kCrc2Byte,      // CRC在2字节计算

    kCrc1ByteInv = 0x04,  // CRC在1字节上计算并反转
    kCrc2ByteInv = 0x06,  // CRC在2字节上计算并反转
  };

  enum class Whitening {
    kNoEncoding = 0,
    kEnable,
  };

  // 中断状态
  struct IrqStatus {
    // 全局标志
    struct {
      bool tx_done = false;            // 发送完成标志
      bool rx_done = false;            // 接收完成标志
      bool preamble_detected = false;  // 检测到前导字标志
      bool crc_error = false;          // CRC错误标志
      bool tx_rx_timeout = false;      // 发送或接收超时标志
    } all_flag;

    // GFSK模式标志
    struct {
      bool sync_word_valid = false;  // 同步字正确性标志
    } gfsk_flag;

    // LoRa和寄存器模式标志
    struct {
      bool header_valid = false;  // 头字节正确性标志
      bool header_error = false;  // 头字节错误标志
      bool cad_done = false;      // cad传输完成标志
      bool cad_detected = false;  // cad检测成功标志
    } lora_reg_flag;

    // LRFHSS模式标志
    struct {
      // 每次跳频后，在功率放大器 (PA) 再次完成升压 (ramped-up) 之后触发标志
      bool pa_ramped_up_hop = false;

    } lrfhss_flag;
  };

  struct PacketMetrics {
    struct {
      float rssi_average = 0.0;  // 平均最后一个接收到的数据包的RSSI
      // 估算LoRa信号（去扩频后）在最后一个接收到的数据包上的RSSI
      float rssi_instantaneous = 0.0;
      float snr = 0.0;  // 最后一个接收到的数据包的SNR估计值
    } lora;

    struct {
      // 接收到的数据包有效载荷部分的RSSI平均值，在pkt_done中断请求时锁定
      float rssi_average = 0.0;
      float rssi_sync = 0.0;  // 在检测到同步地址时锁定的RSSI值
    } gfsk;
  };

  // Table 13-80: Status Fields
  /**
   * Bit |  RxStatus kFsk
   * ----|-----------------
   * 0   | pkt send
   * 1   | pkt received
   * 2   | abort err
   * 3   | length err
   * 4   | crc err
   * 5   | adrs err
   * 6-7 | RFU
   */
  struct GfskPacketStatus {
    bool packet_send_done_flag = false;     // 发送完成标志
    bool packet_receive_done_flag = false;  // 接收完成标志
    bool abort_error_flag = false;          // 异常终止错误标志
    bool length_error_flag = false;         // 长度错误标志
    bool crc_error_flag = false;            // CRC错误标志
    bool address_error_flag = false;        // 地址错误标志
  };

  struct RxBufferStatus {
    uint8_t payload_length = 0;
    uint8_t start_pointer = 0;
  };

  // 发送状态集合，用于统一判断TxDone、Timeout和原始IRQ标志
  struct SendStatus {
    uint16_t irq_flags = 0;  // 当前读取到的IRQ原始标志
    PacketType packet_type = PacketType::kLora;
    IrqStatus irq_status;
    bool done = false;     // TxDone标志
    bool timeout = false;  // 有效Timeout标志，TxDone优先
    bool error = false;    // 发送失败标志，目前由Timeout触发
  };

  // 接收状态集合，用于统一判断LoRa/GFSK的RxDone、错误状态和FIFO状态
  struct ReceiveStatus {
    bool valid = false;      // 状态有效标志，由 GetReceiveStatus() 设置
    uint16_t irq_flags = 0;  // 当前读取到的IRQ原始标志
    PacketType packet_type = PacketType::kLora;
    IrqStatus irq_status;
    GfskPacketStatus gfsk_packet_status;
    uint32_t gfsk_packet_status_raw =
        0;  // GFSK包状态原始值，可继续解析RSSI指标
    RxBufferStatus rx_buffer_status;
    bool done = false;               // RxDone标志
    bool timeout = false;            // 有效Timeout标志，RxDone优先
    bool crc_error = false;          // IRQ或GFSK包状态中的CRC错误
    bool header_error = false;       // LoRa HeaderError标志
    bool abort_error = false;        // GFSK异常终止错误
    bool length_error = false;       // GFSK长度错误
    bool address_error = false;      // GFSK地址过滤错误
    bool packet_received = false;    // 有效包已接收；GFSK会额外参考pkt_received
    bool payload_available = false;  // RX FIFO中存在可读payload
    bool error = false;              // 接收失败标志
  };

  struct PacketStats {
    uint16_t packet_received = 0;
    uint16_t crc_error = 0;
    uint16_t header_error = 0;
    uint16_t length_error = 0;
  };

  // SX126x板级初始化配置
  struct Config {
    bool enable_dio3_tcxo = true;
    Dio3TcxoVoltage tcxo_voltage = Dio3TcxoVoltage::kOutput1600Mv;
    uint32_t tcxo_startup_time_us = 5000;
    RegulatorMode regulator_mode = RegulatorMode::kLdoAndDcdc;
    Dio2Mode dio2_mode = Dio2Mode::kRfSwitch;
    bool enable_tx_clamp_workaround = true;
    bool enable_retention_list = true;
  };

  explicit Sx126x(std::shared_ptr<BusSpiGuide> bus, ChipType chip_type,
      int32_t busy, int32_t cs = kDefaultValue,
      int32_t rst = kDefaultValue)
      : Sx126x(bus, chip_type, busy, cs, rst, Config{}) {}

  explicit Sx126x(std::shared_ptr<BusSpiGuide> bus, ChipType chip_type,
      int32_t busy, int32_t cs, int32_t rst, const Config& config)
      : ChipSpiGuide(bus, cs),
        chip_type_(chip_type),
        config_(config),
        rst_(rst),
        busy_(busy) {}

  explicit Sx126x(std::shared_ptr<BusSpiGuide> bus, ChipType chip_type,
      bool (*busy_wait_callback)(), int32_t cs = kDefaultValue,
      int32_t rst = kDefaultValue)
      : Sx126x(bus, chip_type, busy_wait_callback, cs, rst, Config{}) {}

  explicit Sx126x(std::shared_ptr<BusSpiGuide> bus, ChipType chip_type,
      bool (*busy_wait_callback)(), int32_t cs, int32_t rst,
      const Config& config)
      : ChipSpiGuide(bus, cs),
        chip_type_(chip_type),
        config_(config),
        rst_(rst),
        busy_wait_callback_(busy_wait_callback) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;

  std::string GetDeviceId();

  /**
   * @brief 检查设备忙
   * @return 如果返回 [true] 代表忙检查成功设备可以接收数据了，如果返回 [false]
   * 代表满检测超过设定的忙等待最大阈值
   */
  bool CheckBusy();

  /**
   * @brief 返回设备的状态，主机可以直接获取芯片状态
   * @return 状态数据，读取错误返回(-1)
   */
  uint8_t GetStatus();

  /**
   * @brief 命令解析，详细请参考SX126x手册 13-76: Status Bytes Definition
   * @param parse_status 解析状态字节，由 GetStatus() 函数获取
   * @return CmdStatus 命令状态
   */
  CmdStatus ParseCmdStatus(uint8_t parse_status);

  /**
   * @brief 芯片模式解析，详细请参考SX126x手册 13-76: Status Bytes Definition
   * @param parse_status 解析状态字节，由 GetStatus() 函数获取
   * @return ChipModeStatus 芯片模式状态
   */
  ChipModeStatus ParseChipModeStatus(uint8_t parse_status);

  /**
   * @brief 中断解析，详细请参考SX126x手册 13-29: IRQ registers
   * @param irq_flag IRQ状态字，由 GetIrqFlag() 函数获取
   * @param status 使用 IrqStatus 结构体保存解析后的IRQ状态
   * @return 解析成功返回 true，失败返回 false
   */
  bool ParseIrqStatus(uint16_t irq_flag, IrqStatus& status);

  /**
   * @brief 配置功耗模式，应用程序如果对时间要求严格需要切换到 kStdbyXosc
   * 模式，使用 kStdbyXosc 前， 需要在 kStdbyRc 模式配置 SetRegulatorMode() 为
   * kLdoAndDcdc 模式再切换到 kStdbyXosc 模式
   * @param config 使用 StdbyConfig:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetStandby(StdbyConfig config);

  /**
   * @brief 配置由DIO3控制的外部TCXO参考电压
   * @param voltage 使用 Dio3TcxoVoltage:: 配置
   * @param time_out_us TCXO启动等待时间，单位us，内部转换为15.625us RTC步进
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDio3AsTcxoCtrl(Dio3TcxoVoltage voltage, uint32_t time_out_us);

  /**
   * @brief 优化功率放大器（PA）的钳位阈值（SX126x手册第15.2节）
   * @param enable [true]：开启修复，[false]：关闭修复
   * @return 成功返回 true，失败返回 false
   */
  bool FixTxClamp(bool enable);

  /**
   * @brief 设置SX126x FIFO数据缓冲区的TX/RX基地址
   * @param tx_base_address tx 基地址
   * @param rx_base_address rx 基地址
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetBufferBaseAddress(uint8_t tx_base_address, uint8_t rx_base_address);

  /**
   * @brief 设置传输数据包类型
   * @param type 使用 PacketType:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetPacketType(PacketType type);

  /**
   * @brief 芯片在成功发送数据包或成功接收数据包后进入的模式
   * @param mode 使用 FallbackMode:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRxTxFallbackMode(FallbackMode mode);

  /**
   * @brief 设置信道活动检测（kCad）参数
   * @param num 使用 CadSymbolNum:: 配置
   * @param cad_det_peak LoRa 调制解调器在尝试与实际 LoRa
   * 前导码符号进行相关时的灵敏度，设置取决于 LoRa 的扩频因子（Spreading
   * Factor）和带宽（Bandwidth），同时也取决于用于验证检测的符号数量
   * @param cad_det_min LoRa 调制解调器在尝试与实际 LoRa
   * 前导码符号进行相关时的灵敏度，设置取决于 LoRa 的扩频因子（Spreading
   * Factor）和带宽（Bandwidth），同时也取决于用于验证检测的符号数量
   * @param exit_mode 在
   * kCad（信道活动检测）操作完成后要执行的操作。此参数是可选的
   * @param time_out_us
   * 仅在 exit_mode = kRx 或 kLbt 时使用，单位us，内部转换为15.625us RTC步进
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetCadParams(CadSymbolNum num, uint8_t cad_det_peak, uint8_t cad_det_min,
      CadExitMode exit_mode, uint32_t time_out_us);

  /**
   * @brief 清除一个IRQ标志
   * @param flag 使用 IrqMaskFlag:: 配置，需要清除的IRQ标志，写1清除对应标志位
   * @return 操作成功返回 true，失败返回 false
   */
  bool ClearIrqFlag(IrqMaskFlag flag);

  /**
   * @brief 清除一个或多个IRQ标志，可通过 IrqMask() 组合多个 IrqMaskFlag
   * @param flags 需要清除的IRQ标志位，设置为1的位会被清除
   * @return 操作成功返回 true，失败返回 false
   */
  bool ClearIrqFlag(uint16_t flags);

  /**
   * @brief 设置IRQ总掩码和DIO1/DIO2/DIO3中断映射
   * @param irq_mask
   * IrqMask用于屏蔽或解除屏蔽可由设备触发的中断请求（kIrq），默认情况下，所有IRQ都被屏蔽（所有位为‘0’），
   * 用户可以通过将相应的掩码设置为‘1’来逐个（或同时多个）启用它们。
   * @param dio1_mask 可通过 IrqMask() 组合多个 IrqMaskFlag，当中断发生时，如果
   * DIO1Mask 和 IrqMask
   * 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
   * 并且DIO1Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，
   * 并同时出现在DIO1上，一个IRQ可以映射到所有DIO，
   * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
   * @param dio2_mask 可通过 IrqMask() 组合多个 IrqMaskFlag，当中断发生时，如果
   * DIO2Mask 和 IrqMask
   * 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
   * 并且DIO2Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，
   * 并同时出现在DIO2上，一个IRQ可以映射到所有DIO，
   * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
   * @param dio3_mask 可通过 IrqMask() 组合多个 IrqMaskFlag，当中断发生时，如果
   * DIO3Mask 和 IrqMask
   * 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
   * 并且DIO3Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，
   * 并同时出现在DIO3上，一个IRQ可以映射到所有DIO，
   * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDioIrqParams(uint16_t irq_mask, uint16_t dio1_mask,
      uint16_t dio2_mask, uint16_t dio3_mask);

  /**
   * @brief 执行SX126x内部模块校准
   * 在电源启动时，无线电设备会执行RC64k、kRc13m、PLL和ADC的校准，然而，从STDBY_RC模式开始，可以随时启动一个或多个模块的校准，
   * 校准功能会启动由calibParam定义的模块的校准，如果所有模块都进行校准，总校准时间为3.5毫秒，校准必须在STDBY_RC模式下启动，
   * 并且在校准过程中BUSY引脚将保持高电平，BUSY引脚的下降沿表示校准过程结束
   * @param calib_param 需要校准的参数设置
   * @return 操作成功返回 true，失败返回 false
   */
  bool Calibrate(uint8_t calib_param);

  /**
   * @brief 获取当前使用的数据包类型
   * @return PacketType 包类型
   */
  PacketType GetPacketType();

  /**
   * @brief 设置LDO/DC-DC供电调节模式
   * 默认情况下，只使用LDO（低压差线性稳压器），这在成本敏感的应用中非常有用，因为DC-DC转换器所需的额外元件会增加成本，
   * 仅使用线性稳压器意味着接收或发送电流几乎会加倍，此功能允许指定是使用DC-DC还是LDO来进行电源调节
   * @param mode 使用 RegulatorMode:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRegulatorMode(RegulatorMode mode);

  /**
   * @brief 设置电流限制
   * @param current （0mA ~ 140mA）步长为2.5mA，有越界校正
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetCurrentLimit(float current);

  /**
   * @brief 获取电流限制
   * @return 返回读取到的数值
   */
  uint8_t GetCurrentLimit();

  /**
   * @brief 配置DIO2的模式功能，IRQ或者控制外部RF开关
   * @param mode 使用 Dio2Mode:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDio2AsRfSwitchCtrl(Dio2Mode mode);

  /**
   * @brief 选择不同设备要使用的功率放大器（PA）及其配置
   * @param pa_duty_cycle 控制着两个功率放大器（kSx1261 和
   * kSx1262）的占空比（导通角），最大输出功率、功耗和谐波都会随着 paDutyCycle
   * 的改变而显著变化，
   * 实现功率放大器最佳效率的推荐设置请参考手册13.1.14点，改变 paDutyCycle
   * 会影响谐波中的功率分布，因此应根据给定的匹配网络进行选择和调整
   * @param hp_max 选择 kSx1262 中功率放大器的大小，此值对 kSx1261
   * 没有影响，通过减小 hpMax 的值可以降低最大输出功率，有效范围在 0x00 到 0x07
   * 之间， 0x07 是 kSx1262 实现 +22 dBm 输出功率的最大支持值，将 hpMax 增加到
   * 0x07 以上可能会导致设备过早老化， 或在极端温度下使用时可能损坏设备
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetPaConfig(uint8_t pa_duty_cycle, uint8_t hp_max);

  /**
   * @brief 设置TX（发送）输出功率，并通过使用参数 ramp_time 来设置 kTx
   * 上升时间，此命令适用于所有选定的协议
   * @param power 输出功率定义为以 dBm 为单位的功率，范围如下：
   * 如果选择低功率 PA，则范围为 -17 (0xEF) 到 +14 (0x0E) dBm，步长为 1 dB，
   * 如果选择高功率 PA，则范围为 -9 (0xF7) 到 +22 (0x16) dBm，步长为 1 dB，
   * 通过 SetPaConfig() 的 device_sel 参数来选择高功率 PA 或低功率
   * PA，默认情况下，设置为低功率 PA 和 +14 dBm
   * @param ramp_time 使用 RampTime:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetTxParams(int8_t power, RampTime ramp_time);

  /**
   * @brief 设置LoRa同步字寄存器
   * 每个LoRa数据包的开始部分都包含一个同步字，接收机通过匹配这个同步字来确认数据包的有效性，
   * 如果接收机发现接收到的数据包中的同步字与预设值一致，则认为这是一个有效的数据包，并继续解码后续的数据
   * @param sync_word
   * 支持官方示例的 8-bit 同步字（0x12 为专用网络，0x34
   * 为公共网络），也支持直接写入寄存器的 16-bit 同步字（0x1424
   * 为专用网络，0x3444 为公共网络）。其他值可能无法与SX126x系列等
   * LoRa设备互操作，或者降低接收灵敏度。
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetLoraSyncWord(uint16_t sync_word);

  /**
   * @brief 获取当前设置的同步字
   * @return 返回读取到的数值
   */
  uint16_t GetLoraSyncWord();

  /**
   * @brief 修复LoRa模式反转 IQ 配置 （SX126x手册第15.4节）
   * @param iq 使用 InvertIq:: 配置
   * @return 成功返回 true，失败返回 false
   */
  bool FixLoraInvertedIq(InvertIq iq);

  /**
   * @brief 设置数据包处理块的参数
   * @param preamble_length
   * 前导长度是一个16位的值，表示无线电将发送的LoRa符号数量
   * @param header_type 使用 LoraHeaderType:: 配置，当字节 header_type 的值为
   * 0 时，有效载荷长度、编码率和头CRC将被添加到LoRa头部，并传输给接收器
   * @param payload_length
   * 数据包的有效载荷（即实际传输的数据）的长度，这个参数通常用于指示数据包中有效数据的字节数，
   * 在进行数据传输时，发送端会设置这个长度，而接收端则根据这个长度来解析接收到的数据包
   * @param crc_type 使用 LoraCrcType:: 配置
   * @param iq 使用 InvertIq:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetLoraPacketParams(uint16_t preamble_length, LoraHeaderType header_type,
      uint8_t payload_length, LoraCrcType crc_type, InvertIq iq);

  /**
   * @brief 设置LoRa调制参数
   * 配置无线电的调制参数，根据在此函数调用之前选择的数据包类型，这些参数将由芯片以不同的方式解释
   * @param sf 使用 Sf:: 配置，LoRa调制中使用的扩频因子
   * @param bw 使用 LoraBw:: 配置，LoRa信号的带宽
   * @param cr 使用 Cr::
   * 配置，LoRa有效载荷使用前向纠错机制，该机制有多个编码级别
   * @param ldro 使用 Ldro::
   * 配置，低数据速率优化，通常由 ConfigLoraParams() 按Semtech官方示例表自动计算
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetLoraModulationParams(Sf sf, LoraBw bw, Cr cr, Ldro ldro);

  /**
   * @brief 设置输出功率
   * @param power SX1262为（-9 ~ 22），SX1261为（-17 ~ 14），有越界校正
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetOutputPower(int8_t power);

  /**
   * @brief 校准设备在其工作频段内的镜像抑制
   * @param freq_mhz 使用 ImgCalFreq:: 配置，需要校准的频率范围
   * @return 操作成功返回 true，失败返回 false
   */
  bool CalibrateImage(ImgCalFreq freq_mhz);

  /**
   * @brief 校准设备在指定频率范围内的镜像抑制
   * @param start_freq_mhz 校准范围的起始频率，单位 MHz
   * @param end_freq_mhz 校准范围的结束频率，单位 MHz
   * @return 操作成功返回 true，失败返回 false
   */
  bool CalibrateImage(uint16_t start_freq_mhz, uint16_t end_freq_mhz);

  /**
   * @brief 设置射频频率模式的频率
   * @param freq_mhz （150 ~ 960）RF的频率设置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRfFrequency(double freq_mhz);

  /**
   * @brief 设置传输频率
   * @param freq_mhz （150 ~ 960）频率设置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetFrequency(double freq_mhz);

  /**
   * @brief 将芯片切换到频率合成器模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetFs();

  /**
   * @brief 配置LoRa模式的传输参数
   * @param freq_mhz （150 ~ 960）频率设置
   * @param bw 使用 LoraBw:: 配置，带宽设置
   * @param current_limit （0 ~ 140）电流限制
   * @param power SX1262为（-9 ~ 22），SX1261为（-17 ~ 14），设置功率
   * @param crc_type 使用 LoraCrcType::
   * 配置，Crc校验，默认关闭并对齐Semtech官方示例
   * @param sf 使用 Sf:: 配置，扩频因子设置，默认SF7并对齐Semtech官方示例
   * @param cr 使用 Cr:: 配置，纠错编码级别，默认4/5并对齐Semtech官方示例
   * @param sync_word
   * 支持官方示例的8-bit同步字（0x12专用网络，0x34公共网络），也支持直接写入寄存器的16-bit同步字
   * （0x1424专用网络，0x3444公共网络）
   * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
   * @return 成功返回 true，失败返回 false
   */
  bool ConfigLoraParams(double freq_mhz, LoraBw bw, float current_limit,
      int8_t power, Sf sf = Sf::kSf7, Cr cr = Cr::kCr45,
      LoraCrcType crc_type = LoraCrcType::kOff, uint16_t preamble_length = 8,
      uint16_t sync_word = 0x1424);

  /**
   * @brief 设置接收占空比模式
   * @param rx_time_us 接收窗口时间，单位为us，芯片内部会转换为 15.625us 步进
   * @param sleep_time_us 休眠窗口时间，单位为us，芯片内部会转换为 15.625us 步进
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRxDutyCycle(uint32_t rx_time_us, uint32_t sleep_time_us);

  /**
   * @brief 设置是否在检测到前导码后停止接收超时计时器
   * @param enable [true]：检测到前导码后停止计时器，[false]：计时器继续运行
   * @return 操作成功返回 true，失败返回 false
   */
  bool StopTimerOnPreamble(bool enable);

  /**
   * @brief 设置设备模式为接收模式
   * @param time_out_us
   * RX超时时间，单位us，内部转换为15.625us
   * RTC步进；0x000000和0xFFFFFF按芯片手册特殊值保留
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRx(uint32_t time_out_us);

  /**
   * @brief 开始LoRa模式传输
   * @param chip_mode 使用 ChipMode:: 配置，芯片的模式
   * @param fallback_mode
   * 从RX或TX模式退出返回的模式设定，默认STDBY_RC并对齐Semtech官方示例
   * @param time_out_us
   * TX/RX超时时间，单位us，内部转换为15.625us
   * RTC步进；0x000000和0xFFFFFF按芯片手册特殊值保留
   * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
   * @return 操作成功返回 true，失败返回 false
   */
  bool StartLora(ChipMode chip_mode, uint32_t time_out_us = 0xFFFFFF,
      FallbackMode fallback_mode = FallbackMode::kStdbyRc,
      uint16_t preamble_length = 8);

  /**
   * @brief 启动LoRa信道活动检测
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetCad();

  /**
   * @brief 发送无限长前导码，常用于射频链路测试
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetTxInfinitePreamble();

  /**
   * @brief 设置LoRa接收符号超时时间
   * @param symbol_count 符号数量，写入芯片的 SetLoRaSymbNumTimeout 参数
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetLoraSymbolTimeout(uint8_t symbol_count);

  /**
   * @brief 获取中断请求状态
   * @return 中断状态，读取错误返回(-1)
   */
  uint16_t GetIrqFlag();

  /**
   * @brief 获取接收缓冲区状态
   * @param status 使用 RxBufferStatus 结构体保存有效载荷长度和FIFO起始地址
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetRxBufferStatus(RxBufferStatus& status);

  /**
   * @brief 获取接收到的数据长度
   * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
   */
  uint8_t GetRxBufferLength();

  /**
   * @brief 读取数据
   * @param data 读取数据的指针
   * @param length 要读取数据的长度，最大255
   * @param offset 数据偏移量
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadBuffer(uint8_t* data, uint8_t length, uint8_t offset = 0);

  /**
   * @brief 读取并解析接收相关状态
   * @param status 使用 ReceiveStatus 结构体保存IRQ、GFSK包状态和RX FIFO状态
   * @note 该函数会在RxDone后按Semtech官方流程停止RX超时计时器；
   * 该函数不清除IRQ，ReceiveData() 会在处理完成后清除已处理的IRQ标志
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetReceiveStatus(ReceiveStatus& status);

  /**
   * @brief 接收数据
   * @param data 接收数据的指针
   * @param length
   * 接收数据的长度，如果等于0将默认读取RX FIFO中的完整payload
   * @param status
   * 可选的接收状态输出指针；为 nullptr 时只接收数据，不输出状态
   * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
   */
  uint8_t ReceiveData(
      uint8_t* data, uint8_t length = 0, ReceiveStatus* status = nullptr);

  /**
   * @brief 获取LoRa模式的包的指标信息
   * @param metrics 使用 PacketMetrics 结构体保存包指标信息
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetLoraPacketMetrics(PacketMetrics& metrics);

  /**
   * @brief 获取当前瞬时RSSI
   * @param rssi_dbm 输出RSSI值，单位为dBm
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetRssiInst(float& rssi_dbm);

  /**
   * @brief 获取芯片内部的数据包统计信息
   * @param stats 使用 PacketStats 结构体保存接收包数量和错误计数
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetPacketStats(PacketStats& stats);

  /**
   * @brief 清空芯片内部的数据包统计计数器
   * @return 操作成功返回 true，失败返回 false
   */
  bool ResetStats();

  /**
   * @brief 获取芯片内部设备错误状态
   * @return 设备错误状态位，读取失败返回0xFFFF
   */
  uint16_t GetDeviceErrors();

  /**
   * @brief 清除芯片内部设备错误状态
   * @return 操作成功返回 true，失败返回 false
   */
  bool ClearDeviceErrors();

  /**
   * @brief 设置接收增益增强模式
   * @param enable [true]：开启接收增益增强，[false]：关闭接收增益增强
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRxBoosted(bool enable);

  /**
   * @brief 修复LoRa 500kHz带宽发送灵敏度相关配置
   * 在 LoRa 500kHz 带宽下清除对应位，其他 LoRa/GFSK
   * 传输前置位（SX126x手册第15.1节）
   * @param enable
   * [true]：LoRa 500kHz 传输前清除对应位，[false]：其他 LoRa/GFSK 传输前置位
   * @return 成功返回 true，失败返回 false
   */
  bool FixBw500KhzSensitivity(bool enable);

  /**
   * @brief 设置设备模式为发送模式
   * @param time_out_us
   * TX超时时间，单位us，内部转换为15.625us
   * RTC步进；0x000000和0xFFFFFF按芯片手册特殊值保留
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetTx(uint32_t time_out_us);

  /**
   * @brief 写入数据
   * @param data 写入数据的指针
   * @param length 要写入数据的长度，最大255
   * @param offset 写入偏移量
   * @return 写入成功返回 true，失败返回 false
   */
  bool WriteBuffer(const uint8_t* data, uint8_t length, uint8_t offset = 0);

  /**
   * @brief 读取并解析发送相关状态
   * @param status 使用 SendStatus 结构体保存发送完成或超时状态
   * @note
   * 该函数只读取状态，不清除IRQ；调用方确认TxDone或Timeout后再清除对应IRQ标志
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetSendStatus(SendStatus& status);

  /**
   * @brief 发送数据
   * @param data 发送数据的指针
   * @param length 发送数据的长度，最大255
   * @param time_out_us
   * TX超时时间，单位us，内部转换为15.625us
   * RTC步进；0x000000和0xFFFFFF按芯片手册特殊值保留
   * @return 操作成功返回 true，失败返回 false
   */
  bool SendData(const uint8_t* data, uint8_t length, uint32_t time_out_us = 0);

  /**
   * @brief 设置LoRa模式的CRC
   * @param crc_type 使用 LoraCrcType:: 配置，Crc校验
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetLoraCrcPacketParams(LoraCrcType crc_type);

  /**
   * @brief 设置GFSK同步字寄存器；改变有效同步字长度时还需要同步更新GFSK包参数
   * @param sync_word 同步字数据，最大8个字节
   * @param length 同步字数据长度，单位Byte，范围1~8
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskSyncWord(const uint8_t* sync_word, uint8_t length);

  /**
   * @brief 设置GFSK地址过滤使用的节点地址和广播地址
   * @param node_address 节点地址
   * @param broadcast_address 广播地址
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskPacketAddress(uint8_t node_address, uint8_t broadcast_address);

  /**
   * @brief 设置GFSK CRC seed和polynomial；改变CRC类型时还需要同步更新GFSK包参数
   * @param initial 参数1，官方示例宏为0x01234567，芯片写入低16位0x4567
   * @param polynomial 参数2，官方示例宏为0x01234567，芯片写入低16位0x4567
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskCrc(uint16_t initial, uint16_t polynomial);

  /**
   * @brief 设置GFSK数据白化初始种子
   * @param seed 白化种子，官方示例默认值为 0x0123
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskWhiteningSeed(uint16_t seed);

  /**
   * @brief 设置数据包处理块的参数
   * @param preamble_length
   * 实际发送/接收的GFSK前导长度，单位Bit
   * @param preamble_detector_length
   * 前导检测门限，和 preamble_length
   * 不是同一个配置；例如官方默认是发送32bit前导，
   * 但检测16bit前导后即可继续找同步字
   * @param sync_word_bit_length
   * 同步字有效长度，单位Bit；例如5字节同步字需要传40
   * @param addr_comp 使用 AddrComp::
   * 配置，用于比较接收到的数据包地址与设备预设的地址（节点地址和广播地址）的机制
   * @param header_type 使用 GfskHeaderType:: 配置
   * @param payload_length
   * 数据包的有效载荷（即实际传输的数据）的长度，这个参数通常用于指示数据包中有效数据的字节数，
   * 在进行数据传输时，发送端会设置这个长度，而接收端则根据这个长度来解析接收到的数据包
   * @param crc_type 使用 GfskCrcType:: 配置
   * @param whitening 使用 Whitening:: 配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskPacketParams(uint16_t preamble_length,
      PreambleDetector preamble_detector_length, uint8_t sync_word_bit_length,
      AddrComp addr_comp, GfskHeaderType header_type, uint8_t payload_length,
      GfskCrcType crc_type, Whitening whitening);

  /**
   * @brief 设置GFSK调制参数
   * 配置无线电的调制参数，根据在此函数调用之前选择的数据包类型，这些参数将由芯片以不同的方式解释
   * @param br （0.6 ~ 300）传输比特率，单位为kbps
   * @param ps 使用 PulseShape:: 配置，高斯滤波器的滚降因子
   * @param bw 使用 GfskBw:: 配置，带宽
   * @param freq_deviation_khz （0.6 ~ 200）频率偏移
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskModulationParams(
      double br, PulseShape ps, GfskBw bw, double freq_deviation_khz);

  /**
   * @brief 配置GFSK模式的传输参数
   * @param freq_mhz （150 ~ 960）频率设置
   * @param br （0.6 ~ 300）传输比特率，单位kbps
   * @param bw 使用 GfskBw:: 配置，带宽设置
   * @param current_limit （0 ~ 140）电流限制
   * @param power SX1262为（-9 ~ 22），SX1261为（-17 ~ 14），设置功率
   * @param freq_deviation_khz （0.6 ~ 200）频率偏移
   * @param sync_word 设置同步字数据指针；为空时使用官方示例默认同步字
   * @param sync_word_length
   * 同步字有效长度，单位Byte；为0时使用官方示例默认40bit同步字
   * @param ps 使用 PulseShape:: 配置，高斯滤波器的滚降因子
   * @param crc_type 使用 GfskCrcType:: 配置，Crc校验
   * @param crc_initial CRC参数1，官方示例宏为0x01234567，芯片写入低16位0x4567
   * @param crc_polynomial
   * CRC参数2，官方示例宏为0x01234567，芯片写入低16位0x4567
   * @param preamble_length 实际发送/接收的GFSK前导长度，单位Bit
   * @param header_type 使用 GfskHeaderType:: 配置，固定包或可变长度包
   * @param whitening 使用 Whitening:: 配置，是否启用数据白化
   * @param addr_comp 使用 AddrComp:: 配置，是否启用节点/广播地址过滤
   * @param node_address GFSK地址过滤的节点地址，默认0x05并对齐Semtech官方示例
   * @param broadcast_address
   * GFSK地址过滤的广播地址，默认0xAB并对齐Semtech官方示例
   * @param whitening_seed GFSK白化种子，官方示例默认值为0x0123
   * @param preamble_detector
   * 前导检测门限，和 preamble_length
   * 不是同一个配置；默认kLength16bit对齐官方示例
   * @return 配置成功返回 true，失败返回 false
   */
  bool ConfigGfskParams(double freq_mhz, double br, GfskBw bw,
      float current_limit, int8_t power, double freq_deviation_khz = 25.0,
      const uint8_t* sync_word = nullptr, uint8_t sync_word_length = 0,
      PulseShape ps = PulseShape::kNoFilter,
      GfskCrcType crc_type = GfskCrcType::kCrc1ByteInv,
      uint16_t crc_initial = 0x4567, uint16_t crc_polynomial = 0x4567,
      uint16_t preamble_length = 32,
      GfskHeaderType header_type = GfskHeaderType::kVariablePacket,
      Whitening whitening = Whitening::kNoEncoding,
      AddrComp addr_comp = AddrComp::kFilteringDisable,
      uint8_t node_address = 0x05, uint8_t broadcast_address = 0xAB,
      uint16_t whitening_seed = 0x0123,
      PreambleDetector preamble_detector = PreambleDetector::kLength16bit);

  /**
   * @brief 开始GFSK模式传输
   * @param chip_mode 使用 ChipMode:: 配置，芯片的模式
   * @param fallback_mode
   * 从RX或TX模式退出返回的模式设定，默认STDBY_RC并对齐Semtech官方示例
   * @param time_out_us
   * TX/RX超时时间，单位us，内部转换为15.625us
   * RTC步进；0x000000和0xFFFFFF按芯片手册特殊值保留
   * @param preamble_length
   * 实际发送/接收的GFSK前导长度，单位Bit；前导检测门限使用当前配置，必要时仅按此前导长度压到合法范围
   * @return 操作成功返回 true，失败返回 false
   */
  bool StartGfsk(ChipMode chip_mode, uint32_t time_out_us = 0xFFFFFF,
      FallbackMode fallback_mode = FallbackMode::kStdbyRc,
      uint16_t preamble_length = 32);

  /**
   * @brief 获取GFSK模式包的状态
   * @return uint32_t 值的排序为
   * [未使用(8bit)|RxStatus(8bit)|RssiSync(8bit)|RssiAvg(8bit)]
   */
  uint32_t GetGfskPacketStatus();

  /**
   * @brief GFSK模式数据接收解析
   * @param parse_status
   * 需要解析的状态数据，使用 GetGfskPacketStatus() 函数的返回值配置
   * （[RxStatus(8bit)]数据）
   * @param status 由 GfskPacketStatus 结构体保存包状态
   * @return 解析成功返回 true，失败返回 false
   */
  bool ParseGfskPacketStatus(uint32_t parse_status, GfskPacketStatus& status);

  /**
   * @brief 解析GFSK模式的包的指标信息
   * @param parse_metrics
   * 需要解析的指标数据，使用 GetGfskPacketStatus() 函数的返回值配置
   * （[RssiSync(8bit)|RssiAvg(8bit)]数据）
   * @param metrics 使用 PacketMetrics 结构体保存包指标信息
   * @return 解析成功返回 true，失败返回 false
   */
  bool ParseGfskPacketMetrics(uint32_t parse_metrics, PacketMetrics& metrics);

  /**
   * @brief 设置GFSK同步字寄存器，并同步更新GFSK包参数中的同步字有效长度
   * @param sync_word 设置同步字数据指针
   * @param sync_word_length 同步字有效长度，单位Byte，范围1~8
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskSyncWordPacketParams(
      const uint8_t* sync_word, uint8_t sync_word_length);

  /**
   * @brief 设置GFSK CRC类型、seed和polynomial，并同步更新GFSK包参数中的CRC类型
   * @param crc_type 使用 GfskCrcType:: 配置，Crc校验
   * @param crc_initial CRC参数1，官方示例宏为0x01234567，芯片写入低16位0x4567
   * @param crc_polynomial
   * CRC参数2，官方示例宏为0x01234567，芯片写入低16位0x4567
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGfskCrcPacketParams(
      GfskCrcType crc_type, uint16_t crc_initial, uint16_t crc_polynomial);

  /**
   * @brief 设置中断引脚的模式
   * @param dio1_mode 使用 IrqMaskFlag:: 配置，DIO1需要配置的芯片中断模式
   * @param dio2_mode 使用 IrqMaskFlag:: 配置，DIO2需要配置的芯片中断模式
   * @param dio3_mode 使用 IrqMaskFlag:: 配置，DIO3需要配置的芯片中断模式
   * @param irq_mask SX126x内部启用的IRQ总掩码，为0时默认使用DIO1/2/3掩码合集
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetIrqGpioMode(IrqMaskFlag dio1_mode,
      IrqMaskFlag dio2_mode = IrqMaskFlag::kDisable,
      IrqMaskFlag dio3_mode = IrqMaskFlag::kDisable, uint16_t irq_mask = 0);

  /**
   * @brief 设置中断引脚的组合掩码模式，可把多个IRQ源映射到同一个DIO
   * @param dio1_mask DIO1需要映射的IRQ掩码
   * @param dio2_mask DIO2需要映射的IRQ掩码
   * @param dio3_mask DIO3需要映射的IRQ掩码
   * @param irq_mask SX126x内部启用的IRQ总掩码，为0时默认使用DIO1/2/3掩码合集
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetIrqGpioMode(uint16_t dio1_mask, uint16_t dio2_mask = 0,
      uint16_t dio3_mask = 0, uint16_t irq_mask = 0);

  /**
   * @brief 清空buffer缓冲区所有数据
   * @return 操作成功返回 true，失败返回 false
   */
  bool ClearBuffer();

  /**
   * @brief 发送一个连续波（RF 纯音）用于测试TX最大发射功率（信号类型：
   * 单一频率的连续信号，没有调制），
   * 用于检查无线电在特定频率和输出功率下的基本发射能力。可以用来测量发射功率、频率精度等
   * 这是一个测试命令，适用于所有数据包类型，用于在选定的频率和输出功率下生成连续波（RF
   * 信号） 设备会保持在 kTx
   * 连续波模式，直到主机发送模式配置命令。虽然此命令在实际应用中没有真正的用例，
   * 但它可以为开发者提供有价值的帮助，以检查和监控无线电在 Tx 模式下的性能
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetTxContinuousWave();

  /**
   * @brief 将设备设置为电流消耗最低的睡眠模式（kSleep
   * 模式），该命令只能在待机模式（kStdbyRc 或 kStdbyXosc）下发送，在整个 kSleep
   * 期间，kBusy（忙碌）线会被拉高， 在 kNss
   * 的上升沿之后，除了备份电源调节器（如果需要）和参数 sleepConfig
   * 中指定的模块外，所有模块都将被关闭，开启睡眠，可以通过 kNss
   * 线的下降沿从主机处理器唤醒设备，
   * 也可以使用 SetStandby() 函数进行唤醒或者使用RTC唤醒
   * @param mode 使用 SleepMode:: 进行配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetSleep(SleepMode mode = SleepMode::kWarmStart);

  /**
   * @brief 通过NSS下降沿唤醒睡眠中的SX126x，并重新应用必要的手册勘误配置
   * @return 成功返回 true，失败返回 false
   */
  bool Wakeup();

 private:
  enum class Cmd {
    kWoResetStats = 0x00,
    kWoClearIrqStatus = 0x02,
    kWoClearDeviceErrors = 0x07,
    kWoSetDioIrqParams = 0x08,

    // 用于读写寄存器命令
    kWoWriteRegister = 0x0D,
    kWoWriteBuffer = 0x0E,
    kRoGetStats = 0x10,
    kRoGetPacketType = 0x11,
    kRoGetIrqStatus,
    kRoGetRxBufferStatus,
    kRoGetPacketStatus,
    kRoGetRssiInst,
    kRoGetDeviceErrors = 0x17,
    kWoReadRegister = 0x1D,
    kRoReadBuffer = 0x1E,

    kWoSetStandby = 0x80,
    kWoSetRx = 0x82,
    kWoSetTx,
    kWoSetSleep,
    kWoSetRfFrequency = 0x86,
    kWoSetCadParams = 0x88,
    kWoCalibrate,
    kWoSetPacketType,
    kWoSetModulationParams,
    kWoSetPacketParams,

    kWoSetTxParams = 0x8E,
    kWoSetBufferBaseAddress,

    kWoSetRxTxFallbackMode = 0x93,
    kWoSetRxDutyCycle,
    kWoSetPaConfig = 0x95,
    kWoSetRegulatorMode,
    kWoSetDio3AsTcxoCtrl,
    kWoCalibrateImage,

    kWoSetDio2AsRfSwitchCtrl = 0x9D,
    kWoStopTimerOnPreamble = 0x9F,
    kWoSetLoraSymbolNumTimeout = 0xA0,
    kRoGetStatus = 0xC0,
    kWoSetFs,
    kWoSetCad = 0xC5,
    kWoSetTxContinuousWave = 0xD1,
    kWoSetTxInfinitePreamble = 0xD2,

  };

  // 访问寄存器需要通过前置读写命令来访问
  // 采用大端先发的规则发送（0x0001 先发0x00后发0x01）
  enum class Reg {
    kRwRetentionListBaseAddress = 0x029F,
    kRoDeviceId = 0x0320,
    kRwWhiteningSeedStart = 0x06B8,
    kRwCrcValueProgrammingStart = 0x06BC,
    kRwCrcPolynomialStart = 0x06BE,
    kRwSyncWordProgrammingStart = 0x06C0,
    kRwGfskNodeAddress = 0x06CD,
    kRwLoraSymbolTimeout = 0x0706,
    kRwIqPolaritySetup = 0x0736,
    kRwLoraSyncWordStart = 0x0740,
    kRwTxModulation = 0x0889,
    kRwRxGain = 0x08AC,
    kRwTxClampConfig = 0x08D8,
    kRwOcpConfiguration = 0x08E7,
    kRwRtcControl = 0x0902,
    kRwEventClear = 0x0944,
  };

  struct Param {
    PacketType packet_type = PacketType::kLora;
    RegulatorMode regulator_mode = RegulatorMode::kLdoAndDcdc;
    double freq_mhz = 868.0;
    float current_limit = 140;
    int8_t power = 14;

    struct {
      double bit_rate = 50.0;
      GfskBw band_width = GfskBw::kBw117300Hz;
      float freq_deviation_khz = 25.0;

      struct {
        const uint8_t* data = nullptr;
        uint8_t length = 0;
      } sync_word;

      PreambleDetector preamble_detector = PreambleDetector::kLength16bit;
      AddrComp address_comparison = AddrComp::kFilteringDisable;
      GfskHeaderType header_type = GfskHeaderType::kVariablePacket;
      uint8_t payload_length = kMaxPayloadSize;
      PulseShape pulse_shape = PulseShape::kNoFilter;

      struct {
        GfskCrcType type = GfskCrcType::kCrc1ByteInv;
        uint16_t initial = 0x4567;
        uint16_t polynomial = 0x4567;
      } crc;

      Whitening whitening = Whitening::kNoEncoding;

      uint16_t preamble_length = 32;
    } gfsk;

    struct {
      Sf spreading_factor = Sf::kSf7;
      LoraBw band_width = LoraBw::kBw125000Hz;
      Ldro low_data_rate_optimize = Ldro::kLdroOff;
      Cr cr = Cr::kCr45;
      uint16_t sync_word = 0x1424;
      uint16_t preamble_length = 8;
      LoraHeaderType header_type = LoraHeaderType::kVariableLengthPacket;
      uint8_t payload_length = kMaxPayloadSize;
      LoraCrcType crc_type = LoraCrcType::kOff;
      InvertIq invert_iq = InvertIq::kStandardIqSetup;
    } lora;

    bool rx_boosted = false;
  };

  // SX1262的ID为SX1261
  static constexpr const char* kDeviceId = "SX1261";
  static constexpr uint32_t kTimeoutDisabled = 0x000000;
  static constexpr uint32_t kTimeoutContinuous = 0xFFFFFF;
  static constexpr uint16_t kBusyPinTimeoutCount = 10000;
  static constexpr uint16_t kBusyFunctionTimeoutCount = kBusyPinTimeoutCount;
  static constexpr uint8_t kCalibrateAll = 0x7F;
  static constexpr uint8_t kMaxPayloadSize = 255;
  static constexpr size_t kMaxSpiFrameSize = 4 + kMaxPayloadSize;

  /**
   * @brief 写入SX126x命令和命令参数
   * @param command 使用 Cmd:: 配置，需要写入的命令
   * @param data 命令参数数据指针，没有参数时为 nullptr
   * @param length 命令参数长度
   * @return 写入成功返回 true，失败返回 false
   */
  bool WriteCommand(
      Cmd command, const uint8_t* data = nullptr, size_t length = 0);

  /**
   * @brief 读取SX126x命令返回数据
   * @param command 使用 Cmd:: 配置，需要读取的命令
   * @param data 读取数据保存指针
   * @param length 读取数据长度
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadCommand(Cmd command, uint8_t* data, size_t length);

  /**
   * @brief 写入SX126x寄存器
   * @param address 寄存器地址
   * @param data 写入数据指针
   * @param length 写入数据长度
   * @return 写入成功返回 true，失败返回 false
   */
  bool WriteRegister(uint16_t address, const uint8_t* data, size_t length);

  /**
   * @brief 读取SX126x寄存器
   * @param address 寄存器地址
   * @param data 读取数据保存指针
   * @param length 读取数据长度
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadRegister(uint16_t address, uint8_t* data, size_t length);

  /**
   * @brief 直接写入SX126x FIFO缓冲区
   * @param offset FIFO写入偏移地址
   * @param data 写入数据指针
   * @param length 写入数据长度
   * @return 写入成功返回 true，失败返回 false
   */
  bool WriteBufferRaw(uint8_t offset, const uint8_t* data, size_t length);

  /**
   * @brief 直接读取SX126x FIFO缓冲区
   * @param offset FIFO读取偏移地址
   * @param data 读取数据保存指针
   * @param length 读取数据长度
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadBufferRaw(uint8_t offset, uint8_t* data, size_t length);

  /**
   * @brief 将微秒转换为SX126x RTC 15.625us步进
   * @param time_us 微秒时间
   * @return RTC步进值
   */
  uint32_t MicrosecondsToRtcStep(uint32_t time_us) const;

  /**
   * @brief 将超时时间转换为SX126x RTC步进，并保留特殊超时值
   * @param time_us 微秒超时时间，0和0xFFFFFF会按芯片特殊值保留
   * @return RTC步进值
   */
  uint32_t TimeoutMicrosecondsToRtcStep(uint32_t time_us) const;

  /**
   * @brief 获取LoRa带宽对应的Hz数值
   * @param bw 使用 LoraBw:: 配置
   * @return LoRa带宽Hz数值
   */
  float GetLoraBandwidthHz(LoraBw bw) const;

  /**
   * @brief 根据官方 LoRa 带宽和扩频因子表获取低数据率优化配置
   * @param sf 使用 Sf:: 配置
   * @param bw 使用 LoraBw:: 配置
   * @return 使用 Ldro:: 配置
   */
  Ldro GetLoraLowDataRateOptimize(Sf sf, LoraBw bw) const;

  /**
   * @brief 根据Semtech官方CAD示例表获取LoRa CAD默认参数
   * @param sf 使用 Sf:: 配置
   * @param symbol_num CAD检测使用的LoRa符号数量
   * @param cad_det_peak CAD峰值检测门限
   * @param cad_det_min CAD最小峰值门限
   */
  void GetLoraCadParams(Sf sf, CadSymbolNum& symbol_num, uint8_t& cad_det_peak,
      uint8_t& cad_det_min) const;

  /**
   * @brief 根据GFSK实际前导长度获取最大合法前导检测器配置
   * @param preamble_length 实际发送/接收的GFSK前导长度，单位Bit
   * @return 使用 PreambleDetector:: 配置
   */
  PreambleDetector GetGfskMaxPreambleDetector(uint16_t preamble_length) const;

  /**
   * @brief 将手册勘误相关寄存器加入Sleep warm start保留列表
   * @param register_address 需要保留的寄存器地址列表
   * @param register_count 寄存器数量
   * @return 成功返回 true，失败返回 false
   */
  bool AddRegistersToRetentionList(
      const uint16_t* register_address, size_t register_count);

  /**
   * @brief 初始化Sleep warm start保留列表
   * @return 初始化成功返回 true，失败返回 false
   */
  bool InitRetentionList();

  /**
   * @brief 从Sleep唤醒后重新应用必要的手册勘误配置
   * @return 成功返回 true，失败返回 false
   */
  bool ApplyWorkaroundsAfterWakeup();

  /**
   * @brief 停止RxDone后的RX超时计时器，对齐Semtech官方 sx126x_handle_rx_done()
   * @return 操作成功返回 true，失败返回 false
   */
  bool StopRxTimeoutTimer();

  ChipType chip_type_;
  Config config_;
  Param param_;
  int32_t rst_;
  int32_t busy_ = kDefaultValue;
  bool (*busy_wait_callback_)() = nullptr;
};
}  // namespace cpp_bus_driver
