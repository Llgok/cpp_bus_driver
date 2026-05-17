/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-04-30 13:37:40
 * @License: GPL 3.0
 */
#include "chip_guide.h"

namespace cpp_bus_driver {
namespace {

/**
 * @brief 检查初始化序列剩余数据是否足够
 * @param index 当前序列索引
 * @param length 序列总长度
 * @param required 从当前索引开始需要的数据项数量
 * @return 数据项数量足够返回true
 */
bool HasSequenceBytes(size_t index, size_t length, size_t required) {
  return index <= length && required <= length - index;
}

template <typename Bus>
bool HasBus(const std::shared_ptr<Bus>& bus, Tool* tool, const char* file,
    size_t line) {
  if (bus != nullptr) {
    return true;
  }

  tool->LogMessage(Tool::LogLevel::kInfo, file, line, "Invalid argument\n");
  return false;
}

}  // namespace

bool ChipI2cGuide::Init(int32_t freq_hz) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

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
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Deinit(delete_bus)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipI2cGuide::InitSequence(const uint8_t* sequence, size_t length) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (sequence == nullptr || length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        if (!HasSequenceBytes(index, length, 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        DelayMs(sequence[index + 1]);
        index += 2;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8):
        if (!HasSequenceBytes(index, length, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        if (!bus_->Write(&sequence[index + 1], 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 3;
        break;

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipI2cGuide InitSequence failed (error index: %zu)\n", index);
        return false;
    }
  }

  return true;
}

bool ChipI2cGuide::InitSequence(const uint16_t* sequence, size_t length) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (sequence == nullptr || length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        if (!HasSequenceBytes(index, length, 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        DelayMs(sequence[index + 1]);
        index += 2;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC16D8): {
        if (!HasSequenceBytes(index, length, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        const uint8_t buffer[] = {
            static_cast<uint8_t>(sequence[index + 1] >> 8),
            static_cast<uint8_t>(sequence[index + 1]),
            static_cast<uint8_t>(sequence[index + 2]),
        };

        if (!bus_->Write(buffer, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipI2cGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 3;
        break;
      }
      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipI2cGuide InitSequence failed (error index: %zu)\n", index);
        return false;
    }
  }

  return true;
}

bool ChipSpiGuide::Init(int32_t freq_hz) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Init(freq_hz, cs_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipSpiGuide::Deinit(bool delete_bus) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Deinit(delete_bus)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipSpiGuide::InitSequence(const uint8_t* sequence, size_t length) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (sequence == nullptr || length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        if (!HasSequenceBytes(index, length, 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipSpiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        DelayMs(sequence[index + 1]);
        index += 2;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8):
        if (!HasSequenceBytes(index, length, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipSpiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        if (!bus_->Write(&sequence[index + 1], 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipSpiGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 3;
        break;

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipSpiGuide InitSequence failed (error index: %zu)\n", index);
        return false;
    }
  }

  return true;
}

bool ChipQspiGuide::Init(int32_t freq_hz) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Init(freq_hz, cs_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipQspiGuide::Deinit() {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Deinit(true)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipQspiGuide::InitSequence(const uint32_t* sequence, size_t length) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (sequence == nullptr || length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        if (!HasSequenceBytes(index, length, 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipQspiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        DelayMs(sequence[index + 1]);
        index += 2;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24): {
        if (!HasSequenceBytes(index, length, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipQspiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        uint8_t buffer[] = {
            static_cast<uint8_t>(sequence[index + 1]),
            static_cast<uint8_t>(sequence[index + 2] >> 16),
            static_cast<uint8_t>(sequence[index + 2] >> 8),
            static_cast<uint8_t>(sequence[index + 2]),
        };
        if (!bus_->Write(buffer, 4)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipQspiGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 3;

        break;
      }

      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8R24D8): {
        if (!HasSequenceBytes(index, length, 4)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipQspiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        uint8_t buffer[] = {
            static_cast<uint8_t>(sequence[index + 1]),
            static_cast<uint8_t>(sequence[index + 2] >> 16),
            static_cast<uint8_t>(sequence[index + 2] >> 8),
            static_cast<uint8_t>(sequence[index + 2]),
            static_cast<uint8_t>(sequence[index + 3]),
        };

        if (!bus_->Write(buffer, 5)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipQspiGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 4;

        break;
      }

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipQspiGuide InitSequence failed (error index: %zu)\n", index);
        return false;
    }
  }

  return true;
}

bool ChipUartGuide::Init(int32_t baud_rate) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Init(baud_rate)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipUartGuide::Deinit() {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Deinit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipI2sGuide::Init(
    uint16_t mclk_multiple, uint32_t sample_rate_hz, uint8_t data_bit_width) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  i2s_mclk_multiple_t resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
  if (mclk_multiple <= 128) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
  } else if (mclk_multiple <= 192) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_192;
  } else if (mclk_multiple <= 256) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
  } else if (mclk_multiple <= 384) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_384;
  } else if (mclk_multiple <= 512) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_512;
  } else if (mclk_multiple <= 576) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_576;
  } else if (mclk_multiple <= 768) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_768;
  } else if (mclk_multiple <= 1024) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1024;
  } else if (mclk_multiple <= 1152) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1152;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
  }

  i2s_data_bit_width_t resolved_data_bit_width =
      i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_8BIT;
  if (data_bit_width <= 8) {
    resolved_data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_8BIT;
  } else if (data_bit_width <= 16) {
    resolved_data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
  } else if (data_bit_width <= 24) {
    resolved_data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_24BIT;
  } else if (data_bit_width <= 32) {
    resolved_data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_32BIT;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    resolved_data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
  }

  if (!bus_->Init(
      resolved_mclk_multiple, sample_rate_hz, resolved_data_bit_width)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
  nrf_i2s_ratio_t resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_32_X;
  if (mclk_multiple <= 32) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_32_X;
  } else if (mclk_multiple <= 48) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_48_X;
  } else if (mclk_multiple <= 64) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_64_X;
  } else if (mclk_multiple <= 96) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_96_X;
  } else if (mclk_multiple <= 128) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_128_X;
  } else if (mclk_multiple <= 192) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_192_X;
  } else if (mclk_multiple <= 256) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_256_X;
  } else if (mclk_multiple <= 384) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_384_X;
  } else if (mclk_multiple <= 512) {
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_512_X;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    resolved_ratio = nrf_i2s_ratio_t::NRF_I2S_RATIO_32_X;
  }

