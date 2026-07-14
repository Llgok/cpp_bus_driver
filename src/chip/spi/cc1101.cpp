/*
 * @Description: TI CC1101 亚 GHz 无线收发芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-07-12
 * @LastEditTime: 2026-07-12
 * @License: GPL 3.0
 */
#include "cc1101.h"

namespace cpp_bus_driver {
namespace {
constexpr uint8_t kGdoHighImpedance = 0x2E;
constexpr uint8_t kGdoSyncWord = 0x06;
constexpr uint8_t kGdoRxFifoThresholdOrPacketEnd = 0x01;
constexpr uint8_t kRxFifoThresholdMaximum = 0x0F;
constexpr uint8_t kAppendStatusMask = 0x04;
constexpr uint8_t kCrcAutoflushMask = 0x08;
constexpr uint8_t kCrcMask = 0x04;
constexpr uint8_t kFecMask = 0x80;
constexpr uint8_t kManchesterMask = 0x08;
constexpr uint8_t kWhiteningMask = 0x40;
constexpr uint8_t kModulationMask = 0x70;
constexpr uint8_t kPacketLengthMask = 0x03;
constexpr uint8_t kAddressCheckMask = 0x03;
constexpr uint8_t kSyncModeMask = 0x07;
constexpr uint8_t kPreambleMask = 0x70;
constexpr uint8_t kPreambleQualityMask = 0xE0;
constexpr uint8_t kChannelSpacingExponentMask = 0x03;
constexpr uint8_t kBitRateToleranceMask = 0x03;
constexpr uint8_t kCarrierSenseAbsoluteMask = 0x0F;
constexpr uint8_t kCarrierSenseRelativeMask = 0x30;
constexpr uint8_t kCcaModeMask = 0x30;
constexpr uint8_t kMarcStateMask = 0x1F;
constexpr uint8_t kDefaultMcsm0 = 0x18;
constexpr uint8_t kDefaultMcsm1 = 0x30;
constexpr uint32_t kCcaRssiSettlingUs = 1000;
constexpr uint8_t kPartNumberCc1101 = 0x00;
constexpr uint8_t kOfficialVersions[] = {0x04, 0x14};
constexpr uint8_t kCompatibleCloneVersion = 0x17;

/**
 * @brief 检查 VERSION 是否属于 TI 公开版本。
 * @param version VERSION 状态寄存器值。
 * @return 属于 TI 版本返回 true，否则返回 false。
 */
bool IsOfficialVersion(uint8_t version) {
  for (const uint8_t known : kOfficialVersions) {
    if (version == known) {
      return true;
    }
  }
  return false;
}
}  // namespace

bool Cc1101::Init(int32_t freq_hz) {
  if (bus_ == nullptr || cs_ == kDefaultValue ||
      miso_ == kDefaultValue) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid argument\n");
    return false;
  }

  bool result = true;
  result &= SetGpioMode(cs_, GpioMode::kOutput);
  result &= SetGpioMode(miso_, GpioMode::kInput);
  if (gdo0_ != kDefaultValue) {
    result &= SetGpioMode(gdo0_, GpioMode::kInput);
  }
  if (gdo2_ != kDefaultValue) {
    result &= SetGpioMode(gdo2_, GpioMode::kInput);
  }
  result &= GpioWrite(cs_, true);
  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Gpio initialization failed\n");
    return false;
  }

  if (freq_hz == kDefaultValue) {
    freq_hz = 10000000;
  }
  if (freq_hz <= 0 || freq_hz > 10000000) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid SPI frequency (frequency: %d)\n", freq_hz);
    return false;
  }

  const int32_t bus_cs = kDefaultValue;
  if (!bus_->Init(freq_hz, bus_cs)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SPI initialization failed\n");
    return false;
  }
  spi_frequency_hz_ = freq_hz;

  if (!Reset()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Reset failed\n");
    bus_->Deinit(false);
    return false;
  }

  uint8_t part_number = 0;
  uint8_t version = 0;
  if (!GetPartNumber(&part_number) || !GetVersion(&version) ||
      part_number != kPartNumberCc1101 ||
      (!IsOfficialVersion(version) &&
          version != kCompatibleCloneVersion)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "CC1101 not found (part: %#X version: %#X)\n",
        part_number, version);
    bus_->Deinit(false);
    return false;
  }
  if (version == kCompatibleCloneVersion) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Compatible CC1101 clone detected (version: %#X)\n", version);
  }

  if (!Configure(config_)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Configure failed\n");
    bus_->Deinit(false);
    return false;
  }

  initialized_ = true;
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "CC1101 initialization success (version: %#X)\n", version);
  return true;
}

bool Cc1101::Deinit(bool delete_bus) {
  bool result = true;
  if (initialized_) {
    result &= Standby();
    result &= Sleep();
  }
  result &= bus_ != nullptr && bus_->Deinit(delete_bus);
  if (cs_ != kDefaultValue) {
    result &= ResetGpio(cs_);
  }
  if (gdo0_ != kDefaultValue) {
    result &= ResetGpio(gdo0_);
  }
  if (gdo2_ != kDefaultValue) {
    result &= ResetGpio(gdo2_);
  }
  initialized_ = false;
  return result;
}

bool Cc1101::Reset() {
  if (bus_ == nullptr) {
    return false;
  }

  // CC1101 数据手册 10.3.1：通过 CSn 时序唤醒并同步 SPI 接口。
  bool result = GpioWrite(cs_, true);
  DelayUs(5);
  result &= GpioWrite(cs_, false);
  DelayUs(10);
  result &= GpioWrite(cs_, true);
  DelayUs(50);
  result &= GpioWrite(cs_, false);
  if (!result || !WaitForReady()) {
    GpioWrite(cs_, true);
    return false;
  }

  const uint8_t command = static_cast<uint8_t>(Command::kReset);
  uint8_t status = 0;
  result = bus_->WriteRead(&command, &status, 1);
  if (result) {
    result = WaitForReady();
  }
  result &= GpioWrite(cs_, true);
  DelayUs(100);
  sleeping_ = false;
  return result;
}

bool Cc1101::Configure(const Config& config) {
  if (!ValidateConfig(config)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid configuration\n");
    return false;
  }

  const Config previous_config = config_;
  // 配置过程需要使用目标晶振、频段和调制信息。
  config_ = config;
  bool result = Standby();
  result &= WriteRegister(Register::kMcsm0, kDefaultMcsm0);
  result &= WriteRegister(Register::kMcsm1, kDefaultMcsm1);
  result &= WriteRegister(Register::kIocfg0, kGdoHighImpedance);
  result &= WriteRegister(Register::kIocfg2, kGdoHighImpedance);
  result &= SetFrequency(config.frequency_mhz);
  result &= SetDataRate(config.data_rate_kbaud);
  result &= SetReceiveBandwidth(config.receive_bandwidth_khz);
  if (config.modulation == Modulation::kMsk) {
    result &= SetMskPhaseChangePeriod(
        config.msk_phase_change_period);
  } else if (config.modulation != Modulation::kAskOok) {
    result &= SetFrequencyDeviation(config.frequency_deviation_khz);
  }
  result &= SetChannelSpacing(config.channel_spacing_khz);
  result &= SetBitRateTolerance(config.bit_rate_tolerance);
  result &= SetChannel(config.channel);
  result &= SetCarrierSenseThreshold(
      config.carrier_sense_threshold,
      config.carrier_sense_relative);
  result &= SetCcaMode(config.cca_mode);
  result &= SetModulation(config.modulation);
  result &= SetEncoding(config.encoding);
  result &= SetSyncWord(config.sync_word_high, config.sync_word_low,
      config.sync_mode);
  result &= SetPreambleLength(config.preamble_length_bits);
  result &= SetPreambleQualityThreshold(
      config.preamble_quality_threshold);
  result &= SetPacketLengthMode(config.packet_length_mode,
      config.maximum_packet_length);
  result &= SetAddressCheck(config.address_check,
      config.device_address);
  result &= SetCrc(config.crc_enabled);
  result &= UpdateRegisterBits(Register::kPktctrl1,
      kAppendStatusMask,
      config.append_status ? kAppendStatusMask : 0);
  result &= SetCrcAutoflush(config.crc_autoflush);
  result &= SetFec(config.fec_enabled);
  result &= SetOutputPower(config.output_power_dbm);
  result &= FlushRx();
  result &= FlushTx();
  if (!result) {
    config_ = previous_config;
    return false;
  }
  config_ = config;
  return true;
}

