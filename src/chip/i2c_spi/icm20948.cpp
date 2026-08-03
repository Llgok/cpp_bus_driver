/*
 * @Description: ICM20948 九轴惯性传感器 I2C/SPI 共用驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-07-31 15:20:00
 * @LastEditTime: 2026-08-03 16:11:40
 * @License: GPL 3.0
 */
#include "icm20948.h"

namespace cpp_bus_driver {

bool Icm20948::Init(int32_t freq_hz) { return Init(Config{}, freq_hz); }

bool Icm20948::Init(const Config& config, int32_t freq_hz) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (i2c_bus_ == nullptr && spi_bus_ == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid bus\n");
    return false;
  }

  if ((UsesI2c() && (i2c_address_ < 0 || i2c_address_ > 0x7F)) ||
      (!UsesI2c() && spi_cs_ == kDefaultValue) ||
      (freq_hz != kDefaultValue && freq_hz <= 0)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid bus config\n");
    return false;
  }

  if (!IsValidAccelRange(config.accel_range) ||
      !IsValidGyroRange(config.gyro_range) || !IsValidDlpf(config.accel_dlpf) ||
      !IsValidDlpf(config.gyro_dlpf) || !IsValidDlpf(config.temperature_dlpf) ||
      !IsValidMagnetometerMode(config.magnetometer_mode)) {
    LogMessage(
        LogLevel::kWarning, __FILE__, __LINE__, "Invalid sensor config\n");
    return false;
  }

  if (initialized_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Icm20948 has been initialized\n");
    return true;
  }

  if (!InitBus(freq_hz)) {
    return false;
  }

  config_ = config;
  if (config_.accel_sample_rate_divider > 0x0FFF) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Accel sample rate divider out of range\n");
    config_.accel_sample_rate_divider = 0x0FFF;
  }
  resume_magnetometer_mode_ = config_.magnetometer_mode;

  uint8_t device_id = 0;
  if (!ResetDevice()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Reset ICM20948 failed\n");
    EnterSafeStateAfterInitializationFailure();
    return false;
  }
  if (!ConfigureHostInterface()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Configure ICM20948 host interface failed\n");
    EnterSafeStateAfterInitializationFailure();
    return false;
  }
  if (!GetDeviceId(device_id) || device_id != kDeviceId) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ICM20948 device id mismatch (read: %#X, expected: %#X)\n", device_id,
        kDeviceId);
    EnterSafeStateAfterInitializationFailure();
    return false;
  }

  if (!ConfigureDevice(config_)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "ConfigureDevice failed\n");
    EnterSafeStateAfterInitializationFailure();
    return false;
  }
  initialized_ = true;

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "ICM20948 initialized by %s\n", UsesI2c() ? "I2C" : "SPI");
  return true;
}

bool Icm20948::Deinit(bool delete_bus) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  bool power_result = true;
  if (bus_initialized_ && !sleeping_) {
    if (auxiliary_i2c_master_enabled_) {
      power_result &= SetActiveMagnetometerMode(MagnetometerMode::kPowerDown);
    }
    power_result &= SetCoreSleep(true);
  }

  bool bus_result = true;
  if (bus_initialized_) {
    if (UsesI2c()) {
      bus_result = i2c_bus_->Deinit(delete_bus);
    } else {
      bus_result = spi_bus_->Deinit(delete_bus);
    }
  }

  if (bus_result) {
    selected_bank_ = Bank::kInvalid;
    active_magnetometer_mode_ = MagnetometerMode::kPowerDown;
    bus_initialized_ = false;
    initialized_ = false;
    sleeping_ = true;
    auxiliary_i2c_master_enabled_ = false;
    magnetometer_stream_ready_ = false;
  }
  return power_result && bus_result;
}

bool Icm20948::Reset() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (!bus_initialized_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "ICM20948 bus is not initialized\n");
    return false;
  }

  uint8_t device_id = 0;
  if (!ResetDevice()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Reset ICM20948 failed\n");
    EnterSafeStateAfterInitializationFailure();
    return false;
  }
  if (!ConfigureHostInterface()) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Configure ICM20948 host interface after reset failed\n");
    EnterSafeStateAfterInitializationFailure();
    return false;
  }
  if (!GetDeviceId(device_id) || device_id != kDeviceId) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "ICM20948 device id mismatch after reset "
        "(read: %#X, expected: %#X)\n",
        device_id, kDeviceId);
    EnterSafeStateAfterInitializationFailure();
    return false;
  }
  if (!ConfigureDevice(config_)) {
    EnterSafeStateAfterInitializationFailure();
    return false;
  }
  initialized_ = true;
  return true;
}

bool Icm20948::GetDeviceId(uint8_t& device_id) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!bus_initialized_) {
    return false;
  }
  return ReadRegister(Cmd::kRoWhoAmI, &device_id);
}

