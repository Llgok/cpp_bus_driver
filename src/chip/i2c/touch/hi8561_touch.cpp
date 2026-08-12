/*
 * @Description: HI8561 电容触摸控制器驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-08-11 00:00:00
 * @License: GPL 3.0
 */
#include "hi8561_touch.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>

namespace cpp_bus_driver {

bool Hi8561Touch::Init(int32_t freq_hz) {
  std::lock_guard<std::mutex> lock(mutex_);

  runtime_layout_ = RuntimeLayout();
  last_debug_report_ms_ = 0;

  if (rst_ != kDefaultValue) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
    result &= GpioWrite(rst_, 0);
    DelayMs(10);
    result &= GpioWrite(rst_, 1);
    DelayMs(100);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "HI8561 reset failed (pin: %ld)\n", static_cast<long>(rst_));
      return false;
    }
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "HI8561 init failed\n");
    ChipI2cGuide::Deinit(false);
    return false;
  }

  if (!DiscoverRuntimeLayout()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 dynamic section discovery failed\n");
    ChipI2cGuide::Deinit(false);
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HI8561 init success (touch address: 0X%08lX, report size: %u)\n",
      static_cast<unsigned long>(
          runtime_layout_.coordinate_report.address),
      static_cast<unsigned int>(runtime_layout_.coordinate_report.length));
  last_debug_report_ms_ = GetSystemTimeMs();
  return true;
}

bool Hi8561Touch::Deinit(bool delete_bus) {
  std::lock_guard<std::mutex> lock(mutex_);

  bool result = ChipI2cGuide::Deinit(delete_bus);
  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "HI8561 deinit failed\n");
  }

  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }

  runtime_layout_ = RuntimeLayout();
  last_debug_report_ms_ = 0;
  return result;
}

TouchReadStatus Hi8561Touch::ReadPrimaryTouch(TouchFrame* frame) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (frame == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read primary touch failed (frame is null)\n");
    return TouchReadStatus::kInvalidData;
  }
  *frame = TouchFrame();

  if (!IsSectionValid(
          runtime_layout_.coordinate_report, kPrimaryReportSize)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 read primary touch failed (driver is not initialized)\n");
    return TouchReadStatus::kInvalidData;
  }

  std::array<uint8_t, kPrimaryReportSize> report{};
  if (!ReadEram(runtime_layout_.coordinate_report.address, report.data(),
          report.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 read primary touch failed (I2C transfer failed)\n");
    return TouchReadStatus::kBusError;
  }

  const uint8_t reported_contact_count = report[0];
  if (reported_contact_count > kMaxTouchContactCount) {
    LogTouchReport(
        "single", report.data(), report.size(), reported_contact_count, *frame);
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 read primary touch failed (contact count: %u)\n",
        static_cast<unsigned int>(reported_contact_count));
    return TouchReadStatus::kInvalidData;
  }
  frame->sequence = report[1];
  frame->gesture = report[2];
  if (reported_contact_count == 0) {
    LogTouchReport(
        "single", report.data(), report.size(), reported_contact_count, *frame);
    return TouchReadStatus::kNoData;
  }

  TouchContact contact;
  const uint8_t* primary_data = &report[kTouchCoordinateOffset];
  if (ParseContact(primary_data, 1, &contact)) {
    frame->contacts[0] = contact;
    frame->contact_count = 1;
    LogTouchReport(
        "single", report.data(), report.size(), reported_contact_count, *frame);
    return TouchReadStatus::kSuccess;
  }
  frame->edge_touch = IsEdgeContact(primary_data);

  // 边缘标记通常位于固件报告的最后一个有效槽位，仅在首点无效时补读该槽位。
  if (!frame->edge_touch && reported_contact_count > 1) {
    std::array<uint8_t, kTouchBytesPerContact> last_contact{};
    const size_t last_contact_offset =
        kTouchCoordinateOffset +
        (reported_contact_count - 1) * kTouchBytesPerContact;
    if (last_contact_offset + last_contact.size() >
        runtime_layout_.coordinate_report.length) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "HI8561 read primary touch failed (contact data exceeds report: "
          "count %u, size %lu)\n",
          static_cast<unsigned int>(reported_contact_count),
          static_cast<unsigned long>(
              runtime_layout_.coordinate_report.length));
      return TouchReadStatus::kInvalidData;
    }
    const uint32_t last_contact_address =
        runtime_layout_.coordinate_report.address +
        static_cast<uint32_t>(last_contact_offset);
    if (!ReadEram(
            last_contact_address, last_contact.data(), last_contact.size())) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "HI8561 read primary touch failed (edge slot transfer failed)\n");
      return TouchReadStatus::kBusError;
    }
    frame->edge_touch = IsEdgeContact(last_contact.data());
  }

  LogTouchReport(
      "single", report.data(), report.size(), reported_contact_count, *frame);
  return frame->edge_touch ? TouchReadStatus::kSuccess
                           : TouchReadStatus::kNoData;
}

