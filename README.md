<h1 align="center">cpp_bus_driver</h1>

## **English | [Chinese](./README_CN.md)**

[![Release](https://img.shields.io/github/v/release/Llgok/cpp_bus_driver?style=flat-square)](https://github.com/Llgok/cpp_bus_driver/releases)
[![License](https://img.shields.io/github/license/Llgok/cpp_bus_driver?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.3%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)
[![C++](https://img.shields.io/badge/C%2B%2B-11%2B-00599c?style=flat-square)](https://isocpp.org/)

**cpp_bus_driver** is a microcontroller peripheral driver library written in C++11 and later. It organizes common buses, chip drivers, and utilities such as GPIO, PWM, and interrupts into a unified C++ interface, so different peripherals can be initialized, read, written, released, and reused in a similar way.

Starting from **v2**, the project enters a new major version. API naming, directory structure, lifecycle management, and log configuration have been significantly adjusted, making the library more suitable for long-term maintenance and complex hardware project integration.

## Table of Contents

- [Features](#features)
- [Supported Frameworks](#supported-frameworks)
- [Quick Start](#quick-start)
- [v2 Migration Guide](#v2-migration-guide)
- [Development Plan](#development-plan)

## Features

### Unified Bus and Chip Abstractions

The library supports common microcontroller peripheral buses and provides a similar C++ usage model at both the bus layer and the chip layer. You can create a bus object first, then pass that bus into a chip object, keeping initialization, read/write, configuration, and release flows clear and consistent.

The supported bus drivers and chip drivers will continue to evolve with future versions. For the latest list, see the unified entry file [`cpp_bus_driver.h`](./src/cpp_bus_driver.h).

### Engineering-Oriented API Design

- Uses Google C++ style naming for more consistent APIs.
- Supports configurable log levels, making bus and chip debugging easier.
- Supports shared bus scenarios, such as mounting multiple I2C devices on the same master bus.

## Supported Frameworks

| Framework | Status | Description |
| --- | --- | --- |
| ESP-IDF | Recommended | Starting from v2.0.0, the minimum supported ESP-IDF version is v5.5.3 |
| Arduino NRF | Supported | Suitable for some NRF52840 Arduino scenarios |
| Arduino ESP32 | Shared backend | Uses the same backend as ESP-IDF; the core must meet this library's minimum ESP-IDF version requirement of v5.5.3 |

> [!NOTE]
> Available features depend on the target chip and SDK configuration. Arduino-ESP32 builds require ESP-IDF 5.5.3 or newer; older versions are rejected at compile time. This is the ESP-IDF version, not the Arduino-ESP32 package version. Meeting the minimum requirement does not mean every newer version has been validated.

## Quick Start

### Integration

#### Use as an ESP-IDF Component

It is recommended to place this repository in your project's `components` directory:

```bash
your_project/
├── components/
│   └── cpp_bus_driver/
├── main/
└── CMakeLists.txt
```

Clone commands:

```bash
git clone https://github.com/Llgok/cpp_bus_driver.git
```

Then include the unified entry header in your code:

```cpp
#include "cpp_bus_driver.h"
```

#### Use with Arduino IDE (nRF52840)

After the library is indexed by Arduino Library Manager, install
`cpp_bus_driver` from **Library Manager**. Before it is indexed, clone this
repository into your sketchbook's `libraries/cpp_bus_driver` directory.

Install the **Adafruit nRF52** board package from Boards Manager and select an
nRF52840 board. Adafruit nRF52 1.6.1 does not link the full C++ standard library
by default. Create `platform.local.txt` in the board package directory (on
Windows: `C:\Users\<username>\AppData\Local\Arduino15\packages\adafruit\hardware\nrf52\1.6.1\platform.local.txt`)
with the following content, then restart Arduino IDE:

```properties
compiler.libraries.ldflags=-lstdc++
```

This setting is required because `cpp_bus_driver` uses C++ standard library
types such as `std::string` and `std::vector`. Recreate the file if an Adafruit
nRF52 board package upgrade or reinstall removes it. Then include the unified
entry header:

```cpp
#include <cpp_bus_driver.h>
```

> [!IMPORTANT]
> Arduino nRF52 builds must define `NRF52840_XXAA` (nRF52840).

#### Use with Arduino ESP32

Install an Arduino-ESP32 core that meets this library's ESP-IDF version
requirement: **ESP-IDF 5.5.3 or newer**. Include `<cpp_bus_driver.h>` to use the
shared ESP-IDF backend. Features remain subject to the target chip's
capabilities and the components provided by the core.

Do not let Arduino objects such as `Wire`, `SPI`, or `HardwareSerial` and
this library manage the same peripheral simultaneously.

#### Use as a Git Submodule

```bash
git submodule add https://github.com/Llgok/cpp_bus_driver.git
git submodule update --init --recursive
```

### Bus Drivers

Bus drivers interact with MCU peripherals. See the
[entry header](src/cpp_bus_driver.h) for drivers available on each platform.

Use `HardwareI2c` for hardware I2C. `LegacyHardwareI2c` is available only on
ESP-IDF and Arduino ESP32 and uses the legacy ESP-IDF I2C driver; Arduino
nRF52 uses `HardwareI2c`. Bus availability depends on the target platform.

See the [bus interfaces](src/bus/bus_base.h) and
[chip interfaces](src/chip/chip_base.h) for base class declarations.

Typical lifecycle:

```cpp
bus->Init();
bus->Write(data, length);
bus->Read(data, length);
bus->Deinit();
```

> [!NOTE]
> `Deinit()` support depends on the specific bus class. For display-oriented QSPI scenarios, release flow is usually managed by the upper-layer chip driver. If multiple chips share the same I2C bus, choose whether to delete the underlying bus according to resource ownership. For example, `Deinit(false)` releases only the current device, while `Deinit(true)` releases the bus as well.

### Chip Drivers

Chip drivers are built on top of bus drivers. Usually, you create a bus first and then pass that bus into the chip.

```cpp
auto i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c>(
    sda, scl, I2C_NUM_0);

auto chip = std::make_unique<cpp_bus_driver::Xl95x5>(i2c_bus);

chip->Init();
chip->Deinit();
```

The chip-layer `Deinit()` releases the bus device used by the chip first, and switches related GPIO pins to the disabled state. This is suitable for low-power and reinitialization scenarios.

### Log Configuration

`cpp_bus_driver` provides a library-wide minimum log level for controlling debug messages, general messages, warning messages, and error messages.

In an ESP-IDF project, run:

```bash
idf.py menuconfig
```

Then enter `cpp_bus_driver configuration` and select the default log level used at startup. The application can also change the level at runtime through the thread-safe API:

```cpp
cpp_bus_driver::Logger::SetMinimumLogLevel(
    cpp_bus_driver::Logger::LogLevel::kWarning);
const auto level = cpp_bus_driver::Logger::GetMinimumLogLevel();
```

Setting the level to `kNone` disables all logs. Call `Logger::ShouldLog()` before constructing expensive log arguments when needed.

## v2 Migration Guide

> [!IMPORTANT]
> v2 is a brand-new major version with many API and directory naming changes. The v1 branch will remain available for existing projects, but no new features will be added to it.

Use current API names in new code. Legacy aliases and their removal policy
are maintained in [compatibility_2_x.h](src/compatibility_2_x.h).

When migrating from v1 to v2, pay special attention to the following changes:

| v1 | v2 |
| --- | --- |
| `iic` | `i2c` |
| `iis` | `i2s` |
| `Pin` related APIs | `Gpio` related APIs |
| Old-style function naming | Google C++ style function naming |
| Less manual resource release support | bus / chip both add the `Deinit()` lifecycle |

Common migration example:

```cpp
cpp_bus_driver::PlatformHal platform_hal;
platform_hal.SetGpioMode(pin, cpp_bus_driver::PlatformHal::GpioMode::kOutput);
platform_hal.GpioWrite(pin, true);
platform_hal.InitGpioInterrupt(pin,
    cpp_bus_driver::PlatformHal::InterruptMode::kFalling, InterruptCallback,
    nullptr, cpp_bus_driver::PlatformHal::GpioStatus::kPullup);
```

## Development Plan

cpp_bus_driver is still under active development. Issues, bug reports, and feature requests are welcome.