bool Icm20948::GetMagnetometerDeviceId(uint8_t& device_id) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || sleeping_ || !auxiliary_i2c_master_enabled_) {
    return false;
  }
  return ReadAk09916Register(Ak09916Cmd::kRoDeviceId, device_id, true);
}

bool Icm20948::SetSleep(bool sleep) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (!initialized_) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "ICM20948 is not initialized\n");
    return false;
  }
  if (sleep == sleeping_) {
    if (!sleep) {
      if (!UsesI2c() && !UpdateRegister(Cmd::kRwUserCtrl, 0x00, 0x10)) {
        return false;
      }
      if (resume_magnetometer_mode_ != MagnetometerMode::kPowerDown &&
          (active_magnetometer_mode_ != resume_magnetometer_mode_ ||
              !magnetometer_stream_ready_)) {
        return SetActiveMagnetometerMode(resume_magnetometer_mode_);
      }
    }
    return true;
  }

  if (sleep) {
    if (active_magnetometer_mode_ != MagnetometerMode::kPowerDown) {
      resume_magnetometer_mode_ = active_magnetometer_mode_;
    }
    if (!SetActiveMagnetometerMode(MagnetometerMode::kPowerDown) ||
        !SetCoreSleep(true)) {
      return false;
    }
    return true;
  }

  if (!SetCoreSleep(false)) {
    return false;
  }

  if (resume_magnetometer_mode_ != MagnetometerMode::kPowerDown &&
      !SetActiveMagnetometerMode(resume_magnetometer_mode_)) {
    return false;
  }
  return true;
}

bool Icm20948::SetSensorEnabled(bool accelerometer_enabled,
    bool gyroscope_enabled, bool temperature_enabled) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (!initialized_) {
    return false;
  }

  uint8_t power_management_2 = 0;
  if (!accelerometer_enabled) {
    power_management_2 |= 0x38;
  }
  if (!gyroscope_enabled) {
    power_management_2 |= 0x07;
  }

  if (!WriteRegister(Cmd::kRwPowerManagement2, power_management_2)) {
    return false;
  }
  config_.accelerometer_enabled = accelerometer_enabled;
  config_.gyroscope_enabled = gyroscope_enabled;

  if (!UpdateRegister(
          Cmd::kRwPowerManagement1, 0x08, temperature_enabled ? 0x00 : 0x08)) {
    return false;
  }
  config_.temperature_enabled = temperature_enabled;
  return true;
}

bool Icm20948::SetAccelRange(AccelRange range) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || !IsValidAccelRange(range)) {
    return false;
  }

  const bool result = UpdateRegister(
      Cmd::kRwAccelConfig, 0x06, static_cast<uint8_t>(range) << 1);
  if (result) {
    config_.accel_range = range;
  }
  return result;
}

bool Icm20948::SetGyroRange(GyroRange range) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || !IsValidGyroRange(range)) {
    return false;
  }

  const bool result = UpdateRegister(
      Cmd::kRwGyroConfig1, 0x06, static_cast<uint8_t>(range) << 1);
  if (result) {
    config_.gyro_range = range;
  }
  return result;
}

bool Icm20948::SetAccelDlpf(Dlpf dlpf, bool enable) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || !IsValidDlpf(dlpf)) {
    return false;
  }

  const uint8_t value = static_cast<uint8_t>(
      (static_cast<uint8_t>(dlpf) << 3) | static_cast<uint8_t>(enable));
  const bool result = UpdateRegister(Cmd::kRwAccelConfig, 0x39, value);
  if (result) {
    config_.accel_dlpf = dlpf;
    config_.accel_dlpf_enabled = enable;
  }
  return result;
}

bool Icm20948::SetGyroDlpf(Dlpf dlpf, bool enable) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || !IsValidDlpf(dlpf)) {
    return false;
  }

  const uint8_t value = static_cast<uint8_t>(
      (static_cast<uint8_t>(dlpf) << 3) | static_cast<uint8_t>(enable));
  const bool result = UpdateRegister(Cmd::kRwGyroConfig1, 0x39, value);
  if (result) {
    config_.gyro_dlpf = dlpf;
    config_.gyro_dlpf_enabled = enable;
  }
  return result;
}

bool Icm20948::SetTemperatureDlpf(Dlpf dlpf) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || !IsValidDlpf(dlpf)) {
    return false;
  }

  const bool result = UpdateRegister(
      Cmd::kRwTemperatureConfig, 0x07, static_cast<uint8_t>(dlpf));
  if (result) {
    config_.temperature_dlpf = dlpf;
  }
  return result;
}

