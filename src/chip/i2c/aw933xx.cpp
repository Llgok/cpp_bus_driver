/*
 * @Description: AW933xx 固件加载、采样及硬件算法实现
 */
#include "chip/i2c/aw933xx.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace cpp_bus_driver {
namespace {

#include "aw933xx_firmware_v9_0_4.inc"

/**
 * @brief 将固件容器中的四字节小端数据解析为无符号整数。
 * @param data 至少包含四个可读字节的数据指针。
 * @return 解析后的 32 位无符号整数。
 */
uint32_t ReadLittleEndian(const uint8_t* data) {
  return uint32_t{data[0]} | (uint32_t{data[1]} << 8) |
         (uint32_t{data[2]} << 16) | (uint32_t{data[3]} << 24);
}

/**
 * @brief 保持寄存器原始位模式，将采样值解释为有符号整数。
 * @param value 寄存器中的 32 位原始数据。
 * @return 相同位模式对应的 32 位有符号整数。
 */
int32_t SignedValue(uint32_t value) {
  int32_t result;
  std::memcpy(&result, &value, sizeof(result));
  return result;
}

}  // namespace

bool Aw933xx::Init(int32_t freq_hz) {
  electrode_mask_ = 0;
  interrupt_pin_ = InterruptPin::kDisabled;
  if (freq_hz <= 0 || freq_hz > 400000) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "AW933xx invalid I2C frequency: %ld Hz\n", static_cast<long>(freq_hz));
    return false;
  }
  if (!I2cChipBase::Init(freq_hz)) {
    return false;
  }
  DelayMs(50);
  uint32_t chip_id;
  if (!GetChipId(&chip_id)) {
    return false;
  }
  switch (chip_id) {
    case 0xA9630340:
      electrode_mask_ = 0x0007;
      break;
    case 0xA9630300:
      electrode_mask_ = 0x0029;  // BFOR: CS0、CS3、CS5。
      break;
    case 0xA9630520:
      electrode_mask_ = 0x001F;
      break;
    case 0xA9630500:
      electrode_mask_ = 0x002F;  // BFOR: CS0、CS1、CS2、CS3、CS5。
      break;
    case 0xA9330710:
      electrode_mask_ = 0x00FF;  // CS0~CS7，其中 CS2/CS5 可复用为 INTN。
      break;
    case 0xA9331010:
      electrode_mask_ = 0x03FF;
      break;
    case 0xA9331210:
    case 0xA9331280:
      electrode_mask_ = 0x0FFF;
      break;
    case 0xA93312A0:
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "AWS93312PLR detected; bundled firmware compatibility is not "
          "confirmed (chip ID: 0x%08lX)\n",
          static_cast<unsigned long>(chip_id));
      return false;
    default:
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "Unsupported AW933xx ID: 0x%08lX\n",
          static_cast<unsigned long>(chip_id));
      return false;
  }
  if (!SoftwareReset() || !LoadFirmware() ||
      !SetOperatingMode(OperatingMode::kSleep) ||
      !ApplySettings(kDefaultSettings,
          sizeof(kDefaultSettings) / sizeof(kDefaultSettings[0])) ||
      !ConfigureScan(0) || !SetInterruptMask(0)) {
    return false;
  }
  // EVB 默认有佩戴和滑动映射，初始化不应直接对最终电极启用它们。
  for (uint8_t i = 0; i < kSliderCount; ++i) {
    if (!WriteRegister(
            static_cast<uint16_t>(Register::kSliderControl) + i * kSliderStride,
            0)) {
      return false;
    }
  }
  if (!UpdateRegister(static_cast<uint16_t>(Register::kWearMode), 0xF, 0) ||
      !UpdateRegister(
          static_cast<uint16_t>(Register::kWearMode) + kWearStride, 0xF, 0)) {
    return false;
  }
  return true;
}

bool Aw933xx::Deinit(bool delete_bus) {
  const bool stopped = SetOperatingMode(OperatingMode::kSleep);
  const bool released = I2cChipBase::Deinit(delete_bus);
  return stopped && released;
}