TouchReadStatus Hi8561Touch::ReadTouchFrame(TouchFrame* frame) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (frame == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read touch frame failed (frame is null)\n");
    return TouchReadStatus::kInvalidData;
  }
  *frame = TouchFrame();

  if (!IsSectionValid(runtime_layout_.coordinate_report,
          kTouchReportReadSize)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 read touch frame failed (invalid report section: "
        "address 0X%08lX, size %lu)\n",
        static_cast<unsigned long>(
            runtime_layout_.coordinate_report.address),
        static_cast<unsigned long>(runtime_layout_.coordinate_report.length));
    return TouchReadStatus::kInvalidData;
  }

  std::array<uint8_t, kTouchReportReadSize> report{};
  if (!ReadEram(runtime_layout_.coordinate_report.address, report.data(),
          report.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 read touch frame failed (I2C transfer failed)\n");
    return TouchReadStatus::kBusError;
  }

  const uint8_t reported_contact_count = report[0];
  if (reported_contact_count > kMaxTouchContactCount) {
    LogTouchReport(
        "multi", report.data(), report.size(), reported_contact_count, *frame);
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 read touch frame failed (contact count: %u)\n",
        static_cast<unsigned int>(reported_contact_count));
    return TouchReadStatus::kInvalidData;
  }

  frame->sequence = report[1];
  frame->gesture = report[2];
  std::copy_n(
      &report[kTouchStateOffset], frame->state.size(), frame->state.begin());
  if (reported_contact_count == 0) {
    LogTouchReport(
        "multi", report.data(), report.size(), reported_contact_count, *frame);
    return frame->gesture != static_cast<uint8_t>(Gesture::kNone)
               ? TouchReadStatus::kSuccess
               : TouchReadStatus::kNoData;
  }

  uint8_t accounted_contacts = 0;
  for (size_t slot = 0; slot < kMaxTouchContactCount; ++slot) {
    const size_t offset = kTouchCoordinateOffset + slot * kTouchBytesPerContact;
    const uint8_t* contact_data = &report[offset];
    TouchContact contact;
    if (!ParseContact(contact_data, static_cast<uint8_t>(slot + 1), &contact)) {
      if (IsEdgeContact(contact_data)) {
        frame->edge_touch = true;
        ++accounted_contacts;
        if (accounted_contacts >= reported_contact_count) {
          break;
        }
      }
      continue;
    }

    if (frame->contact_count >= frame->contacts.size()) {
      break;
    }
    frame->contacts[frame->contact_count++] = contact;
    ++accounted_contacts;

    if (accounted_contacts >= reported_contact_count) {
      break;
    }
  }

  if (frame->contact_count == 0 && !frame->edge_touch &&
      frame->gesture == static_cast<uint8_t>(Gesture::kNone)) {
    LogTouchReport(
        "multi", report.data(), report.size(), reported_contact_count, *frame);
    return TouchReadStatus::kNoData;
  }
  LogTouchReport(
      "multi", report.data(), report.size(), reported_contact_count, *frame);
  return TouchReadStatus::kSuccess;
}

