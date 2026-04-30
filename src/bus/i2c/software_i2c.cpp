/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2026-04-29 14:22:26
 * @License: GPL 3.0
 */
#include "software_i2c.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
bool SoftwareI2c::Init(uint32_t freq_hz, uint16_t address) {
  if (freq_hz == CPP_BUS_DRIVER_DEFAULT_VALUE) {
    freq_hz = CPP_BUS_DRIVER_DEFAULT_I2C_FREQ_HZ;
  }

  uint32_t buffer_transmit_delay_us = static_cast<uint32_t>(
      (1000000.0 / static_cast<double>(freq_hz)) / 2.0 + 0.5);

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config address: %#X\n", address);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config sda_: %d\n", sda_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config scl_: %d\n", scl_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config freq_hz: %d hz\n", freq_hz);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "SoftwareI2c config transmit_delay_us_: %d us\n",
      buffer_transmit_delay_us);

  if (!SetGpioMode(sda_, GpioMode::kInputOutputOd, GpioStatus::kPullup)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioMode failed\n");
    return false;
  }

  if (!SetGpioMode(scl_, GpioMode::kOutputOd, GpioStatus::kPullup)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioMode failed\n");
    return false;
  }

  freq_hz_ = freq_hz;
  transmit_delay_us_ = buffer_transmit_delay_us;
  address_ = address;

  return true;
}

bool SoftwareI2c::Deinit(bool delete_bus) {
  bool result = true;

  result &= SetGpioMode(sda_, GpioMode::kDisable, GpioStatus::kDisable);
  result &= SetGpioMode(scl_, GpioMode::kDisable, GpioStatus::kDisable);

  return result;
}

bool SoftwareI2c::StartTransmit() {
  if (!GpioWrite(scl_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  if (!GpioWrite(sda_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);
  if (!GpioWrite(sda_, 0)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);
  if (!GpioWrite(scl_, 0)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);

  return true;
}

bool SoftwareI2c::Read(uint8_t* data, size_t length) {
  if (!StartTransmit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StartTransmit failed\n");
    return false;
  }

  // 读操作发送地址最后一位为1
  if (!WriteByte((address_ << 1) | 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteByte failed\n");
    return false;
  }
  if (!WaitAck()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
    return false;
  }

  uint8_t* buffer_ptr = data;
  for (size_t i = 0; i < (length - 1); i++) {
    if (!ReadByte(buffer_ptr++)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "ReadByte failed\n");
      return false;
    }

    if (!WriteAck(AckBit::kAck)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
      return false;
    }
  }

  // 读取最后一位数据
  if (!ReadByte(buffer_ptr)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "ReadByte failed\n");
    return false;
  }

  if (!WriteAck(AckBit::kNack)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
    return false;
  }

  if (!StopTransmit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StopTransmit failed\n");
    return false;
  }

  return true;
}

bool SoftwareI2c::Write(const uint8_t* data, size_t length) {
  if (!StartTransmit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StartTransmit failed\n");
    return false;
  }

  // 写操作发送地址最后一位为0
  if (!WriteByte(address_ << 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteByte failed\n");
    return false;
  }
  if (!WaitAck()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
    return false;
  }

  for (size_t i = 0; i < length; i++) {
    if (!WriteByte(data[i])) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteByte failed\n");
      return false;
    }
    if (!WaitAck()) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
      return false;
    }
  }

  if (!StopTransmit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StopTransmit failed\n");
    return false;
  }

  return true;
}

bool SoftwareI2c::WriteRead(const uint8_t* write_data, size_t write_length,
    uint8_t* read_data, size_t read_length) {
  if (!StartTransmit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StartTransmit failed\n");
    return false;
  }

  // 写操作发送地址最后一位为0
  if (!WriteByte(address_ << 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteByte failed\n");
    return false;
  }
  if (!WaitAck()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
    return false;
  }

  for (size_t i = 0; i < write_length; i++) {
    if (!WriteByte(write_data[i])) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteByte failed\n");
      return false;
    }
    if (!WaitAck()) {
      // 如果不为最后一位数据，则报错
      if (i != (write_length - 1)) {
        LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
        return false;
      }
    }
  }

  if (!Read(read_data, read_length)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  return true;
}

bool SoftwareI2c::StopTransmit() {
  if (!GpioWrite(sda_, 0)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  if (!GpioWrite(scl_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);
  if (!GpioWrite(sda_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }

  return true;
}

bool SoftwareI2c::Probe(const uint16_t address) {
  if (!StartTransmit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StartTransmit failed\n");
    return false;
  }

  // 写操作发送地址最后一位为0
  if (!WriteByte(address << 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WriteByte failed\n");
    return false;
  }
  if (!WaitAck()) {
    // LogMessage(LogLevel::kBus, __FILE__, __LINE__, "WaitAck failed\n");
    return false;
  }

  if (!StopTransmit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "StopTransmit failed\n");
    return false;
  }

  return true;
}

bool SoftwareI2c::WriteByte(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    if (!GpioWrite(sda_, data & 0x80)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
      return false;
    }

    DelayUs(transmit_delay_us_);
    if (!GpioWrite(scl_, 1)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
      return false;
    }
    DelayUs(transmit_delay_us_);
    if (!GpioWrite(scl_, 0)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
      return false;
    }

    data <<= 1;
  }

  // 释放sda
  if (!GpioWrite(sda_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }

  return true;
}

bool SoftwareI2c::ReadByte(uint8_t* data) {
  if (!GpioWrite(sda_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }

  uint8_t buffer_data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    buffer_data <<= 1;

    if (!GpioWrite(scl_, 1)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
      return false;
    }
    DelayUs(transmit_delay_us_);

    if (GpioRead(sda_) == 1) {
      buffer_data |= 0x01;
    }

    if (!GpioWrite(scl_, 0)) {
      LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
      return false;
    }
    DelayUs(transmit_delay_us_);
  }

  *data = buffer_data;

  return true;
}

bool SoftwareI2c::WaitAck() {
  DelayUs(transmit_delay_us_);
  if (!GpioWrite(scl_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);

  // sda应该保持低电平作为应答(ack)
  bool buffer_ack = !GpioRead(sda_);

  if (!GpioWrite(scl_, 0)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);

  return buffer_ack;
}

bool SoftwareI2c::WriteAck(AckBit ack) {
  if (!GpioWrite(sda_, static_cast<bool>(ack))) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }

  if (!GpioWrite(scl_, 1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);
  if (!GpioWrite(scl_, 0)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "GpioWrite failed\n");
    return false;
  }
  DelayUs(transmit_delay_us_);

  return true;
}

#endif
}  // namespace cpp_bus_driver
