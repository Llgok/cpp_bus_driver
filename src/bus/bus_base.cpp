/*
 * @Description: 各类总线公共基类的辅助实现
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2026-09-04 11:43:29
 * @License: GPL 3.0
 */
#include "bus/bus_base.h"

namespace cpp_bus_driver {

bool I2cBusBase::Deinit(bool delete_bus) {
  LogMessage(LogLevel::kError, __FILE__, __LINE__, "Deinit failed\n");
  return false;
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

}  // namespace cpp_bus_driver
