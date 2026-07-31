/*
 * @Description: Nordic nRF24L01 系列 2.4 GHz 射频收发芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-07-31 10:00:00
 * @LastEditTime: 2026-07-31 10:00:00
 * @License: GPL 3.0
 */
#include "nrf24l01x.h"

#include <algorithm>
#include <array>

namespace cpp_bus_driver {
namespace {

// 下列掩码直接对应 Nordic 产品规格书第 9 章寄存器位，避免依赖 8051 位域布局。
constexpr uint8_t kConfigPrimaryRxMask = 0x01;        // 主接收端角色位
constexpr uint8_t kConfigPowerUpMask = 0x02;          // 射频电源状态位
constexpr uint8_t kConfigCrcLengthMask = 0x04;        // CRC 字节数选择位
constexpr uint8_t kConfigCrcEnableMask = 0x08;        // 空中 CRC 总开关
constexpr uint8_t kRfSetupPowerMask = 0x06;           // 两位发射功率字段
constexpr uint8_t kRfSetupDataRateHighMask = 0x08;    // 2 Mbps 选择位
constexpr uint8_t kRfSetupPllLockMask = 0x10;         // 射频测试锁相环位
constexpr uint8_t kRfSetupDataRateLowMask = 0x20;     // 250 kbps 选择位
constexpr uint8_t kRfSetupContinuousWaveMask = 0x80;  // 连续载波测试位
constexpr uint8_t kRfSetupLnaHighCurrentMask = 0x01;  // 旧芯片兼容保留位
constexpr uint8_t kStatusRxPipeMask = 0x0E;           // 来源管道字段
constexpr uint8_t kStatusTxFullMask = 0x01;           // TX FIFO 满快照
constexpr uint8_t kObservePacketLostMask = 0xF0;      // 累计丢包计数
constexpr uint8_t kObserveRetransmitMask = 0x0F;      // 本包重发次数
constexpr uint8_t kFifoTxReuseMask = 0x40;            // TX 负载复用标志
constexpr uint8_t kFifoTxFullMask = 0x20;             // TX FIFO 满标志
constexpr uint8_t kFifoTxEmptyMask = 0x10;            // TX FIFO 空标志
constexpr uint8_t kFifoRxFullMask = 0x02;             // RX FIFO 满标志
constexpr uint8_t kFifoRxEmptyMask = 0x01;            // RX FIFO 空标志

// 覆盖规格书列出的 90 mH 晶体等效电感最坏启动时间，避免模块尚未稳定便进入收发。
constexpr uint32_t kPowerDownToStandbyUs = 4500;
constexpr uint32_t kStandbyToActiveUs = 130;  // 射频稳定阶段最大持续时间
constexpr uint32_t kMinimumCePulseUs = 10;    // 启动单次发射的 CE 高脉宽下限
// 上电复位时间会随电源斜率和晶体而变化，探测窗口按规格书最大值设置。
constexpr uint32_t kPowerOnResetTimeoutMs = 100;

/**
 * @brief 检查中断枚举是否能安全转换为 CONFIG/STATUS 位号。
 * @param source 调用者传入的中断源。
 * @return 仅 RX_DR、TX_DS 和 MAX_RT 返回 true。
 */
bool IsValidIrqSource(Nrf24l01x::IrqSource source) {
  return source == Nrf24l01x::IrqSource::kRxDataReady ||
         source == Nrf24l01x::IrqSource::kTxDataSent ||
         source == Nrf24l01x::IrqSource::kMaximumRetransmit;
}

/**
 * @brief 把中断位号转换为寄存器掩码。
 * @param source RX_DR、TX_DS 或 MAX_RT 枚举。
 * @return 仅包含目标事件的一位掩码。
 */
uint8_t BitForIrq(Nrf24l01x::IrqSource source) {
  return static_cast<uint8_t>(1U << static_cast<uint8_t>(source));
}

/**
 * @brief 判断地址枚举是否表示六条接收管道之一。
 * @param address 待检查目标。
 * @return P0~P5 返回 true，TX 和批量选择返回 false。
 */
bool IsPipeAddress(Nrf24l01x::Address address) {
  return address >= Nrf24l01x::Address::kPipe0 &&
         address <= Nrf24l01x::Address::kPipe5;
}

}  // namespace

Nrf24l01x::Cmd Nrf24l01x::CmdForAddress(Address address) {
  if (address == Address::kTransmit) {
    return Cmd::kTxAddress;
  }
  return static_cast<Cmd>(static_cast<uint8_t>(Cmd::kRxAddressPipe0) +
                          static_cast<uint8_t>(address));
}

Nrf24l01x::Cmd Nrf24l01x::CmdForPayloadWidth(uint8_t pipe) {
  return static_cast<Cmd>(
      static_cast<uint8_t>(Cmd::kRxPayloadWidthPipe0) + pipe);
}

bool Nrf24l01x::Init(int32_t frequency_hz) {
  if (initialized_) {
    return true;
  }
  if (bus_ == nullptr || cs_ == kDefaultValue || ce_ == kDefaultValue) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid nRF24L01x bus or GPIO argument\n");
    return false;
  }
  if (frequency_hz == kDefaultValue) {
    frequency_hz = kMaximumSpiFrequencyHz;
  }
  if (frequency_hz <= 0 || frequency_hz > kMaximumSpiFrequencyHz) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid nRF24L01x SPI frequency (frequency: %d)\n", frequency_hz);
    return false;
  }

  bool result = SetGpioMode(cs_, GpioMode::kOutput);
  result &= SetGpioMode(ce_, GpioMode::kOutput);
  if (irq_ != kDefaultValue) {
    result &= SetGpioMode(irq_, GpioMode::kInput, GpioStatus::kPullup);
  }
  result &= GpioWrite(cs_, true);
  result &= GpioWrite(ce_, false);
  if (!result) {
    return FailInitialization("GPIO initialization failed");
  }

  if (!bus_->Init(frequency_hz, kDefaultValue)) {
    return FailInitialization("SPI initialization failed");
  }
  bus_initialized_ = true;
  spi_frequency_hz_ = frequency_hz;

  // 已稳定工作的芯片会立即通过；刚上电的模块则在 TPOR 窗口内逐毫秒重试。
  const int64_t probe_start_ms = GetSystemTimeMs();
  bool detected = false;
  do {
    detected = Probe();
    if (!detected) {
      DelayMs(1);
    }
  } while (!detected && GetSystemTimeMs() - probe_start_ms <
                            static_cast<int64_t>(kPowerOnResetTimeoutMs));
  if (!detected) {
    return FailInitialization("nRF24L01x probe failed");
  }
  if (!Configure(Config{})) {
    return FailInitialization("nRF24L01x default configuration failed");
  }

  initialized_ = true;
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "nRF24L01x initialization success (SPI: %d Hz)\n", spi_frequency_hz_);
  return true;
}

