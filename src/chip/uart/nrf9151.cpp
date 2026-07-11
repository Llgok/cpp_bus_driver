/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-07-11 11:58:39
 * @LastEditTime: 2026-07-11 14:39:16
 * @License: GPL 3.0
 */
#include "nrf9151.h"

namespace cpp_bus_driver {
namespace {

/**
 * @brief 去除字符串首尾的空格、制表符和换行符
 * @param value 需要处理的字符串
 * @return 去除首尾空白字符后的字符串
 */
std::string Trim(const std::string& value) {
  const size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }

  const size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

/**
 * @brief 从完整响应中提取包含指定内容的数据行
 * @param response 完整响应字符串
 * @param marker 目标行必须包含的字符串
 * @param line 用于保存目标数据行的指针
 * @return 找到目标数据行时返回 true，否则返回 false
 */
bool ExtractLineContaining(
    const std::string& response, const char* marker, std::string* line) {
  if ((marker == nullptr) || (line == nullptr)) {
    return false;
  }

  const size_t marker_position = response.find(marker);
  if (marker_position == std::string::npos) {
    return false;
  }

  const size_t previous_line_end = response.rfind('\n', marker_position);
  const size_t line_start = previous_line_end == std::string::npos
                                ? 0
                                : previous_line_end + 1;
  const size_t next_line_end = response.find('\n', marker_position);
  const size_t line_length = next_line_end == std::string::npos
                                 ? response.size() - line_start
                                 : next_line_end - line_start;

  *line = Trim(response.substr(line_start, line_length));
  return !line->empty();
}

/**
 * @brief 去除字符串首尾空白字符和成对的双引号
 * @param value 需要处理的字符串
 * @return 去除首尾空白字符和成对双引号后的字符串
 */
std::string RemoveQuotes(const std::string& value) {
  const std::string trimmed = Trim(value);
  if ((trimmed.size() >= 2) && (trimmed.front() == '"') &&
      (trimmed.back() == '"')) {
    return trimmed.substr(1, trimmed.size() - 2);
  }
  return trimmed;
}

/**
 * @brief 解析支持双引号字段的逗号分隔字符串
 * @param value 需要解析的逗号分隔字符串
 * @return 解析并去除字段双引号后的字符串数组
 */
std::vector<std::string> ParseCsv(const std::string& value) {
  std::vector<std::string> fields;
  std::string field;
  bool in_quotes = false;

  for (const char character : value) {
    if (character == '"') {
      in_quotes = !in_quotes;
      field.push_back(character);
    } else if ((character == ',') && !in_quotes) {
      fields.push_back(RemoveQuotes(field));
      field.clear();
    } else {
      field.push_back(character);
    }
  }
  fields.push_back(RemoveQuotes(field));
  return fields;
}

}  // namespace

bool Nrf9151::Init(int32_t baud_rate) {
  return Init(baud_rate, kDefaultCommandTimeoutMs);
}

bool Nrf9151::Init(int32_t baud_rate, uint32_t timeout_ms) {
  if (!ChipUartGuide::Init(baud_rate)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init uart failed\n");
    return false;
  }

  if (!GetDeviceId(timeout_ms)) {
    LogMessage(
        LogLevel::kError, __FILE__, __LINE__, "Get nrf9151 device id failed\n");
    return false;
  }

  LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
      "Get nrf9151 device id success (model: %s)\n", device_id_.c_str());
  return true;
}

bool Nrf9151::Deinit() {
  device_id_.clear();
  return ChipUartGuide::Deinit();
}

const char* Nrf9151::CommandResultToString(CommandResult result) {
  switch (result) {
    case CommandResult::kOk:
      return "ok";
    case CommandResult::kError:
      return "error";
    case CommandResult::kTimeout:
      return "timeout";
    case CommandResult::kIoError:
      return "io_error";
    default:
      return "unknown";
  }
}

bool Nrf9151::GetDeviceId(uint32_t timeout_ms) {
  std::string response;
  if (SendCommand("AT+CGMM", &response, timeout_ms) != CommandResult::kOk) {
    return false;
  }

  std::string model;
  if (!ExtractLineContaining(response, "nRF9151", &model)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "AT+CGMM response does not contain a model\n");
    return false;
  }

  device_id_ = model;
  return true;
}

