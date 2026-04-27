/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-04-25 15:47:22
 * @License: GPL 3.0
 */
#include "sx126x.h"

namespace cpp_bus_driver {
bool Sx126x::Init(int32_t freq_hz) {
  if (busy_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetPinMode(busy_, PinMode::kInput, PinStatus::kDisable);
  }

  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetPinMode(rst_, PinMode::kOutput, PinStatus::kPullup);

    PinWrite(rst_, 1);
    DelayMs(10);
    PinWrite(rst_, 0);
    DelayMs(10);
    PinWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipSpiGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sx126x id failed (error id: %s)\n", buffer.c_str());
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get sx126x id success (id: %s)\n", buffer.c_str());
  }

  if (!FixTxClamp(true)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "FixTxClamp failed\n");
  }

  // 启用13MHz晶振模式
  if (!SetStandby(StdbyConfig::kStdbyRc)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetStandby failed\n");
  }

  // TCXO的供电电压不能超过供电电压减去200 mV （VDDop > kVtcxo + 200 mV）
  if (!SetDio3AsTcxoCtrl(Dio3TcxoVoltage::kOutput1600Mv, 5000)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetDio3AsTcxoCtrl failed\n");
  }

  // 设置电源调节器模式
  if (!SetRegulatorMode(RegulatorMode::kLdoAndDcdc)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetRegulatorMode failed\n");
  }

  // 设置DIO2的模式功能为控制RF开关
  if (!SetDio2AsRfSwitchCtrl(Dio2Mode::kRfSwitch)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetDio2AsRfSwitchCtrl failed\n");
  }

  return true;
}

std::string Sx126x::GetDeviceId() {
  uint8_t buffer[7] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(Reg::kRoDeviceId), buffer, 7)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return "fail";
  }

  return std::string(reinterpret_cast<char*>(buffer + 1), 6);
}

bool Sx126x::CheckBusy() {
  if (busy_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    uint16_t timeout_count = 0;
    while (1) {
      DelayUs(1);
      if (PinRead(busy_) == 0) {
        break;
      }
      timeout_count++;
      if (timeout_count > kBusyPinTimeoutCount) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "busy_ timeout\n");
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
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "busy_wait_callback_ timeout\n");
        return false;
      }
    }
  } else {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "CheckBusy failed\n");
    return false;
  }

  return true;
}

uint8_t Sx126x::GetStatus() {
  uint8_t buffer = 0;

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetStatus), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

Sx126x::CmdStatus Sx126x::ParseCmdStatus(uint8_t parse_status) {
  if ((parse_status == 0x00) || (parse_status == static_cast<uint8_t>(-1))) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
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
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return CmdStatus::kFalse;
  }

  return static_cast<CmdStatus>(buffer);
}

Sx126x::ChipModeStatus Sx126x::ParseChipModeStatus(uint8_t parse_status) {
  if ((parse_status == 0x00) || (parse_status == static_cast<uint8_t>(-1))) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
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
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return ChipModeStatus::kFalse;
  }

  return static_cast<ChipModeStatus>(buffer);
}

bool Sx126x::ParseIrqStatus(uint16_t irq_flag, IrqStatus& status) {
  if (irq_flag == static_cast<uint16_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

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

  return true;
}

bool Sx126x::SetStandby(StdbyConfig config) {
  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetStandby),
          static_cast<uint8_t>(config))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetDio3AsTcxoCtrl(Dio3TcxoVoltage voltage, uint32_t time_out_us) {
  time_out_us = static_cast<float>(time_out_us) / 15.625;
  uint8_t buffer[] = {
      static_cast<uint8_t>(voltage),
      static_cast<uint8_t>(time_out_us >> 16),
      static_cast<uint8_t>(time_out_us >> 8),
      static_cast<uint8_t>(time_out_us),
  };

  CheckBusy();
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoSetDio3AsTcxoCtrl), buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::FixTxClamp(bool enable) {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(Reg::kRwTxClampConfig), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (enable) {
    buffer[1] |= 0B00011110;
  } else {
    buffer[1] = (buffer[1] & 0B11100001) | 0B00010000;
  }

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
          static_cast<uint16_t>(Reg::kRwTxClampConfig), buffer[1])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetBufferBaseAddress(
    uint8_t tx_base_address, uint8_t rx_base_address) {
  uint8_t buffer[] = {tx_base_address, rx_base_address};

  CheckBusy();
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoSetBufferBaseAddress), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetPacketType(PacketType type) {
  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetPacketType),
          static_cast<uint8_t>(type))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  param_.packet_type = type;

  return true;
}