bool Icm20948::SetAccelSampleRateDivider(uint16_t divider) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_) {
    return false;
  }

  if (divider > 0x0FFF) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Accel sample rate divider out of range\n");
    divider = 0x0FFF;
  }

  const uint16_t previous_divider = config_.accel_sample_rate_divider;
  if (!WriteRegister(Cmd::kRwAccelSampleRateDividerHigh,
          static_cast<uint8_t>((divider >> 8) & 0x0F))) {
    return false;
  }
  if (!WriteRegister(
          Cmd::kRwAccelSampleRateDividerLow, static_cast<uint8_t>(divider))) {
    const bool rollback_result =
        WriteRegister(Cmd::kRwAccelSampleRateDividerHigh,
            static_cast<uint8_t>((previous_divider >> 8) & 0x0F)) &&
        WriteRegister(Cmd::kRwAccelSampleRateDividerLow,
            static_cast<uint8_t>(previous_divider));
    if (!rollback_result) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "Failed to restore accel sample rate divider\n");
    }
    return false;
  }
  config_.accel_sample_rate_divider = divider;
  return true;
}

bool Icm20948::SetGyroSampleRateDivider(uint8_t divider) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_) {
    return false;
  }

  const bool result = WriteRegister(Cmd::kRwGyroSampleRateDivider, divider);
  if (result) {
    config_.gyro_sample_rate_divider = divider;
  }
  return result;
}

bool Icm20948::SetMagnetometerMode(MagnetometerMode mode) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (!initialized_ || !IsValidMagnetometerMode(mode)) {
    return false;
  }

  if (sleeping_) {
    config_.magnetometer_mode = mode;
    resume_magnetometer_mode_ = mode;
    return true;
  }
  if (!SetActiveMagnetometerMode(mode)) {
    return false;
  }
  config_.magnetometer_mode = mode;
  resume_magnetometer_mode_ = mode;
  return true;
}

bool Icm20948::SetDataReadyInterrupt(bool enable) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_) {
    return false;
  }

  const bool result =
      UpdateRegister(Cmd::kRwInterruptEnable1, 0x01, enable ? 0x01 : 0x00);
  if (result) {
    config_.data_ready_interrupt_enabled = enable;
  }
  return result;
}

bool Icm20948::GetDataReady(bool& ready) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || sleeping_) {
    return false;
  }

  uint8_t status = 0;
  if (!ReadRegister(Cmd::kRoInterruptStatus1, &status)) {
    return false;
  }
  ready = (status & 0x01) != 0;
  return true;
}

bool Icm20948::ReadRawData(RawData& data) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || sleeping_) {
    return false;
  }

  const bool read_magnetometer =
      active_magnetometer_mode_ != MagnetometerMode::kPowerDown;
  if (read_magnetometer &&
      (!magnetometer_stream_ready_ || !CheckMagnetometerStreamHealth())) {
    return false;
  }

  // 0x2D 至 0x3A 为主传感器数据，启用磁力计时继续读取至 0x43。
  uint8_t buffer[23] = {0};
  const size_t read_length = read_magnetometer ? sizeof(buffer) : 14;
  if (!ReadRegister(Cmd::kRoAccelXoutH, buffer, read_length)) {
    return false;
  }

  data.acceleration.x = DecodeBigEndian(&buffer[0]);
  data.acceleration.y = DecodeBigEndian(&buffer[2]);
  data.acceleration.z = DecodeBigEndian(&buffer[4]);
  data.angular_velocity.x = DecodeBigEndian(&buffer[6]);
  data.angular_velocity.y = DecodeBigEndian(&buffer[8]);
  data.angular_velocity.z = DecodeBigEndian(&buffer[10]);
  data.temperature = DecodeBigEndian(&buffer[12]);

  if (!read_magnetometer) {
    data.magnetic_field = {};
    data.magnetometer_data_ready = false;
    data.magnetometer_data_overrun = false;
    data.magnetometer_overflow = false;
    return true;
  }

  data.magnetometer_data_ready = (buffer[14] & 0x01) != 0;
  data.magnetometer_data_overrun = (buffer[14] & 0x02) != 0;
  data.magnetic_field.x = DecodeLittleEndian(&buffer[15]);
  data.magnetic_field.y = DecodeLittleEndian(&buffer[17]);
  data.magnetic_field.z = DecodeLittleEndian(&buffer[19]);
  data.magnetometer_overflow = (buffer[22] & 0x08) != 0;
  return true;
}