  nrf_i2s_swidth_t resolved_swidth = nrf_i2s_swidth_t::NRF_I2S_SWIDTH_8_BIT;
  if (data_bit_width <= 8) {
    resolved_swidth = nrf_i2s_swidth_t::NRF_I2S_SWIDTH_8_BIT;
  } else if (data_bit_width <= 16) {
    resolved_swidth = nrf_i2s_swidth_t::NRF_I2S_SWIDTH_16_BIT;
  } else if (data_bit_width <= 24) {
    resolved_swidth = nrf_i2s_swidth_t::NRF_I2S_SWIDTH_24_BIT;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    resolved_swidth = nrf_i2s_swidth_t::NRF_I2S_SWIDTH_16_BIT;
  }

  if (!bus_->Init(
      resolved_ratio, sample_rate_hz, resolved_swidth)) {
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
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Deinit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
bool ChipI2sGuide::SetClockReconfig(uint16_t mclk_multiple,
    uint32_t sample_rate_hz, BusI2sGuide::DataMode data_mode) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  i2s_mclk_multiple_t resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
  if (mclk_multiple <= 128) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128;
  } else if (mclk_multiple <= 192) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_192;
  } else if (mclk_multiple <= 256) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
  } else if (mclk_multiple <= 384) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_384;
  } else if (mclk_multiple <= 512) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_512;
  } else if (mclk_multiple <= 576) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_576;
  } else if (mclk_multiple <= 768) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_768;
  } else if (mclk_multiple <= 1024) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1024;
  } else if (mclk_multiple <= 1152) {
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_1152;
  } else {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Value out of range\n");
    resolved_mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256;
  }

  if (!bus_->SetClockReconfig(
      resolved_mclk_multiple, sample_rate_hz, data_mode)) {
    LogMessage(LogLevel::kChip, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}
#endif

bool ChipSdioGuide::Init(int32_t freq_hz) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Init(freq_hz)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipSdioGuide::Deinit() {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Deinit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipMipiGuide::Init(float freq_mhz, float lane_bit_rate_mbps) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Init(freq_mhz, lane_bit_rate_mbps, init_sequence_format_)) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Init failed\n");
    return false;
  }

  return true;
}

bool ChipMipiGuide::Deinit() {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (!bus_->Deinit()) {
    LogMessage(LogLevel::kBus, __FILE__, __LINE__, "Deinit failed\n");
    return false;
  }

  return true;
}

bool ChipMipiGuide::InitSequence(const uint8_t* sequence, size_t length) {
  if (!HasBus(bus_, this, __FILE__, __LINE__)) {
    return false;
  }

  if (sequence == nullptr || length == 0) {
    LogMessage(LogLevel::kInfo, __FILE__, __LINE__, "Invalid argument\n");
    return false;
  }

  size_t index = 0;
  while (index < length) {
    switch (sequence[index]) {
      case static_cast<uint8_t>(InitSequenceFormat::kDelayMs):
        if (!HasSequenceBytes(index, length, 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        DelayMs(sequence[index + 1]);
        index += 2;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8):
        if (!HasSequenceBytes(index, length, 2)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        if (!bus_->Write(
                static_cast<int32_t>(sequence[index + 1]), nullptr, 0)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 2;
        break;
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8ByteData): {
        if (!HasSequenceBytes(index, length, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        const size_t data_length = sequence[index + 2];
        if (!HasSequenceBytes(index, length, 3 + data_length)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        if (!bus_->Write(static_cast<int32_t>(sequence[index + 1]),
                &sequence[index + 3], data_length)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 3 + data_length;
        break;
      }
      case static_cast<uint8_t>(InitSequenceFormat::kWriteC8D8):
        if (!HasSequenceBytes(index, length, 3)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide InitSequence short data (error index: %zu)\n",
              index);
          return false;
        }
        if (!bus_->Write(static_cast<int32_t>(sequence[index + 1]),
                &sequence[index + 2], 1)) {
          LogMessage(LogLevel::kChip, __FILE__, __LINE__,
              "ChipMipiGuide write failed (error index: %zu)\n", index);
          return false;
        }
        index += 3;
        break;

      default:
        LogMessage(LogLevel::kChip, __FILE__, __LINE__,
            "ChipMipiGuide InitSequence failed (error index: %zu)\n", index);
        return false;
    }
  }

  return true;
}
}  // namespace cpp_bus_driver
