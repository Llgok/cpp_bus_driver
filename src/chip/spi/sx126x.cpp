/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-06-30 17:09:45
 * @License: GPL 3.0
 */
#include "sx126x.h"

namespace cpp_bus_driver {
namespace {
constexpr uint8_t kNop = 0x00;
}  // namespace

bool Sx126x::Init(int32_t freq_hz) {
  initialized_ = false;
  sleeping_ = false;
  configured_ = false;
  gfsk_low_rate_workaround_active_ = false;
  image_calibration_start_mhz_ = 902;
  image_calibration_end_mhz_ = 928;

  if (!ValidateHardwareConfig()) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Invalid hardware config\n");
    return false;
  }

  if (busy_ != kDefaultValue) {
    bool result = true;
    result &= SetGpioMode(busy_, GpioMode::kInput, GpioStatus::kDisable);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Busy failed\n");
      return false;
    }
  }

  if (rst_ != kDefaultValue) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    result &= GpioWrite(rst_, 1);
    DelayMs(10);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(10);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!ChipSpiGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  DeviceId device_id;
  if (!GetDeviceId(device_id) || (device_id.bytes != kDeviceId)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Get sx126x id failed (error id: %.6s)\n",
        reinterpret_cast<const char*>(device_id.bytes.data()));
    Deinit(false);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sx126x id success (id: %.6s)\n",
        reinterpret_cast<const char*>(device_id.bytes.data()));
  }

  if (!ApplyHardwareConfig(hardware_config_.enable_dio3_tcxo)) {
    Deinit(false);
    return false;
  }

  initialized_ = true;
  return true;
}

bool Sx126x::Deinit(bool delete_bus) {
  if (!ChipSpiGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  bool result = true;
  if (busy_ != kDefaultValue) {
    result &= ResetGpio(busy_);
  }
  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  initialized_ = false;
  sleeping_ = false;
  configured_ = false;
  gfsk_low_rate_workaround_active_ = false;

  return result;
}

bool Sx126x::GetDeviceId(DeviceId& device_id) {
  device_id = DeviceId{};
  if (!ReadRegister(static_cast<uint16_t>(Reg::kRoDeviceId),
          device_id.bytes.data(), device_id.bytes.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  return true;
}

bool Sx126x::GetConfig(LoraConfig& config) const {
  config = LoraConfig{};
  if (!configured_ || (param_.packet_type != PacketType::kLora)) {
    return false;
  }
  config = lora_config_;
  return true;
}

bool Sx126x::GetConfig(GfskConfig& config) const {
  config = GfskConfig{};
  if (!configured_ || (param_.packet_type != PacketType::kGfsk)) {
    return false;
  }
  config = gfsk_config_;
  return true;
}

bool Sx126x::CheckBusy() {
  if (busy_ != kDefaultValue) {
    uint16_t timeout_count = 0;
    while (1) {
      DelayUs(1);
      if (GpioRead(busy_) == 0) {
        break;
      }
      timeout_count++;
      if (timeout_count > kBusyPinTimeoutCount) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__, "busy_ timeout\n");
        return false;
      }
    }
  } else if (busy_wait_callback_ != nullptr) {
    uint16_t timeout_count = 0;
    while (1) {
      DelayUs(1);
      if (busy_wait_callback_() == 0) {
        break;
      }
      timeout_count++;
      if (timeout_count > kBusyFunctionTimeoutCount) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "busy_wait_callback_ timeout\n");
        return false;
      }
    }
  } else {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "CheckBusy failed\n");
    return false;
  }

  return true;
}

bool Sx126x::GetStatus(uint8_t& status) {
  status = 0;
  if (!CheckBusy()) {
    return false;
  }
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetStatus), &status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return true;
}

Sx126x::CmdStatus Sx126x::ParseCmdStatus(uint8_t parse_status) {
  if ((parse_status == 0x00) || (parse_status == static_cast<uint8_t>(-1))) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return CmdStatus::kFalse;
  }

  const uint8_t buffer = (parse_status & 0B00001110) >> 1;

  switch (buffer) {
    case static_cast<uint8_t>(CmdStatus::kRfu):
      break;
    case static_cast<uint8_t>(CmdStatus::kDataIsAvailableToHost):
      break;
    case static_cast<uint8_t>(CmdStatus::kCmdTimeout):
      break;
    case static_cast<uint8_t>(CmdStatus::kCmdProcessingError):
      break;
    case static_cast<uint8_t>(CmdStatus::kFailToExecuteCmd):
      break;
    case static_cast<uint8_t>(CmdStatus::kCmdTxDone):
      break;

    default:
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      return CmdStatus::kFalse;
  }

  return static_cast<CmdStatus>(buffer);
}

Sx126x::ChipModeStatus Sx126x::ParseChipModeStatus(uint8_t parse_status) {
  if ((parse_status == 0x00) || (parse_status == static_cast<uint8_t>(-1))) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return ChipModeStatus::kFalse;
  }

  const uint8_t buffer = (parse_status & 0B01110000) >> 4;

  switch (buffer) {
    case static_cast<uint8_t>(ChipModeStatus::kStbyRc):
      break;
    case static_cast<uint8_t>(ChipModeStatus::kStbyXosc):
      break;
    case static_cast<uint8_t>(ChipModeStatus::kFs):
      break;
    case static_cast<uint8_t>(ChipModeStatus::kRx):
      break;
    case static_cast<uint8_t>(ChipModeStatus::kTx):
      break;

    default:
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      return ChipModeStatus::kFalse;
  }

  return static_cast<ChipModeStatus>(buffer);
}

Sx126x::IrqStatus Sx126x::ParseIrqStatus(uint16_t irq_flag) {
  IrqStatus status;
  status.all_flag.tx_done = irq_flag & 0B0000000000000001;
  status.all_flag.rx_done = (irq_flag & 0B0000000000000010) >> 1;
  status.all_flag.preamble_detected = (irq_flag & 0B0000000000000100) >> 2;
  status.gfsk_flag.sync_word_valid = (irq_flag & 0B0000000000001000) >> 3;
  status.lora_reg_flag.header_valid = (irq_flag & 0B0000000000010000) >> 4;
  status.lora_reg_flag.header_error = (irq_flag & 0B0000000000100000) >> 5;
  status.all_flag.crc_error = (irq_flag & 0B0000000001000000) >> 6;
  status.lora_reg_flag.cad_done = (irq_flag & 0B0000000010000000) >> 7;
  status.lora_reg_flag.cad_detected = (irq_flag & 0B0000000100000000) >> 8;
  status.all_flag.tx_rx_timeout = (irq_flag & 0B0000001000000000) >> 9;
  status.lrfhss_flag.pa_ramped_up_hop = (irq_flag & 0B0100000000000000) >> 14;

  return status;
}

