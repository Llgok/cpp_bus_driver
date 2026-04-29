/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-04-28 15:43:31
 * @License: GPL 3.0
 */
#include "bq27220xxxx.h"

namespace cpp_bus_driver {
bool Bq27220xxxx::Init(int32_t freq_hz) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    GpioWrite(rst_, 1);
    DelayMs(10);
    GpioWrite(rst_, 0);
    DelayMs(10);
    GpioWrite(rst_, 1);
    DelayMs(10);
  }

  if (!ChipI2cGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  auto buffer = GetDeviceId();
  if (buffer != kDeviceId) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get bq27220xxxx id failed (error id: %#X)\n", buffer);
    return false;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get bq27220xxxx id success (id: %#X)\n", buffer);
  }
  return true;
}

bool Bq27220xxxx::EnterConfigUpdate() {
  // 发送 EnterCfgUpdate 子命令 (0x0090)
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatusStart),
          static_cast<uint16_t>(ControlStatusReg::kWoEnterConfigUpdate),
          Endian::kLittle)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  // 通过轮询 OperationStatus() 寄存器直到位 2 被设置来确认 Cfgupdate
  // 模式，可能最多需要 1 秒
  uint8_t timeout_count = 0;
  while (1) {
    OperationStatus os;
    if (GetOperationStatus(os)) {
      if (os.flag.config_update) {
        break;
      }
    }

    timeout_count++;
    if (timeout_count > 100) {
      LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "EnterConfigUpdate timeout\n");
      return false;
    }
    DelayMs(10);
  }

  return true;
}

bool Bq27220xxxx::ExitConfigUpdate() {
  // 通过发送 EXIT_CFG_UPDATE_REINIT (0x0091) 或 EXIT_CFG_UPDATE (0x0092)
  // 命令退出 Cfgupdate 模式
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatusStart),
          static_cast<uint16_t>(ControlStatusReg::kWoExitConfigUpdate),
          Endian::kLittle)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  // 通过轮询 OperationStatus() 寄存器直到位 2 被清除来确认 Cfgupdate 模式
  uint8_t timeout_count = 0;
  while (1) {
    OperationStatus os;
    if (GetOperationStatus(os)) {
      if (!os.flag.config_update) {
        break;
      }
    }

    timeout_count++;
    if (timeout_count > 100) {
      LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "ExitConfigUpdate timeout\n");
      return false;
    }
    DelayMs(10);
  }

  return true;
}

uint16_t Bq27220xxxx::GetDeviceId() {
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwControlStatusStart),
          static_cast<uint16_t>(ControlStatusReg::kRoDeviceId),
          Endian::kLittle)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return -1;
  }
  DelayMs(20);  // 必须有延时

  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwMacDataStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetDesignCapacity() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoDesignCapacityStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetVoltage() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoVoltageStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

int16_t Bq27220xxxx::GetCurrent() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoCurrentStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetRemainingCapacity() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoRemainingCapacityStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetFullChargeCapacity() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoFullChargeCapacityStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
}

int16_t Bq27220xxxx::GetAtRate() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwAtRateStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
}

bool Bq27220xxxx::SetAtRate(int16_t rate) {
  // 小端发送
  const uint8_t buffer[2] = {
      static_cast<uint8_t>(rate),
      static_cast<uint8_t>(rate >> 8),
  };

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwAtRateStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint16_t Bq27220xxxx::GetAtRateTimeToEmpty() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoAtRateTimeToEmptyStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

float Bq27220xxxx::GetTemperatureKelvin() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTemperatureStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1;
}

float Bq27220xxxx::GetTemperatureCelsius() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTemperatureStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1 - 273.15;
}

