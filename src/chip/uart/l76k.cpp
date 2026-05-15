/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2026-04-30 14:32:02
 * @License: GPL 3.0
 */
#include "l76k.h"

namespace cpp_bus_driver {
namespace {
/**
 * @brief 按小端格式追加 16 位无符号数据
 * @param data 目标数据缓存
 * @param value 需要追加的 16 位数值
 */
void AppendU16(std::vector<uint8_t>& data, uint16_t value) {
  data.push_back(value & 0xFF);
  data.push_back((value >> 8) & 0xFF);
}

/**
 * @brief 按小端格式追加 32 位无符号数据
 * @param data 目标数据缓存
 * @param value 需要追加的 32 位数值
 */
void AppendU32(std::vector<uint8_t>& data, uint32_t value) {
  data.push_back(value & 0xFF);
  data.push_back((value >> 8) & 0xFF);
  data.push_back((value >> 16) & 0xFF);
  data.push_back((value >> 24) & 0xFF);
}

/**
 * @brief 将 PCAS03 字段值转换为字符串
 * @param value 字段值，负数表示该字段留空以保持原配置
 * @return 字段字符串
 */
std::string PcasField(int8_t value) {
  if (value < 0) {
    return "";
  }

  return std::to_string(value);
}

/**
 * @brief 判断 PCAS03 输出频率字段是否有效
 * @param value 输出频率，-1 表示保持原配置，0~9 为手册支持值
 * @return 字段有效返回 true
 */
bool IsPcasOutputRateValid(int8_t value) { return value >= -1 && value <= 9; }

/**
 * @brief 将波特率枚举转换为 PCAS01 命令参数
 * @param baud_rate 波特率枚举
 * @param value 输出 PCAS01 命令参数
 * @return 转换成功返回 true
 */
bool BaudRateToPcas01Value(L76k::BaudRate baud_rate, uint8_t& value) {
  switch (baud_rate) {
    case L76k::BaudRate::kBr4800Bps:
      value = 0;
      return true;
    case L76k::BaudRate::kBr9600Bps:
      value = 1;
      return true;
    case L76k::BaudRate::kBr19200Bps:
      value = 2;
      return true;
    case L76k::BaudRate::kBr38400Bps:
      value = 3;
      return true;
    case L76k::BaudRate::kBr57600Bps:
      value = 4;
      return true;
    case L76k::BaudRate::kBr115200Bps:
      value = 5;
      return true;
    default:
      return false;
  }
}
}  // namespace

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

bool L76k::Deinit() {
  if (!ChipUartGuide::Deinit()) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  if (wake_up_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    ChipUartGuide::SetGpioMode(
        wake_up_, GpioMode::kDisable, GpioStatus::kDisable);
  }
  if (rst_ != CPP_BUS_DRIVER_DEFAULT_VALUE) {
    ChipUartGuide::SetGpioMode(rst_, GpioMode::kDisable, GpioStatus::kDisable);
  }

  return true;
}

bool L76k::GetDeviceId(size_t* search_index) {
  std::unique_ptr<uint8_t[]> buffer;
  uint32_t buffer_length = 0;

  if (!GetInfoData(buffer, &buffer_length)) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "GetInfoData failed\n");
    return false;
  }

  const char* buffer_cmd = "$G";
  if (!ChipUartGuide::Search(buffer.get(), buffer_length, buffer_cmd,
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
  if (data == nullptr) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return 0;
  }

  const uint32_t length_buffer = bus_->GetRxBufferLength();
  if (length_buffer == 0) {
    return 0;
  }

  uint32_t read_length = length_buffer;
  if ((length == 0) || (length >= length_buffer)) {
    read_length = length_buffer;
  } else if (length < length_buffer) {
    read_length = length;
  }

  const int32_t result = bus_->Read(data, read_length);
  if (result <= 0) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
    return 0;
  }

  return result;
}

size_t L76k::GetRxBufferLength() { return bus_->GetRxBufferLength(); }