bool Aw933xx::SoftwareReset() {
  if (!WriteRegister(static_cast<uint16_t>(Register::kReset), 0)) {
    return false;
  }
  operating_mode_ = OperatingMode::kActive;
  DelayMs(30);
  return WaitForInitialization();
}

bool Aw933xx::GetChipId(uint32_t* value) {
  return ReadRegister(static_cast<uint16_t>(Register::kChipId), value);
}

bool Aw933xx::GetFirmwareVersion(uint32_t* value) {
  return ReadRegister(static_cast<uint16_t>(Register::kFirmwareVersion), value);
}

bool Aw933xx::SetOperatingMode(OperatingMode mode) {
  if (mode != OperatingMode::kActive && mode != OperatingMode::kSleep &&
      mode != OperatingMode::kDeepSleep) {
    return false;
  }
  if (operating_mode_ == OperatingMode::kDeepSleep &&
      !UpdateRegister(static_cast<uint16_t>(Register::kClockControl), 1, 1)) {
    return false;
  }
  const auto command = static_cast<Command>(mode);
  if (!WriteRegister(static_cast<uint16_t>(Register::kCommand),
          static_cast<uint32_t>(command))) {
    return false;
  }
  operating_mode_ = mode;
  return true;
}

bool Aw933xx::ConfigureScan(uint16_t channels, uint16_t period_ms) {
  if ((channels & ~0xFFF) != 0 || period_ms < 10 || period_ms > 100 ||
      operating_mode_ != OperatingMode::kSleep) {
    return false;
  }
  if (!WriteRegister(static_cast<uint16_t>(Register::kScanPeriod), period_ms) ||
      !WriteRegister(
          static_cast<uint16_t>(Register::kScanControl2), channels) ||
      !WriteRegister(static_cast<uint16_t>(Register::kScanEnable), channels)) {
    return false;
  }
  return true;
}

bool Aw933xx::ConfigureElectrodes(uint8_t channel,
    const std::array<ElectrodeMode, kChannelCount>& electrodes) {
  if (!CheckChannel(channel) || operating_mode_ != OperatingMode::kSleep) {
    return false;
  }
  uint32_t selection[2] = {};
  for (uint8_t cs = 0; cs < kChannelCount; ++cs) {
    const auto mode = electrodes[cs];
    if (static_cast<uint8_t>(mode) > 3 ||
        (mode != ElectrodeMode::kHighImpedance &&
            (electrode_mask_ & (1U << cs)) == 0) ||
        (mode != ElectrodeMode::kHighImpedance &&
            ((interrupt_pin_ == InterruptPin::kCs2 && cs == 2) ||
                (interrupt_pin_ == InterruptPin::kCs5 && cs == 5)))) {
      return false;
    }
    selection[cs / 10] |= static_cast<uint32_t>(mode) << ((cs % 10) * 3);
  }
  return WriteRegister(static_cast<uint16_t>(Register::kElectrodeSelect0) +
                           channel * kChannelStride,
             selection[0]) &&
         WriteRegister(static_cast<uint16_t>(Register::kElectrodeSelect1) +
                           channel * kChannelStride,
             selection[1]);
}