bool Bq27220xxxx::SetTemperatureMode(TemperatureMode mode) {
  if (!EnterConfigUpdate()) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "EnterConfigUpdate failed\n");
    return false;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRamRegister),
          static_cast<uint16_t>(ConfigurationReg::kRwOperationConfigA),
          Endian::kLittle)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRwMacDataStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  switch (mode) {
    case TemperatureMode::kInternal:
      buffer[0] &= 0B01111110;
      break;
    case TemperatureMode::kExternalNtc:
      buffer[0] = (buffer[0] | 0B10000000) & 0B11111110;
      break;

    default:
      break;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwMacDataStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  uint8_t buffer_new_chksum =
      0xFF -
      (0xFF &
          (static_cast<uint8_t>(
               static_cast<uint16_t>(ConfigurationReg::kRwOperationConfigA) >>
               8) +
              static_cast<uint8_t>(ConfigurationReg::kRwOperationConfigA) +
              buffer[0] + buffer[1]));

  uint8_t buffer_data_len = 6;

  // 写入新校验和
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRoMacDataSumStart), buffer_new_chksum)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 写入块长度，当整个块的正确校验和以及长度被写入时，数据实际上被传输到 kRam
  // 中
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRoMacDataLenStart), buffer_data_len)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  if (!ExitConfigUpdate()) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "ExitConfigUpdate failed\n");
    return false;
  }

  return true;
}

bool Bq27220xxxx::GetBatteryStatus(BatteryStatus& status) {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoBatteryStatusStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  status.flag.fd = (buffer[1] & 0B10000000) >> 7;
  status.flag.ocvcomp = (buffer[1] & 0B01000000) >> 6;
  status.flag.ocvfail = (buffer[1] & 0B00100000) >> 5;
  status.flag.sleep = (buffer[1] & 0B00010000) >> 4;
  status.flag.otc = (buffer[1] & 0B00001000) >> 3;
  status.flag.otd = (buffer[1] & 0B00000100) >> 2;
  status.flag.fc = (buffer[1] & 0B00000010) >> 1;
  status.flag.chginh = buffer[1] & 0B00000001;
  status.flag.tca = (buffer[0] & 0B01000000) >> 6;
  status.flag.ocvgd = (buffer[0] & 0B00100000) >> 5;
  status.flag.auth_gd = (buffer[0] & 0B00010000) >> 4;
  status.flag.battpres = (buffer[0] & 0B00001000) >> 3;
  status.flag.tda = (buffer[0] & 0B00000100) >> 2;
  status.flag.sysdwn = (buffer[0] & 0B00000010) >> 1;
  status.flag.dsg = buffer[0] & 0B00000001;

  return true;
}

bool Bq27220xxxx::GetOperationStatus(OperationStatus& status) {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoOperationStatusStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return false;
  }

  status.sec = (buffer[0] & 0B00000110) >> 1;

  status.flag.config_update = (buffer[1] & 0B00000100) >> 2;
  status.flag.btp_int = (buffer[0] & 0B10000000) >> 7;
  status.flag.smth = (buffer[0] & 0B01000000) >> 6;
  status.flag.init_comp = (buffer[0] & 0B00100000) >> 5;
  status.flag.vdq = (buffer[0] & 0B00010000) >> 4;
  status.flag.edv2 = (buffer[0] & 0B00001000) >> 3;
  status.flag.calmd = buffer[0] & 0B00000001;

  return true;
}