bool Icm20948::ReadData(SensorData& data) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  RawData raw;
  if (!ReadRawData(raw)) {
    return false;
  }

  const float accel_sensitivity = GetAccelSensitivity(config_.accel_range);
  const float gyro_sensitivity = GetGyroSensitivity(config_.gyro_range);

  data.acceleration_g.x = raw.acceleration.x / accel_sensitivity;
  data.acceleration_g.y = raw.acceleration.y / accel_sensitivity;
  data.acceleration_g.z = raw.acceleration.z / accel_sensitivity;
  data.angular_velocity_dps.x = raw.angular_velocity.x / gyro_sensitivity;
  data.angular_velocity_dps.y = raw.angular_velocity.y / gyro_sensitivity;
  data.angular_velocity_dps.z = raw.angular_velocity.z / gyro_sensitivity;
  data.temperature_celsius =
      raw.temperature / kTemperatureSensitivity + kTemperatureOffsetCelsius;
  data.magnetic_field_ut.x = raw.magnetic_field.x * kMagnetometerSensitivityUt;
  data.magnetic_field_ut.y = raw.magnetic_field.y * kMagnetometerSensitivityUt;
  data.magnetic_field_ut.z = raw.magnetic_field.z * kMagnetometerSensitivityUt;
  data.magnetometer_data_ready = raw.magnetometer_data_ready;
  data.magnetometer_data_overrun = raw.magnetometer_data_overrun;
  data.magnetometer_overflow = raw.magnetometer_overflow;
  return true;
}

bool Icm20948::ReadAcceleration(Vector3& acceleration_g) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || sleeping_) {
    return false;
  }

  uint8_t buffer[6] = {0};
  if (!ReadRegister(Cmd::kRoAccelXoutH, buffer, sizeof(buffer))) {
    return false;
  }

  const float sensitivity = GetAccelSensitivity(config_.accel_range);
  acceleration_g.x = DecodeBigEndian(&buffer[0]) / sensitivity;
  acceleration_g.y = DecodeBigEndian(&buffer[2]) / sensitivity;
  acceleration_g.z = DecodeBigEndian(&buffer[4]) / sensitivity;
  return true;
}

bool Icm20948::ReadAngularVelocity(Vector3& angular_velocity_dps) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || sleeping_) {
    return false;
  }

  uint8_t buffer[6] = {0};
  if (!ReadRegister(Cmd::kRoGyroXoutH, buffer, sizeof(buffer))) {
    return false;
  }

  const float sensitivity = GetGyroSensitivity(config_.gyro_range);
  angular_velocity_dps.x = DecodeBigEndian(&buffer[0]) / sensitivity;
  angular_velocity_dps.y = DecodeBigEndian(&buffer[2]) / sensitivity;
  angular_velocity_dps.z = DecodeBigEndian(&buffer[4]) / sensitivity;
  return true;
}

bool Icm20948::ReadTemperature(float& temperature_celsius) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || sleeping_) {
    return false;
  }

  uint8_t buffer[2] = {0};
  if (!ReadRegister(Cmd::kRoTemperatureOutH, buffer, sizeof(buffer))) {
    return false;
  }

  temperature_celsius = DecodeBigEndian(buffer) / kTemperatureSensitivity +
                        kTemperatureOffsetCelsius;
  return true;
}

bool Icm20948::ReadMagnetometer(Vector3& magnetic_field_ut, bool& data_ready,
    bool& data_overrun, bool& overflow) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (!initialized_ || sleeping_ ||
      active_magnetometer_mode_ == MagnetometerMode::kPowerDown ||
      !magnetometer_stream_ready_ || !CheckMagnetometerStreamHealth()) {
    return false;
  }

  uint8_t buffer[9] = {0};
  if (!ReadRegister(Cmd::kRoExternalSensorData00, buffer, sizeof(buffer))) {
    return false;
  }

  data_ready = (buffer[0] & 0x01) != 0;
  data_overrun = (buffer[0] & 0x02) != 0;
  overflow = (buffer[8] & 0x08) != 0;
  magnetic_field_ut.x =
      DecodeLittleEndian(&buffer[1]) * kMagnetometerSensitivityUt;
  magnetic_field_ut.y =
      DecodeLittleEndian(&buffer[3]) * kMagnetometerSensitivityUt;
  magnetic_field_ut.z =
      DecodeLittleEndian(&buffer[5]) * kMagnetometerSensitivityUt;
  return true;
}

bool Icm20948::initialized() const {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  return initialized_;
}

bool Icm20948::sleeping() const {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  return sleeping_;
}

Icm20948::Config Icm20948::config() const {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  return config_;
}

bool Icm20948::InitBus(int32_t freq_hz) {
  if (bus_initialized_) {
    return true;
  }

  if (freq_hz == kDefaultValue) {
    freq_hz = UsesI2c() ? kDefaultIcmI2cFreqHz : kDefaultIcmSpiFreqHz;
  }

  if (UsesI2c() && freq_hz > kDefaultIcmI2cFreqHz) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "ICM20948 I2C frequency limited to %d Hz\n", kDefaultIcmI2cFreqHz);
    freq_hz = kDefaultIcmI2cFreqHz;
  } else if (!UsesI2c() && freq_hz > kDefaultIcmSpiFreqHz) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "ICM20948 SPI frequency limited to %d Hz\n", kDefaultIcmSpiFreqHz);
    freq_hz = kDefaultIcmSpiFreqHz;
  }

  bool result = false;
  if (UsesI2c()) {
    result = i2c_bus_->Init(freq_hz, i2c_address_);
    if (result) {
      result = i2c_bus_->Probe(i2c_address_);
      if (!result) {
        i2c_bus_->Deinit(true);
      }
    }
  } else {
    result = spi_bus_->Init(freq_hz, spi_cs_);
  }

  bus_initialized_ = result;
  selected_bank_ = Bank::kInvalid;
  if (!result) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "InitBus failed\n");
  }
  return result;
}

