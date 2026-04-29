/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-04-22 15:07:08
 * @License: GPL 3.0
 */
#include "l76k.h"

namespace cpp_bus_driver {
bool L76k::Init(int32_t baud_rate) {
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    ChipUartGuide::SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup);

    ChipUartGuide::GpioWrite(rst_, 1);
    ChipUartGuide::DelayMs(10);
    ChipUartGuide::GpioWrite(rst_, 0);
    ChipUartGuide::DelayMs(10);
    ChipUartGuide::GpioWrite(rst_, 1);
    ChipUartGuide::DelayMs(10);
  }

  if (wake_up_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    if (!ChipUartGuide::SetGpioMode(
            wake_up_, GpioMode::kOutput, GpioStatus::kPullup)) {
      ChipUartGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "GpioMode failed\n");
    }
  }

  if (!Sleep(false)) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Sleep failed\n");
  }

  if (!ChipUartGuide::Init(baud_rate)) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  size_t buffer_index = 0;
  if (!GetDeviceId(&buffer_index)) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Get l76k id failed\n");
    return false;
  } else {
    ChipUartGuide::LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
        "Get l76k id success (index: %d)\n", buffer_index);
  }

  return true;
}

bool L76k::GetDeviceId(size_t* search_index) {
  std::unique_ptr<uint8_t[]> buffer;
  uint32_t buffer_lenght = 0;

  if (!GetInfoData(buffer, &buffer_lenght)) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "GetInfoData failed\n");
    return false;
  }

  const char* buffer_cmd = "\r\n$G";
  if (!ChipUartGuide::Search(buffer.get(), buffer_lenght, buffer_cmd,
          std::strlen(buffer_cmd), search_index)) {
    return false;
  }

  return true;
}

bool L76k::Sleep(bool enable) {
  if (wake_up_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    if (!ChipUartGuide::GpioWrite(wake_up_, !enable)) {
      ChipUartGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "GpioWrite failed\n");
      return false;
    }
  } else if (wake_up_callback_ != nullptr) {
    if (!wake_up_callback_(!enable)) {
      ChipUartGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "wake_up_callback_ failed\n");
      return false;
    }
  } else {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Sleep failed\n");
    return false;
  }

  return true;
}

uint32_t L76k::ReadData(uint8_t* data, uint32_t length) {
  uint32_t length_buffer = bus_->GetRxBufferLength();
  if (length_buffer == 0) {
    return false;
  }

  if ((length == 0) || (length >= length_buffer)) {
    if (!bus_->Read(data, length_buffer)) {
      ChipUartGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  } else if (length < length_buffer) {
    if (!bus_->Read(data, length)) {
      ChipUartGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
      return false;
    }
  }

  return length_buffer;
}

size_t L76k::GetRxBufferLength() { return bus_->GetRxBufferLength(); }

bool L76k::ClearRxBufferData() { return bus_->ClearRxBufferData(); }

bool L76k::GetInfoData(std::unique_ptr<uint8_t[]>& data, uint32_t* length,
    uint32_t max_length, uint8_t timeout_count) {
  uint8_t buffer_timeout_count = 0;

  while (1) {
    ChipUartGuide::DelayMs(update_freq_);

    uint32_t buffer_lenght = GetRxBufferLength();
    if (buffer_lenght > max_length) {
      buffer_lenght = max_length;
    }

    if (buffer_lenght > 0) {
      data = std::make_unique<uint8_t[]>(buffer_lenght);
      if (data == nullptr) {
        ChipUartGuide::LogMessage(
            LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
        data = nullptr;
        *length = 0;
        return false;
      }

      buffer_lenght = bus_->Read(data.get(), buffer_lenght);
      if (!buffer_lenght) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
        data = nullptr;
        *length = 0;
        return false;
      }

      ChipUartGuide::LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
          "GetInfoData length: %d\n", buffer_lenght);
      *length = buffer_lenght;
      break;
    }

    buffer_timeout_count++;
    if (buffer_timeout_count > timeout_count)  // 超时
    {
      ChipUartGuide::LogMessage(
          LogLevel::kChip, __FILE__, __LINE__, "Read timeout\n");
      data = nullptr;
      *length = 0;
      return false;
    }
  }

  return true;
}

