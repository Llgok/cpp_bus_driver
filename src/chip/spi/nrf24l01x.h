/*
 * @Description: Nordic nRF24L01 系列 2.4 GHz 射频收发芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-07-31 10:00:00
 * @LastEditTime: 2026-07-31 10:15:21
 * @License: GPL 3.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Nrf24l01x final : public ChipSpiGuide {
 public:
  // STATUS/CONFIG 中三个可屏蔽中断源的位号。
  enum class IrqSource : uint8_t {
    kMaximumRetransmit = 4,  // 自动重发次数已经耗尽
    kTxDataSent = 5,         // 一包数据已成功发送
    kRxDataReady = 6,        // RX FIFO 收到新负载
  };

  // CONFIG.PRIM_RX 对应的主发射端与主接收端状态。
  enum class OperationMode : uint8_t {
    kPrimaryTransmitter,  // PRIM_RX=0，由 CE 脉冲启动一次发射
    kPrimaryReceiver,     // PRIM_RX=1，CE 保持高电平持续监听
  };

  // CONFIG.PWR_UP 控制的低功耗和晶振工作状态。
  enum class PowerMode : uint8_t {
    kPowerDown,  // PWR_UP=0，寄存器内容和 FIFO 数据保持
    kPowerUp,    // PWR_UP=1，进入 Standby-I 或活动状态
  };

  // RF_PWR 的四档发射功率，枚举值可直接左移到 RF_SETUP。
  enum class OutputPower : uint8_t {
    kMinus18Dbm,  // 最低发射功率
    kMinus12Dbm,  // 中低发射功率
    kMinus6Dbm,   // 中高发射功率
    kZeroDbm,     // 最高发射功率
  };

  // nRF24L01 系列的数据速率；250 kbps 仅适用于 nRF24L01+ 及兼容芯片。
  enum class DataRate : uint8_t {
    k1Mbps,    // RF_DR_LOW=0、RF_DR_HIGH=0
    k2Mbps,    // RF_DR_HIGH=1
    k250Kbps,  // RF_DR_LOW=1
  };

  // CONFIG.EN_CRC 与 CRCO 的组合状态。
  enum class CrcMode : uint8_t {
    kDisabled,  // EN_CRC=0；启用自动应答时硬件仍会强制 CRC
    k8Bit,      // 1 字节空中校验
    k16Bit,     // 2 字节空中校验
  };

  // 六条接收数据管道、发射地址以及批量管道选择。
  enum class Address : uint8_t {
    kPipe0 = 0,        // 首条完整地址管道
    kPipe1 = 1,        // 第二条完整地址管道
    kPipe2 = 2,        // 与 P1 共用高位的管道 2
    kPipe3 = 3,        // 与 P1 共用高位的管道 3
    kPipe4 = 4,        // 与 P1 共用高位的管道 4
    kPipe5 = 5,        // 与 P1 共用高位的管道 5
    kTransmit = 6,     // TX_ADDR 目标
    kAllPipes = 0xFF,  // 管道批量操作标记
  };

  // SETUP_AW 可表达的三种空中地址长度。
  enum class AddressWidth : uint8_t {
    k3Bytes = 3,  // 最短地址
    k4Bytes = 4,  // 四字节地址
    k5Bytes = 5,  // 默认完整地址
  };

  // 阻塞发射流程可区分的五类终止结果。
  enum class TransmitResult : uint8_t {
    kSuccess,            // TX_DS 已置位，数据包发送完成
    kMaximumRetransmit,  // MAX_RT 已置位且残留 TX 负载已清理
    kTimeout,            // 等待窗口内未观察到终止中断
    kInvalidArgument,    // 缓冲区、长度或超时参数不合法
    kBusError,           // SPI、GPIO 或状态清理过程失败
  };

  // Enhanced ShockBurst 的常用默认配置。
  struct Config {
    OperationMode operation_mode =
        OperationMode::kPrimaryTransmitter;              // 默认作为 PTX
    PowerMode power_mode = PowerMode::kPowerDown;        // 配置完成后保持低功耗
    CrcMode crc_mode = CrcMode::k8Bit;                   // 默认采用 1 字节 CRC
    OutputPower output_power = OutputPower::kZeroDbm;    // RF_PWR 上电值
    DataRate data_rate = DataRate::k2Mbps;               // 默认空中速率
    AddressWidth address_width = AddressWidth::k5Bytes;  // 完整地址长度
    uint8_t rf_channel = 2;                        // 2400 MHz + rf_channel
    uint8_t retransmit_count = 3;                  // SETUP_RETR.ARC，范围 0~15
    uint16_t retransmit_delay_us = 250;            // 250~4000 us，步进 250 us
    uint8_t enabled_pipe_mask = 0x03;              // EN_RXADDR 的低 6 位
    uint8_t auto_ack_pipe_mask = 0x3F;             // EN_AA 的低 6 位
    uint8_t dynamic_payload_pipe_mask = 0;         // DYNPD 的低 6 位
    std::array<uint8_t, 6> rx_payload_width = {};  // 各管道静态负载长度
    bool rx_data_ready_irq = true;                 // 允许 RX_DR 拉低 IRQ
    bool tx_data_sent_irq = true;                  // 允许 TX_DS 拉低 IRQ
    bool maximum_retransmit_irq = true;            // 允许 MAX_RT 拉低 IRQ
    bool dynamic_payload_enabled = false;          // FEATURE.EN_DPL
    bool ack_payload_enabled = false;  // FEATURE.EN_ACK_PAY，依赖 DPL_P0
    bool dynamic_ack_enabled = false;  // FEATURE.EN_DYN_ACK
  };

  // 一次读取 STATUS 后解析出的运行状态。
  struct Status {
    bool rx_data_ready = false;       // RX FIFO 中有新负载
    bool tx_data_sent = false;        // 发射成功或 PRX 应答负载发送完成
    bool maximum_retransmit = false;  // 自动重发次数已耗尽
    bool tx_fifo_full = false;        // STATUS.TX_FULL 快照
    uint8_t rx_pipe = 7;              // 0~5 为数据管道，7 表示 RX FIFO 为空
    uint8_t raw = 0;                  // 未加工的 STATUS 字节，便于排查兼容芯片
  };

  static constexpr int32_t kMaximumSpiFrequencyHz =
      10000000;  // 规格书规定的 SCK 上限
  static constexpr std::size_t kMaximumPayloadLength =
      32;  // 单个 Enhanced ShockBurst 负载容量
  static constexpr uint8_t kAllPipeMask = 0x3F;  // P0~P5 位图
  static constexpr uint8_t kAllIrqMask = 0x70;   // RX_DR、TX_DS、MAX_RT
  static constexpr uint8_t kInvalidRxPipe = 7;   // STATUS.RX_P_NO 空值

  /**
   * @brief 创建一个外置 nRF24L01 系列芯片驱动对象。
   * @param bus 使用 SPI 模式 0、MSB first 的 cpp_bus_driver 总线。
   * @param csn 低有效 SPI 片选引脚。
   * @param ce 射频状态机使能引脚。
   * @param irq 可选的低有效中断引脚。
   */
  explicit Nrf24l01x(std::shared_ptr<BusSpiGuide> bus, int32_t csn, int32_t ce,
      int32_t irq = kDefaultValue)
      : ChipSpiGuide(bus, csn), ce_(ce), irq_(irq) {}

  /**
   * @brief 析构 C++ 所有权对象。
   *
   * 调用方在销毁前应显式执行 Deinit，以便决定是否释放共享 SPI 总线。
   */
  ~Nrf24l01x() override = default;

  Nrf24l01x(const Nrf24l01x&) = delete;
  Nrf24l01x& operator=(const Nrf24l01x&) = delete;
  Nrf24l01x(Nrf24l01x&&) = delete;
  Nrf24l01x& operator=(Nrf24l01x&&) = delete;

  /**
   * @brief 初始化 GPIO 和 SPI，探测芯片并应用默认配置。
   * @param frequency_hz SPI 时钟，最大 10 MHz。
   * @return 芯片可读写且全部默认配置成功时返回 true。
   */
  bool Init(int32_t frequency_hz = kMaximumSpiFrequencyHz) override;

  /**
   * @brief 关闭射频、释放 SPI 设备并复位专用 GPIO。
   * @param delete_bus 是否同时请求释放底层共享 SPI 总线。
   * @return 所有资源释放成功时返回 true。
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 一次应用完整 Enhanced ShockBurst 基础配置。
   * @param config 目标射频、管道和功能配置。
   * @return 参数有效且所有寄存器写入成功时返回 true。
   */
  bool Configure(const Config& config);

  /**
   * @brief 通过 RF_CH 寄存器读写探测 SPI 链路上的 nRF24L01 系列芯片。
   * @return 写入值可回读且 STATUS 保留位合法时返回 true。
   */
  bool Probe();

  /**
   * @brief 切换主发射端 PTX 或主接收端 PRX 工作方向。
   * @param mode 需要写入 CONFIG.PRIM_RX 的模式。
   * @return CONFIG 读改写完成时为 true。
   */
  bool SetOperationMode(OperationMode mode);

  /**
   * @brief 更新 PWR_UP，并在首次上电时等待晶振稳定。
   * @param mode 目标供电状态。
   * @return CE 与 CONFIG 都切换成功时为 true。
   */
  bool SetPowerMode(PowerMode mode);

  /**
   * @brief 选择关闭、8 位或 16 位空中 CRC。
   * @param mode CRC 长度设置。
   * @return EN_CRC/CRCO 位写入无误时为 true。
   */
  bool SetCrcMode(CrcMode mode);

  /**
   * @brief 控制指定事件是否允许驱动低电平 IRQ。
   * @param source RX_DR、TX_DS 或 MAX_RT。
   * @param enabled true 表示不屏蔽该中断源。
   * @return 对应 MASK 位更新成功时为 true。
   */
  bool SetIrqMode(IrqSource source, bool enabled);

  /**
   * @brief 查询一个中断源当前是否未被屏蔽。
   * @param source 待检查的状态事件。
   * @param enabled 返回中断使能状态。
   * @return CONFIG 可读取且输出指针有效时为 true。
   */
  bool GetIrqMode(IrqSource source, bool* enabled);

  /**
   * @brief 取出旧 IRQ 标志并用写 1 方式一次清除三项标志。
   * @param flags 返回清除前 STATUS[6:4]。
   * @return STATUS 写事务执行成功时为 true。
   */
  bool GetClearIrqFlags(uint8_t* flags);

  /**
   * @brief 清除全部 IRQ 后重新读取可靠的 RX_P_NO/TX_FULL。
   * @param status 合成后的完整 STATUS 快照。
   * @return 两次 SPI 访问都成功时为 true。
   */
  bool ClearIrqFlagsGetStatus(uint8_t* status);

  /**
   * @brief 单独清除 RX_DR、TX_DS 或 MAX_RT 标志。
   * @param source 需要写 1 清除的事件。
   * @return STATUS 写入完成时为 true。
   */
  bool ClearIrqFlag(IrqSource source);

  /**
   * @brief 使用 NOP 无副作用地读取全部 IRQ 标志。
   * @param flags 返回 STATUS[6:4]。
   * @return 状态字节可用时为 true。
   */
  bool GetIrqFlags(uint8_t* flags);

  /**
   * @brief 打开一条或全部 RX 管道并同步设置自动应答。
   * @param pipe 管道编号或 kAllPipes。
   * @param auto_ack 是否允许该管道自动回复 ACK。
   * @return EN_RXADDR 和 EN_AA 均更新成功时为 true。
   */
  bool OpenPipe(Address pipe, bool auto_ack);

  /**
   * @brief 关闭指定 RX 管道，同时移除其自动应答许可。
   * @param pipe 单条管道或全部管道。
   * @return 两个管道位图保持一致时为 true。
   */
  bool ClosePipe(Address pipe);

  /**
   * @brief 写入 TX、P0/P1 完整地址或 P2~P5 独立低字节。
   * @param address 地址目标。
   * @param data 按 SPI 多字节寄存器顺序存放，最低有效字节在前。
   * @param length 完整地址必须匹配 SETUP_AW，P2~P5 必须为 1。
   * @return 地址长度合法且连续写入成功时为 true。
   */
  bool SetAddress(Address address, const uint8_t* data, std::size_t length);

  /**
   * @brief 读取指定管道或发射端地址。
   * @param address 地址寄存器目标。
   * @param data 接收地址的缓冲区，返回内容同样是最低有效字节在前。
   * @param capacity 缓冲区容量。
   * @param length 返回实际地址字节数。
   * @return 容量充足并完成读取时为 true。
   */
  bool GetAddress(Address address, uint8_t* data, std::size_t capacity,
      std::size_t* length);

  /**
   * @brief 设置自动重发次数与两次发射间隔。
   * @param count 最多重发 15 次。
   * @param delay_us 250~4000 us，必须是 250 us 的整数倍。
   * @return SETUP_RETR 编码有效并写入成功时为 true。
   */
  bool SetAutoRetransmit(uint8_t count, uint16_t delay_us);

  /**
   * @brief 设定 P0、P1 和 TX 共用的地址宽度。
   * @param width 3、4 或 5 字节。
   * @return SETUP_AW 接受该编码时为 true。
   */
  bool SetAddressWidth(AddressWidth width);

  /**
   * @brief 解码当前 SETUP_AW 为实际字节数。
   * @param width 返回 3~5。
   * @return 寄存器不是保留值时为 true。
   */
  bool GetAddressWidth(uint8_t* width);

  /**
   * @brief 设置某条管道的静态接收负载长度。
   * @param pipe RX 管道 0~5。
   * @param width 0~32 字节。
   * @return RX_PW_Px 写入成功时为 true。
   */
  bool SetRxPayloadWidth(uint8_t pipe, uint8_t width);

  /**
   * @brief 读取单条管道的静态负载长度寄存器。
   * @param pipe RX 管道索引。
   * @param width 返回 RX_PW_Px 原始值。
   * @return 管道编号和输出地址有效时为 true。
   */
  bool GetRxPayloadWidth(uint8_t pipe, uint8_t* width);

  /**
   * @brief 合并查询管道启用位与自动应答位。
   * @param pipe 目标管道 0~5。
   * @param status bit0 表示已打开，bit1 表示自动应答。
   * @return EN_RXADDR 与 EN_AA 均读取成功时为 true。
   */
  bool GetPipeStatus(uint8_t pipe, uint8_t* status);

  /**
   * @brief 读取 OBSERVE_TX 的丢包和本次重发计数。
   * @param status 返回未拆分的 OBSERVE_TX。
   * @return 寄存器读取成功时为 true。
   */
  bool GetAutoRetransmitStatus(uint8_t* status);

  /**
   * @brief 获取随写 RF_CH 才会复位的累计丢包计数器。
   * @param count 返回 PLOS_CNT，范围 0~15。
   * @return OBSERVE_TX 可访问时为 true。
   */
  bool GetPacketLostCount(uint8_t* count);

  /**
   * @brief 选择 2.4 GHz ISM 频段内的射频信道。
   * @param channel 0~125，对应 2400~2525 MHz。
   * @return 参数未越界且 RF_CH 写入完成时为 true。
   */
  bool SetRfChannel(uint8_t channel);

  /**
   * @brief 调整芯片内部功率放大器的输出等级。
   * @param power -18、-12、-6 或 0 dBm。
   * @return RF_PWR 位读改写成功时为 true。
   */
  bool SetOutputPower(OutputPower power);

  /**
   * @brief 配置 250 kbps、1 Mbps 或 2 Mbps 空中速率。
   * @param data_rate 目标调制速率。
   * @return RF_DR_LOW/RF_DR_HIGH 组合合法时为 true。
   */
  bool SetDataRate(DataRate data_rate);

  /**
   * @brief 提取 TX FIFO 的 FULL 与 EMPTY 两位状态。
   * @param status 返回压缩后的 FIFO_STATUS[5:4]。
   * @return FIFO_STATUS 读取完成时为 true。
   */
  bool GetTxFifoStatus(uint8_t* status);

  /**
   * @brief 判断三层 TX FIFO 是否没有待发负载。
   * @param empty 返回 TX_EMPTY。
   * @return 能取得 FIFO 状态时为 true。
   */
  bool TxFifoEmpty(bool* empty);

  /**
   * @brief 判断 TX FIFO 是否已占满全部三个槽位。
   * @param full 返回 TX_FULL。
   * @return 输出结果可信时为 true。
   */
  bool TxFifoFull(bool* full);

  /**
   * @brief 获取 RX FIFO 的 FULL 与 EMPTY 原始位。
   * @param status 返回 FIFO_STATUS[1:0]。
   * @return 状态寄存器访问无误时为 true。
   */
  bool GetRxFifoStatus(uint8_t* status);

  /**
   * @brief 读取完整 FIFO_STATUS，供高级队列管理使用。
   * @param status 返回寄存器原值。
   * @return SPI 读取成功时为 true。
   */
  bool GetFifoStatus(uint8_t* status);

  /**
   * @brief 检查 RX FIFO 是否已经没有可读数据。
   * @param empty 返回 RX_EMPTY。
   * @return 读取与解析过程正常时为 true。
   */
  bool RxFifoEmpty(bool* empty);

  /**
   * @brief 检查 RX FIFO 是否填满三个接收槽位。
   * @param full 返回 RX_FULL。
   * @return FIFO 快照有效时为 true。
   */
  bool RxFifoFull(bool* full);

  /**
   * @brief 读取最近一次数据包已经发生的自动重发次数。
   * @param count 返回 ARC_CNT，范围 0~15。
   * @return OBSERVE_TX 低半字节可读取时为 true。
   */
  bool GetTransmitAttempts(uint8_t* count);

  /**
   * @brief 查询最近 RX 时段是否检测到高于 -64 dBm 的信号。
   * @param detected 返回 nRF24L01+ 的 RPD 或 nRF24L01 的 CD 状态位。
   * @return RPD 寄存器读取成功时为 true。
   */
  bool GetCarrierDetect(bool* detected);

  /**
   * @brief 发送 ACTIVATE + 0x73，兼容旧 nRF24L01 和部分兼容芯片。
   * @return SPI 指令发送成功时返回 true。
   *
   * 功能寄存器写入失败时驱动会自动尝试该指令。
   */
  bool ActivateFeatures();

  /**
   * @brief 选择哪些 RX 管道使用动态负载长度。
   * @param pipe_mask DYNPD 的低 6 位。
   * @return 位图未使用保留位且写入成功时为 true。
   */
  bool SetupDynamicPayload(uint8_t pipe_mask);

  /**
   * @brief 打开或关闭 FEATURE.EN_DPL。
   * @param enabled 动态负载总开关。
   * @return 必要时执行 ACTIVATE 后可以回读目标位时为 true。
   */
  bool EnableDynamicPayload(bool enabled);

  /**
   * @brief 控制 PRX 是否允许把用户数据放入 ACK 包。
   * @param enabled ACK Payload 功能状态。
   * @return EN_ACK_PAY 写入并验证成功时为 true。
   */
  bool EnableAckPayload(bool enabled);

  /**
   * @brief 允许单个 TX 负载通过 NO_ACK 指令跳过自动应答。
   * @param enabled 动态无应答功能开关。
   * @return EN_DYN_ACK 可正确保持时为 true。
   */
  bool EnableDynamicAck(bool enabled);

  /**
   * @brief 读取 RX FIFO 顶部负载的实际动态长度。
   * @param width 返回 R_RX_PL_WID 的 6 位结果。
   * @return 指令和随后的一个字节读取成功时为 true。
   */
  bool ReadRxPayloadWidth(uint8_t* width);

  /**
   * @brief 把一个需要正常 ACK 策略的负载压入 TX FIFO。
   * @param payload 待发数据。
   * @param length 1~32 字节。
   * @return W_TX_PAYLOAD 事务成功时为 true。
   */
  bool WriteTxPayload(const uint8_t* payload, std::size_t length);

  /**
   * @brief 写入一包明确不等待 ACK 的动态负载。
   * @param payload 用户数据起始地址。
   * @param length 有效数据长度。
   * @return W_TX_PAYLOAD_NOACK 可以完整送入 FIFO 时为 true。
   */
  bool WriteTxPayloadNoAck(const uint8_t* payload, std::size_t length);

  /**
   * @brief 为指定 PRX 管道预装下一包 ACK Payload。
   * @param pipe 应答所属管道 0~5。
   * @param payload 要随 ACK 返回的数据。
   * @param length 1~32 字节。
   * @return W_ACK_PAYLOAD 指令参数合法时为 true。
   */
  bool WriteAckPayload(
      uint8_t pipe, const uint8_t* payload, std::size_t length);

  /**
   * @brief 从 RX FIFO 读取顶部负载并返回来源管道。
   * @param payload 接收数据缓冲区。
   * @param capacity 缓冲区可写容量。
   * @param length 返回芯片报告的负载长度。
   * @param pipe 可选的 RX_P_NO 输出。
   * @return 长度不超过 32 且数据全部读出时为 true。
   */
  bool ReadRxPayload(uint8_t* payload, std::size_t capacity,
      std::size_t* length, uint8_t* pipe = nullptr);

  /**
   * @brief 从 STATUS.RX_P_NO 判断 FIFO 顶部数据来自哪条管道。
   * @param pipe 返回 0~5，值 7 代表 FIFO 为空。
   * @return NOP 状态读取成功时为 true。
   */
  bool GetRxDataSource(uint8_t* pipe);

  /**
   * @brief 让芯片重新发送 TX FIFO 顶部的同一份负载。
   * @return REUSE_TX_PL 命令送达时为 true。
   */
  bool ReuseTx();

  /**
   * @brief 查询当前是否处于重复使用 TX Payload 状态。
   * @param reused 返回 FIFO_STATUS.TX_REUSE。
   * @return FIFO_STATUS 可用时为 true。
   */
  bool GetReuseTxStatus(bool* reused);

  /**
   * @brief 丢弃 RX FIFO 中的全部负载。
   * @return FLUSH_RX 指令完成时为 true。
   */
  bool FlushRx();

  /**
   * @brief 清除 TX FIFO，包括 MAX_RT 后仍保留的负载。
   * @return FLUSH_TX 指令完成时为 true。
   */
  bool FlushTx();

  /**
   * @brief 发送 NOP 并取得当前 STATUS，不改变 FIFO。
   * @param status 返回 SPI 第一个响应字节。
   * @return 单字节事务成功时为 true。
   */
  bool NoOperation(uint8_t* status);

  /**
   * @brief 控制 RF_SETUP.PLL_LOCK 测试位。
   * @param locked 是否强制保持 PLL 锁定。
   * @return 测试位更新成功时为 true。
   */
  bool SetPllMode(bool locked);

  /**
   * @brief 提供原始 nRF24L01 的 LNA_HCURR 兼容入口。
   * @param high_current 旧芯片 LNA 高电流模式。
   * @return high_current 为 false 且 RF_SETUP bit0 清零成功时返回 true。
   *
   * nRF24L01+ 官方实现将该位保留为 0，因此不接受 high_current=true。
   */
  bool SetLnaGain(bool high_current);

  /**
   * @brief 开关连续载波测试输出。
   * @param enabled 是否置位 CONT_WAVE。
   * @return RF_SETUP 测试位修改成功时为 true。
   */
  bool EnableContinuousWave(bool enabled);

  /**
   * @brief 保持 CE 为低并让芯片进入 Standby-I。
   * @return PWR_UP 置位且启动等待结束时为 true。
   */
  bool Standby();

  /**
   * @brief 拉低 CE 后进入寄存器与 FIFO 保持的最低功耗模式。
   * @return CONFIG.PWR_UP 清零成功时为 true。
   */
  bool PowerDown();

  /**
   * @brief 配置 PRX、等待射频稳定并把 CE 保持为高。
   * @return 芯片进入持续监听状态时为 true。
   */
  bool StartReceive();

  /**
   * @brief 结束持续接收并等待状态机回到 Standby-I。
   * @return CE 可可靠拉低时为 true。
   */
  bool StopReceive();

  /**
   * @brief 产生一次符合规格书下限的 CE 发射脉冲。
   * @param high_time_us CE 高电平时间，不得少于 10 us。
   * @return 两次 GPIO 翻转都成功时为 true。
   */
  bool PulseCe(uint32_t high_time_us = 15);

  /**
   * @brief 读取低有效 IRQ 引脚的即时电平。
   * @param active 返回是否存在未屏蔽中断。
   * @return 构造时提供了 IRQ 引脚才为 true。
   */
  bool IrqActive(bool* active);

  /**
   * @brief 将 STATUS 原始位解析为易读结构体。
   * @param status 返回 IRQ、管道和 TX FIFO 状态。
   * @return NOP 事务成功时为 true。
   */
  bool ReadStatus(Status* status);

  /**
   * @brief 阻塞发送一包数据并区分成功、MAX_RT、超时和总线错误。
   * @param payload 需要压入 TX FIFO 的数据。
   * @param length 负载长度，范围 1~32。
   * @param no_ack true 时使用 W_TX_PAYLOAD_NOACK，并要求预先启用 EN_DYN_ACK。
   * @param timeout_ms 等待 TX_DS 或 MAX_RT 的最长时间。
   * @return 细分后的 TransmitResult。
   */
  TransmitResult Transmit(const uint8_t* payload, std::size_t length,
      bool no_ack = false, uint32_t timeout_ms = 100);

  /**
   * @brief 等待一包数据并读取有效负载。
   * @param payload 接收缓冲区。
   * @param capacity 缓冲区容量，至少 32 字节可覆盖全部负载。
   * @param length 实际负载长度。
   * @param pipe 命中管道，可传入 nullptr。
   * @param timeout_ms 等待超时。
   * @param keep_listening 读取后是否保持 PRX 接收状态。
   */
  bool Receive(uint8_t* payload, std::size_t capacity, std::size_t* length,
      uint8_t* pipe = nullptr, uint32_t timeout_ms = 100,
      bool keep_listening = true);

  /** @brief 返回 Init 是否已完成全部探测与配置。 */
  bool initialized() const { return initialized_; }

  /** @brief 返回缓存的 PWR_UP 状态，避免额外占用 SPI。 */
  bool powered_up() const { return powered_up_; }

  /** @brief 指示 CE 是否由 StartReceive 保持为高。 */
  bool receiving() const { return receiving_; }

  /** @brief 获取实际注册到 SPI 设备的时钟频率。 */
  int32_t spi_frequency_hz() const { return spi_frequency_hz_; }

  /** @brief 只读访问最近一次成功应用的高层配置。 */
  const Config& config() const { return config_; }

 private:
  // nRF24L01 系列 SPI 可访问寄存器。
  enum class Cmd : uint8_t {
    kConfig = 0x00,                    // 中断、CRC、电源和角色配置
    kEnableAutoAcknowledgment = 0x01,  // 六条管道的自动应答位图
    kEnableRxAddress = 0x02,           // 接收管道启用开关
    kSetupAddressWidth = 0x03,         // P0、P1、TX 共用地址宽度
    kSetupRetransmission = 0x04,       // 自动重发间隔与次数
    kRfChannel = 0x05,                 // 2.4 GHz 信道偏移
    kRfSetup = 0x06,                   // 速率、功率和射频测试位
    kStatus = 0x07,                    // IRQ、来源管道及 TX 满状态
    kObserveTx = 0x08,                 // 丢包累计与本包重发计数
    kReceivedPowerDetector = 0x09,     // 接收功率门限检测结果
    kRxAddressPipe0 = 0x0A,            // 管道 0 完整接收地址
    kRxAddressPipe1 = 0x0B,            // 管道 1 完整接收地址
    kRxAddressPipe2 = 0x0C,            // 管道 2 独立最低地址字节
    kRxAddressPipe3 = 0x0D,            // 管道 3 独立最低地址字节
    kRxAddressPipe4 = 0x0E,            // 管道 4 独立最低地址字节
    kRxAddressPipe5 = 0x0F,            // 管道 5 独立最低地址字节
    kTxAddress = 0x10,                 // 发射及自动应答接收地址
    kRxPayloadWidthPipe0 = 0x11,       // 管道 0 静态负载长度
    kRxPayloadWidthPipe1 = 0x12,       // 管道 1 静态负载长度
    kRxPayloadWidthPipe2 = 0x13,       // 管道 2 静态负载长度
    kRxPayloadWidthPipe3 = 0x14,       // 管道 3 静态负载长度
    kRxPayloadWidthPipe4 = 0x15,       // 管道 4 静态负载长度
    kRxPayloadWidthPipe5 = 0x16,       // 管道 5 静态负载长度
    kFifoStatus = 0x17,                // RX/TX FIFO 空、满和复用状态
    kDynamicPayload = 0x1C,            // 各管道动态长度许可位图
    kFeature = 0x1D,                   // DPL、ACK Payload、NO_ACK 总开关
  };

  // 数据手册第 8 章定义的 SPI 指令。
  enum class SpiCmd : uint8_t {
    kReadRxPayloadWidth = 0x60,   // 查询 FIFO 顶部动态长度
    kReadRxPayload = 0x61,        // 弹出并读取一个接收负载
    kWriteTxPayload = 0xA0,       // 压入需要正常 ACK 的发送负载
    kFlushTx = 0xE1,              // 清空全部待发数据
    kFlushRx = 0xE2,              // 丢弃全部已收数据
    kReuseTxPayload = 0xE3,       // 复用上次发送的负载
    kActivateFeatures = 0x50,     // 切换 FEATURE 寄存器可写状态
    kWriteAckPayload = 0xA8,      // 为指定管道准备应答负载
    kWriteTxPayloadNoAck = 0xB0,  // 压入不请求 ACK 的单次负载
    kNoOperation = 0xFF,          // 只读取 STATUS
  };

  static constexpr int32_t kDefaultValue =
      cpp_bus_driver::kDefaultValue;  // 与底层总线保持一致的未配置标记
  static constexpr uint8_t kRegisterMask = 0x1F;  // SPI 指令中的地址范围
  static constexpr uint8_t kWriteRegisterCommand = 0x20;   // W_REGISTER 前缀
  static constexpr uint8_t kFeatureActivationData = 0x73;  // ACTIVATE 固定数据
  static constexpr uint8_t kDynamicPayloadFeatureMask = 0x04;  // EN_DPL
  static constexpr uint8_t kAckPayloadFeatureMask = 0x02;      // EN_ACK_PAY
  static constexpr uint8_t kDynamicAckFeatureMask = 0x01;      // EN_DYN_ACK

  /**
   * @brief 将地址目标映射为 RX_ADDR_Px 或 TX_ADDR 寄存器。
   * @param address 已验证有效的 P0~P5 或 TX。
   * @return 对应寄存器命令。
   */
  static Cmd CmdForAddress(Address address);

  /**
   * @brief 根据管道号计算 RX_PW_Px 寄存器。
   * @param pipe 已限制在 0~5 的索引。
   * @return 对应静态负载长度寄存器命令。
   */
  static Cmd CmdForPayloadWidth(uint8_t pipe);

  /**
   * @brief 读取一个寄存器并可选返回同一事务的 STATUS。
   * @param address 寄存器地址。
   * @param value 返回第二个 SPI 字节。
   * @param status 可选的首字节状态快照。
   * @return R_REGISTER 访问成功时为 true。
   */
  bool ReadRegister(Cmd cmd, uint8_t* value, uint8_t* status = nullptr);

  /**
   * @brief 写入一个寄存器并可选返回同一事务的 STATUS。
   * @param address 目标寄存器。
   * @param value 新的 8 位内容。
   * @param status 可选的写入前状态。
   * @return W_REGISTER 命令执行成功时为 true。
   */
  bool WriteRegister(Cmd cmd, uint8_t value, uint8_t* status = nullptr);

  /**
   * @brief 连续读取多字节地址寄存器。
   * @param address 起始寄存器。
   * @param data 保存回读内容的数组。
   * @param length 需要继续产生时钟的字节数。
   * @param status 可选 STATUS 输出。
   * @return 长度在内部事务缓冲区范围内且传输完成时为 true。
   */
  bool ReadBuffer(Cmd cmd, uint8_t* data, std::size_t length,
      uint8_t* status = nullptr);

  /**
   * @brief 向多字节寄存器连续写入地址或其他原始数据。
   * @param address 写入起点。
   * @param data 连续发送内容。
   * @param length 数据字节数。
   * @param status 可选的命令响应状态。
   * @return CSN 包围的完整突发事务成功时为 true。
   */
  bool WriteBuffer(Cmd cmd, const uint8_t* data, std::size_t length,
      uint8_t* status = nullptr);

  /**
   * @brief 执行单字节 SPI 事务。
   * @param value MOSI 上发送的指令或数据。
   * @param response 同时从 MISO 收到的字节。
   * @return CSN 拉低、交换和释放均成功时为 true。
   */
  bool TransferByte(uint8_t value, uint8_t* response);

  /**
   * @brief 构造一笔命令加最多 32 字节数据的全双工 SPI 事务。
   * @param command 首个 MOSI 命令字节。
   * @param write_data 可为空的后续发送数据。
   * @param read_data 可为空的后续接收缓冲区。
   * @param length 命令之后的字节数。
   * @param status 可选的首个 MISO 字节输出。
   * @return 总线交换与 CSN 收尾都成功时为 true。
   */
  bool Exchange(uint8_t command, const uint8_t* write_data, uint8_t* read_data,
      std::size_t length, uint8_t* status);

  /**
   * @brief 发送不附带参数的 FIFO 或状态指令。
   * @param command 单字节命令。
   * @param status 可选状态输出。
   * @return 单字节 Exchange 成功时为 true。
   */
  bool ExecuteCommand(SpiCmd command, uint8_t* status = nullptr);

  /**
   * @brief 为负载类指令附加一段连续写数据。
   * @param command W_TX_PAYLOAD 等命令。
   * @param data 发送内容。
   * @param length 内容长度。
   * @param status 可选状态快照。
   * @return 参数有效且整段写完时为 true。
   */
  bool WriteCommand(SpiCmd command, const uint8_t* data, std::size_t length,
      uint8_t* status = nullptr);

  /**
   * @brief 在命令之后发送 NOP 时钟并收集响应。
   * @param command R_RX_PAYLOAD 等读命令。
   * @param data 返回数据。
   * @param length 希望读取的字节数。
   * @param status 可选 STATUS。
   * @return 读事务完成时为 true。
   */
  bool ReadCommand(SpiCmd command, uint8_t* data, std::size_t length,
      uint8_t* status = nullptr);

  /**
   * @brief 以读改写方式只更新寄存器中的指定掩码。
   * @param address 目标寄存器。
   * @param mask 允许改变的位。
   * @param value 掩码范围内的新值。
   * @return 读取失败或写入失败时为 false。
   */
  bool UpdateRegisterBits(Cmd cmd, uint8_t mask, uint8_t value);

  /**
   * @brief 修改 FEATURE 位并对锁定功能自动执行一次 ACTIVATE。
   * @param mask FEATURE 中的单项功能掩码。
   * @param enabled 期望状态。
   * @return 最终回读值与期望一致时为 true。
   */
  bool UpdateFeatureBits(uint8_t mask, bool enabled);

  /**
   * @brief 在访问硬件前检查配置间的范围和依赖关系。
   * @param config 待验证配置。
   * @return 所有字段都能被驱动表达时为 true。
   */
  bool ValidateConfig(const Config& config) const;

  /**
   * @brief 统一维护 CE 输出，避免高层流程直接操作 GPIO。
   * @param enabled 目标逻辑电平。
   * @return GPIO 写入成功时为 true。
   */
  bool SetCe(bool enabled);

  /**
   * @brief 记录初始化错误并回滚总线及三个专用 GPIO。
   * @param reason 写入日志的具体失败阶段。
   * @return 固定返回 false，便于 Init 直接返回。
   */
  bool FailInitialization(const char* reason);

  /**
   * @brief 把 STATUS 位图拆分到公开状态结构。
   * @param raw 芯片返回的原始字节。
   * @param status 已验证非空的目标结构。
   */
  static void DecodeStatus(uint8_t raw, Status* status);

  int32_t ce_ = kDefaultValue;                // 控制 RX/TX 状态机的 Chip Enable
  int32_t irq_ = kDefaultValue;               // 可选的低有效中断输入
  int32_t spi_frequency_hz_ = kDefaultValue;  // 当前 SPI 设备时钟
  Config config_{};                           // 最近成功写入芯片的高层配置镜像
  bool bus_initialized_ = false;              // SPI 子设备已注册
  bool initialized_ = false;                  // 探测和默认配置均已完成
  bool powered_up_ = false;                   // CONFIG.PWR_UP 软件镜像
  bool receiving_ = false;                    // CE 正由驱动保持高电平
};

}  // namespace cpp_bus_driver
