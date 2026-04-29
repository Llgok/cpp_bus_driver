/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-26 11:13:26
 * @LastEditTime: 2026-04-22 16:40:43
 * @License: GPL 3.0
 */
#include "aw862xx.h"

namespace cpp_bus_driver {
bool Aw862xx::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(10);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer == static_cast<uint8_t>(CPP_BUS_DRIVER_DEFAULT_VALUE)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get aw862xx id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get aw862xx id success (id: %#X)\n", buffer);
  }

  return true;
}

uint8_t Aw862xx::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return buffer;
}

bool Aw862xx::SoftwareReset() {
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoSrst), static_cast<uint8_t>(0xAA))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  return true;
}

float Aw862xx::GetInputVoltage() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl1), &buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[0] |= 0B00001000;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl1), buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDetcfg2), &buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[0] |= 0B00000010;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwDetcfg2), buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  DelayMs(3);

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl1), &buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[0] &= 0B11110111;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl1), buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDetVbat), &buffer[0])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDetLo), &buffer[1])) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[1] = (buffer[1] & 0B00110000) >> 4;

  return ((6.1 * (buffer[0] * 4 + buffer[1])) / 1024);
}

bool Aw862xx::SetPlayMode(PlayMode mode) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg3), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111100) | static_cast<uint8_t>(mode);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg3), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetGoFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg4), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111110) | 0B00000001;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg4), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

Aw862xx::GlobalStatus Aw862xx::GetGlobalStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGlbrd5), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return GlobalStatus::kFalse;
  }

  buffer &= 0B00001111;

  switch (buffer) {
    case static_cast<uint8_t>(GlobalStatus::kStandby):
      break;
    case static_cast<uint8_t>(GlobalStatus::kCont):
      break;
    case static_cast<uint8_t>(GlobalStatus::kRam):
      break;
    case static_cast<uint8_t>(GlobalStatus::kRtp):
      break;
    case static_cast<uint8_t>(GlobalStatus::kTrig):
      break;
    case static_cast<uint8_t>(GlobalStatus::kBrake):
      break;

    default:
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return GlobalStatus::kFalse;
  }

  return static_cast<GlobalStatus>(buffer);
}