bool Sx126x::SetRxTxFallbackMode(FallbackMode mode) {
  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetRxTxFallbackMode),
          static_cast<uint8_t>(mode))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetCadParams(CadSymbolNum num, uint8_t cad_det_peak,
    uint8_t cad_det_min, CadExitMode exit_mode, uint32_t time_out_us) {
  time_out_us = static_cast<float>(time_out_us) / 15.625;
  uint8_t buffer[] = {
      static_cast<uint8_t>(num),
      cad_det_peak,
      cad_det_min,
      static_cast<uint8_t>(exit_mode),
      static_cast<uint8_t>(time_out_us >> 16),
      static_cast<uint8_t>(time_out_us >> 8),
      static_cast<uint8_t>(time_out_us),
  };

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetCadParams), buffer, 7)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::ClearIrqFlag(IrqMaskFlag flag) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(static_cast<uint16_t>(flag) >> 8),
      static_cast<uint8_t>(flag),
  };

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoClearIrqStatus), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
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

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetDioIrqParams), buffer, 8)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::Calibrate(uint8_t calib_param) {
  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoCalibrate), calib_param)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

Sx126x::PacketType Sx126x::GetPacketType() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetPacketType), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return PacketType::kFalse;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return PacketType::kFalse;
  }

  switch (buffer[1]) {
    case static_cast<uint8_t>(PacketType::kGfsk):
      break;
    case static_cast<uint8_t>(PacketType::kLora):
      break;
    case static_cast<uint8_t>(PacketType::kLrFhss):
      break;

    default:
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return PacketType::kFalse;
  }

  return static_cast<PacketType>(buffer[1]);
}

bool Sx126x::SetRegulatorMode(RegulatorMode mode) {
  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetRegulatorMode),
          static_cast<uint8_t>(mode))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  param_.regulator_mode = mode;

  return true;
}

bool Sx126x::SetCurrentLimit(float current) {
  if (current < 0.0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    current = 0.0;
  } else if (current > 140.0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    current = 140.0;
  }

  current /= 2.5;

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), current)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  param_.current_limit = current;

  return true;
}

uint8_t Sx126x::GetCurrentLimit() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return -1;
  }

  return static_cast<float>(buffer[1]) * 2.5;
}

bool Sx126x::SetDio2AsRfSwitchCtrl(Dio2Mode mode) {
  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetDio2AsRfSwitchCtrl),
          static_cast<uint8_t>(mode))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetPaConfig(uint8_t pa_duty_cycle, uint8_t hp_max) {
  uint8_t buffer[] = {
      pa_duty_cycle,
      hp_max,
      static_cast<uint8_t>(chip_type_),
      0x01,  // 这一位固定为0x01
  };

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetPaConfig), buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetTxParams(uint8_t power, RampTime ramp_time) {
  uint8_t buffer[] = {power, static_cast<uint8_t>(ramp_time)};

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetTxParams), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetLoraSyncWord(uint16_t sync_word) {
  uint8_t buffer[2] = {
      static_cast<uint8_t>(sync_word >> 8), static_cast<uint8_t>(sync_word)};

  for (uint8_t i = 0; i < 2; i++) {
    CheckBusy();
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
            static_cast<uint16_t>(Reg::kRwLoraSyncWordStart) + i, buffer[i])) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }
  param_.lora.sync_word = sync_word;

  return true;
}

uint16_t Sx126x::GetLoraSyncWord() {
  uint8_t buffer[4] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(Reg::kRwLoraSyncWordStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return -1;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(
              static_cast<uint16_t>(Reg::kRwLoraSyncWordStart) + 1),
          &buffer[2], 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  buffer_cs = ParseCmdStatus(buffer[2]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[2]);
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[3];
}

bool Sx126x::FixLoraInvertedIq(InvertIq iq) {
  uint8_t buffer[2] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(Reg::kRwIqPolaritySetup), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return false;
  }

  if (iq == InvertIq::kStandardIqSetup) {
    buffer[1] |= 0B00000100;
  } else {
    buffer[1] &= 0B11111011;
  }

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
          static_cast<uint16_t>(Reg::kRwIqPolaritySetup), buffer[1])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetLoraModulationParams(Sf sf, LoraBw bw, Cr cr, Ldro ldro) {
  uint8_t buffer[] = {static_cast<uint8_t>(sf), static_cast<uint8_t>(bw),
      static_cast<uint8_t>(cr), static_cast<uint8_t>(ldro)};

  CheckBusy();
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoSetModulationParams), buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
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
  uint8_t buffer[] = {
      static_cast<uint8_t>(preamble_length >> 8),
      static_cast<uint8_t>(preamble_length),
      static_cast<uint8_t>(header_type),
      payload_length,
      static_cast<uint8_t>(crc_type),
      static_cast<uint8_t>(iq),
  };

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetPacketParams), buffer, 6)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  param_.lora.preamble_length = preamble_length;
  param_.lora.header_type = header_type;
  param_.lora.payload_length = payload_length;
  param_.lora.crc_type = crc_type;
  param_.lora.invert_iq = iq;

  return true;
}

