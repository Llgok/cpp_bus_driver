/*
 * @Description: TCA8418 键盘扫描与 GPIO 扩展芯片驱动实现
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-06-01 17:30:00
 * @License: GPL 3.0
 */
#include "tca8418.h"

namespace cpp_bus_driver {

bool Tca8418::Init(int32_t freq_hz) {
  if (rst_ != kDefaultValue) {
    bool result = true;
    result &= SetGpioMode(
        rst_, Tool::GpioMode::kOutput, Tool::GpioStatus::kPullup);
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

  Configuration config;
  config.auto_increment = true;
  config.overflow_mode = true;
  if (!SetConfiguration(config)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "SetConfiguration failed\n");
    return false;
  }

  bool clear_result = true;
  uint32_t gpio_irq_status = 0;
  clear_result &= ClearEventFifo();
  clear_result &= GetClearGpioIrqFlag(&gpio_irq_status);
  clear_result &= ClearIrqFlag(IrqFlag::kAll);
  if (!clear_result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Clear status failed\n");
    return false;
  }

  return true;
}

bool Tca8418::Deinit(bool delete_bus) {
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

uint8_t Tca8418::PackConfiguration(const Configuration& config) {
  return (static_cast<uint8_t>(config.auto_increment) << 7) |
         (static_cast<uint8_t>(config.gpi_events_disabled_when_locked) << 6) |
         (static_cast<uint8_t>(config.overflow_mode) << 5) |
         (static_cast<uint8_t>(config.interrupt_pulse_mode) << 4) |
         (static_cast<uint8_t>(config.overflow_interrupt) << 3) |
         (static_cast<uint8_t>(config.keypad_lock_interrupt) << 2) |
         (static_cast<uint8_t>(config.gpio_interrupt) << 1) |
         static_cast<uint8_t>(config.key_events_interrupt);
}

Tca8418::Configuration Tca8418::UnpackConfiguration(uint8_t value) {
  Configuration config;
  config.auto_increment = (value & 0x80) != 0;
  config.gpi_events_disabled_when_locked = (value & 0x40) != 0;
  config.overflow_mode = (value & 0x20) != 0;
  config.interrupt_pulse_mode = (value & 0x10) != 0;
  config.overflow_interrupt = (value & 0x08) != 0;
  config.keypad_lock_interrupt = (value & 0x04) != 0;
  config.gpio_interrupt = (value & 0x02) != 0;
  config.key_events_interrupt = (value & 0x01) != 0;
  return config;
}

Tca8418::EventType Tca8418::DecodeEventType(uint8_t event_num) {
  if ((event_num >= kKeypadFirstEvent) && (event_num <= kKeypadLastEvent)) {
    return EventType::kKeypad;
  }
  if ((event_num >= kGpioFirstEvent) && (event_num <= kGpioLastEvent)) {
    return EventType::kGpio;
  }
  return EventType::kUnknown;
}

bool Tca8418::IsValidFifoEventNumber(uint8_t event_num) {
  return DecodeEventType(event_num) != EventType::kUnknown;
}

bool Tca8418::IsValidUnlockKeyNumber(uint8_t key_num) {
  return (key_num == 0) || IsValidFifoEventNumber(key_num);
}

bool Tca8418::IsValidGpio(Gpio gpio) {
  return static_cast<uint8_t>(gpio) < kGpioCount;
}

bool Tca8418::SetConfiguration(const Configuration& config) {
  return WriteRegister(Register::kConfiguration, PackConfiguration(config));
}

bool Tca8418::GetConfiguration(Configuration* config) {
  if (config == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t value = 0;
  if (!ReadRegister(Register::kConfiguration, &value)) {
    return false;
  }

  *config = UnpackConfiguration(value);
  return true;
}

bool Tca8418::SetAutoIncrement(bool enable) {
  return UpdateRegisterBits(
      Register::kConfiguration, 0x80, enable ? 0x80 : 0x00);
}

bool Tca8418::SetGpiEventsDisabledWhenLocked(bool disable) {
  return UpdateRegisterBits(
      Register::kConfiguration, 0x40, disable ? 0x40 : 0x00);
}

bool Tca8418::SetFifoOverflowMode(bool enable) {
  return UpdateRegisterBits(
      Register::kConfiguration, 0x20, enable ? 0x20 : 0x00);
}

bool Tca8418::SetInterruptPulseMode(bool enable) {
  return UpdateRegisterBits(
      Register::kConfiguration, 0x10, enable ? 0x10 : 0x00);
}

bool Tca8418::SetInterruptEnable(IrqMask mask) {
  return SetInterruptEnable(static_cast<uint8_t>(mask));
}

bool Tca8418::SetInterruptEnable(uint8_t mask) {
  const uint8_t value = mask & kValidInterruptEnableMask;
  return UpdateRegisterBits(Register::kConfiguration,
      kValidInterruptEnableMask, value);
}

bool Tca8418::SetKeypadScanWindow(
    uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  if ((w == 0) || (h == 0) || (x >= kColumnCount) || (y >= kRowCount) ||
      (w > (kColumnCount - x)) || (h > (kRowCount - y))) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  uint32_t keypad_mask = 0;
  for (uint8_t row = y; row < (y + h); row++) {
    keypad_mask |= static_cast<uint32_t>(1UL) << row;
  }
  for (uint8_t col = x; col < (x + w); col++) {
    keypad_mask |= static_cast<uint32_t>(1UL) << (kRowCount + col);
  }

  return SetKeypadPins(keypad_mask);
}

bool Tca8418::SetKeypadPins(uint32_t keypad_mask) {
  return WriteGpioRegisterBlock(Register::kKeypadGpio1, keypad_mask);
}

bool Tca8418::GetKeypadPins(uint32_t* keypad_mask) {
  return ReadGpioRegisterBlock(Register::kKeypadGpio1, keypad_mask);
}

uint8_t Tca8418::GetFingerCount() {
  uint8_t value = 0;
  if (!ReadRegister(Register::kKeyLockAndEventCounter, &value)) {
    return kInvalidU8;
  }

  return value & 0x0F;
}

bool Tca8418::ReadKeyEvent(TouchInfo* event) {
  if (event == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t value = 0;
  if (!ReadRegister(Register::kKeyEventA, &value)) {
    return false;
  }

  const uint8_t event_num = value & 0x7F;
  if (event_num == 0) {
    return false;
  }
  if (!IsValidFifoEventNumber(event_num)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  event->num = event_num;
  event->press_flag = (value & 0x80) != 0;
  event->event_type = DecodeEventType(event_num);
  return true;
}

bool Tca8418::GetMultipleTouchPoint(TouchPoint& tp) {
  tp.finger_count = 0;
  tp.info.clear();

  uint8_t event_count = GetFingerCount();
  if ((event_count == kInvalidU8) || (event_count == 0)) {
    return false;
  }
  if (event_count > kFifoDepth) {
    event_count = kFifoDepth;
  }

  for (uint8_t i = 0; i < event_count; i++) {
    TouchInfo event;
    if (!ReadKeyEvent(&event)) {
      break;
    }
    tp.info.push_back(event);
  }

  tp.finger_count = static_cast<uint8_t>(tp.info.size());
  return tp.finger_count > 0;
}

uint8_t Tca8418::GetIrqFlag() {
  uint8_t value = 0;
  if (!ReadRegister(Register::kInterruptStatus, &value)) {
    return kInvalidU8;
  }

  return value & kValidIrqMask;
}

bool Tca8418::ParseIrqStatus(uint8_t irq_flag, IrqStatus& status) {
  if ((irq_flag == kInvalidU8) || ((irq_flag & ~kValidIrqMask) != 0)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  status.ctrl_alt_del_key_sequence_flag = (irq_flag & 0x10) != 0;
  status.fifo_overflow_flag = (irq_flag & 0x08) != 0;
  status.keypad_lock_flag = (irq_flag & 0x04) != 0;
  status.gpio_interrupt_flag = (irq_flag & 0x02) != 0;
  status.key_events_flag = (irq_flag & 0x01) != 0;

  return true;
}

bool Tca8418::ClearIrqFlag(IrqFlag flag) {
  return ClearIrqFlag(static_cast<uint8_t>(flag));
}

bool Tca8418::ClearIrqFlag(uint8_t flags) {
  return WriteRegister(Register::kInterruptStatus, flags & kValidIrqMask);
}

bool Tca8418::ClearEventFifo() {
  bool result = true;
  uint8_t event_count = GetFingerCount();
  if (event_count == kInvalidU8) {
    return false;
  }
  if (event_count > kFifoDepth) {
    event_count = kFifoDepth;
  }

  for (uint8_t i = 0; i < event_count; i++) {
    uint8_t value = 0;
    result &= ReadRegister(Register::kKeyEventA, &value);
  }
  result &= ClearIrqFlag(IrqFlag::kKeyEvents);
  return result;
}

bool Tca8418::GetClearGpioIrqFlag(uint32_t* status) {
  return ReadGpioRegisterBlock(Register::kGpioInterruptStatus1, status);
}

uint32_t Tca8418::GetClearGpioIrqFlag() {
  uint32_t status = 0;
  if (!GetClearGpioIrqFlag(&status)) {
    return 0;
  }
  return status;
}

bool Tca8418::SetIrqGpioMode(IrqMask mode) {
  return SetInterruptEnable(mode);
}

bool Tca8418::SetIrqGpioMode(uint8_t mode) {
  return SetInterruptEnable(mode);
}

bool Tca8418::ParseTouchNum(uint8_t num, TouchPosition& position) {
  if ((num < kKeypadFirstEvent) || (num > kKeypadLastEvent)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  position.x = (num - 1) % kColumnCount;
  position.y = (num - 1) / kColumnCount;
  return true;
}

bool Tca8418::GetKeyLockInfo(KeyLockInfo* info) {
  if (info == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t value = 0;
  if (!ReadRegister(Register::kKeyLockAndEventCounter, &value)) {
    return false;
  }

  info->key_lock_enabled = (value & 0x40) != 0;
  info->lock2_status = (value & 0x20) != 0;
  info->lock1_status = (value & 0x10) != 0;
  info->locked = info->lock1_status && info->lock2_status;
  info->event_count = value & 0x0F;
  return true;
}

bool Tca8418::SetKeypadLock(bool enable) {
  return UpdateRegisterBits(Register::kKeyLockAndEventCounter, 0x40,
      enable ? 0x40 : 0x00);
}

bool Tca8418::SetKeypadLockTimer(
    uint8_t lock_to_lock_timer_s, uint8_t interrupt_mask_timer_s) {
  if ((lock_to_lock_timer_s > 7) || (interrupt_mask_timer_s > 31)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  const uint8_t value =
      (interrupt_mask_timer_s << 3) | lock_to_lock_timer_s;
  return WriteRegister(Register::kKeypadLockTimer, value);
}

bool Tca8418::SetUnlockKeys(uint8_t unlock_key_1, uint8_t unlock_key_2) {
  if (!IsValidUnlockKeyNumber(unlock_key_1) ||
      !IsValidUnlockKeyNumber(unlock_key_2)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  bool result = true;
  result &= WriteRegister(Register::kUnlock1, unlock_key_1 & 0x7F);
  result &= WriteRegister(Register::kUnlock2, unlock_key_2 & 0x7F);
  return result;
}

bool Tca8418::SetGpioDirection(Gpio gpio, GpioMode mode) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  return SetGpioDirectionMask(GpioMask(gpio), mode);
}

bool Tca8418::SetGpioDirectionMask(uint32_t gpio_mask, GpioMode mode) {
  return UpdateGpioRegisterBlock(Register::kGpioDirection1, gpio_mask,
      mode == GpioMode::kOutput ? gpio_mask : 0);
}

bool Tca8418::GetGpioDirectionMask(uint32_t* output_mask) {
  return ReadGpioRegisterBlock(Register::kGpioDirection1, output_mask);
}

bool Tca8418::SetGpioOutput(Gpio gpio, bool high) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  return UpdateGpioRegisterBlock(
      Register::kGpioDataOut1, GpioMask(gpio), high ? GpioMask(gpio) : 0);
}

bool Tca8418::SetGpioOutputMask(uint32_t high_mask) {
  return WriteGpioRegisterBlock(Register::kGpioDataOut1, high_mask);
}

bool Tca8418::SetGpioOutputMask(uint32_t gpio_mask, uint32_t high_mask) {
  return UpdateGpioRegisterBlock(
      Register::kGpioDataOut1, gpio_mask, high_mask);
}

bool Tca8418::GetGpioOutputMask(uint32_t* high_mask) {
  return ReadGpioRegisterBlock(Register::kGpioDataOut1, high_mask);
}

bool Tca8418::GetGpioDataStatus(uint32_t* status, bool read_twice) {
  if (!ReadGpioRegisterBlock(Register::kGpioDataStatus1, status)) {
    return false;
  }
  if (read_twice) {
    return ReadGpioRegisterBlock(Register::kGpioDataStatus1, status);
  }
  return true;
}

bool Tca8418::GetGpioInput(Gpio gpio, bool* high) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  if (high == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint32_t status = 0;
  if (!GetGpioDataStatus(&status)) {
    return false;
  }

  *high = (status & GpioMask(gpio)) != 0;
  return true;
}

bool Tca8418::SetGpioInterruptEnable(Gpio gpio, bool enable) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  return SetGpioInterruptEnableMask(GpioMask(gpio), enable);
}

bool Tca8418::SetGpioInterruptEnableMask(uint32_t gpio_mask, bool enable) {
  return UpdateGpioRegisterBlock(Register::kGpioInterruptEnable1, gpio_mask,
      enable ? gpio_mask : 0);
}

bool Tca8418::GetGpioInterruptEnableMask(uint32_t* enable_mask) {
  return ReadGpioRegisterBlock(Register::kGpioInterruptEnable1, enable_mask);
}

bool Tca8418::SetGpioInterruptTrigger(
    Gpio gpio, GpioInterruptTrigger trigger) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  return SetGpioInterruptTriggerMask(GpioMask(gpio), trigger);
}

bool Tca8418::SetGpioInterruptTriggerMask(
    uint32_t gpio_mask, GpioInterruptTrigger trigger) {
  return UpdateGpioRegisterBlock(Register::kGpioInterruptLevel1, gpio_mask,
      trigger == GpioInterruptTrigger::kRisingOrHigh ? gpio_mask : 0);
}

bool Tca8418::GetGpioInterruptTriggerMask(uint32_t* rising_high_mask) {
  return ReadGpioRegisterBlock(
      Register::kGpioInterruptLevel1, rising_high_mask);
}

bool Tca8418::SetGpiEventMode(Gpio gpio, bool enable) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  return SetGpiEventModeMask(GpioMask(gpio), enable);
}

bool Tca8418::SetGpiEventModeMask(uint32_t gpio_mask, bool enable) {
  return UpdateGpioRegisterBlock(
      Register::kGpiEventMode1, gpio_mask, enable ? gpio_mask : 0);
}

bool Tca8418::GetGpiEventModeMask(uint32_t* enable_mask) {
  return ReadGpioRegisterBlock(Register::kGpiEventMode1, enable_mask);
}

bool Tca8418::SetGpioDebounce(Gpio gpio, bool enable) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  return SetGpioDebounceMask(GpioMask(gpio), enable);
}

bool Tca8418::SetGpioDebounceMask(uint32_t gpio_mask, bool enable) {
  return UpdateGpioRegisterBlock(
      Register::kDebounceDisable1, gpio_mask, enable ? 0 : gpio_mask);
}

bool Tca8418::GetGpioDebounceEnabledMask(uint32_t* enable_mask) {
  if (enable_mask == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint32_t disabled_mask = 0;
  if (!ReadGpioRegisterBlock(Register::kDebounceDisable1, &disabled_mask)) {
    return false;
  }
  *enable_mask = (~disabled_mask) & kAllGpioMask;
  return true;
}

bool Tca8418::SetGpioPullup(Gpio gpio, bool enable) {
  if (!IsValidGpio(gpio)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }
  return SetGpioPullupMask(GpioMask(gpio), enable);
}

bool Tca8418::SetGpioPullupMask(uint32_t gpio_mask, bool enable) {
  return UpdateGpioRegisterBlock(
      Register::kGpioPullupDisable1, gpio_mask, enable ? 0 : gpio_mask);
}

bool Tca8418::GetGpioPullupEnabledMask(uint32_t* enable_mask) {
  if (enable_mask == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint32_t disabled_mask = 0;
  if (!ReadGpioRegisterBlock(Register::kGpioPullupDisable1, &disabled_mask)) {
    return false;
  }
  *enable_mask = (~disabled_mask) & kAllGpioMask;
  return true;
}

bool Tca8418::ReadRegister(Register reg, uint8_t* value) {
  if (value == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!bus_->Read(static_cast<uint8_t>(reg), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return true;
}

bool Tca8418::WriteRegister(Register reg, uint8_t value) {
  if (!bus_->Write(static_cast<uint8_t>(reg), value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool Tca8418::UpdateRegisterBits(Register reg, uint8_t mask, uint8_t value) {
  uint8_t buffer = 0;
  if (!ReadRegister(reg, &buffer)) {
    return false;
  }

  buffer = (buffer & ~mask) | (value & mask);
  return WriteRegister(reg, buffer);
}

bool Tca8418::ReadGpioRegisterBlock(Register start_reg, uint32_t* value) {
  if (value == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  uint8_t buffer[3] = {};
  const uint8_t start_address = static_cast<uint8_t>(start_reg);
  for (uint8_t i = 0; i < 3; i++) {
    if (!bus_->Read(static_cast<uint8_t>(start_address + i), &buffer[i])) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  }

  *value = (static_cast<uint32_t>(buffer[0]) << 0) |
           (static_cast<uint32_t>(buffer[1]) << 8) |
           (static_cast<uint32_t>(buffer[2] & 0x03) << 16);
  *value &= kAllGpioMask;
  return true;
}

bool Tca8418::WriteGpioRegisterBlock(Register start_reg, uint32_t value) {
  value &= kAllGpioMask;

  const uint8_t buffer[3] = {
      static_cast<uint8_t>(value & 0xFF),
      static_cast<uint8_t>((value >> 8) & 0xFF),
      static_cast<uint8_t>((value >> 16) & 0x03),
  };
  const uint8_t start_address = static_cast<uint8_t>(start_reg);
  bool result = true;
  for (uint8_t i = 0; i < 3; i++) {
    result &= bus_->Write(static_cast<uint8_t>(start_address + i), buffer[i]);
  }

  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
  }
  return result;
}

bool Tca8418::UpdateGpioRegisterBlock(
    Register start_reg, uint32_t mask, uint32_t value) {
  mask &= kAllGpioMask;
  value &= kAllGpioMask;

  uint32_t buffer = 0;
  if (!ReadGpioRegisterBlock(start_reg, &buffer)) {
    return false;
  }

  buffer = (buffer & ~mask) | (value & mask);
  return WriteGpioRegisterBlock(start_reg, buffer);
}

}  // namespace cpp_bus_driver
