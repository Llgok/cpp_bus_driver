/*
 * @Description: GT9895 电容触摸控制器驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-07-09 09:15:31
 * @LastEditTime: 2026-08-11 00:00:00
 * @License: GPL 3.0
 */
#include "gt9895.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>

namespace cpp_bus_driver {

bool Gt9895::Init(int32_t freq_hz) {
  last_debug_report_ms_ = 0;
  last_failure_report_ms_ = 0;

  if (i2c_address_ != kDefaultI2cAddress &&
      i2c_address_ != kAlternateI2cAddress) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 init failed (unsupported I2C address: 0X%02X)\n",
        static_cast<unsigned int>(i2c_address_));
    return false;
  }

  const TouchCoordinateTransform& transform = coordinate_transform_;
  const bool transform_disabled =
      transform.source_width == 0 && transform.source_height == 0 &&
      transform.target_width == 0 && transform.target_height == 0;
  if (!transform_disabled && !IsCoordinateTransformValid(transform)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 init failed (invalid coordinate transform: %u x %u -> "
        "%u x %u)\n",
        static_cast<unsigned int>(transform.source_width),
        static_cast<unsigned int>(transform.source_height),
        static_cast<unsigned int>(transform.target_width),
        static_cast<unsigned int>(transform.target_height));
    return false;
  }

  if (!ResetController()) {
    return false;
  }
  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GT9895 init failed\n");
    return false;
  }

  ChipInfo chip_info;
  if (!ReadChipInfo(&chip_info)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 init failed (firmware information is invalid)\n");
    return false;
  }
  chip_info_ = chip_info;

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "GT9895 init success (product id: %s, sensor id: %u, version: "
      "0X%08lX)\n",
      chip_info_.patch_product_id.data(),
      static_cast<unsigned int>(chip_info_.sensor_id),
      static_cast<unsigned long>(chip_info_.patch_version));
  last_debug_report_ms_ = GetSystemTimeMs();
  return true;
}

bool Gt9895::Deinit(bool delete_bus) {
  bool result = ChipI2cGuide::Deinit(delete_bus);
  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "GT9895 deinit failed\n");
  }

  if (irq_ != kDefaultValue) {
    result &= ResetGpio(irq_);
  }
  if (rst_ != kDefaultValue) {
    result &= ResetGpio(rst_);
  }
  chip_info_ = ChipInfo();
  last_debug_report_ms_ = 0;
  last_failure_report_ms_ = 0;
  return result;
}

TouchReadStatus Gt9895::ReadPrimaryTouch(TouchFrame* frame) {
  return ReadTouchReport(1, frame);
}

TouchReadStatus Gt9895::ReadTouchFrame(TouchFrame* frame) {
  return ReadTouchReport(kMaxTouchContactCount, frame);
}