bool Aw933xx::ConfigureWidget(
    uint8_t widget_index, const WidgetConfig& config) {
  const uint8_t widget = static_cast<uint8_t>(config.widget);
  if (widget_index >= kSliderCount || widget > 6 ||
      operating_mode_ != OperatingMode::kSleep || config.scan_period_ms < 10 ||
      config.scan_period_ms > 100 || config.x_channels > 12 ||
      config.y_channels > 12 ||
      config.first_channel + config.x_channels + config.y_channels > 12 ||
      (config.widget != Widget::kDisabled && config.x_channels == 0)) {
    return false;
  }
  const uint16_t base = static_cast<uint16_t>(Register::kSliderTouchThreshold) +
                        widget_index * kSliderStride;
  const uint32_t function = widget == 0 ? 0 : 1U << (widget - 1);
  const uint32_t slider_type = widget == 0 ? 0 : widget - 1;
  const std::array<uint32_t, 2> settings[] = {
      {base, config.touch_threshold},
      {static_cast<uint16_t>(base + 4), config.release_threshold},
      {static_cast<uint16_t>(base + 8), config.touch_threshold},
      {static_cast<uint16_t>(base + 12), config.release_threshold},
      {static_cast<uint16_t>(base + 16),
          (uint32_t{config.interrupt_enabled} << 28) | (function << 24) |
              (uint32_t{config.debounce} << 16) |
              (uint32_t{config.y_channels} << 12) |
              (uint32_t{config.x_channels} << 8) | config.first_channel},
      {static_cast<uint16_t>(base + 20),
          (uint32_t{config.scan_period_ms} << 16) | config.click_interval_ms},
      {static_cast<uint16_t>(base + 24),
          (uint32_t{config.longer_press_ms} << 16) | config.long_press_ms},
      {static_cast<uint16_t>(base + 48), config.position_threshold},
      {static_cast<uint16_t>(base + 52), config.position_threshold},
      {static_cast<uint16_t>(base + 56), (3U << 8) | slider_type},
      {static_cast<uint16_t>(base + 64), config.minimum_distance},
  };
  // BUTTON_CFG3 高位包含官方默认配置；仅更新低 16 位时间参数。
  if (!ApplySettings(settings, sizeof(settings) / sizeof(settings[0])) ||
      !UpdateRegister(base + 28, 0xFFFF, config.super_long_press_ms)) {
    return false;
  }
  uint32_t control, timing, hold_thresholds, hold_config, touch_threshold;
  if (!ReadRegister(base + 16, &control) || !ReadRegister(base + 20, &timing) ||
      !ReadRegister(base + 24, &hold_thresholds) ||
      !ReadRegister(base + 28, &hold_config) ||
      !ReadRegister(base, &touch_threshold)) {
    return false;
  }
  constexpr const char* kWidgetNames[] = {"disabled", "button", "1D slider",
      "2D slider", "circular slider", "wear", "force"};
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "AW933xx group %u (%s): start CH%u, X channels=%u, Y channels=%u, "
      "debounce=%u, touch IRQ=%s, touch threshold=%lu; "
      "scan=%lu ms, click interval=%lu ms, hold thresholds=%lu/%lu/%lu ms\n",
      static_cast<unsigned>(widget_index), kWidgetNames[widget],
      static_cast<unsigned>(control & 0xF),
      static_cast<unsigned>((control >> 8) & 0xF),
      static_cast<unsigned>((control >> 12) & 0xF),
      static_cast<unsigned>((control >> 16) & 0xFF),
      (control & (1U << 28)) != 0 ? "enabled" : "disabled",
      static_cast<unsigned long>(touch_threshold),
      static_cast<unsigned long>(timing >> 16),
      static_cast<unsigned long>(timing & 0xFFFF),
      static_cast<unsigned long>(hold_thresholds & 0xFFFF),
      static_cast<unsigned long>(hold_thresholds >> 16),
      static_cast<unsigned long>(hold_config & 0xFFFF));
  return true;
}

bool Aw933xx::ConfigureWear(uint8_t wear, uint8_t detect_channel,
    uint8_t reference_channel, uint32_t on_threshold, uint32_t off_threshold,
    bool enabled) {
  if (wear >= kWearCount || !CheckChannel(detect_channel) ||
      !CheckChannel(reference_channel) || detect_channel == reference_channel ||
      operating_mode_ != OperatingMode::kSleep) {
    return false;
  }
  const uint16_t threshold_stride = wear * 0x3C;
  return WriteRegister(static_cast<uint16_t>(Register::kWearOnThreshold) +
                           threshold_stride,
             on_threshold) &&
         WriteRegister(static_cast<uint16_t>(Register::kWearOffThreshold) +
                           threshold_stride,
             off_threshold) &&
         UpdateRegister(
             static_cast<uint16_t>(Register::kWearMode) + wear * kWearStride,
             0xFFF,
             (uint32_t{reference_channel} << 8) |
                 (uint32_t{detect_channel} << 4) | (enabled ? 1U : 0U));
}

