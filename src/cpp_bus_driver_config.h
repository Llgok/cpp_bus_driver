/*
 * @Description: cpp_bus_driver 编译配置、平台选择与外设能力检测
 * @Author: LILYGO_L
 * @Date: 2024-12-18 14:54:01
 * @LastEditTime: 2026-09-05 14:56:26
 * @License: GPL 3.0
 */
#pragma once

// 加载 ESP32 构建配置和外设能力，避免依赖其他头文件的包含顺序。
#if defined(ESP_PLATFORM) || (defined(ARDUINO) && defined(ARDUINO_ARCH_ESP32))
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#endif

#define CPP_BUS_DRIVER_PLATFORM_ESP_IDF 1
#define CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32 2
#define CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52 3

// 根据构建环境自动选择平台。
#if defined(ARDUINO) && defined(ARDUINO_ARCH_ESP32)
#define CPP_BUS_DRIVER_PLATFORM CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32

// Arduino-ESP32 共用 ESP-IDF 后端，底层版本要求与库一致，最低为 5.5.3。
#include "esp_idf_version.h"
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 3)
#error "Arduino-ESP32 requires an ESP-IDF 5.5.3 or newer core."
#endif

#elif defined(ARDUINO) && defined(NRF52840_XXAA)
#define CPP_BUS_DRIVER_PLATFORM CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52

#elif defined(ESP_PLATFORM)
#define CPP_BUS_DRIVER_PLATFORM CPP_BUS_DRIVER_PLATFORM_ESP_IDF

#else
#error "Unsupported build platform."
#endif

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#if !defined(__cpp_lib_make_unique)
#define CPP_BUS_DRIVER_CUSTOM_TEMPLATE_MAKE_UNIQUE
#endif
#endif

#if defined(CPP_BUS_DRIVER_CUSTOM_TEMPLATE_MAKE_UNIQUE)
// 部分 Arduino nRF52 工具链只提供 C++11 标准库，没有
// std::make_unique。这里补充与 C++14 一致的兼容实现，使库内智能指针
// 写法可在这些旧工具链中编译；工具链原生支持后不会启用此兼容代码。
namespace std {
// 通用模板（非数组类型）
template <typename T, typename... Args,
    typename = typename std::enable_if<!std::is_array<T>::value>::type>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 特化模板（动态数组类型）
template <typename T,
    typename = typename std::enable_if<std::is_array<T>::value>::type>
std::unique_ptr<T> make_unique(std::size_t size) {
  using U = typename std::remove_extent<T>::type;  // 获取数组元素类型
  return std::unique_ptr<T>(new U[size]());        // 值初始化
}
}  // namespace std
#endif