bool Hi8561Touch::ReadFirmwareInfo(FirmwareInfo* firmware_info) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (firmware_info == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read firmware info failed (output is null)\n");
    return false;
  }
  *firmware_info = FirmwareInfo();

  if (runtime_layout_.dsram_section_count <=
      kDsramFirmwareConfigSectionIndex) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read firmware info failed (driver is not initialized)\n");
    return false;
  }

  SectionInfo firmware_config;
  if (!ReadSectionInfo(kDsramSectionTableAddress,
          runtime_layout_.dsram_section_count,
          kDsramFirmwareConfigSectionIndex, &firmware_config) ||
      !IsSectionValid(firmware_config, kFirmwareConfigSize)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read firmware info failed (firmware section unavailable: "
        "address 0X%08lX, size %lu)\n",
        static_cast<unsigned long>(firmware_config.address),
        static_cast<unsigned long>(firmware_config.length));
    return false;
  }

  return ReadFirmwareInfoFromSection(firmware_config, firmware_info);
}

void Hi8561Touch::LogTouchReport(const char* mode, const uint8_t* report,
    size_t report_size, uint8_t reported_contact_count,
    const TouchFrame& frame) {
  if (!ShouldLog(LogLevel::kDebug) || mode == nullptr || report == nullptr ||
      report_size == 0) {
    return;
  }

  const int64_t now_ms = GetSystemTimeMs();
  if (now_ms - last_debug_report_ms_ < kDebugReportIntervalMs) {
    return;
  }
  last_debug_report_ms_ = now_ms;

  std::array<char, kTouchReportReadSize * 3> raw_text{};
  size_t raw_used = 0;
  for (size_t i = 0; i < report_size; ++i) {
    const int written = std::snprintf(raw_text.data() + raw_used,
        raw_text.size() - raw_used, i == 0 ? "%02X" : " %02X",
        static_cast<unsigned int>(report[i]));
    if (written < 0 || static_cast<size_t>(written) >=
                           raw_text.size() - raw_used) {
      break;
    }
    raw_used += static_cast<size_t>(written);
  }

  std::array<char, kMaxTouchContactCount * 48 + 1> point_text{};
  size_t point_used = 0;
  for (size_t i = 0; i < frame.contact_count; ++i) {
    const TouchContact& contact = frame.contacts[i];
    const int written = std::snprintf(point_text.data() + point_used,
        point_text.size() - point_used,
        "%sP%u{id:%u,x:%u,y:%u,p:%u,tool:%u}", i == 0 ? "" : " ",
        static_cast<unsigned int>(i + 1),
        static_cast<unsigned int>(contact.id),
        static_cast<unsigned int>(contact.x),
        static_cast<unsigned int>(contact.y),
        static_cast<unsigned int>(contact.pressure),
        static_cast<unsigned int>(contact.tool));
    if (written < 0 || static_cast<size_t>(written) >=
                           point_text.size() - point_used) {
      break;
    }
    point_used += static_cast<size_t>(written);
  }

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "HI8561 touch report (mode: %s, reported: %u, parsed: %u, "
      "sequence: %u, gesture: 0X%02X, edge: %s, state: %02X %02X %02X "
      "%02X %02X, points: %s, raw[%u]: %s)\n",
      mode, static_cast<unsigned int>(reported_contact_count),
      static_cast<unsigned int>(frame.contact_count),
      static_cast<unsigned int>(frame.sequence),
      static_cast<unsigned int>(frame.gesture),
      frame.edge_touch ? "yes" : "no",
      static_cast<unsigned int>(frame.state[0]),
      static_cast<unsigned int>(frame.state[1]),
      static_cast<unsigned int>(frame.state[2]),
      static_cast<unsigned int>(frame.state[3]),
      static_cast<unsigned int>(frame.state[4]),
      point_used == 0 ? "none" : point_text.data(),
      static_cast<unsigned int>(report_size), raw_text.data());
}