bool Cc1101::ApplyRegisterSettings(
    const RegisterSetting* settings, size_t count,
    const Config& config) {
  if (settings == nullptr || count == 0 || !ValidateConfig(config)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid argument\n");
    return false;
  }

  if (!Standby()) {
    return false;
  }
  for (size_t index = 0; index < count; ++index) {
    if (!WriteRegister(settings[index].address,
            settings[index].value)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "Register setting failed (index: %zu)\n", index);
      return false;
    }
  }
  config_ = config;
  return true;
}

bool Cc1101::WriteRegister(Register address, uint8_t value) {
  if (GetMaximumBurstLength(address) == 0) {
    return false;
  }
  const uint8_t buffer[] = {
      static_cast<uint8_t>(address),
      value,
  };
  uint8_t status[sizeof(buffer)] = {0};
  if (!Transfer(buffer, status, sizeof(buffer))) {
    return false;
  }
  if (address == Register::kTest0) {
    test0_value_ = value;
  } else if (address == Register::kFscal2) {
    fscal2_value_ = value;
  } else if (address == Register::kPatable) {
    pa_table_cache_[0] = value;
    pa_table_length_ = 1;
  }
  return true;
}

bool Cc1101::ReadRegister(Register address, uint8_t* value) {
  if (value == nullptr || GetMaximumBurstLength(address) == 0) {
    return false;
  }
  const uint8_t buffer[] = {
      static_cast<uint8_t>(static_cast<uint8_t>(address) | kReadSingle),
      0,
  };
  uint8_t response[sizeof(buffer)] = {0};
  if (!Transfer(buffer, response, sizeof(buffer))) {
    return false;
  }
  *value = response[1];
  return true;
}

bool Cc1101::WriteBurst(
    Register address, const uint8_t* data, size_t length) {
  const size_t maximum_length = GetMaximumBurstLength(address);
  if (data == nullptr || length == 0 || length > maximum_length) {
    return false;
  }
  std::vector<uint8_t> buffer(length + 1, 0);
  std::vector<uint8_t> response(length + 1, 0);
  buffer[0] = static_cast<uint8_t>(address) | kBurst;
  std::memcpy(&buffer[1], data, length);
  if (!Transfer(buffer.data(), response.data(), buffer.size())) {
    return false;
  }
  if (address == Register::kPatable) {
    pa_table_length_ = std::min<size_t>(8, length);
    std::memcpy(pa_table_cache_, data, pa_table_length_);
  }
  return true;
}

bool Cc1101::ReadBurst(
    Register address, uint8_t* data, size_t length) {
  const size_t maximum_length = GetMaximumBurstLength(address);
  if (data == nullptr || length == 0 || length > maximum_length) {
    return false;
  }
  std::vector<uint8_t> buffer(length + 1, 0);
  std::vector<uint8_t> response(length + 1, 0);
  buffer[0] = static_cast<uint8_t>(address) | kReadBurst;
  if (!Transfer(buffer.data(), response.data(), buffer.size())) {
    return false;
  }
  std::memcpy(data, &response[1], length);
  return true;
}

bool Cc1101::ReadStatusRegister(Register address, uint8_t* value) {
  const uint8_t raw_address = static_cast<uint8_t>(address);
  if (value == nullptr || raw_address < 0x30 || raw_address > 0x3D) {
    return false;
  }
  const uint8_t buffer[] = {
      static_cast<uint8_t>(static_cast<uint8_t>(address) | kReadBurst),
      0,
  };
  uint8_t response[sizeof(buffer)] = {0};
  if (!Transfer(buffer, response, sizeof(buffer))) {
    return false;
  }
  *value = response[1];
  return true;
}

bool Cc1101::Strobe(Command command, ChipStatus* status) {
  const uint8_t value = static_cast<uint8_t>(command);
  uint8_t response = 0;
  if (!Transfer(&value, &response, 1)) {
    return false;
  }
  if (status != nullptr) {
    *status = ParseChipStatus(response);
  }
  return true;
}

bool Cc1101::SetFrequency(double frequency_mhz) {
  if (!ValidateFrequency(
          frequency_mhz, config_.crystal_frequency_mhz)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid frequency (frequency: %.03f MHz)\n", frequency_mhz);
    return false;
  }
  if (!Standby()) {
    return false;
  }

  const double word_value = frequency_mhz * 65536.0 /
      config_.crystal_frequency_mhz;
  const uint32_t word = static_cast<uint32_t>(
      std::lround(word_value));
  const uint8_t values[] = {
      static_cast<uint8_t>(word >> 16),
      static_cast<uint8_t>(word >> 8),
      static_cast<uint8_t>(word),
  };
  if (!WriteBurst(Register::kFreq2, values, sizeof(values))) {
    return false;
  }
  config_.frequency_mhz = frequency_mhz;
  return SetOutputPower(config_.output_power_dbm);
}

bool Cc1101::SetDataRate(double data_rate_kbaud) {
  const double crystal_hz = config_.crystal_frequency_mhz * 1000000.0;
  if (!ValidateDataRate(data_rate_kbaud, config_.modulation)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid data rate (rate: %.03f kBaud)\n", data_rate_kbaud);
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }

  const double target_hz = data_rate_kbaud * 1000.0;
  // 穷举 DRATE_E/DRATE_M，选择与目标速率误差最小的组合。
  double best_error = 1.0e30;
  uint8_t best_exponent = 0;
  uint8_t best_mantissa = 0;
  for (uint8_t exponent = 0; exponent <= 15; ++exponent) {
    for (uint16_t mantissa = 0; mantissa <= 255; ++mantissa) {
      const double actual = crystal_hz *
          static_cast<double>(256 + mantissa) *
          static_cast<double>(1UL << exponent) /
          static_cast<double>(1UL << 28);
      const double error = std::fabs(actual - target_hz);
      if (error < best_error) {
        best_error = error;
        best_exponent = exponent;
        best_mantissa = static_cast<uint8_t>(mantissa);
      }
    }
  }

  bool result = UpdateRegisterBits(Register::kMdmcfg4, 0x0F,
      best_exponent);
  result &= WriteRegister(Register::kMdmcfg3, best_mantissa);
  if (result) {
    config_.data_rate_kbaud = data_rate_kbaud;
  }
  return result;
}