bool Icm20948::ResetDevice() {
  selected_bank_ = Bank::kInvalid;
  if (!WriteRegister(Cmd::kRwPowerManagement1, 0x80)) {
    return false;
  }
  DelayMs(kResetDelayMs);
  selected_bank_ = Bank::kInvalid;
  active_magnetometer_mode_ = MagnetometerMode::kPowerDown;
  sleeping_ = true;
  initialized_ = false;
  auxiliary_i2c_master_enabled_ = false;
  magnetometer_stream_ready_ = false;
  return true;
}

bool Icm20948::SetCoreSleep(bool sleep) {
  if (sleep) {
    if (!UpdateRegister(Cmd::kRwPowerManagement1, 0x00, 0x40)) {
      return false;
    }
    // 官方流程在设置 SLEEP 后至少等待 100 us，1 ms 同时兼容当前延时接口。
    DelayMs(1);
    sleeping_ = true;
    return true;
  }

  if (!UpdateRegister(Cmd::kRwPowerManagement1, 0x47, 0x01)) {
    return false;
  }
  sleeping_ = false;
  DelayMs(kGyroscopeStartDelayMs);

  // SPI 模式唤醒后重新确认 I2C 从接口关闭，避免异常复位后接口冲突。
  if (!UsesI2c() && !UpdateRegister(Cmd::kRwUserCtrl, 0x00, 0x10)) {
    return false;
  }
  return true;
}

void Icm20948::EnterSafeStateAfterInitializationFailure() {
  initialized_ = false;
  if (!bus_initialized_) {
    return;
  }

  if (!sleeping_) {
    if (auxiliary_i2c_master_enabled_ &&
        active_magnetometer_mode_ != MagnetometerMode::kPowerDown &&
        !SetActiveMagnetometerMode(MagnetometerMode::kPowerDown)) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "Failed to power down AK09916 after initialization error\n");
    }
    if (!SetCoreSleep(true)) {
      LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
          "Failed to enter ICM20948 sleep after initialization error\n");
    }
  }
}

bool Icm20948::ConfigureDevice(const Config& config) {
  const uint8_t power_management_1 =
      static_cast<uint8_t>((config.temperature_enabled ? 0x00 : 0x08) | 0x01);
  uint8_t power_management_2 = 0;
  if (!config.accelerometer_enabled) {
    power_management_2 |= 0x38;
  }
  if (!config.gyroscope_enabled) {
    power_management_2 |= 0x07;
  }

  if (!WriteRegister(Cmd::kRwPowerManagement1, power_management_1)) {
    return false;
  }
  sleeping_ = false;
  DelayMs(kGyroscopeStartDelayMs);
  // 辅助 I2C 必须保持周期调度，SLV0/SLV4 事务才会按内部 ODR 执行。
  if (!WriteRegister(Cmd::kRwPowerManagement2, power_management_2) ||
      !WriteRegister(Cmd::kRwLpConfig, 0x40)) {
    return false;
  }

  const uint8_t gyro_config =
      static_cast<uint8_t>((static_cast<uint8_t>(config.gyro_dlpf) << 3) |
                           (static_cast<uint8_t>(config.gyro_range) << 1) |
                           static_cast<uint8_t>(config.gyro_dlpf_enabled));
  const uint8_t accel_config =
      static_cast<uint8_t>((static_cast<uint8_t>(config.accel_dlpf) << 3) |
                           (static_cast<uint8_t>(config.accel_range) << 1) |
                           static_cast<uint8_t>(config.accel_dlpf_enabled));

  if (!WriteRegister(Cmd::kRwGyroConfig1, gyro_config) ||
      !WriteRegister(Cmd::kRwAccelConfig, accel_config) ||
      !WriteRegister(
          Cmd::kRwGyroSampleRateDivider, config.gyro_sample_rate_divider) ||
      !WriteRegister(Cmd::kRwAccelSampleRateDividerHigh,
          static_cast<uint8_t>(
              (config.accel_sample_rate_divider >> 8) & 0x0F)) ||
      !WriteRegister(Cmd::kRwAccelSampleRateDividerLow,
          static_cast<uint8_t>(config.accel_sample_rate_divider)) ||
      !WriteRegister(Cmd::kRwTemperatureConfig,
          static_cast<uint8_t>(config.temperature_dlpf)) ||
      !WriteRegister(Cmd::kRwOdrAlignEnable, 0x01) ||
      !WriteRegister(Cmd::kRwInterruptEnable1,
          config.data_ready_interrupt_enabled ? 0x01 : 0x00) ||
      !EnableAuxiliaryI2cMaster() ||
      !ConfigureMagnetometer(config.magnetometer_mode)) {
    return false;
  }

  config_ = config;
  resume_magnetometer_mode_ = config.magnetometer_mode;
  sleeping_ = false;
  return true;
}

