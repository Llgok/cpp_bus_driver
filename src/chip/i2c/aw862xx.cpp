/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-26 11:13:26
 * @LastEditTime: 2026-07-01 14:22:46
 * @License: GPL 3.0
 */
#include "aw862xx.h"

namespace cpp_bus_driver {

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
constexpr uint8_t Aw862xx::kHapticWaveformRtpData[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k101635_130[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k0809_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k0815_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k9595_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k0832_205[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k0832_235[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k041230_235[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k041235_240[];
constexpr uint8_t Aw862xx::kHapticWaveformRam12k0832_260[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k101635_130[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k0619_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k0809_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k0815_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k1010_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k1040_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k9595_170[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k0832_205[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k0832_235[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k041230_235[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k041235_240[];
constexpr uint8_t Aw862xx::kHapticWaveformRam24k0832_260[];
#endif

Aw862xx::RamWaveformInfo Aw862xx::GetRamWaveformInfo(
    RamWaveformLibrary library) {
  RamWaveformInfo info;

  switch (library) {
    case RamWaveformLibrary::kRam12k101635_130:
      info = {"12k_101635_130", kHapticWaveformRam12k101635_130,
          sizeof(kHapticWaveformRam12k101635_130), SampleRate::kRate12Khz, 130,
          0};
      break;
    case RamWaveformLibrary::kRam12k0809_170:
      info = {"12k_0809_170", kHapticWaveformRam12k0809_170,
          sizeof(kHapticWaveformRam12k0809_170), SampleRate::kRate12Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam12k0815_170:
      info = {"12k_0815_170", kHapticWaveformRam12k0815_170,
          sizeof(kHapticWaveformRam12k0815_170), SampleRate::kRate12Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam12k9595_170:
      info = {"12k_9595_170", kHapticWaveformRam12k9595_170,
          sizeof(kHapticWaveformRam12k9595_170), SampleRate::kRate12Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam12k0832_205:
      info = {"12k_0832_205", kHapticWaveformRam12k0832_205,
          sizeof(kHapticWaveformRam12k0832_205), SampleRate::kRate12Khz, 205,
          0};
      break;
    case RamWaveformLibrary::kRam12k0832_235:
      info = {"12k_0832_235", kHapticWaveformRam12k0832_235,
          sizeof(kHapticWaveformRam12k0832_235), SampleRate::kRate12Khz, 235,
          0};
      break;
    case RamWaveformLibrary::kRam12k041230_235:
      info = {"12k_041230_235", kHapticWaveformRam12k041230_235,
          sizeof(kHapticWaveformRam12k041230_235), SampleRate::kRate12Khz, 235,
          0};
      break;
    case RamWaveformLibrary::kRam12k041235_240:
      info = {"12k_041235_240", kHapticWaveformRam12k041235_240,
          sizeof(kHapticWaveformRam12k041235_240), SampleRate::kRate12Khz, 240,
          0};
      break;
    case RamWaveformLibrary::kRam12k0832_260:
      info = {"12k_0832_260", kHapticWaveformRam12k0832_260,
          sizeof(kHapticWaveformRam12k0832_260), SampleRate::kRate12Khz, 260,
          0};
      break;
    case RamWaveformLibrary::kRam24k101635_130:
      info = {"24k_101635_130", kHapticWaveformRam24k101635_130,
          sizeof(kHapticWaveformRam24k101635_130), SampleRate::kRate24Khz, 130,
          0};
      break;
    case RamWaveformLibrary::kRam24k0619_170:
      info = {"24k_0619_170", kHapticWaveformRam24k0619_170,
          sizeof(kHapticWaveformRam24k0619_170), SampleRate::kRate24Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam24k0809_170:
      info = {"24k_0809_170", kHapticWaveformRam24k0809_170,
          sizeof(kHapticWaveformRam24k0809_170), SampleRate::kRate24Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam24k0815_170:
      info = {"24k_0815_170", kHapticWaveformRam24k0815_170,
          sizeof(kHapticWaveformRam24k0815_170), SampleRate::kRate24Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam24k1010_170:
      info = {"24k_1010_170", kHapticWaveformRam24k1010_170,
          sizeof(kHapticWaveformRam24k1010_170), SampleRate::kRate24Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam24k1040_170:
      info = {"24k_1040_170", kHapticWaveformRam24k1040_170,
          sizeof(kHapticWaveformRam24k1040_170), SampleRate::kRate24Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam24k9595_170:
      info = {"24k_9595_170", kHapticWaveformRam24k9595_170,
          sizeof(kHapticWaveformRam24k9595_170), SampleRate::kRate24Khz, 170,
          0};
      break;
    case RamWaveformLibrary::kRam24k0832_205:
      info = {"24k_0832_205", kHapticWaveformRam24k0832_205,
          sizeof(kHapticWaveformRam24k0832_205), SampleRate::kRate24Khz, 205,
          0};
      break;
    case RamWaveformLibrary::kRam24k0832_235:
      info = {"24k_0832_235", kHapticWaveformRam24k0832_235,
          sizeof(kHapticWaveformRam24k0832_235), SampleRate::kRate24Khz, 235,
          0};
      break;
    case RamWaveformLibrary::kRam24k041230_235:
      info = {"24k_041230_235", kHapticWaveformRam24k041230_235,
          sizeof(kHapticWaveformRam24k041230_235), SampleRate::kRate24Khz, 235,
          0};
      break;
    case RamWaveformLibrary::kRam24k041235_240:
      info = {"24k_041235_240", kHapticWaveformRam24k041235_240,
          sizeof(kHapticWaveformRam24k041235_240), SampleRate::kRate24Khz, 240,
          0};
      break;
    case RamWaveformLibrary::kRam24k0832_260:
      info = {"24k_0832_260", kHapticWaveformRam24k0832_260,
          sizeof(kHapticWaveformRam24k0832_260), SampleRate::kRate24Khz, 260,
          0};
      break;
    default:
      return info;
  }

  info.waveform_count = GetRamWaveformCount(info.data, info.length);
  return info;
}

const char* Aw862xx::ChipTypeToString(ChipType chip_type) {
  switch (chip_type) {
    case ChipType::kAw8623:
      return "AW8623";
    case ChipType::kAw8624:
      return "AW8624";
    case ChipType::kAw86214:
      return "AW86214";
    case ChipType::kAw86223:
      return "AW86223";
    case ChipType::kAw86224:
      return "AW86224";
    case ChipType::kAw86225:
      return "AW86225";
    case ChipType::kAw86233:
      return "AW86233";
    case ChipType::kAw86234:
      return "AW86234";
    case ChipType::kAw86235:
      return "AW86235";
    case ChipType::kAw86243:
      return "AW86243";
    case ChipType::kAw86245:
      return "AW86245";
    case ChipType::kUnknown:
    default:
      return "Unknown";
  }
}

const char* Aw862xx::SampleRateToString(SampleRate sample_rate) {
  switch (sample_rate) {
    case SampleRate::kRate12Khz:
      return "12kHz";
    case SampleRate::kRate24Khz:
      return "24kHz";
    case SampleRate::kRate48Khz:
      return "48kHz";
    default:
      return "Unknown";
  }
}

bool Aw862xx::Init(int32_t freq_hz) {
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

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  // GetDeviceId
  // 有可能会在正常工作状态下读取到芯片ID失败，导致无法识别芯片类型，所以这边直接读取寄存器来判断芯片是否存在
  // const ChipType chip_type = GetDeviceId();
  // if (chip_type == ChipType::kUnknown) {
  //   LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
  //       "Get aw862xx id failed (chip: %s)\n", ChipTypeToString(chip_type));
  //   return false;
  // }

  // LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Get %s id success\n",
  //     ChipTypeToString(chip_type));

  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Get aw862xx id failed\n");
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Get aw862xx id success\n");

  return true;
}

bool Aw862xx::Deinit(bool delete_bus) {
  bool result = true;

  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }
  return result;
}

Aw862xx::ChipType Aw862xx::GetDeviceId() {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoDeviceId), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    chip_type_ = ChipType::kUnknown;
    return chip_type_;
  }

  chip_type_ = ChipType::kUnknown;

  uint8_t chip_id_high = 0;
  if (bus_->Read(static_cast<uint8_t>(Cmd::kRoChipIdHigh), &chip_id_high)) {
    if (chip_id_high == 0x23) {
      uint8_t chip_id_low = 0;
      if (bus_->Read(
              static_cast<uint8_t>(Cmd::kRoAw8623xChipIdLow), &chip_id_low)) {
        switch ((static_cast<uint16_t>(chip_id_high) << 8) | chip_id_low) {
          case 0x2330:
            chip_type_ = ChipType::kAw86233;
            return chip_type_;
          case 0x2340:
            chip_type_ = ChipType::kAw86234;
            return chip_type_;
          case 0x2350:
            chip_type_ = ChipType::kAw86235;
            return chip_type_;

          default:
            break;
        }
      }
    }

    if (chip_id_high == 0x24) {
      uint8_t chip_id_low = 0;
      if (bus_->Read(
              static_cast<uint8_t>(Cmd::kRoAw8624xChipIdLow), &chip_id_low)) {
        switch ((static_cast<uint16_t>(chip_id_high) << 8) | chip_id_low) {
          case 0x2430:
            chip_type_ = ChipType::kAw86243;
            return chip_type_;
          case 0x2450:
            chip_type_ = ChipType::kAw86245;
            return chip_type_;

          default:
            chip_type_ = ChipType::kAw8624;
            return chip_type_;
        }
      }
    }
  }

  if (buffer == 0x24) {
    chip_type_ = ChipType::kAw8624;
    return chip_type_;
  }

  if (buffer == 0x23) {
    chip_type_ = ChipType::kAw8623;
    return chip_type_;
  }

  if (buffer == 0x00) {
    uint8_t ef_id = 0;
    if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoEfId), &ef_id)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read ef id failed\n");
      chip_type_ = ChipType::kUnknown;
      return chip_type_;
    }

    switch (ef_id & 0x41) {
      case 0x00:
        chip_type_ = ChipType::kAw86224;
        break;
      case 0x01:
        chip_type_ = ChipType::kAw86223;
        break;
      case 0x41:
        chip_type_ = ChipType::kAw86214;
        break;

      default:
        chip_type_ = ChipType::kUnknown;
        break;
    }
    return chip_type_;
  }

  if (buffer == 0x01) {
    uint8_t ef_id = 0;
    if (bus_->Read(static_cast<uint8_t>(Cmd::kRoEfId), &ef_id) &&
        ((ef_id & 0x41) == 0x41)) {
      chip_type_ = ChipType::kAw86214;
    }
  }

  return chip_type_;
}

bool Aw862xx::SoftwareReset() {
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kWoSrst), static_cast<uint8_t>(0xAA))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  return true;
}

float Aw862xx::GetInputVoltage() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl1), &buffer[0])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[0] |= 0B00001000;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl1), buffer[0])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDetcfg2), &buffer[0])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[0] |= 0B00000010;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwDetcfg2), buffer[0])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  DelayMs(3);

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl1), &buffer[0])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[0] &= 0B11110111;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl1), buffer[0])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDetVbat), &buffer[0])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwDetLo), &buffer[1])) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }
  buffer[1] = (buffer[1] & 0B00110000) >> 4;

  return ((6.1 * (buffer[0] * 4 + buffer[1])) / 1024);
}