bool Cc1101::SetFrequencyDeviation(double deviation_khz) {
  const double crystal_hz = config_.crystal_frequency_mhz * 1000000.0;
  const double minimum_khz = crystal_hz * 8.0 /
      static_cast<double>(1UL << 17) / 1000.0;
  const double maximum_khz = crystal_hz * 15.0 * 128.0 /
      static_cast<double>(1UL << 17) / 1000.0;
  if (!std::isfinite(deviation_khz) ||
      config_.modulation == Modulation::kAskOok ||
      config_.modulation == Modulation::kMsk ||
      deviation_khz < minimum_khz || deviation_khz > maximum_khz) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid deviation (deviation: %.03f kHz)\n", deviation_khz);
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }

  const double target_hz = deviation_khz * 1000.0;
  // 穷举 DEVIATION_E/DEVIATION_M，避免固定配置依赖 26 MHz 晶振。
  double best_error = 1.0e30;
  uint8_t best_exponent = 0;
  uint8_t best_mantissa = 0;
  for (uint8_t exponent = 0; exponent <= 7; ++exponent) {
    for (uint8_t mantissa = 0; mantissa <= 7; ++mantissa) {
      const double actual = crystal_hz *
          static_cast<double>(8 + mantissa) *
          static_cast<double>(1UL << exponent) /
          static_cast<double>(1UL << 17);
      const double error = std::fabs(actual - target_hz);
      if (error < best_error) {
        best_error = error;
        best_exponent = exponent;
        best_mantissa = mantissa;
      }
    }
  }

  const uint8_t value = static_cast<uint8_t>(
      (best_exponent << 4) | best_mantissa);
  if (!WriteRegister(Register::kDeviatn, value)) {
    return false;
  }
  config_.frequency_deviation_khz = deviation_khz;
  return true;
}

bool Cc1101::SetMskPhaseChangePeriod(uint8_t period) {
  if (period > 7) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  if (!UpdateRegisterBits(Register::kDeviatn, 0x07, period)) {
    return false;
  }
  config_.msk_phase_change_period = period;
  return true;
}

bool Cc1101::SetReceiveBandwidth(double bandwidth_khz) {
  const double crystal_hz = config_.crystal_frequency_mhz * 1000000.0;
  const double minimum_khz = crystal_hz / 448.0 / 1000.0;
  const double maximum_khz = crystal_hz / 32.0 / 1000.0;
  const double minimum_allowed = std::floor(minimum_khz * 10.0) / 10.0;
  const double maximum_allowed = std::ceil(maximum_khz * 10.0) / 10.0;
  if (!std::isfinite(bandwidth_khz) ||
      bandwidth_khz < minimum_allowed ||
      bandwidth_khz > maximum_allowed) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid bandwidth (bandwidth: %.03f kHz)\n", bandwidth_khz);
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }

  const double target_hz = bandwidth_khz * 1000.0;
  // 从 16 组 CHANBW_E/CHANBW_M 中选择最接近的硬件带宽。
  double best_error = 1.0e30;
  uint8_t best_exponent = 0;
  uint8_t best_mantissa = 0;
  for (uint8_t exponent = 0; exponent <= 3; ++exponent) {
    for (uint8_t mantissa = 0; mantissa <= 3; ++mantissa) {
      const double actual = crystal_hz /
          (8.0 * static_cast<double>(4 + mantissa) *
              static_cast<double>(1UL << exponent));
      const double error = std::fabs(actual - target_hz);
      if (error < best_error) {
        best_error = error;
        best_exponent = exponent;
        best_mantissa = mantissa;
      }
    }
  }

  const uint8_t value = static_cast<uint8_t>(
      (best_exponent << 6) | (best_mantissa << 4));
  if (!UpdateRegisterBits(Register::kMdmcfg4, 0xF0, value)) {
    return false;
  }
  config_.receive_bandwidth_khz = bandwidth_khz;
  return true;
}

bool Cc1101::SetChannelSpacing(double spacing_khz) {
  const double crystal_hz = config_.crystal_frequency_mhz * 1000000.0;
  const double minimum_hz = crystal_hz * 256.0 /
      static_cast<double>(1UL << 18);
  const double maximum_hz = crystal_hz * 511.0 * 8.0 /
      static_cast<double>(1UL << 18);
  const double target_hz = spacing_khz * 1000.0;
  if (!std::isfinite(spacing_khz) ||
      target_hz < minimum_hz || target_hz > maximum_hz) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }

  double best_error = 1.0e30;
  uint8_t best_exponent = 0;
  uint8_t best_mantissa = 0;
  for (uint8_t exponent = 0; exponent <= 3; ++exponent) {
    for (uint16_t mantissa = 0; mantissa <= 255; ++mantissa) {
      const double actual = crystal_hz *
          static_cast<double>(256 + mantissa) *
          static_cast<double>(1UL << exponent) /
          static_cast<double>(1UL << 18);
      const double error = std::fabs(actual - target_hz);
      if (error < best_error) {
        best_error = error;
        best_exponent = exponent;
        best_mantissa = static_cast<uint8_t>(mantissa);
      }
    }
  }

  bool result = UpdateRegisterBits(Register::kMdmcfg1,
      kChannelSpacingExponentMask, best_exponent);
  result &= WriteRegister(Register::kMdmcfg0, best_mantissa);
  if (result) {
    config_.channel_spacing_khz = spacing_khz;
  }
  return result;
}

bool Cc1101::SetBitRateTolerance(uint8_t tolerance) {
  if (tolerance > kBitRateToleranceMask) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  if (!UpdateRegisterBits(Register::kBscfg,
          kBitRateToleranceMask, tolerance)) {
    return false;
  }
  config_.bit_rate_tolerance = tolerance;
  return true;
}

bool Cc1101::SetOutputPower(int8_t power_dbm) {
  uint8_t value = 0;
  if (!SelectPaValue(power_dbm, &value)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Unsupported output power (power: %d dBm)\n", power_dbm);
    return false;
  }
  if (!SetOutputPowerRaw(value)) {
    return false;
  }
  config_.output_power_dbm = power_dbm;
  return true;
}

bool Cc1101::SetOutputPowerRaw(uint8_t pa_value) {
  if (pa_value >= 0x61 && pa_value <= 0x6F) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Forbidden PATABLE value (value: %#X)\n", pa_value);
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }

  if (config_.modulation == Modulation::kAskOok) {
    const uint8_t values[] = {0, pa_value};
    if (!WriteBurst(Register::kPatable, values, sizeof(values))) {
      return false;
    }
    return UpdateRegisterBits(Register::kFrend0, 0x07, 0x01);
  }

  if (!WriteRegister(Register::kPatable, pa_value)) {
    return false;
  }
  return UpdateRegisterBits(Register::kFrend0, 0x07, 0);
}

