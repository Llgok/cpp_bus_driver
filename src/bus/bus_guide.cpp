/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2026-04-22 17:35:53
 * @License: GPL 3.0
 */
#include "bus_guide.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
i2c_cmd_handle_t BusI2cGuide::CmdLinkCreate() {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "CmdLinkCreate failed\n");
  return nullptr;
}

bool BusI2cGuide::StartTransmit(
    i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en) {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StartTransmit failed\n");
  return false;
}

bool BusI2cGuide::Write(
    i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en) {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
  return false;
}

bool BusI2cGuide::Write(i2c_cmd_handle_t cmd_handle, const uint8_t* data,
    size_t data_len, bool ack_en) {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
  return false;
}

bool BusI2cGuide::Read(i2c_cmd_handle_t cmd_handle, uint8_t* data,
    size_t data_len, i2c_ack_type_t ack) {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Read failed\n");
  return false;
}

bool BusI2cGuide::StopTransmit(i2c_cmd_handle_t cmd_handle) {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StopTransmit failed\n");
  return false;
}

bool BusI2cGuide::StartTransmit() {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StartTransmit failed\n");
  return false;
}

bool BusI2cGuide::StopTransmit() {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StopTransmit failed\n");
  return false;
}

#endif

bool BusI2cGuide::Deinit() {
  LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
  return false;
}

