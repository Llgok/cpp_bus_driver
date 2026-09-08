/*
 * @Description: 基于 SDIO 的 ESP-AT 通信驱动实现
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-09-05 14:57:02
 * @License: GPL 3.0
 */
#include "chip/sdio/esp_at.h"

#include <algorithm>
#include <array>
#include <cstring>

#include "utility/byte_search.h"

namespace cpp_bus_driver {
namespace {

/**
 * @brief 将字节长度向上对齐到4字节边界
 * @param value 原始字节长度
 * @return 4字节对齐后的字节长度
 */
size_t AlignTo4(size_t value) { return (value + 3) & ~static_cast<size_t>(3); }

}  // namespace

bool EspAt::Init(int32_t freq_hz) {
  connect_.status = true;
  connect_.error_count = 0;
  connect_.receive_total_length_index = 0;

  if (rst_ != kPinNotConnected) {
    bool result = true;
    result &= SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    result &= GpioWrite(rst_, 0);
    DelayMs(50);
    result &= GpioWrite(rst_, 1);
    DelayMs(1000);
    if (!result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Rst failed\n");
      return false;
    }
  } else if (rst_callback_ != nullptr) {
    rst_callback_(0);
    DelayMs(50);
    rst_callback_(1);
    DelayMs(1000);
  }

  if (!SdioChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    return false;
  }

  if (!ConfigureSdioFunctions()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ConfigureSdioFunctions failed\n");
    return false;
  }

  if (!WaitForReady()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "WaitForReady failed\n");
    return false;
  }

  if (!GetChipId()) {
    return false;
  }
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "ESP-AT command response received\n");

  return true;
}

bool EspAt::Deinit() {
  bool result = true;

  if (!SdioChipBase::Deinit()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Deinit failed\n");
    result = false;
  }

  if (rst_ != kPinNotConnected) {
    result &= ResetGpio(rst_);
  }

  return result;
}

bool EspAt::ConfigureSdioFunctions() {
  // 启用功能 1
  if (!WriteRegister(
          0, static_cast<uint32_t>(RegisterAddress::kSdIoCccrFnEnable), 6)) {
    return false;
  }
  if (!WriteRegister(
          0, static_cast<uint32_t>(RegisterAddress::kSdIoCccrFnReady), 6)) {
    return false;
  }

  // 启用功能 1、功能 2 和主中断
  if (!WriteRegister(
          0, static_cast<uint32_t>(RegisterAddress::kSdIoCccrIntEnable), 7)) {
    return false;
  }

  if (!WriteRegister(
          0, static_cast<uint32_t>(RegisterAddress::kSdIoCccrBlksizel), 0)) {
    return false;
  }
  if (!WriteRegister(
          0, static_cast<uint32_t>(RegisterAddress::kSdIoCccrBlksizeh), 2)) {
    return false;
  }

  if (!WriteRegister(0, static_cast<uint32_t>(0x110), 0)) {
    return false;
  }
  // 将块大小设置为 512 字节（0x200）
  if (!WriteRegister(0, static_cast<uint32_t>(0x111), 2)) {
    return false;
  }

  if (!WriteRegister(0, static_cast<uint32_t>(0x210), 0)) {
    return false;
  }
  if (!WriteRegister(0, static_cast<uint32_t>(0x210), 2)) {
    return false;
  }

  return true;
}

bool EspAt::WaitForReady() {
  if (!WaitForResponse("\r\nready\r\n")) {
    connect_.status = false;
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Connect timeout\n");
    return false;
  }
  return true;
}

bool EspAt::GetChipId() {
  static constexpr char kCommand[] = "AT\r\n";
  return SendPacket(kCommand, sizeof(kCommand) - 1) &&
         WaitForResponse("\r\nOK\r\n");
}

bool EspAt::WaitForResponse(const char* text) {
  if (text == nullptr) {
    return false;
  }
  const size_t text_length = std::strlen(text);
  // 保留上一块的短尾部，支持 ready/OK 跨接收块出现。
  std::array<uint8_t, kMaxTransmitBlockBufferSize> buffer{};
  if (text_length == 0 || text_length > buffer.size()) {
    return false;
  }
  size_t retained = 0;
  const int64_t started_ms = GetSystemTimeMs();
  while (connect_.status &&
         GetSystemTimeMs() - started_ms < kTransmitTimeoutCount * 10U) {
    const uint32_t flags = GetInterruptFlags();
    if (HasReceivePacketInterrupt(flags) && !ClearInterruptFlags(flags)) {
      return false;
    }
    const size_t available = GetReceiveDataLength();
    if (available == 0) {
      DelayMs(10);
      continue;
    }
    const size_t length = std::min(available, buffer.size() - retained);
    if (!ReadPacketData(buffer.data() + retained, length)) {
      return false;
    }
    const size_t total = retained + length;
    if (byte_search::ContainsText(buffer.data(), total, text, text_length)) {
      return true;
    }
    retained = std::min(total, text_length - 1);
    std::memmove(buffer.data(), buffer.data() + total - retained, retained);
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "ESP-AT response wait failed (%s)\n",
      connect_.status ? "timeout" : "disconnected");
  return false;
}