bool Hi8561Touch::ParseContact(
    const uint8_t* data, uint8_t contact_id, TouchContact* contact) {
  if (data == nullptr || contact == nullptr || contact_id == 0) {
    return false;
  }
  const uint16_t x = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  const uint16_t y = (static_cast<uint16_t>(data[2]) << 8) | data[3];
  if (x == std::numeric_limits<uint16_t>::max() &&
      y == std::numeric_limits<uint16_t>::max()) {
    return false;
  }

  contact->id = contact_id;
  contact->x = x;
  contact->y = y;
  contact->pressure = data[4];
  contact->tool = TouchToolType::kFinger;
  return true;
}

bool Hi8561Touch::IsEdgeContact(const uint8_t* data) {
  return data != nullptr && data[0] == 0xFF && data[1] == 0xFF &&
         data[2] == 0xFF && data[3] == 0xFF && data[4] == 0;
}

bool Hi8561Touch::SetHighSensitivityEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  return WriteHostBooleanSetting(
      kHighSensitivityOffset, enabled, 0x3A, 0xA3);
}

bool Hi8561Touch::SetGestureWakeEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  const bool result =
      WriteHostBooleanSetting(kGestureWakeOffset, enabled, 0x01, 0x00);
  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "HI8561 gesture wake state changed (enabled: %s, result: %s)\n",
      enabled ? "yes" : "no", result ? "success" : "failed");
  return result;
}

bool Hi8561Touch::SetUsbConnected(bool connected) {
  std::lock_guard<std::mutex> lock(mutex_);
  return WriteHostBooleanSetting(kUsbStateOffset, connected, 0x01, 0x00);
}

bool Hi8561Touch::SetRotationBorderMode(RotationBorderMode mode) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!IsSectionValid(
          runtime_layout_.host, kRotationBorderOffset + kRuntimeFieldSize)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 set rotation border failed (feature is not supported)\n");
    return false;
  }

  switch (mode) {
    case RotationBorderMode::kPortrait:
    case RotationBorderMode::kLandscapeNotchLeft:
    case RotationBorderMode::kLandscapeNotchRight:
      break;
    default:
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "HI8561 set rotation border failed (invalid mode: 0X%04X)\n",
          static_cast<unsigned int>(mode));
      return false;
  }

  const uint16_t value = static_cast<uint16_t>(mode);
  const uint8_t data[] = {
      static_cast<uint8_t>(value),
      static_cast<uint8_t>(value >> 8),
  };
  return WriteRuntimeMemory(
      runtime_layout_.host.address + kRotationBorderOffset, data,
      sizeof(data));
}

bool Hi8561Touch::SetEarphoneConnected(
    bool analog_connected, bool usb_connected) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!IsSectionValid(
          runtime_layout_.host, kEarphoneStateOffset + kRuntimeFieldSize)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 set earphone state failed (feature is not supported)\n");
    return false;
  }

  const uint8_t data[] = {
      static_cast<uint8_t>(analog_connected ? 1 : 0),
      static_cast<uint8_t>(usb_connected ? 1 : 0),
  };
  return WriteRuntimeMemory(
      runtime_layout_.host.address + kEarphoneStateOffset, data,
      sizeof(data));
}

bool Hi8561Touch::SetVirtualProximityEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  return WriteHostBooleanSetting(
      kVirtualProximityOffset, enabled, 0x3A, 0xA3);
}

bool Hi8561Touch::GetFrequencyBand(uint8_t* frequency_band) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (frequency_band == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 get frequency band failed (output is null)\n");
    return false;
  }
  if (runtime_layout_.dsram_section_count <= kDsramDebugSectionIndex) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 get frequency band failed (driver is not initialized)\n");
    return false;
  }

  SectionInfo debug;
  if (!ReadSectionInfo(kDsramSectionTableAddress,
          runtime_layout_.dsram_section_count, kDsramDebugSectionIndex,
          &debug) ||
      !IsSectionValid(debug, kFrequencyBandOffset + kRuntimeFieldSize)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 get frequency band failed (feature is not supported)\n");
    return false;
  }

  uint8_t data[kRuntimeFieldSize] = {};
  if (!ReadRuntimeMemory(
          debug.address + kFrequencyBandOffset, data, sizeof(data))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 get frequency band failed (I2C transfer failed)\n");
    return false;
  }
  *frequency_band = data[0];
  return true;
}