bool Sx126x::SetOutputPower(int8_t power) {
  if (power < (-9)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    power = -9;
  } else if (power > 22) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    power = 22;
  }

  uint8_t buffer[2] = {0};

  // 读取OCP配置
  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return false;
  }

  if (!SetPaConfig(0x04, 0x07)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetPaConfig failed\n");
    return false;
  }

  if (!SetTxParams(power, RampTime::kRamp200Us)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetTxParams failed\n");
    return false;
  }

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
          static_cast<uint16_t>(Reg::kRwOcpConfiguration), buffer[1])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
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
      break;
  }

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoCalibrateImage), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetRfFrequency(double freq_mhz) {
  uint32_t buffer_freq = (freq_mhz * (static_cast<uint32_t>(1) << 25)) / 32.0;

  uint8_t buffer[] = {
      static_cast<uint8_t>(buffer_freq >> 24),
      static_cast<uint8_t>(buffer_freq >> 16),
      static_cast<uint8_t>(buffer_freq >> 8),
      static_cast<uint8_t>(buffer_freq),
  };

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetRfFrequency), buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetFrequency(double freq_mhz) {
  if (freq_mhz < 150.0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    freq_mhz = 150.0;
  } else if (freq_mhz > 960.0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    freq_mhz = 960.0;
  }

  if ((freq_mhz >= 902.0) && (freq_mhz <= 928.0)) {
    CalibrateImage(ImgCalFreq::kFreq902_928Mhz);
  } else if ((freq_mhz >= 863.0) && (freq_mhz <= 870.0)) {
    CalibrateImage(ImgCalFreq::kFreq863_870Mhz);
  } else if ((freq_mhz >= 779.0) && (freq_mhz <= 787.0)) {
    CalibrateImage(ImgCalFreq::kFreq779_787Mhz);
  } else if ((freq_mhz >= 470.0) && (freq_mhz <= 510.0)) {
    CalibrateImage(ImgCalFreq::kFreq470_510Mhz);
  } else if ((freq_mhz >= 430.0) && (freq_mhz <= 440.0)) {
    CalibrateImage(ImgCalFreq::kFreq430_440Mhz);
  }

  // 设置射频频率模式的频率
  if (!SetRfFrequency(freq_mhz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetRfFrequency failed\n");
    return false;
  }
  param_.freq_mhz = freq_mhz;

  return true;
}

bool Sx126x::ConfigLoraParams(double freq_mhz, LoraBw bw, float current_limit,
    int8_t power, Sf sf, Cr cr, LoraCrcType crc_type, uint16_t preamble_length,
    uint16_t sync_word) {
  // 启用13MHz晶振模式
  if (!SetStandby(StdbyConfig::kStdbyRc)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetStandby failed\n");
    return false;
  }

  // 设置包类型
  if (!SetPacketType(PacketType::kLora)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetPacketType failed\n");
    return false;
  }

  if (!SetCadParams(CadSymbolNum::kOn8Symb, static_cast<uint8_t>(sf) + 13, 10,
          CadExitMode::kOnly, 0)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetCadParams failed\n");
    return false;
  }

  // 校准
  if (!Calibrate(0b01111111)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Calibrate failed\n");
    return false;
  }

  DelayMs(5);
  CheckBusy();

  // 检查 calibrate 命令结果
  CmdStatus buffer_cs = ParseCmdStatus(GetStatus());
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n",
        static_cast<uint8_t>(buffer_cs));
    return false;
  }

  // 自动设置低速率优化（大于或等于16.384 ms时建议启用）
  // kSf11 with kBw125 kHz: time = (2^11 / 125000Hz)*1000 = 16.384 Ms (建议启用
  // kLdro) kSf12 with kBw125 kHz: time = (2^12 / 125000Hz)*1000 = 32.768 ms
  // (建议启用 kLdro) kSf12 with kBw250 kHz: time = (2^12 / 250000Hz)*1000
  // = 16.384 Ms (建议启用 kLdro)

  float buffer_time = 0;
  switch (bw) {
    case LoraBw::kBw7810Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        7810.0) *
                    1000.0;
      break;
    case LoraBw::kBw15630Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        15630.0) *
                    1000.0;
      break;
    case LoraBw::kBw31250Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        31250.0) *
                    1000.0;
      break;
    case LoraBw::kBw62500Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        62500.0) *
                    1000.0;
      break;
    case LoraBw::kBw125000Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        125000.0) *
                    1000.0;
      break;
    case LoraBw::kBw250000Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        250000.0) *
                    1000.0;
      break;
    case LoraBw::kBw500000Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        500000.0) *
                    1000.0;
      break;
    case LoraBw::kBw10420Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        10420.0) *
                    1000.0;
      break;
    case LoraBw::kBw20830Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        20830.0) *
                    1000.0;
      break;
    case LoraBw::kBw41670Hz:
      buffer_time = (static_cast<float>((static_cast<uint32_t>(1)
                                         << static_cast<uint8_t>(sf))) /
                        41670.0) *
                    1000.0;
      break;

    default:
      break;
  }
  if (buffer_time >= 16.384) {
    if (!SetLoraModulationParams(sf, bw, cr, Ldro::kLdroOn)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__,
          "SetLoraModulationParams failed\n");
      return false;
    }
  } else {
    if (!SetLoraModulationParams(sf, bw, cr, Ldro::kLdroOff)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__,
          "SetLoraModulationParams failed\n");
      return false;
    }
  }

  // 设置同步字
  if (!SetLoraSyncWord(sync_word)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetLoraSyncWord failed\n");
    return false;
  }

  if (!FixLoraInvertedIq(InvertIq::kStandardIqSetup)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "FixLoraInvertedIq failed\n");
    return false;
  }

  // 设置包的参数
  if (!SetLoraPacketParams(preamble_length,
          LoraHeaderType::kVariableLengthPacket, kMaxTransmitBufferSize,
          crc_type, InvertIq::kStandardIqSetup)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetLoraPacketParams failed\n");
    return false;
  }

  // 设置电流限制
  if (!SetCurrentLimit(current_limit)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetCurrentLimit failed\n");
    return false;
  }

  // 设置频率
  if (!SetFrequency(freq_mhz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetFrequency failed\n");
    return false;
  }

  // 设置功率
  if (!SetOutputPower(power)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetOutputPower failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetRx(uint32_t time_out_us) {
  if (time_out_us != 0xFFFFFF) {
    time_out_us = static_cast<float>(time_out_us) / 15.625;
  }

  uint8_t buffer[] = {
      static_cast<uint8_t>(time_out_us >> 16),
      static_cast<uint8_t>(time_out_us >> 8),
      static_cast<uint8_t>(time_out_us),
  };

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetRx), buffer, 3)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::StartLoraTransmit(ChipMode chip_mode, uint32_t time_out_us,
    FallbackMode fallback_mode, uint16_t preamble_length) {
  // 从RX或TX模式退出返回的模式设定
  if (!SetRxTxFallbackMode(fallback_mode)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetPacketType failed\n");
    return false;
  }

  // 设置包类型
  if (!SetLoraPacketParams(preamble_length, param_.lora.header_type,
          param_.lora.payload_length, param_.lora.crc_type,
          param_.lora.invert_iq)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetLoraPacketParams failed\n");
    return false;
  }

  switch (chip_mode) {
    case ChipMode::kRx:
      // 设置为接收模式
      if (!SetRx(time_out_us)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetRx failed\n");
        return false;
      }
      break;
    case ChipMode::kTx:
      // 设置为发送模式
      // if (SetTx(time_out_us))
      // {
      //     LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetRx failed\n");
      //     return false;
      // }
      break;

    default:
      break;
  }

  return true;
}

uint16_t Sx126x::GetIrqFlag() {
  uint8_t buffer[3] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetIrqStatus), buffer, 3)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[2];
}