bool Bq27220xxxx::SetDesignCapacity(uint16_t capacity) {
  if (!EnterConfigUpdate()) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "EnterConfigUpdate failed\n");
    return false;
  }

  // 将 0x929F 写入 0x3E 以访问 Design Capacity
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRamRegister),
          static_cast<uint16_t>(GasGaugingReg::kWoDesignCapacity),
          Endian::kLittle)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  // 从 0x40 开始读取写入两个 Design Capacity 字节，也就是 capacity 值
  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwMacDataStart), capacity)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 计算新校验和，校验和为 (255 – x)，其中 x 是逐字节的 BlockData() 8
  // 位总和（0x40 至 0x5F）
  // 计算新校验和的一种快速方法是使用新旧数据总和字节的数据替换方法，请参阅所示方法的代码
  // uint8_t buffer_new_chksum = 255 - ((((255 - buffer_old_chksum -
  // buffer_old_dc_msb - buffer_old_dc_lsb) % 256) +
  //                                     static_cast<uint8_t>(capacity >> 8) +
  //                                     static_cast<uint8_t>(capacity)) %
  //                                    256)-23;
  uint8_t buffer_new_chksum =
      0xFF -
      (0xFF &
          (static_cast<uint8_t>(
               static_cast<uint16_t>(GasGaugingReg::kWoDesignCapacity) >> 8) +
              static_cast<uint8_t>(GasGaugingReg::kWoDesignCapacity) +
              static_cast<uint8_t>(capacity >> 8) +
              static_cast<uint8_t>(capacity)));

  uint8_t buffer_data_len = 6;
  // buffer_new_chksum=0xB0;

  // 写入新校验和
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRoMacDataSumStart), buffer_new_chksum)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 写入块长度，当整个块的正确校验和以及长度被写入时，数据实际上被传输到 kRam
  // 中
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRoMacDataLenStart), buffer_data_len)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  if (!ExitConfigUpdate()) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "ExitConfigUpdate failed\n");
    return false;
  }

  return true;
}

uint16_t Bq27220xxxx::GetTimeToEmpty() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTimeToEmptyStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetTimeToFull() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoTimeToFullStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

int16_t Bq27220xxxx::GetStandbyCurrent() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoStandbyCurrentStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetStandbyTimeToEmpty() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoStandbyTimeToEmptyStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

int16_t Bq27220xxxx::GetMaxLoadCurrent() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoMaxLoadCurrentStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetMaxLoadTimeToEmpty() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoMaxLoadTimeToEmptyStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetRawCoulombCount() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoRawCoulombCountStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

int16_t Bq27220xxxx::GetAveragePower() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoAveragePowerStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
}

float Bq27220xxxx::GetChipTemperatureKelvin() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoInternalTemperatureStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1;
}

float Bq27220xxxx::GetChipTemperatureCelsius() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoInternalTemperatureStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1 - 273.15;
}

uint16_t Bq27220xxxx::GetCycleCount() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(static_cast<uint8_t>(Cmd::kRoCycleCountStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetStatusOfCharge() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoStatusOfChargeStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetStatusOfHealth() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoStatusOfHealthStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetChargingVoltage() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoChargingVoltageStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

uint16_t Bq27220xxxx::GetChargingCurrent() {
  uint8_t buffer[2] = {0};

  if (!bus_->Read(
          static_cast<uint8_t>(Cmd::kRoChargingCurrentStart), buffer, 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return -1;
  }

  return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
}

bool Bq27220xxxx::SetSleepCurrentThreshold(uint16_t threshold) {
  if (threshold > 100) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    threshold = 100;
  }

  if (!EnterConfigUpdate()) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "EnterConfigUpdate failed\n");
    return false;
  }

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwRamRegister),
          static_cast<uint16_t>(ConfigurationReg::kRwSleepCurrent),
          Endian::kLittle)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  if (!bus_->Write(static_cast<uint8_t>(Cmd::kRwMacDataStart), threshold)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  uint8_t buffer_new_chksum =
      0xFF -
      (0xFF &
          (static_cast<uint8_t>(
               static_cast<uint16_t>(ConfigurationReg::kRwSleepCurrent) >> 8) +
              static_cast<uint8_t>(ConfigurationReg::kRwSleepCurrent) +
              static_cast<uint8_t>(threshold >> 8) +
              static_cast<uint8_t>(threshold)));

  uint8_t buffer_data_len = 6;

  // 写入新校验和
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRoMacDataSumStart), buffer_new_chksum)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // 写入块长度，当整个块的正确校验和以及长度被写入时，数据实际上被传输到 kRam
  // 中
  if (!bus_->Write(
          static_cast<uint8_t>(Cmd::kRoMacDataLenStart), buffer_data_len)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  DelayMs(10);  // 必须有延时

  if (!ExitConfigUpdate()) {
    LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "ExitConfigUpdate failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
