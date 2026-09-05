/*
 * @Description: cpp_bus_driver 2.x 旧接口兼容声明
 * @Author: LILYGO_L
 * @Date: 2026-09-04 12:06:07
 * @LastEditTime: 2026-09-04 12:08:50
 * @License: GPL 3.0
 */
#pragma once

// 此文件仅用于 2.x 源码兼容，将在 3.0.0 整体删除。
#if !defined(CPP_BUS_DRIVER_PLATFORM)
#include "cpp_bus_driver_config.h"
#endif

namespace cpp_bus_driver {
class I2cBusBase;
class I2sBusBase;
class SpiBusBase;
class QspiBusBase;
class UartBusBase;
class SdioBusBase;
class MipiBusBase;

class I2cChipBase;
class I2sChipBase;
class SpiChipBase;
class QspiChipBase;
class UartChipBase;
class SdioChipBase;
class MipiChipBase;

class HardwareI2c;
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
class LegacyHardwareI2c;
#endif

// 兼容 2.x Guide 类型名；请改用对应 Base 类型，以下别名将在 3.0.0 删除。
using BusI2cGuide = I2cBusBase;
using BusI2sGuide = I2sBusBase;
using BusSpiGuide = SpiBusBase;
using BusQspiGuide = QspiBusBase;
using BusUartGuide = UartBusBase;
using BusSdioGuide = SdioBusBase;
using BusMipiGuide = MipiBusBase;

using ChipI2cGuide = I2cChipBase;
using ChipI2sGuide = I2sChipBase;
using ChipSpiGuide = SpiChipBase;
using ChipQspiGuide = QspiChipBase;
using ChipUartGuide = UartChipBase;
using ChipSdioGuide = SdioChipBase;
using ChipMipiGuide = MipiChipBase;

// 兼容 2.x 编号类型名；请改用 HardwareI2c 或 LegacyHardwareI2c。
// 以下别名将在 3.0.0 删除。
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
using HardwareI2c1 = HardwareI2c;
using HardwareI2c2 = LegacyHardwareI2c;
#elif CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_NRF52
using HardwareI2c2 = HardwareI2c;
#endif
}  // namespace cpp_bus_driver