uint8_t Sx126x::GetRxBufferLength() {
  uint8_t buffer[3] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetRxBufferStatus), buffer, 3)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return false;
  }

  return buffer[1];
}

bool Sx126x::ReadBuffer(uint8_t* data, uint8_t length, uint8_t offset) {
  // 设置基地址
  if (!SetBufferBaseAddress(0, 0)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetBufferBaseAddress failed\n");
    return false;
  }

  uint8_t buffer[length + 1] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoReadBuffer), offset, buffer,
          length + 1)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return false;
  }

  std::memcpy(data, &buffer[1], length);

  return true;
}

uint8_t Sx126x::ReceiveData(uint8_t* data, uint8_t length) {
  assert_ = 0;

  // // 检查中断
  // IrqStatus buffer_is;
  // if (!AssertIrqFlag(GetIrqFlag(), buffer_is))
  // {
  //     LogMessage(LogLevel::kChip, __FILE__, __LINE__, "AssertIrqFlag
  //     failed\n"); assert_ = 1; return false;
  // }
  // else
  // {
  //     if (buffer_is.all_flag.tx_rx_timeout)
  //     {
  //         LogMessage(LogLevel::kChip, __FILE__, __LINE__, "receive
  //         timeout\n"); assert_ = 2; return false;
  //     }
  //     if (buffer_is.all_flag.crc_error)
  //     {
  //         LogMessage(LogLevel::kChip, __FILE__, __LINE__, "receive crc
  //         error\n"); assert_ = 3; return false;
  //     }

  //     if (param_.packet_type == PacketType::kLora)
  //     {
  //         if (buffer_is.lora_reg_flag.header_error)
  //         {
  //             LogMessage(LogLevel::kChip, __FILE__, __LINE__, "lora receive
  //             header error\n"); assert_ = 4; return false;
  //         }
  //     }
  // }

  uint8_t buffer_length = GetRxBufferLength();
  if (buffer_length == 0) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "GetRxBufferLength failed\n");
    assert_ = 1;
    return false;
  }

  if ((length == 0) || (length >= buffer_length)) {
    if (!ReadBuffer(data, buffer_length)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadBuffer failed\n");
      assert_ = 2;
      return false;
    }
  } else if (length < buffer_length) {
    if (!ReadBuffer(data, length)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadBuffer failed\n");
      assert_ = 2;
      return false;
    }
  }

  return buffer_length;
}

