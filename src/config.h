/*
 * @Description: cpp_bus_driver 编译配置、平台选择与公共依赖声明
 * @Author: LILYGO_L
 * @Date: 2024-12-18 14:54:01
 * @LastEditTime: 2026-07-14 01:41:50
 * @License: GPL 3.0
 */
#pragma once

#include <stdarg.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#if defined(CONFIG_IDF_INIT_VERSION)
#define CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF

#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define CPP_BUS_DRIVER_CHIP_ESP32P4
#endif

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "sdmmc_cmd.h"
#if defined(CPP_BUS_DRIVER_CHIP_ESP32P4)
#include "esp_lcd_mipi_dsi.h"
#endif
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#elif defined(ARDUINO)
#include "Arduino.h"
#include "SPI.h"
#include "Wire.h"

#if defined(NRF52840_XXAA)

#define CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
#define CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_CPP11_SUPPORT
#define CPP_BUS_DRIVER_CUSTOM_TEMPLATE_MAKE_UNIQUE

#include "nrfx_i2s.h"

#endif

#else
#error "Missing required macro definition."
#endif

#include "tool.h"

#if defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_DEBUG)
#define CPP_BUS_DRIVER_LOG_LEVEL_DEBUG
#define CPP_BUS_DRIVER_LOG_LEVEL_INFO
#define CPP_BUS_DRIVER_LOG_LEVEL_WARNING
#define CPP_BUS_DRIVER_LOG_LEVEL_ERROR
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_INFO)
#define CPP_BUS_DRIVER_LOG_LEVEL_INFO
#define CPP_BUS_DRIVER_LOG_LEVEL_WARNING
#define CPP_BUS_DRIVER_LOG_LEVEL_ERROR
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_WARNING)
#define CPP_BUS_DRIVER_LOG_LEVEL_WARNING
#define CPP_BUS_DRIVER_LOG_LEVEL_ERROR
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_ERROR)
#define CPP_BUS_DRIVER_LOG_LEVEL_ERROR
#elif defined(CONFIG_CPP_BUS_DRIVER_LOG_LEVEL_NONE)
#else
#define CPP_BUS_DRIVER_LOG_LEVEL_INFO
#define CPP_BUS_DRIVER_LOG_LEVEL_WARNING
#define CPP_BUS_DRIVER_LOG_LEVEL_ERROR
#endif

namespace cpp_bus_driver {

constexpr int kDefaultValue = -1;

constexpr int kDefaultI2cFreqHz = 100000;
constexpr int kDefaultI2cWaitTimeoutMs = 1000;

constexpr int kDefaultSpiFreqHz = 10000000;

constexpr int kDefaultQspiFreqHz = 10000000;
constexpr int kDefaultQspiWaitTimeoutMs = 1000;

constexpr int kDefaultUartBaudRate = 115200;
constexpr int kDefaultUartWaitTimeoutMs = 1000;

constexpr int kDefaultI2sWaitTimeoutMs = 1000;

constexpr float kDefaultMipiFreqMhz = 60.0F;
constexpr float kDefaultMipiLaneBitRateMbps = 1000.0F;

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
constexpr int kDefaultSdioFreqHz = SDMMC_FREQ_DEFAULT;
#endif

}  // namespace cpp_bus_driver

#if defined(CPP_BUS_DRIVER_CUSTOM_TEMPLATE_MAKE_UNIQUE)
namespace std {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_CPP11_SUPPORT)
// C++ 11
//  通用模板（非数组类型）
template <typename T, typename... Args,
    typename = typename std::enable_if<!std::is_array<T>::value>::type>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 特化模板（动态数组类型）
template <typename T,
    typename = typename std::enable_if<std::is_array<T>::value>::type>
std::unique_ptr<T> make_unique(size_t size) {
  using U = typename std::remove_extent<T>::type;  // 获取数组元素类型
  return std::unique_ptr<T>(new U[size]());        // 值初始化
}
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_CPP14_SUPPORT)
// C++ 14
// 通用模板（非数组类型）
template <typename T, typename... Args,
    typename = std::enable_if_t<!std::is_array<T>::value> >
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 特化模板（动态数组类型）
template <typename T, typename = std::enable_if_t<std::is_array<T>::value> >
std::unique_ptr<T> make_unique(size_t size) {
  using U = typename std::remove_extent<T>::type;  // 获取数组元素类型
  return std::unique_ptr<T>(new U[size]());        // 值初始化
}

#else
#error "Missing required macro definition."
#endif
}  // namespace std
#endif
