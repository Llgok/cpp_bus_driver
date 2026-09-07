/*
 * @Description: S023MSAFJF10111E1 显示面板辅助控制驱动接口
 * @Author: LILYGO_L
 * @Date: 2026-03-14 11:11:19
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "chip/chip_base.h"

namespace cpp_bus_driver {

class S023msafjf10111e1 final : public I2cChipBase {
 public:
  // ADDR0 为低电平时的 7 位 I2C 地址。
  static constexpr uint8_t kI2cAddressAddr0Low = 0x50;
  // ADDR0 为高电平时的 7 位 I2C 地址。
  static constexpr uint8_t kI2cAddressAddr0High = 0x54;
  // 默认 RGB 亮度增益，Q1.8 格式，0x100 对应 1 倍。
  static constexpr uint16_t kDefaultBrightnessGain = 0x100;
  // 9 位 RGB 增益最大编码，0x1FF 对应 511/256 倍。
  static constexpr uint16_t kMaxBrightnessGain = 0x1FF;

  // RSMX/RSMY 镜像组合，置位表示对应方向镜像。
  enum class MirrorMode {
    kOff,                 // 不镜像。
    kHorizontal,          // 水平镜像。
    kVertical,            // 垂直镜像。
    kHorizontalVertical,  // 同时水平、垂直镜像。
  };

  // 手册第 6.3 节列出的 BIST 图案寄存器完整编码。
  enum class BistPattern : uint8_t {
    kColorBar = 0x80,  // 彩条。
    kBlank = 0x82,     // 空白画面。
    kWhite = 0x92,     // 全白。
    kRed = 0xA2,       // 全红。
    kGreen = 0xB2,     // 全绿。
    kBlue = 0xC2,      // 全蓝。
  };

  /**
   * @brief 构造面板辅助控制对象，不初始化 I2C 或 MIPI 像素传输总线。
   * @param bus 该面板 I2C 地址专用的总线设备包装。
   * @param address 7 位 I2C 地址，ADDR0 高电平为 0x54，低电平为 0x50。
   * @param rst 面板复位 GPIO；kPinNotConnected 表示不由此驱动控制复位。
   * @return 无返回值，需调用 Init() 初始化辅助控制接口。
   * @note 寄存器协议依据 S023MSAFJF10111E1 v1.0 第 5.2、6.3 节。
   * 同一面板的配置调用须串行；多次寄存器写入失败时不保证自动回滚已写部分。
   */
  explicit S023msafjf10111e1(std::shared_ptr<I2cBusBase> bus,
      int16_t address = kI2cAddressAddr0High, int32_t rst = kPinNotConnected)
      : I2cChipBase(bus, address), rst_(rst) {}

  /**
   * @brief 初始化 I2C 并探测面板地址，接有复位 GPIO 时先执行硬件复位。
   * @param freq_hz I2C 频率，单位 Hz，必须大于 0，默认 100000。
   * @return 复位及总线探测成功返回 true；空总线、频率无效或硬件操作失败返回
   * false。
   * @note 不负责面板供电和 MIPI 初始化；应由设备层先满足面板电源时序。
   */
  bool Init(int32_t freq_hz = kDefaultFrequencyHz) override;

  /**
   * @brief 尝试退出本对象开启的 BIST，解除总线设备并释放复位 GPIO 配置。
   * @param delete_bus true 请求释放自有物理总线，false 仅解除设备并保留总线。
   * @return 所有操作成功返回 true；任一恢复或资源释放操作失败返回 false。
   * @note 即使 BIST 恢复失败，也继续尝试总线和 GPIO 资源释放。
   */
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 拉低复位 GPIO 10 ms 后拉高并等待 10 ms，恢复面板硬件默认配置。
   * @return GPIO 操作成功返回 true；未接复位脚或 GPIO 操作失败返回 false。
   * @note 默认配置包含 MIPI RGB888 输入；复位会丢弃本对象保存的 BIST 恢复状态。
   */
  bool Reset();

  /**
   * @brief 设置 RSMX/RSMY 镜像位，保留同寄存器内的像素偏移和保留位。
   * @param mode 不镜像、水平、垂直或水平与垂直同时镜像。
   * @return 读写成功返回 true；枚举无效或总线访问失败返回 false。
   */
  bool SetMirror(MirrorMode mode);

  /**
   * @brief 读取 RSMX/RSMY 并转换为镜像组合。
   * @param mode 保存镜像模式的非空指针；失败时指向的值保持不变。
   * @return 读取成功返回 true；空指针或总线访问失败返回 false。
   */
  bool GetMirror(MirrorMode* mode);

  /**
   * @brief 设置相对面板中心的 orbit 像素偏移，保留镜像位和保留位。
   * @param x 水平偏移，单位像素，范围 [-4, 4]，正值向右。
   * @param y 垂直偏移，单位像素，范围 [-10, 10]，正值向下。
   * @return 读写成功返回 true；偏移越界或总线访问失败返回 false。
   * @note 参数越界时不访问总线，零偏移对应手册编码 X=4、Y=10。
   */
  bool SetPixelShift(int8_t x, int8_t y);

  /**
   * @brief 读取 orbit 编码并减去中心值，返回相对中心的像素偏移。
   * @param x 保存水平偏移的非空指针，范围 [-4, 4] 像素。
   * @param y 保存垂直偏移的非空指针，范围 [-10, 10] 像素，应与 x 指向不同变量。
   * @return 读取及编码校验成功返回 true；空指针、编码越界或总线失败返回 false。
   * @note 失败时两个输出均保持原值。
   */
  bool GetPixelShift(int8_t* x, int8_t* y);

  /**
   * @brief 启用当前 BIST 图案，或恢复本对象进入 BIST 前的控制寄存器值。
   * @param enabled true 保存快照并开启 BIST，false 尝试恢复快照。
   * @return 操作成功返回 true；保存或写入控制寄存器失败返回 false。
   * @note false 在本对象没有待恢复快照时直接成功，不探测外部设置的 BIST 状态。
   * 部分写入失败仍保留快照，便于重试恢复；退出不复位镜像、偏移或亮度增益。
   * @note 手册未提供完整的视频模式恢复序列，恢复快照成功不保证视频输出恢复。
   * 已验证的切换流程为硬件复位后进入目标模式，并重新配置所需显示参数。
   */
  bool SetBistEnabled(bool enabled);

  /**
   * @brief 自动开启 BIST 并选择手册列出的自检图案。
   * @param pattern 彩条、空白、白、红、绿或蓝图案。
   * @return 启用并写入图案成功返回 true；编码无效或总线访问失败返回 false。
   * @note 图案写入失败时 BIST 可能已经开启，可调用 SetBistEnabled(false) 恢复。
   * @note BIST 期间不要调用 SetBrightnessGain，它会改写公共控制寄存器并配置亮度路径。
   */
  bool SetBistPattern(BistPattern pattern);

  /**
   * @brief 读取 BIST 图案编码并校验是否属于手册列出的六种图案。
   * @param pattern 保存图案的非空指针；失败时指向的值保持不变。
   * @return 读取成功且编码有效返回 true；空指针、未知编码或总线失败返回 false。
   * @note 读取图案设置不代表 BIST 当前正在输出。
   */
  bool GetBistPattern(BistPattern* pattern);

  /**
   * @brief 尝试恢复 BIST 控制快照并设置 RGB 亮度增益，保留增益高字节的其他位。
   * @param gain Q1.8 格式的增益编码，范围 [0, 511]，实际乘数为 gain/256。
   * @return 全部配置写入成功返回 true；增益越界、BIST 恢复或总线操作失败返回
   * false。
   * @note 256 为 1 倍默认增益，0 只显示黑色而非关电。越界时不访问总线；亮度与
   * BIST 共用 0x2401，调用会配置亮度控制路径，失败时可能已有部分设置生效。
   * @note 亮度路径按手册写入 0x2401=0x1C、0x280A=0x03，不适用于保持 BIST 输出。
   * 手册未定义公共控制寄存器各位的含义，不能通过读改写保证两种配置兼容。
   */
  bool SetBrightnessGain(uint16_t gain);

  /**
   * @brief 读取亮度增益低字节和高字节最低位，合成为 9 位增益编码。
   * @param gain 保存 Q1.8 编码的非空指针，范围 [0, 511]；失败时保持原值。
   * @return 读取成功返回 true；空指针或总线访问失败返回 false。
   */
  bool GetBrightnessGain(uint16_t* gain);

 private:
  // 默认 I2C 控制接口频率，单位 Hz。
  static constexpr int32_t kDefaultFrequencyHz = 100000;

  // 使用手册原始地址；低字节优先的转换仅在总线读写函数中执行。
  enum class Register : uint16_t {
    kConfigurationControl = 0x2401,  // 视频、BIST 和亮度路径公共控制。
    kBrightnessGainLow = 0x2803,  // 9 位增益的低 8 位。
    kBrightnessGainHigh = 0x2804,  // bit0 保存增益最高位，其他位需保留。
    kBrightnessControl = 0x280A,           // 亮度控制配置。
    kBistPattern = 0x2850,                 // 自检图案完整编码。
    kBistControl = 0x2851,                 // 自检输出控制。
    kHorizontalMirrorPixelShift = 0x2C03,  // 水平镜像与水平 orbit 偏移。
    kVerticalMirrorPixelShift = 0x2C04,  // 垂直镜像与垂直 orbit 偏移。
  };

  /**
   * @brief 按手册低字节优先的 16 位地址格式读取连续寄存器，使用重复 START。
   * @param reg 首个寄存器的手册原始地址。
   * @param data 非空接收缓冲区，至少可容纳 length 字节。
   * @param length 读取字节数，必须大于 0。
   * @return 读取成功返回 true；空总线、参数无效或总线访问失败返回 false。
   * @note 原始缓冲区不提供失败不变保证，公共 getter 使用局部缓冲后再更新输出。
   */
  bool ReadRegisters(Register reg, uint8_t* data, size_t length);

  /**
   * @brief 拼接低地址字节、高地址字节和数据，写入连续配置寄存器。
   * @param reg 首个寄存器的手册原始地址。
   * @param data 非空待写缓冲区，至少包含 length 字节。
   * @param length 写入字节数，仅支持 1 或 2，覆盖当前已公开的连续配置字段。
   * @return 写入成功返回 true；空总线、参数无效或总线访问失败返回 false。
   */
  bool WriteRegisters(Register reg, const uint8_t* data, size_t length);

  /**
   * @brief 通过连续写入辅助函数写入单个寄存器字节。
   * @param reg 寄存器的手册原始地址。
   * @param value 要写入的完整字节。
   * @return 写入成功返回 true；总线为空或总线访问失败返回 false。
   */
  bool WriteRegister(Register reg, uint8_t value);

  // 面板复位 GPIO，kPinNotConnected 表示未连接。
  int32_t rst_;
  // 已保存进入 BIST 前的状态，直到完整恢复或硬件复位才清除。
  bool bist_restore_pending_ = false;
  // 进入 BIST 前 0x2401 的控制快照。
  uint8_t saved_configuration_control_ = 0;
  // 进入 BIST 前 0x2851 的控制快照。
  uint8_t saved_bist_control_ = 0;
};

}  // namespace cpp_bus_driver
