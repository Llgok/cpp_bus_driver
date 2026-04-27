/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:26:23
 * @LastEditTime: 2026-04-22 17:36:05
 * @License: GPL 3.0
 */
#include "hardware_uart.h"

namespace cpp_bus_driver {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
bool HardwareUart::Init(int32_t baud_rate) {
  if (init_flag_) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "HardwareUart has been initialized\n");
    return true;
  }

  if (baud_rate == CPP_BUS_DRIVER_DEFAULT_VALUE) {
    baud_rate = CPP_BUS_DRIVER_DEFAULT_UART_BAUD_RATE;
  }

  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareUart port_: %d\n", port_);
  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareUart tx_: %d\n", tx_);
  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareUart rx_: %d\n", rx_);
  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareUart rts_: %d\n", rts_);
  LogMessage(
      LogLevel::kInfo, __FILE__, __LINE__, "HardwareUart cts_: %d\n", cts_);
  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "HardwareUart baud_rate: %d bps\n", baud_rate);

  const uart_config_t uart_config = {
      .baud_rate = baud_rate,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = [](int32_t rts, int32_t cts) -> uart_hw_flowcontrol_t {
        if ((rts != CPP_BUS_DRIVER_DEFAULT_VALUE) &&
            (cts != CPP_BUS_DRIVER_DEFAULT_VALUE)) {
          return uart_hw_flowcontrol_t::UART_HW_FLOWCTRL_CTS_RTS;
        } else if ((rts != CPP_BUS_DRIVER_DEFAULT_VALUE) &&
                   (cts == CPP_BUS_DRIVER_DEFAULT_VALUE)) {
          return uart_hw_flowcontrol_t::UART_HW_FLOWCTRL_RTS;
        } else if ((rts == CPP_BUS_DRIVER_DEFAULT_VALUE) &&
                   (cts != CPP_BUS_DRIVER_DEFAULT_VALUE)) {
          return uart_hw_flowcontrol_t::UART_HW_FLOWCTRL_CTS;
        }

        return uart_hw_flowcontrol_t::UART_HW_FLOWCTRL_DISABLE;
      }(rts_, cts_),
      .rx_flow_ctrl_thresh = 122,
      .source_clk = UART_SCLK_DEFAULT,
      .flags =
          {
              .allow_pd = 0,
              .backup_before_sleep = 0,
          },
  };

  esp_err_t result = uart_driver_install(
      static_cast<uart_port_t>(port_), kUartRxMaxSize, 0, 0, NULL, 0);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_driver_install failed (error code: %#X)\n", result);
    return false;
  }

  result = uart_param_config(static_cast<uart_port_t>(port_), &uart_config);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_param_config failed (error code: %#X)\n", result);
    return false;
  }

  result = uart_set_pin(static_cast<uart_port_t>(port_), tx_, rx_, rts_, cts_);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_set_pin failed (error code: %#X)\n", result);
    return false;
  }

  baud_rate_ = baud_rate;
  init_flag_ = true;

  return true;
}

int32_t HardwareUart::Read(void* data, uint32_t length) {
  int32_t buffer_size = uart_read_bytes(static_cast<uart_port_t>(port_), data,
      length, pdMS_TO_TICKS(CPP_BUS_DRIVER_DEFAULT_UART_WAIT_TIMEOUT_MS));
  if (buffer_size == (-1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_read_bytes failed (uart_read_bytes == (-1))\n");
    return false;
  }

  return buffer_size;
}

int32_t HardwareUart::Write(const void* data, size_t length) {
  int32_t buffer_size =
      uart_write_bytes(static_cast<uart_port_t>(port_), data, length);
  if (buffer_size == (-1)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_write_bytes failed (uart_write_bytes == (-1))\n");
    return false;
  }

  return buffer_size;
}

size_t HardwareUart::GetRxBufferLength() {
  size_t buffer = 0;

  esp_err_t result =
      uart_get_buffered_data_len(static_cast<uart_port_t>(port_), &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_get_buffered_data_len failed (error code: %#X)\n", result);
    return false;
  }

  return buffer;
}

bool HardwareUart::ClearRxBufferData() {
  esp_err_t result = uart_flush_input(static_cast<uart_port_t>(port_));
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_flush_input failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

bool HardwareUart::SetBaudRate(uint32_t baud_rate) {
  esp_err_t result =
      uart_set_baudrate(static_cast<uart_port_t>(port_), baud_rate);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_set_baudrate failed (error code: %#X)\n", result);
    return false;
  }

  return true;
}

uint32_t HardwareUart::GetBaudRate() {
  uint32_t buffer = 0;

  esp_err_t result =
      uart_get_baudrate(static_cast<uart_port_t>(port_), &buffer);
  if (result != ESP_OK) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "uart_get_baudrate failed (error code: %#X)\n", result);
    return -1;
  }

  return buffer;
}
#endif
}  // namespace cpp_bus_driver