TouchReadStatus Gt9895::ReadTouchReport(
    size_t max_contacts, TouchFrame* frame) {
  if (frame == nullptr || max_contacts == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "GT9895 read touch report failed (invalid output)\n");
    return TouchReadStatus::kInvalidData;
  }

  const size_t contact_limit =
      std::min(max_contacts, static_cast<size_t>(kMaxTouchContactCount));
  const size_t prefetched_contact_count =
      contact_limit == 1 ? 1 : kPrefetchedContactCount;
  const size_t initial_report_size =
      contact_limit == 1 ? kPrimaryReportSize : kInitialReportSize;
  const char* const mode = contact_limit == 1 ? "single" : "multi";
  std::array<uint8_t, kMaximumReportSize> report{};
  TouchReadStatus failure_status = TouchReadStatus::kInvalidData;
  for (size_t attempt = 0; attempt < kReadAttemptCount; ++attempt) {
    const bool retry_available = attempt + 1 < kReadAttemptCount;
    *frame = TouchFrame();
    size_t report_size = initial_report_size;
    if (!ReadRegister(
            kTouchEventAddress, report.data(), initial_report_size)) {
      failure_status = TouchReadStatus::kBusError;
      if (!retry_available) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "GT9895 read touch report failed (initial transfer, attempts: "
            "%u)\n",
            static_cast<unsigned int>(kReadAttemptCount));
      }
      if (retry_available) {
        DelayMs(kReadRetryDelayMs);
      }
      continue;
    }
    if (report[0] == 0) {
      LogTouchReport(mode, report.data(), report_size, 0, *frame);
      return TouchReadStatus::kNoData;
    }
    if (!HasValidChecksum(report.data(), kEventHeaderSize)) {
      failure_status = TouchReadStatus::kInvalidData;
      if (!retry_available) {
        LogInvalidTouchReport(
            "event header checksum", report.data(), report_size, 0);
      }
      if (retry_available) {
        DelayMs(kReadRetryDelayMs);
      }
      continue;
    }
    if ((report[0] & kTouchEventMask) == 0) {
      frame->event_flags = report[0];
      const uint8_t reported_contact_count = report[2] & 0x0F;
      LogTouchReport(
          mode, report.data(), report_size, reported_contact_count, *frame);
      if (!ClearTouchStatus()) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "GT9895 read touch report failed (event acknowledgement)\n");
        return TouchReadStatus::kBusError;
      }
      return TouchReadStatus::kNoData;
    }

    uint8_t reported_contact_count = report[2] & 0x0F;
    if (reported_contact_count > kMaxTouchContactCount) {
      LogInvalidTouchReport(
          "contact count", report.data(), report_size, 0);
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "GT9895 read touch report failed (contact count: %u)\n",
          static_cast<unsigned int>(reported_contact_count));
      return TouchReadStatus::kInvalidData;
    }
    if (reported_contact_count == 0) {
      frame->event_flags = report[0];
      frame->edge_touch = (report[0] & kEdgeTouchMask) != 0;
      LogTouchReport(
          mode, report.data(), report_size, reported_contact_count, *frame);
      return frame->edge_touch ? TouchReadStatus::kSuccess
                               : TouchReadStatus::kNoData;
    }

    if (reported_contact_count > prefetched_contact_count &&
        contact_limit > 1) {
      const size_t complete_report_size =
          kEventHeaderSize +
          static_cast<size_t>(reported_contact_count) * kBytesPerContact +
          kChecksumSize;
      const size_t remaining_report_size =
          complete_report_size - initial_report_size;
      // 首次读取已经取得报告前缀，只续读剩余数据，避免重新读取整帧时
      // 控制器更新报告区导致前后两次数据属于不同帧。
      if (!ReadRegister(
              kTouchEventAddress + initial_report_size,
              report.data() + initial_report_size, remaining_report_size)) {
        failure_status = TouchReadStatus::kBusError;
        if (!retry_available) {
          LogMessage(LogLevel::kError, __FILE__, __LINE__,
              "GT9895 read touch report failed (remaining transfer, "
              "attempts: %u)\n",
              static_cast<unsigned int>(kReadAttemptCount));
        }
        if (retry_available) {
          DelayMs(kReadRetryDelayMs);
        }
        continue;
      }
      report_size = complete_report_size;
    }

    const uint8_t primary_contact_type = report[kEventHeaderSize] & 0x0F;
    const size_t payload_contact_count =
        primary_contact_type == kStylusType ||
                primary_contact_type == kStylusHoverType
            ? kPrefetchedContactCount
            : static_cast<size_t>(reported_contact_count);
    const size_t payload_size =
        payload_contact_count * kBytesPerContact + kChecksumSize;
    const bool payload_available =
        kEventHeaderSize + payload_size <= initial_report_size ||
        contact_limit > 1;
    if (payload_available &&
        !HasValidChecksum(&report[kEventHeaderSize], payload_size)) {
      failure_status = TouchReadStatus::kInvalidData;
      if (!retry_available) {
        LogInvalidTouchReport("contact payload checksum", report.data(),
            report_size, payload_size);
      }
      if (retry_available) {
        DelayMs(kReadRetryDelayMs);
      }
      continue;
    }

    frame->event_flags = report[0];
    frame->edge_touch = (report[0] & kEdgeTouchMask) != 0;
    const size_t parsed_contact_count =
        std::min(contact_limit, static_cast<size_t>(reported_contact_count));
    for (size_t i = 0; i < parsed_contact_count; ++i) {
      ParseContact(&report[kEventHeaderSize + i * kBytesPerContact],
          &frame->contacts[i]);
    }
    frame->contact_count = static_cast<uint8_t>(parsed_contact_count);
    LogTouchReport(
        mode, report.data(), report_size, reported_contact_count, *frame);
    return TouchReadStatus::kSuccess;
  }

  *frame = TouchFrame();
  return failure_status;
}

void Gt9895::LogTouchReport(const char* mode, const uint8_t* report,
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

  std::array<char, kMaximumReportSize * 3> raw_text{};
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
      "GT9895 touch report (mode: %s, event: 0X%02X, reported: %u, "
      "parsed: %u, edge: %s, points: %s, raw[%u]: %s)\n",
      mode, static_cast<unsigned int>(frame.event_flags),
      static_cast<unsigned int>(reported_contact_count),
      static_cast<unsigned int>(frame.contact_count),
      frame.edge_touch ? "yes" : "no",
      point_used == 0 ? "none" : point_text.data(),
      static_cast<unsigned int>(report_size), raw_text.data());
}