bool EspAt::IsConnected() const { return connect_.status; }

void EspAt::UpdateConnectionErrorCount(int8_t delta) {
  connect_.error_count += delta;
  if (connect_.error_count < 0) {
    connect_.error_count = 0;
  } else if (connect_.error_count > kConnectErrorCount) {
    connect_.error_count = kConnectErrorCount + 1;
    connect_.status = false;
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Connect error count > kConnectErrorCount\n");
  }
}

uint32_t EspAt::GetInterruptFlags() {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return kInvalidInterruptFlags;
  }

  uint32_t interrupt_flags = 0;

  if (!ReadRegister(1, static_cast<uint32_t>(RegisterAddress::kInterruptRaw),
          &interrupt_flags, sizeof(interrupt_flags))) {
    UpdateConnectionErrorCount(1);
    return kInvalidInterruptFlags;
  }

  UpdateConnectionErrorCount(-1);
  return interrupt_flags;
}

bool EspAt::ClearInterruptFlags(uint32_t interrupt_flags) {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  if (!WriteRegister(1, static_cast<uint32_t>(RegisterAddress::kInterruptClear),
          &interrupt_flags, sizeof(interrupt_flags))) {
    UpdateConnectionErrorCount(1);
    return false;
  }

  UpdateConnectionErrorCount(-1);
  return true;
}

bool EspAt::HasReceivePacketInterrupt(uint32_t interrupt_flags) const {
  if (interrupt_flags == kInvalidInterruptFlags) {
    return false;
  }

  return (interrupt_flags &
             static_cast<uint32_t>(InterruptFlag::kRxNewPacket)) != 0;
}

uint32_t EspAt::GetReceiveDataLength() {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return 0;
  }

  uint32_t received_total_length = 0;

  if (!ReadRegister(1, static_cast<uint32_t>(RegisterAddress::kPacketLength),
          &received_total_length, sizeof(received_total_length))) {
    UpdateConnectionErrorCount(1);
    return 0;
  }

  received_total_length &= kRxBufferMask;
  const uint32_t available_length = (received_total_length + kRxBufferMax -
                                        connect_.receive_total_length_index) %
                                    kRxBufferMax;

  UpdateConnectionErrorCount(-1);
  return available_length;
}

bool EspAt::ReceivePacket(uint8_t* data, size_t* byte) {
  if (byte == nullptr) {
    return false;
  }
  const size_t capacity = *byte;
  *byte = 0;
  if (data == nullptr || !connect_.status) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Invalid receive buffer or connection\n");
    return false;
  }
  const size_t length = GetReceiveDataLength();
  if (length == 0) {
    return false;
  }
  if (length > kMaxPacketSize) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Receive length exceeds SDIO address range\n");
    return false;
  }
  if (capacity < length) {
    *byte = length;
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Receive buffer is too small\n");
    return false;
  }
  if (!ReadPacketData(data, length)) {
    return false;
  }
  *byte = length;
  return true;
}

bool EspAt::ReadPacketData(uint8_t* data, size_t length) {
  if (data == nullptr || length == 0 || length > kMaxPacketSize ||
      !connect_.status) {
    return false;
  }
  const size_t block_length =
      (length / kMaxTransmitBlockBufferSize) * kMaxTransmitBlockBufferSize;
  const uint32_t end_address =
      static_cast<uint32_t>(RegisterAddress::kSlaveCmd53EndAddr);
  if (block_length != 0) {
    if (!bus_->ReadBlock(1, end_address - static_cast<uint32_t>(length), data,
            block_length)) {
      // 失败时无法确定从设备已经消费多少数据，禁止继续使用旧计数重试。
      connect_.status = false;
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "ESP-AT packet block read failed (function: 1, address: %#X, size: "
          "%zu)\n",
          static_cast<unsigned>(end_address - static_cast<uint32_t>(length)),
          block_length);
      return false;
    }
    connect_.receive_total_length_index =
        (connect_.receive_total_length_index + block_length) & kRxBufferMask;
  }
  const size_t tail_length = length - block_length;
  if (tail_length != 0) {
    // 余数最多 511 字节，四字节对齐后最多 512 字节。
    alignas(4) std::array<uint8_t, kMaxTransmitBlockBufferSize> tail{};
    if (!bus_->Read(1, end_address - static_cast<uint32_t>(tail_length),
            tail.data(), AlignTo4(tail_length))) {
      connect_.status = false;
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "ESP-AT packet tail read failed (function: 1, address: %#X, size: "
          "%zu)\n",
          static_cast<unsigned>(
              end_address - static_cast<uint32_t>(tail_length)),
          AlignTo4(tail_length));
      return false;
    }
    std::memcpy(data + block_length, tail.data(), tail_length);
    connect_.receive_total_length_index =
        (connect_.receive_total_length_index + tail_length) & kRxBufferMask;
  }
  UpdateConnectionErrorCount(-1);
  return true;
}