bool Aw933xx::SetInterruptPin(InterruptPin pin) {
  if (operating_mode_ != OperatingMode::kSleep ||
      (pin != InterruptPin::kDisabled && pin != InterruptPin::kCs2 &&
          pin != InterruptPin::kCs5)) {
    return false;
  }
  uint32_t chip_id, scan_enable;
  if (!GetChipId(&chip_id) ||
      (chip_id != 0xA9330710 && chip_id != 0xA9630500) ||
      !ReadRegister(
          static_cast<uint16_t>(Register::kScanEnable), &scan_enable) ||
      (scan_enable & 0xFFF) != 0) {
    return false;
  }
  // 此操作安排在映射电极之前，避免中断引脚仍被采样通道驱动。
  if (!WriteRegister(
          static_cast<uint16_t>(Register::kPeripheralAccess), 0x3C00D013) ||
      !WriteRegister(static_cast<uint16_t>(Register::kPinMux),
          pin == InterruptPin::kCs2   ? 0x20
          : pin == InterruptPin::kCs5 ? 0x800
                                      : 0) ||
      !WriteRegister(static_cast<uint16_t>(Register::kInterruptOutput),
          pin == InterruptPin::kCs2   ? 4
          : pin == InterruptPin::kCs5 ? 0x20
                                      : 0)) {
    return false;
  }
  interrupt_pin_ = pin;
  return true;
}

bool Aw933xx::SetInterruptMask(uint32_t mask) {
  return (mask & ~0xFFFFFU) == 0 &&
         WriteRegister(static_cast<uint16_t>(Register::kInterruptEnable), mask);
}

bool Aw933xx::Calibrate(uint16_t channels) {
  if (channels == 0 || (channels & ~0xFFF) != 0) {
    return false;
  }
  uint32_t scan_enable;
  return ReadRegister(
             static_cast<uint16_t>(Register::kScanEnable), &scan_enable) &&
         (channels & ~scan_enable) == 0 &&
         UpdateRegister(
             static_cast<uint16_t>(Register::kCalibration), 0xFFF, channels);
}

bool Aw933xx::ReadChannel(uint8_t channel, ChannelData* data) {
  if (!CheckChannel(channel) || data == nullptr) {
    return false;
  }
  uint32_t raw, valid, baseline, difference;
  const uint16_t offset = channel * kChannelStride;
  if (!ReadRegister(static_cast<uint16_t>(Register::kRaw) + offset, &raw) ||
      !ReadRegister(static_cast<uint16_t>(Register::kValid) + offset, &valid) ||
      !ReadRegister(
          static_cast<uint16_t>(Register::kBaseline) + offset, &baseline) ||
      !ReadRegister(
          static_cast<uint16_t>(Register::kDifference) + offset, &difference)) {
    return false;
  }
  *data = {SignedValue(raw), SignedValue(valid), SignedValue(baseline),
      SignedValue(difference)};
  return true;
}

bool Aw933xx::ReadOffsetCapacitance(uint8_t channel, float* picofarads) {
  if (!CheckChannel(channel) || picofarads == nullptr) {
    return false;
  }
  uint32_t config, offset;
  if (!ReadRegister(0x007C + channel * kChannelStride, &config) ||
      !ReadRegister(static_cast<uint16_t>(Register::kAfeOffset) +
                        channel * kChannelStride,
          &offset)) {
    return false;
  }
  const uint8_t mode = config & 0xFF;
  if (mode != 0 && mode != 4 && mode != 5) {
    return false;
  }
  int32_t coarse = 0;
  for (int i = 0; i < 8; ++i) {
    const int sign = mode == 0 || (offset & (1U << i)) == 0 ? 1 : -1;
    coarse += sign * static_cast<int>((offset >> (8 + i)) & 1) * (1 << i);
  }
  *picofarads =
      (coarse * 9900 + static_cast<int>((offset >> 16) & 0xFF) * 152) /
      10000.0f;
  return true;
}

bool Aw933xx::ReadSliderState(uint8_t slider, SliderState* state) {
  if (slider >= kSliderCount || state == nullptr) {
    return false;
  }
  uint32_t button, position, distance;
  const uint16_t offset = slider * kSliderStride;
  if (!ReadRegister(
          static_cast<uint16_t>(Register::kSliderState) + offset, &button) ||
      !ReadRegister(static_cast<uint16_t>(Register::kSliderPosition) + offset,
          &position) ||
      !ReadRegister(static_cast<uint16_t>(Register::kSliderDistance) + offset,
          &distance)) {
    return false;
  }
  state->button_state = button;
  state->position_x = position & 0xFFFF;
  state->position_y = position >> 16;
  state->distance_x = static_cast<int16_t>(distance & 0xFFFF);
  state->distance_y = static_cast<int16_t>(distance >> 16);
  return true;
}

