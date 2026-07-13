/*
 * @Description: Texas Instruments CC1101 transceiver driver
 * @Author: LILYGO_L
 * @Date: 2026-07-12
 * @LastEditTime: 2026-07-12 13:27:05
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Cc1101 final : public ChipSpiGuide {
 public:
  // CC1101 配置、状态、PATABLE 和 FIFO 寄存器地址
  enum class Register : uint8_t {
    kIocfg2 = 0x00,
    kIocfg1 = 0x01,
    kIocfg0 = 0x02,
    kFifothr = 0x03,
    kSync1 = 0x04,
    kSync0 = 0x05,
    kPktlen = 0x06,
    kPktctrl1 = 0x07,
    kPktctrl0 = 0x08,
    kAddr = 0x09,
    kChannr = 0x0A,
    kFsctrl1 = 0x0B,
    kFsctrl0 = 0x0C,
    kFreq2 = 0x0D,
    kFreq1 = 0x0E,
    kFreq0 = 0x0F,
    kMdmcfg4 = 0x10,
    kMdmcfg3 = 0x11,
    kMdmcfg2 = 0x12,
    kMdmcfg1 = 0x13,
    kMdmcfg0 = 0x14,
    kDeviatn = 0x15,
    kMcsm2 = 0x16,
    kMcsm1 = 0x17,
    kMcsm0 = 0x18,
    kFoccfg = 0x19,
    kBscfg = 0x1A,
    kAgcctrl2 = 0x1B,
    kAgcctrl1 = 0x1C,
    kAgcctrl0 = 0x1D,
    kWorevt1 = 0x1E,
    kWorevt0 = 0x1F,
    kWorctrl = 0x20,
    kFrend1 = 0x21,
    kFrend0 = 0x22,
    kFscal3 = 0x23,
    kFscal2 = 0x24,
    kFscal1 = 0x25,
    kFscal0 = 0x26,
    kRcctrl1 = 0x27,
    kRcctrl0 = 0x28,
    kFstest = 0x29,
    kPtest = 0x2A,
    kAgctest = 0x2B,
    kTest2 = 0x2C,
    kTest1 = 0x2D,
    kTest0 = 0x2E,
    kPartnum = 0x30,
    kVersion = 0x31,
    kFreqest = 0x32,
    kLqi = 0x33,
    kRssi = 0x34,
    kMarcstate = 0x35,
    kWortime1 = 0x36,
    kWortime0 = 0x37,
    kPktstatus = 0x38,
    kVcoVcDac = 0x39,
    kTxbytes = 0x3A,
    kRxbytes = 0x3B,
    kRcctrl1Status = 0x3C,
    kRcctrl0Status = 0x3D,
    kPatable = 0x3E,
    kFifo = 0x3F,
  };

  // CC1101 SPI 命令选通指令
  enum class Command : uint8_t {
    kReset = 0x30,
    kFrequencySynthesizerOn = 0x31,
    kCrystalOff = 0x32,
    kCalibrate = 0x33,
    kReceive = 0x34,
    kTransmit = 0x35,
    kIdle = 0x36,
    kWakeOnRadio = 0x38,
    kPowerDown = 0x39,
    kFlushRx = 0x3A,
    kFlushTx = 0x3B,
    kWakeOnRadioReset = 0x3C,
    kNoOperation = 0x3D,
  };

  // MARCSTATE 寄存器中常用的主状态机状态
  enum class State : uint8_t {
    kSleep = 0x00,
    kIdle = 0x01,
    kCrystalOff = 0x02,
    kManualCalibration = 0x05,
    kFrequencySynthesizerLock = 0x0A,
    kReceive = 0x0D,
    kRxFifoOverflow = 0x11,
    kFrequencySynthesizerOn = 0x12,
    kTransmit = 0x13,
    kTxFifoUnderflow = 0x16,
  };

  // MDMCFG2.MOD_FORMAT 支持的调制格式
  enum class Modulation : uint8_t {
    k2Fsk = 0x00,
    kGfsk = 0x01,
    kAskOok = 0x03,
    k4Fsk = 0x04,
    kMsk = 0x07,
  };

  // 数据编码或白化方式
  enum class Encoding : uint8_t {
    kNrz,
    kManchester,
    kWhitening,
  };

  // PKTCTRL0.LENGTH_CONFIG 数据包长度模式
  enum class PacketLengthMode : uint8_t {
    kFixed = 0,
    kVariable = 1,
    kInfinite = 2,  // 仅供原始寄存器流程，封装收发接口不支持
  };

  // PKTCTRL1.ADR_CHK 地址过滤模式
  enum class AddressCheck : uint8_t {
    kDisabled = 0,
    kNoBroadcast = 1,
    kBroadcast0 = 2,
    kBroadcast0And255 = 3,
  };

  // MDMCFG2.SYNC_MODE 同步字检测模式
  enum class SyncMode : uint8_t {
    kDisabled = 0,
    kFifteenOfSixteen = 1,
    kSixteenOfSixteen = 2,
    kThirtyOfThirtyTwo = 3,
    kDisabledWithCarrierSense = 4,
    kFifteenOfSixteenWithCarrierSense = 5,
    kSixteenOfSixteenWithCarrierSense = 6,
    kThirtyOfThirtyTwoWithCarrierSense = 7,
  };

  // MCSM1.CCA_MODE 发射前的信道检测条件
  enum class CcaMode : uint8_t {
    kAlways = 0,
    kRssiBelowThreshold = 1,
    kUnlessReceiving = 2,
    kRssiBelowThresholdUnlessReceiving = 3,
  };

  // 用于批量应用 SmartRF Studio 导出值的寄存器配置项
  struct RegisterSetting {
    Register address;  // 配置寄存器地址
    uint8_t value;     // 写入值
  };

  // CC1101 初始化及射频数据包配置
  struct Config {
    double crystal_frequency_mhz = 26.0;  // 晶振频率，单位 MHz
    double frequency_mhz = 868.0;         // 射频中心频率，单位 MHz
    double data_rate_kbaud = 4.8;         // 符号速率，单位 kBaud
    double frequency_deviation_khz = 5.0;  // FSK 频偏，单位 kHz
    uint8_t msk_phase_change_period = 7;   // MSK 相位变化周期
    double receive_bandwidth_khz = 58.0;   // RX 滤波带宽，单位 kHz
    double channel_spacing_khz = 199.95;   // 信道间隔，单位 kHz
    int8_t output_power_dbm = 10;          // 发射功率，单位 dBm
    uint16_t preamble_length_bits = 32;    // 前导码长度，单位 bit
    uint8_t preamble_quality_threshold = 0;  // PKTCTRL1.PQT
    uint8_t sync_word_high = 0x12;         // 同步字高字节
    uint8_t sync_word_low = 0xAD;          // 同步字低字节
    Modulation modulation = Modulation::k2Fsk;  // 调制格式
    Encoding encoding = Encoding::kNrz;         // 数据编码
    PacketLengthMode packet_length_mode =
        PacketLengthMode::kVariable;  // 数据包长度模式
    AddressCheck address_check =
        AddressCheck::kDisabled;  // 地址过滤模式
    SyncMode sync_mode =
        SyncMode::kSixteenOfSixteen;  // 同步字检测条件
    uint8_t maximum_packet_length = 255;  // PKTLEN 寄存器值
    uint8_t device_address = 0;           // 本机地址
    uint8_t channel = 0;                  // CHANNR 信道编号
    uint8_t bit_rate_tolerance = 0;        // BSCFG.BS_LIMIT
    int8_t carrier_sense_threshold = 0;    // 绝对载波侦听阈值
    uint8_t carrier_sense_relative = 0;    // 相对载波侦听阈值
    CcaMode cca_mode =
        CcaMode::kRssiBelowThresholdUnlessReceiving;
    bool crc_enabled = true;              // 硬件 CRC
    bool crc_autoflush = false;            // CRC 失败自动清空 RX FIFO
    bool append_status = true;            // RX FIFO 附加 RSSI/LQI
    bool fec_enabled = false;             // 卷积编码 FEC
  };

  // SPI 状态字节解析结果
  struct ChipStatus {
    bool ready = false;                   // CHIP_RDYn 为低
    uint8_t state = 0;                    // 主状态机状态位
    uint8_t fifo_bytes_available = 0;     // FIFO 可用字节提示
  };

  // 最近接收数据包的链路质量
  struct PacketMetrics {
    float rssi_dbm = 0.0F;  // 接收信号强度，单位 dBm
    uint8_t lqi = 0;        // 链路质量，范围 0~127
    bool crc_valid = false;  // 硬件 CRC 校验结果
  };

  /**
   * @brief 创建采用默认射频配置的 CC1101 驱动。
   * @param bus SPI 总线对象，必须配置为模式 0。
   * @param cs CC1101 CS 引脚。
   * @param miso SPI MISO 引脚，同时用于读取 CHIP_RDYn。
   * @param gdo0 可选 GDO0 引脚，用于阻塞收发和接收中断。
   * @param gdo2 可选 GDO2 引脚，保留给板级状态信号使用。
   */
  explicit Cc1101(std::shared_ptr<BusSpiGuide> bus, int32_t cs,
      int32_t miso, int32_t gdo0 = kDefaultValue,
      int32_t gdo2 = kDefaultValue)
      : Cc1101(bus, cs, miso, gdo0, gdo2, Config{}) {}

  /**
   * @brief 创建采用指定射频配置的 CC1101 驱动。
   * @param bus SPI 总线对象，必须配置为模式 0。
   * @param cs CC1101 CS 引脚。
   * @param miso SPI MISO 引脚，同时用于读取 CHIP_RDYn。
   * @param gdo0 可选 GDO0 引脚。
   * @param gdo2 可选 GDO2 引脚。
   * @param config 初始化时应用的射频及数据包配置。
   */
  explicit Cc1101(std::shared_ptr<BusSpiGuide> bus, int32_t cs,
      int32_t miso, int32_t gdo0, int32_t gdo2,
      const Config& config)
      : ChipSpiGuide(bus, cs),
        config_(config),
        miso_(miso),
        gdo0_(gdo0),
        gdo2_(gdo2) {}

  /**
   * @brief 初始化 SPI、复位芯片、检查型号并应用 Config。
   * @param freq_hz SPI 时钟，CC1101 最大支持 10 MHz。
   * @return 初始化成功返回 true，失败返回 false。
   */
  bool Init(int32_t freq_hz = 10000000) override;

  /**
   * @brief 使芯片进入掉电模式并释放 SPI 设备和专用 GPIO。
   * @param delete_bus 是否尝试释放底层 SPI 总线。
   * @return 操作成功返回 true，失败返回 false。
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 执行数据手册规定的手动 SPI 复位序列。
   * @return 复位成功并等待芯片就绪后返回 true，否则返回 false。
   */
  bool Reset();

  /**
   * @brief 一次性应用完整射频及数据包配置。
   * @param config 需要应用的配置。
   * @return 全部配置成功返回 true，失败返回 false。
   */
  bool Configure(const Config& config);

  /**
   * @brief 批量写入 SmartRF Studio 或其他工具导出的寄存器值。
   * @param settings 寄存器配置数组。
   * @param count 配置项数量。
   * @param config 与导出寄存器值对应的高层配置。
   * @return 全部写入成功返回 true，失败返回 false。
   */
  bool ApplyRegisterSettings(
      const RegisterSetting* settings, size_t count,
      const Config& config);

  /**
   * @brief 写入单个配置寄存器。
   * @param address 目标配置寄存器地址。
   * @param value 需要写入的值。
   * @return 写入成功返回 true，失败返回 false。
   */
  bool WriteRegister(Register address, uint8_t value);

  /**
   * @brief 读取单个配置寄存器或 FIFO 字节。
   * @param address 目标寄存器地址。
   * @param value 读取结果输出地址。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool ReadRegister(Register address, uint8_t* value);

  /**
   * @brief 从指定地址开始执行 SPI 连续写入。
   * @param address 连续写入的起始地址。
   * @param data 需要写入的数据。
   * @param length 写入字节数。
   * @return 连续写入成功返回 true，失败返回 false。
   */
  bool WriteBurst(Register address, const uint8_t* data, size_t length);

  /**
   * @brief 从指定地址开始执行 SPI 连续读取。
   * @param address 连续读取的起始地址。
   * @param data 读取数据输出缓冲区。
   * @param length 读取字节数。
   * @return 连续读取成功返回 true，失败返回 false。
   */
  bool ReadBurst(Register address, uint8_t* data, size_t length);

  /**
   * @brief 读取状态寄存器。
   * @param address 目标状态寄存器地址。
   * @param value 读取结果输出地址。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool ReadStatusRegister(Register address, uint8_t* value);

  /**
   * @brief 发送命令选通指令并可选返回 SPI 状态字节。
   * @param command 需要发送的命令。
   * @param status 可选状态输出，允许为 nullptr。
   * @return 命令发送成功返回 true，失败返回 false。
   */
  bool Strobe(Command command, ChipStatus* status = nullptr);

  /**
   * @brief 设置射频中心频率。
   * @param frequency_mhz 频率，27 MHz 晶振时中频段从 392 MHz 开始。
   * @return 频率设置成功返回 true，参数无效或写入失败返回 false。
   */
  bool SetFrequency(double frequency_mhz);

  /**
   * @brief 按当前调制格式设置数据速率。
   * @param data_rate_kbaud 符号速率，单位 kBaud。
   * @return 数据速率设置成功返回 true，否则返回 false。
   */
  bool SetDataRate(double data_rate_kbaud);

  /**
   * @brief 设置 FSK 频偏并自动选择 DEVIATN 参数。
   * @param deviation_khz 频偏，单位 kHz。
   * @return 频偏设置成功返回 true，否则返回 false。
   */
  bool SetFrequencyDeviation(double deviation_khz);

  /**
   * @brief 设置 MSK 相位变化周期。
   * @param period DEVIATN.MSK_PHASE_CHANGE_PERIOD，范围 0~7。
   * @return 参数有效且设置成功返回 true，否则返回 false。
   */
  bool SetMskPhaseChangePeriod(uint8_t period);

  /**
   * @brief 设置接收滤波器带宽并选择最接近的硬件值。
   * @param bandwidth_khz 接收带宽，单位 kHz。
   * @return 接收带宽设置成功返回 true，否则返回 false。
   */
  bool SetReceiveBandwidth(double bandwidth_khz);

  /**
   * @brief 设置信道间隔并自动选择 MDMCFG1/MDMCFG0 参数。
   * @param spacing_khz 信道间隔，单位 kHz。
   * @return 信道间隔设置成功返回 true，否则返回 false。
   */
  bool SetChannelSpacing(double spacing_khz);

  /**
   * @brief 设置位同步器允许的数据速率误差。
   * @param tolerance BSCFG.BS_LIMIT 原始值，范围 0~3。
   * @return 参数有效且设置成功返回 true，否则返回 false。
   */
  bool SetBitRateTolerance(uint8_t tolerance);

  /**
   * @brief 按当前频段写入 TI 推荐的 PATABLE 发射功率值。
   * @param power_dbm 支持 -30、-20、-15、-10、0、5、7、10 dBm。
   * @return 发射功率设置成功返回 true，否则返回 false。
   */
  bool SetOutputPower(int8_t power_dbm);

  /**
   * @brief 直接设置 PATABLE，供 SmartRF Studio 配置使用。
   * @param pa_value PATABLE 原始值，禁止使用数据手册保留值。
   * @return PATABLE 设置成功返回 true，否则返回 false。
   */
  bool SetOutputPowerRaw(uint8_t pa_value);

  /**
   * @brief 设置 2-FSK、GFSK、ASK/OOK、4-FSK 或 MSK 调制。
   * @param modulation 目标调制格式。
   * @return 调制格式和对应功率表设置成功返回 true，否则返回 false。
   */
  bool SetModulation(Modulation modulation);

  /**
   * @brief 设置 NRZ、Manchester 或数据白化。
   * @param encoding 目标编码方式。
   * @return 编码设置成功返回 true，配置冲突或写入失败返回 false。
   */
  bool SetEncoding(Encoding encoding);

  /**
   * @brief 设置 16 位同步字及检测模式。
   * @param high 同步字高字节。
   * @param low 同步字低字节。
   * @param mode 同步字匹配和载波侦听模式。
   * @return 同步字和检测模式设置成功返回 true，否则返回 false。
   */
  bool SetSyncWord(uint8_t high, uint8_t low,
      SyncMode mode = SyncMode::kSixteenOfSixteen);

  /**
   * @brief 设置前导码长度。
   * @param length_bits 支持 16、24、32、48、64、96、128、192 bit。
   * @return 前导码设置成功返回 true，长度不受支持时返回 false。
   */
  bool SetPreambleLength(uint16_t length_bits);

  /**
   * @brief 设置前导码质量检测阈值。
   * @param threshold PKTCTRL1.PQT 原始值，范围 0~7。
   * @return 参数有效且设置成功返回 true，否则返回 false。
   */
  bool SetPreambleQualityThreshold(uint8_t threshold);

  /**
   * @brief 设置固定或可变长度数据包模式。
   * @param mode 数据包长度模式，kInfinite 会返回 false。
   * @param maximum_length 写入 PKTLEN 的最大空中包长。
   * @return 数据包长度模式设置成功返回 true，否则返回 false。
   */
  bool SetPacketLengthMode(PacketLengthMode mode,
      uint8_t maximum_length = 255);

  /**
   * @brief 设置接收地址过滤和本机地址。
   * @param check 地址检查及广播接收策略。
   * @param device_address 本机地址。
   * @return 地址过滤配置成功返回 true，否则返回 false。
   */
  bool SetAddressCheck(AddressCheck check, uint8_t device_address = 0);

  /**
   * @brief 开启或关闭硬件 CRC 生成和校验。
   * @param enabled true 开启 CRC，false 关闭 CRC。
   * @return 设置成功返回 true，失败返回 false。
   */
  bool SetCrc(bool enabled);

  /**
   * @brief 设置 CRC 校验失败时是否自动清空 RX FIFO。
   * @param enabled true 开启自动清空，false 关闭自动清空。
   * @return 当前包长配置支持且设置成功返回 true，否则返回 false。
   */
  bool SetCrcAutoflush(bool enabled);

  /**
   * @brief 开启或关闭 FEC。
   * @param enabled true 开启 FEC，false 关闭 FEC。
   * @return 配置兼容且设置成功返回 true，否则返回 false。
   */
  bool SetFec(bool enabled);

  /**
   * @brief 设置 CHANNR 信道编号。
   * @param channel 信道编号。
   * @return 设置成功返回 true，失败返回 false。
   */
  bool SetChannel(uint8_t channel);

  /**
   * @brief 设置绝对和相对载波侦听阈值。
   * @param absolute_threshold 绝对阈值，范围 -8~7 dB。
   * @param relative_threshold 相对阈值编码，范围 0~3。
   * @return 参数有效且设置成功返回 true，否则返回 false。
   */
  bool SetCarrierSenseThreshold(
      int8_t absolute_threshold, uint8_t relative_threshold = 0);

  /**
   * @brief 设置 STX 命令采用的 CCA 条件。
   * @param mode 发射前信道检测模式。
   * @return 设置成功返回 true，失败返回 false。
   */
  bool SetCcaMode(CcaMode mode);

  /**
   * @brief 设置 GDO0 或 GDO2 的内部信号映射。
   * @param output 仅允许 Register::kIocfg0 或 Register::kIocfg2。
   * @param signal GDOx_CFG 信号编号，范围 0x00~0x3F。
   * @param inverted true 输出反相，false 正常输出。
   * @return 参数有效且设置成功返回 true，否则返回 false。
   */
  bool SetGdoMapping(Register output, uint8_t signal, bool inverted = false);

  /**
   * @brief 进入 IDLE 状态。
   * @param timeout_ms 等待 MARCSTATE 进入 IDLE 的超时时间。
   * @return 状态机在超时前进入 IDLE 返回 true，否则返回 false。
   */
  bool Standby(uint32_t timeout_ms = 100);

  /**
   * @brief 从 IDLE 进入最低功耗的 SLEEP 状态。
   * @return 进入 SLEEP 状态成功返回 true，失败返回 false。
   */
  bool Sleep();

  /**
   * @brief 从 SLEEP 唤醒并恢复易失的校准值和 PATABLE 内容。
   * @return 唤醒及配置恢复成功返回 true，否则返回 false。
   */
  bool Wakeup();

  /**
   * @brief 从 IDLE 启动一次频率合成器校准并等待完成。
   * @param timeout_ms 等待校准完成的超时时间，单位 ms。
   * @return 校准完成并返回 IDLE 时返回 true，否则返回 false。
   */
  bool Calibrate(uint32_t timeout_ms = 100);

  /**
   * @brief 清空 RX FIFO 并启动异步数据包接收。
   *
   * 该模式不会在包接收期间排空 FIFO，因此完整空中包、
   * 可选长度字节和附加状态的总长必须不超过 64 字节。
   *
   * @return RX FIFO 清空且接收模式启动成功返回 true，否则返回 false。
   */
  bool StartReceive();

  /**
   * @brief 先进入 IDLE，再清空 RX FIFO。
   * @return RX FIFO 清空成功返回 true，失败返回 false。
   */
  bool FlushRx();

  /**
   * @brief 先进入 IDLE，再清空 TX FIFO。
   * @return TX FIFO 清空成功返回 true，失败返回 false。
   */
  bool FlushTx();

  /**
   * @brief 阻塞发送数据包，并按 CCA 设置检查信道。
   * @param data 用户负载。
   * @param length 用户负载长度，不包含长度字节和目标地址。
   * @param timeout_ms 超时时间；传入 0 时根据数据速率自动计算。
   * @param destination 需要写入数据包的目标地址。
   * @param include_destination 是否强制写入目标地址字段。
   * @return 发送完成且状态恢复成功返回 true。
   */
  bool Transmit(const uint8_t* data, size_t length,
      uint32_t timeout_ms = 0, uint8_t destination = 0,
      bool include_destination = false);

  /**
   * @brief 阻塞等待并接收数据包，长包会自动排空 RX FIFO。
   * @param data 用户负载输出缓冲区。
   * @param capacity 输出缓冲区容量。
   * @param received 实际收到的用户负载长度。
   * @param metrics 可选 RSSI、LQI 和 CRC 输出。
   * @param timeout_ms 超时时间；传入 0 时根据最大包长自动计算。
   * @return 完整包接收成功且 CRC 有效时返回 true，否则返回 false。
   */
  bool Receive(uint8_t* data, size_t capacity, size_t* received,
      PacketMetrics* metrics = nullptr, uint32_t timeout_ms = 0);

  /**
   * @brief 从异步接收模式的 RX FIFO 读取一个完整数据包。
   *
   * 可变包且附加状态时最大空中包长为 61 字节；
   * 启用地址字段时，用户负载容量再减少 1 字节。
   *
   * @param data 用户负载输出缓冲区。
   * @param capacity 输出缓冲区容量。
   * @param received 实际收到的用户负载长度。
   * @param metrics 可选 RSSI、LQI 和 CRC 输出。
   * @return 完整包存在且 CRC 有效时返回 true。
   */
  bool ReadReceivedPacket(uint8_t* data, size_t capacity,
      size_t* received, PacketMetrics* metrics = nullptr);

  /**
   * @brief 读取稳定的 MARCSTATE 主状态机状态。
   * @param state 主状态机状态输出地址。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool GetState(State* state);

  /**
   * @brief 读取 PARTNUM 状态寄存器，CC1101 应返回 0。
   * @param part_number 芯片型号输出地址。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool GetPartNumber(uint8_t* part_number);

  /**
   * @brief 读取 VERSION 状态寄存器。
   * @param version 芯片版本输出地址。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool GetVersion(uint8_t* version);

  /**
   * @brief 读取并换算当前 RSSI。
   * @param rssi_dbm 接收信号强度输出地址，单位 dBm。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool GetRssi(float* rssi_dbm);

  /**
   * @brief 读取当前 LQI。
   * @param lqi 链路质量输出地址，范围 0~127。
   * @return 读取成功返回 true，失败返回 false。
   */
  bool GetLqi(uint8_t* lqi);

  /**
   * @brief 发送 SNOP 并解析返回的 SPI 状态字节。
   * @param status 芯片状态输出地址。
   * @return 状态读取并解析成功返回 true，失败返回 false。
   */
  bool GetChipStatus(ChipStatus* status);

  /**
   * @brief 返回驱动当前保存的配置。
   * @return 当前配置的只读引用。
   */
  const Config& config() const { return config_; }

 private:
  static constexpr size_t kFifoSize = 64;
  static constexpr size_t kMaximumPacketLength = 255;
  static constexpr uint8_t kReadSingle = 0x80;
  static constexpr uint8_t kBurst = 0x40;
  static constexpr uint8_t kReadBurst = kReadSingle | kBurst;
  static constexpr uint8_t kChipReadyMask = 0x80;
  static constexpr uint8_t kStateMask = 0x70;
  static constexpr uint8_t kFifoCountMask = 0x0F;
  static constexpr uint8_t kStatusFifoCountMask = 0x7F;
  static constexpr uint8_t kStatusFifoErrorMask = 0x80;
  static constexpr uint8_t kCrcValidMask = 0x80;
  static constexpr uint32_t kReadyTimeoutUs = 5000;

  /**
   * @brief 执行一次满足 CC1101 CHIP_RDYn 时序要求的 SPI 传输。
   * @param write_data SPI 发送缓冲区。
   * @param read_data SPI 接收缓冲区。
   * @param length 传输字节数。
   * @param wait_ready CS 拉低后是否等待 CHIP_RDYn 变低。
   * @return 传输成功返回 true，失败返回 false。
   */
  bool Transfer(const uint8_t* write_data, uint8_t* read_data,
      size_t length, bool wait_ready = true);

  /**
   * @brief 确保芯片处于允许修改配置寄存器的 IDLE 状态。
   * @return 芯片已经或成功进入 IDLE 返回 true，否则返回 false。
   */
  bool EnsureIdle();

  /**
   * @brief 等待 MISO 上的 CHIP_RDYn 信号变低。
   * @param timeout_us 最大等待时间，单位 us。
   * @return 芯片在超时前就绪返回 true，否则返回 false。
   */
  bool WaitForReady(uint32_t timeout_us = kReadyTimeoutUs);

  /**
   * @brief 等待 GDO0 进入指定电平。
   * @param level 需要等待的目标电平。
   * @param timeout_ms 最大等待时间，单位 ms。
   * @return GDO0 在超时前达到目标电平返回 true，否则返回 false。
   */
  bool WaitForGdo0(bool level, uint32_t timeout_ms);

  /**
   * @brief 等待主状态机进入指定状态。
   * @param state 需要等待的 MARCSTATE 状态。
   * @param timeout_ms 最大等待时间，单位 ms。
   * @return 状态机在超时前进入目标状态返回 true，否则返回 false。
   */
  bool WaitForState(State state, uint32_t timeout_ms);

  /**
   * @brief 按 TI 勘误要求稳定读取连续变化的状态寄存器。
   * @param address 需要读取的状态寄存器地址。
   * @param value 两次连续读数一致后的输出值。
   * @return 获得稳定读数返回 true，失败返回 false。
   */
  bool ReadStableStatus(Register address, uint8_t* value);

  /**
   * @brief 读取、修改并写回配置寄存器中的指定位。
   * @param address 配置寄存器地址。
   * @param mask 需要更新的位掩码。
   * @param value 掩码范围内的新值。
   * @return 寄存器更新成功返回 true，失败返回 false。
   */
  bool UpdateRegisterBits(
      Register address, uint8_t mask, uint8_t value);

  /**
   * @brief 从 RX FIFO 解析并读取一个已完整到达的数据包。
   * @param data 用户负载输出缓冲区。
   * @param capacity 输出缓冲区容量。
   * @param available RX FIFO 当前可读字节数。
   * @param received 实际读取的用户负载长度。
   * @param metrics 可选 RSSI、LQI 和 CRC 输出。
   * @return 数据包完整且 CRC 有效时返回 true，否则返回 false。
   */
  bool ReadPacketFromFifo(uint8_t* data, size_t capacity,
      size_t available, size_t* received, PacketMetrics* metrics);

  /**
   * @brief 将指定数量的数据从 RX FIFO 追加到用户缓冲区。
   * @param data 用户负载输出缓冲区。
   * @param capacity 输出缓冲区容量。
   * @param copied 已复制长度，成功后同步增加。
   * @param bytes_to_read 本次需要从 FIFO 读取的字节数。
   * @return 缓冲区容量足够且读取成功返回 true，否则返回 false。
   */
  bool DrainReceiveFifo(uint8_t* data, size_t capacity,
      size_t* copied, size_t bytes_to_read);

  /**
   * @brief 从状态寄存器读取最近数据包的 RSSI、LQI 和 CRC。
   * @param metrics 可选数据包指标输出地址。
   * @return 状态读取成功且 CRC 有效时返回 true，否则返回 false。
   */
  bool ReadPacketMetrics(PacketMetrics* metrics);

  /**
   * @brief 返回指定 SPI 起始地址允许的最大连续访问长度。
   * @param address 连续访问起始地址。
   * @return 允许长度；返回 0 表示不支持连续访问。
   */
  size_t GetMaximumBurstLength(Register address) const;

  /**
   * @brief 按当前频段选择 TI 推荐的 PATABLE 值。
   * @param power_dbm 目标发射功率，单位 dBm。
   * @param value 匹配到的 PATABLE 原始值。
   * @return 当前频段和功率存在对应值时返回 true，否则返回 false。
   */
  bool SelectPaValue(int8_t power_dbm, uint8_t* value) const;

  /**
   * @brief 检查完整高层配置是否符合 CC1101 限制。
   * @param config 需要检查的配置。
   * @return 配置有效返回 true，否则返回 false。
   */
  bool ValidateConfig(const Config& config) const;

  /**
   * @brief 检查符号速率是否符合指定调制格式的官方范围。
   * @param data_rate_kbaud 符号速率，单位 kBaud。
   * @param modulation 目标调制格式。
   * @return 数据速率有效返回 true，否则返回 false。
   */
  bool ValidateDataRate(
      double data_rate_kbaud, Modulation modulation) const;

  /**
   * @brief 获取当前调制格式每个符号携带的原始比特数。
   * @return 4-FSK 返回 2，其他调制格式返回 1。
   */
  uint8_t GetBitsPerSymbol() const;

  /**
   * @brief 检查前导码长度是否可由 MDMCFG1 表示。
   * @param length_bits 前导码长度，单位 bit。
   * @return 长度可表示返回 true，否则返回 false。
   */
  bool ValidatePreambleLength(uint16_t length_bits) const;

  /**
   * @brief 检查功率是否存在 TI 多层电感 PATABLE 推荐值。
   * @param power_dbm 目标发射功率，单位 dBm。
   * @return 存在推荐值返回 true，否则返回 false。
   */
  bool ValidateOutputPower(int8_t power_dbm) const;

  /**
   * @brief 检查频率是否处于 CC1101 支持的三个频段之一。
   * @param frequency_mhz 需要检查的频率，单位 MHz。
   * @param crystal_frequency_mhz 晶振频率，单位 MHz。
   * @return 频率有效返回 true，否则返回 false。
   */
  bool ValidateFrequency(
      double frequency_mhz, double crystal_frequency_mhz) const;

  /**
   * @brief 根据数据速率和负载长度估算阻塞发送超时时间。
   * @param length 用户负载长度。
   * @return 建议超时时间，单位 ms。
   */
  uint32_t CalculatePacketTimeoutMs(size_t length) const;

  /**
   * @brief 计算满足 RX FIFO 勘误要求的轮询间隔。
   * @return 建议轮询间隔，单位 us。
   */
  uint32_t CalculateFifoPollIntervalUs() const;

  /**
   * @brief 恢复 SLEEP 状态不会保留的射频配置。
   * @return TEST/FSCAL 和 PATABLE 恢复成功返回 true，否则返回 false。
   */
  bool RestoreAfterWakeup();

  /**
   * @brief 获取当前单调系统时间。
   * @return 当前系统时间，单位 us。
   */
  int64_t CurrentTimeUs() const;

  /**
   * @brief 获取当前单调系统时间。
   * @return 当前系统时间，单位 ms。
   */
  int64_t CurrentTimeMs() const;

  /**
   * @brief 将 RSSI 状态寄存器原始值换算为 dBm。
   * @param raw RSSI 状态寄存器原始值。
   * @return 换算后的接收信号强度，单位 dBm。
   */
  float DecodeRssi(uint8_t raw) const;

  /**
   * @brief 解析 SPI 传输返回的芯片状态字节。
   * @param raw SPI 状态原始值。
   * @return 解析后的就绪状态、主状态机状态和 FIFO 提示。
   */
  ChipStatus ParseChipStatus(uint8_t raw) const;

  Config config_;
  int32_t miso_ = kDefaultValue;
  int32_t gdo0_ = kDefaultValue;
  int32_t gdo2_ = kDefaultValue;
  PacketMetrics last_metrics_;
  uint8_t pa_table_cache_[8] = {0xC6};
  size_t pa_table_length_ = 1;
  uint8_t test0_value_ = 0x0B;
  uint8_t fscal2_value_ = 0x0A;
  bool sleeping_ = false;
  bool initialized_ = false;
  int32_t spi_frequency_hz_ = 10000000;
  mutable uint32_t last_micros_ = 0;
  mutable uint64_t micros_epoch_ = 0;
};
}  // namespace cpp_bus_driver