bool Nrf9151::GetSerialModemVersion(
    SerialModemVersion* version, uint32_t timeout_ms) {
  if (version == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  std::string response;
  if (SendCommand("AT#XSMVER", &response, timeout_ms) != CommandResult::kOk) {
    return false;
  }

  std::string value;
  if (!ExtractResponseLine(response, "AT#XSMVER", "#XSMVER:", &value)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "AT#XSMVER response format is invalid\n");
    return false;
  }

  const std::vector<std::string> fields = ParseCsv(value);
  if (fields.size() < 2) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "AT#XSMVER response does not contain required versions\n");
    return false;
  }

  version->application = fields[0];
  version->ncs = fields[1];
  version->customer = fields.size() > 2 ? fields[2] : "";
  return true;
}

bool Nrf9151::GetModemFirmwareVersion(
    std::string* version, uint32_t timeout_ms) {
  if (version == nullptr) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  std::string response;
  if (SendCommand("AT+CGMR", &response, timeout_ms) != CommandResult::kOk) {
    return false;
  }

  if (!ExtractLineContaining(response, "mfw_nrf91", version)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "AT+CGMR response does not contain a firmware version\n");
    return false;
  }
  return true;
}

Nrf9151::CommandResult Nrf9151::SendCommand(
    const char* command, std::string* response, uint32_t timeout_ms) {
  if ((command == nullptr) || (response == nullptr) || (timeout_ms == 0)) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return CommandResult::kIoError;
  }

  response->clear();
  std::string request(command);
  if (request.empty() ||
      ((request.back() != '\r') && (request.back() != '\n'))) {
    request.push_back('\r');
  }

  const int32_t written = bus_->Write(request.data(), request.size());
  if (written != static_cast<int32_t>(request.size())) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Write AT command failed (expected: %d, actual: %d)\n",
        static_cast<int>(request.size()), written);
    return CommandResult::kIoError;
  }

  const int64_t deadline_ms = GetSystemTimeMs() + timeout_ms;
  size_t parsed_length = 0;

  while (GetSystemTimeMs() < deadline_ms) {
    const size_t available = bus_->GetRxBufferLength();
    if (available == 0) {
      DelayMs(10);
      continue;
    }

    std::vector<uint8_t> buffer(available);
    const int32_t read_length =
        bus_->Read(buffer.data(), static_cast<uint32_t>(buffer.size()));
    if (read_length <= 0) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
      return CommandResult::kIoError;
    }

    response->append(reinterpret_cast<const char*>(buffer.data()),
        static_cast<size_t>(read_length));
    if (response->size() > kMaxResponseLength) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "AT command response is too long\n");
      return CommandResult::kError;
    }

    while (true) {
      const size_t line_end = response->find('\n', parsed_length);
      if (line_end == std::string::npos) {
        break;
      }

      const std::string line =
          Trim(response->substr(parsed_length, line_end - parsed_length));
      parsed_length = line_end + 1;

      if (line == "OK") {
        return CommandResult::kOk;
      }
      if ((line == "ERROR") || (line.rfind("+CME ERROR", 0) == 0) ||
          (line.rfind("+CMS ERROR", 0) == 0)) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "AT command failed: %s\n", line.c_str());
        return CommandResult::kError;
      }
    }
  }

  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "AT command timeout (command: %s, timeout: %d ms)\n", command,
      static_cast<int>(timeout_ms));
  return CommandResult::kTimeout;
}

bool Nrf9151::ExtractResponseLine(const std::string& response,
    const char* command, const char* prefix, std::string* line) {
  if ((command == nullptr) || (line == nullptr)) {
    return false;
  }

  size_t offset = 0;
  while (offset < response.size()) {
    const size_t line_end = response.find('\n', offset);
    const size_t count = line_end == std::string::npos
                             ? response.size() - offset
                             : line_end - offset;
    std::string candidate = Trim(response.substr(offset, count));

    if (!candidate.empty() && (candidate != command) && (candidate != "OK") &&
        (candidate != "ERROR")) {
      if (prefix == nullptr) {
        // 等待 CGMM、CGMR 等未带前缀的识别响应时，忽略常见的 URC 前缀。
        if ((candidate.front() == '+') || (candidate.front() == '%') ||
            (candidate.front() == '#')) {
          if (line_end == std::string::npos) {
            break;
          }
          offset = line_end + 1;
          continue;
        }
        *line = candidate;
        return true;
      }

      const size_t prefix_length = std::strlen(prefix);
      if (candidate.compare(0, prefix_length, prefix) == 0) {
        *line = Trim(candidate.substr(prefix_length));
        return true;
      }
    }

    if (line_end == std::string::npos) {
      break;
    }
    offset = line_end + 1;
  }

  return false;
}
}  // namespace cpp_bus_driver