bool Aw933xx::ReadWearState(uint8_t* state) {
  uint32_t value;
  if (state == nullptr ||
      !ReadRegister(static_cast<uint16_t>(Register::kWearState), &value)) {
    return false;
  }
  *state = value & 3;
  return true;
}

bool Aw933xx::ReadEvents(Events* events) {
  if (events == nullptr) {
    return false;
  }
  Events result;
  uint32_t proximity0, proximity1;
  if (!ReadRegister(
          static_cast<uint16_t>(Register::kInterruptSource), &result.source) ||
      !ReadRegister(
          static_cast<uint16_t>(Register::kProximity0), &proximity0) ||
      !ReadRegister(
          static_cast<uint16_t>(Register::kProximity1), &proximity1) ||
      !ReadWearState(&result.wear)) {
    return false;
  }
  result.proximity0 = proximity0 & 0xFFF;
  result.proximity1 = proximity1 & 0xFFF;
  result.learning_finished = (result.source >> 17) & 3;
  for (uint8_t i = 0; i < kSliderCount; ++i) {
    if ((result.source & (1U << (5 + i))) == 0) {
      continue;
    }
    uint32_t report;
    if (!ReadRegister(
            static_cast<uint16_t>(Register::kSliderReport) + i * kSliderStride,
            &report)) {
      return false;
    }
    result.sliders[i] = {static_cast<uint8_t>(report & 0xF),
        static_cast<uint8_t>((report >> 4) & 0x3F),
        static_cast<uint8_t>((report >> 10) & 0x3F),
        static_cast<uint8_t>(report >> 16), static_cast<uint8_t>(report >> 24)};
  }
  *events = result;
  return true;
}

bool Aw933xx::StartWearLearning() {
  return WriteRegister(static_cast<uint16_t>(Register::kCommand),
      static_cast<uint32_t>(Command::kStartWearLearning));
}

bool Aw933xx::FinishWearLearning(
    std::array<WearCalibration, kWearCount>* data) {
  if (data == nullptr) {
    return false;
  }
  std::array<WearCalibration, kWearCount> result;
  for (uint8_t i = 0; i < kWearCount; ++i) {
    if (!ReadWearCalibration(i, &result[i])) {
      return false;
    }
  }
  if (!WriteRegister(static_cast<uint16_t>(Register::kCommand),
          static_cast<uint32_t>(Command::kFinishWearLearning))) {
    return false;
  }
  *data = result;
  return true;
}

bool Aw933xx::ReadWearCalibration(uint8_t wear, WearCalibration* data) {
  if (wear >= kWearCount || data == nullptr) {
    return false;
  }
  WearCalibration result;
  for (size_t i = 0; i < result.values.size(); ++i) {
    if (!ReadRegister(static_cast<uint16_t>(
                          static_cast<uint16_t>(Register::kLearnedOffset) +
                          wear * kWearStride + i * 4),
            &result.values[i])) {
      return false;
    }
  }
  *data = result;
  return true;
}

bool Aw933xx::RestoreWearCalibration(
    uint8_t wear, const WearCalibration& data) {
  if (wear >= kWearCount) {
    return false;
  }
  for (size_t i = 0; i < data.values.size(); ++i) {
    if (!WriteRegister(static_cast<uint16_t>(
                           static_cast<uint16_t>(Register::kLearnedOffset) +
                           wear * kWearStride + i * 4),
            data.values[i])) {
      return false;
    }
  }
  return true;
}