bool Icm20948::ConfigureHostInterface() {
  // SPI 模式必须在启动等待结束后立即关闭主接口的 I2C 从机功能。
  const uint8_t user_ctrl = UsesI2c() ? 0x00 : 0x10;
  if (!WriteRegister(Cmd::kRwUserCtrl, user_ctrl)) {
    return false;
  }
  return SetCoreSleep(false);
}

bool Icm20948::ConfigureMagnetometer(MagnetometerMode mode) {
  if (!IsValidMagnetometerMode(mode)) {
    return false;
  }

  if (!WriteAk09916Register(Ak09916Cmd::kWoControl3, 0x01)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Reset AK09916 failed\n");
    return false;
  }
  DelayMs(kMagnetometerResetDelayMs);
  active_magnetometer_mode_ = MagnetometerMode::kPowerDown;
  magnetometer_stream_ready_ = false;

  uint8_t device_id = 0;
  if (!ReadAk09916Register(Ak09916Cmd::kRoDeviceId, device_id, false) ||
      device_id != kAk09916DeviceId) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "AK09916 device id mismatch (read: %#X, expected: %#X)\n", device_id,
        kAk09916DeviceId);
    return false;
  }

  return SetActiveMagnetometerMode(mode);
}

bool Icm20948::ConfigureMagnetometerStream(MagnetometerMode mode) {
  if (mode == MagnetometerMode::kPowerDown) {
    const bool result = WriteRegister(Cmd::kRwI2cSlave0Ctrl, 0x00) &&
                        WriteRegister(Cmd::kRwI2cMasterDelayCtrl, 0x00);
    if (result) {
      magnetometer_stream_ready_ = false;
    }
    return result;
  }

  // 从 ST1 开始读取 9 字节，确保每次都以 ST2 结束本次磁场数据读取。
  const bool result = WriteRegister(Cmd::kRwI2cMasterDelayCtrl, 0x00) &&
                      WriteRegister(Cmd::kRwI2cSlave0Address,
                          static_cast<uint8_t>(kAk09916Address | 0x80)) &&
                      WriteRegister(Cmd::kRwI2cSlave0Register,
                          static_cast<uint8_t>(Ak09916Cmd::kRoStatus1)) &&
                      WriteRegister(Cmd::kRwI2cSlave0Ctrl, 0x89);
  magnetometer_stream_ready_ = result;
  return result;
}

bool Icm20948::SetActiveMagnetometerMode(MagnetometerMode mode) {
  if (!IsValidMagnetometerMode(mode)) {
    return false;
  }

  if (mode == active_magnetometer_mode_ &&
      ((mode == MagnetometerMode::kPowerDown && !magnetometer_stream_ready_) ||
          (mode != MagnetometerMode::kPowerDown &&
              magnetometer_stream_ready_))) {
    return true;
  }

  // AK09916 在不同连续测量模式间切换前先进入 Power-down。
  if (active_magnetometer_mode_ != MagnetometerMode::kPowerDown &&
      (active_magnetometer_mode_ != mode || !magnetometer_stream_ready_)) {
    if (!WriteAk09916Register(Ak09916Cmd::kRwControl2,
            static_cast<uint8_t>(MagnetometerMode::kPowerDown))) {
      return false;
    }
    DelayMs(kMagnetometerModeDelayMs);
    active_magnetometer_mode_ = MagnetometerMode::kPowerDown;
    magnetometer_stream_ready_ = false;
  }

  if (mode != active_magnetometer_mode_) {
    if (!WriteAk09916Register(
            Ak09916Cmd::kRwControl2, static_cast<uint8_t>(mode))) {
      return false;
    }
    DelayMs(kMagnetometerModeDelayMs);
    active_magnetometer_mode_ = mode;
    magnetometer_stream_ready_ = false;
  }

  if (!ConfigureMagnetometerStream(mode)) {
    return false;
  }
  return true;
}