bool Aw862xx::SetPlayMode(PlayMode mode) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg3), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111100) | static_cast<uint8_t>(mode);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg3), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetGoFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg4), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111110) | 0B00000001;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg4), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

Aw862xx::GlobalStatus Aw862xx::GetGlobalStatus() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoGlbrd5), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
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
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
      return GlobalStatus::kFalse;
  }

  return static_cast<GlobalStatus>(buffer);
}

bool Aw862xx::RunRtpPlaybackWaveform(
    const uint8_t* waveform_data, size_t length) {
  if (!SetPlayMode(PlayMode::kRtp)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetPlayMode failed\n");
    return false;
  }

  if (!SetGoFlag()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetGoFlag failed\n");
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
          LogLevel::kError, __FILE__, __LINE__, "Rtp mode switch timeout\n");
      return false;
    }
    DelayUs(1000);
  }

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRtpdata), waveform_data, length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetWaveformDataSampleRate(SampleRate rate) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl2), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111100) | static_cast<uint8_t>(rate);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl2), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetPlayingChangedGainBypass(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl7), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(enable) << 6);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl7), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetD2sGain(D2sGain gain) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl7), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11000000) | (static_cast<uint8_t>(gain));
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl7), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetLraOscFrequency(uint8_t freq_hz) {
  if (freq_hz > 63) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    freq_hz = 63;
  }
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwTrimcfg3), freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetF0DetectionMode(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg1), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enable) << 3);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg1), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetTrackSwitch(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg6), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg6), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetAutoBrakeStop(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg3), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(enable) << 2);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg3), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive1Level(uint8_t level) {
  if (level > 127) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    level = 127;
  }
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg6), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10000000) | level;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg6), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive2Level(uint8_t level) {
  if (level > 127) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    level = 127;
  }
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwContcfg7), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10000000) | level;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg7), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive1Times(uint8_t times) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg8), times)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDrive2Times(uint8_t times) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg9), times)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContTrackMargin(uint8_t value) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg11), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetContDriveWidth(uint8_t value) {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwContcfg3), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint32_t Aw862xx::GetF0Detection() {
  static constexpr uint32_t kLraVrmsMv = 1000;
  static constexpr uint8_t kContDrv1Level = 0x7F;
  static constexpr uint8_t kContDrv1Time = 0x04;
  static constexpr uint8_t kContDrv2Time = 0x14;
  static constexpr uint8_t kContTrackMargin = 0x0F;
  static constexpr uint8_t kContBrakeGain = 0x08;

  // 使用f0校验必须进行一次软件复位保证稳定触发校验流程
  if (!SoftwareReset()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SoftwareReset failed\n");
    return -1;
  }

  if (!SetLraOscFrequency(0)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetLraOscFrequency failed\n");
    return -1;
  }

  if (!SetPlayMode(PlayMode::kCont)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetPlayMode failed\n");
    return -1;
  }

  if (!SetF0DetectionMode(true)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetF0DetectionMode failed\n");
    return -1;
  }

  if (!SetTrackSwitch(true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetTrackSwitch failed\n");
    return -1;
  }

  if (!SetAutoBrakeStop(true)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetAutoBrakeStop failed\n");
    return -1;
  }

  if (!SetContDrive1Level(kContDrv1Level)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "set_cont_drive_1_level failed\n");
    return -1;
  }

  // 使用原厂AW862XX_DRV2_LVL_FORMULA(f0, vrms)公式。
  uint32_t cont_drive_2_level_buffer =
      ((f0_value_ < 1800) ? 1809920 : 1990912) / 1000 * kLraVrmsMv / 61000;
  if (cont_drive_2_level_buffer > 127) {
    cont_drive_2_level_buffer = 127;
  }

  if (!SetContDrive2Level(static_cast<uint8_t>(cont_drive_2_level_buffer))) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "set_cont_drive_2_level failed\n");
    return -1;
  }

  if (!SetContDrive1Times(kContDrv1Time)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "set_cont_drive_1_times failed\n");
    return -1;
  }

  if (!SetContDrive2Times(kContDrv2Time)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "set_cont_drive_2_times failed\n");
    return -1;
  }

  if (!SetContTrackMargin(kContTrackMargin)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetContTrackMargin failed\n");
    return -1;
  }

  int32_t cont_drive_width_buffer =
      240000 / f0_value_ - kContTrackMargin - kContBrakeGain - 8;

  if (cont_drive_width_buffer < 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    cont_drive_width_buffer = 0;
  } else if (cont_drive_width_buffer > 255) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    cont_drive_width_buffer = 255;
  }

  if (!SetContDriveWidth(cont_drive_width_buffer)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetContDriveWidth failed\n");
    return -1;
  }

  if (!SetGoFlag()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetGoFlag failed\n");
    return -1;
  }

  DelayMs(300);

  uint8_t buffer[2] = {0};

  for (uint8_t i = 0; i < 2; i++) {
    if (!bus_->Read(
            static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRoContrd14) + i),
            &buffer[i])) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
      return -1;
    }
  }

  if (!SetF0DetectionMode(false)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetF0DetectionMode failed\n");
    return -1;
  }

  if (!SetAutoBrakeStop(false)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetAutoBrakeStop failed\n");
    return -1;
  }

  return 384000 * 10 / ((static_cast<uint32_t>(buffer[0]) << 8) | buffer[1]);
}

