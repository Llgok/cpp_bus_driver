/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-05-15 17:00:38
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {
class L76k final : public ChipUartGuide, public GnssParser {
 public:
  // 更新频率（定位频率）
  enum class UpdateFreq {
    kFreq1Hz,
    kFreq2Hz,
    kFreq5Hz,
  };

  // 波特率
  enum class BaudRate {
    kBr4800Bps,
    kBr9600Bps,
    kBr19200Bps,
    kBr38400Bps,
    kBr57600Bps,
    kBr115200Bps,
  };

  // 重启模式
  enum class RestartMode {
    kHotStart,
    kWarmStart,
    kColdStart,
    kColdStartFactoryReset,
  };

  // kGnss 星系
  enum class GnssConstellation {
    kGps,     // 美国全球定位系统
    kBeidou,  // 中国的全球卫星导航系统
    kGpsBeidou,
    kGlonass,  // 俄罗斯的全球卫星导航系统
    kGpsGlonass,
    kBeidouGlonass,
    kGpsBeidouGlonass,
  };

  // NMEA 标准输出语句类型，对应 CASIC CFG-MSG 中的 NMEA-* 消息号
  enum class NmeaSentence {
    kGga,
    kGll,
    kGsa,
    kGsv,
    kRmc,
    kVtg,
    kZda,
  };

  // CASIC CFG-RST 复位方式
  enum class CasicResetMode {
    kImmediateHardwareReset = 0,
    kSoftwareReset = 1,
    kSoftwareResetGpsOnly = 2,
    kHardwareResetAfterPowerOff = 4,
  };

  // PCAS03 输出配置，取值 0 表示关闭，1~9 表示每 N 次定位输出一次，-1
  // 表示保持原配置
  struct NmeaOutputConfig {
    int8_t rmc = -1;
    int8_t gga = -1;
    int8_t gll = -1;
    int8_t gsv = -1;
    int8_t gsa = -1;
    int8_t vtg = -1;
    int8_t zda = -1;
    int8_t ant = -1;
  };

  // CASIC CFG-PRT 串口配置
  struct CasicPortConfig {
    uint8_t port_id = 0xFF;
    uint8_t proto_mask = 0x33;
    uint16_t mode = 0x08C0;
    uint32_t baud_rate = 9600;
  };

  explicit L76k(std::shared_ptr<BusUartGuide> bus, const int32_t wake_up,
      const int32_t rst = kDefaultValue)
      : ChipUartGuide(bus), wake_up_(wake_up), rst_(rst) {}

  explicit L76k(std::shared_ptr<BusUartGuide> bus,
      const std::function<bool(bool)>& wake_up_callback,
      const int32_t rst = kDefaultValue)
      : ChipUartGuide(bus), wake_up_callback_(wake_up_callback), rst_(rst) {}

  bool Init(int32_t baud_rate = 9600) override;
  bool Deinit() override;

  bool GetDeviceId(size_t* search_index = nullptr);

  /**
   * @brief 启动睡眠
   * @param enable [true]：进入睡眠，[false]：退出睡眠
   * @return
   * @Date 2025-02-14 16:18:00
   */
  bool Sleep(bool enable);

  /**
   * @brief 直接读取数据
   * @param *data 读取数据的指针
   * @param length 要读取数据的长度
   * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
   * @Date 2025-02-13 18:04:11
   */
  uint32_t ReadData(uint8_t* data, uint32_t length = 0);

  /**
   * @brief 获取接收缓存数据的长度
   * @return
   * @Date 2025-02-13 18:22:46
   */
  size_t GetRxBufferLength();

  /**
   * @brief 清除接收缓存中的所有数据
   * @return
   * @Date 2025-02-13 18:22:46
   */
  bool ClearRxBufferData();

  /**
   * @brief 获取信息数据
   * @param &data 获取数据的指针
   * @param *length 获取长度的指针
   * @param max_length 获取数据的最大长度
   * @param timeout_count 超时计数
   * @return
   * @Date 2025-03-24 10:03:31
   */
  bool GetInfoData(std::unique_ptr<uint8_t[]>& data, uint32_t* length,
      uint32_t max_length = kMaxReceiveSize,
      uint8_t timeout_count = kGetInformationTimeoutCount);

  /**
   * @brief 获取当前定位更新时间间隔
   * @return 定位更新时间间隔，单位为毫秒
   * @Date 2026-05-16 11:20:00
   */
  uint16_t update_interval_ms() const { return update_interval_ms_; }

  /**
   * @brief 设置定位频率
   * @param freq 使用 UpdateFreq::配置，频率设定
   * @return
   * @Date 2025-02-14 18:18:57
   */
  bool SetUpdateFrequency(UpdateFreq freq);

  /**
   * @brief 设置模块和系统的波特率
   * @param baud_rate 使用 BaudRate::配置，波特率设定
   * @return
   * @Date 2025-02-17 13:45:17
   */
  bool SetBaudRate(BaudRate baud_rate);

  /**
   * @brief 获取系统的波特率
   * @return 波特率数据
   * @Date 2025-02-17 13:45:58
   */
  uint32_t GetBaudRate();

  /**
   * @brief 设置重启模式
   * @param mode 使用 RestartMode::配置
   * @return
   * @Date 2025-02-17 14:27:01
   */
  bool SetRestartMode(RestartMode mode);

  /**
   * @brief 设置GNSS的星系
   * @param constellation 使用 GnssConstellation::配置
   * @return
   * @Date 2025-02-17 14:45:15
   */
  bool SetGnssConstellation(GnssConstellation constellation);

  /**
   * @brief 通过 PCAS03 配置各类 NMEA 语句输出频率
   * @param config 输出频率配置，0 关闭，1~9 每 N 次定位输出一次，-1 保持原配置
   * @return
   */
  bool SetNmeaOutputConfig(const NmeaOutputConfig& config);

  /**
   * @brief 通过 CASIC CFG-MSG 配置单个 NMEA 语句输出频率
   * @param sentence NMEA 语句类型
   * @param rate 0 关闭，1~9 每 N 次定位输出一次，0xFFFF 立即查询输出一次
   * @return
   */
  bool SetNmeaSentenceOutput(NmeaSentence sentence, uint16_t rate);

  /**
   * @brief 通过 CASIC CFG-MSG 查询当前 NMEA 输出配置
   * @return 写入查询命令成功返回 true
   */
  bool QueryNmeaSentenceOutput();

  /**
   * @brief 通过 CASIC CFG-PRT 配置串口协议掩码、工作模式和波特率
   * @param config 串口配置
   * @return
   */
  bool SetCasicPortConfig(const CasicPortConfig& config);

  /**
   * @brief 通过 CASIC CFG-PRT 查询当前串口配置
   * @return 写入查询命令成功返回 true
   */
  bool QueryCasicPortConfig();

  /**
   * @brief 通过 CASIC CFG-PRT 设置模块波特率，并同步更新本地 UART 波特率
   * @param baud_rate 目标波特率
   * @return 设置成功返回 true
   */
  bool SetCasicBaudRate(BaudRate baud_rate);

  /**
   * @brief 通过 CASIC CFG-RST 重启模块或清理备份 RAM 数据
   * @param start_mode 启动模式
   * @param nav_bbr_mask 需要清理的备份 RAM 掩码，默认不清理
   * @param reset_mode 复位方式
   * @return
   */
  bool SetCasicRestartMode(RestartMode start_mode, uint16_t nav_bbr_mask = 0,
      CasicResetMode reset_mode = CasicResetMode::kImmediateHardwareReset);

  /**
   * @brief 通过 CASIC CFG-RATE 设置定位时间间隔
   * @param interval_ms 定位间隔，只支持 200、500、1000 ms
   * @return
   */
  bool SetCasicUpdateInterval(uint16_t interval_ms);

  /**
   * @brief 通过 CASIC CFG-RATE 查询当前定位时间间隔
   * @return
   */
  bool QueryCasicUpdateInterval();

 private:
  enum class Cmd {
    kRoDeviceId = 0x00,

  };

  static constexpr uint8_t kGetInformationTimeoutCount = 3;  // 获取信息超时计数
  static constexpr uint16_t kMaxReceiveSize = 1024 * 2;      // 最大接收尺寸

  /**
   * @brief 生成 PCAS/NMEA XOR 校验并写入完整命令
   * @param body 不包含 '$'、'*'、校验值和换行符的命令主体
   * @return 写入成功返回 true
   */
  bool WritePcasCommand(const std::string& body);

  /**
   * @brief 生成 CASIC 帧头和 32 位累加校验并写入完整命令
   * @param class_id CASIC 消息类别
   * @param message_id CASIC 消息 ID
   * @param payload 消息负载，长度必须为 4 字节对齐
   * @return 写入成功返回 true
   */
  bool WriteCasicCommand(uint8_t class_id, uint8_t message_id,
      const std::vector<uint8_t>& payload);

  /**
   * @brief 将枚举波特率转换为实际数值
   * @param baud_rate 波特率枚举
   * @return 实际波特率，非法枚举返回 0
   */
  uint32_t BaudRateToValue(BaudRate baud_rate);

  int32_t wake_up_ = kDefaultValue;
  std::function<bool(bool)> wake_up_callback_ = nullptr;
  int32_t rst_;
  uint16_t update_interval_ms_ = 1000;  // 默认更新间隔为 1000ms（1Hz）
};
}  // namespace cpp_bus_driver