bool Hi8561Touch::DiscoverRuntimeLayout() {
  uint8_t ready_data[2] = {};
  bool section_ready = false;
  for (size_t attempt = 0; attempt < 25; ++attempt) {
    if (!ReadEram(kEramAddress, ready_data, sizeof(ready_data))) {
      return false;
    }
    const uint16_t ready_value = static_cast<uint16_t>(ready_data[0]) |
                                 (static_cast<uint16_t>(ready_data[1]) << 8);
    if (ready_value == kSectionReadyValue) {
      section_ready = true;
      break;
    }
    DelayMs(2);
  }
  if (!section_ready) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 dynamic section discovery failed (ready value: 0X%02X%02X)\n",
        static_cast<unsigned int>(ready_data[1]),
        static_cast<unsigned int>(ready_data[0]));
    return false;
  }

  uint8_t count_data[4] = {};
  if (!ReadEram(kEramAddress + 2, count_data, sizeof(count_data))) {
    return false;
  }
  const uint16_t dsram_count = static_cast<uint16_t>(count_data[0]) |
                               (static_cast<uint16_t>(count_data[1]) << 8);
  if (!ReadEram(kEsramCountAddress, count_data, sizeof(count_data))) {
    return false;
  }
  const uint16_t esram_count = static_cast<uint16_t>(count_data[0]) |
                               (static_cast<uint16_t>(count_data[1]) << 8);
  if (dsram_count <= kDsramHostSectionIndex ||
      dsram_count > kMaxDsramSectionCount ||
      esram_count <= kEsramCoordinateSectionIndex ||
      esram_count > kMaxEsramSectionCount) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 dynamic section discovery failed (DSRAM count: %u, "
        "ESRAM count: %u)\n",
        static_cast<unsigned int>(dsram_count),
        static_cast<unsigned int>(esram_count));
    return false;
  }

  SectionInfo host;
  SectionInfo coordinate;
  if (!ReadSectionInfo(kDsramSectionTableAddress, dsram_count,
          kDsramHostSectionIndex, &host) ||
      !ReadSectionInfo(kEsramSectionTableAddress, esram_count,
          kEsramCoordinateSectionIndex, &coordinate)) {
    return false;
  }

  if (!IsSectionValid(coordinate, kPrimaryReportSize) ||
      coordinate.address < kEramAddress ||
      coordinate.address + kPrimaryReportSize >
          kEramAddress + kEramSize) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 dynamic section discovery failed (invalid required section: "
        "host 0X%08lX/%lu, coordinate 0X%08lX/%lu)\n",
        static_cast<unsigned long>(host.address),
        static_cast<unsigned long>(host.length),
        static_cast<unsigned long>(coordinate.address),
        static_cast<unsigned long>(coordinate.length));
    return false;
  }

  runtime_layout_.dsram_section_count = dsram_count;
  runtime_layout_.host = host;
  runtime_layout_.coordinate_report = coordinate;
  return true;
}

