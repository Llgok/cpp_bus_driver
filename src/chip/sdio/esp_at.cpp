/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2026-04-24 10:47:02
 * @License: GPL 3.0
 */
#include "esp_at.h"

namespace cpp_bus_driver {
bool EspAt::Init(int32_t freq_hz) {
  connect_.status = true;
  connect_.error_count = 0;
  connect_.receive_total_length_index = 0;

  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    SetPinMode(rst_, PinMode::kOutput, PinStatus::kPullup);

    PinWrite(rst_, 1);
    DelayMs(50);
    PinWrite(rst_, 0);
    DelayMs(50);
    PinWrite(rst_, 1);
    DelayMs(1000);
  } else if (rst_callback_ != nullptr) {
    rst_callback_(1);
    DelayMs(50);
    rst_callback_(0);
    DelayMs(50);
    rst_callback_(1);
    DelayMs(1000);
  }

  if (!ChipSdioGuide::Init(freq_hz)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!InitSequence()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitSequence failed\n");
    return false;
  }

  if (!InitConnect()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "InitConnect failed\n");
    return false;
  }

  if (!GetDeviceId()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Get espat id failed\n");
    return false;
  } else {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Get espat id success\n");
  }

  return true;
}

bool EspAt::SetSleep(SleepMode mode, int16_t timeout_ms) {
  if (mode == SleepMode::kPowerDown) {
    if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
      PinWrite(rst_, 0);
    } else if (rst_callback_ != nullptr) {
      rst_callback_(0);
    } else {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetSleep failed\n");
      return false;
    }

    connect_.status = false;
    return true;
  }

  const char* buffer = nullptr;

  switch (mode) {
    case SleepMode::kDisableSleep:
      buffer = "AT+SLEEP=0\r\n";
      break;
    case SleepMode::kModemSleep:
      buffer = "AT+SLEEP=1\r\n";
      break;
    case SleepMode::kLightSleep:
      buffer = "AT+SLEEP=2\r\n";
      break;
    case SleepMode::kModemSleepListenInterval:
      buffer = "AT+SLEEP=3\r\n";
      break;

    default:
      break;
  }

  SendPacket(buffer, strlen(buffer));

  int16_t buffer_timeout_count = 0;

  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          break;
        }

        buffer_cmd = "\r\nERROR\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetSleep error\n");
          return false;
        }

        buffer_cmd = "\r\nbusy p...\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetSleep busy\n");
          buffer_timeout_count = 0;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_ms / 10) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetSleep timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::SetDeepSleep(uint32_t sleep_time_ms, int16_t timeout_ms) {
  if (sleep_time_ms > ((1U << 31) - 1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  char at_cmd[30];
  snprintf(at_cmd, sizeof(at_cmd), "AT+GSLP=%ld\r\n", sleep_time_ms);
  const char* buffer = at_cmd;

  SendPacket(buffer, strlen(buffer));

  int16_t buffer_timeout_count = 0;

  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          break;
        }

        buffer_cmd = "\r\nERROR\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(
              LogLevel::kChip, __FILE__, __LINE__, "SetDeepSleep error\n");
          return false;
        }

        buffer_cmd = "\r\nbusy p...\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(
              LogLevel::kChip, __FILE__, __LINE__, "SetDeepSleep busy\n");
          buffer_timeout_count = 0;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_ms / 10) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetDeepSleep timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::InitSequence() {
  // enable function 1
  if (!bus_->Write(0, static_cast<uint32_t>(Cmd::kSdIoCccrFnEnable), 6)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(0, static_cast<uint32_t>(Cmd::kSdIoCccrFnReady), 6)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  // enable interrupts for function 1&2 and master enable
  if (!bus_->Write(0, static_cast<uint32_t>(Cmd::kSdIoCccrIntEnable), 7)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(0, static_cast<uint32_t>(Cmd::kSdIoCccrBlksizel), 0)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(0, static_cast<uint32_t>(Cmd::kSdIoCccrBlksizeh), 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(0, static_cast<uint32_t>(0x110), 0)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  // Set block size 512 (0x200)
  if (!bus_->Write(0, static_cast<uint32_t>(0x111), 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  if (!bus_->Write(0, static_cast<uint32_t>(0x210), 0)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  if (!bus_->Write(0, static_cast<uint32_t>(0x210), 2)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool EspAt::InitConnect() {
  uint8_t buffer_timeout_count = 0;

  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "\r\nready\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          connect_.status = true;
          LogMessage(
              LogLevel::kChip, __FILE__, __LINE__, "InitConnect success\n");
          break;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > kTransmitTimeoutCount) {
      connect_.status = false;
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Connect timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::GetDeviceId() {
  const std::string buffer_cmd = "AT\r\n";

  SendPacket(buffer_cmd);

  int8_t buffer_timeout_count = 0;
  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Connect
          // success\n");
          break;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > kTransmitTimeoutCount) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "GetDeviceId timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::Reconnect() {
  connect_.status = true;
  connect_.error_count = 0;

  if (!GetDeviceId()) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "GetDeviceId fail, starting reinitialization\n");
    Init();

    return false;
  }

  return true;
}

bool EspAt::GetConnectStatus() { return connect_.status; }

void EspAt::SetConnectCount(int8_t count) {
  connect_.error_count += count;
  if (connect_.error_count < 0) {
    connect_.error_count = 0;
  } else if (connect_.error_count > kConnectErrorCount) {
    connect_.error_count = kConnectErrorCount + 1;
    connect_.status = false;
    LogMessage(LogLevel::kChip, __FILE__, __LINE__,
        "Connect error count > kConnectErrorCount\n");
  }
}

uint32_t EspAt::GetIrqFlag() {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint32_t buffer = 0;

  if (!bus_->Read(1, static_cast<uint32_t>(Cmd::kInterruptRaw), &buffer,
          sizeof(uint32_t))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    SetConnectCount(1);
    return -1;
  }

  SetConnectCount(-1);
  return buffer;
}

bool EspAt::ClearIrqFlag(uint32_t irq_mask) {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  if (!bus_->Write(1, static_cast<uint32_t>(Cmd::kInterruptClear), &irq_mask,
          sizeof(uint32_t))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    SetConnectCount(1);
    return false;
  }

  SetConnectCount(-1);
  return true;
}

bool EspAt::ParseRxNewPacketFlag(uint32_t flag) {
  if (flag == static_cast<uint32_t>(-1)) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!((flag & static_cast<uint32_t>(IrqFlag::kRxNewPacket)) >> 23)) {
    // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "kRxNewPacket failed\n");
    return false;
  }

  return true;
}

uint32_t EspAt::GetRxDataLength() {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint32_t buffer = 0;

  if (!bus_->Read(1, static_cast<uint32_t>(Cmd::kPacketLength), &buffer,
          sizeof(uint32_t))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    SetConnectCount(1);
    return false;
  }

  buffer &= kRxBufferMask;
  buffer = (buffer + kRxBufferMax - connect_.receive_total_length_index) %
           kRxBufferMax;

  SetConnectCount(-1);
  return buffer;
}

bool EspAt::ReceivePacket(std::vector<uint8_t>& data) {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint32_t buffer_lenght = GetRxDataLength();

  if (buffer_lenght == 0) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "GetRxDataLength failed\n");
    return false;
  }

  data.resize(buffer_lenght);

  size_t buffer_block_length = (buffer_lenght / kMaxTransmitBlockBufferSize) *
                               kMaxTransmitBlockBufferSize;
  if (buffer_block_length != 0) {
    // 多字节对齐读取
    if (!bus_->ReadBlock(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_lenght,
            data.data(), buffer_block_length)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadBlock failed\n");
      connect_.status = false;
      return false;
    }
    buffer_lenght -= buffer_block_length;

    connect_.receive_total_length_index += buffer_block_length;
  }

  if (buffer_lenght != 0) {
    // 4字节对齐读取
    if (!bus_->Read(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_lenght,
            data.data() + buffer_block_length, (buffer_lenght + 3) & (~3))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      SetConnectCount(1);
      return false;
    }

    connect_.receive_total_length_index += buffer_lenght;
  }

  SetConnectCount(-1);
  return true;
}

bool EspAt::ReceivePacket(uint8_t* data, size_t* byte) {
  if (data == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint32_t buffer_lenght = GetRxDataLength();

  if (buffer_lenght == 0) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "GetRxDataLength failed\n");
    return false;
  }

  *byte = buffer_lenght;

  size_t buffer_block_length = (buffer_lenght / kMaxTransmitBlockBufferSize) *
                               kMaxTransmitBlockBufferSize;
  if (buffer_block_length != 0) {
    // 多字节对齐读取
    if (!bus_->ReadBlock(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_lenght,
            data, buffer_block_length)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadBlock failed\n");
      *byte = 0;
      connect_.status = false;
      return false;
    }
    buffer_lenght -= buffer_block_length;

    connect_.receive_total_length_index += buffer_block_length;
  }

  if (buffer_lenght != 0) {
    // 4字节对齐读取
    if (!bus_->Read(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_lenght,
            data + buffer_block_length, (buffer_lenght + 3) & (~3))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      *byte = 0;
      SetConnectCount(1);
      return false;
    }

    connect_.receive_total_length_index += buffer_lenght;
  }

  SetConnectCount(-1);
  return true;
}

bool EspAt::ReceivePacket(std::unique_ptr<uint8_t[]>& data, size_t* byte) {
  if (byte == nullptr) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint32_t buffer_lenght = GetRxDataLength();

  if (buffer_lenght == 0) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "GetRxDataLength failed\n");
    return false;
  }

  *byte = buffer_lenght;

  data = std::make_unique<uint8_t[]>(buffer_lenght);

  size_t buffer_block_length = (buffer_lenght / kMaxTransmitBlockBufferSize) *
                               kMaxTransmitBlockBufferSize;
  if (buffer_block_length != 0) {
    // 多字节对齐读取
    if (!bus_->ReadBlock(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_lenght,
            data.get(), buffer_block_length)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReadBlock failed\n");
      *byte = 0;
      connect_.status = false;
      return false;
    }
    buffer_lenght -= buffer_block_length;

    connect_.receive_total_length_index += buffer_block_length;
  }

  if (buffer_lenght != 0) {
    // 4字节对齐读取
    if (!bus_->Read(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_lenght,
            data.get() + buffer_block_length, (buffer_lenght + 3) & (~3))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      *byte = 0;
      SetConnectCount(1);
      return false;
    }

    connect_.receive_total_length_index += buffer_lenght;
  }

  SetConnectCount(-1);
  return true;
}