bool Aw862xx::SetF0Preset(uint32_t f0_0p1_hz) {
  if (f0_0p1_hz == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  f0_value_ = f0_0p1_hz;
  return true;
}

bool Aw862xx::SetF0Calibrate(uint32_t f0_value) {
  uint32_t f0_calibrate_value_min_buffer = f0_value_ * 93 / 100;
  uint32_t f0_calibrate_value_max_buffer = f0_value_ * 107 / 100;

  if (f0_value < f0_calibrate_value_min_buffer ||
      f0_value > f0_calibrate_value_max_buffer) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetF0Calibrate failed\n");
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
        LogLevel::kError, __FILE__, __LINE__, "SetLraOscFrequency failed\n");
    return false;
  }

  f0_value_ = f0_value;

  return true;
}

bool Aw862xx::GetSystemStatus(SystemStatus& status) {
  uint8_t buffer = 0;

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoSysst), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  status.uvls_flag = (buffer & 0B00100000) != 0;
  status.rtp_fifo_empty = (buffer & 0B00010000) != 0;
  status.rtp_fifo_full = (buffer & 0B00001000) != 0;
  status.over_current_flag = (buffer & 0B00000100) != 0;
  status.over_temperature_flag = (buffer & 0B00000010) != 0;
  status.playback_flag = (buffer & 0B00000001) != 0;

  return true;
}