bool L76k::ClearRxBufferData() { return bus_->ClearRxBufferData(); }

bool L76k::GetInfoData(std::unique_ptr<uint8_t[]>& data, uint32_t* length,
    uint32_t max_length, uint8_t timeout_count) {
  if (length == nullptr) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    data = nullptr;
    return false;
  }
  if (max_length == 0) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    data = nullptr;
    *length = 0;
    return false;
  }

  uint8_t buffer_timeout_count = 0;

  while (true) {
    ChipUartGuide::DelayMs(update_freq_);

    uint32_t buffer_length = GetRxBufferLength();
    if (buffer_length > max_length) {
      buffer_length = max_length;
    }

    if (buffer_length > 0) {
      data = std::make_unique<uint8_t[]>(buffer_length);
      if (data == nullptr) {
        ChipUartGuide::LogMessage(
            LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
        data = nullptr;
        *length = 0;
        return false;
      }

      const int32_t read_length = bus_->Read(data.get(), buffer_length);
      if (read_length <= 0) {
        ChipUartGuide::LogMessage(
            LogLevel::kChip, __FILE__, __LINE__, "Read failed\n");
        data = nullptr;
        *length = 0;
        return false;
      }

      ChipUartGuide::LogMessage(LogLevel::kDebug, __FILE__, __LINE__,
          "GetInfoData length: %d\n", read_length);
      buffer_length = static_cast<uint32_t>(read_length);
      *length = buffer_length;
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
  uint16_t interval_ms = 0;
  switch (freq) {
    case UpdateFreq::kFreq1Hz:
      interval_ms = 1000;
      break;
    case UpdateFreq::kFreq2Hz:
      interval_ms = 500;
      break;
    case UpdateFreq::kFreq5Hz:
      interval_ms = 200;
      break;
    default:
      ChipUartGuide::LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }

  if (!WritePcasCommand("PCAS02," + std::to_string(interval_ms))) {
    return false;
  }

  update_freq_ = interval_ms;
  return true;
}

bool L76k::SetBaudRate(BaudRate baud_rate) {
  uint8_t pcas_value = 0;
  const uint32_t baud_rate_value = BaudRateToValue(baud_rate);
  if (baud_rate_value == 0 || !BaudRateToPcas01Value(baud_rate, pcas_value)) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  const std::string body = "PCAS01," + std::to_string(pcas_value);
  if (!WritePcasCommand(body)) {
    return false;
  }

  // 只有设置波特率时需要延时
  // 因为没有忙总线，所以这里写入数据需要在模块未发送数据空闲的时候写。
  // 延时时间为更新频率的一半。
  ChipUartGuide::DelayMs(update_freq_ / 2);
  if (!WritePcasCommand(body)) {
    return false;
  }

  if (!bus_->SetBaudRate(baud_rate_value)) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
    return false;
  }

  return true;
}

uint32_t L76k::GetBaudRate() { return bus_->GetBaudRate(); }

bool L76k::SetRestartMode(RestartMode mode) {
  uint8_t restart_mode = 0;

  switch (mode) {
    case RestartMode::kHotStart:
      restart_mode = 0;
      break;
    case RestartMode::kWarmStart:
      restart_mode = 1;
      break;
    case RestartMode::kColdStart:
      restart_mode = 2;
      break;
    case RestartMode::kColdStartFactoryReset:
      restart_mode = 3;
      break;
    default:
      ChipUartGuide::LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }

  return WritePcasCommand("PCAS10," + std::to_string(restart_mode));
}

bool L76k::SetGnssConstellation(GnssConstellation constellation) {
  uint8_t mode = 0;

  switch (constellation) {
    case GnssConstellation::kGps:
      mode = 1;
      break;
    case GnssConstellation::kBeidou:
      mode = 2;
      break;
    case GnssConstellation::kGpsBeidou:
      mode = 3;
      break;
    case GnssConstellation::kGlonass:
      mode = 4;
      break;
    case GnssConstellation::kGpsGlonass:
      mode = 5;
      break;
    case GnssConstellation::kBeidouGlonass:
      mode = 6;
      break;
    case GnssConstellation::kGpsBeidouGlonass:
      mode = 7;
      break;
    default:
      ChipUartGuide::LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }

  return WritePcasCommand("PCAS04," + std::to_string(mode));
}

bool L76k::SetNmeaOutputConfig(const NmeaOutputConfig& config) {
  if (!IsPcasOutputRateValid(config.gga) ||
      !IsPcasOutputRateValid(config.gll) ||
      !IsPcasOutputRateValid(config.gsa) ||
      !IsPcasOutputRateValid(config.gsv) ||
      !IsPcasOutputRateValid(config.rmc) ||
      !IsPcasOutputRateValid(config.vtg) ||
      !IsPcasOutputRateValid(config.zda) ||
      !IsPcasOutputRateValid(config.ant)) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::string body = "PCAS03,";
  body += PcasField(config.gga);
  body += ",";
  body += PcasField(config.gll);
  body += ",";
  body += PcasField(config.gsa);
  body += ",";
  body += PcasField(config.gsv);
  body += ",";
  body += PcasField(config.rmc);
  body += ",";
  body += PcasField(config.vtg);
  body += ",";
  body += PcasField(config.zda);
  body += ",";
  body += PcasField(config.ant);
  body += ",0,0,,,0,0";

  return WritePcasCommand(body);
}

bool L76k::SetNmeaSentenceOutput(NmeaSentence sentence, uint16_t rate) {
  if (!((rate <= 9) || (rate == 0xFFFF))) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  uint8_t message_id = 0;
  switch (sentence) {
    case NmeaSentence::kGga:
      message_id = 0x00;
      break;
    case NmeaSentence::kGll:
      message_id = 0x01;
      break;
    case NmeaSentence::kGsa:
      message_id = 0x02;
      break;
    case NmeaSentence::kGsv:
      message_id = 0x03;
      break;
    case NmeaSentence::kRmc:
      message_id = 0x04;
      break;
    case NmeaSentence::kVtg:
      message_id = 0x05;
      break;
    case NmeaSentence::kZda:
      message_id = 0x08;
      break;
    default:
      ChipUartGuide::LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }

  std::vector<uint8_t> payload;
  payload.push_back(0x4E);
  payload.push_back(message_id);
  AppendU16(payload, rate);

  return WriteCasicCommand(0x06, 0x01, payload);
}

bool L76k::QueryNmeaSentenceOutput() {
  return WriteCasicCommand(0x06, 0x01, std::vector<uint8_t>());
}

bool L76k::SetCasicPortConfig(const CasicPortConfig& config) {
  std::vector<uint8_t> payload;
  payload.push_back(config.port_id);
  payload.push_back(config.proto_mask);
  AppendU16(payload, config.mode);
  AppendU32(payload, config.baud_rate);

  return WriteCasicCommand(0x06, 0x00, payload);
}

bool L76k::QueryCasicPortConfig() {
  return WriteCasicCommand(0x06, 0x00, std::vector<uint8_t>());
}

bool L76k::SetCasicBaudRate(BaudRate baud_rate) {
  const uint32_t baud_rate_value = BaudRateToValue(baud_rate);
  if (baud_rate_value == 0) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  CasicPortConfig config;
  config.baud_rate = baud_rate_value;
  if (!SetCasicPortConfig(config)) {
    return false;
  }

  ChipUartGuide::DelayMs(update_freq_ / 2);
  if (!bus_->SetBaudRate(baud_rate_value)) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "SetBaudRate failed\n");
    return false;
  }

  return true;
}

