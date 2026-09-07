/*
 * @Description: BQ25890 寄存器驱动
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
    Feature::kBoostTemperatureThresholds,
    Feature::kHvdcp,
    Feature::kMaxCharge,
    Feature::kUsbInputDetection,
    Feature::kJeita,
    Feature::kBoostCurrentLimit,
    Feature::kBatteryLoad,
};

// SLUSC86D：BOOST_LIM 编码 0-7，单位 mA。
constexpr uint16_t kBoostCurrentLimitsMa[] = {
    500, 750, 1200, 1400, 1650, 1875, 2150, 2450};

}  // namespace

Bq2589x::Bq25890Driver::Bq25890Driver()
    : ModelDriver(ChipModel::kBq25890, 0x6A, 3, 1, kFeatures) {}

bool Bq2589x::Bq25890Driver::Init(Bq2589x& chip) const {
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

bool Bq2589x::Bq25890Driver::SetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t offset_mv) const {
  return chip.SetLinearField(
      Register::kReg01, 0x1F, 0, offset_mv, 0, 3100, 100);
}

bool Bq2589x::Bq25890Driver::GetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t& offset_mv) const {
  return chip.GetLinearField(
      Register::kReg01, 0x1F, 0, 0, 3100, 100, offset_mv);
}

bool Bq2589x::Bq25890Driver::SetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetLinearField(
      Register::kReg04, 0x7F, 0, current_ma, 0, 5056, 64);
}

bool Bq2589x::Bq25890Driver::GetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetLinearField(
      Register::kReg04, 0x7F, 0, 0, 5056, 64, current_ma);
}

bool Bq2589x::Bq25890Driver::SetBoostCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetTableField(Register::kReg0a, 0x07, 0, current_ma,
      kBoostCurrentLimitsMa,
      sizeof(kBoostCurrentLimitsMa) / sizeof(kBoostCurrentLimitsMa[0]));
}

bool Bq2589x::Bq25890Driver::GetBoostCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetTableField(Register::kReg0a, 0x07, 0, kBoostCurrentLimitsMa,
      sizeof(kBoostCurrentLimitsMa) / sizeof(kBoostCurrentLimitsMa[0]),
      current_ma);
}

}  // namespace cpp_bus_driver