bool Nrf24l01x::Deinit(bool delete_bus) {
  bool result = true;
  if (bus_initialized_) {
    result &= PowerDown();
    result &= bus_ != nullptr && bus_->Deinit(delete_bus);
  }
  if (cs_ != kDefaultValue) {
    result &= ResetGpio(cs_);
  }
  if (ce_ != kDefaultValue) {
    result &= ResetGpio(ce_);
  }
  if (irq_ != kDefaultValue) {
    result &= ResetGpio(irq_);
  }

  bus_initialized_ = false;
  initialized_ = false;
  powered_up_ = false;
  receiving_ = false;
  spi_frequency_hz_ = kDefaultValue;
  return result;
}

bool Nrf24l01x::Configure(const Config& config) {
  if (!ValidateConfig(config) || !bus_initialized_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid nRF24L01x configuration\n");
    return false;
  }

  bool result = SetCe(false);

  uint8_t config_value = 0;
  if (config.operation_mode == OperationMode::kPrimaryReceiver) {
    config_value |= kConfigPrimaryRxMask;
  }
  if (config.power_mode == PowerMode::kPowerUp) {
    config_value |= kConfigPowerUpMask;
  }
  if (config.crc_mode != CrcMode::kDisabled) {
    config_value |= kConfigCrcEnableMask;
  }
  if (config.crc_mode == CrcMode::k16Bit) {
    config_value |= kConfigCrcLengthMask;
  }
  if (!config.maximum_retransmit_irq) {
    config_value |= BitForIrq(IrqSource::kMaximumRetransmit);
  }
  if (!config.tx_data_sent_irq) {
    config_value |= BitForIrq(IrqSource::kTxDataSent);
  }
  if (!config.rx_data_ready_irq) {
    config_value |= BitForIrq(IrqSource::kRxDataReady);
  }

  const uint8_t retransmit_delay =
      static_cast<uint8_t>((config.retransmit_delay_us / 250U) - 1U);
  const uint8_t retransmission =
      static_cast<uint8_t>((retransmit_delay << 4) | config.retransmit_count);
  const uint8_t address_width =
      static_cast<uint8_t>(static_cast<uint8_t>(config.address_width) - 2U);

  uint8_t rf_setup =
      static_cast<uint8_t>(static_cast<uint8_t>(config.output_power) << 1);
  if (config.data_rate == DataRate::k2Mbps) {
    rf_setup |= kRfSetupDataRateHighMask;
  } else if (config.data_rate == DataRate::k250Kbps) {
    rf_setup |= kRfSetupDataRateLowMask;
  }

  uint8_t feature = 0;
  if (config.dynamic_payload_enabled) {
    feature |= kDynamicPayloadFeatureMask;
  }
  if (config.ack_payload_enabled) {
    feature |= kAckPayloadFeatureMask;
  }
  if (config.dynamic_ack_enabled) {
    feature |= kDynamicAckFeatureMask;
  }

  result &= WriteRegister(Cmd::kConfig, config_value);
  result &=
      WriteRegister(Cmd::kEnableAutoAcknowledgment, config.auto_ack_pipe_mask);
  result &= WriteRegister(Cmd::kEnableRxAddress, config.enabled_pipe_mask);
  result &= WriteRegister(Cmd::kSetupAddressWidth, address_width);
  result &= WriteRegister(Cmd::kSetupRetransmission, retransmission);
  result &= WriteRegister(Cmd::kRfChannel, config.rf_channel);
  result &= WriteRegister(Cmd::kRfSetup, rf_setup);
  for (uint8_t pipe = 0; pipe < config.rx_payload_width.size(); ++pipe) {
    result &=
        WriteRegister(CmdForPayloadWidth(pipe), config.rx_payload_width[pipe]);
  }

  bool feature_result = WriteRegister(Cmd::kFeature, feature);
  uint8_t feature_readback = 0;
  feature_result &= ReadRegister(Cmd::kFeature, &feature_readback);
  if (feature_result && feature_readback != feature) {
    // 旧 nRF24L01 或部分兼容芯片在 ACTIVATE 之前锁定 FEATURE。
    feature_result = ActivateFeatures();
    feature_result &= WriteRegister(Cmd::kFeature, feature);
    feature_result &= ReadRegister(Cmd::kFeature, &feature_readback);
    feature_result &= feature_readback == feature;
  }
  result &= feature_result;
  result &=
      WriteRegister(Cmd::kDynamicPayload, config.dynamic_payload_pipe_mask);

  uint8_t ignored_flags = 0;
  result &= GetClearIrqFlags(&ignored_flags);
  result &= FlushRx();
  result &= FlushTx();
  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Apply nRF24L01x configuration failed\n");
    return false;
  }

  config_ = config;
  powered_up_ = config.power_mode == PowerMode::kPowerUp;
  receiving_ = false;
  if (powered_up_) {
    DelayUs(kPowerDownToStandbyUs);
  }
  return true;
}

bool Nrf24l01x::Probe() {
  if (!bus_initialized_) {
    return false;
  }

  uint8_t original_channel = 0;
  uint8_t status = 0;
  if (!ReadRegister(Cmd::kRfChannel, &original_channel, &status) ||
      (status & 0x80U) != 0 || original_channel > 125U) {
    return false;
  }

  const uint8_t probe_channel = original_channel == 0x55U ? 0x2AU : 0x55U;
  uint8_t readback = 0;
  const bool probe_written = WriteRegister(Cmd::kRfChannel, probe_channel);
  const bool probe_read =
      probe_written && ReadRegister(Cmd::kRfChannel, &readback);
  // 即使回读失败也尝试还原 RF_CH，避免诊断流程改变后续工作信道。
  const bool channel_restored =
      WriteRegister(Cmd::kRfChannel, original_channel);
  return probe_written && probe_read && channel_restored &&
         readback == probe_channel;
}