bool Sx126x::GetLoraPacketMetrics(PacketMetrics& metrics) {
  uint8_t buffer[4] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetPacketStatus), buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return false;
  }

  metrics.lora.rssi_average = -1.0 * static_cast<float>(buffer[1]) / 2.0;
  metrics.lora.snr = static_cast<float>(static_cast<int8_t>(buffer[2])) / 4.0;
  metrics.lora.rssi_instantaneous = -1.0 * static_cast<float>(buffer[3]) / 2.0;

  return true;
}

bool Sx126x::FixBw500KhzSensitivity(bool enable) {
  uint8_t buffer[2] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kWoReadRegister),
          static_cast<uint16_t>(Reg::kRwTxModulation), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return false;
  }

  if (enable) {
    buffer[1] &= 0xFB;
  } else {
    buffer[1] |= 0x04;
  }

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
          static_cast<uint16_t>(Reg::kRwTxModulation), buffer[1])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetTx(uint32_t time_out_us) {
  if (time_out_us != 0xFFFFFF) {
    time_out_us = static_cast<float>(time_out_us) / 15.625;
  }

  uint8_t buffer[] = {
      static_cast<uint8_t>(time_out_us >> 16),
      static_cast<uint8_t>(time_out_us >> 8),
      static_cast<uint8_t>(time_out_us),
  };

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetTx), buffer, 3)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::WriteBuffer(const uint8_t* data, uint8_t length, uint8_t offset) {
  // 设置基地址
  if (!SetBufferBaseAddress(0, 0)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetBufferBaseAddress failed\n");
    return false;
  }

  // 没有校验
  // CheckBusy();
  // if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteBuffer), offset, data,
  // length))
  // {
  //     LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
  //     return false;
  // }

  // 有校验
  uint8_t buffer[2 + length] = {
      static_cast<uint8_t>(Cmd::kWoWriteBuffer),
      offset,
  };

  uint8_t assert[2 + length] = {0};

  std::memcpy(&buffer[2], data, length);

  CheckBusy();
  if (!bus_->WriteRead(buffer, assert, 2 + length)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  for (uint16_t i = 1; i < (2 + length); i++) {
    CmdStatus buffer_cs = ParseCmdStatus(assert[i]);
    if ((buffer_cs != CmdStatus::kRfu) &&
        (buffer_cs != CmdStatus::kCmdTxDone) &&
        (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
      if (i == 1) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "Offset data write failed (error code: %#X)\n", assert[i]);
        return false;
      } else {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "Data[%d] write failed (error code: %#X)\n", i - 2, assert[i]);
        return false;
      }
    }
  }

  return true;
}

