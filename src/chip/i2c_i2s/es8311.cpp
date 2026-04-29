/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-04-23 11:48:36
 * @License: GPL 3.0
 */
#include "es8311.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
constexpr const Es8311::ClockCoeff Es8311::kClockCoeffTable_[];
#endif

bool Es8311::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    ChipI2cGuide::SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    ChipI2cGuide::GpioWrite(rst_, 1);
    ChipI2cGuide::DelayMs(10);
    ChipI2cGuide::GpioWrite(rst_, 0);
    ChipI2cGuide::DelayMs(10);
    ChipI2cGuide::GpioWrite(rst_, 1);
    ChipI2cGuide::DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!SoftwareReset(true)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SoftwareReset failed\n");
    return false;
  }
  ChipI2cGuide::DelayMs(20);
  if (!SoftwareReset(false)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SoftwareReset failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    ChipI2cGuide::LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get es8311 id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    ChipI2cGuide::LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get es8311 id success (id: %#X)\n", buffer);
  }

  if (!SetMasterClockSource(ClockSource::kAdcDacMclk)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetMasterClockSource failed\n");
    return false;
  }
  if (!SetClock(ClockSource::kAdcDacMclk, true)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetClock failed\n");
    return false;
  }
  if (!SetClock(ClockSource::kAdcDacBclk, true)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetClock failed\n");
    return false;
  }
  if (!SetSerialPortMode(SerialPortMode::kSlave)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetSerialPortMode failed\n");
    return false;
  }

  return true;
}