uint32_t EspAt::GetTxBlockBufferLength() {
  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint32_t buffer = 0;

  if (!bus_->Read(1, static_cast<uint32_t>(Cmd::kTokenRdata), &buffer,
          sizeof(uint32_t))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    SetConnectCount(1);
    return false;
  }

  return (buffer >> kTxBufferOffset) & kTxBufferMask;
}

bool EspAt::SendPacket(const char* data, size_t byte) {
  if (data == nullptr || byte == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint16_t buffer_timeout_count = 0;

  while (1) {
    if (GetTxBlockBufferLength() * kMaxTransmitBlockBufferSize >= byte) {
      break;
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > 100)  // 超时
    {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__,
          "GetTxBlockBufferLength timeout\n");
      return false;
    }

    DelayMs(10);
  }

  size_t buffer_block_length =
      (byte / kMaxTransmitBlockBufferSize) * kMaxTransmitBlockBufferSize;
  if (buffer_block_length != 0) {
    // 多字节对齐发送
    if (!bus_->WriteBlock(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - byte, data,
            buffer_block_length)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteBlock failed\n");
      SetConnectCount(1);
      return false;
    }
    byte -= buffer_block_length;
  }

  if (byte != 0) {
    // 4字节对齐发送
    if (!bus_->Write(1, static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - byte,
            data + buffer_block_length, (byte + 3) & (~3))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      SetConnectCount(1);
      return false;
    }
  }

  SetConnectCount(-1);
  return true;
}