bool Aw933xx::CaptureWearCalibration(uint8_t wear, WearCalibration* data) {
  if (wear >= kWearCount || data == nullptr) {
    return false;
  }
  const uint16_t mode_address =
      static_cast<uint16_t>(Register::kWearMode) + wear * kWearStride;
  uint32_t mode;
  if (!ReadRegister(mode_address, &mode)) {
    return false;
  }
  const uint8_t detect = (mode >> 4) & 0xF;
  const uint8_t reference = (mode >> 8) & 0xF;
  if (!CheckChannel(detect) || !CheckChannel(reference) ||
      detect == reference || !WriteRegister(mode_address, mode & ~0xFU)) {
    return false;
  }
  bool success = Calibrate((1U << detect) | (1U << reference));
  if (success) {
    DelayMs(500);
  }
  WearCalibration result;
  const uint16_t registers[] = {
      static_cast<uint16_t>(static_cast<uint16_t>(Register::kAfeOffset) +
                            detect * kChannelStride),
      static_cast<uint16_t>(static_cast<uint16_t>(Register::kAfeOffset) +
                            reference * kChannelStride),
      static_cast<uint16_t>(
          static_cast<uint16_t>(Register::kRaw) + detect * kChannelStride),
      static_cast<uint16_t>(
          static_cast<uint16_t>(Register::kRaw) + reference * kChannelStride)};
  for (size_t i = 0; success && i < result.values.size(); ++i) {
    success = ReadRegister(registers[i], &result.values[i]);
  }
  if (success) {
    success = RestoreWearCalibration(wear, result);
  }
  // 无论采样是否成功都尝试恢复佩戴算法。
  const bool restored = WriteRegister(mode_address, mode);
  if (!success || !restored) {
    return false;
  }
  *data = result;
  return true;
}

bool Aw933xx::MeasureDifference(uint8_t channel, uint16_t samples,
    uint16_t interval_ms, ChannelStatistics* statistics) {
  if (!CheckChannel(channel) || statistics == nullptr || samples < 2 ||
      samples > 1000 || interval_ms < 10 || interval_ms > 1000) {
    return false;
  }
  ChannelStatistics result;
  result.minimum = std::numeric_limits<int32_t>::max();
  result.maximum = std::numeric_limits<int32_t>::min();
  int64_t sum = 0;
  for (uint16_t i = 0; i < samples; ++i) {
    uint32_t value;
    if (!ReadRegister(static_cast<uint16_t>(Register::kDifference) +
                          channel * kChannelStride,
            &value)) {
      return false;
    }
    const int32_t difference = SignedValue(value);
    result.minimum = std::min(result.minimum, difference);
    result.maximum = std::max(result.maximum, difference);
    sum += difference;
    if (i + 1 < samples) {
      DelayMs(interval_ms);
    }
  }
  result.average = static_cast<int32_t>(sum / samples);
  result.peak_to_peak = int64_t{result.maximum} - result.minimum;
  *statistics = result;
  return true;
}

bool Aw933xx::CheckElectrodeShorts(uint16_t* ground, uint16_t* supply) {
  if (ground == nullptr || supply == nullptr || ground == supply ||
      electrode_mask_ == 0) {
    return false;
  }
  // 官方例程在发现短路后提前返回，会遗留 GPIO 模式；这里统一执行恢复。
  constexpr uint16_t registers[] = {
      0xFFF4, 0xC100, 0xE004, 0xE00C, 0xE010, 0xE014};
  std::array<uint32_t, 6> saved;
  for (size_t i = 0; i < saved.size(); ++i) {
    if (!ReadRegister(registers[i], &saved[i])) {
      return false;
    }
  }
  const OperatingMode previous = operating_mode_;
  if (!SetOperatingMode(OperatingMode::kSleep)) {
    return false;
  }
  uint16_t mask = electrode_mask_;
  if (interrupt_pin_ == InterruptPin::kCs2) {
    mask &= ~(1U << 2);
  } else if (interrupt_pin_ == InterruptPin::kCs5) {
    mask &= ~(1U << 5);
  }
  uint32_t pulled_up = 0, pulled_down = 0;
  bool success = WriteRegister(0xFFF4, saved[0] | 0x3C00D103) &&
                 WriteRegister(0xC100, 0x555555) && WriteRegister(0xE004, 0) &&
                 WriteRegister(0xE00C, mask) && WriteRegister(0xE010, mask) &&
                 WriteRegister(0xE014, 0);
  if (success) {
    DelayMs(2);
    success = ReadRegister(0xE008, &pulled_up) && WriteRegister(0xE010, 0) &&
              WriteRegister(0xE014, mask);
  }
  if (success) {
    DelayMs(2);
    success = ReadRegister(0xE008, &pulled_down);
  }
  bool restored = true;
  for (size_t i = saved.size(); i > 0; --i) {
    restored = WriteRegister(registers[i - 1], saved[i - 1]) && restored;
  }
  restored = SetOperatingMode(previous) && restored;
  if (!success || !restored) {
    return false;
  }
  *ground = static_cast<uint16_t>(~pulled_up) & mask;
  *supply = static_cast<uint16_t>(pulled_down) & mask;
  return true;
}