bool Hi8561Touch::ReadFirmwareInfoFromSection(
    const SectionInfo& firmware_config_section,
    FirmwareInfo* firmware_info) {
  if (firmware_info == nullptr) {
    return false;
  }

  uint8_t config[kFirmwareConfigSize] = {};
  uint8_t version_info[kFirmwareVersionSize] = {};
  uint8_t panel_info[kRuntimeFieldSize] = {};

  if (!SetBackdoorModeEnabled(true)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read firmware info failed (enter backdoor mode failed)\n");
    return false;
  }

  const bool config_read = ReadBackdoorMemory(
      firmware_config_section.address, config, sizeof(config));
  const bool version_supported =
      IsSectionValid(runtime_layout_.host,
          kFirmwareVersionOffset + kFirmwareVersionSize);
  const bool version_read =
      !version_supported ||
      ReadBackdoorMemory(
          runtime_layout_.host.address + kFirmwareVersionOffset,
          version_info, sizeof(version_info));
  const bool panel_supported = IsSectionValid(
      runtime_layout_.host, kPanelInfoOffset + kRuntimeFieldSize);
  const bool panel_read =
      !panel_supported ||
      ReadBackdoorMemory(runtime_layout_.host.address + kPanelInfoOffset,
          panel_info, sizeof(panel_info));
  const bool backdoor_exit = SetBackdoorModeEnabled(false);

  if (!backdoor_exit) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read firmware info failed (exit backdoor mode failed)\n");
    return false;
  }
  if (!config_read) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read firmware info failed (configuration read failed)\n");
    return false;
  }

  FirmwareInfo parsed_info;
  parsed_info.x_resolution = static_cast<uint16_t>(config[0]) |
                             (static_cast<uint16_t>(config[1]) << 8);
  parsed_info.y_resolution = static_cast<uint16_t>(config[2]) |
                             (static_cast<uint16_t>(config[3]) << 8);
  parsed_info.x_channel_count = config[4];
  parsed_info.y_channel_count = config[5];
  if (parsed_info.x_resolution == 0 || parsed_info.y_resolution == 0 ||
      parsed_info.x_channel_count == 0 || parsed_info.y_channel_count == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 read firmware info failed (resolution: %u x %u, "
        "channels: %u x %u)\n",
        static_cast<unsigned int>(parsed_info.x_resolution),
        static_cast<unsigned int>(parsed_info.y_resolution),
        static_cast<unsigned int>(parsed_info.x_channel_count),
        static_cast<unsigned int>(parsed_info.y_channel_count));
    return false;
  }

  if (version_supported) {
    if (!version_read) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "HI8561 optional firmware version read failed\n");
    } else {
      parsed_info.customer_id_version =
          static_cast<uint32_t>(version_info[0]) |
          (static_cast<uint32_t>(version_info[1]) << 8) |
          (static_cast<uint32_t>(version_info[2]) << 16) |
          (static_cast<uint32_t>(version_info[3]) << 24);
      parsed_info.firmware_version =
          static_cast<uint32_t>(version_info[4]) |
          (static_cast<uint32_t>(version_info[5]) << 8) |
          (static_cast<uint32_t>(version_info[6]) << 16) |
          (static_cast<uint32_t>(version_info[7]) << 24);
    }
  }

  if (panel_supported) {
    if (!panel_read) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "HI8561 optional panel information read failed\n");
    } else {
      parsed_info.panel_maker = panel_info[0];
      parsed_info.panel_version = panel_info[1];
    }
  }
  *firmware_info = parsed_info;
  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "HI8561 firmware info read success (resolution: %u x %u, "
      "channels: %u x %u, customer version: 0X%08lX, "
      "firmware version: 0X%08lX, panel: %u/%u)\n",
      static_cast<unsigned int>(parsed_info.x_resolution),
      static_cast<unsigned int>(parsed_info.y_resolution),
      static_cast<unsigned int>(parsed_info.x_channel_count),
      static_cast<unsigned int>(parsed_info.y_channel_count),
      static_cast<unsigned long>(parsed_info.customer_id_version),
      static_cast<unsigned long>(parsed_info.firmware_version),
      static_cast<unsigned int>(parsed_info.panel_maker),
      static_cast<unsigned int>(parsed_info.panel_version));
  return true;
}

bool Hi8561Touch::SetBackdoorModeEnabled(bool enabled) {
  if (enabled) {
    const uint8_t command[] = {0xF2, 0xAA, 0xF0, 0x0F, 0x55, 0x68};
    return bus_->Write(command, sizeof(command));
  }

  const uint8_t command[] = {0xF2, 0xAA, 0x88, 0x00, 0x00, 0x00};
  return bus_->Write(command, sizeof(command));
}

bool Hi8561Touch::ReadBackdoorMemory(
    uint32_t address, uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > UINT16_MAX ||
      length - 1 > UINT32_MAX - address) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 backdoor memory read failed (address: 0X%08lX, size: %zu)\n",
        static_cast<unsigned long>(address), length);
    return false;
  }

  const uint8_t command[] = {
      0xF3,
      static_cast<uint8_t>(address >> 24),
      static_cast<uint8_t>(address >> 16),
      static_cast<uint8_t>(address >> 8),
      static_cast<uint8_t>(address),
      0x03,
  };
  return bus_->WriteRead(command, sizeof(command), data, length);
}