bool Nrf24l01x::SetOperationMode(OperationMode mode) {
  if (mode != OperationMode::kPrimaryTransmitter &&
      mode != OperationMode::kPrimaryReceiver) {
    return false;
  }
  const uint8_t value =
      mode == OperationMode::kPrimaryReceiver ? kConfigPrimaryRxMask : 0;
  if (!UpdateRegisterBits(Cmd::kConfig, kConfigPrimaryRxMask, value)) {
    return false;
  }
  config_.operation_mode = mode;
  return true;
}

bool Nrf24l01x::SetPowerMode(PowerMode mode) {
  if (mode != PowerMode::kPowerDown && mode != PowerMode::kPowerUp) {
    return false;
  }
  if (mode == PowerMode::kPowerDown) {
    if (!SetCe(false)) {
      return false;
    }
    receiving_ = false;
  }
  const uint8_t value = mode == PowerMode::kPowerUp ? kConfigPowerUpMask : 0;
  if (!UpdateRegisterBits(Cmd::kConfig, kConfigPowerUpMask, value)) {
    return false;
  }

  const bool was_powered_up = powered_up_;
  powered_up_ = mode == PowerMode::kPowerUp;
  config_.power_mode = mode;
  if (powered_up_ && !was_powered_up) {
    DelayUs(kPowerDownToStandbyUs);
  }
  return true;
}

bool Nrf24l01x::SetCrcMode(CrcMode mode) {
  if (mode != CrcMode::kDisabled && mode != CrcMode::k8Bit &&
      mode != CrcMode::k16Bit) {
    return false;
  }
  uint8_t value = 0;
  if (mode != CrcMode::kDisabled) {
    value |= kConfigCrcEnableMask;
  }
  if (mode == CrcMode::k16Bit) {
    value |= kConfigCrcLengthMask;
  }
  if (!UpdateRegisterBits(
          Cmd::kConfig, kConfigCrcEnableMask | kConfigCrcLengthMask, value)) {
    return false;
  }
  config_.crc_mode = mode;
  return true;
}

bool Nrf24l01x::SetIrqMode(IrqSource source, bool enabled) {
  if (!IsValidIrqSource(source)) {
    return false;
  }
  const uint8_t mask = BitForIrq(source);
  if (!UpdateRegisterBits(Cmd::kConfig, mask, enabled ? 0 : mask)) {
    return false;
  }
  switch (source) {
    case IrqSource::kRxDataReady:
      config_.rx_data_ready_irq = enabled;
      break;
    case IrqSource::kTxDataSent:
      config_.tx_data_sent_irq = enabled;
      break;
    case IrqSource::kMaximumRetransmit:
      config_.maximum_retransmit_irq = enabled;
      break;
  }
  return true;
}

bool Nrf24l01x::GetIrqMode(IrqSource source, bool* enabled) {
  if (!IsValidIrqSource(source) || enabled == nullptr) {
    return false;
  }
  uint8_t config = 0;
  if (!ReadRegister(Cmd::kConfig, &config)) {
    return false;
  }
  *enabled = (config & BitForIrq(source)) == 0;
  return true;
}

bool Nrf24l01x::GetClearIrqFlags(uint8_t* flags) {
  if (flags == nullptr) {
    return false;
  }
  uint8_t status = 0;
  if (!WriteRegister(Cmd::kStatus, kAllIrqMask, &status)) {
    return false;
  }
  *flags = status & kAllIrqMask;
  return true;
}

bool Nrf24l01x::ClearIrqFlagsGetStatus(uint8_t* status) {
  if (status == nullptr) {
    return false;
  }
  uint8_t previous = 0;
  uint8_t current = 0;
  if (!WriteRegister(Cmd::kStatus, kAllIrqMask, &previous) ||
      !NoOperation(&current)) {
    return false;
  }
  *status =
      static_cast<uint8_t>((previous & kAllIrqMask) | (current & ~kAllIrqMask));
  return true;
}

bool Nrf24l01x::ClearIrqFlag(IrqSource source) {
  if (!IsValidIrqSource(source)) {
    return false;
  }
  return WriteRegister(Cmd::kStatus, BitForIrq(source));
}

bool Nrf24l01x::GetIrqFlags(uint8_t* flags) {
  if (flags == nullptr) {
    return false;
  }
  uint8_t status = 0;
  if (!NoOperation(&status)) {
    return false;
  }
  *flags = status & kAllIrqMask;
  return true;
}

bool Nrf24l01x::OpenPipe(Address pipe, bool auto_ack) {
  uint8_t enabled_pipes = 0;
  uint8_t auto_ack_pipes = 0;
  if (!ReadRegister(Cmd::kEnableRxAddress, &enabled_pipes) ||
      !ReadRegister(Cmd::kEnableAutoAcknowledgment, &auto_ack_pipes)) {
    return false;
  }

  if (pipe == Address::kAllPipes) {
    enabled_pipes = kAllPipeMask;
    auto_ack_pipes = auto_ack ? kAllPipeMask : 0;
  } else if (IsPipeAddress(pipe)) {
    const uint8_t mask = static_cast<uint8_t>(1U << static_cast<uint8_t>(pipe));
    enabled_pipes |= mask;
    if (auto_ack) {
      auto_ack_pipes |= mask;
    } else {
      auto_ack_pipes &= static_cast<uint8_t>(~mask);
    }
  } else {
    return false;
  }

  const bool result =
      WriteRegister(Cmd::kEnableRxAddress, enabled_pipes) &&
      WriteRegister(Cmd::kEnableAutoAcknowledgment, auto_ack_pipes);
  if (result) {
    config_.enabled_pipe_mask = enabled_pipes;
    config_.auto_ack_pipe_mask = auto_ack_pipes;
  }
  return result;
}