bool Aw862xx::SetClock(bool enable) { return SetRamInit(enable); }

bool Aw862xx::SetRamInit(bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl1), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enable) << 3);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwSysctrl1), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetRrtModeGain(uint8_t gain) {
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPlaycfg2), static_cast<uint8_t>(gain))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::ParseRamHeader(const uint8_t* waveform_data, size_t length,
    uint16_t& base_addr, uint8_t& waveform_count) {
  base_addr = 0;
  waveform_count = 0;

  if (waveform_data == nullptr || length < 9) {
    return false;
  }

  uint8_t ram_num = 1;
  for (size_t i = 3; (i + 3) < length && ram_num < 8; i += 4) {
    const uint16_t last_end =
        (static_cast<uint16_t>(waveform_data[i]) << 8) | waveform_data[i + 1];
    const uint16_t next_start =
        (static_cast<uint16_t>(waveform_data[i + 2]) << 8) |
        waveform_data[i + 3];

    if ((next_start - last_end) == 1) {
      ram_num++;
    } else {
      break;
    }
  }

  const uint16_t first_wave_addr =
      (static_cast<uint16_t>(waveform_data[1]) << 8) | waveform_data[2];
  const uint8_t header_waveform_count = ram_num;

  while (ram_num > 0) {
    const size_t table_end_index = static_cast<size_t>(ram_num) * 4;
    if (table_end_index >= length) {
      ram_num--;
      continue;
    }

    const uint16_t last_wave_end =
        (static_cast<uint16_t>(waveform_data[table_end_index - 1]) << 8) |
        waveform_data[table_end_index];
    const uint16_t candidate_base = first_wave_addr - ram_num * 4 - 1;

    if ((last_wave_end >= candidate_base) &&
        (static_cast<size_t>(last_wave_end - candidate_base + 1) == length)) {
      base_addr = candidate_base;
      waveform_count = ram_num;
      return true;
    }

    ram_num--;
  }

  // 部分官方波形库的地址表有效，但最后一个结束地址不会覆盖完整数据长度。
  // 这里按官方 first_wave_addr 推导方式回退解析 base 地址和 sequence 数量。
  const uint16_t header_bytes =
      static_cast<uint16_t>(header_waveform_count) * 4 + 1;
  if (first_wave_addr > header_bytes &&
      static_cast<size_t>(header_bytes) < length) {
    base_addr = first_wave_addr - header_bytes;
    waveform_count = header_waveform_count;
    return true;
  }

  return false;
}