bool Hi8561Touch::ReadEram(uint32_t address, uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > UINT16_MAX ||
      address < kEramAddress || address > kEramAddress + kEramSize ||
      length > kEramAddress + kEramSize - address) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 ERAM read failed (address: 0X%08lX, size: %zu)\n",
        static_cast<unsigned long>(address), length);
    return false;
  }

  return ReadBackdoorMemory(address, data, length);
}

bool Hi8561Touch::ReadRuntimeMemory(
    uint32_t address, uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > UINT16_MAX) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 runtime read failed (address: 0X%08lX, size: %zu)\n",
        static_cast<unsigned long>(address), length);
    return false;
  }

  const uint8_t command[] = {
      static_cast<uint8_t>(address >> 24),
      static_cast<uint8_t>(address >> 16),
      static_cast<uint8_t>(address >> 8),
      static_cast<uint8_t>(address),
      static_cast<uint8_t>(length >> 8),
      static_cast<uint8_t>(length),
  };
  return bus_->WriteRead(command, sizeof(command), data, length);
}

bool Hi8561Touch::WriteRuntimeMemory(
    uint32_t address, const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > 2) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 runtime write failed (address: 0X%08lX, size: %zu)\n",
        static_cast<unsigned long>(address), length);
    return false;
  }

  std::array<uint8_t, 8> packet{};
  packet[0] = static_cast<uint8_t>(address >> 24);
  packet[1] = static_cast<uint8_t>(address >> 16);
  packet[2] = static_cast<uint8_t>(address >> 8);
  packet[3] = static_cast<uint8_t>(address);
  packet[4] = static_cast<uint8_t>(length >> 8);
  packet[5] = static_cast<uint8_t>(length);
  std::copy_n(data, length, packet.begin() + 6);
  return bus_->Write(packet.data(), 6 + length);
}

bool Hi8561Touch::ReadSectionInfo(uint32_t table_address, size_t section_count,
    size_t section_index, SectionInfo* section) {
  if (section == nullptr || section_index >= section_count) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "HI8561 section read failed (index: %zu, count: %zu)\n", section_index,
        section_count);
    return false;
  }

  uint8_t data[8] = {};
  if (!ReadEram(
          table_address + section_index * sizeof(data), data, sizeof(data))) {
    return false;
  }
  section->address = static_cast<uint32_t>(data[0]) |
                     (static_cast<uint32_t>(data[1]) << 8) |
                     (static_cast<uint32_t>(data[2]) << 16) |
                     (static_cast<uint32_t>(data[3]) << 24);
  section->length = static_cast<uint32_t>(data[4]) |
                    (static_cast<uint32_t>(data[5]) << 8) |
                    (static_cast<uint32_t>(data[6]) << 16) |
                    (static_cast<uint32_t>(data[7]) << 24);
  return true;
}

bool Hi8561Touch::IsSectionValid(
    const SectionInfo& section, size_t required_length) {
  return section.address != 0 && section.length >= required_length &&
         required_length <= UINT32_MAX - section.address;
}

bool Hi8561Touch::WriteHostBooleanSetting(size_t offset, bool enabled,
    uint8_t enabled_low_byte, uint8_t enabled_high_byte) {
  if (offset > SIZE_MAX - kRuntimeFieldSize ||
      !IsSectionValid(runtime_layout_.host, offset + kRuntimeFieldSize)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "HI8561 setting write failed (feature is not supported, offset: %zu)\n",
        offset);
    return false;
  }
  const uint8_t data[] = {
      static_cast<uint8_t>(enabled ? enabled_low_byte : 0),
      static_cast<uint8_t>(enabled ? enabled_high_byte : 0),
  };
  return WriteRuntimeMemory(
      runtime_layout_.host.address + offset, data, sizeof(data));
}

}  // namespace cpp_bus_driver