bool Cc1101::SetModulation(Modulation modulation) {
  const bool supported = modulation == Modulation::k2Fsk ||
      modulation == Modulation::kGfsk ||
      modulation == Modulation::kAskOok ||
      modulation == Modulation::k4Fsk ||
      modulation == Modulation::kMsk;
  if (!supported) {
    return false;
  }
  if (!ValidateDataRate(config_.data_rate_kbaud, modulation)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Data rate is invalid for modulation\n");
    return false;
  }
  if (config_.encoding == Encoding::kManchester &&
      (modulation == Modulation::k4Fsk ||
          modulation == Modulation::kMsk)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Manchester is incompatible with 4-FSK and MSK\n");
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  const uint8_t value = static_cast<uint8_t>(modulation) << 4;
  if (!UpdateRegisterBits(Register::kMdmcfg2,
          kModulationMask, value)) {
    return false;
  }
  config_.modulation = modulation;
  bool result = SetOutputPower(config_.output_power_dbm);
  if (modulation == Modulation::kMsk) {
    result &= SetMskPhaseChangePeriod(
        config_.msk_phase_change_period);
  } else if (modulation != Modulation::kAskOok) {
    result &= SetFrequencyDeviation(
        config_.frequency_deviation_khz);
  }
  return result;
}

bool Cc1101::SetEncoding(Encoding encoding) {
  if (encoding == Encoding::kManchester &&
      (config_.fec_enabled ||
          config_.modulation == Modulation::k4Fsk ||
          config_.modulation == Modulation::kMsk)) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  bool result = true;
  switch (encoding) {
    case Encoding::kNrz:
      result &= UpdateRegisterBits(Register::kMdmcfg2,
          kManchesterMask, 0);
      result &= UpdateRegisterBits(Register::kPktctrl0,
          kWhiteningMask, 0);
      break;
    case Encoding::kManchester:
      result &= UpdateRegisterBits(Register::kMdmcfg2,
          kManchesterMask, kManchesterMask);
      result &= UpdateRegisterBits(Register::kPktctrl0,
          kWhiteningMask, 0);
      break;
    case Encoding::kWhitening:
      result &= UpdateRegisterBits(Register::kMdmcfg2,
          kManchesterMask, 0);
      result &= UpdateRegisterBits(Register::kPktctrl0,
          kWhiteningMask, kWhiteningMask);
      break;
    default:
      return false;
  }
  if (result) {
    config_.encoding = encoding;
  }
  return result;
}

bool Cc1101::SetSyncWord(
    uint8_t high, uint8_t low, SyncMode mode) {
  if (static_cast<uint8_t>(mode) > 7 || !EnsureIdle()) {
    return false;
  }
  const uint8_t values[] = {high, low};
  bool result = WriteBurst(Register::kSync1, values, sizeof(values));
  result &= UpdateRegisterBits(Register::kMdmcfg2, kSyncModeMask,
      static_cast<uint8_t>(mode));
  if (result) {
    config_.sync_word_high = high;
    config_.sync_word_low = low;
    config_.sync_mode = mode;
  }
  return result;
}

bool Cc1101::SetPreambleLength(uint16_t length_bits) {
  uint8_t value = 0;
  switch (length_bits) {
    case 16:
      value = 0;
      break;
    case 24:
      value = 1;
      break;
    case 32:
      value = 2;
      break;
    case 48:
      value = 3;
      break;
    case 64:
      value = 4;
      break;
    case 96:
      value = 5;
      break;
    case 128:
      value = 6;
      break;
    case 192:
      value = 7;
      break;
    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "Unsupported preamble length (length: %u)\n", length_bits);
      return false;
  }
  if (!EnsureIdle()) {
    return false;
  }

  bool result = UpdateRegisterBits(Register::kMdmcfg1,
      kPreambleMask, static_cast<uint8_t>(value << 4));
  if (result) {
    config_.preamble_length_bits = length_bits;
  }
  return result;
}

bool Cc1101::SetPreambleQualityThreshold(uint8_t threshold) {
  if (threshold > 7) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  if (!UpdateRegisterBits(Register::kPktctrl1,
          kPreambleQualityMask,
          static_cast<uint8_t>(threshold << 5))) {
    return false;
  }
  config_.preamble_quality_threshold = threshold;
  return true;
}

bool Cc1101::SetPacketLengthMode(
    PacketLengthMode mode, uint8_t maximum_length) {
  const bool supported = mode == PacketLengthMode::kFixed ||
      mode == PacketLengthMode::kVariable;
  const size_t autoflush_limit =
      (mode == PacketLengthMode::kVariable ? 63 : 64) -
      (config_.append_status ? 2 : 0);
  if (!supported || maximum_length == 0 ||
      (config_.fec_enabled &&
          (mode != PacketLengthMode::kFixed || maximum_length < 2))) {
    return false;
  }
  if (config_.crc_autoflush && maximum_length > autoflush_limit) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  bool result = UpdateRegisterBits(Register::kPktctrl0,
      kPacketLengthMask, static_cast<uint8_t>(mode));
  result &= WriteRegister(Register::kPktlen, maximum_length);
  if (result) {
    config_.packet_length_mode = mode;
    config_.maximum_packet_length = maximum_length;
  }
  return result;
}

bool Cc1101::SetAddressCheck(
    AddressCheck check, uint8_t device_address) {
  if (static_cast<uint8_t>(check) > 3 || !EnsureIdle()) {
    return false;
  }
  bool result = UpdateRegisterBits(Register::kPktctrl1,
      kAddressCheckMask, static_cast<uint8_t>(check));
  result &= WriteRegister(Register::kAddr, device_address);
  if (result) {
    config_.address_check = check;
    config_.device_address = device_address;
  }
  return result;
}

bool Cc1101::SetCrc(bool enabled) {
  if (!EnsureIdle()) {
    return false;
  }
  if (!enabled && config_.crc_autoflush &&
      !SetCrcAutoflush(false)) {
    return false;
  }
  if (!UpdateRegisterBits(Register::kPktctrl0,
          kCrcMask, enabled ? kCrcMask : 0)) {
    return false;
  }
  config_.crc_enabled = enabled;
  return true;
}

bool Cc1101::SetCrcAutoflush(bool enabled) {
  const size_t maximum =
      (config_.packet_length_mode == PacketLengthMode::kVariable ? 63 : 64) -
      (config_.append_status ? 2 : 0);
  if (enabled && (!config_.crc_enabled ||
      config_.maximum_packet_length > maximum)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "CRC autoflush packet length exceeds RX FIFO capacity\n");
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  if (!UpdateRegisterBits(Register::kPktctrl1,
          kCrcAutoflushMask, enabled ? kCrcAutoflushMask : 0)) {
    return false;
  }
  config_.crc_autoflush = enabled;
  return true;
}

bool Cc1101::SetFec(bool enabled) {
  if (enabled &&
      (config_.packet_length_mode != PacketLengthMode::kFixed ||
          config_.encoding == Encoding::kManchester ||
          config_.maximum_packet_length < 2)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "FEC requires fixed length, at least 2 bytes, and no Manchester\n");
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  if (!UpdateRegisterBits(Register::kMdmcfg1,
          kFecMask, enabled ? kFecMask : 0)) {
    return false;
  }
  config_.fec_enabled = enabled;
  return true;
}

bool Cc1101::SetChannel(uint8_t channel) {
  if (!EnsureIdle()) {
    return false;
  }
  if (!WriteRegister(Register::kChannr, channel)) {
    return false;
  }
  config_.channel = channel;
  return true;
}

bool Cc1101::SetCarrierSenseThreshold(
    int8_t absolute_threshold, uint8_t relative_threshold) {
  if (absolute_threshold < -8 || absolute_threshold > 7 ||
      relative_threshold > 3) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  const uint8_t absolute =
      static_cast<uint8_t>(absolute_threshold) &
      kCarrierSenseAbsoluteMask;
  const uint8_t relative =
      static_cast<uint8_t>(relative_threshold << 4);
  bool result = UpdateRegisterBits(Register::kAgcctrl1,
      kCarrierSenseAbsoluteMask, absolute);
  result &= UpdateRegisterBits(Register::kAgcctrl1,
      kCarrierSenseRelativeMask, relative);
  if (result) {
    config_.carrier_sense_threshold = absolute_threshold;
    config_.carrier_sense_relative = relative_threshold;
  }
  return result;
}

bool Cc1101::SetCcaMode(CcaMode mode) {
  if (static_cast<uint8_t>(mode) > 3) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  if (!UpdateRegisterBits(Register::kMcsm1, kCcaModeMask,
          static_cast<uint8_t>(mode) << 4)) {
    return false;
  }
  config_.cca_mode = mode;
  return true;
}

bool Cc1101::SetGdoMapping(
    Register output, uint8_t signal, bool inverted) {
  if ((output != Register::kIocfg0 && output != Register::kIocfg2) ||
      signal > 0x3F) {
    return false;
  }
  if (!EnsureIdle()) {
    return false;
  }
  return WriteRegister(output,
      static_cast<uint8_t>(signal | (inverted ? 0x40 : 0)));
}