bool EspAt::SendPacket(const std::string data) {
  if (data.size() == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  if (!connect_.status) {
    LogMessage(LogLevel::kDebug, __FILE__, __LINE__, "Connect failed\n");
    return false;
  }

  uint16_t buffer_timeout_count = 0;
  size_t buffer_length = data.length();

  while (1) {
    if (GetTxBlockBufferLength() * kMaxTransmitBlockBufferSize >=
        buffer_length) {
      break;
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > 100)  // 超时
    {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__,
          "GetTxBlockBufferLength timeout\n");
      return false;
    }

    DelayMs(10);
  }

  size_t buffer_block_length = (buffer_length / kMaxTransmitBlockBufferSize) *
                               kMaxTransmitBlockBufferSize;
  if (buffer_block_length != 0) {
    // 多字节对齐发送
    if (!bus_->WriteBlock(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_length,
            data.data(), buffer_block_length)) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WriteBlock failed\n");
      SetConnectCount(1);
      return false;
    }
    buffer_length -= buffer_block_length;
  }

  if (buffer_length != 0) {
    // 4字节对齐发送
    if (!bus_->Write(1,
            static_cast<uint32_t>(Cmd::kSlaveCmd53EndAddr) - buffer_length,
            data.data() + buffer_block_length, (buffer_length + 3) & (~3))) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
      SetConnectCount(1);
      return false;
    }
  }

  SetConnectCount(-1);
  return true;
}

