/*
 * @Description: BQ25898D 寄存器驱动
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
    Feature::kDpDmDac,
    Feature::k12VoltDetection,
    Feature::kHvdcp,
    Feature::kMaxCharge,
    Feature::kUsbInputDetection,
    Feature::kForceDsel,
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

Bq2589x::Bq25898dDriver::Bq25898dDriver()
    : ModelDriver(ChipModel::kBq25898d, 0x6A, 2, 1, kFeatures) {}

bool Bq2589x::Bq25898dDriver::Init(Bq2589x& chip) const {
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

bool Bq2589x::Bq25898dDriver::SetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t offset_mv) const {
  return chip.SetTableField(Register::kReg01, 0x01, 0, offset_mv,
      kVindpmOffsetsMv, sizeof(kVindpmOffsetsMv) / sizeof(kVindpmOffsetsMv[0]));
}

bool Bq2589x::Bq25898dDriver::GetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t& offset_mv) const {
  return chip.GetTableField(Register::kReg01, 0x01, 0, kVindpmOffsetsMv,
      sizeof(kVindpmOffsetsMv) / sizeof(kVindpmOffsetsMv[0]), offset_mv);
}

bool Bq2589x::Bq25898dDriver::SetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetLinearField(
      Register::kReg04, 0x7F, 0, current_ma, 0, 4032, 64);
}

bool Bq2589x::Bq25898dDriver::GetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetLinearField(
      Register::kReg04, 0x7F, 0, 0, 4032, 64, current_ma);
}

bool Bq2589x::Bq25898dDriver::SetBoostCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetTableField(Register::kReg0a, 0x07, 0, current_ma,
      kBoostCurrentLimitsMa,
      sizeof(kBoostCurrentLimitsMa) / sizeof(kBoostCurrentLimitsMa[0]));
}

bool Bq2589x::Bq25898dDriver::GetBoostCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetTableField(Register::kReg0a, 0x07, 0, kBoostCurrentLimitsMa,
      sizeof(kBoostCurrentLimitsMa) / sizeof(kBoostCurrentLimitsMa[0]),
      current_ma);
}

bool Bq2589x::Bq25898dDriver::SetDpDmDac(
    Bq2589x& chip, bool dplus, DpDmVoltage voltage) const {
  const auto code = static_cast<uint8_t>(voltage);
  const uint8_t maximum = dplus ? 7 : 6;
  if (code > maximum || !chip.IsDpDmDacReady()) {
    return false;
  }
  return chip.UpdateRegisterBits(Register::kReg01, dplus ? 0xE0 : 0x1C,
      static_cast<uint8_t>(code << (dplus ? 5 : 2)));
}

bool Bq2589x::Bq25898dDriver::GetDpDmDac(
    Bq2589x& chip, bool dplus, DpDmVoltage& voltage) const {
  uint8_t code = 0;
  if (!chip.ReadField(
          Register::kReg01, dplus ? 0xE0 : 0x1C, dplus ? 5 : 2, code)) {
    return false;
  }
  // DMINUS_DAC 编码 6 和 7 均表示 3.3 V。
  voltage = !dplus && code == 7 ? DpDmVoltage::k3300Mv
                                : static_cast<DpDmVoltage>(code);
  return true;
}

}  // namespace cpp_bus_driver