bool Sx126x::SendData(const uint8_t* data, uint8_t length, uint32_t time_out_us) {
  switch (param_.packet_type) {
    case PacketType::kGfsk:
      if (param_.gfsk.payload_length != length) {
        // 重新设置长度
        if (!SetGfskPacketParams(param_.gfsk.preamble_length,
                param_.gfsk.preamble_detector, param_.gfsk.sync_word.length * 8,
                param_.gfsk.address_comparison, param_.gfsk.header_type, length,
                param_.gfsk.crc.type, param_.gfsk.whitening)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "SetGfskPacketParams failed\n");
          return false;
        }
        param_.gfsk.payload_length = length;
      }
      break;
    case PacketType::kLora:
      if (param_.lora.payload_length != length) {
        // 重新设置长度
        if (!SetLoraPacketParams(param_.lora.preamble_length,
                param_.lora.header_type, length, param_.lora.crc_type,
                param_.lora.invert_iq)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "SetLoraPacketParams failed\n");
          return false;
        }
        param_.lora.payload_length = length;
      }
      break;

    default:
      break;
  }

  if (!WriteBuffer(data, length, 0)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteBuffer failed\n");
    return false;
  }

  if (param_.packet_type == PacketType::kLora) {
    if (param_.lora.band_width == LoraBw::kBw500000Hz) {
      FixBw500KhzSensitivity(true);
    } else {
      FixBw500KhzSensitivity(false);
    }
  }

  if (!SetTx(time_out_us)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetTx failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetLoraCrcPacketParams(LoraCrcType crc_type) {
  // 设置CRC
  if (!SetLoraPacketParams(param_.lora.preamble_length, param_.lora.header_type,
          param_.lora.payload_length, crc_type, param_.lora.invert_iq)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetLoraPacketParams failed\n");
    return false;
  }
  param_.lora.crc_type = crc_type;

  return true;
}

bool Sx126x::SetGfskModulationParams(
    double br, PulseShape ps, GfskBw bw, double freq_deviation_khz) {
  if (br < 0.6) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    br = 0.6;
  } else if (br > 300.0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    br = 300.0;
  }

  if (freq_deviation_khz < 0.6) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    freq_deviation_khz = 0.6;
  } else if (freq_deviation_khz > 200.0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    freq_deviation_khz = 200.0;
  }

  // 计算原始比特率值
  uint32_t buffer_br = (32.0 * 1000000.0 * 32.0) / (br * 1000.0);

  // 计算原始频率偏差值
  uint32_t buffer_freq_deviation_khz =
      ((freq_deviation_khz * 1000.0) *
          static_cast<double>(static_cast<uint32_t>(1) << 25)) /
      (32.0 * 1000000.0);

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

  CheckBusy();
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoSetModulationParams), buffer, 8)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  param_.gfsk.bit_rate = br;
  param_.gfsk.pulse_shape = ps;
  param_.gfsk.band_width = bw;
  param_.gfsk.freq_deviation_khz = freq_deviation_khz;

  return true;
}

bool Sx126x::SetGfskSyncWord(uint8_t* sync_word, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    CheckBusy();
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
            static_cast<uint16_t>(Reg::kRwSyncWordProgrammingStart) + i,
            sync_word[i])) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

bool Sx126x::SetGfskPacketParams(uint16_t preamble_length,
    PreambleDetector preamble_detector_length, uint8_t sync_word_length,
    AddrComp addr_comp, GfskHeaderType header_type, uint8_t payload_length,
    GfskCrcType crc_type, Whitening whitening) {
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

  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetPacketParams), buffer, 9)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  param_.gfsk.preamble_length = preamble_length;
  param_.gfsk.address_comparison = addr_comp;
  param_.gfsk.header_type = header_type;
  param_.gfsk.payload_length = payload_length;
  param_.gfsk.crc.type = crc_type;
  param_.gfsk.whitening = whitening;

  return true;
}

bool Sx126x::SetGfskCrc(uint16_t initial, uint16_t polynomial) {
  uint8_t buffer[] = {
      static_cast<uint8_t>(initial >> 8),
      static_cast<uint8_t>(initial),
      static_cast<uint8_t>(polynomial >> 8),
      static_cast<uint8_t>(polynomial),
  };

  for (uint8_t i = 0; i < 4; i++) {
    CheckBusy();
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoWriteRegister),
            static_cast<uint16_t>(Reg::kRwCrcValueProgrammingStart) + i,
            buffer[i])) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }
  param_.gfsk.crc.initial = initial;
  param_.gfsk.crc.polynomial = polynomial;

  return true;
}

