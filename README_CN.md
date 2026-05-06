<h1 align="center">cpp_bus_driver</h1>

## **[English](./README.md) | 中文**

[![Release](https://img.shields.io/github/v/release/Llgok/cpp_bus_driver?style=flat-square)](https://github.com/Llgok/cpp_bus_driver/releases)
[![License](https://img.shields.io/github/license/Llgok/cpp_bus_driver?style=flat-square)](./LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.3%2B-ff6f00?style=flat-square)](https://github.com/espressif/esp-idf)
[![C++](https://img.shields.io/badge/C%2B%2B-11%2B-00599c?style=flat-square)](https://isocpp.org/)

**cpp_bus_driver** 是一个基于 C++11 及以上标准编写的微控制器外设驱动库。它把常见总线、芯片驱动和 GPIO / PWM / 中断等工具能力整理成统一的 C++ 接口，让不同外设可以用相近的方式初始化、读写、释放和复用。

项目从 **v2** 开始进入新的主版本：API 命名、目录结构、生命周期管理和日志配置都进行了较大调整，更适合长期维护和复杂硬件项目集成。

## 目录

- [特性](#特性)
- [支持框架](#支持框架)
- [快速开始](#快速开始)
- [v2 迁移说明](#v2-迁移说明)
- [开发计划](#开发计划)

## 特性

### 统一总线与芯片抽象

支持常见微控制器外设总线，并在总线层和芯片层提供相近的 C++ 使用方式。你可以先创建 bus 对象，再把 bus 传入 chip 对象，让初始化、读写、配置和释放流程保持清晰统一。

当前支持的总线驱动和芯片驱动会随版本持续更新，具体可查看统一入口文件 [`cpp_bus_driver_library.h`](./src/cpp_bus_driver_library.h)。

### 面向工程项目的接口设计

- 使用 Google C++ 风格命名，接口更统一。
- 支持可配置日志等级，方便调试总线和芯片问题。
- 支持共享 bus 场景，例如多个 I2C 设备挂载到同一个 master bus。

## 支持框架

| 框架 | 状态 | 说明 |
| --- | --- | --- |
| ESP-IDF | 推荐 | 从v2.0.0起，最小支持的ESP-IDF版本为v5.5.3 |
| Arduino NRF | 支持 | 适用于部分 NRF52840 Arduino 场景 |

> [!NOTE]
> 不同框架下可用的总线和芯片能力可能不同。ESP-IDF 是当前功能最完整的适配目标。

## 快速开始

### 集成方式

#### 作为 ESP-IDF component 使用

推荐把本仓库放入工程的 `components` 目录：

```bash
your_project/
├── components/
│   └── cpp_bus_driver/
├── main/
└── CMakeLists.txt
```

然后在代码中包含统一入口：

```cpp
#include "cpp_bus_driver_library.h"
```

#### 作为 Git submodule 使用

```bash
git submodule add https://github.com/Llgok/cpp_bus_driver.git
git submodule update --init --recursive
```

### 总线驱动

总线驱动负责和 MCU 外设驱动层交互，常用对象包括：

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

典型生命周期：

```cpp
bus->Init();
bus->Write(data, length);
bus->Read(data, length);
bus->Deinit();
```

> [!NOTE]
> `Deinit()` 能力以具体 bus 类为准。屏幕类 QSPI 场景通常由上层 chip 驱动负责管理释放流程。如果多个芯片共用同一个 I2C bus，反初始化时可以根据资源归属选择是否删除底层 bus，例如 `Deinit(false)` 只释放当前 device，`Deinit(true)` 会连同 bus 一起释放。

### 芯片驱动

芯片驱动建立在总线驱动之上。通常先创建 bus，再把 bus 传入 chip。

```cpp
auto i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c1>(
    sda, scl, I2C_NUM_0);

auto chip = std::make_unique<cpp_bus_driver::Xl95x5>(i2c_bus);

chip->Init();
chip->Deinit();
```

芯片层 `Deinit()` 会优先释放自身使用的 bus device，并把相关 GPIO 引脚切换到禁用状态，适合低功耗和重新初始化场景。

### 日志配置

`cpp_bus_driver` 提供日志等级配置，用于控制调试信息、普通信息、总线错误和芯片错误输出。

在 ESP-IDF 工程中可以通过：

```bash
idf.py menuconfig
```

进入 `cpp_bus_driver configuration`，选择需要的日志等级。

## v2 迁移说明

> [!IMPORTANT]
> v2 是一个全新的主版本，包含大量 API 和目录命名调整。v1 分支会继续保留给旧项目使用，但后续不会再添加新功能。

从 v1 迁移到 v2 时，建议重点检查下面这些变化：

| v1 | v2 |
| --- | --- |
| `iic` | `i2c` |
| `iis` | `i2s` |
| `Pin` 相关 API | `Gpio` 相关 API |
| 旧风格函数命名 | Google C++ 风格函数命名 |
| 手动释放资源较少 | bus / chip 均增加 `Deinit()` 生命周期 |

常见改动示例：

```cpp
tool.SetGpioMode(pin, cpp_bus_driver::Tool::GpioMode::kOutput);
tool.GpioWrite(pin, true);
tool.InitGpioInterrupt(pin, cpp_bus_driver::Tool::InterruptMode::kFalling,
    InterruptCallback);
```

## 开发计划

cpp_bus_driver 目前仍处于活跃开发阶段，欢迎向我们提交 Issue 反馈问题或提交 Feature 请求。