bool Icm20948::EnableAuxiliaryI2cMaster() {
  const uint8_t user_ctrl_set =
      static_cast<uint8_t>(0x20 | (UsesI2c() ? 0x00 : 0x10));
  if (!UpdateRegister(Cmd::kRwUserCtrl, 0x30, user_ctrl_set)) {
    auxiliary_i2c_master_enabled_ = false;
    return false;
  }
  // 手册推荐 I2C_MST_CLK=7，标称约 345.6 kHz，避免超过从设备上限。
  if (!WriteRegister(Cmd::kRwI2cMasterCtrl, 0x07)) {
    auxiliary_i2c_master_enabled_ = false;
    return false;
  }
  // 无加速度计和陀螺仪时，辅助 I2C 约以 68.75 Hz 运行。
  auxiliary_i2c_master_enabled_ =
      WriteRegister(Cmd::kRwI2cMasterOdrConfig, 0x04);
  return auxiliary_i2c_master_enabled_;
}

bool Icm20948::WaitForAuxiliaryTransaction() {
  const int64_t start_time_ms = GetSystemTimeMs();
  const uint32_t timeout_ms = GetAuxiliaryTransactionTimeoutMs();
  while (GetSystemTimeMs() - start_time_ms < timeout_ms) {
    // SLV4_EN 在单字节事务完成后由硬件自动清零，不依赖完成中断使能。
    uint8_t control = 0;
    if (!ReadRegister(Cmd::kRwI2cSlave4Ctrl, &control)) {
      return false;
    }
    if ((control & 0x80) == 0) {
      uint8_t status = 0;
      if (!ReadRegister(Cmd::kRoI2cMasterStatus, &status)) {
        return false;
      }
      if ((status & 0x30) != 0) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "AK09916 auxiliary transaction failed (status: %#X)\n", status);
        return false;
      }
      return true;
    }

    DelayMs(1);
  }

  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "AK09916 auxiliary transaction timeout\n");
  return false;
}

uint32_t Icm20948::GetAuxiliaryTransactionTimeoutMs() const {
  uint32_t sample_period_ms = 0;
  if (config_.gyroscope_enabled) {
    const uint32_t numerator =
        1000U * (static_cast<uint32_t>(config_.gyro_sample_rate_divider) + 1U);
    sample_period_ms = (numerator + 1099U) / 1100U;
  } else if (config_.accelerometer_enabled) {
    const uint32_t numerator =
        1000U * (static_cast<uint32_t>(config_.accel_sample_rate_divider) + 1U);
    sample_period_ms = (numerator + 1124U) / 1125U;
  } else {
    // I2C_MST_ODR_CONFIG=4 时为 1100/16=68.75 Hz。
    sample_period_ms = 15;
  }

  const uint32_t calculated_timeout =
      sample_period_ms + kAuxiliaryTransactionTimeoutMarginMs;
  return calculated_timeout < kMinimumAuxiliaryTransactionTimeoutMs
             ? kMinimumAuxiliaryTransactionTimeoutMs
             : calculated_timeout;
}

bool Icm20948::CheckMagnetometerStreamHealth() {
  uint8_t status = 0;
  if (!ReadRegister(Cmd::kRoI2cMasterStatus, &status)) {
    return false;
  }
  if ((status & 0x21) == 0) {
    return true;
  }

  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "AK09916 stream failed (status: %#X)\n", status);
  return false;
}

bool Icm20948::ReadAk09916Register(
    Ak09916Cmd cmd, uint8_t& data, bool restore_stream) {
  bool result = WriteRegister(Cmd::kRwI2cSlave0Ctrl, 0x00);
  if (result) {
    magnetometer_stream_ready_ = false;
  }

  uint8_t ignored_status = 0;
  if (result) {
    result =
        ReadRegister(Cmd::kRoI2cMasterStatus, &ignored_status) &&
        WriteRegister(Cmd::kRwI2cSlave4Address,
            static_cast<uint8_t>(kAk09916Address | 0x80)) &&
        WriteRegister(Cmd::kRwI2cSlave4Register, static_cast<uint8_t>(cmd)) &&
        WriteRegister(Cmd::kRwI2cSlave4Ctrl, 0x80) &&
        WaitForAuxiliaryTransaction() &&
        ReadRegister(Cmd::kRoI2cSlave4DataIn, &data);
  }

  if (restore_stream &&
      active_magnetometer_mode_ != MagnetometerMode::kPowerDown) {
    const bool restore_result =
        ConfigureMagnetometerStream(active_magnetometer_mode_);
    result = result && restore_result;
  }
  return result;
}

bool Icm20948::WriteAk09916Register(Ak09916Cmd cmd, uint8_t data) {
  bool result = WriteRegister(Cmd::kRwI2cSlave0Ctrl, 0x00);
  if (result) {
    magnetometer_stream_ready_ = false;
  }

  uint8_t ignored_status = 0;
  if (!result) {
    return false;
  }
  return ReadRegister(Cmd::kRoI2cMasterStatus, &ignored_status) &&
         WriteRegister(Cmd::kRwI2cSlave4Address, kAk09916Address) &&
         WriteRegister(Cmd::kRwI2cSlave4Register, static_cast<uint8_t>(cmd)) &&
         WriteRegister(Cmd::kRwI2cSlave4DataOut, data) &&
         WriteRegister(Cmd::kRwI2cSlave4Ctrl, 0x80) &&
         WaitForAuxiliaryTransaction();
}

