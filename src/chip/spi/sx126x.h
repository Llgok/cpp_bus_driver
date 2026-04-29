
/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-04-25 15:50:55
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
    // 直到检测到数据包或计时器达到由 cadTimeout * 15.625μs 定义的超时时间。
    kRx,
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

  // Table 13-29: kIrq Registers
  // 中断掩码和标志，掩码和标志完全相同，设置1开启中断或者清除标志
  /**
   * Bit | kIrq              | Description                              |
   * Modulation
   * ----|------------------|------------------------------------------|-----------
   * 0   | TxDone           | Packet transmission completed            | All
   * 1   | RxDone           | Packet received                          | All
   * 2   | PreambleDetected | Preamble detected                        | All
   * 3   | SyncWordValid    | Valid sync word detected                 | kFsk
   * 4   | HeaderValid      | Valid LoRa header received               |
   * LoRa&reg; 5   | HeaderErr        | LoRa header kCrc error | LoRa&reg; 6   |
   * CrcErr           | Wrong kCrc received                       | All 7   |
   * CadDone          | Channel activity detection finished       | LoRa&reg; 8
   * | CadDetected      | Channel activity detected                | LoRa&reg;
   * 9   | Timeout          | Rx or Tx timeout                         | All
   * 10-13| -                | kRfu                                      | -
   * 14  | LrFhssHop        | Asserted at each hop, in Long Range kFhss, after
   * the PA has ramped-up again | LR-kFhss 15  | -                | kRfu | -
   */
  enum class IrqMaskFlag {
    kDisable = 0,                  // 取消中断
    kAll = 0B0100001111111111,     // 全部中断
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
  };

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
    kWarmStart = 0B00000100,  // 热启动，关闭RTC唤醒

    // 冷启动，开启RTC唤醒（RTC唤醒来自RC64k）
    kColdStartWakeUpOnRtcTimeout = 0B00000001,
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

  // LORA带宽
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

    // LORA和寄存器模式标志
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
   * 6   | sync err
   * 7   | preamble err
   */
  struct GfskPacketStatus {
    bool packet_send_done_flag = false;     // 发送完成标志
    bool packet_receive_done_flag = false;  // 接收完成标志
    bool abort_error_flag = false;          // 异常终止错误标志
    bool length_error_flag = false;         // 长度错误标志
    bool crc_error_flag = false;            // CRC错误标志
    bool address_error_flag = false;        // 地址错误标志
    bool sync_word_flag = false;            // 同步字错误标志
    bool preamble_error_flag = false;       // 前导字错误标志
  };

  explicit Sx126x(std::shared_ptr<BusSpiGuide> bus, ChipType chip_type,
      int32_t busy, int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipSpiGuide(bus, cs), chip_type_(chip_type), rst_(rst), busy_(busy) {}

  explicit Sx126x(std::shared_ptr<BusSpiGuide> bus, ChipType chip_type,
      bool (*busy_wait_callback)(), int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE,
      int32_t rst = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : ChipSpiGuide(bus, cs),
        chip_type_(chip_type),
        rst_(rst),
        busy_wait_callback_(busy_wait_callback) {}

  bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE) override;

  std::string GetDeviceId();

  /**
   * @brief 检查设备忙
   * @return 如果返回 [true] 代表忙检查成功设备可以接收数据了，如果返回 [false]
   * 代表满检测超过设定的忙等待最大阈值
   * @Date 2025-02-14 16:12:30
   */
  bool CheckBusy();

  /**
   * @brief 返回设备的状态，主机可以直接获取芯片状态
   * @return 状态数据，读取错误返回(-1)
   * @Date 2025-01-18 17:04:10
   */
  uint8_t GetStatus();

  /**
   * @brief 命令解析，详细请参考SX126x手册 13-76: Status Bytes Definition
   * @param parse_status 解析状态语句，由get_status()函数获取
   * @return CmdStatus 由Cmd_Status::配置，命令状态
   * @Date 2025-02-12 14:41:21
   */
  CmdStatus ParseCmdStatus(uint8_t parse_status);

  /**
   * @brief 芯片模式解析，详细请参考SX126x手册 13-76: Status Bytes Definition
   * @param parse_status 解析状态语句，由get_status()函数获取
   * @return ChipModeStatus 由Chip_Mode_Status::配置，芯片模式状态
   * @Date 2025-02-13 13:52:33
   */
  ChipModeStatus ParseChipModeStatus(uint8_t parse_status);

  /**
   * @brief 中断解析，详细请参考SX126x手册 13-29: kIrq Registers
   * @param irq_flag 解析状态语句，由get_irq_flag()函数获取
   * @param &status 使用Irq_Status结构体配置，相应位自动置位
   * @return
   * @Date 2025-02-13 13:33:15
   */
  bool ParseIrqStatus(uint16_t irq_flag, IrqStatus& status);

  /**
   * @brief 配置功耗模式，应用程序如果对时间要求严格需要切换到 kStdbyXosc
   * 模式，使用 kStdbyXosc 前， 需要在 kStdbyRc 模式配置 SetRegulatorMode() 为
   * kLdoAndDcdc 模式再切换到 kStdbyXosc 模式
   * @param config 使用 StdbyConfig:: 配置
   * @return
   * @Date 2025-01-17 16:49:59
   */
  bool SetStandby(StdbyConfig config);

  /**
   * @brief 配置由DIO3控制的外部TCXO参考电压
   * @param voltage 使用 Dio3TcxoVoltage:: 配置
   * @param time_out_us （0 ~ 16777215）超时时间，Delay duration = Delay(23:0)
   * *15.625 µs，每一步等于15.625 µs
   * @return
   * @Date 2025-01-22 12:04:01
   */
  bool SetDio3AsTcxoCtrl(Dio3TcxoVoltage voltage, uint32_t time_out_us);

  /**
   * @brief 优化功率放大器（PA）的钳位阈值（SX126x手册第15.2节）
   * @param enable [true]：开启修复，[false]：关闭修复
   * @return
   * @Date 2025-01-17 16:58:01
   */
  bool FixTxClamp(bool enable);

  /**
   * @brief
   * 此命令为数据缓冲区设置基地址，适用于所有操作模式下的数据包处理操作，包括发送（kTx）和接收（kRx）模式
   * @param tx_base_address tx 基地址
   * @param rx_base_address rx 基地址
   * @return
   * @Date 2025-01-17 17:08:57
   */
  bool SetBufferBaseAddress(uint8_t tx_base_address, uint8_t rx_base_address);

  /**
   * @brief 设置传输数据包类型
   * @param type 使用 PacketType:: 配置
   * @return
   * @Date 2025-01-17 17:29:17
   */
  bool SetPacketType(PacketType type);

  /**
   * @brief 芯片在成功发送数据包或成功接收数据包后进入的模式
   * @param mode 使用 FallbackMode:: 配置
   * @return
   * @Date 2025-01-17 17:34:32
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
   * @param time_out_us 仅在 cadExitMode = kCadRx 时执行 kCad
   * 时使用，在这里，cadTimeout 表示设备在成功完成 kCad
   * 后保持在接收模式（Rx）的时间， 接收超时时间（Rx Timeout）= cadTimeout
   * * 15.625μs，最大值为 16777215 （0xFFFFFF）
   * @return
   * @Date 2025-01-17 18:07:39
   */
  bool SetCadParams(CadSymbolNum num, uint8_t cad_det_peak, uint8_t cad_det_min,
      CadExitMode exit_mode, uint32_t time_out_us);

  /**
   * @brief
   * 用于清除IRQ寄存器中的一个IRQ标志,此函数通过将ClearIrqParam中与待清除的IRQ标志位位置相对应的位设置为1，来清除IRQ寄存器中的IRQ标志,
   * 例如，如果ClearIrqParam的第0位被设置为1，那么IRQ寄存器中第0位的IRQ标志将被清除,如果一个DIO（数字输入/输出）被映射到单一的IRQ源，
   * 当IRQ寄存器中对应的位被清除时，该DIO也会被清除,如果DIO被设置为0且与多个IRQ源相关联，那么只有当IRQ寄存器中所有映射到该DIO的位都被清除时，DIO才会被设置为0
   * @param flag 使用Irq_Mask_Flag::配置，需要清除的标志，设置1为清除标志
   * @return
   * @Date 2025-01-18 09:27:58
   */
  bool ClearIrqFlag(IrqMaskFlag flag);

  /**
   * @brief 设置中断请求（kIrq）标志（中断标志参考SX126x手册表格 13-29: kIrq
   * Registers）
   * @param irq_mask
   * IrqMask用于屏蔽或解除屏蔽可由设备触发的中断请求（kIrq），默认情况下，所有IRQ都被屏蔽（所有位为‘0’），
   * 用户可以通过将相应的掩码设置为‘1’来逐个（或同时多个）启用它们。
   * @param dio1_mask 使用 IRQ_Registers:: 配置，当中断发生时，如果 DIO1Mask 和
   * IrqMask
   * 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
   * 并且DIO1Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，并同时出现在DIO1上，一个IRQ可以映射到所有DIO，
   * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
   * @param dio2_mask 使用 IRQ_Registers:: 配置，当中断发生时，如果 DIO2Mask 和
   * IrqMask
   * 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
   * 并且DIO2Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，并同时出现在DIO1上，一个IRQ可以映射到所有DIO，
   * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
   * @param dio3_mask 使用 IRQ_Registers:: 配置，当中断发生时，如果 DIO3Mask 和
   * IrqMask
   * 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
   * 并且DIO3Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，并同时出现在DIO1上，一个IRQ可以映射到所有DIO，
   * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
   * @return
   * @Date 2025-01-18 09:48:15
   */
  bool SetDioIrqParams(uint16_t irq_mask, uint16_t dio1_mask,
      uint16_t dio2_mask, uint16_t dio3_mask);

  /**
   * @brief
   * 在电源启动时，无线电设备会执行RC64k、kRc13m、PLL和ADC的校准，然而，从STDBY_RC模式开始，可以随时启动一个或多个模块的校准，
   * 校准功能会启动由calibParam定义的模块的校准，如果所有模块都进行校准，总校准时间为3.5毫秒，校准必须在STDBY_RC模式下启动，
   * 并且在校准过程中BUSY引脚将保持高电平，BUSY引脚的下降沿表示校准过程结束
   * @param calib_param 需要校准的参数设置
   * @return
   * @Date 2025-01-18 13:59:03
   */
  bool Calibrate(uint8_t calib_param);

  /**
   * @brief 获取当前使用的数据包类型
   * @return 由Packet_Type::配置，包类型
   * @Date 2025-01-21 16:28:58
   */
  PacketType GetPacketType();

  /**
   * @brief
   * 默认情况下，只使用LDO（低压差线性稳压器），这在成本敏感的应用中非常有用，因为DC-DC转换器所需的额外元件会增加成本，
   * 仅使用线性稳压器意味着接收或发送电流几乎会加倍，此功能允许指定是使用DC-DC还是LDO来进行电源调节
   * @param mode 使用 RegulatorMode:: 配置
   * @return
   * @Date 2025-01-22 13:44:23
   */
  bool SetRegulatorMode(RegulatorMode mode);

  /**
   * @brief 设置电流限制
   * @param current （0mA ~ 140mA）步长为2.5mA，有越界校正
   * @return
   * @Date 2025-01-22 14:09:23
   */
  bool SetCurrentLimit(float current);

  /**
   * @brief 获取电流限制
   * @return
   * @Date 2025-01-22 14:45:10
   */
  uint8_t GetCurrentLimit();

  /**
   * @brief 配置DIO2的模式功能，IRQ或者控制外部RF开关
   * @param mode 使用 Dio2Mode:: 配置
   * @return
   * @Date 2025-01-22 14:47:07
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
   * @return
   * @Date 2025-01-22 16:26:38
   */
  bool SetPaConfig(uint8_t pa_duty_cycle, uint8_t hp_max);

  /**
   * @brief 设置TX（发送）输出功率，并通过使用参数 ramp_time 来设置 kTx
   * 上升时间，此命令适用于所有选定的协议
   * @param power 输出功率定义为以 dBm 为单位的功率，范围如下：
   * 如果选择低功率 PA，则范围为 -17 (0xEF) 到 +14 (0x0E) dBm，步长为 1 dB，
   * 如果选择高功率 PA，则范围为 -9 (0xF7) 到 +22 (0x16) dBm，步长为 1 dB，
   * 通过命令 set_pa_config 和参数 device 来选择高功率 PA 或低功率
   * PA，默认情况下，设置为低功率 PA 和 +14 dBm
   * @param ramp_time 使用 RampTime:: 配置
   * @return
   * @Date 2025-01-22 16:46:55
   */
  bool SetTxParams(uint8_t power, RampTime ramp_time);

  /**
   * @brief
   * 设置LORA的同步字，每个LoRa数据包的开始部分都包含一个同步字，接收机通过匹配这个同步字来确认数据包的有效性，
   * 如果接收机发现接收到的数据包中的同步字与预设值一致，则认为这是一个有效的数据包，并继续解码后续的数据
   * @param sync_word
   * 它可以设置为任何值，但有些值无法与SX126X系列等其他LoRa设备互操作，有些值会降低您的接收灵敏度，
   * 除非你有如何检查接收灵敏度和可靠性极限的经验和知识，否则请使用标准值，0x3444
   * 为公共网络，0x1424 为专用网络
   * @return
   * @Date 2025-01-22 09:15:24
   */
  bool SetLoraSyncWord(uint16_t sync_word);

  /**
   * @brief 获取当前设置的同步字
   * @return
   * @Date 2025-01-22 09:30:25
   */
  uint16_t GetLoraSyncWord();

  /**
   * @brief 修复LORA模式反转 IQ 配置 （SX126x手册第15.4节）
   * @param iq 使用 InvertIq:: 配置
   * @return
   * @Date 2025-01-22 09:58:32
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
   * @return
   * @Date 2025-01-22 11:44:47
   */
  bool SetLoraPacketParams(uint16_t preamble_length, LoraHeaderType header_type,
      uint8_t payload_length, LoraCrcType crc_type, InvertIq iq);

  /**
   * @brief
   * 配置无线电的调制参数，根据在此函数调用之前选择的数据包类型，这些参数将由芯片以不同的方式解释
   * @param sf 使用 Sf:: 配置，LoRa调制中使用的扩频因子
   * @param bw 使用 LoraBw:: 配置，LoRa信号的带宽
   * @param cr 使用 Cr::
   * 配置，LoRa有效载荷使用前向纠错机制，该机制有多个编码级别
   * @param ldro 使用 Ldro::
   * 配置，低数据速率优化，当LoRa符号时间等于或大于16.38毫秒时（通常在SF11和BW125以及SF12与BW125和BW250的情况下），
   * 通常会设置此参数
   * @return
   * @Date 2025-01-21 16:47:47
   */
  bool SetLoraModulationParams(Sf sf, LoraBw bw, Cr cr, Ldro ldro);

  /**
   * @brief 设置输出功率
   * @param power （-9 ~ 22）有越界校正
   * @return
   * @Date 2025-02-06 17:42:39
   */
  bool SetOutputPower(int8_t power);

  /**
   * @brief 校准设备在其工作频段内的镜像抑制
   * @param freq_mhz 使用 ImgCalFreq:: 配置，需要校准的频率范围
   * @return
   * @Date 2025-02-06 18:00:12
   */
  bool CalibrateImage(ImgCalFreq freq_mhz);

  /**
   * @brief 设置射频频率模式的频率
   * @param freq_mhz （150 ~ 960）RF的频率设置
   * @return
   * @Date 2025-02-07 09:43:33
   */
  bool SetRfFrequency(double freq_mhz);

  /**
   * @brief 设置传输频率
   * @param freq_mhz （150 ~ 960）频率设置
   * @return
   * @Date 2025-02-07 09:44:36
   */
  bool SetFrequency(double freq_mhz);

  /**
   * @brief 配置LORA模式的传输参数
   * @param freq_mhz （150 ~ 960）频率设置
   * @param bw 使用 LoraBw:: 配置，带宽设置
   * @param current_limit （0 ~ 140）电流限制
   * @param power （-9 ~ 22）设置功率
   * @param crc_type 使用 LoraCrcType:: 配置，Crc校验
   * @param sf 使用 Sf:: 配置，扩频因子设置
   * @param cr 使用 Cr:: 配置，纠错编码级别
   * @param sync_word 同步字设置，0x3444 为公共网络，0x1424 为专用网络
   * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
   * @return
   * @Date 2025-02-07 09:55:00
   */
  bool ConfigLoraParams(double freq_mhz, LoraBw bw, float current_limit,
      int8_t power, Sf sf = Sf::kSf9, Cr cr = Cr::kCr47,
      LoraCrcType crc_type = LoraCrcType::kOn, uint16_t preamble_length = 8,
      uint16_t sync_word = 0x1424);

  /**
   * @brief 设置设备模式为接收模式
   * @param time_out_us 超时时间 = 设置的超时时间
   * * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF） 当设置为
   * [0x000000]
   * 时，设备将保持在RX模式下，直到接收发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式
   * 当设置为 [0xFFFFFF]
   * 时，设备将一直处于所设置的模式，直到主机发送命令更改操作模式。
   * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包
   * 指示给主机，设备将自动搜索一个新的数据包
   * @return
   * @Date 2025-02-07 11:31:43
   */
  bool SetRx(uint32_t time_out_us);

  /**
   * @brief 开始LORA模式传输
   * @param chip_mode 使用 ChipMode:: 配置，芯片的模式
   * @param fallback_mode 从RX或TX模式退出返回的模式设定
   * 当设置为 [0x000000]
   * 时，禁用超时，设备将保持在TX或RX模式下，直到TX或RX发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式
   * 当设置为 [0xFFFFFF]
   * 时，设备将一直处于所设置的模式，直到主机发送命令更改操作模式。
   * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包
   * 指示给主机，设备将自动搜索一个新的数据包
   * @param time_out_us 超时时间 = 设置的超时时间
   * * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）
   * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
   * @return
   * @Date 2025-02-07 11:44:43
   */
  bool StartLoraTransmit(ChipMode chip_mode, uint32_t time_out_us = 0xFFFFFF,
      FallbackMode fallback_mode = FallbackMode::kStdbyXosc,
      uint16_t preamble_length = 8);

  /**
   * @brief 获取中断请求状态
   * @return 中断状态，读取错误返回(-1)
   * @Date 2025-02-07 13:57:28
   */
  uint16_t GetIrqFlag();

  /**
   * @brief 获取接收到的数据长度
   * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
   * @Date 2025-02-07 14:39:31
   */
  uint8_t GetRxBufferLength();

  /**
   * @brief 读取数据
   * @param *data 读取数据的指针
   * @param length 要读取数据的长度，最大255
   * @param offset 数据偏移量
   * @return
   * @Date 2025-02-07 14:55:09
   */
  bool ReadBuffer(uint8_t* data, uint8_t length, uint8_t offset = 0);

  /**
   * @brief 接收数据
   * @param *data 接收数据的指针
   * @param length
   * 接收数据的长度，如果等于0将默认接收对应接收到的数据长度并返回接受到的长度数据，最大255
   * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
   * @Date 2025-02-12 11:17:14
   */
  uint8_t ReceiveData(uint8_t* data, uint8_t length = 0);

  /**
   * @brief 获取LORA模式的包的指标信息
   * @param &metrics 使用 Packet_Metrics的结构体配置，读出包的相关指标信息
   * @return
   * @Date 2025-02-12 17:02:03
   */
  bool GetLoraPacketMetrics(PacketMetrics& metrics);

  /**
   * @brief
   * 修复LORA发送模式在带宽为500khz下灵敏度衰减问题，在任何传输之前设置（设置一次即可不是每次都要设置）（SX126x手册第15.1节）
   * @param enable
   * [true]：开启修复，[false]：关闭修复，只有在LORA模式并且带宽为500khz才开启
   * @return
   * @Date 2025-02-12 18:20:23
   */
  bool FixBw500KhzSensitivity(bool enable);

  /**
   * @brief 设置设备模式为接收模式
   * @param time_out_us 超时时间 = 设置的超时时间
   * * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）。 当设置为
   * [0x000000]
   * 时，设备将保持在TX模式下，直到发送发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式。
   * 当设置为 [0xFFFFFF]
   * 时，设备将一直处于TX模式，直到主机发送命令更改操作模式。
   * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包
   * 指示给主机，设备将自动搜索一个新的数据包。
   * @return
   * @Date 2025-02-12 18:27:27
   */
  bool SetTx(uint32_t time_out_us);

  /**
   * @brief 写入数据
   * @param *data 写入数据的指针
   * @param length 要写入数据的长度，最大255
   * @param offset 写入偏移量
   * @return
   * @Date 2025-02-13 09:51:03
   */
  bool WriteBuffer(const uint8_t* data, uint8_t length, uint8_t offset = 0);

  /**
   * @brief 发送数据
   * @param *data 发送数据的指针
   * @param length 发送数据的长度，最大255
   * @param time_out_us 超时时间 = 设置的超时时间
   * * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）。 当设置为
   * [0x000000]
   * 时，设备将保持在TX模式下，直到发送发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式。
   * 当设置为 [0xFFFFFF]
   * 时，设备将一直处于TX模式，直到主机发送命令更改操作模式。
   * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包
   * 指示给主机，设备将自动搜索一个新的数据包。
   * @return
   * @Date 2025-02-13 09:08:52
   */
  bool SendData(const uint8_t* data, uint8_t length, uint32_t time_out_us = 0);

  /**
   * @brief 设置LORA模式的CRC
   * @param crc_type 使用 LoraCrcType:: 配置，Crc校验
   * @return
   * @Date 2025-02-21 15:36:16
   */
  bool SetLoraCrcPacketParams(LoraCrcType crc_type);

  /**
   * @brief 设置GFSK的同步字（还需要同时设置set_gfsx_packet_params）
   * @param *sync_word 同步字数据，最大8个字节
   * @param length 同步字数据长度
   * @return
   * @Date 2025-02-18 16:24:14
   */
  bool SetGfskSyncWord(uint8_t* sync_word, uint8_t length);

  /**
   * @brief 设置GFSK的校验参数（还需要同时设置set_gfsx_packet_params）
   * @param initial 参数1，默认为 0x1D0F
   * @param polynomial 参数2，默认为 0x1021
   * @return
   * @Date 2025-02-19 09:33:11
   */
  bool SetGfskCrc(uint16_t initial, uint16_t polynomial);

  /**
   * @brief 设置数据包处理块的参数
   * @param preamble_length
   * 前导长度是一个16位的值，表示无线电将发送的GFSK符号数量
   * @param preamble_detector_length 使用 PreambleDetector::
   * 配置,前导码检测器充当数据包控制器的门控器，当前导码检测器长度不为 0 时，
   * 数据包控制器仅在无线电成功接收到一定数量的前导码位后才会激活
   * @param sync_word_bit_length 同步字长度，1个字节等于8位，写入的时候要乘以8
   * @param addr_comp 使用 AddrComp::
   * 配置，用于比较接收到的数据包地址与设备预设的地址（节点地址和广播地址）的机制
   * @param header_type 使用 GfskHeaderType:: 配置
   * @param payload_length
   * 数据包的有效载荷（即实际传输的数据）的长度，这个参数通常用于指示数据包中有效数据的字节数，
   * 在进行数据传输时，发送端会设置这个长度，而接收端则根据这个长度来解析接收到的数据包
   * @param crc_type 使用 GfskCrcType:: 配置
   * @param whitening 使用 Whitening:: 配置
   * @return
   * @Date 2025-02-18 17:29:22
   */
  bool SetGfskPacketParams(uint16_t preamble_length,
      PreambleDetector preamble_detector_length, uint8_t sync_word_bit_length,
      AddrComp addr_comp, GfskHeaderType header_type, uint8_t payload_length,
      GfskCrcType crc_type, Whitening whitening);

  /**
   * @brief
   * 配置无线电的调制参数，根据在此函数调用之前选择的数据包类型，这些参数将由芯片以不同的方式解释
   * @param br （0.6 ~ 300）传输比特率
   * @param ps 使用 PulseShape:: 配置，高斯滤波器的滚降因子
   * @param bw 使用 GfskBw:: 配置，带宽
   * @param freq_deviation_khz （0.6 ~ 200）频率偏移
   * @return
   * @Date 2025-02-19 09:17:55
   */
  bool SetGfskModulationParams(
      double br, PulseShape ps, GfskBw bw, double freq_deviation_khz);

  /**
   * @brief 配置GFSK模式的传输参数
   * @param freq_mhz （150 ~ 960）频率设置
   * @param br （0.6 ~ 300）传输比特率
   * @param bw 使用 GfskBw:: 配置，带宽设置
   * @param current_limit （0 ~ 140）电流限制
   * @param power （-9 ~ 22）设置功率
   * @param freq_deviation_khz （0.6 ~ 200）频率偏移
   * @param *sync_word 设置同步字数据指针
   * @param sync_word_length 设置同步字数据长度
   * @param ps 使用 PulseShape:: 配置，高斯滤波器的滚降因子
   * @param sf 使用 Sf:: 配置，扩频因子设置
   * @param crc_type 使用 GfskCrcType:: 配置，Crc校验
   * @param crc_initial CRC参数1，默认为 0x1D0F
   * @param crc_polynomial CRC参数2，默认为 0x1021
   * @param preamble_length 前导长度，表示无线电将发送的GFSK符号数量
   * @return
   * @Date 2025-02-19 09:17:55
   */
  bool ConfigGfskParams(double freq_mhz, double br, GfskBw bw,
      float current_limit, int8_t power, double freq_deviation_khz = 10.0,
      uint8_t* sync_word = nullptr, uint8_t sync_word_length = 0,
      PulseShape ps = PulseShape::kGaussianBt1, Sf sf = Sf::kSf9,
      GfskCrcType crc_type = GfskCrcType::kCrc2ByteInv,
      uint16_t crc_initial = 0x1D0F, uint16_t crc_polynomial = 0x1021,
      uint16_t preamble_length = 32);

  /**
   * @brief 开始GFSK模式传输
   * @param chip_mode 使用 ChipMode:: 配置，芯片的模式
   * @param fallback_mode 从RX或TX模式退出返回的模式设定
   * @param time_out_us 超时时间 = 设置的超时时间
   * * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF） 当设置为
   * [0x000000]
   * 时，禁用超时，设备将保持在TX或RX模式下，直到TX或RX发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式
   * 当设置为 [0xFFFFFF]
   * 时，设备将一直处于所设置的模式，直到主机发送命令更改操作模式
   * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包
   * 指示给主机，设备将自动搜索一个新的数据包
   * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
   * @return
   * @Date 2025-02-19 09:17:55
   */
  bool StartGfskTransmit(ChipMode chip_mode, uint32_t time_out_us = 0xFFFFFF,
      FallbackMode fallback_mode = FallbackMode::kStdbyXosc,
      uint16_t preamble_length = 32);

  /**
   * @brief 获取GFSK模式包的状态
   * @return uint32_t 值的排序为
   * [未使用(8bit)|RxStatus(8bit)|RssiSync(8bit)|RssiAvg(8bit)]
   * @Date 2025-02-28 16:34:18
   */
  uint32_t GetGfskPacketStatus();

  /**
   * @brief GFSK模式数据接收解析
   * @param parse_status
   * 需要解析的状态数据，使用get_gfsk_packet_status()函数的返回值配置
   * （[RxStatus(8bit)]数据）
   * @param status 由Gfsk_Packet_Status结构体配置，包状态
   * @return
   * @Date 2025-02-21 15:17:05
   */
  bool ParseGfskPacketStatus(uint32_t parse_status, GfskPacketStatus& status);

  /**
   * @brief 解析GFSK模式的包的指标信息
   * @param parse_metrics
   * 需要解析的指标数据，使用get_gfsk_packet_status()函数的返回值配置（[RssiSync(8bit)|RssiAvg(8bit)]数据）
   * @param &metrics 使用 Packet_Metrics的结构体配置，读出包的相关指标信息
   * @return
   * @Date 2025-02-21 15:17:53
   */
  bool ParseGfskPacketMetrics(uint32_t parse_metrics, PacketMetrics& metrics);

  /**
   * @brief 设置GFSK模式的同步字，需要同时设置两个寄存器
   * @param *sync_word 设置同步字数据指针
   * @param sync_word_length 设置同步字数据长度
   * @return
   * @Date 2025-02-21 15:36:16
   */
  bool SetGfskSyncWordPacketParams(
      uint8_t* sync_word, uint8_t sync_word_length);

  /**
   * @brief 设置GFSK模式的CRC，需要同时设置两个寄存器
   * @param crc_type 使用 GfskCrcType:: 配置，Crc校验
   * @param crc_initial CRC参数1，默认为 0x1D0F
   * @param crc_polynomial CRC参数2，默认为 0x1021
   * @return
   * @Date 2025-02-21 15:36:16
   */
  bool SetGfskCrcPacketParams(
      GfskCrcType crc_type, uint16_t crc_initial, uint16_t crc_polynomial);

  /**
   * @brief 设置中断引脚的模式
   * @param dio1_mode 使用 IrqMaskFlag:: 配置，DIO1需要配置的芯片中断模式
   * @param dio2_mode 使用 IrqMaskFlag:: 配置，DIO2需要配置的芯片中断模式
   * @param dio3_mode 使用 IrqMaskFlag:: 配置，DIO3需要配置的芯片中断模式
   * @return
   * @Date 2025-03-15 11:48:59
   */
  bool SetIrqGpioMode(IrqMaskFlag dio1_mode,
      IrqMaskFlag dio2_mode = IrqMaskFlag::kDisable,
      IrqMaskFlag dio3_mode = IrqMaskFlag::kDisable);

  /**
   * @brief 清空buffer缓冲区所有数据
   * @return
   * @Date 2025-03-15 15:06:48
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
   * @return
   * @Date 2025-03-28 17:53:52
   */
  bool SetTxContinuousWave();

  /**
   * @brief 将设备设置为电流消耗最低的睡眠模式（kSleep
   * 模式），该命令只能在待机模式（kStdbyRc 或 kStdbyXosc）下发送，在整个 kSleep
   * 期间，kBusy（忙碌）线会被拉高， 在 kNss
   * 的上升沿之后，除了备份电源调节器（如果需要）和参数 sleepConfig
   * 中指定的模块外，所有模块都将被关闭，开启睡眠，可以通过 kNss
   * 线的下降沿从主机处理器唤醒设备，
   * 也可以使用set_standby()函数进行唤醒或者使用rtc唤醒
   * @param mode 使用Sleep_Mode::进行配置
   * @return
   * @Date 2025-05-08 17:27:59
   */
  bool SetSleep(SleepMode mode = SleepMode::kWarmStart);

 private:
  enum class Cmd {
    // 用于读写寄存器命令
    kWoWriteRegister = 0x0D,
    kWoReadRegister = 0x1D,

    kWoClearIrqStatus = 0x02,
    kWoSetDioIrqParams = 0x08,
    kWoWriteBuffer = 0x0E,
    kRoGetPacketType = 0x11,
    kRoGetIrqStatus,
    kRoGetRxBufferStatus,
    kRoGetPacketStatus,

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
    kWoSetPaConfig = 0x95,
    kWoSetRegulatorMode,
    kWoSetDio3AsTcxoCtrl,
    kWoCalibrateImage,

    kWoSetDio2AsRfSwitchCtrl = 0x9D,
    kRoGetStatus = 0xC0,
    kWoSetTxContinuousWave = 0xD1,

  };

  // 访问寄存器需要通过前置读写命令来访问
  // 采用大端先发的规则发送（0x0001 先发0x00后发0x01）
  enum class Reg {
    kRoDeviceId = 0x0320,

    kRwCrcValueProgrammingStart = 0x06BC,
    kRwSyncWordProgrammingStart = 0x06C0,
    kRwIqPolaritySetup = 0x0736,
    kRwLoraSyncWordStart = 0x0740,
    kRwTxModulation = 0x0889,
    kRwTxClampConfig = 0x08D8,
    kRwOcpConfiguration = 0x08E7,
  };

  struct Param {
    PacketType packet_type = PacketType::kGfsk;
    RegulatorMode regulator_mode = RegulatorMode::kLdoAndDcdc;
    float freq_mhz = 868.0;
    float current_limit = 140;
    int8_t power = 10;

    struct {
      uint32_t bit_rate = 100.0;
      GfskBw band_width = GfskBw::kBw467000Hz;
      float freq_deviation_khz = 10.0;

      struct {
        uint8_t* data = nullptr;
        uint8_t length = 0;
      } sync_word;

      PreambleDetector preamble_detector = PreambleDetector::kLength8bit;
      AddrComp address_comparison = AddrComp::kFilteringDisable;
      GfskHeaderType header_type = GfskHeaderType::kVariablePacket;
      uint8_t payload_length = kMaxTransmitBufferSize;
      PulseShape pulse_shape = PulseShape::kGaussianBt1;
      Sf spreading_factor = Sf::kSf9;

      struct {
        GfskCrcType type = GfskCrcType::kCrc2ByteInv;
        uint16_t initial = 0x1D0F;
        uint16_t polynomial = 0x1021;
      } crc;

      Whitening whitening = Whitening::kNoEncoding;

      uint16_t preamble_length = 32;
    } gfsk;

    struct {
      Sf spreading_factor = Sf::kSf9;
      LoraBw band_width = LoraBw::kBw125000Hz;
      Ldro low_data_rate_optimize = Ldro::kLdroOff;
      Cr cr = Cr::kCr47;
      uint16_t sync_word = 0x1424;
      uint16_t preamble_length = 8;
      LoraHeaderType header_type = LoraHeaderType::kVariableLengthPacket;
      uint8_t payload_length = kMaxTransmitBufferSize;
      LoraCrcType crc_type = LoraCrcType::kOn;
      InvertIq invert_iq = InvertIq::kStandardIqSetup;
    } lora;
  };

  // SX1262的ID为SX1261
  static constexpr const char* kDeviceId = "SX1261";
  static constexpr uint16_t kBusyPinTimeoutCount = 10000;
  static constexpr uint8_t kBusyFunctionTimeoutCount = 100;
  static constexpr uint8_t kMaxTransmitBufferSize = 255;

  ChipType chip_type_;
  Param param_;
  int32_t rst_;
  uint8_t assert_ = 0;
  int32_t busy_ = CPP_BUS_DRIVER_DEFAULT_VALUE;
  bool (*busy_wait_callback_)() = nullptr;
};
}  // namespace cpp_bus_driver