bool EspAt::SetWifiMode(WifiMode mode, int16_t timeout_ms) {
  const char* buffer = nullptr;

  switch (mode) {
    case WifiMode::kOff:
      buffer = "AT+CWMODE=0\r\n";
      break;
    case WifiMode::kStation:
      buffer = "AT+CWMODE=1\r\n";
      break;
    case WifiMode::kSoftap:
      buffer = "AT+CWMODE=2\r\n";
      break;
    case WifiMode::kStationSoftap:
      buffer = "AT+CWMODE=3\r\n";
      break;

    default:
      break;
  }

  SendPacket(buffer, strlen(buffer));

  int16_t buffer_timeout_count = 0;

  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          break;
        }

        buffer_cmd = "\r\nERROR\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(
              LogLevel::kChip, __FILE__, __LINE__, "SetWifiMode error\n");
          return false;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_ms / 10) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetWifiMode timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::WifiScan(std::vector<uint8_t>& data, int16_t timeout_ms) {
  const char* buffer = "AT+CWLAP\r\n";

  SendPacket(buffer, strlen(buffer));

  int16_t buffer_timeout_count = 0;

  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "+CWLAP:";
        size_t buffer_index = 0;
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd), &buffer_index)) {
          buffer.pop_back();  // 这里移除末尾的'\0'否则每个数据后都会有'\0'
          data.insert(data.end(),
              buffer.begin() + buffer_index + strlen(buffer_cmd), buffer.end());
        }

        buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          data.push_back('\0');  // 数据末尾加上'\0'使数据有终止符号
          break;
        }

        buffer_cmd = "\r\nERROR\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WifiScan error\n");
          return false;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_ms / 10) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WifiScan timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::WaitInterrupt(uint32_t timeout_ms) {
  if (!bus_->WaitInterrupt(timeout_ms)) {
    // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "WaitInterrupt
    // failed\n");
    return false;
  }

  return true;
}

bool EspAt::SetFlashSave(bool enable, int16_t timeout_ms) {
  std::string buffer;
  if (enable) {
    buffer = "AT+SYSSTORE=1\r\n";  // 存入flash
  } else {
    buffer = "AT+SYSSTORE=0\r\n";  // 不存入flash
  }

  SendPacket(buffer);

  int16_t buffer_timeout_count = 0;

  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          break;
        }

        buffer_cmd = "\r\nERROR\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(
              LogLevel::kChip, __FILE__, __LINE__, "SetWifiConnect error\n");
          return false;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_ms / 10) {
      LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "SetWifiConnect timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::SetWifiConnect(
    std::string ssid, std::string password, int16_t timeout_ms) {
  std::string buffer = "AT+CWJAP=\"" + ssid + "\"" + ",\"";
  if (password != "") {
    buffer = buffer + password + "\"\r\n";
  }

  SendPacket(buffer);

  int16_t buffer_timeout_count = 0;

  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          break;
        }

        buffer_cmd = "\r\nERROR\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(
              LogLevel::kChip, __FILE__, __LINE__, "SetWifiConnect error\n");
          return false;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_ms / 10) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "SetWifiMode timeout\n");
      return false;
    }

    DelayMs(10);
  }

  return true;
}