bool Aw862xx::RunRtpPlaybackWaveform(
    const uint8_t* waveform_data, size_t length) {
  if (!SetPlayMode(PlayMode::kRtp)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetPlayMode failed\n");
    return false;
  }

  if (!SetGoFlag()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetGoFlag failed\n");
    return false;
  }

  // DelayUs(1000);

  uint8_t timeout_count_buffer = 0;
  while (1) {
    if (GetGlobalStatus() == GlobalStatus::kRtp) {
      break;
    }

    timeout_count_buffer++;
    if (timeout_count_buffer > 100) {
      LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Rtp mode switch timeout\n");
      return false;
    }
    DelayUs(1000);
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRtpdata), waveform_data, length)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetWaveformDataSampleRate(SampleRate rate) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111100) | static_cast<uint8_t>(rate);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetPlayingChangedGainBypass(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl7), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(enable) << 6);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl7), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetD2sGain(D2sGain gain) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl7), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11000000) | (static_cast<uint8_t>(gain));
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl7), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetLraOscFrequency(uint8_t freq_hz) {
  if (freq_hz > 63) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    freq_hz = 63;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwTrimcfg3), freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetF0DetectionMode(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg1), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enable) << 3);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg1), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetTrackSwitch(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg6), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg6), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetAutoBrakeStop(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg3), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(enable) << 2);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg3), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive1Level(uint8_t level) {
  if (level > 127) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    level = 127;
  }
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg6), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10000000) | level;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg6), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive2Level(uint8_t level) {
  if (level > 127) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    level = 127;
  }
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg7), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10000000) | level;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg7), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive1Times(uint8_t times) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg8), times)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive2Times(uint8_t times) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg9), times)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContTrackMargin(uint8_t value) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg11), value)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDriveWidth(uint8_t value) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg3), value)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint32_t Aw862xx::GetF0Detection() {
  if (!SetLraOscFrequency(0)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetLraOscFrequency failed\n");
    return -1;
  }

  if (!SetPlayMode(PlayMode::kCont)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetPlayMode failed\n");
    return -1;
  }

  if (!SetF0DetectionMode(true)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetF0DetectionMode failed\n");
    return -1;
  }

  if (!SetTrackSwitch(true)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetTrackSwitch failed\n");
    return -1;
  }

  if (!SetAutoBrakeStop(true)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetAutoBrakeStop failed\n");
    return -1;
  }

  if (!SetContDrive1Level(127)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "set_cont_drive_1_level failed\n");
    return -1;
  }

  uint8_t cont_drive_2_level_buffer =
      ((f0_value_ < 1800) ? 1809920 : 1990912) / 1000 * 61000000;

  if (!SetContDrive1Level(cont_drive_2_level_buffer)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "set_cont_drive_1_level failed\n");
    return -1;
  }

  if (!SetContDrive1Times(4)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "set_cont_drive_1_times failed\n");
    return -1;
  }

  if (!SetContDrive2Times(6)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "set_cont_drive_2_times failed\n");
    return -1;
  }

  if (!SetContTrackMargin(15)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetContTrackMargin failed\n");
    return -1;
  }

  int32_t cont_drive_width_buffer = 240000 / f0_value_ - 8 - 8 - 15;

  if (cont_drive_width_buffer < 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    cont_drive_width_buffer = 0;
  } else if (cont_drive_width_buffer > 255) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    cont_drive_width_buffer = 255;
  }

  if (!SetContDriveWidth(cont_drive_width_buffer)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetContDriveWidth failed\n");
    return -1;
  }

  if (!SetGoFlag()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetGoFlag failed\n");
    return -1;
  }

  DelayMs(300);

  uint8_t buffer[2] = {0};

  for (uint8_t i = 0; i < 2; i++) {
    if (!bus_->Read(
            static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRoContrd14) + i),
            &buffer[i])) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return -1;
    }
  }

  if (!SetF0DetectionMode(false)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetF0DetectionMode failed\n");
    return -1;
  }

  if (!SetAutoBrakeStop(false)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetAutoBrakeStop failed\n");
    return -1;
  }

  return 384000 * 10 / ((static_cast<uint32_t>(buffer[0]) << 8) | buffer[1]);
}

bool Aw862xx::SetF0Calibrate(uint32_t f0_value) {
  uint32_t f0_calibrate_value_min_buffer = f0_value_ * 93 / 100;
  uint32_t f0_calibrate_value_max_buffer = f0_value_ * 107 / 100;

  if (f0_value < f0_calibrate_value_min_buffer ||
      f0_value > f0_calibrate_value_max_buffer) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetF0Calibrate failed\n");
    return false;
  }

  int32_t f0_calibrate_step_buffer =
      100000 *
      ((static_cast<int32_t>(f0_value) - static_cast<int32_t>(f0_value_))) /
      ((static_cast<int32_t>(f0_value_) * 24));

  if (f0_calibrate_step_buffer >= 0) {
    if (f0_calibrate_step_buffer % 10 >= 5) {
      f0_calibrate_step_buffer = 32 + (f0_calibrate_step_buffer / 10 + 1);
    } else {
      f0_calibrate_step_buffer = 32 + f0_calibrate_step_buffer / 10;
    }
  } else {
    if (f0_calibrate_step_buffer % 10 <= -5) {
      f0_calibrate_step_buffer = 32 + (f0_calibrate_step_buffer / 10 - 1);
    } else {
      f0_calibrate_step_buffer = 32 + f0_calibrate_step_buffer / 10;
    }
  }

  int8_t f0_calibrate_lra_buffer = 0;

  if (f0_calibrate_step_buffer > 31) {
    f0_calibrate_lra_buffer =
        static_cast<int8_t>(f0_calibrate_step_buffer) - 32;
  } else {
    f0_calibrate_lra_buffer =
        static_cast<int8_t>(f0_calibrate_step_buffer) + 32;
  }

  if (!SetLraOscFrequency(f0_calibrate_lra_buffer)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetLraOscFrequency failed\n");
    return false;
  }

  f0_value_ = f0_value;

  return true;
}

