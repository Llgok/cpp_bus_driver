/*
 * @Description: 各类总线公共基类的辅助实现
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2026-09-04 11:43:29
 * @License: GPL 3.0
 */
#include "bus/bus_base.h"

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace cpp_bus_driver {
namespace {
// 每次事务独立持有缓冲区，不引入共享可变状态；小事务不申请堆内存。
class TransferBuffer {
 public:
  /**
   * @brief 校验头部与正文长度，并准备清零后的事务缓冲区
   * @param prefix_size 命令头长度
   * @param payload_size 正文长度
   * @return 长度加法溢出或堆分配失败时返回 false
   */
  bool Init(size_t prefix_size, size_t payload_size) {
    if (payload_size > std::numeric_limits<size_t>::max() - prefix_size) {
      return false;
    }
    size_ = prefix_size + payload_size;
    if (size_ > local_.size()) {
      heap_.reset(new (std::nothrow) uint8_t[size_]());
      return heap_ != nullptr;
    }
    return true;
  }

  uint8_t* data() { return heap_ != nullptr ? heap_.get() : local_.data(); }
  size_t size() const { return size_; }
  uint8_t& operator[](size_t index) { return data()[index]; }

 private:
  // 大数据仍放堆上，避免将最大传输容量放入任务栈。
  std::array<uint8_t, 128> local_{};
  std::unique_ptr<uint8_t[]> heap_;
  size_t size_ = 0;
};
}  // namespace

bool I2cBusBase::Deinit(bool delete_bus) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
  return false;
}