bool Sx126x::SetStandby(StdbyConfig config) {
  const uint8_t buffer = static_cast<uint8_t>(config);
  if (buffer > 1) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (!WriteCommand(Cmd::kWoSetStandby, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetDio3AsTcxoCtrl(Dio3TcxoVoltage voltage, uint32_t time_out_us) {
  if ((static_cast<uint8_t>(voltage) > 7) ||
      (time_out_us > 262143984U)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range\n");
    return false;
  }
  const uint32_t timeout = MicrosecondsToRtcStep(time_out_us);
  uint8_t buffer[] = {
      static_cast<uint8_t>(voltage),
      static_cast<uint8_t>(timeout >> 16),
      static_cast<uint8_t>(timeout >> 8),
      static_cast<uint8_t>(timeout),
  };

  if (!WriteCommand(Cmd::kWoSetDio3AsTcxoCtrl, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::FixTxClamp(bool enable) {
  uint8_t buffer = 0;

  if (!ReadRegister(static_cast<uint16_t>(Reg::kRwTxClampConfig), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  if (enable) {
    buffer |= 0B00011110;
  } else {
    buffer = (buffer & 0B11100001) | 0B00010000;
  }

  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwTxClampConfig), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetBufferBaseAddress(
    uint8_t tx_base_address, uint8_t rx_base_address) {
  uint8_t buffer[] = {tx_base_address, rx_base_address};

  if (!WriteCommand(Cmd::kWoSetBufferBaseAddress, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetPacketType(PacketType type) {
  const uint8_t buffer = static_cast<uint8_t>(type);
  if ((type != PacketType::kGfsk) && (type != PacketType::kLora) &&
      (type != PacketType::kLrFhss)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (!WriteCommand(Cmd::kWoSetPacketType, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }
  if (type != param_.packet_type) {
    configured_ = false;
  }
  param_.packet_type = type;

  return true;
}

bool Sx126x::SetRxTxFallbackMode(FallbackMode mode) {
  const uint8_t buffer = static_cast<uint8_t>(mode);
  if ((mode != FallbackMode::kStdbyRc) &&
      (mode != FallbackMode::kStdbyXosc) && (mode != FallbackMode::kFs)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (!WriteCommand(Cmd::kWoSetRxTxFallbackMode, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetCadParams(CadSymbolNum num, uint8_t cad_det_peak,
    uint8_t cad_det_min, CadExitMode exit_mode, uint32_t time_out_us) {
  const uint8_t symbol_count = static_cast<uint8_t>(num);
  const uint8_t exit = static_cast<uint8_t>(exit_mode);
  if ((symbol_count > 4) ||
      ((exit != 0) && (exit != 1) && (exit != 0x10)) ||
      (time_out_us > 262143984U)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  const uint32_t timeout = MicrosecondsToRtcStep(time_out_us);
  uint8_t buffer[] = {
      static_cast<uint8_t>(num),
      cad_det_peak,
      cad_det_min,
      static_cast<uint8_t>(exit_mode),
      static_cast<uint8_t>(timeout >> 16),
      static_cast<uint8_t>(timeout >> 8),
      static_cast<uint8_t>(timeout),
  };

  if (!WriteCommand(Cmd::kWoSetCadParams, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::ClearIrqFlag(IrqMaskFlag flag) {
  return ClearIrqFlag(static_cast<uint16_t>(flag));
}

bool Sx126x::ClearIrqFlag(uint16_t flags) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(flags >> 8),
      static_cast<uint8_t>(flags),
  };

  if (!WriteCommand(Cmd::kWoClearIrqStatus, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetDioIrqParams(uint16_t irq_mask, uint16_t dio1_mask,
    uint16_t dio2_mask, uint16_t dio3_mask) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(irq_mask >> 8),
      static_cast<uint8_t>(irq_mask),
      static_cast<uint8_t>(dio1_mask >> 8),
      static_cast<uint8_t>(dio1_mask),
      static_cast<uint8_t>(dio2_mask >> 8),
      static_cast<uint8_t>(dio2_mask),
      static_cast<uint8_t>(dio3_mask >> 8),
      static_cast<uint8_t>(dio3_mask),
  };

  if (!WriteCommand(Cmd::kWoSetDioIrqParams, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::Calibrate(uint8_t calib_param) {
  if (!WriteCommand(Cmd::kWoCalibrate, &calib_param, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::GetPacketType(PacketType& packet_type) {
  packet_type = PacketType::kFalse;
  uint8_t buffer = 0;

  if (!ReadCommand(Cmd::kRoGetPacketType, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  switch (buffer) {
    case static_cast<uint8_t>(PacketType::kGfsk):
      break;
    case static_cast<uint8_t>(PacketType::kLora):
      break;
    case static_cast<uint8_t>(PacketType::kLrFhss):
      break;

    default:
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }

  packet_type = static_cast<PacketType>(buffer);
  if (packet_type != param_.packet_type) {
    configured_ = false;
  }
  param_.packet_type = packet_type;
  return true;
}

bool Sx126x::SetRegulatorMode(RegulatorMode mode) {
  const uint8_t buffer = static_cast<uint8_t>(mode);
  if (buffer > 1) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (!WriteCommand(Cmd::kWoSetRegulatorMode, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }
  param_.regulator_mode = mode;

  return true;
}

bool Sx126x::SetCurrentLimit(float current) {
  if (!std::isfinite(current)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (current < 0.0) {
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    current = 0.0;
  } else if (current > 157.5) {
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    current = 157.5;
  }

  const uint8_t buffer = static_cast<uint8_t>(current / 2.5f);

  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  param_.current_limit = current;

  return true;
}

bool Sx126x::GetCurrentLimit(float& current_ma) {
  current_ma = 0.0f;
  uint8_t buffer = 0;

  if (!ReadRegister(
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  current_ma = static_cast<float>(buffer) * 2.5f;
  return true;
}

bool Sx126x::SetDio2AsRfSwitchCtrl(Dio2Mode mode) {
  const uint8_t buffer = static_cast<uint8_t>(mode);
  if (buffer > 1) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (!WriteCommand(Cmd::kWoSetDio2AsRfSwitchCtrl, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetPaConfig(uint8_t pa_duty_cycle, uint8_t hp_max) {
  const uint8_t max_duty_cycle =
      ((chip_type_ == ChipType::kSx1261) && (param_.freq_mhz >= 400.0))
          ? 0x07
          : 0x04;
  if ((pa_duty_cycle > max_duty_cycle) || (hp_max > 0x07) ||
      ((chip_type_ == ChipType::kSx1261) && (hp_max != 0))) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range\n");
    return false;
  }

  uint8_t buffer[] = {
      pa_duty_cycle,
      hp_max,
      static_cast<uint8_t>(chip_type_),
      0x01,  // 这一位固定为0x01
  };

  if (!WriteCommand(Cmd::kWoSetPaConfig, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetTxParams(int8_t power, RampTime ramp_time) {
  const int8_t min_power = (chip_type_ == ChipType::kSx1261) ? -17 : -9;
  const int8_t max_power = (chip_type_ == ChipType::kSx1261) ? 14 : 22;
  if ((power < min_power) || (power > max_power) ||
      (static_cast<uint8_t>(ramp_time) > 7)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range\n");
    return false;
  }

  uint8_t buffer[] = {
      static_cast<uint8_t>(power), static_cast<uint8_t>(ramp_time)};

  if (!WriteCommand(Cmd::kWoSetTxParams, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetLoraSyncWord(uint16_t sync_word) {
  uint8_t buffer[2] = {0};

  if (sync_word <= 0xFF) {
    if (!ReadRegister(
            static_cast<uint16_t>(Reg::kRwLoraSyncWordStart), buffer, 2)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
      return false;
    }

    buffer[0] = (buffer[0] & 0x0F) | (sync_word & 0xF0);
    buffer[1] =
        (buffer[1] & 0x0F) | static_cast<uint8_t>((sync_word & 0x0F) << 4);
  } else {
    buffer[0] = static_cast<uint8_t>(sync_word >> 8);
    buffer[1] = static_cast<uint8_t>(sync_word);
  }

  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwLoraSyncWordStart), buffer, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  param_.lora.sync_word = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];

  return true;
}

bool Sx126x::GetLoraSyncWord(uint16_t& sync_word) {
  sync_word = 0;
  uint8_t buffer[2] = {0};

  if (!ReadRegister(
          static_cast<uint16_t>(Reg::kRwLoraSyncWordStart), buffer, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  sync_word = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
  return true;
}

bool Sx126x::FixLoraInvertedIq(InvertIq iq) {
  uint8_t buffer = 0;

  if (!ReadRegister(
          static_cast<uint16_t>(Reg::kRwIqPolaritySetup), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  if (iq == InvertIq::kStandardIqSetup) {
    buffer |= 0B00000100;
  } else {
    buffer &= 0B11111011;
  }

  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwIqPolaritySetup), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  param_.lora.invert_iq = iq;

  return true;
}

bool Sx126x::SetLoraModulationParams(Sf sf, LoraBw bw, Cr cr, Ldro ldro) {
  const uint8_t sf_value = static_cast<uint8_t>(sf);
  const uint8_t cr_value = static_cast<uint8_t>(cr);
  if ((sf_value < 5) || (sf_value > 12) ||
      (GetLoraBandwidthHz(bw) <= 0.0f) || (cr_value < 1) ||
      (cr_value > 4) || (static_cast<uint8_t>(ldro) > 1)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer[] = {static_cast<uint8_t>(sf), static_cast<uint8_t>(bw),
      static_cast<uint8_t>(cr), static_cast<uint8_t>(ldro)};

  if (!WriteCommand(Cmd::kWoSetModulationParams, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }
  if (!FixBw500KhzSensitivity(bw == LoraBw::kBw500000Hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "FixBw500KhzSensitivity failed\n");
    return false;
  }
  param_.lora.spreading_factor = sf;
  param_.lora.band_width = bw;
  param_.lora.cr = cr;
  param_.lora.low_data_rate_optimize = ldro;

  return true;
}

bool Sx126x::SetLoraPacketParams(uint16_t preamble_length,
    LoraHeaderType header_type, uint8_t payload_length, LoraCrcType crc_type,
    InvertIq iq) {
  if ((preamble_length == 0) ||
      (static_cast<uint8_t>(header_type) > 1) ||
      (static_cast<uint8_t>(crc_type) > 1) ||
      (static_cast<uint8_t>(iq) > 1)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer[] = {
      static_cast<uint8_t>(preamble_length >> 8),
      static_cast<uint8_t>(preamble_length),
      static_cast<uint8_t>(header_type),
      payload_length,
      static_cast<uint8_t>(crc_type),
      static_cast<uint8_t>(iq),
  };

  if (!WriteCommand(Cmd::kWoSetPacketParams, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }
  if (!FixLoraInvertedIq(iq)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "FixLoraInvertedIq failed\n");
    return false;
  }
  param_.lora.preamble_length = preamble_length;
  param_.lora.header_type = header_type;
  param_.lora.payload_length = payload_length;
  param_.lora.crc_type = crc_type;
  param_.lora.invert_iq = iq;

  return true;
}

bool Sx126x::SetOutputPower(int8_t power, RampTime ramp_time) {
  const int8_t min_power = (chip_type_ == ChipType::kSx1261) ? -17 : -9;
  const int8_t max_power = (chip_type_ == ChipType::kSx1261) ? 14 : 22;

  if (power < min_power) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    power = min_power;
  } else if (power > max_power) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    power = max_power;
  }

  uint8_t ocp_config = 0;

  // 读取OCP配置
  if (!ReadRegister(
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), &ocp_config, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  const uint8_t pa_duty_cycle = 0x04;
  const uint8_t hp_max = (chip_type_ == ChipType::kSx1261) ? 0x00 : 0x07;
  if (!SetPaConfig(pa_duty_cycle, hp_max)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetPaConfig failed\n");
    return false;
  }

  if (!SetTxParams(power, ramp_time)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetTxParams failed\n");
    return false;
  }

  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), &ocp_config, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  param_.power = power;

  return true;
}

bool Sx126x::CalibrateImage(ImgCalFreq freq_mhz) {
  uint8_t buffer[2] = {0};

  switch (freq_mhz) {
    case ImgCalFreq::kFreq430_440Mhz:
      buffer[0] = 0x6B;
      buffer[1] = 0x6F;
      break;
    case ImgCalFreq::kFreq470_510Mhz:
      buffer[0] = 0x75;
      buffer[1] = 0x81;
      break;
    case ImgCalFreq::kFreq779_787Mhz:
      buffer[0] = 0xC1;
      buffer[1] = 0xC5;
      break;
    case ImgCalFreq::kFreq863_870Mhz:
      buffer[0] = 0xD7;
      buffer[1] = 0xDB;
      break;
    case ImgCalFreq::kFreq902_928Mhz:
      buffer[0] = 0xE1;
      buffer[1] = 0xE9;
      break;

    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
      return false;
  }

  if (!WriteCommand(Cmd::kWoCalibrateImage, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::CalibrateImage(uint16_t start_freq_mhz, uint16_t end_freq_mhz) {
  if (start_freq_mhz > end_freq_mhz) {
    std::swap(start_freq_mhz, end_freq_mhz);
  }

  if ((start_freq_mhz < 150) || (end_freq_mhz < 150) ||
      (start_freq_mhz > 960) || (end_freq_mhz > 960)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
  }
  start_freq_mhz =
      std::min<uint16_t>(960, std::max<uint16_t>(150, start_freq_mhz));
  end_freq_mhz = std::min<uint16_t>(960, std::max<uint16_t>(150, end_freq_mhz));

  static constexpr uint16_t kImageCalibrationStepMhz = 4;
  const uint8_t buffer[] = {
      static_cast<uint8_t>(start_freq_mhz / kImageCalibrationStepMhz),
      static_cast<uint8_t>((end_freq_mhz + kImageCalibrationStepMhz - 1) /
                           kImageCalibrationStepMhz),
  };

  if (!WriteCommand(Cmd::kWoCalibrateImage, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetRfFrequency(double freq_mhz) {
  if (!std::isfinite(freq_mhz) || (freq_mhz < 150.0) ||
      (freq_mhz > 960.0)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range\n");
    return false;
  }

  const uint32_t buffer_freq = static_cast<uint32_t>(
      (std::round)((freq_mhz *
                       static_cast<double>(static_cast<uint32_t>(1) << 25)) /
                   32.0));

  uint8_t buffer[] = {
      static_cast<uint8_t>(buffer_freq >> 24),
      static_cast<uint8_t>(buffer_freq >> 16),
      static_cast<uint8_t>(buffer_freq >> 8),
      static_cast<uint8_t>(buffer_freq),
  };

  if (!WriteCommand(Cmd::kWoSetRfFrequency, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetFrequency(double freq_mhz) {
  if (!std::isfinite(freq_mhz)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (freq_mhz < 150.0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    freq_mhz = 150.0;
  } else if (freq_mhz > 960.0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    freq_mhz = 960.0;
  }

  if ((freq_mhz >= image_calibration_start_mhz_) &&
      (freq_mhz <= image_calibration_end_mhz_)) {
    if (!SetRfFrequency(freq_mhz)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "SetRfFrequency failed\n");
      return false;
    }
    param_.freq_mhz = freq_mhz;
    return true;
  }

  bool calibrated = false;
  if ((freq_mhz >= 902.0) && (freq_mhz <= 928.0)) {
    if (!CalibrateImage(ImgCalFreq::kFreq902_928Mhz)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "CalibrateImage failed\n");
      return false;
    }
    image_calibration_start_mhz_ = 902;
    image_calibration_end_mhz_ = 928;
    calibrated = true;
  } else if ((freq_mhz >= 863.0) && (freq_mhz <= 870.0)) {
    if (!CalibrateImage(ImgCalFreq::kFreq863_870Mhz)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "CalibrateImage failed\n");
      return false;
    }
    image_calibration_start_mhz_ = 863;
    image_calibration_end_mhz_ = 870;
    calibrated = true;
  } else if ((freq_mhz >= 779.0) && (freq_mhz <= 787.0)) {
    if (!CalibrateImage(ImgCalFreq::kFreq779_787Mhz)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "CalibrateImage failed\n");
      return false;
    }
    image_calibration_start_mhz_ = 779;
    image_calibration_end_mhz_ = 787;
    calibrated = true;
  } else if ((freq_mhz >= 470.0) && (freq_mhz <= 510.0)) {
    if (!CalibrateImage(ImgCalFreq::kFreq470_510Mhz)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "CalibrateImage failed\n");
      return false;
    }
    image_calibration_start_mhz_ = 470;
    image_calibration_end_mhz_ = 510;
    calibrated = true;
  } else if ((freq_mhz >= 430.0) && (freq_mhz <= 440.0)) {
    if (!CalibrateImage(ImgCalFreq::kFreq430_440Mhz)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "CalibrateImage failed\n");
      return false;
    }
    image_calibration_start_mhz_ = 430;
    image_calibration_end_mhz_ = 440;
    calibrated = true;
  }

  if (!calibrated) {
    const uint16_t start_freq_mhz = static_cast<uint16_t>(std::floor(freq_mhz));
    const uint16_t end_freq_mhz = static_cast<uint16_t>(std::ceil(freq_mhz));
    if (!CalibrateImage(start_freq_mhz, end_freq_mhz)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "CalibrateImage failed\n");
      return false;
    }
    image_calibration_start_mhz_ = start_freq_mhz;
    image_calibration_end_mhz_ = end_freq_mhz;
  }

  // 设置射频频率模式的频率
  if (!SetRfFrequency(freq_mhz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRfFrequency failed\n");
    return false;
  }
  param_.freq_mhz = freq_mhz;

  return true;
}

bool Sx126x::SetFs() {
  if (!WriteCommand(Cmd::kWoSetFs)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetRxDutyCycle(uint32_t rx_time_us, uint32_t sleep_time_us) {
  const uint32_t rx_time = MicrosecondsToRtcStep(rx_time_us);
  const uint32_t sleep_time = MicrosecondsToRtcStep(sleep_time_us);
  uint8_t buffer[] = {
      static_cast<uint8_t>(rx_time >> 16),
      static_cast<uint8_t>(rx_time >> 8),
      static_cast<uint8_t>(rx_time),
      static_cast<uint8_t>(sleep_time >> 16),
      static_cast<uint8_t>(sleep_time >> 8),
      static_cast<uint8_t>(sleep_time),
  };

  if (!WriteCommand(Cmd::kWoSetRxDutyCycle, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::StopTimerOnPreamble(bool enable) {
  const uint8_t buffer = enable ? 1 : 0;
  if (!WriteCommand(Cmd::kWoStopTimerOnPreamble, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetCad() {
  if (!WriteCommand(Cmd::kWoSetCad)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetTxInfinitePreamble() {
  if (!WriteCommand(Cmd::kWoSetTxInfinitePreamble)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetLoraSymbolTimeout(uint8_t symbol_count) {
  if (symbol_count > 248) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    symbol_count = 248;
  }

  uint8_t exp = 0;
  uint8_t mant = static_cast<uint8_t>((symbol_count + 1) >> 1);
  while (mant > 31) {
    mant = static_cast<uint8_t>((mant + 3) >> 2);
    ++exp;
  }

  const uint8_t command_value = static_cast<uint8_t>(mant << ((2 * exp) + 1));
  if (!WriteCommand(Cmd::kWoSetLoraSymbolNumTimeout, &command_value, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  if (symbol_count > 0) {
    const uint8_t register_value = static_cast<uint8_t>(exp + (mant << 3));
    if (!WriteRegister(static_cast<uint16_t>(Reg::kRwLoraSymbolTimeout),
            &register_value, 1)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
      return false;
    }
  }

  return true;
}

bool Sx126x::Configure(const LoraConfig& config) {
  if (!initialized_ || sleeping_ || !ValidateConfig(config)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid state/config\n");
    return false;
  }
  configured_ = false;

  // 切换到STDBY_RC模式
  if (!SetStandby(StdbyConfig::kStdbyRc)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetStandby failed\n");
    return false;
  }

  // 设置包类型
  if (!SetPacketType(PacketType::kLora)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetPacketType failed\n");
    return false;
  }

  if (!SetBufferBaseAddress(0, 0)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetBufferBaseAddress failed\n");
    return false;
  }

  if (!StopTimerOnPreamble(config.stop_timer_on_preamble)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "StopTimerOnPreamble failed\n");
    return false;
  }
  if (!SetLoraSymbolTimeout(config.symbol_timeout)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetLoraSymbolTimeout failed\n");
    return false;
  }

  if (config.configure_cad &&
      !SetCadParams(config.cad_symbol_num, config.cad_det_peak,
          config.cad_det_min, config.cad_exit_mode,
          config.cad_timeout_us)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetCadParams failed\n");
    return false;
  }

  // 根据实际符号时间自动配置LDRO，并恢复离开GFSK时的修正寄存器
  const Ldro ldro = GetLoraLowDataRateOptimize(
      config.spreading_factor, config.bandwidth);
  if (!ResetGfskLowRateWorkaround()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ResetGfskLowRateWorkaround failed\n");
    return false;
  }
  if (!SetLoraModulationParams(config.spreading_factor, config.bandwidth,
          config.coding_rate, ldro)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetLoraModulationParams failed\n");
    return false;
  }

  // 设置同步字
  if (!SetLoraSyncWord(config.sync_word)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetLoraSyncWord failed\n");
    return false;
  }

  // 设置包的参数
  if (!SetLoraPacketParams(config.preamble_length, config.header_type,
          config.payload_length, config.crc_type, config.invert_iq)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetLoraPacketParams failed\n");
    return false;
  }

  // 设置电流限制
  if (!SetCurrentLimit(config.current_limit)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetCurrentLimit failed\n");
    return false;
  }

  // 设置频率
  if (!SetFrequency(config.frequency_mhz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetFrequency failed\n");
    return false;
  }

  // 设置功率
  if (!SetOutputPower(config.power, config.ramp_time)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetOutputPower failed\n");
    return false;
  }

  if (!SetRxBoosted(config.rx_boosted)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRxBoosted failed\n");
    return false;
  }

  if (!ResetStats()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ResetStats failed\n");
    return false;
  }

  if (!ClearIrqFlag(IrqMaskFlag::kAll)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ClearIrqFlag failed\n");
    return false;
  }

  lora_config_ = config;
  configured_ = true;
  return true;
}

bool Sx126x::SetRx(uint32_t time_out_us) {
  const uint32_t timeout = TimeoutMicrosecondsToRtcStep(time_out_us);

  uint8_t buffer[] = {
      static_cast<uint8_t>(timeout >> 16),
      static_cast<uint8_t>(timeout >> 8),
      static_cast<uint8_t>(timeout),
  };

  if (!WriteCommand(Cmd::kWoSetRx, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::StartLora(ChipMode chip_mode, uint32_t timeout_us,
    FallbackMode fallback_mode, uint16_t preamble_length) {
  // 从RX或TX模式退出返回的模式设定
  if (!SetRxTxFallbackMode(fallback_mode)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetRxTxFallbackMode failed\n");
    return false;
  }

  // 设置包的参数
  if (!SetLoraPacketParams(preamble_length, param_.lora.header_type,
          param_.lora.payload_length, param_.lora.crc_type,
          param_.lora.invert_iq)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetLoraPacketParams failed\n");
    return false;
  }

  switch (chip_mode) {
    case ChipMode::kRx:
      // 设置为接收模式
      if (!SetRx(timeout_us)) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRx failed\n");
        return false;
      }
      break;
    case ChipMode::kTx:
      // 设置为发送模式
      if (!SetTx(timeout_us)) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetTx failed\n");
        return false;
      }
      break;

    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
      return false;
  }

  return true;
}

bool Sx126x::GetIrqFlag(uint16_t& irq_flags) {
  irq_flags = 0;
  uint8_t buffer[2] = {0};

  if (!ReadCommand(Cmd::kRoGetIrqStatus, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  irq_flags = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
  return true;
}

bool Sx126x::GetRxBufferStatus(RxBufferStatus& status) {
  status = RxBufferStatus{};
  uint8_t buffer[2] = {0};

  if (!ReadCommand(Cmd::kRoGetRxBufferStatus, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  status.payload_length = buffer[0];
  status.start_pointer = buffer[1];

  return true;
}

bool Sx126x::GetRxBufferLength(uint8_t& length) {
  length = 0;
  RxBufferStatus status;

  if (!GetRxBufferStatus(status)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "GetRxBufferStatus failed\n");
    return false;
  }

  length = status.payload_length;
  return true;
}

bool Sx126x::ReadBuffer(uint8_t* data, uint8_t length, uint8_t offset) {
  // 设置基地址
  if (!ReadBufferRaw(offset, data, length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadBufferRaw failed\n");
    return false;
  }

  return true;
}

bool Sx126x::GetReceiveStatus(ReceiveStatus& status) {
  status = ReceiveStatus{};
  if (!initialized_ || sleeping_ || !configured_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid state\n");
    return false;
  }
  status.packet_type = param_.packet_type;
  uint16_t irq_flag = 0;
  if (!GetIrqFlag(irq_flag)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GetIrqFlag failed\n");
    return false;
  }
  status.irq_flags = irq_flag;
  status.irq_status = ParseIrqStatus(irq_flag);

  status.done = status.irq_status.all_flag.rx_done;
  if (status.done && !StopRxTimeoutTimer()) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "StopRxTimeoutTimer failed\n");
    return false;
  }

  status.timeout = status.irq_status.all_flag.tx_rx_timeout && !status.done;
  status.crc_error = status.irq_status.all_flag.crc_error;
  status.header_error = (status.packet_type == PacketType::kLora) &&
                        status.irq_status.lora_reg_flag.header_error;
  status.packet_received = (status.packet_type == PacketType::kLora) &&
                           status.done && !status.header_error;

  if ((status.packet_type == PacketType::kGfsk) && status.done) {
    if (!GetGfskPacketStatus(status.gfsk_packet_status_raw)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "GetGfskPacketStatus failed\n");
      return false;
    }
    status.gfsk_packet_status =
        ParseGfskPacketStatus(status.gfsk_packet_status_raw);

    status.packet_received = status.gfsk_packet_status.packet_receive_done_flag;
    status.abort_error = status.gfsk_packet_status.abort_error_flag;
    status.length_error = status.gfsk_packet_status.length_error_flag;
    status.crc_error =
        status.crc_error || status.gfsk_packet_status.crc_error_flag;
    status.address_error = status.gfsk_packet_status.address_error_flag;
  }

  status.error = status.timeout || status.crc_error || status.header_error ||
                 status.abort_error || status.length_error ||
                 status.address_error ||
                 (status.done && !status.packet_received);

  if (status.done && !status.error) {
    if (!GetRxBufferStatus(status.rx_buffer_status)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "GetRxBufferStatus failed\n");
      return false;
    }
    status.payload_available = status.rx_buffer_status.payload_length > 0;
    status.error = !status.payload_available;
  }

  status.valid = true;
  return true;
}

bool Sx126x::ReadReceivedPacket(uint8_t* data, size_t capacity,
    size_t& received_length, ReceiveStatus* status) {
  received_length = 0;
  if (!initialized_ || sleeping_ || !configured_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid state\n");
    return false;
  }
  if ((data == nullptr) && (capacity > 0)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  ReceiveStatus receive_status;
  if (!GetReceiveStatus(receive_status)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "GetReceiveStatus failed\n");
    if (status != nullptr) {
      *status = receive_status;
    }
    return false;
  }
  if (status != nullptr) {
    *status = receive_status;
  }

  if (receive_status.timeout) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Receive timeout\n");
    ClearIrqFlag(receive_status.irq_flags);
    return false;
  }

  if (receive_status.crc_error) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Receive crc error\n");
    ClearIrqFlag(receive_status.irq_flags);
    return false;
  }

  if (receive_status.header_error) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Receive lora header error\n");
    ClearIrqFlag(receive_status.irq_flags);
    return false;
  }

  if (!receive_status.done) {
    if (receive_status.irq_flags != 0) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "Receive pending (irq flags: %#X)\n", receive_status.irq_flags);
    }
    return false;
  }

  if (receive_status.abort_error || receive_status.length_error ||
      receive_status.address_error || !receive_status.packet_received) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Receive packet status error\n");
    ClearIrqFlag(receive_status.irq_flags);
    return false;
  }

  if (!receive_status.payload_available) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Receive empty payload\n");
    ClearIrqFlag(receive_status.irq_flags);
    return false;
  }

  received_length = receive_status.rx_buffer_status.payload_length;
  if ((data == nullptr) || (capacity < received_length)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Receive buffer too small (need: %u)\n",
        static_cast<unsigned int>(received_length));
    return false;
  }

  if (!ReadBuffer(data, static_cast<uint8_t>(received_length),
          receive_status.rx_buffer_status.start_pointer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadBuffer failed\n");
    received_length = 0;
    return false;
  }

  if (!ClearIrqFlag(receive_status.irq_flags)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ClearIrqFlag failed\n");
    return false;
  }
  return true;
}

bool Sx126x::GetLoraPacketMetrics(PacketMetrics& metrics) {
  metrics = PacketMetrics{};
  uint8_t buffer[3] = {0};

  if (!ReadCommand(Cmd::kRoGetPacketStatus, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  metrics.lora.rssi_average = -1.0f * static_cast<float>(buffer[0]) / 2.0f;
  metrics.lora.snr = static_cast<float>(static_cast<int8_t>(buffer[1])) / 4.0f;
  metrics.lora.rssi_instantaneous =
      -1.0f * static_cast<float>(buffer[2]) / 2.0f;

  return true;
}

bool Sx126x::GetRssiInst(float& rssi_dbm) {
  rssi_dbm = 0.0f;
  uint8_t buffer = 0;

  if (!ReadCommand(Cmd::kRoGetRssiInst, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  rssi_dbm = -1.0f * static_cast<float>(buffer) / 2.0f;

  return true;
}

bool Sx126x::GetPacketStats(PacketStats& stats) {
  stats = PacketStats{};
  uint8_t buffer[6] = {0};

  if (!ReadCommand(Cmd::kRoGetStats, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  stats.packet_received = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
  stats.crc_error = (static_cast<uint16_t>(buffer[2]) << 8) | buffer[3];
  if (param_.packet_type == PacketType::kLora) {
    stats.header_error = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];
    stats.length_error = 0;
  } else {
    stats.header_error = 0;
    stats.length_error = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];
  }

  return true;
}

bool Sx126x::ResetStats() {
  const uint8_t buffer[6] = {0};
  if (!WriteCommand(Cmd::kWoResetStats, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::GetDeviceErrors(uint16_t& errors) {
  errors = 0;
  uint8_t buffer[2] = {0};

  if (!ReadCommand(Cmd::kRoGetDeviceErrors, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  errors = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
  return true;
}

bool Sx126x::ClearDeviceErrors() {
  const uint8_t buffer[2] = {0};
  if (!WriteCommand(Cmd::kWoClearDeviceErrors, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetRxBoosted(bool enable) {
  const uint8_t buffer = enable ? 0x96 : 0x94;
  if (!WriteRegister(static_cast<uint16_t>(Reg::kRwRxGain), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  param_.rx_boosted = enable;

  return true;
}

bool Sx126x::FixBw500KhzSensitivity(bool enable) {
  uint8_t buffer = 0;

  if (!ReadRegister(static_cast<uint16_t>(Reg::kRwTxModulation), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  if (enable) {
    buffer &= 0xFB;
  } else {
    buffer |= 0x04;
  }

  if (!WriteRegister(static_cast<uint16_t>(Reg::kRwTxModulation), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetTx(uint32_t time_out_us) {
  const uint32_t timeout = TimeoutMicrosecondsToRtcStep(time_out_us);

  uint8_t buffer[] = {
      static_cast<uint8_t>(timeout >> 16),
      static_cast<uint8_t>(timeout >> 8),
      static_cast<uint8_t>(timeout),
  };

  if (!WriteCommand(Cmd::kWoSetTx, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::WriteBuffer(const uint8_t* data, uint8_t length, uint8_t offset) {
  // 设置基地址
  if (!SetBufferBaseAddress(0, 0)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetBufferBaseAddress failed\n");
    return false;
  }

  if (!WriteBufferRaw(offset, data, length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteBufferRaw failed\n");
    return false;
  }

  return true;
}

bool Sx126x::GetSendStatus(SendStatus& status) {
  status = SendStatus{};
  if (!initialized_ || sleeping_ || !configured_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid state\n");
    return false;
  }
  status.packet_type = param_.packet_type;
  uint16_t irq_flag = 0;
  if (!GetIrqFlag(irq_flag)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GetIrqFlag failed\n");
    return false;
  }
  status.irq_flags = irq_flag;
  status.irq_status = ParseIrqStatus(irq_flag);

  status.done = status.irq_status.all_flag.tx_done;
  status.timeout = status.irq_status.all_flag.tx_rx_timeout && !status.done;
  status.error = status.timeout;

  return true;
}

bool Sx126x::StartTransmit(const uint8_t* data, size_t length,
    uint32_t timeout_us, FallbackMode fallback_mode) {
  if (!initialized_ || sleeping_ || !configured_ || (data == nullptr) ||
      (length == 0) || (length > kMaxPayloadSize)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid state/argument\n");
    return false;
  }
  if (!SetRxTxFallbackMode(fallback_mode)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetRxTxFallbackMode failed\n");
    return false;
  }

  const uint8_t payload_length = static_cast<uint8_t>(length);
  switch (param_.packet_type) {
    case PacketType::kGfsk:
      if (param_.gfsk.payload_length != payload_length) {
        // 重新设置长度
        if (!SetGfskPacketParams(param_.gfsk.preamble_length,
                param_.gfsk.preamble_detector, param_.gfsk.sync_word.length * 8,
                param_.gfsk.address_comparison, param_.gfsk.header_type,
                payload_length,
                param_.gfsk.crc.type, param_.gfsk.whitening)) {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "SetGfskPacketParams failed\n");
          return false;
        }
        param_.gfsk.payload_length = payload_length;
      }
      break;
    case PacketType::kLora:
      if (param_.lora.payload_length != payload_length) {
        // 重新设置长度
        if (!SetLoraPacketParams(param_.lora.preamble_length,
                param_.lora.header_type, payload_length,
                param_.lora.crc_type,
                param_.lora.invert_iq)) {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "SetLoraPacketParams failed\n");
          return false;
        }
        param_.lora.payload_length = payload_length;
      }
      break;

    default:
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Unsupported packet type\n");
      return false;
  }

  if (!WriteBuffer(data, payload_length, 0)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteBuffer failed\n");
    return false;
  }

  if (param_.packet_type == PacketType::kLora) {
    if (!FixBw500KhzSensitivity(
            param_.lora.band_width == LoraBw::kBw500000Hz)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "FixBw500KhzSensitivity failed\n");
      return false;
    }
  } else if (param_.packet_type == PacketType::kGfsk) {
    if (!FixBw500KhzSensitivity(false)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "FixBw500KhzSensitivity failed\n");
      return false;
    }
  }

  const uint16_t irq_mask = IrqMask(IrqMaskFlag::kTxDone) |
                            IrqMask(IrqMaskFlag::kTimeout);
  if (!SetIrqGpioMode(irq_mask) ||
      !ClearIrqFlag(IrqMaskFlag::kAll)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "IRQ setup failed\n");
    return false;
  }
  if (!SetTx(timeout_us)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetTx failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetLoraCrcPacketParams(LoraCrcType crc_type) {
  // 设置CRC
  if (!SetLoraPacketParams(param_.lora.preamble_length, param_.lora.header_type,
          param_.lora.payload_length, crc_type, param_.lora.invert_iq)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetLoraPacketParams failed\n");
    return false;
  }
  param_.lora.crc_type = crc_type;

  return true;
}

bool Sx126x::SetGfskModulationParams(
    double br, PulseShape ps, GfskBw bw, double freq_deviation_khz) {
  const uint8_t pulse_shape = static_cast<uint8_t>(ps);
  const bool valid_pulse_shape = (pulse_shape == 0) ||
                                 ((pulse_shape >= 0x08) &&
                                     (pulse_shape <= 0x0B));
  if (!std::isfinite(br) || !std::isfinite(freq_deviation_khz) ||
      !valid_pulse_shape || (GetGfskBandwidthKhz(bw) <= 0.0f)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (br < 0.6) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    br = 0.6;
  } else if (br > 300.0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    br = 300.0;
  }

  if (freq_deviation_khz < 0.6) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    freq_deviation_khz = 0.6;
  } else if (freq_deviation_khz > 200.0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    freq_deviation_khz = 200.0;
  }

  const double required_bandwidth = br + (2.0 * freq_deviation_khz);
  if (((freq_deviation_khz + (br / 2.0)) > 250.0) ||
      (((2.0 * freq_deviation_khz) / br) < 0.5) ||
      (GetGfskBandwidthKhz(bw) < required_bandwidth)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid modulation parameters\n");
    return false;
  }

  // 计算原始比特率值
  uint32_t buffer_br = (32.0 * 1000000.0 * 32.0) / (br * 1000.0);

  // 计算原始频率偏差值
  uint32_t buffer_freq_deviation_khz = static_cast<uint32_t>(
      (std::round)(((freq_deviation_khz * 1000.0) *
                       static_cast<double>(static_cast<uint32_t>(1) << 25)) /
                   (32.0 * 1000000.0)));

  uint8_t buffer[] = {
      static_cast<uint8_t>(buffer_br >> 16),
      static_cast<uint8_t>(buffer_br >> 8),
      static_cast<uint8_t>(buffer_br),
      static_cast<uint8_t>(ps),
      static_cast<uint8_t>(bw),
      static_cast<uint8_t>(buffer_freq_deviation_khz >> 16),
      static_cast<uint8_t>(buffer_freq_deviation_khz >> 8),
      static_cast<uint8_t>(buffer_freq_deviation_khz),
  };

  if (!WriteCommand(Cmd::kWoSetModulationParams, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }
  if (!FixBw500KhzSensitivity(false)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "FixBw500KhzSensitivity failed\n");
    return false;
  }
  param_.gfsk.bit_rate = br;
  param_.gfsk.pulse_shape = ps;
  param_.gfsk.band_width = bw;
  param_.gfsk.freq_deviation_khz = freq_deviation_khz;

  return true;
}

bool Sx126x::SetGfskSyncWord(const uint8_t* sync_word, uint8_t length) {
  if ((length > 8) || ((length > 0) && (sync_word == nullptr))) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer[8] = {0};
  if (length > 0) {
    std::memcpy(buffer, sync_word, length);
  }
  if (!WriteRegister(static_cast<uint16_t>(Reg::kRwSyncWordProgrammingStart),
          buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  param_.gfsk.sync_word.data = {};
  if (length > 0) {
    std::memcpy(param_.gfsk.sync_word.data.data(), sync_word, length);
  }
  param_.gfsk.sync_word.length = length;

  return true;
}

bool Sx126x::SetGfskPacketAddress(
    uint8_t node_address, uint8_t broadcast_address) {
  uint8_t buffer[] = {node_address, broadcast_address};
  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwGfskNodeAddress), buffer, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetGfskPacketParams(uint16_t preamble_length,
    PreambleDetector preamble_detector_length, uint8_t sync_word_length,
    AddrComp addr_comp, GfskHeaderType header_type, uint8_t payload_length,
    GfskCrcType crc_type, Whitening whitening) {
  uint8_t detector_bits = 0;
  switch (preamble_detector_length) {
    case PreambleDetector::kLengthOff:
      break;
    case PreambleDetector::kLength8bit:
      detector_bits = 8;
      break;
    case PreambleDetector::kLength16bit:
      detector_bits = 16;
      break;
    case PreambleDetector::kLength24bit:
      detector_bits = 24;
      break;
    case PreambleDetector::kLength32bit:
      detector_bits = 32;
      break;
    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "Invalid argument\n");
      return false;
  }

  const uint8_t crc = static_cast<uint8_t>(crc_type);
  const bool valid_crc = (crc <= 2) || (crc == 4) || (crc == 6);
  if ((preamble_length == 0) || (sync_word_length > 64) ||
      (detector_bits > preamble_length) ||
      ((detector_bits > 0) && (detector_bits >= sync_word_length)) ||
      (static_cast<uint8_t>(addr_comp) > 2) ||
      (static_cast<uint8_t>(header_type) > 1) || !valid_crc ||
      (static_cast<uint8_t>(whitening) > 1)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer[] = {
      static_cast<uint8_t>(preamble_length >> 8),
      static_cast<uint8_t>(preamble_length),
      static_cast<uint8_t>(preamble_detector_length),
      sync_word_length,
      static_cast<uint8_t>(addr_comp),
      static_cast<uint8_t>(header_type),
      payload_length,
      static_cast<uint8_t>(crc_type),
      static_cast<uint8_t>(whitening),
  };

  if (!WriteCommand(Cmd::kWoSetPacketParams, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }
  param_.gfsk.preamble_length = preamble_length;
  param_.gfsk.preamble_detector = preamble_detector_length;
  param_.gfsk.address_comparison = addr_comp;
  param_.gfsk.header_type = header_type;
  param_.gfsk.payload_length = payload_length;
  param_.gfsk.crc.type = crc_type;
  param_.gfsk.whitening = whitening;

  return true;
}

bool Sx126x::SetGfskCrc(uint16_t initial, uint16_t polynomial) {
  uint8_t crc_seed[] = {
      static_cast<uint8_t>(initial >> 8),
      static_cast<uint8_t>(initial),
  };
  uint8_t crc_polynomial[] = {
      static_cast<uint8_t>(polynomial >> 8),
      static_cast<uint8_t>(polynomial),
  };

  if (!WriteRegister(static_cast<uint16_t>(Reg::kRwCrcValueProgrammingStart),
          crc_seed, sizeof(crc_seed))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  if (!WriteRegister(static_cast<uint16_t>(Reg::kRwCrcPolynomialStart),
          crc_polynomial, sizeof(crc_polynomial))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  param_.gfsk.crc.initial = initial;
  param_.gfsk.crc.polynomial = polynomial;

  return true;
}

bool Sx126x::SetGfskWhiteningSeed(uint16_t seed) {
  if (seed > 0x01FF) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Value out of range\n");
    return false;
  }

  uint8_t msb = 0;
  if (!ReadRegister(
          static_cast<uint16_t>(Reg::kRwWhiteningSeedStart), &msb, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  msb = static_cast<uint8_t>((msb & 0xFE) | ((seed >> 8) & 0x01));
  const uint8_t lsb = static_cast<uint8_t>(seed);
  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwWhiteningSeedStart), &msb, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }
  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwWhiteningSeedStart) + 1, &lsb, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  return true;
}

bool Sx126x::Configure(const GfskConfig& config) {
  if (!initialized_ || sleeping_ || !ValidateConfig(config)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid state/config\n");
    return false;
  }
  configured_ = false;

  const uint8_t sync_word_length = config.sync_word_length;

  // 切换到STDBY_RC模式
  if (!SetStandby(StdbyConfig::kStdbyRc)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetStandby failed\n");
    return false;
  }

  // 设置包类型
  if (!SetPacketType(PacketType::kGfsk)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetPacketType failed\n");
    return false;
  }

  if (!SetBufferBaseAddress(0, 0)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetBufferBaseAddress failed\n");
    return false;
  }
  if (!StopTimerOnPreamble(config.stop_timer_on_preamble)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "StopTimerOnPreamble failed\n");
    return false;
  }

  // 切换调制方式前恢复官方GFSK低速率修正寄存器
  if (!ResetGfskLowRateWorkaround()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ResetGfskLowRateWorkaround failed\n");
    return false;
  }
  if (!SetGfskModulationParams(config.bit_rate_kbps, config.pulse_shape,
          config.bandwidth, config.frequency_deviation_khz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetGfskModulationParams failed\n");
    return false;
  }

  if (!SetGfskSyncWord(config.sync_word.data(), sync_word_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetGfskSyncWord failed\n");
    return false;
  }

  param_.gfsk.preamble_detector = config.preamble_detector;

  if (config.address_comparison != AddrComp::kFilteringDisable) {
    if (!SetGfskPacketAddress(config.node_address, config.broadcast_address)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "SetGfskPacketAddress failed\n");
      return false;
    }
  }

  if (config.whitening != Whitening::kNoEncoding) {
    if (!SetGfskWhiteningSeed(config.whitening_seed)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "SetGfskWhiteningSeed failed\n");
      return false;
    }
  }

  // 设置包的参数
  if (!SetGfskPacketParams(config.preamble_length,
          param_.gfsk.preamble_detector, sync_word_length * 8,
          config.address_comparison, config.header_type,
          config.payload_length, config.crc_type, config.whitening)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }
  param_.gfsk.sync_word.data = config.sync_word;
  param_.gfsk.sync_word.length = sync_word_length;

  if (!ApplyGfskLowRateWorkaround(config)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ApplyGfskLowRateWorkaround failed\n");
    return false;
  }

  param_.gfsk.crc.initial = config.crc_initial;
  param_.gfsk.crc.polynomial = config.crc_polynomial;
  // 设置CRC。官方示例仅在CRC类型不为OFF时写入seed和polynomial寄存器。
  if (config.crc_type != GfskCrcType::kCrcOff) {
    if (!SetGfskCrc(config.crc_initial, config.crc_polynomial)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetGfskCrc failed\n");
      return false;
    }
  }

  // 设置电流限制
  if (!SetCurrentLimit(config.current_limit)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetCurrentLimit failed\n");
    return false;
  }

  // 设置频率
  if (!SetFrequency(config.frequency_mhz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetFrequency failed\n");
    return false;
  }

  // 设置功率
  if (!SetOutputPower(config.power, config.ramp_time)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetOutputPower failed\n");
    return false;
  }

  if (!SetRxBoosted(config.rx_boosted)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRxBoosted failed\n");
    return false;
  }

  if (!ResetStats()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ResetStats failed\n");
    return false;
  }

  if (!ClearIrqFlag(IrqMaskFlag::kAll)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ClearIrqFlag failed\n");
    return false;
  }

  gfsk_config_ = config;
  configured_ = true;
  return true;
}

bool Sx126x::StartReceive(
    uint32_t timeout_us, FallbackMode fallback_mode) {
  if (!initialized_ || sleeping_ || !configured_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid state\n");
    return false;
  }

  uint16_t irq_mask = IrqMask(IrqMaskFlag::kRxDone) |
                      IrqMask(IrqMaskFlag::kTimeout) |
                      IrqMask(IrqMaskFlag::kCrcError);
  if (param_.packet_type == PacketType::kLora) {
    irq_mask |= IrqMask(IrqMaskFlag::kHeaderError);
  }
  if (!SetIrqGpioMode(irq_mask) ||
      !ClearIrqFlag(IrqMaskFlag::kAll)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "IRQ setup failed\n");
    return false;
  }

  switch (param_.packet_type) {
    case PacketType::kLora:
      return StartLora(ChipMode::kRx, timeout_us, fallback_mode,
          param_.lora.preamble_length);
    case PacketType::kGfsk:
      return StartGfsk(ChipMode::kRx, timeout_us, fallback_mode,
          param_.gfsk.preamble_length);
    default:
      LogMessage(
          LogLevel::kWarning, __FILE__, __LINE__, "Unsupported packet type\n");
      return false;
  }
}

bool Sx126x::StartGfsk(ChipMode chip_mode, uint32_t timeout_us,
    FallbackMode fallback_mode, uint16_t preamble_length) {
  // 从RX或TX模式退出返回的模式设定
  if (!SetRxTxFallbackMode(fallback_mode)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetRxTxFallbackMode failed\n");
    return false;
  }

  PreambleDetector preamble_detector = param_.gfsk.preamble_detector;
  const PreambleDetector max_preamble_detector =
      GetGfskMaxPreambleDetector(
          preamble_length, param_.gfsk.sync_word.length);
  if (static_cast<uint8_t>(preamble_detector) >
      static_cast<uint8_t>(max_preamble_detector)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    preamble_detector = max_preamble_detector;
  }

  // 设置包的参数
  if (!SetGfskPacketParams(preamble_length, preamble_detector,
          param_.gfsk.sync_word.length * 8, param_.gfsk.address_comparison,
          param_.gfsk.header_type, param_.gfsk.payload_length,
          param_.gfsk.crc.type, param_.gfsk.whitening)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }

  switch (chip_mode) {
    case ChipMode::kRx:
      // 设置为接收模式
      if (!SetRx(timeout_us)) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRx failed\n");
        return false;
      }
      break;
    case ChipMode::kTx:
      // 设置为发送模式
      if (!SetTx(timeout_us)) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetTx failed\n");
        return false;
      }
      break;

    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
      return false;
  }

  return true;
}

bool Sx126x::GetGfskPacketStatus(uint32_t& status) {
  status = 0;
  uint8_t buffer[3] = {0};

  if (!ReadCommand(Cmd::kRoGetPacketStatus, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadCommand failed\n");
    return false;
  }

  status = (static_cast<uint32_t>(buffer[0]) << 16) |
           (static_cast<uint32_t>(buffer[1]) << 8) |
           static_cast<uint32_t>(buffer[2]);
  return true;
}

Sx126x::GfskPacketStatus Sx126x::ParseGfskPacketStatus(
    uint32_t parse_status) {
  GfskPacketStatus status;
  parse_status >>= 16;

  status.packet_send_done_flag = parse_status & 0B00000001;
  status.packet_receive_done_flag = (parse_status & 0B00000010) >> 1;
  status.abort_error_flag = (parse_status & 0B00000100) >> 2;
  status.length_error_flag = (parse_status & 0B00001000) >> 3;
  status.crc_error_flag = (parse_status & 0B00010000) >> 4;
  status.address_error_flag = (parse_status & 0B00100000) >> 5;

  return status;
}

Sx126x::PacketMetrics Sx126x::ParseGfskPacketMetrics(
    uint32_t parse_metrics) {
  PacketMetrics metrics;
  const uint8_t buffer[2] = {
      static_cast<uint8_t>(
          (parse_metrics & 0B00000000000000001111111111111111) >> 8),
      static_cast<uint8_t>(parse_metrics),
  };

  metrics.gfsk.rssi_sync = -1.0 * static_cast<float>(buffer[0]) / 2.0;
  metrics.gfsk.rssi_average = -1.0 * static_cast<float>(buffer[1]) / 2.0;

  return metrics;
}

bool Sx126x::SetGfskSyncWordPacketParams(
    const uint8_t* sync_word, uint8_t sync_word_length) {
  if ((sync_word_length > 8) ||
      ((sync_word_length > 0) && (sync_word == nullptr))) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  // 设置同步字（有效同步字长度会在GFSK包参数中同步更新）
  if (!SetGfskSyncWord(sync_word, sync_word_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetGfskSyncWord failed\n");
    return false;
  }

  PreambleDetector preamble_detector = param_.gfsk.preamble_detector;
  const PreambleDetector max_preamble_detector =
      GetGfskMaxPreambleDetector(
          param_.gfsk.preamble_length, sync_word_length);
  if (static_cast<uint8_t>(preamble_detector) >
      static_cast<uint8_t>(max_preamble_detector)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    preamble_detector = max_preamble_detector;
  }

  // 设置包的参数
  if (!SetGfskPacketParams(param_.gfsk.preamble_length, preamble_detector,
          sync_word_length * 8, param_.gfsk.address_comparison,
          param_.gfsk.header_type, param_.gfsk.payload_length,
          param_.gfsk.crc.type, param_.gfsk.whitening)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }
  param_.gfsk.sync_word.data = {};
  if (sync_word_length > 0) {
    std::memcpy(
        param_.gfsk.sync_word.data.data(), sync_word, sync_word_length);
  }
  param_.gfsk.sync_word.length = sync_word_length;

  return true;
}

bool Sx126x::SetGfskCrcPacketParams(
    GfskCrcType crc_type, uint16_t crc_initial, uint16_t crc_polynomial) {
  // 设置包的参数
  if (!SetGfskPacketParams(param_.gfsk.preamble_length,
          param_.gfsk.preamble_detector, param_.gfsk.sync_word.length * 8,
          param_.gfsk.address_comparison, param_.gfsk.header_type,
          param_.gfsk.payload_length, crc_type, param_.gfsk.whitening)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }

  // CRC类型会在GFSK包参数中同步更新；CRC关闭时不写seed和polynomial寄存器。
  if (crc_type != GfskCrcType::kCrcOff) {
    if (!SetGfskCrc(crc_initial, crc_polynomial)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetGfskCrc failed\n");
      return false;
    }
  }

  param_.gfsk.crc.type = crc_type;
  param_.gfsk.crc.initial = crc_initial;
  param_.gfsk.crc.polynomial = crc_polynomial;

  return true;
}

bool Sx126x::SetIrqGpioMode(IrqMaskFlag dio1_mode, IrqMaskFlag dio2_mode,
    IrqMaskFlag dio3_mode, uint16_t irq_mask) {
  return SetIrqGpioMode(static_cast<uint16_t>(dio1_mode),
      static_cast<uint16_t>(dio2_mode), static_cast<uint16_t>(dio3_mode),
      irq_mask);
}

bool Sx126x::SetIrqGpioMode(uint16_t dio1_mask, uint16_t dio2_mask,
    uint16_t dio3_mask, uint16_t irq_mask) {
  // irq_mask 为0时，仅启用映射到DIO的IRQ源。
  if (irq_mask == 0) {
    irq_mask = dio1_mask | dio2_mask | dio3_mask;
  }

  if (!SetDioIrqParams(irq_mask, dio1_mask, dio2_mask, dio3_mask)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetDioIrqParams failed\n");
    return false;
  }

  return true;
}

bool Sx126x::ClearBuffer() {
  std::unique_ptr<uint8_t[]> buffer =
      std::make_unique<uint8_t[]>(kMaxPayloadSize);

  std::memset(buffer.get(), 0, kMaxPayloadSize);

  if (!WriteBuffer(buffer.get(), kMaxPayloadSize, 0)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteBuffer failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetTxContinuousWave() {
  if (!WriteCommand(Cmd::kWoSetTxContinuousWave)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetSleep(SleepMode mode) {
  if (!initialized_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid state\n");
    return false;
  }
  const uint8_t mode_value = static_cast<uint8_t>(mode);
  if ((mode_value != 0x00) && (mode_value != 0x01) &&
      (mode_value != 0x04) && (mode_value != 0x05)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if (sleeping_) {
    return true;
  }
  if (!SetStandby(StdbyConfig::kStdbyRc)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetStandby failed\n");
    return false;
  }

  const uint8_t buffer = mode_value;
  if (!WriteCommand(Cmd::kWoSetSleep, &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteCommand failed\n");
    return false;
  }

  // 等待Sx126x进入睡眠
  DelayMs(1);

  sleep_mode_ = mode;
  sleeping_ = true;
  if ((static_cast<uint8_t>(mode) & 0x04) == 0) {
    configured_ = false;
  }
  return true;
}

bool Sx126x::Wakeup() {
  if (!initialized_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid state\n");
    return false;
  }
  if (!sleeping_) {
    return true;
  }

  uint8_t status = 0;
  // 直接读取任意命令触发cs引脚变化唤醒设备
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetStatus), &status)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  DelayMs(1);
  if (!CheckBusy()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "CheckBusy failed\n");
    return false;
  }

  const bool cold_start =
      (static_cast<uint8_t>(sleep_mode_) & 0x04) == 0;
  if (cold_start) {
    gfsk_low_rate_workaround_active_ = false;
    image_calibration_start_mhz_ = 902;
    image_calibration_end_mhz_ = 928;
    if (!ApplyHardwareConfig(hardware_config_.enable_dio3_tcxo)) {
      return false;
    }
  } else {
    if (hardware_config_.enable_retention_list && !InitRetentionList()) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "InitRetentionList failed\n");
      return false;
    }
    if (!ApplyWorkaroundsAfterWakeup()) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "ApplyWorkaroundsAfterWakeup failed\n");
      return false;
    }
  }

  sleeping_ = false;
  return true;
}

bool Sx126x::WriteCommand(Cmd command, const uint8_t* data, size_t length) {
  if ((length > 0) && (data == nullptr)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if ((length + 1) > kMaxSpiFrameSize) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::array<uint8_t, kMaxSpiFrameSize> buffer = {};
  buffer[0] = static_cast<uint8_t>(command);
  if (length > 0) {
    std::memcpy(&buffer[1], data, length);
  }

  if (!CheckBusy()) {
    return false;
  }
  if (!bus_->Write(buffer.data(), length + 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::ReadCommand(Cmd command, uint8_t* data, size_t length) {
  if ((length > 0) && (data == nullptr)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if ((length + 2) > kMaxSpiFrameSize) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::array<uint8_t, kMaxSpiFrameSize> write_buffer = {};
  std::array<uint8_t, kMaxSpiFrameSize> read_buffer = {};
  write_buffer[0] = static_cast<uint8_t>(command);
  write_buffer[1] = kNop;

  if (!CheckBusy()) {
    return false;
  }
  if (!bus_->WriteRead(write_buffer.data(), read_buffer.data(), length + 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  if (length > 0) {
    std::memcpy(data, &read_buffer[2], length);
  }

  return true;
}

bool Sx126x::WriteRegister(
    uint16_t address, const uint8_t* data, size_t length) {
  if ((length > 0) && (data == nullptr)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if ((length + 3) > kMaxSpiFrameSize) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::array<uint8_t, kMaxSpiFrameSize> buffer = {};
  buffer[0] = static_cast<uint8_t>(Cmd::kWoWriteRegister);
  buffer[1] = static_cast<uint8_t>(address >> 8);
  buffer[2] = static_cast<uint8_t>(address);
  if (length > 0) {
    std::memcpy(&buffer[3], data, length);
  }

  if (!CheckBusy()) {
    return false;
  }
  if (!bus_->Write(buffer.data(), length + 3)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::ReadRegister(uint16_t address, uint8_t* data, size_t length) {
  if ((length > 0) && (data == nullptr)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if ((length + 4) > kMaxSpiFrameSize) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::array<uint8_t, kMaxSpiFrameSize> write_buffer = {};
  std::array<uint8_t, kMaxSpiFrameSize> read_buffer = {};
  write_buffer[0] = static_cast<uint8_t>(Cmd::kWoReadRegister);
  write_buffer[1] = static_cast<uint8_t>(address >> 8);
  write_buffer[2] = static_cast<uint8_t>(address);
  write_buffer[3] = kNop;

  if (!CheckBusy()) {
    return false;
  }
  if (!bus_->WriteRead(write_buffer.data(), read_buffer.data(), length + 4)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  if (length > 0) {
    std::memcpy(data, &read_buffer[4], length);
  }

  return true;
}

bool Sx126x::WriteBufferRaw(
    uint8_t offset, const uint8_t* data, size_t length) {
  if ((length > 0) && (data == nullptr)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if ((length + 2) > kMaxSpiFrameSize) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::array<uint8_t, kMaxSpiFrameSize> buffer = {};
  buffer[0] = static_cast<uint8_t>(Cmd::kWoWriteBuffer);
  buffer[1] = offset;
  if (length > 0) {
    std::memcpy(&buffer[2], data, length);
  }

  if (!CheckBusy()) {
    return false;
  }
  if (!bus_->Write(buffer.data(), length + 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::ReadBufferRaw(uint8_t offset, uint8_t* data, size_t length) {
  if ((length > 0) && (data == nullptr)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  if ((length + 3) > kMaxSpiFrameSize) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::array<uint8_t, kMaxSpiFrameSize> write_buffer = {};
  std::array<uint8_t, kMaxSpiFrameSize> read_buffer = {};
  write_buffer[0] = static_cast<uint8_t>(Cmd::kRoReadBuffer);
  write_buffer[1] = offset;
  write_buffer[2] = kNop;

  if (!CheckBusy()) {
    return false;
  }
  if (!bus_->WriteRead(write_buffer.data(), read_buffer.data(), length + 3)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  if (length > 0) {
    std::memcpy(data, &read_buffer[3], length);
  }

  return true;
}

uint32_t Sx126x::MicrosecondsToRtcStep(uint32_t time_us) const {
  constexpr uint32_t kRtcStepNumerator = 64;
  constexpr uint32_t kRtcStepDenominator = 1000;
  const uint64_t steps = (static_cast<uint64_t>(time_us) * kRtcStepNumerator +
                             kRtcStepDenominator - 1) /
                         kRtcStepDenominator;
  return static_cast<uint32_t>(std::min<uint64_t>(steps, kTimeoutContinuous));
}

uint32_t Sx126x::TimeoutMicrosecondsToRtcStep(uint32_t time_us) const {
  if ((time_us == kTimeoutDisabled) || (time_us == kTimeoutContinuous)) {
    return time_us;
  }
  return MicrosecondsToRtcStep(time_us);
}

float Sx126x::GetLoraBandwidthHz(LoraBw bw) const {
  switch (bw) {
    case LoraBw::kBw7810Hz:
      return 7810.0f;
    case LoraBw::kBw10420Hz:
      return 10420.0f;
    case LoraBw::kBw15630Hz:
      return 15630.0f;
    case LoraBw::kBw20830Hz:
      return 20830.0f;
    case LoraBw::kBw31250Hz:
      return 31250.0f;
    case LoraBw::kBw41670Hz:
      return 41670.0f;
    case LoraBw::kBw62500Hz:
      return 62500.0f;
    case LoraBw::kBw125000Hz:
      return 125000.0f;
    case LoraBw::kBw250000Hz:
      return 250000.0f;
    case LoraBw::kBw500000Hz:
      return 500000.0f;
    default:
      return 0.0f;
  }
}

Sx126x::Ldro Sx126x::GetLoraLowDataRateOptimize(Sf sf, LoraBw bw) const {
  const float bandwidth_hz = GetLoraBandwidthHz(bw);
  const uint8_t spreading_factor = static_cast<uint8_t>(sf);
  if ((bandwidth_hz <= 0.0f) || (spreading_factor < 5) ||
      (spreading_factor > 12)) {
    return Ldro::kLdroOff;
  }

  const double symbol_time_ms =
      (static_cast<double>(uint32_t{1} << spreading_factor) * 1000.0) /
      bandwidth_hz;
  return (symbol_time_ms >= 16.384) ? Ldro::kLdroOn : Ldro::kLdroOff;
}

Sx126x::PreambleDetector Sx126x::GetGfskMaxPreambleDetector(
    uint16_t preamble_length, uint8_t sync_word_length) const {
  PreambleDetector sync_word_limit = PreambleDetector::kLength32bit;
  if (sync_word_length <= 1) {
    sync_word_limit = PreambleDetector::kLengthOff;
  } else if (sync_word_length == 2) {
    sync_word_limit = PreambleDetector::kLength8bit;
  } else if (sync_word_length == 3) {
    sync_word_limit = PreambleDetector::kLength16bit;
  } else if (sync_word_length == 4) {
    sync_word_limit = PreambleDetector::kLength24bit;
  }

  PreambleDetector preamble_limit = PreambleDetector::kLengthOff;
  if (preamble_length >= 32) {
    preamble_limit = PreambleDetector::kLength32bit;
  } else if (preamble_length >= 24) {
    preamble_limit = PreambleDetector::kLength24bit;
  } else if (preamble_length >= 16) {
    preamble_limit = PreambleDetector::kLength16bit;
  } else if (preamble_length >= 8) {
    preamble_limit = PreambleDetector::kLength8bit;
  }

  return (static_cast<uint8_t>(preamble_limit) <
             static_cast<uint8_t>(sync_word_limit))
      ? preamble_limit
      : sync_word_limit;
}

bool Sx126x::ValidateHardwareConfig() const {
  const uint8_t tcxo =
      static_cast<uint8_t>(hardware_config_.tcxo_voltage);
  const uint8_t regulator =
      static_cast<uint8_t>(hardware_config_.regulator_mode);
  const uint8_t dio2 = static_cast<uint8_t>(hardware_config_.dio2_mode);
  const bool has_busy_source =
      (busy_ >= 0) || (busy_wait_callback_ != nullptr);
  const bool valid_optional_pins =
      ((busy_ == kDefaultValue) || (busy_ >= 0)) &&
      ((rst_ == kDefaultValue) || (rst_ >= 0)) &&
      ((cs_ == kDefaultValue) || (cs_ >= 0));
  const bool valid_chip = (chip_type_ == ChipType::kSx1261) ||
                          (chip_type_ == ChipType::kSx1262);

  return valid_chip && has_busy_source && valid_optional_pins && (tcxo <= 7) &&
         (regulator <= 1) && (dio2 <= 1) &&
         (!hardware_config_.enable_dio3_tcxo ||
             ((hardware_config_.tcxo_startup_time_us > 0) &&
                 (hardware_config_.tcxo_startup_time_us <= 262143984U)));
}

bool Sx126x::ValidateConfig(const LoraConfig& config) const {
  const int8_t min_power =
      (chip_type_ == ChipType::kSx1261) ? -17 : -9;
  const int8_t max_power =
      (chip_type_ == ChipType::kSx1261) ? 14 : 22;
  const uint8_t sf = static_cast<uint8_t>(config.spreading_factor);
  const uint8_t cr = static_cast<uint8_t>(config.coding_rate);
  const uint8_t header = static_cast<uint8_t>(config.header_type);
  const uint8_t crc = static_cast<uint8_t>(config.crc_type);
  const uint8_t iq = static_cast<uint8_t>(config.invert_iq);
  const uint8_t ramp = static_cast<uint8_t>(config.ramp_time);
  const uint8_t cad_symbols =
      static_cast<uint8_t>(config.cad_symbol_num);
  const uint8_t cad_exit = static_cast<uint8_t>(config.cad_exit_mode);
  const bool valid_cad_exit =
      (cad_exit == 0) || (cad_exit == 1) || (cad_exit == 0x10);

  return std::isfinite(config.frequency_mhz) &&
         (config.frequency_mhz >= 150.0) &&
         (config.frequency_mhz <= 960.0) &&
         std::isfinite(config.current_limit) &&
         (config.current_limit >= 0.0f) &&
         (config.current_limit <= 157.5f) &&
         (config.power >= min_power) && (config.power <= max_power) &&
         (sf >= 5) && (sf <= 12) &&
         (GetLoraBandwidthHz(config.bandwidth) > 0.0f) &&
         (cr >= 1) && (cr <= 4) && (config.preamble_length > 0) &&
         (config.symbol_timeout <= 248) &&
         (header <= 1) && (crc <= 1) && (iq <= 1) && (ramp <= 7) &&
         (!config.configure_cad ||
             ((cad_symbols <= 4) && valid_cad_exit &&
                 (config.cad_timeout_us <= 262143984U)));
}

bool Sx126x::ValidateConfig(const GfskConfig& config) const {
  const int8_t min_power =
      (chip_type_ == ChipType::kSx1261) ? -17 : -9;
  const int8_t max_power =
      (chip_type_ == ChipType::kSx1261) ? 14 : 22;
  const float bandwidth_khz = GetGfskBandwidthKhz(config.bandwidth);
  uint8_t detector_bits = 0;
  switch (config.preamble_detector) {
    case PreambleDetector::kLengthOff:
      break;
    case PreambleDetector::kLength8bit:
      detector_bits = 8;
      break;
    case PreambleDetector::kLength16bit:
      detector_bits = 16;
      break;
    case PreambleDetector::kLength24bit:
      detector_bits = 24;
      break;
    case PreambleDetector::kLength32bit:
      detector_bits = 32;
      break;
    default:
      return false;
  }

  const uint8_t pulse_shape = static_cast<uint8_t>(config.pulse_shape);
  const bool valid_pulse_shape = (pulse_shape == 0) ||
                                 ((pulse_shape >= 0x08) &&
                                     (pulse_shape <= 0x0B));
  const uint8_t crc = static_cast<uint8_t>(config.crc_type);
  const bool valid_crc = (crc <= 2) || (crc == 4) || (crc == 6);
  const uint8_t header = static_cast<uint8_t>(config.header_type);
  const uint8_t address =
      static_cast<uint8_t>(config.address_comparison);
  const uint8_t whitening = static_cast<uint8_t>(config.whitening);
  const uint8_t ramp = static_cast<uint8_t>(config.ramp_time);
  const double required_bandwidth = config.bit_rate_kbps +
                                    (2.0 * config.frequency_deviation_khz) +
                                    config.frequency_error_khz;
  const uint16_t sync_word_bits =
      static_cast<uint16_t>(config.sync_word_length) * 8U;

  return std::isfinite(config.frequency_mhz) &&
         (config.frequency_mhz >= 150.0) &&
         (config.frequency_mhz <= 960.0) &&
         std::isfinite(config.current_limit) &&
         (config.current_limit >= 0.0f) &&
         (config.current_limit <= 157.5f) &&
         (config.power >= min_power) && (config.power <= max_power) &&
         std::isfinite(config.bit_rate_kbps) &&
         (config.bit_rate_kbps >= 0.6) &&
         (config.bit_rate_kbps <= 300.0) &&
         std::isfinite(config.frequency_deviation_khz) &&
         (config.frequency_deviation_khz >= 0.6) &&
         (config.frequency_deviation_khz <= 200.0) &&
         ((config.frequency_deviation_khz +
              (config.bit_rate_kbps / 2.0)) <= 250.0) &&
         (((2.0 * config.frequency_deviation_khz) /
              config.bit_rate_kbps) >= 0.5) &&
         std::isfinite(config.frequency_error_khz) &&
         (config.frequency_error_khz >= 0.0) &&
         (bandwidth_khz >= required_bandwidth) &&
         (config.sync_word_length <= config.sync_word.size()) &&
         (config.preamble_length > 0) &&
         (detector_bits <= config.preamble_length) &&
         ((detector_bits == 0) || (detector_bits < sync_word_bits)) &&
         valid_pulse_shape && valid_crc && (header <= 1) &&
         (address <= 2) && (whitening <= 1) &&
         (config.whitening_seed <= 0x01FF) && (ramp <= 7);
}

bool Sx126x::ApplyHardwareConfig(bool calibrate_tcxo) {
  if (!SetStandby(StdbyConfig::kStdbyRc)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetStandby failed\n");
    return false;
  }
  if (!SetRegulatorMode(hardware_config_.regulator_mode)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetRegulatorMode failed\n");
    return false;
  }
  if (!SetDio2AsRfSwitchCtrl(hardware_config_.dio2_mode)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetDio2AsRfSwitchCtrl failed\n");
    return false;
  }
  if (hardware_config_.enable_dio3_tcxo &&
      !SetDio3AsTcxoCtrl(hardware_config_.tcxo_voltage,
          hardware_config_.tcxo_startup_time_us)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetDio3AsTcxoCtrl failed\n");
    return false;
  }
  if (hardware_config_.enable_dio3_tcxo && calibrate_tcxo &&
      !Calibrate(kCalibrateAll)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Calibrate failed\n");
    return false;
  }
  if (hardware_config_.enable_retention_list && !InitRetentionList()) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "InitRetentionList failed\n");
    return false;
  }
  if ((chip_type_ == ChipType::kSx1262) &&
      hardware_config_.enable_tx_clamp_workaround && !FixTxClamp(true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "FixTxClamp failed\n");
    return false;
  }
  return true;
}

float Sx126x::GetGfskBandwidthKhz(GfskBw bandwidth) const {
  switch (bandwidth) {
    case GfskBw::kBw4800Hz:
      return 4.8f;
    case GfskBw::kBw5800Hz:
      return 5.8f;
    case GfskBw::kBw7300Hz:
      return 7.3f;
    case GfskBw::kBw9700Hz:
      return 9.7f;
    case GfskBw::kBw11700Hz:
      return 11.7f;
    case GfskBw::kBw14600Hz:
      return 14.6f;
    case GfskBw::kBw19500Hz:
      return 19.5f;
    case GfskBw::kBw23400Hz:
      return 23.4f;
    case GfskBw::kBw29300Hz:
      return 29.3f;
    case GfskBw::kBw39000Hz:
      return 39.0f;
    case GfskBw::kBw46900Hz:
      return 46.9f;
    case GfskBw::kBw58600Hz:
      return 58.6f;
    case GfskBw::kBw78200Hz:
      return 78.2f;
    case GfskBw::kBw93800Hz:
      return 93.8f;
    case GfskBw::kBw117300Hz:
      return 117.3f;
    case GfskBw::kBw156200Hz:
      return 156.2f;
    case GfskBw::kBw187200Hz:
      return 187.2f;
    case GfskBw::kBw234300Hz:
      return 234.3f;
    case GfskBw::kBw312000Hz:
      return 312.0f;
    case GfskBw::kBw373600Hz:
      return 373.6f;
    case GfskBw::kBw467000Hz:
      return 467.0f;
    default:
      return 0.0f;
  }
}

bool Sx126x::ReadModifyWriteRegister(
    Reg reg, uint8_t mask, uint8_t value) {
  uint8_t register_value = 0;
  if (!ReadRegister(static_cast<uint16_t>(reg), &register_value, 1)) {
    return false;
  }
  register_value =
      static_cast<uint8_t>((register_value & ~mask) | (value & mask));
  return WriteRegister(
      static_cast<uint16_t>(reg), &register_value, 1);
}

bool Sx126x::ResetGfskLowRateWorkaround() {
  if (!gfsk_low_rate_workaround_active_) {
    return true;
  }
  const bool result =
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround1, 0x18, 0x08) &&
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround2, 0x1C, 0x00) &&
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround3, 0x10, 0x10) &&
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround4, 0x70, 0x00);
  if (result) {
    gfsk_low_rate_workaround_active_ = false;
  }
  return result;
}

bool Sx126x::ApplyGfskLowRateWorkaround(const GfskConfig& config) {
  const bool is_1200_bps =
      (std::fabs(config.bit_rate_kbps - 1.2) < 0.0001) &&
      (std::fabs(config.frequency_deviation_khz - 5.0) < 0.0001) &&
      (config.bandwidth == GfskBw::kBw19500Hz);
  if (is_1200_bps) {
    const bool result = ReadModifyWriteRegister(
        Reg::kRwGfskWorkaround3, 0x10, 0x00);
    gfsk_low_rate_workaround_active_ = result;
    return result;
  }

  const bool is_600_bps =
      (std::fabs(config.bit_rate_kbps - 0.6) < 0.0001) &&
      (std::fabs(config.frequency_deviation_khz - 0.8) < 0.0001) &&
      (config.bandwidth == GfskBw::kBw4800Hz);
  if (!is_600_bps) {
    return true;
  }

  const bool result =
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround1, 0x18, 0x18) &&
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround2, 0x1C, 0x04) &&
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround3, 0x10, 0x00) &&
      ReadModifyWriteRegister(Reg::kRwGfskWorkaround4, 0x70, 0x50);
  gfsk_low_rate_workaround_active_ = result;
  return result;
}

bool Sx126x::AddRegistersToRetentionList(
    const uint16_t* register_address, size_t register_count) {
  constexpr size_t kRetentionListSize = 9;
  constexpr uint8_t kMaxRetentionRegisterCount = 4;

  if (((register_count > 0) && (register_address == nullptr)) ||
      (register_count > kMaxRetentionRegisterCount)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  std::array<uint8_t, kRetentionListSize> buffer = {};
  if (!ReadRegister(static_cast<uint16_t>(Reg::kRwRetentionListBaseAddress),
          buffer.data(), buffer.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  const uint8_t initial_count = buffer[0];
  if (initial_count > kMaxRetentionRegisterCount) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  uint8_t* list = &buffer[1];
  for (size_t i = 0; i < register_count; ++i) {
    bool should_add = true;
    for (uint8_t j = 0; j < buffer[0]; ++j) {
      const uint16_t current_register =
          (static_cast<uint16_t>(list[2 * j]) << 8) | list[(2 * j) + 1];
      if (current_register == register_address[i]) {
        should_add = false;
        break;
      }
    }

    if (should_add) {
      if (buffer[0] >= kMaxRetentionRegisterCount) {
        LogMessage(
            LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
        return false;
      }
      list[2 * buffer[0]] = static_cast<uint8_t>(register_address[i] >> 8);
      list[(2 * buffer[0]) + 1] = static_cast<uint8_t>(register_address[i]);
      ++buffer[0];
    }
  }

  if (buffer[0] == initial_count) {
    return true;
  }

  if (!WriteRegister(static_cast<uint16_t>(Reg::kRwRetentionListBaseAddress),
          buffer.data(), buffer.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  return true;
}

bool Sx126x::InitRetentionList() {
  const uint16_t register_list[] = {
      static_cast<uint16_t>(Reg::kRwRxGain),
      static_cast<uint16_t>(Reg::kRwTxModulation),
      static_cast<uint16_t>(Reg::kRwIqPolaritySetup),
  };

  return AddRegistersToRetentionList(
      register_list, sizeof(register_list) / sizeof(register_list[0]));
}

bool Sx126x::ApplyWorkaroundsAfterWakeup() {
  if ((chip_type_ == ChipType::kSx1262) &&
      hardware_config_.enable_tx_clamp_workaround && !FixTxClamp(true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "FixTxClamp failed\n");
    return false;
  }

  if (configured_ && (param_.packet_type == PacketType::kLora)) {
    if (!FixLoraInvertedIq(param_.lora.invert_iq)) {
      LogMessage(
          LogLevel::kError, __FILE__, __LINE__, "FixLoraInvertedIq failed\n");
      return false;
    }
    if (!FixBw500KhzSensitivity(
            param_.lora.band_width == LoraBw::kBw500000Hz)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "FixBw500KhzSensitivity failed\n");
      return false;
    }
  } else if (configured_ && (param_.packet_type == PacketType::kGfsk)) {
    if (!FixBw500KhzSensitivity(false)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "FixBw500KhzSensitivity failed\n");
      return false;
    }
    if (!ResetGfskLowRateWorkaround() ||
        !ApplyGfskLowRateWorkaround(gfsk_config_)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "ApplyGfskLowRateWorkaround failed\n");
      return false;
    }
  }

  if (param_.rx_boosted && !SetRxBoosted(true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRxBoosted failed\n");
    return false;
  }

  return true;
}

bool Sx126x::StopRxTimeoutTimer() {
  uint8_t reg_value = 0;

  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwRtcControl), &reg_value, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  if (!ReadRegister(static_cast<uint16_t>(Reg::kRwEventClear), &reg_value, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  static constexpr uint8_t kTimeoutEventClearMask = 0x02;
  reg_value |= kTimeoutEventClearMask;
  if (!WriteRegister(
          static_cast<uint16_t>(Reg::kRwEventClear), &reg_value, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
