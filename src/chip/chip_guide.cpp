/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-04-30 13:37:40
 * @License: GPL 3.0
 */
#include "chip_guide.h"

namespace cpp_bus_driver {
bool ChipI2cGuide::Init(int32_t freq_hz) {
  if (!bus_->Init(freq_hz, address_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  if (!bus_->Probe(address_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__,
        "Probe failed (not found address: %#X)\n", address_);
    return false;
  }

  return true;
}

bool ChipI2cGuide::Deinit(bool delete_bus) {
  if (!bus_->Deinit(delete_bus)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipI2cGuide::InitSequence(const uint8_t* sequence, size_t length) {
  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        index++;
        DelayMs(sequence[index]);
        index++;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8):
        index++;
        if (!bus_->Write(&sequence[index], 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide write failed (error index: %d)\n", index);
          return false;
        }
        index += 2;
        break;

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipI2cGuide InitSequence failed (error index: %d)\n", index);
        return false;
        break;
    }
  }

  return true;
}

bool ChipI2cGuide::InitSequence(const uint16_t* sequence, size_t length) {
  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        index++;
        DelayMs(sequence[index]);
        index++;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC16D8): {
        index++;
        const uint8_t buffer[] = {
            static_cast<uint8_t>(sequence[index] >> 8),
            static_cast<uint8_t>(sequence[index]),
            static_cast<uint8_t>(sequence[index + 1]),
        };

        if (!bus_->Write(buffer, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide write failed (error index: %d)\n", index);
          return false;
        }
        index += 2;
        break;
      }
      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipI2cGuide InitSequence failed (error index: %d)\n", index);
        return false;
        break;
    }
  }

  return true;
}

bool ChipSpiGuide::Init(int32_t freq_hz) {
  if (!bus_->Init(freq_hz, cs_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipSpiGuide::Deinit(bool delete_bus) {
  if (!bus_->Deinit(delete_bus)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipSpiGuide::InitSequence(const uint8_t* sequence, size_t length) {
  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        index++;
        DelayMs(sequence[index]);
        index++;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8):
        index++;
        if (!bus_->Write(&sequence[index], 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipSpiGuide write failed (error index: %d)\n", index);
          return false;
        }
        index += 2;
        break;

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipSpiGuide InitSequence failed (error index: %d)\n", index);
        return false;
        break;
    }
  }

  return true;
}

bool ChipQspiGuide::Init(int32_t freq_hz) {
  if (!bus_->Init(freq_hz, cs_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipQspiGuide::Deinit() { return true; }

bool ChipQspiGuide::InitSequence(const uint32_t* sequence, size_t length) {
  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        index++;
        DelayMs(sequence[index]);
        index++;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24): {
        index++;
        uint8_t buffer[] = {
            static_cast<uint8_t>(sequence[index]),
            static_cast<uint8_t>(sequence[index + 1] >> 16),
            static_cast<uint8_t>(sequence[index + 1] >> 8),
            static_cast<uint8_t>(sequence[index + 1]),
        };
        if (!bus_->Write(buffer, 4)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipQspiGuide write failed (error index: %d)\n", index);
          return false;
        }
        index += 2;

        break;
      }

      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8): {
        index++;
        uint8_t buffer[] = {
            static_cast<uint8_t>(sequence[index]),
            static_cast<uint8_t>(sequence[index + 1] >> 16),
            static_cast<uint8_t>(sequence[index + 1] >> 8),
            static_cast<uint8_t>(sequence[index + 1]),
            static_cast<uint8_t>(sequence[index + 2]),
        };

        if (!bus_->Write(buffer, 5)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipQspiGuide write failed (error index: %d)\n", index);
          return false;
        }
        index += 3;

        break;
      }

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipQspiGuide InitSequence failed (error index: %d)\n", index);
        return false;
        break;
    }
  }

  return true;
}

bool ChipUartGuide::Init(int32_t baud_rate) {
  if (!bus_->Init(baud_rate)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipUartGuide::Deinit() {
  if (!bus_->Deinit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipI2sGuide::Init(
    uint16_t mclk_multiple, uint32_t sample_rate_hz, uint8_t data_bit_width) {
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  if (!bus_->Init(
          [this](uint16_t mm) -> i2s_mclk_multiple_t {
            if (mm <= 128) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
            } else if (mm <= 192) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_192;
            } else if (mm <= 256) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
            } else if (mm <= 384) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_384;
            } else if (mm <= 512) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_512;
            } else if (mm <= 576) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_576;
            } else if (mm <= 768) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_768;
            } else if (mm <= 1024) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1024;
            } else if (mm <= 1152) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1152;
            } else {
              LogMessage(
                  LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
            }
          }(mclk_multiple),
          sample_rate_hz,
          [this](uint8_t dbw) -> i2s_data_bit_width_t {
            if (dbw <= 8) {
              return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_8BIT;
            } else if (dbw <= 16) {
              return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
            } else if (dbw <= 24) {
              return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_24BIT;
            } else if (dbw <= 32) {
              return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_32BIT;
            } else {
              LogMessage(
                  LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
              return i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
            }
          }(data_bit_width))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
#elif defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
  if (!bus_->Init(
          [this](uint16_t ratio) -> nrf_i2s_ratio_t {
            if (ratio <= 32) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_32_X;
            } else if (ratio <= 48) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_48_X;
            } else if (ratio <= 64) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_64_X;
            } else if (ratio <= 96) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_96_X;
            } else if (ratio <= 128) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_128_X;
            } else if (ratio <= 192) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_192_X;
            } else if (ratio <= 256) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_256_X;
            } else if (ratio <= 384) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_384_X;
            } else if (ratio <= 512) {
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_512_X;
            } else {
              LogMessage(
                  LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
              return nrf_i2s_ratio_t::NRF_I2S_RATIO_32_X;
            }
          }(mclk_multiple),
          sample_rate_hz,
          [this](uint8_t swidth) -> nrf_i2s_swidth_t {
            if (swidth <= 8) {
              return nrf_i2s_swidth_t::NRF_I2S_SWIDTH_8_BIT;
            } else if (swidth <= 16) {
              return nrf_i2s_swidth_t::NRF_I2S_SWIDTH_16_BIT;
            } else if (swidth <= 24) {
              return nrf_i2s_swidth_t::NRF_I2S_SWIDTH_24_BIT;
            } else {
              LogMessage(
                  LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
              return nrf_i2s_swidth_t::NRF_I2S_SWIDTH_16_BIT;
            }
          }(data_bit_width))) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