bool Sx126x::ConfigGfskParams(double freq_mhz, double br, GfskBw bw,
    float current_limit, int8_t power, double freq_deviation_khz,
    uint8_t* sync_word, uint8_t sync_word_length, PulseShape ps, Sf sf,
    GfskCrcType crc_type, uint16_t crc_initial, uint16_t crc_polynomial,
    uint16_t preamble_length) {
  // 启用13MHz晶振模式
  if (!SetStandby(StdbyConfig::kStdbyRc)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetStandby failed\n");
    return false;
  }

  // 设置包类型
  if (!SetPacketType(PacketType::kGfsk)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetPacketType failed\n");
    return false;
  }

  if (!SetCadParams(CadSymbolNum::kOn8Symb, static_cast<uint8_t>(sf) + 13, 10,
          CadExitMode::kOnly, 0)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetCadParams failed\n");
    return false;
  }
  param_.gfsk.spreading_factor = sf;

  // 校准
  if (!Calibrate(0b01111111)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Calibrate failed\n");
    return false;
  }

  DelayMs(5);
  CheckBusy();

  // 检查 calibrate 命令结果
  CmdStatus buffer_cs = ParseCmdStatus(GetStatus());
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n",
        static_cast<uint8_t>(buffer_cs));
    return false;
  }

  if (!SetGfskModulationParams(br, ps, bw, freq_deviation_khz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "SetGfskModulationParams failed\n");
    return false;
  }

  if ((sync_word == nullptr) || (sync_word_length == 0)) {
    uint8_t buffer_sync_word[] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    sync_word = buffer_sync_word;
    sync_word_length = 8;
  }
  // 设置同步字（还需要同时设置set_gfsk_packet_params）
  if (!SetGfskSyncWord(sync_word, sync_word_length)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetGfskSyncWord failed\n");
    return false;
  }

  uint16_t buffer_preamble_detector =
      std::min(static_cast<uint16_t>(sync_word_length * 8), preamble_length);
  if (buffer_preamble_detector >= 32) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength32bit;
  } else if (buffer_preamble_detector >= 24) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength24bit;
  } else if (buffer_preamble_detector >= 16) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength16bit;
  } else if (buffer_preamble_detector > 0) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength8bit;
  } else {
    param_.gfsk.preamble_detector = PreambleDetector::kLengthOff;
  }

  // 设置包的参数
  if (!SetGfskPacketParams(preamble_length, param_.gfsk.preamble_detector,
          sync_word_length * 8, AddrComp::kFilteringDisable,
          GfskHeaderType::kVariablePacket, param_.gfsk.payload_length, crc_type,
          Whitening::kNoEncoding)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }
  param_.gfsk.sync_word.data = sync_word;
  param_.gfsk.sync_word.length = sync_word_length;

  // 设置CRC（还需要同时设置set_gfsk_packet_params）
  if (!SetGfskCrc(crc_initial, crc_polynomial)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetGfskCrc failed\n");
    return false;
  }

  // 设置电流限制
  if (!SetCurrentLimit(current_limit)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetCurrentLimit failed\n");
    return false;
  }

  // 设置频率
  if (!SetFrequency(freq_mhz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetFrequency failed\n");
    return false;
  }

  // 设置功率
  if (!SetOutputPower(power)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetOutputPower failed\n");
    return false;
  }

  return true;
}

bool Sx126x::StartGfskTransmit(ChipMode chip_mode, uint32_t time_out_us,
    FallbackMode fallback_mode, uint16_t preamble_length) {
  // 从RX或TX模式退出返回的模式设定
  if (!SetRxTxFallbackMode(fallback_mode)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetRxTxFallbackMode failed\n");
    return false;
  }

  uint16_t buffer_preamble_detector = std::min(
      static_cast<uint16_t>(param_.gfsk.sync_word.length * 8), preamble_length);
  if (buffer_preamble_detector >= 32) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength32bit;
  } else if (buffer_preamble_detector >= 24) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength24bit;
  } else if (buffer_preamble_detector >= 16) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength16bit;
  } else if (buffer_preamble_detector > 0) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength8bit;
  } else {
    param_.gfsk.preamble_detector = PreambleDetector::kLengthOff;
  }

  // 设置包的参数
  if (!SetGfskPacketParams(preamble_length, param_.gfsk.preamble_detector,
          param_.gfsk.sync_word.length * 8, param_.gfsk.address_comparison,
          param_.gfsk.header_type, param_.gfsk.payload_length,
          param_.gfsk.crc.type, param_.gfsk.whitening)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }

  switch (chip_mode) {
    case ChipMode::kRx:
      // 设置为接收模式
      if (!SetRx(time_out_us)) {
        LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetRx failed\n");
        return false;
      }
      break;
    case ChipMode::kTx:
      // 设置为发送模式
      // if (SetTx(time_out_us))
      // {
      //     LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetRx failed\n");
      //     return false;
      // }
      break;

    default:
      break;
  }

  return true;
}

uint32_t Sx126x::GetGfskPacketStatus() {
  uint8_t buffer[4] = {0};

  CheckBusy();
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGetPacketStatus), buffer, 4)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  CmdStatus buffer_cs = ParseCmdStatus(buffer[0]);
  if ((buffer_cs != CmdStatus::kRfu) && (buffer_cs != CmdStatus::kCmdTxDone) &&
      (buffer_cs != CmdStatus::kDataIsAvailableToHost)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "ParseCmdStatus failed (error code: %#X)\n", buffer[0]);
    return -1;
  }

  return (static_cast<uint32_t>(buffer[1]) << 16) |
         (static_cast<uint32_t>(buffer[2]) << 8) |
         static_cast<uint32_t>(buffer[3]);
}

