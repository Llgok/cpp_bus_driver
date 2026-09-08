/*
 * @Description: S023MSAFJF10111E1 显示面板辅助控制驱动实现
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @License: GPL 3.0
 */
#include "chip/i2c/s023msafjf10111e1.h"

namespace cpp_bus_driver {
namespace {

// 0x2C03 的 RSMX 水平镜像位。
constexpr uint8_t kHorizontalMirrorMask = 0x10;
// 0x2C04 的 RSMY 垂直镜像位。
constexpr uint8_t kVerticalMirrorMask = 0x80;
// 0x2C03 的水平 orbit 编码字段。
constexpr uint8_t kPixelShiftXMask = 0x0F;
// 0x2C04 的垂直 orbit 编码字段。
constexpr uint8_t kPixelShiftYMask = 0x1F;
// 水平零偏移对应的寄存器编码。
constexpr int kPixelShiftXCenter = 4;
// 垂直零偏移对应的寄存器编码。
constexpr int kPixelShiftYCenter = 10;

/**
 * @brief 校验图案值是否属于手册列出的六个 BIST 编码。
 * @param pattern 待检查的自检图案。
 * @return 彩条、空白、白、红、绿或蓝返回 true，其他编码返回 false。
 */
bool IsValidBistPattern(S023msafjf10111e1::BistPattern pattern) {
  switch (pattern) {
    case S023msafjf10111e1::BistPattern::kColorBar:
    case S023msafjf10111e1::BistPattern::kBlank:
    case S023msafjf10111e1::BistPattern::kWhite:
    case S023msafjf10111e1::BistPattern::kRed:
    case S023msafjf10111e1::BistPattern::kGreen:
    case S023msafjf10111e1::BistPattern::kBlue:
      return true;
    default:
      return false;
  }
}

}  // namespace

/**
 * @brief 接有复位引脚时先复位面板，再初始化 I2C 并探测设备地址。
 * @param freq_hz I2C 频率，单位 Hz，必须大于 0。
 * @return 复位和总线初始化成功返回 true；参数、GPIO 或总线操作失败返回 false。
 */
bool S023msafjf10111e1::Init(int32_t freq_hz) {
  if (bus_ == nullptr || freq_hz <= 0) {
    return false;
  }
  if (rst_ != kPinNotConnected && !Reset()) {
    return false;
  }
  if (!I2cChipBase::Init(freq_hz)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Init failed\n");
    I2cChipBase::Deinit(false);
    if (rst_ != kPinNotConnected) {
      ResetGpio(rst_);
    }
    return false;
  }
  return true;
}

/**
 * @brief 尝试恢复 BIST 快照，并继续解除总线设备和复位 GPIO 配置。
 * @param delete_bus true 请求释放自有物理总线，false 保留总线。
 * @return 所有操作成功返回 true；任一恢复或释放步骤失败返回 false。
 */
bool S023msafjf10111e1::Deinit(bool delete_bus) {
  bool result = SetBistEnabled(false);
  result &= I2cChipBase::Deinit(delete_bus);
  if (rst_ != kPinNotConnected) {
    result &= ResetGpio(rst_);
  }
  return result;
}

/**
 * @brief 将复位脚拉低 10 ms 后拉高并等待 10 ms，丢弃已保存的 BIST 状态。
 * @return GPIO 操作成功返回 true；未接复位引脚或 GPIO 操作失败返回 false。
 */
bool S023msafjf10111e1::Reset() {
  if (rst_ == kPinNotConnected) {
    return false;
  }
  if (!SetGpioMode(rst_, GpioMode::kOutput, GpioStatus::kPullup) ||
      !GpioWrite(rst_, 0)) {
    ResetGpio(rst_);
    return false;
  }
  bist_restore_pending_ = false;
  DelayMs(10);
  if (!GpioWrite(rst_, 1)) {
    ResetGpio(rst_);
    return false;
  }
  DelayMs(10);
  return true;
}

/**
 * @brief 先发送 16 位寄存器地址低字节，再使用重复 START 读取连续数据。
 * @param reg 首个寄存器的手册原始地址。
 * @param data 非空接收缓冲区，至少可容纳 length 字节。
 * @param length 读取长度，必须大于 0。
 * @return 读取成功返回 true；空总线、参数无效或总线失败返回 false。
 * @note 失败时不保证原始缓冲区内容不变，调用者应使用局部缓冲。
 */
bool S023msafjf10111e1::ReadRegisters(
    Register reg, uint8_t* data, size_t length) {
  if (bus_ == nullptr || data == nullptr || length == 0) {
    return false;
  }
  const auto address = static_cast<uint16_t>(reg);
  // 第 5.2 节：先发地址低字节，再发高字节，读操作使用重复 START。
  const uint8_t command[] = {
      static_cast<uint8_t>(address), static_cast<uint8_t>(address >> 8)};
  if (!bus_->WriteRead(command, sizeof(command), data, length)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Read register 0x%04X failed\n", static_cast<unsigned int>(address));
    return false;
  }
  return true;
}

/**
 * @brief 按低字节优先地址格式，写入一个或两个连续寄存器字节。
 * @param reg 首个寄存器的手册原始地址。
 * @param data 非空待写数据缓冲区。
 * @param length 数据长度，仅支持 1 或 2 字节。
 * @return 写入成功返回 true；空总线、参数无效或总线失败返回 false。
 */
bool S023msafjf10111e1::WriteRegisters(
    Register reg, const uint8_t* data, size_t length) {
  // 已公开的配置字段最多占两个连续字节。
  if (bus_ == nullptr || data == nullptr || length == 0 || length > 2) {
    return false;
  }
  const auto address = static_cast<uint16_t>(reg);
  uint8_t command[4] = {static_cast<uint8_t>(address),
      static_cast<uint8_t>(address >> 8), data[0], 0};
  if (length == 2) {
    command[3] = data[1];
  }
  if (!bus_->Write(command, length + 2)) {
    LogMessage(LogLevel::kError, __FILE__, __LINE__,
        "Write register 0x%04X failed\n", static_cast<unsigned int>(address));
    return false;
  }
  return true;
}

/**
 * @brief 复用连续写入逻辑发送单个配置字节。
 * @param reg 目标寄存器的手册原始地址。
 * @param value 写入值。
 * @return 写入成功返回 true；总线为空或总线访问失败返回 false。
 */
bool S023msafjf10111e1::WriteRegister(Register reg, uint8_t value) {
  return WriteRegisters(reg, &value, 1);
}

/**
 * @brief 读改写 RSMX/RSMY 镜像位，保留像素偏移和其他位。
 * @param mode 不镜像、水平、垂直或双向镜像。
 * @return 更新成功返回 true；枚举无效或总线访问失败返回 false。
 */
bool S023msafjf10111e1::SetMirror(MirrorMode mode) {
  uint8_t horizontal = 0;
  uint8_t vertical = 0;
  switch (mode) {
    case MirrorMode::kOff:
      break;
    case MirrorMode::kHorizontal:
      horizontal = kHorizontalMirrorMask;
      break;
    case MirrorMode::kVertical:
      vertical = kVerticalMirrorMask;
      break;
    case MirrorMode::kHorizontalVertical:
      horizontal = kHorizontalMirrorMask;
      vertical = kVerticalMirrorMask;
      break;
    default:
      return false;
  }

  uint8_t data[2];
  if (!ReadRegisters(
          Register::kHorizontalMirrorPixelShift, data, sizeof(data))) {
    return false;
  }
  data[0] = (data[0] & ~kHorizontalMirrorMask) | horizontal;
  data[1] = (data[1] & ~kVerticalMirrorMask) | vertical;
  return WriteRegisters(
      Register::kHorizontalMirrorPixelShift, data, sizeof(data));
}

/**
 * @brief 读取水平和垂直镜像位，并合成为镜像模式。
 * @param mode 保存结果的非空指针；读取失败时保持原值。
 * @return 读取成功返回 true；空指针或总线访问失败返回 false。
 */
bool S023msafjf10111e1::GetMirror(MirrorMode* mode) {
  if (mode == nullptr) {
    return false;
  }
  uint8_t data[2];
  if (!ReadRegisters(
          Register::kHorizontalMirrorPixelShift, data, sizeof(data))) {
    return false;
  }
  const bool horizontal = (data[0] & kHorizontalMirrorMask) != 0;
  const bool vertical = (data[1] & kVerticalMirrorMask) != 0;
  *mode = horizontal ? (vertical ? MirrorMode::kHorizontalVertical
                                 : MirrorMode::kHorizontal)
                     : (vertical ? MirrorMode::kVertical : MirrorMode::kOff);
  return true;
}

/**
 * @brief 将相对中心的像素偏移转换为 orbit 编码，保留同字节镜像位。
 * @param x 水平偏移，范围 [-4, 4] 像素，正值向右。
 * @param y 垂直偏移，范围 [-10, 10] 像素，正值向下。
 * @return 更新成功返回 true；偏移越界或总线访问失败返回 false。
 */
bool S023msafjf10111e1::SetPixelShift(int8_t x, int8_t y) {
  if (x < -kPixelShiftXCenter || x > kPixelShiftXCenter ||
      y < -kPixelShiftYCenter || y > kPixelShiftYCenter) {
    return false;
  }
  uint8_t data[2];
  if (!ReadRegisters(
          Register::kHorizontalMirrorPixelShift, data, sizeof(data))) {
    return false;
  }
  data[0] = (data[0] & ~kPixelShiftXMask) | (x + kPixelShiftXCenter);
  data[1] = (data[1] & ~kPixelShiftYMask) | (y + kPixelShiftYCenter);
  return WriteRegisters(
      Register::kHorizontalMirrorPixelShift, data, sizeof(data));
}

/**
 * @brief 校验 orbit 原始编码，再减去中心编码得到像素偏移。
 * @param x 保存水平偏移的非空指针，结果范围 [-4, 4] 像素。
 * @param y 保存垂直偏移的非空指针，结果范围 [-10, 10] 像素，应与 x
 * 指向不同变量。
 * @return 读取及编码校验成功返回 true；参数、编码或总线失败返回 false。
 * @note 在全部读取及校验成功后才更新两个输出，失败时保持原值。
 */
bool S023msafjf10111e1::GetPixelShift(int8_t* x, int8_t* y) {
  if (x == nullptr || y == nullptr) {
    return false;
  }
  uint8_t data[2];
  if (!ReadRegisters(
          Register::kHorizontalMirrorPixelShift, data, sizeof(data))) {
    return false;
  }
  const int shift_x = data[0] & kPixelShiftXMask;
  const int shift_y = data[1] & kPixelShiftYMask;
  if (shift_x > 2 * kPixelShiftXCenter || shift_y > 2 * kPixelShiftYCenter) {
    return false;
  }
  *x = static_cast<int8_t>(shift_x - kPixelShiftXCenter);
  *y = static_cast<int8_t>(shift_y - kPixelShiftYCenter);
  return true;
}

/**
 * @brief 保存并切换 BIST 控制寄存器，或恢复本对象保存的控制快照。
 * @param enabled true 开启 BIST，false 恢复进入前的状态。
 * @return 操作成功返回 true；读取或写入控制寄存器失败返回 false。
 * @note 无待恢复快照时，关闭操作直接成功；部分失败时保留快照供后续恢复。
 */
bool S023msafjf10111e1::SetBistEnabled(bool enabled) {
  if (!enabled) {
    if (!bist_restore_pending_) {
      return true;
    }
    bool result = WriteRegister(
        Register::kConfigurationControl, saved_configuration_control_);
    result &= WriteRegister(Register::kBistControl, saved_bist_control_);
    if (result) {
      bist_restore_pending_ = false;
    }
    return result;
  }

  if (!bist_restore_pending_) {
    if (!ReadRegisters(Register::kConfigurationControl,
            &saved_configuration_control_, 1) ||
        !ReadRegisters(Register::kBistControl, &saved_bist_control_, 1)) {
      return false;
    }
    // 手册未给出固定的 MIPI 退出序列，恢复进入前的控制值。
    // 即使后续写入失败也保留快照，使关闭 BIST 可以恢复部分写入。
    bist_restore_pending_ = true;
  }
  return WriteRegister(Register::kConfigurationControl, 0x15) &&
         WriteRegister(Register::kBistControl, 0x80);
}

/**
 * @brief 校验图案后开启 BIST，并写入手册规定的完整图案编码。
 * @param pattern 彩条、空白、白、红、绿或蓝图案。
 * @return 开启及写入均成功返回 true；图案无效或总线访问失败返回 false。
 * @note 图案写入失败时 BIST 可能已开启，应按需调用 SetBistEnabled(false) 恢复。
 */
bool S023msafjf10111e1::SetBistPattern(BistPattern pattern) {
  if (!IsValidBistPattern(pattern)) {
    return false;
  }
  return SetBistEnabled(true) &&
         WriteRegister(Register::kBistPattern, static_cast<uint8_t>(pattern));
}

/**
 * @brief 读取并校验 BIST 图案编码，不检查当前是否正在输出 BIST。
 * @param pattern 保存图案的非空指针；失败时保持原值。
 * @return 读取成功且编码有效返回 true；空指针、未知编码或总线失败返回 false。
 */
bool S023msafjf10111e1::GetBistPattern(BistPattern* pattern) {
  if (pattern == nullptr) {
    return false;
  }
  uint8_t value;
  if (!ReadRegisters(Register::kBistPattern, &value, 1)) {
    return false;
  }
  const auto result = static_cast<BistPattern>(value);
  if (!IsValidBistPattern(result)) {
    return false;
  }
  *pattern = result;
  return true;
}

/**
 * @brief 恢复本对象的 BIST 控制快照，配置亮度路径并写入 9 位 RGB 增益。
 * @param gain Q1.8 增益编码，范围 [0, 511]；256 为 1 倍，0 为黑屏而非断电。
 * @return 所有配置成功返回 true；范围无效、BIST 恢复或总线操作失败返回 false。
 * @note 保留增益高字节的其他位；中途失败时可能已有部分控制寄存器写入生效。
 */
bool S023msafjf10111e1::SetBrightnessGain(uint16_t gain) {
  if (gain > kMaxBrightnessGain) {
    return false;
  }
  // 手册要求完整写入亮度路径配置；公共控制位未定义，不能按位合并 BIST 配置。
  // 调用会尝试恢复 BIST 快照，配置写入成功不代表视频模式已恢复。
  uint8_t high;
  if (!ReadRegisters(Register::kBrightnessGainHigh, &high, 1) ||
      !SetBistEnabled(false) ||
      !WriteRegister(Register::kConfigurationControl, 0x1C) ||
      !WriteRegister(Register::kBrightnessControl, 0x03)) {
    return false;
  }
  const uint8_t data[] = {static_cast<uint8_t>(gain),
      static_cast<uint8_t>((high & 0xFE) | (gain >> 8))};
  return WriteRegisters(Register::kBrightnessGainLow, data, sizeof(data));
}

/**
 * @brief 读取并合并增益低 8 位及高字节最低位。
 * @param gain 保存 Q1.8 编码的非空指针，范围 [0, 511]；失败时保持原值。
 * @return 读取成功返回 true；空指针或总线访问失败返回 false。
 */
bool S023msafjf10111e1::GetBrightnessGain(uint16_t* gain) {
  if (gain == nullptr) {
    return false;
  }
  uint8_t data[2];
  if (!ReadRegisters(Register::kBrightnessGainLow, data, sizeof(data))) {
    return false;
  }
  *gain = static_cast<uint16_t>(data[0] | ((data[1] & 0x01) << 8));
  return true;
}

}  // namespace cpp_bus_driver
