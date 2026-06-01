/*
 * @Description: AW21009
 * @Author: LILYGO_L
 * @Date: 2025-09-24 10:47:30
 * @LastEditTime: 2026-06-01 00:00:00
 * @License: GPL 3.0
 */
#include "aw21009.h"

namespace cpp_bus_driver {

bool Aw21009::Init(int32_t freq_hz) {
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
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  const uint8_t device_id = GetDeviceId();
  if (device_id != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get aw21009 id failed (error id: %#X)\n", device_id);
    return false;
  }
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "Get aw21009 id success (id: %#X)\n", device_id);

  if (!SoftwareReset()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SoftwareReset failed\n");
    return false;
  }

  if (!SetGlobalControl(true, ClockFrequency::k16Mhz,
          PwmResolution::k12BitWithDither, true)) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetGlobalControl failed\n");
    return false;
  }
  DelayMs(1);

  bool result = true;
  result &= SetGlobalCurrentLimit(0xFF);
  result &= SetCurrentLimit(LedChannel::kAll, 0xFF, false);
  result &= SetBrightness(LedChannel::kAll, 0, false);
  result &= Update();
  if (!result) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Default config failed\n");
    return false;
  }

  return true;
}

bool Aw21009::Deinit(bool delete_bus) {
  bool result = true;

  result &= SetChipEnable(false);

  if (!ChipI2cGuide::Deinit(delete_bus)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    result = false;
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  return result;
}

bool Aw21009::ReadRegister(uint8_t reg, uint8_t* value) {
  if (value == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!bus_->Read(reg, value)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return true;
}

bool Aw21009::WriteRegister(uint8_t reg, uint8_t value) {
  if (!bus_->Write(reg, value)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw21009::WriteRegisters(
    uint8_t start_reg, const uint8_t* data, size_t length) {
  if (data == nullptr && length != 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!bus_->Write(start_reg, data, length)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Aw21009::WriteMaskedRegister(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t buffer = 0;
  if (!ReadRegister(reg, &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  buffer = (buffer & ~mask) | (value & mask);
  if (!WriteRegister(reg, buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  return true;
}

bool Aw21009::SoftwareReset() {
  if (!WriteRegister(RegisterValue(Register::kReset), 0x00)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteRegister failed\n");
    return false;
  }

  DelayMs(3);
  return true;
}

uint8_t Aw21009::GetDeviceId() {
  uint8_t buffer = 0;

  if (!ReadRegister(RegisterValue(Register::kReset), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadRegister failed\n");
    return static_cast<uint8_t>(-1);
  }

  return buffer;
}

bool Aw21009::Update() {
  return WriteRegister(RegisterValue(Register::kUpdate), 0x00);
}

bool Aw21009::SetGlobalControl(bool auto_power_save,
    ClockFrequency clock_frequency, PwmResolution pwm_resolution,
    bool chip_enable) {
  const uint8_t value =
      (static_cast<uint8_t>(auto_power_save) << 7) |
      (static_cast<uint8_t>(clock_frequency) << 4) |
      (static_cast<uint8_t>(pwm_resolution) << 1) |
      static_cast<uint8_t>(chip_enable);

  return WriteRegister(RegisterValue(Register::kGlobalControl), value);
}

bool Aw21009::SetAutoPowerSave(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl),
      0x80, static_cast<uint8_t>(enable) << 7);
}

bool Aw21009::SetClockFrequency(ClockFrequency clock_frequency) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl),
      0x70, static_cast<uint8_t>(clock_frequency) << 4);
}

bool Aw21009::SetPwmResolution(PwmResolution pwm_resolution) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl),
      0x06, static_cast<uint8_t>(pwm_resolution) << 1);
}

bool Aw21009::SetChipEnable(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl),
      0x01, static_cast<uint8_t>(enable));
}

bool Aw21009::SetBrightness(
    LedChannel channel, uint16_t value, bool update) {
  if (value > kBrightnessMax) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    value = kBrightnessMax;
  }

  bool result = true;
  if (channel == LedChannel::kAll) {
    uint8_t buffer[kLedCount * 2] = {0};
    for (uint8_t i = 0; i < kLedCount; i++) {
      buffer[i * 2] = static_cast<uint8_t>(value);
      buffer[(i * 2) + 1] = static_cast<uint8_t>((value >> 8) & 0x0F);
    }
    result = WriteRegisters(RegisterValue(Register::kBrightnessStart), buffer,
        sizeof(buffer));
  } else if (IsSingleChannel(channel)) {
    result = WriteBrightnessByIndex(ChannelIndex(channel), value);
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!result) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write brightness failed\n");
    return false;
  }

  return !update || Update();
}