uint8_t Aw862xx::GetRamWaveformCount(
    const uint8_t* waveform_data, size_t length) {
  uint16_t base_addr = 0;
  uint8_t waveform_count = 0;
  if (!ParseRamHeader(waveform_data, length, base_addr, waveform_count)) {
    return 0;
  }
  return waveform_count;
}

bool Aw862xx::SetRamBaseAddress(uint16_t base_addr) {
  // RTPCFG1[3:0]和RTPCFG2用于保存RAM base地址。
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwRtpcfg1), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  buffer = (buffer & 0xF0) | static_cast<uint8_t>((base_addr >> 8) & 0x0F);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRtpcfg1), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRtpcfg2),
          static_cast<uint8_t>(base_addr & 0xFF))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetRamFifoThreshold(uint16_t base_addr) {
  // 原厂FIFO阈值公式：AE = base / 2，AF = base - base / 4。
  const uint16_t almost_empty = base_addr >> 1;
  const uint16_t almost_full = base_addr - (base_addr >> 2);
  const uint8_t buffer[3] = {
      static_cast<uint8_t>(
          (((almost_empty >> 8) << 4) & 0xF0) | ((almost_full >> 8) & 0x0F)),
      static_cast<uint8_t>(almost_empty & 0xFF),
      static_cast<uint8_t>(almost_full & 0xFF),
  };

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRtpcfg3), buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetRamAddress(uint16_t ram_addr) {
  const uint8_t buffer[2] = {static_cast<uint8_t>(ram_addr >> 8),
      static_cast<uint8_t>(ram_addr & 0xFF)};

  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRamaddrh), buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::InitRamMode(const uint8_t* waveform_data, size_t length) {
  RamWaveformInfo waveform_info;
  if (ram_waveform_info_.data == waveform_data &&
      ram_waveform_info_.length == length) {
    waveform_info = ram_waveform_info_;
  }
  ram_waveform_info_ = {};

  uint16_t base_addr = 0;
  uint8_t waveform_count = 0;
  if (!ParseRamHeader(waveform_data, length, base_addr, waveform_count)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid AW862xx RAM waveform container\n");
    return false;
  }

  // container_update流程：停止播放、进入raminit、配置地址/FIFO、
  // 写入RAM数据，然后退出raminit。
  if (!StopRamPlaybackWaveform()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "StopRamPlaybackWaveform failed\n");
    return false;
  }

  if (!SetRamInit(true)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRamInit failed\n");
    return false;
  }

  DelayUs(500);

  if (!SetRamBaseAddress(base_addr)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetRamBaseAddress failed\n");
    return false;
  }

  if (!SetRamFifoThreshold(base_addr)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetRamFifoThreshold failed\n");
    return false;
  }

  if (!SetRamAddress(base_addr)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRamAddress failed\n");
    return false;
  }

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  // 写入RAM波形数据，上电时只需写入一次。Arduino nRF 的 Wire
  // 默认发送缓存为64字节，环形缓冲可用63字节，扣除1字节寄存器地址。
  constexpr size_t kRamWriteChunkSize = 62;
  size_t offset = 0;
  while (offset < length) {
    const size_t chunk_length = std::min(kRamWriteChunkSize, length - offset);
    if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRamadata),
            waveform_data + offset, chunk_length)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "Write ram data failed (offset: %u size: %u)\n",
          static_cast<unsigned int>(offset),
          static_cast<unsigned int>(chunk_length));
      return false;
    }
    offset += chunk_length;
  }
