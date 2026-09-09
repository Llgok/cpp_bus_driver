<h1 align="center">cpp_bus_driver</h1>

## **[英文](./README.md) | 中文**

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

当前支持的总线驱动和芯片驱动会随版本持续更新，具体可查看统一入口文件 [`cpp_bus_driver.h`](./src/cpp_bus_driver.h)。

### 面向工程项目的接口设计

- 使用 Google C++ 风格命名，接口更统一。
- 支持可配置日志等级，方便调试总线和芯片问题。
- 支持共享 bus 场景，例如多个 I2C 设备挂载到同一个 master bus。

## 支持框架

| 框架 | 状态 | 说明 |
| --- | --- | --- |
| ESP-IDF | 推荐 | 从v2.0.0起，最小支持的ESP-IDF版本为v5.5.3 |
| Arduino NRF | 支持 | 适用于部分 NRF52840 Arduino 场景 |
| Arduino ESP32 | 共用后端 | 与 ESP-IDF 共用底层实现，核心需满足当前库的 ESP-IDF 最低版本要求，即 v5.5.3 |

> [!NOTE]
> 可用功能取决于目标芯片和 SDK 配置。Arduino-ESP32 构建要求底层 ESP-IDF 为 5.5.3 或更高版本，低于此版本会在编译时明确报错。这里指 ESP-IDF 版本，不是 Arduino-ESP32 包的版本号。满足最低版本要求不代表所有更高版本都已验证兼容。

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

克隆命令：

```bash
git clone https://github.com/Llgok/cpp_bus_driver.git
```

然后在代码中包含统一入口：

```cpp
#include "cpp_bus_driver.h"
```

#### 在 Arduino IDE 中使用（nRF52840）

本库被 Arduino Library Manager 收录后，可在 **库管理器** 中搜索并安装
`cpp_bus_driver`。收录前，可将本仓库克隆到 Arduino Sketchbook 的
`libraries/cpp_bus_driver` 目录。

在开发板管理器中安装 **Adafruit nRF52** 开发板包并选择 nRF52840 开发板。
Adafruit nRF52 1.6.1 默认不会链接完整的 C++ 标准库。请在开发板包目录中新建
`platform.local.txt`（Windows 路径：`C:\Users\<用户名>\AppData\Local\Arduino15\packages\adafruit\hardware\nrf52\1.6.1\platform.local.txt`），
写入以下内容，然后重启 Arduino IDE：

```properties
compiler.libraries.ldflags=-lstdc++
```

`cpp_bus_driver` 使用了 `std::string`、`std::vector` 等 C++ 标准库类型，因此
需要此链接配置。如果升级或重新安装 Adafruit nRF52 开发板包后该文件被删除，
需要重新创建。然后包含统一入口头文件：

```cpp
#include <cpp_bus_driver.h>
```

> [!IMPORTANT]
> Arduino nRF52 构建需要定义 `NRF52840_XXAA`（nRF52840）。

#### 在 Arduino ESP32 中使用

安装满足当前库 ESP-IDF 版本要求的 Arduino-ESP32 核心，底层需为
**ESP-IDF 5.5.3 或更高版本**。包含 `<cpp_bus_driver.h>` 后即可使用共用的 ESP-IDF 后端。
具体功能仍取决于目标芯片的硬件能力以及核心提供的组件。

不要让 Arduino 的 `Wire`、`SPI`、`HardwareSerial` 等对象与本库同时管理同一个外设。

#### 作为 Git submodule 使用

```bash
git submodule add https://github.com/Llgok/cpp_bus_driver.git
git submodule update --init --recursive
```

### 总线驱动

总线驱动负责和 MCU 外设交互，各平台可用的驱动见
[统一入口头文件](src/cpp_bus_driver.h)。

硬件 I2C 使用 `HardwareI2c`。`LegacyHardwareI2c` 仅在 ESP-IDF 和
Arduino ESP32 平台提供，使用旧版 ESP-IDF I2C 驱动；Arduino nRF52 使用
`HardwareI2c`。各总线的可用性取决于目标平台。

基类声明见[总线接口](src/bus/bus_base.h)和
[芯片接口](src/chip/chip_base.h)。

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
auto i2c_bus = std::make_shared<cpp_bus_driver::HardwareI2c>(
    sda, scl, I2C_NUM_0);

auto chip = std::make_unique<cpp_bus_driver::Xl95x5>(i2c_bus);

chip->Init();
chip->Deinit();
```

芯片层 `Deinit()` 会优先释放自身使用的 bus device，并把相关 GPIO 引脚切换到禁用状态，适合低功耗和重新初始化场景。

### 日志配置

`cpp_bus_driver` 提供库级最低日志等级，用于控制调试信息、普通信息、警告信息和错误信息输出。

在 ESP-IDF 工程中可以通过：

```bash
idf.py menuconfig
```

进入 `cpp_bus_driver configuration`，选择启动时使用的默认日志等级。应用也可以在线程安全的运行时接口中动态调整等级：

```cpp
cpp_bus_driver::Logger::SetMinimumLogLevel(
    cpp_bus_driver::Logger::LogLevel::kWarning);
const auto level = cpp_bus_driver::Logger::GetMinimumLogLevel();
```

设置为 `kNone` 会禁止全部日志。需要在构造开销较高的日志参数前主动判断时，可以调用 `Logger::ShouldLog()`。

## v2 迁移说明

> [!IMPORTANT]
> v2 是一个全新的主版本，包含大量 API 和目录命名调整。v1 分支会继续保留给旧项目使用，但后续不会再添加新功能。

新代码应使用当前 API 名称。旧类型别名及移除计划统一维护在
[compatibility_2_x.h](src/compatibility_2_x.h) 中。

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
cpp_bus_driver::PlatformHal platform_hal;
platform_hal.SetGpioMode(pin, cpp_bus_driver::PlatformHal::GpioMode::kOutput);
platform_hal.GpioWrite(pin, true);
platform_hal.InitGpioInterrupt(pin,
    cpp_bus_driver::PlatformHal::InterruptMode::kFalling, InterruptCallback,
    nullptr, cpp_bus_driver::PlatformHal::GpioStatus::kPullup);
```

## 开发计划

cpp_bus_driver 目前仍处于活跃开发阶段，欢迎向我们提交 Issue 反馈问题或提交 Feature 请求。