bool Aw21009::GetBrightness(LedChannel channel, uint16_t* value) {
  if (value == nullptr || !IsSingleChannel(channel)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  const uint8_t reg =
      RegisterValue(Register::kBrightnessStart) + (ChannelIndex(channel) * 2);
  uint8_t buffer[2] = {0};
  if (!bus_->Read(reg, buffer, sizeof(buffer))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  *value = static_cast<uint16_t>(buffer[0]) |
           (static_cast<uint16_t>(buffer[1] & 0x0F) << 8);
  return true;
}

bool Aw21009::SetSingleByteBrightness(
    LedChannel channel, uint8_t value, bool update) {
  bool result = true;
  if (channel == LedChannel::kAll) {
    uint8_t buffer[kLedCount] = {0};
    for (uint8_t i = 0; i < kLedCount; i++) {
      buffer[i] = value;
    }
    result = WriteRegisters(RegisterValue(Register::kBrightnessStart), buffer,
        sizeof(buffer));
  } else if (IsSingleChannel(channel)) {
    result = WriteRegister(
        RegisterValue(Register::kBrightnessStart) + ChannelIndex(channel),
        value);
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!result) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "Write single byte brightness failed\n");
    return false;
  }

  return !update || Update();
}

bool Aw21009::SetRgbBrightness(
    LedGroup group, uint16_t value, bool update) {
  if (value > kBrightnessMax) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    value = kBrightnessMax;
  }

  bool result = true;
  if (group == LedGroup::kAll) {
    for (uint8_t i = 0; i < 3; i++) {
      result &= WriteBrightnessByIndex(i, value);
    }
  } else if (IsSingleGroup(group)) {
    result = WriteBrightnessByIndex(static_cast<uint8_t>(group), value);
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!result) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "Write RGB brightness failed\n");
    return false;
  }

  return !update || Update();
}

bool Aw21009::SetCurrentLimit(
    LedChannel channel, uint8_t value, bool update) {
  bool result = true;
  if (channel == LedChannel::kAll) {
    uint8_t buffer[kLedCount] = {0};
    for (uint8_t i = 0; i < kLedCount; i++) {
      buffer[i] = value;
    }
    result =
        WriteRegisters(RegisterValue(Register::kScalingStart), buffer,
            sizeof(buffer));
  } else if (IsSingleChannel(channel)) {
    result = WriteCurrentLimitByIndex(ChannelIndex(channel), value);
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!result) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "Write current limit failed\n");
    return false;
  }

  return !update || Update();
}

bool Aw21009::GetCurrentLimit(LedChannel channel, uint8_t* value) {
  if (value == nullptr || !IsSingleChannel(channel)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  return ReadRegister(
      RegisterValue(Register::kScalingStart) + ChannelIndex(channel), value);
}

bool Aw21009::SetGlobalCurrentLimit(uint8_t value) {
  return WriteRegister(RegisterValue(Register::kGlobalCurrentControl), value);
}

bool Aw21009::GetGlobalCurrentLimit(uint8_t* value) {
  return ReadRegister(RegisterValue(Register::kGlobalCurrentControl), value);
}

bool Aw21009::SetPhaseDelay(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kPhaseControl),
      0x80, static_cast<uint8_t>(enable) << 7);
}