#else
  // 写入RAM波形数据，上电时只需写入一次。
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwRamadata), waveform_data, length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
#endif

  if (!SetRamInit(false)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRamInit failed\n");
    return false;
  }

  waveform_info.data = waveform_data;
  waveform_info.length = length;
  waveform_info.waveform_count = waveform_count;
  ram_waveform_info_ = waveform_info;
  const bool has_waveform_info = ram_waveform_info_.name != nullptr;

  if (!has_waveform_info) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Unknown AW862xx RAM waveform\n");
    return true;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "AW862xx RAM waveform loaded: %s, sample %s, rated F0 %uHz, "
      "base 0x%04X, %u sequences, %u bytes\n",
      ram_waveform_info_.name,
      SampleRateToString(ram_waveform_info_.sample_rate),
      static_cast<unsigned int>(ram_waveform_info_.rated_f0_hz), base_addr,
      static_cast<unsigned int>(ram_waveform_info_.waveform_count),
      static_cast<unsigned int>(ram_waveform_info_.length));

  return true;
}

bool Aw862xx::InitRamMode(RamWaveformLibrary library) {
  const RamWaveformInfo info = GetRamWaveformInfo(library);
  if (info.data == nullptr || info.length == 0 || info.waveform_count == 0) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Invalid AW862xx RAM waveform library\n");
    return false;
  }

  if (!SetWaveformDataSampleRate(info.sample_rate)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetWaveformDataSampleRate failed\n");
    return false;
  }

  ram_waveform_info_ = info;
  if (!InitRamMode(info.data, info.length)) {
    return false;
  }

  return true;
}