bool Aw933xx::LoadFirmware() {
  return LoadFirmware(kFirmware, sizeof(kFirmware));
}

bool Aw933xx::LoadFirmware(const uint8_t* image, size_t size) {
  // 厂商容器头为小端，I2C 和固件负载字节为大端，不能直接转换结构体指针。
  if (image == nullptr || size < 80 || size > 80 + 0x2FF8 + 0xBF8 ||
      ReadLittleEndian(image + 8) != 0x23 ||
      ReadLittleEndian(image + 12) != 1 ||
      ReadLittleEndian(image + 16) != size - 60) {
    return false;
  }
  uint32_t checksum = 0;
  for (size_t i = 4; i < size; ++i) {
    checksum += image[i];
  }
  const size_t first_size = ReadLittleEndian(image + 68);
  const size_t second_size = ReadLittleEndian(image + 76);
  if (checksum != ReadLittleEndian(image) || first_size == 0 ||
      first_size > 0x2FF8 || second_size > 0xBF8 ||
      first_size + second_size != size - 80 || (first_size & 3) != 0 ||
      (second_size & 3) != 0) {
    return false;
  }
  // 对应官方 MCU loader；第一段固定下载到 0x2000，不使用头中的 MCU 地址。
  if (!WriteRegister(
          static_cast<uint16_t>(Register::kPeripheralAccess), 0x3C00D11F) ||
      !WriteRegister(static_cast<uint16_t>(Register::kRamPower), 0x21660000) ||
      !WriteRegister(
          static_cast<uint16_t>(Register::kCpuControl), 0x00010100) ||
      !WriteRegister(static_cast<uint16_t>(Register::kRamAccess), 0x3C000000) ||
      !TransferFirmwareRegion(0x2000, image + 80, first_size, 0x2FF8, true) ||
      !TransferFirmwareRegion(
          0x1400, image + 80 + first_size, second_size, 0xBF8, false) ||
      !VerifyFirmwareRegion(0x2000, image + 80, first_size, 0x2FF8, true) ||
      !VerifyFirmwareRegion(
          0x1400, image + 80 + first_size, second_size, 0xBF8, false) ||
      !WriteRegister(
          static_cast<uint16_t>(Register::kCpuControl), 0x00000100) ||
      !SoftwareReset()) {
    return false;
  }
  uint32_t version;
  return GetFirmwareVersion(&version) &&
         version == ReadLittleEndian(image + 60);
}

bool Aw933xx::ApplySettings(
    const std::array<uint32_t, 2>* settings, size_t count) {
  if (settings == nullptr && count != 0) {
    return false;
  }
  for (size_t i = 0; i < count; ++i) {
    if (!WriteRegister(static_cast<uint16_t>(settings[i][0]), settings[i][1])) {
      return false;
    }
  }
  return true;
}

bool Aw933xx::ReadRegister(uint16_t address, uint32_t* value) {
  uint8_t buffer[4];
  if (value == nullptr || (address & 3) != 0 ||
      !ReadRegister(address, buffer, sizeof(buffer))) {
    return false;
  }
  *value = (uint32_t{buffer[0]} << 24) | (uint32_t{buffer[1]} << 16) |
           (uint32_t{buffer[2]} << 8) | uint32_t{buffer[3]};
  return true;
}

bool Aw933xx::WriteRegister(uint16_t address, uint32_t value) {
  const uint8_t buffer[] = {static_cast<uint8_t>(value >> 24),
      static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 8),
      static_cast<uint8_t>(value)};
  return (address & 3) == 0 && WriteRegister(address, buffer, sizeof(buffer));
}