bool Nrf24l01x::ClosePipe(Address pipe) {
  uint8_t enabled_pipes = 0;
  uint8_t auto_ack_pipes = 0;
  if (!ReadRegister(Cmd::kEnableRxAddress, &enabled_pipes) ||
      !ReadRegister(Cmd::kEnableAutoAcknowledgment, &auto_ack_pipes)) {
    return false;
  }

  if (pipe == Address::kAllPipes) {
    enabled_pipes = 0;
    auto_ack_pipes = 0;
  } else if (IsPipeAddress(pipe)) {
    const uint8_t mask = static_cast<uint8_t>(1U << static_cast<uint8_t>(pipe));
    enabled_pipes &= static_cast<uint8_t>(~mask);
    auto_ack_pipes &= static_cast<uint8_t>(~mask);
  } else {
    return false;
  }

  const bool result =
      WriteRegister(Cmd::kEnableRxAddress, enabled_pipes) &&
      WriteRegister(Cmd::kEnableAutoAcknowledgment, auto_ack_pipes);
  if (result) {
    config_.enabled_pipe_mask = enabled_pipes;
    config_.auto_ack_pipe_mask = auto_ack_pipes;
  }
  return result;
}

bool Nrf24l01x::SetAddress(
    Address address, const uint8_t* data, std::size_t length) {
  if (data == nullptr ||
      (!IsPipeAddress(address) && address != Address::kTransmit)) {
    return false;
  }

  const bool full_address = address == Address::kPipe0 ||
                            address == Address::kPipe1 ||
                            address == Address::kTransmit;
  uint8_t address_width = 0;
  if (full_address && !GetAddressWidth(&address_width)) {
    return false;
  }
  const std::size_t required_length = full_address ? address_width : 1;
  if (length != required_length) {
    return false;
  }
  return WriteBuffer(CmdForAddress(address), data, required_length);
}

bool Nrf24l01x::GetAddress(
    Address address, uint8_t* data, std::size_t capacity, std::size_t* length) {
  if (data == nullptr || length == nullptr ||
      (!IsPipeAddress(address) && address != Address::kTransmit)) {
    return false;
  }

  const bool full_address = address == Address::kPipe0 ||
                            address == Address::kPipe1 ||
                            address == Address::kTransmit;
  uint8_t address_width = 0;
  if (full_address && !GetAddressWidth(&address_width)) {
    return false;
  }
  const std::size_t required_length = full_address ? address_width : 1;
  if (capacity < required_length) {
    return false;
  }
  if (!ReadBuffer(CmdForAddress(address), data, required_length)) {
    return false;
  }
  *length = required_length;
  return true;
}

bool Nrf24l01x::SetAutoRetransmit(uint8_t count, uint16_t delay_us) {
  if (count > 15U || delay_us < 250U || delay_us > 4000U ||
      delay_us % 250U != 0) {
    return false;
  }
  const uint8_t delay = static_cast<uint8_t>((delay_us / 250U) - 1U);
  const uint8_t value = static_cast<uint8_t>((delay << 4) | count);
  if (!WriteRegister(Cmd::kSetupRetransmission, value)) {
    return false;
  }
  config_.retransmit_count = count;
  config_.retransmit_delay_us = delay_us;
  return true;
}

bool Nrf24l01x::SetAddressWidth(AddressWidth width) {
  const uint8_t width_value = static_cast<uint8_t>(width);
  if (width_value < 3U || width_value > 5U ||
      !WriteRegister(
          Cmd::kSetupAddressWidth, static_cast<uint8_t>(width_value - 2U))) {
    return false;
  }
  config_.address_width = width;
  return true;
}

bool Nrf24l01x::GetAddressWidth(uint8_t* width) {
  if (width == nullptr) {
    return false;
  }
  uint8_t encoded_width = 0;
  if (!ReadRegister(Cmd::kSetupAddressWidth, &encoded_width) ||
      encoded_width < 1U || encoded_width > 3U) {
    return false;
  }
  *width = static_cast<uint8_t>(encoded_width + 2U);
  return true;
}

bool Nrf24l01x::SetRxPayloadWidth(uint8_t pipe, uint8_t width) {
  if (pipe > 5U || width > kMaximumPayloadLength ||
      !WriteRegister(CmdForPayloadWidth(pipe), width)) {
    return false;
  }
  config_.rx_payload_width[pipe] = width;
  return true;
}

bool Nrf24l01x::GetRxPayloadWidth(uint8_t pipe, uint8_t* width) {
  if (pipe > 5U || width == nullptr) {
    return false;
  }
  return ReadRegister(CmdForPayloadWidth(pipe), width);
}

bool Nrf24l01x::GetPipeStatus(uint8_t pipe, uint8_t* status) {
  if (pipe > 5U || status == nullptr) {
    return false;
  }
  uint8_t enabled_pipes = 0;
  uint8_t auto_ack_pipes = 0;
  if (!ReadRegister(Cmd::kEnableRxAddress, &enabled_pipes) ||
      !ReadRegister(Cmd::kEnableAutoAcknowledgment, &auto_ack_pipes)) {
    return false;
  }
  const uint8_t enabled = static_cast<uint8_t>((enabled_pipes >> pipe) & 0x01U);
  const uint8_t auto_ack =
      static_cast<uint8_t>((auto_ack_pipes >> pipe) & 0x01U);
  *status = static_cast<uint8_t>((auto_ack << 1) | enabled);
  return true;
}

bool Nrf24l01x::GetAutoRetransmitStatus(uint8_t* status) {
  return status != nullptr && ReadRegister(Cmd::kObserveTx, status);
}

bool Nrf24l01x::GetPacketLostCount(uint8_t* count) {
  if (count == nullptr) {
    return false;
  }
  uint8_t observe_tx = 0;
  if (!ReadRegister(Cmd::kObserveTx, &observe_tx)) {
    return false;
  }
  *count = static_cast<uint8_t>((observe_tx & kObservePacketLostMask) >> 4);
  return true;
}