bool Cc1101::Standby(uint32_t timeout_ms) {
  if (!Strobe(Command::kIdle)) {
    return false;
  }
  if (!WaitForState(State::kIdle, timeout_ms)) {
    return false;
  }
  if (sleeping_ && !RestoreAfterWakeup()) {
    return false;
  }
  sleeping_ = false;
  return true;
}

bool Cc1101::Sleep() {
  if (!Standby()) {
    return false;
  }
  uint8_t value = 0;
  if (!ReadRegister(Register::kTest0, &value)) {
    return false;
  }
  test0_value_ = value;
  if (!ReadRegister(Register::kFscal2, &value)) {
    return false;
  }
  fscal2_value_ = value;
  if (!Strobe(Command::kPowerDown)) {
    return false;
  }
  sleeping_ = true;
  return true;
}

bool Cc1101::Wakeup() {
  return Standby();
}

bool Cc1101::Calibrate(uint32_t timeout_ms) {
  if (!Standby() || !Strobe(Command::kCalibrate)) {
    return false;
  }
  return WaitForState(State::kIdle, timeout_ms);
}

bool Cc1101::StartReceive() {
  if (config_.packet_length_mode == PacketLengthMode::kInfinite) {
    return false;
  }
  bool result = Standby();
  result &= FlushRx();
  result &= WriteRegister(Register::kFifothr,
      kRxFifoThresholdMaximum);
  result &= WriteRegister(Register::kIocfg0,
      kGdoRxFifoThresholdOrPacketEnd);
  result &= Strobe(Command::kReceive);
  return result;
}

bool Cc1101::FlushRx() {
  if (!Standby()) {
    return false;
  }
  return Strobe(Command::kFlushRx);
}

bool Cc1101::FlushTx() {
  if (!Standby()) {
    return false;
  }
  return Strobe(Command::kFlushTx);
}

bool Cc1101::Transmit(const uint8_t* data, size_t length,
    uint32_t timeout_ms, uint8_t destination,
    bool include_destination) {
  const bool has_address = include_destination;
  const size_t air_length = length + (has_address ? 1 : 0);
  if (data == nullptr || length == 0 ||
      air_length > kMaximumPacketLength ||
      air_length > config_.maximum_packet_length ||
      (config_.fec_enabled && air_length < 2)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid packet length (length: %zu)\n", length);
    return false;
  }
  if (config_.packet_length_mode == PacketLengthMode::kFixed &&
      air_length != config_.maximum_packet_length) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Fixed packet length mismatch\n");
    return false;
  }
  if (config_.packet_length_mode == PacketLengthMode::kInfinite) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Infinite packet mode is not supported by Transmit\n");
    return false;
  }
  if (timeout_ms == 0) {
    timeout_ms = CalculatePacketTimeoutMs(length);
  }

  bool result = Standby();
  result &= FlushTx();
  result &= WriteRegister(Register::kIocfg0, kGdoSyncWord);
  if (!result) {
    return false;
  }

  std::vector<uint8_t> prefix;
  // 可变包先写空中包长；目标地址字段也属于空中包长。
  if (config_.packet_length_mode == PacketLengthMode::kVariable) {
    prefix.push_back(static_cast<uint8_t>(air_length));
  }
  if (has_address) {
    prefix.push_back(destination);
  }

  const size_t initial_payload = std::min(
      length, kFifoSize - prefix.size());
  if (!prefix.empty() &&
      !WriteBurst(Register::kFifo, prefix.data(), prefix.size())) {
    return false;
  }
  if (!WriteBurst(Register::kFifo, data, initial_payload)) {
    return false;
  }

  if (config_.cca_mode != CcaMode::kAlways) {
    if (!Strobe(Command::kReceive) ||
        !WaitForState(State::kReceive, 100)) {
      Standby();
      FlushTx();
      return false;
    }
    if (config_.cca_mode == CcaMode::kRssiBelowThreshold ||
        config_.cca_mode ==
            CcaMode::kRssiBelowThresholdUnlessReceiving) {
      // CCA 需要先等待 RX 链路产生有效 RSSI 采样。
      DelayUs(kCcaRssiSettlingUs);
    }
  }
  if (!Strobe(Command::kTransmit)) {
    Standby();
    FlushTx();
    return false;
  }
  const int64_t deadline = CurrentTimeMs() + timeout_ms;
  if (config_.cca_mode != CcaMode::kAlways &&
      !WaitForState(State::kTransmit,
          std::min<uint32_t>(10, timeout_ms))) {
    Standby();
    FlushTx();
    return false;
  }
  if (gdo0_ != kDefaultValue &&
      !WaitForGdo0(true, timeout_ms)) {
    Standby();
    FlushTx();
    return false;
  }
  size_t written = initial_payload;
  while (written < length) {
    // TXBYTES 属于连续变化状态寄存器，必须稳定读取后再补 FIFO。
    uint8_t tx_bytes = 0;
    if (!ReadStableStatus(Register::kTxbytes, &tx_bytes) ||
        (tx_bytes & kStatusFifoErrorMask) != 0) {
      result = false;
      break;
    }
    const size_t fifo_count = tx_bytes & kStatusFifoCountMask;
    if (fifo_count < kFifoSize) {
      const size_t count = std::min(
          kFifoSize - fifo_count, length - written);
      if (!WriteBurst(Register::kFifo, &data[written], count)) {
        result = false;
        break;
      }
      written += count;
    }
    if (CurrentTimeMs() >= deadline) {
      result = false;
      break;
    }
    DelayUs(CalculateFifoPollIntervalUs());
  }

  if (result && gdo0_ != kDefaultValue) {
    result &= WaitForGdo0(false,
        static_cast<uint32_t>(std::max<int64_t>(
            1, deadline - CurrentTimeMs())));
  }
  if (result) {
    result = WaitForState(State::kIdle,
        static_cast<uint32_t>(std::max<int64_t>(
            1, deadline - CurrentTimeMs())));
  }

  const bool cleanup = Standby() && FlushTx();
  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transmit failed or timed out\n");
  }
  return result && cleanup;
}