void Gt9895::LogInvalidTouchReport(const char* reason, const uint8_t* report,
    size_t report_size, size_t payload_size) {
  if (reason == nullptr || report == nullptr || report_size == 0) {
    return;
  }

  const int64_t now_ms = GetSystemTimeMs();
  if (now_ms - last_failure_report_ms_ < kFailureReportIntervalMs) {
    return;
  }
  last_failure_report_ms_ = now_ms;

  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "GT9895 read touch report failed (%s, attempts: %u)\n", reason,
      static_cast<unsigned int>(kReadAttemptCount));
  if (!ShouldLog(LogLevel::kDebug)) {
    return;
  }

  std::array<char, kMaximumReportSize * 3> raw_text{};
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

  uint32_t header_sum = 0;
  for (size_t i = 0; i + kChecksumSize < kEventHeaderSize; ++i) {
    header_sum += report[i];
  }
  const uint16_t header_expected =
      static_cast<uint16_t>(report[kEventHeaderSize - 2]) |
      (static_cast<uint16_t>(report[kEventHeaderSize - 1]) << 8);

  const bool payload_available = payload_size >= kChecksumSize &&
      kEventHeaderSize + payload_size <= report_size;
  uint32_t payload_sum = 0;
  uint16_t payload_expected = 0;
  if (payload_available) {
    const uint8_t* const payload = &report[kEventHeaderSize];
    for (size_t i = 0; i + kChecksumSize < payload_size; ++i) {
      payload_sum += payload[i];
    }
    payload_expected = static_cast<uint16_t>(payload[payload_size - 2]) |
                       (static_cast<uint16_t>(payload[payload_size - 1]) << 8);
  }

  LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
      "GT9895 invalid touch report (reason: %s, event: 0X%02X, contacts: "
      "%u, header checksum: 0X%04X/0X%04X, payload size: %u, payload "
      "available: %s, payload checksum: 0X%04X/0X%04X, raw[%u]: %s)\n",
      reason, static_cast<unsigned int>(report[0]),
      static_cast<unsigned int>(report[2] & 0x0F),
      static_cast<unsigned int>(static_cast<uint16_t>(header_sum)),
      static_cast<unsigned int>(header_expected),
      static_cast<unsigned int>(payload_size),
      payload_available ? "yes" : "no",
      static_cast<unsigned int>(static_cast<uint16_t>(payload_sum)),
      static_cast<unsigned int>(payload_expected),
      static_cast<unsigned int>(report_size), raw_text.data());
}

bool Gt9895::EnterSleep() {
  if (irq_ != kDefaultValue) {
    if (!SetGpioMode(irq_, GpioMode::kOutput, GpioStatus::kPulldown) ||
        !GpioWrite(irq_, 0)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "GT9895 sleep failed (interrupt pin: %ld)\n",
          static_cast<long>(irq_));
      return false;
    }
  }

  const uint8_t command[] = {
      static_cast<uint8_t>(kCommandAddress >> 24),
      static_cast<uint8_t>(kCommandAddress >> 16),
      static_cast<uint8_t>(kCommandAddress >> 8),
      static_cast<uint8_t>(kCommandAddress),
      0x00,
      0x00,
      0x04,
      0x84,
      0x88,
      0x00,
  };
  if (!bus_->Write(command, sizeof(command))) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 sleep failed (command transfer failed)\n");
    return false;
  }
  return true;
}

bool Gt9895::WakeUp() {
  if (rst_ == kDefaultValue) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 wake failed (reset pin is not configured)\n");
    return false;
  }

  if (irq_ != kDefaultValue) {
    if (!SetGpioMode(irq_, GpioMode::kOutput, GpioStatus::kPullup) ||
        !GpioWrite(irq_, 1)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "GT9895 wake failed (interrupt pin: %ld)\n", static_cast<long>(irq_));
      return false;
    }
    DelayMs(8);
  }
  return ResetController();
}

bool Gt9895::ResetController() {
  if (rst_ == kDefaultValue) {
    return true;
  }

  bool result = true;
  result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);
  result &= GpioWrite(rst_, 0);
  DelayMs(30);
  result &= GpioWrite(rst_, 1);
  DelayMs(100);
  if (irq_ != kDefaultValue) {
    result &= SetGpioMode(irq_, GpioMode::kInput);
  }
  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 reset failed (pin: %ld)\n", static_cast<long>(rst_));
  }
  return result;
}