bool Aw862xx::GetSystemStatus(SystemStatus& status) {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoSysst), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  status.uvls_flag = (buffer & 0B00100000) > 5;
  status.rtp_fifo_empty = (buffer & 0B00010000) > 4;
  status.rtp_fifo_full = (buffer & 0B00001000) > 3;
  status.over_current_flag = (buffer & 0B00000100) > 2;
  status.over_temperature_flag = (buffer & 0B00000010) > 1;
  status.playback_flag = buffer & 0B00000001;

  return true;
}

bool Aw862xx::SetClock(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl1), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enable) << 3);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl1), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetRrtModeGain(uint8_t gain) {
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPlaycfg2), static_cast<uint8_t>(gain))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::InitRamMode(const uint8_t* waveform_data, size_t length) {
  if (!SetClock(true)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetClock failed\n");
    return false;
  }

  DelayUs(1000);

  uint8_t buffer = 0;
  // 默认使用2KB kFifo，1KB RAM波形储存空间
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwRtpcfg1), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110000) | 0x08;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRtpcfg1), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRtpcfg2), static_cast<uint8_t>(0))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRamaddrh), static_cast<uint8_t>(0x08))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRamaddrl), static_cast<uint8_t>(0))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 写入RAM波形数据，上电时只需写入一次
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRamadata), waveform_data, length)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!SetClock(false)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetClock failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::RunRamPlaybackWaveform(uint8_t waveform_sequence_number,
    uint8_t waveform_playback_count, uint8_t gain, bool auto_brake,
    bool gain_bypass) {
  if (waveform_playback_count > 15) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    waveform_playback_count = 15;
  }

  // 设置RAM #x(x=waveform_sequence_number) waveform
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwWavcfg1),
          static_cast<uint8_t>(waveform_sequence_number))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 设置停止
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwWavcfg2), static_cast<uint8_t>(0))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  uint8_t buffer = 0;
  // 设置播放次数（15为无限循环）
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwWavcfg9), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B00001111) |
           (static_cast<uint8_t>(waveform_playback_count) << 4);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwWavcfg9), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!SetPlayMode(PlayMode::kRam)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetPlayMode failed\n");
    return false;
  }

  // 自动制动
  if (!SetAutoBrakeStop(auto_brake)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetAutoBrakeStop failed\n");
    return false;
  }

  if (!SetPlayingChangedGainBypass(gain_bypass)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "SetPlayingChangedGainBypass failed\n");
    return false;
  }

  if (!SetRrtModeGain(gain)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetRrtModeGain failed\n");
    return false;
  }

  // 开始播放
  if (!SetGoFlag()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetGoFlag failed\n");
    return false;
  }
  return true;
}

bool Aw862xx::SetStopFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg4), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111101) | 0B00000010;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg4), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetForceEnterMode(ForceMode mode, bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl2), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  switch (mode) {
    case ForceMode::kActive:
      buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
      break;
    case ForceMode::kStandby:
      buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(enable) << 6);
      break;

    default:
      break;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl2), buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::StopRamPlaybackWaveform() {
  // 停止播放
  if (!SetStopFlag()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetStopFlag failed\n");
    return false;
  }

  if (!SetForceEnterMode(ForceMode::kStandby, true)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetForceEnterMode failed\n");
    return false;
  }

  uint8_t timeout_count_buffer = 0;
  while (1) {
    if (GetGlobalStatus() == GlobalStatus::kStandby) {
      break;
    }

    timeout_count_buffer++;
    if (timeout_count_buffer > 100) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__,
          "Force enter standby mode timeout\n");

      if (!SetForceEnterMode(ForceMode::kStandby, false)) {
        LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "SetForceEnterMode failed\n");
        return false;
      }

      return false;
    }

    DelayUs(1000);
  }

  if (!SetForceEnterMode(ForceMode::kStandby, false)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetForceEnterMode failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