bool Es8311::Init(
    uint16_t mclk_multiple, uint32_t sample_rate_hz, uint8_t data_bit_width) {
  if (!SetClockCoeff(mclk_multiple, sample_rate_hz)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetClockCoeff failed\n");
    return false;
  }

  if (!SetSdpDataBitLength(Sdp::kAdc, [this](uint8_t dbw) -> BitsPerSample {
        if (dbw <= 16) {
          return BitsPerSample::kData16bit;
        } else if (dbw <= 18) {
          return BitsPerSample::kData18bit;
        } else if (dbw <= 20) {
          return BitsPerSample::kData20bit;
        } else if (dbw <= 24) {
          return BitsPerSample::kData24bit;
        } else if (dbw <= 32) {
          return BitsPerSample::kData32bit;
        } else {
          ChipI2cGuide::LogMessage(
              LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
          return BitsPerSample::kData16bit;
        }
      }(data_bit_width))) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetSdpDataBitLength failed\n");
    return false;
  }
  if (!SetSdpDataBitLength(Sdp::kDac, [this](uint8_t dbw) -> BitsPerSample {
        if (dbw <= 16) {
          return BitsPerSample::kData16bit;
        } else if (dbw <= 18) {
          return BitsPerSample::kData18bit;
        } else if (dbw <= 20) {
          return BitsPerSample::kData20bit;
        } else if (dbw <= 24) {
          return BitsPerSample::kData24bit;
        } else if (dbw <= 32) {
          return BitsPerSample::kData32bit;
        } else {
          ChipI2cGuide::LogMessage(
              LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
          return BitsPerSample::kData16bit;
        }
      }(data_bit_width))) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetSdpDataBitLength failed\n");
    return false;
  }

  if (!ChipI2sGuide::Init(mclk_multiple, sample_rate_hz, data_bit_width)) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

uint16_t Es8311::GetDeviceId() {
  uint8_t buffer[2] = {0};

  for (uint8_t i = 0; i < 2; i++) {
    if (!ChipI2cGuide::bus_->Read(
            static_cast<uint8_t>(
                static_cast<uint8_t>(Cmd::kRoDeviceIdStart) + i),
            &buffer[i])) {
      ChipI2cGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return -1;
    }
  }

  return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
}

bool Es8311::SoftwareReset(bool enable) {
  // 启动复位
  if (enable) {
    if (!ChipI2cGuide::bus_->Write(
            static_cast<uint8_t>(Cmd::kRwResetSerialPortModeControl),
            static_cast<uint8_t>(0x1F))) {
      ChipI2cGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  } else  // 关闭复位
  {
    if (!ChipI2cGuide::bus_->Write(
            static_cast<uint8_t>(Cmd::kRwResetSerialPortModeControl),
            static_cast<uint8_t>(0x00))) {
      ChipI2cGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
    if (!ChipI2cGuide::bus_->Write(
            static_cast<uint8_t>(Cmd::kRwResetSerialPortModeControl),
            static_cast<uint8_t>(0x80))) {
      ChipI2cGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      return false;
    }
  }

  return true;
}

bool Es8311::SetMasterClockSource(ClockSource clock) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwClockManager1), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(clock) << 7);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwClockManager1), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetClock(ClockSource clock, bool enalbe, bool invert) {
  uint8_t buffer = 0;

  switch (clock) {
    case ClockSource::kAdcDacMclk:
      if (!ChipI2cGuide::bus_->Read(
              static_cast<uint8_t>(Cmd::kRwClockManager1), &buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
        return false;
      }
      buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(invert) << 6);
      buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(enalbe) << 5);
      // 控制内部ADC时钟的开启或关闭的控制位
      buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enalbe) << 3);
      // 控制内部DAC时钟的开启或关闭的控制位
      buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(enalbe) << 2);
      // 未知位，必须置1才能正常工作
      buffer = (buffer & 0B11111101) | (static_cast<uint8_t>(enalbe) << 1);
      // 未知位，必须置1才能正常工作
      buffer = (buffer & 0B11111110) | (static_cast<uint8_t>(enalbe));

      if (!ChipI2cGuide::bus_->Write(
              static_cast<uint8_t>(Cmd::kRwClockManager1), buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;
    case ClockSource::kAdcDacBclk: {
      if (!ChipI2cGuide::bus_->Read(
              static_cast<uint8_t>(Cmd::kRwClockManager1), &buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
        return false;
      }
      buffer = (buffer & 0B11101111) | (static_cast<uint8_t>(enalbe) << 4);
      // 控制内部ADC时钟的开启或关闭的控制位
      buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enalbe) << 3);
      // 控制内部DAC时钟的开启或关闭的控制位
      buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(enalbe) << 2);
      // 未知位，必须置1才能正常工作
      buffer = (buffer & 0B11111101) | (static_cast<uint8_t>(enalbe) << 1);
      // 未知位，必须置1才能正常工作
      buffer = (buffer & 0B11111110) | (static_cast<uint8_t>(enalbe));
      if (!ChipI2cGuide::bus_->Write(
              static_cast<uint8_t>(Cmd::kRwClockManager1), buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }

      if (!ChipI2cGuide::bus_->Read(
              static_cast<uint8_t>(Cmd::kRwClockManager6), &buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
        return false;
      }
      buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(invert) << 5);
      if (!ChipI2cGuide::bus_->Write(
              static_cast<uint8_t>(Cmd::kRwClockManager6), buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;
    }
    default:
      break;
  }

  return true;
}

bool Es8311::SetDacVolume(uint8_t volume) {
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwDacVolume), volume)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcVolume(uint8_t volume) {
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcVolume), volume)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcAutoVolumeControl(bool enable) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcAlc), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcAlc), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetMic(MicType type, MicInput input) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcDmicPgaGain), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(type) << 6);
  buffer = (buffer & 0B11001111) | (static_cast<uint8_t>(input) << 4);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcDmicPgaGain), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetPowerStatus(PowerStatus status) {
  uint8_t buffer =
      static_cast<uint8_t>(!status.contorl.analog_circuits) << 7 |
      static_cast<uint8_t>(!status.contorl.analog_bias_circuits) << 6 |
      static_cast<uint8_t>(!status.contorl.analog_adc_bias_circuits) << 5 |
      static_cast<uint8_t>(!status.contorl.analog_adc_reference_circuits) << 4 |
      static_cast<uint8_t>(!status.contorl.analog_dac_reference_circuit) << 3 |
      static_cast<uint8_t>(status.contorl.internal_reference_circuits) << 2 |
      static_cast<uint8_t>(status.vmid);

  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPowerUpPowerDownContorl), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetLowPowerStatus(LowPowerStatus status) {
  uint8_t buffer = static_cast<uint8_t>(status.dac) << 7 |
                   static_cast<uint8_t>(status.pga) << 6 |
                   static_cast<uint8_t>(status.pga_output) << 5 |
                   static_cast<uint8_t>(status.adc) << 4 |
                   static_cast<uint8_t>(status.adc_reference) << 3 |
                   static_cast<uint8_t>(status.dac_reference) << 2 |
                   static_cast<uint8_t>(status.flash) << 1 |
                   static_cast<uint8_t>(status.int1);

  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwLowPowerControl), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetClockCoeff(uint16_t mclk_multiple, uint32_t sample_rate_hz) {
  size_t buffer_index = 0;

  // 搜索
  if (!SearchClockCoeff(mclk_multiple, sample_rate_hz, kClockCoeffTable_,
          sizeof(kClockCoeffTable_) / sizeof(ClockCoeff), &buffer_index)) {
    ChipI2cGuide::LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "SearchClockCoeff failed (error index: %d)\n", buffer_index);
    return false;
  }

  const ClockCoeff* buffer_clock_coeff = &kClockCoeffTable_[buffer_index];

  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwClockManager2), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer &= 0x07;
  buffer |= (buffer_clock_coeff->pre_div - 1) << 5;
  buffer |= buffer_clock_coeff->pre_multi << 3;
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwClockManager2), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  buffer = (buffer_clock_coeff->fs_mode << 6) | buffer_clock_coeff->adc_osr;
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwClockManager3), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!ChipI2cGuide::bus_->Write(static_cast<uint8_t>(Cmd::kRwClockManager4),
          buffer_clock_coeff->dac_osr)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  buffer = ((buffer_clock_coeff->adc_div - 1) << 4) |
           (buffer_clock_coeff->dac_div - 1);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwClockManager5), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwClockManager6), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer &= 0xE0;
  if (buffer_clock_coeff->bclk_div < 19) {
    buffer |= (buffer_clock_coeff->bclk_div - 1) << 0;
  } else {
    buffer |= (buffer_clock_coeff->bclk_div) << 0;
  }
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwClockManager6), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwClockManager7), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer &= 0xC0;
  buffer |= buffer_clock_coeff->lrck_h << 0;
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwClockManager7), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!ChipI2cGuide::bus_->Write(static_cast<uint8_t>(Cmd::kRwClockManager8),
          buffer_clock_coeff->lrck_l)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetSdpDataBitLength(Sdp dsp, BitsPerSample length) {
  uint8_t buffer = 0;

  switch (dsp) {
    case Sdp::kDac:
      if (!ChipI2cGuide::bus_->Read(
              static_cast<uint8_t>(Cmd::kRwSdpInFormat), &buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
        return false;
      }
      buffer = (buffer & 0B11100011) | (static_cast<uint8_t>(length) << 2);
      if (!ChipI2cGuide::bus_->Write(
              static_cast<uint8_t>(Cmd::kRwSdpInFormat), buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;
    case Sdp::kAdc:
      if (!ChipI2cGuide::bus_->Read(
              static_cast<uint8_t>(Cmd::kRwSdpOutFormat), &buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
        return false;
      }
      buffer = (buffer & 0B11100011) | (static_cast<uint8_t>(length) << 2);
      if (!ChipI2cGuide::bus_->Write(
              static_cast<uint8_t>(Cmd::kRwSdpOutFormat), buffer)) {
        ChipI2cGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
        return false;
      }
      break;

    default:
      break;
  }

  return true;
}

bool Es8311::SetPgaPower(bool enable) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPgaAdcModulatorPowerControl), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(!enable) << 6);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPgaAdcModulatorPowerControl), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcPower(bool enable) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwPgaAdcModulatorPowerControl), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(!enable) << 5);
  buffer = (buffer & 0B11101111) | (static_cast<uint8_t>(!enable) << 4);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwPgaAdcModulatorPowerControl), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetDacPower(bool enable) {
  uint8_t buffer = 0;

  if (enable) {
    buffer = 0x00;
  } else {
    buffer = 0x02;  // 默认值
  }
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwDacPowerControl), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetOutputToHpDrive(bool enable) {
  uint8_t buffer = 0;

  if (enable) {
    buffer = 0x10;
  } else {
    buffer = 0x40;  // 默认值
  }
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwOutputToHpDriveControl), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcOffsetFreeze(AdcOffsetFreeze offset_freeze) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcEqualizerBypass), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(offset_freeze) << 5);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcEqualizerBypass), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcHpfStage2Coeff(uint8_t coeff) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcEqualizerBypass), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11100000) | coeff;
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcEqualizerBypass), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetDacEqualizer(bool enable) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwDacRamprateEqbypass), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(!enable) << 3);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwDacRamprateEqbypass), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
size_t Es8311::ReadI2s(void* data, size_t byte) {
  size_t buffer = ChipI2sGuide::bus_->Read(data, byte);

  if (buffer == 0) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return buffer;
}