bool Nrf24l01x::SetRfChannel(uint8_t channel) {
  if (channel > 125U || !WriteRegister(Cmd::kRfChannel, channel)) {
    return false;
  }
  config_.rf_channel = channel;
  return true;
}

bool Nrf24l01x::SetOutputPower(OutputPower power) {
  if (static_cast<uint8_t>(power) >
      static_cast<uint8_t>(OutputPower::kZeroDbm)) {
    return false;
  }
  const uint8_t value = static_cast<uint8_t>(static_cast<uint8_t>(power) << 1);
  if (!UpdateRegisterBits(Cmd::kRfSetup, kRfSetupPowerMask, value)) {
    return false;
  }
  config_.output_power = power;
  return true;
}

bool Nrf24l01x::SetDataRate(DataRate data_rate) {
  if (data_rate != DataRate::k1Mbps && data_rate != DataRate::k2Mbps &&
      data_rate != DataRate::k250Kbps) {
    return false;
  }
  uint8_t value = 0;
  if (data_rate == DataRate::k2Mbps) {
    value = kRfSetupDataRateHighMask;
  } else if (data_rate == DataRate::k250Kbps) {
    value = kRfSetupDataRateLowMask;
  }
  if (!UpdateRegisterBits(Cmd::kRfSetup,
          kRfSetupDataRateHighMask | kRfSetupDataRateLowMask, value)) {
    return false;
  }
  config_.data_rate = data_rate;
  return true;
}

bool Nrf24l01x::GetTxFifoStatus(uint8_t* status) {
  if (status == nullptr) {
    return false;
  }
  uint8_t fifo = 0;
  if (!GetFifoStatus(&fifo)) {
    return false;
  }
  *status =
      static_cast<uint8_t>((fifo & (kFifoTxFullMask | kFifoTxEmptyMask)) >> 4);
  return true;
}

bool Nrf24l01x::TxFifoEmpty(bool* empty) {
  if (empty == nullptr) {
    return false;
  }
  uint8_t fifo = 0;
  if (!GetFifoStatus(&fifo)) {
    return false;
  }
  *empty = (fifo & kFifoTxEmptyMask) != 0;
  return true;
}

bool Nrf24l01x::TxFifoFull(bool* full) {
  if (full == nullptr) {
    return false;
  }
  uint8_t fifo = 0;
  if (!GetFifoStatus(&fifo)) {
    return false;
  }
  *full = (fifo & kFifoTxFullMask) != 0;
  return true;
}

bool Nrf24l01x::GetRxFifoStatus(uint8_t* status) {
  if (status == nullptr) {
    return false;
  }
  uint8_t fifo = 0;
  if (!GetFifoStatus(&fifo)) {
    return false;
  }
  *status = fifo & (kFifoRxFullMask | kFifoRxEmptyMask);
  return true;
}

bool Nrf24l01x::GetFifoStatus(uint8_t* status) {
  return status != nullptr && ReadRegister(Cmd::kFifoStatus, status);
}

bool Nrf24l01x::RxFifoEmpty(bool* empty) {
  if (empty == nullptr) {
    return false;
  }
  uint8_t fifo = 0;
  if (!GetFifoStatus(&fifo)) {
    return false;
  }
  *empty = (fifo & kFifoRxEmptyMask) != 0;
  return true;
}

bool Nrf24l01x::RxFifoFull(bool* full) {
  if (full == nullptr) {
    return false;
  }
  uint8_t fifo = 0;
  if (!GetFifoStatus(&fifo)) {
    return false;
  }
  *full = (fifo & kFifoRxFullMask) != 0;
  return true;
}

bool Nrf24l01x::GetTransmitAttempts(uint8_t* count) {
  if (count == nullptr) {
    return false;
  }
  uint8_t observe_tx = 0;
  if (!ReadRegister(Cmd::kObserveTx, &observe_tx)) {
    return false;
  }
  *count = observe_tx & kObserveRetransmitMask;
  return true;
}

bool Nrf24l01x::GetCarrierDetect(bool* detected) {
  if (detected == nullptr) {
    return false;
  }
  uint8_t received_power = 0;
  if (!ReadRegister(Cmd::kReceivedPowerDetector, &received_power)) {
    return false;
  }
  *detected = (received_power & 0x01U) != 0;
  return true;
}

bool Nrf24l01x::ActivateFeatures() {
  const uint8_t activation = kFeatureActivationData;
  return WriteCommand(SpiCmd::kActivateFeatures, &activation, 1);
}

bool Nrf24l01x::SetupDynamicPayload(uint8_t pipe_mask) {
  if ((pipe_mask & ~kAllPipeMask) != 0 ||
      !WriteRegister(Cmd::kDynamicPayload, pipe_mask)) {
    return false;
  }
  config_.dynamic_payload_pipe_mask = pipe_mask;
  return true;
}

bool Nrf24l01x::EnableDynamicPayload(bool enabled) {
  if (!UpdateFeatureBits(kDynamicPayloadFeatureMask, enabled)) {
    return false;
  }
  config_.dynamic_payload_enabled = enabled;
  return true;
}

bool Nrf24l01x::EnableAckPayload(bool enabled) {
  if (!UpdateFeatureBits(kAckPayloadFeatureMask, enabled)) {
    return false;
  }
  config_.ack_payload_enabled = enabled;
  return true;
}

bool Nrf24l01x::EnableDynamicAck(bool enabled) {
  if (!UpdateFeatureBits(kDynamicAckFeatureMask, enabled)) {
    return false;
  }
  config_.dynamic_ack_enabled = enabled;
  return true;
}

bool Nrf24l01x::ReadRxPayloadWidth(uint8_t* width) {
  return width != nullptr && ReadCommand(SpiCmd::kReadRxPayloadWidth, width, 1);
}

bool Nrf24l01x::WriteTxPayload(const uint8_t* payload, std::size_t length) {
  return payload != nullptr && length > 0 && length <= kMaximumPayloadLength &&
         WriteCommand(SpiCmd::kWriteTxPayload, payload, length);
}

bool Nrf24l01x::WriteTxPayloadNoAck(
    const uint8_t* payload, std::size_t length) {
  return payload != nullptr && length > 0 && length <= kMaximumPayloadLength &&
         WriteCommand(SpiCmd::kWriteTxPayloadNoAck, payload, length);
}