#else
  LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
  return false;
#endif
}

bool ChipI2sGuide::Deinit() {
  if (!bus_->Deinit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
bool ChipI2sGuide::SetClockReconfig(uint16_t mclk_multiple,
    uint32_t sample_rate_hz, BusI2sGuide::DataMode data_mode) {
  if (!bus_->SetClockReconfig(
          [this](uint16_t mm) -> i2s_mclk_multiple_t {
            if (mm <= 128) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
            } else if (mm <= 192) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_192;
            } else if (mm <= 256) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
            } else if (mm <= 384) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_384;
            } else if (mm <= 512) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_512;
            } else if (mm <= 576) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_576;
            } else if (mm <= 768) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_768;
            } else if (mm <= 1024) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1024;
            } else if (mm <= 1152) {
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1152;
            } else {
              LogMessage(
                  LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
              return i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
            }
          }(mclk_multiple),
          sample_rate_hz, data_mode)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}
#endif

bool ChipSdioGuide::Init(int32_t freq_hz) {
  if (!bus_->Init(freq_hz)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipSdioGuide::Deinit() { return true; }

bool ChipMipiGuide::Init(float freq_mhz, float lane_bit_rate_mbps) {
  if (!bus_->Init(freq_mhz, lane_bit_rate_mbps, init_sequence_format_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipMipiGuide::Deinit() {
  if (!bus_->Deinit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipMipiGuide::InitSequence(const uint8_t* sequence, size_t length) {
  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        index++;
        DelayMs(sequence[index]);
        index++;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8):
        index++;
        if (!bus_->Write(static_cast<int32_t>(sequence[index]), nullptr, 0)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide write failed (error index: %d)\n", index);
          return false;
        }
        index++;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8ByteData):
        index++;
        if (!bus_->Write(static_cast<int32_t>(sequence[index]),
                &sequence[index + 2], sequence[index + 1])) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide write failed (error index: %d)\n", index);
          return false;
        }
        index += sequence[index + 1] + 2;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8):
        index++;
        if (!bus_->Write(static_cast<int32_t>(sequence[index]),
                &sequence[index + 1], 1)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide write failed (error index: %d)\n", index);
          return false;
        }
        index += 2;
        break;

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipMipiGuide InitSequence failed (error index: %d)\n", index);
        return false;
        break;
    }
  }

  return true;
}
}  // namespace cpp_bus_driver