uint32_t EspAt::GetTransmitBufferBlockCount() {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return 0;
  }

  uint32_t token_register = 0;

  if (!ReadRegister(1, static_cast<uint32_t>(RegisterAddress::kTokenRdata),
          &token_register, sizeof(token_register))) {
    UpdateConnectionErrorCount(1);
    return 0;
  }

  return (token_register >> kTxBufferOffset) & kTxBufferMask;
}

bool EspAt::SendPacket(const char* data, size_t byte) {
  if (data == nullptr || byte == 0 || byte > kMaxPacketSize) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint16_t buffer_timeout_count = 0;

  while (1) {
    if (GetTransmitBufferBlockCount() * kMaxTransmitBlockBufferSize >= byte) {
      break;
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > 100)  // 超时
    {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "GetTransmitBufferBlockCount timeout\n");
      return false;
    }

    DelayMs(10);
  }

  size_t buffer_block_length =
      (byte / kMaxTransmitBlockBufferSize) * kMaxTransmitBlockBufferSize;
  if (buffer_block_length != 0) {
    // 多字节对齐发送
    if (!bus_->WriteBlock(1,
            static_cast<uint32_t>(RegisterAddress::kSlaveCmd53EndAddr) - byte,
            data, buffer_block_length)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "ESP-AT packet block write failed (function: 1, address: %#X, size: "
          "%zu)\n",
          static_cast<unsigned>(
              static_cast<uint32_t>(RegisterAddress::kSlaveCmd53EndAddr) -
              byte),
          buffer_block_length);
      UpdateConnectionErrorCount(1);
      return false;
    }
    byte -= buffer_block_length;
  }

  if (byte != 0) {
    // 4字节对齐发送
    const size_t aligned_length = AlignTo4(byte);
    alignas(4) std::array<uint8_t, kMaxTransmitBlockBufferSize> write_buffer{};
    std::memcpy(write_buffer.data(), data + buffer_block_length, byte);
    if (!bus_->Write(1,
            static_cast<uint32_t>(RegisterAddress::kSlaveCmd53EndAddr) - byte,
            write_buffer.data(), aligned_length)) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "ESP-AT packet tail write failed (function: 1, address: %#X, size: "
          "%zu)\n",
          static_cast<unsigned>(
              static_cast<uint32_t>(RegisterAddress::kSlaveCmd53EndAddr) -
              byte),
          aligned_length);
      UpdateConnectionErrorCount(1);
      return false;
    }
  }

  UpdateConnectionErrorCount(-1);
  return true;
}

bool EspAt::SendPacket(const std::string& data) {
  return SendPacket(data.data(), data.size());
}

bool EspAt::WaitForInterrupt(uint32_t timeout_ms) {
  return bus_->WaitInterrupt(timeout_ms);
}

bool EspAt::ReadRegister(
    uint32_t function, uint32_t reg, void* data, size_t length) {
  if (bus_ != nullptr && bus_->Read(function, reg, data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "ESP-AT register read failed (function: %u, register: %#X)\n",
      static_cast<unsigned>(function), static_cast<unsigned>(reg));
  return false;
}

bool EspAt::WriteRegister(
    uint32_t function, uint32_t reg, const void* data, size_t length) {
  if (bus_ != nullptr && bus_->Write(function, reg, data, length)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "ESP-AT register write failed (function: %u, register: %#X)\n",
      static_cast<unsigned>(function), static_cast<unsigned>(reg));
  return false;
}

bool EspAt::WriteRegister(uint32_t function, uint32_t reg, uint8_t value) {
  if (bus_ != nullptr && bus_->Write(function, reg, value, nullptr)) {
    return true;
  }
  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "ESP-AT register write failed (function: %u, register: %#X)\n",
      static_cast<unsigned>(function), static_cast<unsigned>(reg));
  return false;
}

}  // namespace cpp_bus_driver
