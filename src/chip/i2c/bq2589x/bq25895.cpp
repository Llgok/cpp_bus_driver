/*
 * @Description: BQ25895 寄存器驱动
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
    Feature::kSdpStatus,
    Feature::kBatteryLoad,
};

}  // namespace

Bq2589x::Bq25895Driver::Bq25895Driver()
    : ModelDriver(ChipModel::kBq25895, 0x6A, 7, 1, kFeatures) {}

bool Bq2589x::Bq25895Driver::Init(Bq2589x& chip) const {
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

bool Bq2589x::Bq25895Driver::SetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t offset_mv) const {
  return chip.SetLinearField(
      Register::kReg01, 0x1F, 0, offset_mv, 0, 3100, 100);
}

bool Bq2589x::Bq25895Driver::GetInputVoltageLimitOffset(
    Bq2589x& chip, uint16_t& offset_mv) const {
  return chip.GetLinearField(
      Register::kReg01, 0x1F, 0, 0, 3100, 100, offset_mv);
}

bool Bq2589x::Bq25895Driver::SetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t current_ma) const {
  return chip.SetLinearField(
      Register::kReg04, 0x7F, 0, current_ma, 0, 5056, 64);
}

bool Bq2589x::Bq25895Driver::GetFastChargeCurrentLimit(
    Bq2589x& chip, uint16_t& current_ma) const {
  return chip.GetLinearField(
      Register::kReg04, 0x7F, 0, 0, 5056, 64, current_ma);
}

Bq2589x::ChipStatus Bq2589x::Bq25895Driver::DecodeChipStatus(
    uint8_t value) const {
  ChipStatus result = ModelDriver::DecodeChipStatus(value);
  result.sdp_status_valid = result.vbus_status == VbusStatus::kUsbHostSdp;
  result.usb_sdp_current_limit_ma =
      result.sdp_status_valid ? ((value & 0x02) != 0 ? 500 : 100) : 0;
  return result;
}

Bq2589x::NtcFault Bq2589x::Bq25895Driver::DecodeNtcFault(uint8_t code) const {
  switch (code) {
    case 0:
      return NtcFault::kNormal;
    case 1:
    case 5:
      return NtcFault::kCold;
    case 2:
    case 6:
      return NtcFault::kHot;
    default:
      return NtcFault::kUnknown;
  }
}

}  // namespace cpp_bus_driver
