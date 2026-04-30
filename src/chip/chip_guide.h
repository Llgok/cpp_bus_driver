/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-04-30 13:47:25
 * @License: GPL 3.0
 */
#pragma once

#include "../bus/bus_guide.h"

namespace cpp_bus_driver {
class ChipI2cGuide : public Tool {
 public:
  ChipI2cGuide(std::shared_ptr<BusI2cGuide> bus, int16_t address)
      : bus_(bus), address_(address) {}

  virtual bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);
  virtual bool Deinit(bool delete_bus = false);

  bool InitSequence(const uint8_t* sequence, size_t length);
  bool InitSequence(const uint16_t* sequence, size_t length);

 protected:
  std::shared_ptr<BusI2cGuide> bus_;

 private:
  int16_t address_;
};

class ChipSpiGuide : public Tool {
 public:
  ChipSpiGuide(std::shared_ptr<BusSpiGuide> bus,
      int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : bus_(bus), cs_(cs) {}

  virtual bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);
  virtual bool Deinit(bool delete_bus = false);

  bool InitSequence(const uint8_t* sequence, size_t length);

 protected:
  std::shared_ptr<BusSpiGuide> bus_;

  int32_t cs_;
};

class ChipQspiGuide : public Tool {
 public:
  ChipQspiGuide(std::shared_ptr<BusQspiGuide> bus,
      int32_t cs = CPP_BUS_DRIVER_DEFAULT_VALUE)
      : bus_(bus), cs_(cs) {}

  virtual bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);
  virtual bool Deinit();

  bool InitSequence(const uint32_t* sequence, size_t length);

 protected:
#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
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

  std::shared_ptr<BusQspiGuide> bus_;

  int32_t cs_;
};

class ChipUartGuide : public Tool {
 public:
  ChipUartGuide(std::shared_ptr<BusUartGuide> bus) : bus_(bus) {}

  virtual bool Init(int32_t baud_rate = CPP_BUS_DRIVER_DEFAULT_VALUE);
  virtual bool Deinit();

 protected:
  std::shared_ptr<BusUartGuide> bus_;
};

class ChipI2sGuide : public Tool {
 public:
  ChipI2sGuide(std::shared_ptr<BusI2sGuide> bus) : bus_(bus) {}

  virtual bool Init(uint16_t mclk_multiple, uint32_t sample_rate_hz,
      uint8_t data_bit_width) = 0;
  virtual bool Deinit();

#if defined CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF
  bool SetClockReconfig(uint16_t mclk_multiple, uint32_t sample_rate_hz,
      BusI2sGuide::DataMode data_mode = BusI2sGuide::DataMode::kInputOutput);
#endif

 protected:
  std::shared_ptr<BusI2sGuide> bus_;
};

class ChipSdioGuide : public Tool {
 public:
  ChipSdioGuide(std::shared_ptr<BusSdioGuide> bus) : bus_(bus) {}

  virtual bool Init(int32_t freq_hz = CPP_BUS_DRIVER_DEFAULT_VALUE);
  virtual bool Deinit();

 protected:
  std::shared_ptr<BusSdioGuide> bus_;
};

class ChipMipiGuide : public Tool {
 public:
  ChipMipiGuide(std::shared_ptr<BusMipiGuide> bus,
      InitSequenceFormat init_sequence_format = InitSequenceFormat::kWriteC8D8)
      : bus_(bus), init_sequence_format_(init_sequence_format) {}

  virtual bool Init(float freq_mhz = CPP_BUS_DRIVER_DEFAULT_VALUE,
      float lane_bit_rate_mbps = CPP_BUS_DRIVER_DEFAULT_VALUE);
  virtual bool Deinit();

  bool InitSequence(const uint8_t* sequence, size_t length);

 protected:
  std::shared_ptr<BusMipiGuide> bus_;
  InitSequenceFormat init_sequence_format_;
};
}  // namespace cpp_bus_driver