bool EspAt::GetRealTime(RealTime& time, int16_t timeout_ms) {
  const std::string buffer =
      "AT+HTTPCLIENT=1,0,\"http://httpbin.org/get\",,,1\r\n";

  SendPacket(buffer);

  int16_t buffer_timeout_count = 0;
  std::vector<uint8_t> buffer_data;
  while (1) {
    uint32_t flag = GetIrqFlag();
    if (ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      ClearIrqFlag(flag);

      buffer_timeout_count--;
      if (buffer_timeout_count < 0) {
        buffer_timeout_count = 0;
      }

      std::vector<uint8_t> buffer;
      if (ReceivePacket(buffer)) {
        // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
        buffer.push_back('\0');
        // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "ReceivePacket
        // lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

        const char* buffer_cmd = ",Date: ";
        size_t buffer_index = 0;
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd), &buffer_index)) {
          buffer.pop_back();  // 这里移除末尾的'\0'否则每个数据后都会有'\0'
          buffer_data.insert(buffer_data.end(),
              buffer.begin() + buffer_index + strlen(buffer_cmd), buffer.end());
        }

        buffer_cmd = "\r\nOK\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          buffer_data.push_back('\0');  // 数据末尾加上'\0'使数据有终止符号
          break;
        }

        buffer_cmd = "\r\nERROR\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(
              LogLevel::kChip, __FILE__, __LINE__, "GetRealTime error\n");
          return false;
        }

        buffer_cmd = "\r\nbusy p...\r\n";
        if (Search(buffer.data(), buffer.size(), buffer_cmd,
                std::strlen(buffer_cmd))) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__, "GetRealTime busy\n");
          buffer_timeout_count = 0;
        }
      }
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_ms / 10) {
      LogMessage(LogLevel::kChip, __FILE__, __LINE__, "GetRealTime timeout\n");
      return false;
    }

    DelayMs(10);
  }

  // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "buffer_data: [%s]\n",
  // buffer_data.data());

  const char* buffer_cmd = " ";
  size_t buffer_index = 0;
  size_t buffer_Index_used = 0;
  if (buffer_data.size() == 0) {
    // LogMessage(LogLevel::kChip, __FILE__, __LINE__, "search fail
    // (buffer_data.size() == 0)\n");
    return false;
  }
  if (Search(buffer_data.data(), buffer_data.size(), buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    //-1去除","字符
    time.week.assign(
        buffer_data.begin(), buffer_data.begin() + (buffer_index - 1));
  }
  buffer_Index_used =
      buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

  if (Search(buffer_data.data() + buffer_Index_used,
          buffer_data.size() - buffer_Index_used, buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    std::string buffer(buffer_data.begin() + buffer_Index_used,
        buffer_data.begin() + buffer_Index_used + buffer_index);
    time.day = std::stoi(buffer.c_str());
  }
  buffer_Index_used =
      buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

  if (Search(buffer_data.data() + buffer_Index_used,
          buffer_data.size() - buffer_Index_used, buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    std::string buffer_month(buffer_data.begin() + buffer_Index_used,
        buffer_data.begin() + buffer_Index_used + buffer_index);
    uint8_t buffer_month_2 = 0;
    for (uint8_t i = 0; i < sizeof(kTimeMonthTable_); i++) {
      if (Search(kTimeMonthTable_[i],
              std::strlen((const char*)kTimeMonthTable_[i]),
              buffer_month.data(), std::strlen(buffer_month.c_str()))) {
        buffer_month_2 = i + 1;
        break;
      }
    }
    time.month = buffer_month_2;
  }
  buffer_Index_used =
      buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

  if (Search(buffer_data.data() + buffer_Index_used,
          buffer_data.size() - buffer_Index_used, buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    std::string buffer(buffer_data.begin() + buffer_Index_used,
        buffer_data.begin() + buffer_Index_used + buffer_index);
    time.year = std::stoi(buffer.c_str());
  }
  buffer_Index_used =
      buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

  buffer_cmd = ":";
  if (Search(buffer_data.data() + buffer_Index_used,
          buffer_data.size() - buffer_Index_used, buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    std::string buffer(buffer_data.begin() + buffer_Index_used,
        buffer_data.begin() + buffer_Index_used + buffer_index);
    time.hour = std::stoi(buffer.c_str());
  }
  buffer_Index_used =
      buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

  if (Search(buffer_data.data() + buffer_Index_used,
          buffer_data.size() - buffer_Index_used, buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    std::string buffer(buffer_data.begin() + buffer_Index_used,
        buffer_data.begin() + buffer_Index_used + buffer_index);
    time.minute = std::stoi(buffer.c_str());
  }
  buffer_Index_used =
      buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

  buffer_cmd = " ";
  if (Search(buffer_data.data() + buffer_Index_used,
          buffer_data.size() - buffer_Index_used, buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    std::string buffer(buffer_data.begin() + buffer_Index_used,
        buffer_data.begin() + buffer_Index_used + buffer_index);
    time.second = std::stoi(buffer.c_str());
  }
  buffer_Index_used =
      buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

  // 结束
  buffer_cmd = "\r\n";
  if (Search(buffer_data.data() + buffer_Index_used,
          buffer_data.size() - buffer_Index_used, buffer_cmd,
          std::strlen(buffer_cmd), &buffer_index)) {
    time.time_zone.assign(buffer_data.begin() + buffer_Index_used,
        buffer_data.begin() + buffer_Index_used + buffer_index);
  }

  return true;
}

}  // namespace cpp_bus_driver
