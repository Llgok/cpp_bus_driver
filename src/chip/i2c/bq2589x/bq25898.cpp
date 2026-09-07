/*
 * @Description: BQ25898 寄存器驱动
 * @License: GPL 3.0
 */
#include "chip/i2c/bq2589x/bq2589x.h"

namespace cpp_bus_driver {
namespace {

using Feature = Bq2589x::Feature;

// 当前型号支持的可选功能。
constexpr std::initializer_list<Feature> kFeatures = {
    Feature::kIlimPin,
    Feature::kInputCurrentOptimizer,
    Feature::kOtg,
    Feature::kCurrentPulseControl,
    Feature::kIrCompensation,
    Feature::kBatfetControl,
    Feature::kBatfetDelay,
    Feature::kBatfetReset,
    Feature::kTsAdc,
    Feature::kVokOtg,
    Feature::kBoostMinimumBatteryVoltage,
    Feature::kBoostPfm,
    Feature::kJeita,
    Feature::kBoostCurrentLimit,
};

// VDPM_OS 仅占 REG01 bit 0，编码 0/1 对应 400/600 mV。
constexpr uint16_t kVindpmOffsetsMv[] = {400, 600};

// SLUSCA6B：BOOST_LIM 编码 0-7，单位 mA。
constexpr uint16_t kBoostCurrentLimitsMa[] = {
    500, 800, 1000, 1200, 1500, 1800, 2100, 2400};

}  // namespace

// 地址、PN 和 REV 与 BQ25892 重合，调用方须显式选择型号。
Bq2589x::Bq25898Driver::Bq25898Driver()
    : ModelDriver(ChipModel::kBq25898, 0x6B, 0, 1, kFeatures) {}

bool Bq2589x::Bq25898Driver::Init(Bq2589x& chip) const {
  static constexpr uint8_t kInitSequence[] = {
      // 关闭看门狗，其余位保持手册上电默认值，安全充电定时器保持启用。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kReg07),
      0x8D,

      // 输入限流设为 2000 mA，ILIM 引脚按手册字段说明保持默认启用。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kReg00),
      0x66,

      // 上电初始化：EN_PUMPX 保持默认关闭，快速充电电流设为 512 mA。
      static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8),
      static_cast<uint8_t>(Register::kReg04),
      0x08,
  };
  return chip.InitSequence(kInitSequence, sizeof(kInitSequence));
}

bool Bq2589x::Bq25898Driver::SetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t offset_mv) const {
  return chip.SetTableField(Register::kReg01, 0x01, 0, offset_mv,
      kVindpmOffsetsMv, sizeof(kVindpmOffsetsMv) / sizeof(kVindpmOffsetsMv[0]));
}

bool Bq2589x::Bq25898Driver::GetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t& offset_mv) const {
  return chip.GetTableField(Register::kReg01, 0x01, 0, kVindpmOffsetsMv,
      sizeof(kVindpmOffsetsMv) / sizeof(kVindpmOffsetsMv[0]), offset_mv);
}

bool Bq2589x::Bq25898Driver::SetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetLinearField(
      Register::kReg04, 0x7F, 0, current_ma, 0, 4032, 64);
}

bool Bq2589x::Bq25898Driver::GetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetLinearField(
      Register::kReg04, 0x7F, 0, 0, 4032, 64, current_ma);
}

bool Bq2589x::Bq25898Driver::SetBoostCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetTableField(Register::kReg0a, 0x07, 0, current_ma,
      kBoostCurrentLimitsMa,
      sizeof(kBoostCurrentLimitsMa) / sizeof(kBoostCurrentLimitsMa[0]));
}

bool Bq2589x::Bq25898Driver::GetBoostCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetTableField(Register::kReg0a, 0x07, 0, kBoostCurrentLimitsMa,
      sizeof(kBoostCurrentLimitsMa) / sizeof(kBoostCurrentLimitsMa[0]),
      current_ma);
}

Bq2589x::ChipStatus Bq2589x::Bq25898Driver::DecodeChipStatus(
    uint8_t value) const {
  ChipStatus result = ModelDriver::DecodeChipStatus(value);
  const uint8_t vbus = (value >> 5) & 0x07;
  if (vbus == 2) {
    result.vbus_status = VbusStatus::kAdapter;
  } else if (vbus >= 3 && vbus <= 6) {
    result.vbus_status = VbusStatus::kUnknown;
  }
  return result;
}

}  // namespace cpp_bus_driver
