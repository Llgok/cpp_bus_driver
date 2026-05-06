<h1 align="center">cpp_bus_driver</h1>

## **English | [中文](./README_CN.md)**

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

The supported bus drivers and chip drivers will continue to evolve with future versions. For the latest list, see the unified entry file [`cpp_bus_driver_library.h`](./src/cpp_bus_driver_library.h).

### Engineering-Oriented API Design

- Uses Google C++ style naming for more consistent APIs.
- Supports configurable log levels, making bus and chip debugging easier.
- Supports shared bus scenarios, such as mounting multiple I2C devices on the same master bus.

## Supported Frameworks

| Framework | Status | Description |
| --- | --- | --- |
| ESP-IDF | Recommended | Starting from v2.0.0, the minimum supported ESP-IDF version is v5.5.3 |
| Arduino NRF | Supported | Suitable for some NRF52840 Arduino scenarios |

> [!NOTE]
> Available bus and chip features may vary between frameworks. ESP-IDF is currently the most complete adaptation target.

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

Then include the unified entry header in your code:

```cpp
#include "cpp_bus_driver_library.h"
```

#### Use as a Git Submodule

```bash
git submodule add https://github.com/Llgok/cpp_bus_driver.git
git submodule update --init --recursive
```

### Bus Drivers

Bus drivers are responsible for interacting with the MCU peripheral driver layer. Common objects include:

```cpp
cpp_bus_driver::HardwareI2c1
cpp_bus_driver::HardwareI2c2
cpp_bus_driver::SoftwareI2c
cpp_bus_driver::HardwareSpi
cpp_bus_driver::HardwareQspi
cpp_bus_driver::HardwareUart
cpp_bus_driver::HardwareI2s
cpp_bus_driver::HardwareSdio
cpp_bus_driver::HardwareMipi
```

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
auto i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c1>(
    sda, scl, I2C_NUM_0);

auto chip = std::make_unique<cpp_bus_driver::Xl95x5>(i2c_bus);

chip->Init();
chip->Deinit();
```

The chip-layer `Deinit()` releases the bus device used by the chip first, and switches related GPIO pins to the disabled state. This is suitable for low-power and reinitialization scenarios.

### Log Configuration

`cpp_bus_driver` provides log level configuration for controlling debug messages, general messages, bus errors, and chip errors.

In an ESP-IDF project, run:

```bash
idf.py menuconfig
```

Then enter `cpp_bus_driver configuration` and select the desired log level.

## v2 Migration Guide

> [!IMPORTANT]
> v2 is a brand-new major version with many API and directory naming changes. The v1 branch will remain available for existing projects, but no new features will be added to it.

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
tool.SetGpioMode(pin, cpp_bus_driver::Tool::GpioMode::kOutput);
tool.GpioWrite(pin, true);
tool.InitGpioInterrupt(pin, cpp_bus_driver::Tool::InterruptMode::kFalling,
    InterruptCallback);
```

## Development Plan

cpp_bus_driver is still under active development. Issues, bug reports, and feature requests are welcome.