bool Nrf24l01x::WriteAckPayload(
    uint8_t pipe, const uint8_t* payload, std::size_t length) {
  if (pipe > 5U || payload == nullptr || length == 0 ||
      length > kMaximumPayloadLength) {
    return false;
  }
  const uint8_t command = static_cast<uint8_t>(SpiCmd::kWriteAckPayload) | pipe;
  return Exchange(command, payload, nullptr, length, nullptr);
}

bool Nrf24l01x::ReadRxPayload(uint8_t* payload, std::size_t capacity,
    std::size_t* length, uint8_t* pipe) {
  if (payload == nullptr || length == nullptr) {
    return false;
  }

  uint8_t source = kInvalidRxPipe;
  uint8_t payload_width = 0;
  uint8_t feature = 0;
  uint8_t dynamic_payload = 0;
  if (!GetRxDataSource(&source) || source == kInvalidRxPipe ||
      !ReadRegister(Cmd::kFeature, &feature) ||
      !ReadRegister(Cmd::kDynamicPayload, &dynamic_payload)) {
    return false;
  }

  // R_RX_PL_WID 只用于启用 DPL 的管道；静态模式必须读取对应 RX_PW_Px。
  const bool uses_dynamic_width =
      (feature & kDynamicPayloadFeatureMask) != 0 &&
      (dynamic_payload & static_cast<uint8_t>(1U << source)) != 0;
  const bool width_read = uses_dynamic_width
                              ? ReadRxPayloadWidth(&payload_width)
                              : GetRxPayloadWidth(source, &payload_width);
  if (!width_read) {
    return false;
  }
  if (payload_width > kMaximumPayloadLength) {
    // 动态长度失步时规格书要求立即清空 RX FIFO；静态寄存器异常也采取同样保护。
    const bool flushed = FlushRx();
    const bool cleared = ClearIrqFlag(IrqSource::kRxDataReady);
    static_cast<void>(flushed);
    static_cast<void>(cleared);
    return false;
  }
  if (capacity < payload_width ||
      !ReadCommand(SpiCmd::kReadRxPayload, payload, payload_width)) {
    return false;
  }
  *length = payload_width;
  if (pipe != nullptr) {
    *pipe = source;
  }
  return true;
}

bool Nrf24l01x::GetRxDataSource(uint8_t* pipe) {
  if (pipe == nullptr) {
    return false;
  }
  uint8_t status = 0;
  if (!NoOperation(&status)) {
    return false;
  }
  *pipe = static_cast<uint8_t>((status & kStatusRxPipeMask) >> 1);
  return true;
}

bool Nrf24l01x::ReuseTx() { return ExecuteCommand(SpiCmd::kReuseTxPayload); }

bool Nrf24l01x::GetReuseTxStatus(bool* reused) {
  if (reused == nullptr) {
    return false;
  }
  uint8_t fifo = 0;
  if (!GetFifoStatus(&fifo)) {
    return false;
  }
  *reused = (fifo & kFifoTxReuseMask) != 0;
  return true;
}

bool Nrf24l01x::FlushRx() { return ExecuteCommand(SpiCmd::kFlushRx); }

bool Nrf24l01x::FlushTx() { return ExecuteCommand(SpiCmd::kFlushTx); }

bool Nrf24l01x::NoOperation(uint8_t* status) {
  return status != nullptr && ExecuteCommand(SpiCmd::kNoOperation, status);
}

bool Nrf24l01x::SetPllMode(bool locked) {
  return UpdateRegisterBits(
      Cmd::kRfSetup, kRfSetupPllLockMask, locked ? kRfSetupPllLockMask : 0);
}

bool Nrf24l01x::SetLnaGain(bool high_current) {
  if (high_current) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "nRF24L01+ does not support LNA high-current mode\n");
    return false;
  }
  return UpdateRegisterBits(Cmd::kRfSetup, kRfSetupLnaHighCurrentMask, 0);
}

bool Nrf24l01x::EnableContinuousWave(bool enabled) {
  return UpdateRegisterBits(Cmd::kRfSetup, kRfSetupContinuousWaveMask,
      enabled ? kRfSetupContinuousWaveMask : 0);
}

bool Nrf24l01x::ReadRegister(Cmd cmd, uint8_t* value, uint8_t* status) {
  if (value == nullptr) {
    return false;
  }
  const uint8_t command = static_cast<uint8_t>(cmd) & kRegisterMask;
  return Exchange(command, nullptr, value, 1, status);
}

bool Nrf24l01x::WriteRegister(Cmd cmd, uint8_t value, uint8_t* status) {
  const uint8_t command =
      kWriteRegisterCommand | (static_cast<uint8_t>(cmd) & kRegisterMask);
  return Exchange(command, &value, nullptr, 1, status);
}

bool Nrf24l01x::ReadBuffer(
    Cmd cmd, uint8_t* data, std::size_t length, uint8_t* status) {
  if (data == nullptr || length == 0 || length > kMaximumPayloadLength) {
    return false;
  }
  const uint8_t command = static_cast<uint8_t>(cmd) & kRegisterMask;
  return Exchange(command, nullptr, data, length, status);
}

bool Nrf24l01x::WriteBuffer(
    Cmd cmd, const uint8_t* data, std::size_t length, uint8_t* status) {
  if (data == nullptr || length == 0 || length > kMaximumPayloadLength) {
    return false;
  }
  const uint8_t command =
      kWriteRegisterCommand | (static_cast<uint8_t>(cmd) & kRegisterMask);
  return Exchange(command, data, nullptr, length, status);
}

bool Nrf24l01x::TransferByte(uint8_t value, uint8_t* response) {
  return response != nullptr && Exchange(value, nullptr, nullptr, 0, response);
}

bool Nrf24l01x::Standby() {
  return SetCe(false) && SetPowerMode(PowerMode::kPowerUp);
}

bool Nrf24l01x::PowerDown() {
  return SetCe(false) && SetPowerMode(PowerMode::kPowerDown);
}