bool Aw933xx::UpdateRegister(uint16_t address, uint32_t mask, uint32_t value) {
  uint32_t current;
  return ReadRegister(address, &current) &&
         WriteRegister(address, (current & ~mask) | (value & mask));
}

bool Aw933xx::WaitForInitialization() {
  uint32_t source = 0;
  for (uint32_t elapsed_ms = 0; elapsed_ms < kInitializationTimeoutMs;
      elapsed_ms += kInitializationPollIntervalMs) {
    if (!ReadRegister(
            static_cast<uint16_t>(Register::kInterruptSource), &source)) {
      return false;
    }
    if ((source & 1) != 0) {
      return true;
    }
    DelayMs(kInitializationPollIntervalMs);
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "AW933xx initialization timed out (poll delay: %lu ms, last IRQ "
      "source: 0x%08lX)\n",
      static_cast<unsigned long>(kInitializationTimeoutMs),
      static_cast<unsigned long>(source));
  return false;
}

bool Aw933xx::TransferFirmwareRegion(uint16_t address, const uint8_t* data,
    size_t size, size_t capacity, bool fill) {
  if (size > capacity || (size != 0 && data == nullptr)) {
    return false;
  }
  const size_t total = fill ? capacity : size;
  for (size_t offset = 0; offset < total;) {
    uint8_t buffer[128];
    const size_t count = std::min(sizeof(buffer), total - offset);
    std::memset(buffer, 0xFF, count);
    if (offset < size) {
      std::memcpy(buffer, data + offset, std::min(count, size - offset));
    }
    const uint16_t target = static_cast<uint16_t>(address + offset);
    if (!WriteRegister(target, buffer, count)) {
      return false;
    }
    offset += count;
  }
  return true;
}

bool Aw933xx::VerifyFirmwareRegion(uint16_t address, const uint8_t* data,
    size_t size, size_t capacity, bool fill) {
  if (size > capacity || (size != 0 && data == nullptr)) {
    return false;
  }
  const size_t total = fill ? capacity : size;
  for (size_t offset = 0; offset < total;) {
    uint8_t expected[128];
    uint8_t actual[128];
    const size_t count = std::min(sizeof(actual), total - offset);
    std::memset(expected, 0xFF, count);
    if (offset < size) {
      std::memcpy(expected, data + offset, std::min(count, size - offset));
    }
    const uint16_t target = static_cast<uint16_t>(address + offset);
    if (!ReadRegister(target, actual, count)) {
      return false;
    }
    if (std::memcmp(expected, actual, count) != 0) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "AW933xx firmware verification mismatch (address: %#X)\n",
          static_cast<unsigned>(target));
      return false;
    }
    offset += count;
  }
  return true;
}

bool Aw933xx::ReadRegister(uint16_t address, uint8_t* data, size_t size) {
  const uint8_t command[] = {
      static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address)};
  // IRQSRC/报告寄存器具有读清语义，读取失败不自动重试以免吞掉事件。
  if (bus_ != nullptr && data != nullptr && size != 0 && size <= 128 &&
      size <= 0x10000U - address &&
      bus_->WriteRead(command, sizeof(command), data, size)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "AW933xx register read failed (register: %#X)\n",
      static_cast<unsigned>(address));
  return false;
}

bool Aw933xx::WriteRegister(
    uint16_t address, const uint8_t* data, size_t size) {
  if (bus_ != nullptr && data != nullptr && size != 0 && size <= 128 &&
      size <= 0x10000U - address) {
    uint8_t buffer[130];
    buffer[0] = static_cast<uint8_t>(address >> 8);
    buffer[1] = static_cast<uint8_t>(address);
    std::memcpy(buffer + 2, data, size);
    if (bus_->Write(buffer, size + 2)) {
      return true;
    }
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "AW933xx register write failed (register: %#X)\n",
      static_cast<unsigned>(address));
  return false;
}

bool Aw933xx::CheckChannel(uint8_t channel) const {
  return channel < kChannelCount;
}

}  // namespace cpp_bus_driver