bool BusI2cGuide::Read(
    const uint8_t write_c8, uint8_t* read_data, size_t read_data_length) {
  if (!WriteRead(&write_c8, 1, read_data, read_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Read(
    const uint16_t write_c16, uint8_t* read_data, size_t read_data_length) {
  const uint8_t buffer[] = {
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };
  if (!WriteRead(buffer, 2, read_data, read_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Read(
    const uint32_t write_c32, uint8_t* read_data, size_t read_data_length) {
  const uint8_t buffer[] = {
      static_cast<uint8_t>(write_c32 >> 24),
      static_cast<uint8_t>(write_c32 >> 16),
      static_cast<uint8_t>(write_c32 >> 8),
      static_cast<uint8_t>(write_c32),
  };

  if (!WriteRead(buffer, 4, read_data, read_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Write(const uint8_t write_c8, const uint8_t write_d8) {
  const uint8_t buffer[] = {write_c8, write_d8};
  if (!Write(buffer, 2)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Write(
    const uint8_t write_c8, const uint16_t write_d16, Endian endian) {
  uint8_t buffer[3] = {write_c8};

  switch (endian) {
    case Endian::kBig:
      buffer[1] = write_d16 >> 8;
      buffer[2] = write_d16;
      break;
    case Endian::kLittle:
      buffer[1] = write_d16;
      buffer[2] = write_d16 >> 8;
      break;

    default:
      break;
  }

  if (!Write(buffer, 3)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Write(const uint16_t write_c16, const uint8_t write_d8) {
  const uint8_t buffer[] = {static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16), write_d8};
  if (!Write(buffer, 3)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Write(const uint8_t write_c8, const uint8_t* write_data,
    size_t write_data_length) {
  uint8_t buffer[1 + write_data_length] = {write_c8};

  std::memcpy(&buffer[1], write_data, write_data_length);

  if (!Write(buffer, 1 + write_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Write(const uint32_t write_c32, const uint8_t* write_data,
    size_t write_data_length) {
  uint8_t buffer[4 + write_data_length] = {
      static_cast<uint8_t>(write_c32 >> 24),
      static_cast<uint8_t>(write_c32 >> 16),
      static_cast<uint8_t>(write_c32 >> 8),
      static_cast<uint8_t>(write_c32),
  };

  std::memcpy(&buffer[4], write_data, write_data_length);

  if (!Write(buffer, 4 + write_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusI2cGuide::Scan7bitAddress(std::vector<uint8_t>* address) {
  std::vector<uint8_t> address_buffer;  // 地址存储器

  for (uint8_t i = 1; i < 128; i++) {
    if (Probe(i)) {
      address_buffer.push_back(i);
    }
  }

  if (address_buffer.empty()) {
    LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "address_buffer is empty\n");
    return false;
  }

  address->assign(address_buffer.begin(), address_buffer.end());
  return true;
}

bool BusSpiGuide::Read(const uint8_t write_c8, uint8_t* read_d8) {
  const uint8_t buffer_write[2] = {write_c8};
  uint8_t buffer_read[2] = {0};

  if (!WriteRead(buffer_write, buffer_read, 2)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  *read_d8 = buffer_read[1];

  return true;
}

bool BusSpiGuide::Read(
    const uint8_t write_c8, uint8_t* read_data, size_t read_data_length) {
  const uint8_t buffer_write[1 + read_data_length] = {write_c8};
  uint8_t buffer_read[1 + read_data_length] = {0};

  if (!WriteRead(buffer_write, buffer_read, 1 + read_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  std::memcpy(read_data, &buffer_read[1], read_data_length);

  return true;
}

bool BusSpiGuide::Write(const uint8_t write_c8) {
  if (!Write(&write_c8, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusSpiGuide::Write(const uint8_t write_c8, const uint8_t write_d8) {
  const uint8_t buffer[] = {write_c8, write_d8};

  if (!Write(buffer, 2)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusSpiGuide::Write(const uint8_t write_c8, const uint8_t* write_data,
    size_t write_data_length) {
  uint8_t buffer[1 + write_data_length] = {write_c8};

  std::memcpy(&buffer[1], write_data, write_data_length);

  if (!Write(buffer, 1 + write_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusSpiGuide::Read(const uint8_t write_c8, const uint16_t write_c16,
    uint8_t* read_data, size_t read_data_length) {
  const uint8_t buffer_write[3 + read_data_length] = {
      write_c8,
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };

  uint8_t buffer_read[3 + read_data_length] = {0};

  if (!WriteRead(buffer_write, buffer_read, 3 + read_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  std::memcpy(read_data, &buffer_read[3], read_data_length);

  return true;
}

bool BusSpiGuide::Read(const uint8_t write_c8_1, const uint8_t write_c8_2,
    uint8_t* read_data, size_t read_data_length) {
  const uint8_t buffer_write[2 + read_data_length] = {
      write_c8_1,
      write_c8_2,
  };

  uint8_t buffer_read[2 + read_data_length] = {0};

  if (!WriteRead(buffer_write, buffer_read, 2 + read_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  std::memcpy(read_data, &buffer_read[2], read_data_length);

  return true;
}

bool BusSpiGuide::Read(
    const uint8_t write_c8, const uint16_t write_c16, uint8_t* read_data) {
  const uint8_t buffer_write[4] = {
      write_c8,
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };

  uint8_t buffer_read[4] = {0};

  if (!WriteRead(buffer_write, buffer_read, 4)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteRead failed\n");
    return false;
  }

  std::memcpy(read_data, &buffer_read[3], 1);

  return true;
}

bool BusSpiGuide::Write(const uint8_t write_c8, const uint16_t write_c16,
    const uint8_t* write_data, size_t write_data_length) {
  uint8_t buffer[3 + write_data_length] = {
      write_c8,
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };

  std::memcpy(&buffer[3], write_data, write_data_length);

  if (!Write(buffer, 3 + write_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusSpiGuide::Write(const uint8_t write_c8_1, const uint8_t write_c8_2,
    const uint8_t* write_data, size_t write_data_length) {
  uint8_t buffer[2 + write_data_length] = {
      write_c8_1,
      write_c8_2,
  };

  std::memcpy(&buffer[2], write_data, write_data_length);

  if (!Write(buffer, 2 + write_data_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusSpiGuide::Write(const uint8_t write_c8, const uint16_t write_c16,
    const uint8_t write_data) {
  uint8_t buffer[4] = {
      write_c8,
      static_cast<uint8_t>(write_c16 >> 8),
      static_cast<uint8_t>(write_c16),
  };

  std::memcpy(&buffer[3], &write_data, 1);

  if (!Write(buffer, 4)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusMipiGuide::Write(const uint8_t write_c8) {
  if (!Write(static_cast<uint8_t>(write_c8), nullptr, 0)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool BusMipiGuide::Write(const uint8_t write_c8, const uint8_t write_d8) {
  uint8_t buffer = write_d8;

  if (!Write(static_cast<uint8_t>(write_c8), &buffer, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