bool Nrf24l01x::StartReceive() {
  if (!SetCe(false) || !SetOperationMode(OperationMode::kPrimaryReceiver) ||
      !SetPowerMode(PowerMode::kPowerUp)) {
    return false;
  }
  if (!SetCe(true)) {
    return false;
  }
  // CE 上升沿之后芯片才开始由 Standby-I 转入 RX Settling。
  DelayUs(kStandbyToActiveUs);
  receiving_ = true;
  return true;
}

bool Nrf24l01x::StopReceive() {
  const bool was_receiving = receiving_;
  if (!SetCe(false)) {
    return false;
  }
  receiving_ = false;
  if (was_receiving) {
    DelayUs(kStandbyToActiveUs);
  }
  return true;
}

bool Nrf24l01x::PulseCe(uint32_t high_time_us) {
  if (high_time_us < kMinimumCePulseUs) {
    return false;
  }
  if (!SetCe(true)) {
    return false;
  }
  DelayUs(high_time_us);
  const bool result = SetCe(false);
  receiving_ = false;
  return result;
}

bool Nrf24l01x::IrqActive(bool* active) {
  if (active == nullptr || irq_ == kDefaultValue) {
    return false;
  }
  *active = !GpioRead(irq_);
  return true;
}

bool Nrf24l01x::ReadStatus(Status* status) {
  if (status == nullptr) {
    return false;
  }
  uint8_t raw = 0;
  if (!NoOperation(&raw)) {
    return false;
  }
  DecodeStatus(raw, status);
  return true;
}

Nrf24l01x::TransmitResult Nrf24l01x::Transmit(const uint8_t* payload,
    std::size_t length, bool no_ack, uint32_t timeout_ms) {
  if (!initialized_ || payload == nullptr || length == 0 ||
      length > kMaximumPayloadLength || timeout_ms == 0 ||
      (no_ack && !config_.dynamic_ack_enabled)) {
    return TransmitResult::kInvalidArgument;
  }
  if (!StopReceive() || !SetOperationMode(OperationMode::kPrimaryTransmitter) ||
      !Standby() || !FlushTx()) {
    return TransmitResult::kBusError;
  }

  uint8_t ignored_flags = 0;
  if (!GetClearIrqFlags(&ignored_flags)) {
    return TransmitResult::kBusError;
  }
  const bool payload_written = no_ack ? WriteTxPayloadNoAck(payload, length)
                                      : WriteTxPayload(payload, length);
  if (!payload_written) {
    return TransmitResult::kBusError;
  }
  if (!PulseCe()) {
    // CE 或 GPIO 事务异常时尽量恢复 Standby-I，并丢弃状态未知的待发负载。
    const bool ce_recovered = SetCe(false);
    const bool fifo_recovered = FlushTx();
    static_cast<void>(ce_recovered);
    static_cast<void>(fifo_recovered);
    return TransmitResult::kBusError;
  }

  const int64_t start_time_ms = GetSystemTimeMs();
  while (GetSystemTimeMs() - start_time_ms < static_cast<int64_t>(timeout_ms)) {
    uint8_t status = 0;
    if (!NoOperation(&status)) {
      return TransmitResult::kBusError;
    }
    if ((status & BitForIrq(IrqSource::kTxDataSent)) != 0) {
      if (!ClearIrqFlag(IrqSource::kTxDataSent)) {
        return TransmitResult::kBusError;
      }
      return TransmitResult::kSuccess;
    }
    if ((status & BitForIrq(IrqSource::kMaximumRetransmit)) != 0) {
      // 两个清理动作互不短路，尽量避免一个失败导致另一个也没有执行。
      const bool irq_cleared = ClearIrqFlag(IrqSource::kMaximumRetransmit);
      const bool fifo_flushed = FlushTx();
      return irq_cleared && fifo_flushed ? TransmitResult::kMaximumRetransmit
                                         : TransmitResult::kBusError;
    }
    DelayUs(50);
  }

  return FlushTx() ? TransmitResult::kTimeout : TransmitResult::kBusError;
}

bool Nrf24l01x::Receive(uint8_t* payload, std::size_t capacity,
    std::size_t* length, uint8_t* pipe, uint32_t timeout_ms,
    bool keep_listening) {
  if (!initialized_ || payload == nullptr || length == nullptr ||
      capacity == 0 || timeout_ms == 0) {
    return false;
  }
  if (!receiving_ && !StartReceive()) {
    return false;
  }

  const int64_t start_time_ms = GetSystemTimeMs();
  bool empty = true;
  while (GetSystemTimeMs() - start_time_ms < static_cast<int64_t>(timeout_ms)) {
    if (!RxFifoEmpty(&empty)) {
      return false;
    }
    if (!empty) {
      break;
    }
    DelayUs(50);
  }
  if (empty) {
    if (!keep_listening) {
      StopReceive();
    }
    return false;
  }

  const bool read_result = ReadRxPayload(payload, capacity, length, pipe);
  bool result = read_result;
  if (read_result) {
    bool fifo_empty = false;
    result &= RxFifoEmpty(&fifo_empty);
    if (fifo_empty) {
      result &= ClearIrqFlag(IrqSource::kRxDataReady);
    }
  }
  if (!keep_listening) {
    result &= StopReceive();
  }
  return result;
}

bool Nrf24l01x::Exchange(uint8_t command, const uint8_t* write_data,
    uint8_t* read_data, std::size_t length, uint8_t* status) {
  if (!bus_initialized_ || bus_ == nullptr || length > kMaximumPayloadLength) {
    return false;
  }

  std::array<uint8_t, kMaximumPayloadLength + 1> tx{};
  std::array<uint8_t, kMaximumPayloadLength + 1> rx{};
  tx.fill(static_cast<uint8_t>(SpiCmd::kNoOperation));
  tx[0] = command;
  if (write_data != nullptr && length > 0) {
    std::copy_n(write_data, length, tx.begin() + 1);
  }

  if (!GpioWrite(cs_, false)) {
    return false;
  }
  const bool transferred = bus_->WriteRead(tx.data(), rx.data(), length + 1);
  const bool released = GpioWrite(cs_, true);
  if (!transferred || !released) {
    return false;
  }

  if (status != nullptr) {
    *status = rx[0];
  }
  if (read_data != nullptr && length > 0) {
    std::copy_n(rx.begin() + 1, length, read_data);
  }
  return true;
}