bool Cc1101::Receive(uint8_t* data, size_t capacity,
    size_t* received, PacketMetrics* metrics, uint32_t timeout_ms) {
  if (data == nullptr || received == nullptr || capacity == 0 ||
      gdo0_ == kDefaultValue) {
    return false;
  }
  if (config_.packet_length_mode == PacketLengthMode::kInfinite) {
    return false;
  }
  const size_t maximum_fifo_use = config_.maximum_packet_length +
      (config_.packet_length_mode == PacketLengthMode::kVariable ? 1 : 0) +
      (config_.append_status ? 2 : 0);
  const double bits_per_symbol =
      static_cast<double>(GetBitsPerSymbol());
  const double maximum_safe_rate =
      static_cast<double>(spi_frequency_hz_) /
      (8000.0 * bits_per_symbol);
  if (maximum_fifo_use > kFifoSize &&
      config_.data_rate_kbaud > maximum_safe_rate) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "SPI frequency is too low for long packet RX polling\n");
    return false;
  }
  if (timeout_ms == 0) {
    timeout_ms = CalculatePacketTimeoutMs(
        config_.maximum_packet_length);
  }
  *received = 0;

  bool result = Standby();
  result &= FlushRx();
  result &= WriteRegister(Register::kFifothr, 0x07);
  result &= WriteRegister(Register::kIocfg0, kGdoSyncWord);
  result &= Strobe(Command::kReceive);
  if (!result || !WaitForGdo0(true, timeout_ms)) {
    Standby();
    FlushRx();
    return false;
  }

  const int64_t deadline = CurrentTimeMs() + timeout_ms;
  size_t copied = 0;
  size_t packet_length = config_.packet_length_mode ==
          PacketLengthMode::kFixed
      ? config_.maximum_packet_length
      : 0;
  bool address_pending =
      config_.address_check != AddressCheck::kDisabled;
  while (GpioRead(gdo0_)) {
    // 保留至少一个 FIFO 字节，避免包仍在接收时误判 FIFO 为空。
    uint8_t rx_bytes = 0;
    if (!ReadStableStatus(Register::kRxbytes, &rx_bytes) ||
        (rx_bytes & kStatusFifoErrorMask) != 0) {
      result = false;
      break;
    }
    size_t fifo_count = rx_bytes & kStatusFifoCountMask;
    if (packet_length == 0 && fifo_count > 0) {
      uint8_t length_byte = 0;
      if (!ReadRegister(Register::kFifo, &length_byte) ||
          length_byte == 0) {
        result = false;
        break;
      }
      packet_length = length_byte;
      --fifo_count;
    }
    if (address_pending && packet_length > 0 && fifo_count > 0) {
      uint8_t address = 0;
      if (!ReadRegister(Register::kFifo, &address)) {
        result = false;
        break;
      }
      --packet_length;
      --fifo_count;
      address_pending = false;
    }
    if (fifo_count > 1 && copied < packet_length) {
      const size_t count = std::min(
          fifo_count - 1, packet_length - copied);
      if (!DrainReceiveFifo(data, capacity, &copied, count)) {
        result = false;
        break;
      }
    }
    if (CurrentTimeMs() >= deadline) {
      result = false;
      break;
    }
    DelayUs(CalculateFifoPollIntervalUs());
  }

  uint8_t final_rx_bytes = 0;
  if (result &&
      (!ReadStableStatus(Register::kRxbytes, &final_rx_bytes) ||
          (final_rx_bytes & kStatusFifoErrorMask) != 0)) {
    result = false;
  }
  if (result) {
    const size_t available =
        final_rx_bytes & kStatusFifoCountMask;
    const size_t required =
        (address_pending ? 1 : 0) +
        (packet_length >= copied ? packet_length - copied : 0) +
        (config_.append_status ? 2 : 0);
    if (packet_length == 0 || available < required) {
      result = false;
    }
  }
  if (result && address_pending && packet_length > 0) {
    uint8_t address = 0;
    if (!ReadRegister(Register::kFifo, &address)) {
      result = false;
    } else {
      --packet_length;
      address_pending = false;
    }
  }
  if (result && packet_length > 0) {
    const size_t remaining = packet_length - copied;
    if (!DrainReceiveFifo(data, capacity, &copied, remaining)) {
      result = false;
    }
  }

  if (result && config_.append_status) {
    uint8_t status[2] = {0};
    if (!ReadBurst(Register::kFifo, status, sizeof(status))) {
      result = false;
    } else {
      last_metrics_.rssi_dbm = DecodeRssi(status[0]);
      last_metrics_.lqi = status[1] & 0x7F;
      last_metrics_.crc_valid =
          !config_.crc_enabled || (status[1] & kCrcValidMask) != 0;
      if (metrics != nullptr) {
        *metrics = last_metrics_;
      }
      result &= last_metrics_.crc_valid;
    }
  } else if (result) {
    result = ReadPacketMetrics(metrics);
  }

  *received = copied;
  const bool cleanup = Standby() && FlushRx();
  return result && cleanup;
}

bool Cc1101::ReadReceivedPacket(uint8_t* data, size_t capacity,
    size_t* received, PacketMetrics* metrics) {
  if (data == nullptr || received == nullptr || capacity == 0) {
    return false;
  }
  *received = 0;

  uint8_t rx_bytes = 0;
  if (!ReadStableStatus(Register::kRxbytes, &rx_bytes) ||
      (rx_bytes & kStatusFifoErrorMask) != 0) {
    Standby();
    FlushRx();
    return false;
  }
  const size_t available = rx_bytes & kStatusFifoCountMask;
  if (available == 0) {
    return false;
  }
  const bool result = ReadPacketFromFifo(
      data, capacity, available, received, metrics);
  const bool cleanup = Standby() && FlushRx();
  return result && cleanup;
}

bool Cc1101::GetState(State* state) {
  if (state == nullptr) {
    return false;
  }
  uint8_t value = 0;
  if (!ReadStableStatus(Register::kMarcstate, &value)) {
    return false;
  }
  *state = static_cast<State>(value & kMarcStateMask);
  return true;
}

bool Cc1101::GetPartNumber(uint8_t* part_number) {
  return ReadStatusRegister(Register::kPartnum, part_number);
}

bool Cc1101::GetVersion(uint8_t* version) {
  return ReadStatusRegister(Register::kVersion, version);
}

bool Cc1101::GetRssi(float* rssi_dbm) {
  if (rssi_dbm == nullptr) {
    return false;
  }
  uint8_t raw = 0;
  if (!ReadStableStatus(Register::kRssi, &raw)) {
    return false;
  }
  *rssi_dbm = DecodeRssi(raw);
  return true;
}

bool Cc1101::GetLqi(uint8_t* lqi) {
  if (lqi == nullptr) {
    return false;
  }
  uint8_t raw = 0;
  if (!ReadStableStatus(Register::kLqi, &raw)) {
    return false;
  }
  *lqi = raw & 0x7F;
  return true;
}

bool Cc1101::GetChipStatus(ChipStatus* status) {
  if (status == nullptr) {
    return false;
  }
  const uint8_t command = static_cast<uint8_t>(Command::kNoOperation);
  uint8_t previous = 0;
  if (!Transfer(&command, &previous, 1)) {
    return false;
  }
  for (uint8_t attempt = 0; attempt < 32; ++attempt) {
    uint8_t current = 0;
    if (!Transfer(&command, &current, 1)) {
      return false;
    }
    if (current == previous) {
      *status = ParseChipStatus(current);
      return true;
    }
    previous = current;
  }
  return false;
}

bool Cc1101::Transfer(const uint8_t* write_data,
    uint8_t* read_data, size_t length, bool wait_ready) {
  if (bus_ == nullptr || write_data == nullptr ||
      read_data == nullptr || length == 0) {
    return false;
  }

  // CC1101 要求 CSn 拉低后等待 CHIP_RDYn，再开始发送首字节。
  if (!GpioWrite(cs_, false)) {
    return false;
  }
  bool result = true;
  if (wait_ready) {
    result = WaitForReady();
  }
  if (result) {
    result = bus_->WriteRead(write_data, read_data, length);
  }
  result &= GpioWrite(cs_, true);
  return result;
}

bool Cc1101::EnsureIdle() {
  if (sleeping_) {
    return Standby();
  }
  State state = State::kSleep;
  if (GetState(&state) && state == State::kIdle) {
    return true;
  }
  return Standby();
}

bool Cc1101::WaitForReady(uint32_t timeout_us) {
  const int64_t deadline = CurrentTimeUs() + timeout_us;
  while (GpioRead(miso_)) {
    if (CurrentTimeUs() >= deadline) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "CHIP_RDY timeout\n");
      return false;
    }
    DelayUs(1);
  }
  return true;
}

bool Cc1101::WaitForGdo0(bool level, uint32_t timeout_ms) {
  if (gdo0_ == kDefaultValue) {
    return false;
  }
  const int64_t deadline = CurrentTimeMs() + timeout_ms;
  while (GpioRead(gdo0_) != level) {
    if (CurrentTimeMs() >= deadline) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "GDO0 timeout (level: %d)\n", level);
      return false;
    }
    DelayUs(10);
  }
  return true;
}