bool L76k::SetCasicRestartMode(RestartMode start_mode,
    uint16_t nav_bbr_mask, CasicResetMode reset_mode) {
  uint8_t start_mode_value = 0;
  switch (start_mode) {
    case RestartMode::kHotStart:
      start_mode_value = 0;
      break;
    case RestartMode::kWarmStart:
      start_mode_value = 1;
      break;
    case RestartMode::kColdStart:
      start_mode_value = 2;
      break;
    case RestartMode::kColdStartFactoryReset:
      start_mode_value = 3;
      break;
    default:
      ChipUartGuide::LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }

  switch (reset_mode) {
    case CasicResetMode::kImmediateHardwareReset:
    case CasicResetMode::kSoftwareReset:
    case CasicResetMode::kSoftwareResetGpsOnly:
    case CasicResetMode::kHardwareResetAfterPowerOff:
      break;
    default:
      ChipUartGuide::LogMessage(
          LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
      return false;
  }

  std::vector<uint8_t> payload;
  AppendU16(payload, nav_bbr_mask);
  payload.push_back(static_cast<uint8_t>(reset_mode));
  payload.push_back(start_mode_value);

  return WriteCasicCommand(0x06, 0x02, payload);
}

bool L76k::SetCasicUpdateInterval(uint16_t interval_ms) {
  if (interval_ms != 200 && interval_ms != 500 && interval_ms != 1000) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::vector<uint8_t> payload;
  AppendU16(payload, interval_ms);
  AppendU16(payload, 0);
  if (!WriteCasicCommand(0x06, 0x04, payload)) {
    return false;
  }

  update_freq_ = interval_ms;
  return true;
}

bool L76k::QueryCasicUpdateInterval() {
  return WriteCasicCommand(0x06, 0x04, std::vector<uint8_t>());
}

bool L76k::WritePcasCommand(const std::string& body) {
  uint8_t checksum = 0;
  for (char value : body) {
    checksum ^= static_cast<uint8_t>(value);
  }

  char checksum_buffer[8] = {0};
  snprintf(checksum_buffer, sizeof(checksum_buffer), "%02X", checksum);

  const std::string command = "$" + body + "*" + checksum_buffer + "\r\n";
  if (!bus_->Write(command.c_str(), command.length())) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

bool L76k::WriteCasicCommand(
    uint8_t class_id, uint8_t message_id, const std::vector<uint8_t>& payload) {
  if ((payload.size() % 4) != 0) {
    ChipUartGuide::LogMessage(
        LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    return false;
  }

  std::vector<uint8_t> frame;
  frame.push_back(0xBA);
  frame.push_back(0xCE);
  AppendU16(frame, payload.size());
  frame.push_back(class_id);
  frame.push_back(message_id);
  frame.insert(frame.end(), payload.begin(), payload.end());

  uint32_t checksum =
      (static_cast<uint32_t>(message_id) << 24) |
      (static_cast<uint32_t>(class_id) << 16) |
      static_cast<uint32_t>(payload.size());
  for (size_t i = 0; i + 3 < payload.size(); i += 4) {
    const uint32_t word = static_cast<uint32_t>(payload[i]) |
                          (static_cast<uint32_t>(payload[i + 1]) << 8) |
                          (static_cast<uint32_t>(payload[i + 2]) << 16) |
                          (static_cast<uint32_t>(payload[i + 3]) << 24);
    checksum += word;
  }
  AppendU32(frame, checksum);

  if (!bus_->Write(frame.data(), frame.size())) {
    ChipUartGuide::LogMessage(
        LogLevel::kChip, __FILE__, __LINE__, "Write failed\n");
    return false;
  }

  return true;
}

uint32_t L76k::BaudRateToValue(BaudRate baud_rate) {
  switch (baud_rate) {
    case BaudRate::kBr4800Bps:
      return 4800;
    case BaudRate::kBr9600Bps:
      return 9600;
    case BaudRate::kBr19200Bps:
      return 19200;
    case BaudRate::kBr38400Bps:
      return 38400;
    case BaudRate::kBr57600Bps:
      return 57600;
    case BaudRate::kBr115200Bps:
      return 115200;
    default:
      return 0;
  }
}

}  // namespace cpp_bus_driver