size_t Es8311::WriteI2s(const void* data, size_t byte) {
  size_t buffer = ChipI2sGuide::bus_->Write(data, byte);

  if (buffer == 0) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return buffer;
}

bool Es8311::SetClockReconfig(uint16_t mclk_multiple, uint32_t sample_rate_hz) {
  if (!SetClockCoeff(mclk_multiple, sample_rate_hz)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetClockCoeff failed\n");
    return false;
  }

  if (!ChipI2sGuide::SetClockReconfig(mclk_multiple, sample_rate_hz)) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetClockReconfig failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetI2sChannelEnable(bool enable) {
  if (!ChipI2sGuide::bus_->SetChannelEnable(enable)) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetChannelEnable failed\n");
    return false;
  }

  return true;
}
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF

bool Es8311::StartTransmitI2s(
    uint32_t* write_buffer, uint32_t* read_buffer, size_t max_buffer_length) {
  if (!ChipI2sGuide::bus_->StartTransmit(
          write_buffer, read_buffer, max_buffer_length)) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "StartTransmit failed\n");
    return false;
  }
  return true;
}

void Es8311::StopTransmitI2s() { ChipI2sGuide::bus_->StopTransmit(); }

bool Es8311::SetNextReadI2s(uint32_t* data) {
  if (!ChipI2sGuide::bus_->SetNextRead(data)) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetNextRead failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetNextWriteI2s(uint32_t* data) {
  if (!ChipI2sGuide::bus_->SetNextWrite(data)) {
    ChipI2sGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetNextWrite failed\n");
    return false;
  }

  return true;
}

bool Es8311::GetReadI2sEventFlag() {
  return ChipI2sGuide::bus_->GetReadEventFlag();
}

bool Es8311::GetWriteI2sEventFlag() {
  return ChipI2sGuide::bus_->GetWriteEventFlag();
}

#endif

bool Es8311::SetAdcGain(AdcGain gain) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcGainScaleUp), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11111000) | static_cast<uint8_t>(gain);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcGainScaleUp), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcDataToDac(bool enable) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcDacControlAdcdatSel), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcDacControlAdcdatSel), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcPgaGain(AdcPgaGain gain) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcDmicPgaGain), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B11110000) | static_cast<uint8_t>(gain);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcDmicPgaGain), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetSerialPortMode(SerialPortMode mode) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwResetSerialPortModeControl), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10111011) | (static_cast<uint8_t>(mode) << 6) |
           (!static_cast<uint8_t>(mode) << 2);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwResetSerialPortModeControl), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SetAdcDataFormat(AdcDataFormat format) {
  uint8_t buffer = 0;

  if (!ChipI2cGuide::bus_->Read(
          static_cast<uint8_t>(Cmd::kRwAdcDacControlAdcdatSel), &buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }
  buffer = (buffer & 0B10001111) | (static_cast<uint8_t>(format) << 4);
  if (!ChipI2cGuide::bus_->Write(
          static_cast<uint8_t>(Cmd::kRwAdcDacControlAdcdatSel), buffer)) {
    ChipI2cGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Es8311::SearchClockCoeff(uint16_t mclk_multiple, uint32_t sample_rate_hz,
    const ClockCoeff* library, size_t library_length, size_t* search_index) {
  for (size_t i = 0; i < library_length; i++) {
    if ((library[i].mclk_multiple == mclk_multiple) &&
        (library[i].sample_rate == sample_rate_hz)) {
      if (search_index != nullptr) {
        *search_index = i;
      }
      return true;
    }
  }

  return false;
}
}  // namespace cpp_bus_driver