bool Cc1101::WaitForState(State state, uint32_t timeout_ms) {
  const int64_t deadline = CurrentTimeMs() + timeout_ms;
  do {
    State current = State::kSleep;
    if (!GetState(&current)) {
      return false;
    }
    if (current == state) {
      return true;
    }
    if (current == State::kRxFifoOverflow ||
        current == State::kTxFifoUnderflow) {
      return false;
    }
    DelayUs(10);
  } while (CurrentTimeMs() < deadline);

  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "State timeout (state: %#X)\n", static_cast<uint8_t>(state));
  return false;
}

bool Cc1101::ReadStableStatus(Register address, uint8_t* value) {
  if (value == nullptr) {
    return false;
  }
  // TI 勘误 SWRZ020：连续变化的状态寄存器应读取至两次结果一致。
  uint8_t previous = 0;
  if (!ReadStatusRegister(address, &previous)) {
    return false;
  }
  for (uint8_t attempt = 0; attempt < 32; ++attempt) {
    uint8_t current = 0;
    if (!ReadStatusRegister(address, &current)) {
      return false;
    }
    if (current == previous) {
      *value = current;
      return true;
    }
    previous = current;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "Unstable status register (address: %#X)\n",
      static_cast<uint8_t>(address));
  return false;
}

bool Cc1101::UpdateRegisterBits(
    Register address, uint8_t mask, uint8_t value) {
  uint8_t current = 0;
  if (!ReadRegister(address, &current)) {
    return false;
  }
  current = static_cast<uint8_t>(
      (current & static_cast<uint8_t>(~mask)) | (value & mask));
  return WriteRegister(address, current);
}

bool Cc1101::ReadPacketFromFifo(uint8_t* data, size_t capacity,
    size_t available, size_t* received, PacketMetrics* metrics) {
  // 异步读取必须把长度字节、地址和附加状态都计入 64 字节 FIFO。
  size_t packet_length = config_.maximum_packet_length;
  size_t required = config_.append_status ? 2 : 0;
  if (config_.packet_length_mode == PacketLengthMode::kVariable) {
    uint8_t length_byte = 0;
    if (!ReadRegister(Register::kFifo, &length_byte) ||
        length_byte == 0) {
      return false;
    }
    packet_length = length_byte;
    ++required;
  }
  required += packet_length;
  if (required > kFifoSize || required > available) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Incomplete packet in FIFO (need: %zu available: %zu)\n",
        required, available);
    return false;
  }
  if (config_.address_check != AddressCheck::kDisabled) {
    uint8_t address = 0;
    if (!ReadRegister(Register::kFifo, &address)) {
      return false;
    }
    if (packet_length == 0) {
      return false;
    }
    --packet_length;
  }
  if (packet_length > capacity) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Packet exceeds buffer (length: %zu)\n", packet_length);
    return false;
  }
  if (!ReadBurst(Register::kFifo, data, packet_length)) {
    return false;
  }
  *received = packet_length;

  if (config_.append_status) {
    uint8_t status[2] = {0};
    if (!ReadBurst(Register::kFifo, status, sizeof(status))) {
      return false;
    }
    last_metrics_.rssi_dbm = DecodeRssi(status[0]);
    last_metrics_.lqi = status[1] & 0x7F;
    last_metrics_.crc_valid =
        !config_.crc_enabled || (status[1] & kCrcValidMask) != 0;
    if (metrics != nullptr) {
      *metrics = last_metrics_;
    }
    return last_metrics_.crc_valid;
  }
  return ReadPacketMetrics(metrics);
}

bool Cc1101::DrainReceiveFifo(uint8_t* data, size_t capacity,
    size_t* copied, size_t bytes_to_read) {
  if (copied == nullptr || *copied + bytes_to_read > capacity) {
    return false;
  }
  if (bytes_to_read == 0) {
    return true;
  }
  if (!ReadBurst(Register::kFifo, &data[*copied], bytes_to_read)) {
    return false;
  }
  *copied += bytes_to_read;
  return true;
}

bool Cc1101::ReadPacketMetrics(PacketMetrics* metrics) {
  uint8_t rssi = 0;
  uint8_t lqi = 0;
  if (!ReadStableStatus(Register::kRssi, &rssi) ||
      !ReadStableStatus(Register::kLqi, &lqi)) {
    return false;
  }
  last_metrics_.rssi_dbm = DecodeRssi(rssi);
  last_metrics_.lqi = lqi & 0x7F;
  last_metrics_.crc_valid =
      !config_.crc_enabled || (lqi & kCrcValidMask) != 0;
  if (metrics != nullptr) {
    *metrics = last_metrics_;
  }
  return last_metrics_.crc_valid;
}

size_t Cc1101::GetMaximumBurstLength(Register address) const {
  const uint8_t raw_address = static_cast<uint8_t>(address);
  if (raw_address <= static_cast<uint8_t>(Register::kTest0)) {
    return static_cast<size_t>(
        static_cast<uint8_t>(Register::kTest0) - raw_address + 1);
  }
  if (address == Register::kPatable) {
    return 8;
  }
  if (address == Register::kFifo) {
    return kFifoSize;
  }
  return 0;
}

bool Cc1101::SelectPaValue(int8_t power_dbm, uint8_t* value) const {
  if (value == nullptr) {
    return false;
  }
  constexpr int8_t kPowerLevels[] = {
      -30, -20, -15, -10, 0, 5, 7, 10,
  };
  constexpr uint8_t kPaTable[][4] = {
      {0x12, 0x12, 0x03, 0x03},
      {0x0D, 0x0E, 0x0F, 0x0E},
      {0x1C, 0x1D, 0x1E, 0x1E},
      {0x34, 0x34, 0x27, 0x27},
      {0x51, 0x60, 0x50, 0x8E},
      {0x85, 0x84, 0x81, 0xCD},
      {0xCB, 0xC8, 0xCB, 0xC7},
      {0xC2, 0xC0, 0xC2, 0xC0},
  };

  uint8_t band = 0;
  if (config_.frequency_mhz >= 387.0 &&
      config_.frequency_mhz <= 464.0) {
    band = 1;
  } else if (config_.frequency_mhz >= 779.0 &&
      config_.frequency_mhz < 891.5) {
    band = 2;
  } else if (config_.frequency_mhz >= 891.5) {
    band = 3;
  }

  for (size_t index = 0;
       index < sizeof(kPowerLevels) / sizeof(kPowerLevels[0]); ++index) {
    if (power_dbm == kPowerLevels[index]) {
      *value = kPaTable[index][band];
      return true;
    }
  }
  return false;
}