bool Sx126x::ParseGfskPacketStatus(
    uint32_t parse_status, GfskPacketStatus& status) {
  if (parse_status == static_cast<uint32_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  parse_status >>= 16;

  status.packet_send_done_flag = parse_status & 0B00000001;
  status.packet_receive_done_flag = (parse_status & 0B00000010) >> 1;
  status.abort_error_flag = (parse_status & 0B00000100) >> 2;
  status.length_error_flag = (parse_status & 0B00001000) >> 3;
  status.crc_error_flag = (parse_status & 0B00010000) >> 4;
  status.address_error_flag = (parse_status & 0B00100000) >> 5;
  status.sync_word_flag = (parse_status & 0B01000000) >> 6;
  status.preamble_error_flag = (parse_status & 0B10000000) >> 7;

  return true;
}

bool Sx126x::ParseGfskPacketMetrics(
    uint32_t parse_metrics, PacketMetrics& metrics) {
  if (parse_metrics == static_cast<uint32_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  const uint8_t buffer[2] = {
      static_cast<uint8_t>(
          (parse_metrics & 0B00000000000000001111111111111111) >> 8),
      static_cast<uint8_t>(parse_metrics),
  };

  metrics.gfsk.rssi_sync = -1.0 * static_cast<float>(buffer[0]) / 2.0;
  metrics.gfsk.rssi_average = -1.0 * static_cast<float>(buffer[1]) / 2.0;

  return true;
}

bool Sx126x::SetGfskSyncWordPacketParams(
    uint8_t* sync_word, uint8_t sync_word_length) {
  if ((sync_word == nullptr) || (sync_word_length == 0)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }
  // 设置同步字（还需要同时设置set_gfsk_packet_params）
  if (!SetGfskSyncWord(sync_word, sync_word_length)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetGfskSyncWord failed\n");
    return false;
  }

  uint16_t buffer_preamble_detector = std::min(
      static_cast<uint16_t>(sync_word_length * 8), param_.gfsk.preamble_length);
  if (buffer_preamble_detector >= 32) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength32bit;
  } else if (buffer_preamble_detector >= 24) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength24bit;
  } else if (buffer_preamble_detector >= 16) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength16bit;
  } else if (buffer_preamble_detector > 0) {
    param_.gfsk.preamble_detector = PreambleDetector::kLength8bit;
  } else {
    param_.gfsk.preamble_detector = PreambleDetector::kLengthOff;
  }

  // 设置包的参数
  if (!SetGfskPacketParams(param_.gfsk.preamble_length,
          param_.gfsk.preamble_detector, sync_word_length * 8,
          param_.gfsk.address_comparison, param_.gfsk.header_type,
          param_.gfsk.payload_length, param_.gfsk.crc.type,
          param_.gfsk.whitening)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }
  param_.gfsk.sync_word.data = sync_word;
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
        LogLevel::kChip, __FILE__, __LINE__, "SetGfskPacketParams failed\n");
    return false;
  }

  // 设置CRC（还需要同时设置set_gfsk_packet_params）
  if (!SetGfskCrc(crc_initial, crc_polynomial)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetGfskCrc failed\n");
    return false;
  }

  param_.gfsk.crc.type = crc_type;
  param_.gfsk.crc.initial = crc_initial;
  param_.gfsk.crc.polynomial = crc_polynomial;

  return true;
}

bool Sx126x::SetIrqPinMode(
    IrqMaskFlag dio1_mode, IrqMaskFlag dio2_mode, IrqMaskFlag dio3_mode) {
  // 设置接收中断标志
  // 默认设置 irq_mask：kRxDone, kTimeout, kCrcErr 和 kHeaderErr
  // 默认设置 diox_mask：kRxDone（包接收完成后中断）
  // 设置接收中断标志
  // 默认设置 irq_mask：kTxDone, kTimeout
  // 默认设置 diox_mask：kTxDone（包发送完成后中断）
  if (!SetDioIrqParams(0B0000001001100011, static_cast<uint16_t>(dio1_mode),
          static_cast<uint16_t>(dio2_mode), static_cast<uint16_t>(dio3_mode))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetDioIrqParams failed\n");
    return false;
  }

  return true;
}

bool Sx126x::ClearBuffer() {
  std::unique_ptr<uint8_t[]> buffer =
      std::make_unique<uint8_t[]>(kMaxTransmitBufferSize);

  std::memset(buffer.get(), 0, kMaxTransmitBufferSize);

  if (!WriteBuffer(buffer.get(), kMaxTransmitBufferSize, 0)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteBuffer failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetTxContinuousWave() {
  CheckBusy();
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kWoSetTxContinuousWave))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Sx126x::SetSleep(SleepMode mode) {
  CheckBusy();
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoSetSleep), static_cast<uint8_t>(mode))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 等待Sx126x进入睡眠
  DelayMs(10);

  return true;
}

}  // namespace cpp_bus_driver
