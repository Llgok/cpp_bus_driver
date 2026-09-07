/*
 * @Description: BQ25898C 寄存器驱动
 * @License: GPL 3.0
 */
#include "chip/i2c/bq2589x/bq2589x.h"

namespace cpp_bus_driver {
namespace {

using Feature = Bq2589x::Feature;

// 当前型号支持的可选功能。
// REG09 BATFET_DIS 的访问属性存在手册冲突，暂不开放 BATFET 控制。
constexpr std::initializer_list<Feature> kFeatures = {};

// VDPM_OS 仅占 REG01 bit 0，编码 0/1 对应 400/600 mV。
constexpr uint16_t kVindpmOffsetsMv[] = {400, 600};

}  // namespace

// REG14 bit 2 保留，不能作为 TS_PROFILE 报告。
Bq2589x::Bq25898cDriver::Bq25898cDriver()
    : ModelDriver(ChipModel::kBq25898c, 0x6B, 1, 1, kFeatures, false) {}

bool Bq2589x::Bq25898cDriver::Init(Bq2589x& chip) const {
  static constexpr uint8_t kInitSequence[] = {
      // 关闭看门狗，其余位保持手册上电默认值，安全充电定时器保持启用。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kReg07),
      0x8D,

      // 输入限流设为 2000 mA；无 ILIM 引脚，REG00 bit 6 按只读默认值写 1。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kReg00),
      0x66,

      // 上电初始化：保留位写 0，快速充电电流设为 512 mA。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kReg04),
      0x08,
  };
  return chip.InitSequence(kInitSequence, sizeof(kInitSequence));
}

bool Bq2589x::Bq25898cDriver::SetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t offset_mv) const {
  return chip.SetTableField(Register::kReg01, 0x01, 0, offset_mv,
      kVindpmOffsetsMv, sizeof(kVindpmOffsetsMv) / sizeof(kVindpmOffsetsMv[0]));
}

bool Bq2589x::Bq25898cDriver::GetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t& offset_mv) const {
  return chip.GetTableField(Register::kReg01, 0x01, 0, kVindpmOffsetsMv,
      sizeof(kVindpmOffsetsMv) / sizeof(kVindpmOffsetsMv[0]), offset_mv);
}

bool Bq2589x::Bq25898cDriver::SetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetLinearField(
      Register::kReg04, 0x3F, 0, current_ma, 0, 3008, 64);
}

bool Bq2589x::Bq25898cDriver::GetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetLinearField(
      Register::kReg04, 0x3F, 0, 0, 3008, 64, current_ma);
}

Bq2589x::ChipStatus Bq2589x::Bq25898cDriver::DecodeChipStatus(
    uint8_t value) const {
  ChipStatus result = ModelDriver::DecodeChipStatus(value);
  const uint8_t vbus = (value >> 5) & 0x07;
  if (vbus == 2) {
    result.vbus_status = VbusStatus::kAdapter;
  } else if (vbus >= 3) {
    result.vbus_status = VbusStatus::kUnknown;
  }
  return result;
}

Bq2589x::DpmStatus Bq2589x::Bq25898cDriver::DecodeDpmStatus(
    uint8_t value) const {
  DpmStatus result = ModelDriver::DecodeDpmStatus(value);
  result.input_current_limit_valid = false;
  result.input_current_limit_ma = 0;
  return result;
}

bool Bq2589x::Bq25898cDriver::ReadAdcRegisters(
    I2cBusBase& bus, uint8_t (&data)[5]) const {
  return bus.Read(static_cast<uint8_t>(Register::kReg0e), data, 2) &&
         bus.Read(static_cast<uint8_t>(Register::kReg11), data + 3, 2);
}

Bq2589x::NtcFault Bq2589x::Bq25898cDriver::DecodeNtcFault(
    uint8_t /*code*/) const {
  return NtcFault::kUnknown;
}

}  // namespace cpp_bus_driver