bool Aw862xx::SetRamWaveformSequence(
    uint8_t slot, uint8_t waveform_sequence_number) {
  if (slot >= 8 || waveform_sequence_number > 127) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  if (!bus_->Write(
          static_cast<uint8_t>(static_cast<uint8_t>(Cmd::kRwWavcfg1) + slot),
          waveform_sequence_number)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetRamWaveformLoop(uint8_t slot, uint8_t loop_count) {
  if (slot >= 8) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  if (loop_count > 15) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    loop_count = 15;
  }

  uint8_t buffer = 0;
  const uint8_t loop_reg = static_cast<uint8_t>(Cmd::kRwWavcfg9) + (slot / 2);
  if (!bus_->Read(loop_reg, &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  if (slot % 2) {
    buffer = (buffer & 0xF0) | (loop_count & 0x0F);
  } else {
    buffer = (buffer & 0x0F) | static_cast<uint8_t>((loop_count & 0x0F) << 4);
  }

  if (!bus_->Write(loop_reg, buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::PlayRamWaveform(uint8_t waveform_sequence_number,
    uint8_t loop_count, uint8_t gain, bool auto_brake, bool gain_bypass) {
  if (loop_count == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    loop_count = 1;
  }

  return RunRamPlaybackWaveform(
      waveform_sequence_number, loop_count - 1, gain, auto_brake, gain_bypass);
}

bool Aw862xx::PlayRamLoopWaveform(uint8_t waveform_sequence_number,
    uint8_t gain, bool auto_brake, bool gain_bypass) {
  return RunRamPlaybackWaveform(
      waveform_sequence_number, 15, gain, auto_brake, gain_bypass);
}

bool Aw862xx::RunRamPlaybackWaveform(uint8_t waveform_sequence_number,
    uint8_t loop_count, uint8_t gain, bool auto_brake, bool gain_bypass) {
  if (!ConfigureRamPlaybackWaveform(waveform_sequence_number, loop_count, gain,
          auto_brake, gain_bypass)) {
    return false;
  }
  return StartRamPlaybackWaveform();
}

bool Aw862xx::ConfigureRamPlaybackWaveform(uint8_t waveform_sequence_number,
    uint8_t loop_count, uint8_t gain, bool auto_brake, bool gain_bypass) {
  if (waveform_sequence_number == 0 ||
      waveform_sequence_number > ram_waveform_info_.waveform_count) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  if (loop_count > 15) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    loop_count = 15;
  }

  // 设置RAM #x(x=waveform_sequence_number) waveform
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwWavcfg1),
          static_cast<uint8_t>(waveform_sequence_number))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 设置停止
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRwWavcfg2), static_cast<uint8_t>(0))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  uint8_t buffer = 0;
  // 设置播放次数（15为无限循环）
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwWavcfg9), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B00001111) | (static_cast<uint8_t>(loop_count) << 4);
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwWavcfg9), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!SetPlayMode(PlayMode::kRam)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetPlayMode failed\n");
    return false;
  }

  // 自动制动
  if (!SetAutoBrakeStop(auto_brake)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetAutoBrakeStop failed\n");
    return false;
  }

  if (!SetPlayingChangedGainBypass(gain_bypass)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "SetPlayingChangedGainBypass failed\n");
    return false;
  }

  if (!SetRrtModeGain(gain)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetRrtModeGain failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::StartRamPlaybackWaveform() {
  if (!SetGoFlag()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetGoFlag failed\n");
    return false;
  }
  return true;
}

bool Aw862xx::SetStopFlag() {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwPlaycfg4), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111101) | 0B00000010;
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwPlaycfg4), buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::SetForceEnterMode(ForceMode mode, bool enable) {
  uint8_t buffer = 0;
  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwSysctrl2), &buffer)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
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
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw862xx::StopRamPlaybackWaveform() {
  // 停止播放
  if (!SetStopFlag()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetStopFlag failed\n");
    return false;
  }

  if (!SetForceEnterMode(ForceMode::kStandby, true)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetForceEnterMode failed\n");
    return false;
  }

  uint8_t timeout_count_buffer = 0;
  while (1) {
    if (GetGlobalStatus() == GlobalStatus::kStandby) {
      break;
    }

    timeout_count_buffer++;
    if (timeout_count_buffer > 100) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "Force enter standby mode timeout\n");

      if (!SetForceEnterMode(ForceMode::kStandby, false)) {
        LogMessage(
            LogLevel::kError, __FILE__, __LINE__, "SetForceEnterMode failed\n");
        return false;
      }

      return false;
    }

    DelayUs(1000);
  }

  if (!SetForceEnterMode(ForceMode::kStandby, false)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "SetForceEnterMode failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
