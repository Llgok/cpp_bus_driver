/*
 * @Description: ICM20948 九轴惯性传感器 I2C/SPI 共用驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-07-31 15:20:00
 * @LastEditTime: 2026-08-03 16:11:41
 * @License: GPL 3.0
 */
#pragma once

#include <mutex>

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Icm20948 final : public Tool {
 public:
  enum class Interface {
    kI2c,  // 使用 I2C 主接口。
    kSpi,  // 使用四线 SPI 主接口。
  };

  enum class AccelRange {
    k2g = 0,  // ±2 g。
    k4g,      // ±4 g。
    k8g,      // ±8 g。
    k16g,     // ±16 g。
  };

  enum class GyroRange {
    k250Dps = 0,  // ±250 dps。
    k500Dps,      // ±500 dps。
    k1000Dps,     // ±1000 dps。
    k2000Dps,     // ±2000 dps。
  };

  enum class Dlpf {
    k0 = 0,  // DLPF 配置 0。
    k1,      // DLPF 配置 1。
    k2,      // DLPF 配置 2。
    k3,      // DLPF 配置 3。
    k4,      // DLPF 配置 4。
    k5,      // DLPF 配置 5。
    k6,      // DLPF 配置 6。
    k7,      // DLPF 配置 7。
  };

  enum class MagnetometerMode {
    kPowerDown = 0x00,        // 掉电模式。
    kContinuous10Hz = 0x02,   // 10 Hz 连续测量模式。
    kContinuous20Hz = 0x04,   // 20 Hz 连续测量模式。
    kContinuous50Hz = 0x06,   // 50 Hz 连续测量模式。
    kContinuous100Hz = 0x08,  // 100 Hz 连续测量模式。
  };

  // 三轴有符号原始数据。
  struct RawVector3 {
    int16_t x = 0;  // X 轴原始值。
    int16_t y = 0;  // Y 轴原始值。
    int16_t z = 0;  // Z 轴原始值。
  };

  // 三轴浮点物理量数据。
  struct Vector3 {
    float x = 0.0f;  // X 轴物理量。
    float y = 0.0f;  // Y 轴物理量。
    float z = 0.0f;  // Z 轴物理量。
  };

  // 一次连续寄存器读取获得的完整原始数据。
  struct RawData {
    RawVector3 acceleration;                 // 加速度计原始值。
    RawVector3 angular_velocity;             // 陀螺仪原始值。
    RawVector3 magnetic_field;               // AK09916 磁场原始值。
    int16_t temperature = 0;                 // 温度传感器原始值。
    bool magnetometer_data_ready = false;    // 读取瞬间的 AK09916 DRDY 状态。
    bool magnetometer_data_overrun = false;  // AK09916 DOR 状态。
    bool magnetometer_overflow = false;      // AK09916 HOFL 状态。
  };

  // 由原始数据换算后的物理量。
  struct SensorData {
    Vector3 acceleration_g;                  // 三轴加速度，单位 g。
    Vector3 angular_velocity_dps;            // 三轴角速度，单位 dps。
    Vector3 magnetic_field_ut;               // 三轴磁场，单位 uT。
    float temperature_celsius = 0.0f;        // 温度，单位摄氏度。
    bool magnetometer_data_ready = false;    // 读取瞬间的 AK09916 DRDY 状态。
    bool magnetometer_data_overrun = false;  // AK09916 DOR 状态。
    bool magnetometer_overflow = false;      // AK09916 HOFL 状态。
  };

  // 初始化配置不包含任何运行时校准步骤。
  struct Config {
    AccelRange accel_range = AccelRange::k2g;   // 加速度计量程。
    GyroRange gyro_range = GyroRange::k250Dps;  // 陀螺仪量程。
    Dlpf accel_dlpf = Dlpf::k6;                 // 加速度计 DLPF 档位。
    Dlpf gyro_dlpf = Dlpf::k6;                  // 陀螺仪 DLPF 档位。
    Dlpf temperature_dlpf = Dlpf::k6;           // 温度传感器 DLPF 档位。
    uint16_t accel_sample_rate_divider = 9;     // 加速度计采样分频值。
    uint8_t gyro_sample_rate_divider = 9;       // 陀螺仪采样分频值。
    MagnetometerMode magnetometer_mode =
        MagnetometerMode::kContinuous20Hz;      // AK09916 工作模式。
    bool accel_dlpf_enabled = true;             // 是否启用加速度计 DLPF。
    bool gyro_dlpf_enabled = true;              // 是否启用陀螺仪 DLPF。
    bool accelerometer_enabled = true;          // 是否启用加速度计。
    bool gyroscope_enabled = true;              // 是否启用陀螺仪。
    bool temperature_enabled = true;            // 是否启用温度传感器。
    bool data_ready_interrupt_enabled = false;  // 是否启用数据就绪中断。
  };

  /**
   * @brief 创建使用 I2C 主接口的 ICM20948 驱动
   * @param bus I2C 总线
   * @param address ICM20948 七位 I2C 地址
   */
  explicit Icm20948(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault)
      : i2c_bus_(bus), i2c_address_(address) {}

  /**
   * @brief 创建使用四线 SPI 主接口的 ICM20948 驱动
   * @param bus SPI Mode 0 总线
   * @param cs ICM20948 片选 GPIO
   */
  explicit Icm20948(std::shared_ptr<BusSpiGuide> bus, int32_t cs)
      : spi_bus_(bus), spi_cs_(cs) {}

  /**
   * @brief 使用默认配置初始化 ICM20948
   * @param freq_hz 主机总线频率；默认值按 I2C/SPI 接口分别选择
   * @return 初始化成功返回 true，失败返回 false
   */
  bool Init(int32_t freq_hz = kDefaultValue);

  /**
   * @brief 使用指定配置初始化 ICM20948
   * @param config 加速度计、陀螺仪和磁力计配置
   * @param freq_hz 主机总线频率；默认值按 I2C/SPI 接口分别选择
   * @return 初始化成功返回 true，失败返回 false
   */
  bool Init(const Config& config, int32_t freq_hz = kDefaultValue);

  /**
   * @brief 释放 ICM20948 对应的总线设备
   * @param delete_bus 是否同时释放底层总线
   * @return 释放成功返回 true，失败返回 false
   */
  bool Deinit(bool delete_bus = true);

  /**
   * @brief 复位芯片并重新应用当前配置
   * @return 复位和重新配置成功返回 true，失败返回 false
   */
  bool Reset();

  /**
   * @brief 获取 ICM20948 芯片标识
   * @param device_id 返回 WHO_AM_I 寄存器值
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetDeviceId(uint8_t& device_id);

  /**
   * @brief 获取内部 AK09916 磁力计芯片标识
   * @param device_id 返回 WIA2 寄存器值
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetMagnetometerDeviceId(uint8_t& device_id);

  /**
   * @brief 设置芯片睡眠状态
   * @param sleep true 进入睡眠，false 唤醒并恢复磁力计模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetSleep(bool sleep);

  /**
   * @brief 分别启用或停用加速度计、陀螺仪和温度传感器
   * @param accelerometer_enabled 是否启用加速度计
   * @param gyroscope_enabled 是否启用陀螺仪
   * @param temperature_enabled 是否启用温度传感器
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetSensorEnabled(bool accelerometer_enabled, bool gyroscope_enabled,
      bool temperature_enabled);

  /**
   * @brief 设置加速度计量程
   * @param range 加速度计量程
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAccelRange(AccelRange range);

  /**
   * @brief 设置陀螺仪量程
   * @param range 陀螺仪量程
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGyroRange(GyroRange range);

  /**
   * @brief 设置加速度计数字低通滤波器
   * @param dlpf 滤波器档位
   * @param enable true 启用 DLPF，false 旁路 DLPF
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAccelDlpf(Dlpf dlpf, bool enable = true);

  /**
   * @brief 设置陀螺仪数字低通滤波器
   * @param dlpf 滤波器档位
   * @param enable true 启用 DLPF，false 旁路 DLPF
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGyroDlpf(Dlpf dlpf, bool enable = true);

  /**
   * @brief 设置温度传感器数字低通滤波器
   * @param dlpf 滤波器档位
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetTemperatureDlpf(Dlpf dlpf);

  /**
   * @brief 设置加速度计采样率分频值
   * @param divider 12 位分频值；DLPF有效时输出率为1125/(1+divider) Hz
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAccelSampleRateDivider(uint16_t divider);

  /**
   * @brief 设置陀螺仪采样率分频值
   * @param divider 8 位分频值；DLPF有效时输出率为1100/(1+divider) Hz
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGyroSampleRateDivider(uint8_t divider);

  /**
   * @brief 设置 AK09916 掉电或连续测量模式
   * @param mode 掉电模式或10/20/50/100 Hz连续测量模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetMagnetometerMode(MagnetometerMode mode);

  /**
   * @brief 设置原始数据就绪中断
   * @param enable true 启用中断，false 关闭中断
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDataReadyInterrupt(bool enable);

  /**
   * @brief 读取并清除原始数据就绪状态
   * @param ready 返回数据是否就绪
   * @return 状态读取成功返回 true，失败返回 false
   */
  bool GetDataReady(bool& ready);

  /**
   * @brief 连续读取加速度、角速度、温度和磁场原始数据
   * @param data 返回完整原始数据
   * @return 总线读取成功返回 true，失败返回 false
   */
  bool ReadRawData(RawData& data);

  /**
   * @brief 连续读取并换算全部传感器物理量
   * @param data 返回加速度 g、角速度 dps、磁场 uT 和温度摄氏度
   * @return 总线读取成功返回 true，失败返回 false
   */
  bool ReadData(SensorData& data);

  /**
   * @brief 读取加速度物理量
   * @param acceleration_g 返回三轴加速度，单位 g
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadAcceleration(Vector3& acceleration_g);

  /**
   * @brief 读取角速度物理量
   * @param angular_velocity_dps 返回三轴角速度，单位 dps
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadAngularVelocity(Vector3& angular_velocity_dps);

  /**
   * @brief 读取芯片温度
   * @param temperature_celsius 返回温度，单位摄氏度
   * @return 读取成功返回 true，失败返回 false
   */
  bool ReadTemperature(float& temperature_celsius);

  /**
   * @brief 读取磁力计数据和状态
   * @param magnetic_field_ut 返回三轴磁场，单位 uT
   * @param data_ready 返回本次辅助 I2C 快照中的数据就绪状态
   * @param data_overrun 返回磁力计数据覆盖状态
   * @param overflow 返回磁力计溢出状态
   * @return 总线读取成功返回 true，失败返回 false
   */
  bool ReadMagnetometer(Vector3& magnetic_field_ut, bool& data_ready,
      bool& data_overrun, bool& overflow);

  /**
   * @brief 获取当前自动选择的主机接口类型
   * @return 使用 I2C 总线时返回 kI2c，使用 SPI 总线时返回 kSpi
   */
  Interface interface() const {
    return i2c_bus_ != nullptr ? Interface::kI2c : Interface::kSpi;
  }

  /**
   * @brief 查询传感器是否已完成初始化
   * @return 已完成初始化返回 true，否则返回 false
   */
  bool initialized() const;

  /**
   * @brief 查询传感器是否处于睡眠状态
   * @return 处于睡眠状态返回 true，否则返回 false
   */
  bool sleeping() const;

  /**
   * @brief 获取当前成功应用的配置
   * @return 当前配置的线程安全副本
   */
  Config config() const;

 private:
  // ICM20948 用户寄存器 Bank。
  enum class Bank : uint8_t {
    k0 = 0,           // USER BANK 0。
    k1,               // USER BANK 1。
    k2,               // USER BANK 2。
    k3,               // USER BANK 3。
    kInvalid = 0xFF,  // 尚未缓存有效 Bank。
  };

  // 高字节保存 USER BANK，低字节保存该 Bank 内的寄存器地址。
  enum class Cmd : uint16_t {
    // USER BANK 0 寄存器。
    kRoWhoAmI = 0x0000,
    kRwUserCtrl = 0x0003,
    kRwLpConfig = 0x0005,
    kRwPowerManagement1 = 0x0006,
    kRwPowerManagement2 = 0x0007,
    kRwInterruptEnable1 = 0x0011,
    kRoI2cMasterStatus = 0x0017,
    kRoInterruptStatus1 = 0x001A,
    kRoAccelXoutH = 0x002D,
    kRoGyroXoutH = 0x0033,
    kRoTemperatureOutH = 0x0039,
    kRoExternalSensorData00 = 0x003B,

    // USER BANK 2 寄存器。
    kRwGyroSampleRateDivider = 0x0200,
    kRwGyroConfig1 = 0x0201,
    kRwOdrAlignEnable = 0x0209,
    kRwAccelSampleRateDividerHigh = 0x0210,
    kRwAccelSampleRateDividerLow = 0x0211,
    kRwAccelConfig = 0x0214,
    kRwTemperatureConfig = 0x0253,

    // USER BANK 3 寄存器。
    kRwI2cMasterOdrConfig = 0x0300,
    kRwI2cMasterCtrl = 0x0301,
    kRwI2cMasterDelayCtrl = 0x0302,
    kRwI2cSlave0Address = 0x0303,
    kRwI2cSlave0Register = 0x0304,
    kRwI2cSlave0Ctrl = 0x0305,
    kRwI2cSlave4Address = 0x0313,
    kRwI2cSlave4Register = 0x0314,
    kRwI2cSlave4Ctrl = 0x0315,
    kRwI2cSlave4DataOut = 0x0316,
    kRoI2cSlave4DataIn = 0x0317,
  };

  // AK09916 内部 I2C 寄存器。
  enum class Ak09916Cmd : uint8_t {
    kRoDeviceId = 0x01,         // 芯片标识 WIA2。
    kRoStatus1 = 0x10,          // 数据就绪和覆盖状态 ST1。
    kRoMeasurementData = 0x11,  // 磁场数据起始地址 HXL。
    kRoStatus2 = 0x18,          // 磁场溢出状态 ST2。
    kRwControl2 = 0x31,         // 测量模式控制 CNTL2。
    kWoControl3 = 0x32,         // 软件复位控制 CNTL3。
  };

  static constexpr int16_t kDeviceI2cAddressDefault = 0x68;  // AD0=0。
  static constexpr uint8_t kDeviceId = 0xEA;                // WHO_AM_I 期望值。
  static constexpr uint8_t kAk09916Address = 0x0C;          // 内部磁力计地址。
  static constexpr uint8_t kAk09916DeviceId = 0x09;         // WIA2 期望值。
  static constexpr int32_t kDefaultIcmI2cFreqHz = 400000;   // I2C 上限。
  static constexpr int32_t kDefaultIcmSpiFreqHz = 7000000;  // SPI 上限。
  static constexpr uint32_t kResetDelayMs = 100;            // 主芯片复位等待。
  static constexpr uint32_t kGyroscopeStartDelayMs = 50;    // 陀螺仪启动等待。
  static constexpr uint32_t kMagnetometerResetDelayMs =
      100;                                                  // 磁力计复位等待。
  static constexpr uint32_t kMagnetometerModeDelayMs = 10;  // 模式切换等待。
  static constexpr uint32_t kMinimumAuxiliaryTransactionTimeoutMs =
      100;  // 辅助 I2C 单次传输最短超时。
  static constexpr uint32_t kAuxiliaryTransactionTimeoutMarginMs =
      50;  // 辅助 I2C 单次传输调度余量。
  static constexpr float kTemperatureSensitivity =
      333.87f;  // 温度灵敏度，LSB/°C。
  static constexpr float kTemperatureOffsetCelsius =
      21.0f;  // 温度换算偏移，°C。
  static constexpr float kMagnetometerSensitivityUt =
      0.15f;  // AK09916 灵敏度，uT/LSB。

  /**
   * @brief 初始化构造函数传入的 I2C 或 SPI 总线
   * @param freq_hz 主机总线频率
   * @return 总线设备初始化并探测成功返回 true，否则返回 false
   */
  bool InitBus(int32_t freq_hz);

  /**
   * @brief 复位 ICM20948 并清除驱动运行状态
   * @return 复位命令写入成功返回 true，否则返回 false
   */
  bool ResetDevice();

  /**
   * @brief 直接设置 ICM20948 核心睡眠状态并同步内部状态
   * @param sleep true 进入睡眠，false 唤醒并恢复主接口
   * @return 睡眠位和主接口状态更新成功返回 true，否则返回 false
   */
  bool SetCoreSleep(bool sleep);

  /**
   * @brief 初始化或复位失败后尽量关闭磁力计并让主芯片进入睡眠
   */
  void EnterSafeStateAfterInitializationFailure();

  /**
   * @brief 将完整传感器配置写入 ICM20948 和 AK09916
   * @param config 需要应用的传感器配置
   * @return 所有配置写入成功返回 true，否则返回 false
   */
  bool ConfigureDevice(const Config& config);

  /**
   * @brief 根据传入的总线类型配置 I2C 或四线 SPI 主接口
   * @return 主接口配置成功返回 true，否则返回 false
   */
  bool ConfigureHostInterface();

  /**
   * @brief 复位、识别并配置内部 AK09916 磁力计
   * @param mode 磁力计目标工作模式
   * @return 磁力计配置成功返回 true，否则返回 false
   */
  bool ConfigureMagnetometer(MagnetometerMode mode);

  /**
   * @brief 配置辅助 I2C SLV0 连续读取 AK09916 状态与磁场数据
   * @param mode 磁力计当前硬件工作模式
   * @return 连续数据流配置成功返回 true，否则返回 false
   */
  bool ConfigureMagnetometerStream(MagnetometerMode mode);

  /**
   * @brief 立即切换 AK09916 硬件工作模式
   * @param mode 磁力计目标工作模式
   * @return 模式切换和数据流恢复成功返回 true，否则返回 false
   */
  bool SetActiveMagnetometerMode(MagnetometerMode mode);

  /**
   * @brief 启用 ICM20948 内部辅助 I2C 主机
   * @return 辅助 I2C 时钟和输出率配置成功返回 true，否则返回 false
   */
  bool EnableAuxiliaryI2cMaster();

  /**
   * @brief 等待辅助 I2C SLV4 单字节传输完成
   * @return 传输在超时前完成且未发生 NACK/仲裁丢失时返回 true
   */
  bool WaitForAuxiliaryTransaction();

  /**
   * @brief 根据当前有效传感器 ODR 计算辅助 I2C 单次传输超时
   * @return 包含一个完整调度周期和余量的超时时间，单位毫秒
   */
  uint32_t GetAuxiliaryTransactionTimeoutMs() const;

  /**
   * @brief 检查 AK09916 连续数据通道是否发生 NACK 或仲裁丢失
   * @return 数据通道可用返回 true，否则返回 false
   */
  bool CheckMagnetometerStreamHealth();

  /**
   * @brief 通过辅助 I2C SLV4 读取一个 AK09916 寄存器
   * @param cmd AK09916 寄存器
   * @param data 返回读取值
   * @param restore_stream 读取结束后是否恢复 SLV0 连续数据流
   * @return 读取成功返回 true，否则返回 false
   */
  bool ReadAk09916Register(Ak09916Cmd cmd, uint8_t& data, bool restore_stream);

  /**
   * @brief 通过辅助 I2C SLV4 写入一个 AK09916 寄存器
   * @param cmd AK09916 寄存器
   * @param data 待写入值
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteAk09916Register(Ak09916Cmd cmd, uint8_t data);

  /**
   * @brief 选择 ICM20948 用户寄存器 Bank
   * @param bank 目标 Bank
   * @return Bank 已选择或切换成功返回 true，否则返回 false
   */
  bool SelectBank(Bank bank);

  /**
   * @brief 读取一个或多个连续 ICM20948 寄存器
   * @param cmd 起始寄存器命令
   * @param data 返回数据缓冲区
   * @param length 读取字节数
   * @return 读取成功返回 true，否则返回 false
   */
  bool ReadRegister(Cmd cmd, uint8_t* data, size_t length = 1);

  /**
   * @brief 写入一个 ICM20948 寄存器
   * @param cmd 目标寄存器命令
   * @param data 待写入值
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteRegister(Cmd cmd, uint8_t data);

  /**
   * @brief 写入一个或多个连续 ICM20948 寄存器
   * @param cmd 起始寄存器命令
   * @param data 待写入数据缓冲区
   * @param length 写入字节数
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteRegister(Cmd cmd, const uint8_t* data, size_t length);

  /**
   * @brief 以读改写方式更新 ICM20948 寄存器位
   * @param cmd 目标寄存器命令
   * @param clear_mask 需要先清零的位掩码
   * @param set_mask 需要置位的位掩码
   * @return 更新成功返回 true，否则返回 false
   */
  bool UpdateRegister(Cmd cmd, uint8_t clear_mask, uint8_t set_mask);

  /**
   * @brief 通过自动选择的 I2C 或 SPI 总线读取寄存器
   * @param reg 当前 Bank 内的寄存器地址
   * @param data 返回数据缓冲区
   * @param length 读取字节数
   * @return 读取成功返回 true，否则返回 false
   */
  bool ReadTransport(uint8_t reg, uint8_t* data, size_t length);

  /**
   * @brief 通过自动选择的 I2C 或 SPI 总线写入寄存器
   * @param reg 当前 Bank 内的寄存器地址
   * @param data 待写入数据缓冲区
   * @param length 写入字节数
   * @return 写入成功返回 true，否则返回 false
   */
  bool WriteTransport(uint8_t reg, const uint8_t* data, size_t length);

  /**
   * @brief 从寄存器命令中解析用户 Bank
   * @param cmd 寄存器命令
   * @return 命令对应的用户 Bank
   */
  static Bank GetBank(Cmd cmd);

  /**
   * @brief 从寄存器命令中解析 Bank 内地址
   * @param cmd 寄存器命令
   * @return Bank 内八位寄存器地址
   */
  static uint8_t GetRegisterAddress(Cmd cmd);

  /**
   * @brief 将大端序两字节数据解析为有符号十六位值
   * @param data 至少包含两个字节的缓冲区
   * @return 解析后的有符号十六位值
   */
  static int16_t DecodeBigEndian(const uint8_t* data);

  /**
   * @brief 将小端序两字节数据解析为有符号十六位值
   * @param data 至少包含两个字节的缓冲区
   * @return 解析后的有符号十六位值
   */
  static int16_t DecodeLittleEndian(const uint8_t* data);

  /**
   * @brief 获取指定加速度计量程的灵敏度
   * @param range 加速度计量程
   * @return 灵敏度，单位 LSB/g
   */
  static float GetAccelSensitivity(AccelRange range);

  /**
   * @brief 获取指定陀螺仪量程的灵敏度
   * @param range 陀螺仪量程
   * @return 灵敏度，单位 LSB/dps
   */
  static float GetGyroSensitivity(GyroRange range);

  /**
   * @brief 检查加速度计量程枚举值是否合法
   * @param range 加速度计量程
   * @return 枚举值合法返回 true，否则返回 false
   */
  static bool IsValidAccelRange(AccelRange range);

  /**
   * @brief 检查陀螺仪量程枚举值是否合法
   * @param range 陀螺仪量程
   * @return 枚举值合法返回 true，否则返回 false
   */
  static bool IsValidGyroRange(GyroRange range);

  /**
   * @brief 检查数字低通滤波器枚举值是否合法
   * @param dlpf 数字低通滤波器档位
   * @return 枚举值合法返回 true，否则返回 false
   */
  static bool IsValidDlpf(Dlpf dlpf);

  /**
   * @brief 检查 AK09916 工作模式枚举值是否合法
   * @param mode AK09916 工作模式
   * @return 枚举值合法返回 true，否则返回 false
   */
  static bool IsValidMagnetometerMode(MagnetometerMode mode);

  /**
   * @brief 根据构造函数传入的非空总线自动判断是否使用 I2C
   * @return 使用 I2C 返回 true，使用 SPI 返回 false
   */
  bool UsesI2c() const { return i2c_bus_ != nullptr; }

  std::shared_ptr<BusI2cGuide> i2c_bus_;
  std::shared_ptr<BusSpiGuide> spi_bus_;
  int16_t i2c_address_ = kDeviceI2cAddressDefault;
  int32_t spi_cs_ = kDefaultValue;
  Config config_;  // 当前成功应用的传感器配置。
  MagnetometerMode active_magnetometer_mode_ =
      MagnetometerMode::kPowerDown;  // 当前 AK09916 硬件模式。
  MagnetometerMode resume_magnetometer_mode_ =
      MagnetometerMode::kContinuous20Hz;       // 唤醒时恢复的 AK09916 模式。
  Bank selected_bank_ = Bank::kInvalid;        // 当前寄存器 Bank 缓存。
  bool bus_initialized_ = false;               // 主机总线设备是否已初始化。
  bool initialized_ = false;                   // 传感器是否已完成完整初始化。
  bool sleeping_ = true;                       // ICM20948 是否处于睡眠状态。
  bool auxiliary_i2c_master_enabled_ = false;  // 辅助 I2C 主机是否已启用。
  bool magnetometer_stream_ready_ = false;     // AK09916 连续数据流是否可用。
  mutable std::recursive_mutex mutex_;         // 保护 Bank 与设备运行状态。
};

}  // namespace cpp_bus_driver