bool L76k::SetUpdateFrequency(UpdateFreq freq) {
  const char* buffer = nullptr;

  switch (freq) {
    case UpdateFreq::kFreq1Hz:
      buffer = "$PCAS02,1000*2E\r\n";
      update_freq_ = 1000;
      break;
    case UpdateFreq::kFreq2Hz:
      buffer = "$PCAS02,500*1A\r\n";
      update_freq_ = 500;
      break;
    case UpdateFreq::kFreq5Hz:
      buffer = "$PCAS02,200*1D\r\n";
      update_freq_ = 200;
      break;
    default:
      break;
  }

  if (!bus_->Write(buffer, strlen(buffer))) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool L76k::SetBaudRate(BaudRate baud_rate) {
  const char* buffer = nullptr;

  switch (baud_rate) {
    case BaudRate::kBr4800Bps:
      buffer = "$PCAS01,0*1C\r\n";
      break;
    case BaudRate::kBr9600Bps:
      buffer = "$PCAS01,1*1D\r\n";
      break;
    case BaudRate::kBr19200Bps:
      buffer = "$PCAS01,2*1E\r\n";
      break;
    case BaudRate::kBr38400Bps:
      buffer = "$PCAS01,3*1F\r\n";
      break;
    case BaudRate::kBr57600Bps:
      buffer = "$PCAS01,4*18\r\n";
      break;
    case BaudRate::kBr115200Bps:
      buffer = "$PCAS01,5*19\r\n";
      break;

    default:
      break;
  }

  if (!bus_->Write(buffer, strlen(buffer))) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }
  // 只有设置波特率时候需要延时
  //  因为没有忙总线所以这里写入数据需要在模块未发送数据空闲的时候写，所以要延时，延时时间为更新频率的一半
  ChipUartGuide::DelayMs(update_freq_ / 2);
  if (!bus_->Write(buffer, strlen(buffer))) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  switch (baud_rate) {
    case BaudRate::kBr4800Bps:
      if (!bus_->SetBaudRate(4800)) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
        return false;
      }
      break;
    case BaudRate::kBr9600Bps:
      if (!bus_->SetBaudRate(9600)) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
        return false;
      }
      break;
    case BaudRate::kBr19200Bps:
      if (!bus_->SetBaudRate(19200)) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
        return false;
      }
      break;
    case BaudRate::kBr38400Bps:
      if (!bus_->SetBaudRate(38400)) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
        return false;
      }
      break;
    case BaudRate::kBr57600Bps:
      if (!bus_->SetBaudRate(57600)) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
        return false;
      }
      break;
    case BaudRate::kBr115200Bps:
      if (!bus_->SetBaudRate(115200)) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
        return false;
      }
      break;

    default:
      break;
  }

  return true;
}

uint32_t L76k::GetBaudRate() { return bus_->GetBaudRate(); }

bool L76k::SetRestartMode(RestartMode mode) {
  const char* buffer = nullptr;

  switch (mode) {
    case RestartMode::kHotStart:
      buffer = "$PCAS10,0*1C\r\n";
      break;
    case RestartMode::kWarmStart:
      buffer = "$PCAS10,1*1D\r\n";
      break;
    case RestartMode::kColdStart:
      buffer = "$PCAS10,2*1E\r\n";
      break;
    case RestartMode::kColdStartFactoryReset:
      buffer = "$PCAS10,3*1F\r\n";
      break;
    default:
      break;
  }

  if (!bus_->Write(buffer, strlen(buffer))) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool L76k::SetGnssConstellation(GnssConstellation constellation) {
  const char* buffer = nullptr;

  switch (constellation) {
    case GnssConstellation::kGps:
      buffer = "$PCAS04,1*18\r\n";
      break;
    case GnssConstellation::kBeidou:
      buffer = "$PCAS04,2*1B\r\n";
      break;
    case GnssConstellation::kGpsBeidou:
      buffer = "$PCAS04,3*1A\r\n";
      break;
    case GnssConstellation::kGlonass:
      buffer = "$PCAS04,4*1D\r\n";
      break;
    case GnssConstellation::kGpsGlonass:
      buffer = "$PCAS04,5*1C\r\n";
      break;
    case GnssConstellation::kBeidouGlonass:
      buffer = "$PCAS04,6*1F\r\n";
      break;
    case GnssConstellation::kGpsBeidouGlonass:
      buffer = "$PCAS04,7*1E\r\n";
      break;
    default:
      break;
  }

  if (!bus_->Write(buffer, strlen(buffer))) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

}  // namespace cpp_bus_driver