bool Aw21009::SetPhaseInvert(LedGroup group, bool enable) {
  const uint8_t mask = GroupMask(group);
  if (mask == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  return WriteMaskedRegister(RegisterValue(Register::kPhaseControl), mask,
      enable ? mask : 0);
}

bool Aw21009::SetOpenShortDetection(OpenShortDetectMode mode,
    OpenThreshold open_threshold, ShortThreshold short_threshold) {
  const uint8_t value =
      (static_cast<uint8_t>(open_threshold) << 3) |
      (static_cast<uint8_t>(short_threshold) << 2) |
      static_cast<uint8_t>(mode);

  bool result = true;
  if (mode == OpenShortDetectMode::kDisable) {
    result &= WriteMaskedRegister(
        RegisterValue(Register::kOpenShortDetectControl), 0x0F, value);
    result &= SetPwmFullDuty(false, false);
  } else {
    result &= SetPwmFullDuty(true, true);
    result &= WriteMaskedRegister(
        RegisterValue(Register::kOpenShortDetectControl), 0x0F, value);
  }

  return result;
}

bool Aw21009::GetOpenShortStatus(uint16_t* status) {
  if (status == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer[2] = {0};
  if (!bus_->Read(RegisterValue(Register::kOpenShortStatus0), buffer,
          sizeof(buffer))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  *status = static_cast<uint16_t>(buffer[0]) |
            ((static_cast<uint16_t>(buffer[1]) & 0x01) << 8);
  return true;
}

bool Aw21009::SetThermalRollOff(ThermalRollOffCurrent current,
    ThermalRollOffThreshold threshold) {
  const uint8_t value =
      (static_cast<uint8_t>(current) << 6) | static_cast<uint8_t>(threshold);

  return WriteMaskedRegister(RegisterValue(Register::kOverTemperatureControl),
      0xC0 | 0x03, value);
}

bool Aw21009::SetOverTemperatureProtection(
    bool detect_enable, bool protect_enable) {
  uint8_t value = 0;
  if (!protect_enable) {
    value |= 0x08;
  }
  if (!detect_enable) {
    value |= 0x04;
  }

  return WriteMaskedRegister(RegisterValue(Register::kOverTemperatureControl),
      0x08 | 0x04, value);
}

bool Aw21009::GetThermalStatus(ThermalStatus* status) {
  if (status == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer = 0;
  if (!ReadRegister(RegisterValue(Register::kOverTemperatureControl),
          &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  status->thermal_roll_off = (buffer & 0x20) != 0;
  status->over_temperature = (buffer & 0x10) != 0;
  status->raw = buffer;
  return true;
}

bool Aw21009::SetPwmFullDuty(
    bool led1_to_led6_enable, bool led7_to_led9_enable) {
  const uint8_t value =
      (led7_to_led9_enable ? 0x40 : 0) |
      (led1_to_led6_enable ? 0x20 : 0);

  return WriteMaskedRegister(RegisterValue(Register::kSpreadSpectrumControl),
      0x40 | 0x20, value);
}

bool Aw21009::SetSpreadSpectrum(bool enable, SpreadSpectrumRange range,
    SpreadSpectrumPeriod period) {
  const uint8_t value =
      (static_cast<uint8_t>(enable) << 4) |
      (static_cast<uint8_t>(range) << 2) | static_cast<uint8_t>(period);

  return WriteMaskedRegister(RegisterValue(Register::kSpreadSpectrumControl),
      0x10 | 0x0C | 0x03, value);
}

bool Aw21009::SetUvProtection(bool detect_enable, bool protect_enable) {
  uint8_t value = 0;
  if (!protect_enable) {
    value |= 0x02;
  }
  if (!detect_enable) {
    value |= 0x01;
  }

  return WriteMaskedRegister(RegisterValue(Register::kUvControl),
      0x02 | 0x01, value);
}

bool Aw21009::SetOcpProtection(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kUvControl),
      0x04, enable ? 0 : 0x04);
}

bool Aw21009::SetOcpThreshold(OcpThreshold threshold) {
  return WriteMaskedRegister(RegisterValue(Register::kUvControl),
      0x08, static_cast<uint8_t>(threshold) << 3);
}

bool Aw21009::GetUvStatus(UvStatus* status) {
  if (status == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer = 0;
  if (!ReadRegister(RegisterValue(Register::kUvControl), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  status->rext_status =
      static_cast<RextStatus>((buffer & 0xC0) >> 6);
  status->uvlo = (buffer & 0x20) != 0;
  status->power_up = (buffer & 0x10) != 0;
  status->raw = buffer;
  return true;
}

bool Aw21009::SetBroadcastAddressEnable(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl2),
      0x10, enable ? 0 : 0x10);
}

bool Aw21009::SetUpdateMode(UpdateMode mode) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl2),
      0x0C, static_cast<uint8_t>(mode) << 2);
}

bool Aw21009::SetSingleByteMode(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl2),
      0x02, static_cast<uint8_t>(enable) << 1);
}

bool Aw21009::SetRgbMode(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl2),
      0x01, static_cast<uint8_t>(enable));
}

bool Aw21009::SetPowerSavePwmIs0(bool enable) {
  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl3),
      0x08, static_cast<uint8_t>(enable) << 3);
}

bool Aw21009::SetSlewRate(
    SlewRateRising rising, SlewRateFalling falling) {
  const uint8_t value =
      (static_cast<uint8_t>(rising) << 2) | static_cast<uint8_t>(falling);

  return WriteMaskedRegister(RegisterValue(Register::kGlobalControl3),
      0x04 | 0x03, value);
}

bool Aw21009::SetGroupBrightness(uint16_t value) {
  if (value > kBrightnessMax) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    value = kBrightnessMax;
  }

  const uint8_t buffer[] = {
      static_cast<uint8_t>((value >> 8) & 0x0F),
      static_cast<uint8_t>(value),
  };

  return WriteRegisters(
      RegisterValue(Register::kGroupBrightnessHigh), buffer, sizeof(buffer));
}

bool Aw21009::SetGroupScaling(uint8_t red, uint8_t green, uint8_t blue) {
  const uint8_t buffer[] = {red, green, blue};
  return WriteRegisters(
      RegisterValue(Register::kGroupScalingRed), buffer, sizeof(buffer));
}