bool I2cBusBase::Read(
    const uint8_t write_c8, uint8_t* read_data, size_t read_data_length) {
  if (!WriteRead(&write_c8, 1, read_data, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Read(
    const uint16_t write_c16, uint8_t* read_data, size_t read_data_length) {
  const uint8_t buffer[] = {
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };
  if (!WriteRead(buffer, 2, read_data, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Read(
    const uint32_t write_c32, uint8_t* read_data, size_t read_data_length) {
  const uint8_t buffer[] = {
      static_cast<uint8_t>(write_c32 >> 24),
      static_cast<uint8_t>(write_c32 >> 16),
      static_cast<uint8_t>(write_c32 >> 8),
      static_cast<uint8_t>(write_c32),
  };

  if (!WriteRead(buffer, 4, read_data, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Write(const uint8_t write_c8, const uint8_t write_d8) {
  const uint8_t buffer[] = {write_c8, write_d8};
  if (!Write(buffer, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Write(
    const uint8_t write_c8, const uint16_t write_d16, ByteOrder byte_order) {
  uint8_t buffer[3] = {write_c8};

  switch (byte_order) {
    case ByteOrder::kBig:
      buffer[1] = write_d16 >> 8;
      buffer[2] = write_d16;
      break;
    case ByteOrder::kLittle:
      buffer[1] = write_d16;
      buffer[2] = write_d16 >> 8;
      break;

    default:
      break;
  }

  if (!Write(buffer, 3)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Write(const uint16_t write_c16, const uint8_t write_d8) {
  const uint8_t buffer[] = {static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16), write_d8};
  if (!Write(buffer, 3)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Write(const uint8_t write_c8, const uint8_t* write_data,
    size_t write_data_length) {
  if (write_data == nullptr && write_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer;
  if (!buffer.Init(1, write_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer[0] = write_c8;
  if (write_data_length != 0) {
    std::memcpy(&buffer[1], write_data, write_data_length);
  }

  if (!Write(buffer.data(), buffer.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Write(const uint32_t write_c32, const uint8_t* write_data,
    size_t write_data_length) {
  if (write_data == nullptr && write_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer;
  if (!buffer.Init(4, write_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer[0] = static_cast<uint8_t>(write_c32 >> 24);
  buffer[1] = static_cast<uint8_t>(write_c32 >> 16);
  buffer[2] = static_cast<uint8_t>(write_c32 >> 8);
  buffer[3] = static_cast<uint8_t>(write_c32);
  if (write_data_length != 0) {
    std::memcpy(&buffer[4], write_data, write_data_length);
  }

  if (!Write(buffer.data(), buffer.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool I2cBusBase::Scan7BitAddress(std::vector<uint8_t>* address) {
  if (address == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  std::vector<uint8_t> address_buffer;  // 地址存储器

  for (uint8_t i = 1; i < 128; i++) {
    if (Probe(i)) {
      address_buffer.push_back(i);
    }
  }

  if (address_buffer.empty()) {
    LogMessage(
        LogLevel::kWarning, __FILE__, __LINE__, "address_buffer is empty\n");
    return false;
  }

  address->assign(address_buffer.begin(), address_buffer.end());
  return true;
}

bool SpiBusBase::Read(const uint8_t write_c8, uint8_t* read_d8) {
  if (read_d8 == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  const uint8_t buffer_write[2] = {write_c8};
  uint8_t buffer_read[2] = {0};

  if (!WriteRead(buffer_write, buffer_read, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  *read_d8 = buffer_read[1];

  return true;
}

bool SpiBusBase::Read(
    const uint8_t write_c8, uint8_t* read_data, size_t read_data_length) {
  if (read_data == nullptr && read_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer_write;
  if (!buffer_write.Init(1, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  TransferBuffer buffer_read;
  if (!buffer_read.Init(1, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer_write[0] = write_c8;

  if (!WriteRead(
          buffer_write.data(), buffer_read.data(), buffer_write.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  if (read_data_length != 0) {
    std::memcpy(read_data, &buffer_read[1], read_data_length);
  }

  return true;
}

bool SpiBusBase::Write(const uint8_t write_c8) {
  if (!Write(&write_c8, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool SpiBusBase::Write(const uint8_t write_c8, const uint8_t write_d8) {
  const uint8_t buffer[] = {write_c8, write_d8};

  if (!Write(buffer, 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool SpiBusBase::Write(const uint8_t write_c8, const uint8_t* write_data,
    size_t write_data_length) {
  if (write_data == nullptr && write_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer;
  if (!buffer.Init(1, write_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer[0] = write_c8;
  if (write_data_length != 0) {
    std::memcpy(&buffer[1], write_data, write_data_length);
  }

  if (!Write(buffer.data(), buffer.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool SpiBusBase::Read(const uint8_t write_c8, const uint16_t write_c16,
    uint8_t* read_data, size_t read_data_length) {
  if (read_data == nullptr && read_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer_write;
  if (!buffer_write.Init(3, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  TransferBuffer buffer_read;
  if (!buffer_read.Init(3, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer_write[0] = write_c8;
  buffer_write[1] = static_cast<uint8_t>(write_c16 >> 8);
  buffer_write[2] = static_cast<uint8_t>(write_c16);

  if (!WriteRead(
          buffer_write.data(), buffer_read.data(), buffer_write.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  if (read_data_length != 0) {
    std::memcpy(read_data, &buffer_read[3], read_data_length);
  }

  return true;
}

bool SpiBusBase::Read(const uint8_t write_c8_1, const uint8_t write_c8_2,
    uint8_t* read_data, size_t read_data_length) {
  if (read_data == nullptr && read_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer_write;
  if (!buffer_write.Init(2, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  TransferBuffer buffer_read;
  if (!buffer_read.Init(2, read_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer_write[0] = write_c8_1;
  buffer_write[1] = write_c8_2;

  if (!WriteRead(
          buffer_write.data(), buffer_read.data(), buffer_write.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  if (read_data_length != 0) {
    std::memcpy(read_data, &buffer_read[2], read_data_length);
  }

  return true;
}

bool SpiBusBase::Read(
    const uint8_t write_c8, const uint16_t write_c16, uint8_t* read_data) {
  if (read_data == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  const uint8_t buffer_write[4] = {
      write_c8,
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };

  uint8_t buffer_read[4] = {0};

  if (!WriteRead(buffer_write, buffer_read, 4)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  std::memcpy(read_data, &buffer_read[3], 1);

  return true;
}

bool SpiBusBase::Write(const uint8_t write_c8, const uint16_t write_c16,
    const uint8_t* write_data, size_t write_data_length) {
  if (write_data == nullptr && write_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer;
  if (!buffer.Init(3, write_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer[0] = write_c8;
  buffer[1] = static_cast<uint8_t>(write_c16 >> 8);
  buffer[2] = static_cast<uint8_t>(write_c16);
  if (write_data_length != 0) {
    std::memcpy(&buffer[3], write_data, write_data_length);
  }

  if (!Write(buffer.data(), buffer.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool SpiBusBase::Write(const uint8_t write_c8_1, const uint8_t write_c8_2,
    const uint8_t* write_data, size_t write_data_length) {
  if (write_data == nullptr && write_data_length != 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  TransferBuffer buffer;
  if (!buffer.Init(2, write_data_length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Transfer buffer length overflow or allocation failed\n");
    return false;
  }
  buffer[0] = write_c8_1;
  buffer[1] = write_c8_2;
  if (write_data_length != 0) {
    std::memcpy(&buffer[2], write_data, write_data_length);
  }

  if (!Write(buffer.data(), buffer.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool SpiBusBase::Write(const uint8_t write_c8, const uint16_t write_c16,
    const uint8_t write_data) {
  uint8_t buffer[4] = {
      write_c8,
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };

  std::memcpy(&buffer[3], &write_data, 1);

  if (!Write(buffer, 4)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool MipiBusBase::Write(const uint8_t write_c8) {
  if (!Write(static_cast<uint8_t>(write_c8), nullptr, 0)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool MipiBusBase::Write(const uint8_t write_c8, const uint8_t write_d8) {
  uint8_t buffer = write_d8;

  if (!Write(static_cast<uint8_t>(write_c8), &buffer, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