bool Icm20948::SelectBank(Bank bank) {
  if (bank == Bank::kInvalid) {
    return false;
  }
  if (selected_bank_ == bank) {
    return true;
  }

  const uint8_t value = static_cast<uint8_t>(static_cast<uint8_t>(bank) << 4);
  if (!WriteTransport(0x7F, &value, 1)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Select bank failed (bank: %u)\n", static_cast<unsigned int>(bank));
    selected_bank_ = Bank::kInvalid;
    return false;
  }
  selected_bank_ = bank;
  return true;
}

bool Icm20948::ReadRegister(Cmd cmd, uint8_t* data, size_t length) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if ((data == nullptr && length != 0) || !bus_initialized_) {
    return false;
  }
  return SelectBank(GetBank(cmd)) &&
         ReadTransport(GetRegisterAddress(cmd), data, length);
}

bool Icm20948::WriteRegister(Cmd cmd, uint8_t data) {
  return WriteRegister(cmd, &data, 1);
}

bool Icm20948::WriteRegister(Cmd cmd, const uint8_t* data, size_t length) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if ((data == nullptr && length != 0) || !bus_initialized_) {
    return false;
  }
  return SelectBank(GetBank(cmd)) &&
         WriteTransport(GetRegisterAddress(cmd), data, length);
}

bool Icm20948::UpdateRegister(Cmd cmd, uint8_t clear_mask, uint8_t set_mask) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  uint8_t value = 0;
  if (!ReadRegister(cmd, &value)) {
    return false;
  }
  value = static_cast<uint8_t>((value & ~clear_mask) | set_mask);
  return WriteRegister(cmd, value);
}

bool Icm20948::ReadTransport(uint8_t reg, uint8_t* data, size_t length) {
  if (UsesI2c()) {
    return i2c_bus_->Read(reg, data, length);
  }
  return spi_bus_->Read(static_cast<uint8_t>(reg | 0x80), data, length);
}

bool Icm20948::WriteTransport(uint8_t reg, const uint8_t* data, size_t length) {
  if (UsesI2c()) {
    return i2c_bus_->Write(reg, data, length);
  }
  return spi_bus_->Write(static_cast<uint8_t>(reg & 0x7F), data, length);
}

Icm20948::Bank Icm20948::GetBank(Cmd cmd) {
  return static_cast<Bank>((static_cast<uint16_t>(cmd) >> 8) & 0x03);
}

uint8_t Icm20948::GetRegisterAddress(Cmd cmd) {
  return static_cast<uint8_t>(static_cast<uint16_t>(cmd));
}

int16_t Icm20948::DecodeBigEndian(const uint8_t* data) {
  return static_cast<int16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

int16_t Icm20948::DecodeLittleEndian(const uint8_t* data) {
  return static_cast<int16_t>((static_cast<uint16_t>(data[1]) << 8) | data[0]);
}

float Icm20948::GetAccelSensitivity(AccelRange range) {
  static constexpr float kSensitivity[] = {
      16384.0f,
      8192.0f,
      4096.0f,
      2048.0f,
  };
  return kSensitivity[static_cast<uint8_t>(range)];
}

float Icm20948::GetGyroSensitivity(GyroRange range) {
  static constexpr float kSensitivity[] = {
      131.0f,
      65.5f,
      32.8f,
      16.4f,
  };
  return kSensitivity[static_cast<uint8_t>(range)];
}

bool Icm20948::IsValidAccelRange(AccelRange range) {
  return static_cast<uint8_t>(range) <= static_cast<uint8_t>(AccelRange::k16g);
}

bool Icm20948::IsValidGyroRange(GyroRange range) {
  return static_cast<uint8_t>(range) <=
         static_cast<uint8_t>(GyroRange::k2000Dps);
}

bool Icm20948::IsValidDlpf(Dlpf dlpf) {
  return static_cast<uint8_t>(dlpf) <= static_cast<uint8_t>(Dlpf::k7);
}

bool Icm20948::IsValidMagnetometerMode(MagnetometerMode mode) {
  switch (mode) {
    case MagnetometerMode::kPowerDown:
    case MagnetometerMode::kContinuous10Hz:
    case MagnetometerMode::kContinuous20Hz:
    case MagnetometerMode::kContinuous50Hz:
    case MagnetometerMode::kContinuous100Hz:
      return true;
    default:
      return false;
  }
}

}  // namespace cpp_bus_driver