bool Aw21009::SetGroupConfig(
    uint8_t group_mask, bool use_individual_scaling) {
  const uint8_t value =
      (use_individual_scaling ? 0x40 : 0) | (group_mask & 0x07);

  return WriteMaskedRegister(RegisterValue(Register::kGroupConfig),
      0x40 | 0x07, value);
}

bool Aw21009::SetGroupEnable(LedGroup group, bool enable) {
  const uint8_t mask = GroupMask(group);
  if (mask == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  return WriteMaskedRegister(RegisterValue(Register::kGroupConfig), mask,
      enable ? mask : 0);
}

bool Aw21009::SetPatternConfig(
    bool enable, PatternMode mode, bool switch_on, bool ramp_enable) {
  const uint8_t value =
      (static_cast<uint8_t>(switch_on) << 3) |
      (static_cast<uint8_t>(ramp_enable) << 2) |
      (static_cast<uint8_t>(mode) << 1) | static_cast<uint8_t>(enable);

  return WriteMaskedRegister(RegisterValue(Register::kPatternConfig),
      0x08 | 0x04 | 0x02 | 0x01, value);
}

bool Aw21009::SetManualPatternSwitch(
    bool switch_on, bool ramp_enable) {
  return SetPatternConfig(true, PatternMode::kManual, switch_on, ramp_enable);
}

bool Aw21009::SetPatternTiming(const PatternTiming& timing) {
  uint16_t repeat = timing.repeat;
  if (repeat > 0x0FFF) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    repeat = 0x0FFF;
  }

  const uint8_t loop_end =
      timing.loop_end == PatternLoopEnd::kT3
          ? 0
          : static_cast<uint8_t>(PatternLoopEnd::kT1);

  const uint8_t buffer[] = {
      static_cast<uint8_t>((static_cast<uint8_t>(timing.rise) << 4) |
                           static_cast<uint8_t>(timing.on)),
      static_cast<uint8_t>((static_cast<uint8_t>(timing.fall) << 4) |
                           static_cast<uint8_t>(timing.off)),
      static_cast<uint8_t>((loop_end << 6) |
                           (static_cast<uint8_t>(timing.loop_start) << 4) |
                           ((repeat >> 8) & 0x0F)),
      static_cast<uint8_t>(repeat),
  };

  return WriteRegisters(
      RegisterValue(Register::kPatternTime0), buffer, sizeof(buffer));
}

bool Aw21009::SetPatternBrightness(
    uint8_t max_brightness, uint8_t min_brightness) {
  const uint8_t buffer[] = {max_brightness, min_brightness};
  return WriteRegisters(
      RegisterValue(Register::kGroupBrightnessHigh), buffer, sizeof(buffer));
}

bool Aw21009::StartPattern() {
  bool result = true;
  result &= WriteRegister(RegisterValue(Register::kPatternGo), 0x00);
  result &= WriteRegister(RegisterValue(Register::kPatternGo), 0x01);
  return result;
}

bool Aw21009::StopPattern() {
  return WriteRegister(RegisterValue(Register::kPatternGo), 0x00);
}

bool Aw21009::GetPatternStatus(PatternStatus* status) {
  if (status == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer = 0;
  if (!ReadRegister(RegisterValue(Register::kPatternGo), &buffer)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadRegister failed\n");
    return false;
  }

  status->loop_over = (buffer & 0x04) != 0;
  status->running = (buffer & 0x02) != 0;
  status->raw = buffer;
  return true;
}

uint8_t Aw21009::RegisterValue(Register reg) {
  return static_cast<uint8_t>(reg);
}

uint8_t Aw21009::ChannelIndex(LedChannel channel) {
  return static_cast<uint8_t>(channel);
}

uint8_t Aw21009::GroupMask(LedGroup group) {
  if (group == LedGroup::kAll) {
    return 0x07;
  }
  if (!IsSingleGroup(group)) {
    return 0;
  }

  return static_cast<uint8_t>(1U << static_cast<uint8_t>(group));
}

bool Aw21009::IsSingleChannel(LedChannel channel) {
  return static_cast<uint8_t>(channel) < kLedCount;
}

bool Aw21009::IsSingleGroup(LedGroup group) {
  return static_cast<uint8_t>(group) < 3;
}

bool Aw21009::WriteBrightnessByIndex(uint8_t index, uint16_t value) {
  if (index >= kLedCount) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  const uint8_t buffer[] = {
      static_cast<uint8_t>(value),
      static_cast<uint8_t>((value >> 8) & 0x0F),
  };
  const uint8_t reg =
      RegisterValue(Register::kBrightnessStart) + (index * 2);

  return WriteRegisters(reg, buffer, sizeof(buffer));
}

bool Aw21009::WriteCurrentLimitByIndex(uint8_t index, uint8_t value) {
  if (index >= kLedCount) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  return WriteRegister(RegisterValue(Register::kScalingStart) + index, value);
}

}  // namespace cpp_bus_driver
