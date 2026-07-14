/*
 * @Description: nRF9151 蜂窝通信与 GNSS 模块驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-07-11 11:58:39
 * @LastEditTime: 2026-07-11 14:48:32
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class Nrf9151 final : public ChipUartGuide {
 public:
  // AT 指令默认超时时间，单位为毫秒
  static constexpr uint32_t kDefaultCommandTimeoutMs = 500;

  // ncs-serial-modem 及其构建环境的版本信息
  struct SerialModemVersion {
    std::string application;  // ncs-serial-modem 应用版本
    std::string ncs;          // 构建 ncs-serial-modem 使用的 NCS 版本
    std::string customer;     // 固件中配置的可选用户版本
  };

  // AT 指令执行结果
  enum class CommandResult {
    kOk,       // 指令执行成功并收到 OK
    kError,    // 收到 ERROR、+CME ERROR 或 +CMS ERROR
    kTimeout,  // 等待最终响应超时
    kIoError,  // UART 读写失败
  };

  explicit Nrf9151(std::shared_ptr<BusUartGuide> bus) : ChipUartGuide(bus) {}

  /**
   * @brief 初始化 UART，并使用 AT+CGMM 验证设备是否为 nRF9151
   * @param baud_rate UART 波特率
   * @return UART 初始化且设备型号验证成功时返回 true，否则返回 false
   */
  bool Init(int32_t baud_rate = kDefaultUartBaudRate) override;

  /**
   * @brief 初始化 UART，并在指定时间内使用 AT+CGMM 验证设备型号
   * @param baud_rate UART 波特率
   * @param timeout_ms 等待 AT 指令最终响应的总超时时间，单位为毫秒
   * @return UART 初始化且设备型号验证成功时返回 true，否则返回 false
   */
  bool Init(int32_t baud_rate, uint32_t timeout_ms);

  /**
   * @brief 反初始化 nRF9151 UART 驱动并清除已保存的设备型号
   * @return 反初始化成功时返回 true，否则返回 false
   */
  bool Deinit() override;

  /**
   * @brief 发送 AT+CGMM 获取设备型号，并验证响应中是否包含 nRF9151
   * @param timeout_ms 等待 AT 指令最终响应的总超时时间，单位为毫秒
   * @return 型号读取且验证成功时返回 true，否则返回 false
   */
  bool GetDeviceId(uint32_t timeout_ms = kDefaultCommandTimeoutMs);

  /**
   * @brief 发送 AT#XSMVER 获取 ncs-serial-modem、NCS 和用户版本
   * @param version 用于保存解析后版本信息的指针
   * @param timeout_ms 等待 AT 指令最终响应的总超时时间，单位为毫秒
   * @return 版本信息读取并解析成功时返回 true，否则返回 false
   */
  bool GetSerialModemVersion(SerialModemVersion* version,
      uint32_t timeout_ms = kDefaultCommandTimeoutMs);

  /**
   * @brief 发送 AT+CGMR 获取 nRF91x1 modem firmware 版本
   * @param version 用于保存 modem firmware 版本字符串的指针
   * @param timeout_ms 等待 AT 指令最终响应的总超时时间，单位为毫秒
   * @return 版本信息读取并解析成功时返回 true，否则返回 false
   */
  bool GetModemFirmwareVersion(
      std::string* version, uint32_t timeout_ms = kDefaultCommandTimeoutMs);

  /**
   * @brief 发送 AT 指令并等待 OK、错误响应或超时
   * @param command 不包含结束符的 AT 指令
   * @param response 用于保存完整响应的指针
   * @param timeout_ms 等待最终响应的总超时时间，单位为毫秒
   * @return AT 指令执行结果
   */
  CommandResult SendCommand(const char* command, std::string* response,
      uint32_t timeout_ms = kDefaultCommandTimeoutMs);

  /**
   * @brief 将 AT 指令执行结果转换为可读字符串
   * @param result AT 指令执行结果
   * @return 描述执行结果的静态字符串
   */
  static const char* CommandResultToString(CommandResult result);

  /**
   * @brief 获取最近一次通过 AT+CGMM 验证的设备型号
   * @return 设备型号字符串的常量引用，尚未验证时为空字符串
   */
  const std::string& device_id() const { return device_id_; }

 private:
  // 允许接收的 AT 响应最大长度，单位为字节
  static constexpr size_t kMaxResponseLength = 4096;

  /**
   * @brief 从完整 AT 响应中提取目标数据行
   * @param response 完整 AT 响应
   * @param command 用于过滤命令回显的 AT 指令
   * @param prefix 目标行前缀，为 nullptr 时提取首个普通数据行
   * @param line 用于保存提取结果的指针
   * @return 找到目标数据行时返回 true，否则返回 false
   */
  bool ExtractResponseLine(const std::string& response, const char* command,
      const char* prefix, std::string* line);

  std::string device_id_;  // 最近一次验证成功的设备型号
};
}  // namespace cpp_bus_driver