bool Gt9895::ReadChipInfo(ChipInfo* chip_info) {
  if (chip_info == nullptr) {
    return false;
  }

  std::array<uint8_t, kFirmwareInfoSize> data{};
  bool read_success = false;
  for (size_t attempt = 0; attempt < 2; ++attempt) {
    if (ReadRegister(kFirmwareVersionAddress, data.data(), data.size()) &&
        HasValidChecksum(data.data(), data.size())) {
      read_success = true;
      break;
    }
    DelayMs(attempt == 0 ? 5 : 15);
  }
  if (!read_success) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 firmware information read failed (checksum or I2C error)\n");
    return false;
  }

  ChipInfo parsed;
  std::copy_n(data.begin(), 6, parsed.rom_product_id.begin());
  std::copy_n(data.begin() + 10, 8, parsed.patch_product_id.begin());
  parsed.rom_version = static_cast<uint32_t>(data[6]) |
                       (static_cast<uint32_t>(data[7]) << 8) |
                       (static_cast<uint32_t>(data[8]) << 16);
  parsed.patch_version = static_cast<uint32_t>(data[18]) |
                         (static_cast<uint32_t>(data[19]) << 8) |
                         (static_cast<uint32_t>(data[20]) << 16) |
                         (static_cast<uint32_t>(data[21]) << 24);
  parsed.sensor_id = data[23];

  const uint16_t product_id = parsed.patch_product_id[0] == '9' &&
                                      parsed.patch_product_id[1] == '8' &&
                                      parsed.patch_product_id[2] == '9' &&
                                      parsed.patch_product_id[3] == '5'
                                  ? kExpectedProductId
                                  : 0;
  if (product_id != kExpectedProductId) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "GT9895 firmware information is invalid (product id: %.8s)\n",
        parsed.patch_product_id.data());
    return false;
  }

  *chip_info = parsed;
  return true;
}

bool Gt9895::ReadRegister(uint32_t address, uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    return false;
  }
  const uint8_t command[] = {
      static_cast<uint8_t>(address >> 24),
      static_cast<uint8_t>(address >> 16),
      static_cast<uint8_t>(address >> 8),
      static_cast<uint8_t>(address),
  };
  return bus_->WriteRead(command, sizeof(command), data, length);
}

bool Gt9895::ClearTouchStatus() {
  const uint8_t command[] = {
      static_cast<uint8_t>(kTouchEventAddress >> 24),
      static_cast<uint8_t>(kTouchEventAddress >> 16),
      static_cast<uint8_t>(kTouchEventAddress >> 8),
      static_cast<uint8_t>(kTouchEventAddress),
      0x00,
  };
  return bus_->Write(command, sizeof(command));
}

void Gt9895::ParseContact(const uint8_t* data, TouchContact* contact) const {
  if (data == nullptr || contact == nullptr) {
    return;
  }

  uint16_t x =
      static_cast<uint16_t>(data[2]) | (static_cast<uint16_t>(data[3]) << 8);
  uint16_t y =
      static_cast<uint16_t>(data[4]) | (static_cast<uint16_t>(data[5]) << 8);
  const TouchCoordinateTransform& transform = coordinate_transform_;
  if (IsCoordinateTransformValid(transform)) {
    x = TransformCoordinate(x, transform.source_width, transform.target_width);
    y = TransformCoordinate(
        y, transform.source_height, transform.target_height);
  }

  contact->id = static_cast<uint8_t>(((data[0] >> 4) & 0x0F) + 1);
  contact->x = x;
  contact->y = y;
  contact->pressure =
      static_cast<uint16_t>(data[6]) | (static_cast<uint16_t>(data[7]) << 8);
  switch (data[0] & 0x0F) {
    case kStylusType:
      contact->tool = TouchToolType::kStylus;
      break;
    case kStylusHoverType:
      contact->tool = TouchToolType::kStylusHover;
      break;
    default:
      contact->tool = TouchToolType::kFinger;
      break;
  }
}

bool Gt9895::HasValidChecksum(const uint8_t* data, size_t length) {
  if (data == nullptr || length < kChecksumSize) {
    return false;
  }

  uint32_t checksum = 0;
  for (size_t i = 0; i < length - kChecksumSize; ++i) {
    checksum += data[i];
  }
  const uint16_t expected = static_cast<uint16_t>(data[length - 2]) |
                            (static_cast<uint16_t>(data[length - 1]) << 8);
  return static_cast<uint16_t>(checksum) == expected;
}

bool Gt9895::IsCoordinateTransformValid(
    const TouchCoordinateTransform& transform) {
  return transform.source_width != 0 && transform.source_height != 0 &&
         transform.target_width != 0 && transform.target_height != 0;
}

uint16_t Gt9895::TransformCoordinate(
    uint16_t coordinate, uint16_t source_extent, uint16_t target_extent) {
  const uint32_t scaled =
      static_cast<uint32_t>(coordinate) * target_extent / source_extent;
  return static_cast<uint16_t>(
      std::min<uint32_t>(scaled, static_cast<uint32_t>(target_extent - 1)));
}

}  // namespace cpp_bus_driver
