/*
 * @Description: 基于不同总线访问芯片的公共驱动基类
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-09-04 11:52:13
 * @License: GPL 3.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "bus/bus_base.h"

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
#include "driver/spi_master.h"
#endif

namespace cpp_bus_driver {
class I2cChipBase : public virtual DriverBase {
 public:
  I2cChipBase(std::shared_ptr<I2cBusBase> bus, int16_t address)
      : bus_(bus), address_(address) {}
  virtual bool Init(int32_t freq_hz);
  virtual bool Deinit(bool delete_bus);

  bool InitSequence(const uint8_t* sequence, size_t length);
  bool InitSequence(const uint16_t* sequence, size_t length);

 protected:
  std::shared_ptr<I2cBusBase> bus_;

 private:
  int16_t address_;
};

class SpiChipBase : public virtual DriverBase {
 public:
  SpiChipBase(std::shared_ptr<SpiBusBase> bus, int32_t cs = kPinNotConnected)
      : bus_(bus), cs_(cs) {}
  virtual bool Init(int32_t freq_hz);
  virtual bool Deinit(bool delete_bus);
  bool InitSequence(const uint8_t* sequence, size_t length);

 protected:
  std::shared_ptr<SpiBusBase> bus_;

  int32_t cs_;
};

class QspiChipBase : public virtual DriverBase {
 public:
  QspiChipBase(std::shared_ptr<QspiBusBase> bus, int32_t cs = kPinNotConnected)
      : bus_(bus), cs_(cs) {}
  virtual bool Init(int32_t freq_hz);
  virtual bool Deinit();
  bool InitSequence(const uint32_t* sequence, size_t length);

 protected:
#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  enum class SpiTrans {
    kModeDio = SPI_TRANS_MODE_DIO,
    kModeQio = SPI_TRANS_MODE_QIO,
    kUseRxdata = SPI_TRANS_USE_RXDATA,
    kUseTxdata = SPI_TRANS_USE_TXDATA,
    kModeDioqioAddr = SPI_TRANS_MODE_DIOQIO_ADDR,
    kMultilineAddr = SPI_TRANS_MULTILINE_ADDR,
    kVariableCmd = SPI_TRANS_VARIABLE_CMD,
    kVariableAddr = SPI_TRANS_VARIABLE_ADDR,
    kVariableDummy = SPI_TRANS_VARIABLE_DUMMY,
    kCsKeepActive = SPI_TRANS_CS_KEEP_ACTIVE,
    kMultilineCmd = SPI_TRANS_MULTILINE_CMD,
    kModeOct = SPI_TRANS_MODE_OCT,
  };
#else
  enum class SpiTrans {
    kModeDio,
    kModeQio,
    kUseRxdata,
    kUseTxdata,
    kModeDioqioAddr,
    kMultilineAddr,
    kVariableCmd,
    kVariableAddr,
    kVariableDummy,
    kCsKeepActive,
    kMultilineCmd,
    kModeOct,
  };
#endif

  std::shared_ptr<QspiBusBase> bus_;

  int32_t cs_;
};

class UartChipBase : public virtual DriverBase {
 public:
  UartChipBase(std::shared_ptr<UartBusBase> bus) : bus_(bus) {}
  virtual bool Init(int32_t baud_rate);
  virtual bool Deinit();

 protected:
  std::shared_ptr<UartBusBase> bus_;
};

class I2sChipBase : public virtual DriverBase {
 public:
  I2sChipBase(std::shared_ptr<I2sBusBase> bus) : bus_(bus) {}
  virtual bool Init(uint16_t mclk_multiple, uint32_t sample_rate_hz,
      uint8_t data_bit_width) = 0;
  virtual bool Deinit();

#if CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ESP_IDF || \
    CPP_BUS_DRIVER_PLATFORM == CPP_BUS_DRIVER_PLATFORM_ARDUINO_ESP32
  bool ReconfigureClock(uint16_t mclk_multiple, uint32_t sample_rate_hz,
      I2sBusBase::DataMode data_mode = I2sBusBase::DataMode::kInputOutput);
#endif

 protected:
  std::shared_ptr<I2sBusBase> bus_;
};

class SdioChipBase : public virtual DriverBase {
 public:
  SdioChipBase(std::shared_ptr<SdioBusBase> bus) : bus_(bus) {}
  virtual bool Init(int32_t freq_hz);
  virtual bool Deinit();

 protected:
  std::shared_ptr<SdioBusBase> bus_;
};

class MipiChipBase : public virtual DriverBase {
 public:
  MipiChipBase(std::shared_ptr<MipiBusBase> bus,
      InitSequenceFormat init_sequence_format = InitSequenceFormat::kWriteC8D8)
      : bus_(bus), init_sequence_format_(init_sequence_format) {}
  virtual bool Init(float freq_mhz, float lane_bit_rate_mbps);
  virtual bool Deinit();
  bool InitSequence(const uint8_t* sequence, size_t length);

 protected:
  std::shared_ptr<MipiBusBase> bus_;
  InitSequenceFormat init_sequence_format_;
};

}  // namespace cpp_bus_driver