bool Nrf24l01x::ExecuteCommand(SpiCmd command, uint8_t* status) {
  return Exchange(static_cast<uint8_t>(command), nullptr, nullptr, 0, status);
}

bool Nrf24l01x::WriteCommand(
    SpiCmd command, const uint8_t* data, std::size_t length, uint8_t* status) {
  return data != nullptr && length > 0 &&
         Exchange(static_cast<uint8_t>(command), data, nullptr, length, status);
}

bool Nrf24l01x::ReadCommand(
    SpiCmd command, uint8_t* data, std::size_t length, uint8_t* status) {
  if (length == 0) {
    return ExecuteCommand(command, status);
  }
  return data != nullptr &&
         Exchange(static_cast<uint8_t>(command), nullptr, data, length, status);
}

bool Nrf24l01x::UpdateRegisterBits(Cmd cmd, uint8_t mask, uint8_t value) {
  uint8_t current = 0;
  if (!ReadRegister(cmd, &current)) {
    return false;
  }
  const uint8_t next = static_cast<uint8_t>(
      (current & static_cast<uint8_t>(~mask)) | (value & mask));
  return next == current || WriteRegister(cmd, next);
}

bool Nrf24l01x::UpdateFeatureBits(uint8_t mask, bool enabled) {
  uint8_t feature = 0;
  if (!ReadRegister(Cmd::kFeature, &feature)) {
    return false;
  }
  const uint8_t next = enabled ? static_cast<uint8_t>(feature | mask)
                               : static_cast<uint8_t>(feature & ~mask);
  if (next == feature) {
    return true;
  }

  uint8_t readback = 0;
  bool result = WriteRegister(Cmd::kFeature, next) &&
                ReadRegister(Cmd::kFeature, &readback);
  if (!result) {
    return false;
  }
  if (readback == next) {
    return true;
  }

  result = ActivateFeatures();
  result &= WriteRegister(Cmd::kFeature, next);
  result &= ReadRegister(Cmd::kFeature, &readback);
  return result && readback == next;
}

bool Nrf24l01x::ValidateConfig(const Config& config) const {
  if ((config.operation_mode != OperationMode::kPrimaryTransmitter &&
          config.operation_mode != OperationMode::kPrimaryReceiver) ||
      (config.power_mode != PowerMode::kPowerDown &&
          config.power_mode != PowerMode::kPowerUp) ||
      (config.crc_mode != CrcMode::kDisabled &&
          config.crc_mode != CrcMode::k8Bit &&
          config.crc_mode != CrcMode::k16Bit) ||
      static_cast<uint8_t>(config.output_power) >
          static_cast<uint8_t>(OutputPower::kZeroDbm) ||
      (config.data_rate != DataRate::k1Mbps &&
          config.data_rate != DataRate::k2Mbps &&
          config.data_rate != DataRate::k250Kbps) ||
      config.rf_channel > 125U || config.retransmit_count > 15U ||
      config.retransmit_delay_us < 250U || config.retransmit_delay_us > 4000U ||
      config.retransmit_delay_us % 250U != 0 ||
      (config.enabled_pipe_mask & ~kAllPipeMask) != 0 ||
      (config.auto_ack_pipe_mask & ~kAllPipeMask) != 0 ||
      (config.dynamic_payload_pipe_mask & ~kAllPipeMask) != 0) {
    return false;
  }
  const uint8_t width = static_cast<uint8_t>(config.address_width);
  if (width < 3U || width > 5U) {
    return false;
  }
  for (const uint8_t payload_width : config.rx_payload_width) {
    if (payload_width > kMaximumPayloadLength) {
      return false;
    }
  }
  if (config.dynamic_payload_pipe_mask != 0 &&
      !config.dynamic_payload_enabled) {
    return false;
  }
  if ((config.dynamic_payload_pipe_mask & ~config.auto_ack_pipe_mask) != 0) {
    return false;
  }
  if (config.ack_payload_enabled && !config.dynamic_payload_enabled) {
    return false;
  }
  if (config.ack_payload_enabled &&
      (config.dynamic_payload_pipe_mask & 0x01U) == 0) {
    return false;
  }
  if (config.data_rate == DataRate::k250Kbps &&
      config.auto_ack_pipe_mask != 0 && config.retransmit_count != 0 &&
      config.retransmit_delay_us < 500U) {
    return false;
  }
  return true;
}

bool Nrf24l01x::SetCe(bool enabled) {
  if (ce_ == kDefaultValue) {
    return false;
  }
  return GpioWrite(ce_, enabled);
}

bool Nrf24l01x::FailInitialization(const char* reason) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "%s\n", reason);
  if (ce_ != kDefaultValue) {
    GpioWrite(ce_, false);
  }
  if (cs_ != kDefaultValue) {
    GpioWrite(cs_, true);
  }
  if (bus_initialized_ && bus_ != nullptr) {
    bus_->Deinit(false);
  }
  if (cs_ != kDefaultValue) {
    ResetGpio(cs_);
  }
  if (ce_ != kDefaultValue) {
    ResetGpio(ce_);
  }
  if (irq_ != kDefaultValue) {
    ResetGpio(irq_);
  }
  bus_initialized_ = false;
  initialized_ = false;
  powered_up_ = false;
  receiving_ = false;
  spi_frequency_hz_ = kDefaultValue;
  return false;
}

void Nrf24l01x::DecodeStatus(uint8_t raw, Status* status) {
  status->rx_data_ready = (raw & BitForIrq(IrqSource::kRxDataReady)) != 0;
  status->tx_data_sent = (raw & BitForIrq(IrqSource::kTxDataSent)) != 0;
  status->maximum_retransmit =
      (raw & BitForIrq(IrqSource::kMaximumRetransmit)) != 0;
  status->rx_pipe = static_cast<uint8_t>((raw & kStatusRxPipeMask) >> 1);
  status->tx_fifo_full = (raw & kStatusTxFullMask) != 0;
  status->raw = raw;
}

}  // namespace cpp_bus_driver