bool Cc1101::ValidateConfig(const Config& config) const {
  const bool modulation_valid =
      config.modulation == Modulation::k2Fsk ||
      config.modulation == Modulation::kGfsk ||
      config.modulation == Modulation::kAskOok ||
      config.modulation == Modulation::k4Fsk ||
      config.modulation == Modulation::kMsk;
  const bool encoding_valid = config.encoding == Encoding::kNrz ||
      config.encoding == Encoding::kManchester ||
      config.encoding == Encoding::kWhitening;
  const bool packet_mode_valid =
      config.packet_length_mode == PacketLengthMode::kFixed ||
      config.packet_length_mode == PacketLengthMode::kVariable;
  const size_t autoflush_limit =
      (config.packet_length_mode == PacketLengthMode::kVariable ? 63 : 64) -
      (config.append_status ? 2 : 0);
  const bool manchester_conflict =
      config.encoding == Encoding::kManchester &&
      (config.fec_enabled || config.modulation == Modulation::k4Fsk ||
          config.modulation == Modulation::kMsk);
  const double crystal_hz =
      config.crystal_frequency_mhz * 1000000.0;
  const double minimum_bandwidth_khz =
      std::floor(crystal_hz / 448.0 / 100.0) / 10.0;
  const double maximum_bandwidth_khz =
      std::ceil(crystal_hz / 32.0 / 100.0) / 10.0;
  const double minimum_spacing_khz =
      crystal_hz * 256.0 /
      static_cast<double>(1UL << 18) / 1000.0;
  const double maximum_spacing_khz =
      crystal_hz * 511.0 * 8.0 /
      static_cast<double>(1UL << 18) / 1000.0;
  const double minimum_deviation_khz =
      crystal_hz * 8.0 /
      static_cast<double>(1UL << 17) / 1000.0;
  const double maximum_deviation_khz =
      crystal_hz * 15.0 * 128.0 /
      static_cast<double>(1UL << 17) / 1000.0;
  const bool deviation_used =
      config.modulation == Modulation::k2Fsk ||
      config.modulation == Modulation::kGfsk ||
      config.modulation == Modulation::k4Fsk;
  return config.crystal_frequency_mhz >= 26.0 &&
      config.crystal_frequency_mhz <= 27.0 &&
      ValidateFrequency(
          config.frequency_mhz, config.crystal_frequency_mhz) &&
      modulation_valid && encoding_valid && packet_mode_valid &&
      ValidateDataRate(config.data_rate_kbaud, config.modulation) &&
      config.receive_bandwidth_khz >= minimum_bandwidth_khz &&
      config.receive_bandwidth_khz <= maximum_bandwidth_khz &&
      config.channel_spacing_khz >= minimum_spacing_khz &&
      config.channel_spacing_khz <= maximum_spacing_khz &&
      (!deviation_used ||
          (config.frequency_deviation_khz >= minimum_deviation_khz &&
              config.frequency_deviation_khz <= maximum_deviation_khz)) &&
      ValidatePreambleLength(config.preamble_length_bits) &&
      ValidateOutputPower(config.output_power_dbm) &&
      config.maximum_packet_length != 0 && !manchester_conflict &&
      (!config.fec_enabled ||
          (config.packet_length_mode == PacketLengthMode::kFixed &&
              config.maximum_packet_length >= 2)) &&
      (!config.crc_autoflush ||
          (config.crc_enabled &&
              config.maximum_packet_length <= autoflush_limit)) &&
      config.bit_rate_tolerance <= kBitRateToleranceMask &&
      config.carrier_sense_threshold >= -8 &&
      config.carrier_sense_threshold <= 7 &&
      config.carrier_sense_relative <= 3 &&
      config.preamble_quality_threshold <= 7 &&
      config.msk_phase_change_period <= 7 &&
      static_cast<uint8_t>(config.address_check) <= 3 &&
      static_cast<uint8_t>(config.sync_mode) <= 7 &&
      static_cast<uint8_t>(config.cca_mode) <= 3;
}

bool Cc1101::ValidateDataRate(
    double data_rate_kbaud, Modulation modulation) const {
  if (!std::isfinite(data_rate_kbaud)) {
    return false;
  }
  double minimum_kbaud = 0.6;
  double maximum_kbaud = 0.0;
  switch (modulation) {
    case Modulation::k2Fsk:
      maximum_kbaud = 500.0;
      break;
    case Modulation::kGfsk:
    case Modulation::kAskOok:
      maximum_kbaud = 250.0;
      break;
    case Modulation::k4Fsk:
      maximum_kbaud = 300.0;
      break;
    case Modulation::kMsk:
      minimum_kbaud = 26.0;
      maximum_kbaud = 500.0;
      break;
    default:
      return false;
  }
  return data_rate_kbaud >= minimum_kbaud &&
      data_rate_kbaud <= maximum_kbaud;
}

uint8_t Cc1101::GetBitsPerSymbol() const {
  return config_.modulation == Modulation::k4Fsk ? 2 : 1;
}

bool Cc1101::ValidatePreambleLength(uint16_t length_bits) const {
  return length_bits == 16 || length_bits == 24 ||
      length_bits == 32 || length_bits == 48 ||
      length_bits == 64 || length_bits == 96 ||
      length_bits == 128 || length_bits == 192;
}

bool Cc1101::ValidateOutputPower(int8_t power_dbm) const {
  constexpr int8_t kPowerLevels[] = {
      -30, -20, -15, -10, 0, 5, 7, 10,
  };
  for (const int8_t supported : kPowerLevels) {
    if (power_dbm == supported) {
      return true;
    }
  }
  return false;
}

bool Cc1101::ValidateFrequency(
    double frequency_mhz, double crystal_frequency_mhz) const {
  if (!std::isfinite(frequency_mhz) ||
      !std::isfinite(crystal_frequency_mhz)) {
    return false;
  }
  const double middle_band_minimum =
      crystal_frequency_mhz >= 27.0 ? 392.0 : 387.0;
  return (frequency_mhz >= 300.0 && frequency_mhz <= 348.0) ||
      (frequency_mhz >= middle_band_minimum && frequency_mhz <= 464.0) ||
      (frequency_mhz >= 779.0 && frequency_mhz <= 928.0);
}

uint32_t Cc1101::CalculatePacketTimeoutMs(size_t length) const {
  const double data_rate_bps = config_.data_rate_kbaud * 1000.0;
  const double air_time_ms =
      static_cast<double>((length + 16) * 8) * 1000.0 /
      data_rate_bps;
  return static_cast<uint32_t>(
      std::max(100.0, std::ceil(air_time_ms * 5.0 + 10.0)));
}

uint32_t Cc1101::CalculateFifoPollIntervalUs() const {
  const double bits_per_symbol =
      static_cast<double>(GetBitsPerSymbol());
  const double maximum_interval_us =
      4000.0 /
      (std::max(0.6, config_.data_rate_kbaud) * bits_per_symbol);
  const double status_read_us = 32000000.0 /
      static_cast<double>(std::max(int32_t{1}, spi_frequency_hz_));
  const double interval_us =
      std::max(0.0, maximum_interval_us - status_read_us);
  return static_cast<uint32_t>(
      std::min(1000.0, std::floor(interval_us)));
}

bool Cc1101::RestoreAfterWakeup() {
  bool result = WriteRegister(Register::kTest0, test0_value_);
  result &= WriteRegister(Register::kFscal2, fscal2_value_);
  result &= WriteBurst(Register::kPatable,
      pa_table_cache_, pa_table_length_);
  return result;
}

int64_t Cc1101::CurrentTimeUs() const {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  return esp_timer_get_time();
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  const uint32_t current = micros();
  if (current < last_micros_) {
    micros_epoch_ += (1ULL << 32);
  }
  last_micros_ = current;
  return static_cast<int64_t>(micros_epoch_ + current);
#endif
}

int64_t Cc1101::CurrentTimeMs() const {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  return esp_timer_get_time() / 1000;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  return CurrentTimeUs() / 1000;
#endif
}

float Cc1101::DecodeRssi(uint8_t raw) const {
  const int16_t signed_value = raw >= 128
      ? static_cast<int16_t>(raw) - 256
      : static_cast<int16_t>(raw);
  return static_cast<float>(signed_value) / 2.0F - 74.0F;
}

Cc1101::ChipStatus Cc1101::ParseChipStatus(uint8_t raw) const {
  ChipStatus status;
  status.ready = (raw & kChipReadyMask) == 0;
  status.state = static_cast<uint8_t>(
      (raw & kStateMask) >> 4);
  status.fifo_bytes_available = raw & kFifoCountMask;
  return status;
}
}  // namespace cpp_bus_driver
