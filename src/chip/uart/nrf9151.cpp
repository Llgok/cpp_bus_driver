/*
 * @Description: nRF9151 蜂窝通信与 GNSS 模块驱动实现
 * @Author: LILYGO_L
 * @Date: 2026-07-11 11:58:39
 * @LastEditTime: 2026-09-05 14:57:05
 * @License: GPL 3.0
 */
#include "chip/uart/nrf9151.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

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
  const size_t line_start =
      previous_line_end == std::string::npos ? 0 : previous_line_end + 1;
  const size_t next_line_end = response.find('\n', marker_position);
  const size_t line_length = next_line_end == std::string::npos
                                 ? response.size() - line_start
                                 : next_line_end - line_start;

  *line = Trim(response.substr(line_start, line_length));
  return !line->empty();
}

/**
 * @brief 去除非拥有文本视图两端的空白，不复制正文
 * @param value 输入文本视图
 * @return 去除首尾空白后的视图，其有效期与输入一致
 */
std::string_view TrimView(std::string_view value) {
  const size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) {
    return {};
  }
  const size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

/**
 * @brief 解析版本响应，最多保存前三个字段的视图
 * @param value CSV 正文
 * @param fields 应用、NCS 和可选用户版本的输出视图
 * @param count 实际保存的字段数
 * @return 引号配对且至少包含两个字段时返回 true
 */
bool ParseVersionFields(std::string_view value,
    std::array<std::string_view, 3>* fields, size_t* count) {
  if (fields == nullptr || count == nullptr) {
    return false;
  }
  *count = 0;
  size_t start = 0;
  bool in_quotes = false;
  for (size_t index = 0; index <= value.size(); ++index) {
    if (index < value.size() && value[index] == '"') {
      in_quotes = !in_quotes;
    }
    if (index != value.size() && (value[index] != ',' || in_quotes)) {
      continue;
    }
    if (*count < fields->size()) {
      std::string_view field = TrimView(value.substr(start, index - start));
      if (field.size() >= 2 && field.front() == '"' && field.back() == '"') {
        field.remove_prefix(1);
        field.remove_suffix(1);
      }
      (*fields)[(*count)++] = field;
    }
    start = index + 1;
  }
  return !in_quotes && *count >= 2;
}

}  // namespace

bool Nrf9151::Init(int32_t baud_rate) {
  return Init(baud_rate, kDefaultInitializationTimeoutMs);
}

bool Nrf9151::Init(int32_t baud_rate, uint32_t initialization_timeout_ms) {
  if (initialization_timeout_ms == 0) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  chip_id_.clear();
  if (!UartChipBase::Init(baud_rate)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__, "Init uart failed\n");
    return false;
  }

  const int64_t start_time_ms = GetSystemTimeMs();
  uint32_t probe_attempts = 0;

  while (true) {
    const int64_t elapsed_ms = GetSystemTimeMs() - start_time_ms;
    if (elapsed_ms >= initialization_timeout_ms) {
      break;
    }

    const uint32_t remaining_ms =
        static_cast<uint32_t>(initialization_timeout_ms - elapsed_ms);
    const uint32_t command_timeout_ms =
        std::min(kDefaultCommandTimeoutMs, remaining_ms);
    ++probe_attempts;
    if (GetChipId(command_timeout_ms)) {
      LogMessage(LogLevel::kInfo, __FILE__, __LINE__,
          "Get nrf9151 chip id success "
          "(model: %s, attempts: %u, elapsed: %lld ms)\n",
          chip_id_.c_str(), static_cast<unsigned>(probe_attempts),
          static_cast<long long>(GetSystemTimeMs() - start_time_ms));
      return true;
    }
  }

  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "Init nrf9151 failed (attempts: %u, timeout: %u ms)\n",
      static_cast<unsigned>(probe_attempts),
      static_cast<unsigned>(initialization_timeout_ms));
  if (!Deinit()) {
    LogMessage(LogLevel::kWarning, __FILE__, __LINE__,
        "Release nrf9151 UART after init failure failed\n");
  }
  return false;
}

bool Nrf9151::Deinit() {
  chip_id_.clear();
  return UartChipBase::Deinit();
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

bool Nrf9151::GetChipId(uint32_t timeout_ms) {
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

  chip_id_ = model;
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

  std::array<std::string_view, 3> fields{};
  size_t field_count = 0;
  if (!ParseVersionFields(value, &fields, &field_count)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "AT#XSMVER response does not contain required versions\n");
    return false;
  }

  version->application.assign(
      fields[0].empty() ? "" : fields[0].data(), fields[0].size());
  version->ncs.assign(
      fields[1].empty() ? "" : fields[1].data(), fields[1].size());
  if (field_count > 2) {
    version->customer.assign(
        fields[2].empty() ? "" : fields[2].data(), fields[2].size());
  } else {
    version->customer.clear();
  }
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

  // 在构造字符串前限制命令长度，避免异常参数触发大分配。
  size_t command_length = 0;
  while (
      command_length < kMaxResponseLength && command[command_length] != '\0') {
    ++command_length;
  }
  if (command_length == kMaxResponseLength) {
    LogMessage(
        LogLevel::kWarning, __FILE__, __LINE__, "AT command is too long\n");
    return CommandResult::kError;
  }
  std::string request(command, command_length);
  response->clear();
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

  const uint32_t start_time_ms = static_cast<uint32_t>(GetSystemTimeMs());
  size_t parsed_length = 0;
  // UART 积压较多时分块读取，不按 available 一次性申请接收缓存。
  std::array<uint8_t, 256> buffer{};

  while (
      static_cast<uint32_t>(GetSystemTimeMs()) - start_time_ms < timeout_ms) {
    const size_t available = bus_->GetRxBufferLength();
    if (available == 0) {
      DelayMs(10);
      continue;
    }

    if (response->size() >= kMaxResponseLength) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__,
          "AT command response is too long\n");
      return CommandResult::kError;
    }
    const size_t remaining = kMaxResponseLength - response->size();
    const size_t requested =
        std::min(std::min(available, buffer.size()), remaining);
    const int32_t read_length =
        bus_->Read(buffer.data(), static_cast<uint32_t>(requested));
    if (read_length <= 0 || static_cast<size_t>(read_length) > requested) {
      LogMessage(LogLevel::kError, __FILE__, __LINE__, "Read failed\n");
      return CommandResult::kIoError;
    }

    // requested 已被剩余容量限制，追加前即可保证不超过响应上限。
    response->append(reinterpret_cast<const char*>(buffer.data()),
        static_cast<size_t>(read_length));

    while (true) {
      const size_t line_end = response->find('\n', parsed_length);
      if (line_end == std::string::npos) {
        break;
      }

      const std::string_view line = TrimView(std::string_view(
          response->data() + parsed_length, line_end - parsed_length));
      parsed_length = line_end + 1;

      if (line == "OK") {
        return CommandResult::kOk;
      }
      if ((line == "ERROR") || (line.rfind("+CME ERROR", 0) == 0) ||
          (line.rfind("+CMS ERROR", 0) == 0)) {
        LogMessage(LogLevel::kError, __FILE__, __LINE__,
            "AT command failed: %.*s\n", static_cast<int>(line.size()),
            line.data());
        return CommandResult::kError;
      }
    }
  }

  LogMessage(LogLevel::kError, __FILE__, __LINE__,
      "AT command timeout (command: %s, timeout: %d ms)\n", request.c_str(),
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
