/*
 * @Description: ES8311 音频编解码芯片驱动接口
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:42:57
 * @LastEditTime: 2026-04-30 13:44:48
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Es8311 final : public ChipI2cGuide, public ChipI2sGuide {
 public:
  enum class ClockSource {
    kAdcDacMclk = 0,
    kAdcDacBclk,
  };

  enum class MicType {
    kAnalogMic = 0,  // 模拟麦克风
    kDigitalMic,     // 数字麦克风
  };

  enum class MicInput {
    kNoInput = 0,
    kMic1p1n,
    kMic2p2n,
    kMic1p1nMic2p2n,
  };

  enum class Vmid {
    kPowerDown = 0,
    kStartUpVmidNormalSpeedCharge,
    kNormalVmidOperation,
    kStartUpVmidFastSpeedCharge,
  };

  enum class Sdp {
    kDac = 0,
    kAdc,
  };

  enum class BitsPerSample {
    kData24bit = 0,
    kData20bit,
    kData18bit,
    kData16bit,
    kData32bit,
  };

  enum class AdcOffsetFreeze {
    // 冻结偏置（Offset
    // Freeze）功能用于在ADC开始工作时捕获输入信号的初始直流偏置，并将其固定在整个采样过程中
    // 这意味着ADC会忽略输入信号中的任何缓慢变化的直流分量，只对交流成分进行量化
    kFreezeOffset = 0,

    // 动态高通滤波器（Dynamic High-Pass
    // Filter，kHpf）是一种能够根据信号的变化自动调整其截止频率的滤波器
    // 在这种模式下，ADC会在信号稳定后应用一个高通滤波器，以去除低频噪声和直流偏置
    kDynamicHpf,
  };

  enum class AdcGain {
    kGain0db = 0,
    kGain6db,
    kGain12db,
    kGain18db,
    kGain24db,
    kGain30db,
    kGain36db,
    kGain42db,
  };

  enum class AdcPgaGain {
    kGain0db = 0,
    kGain3db,
    kGain6db,
    kGain9db,
    kGain12db,
    kGain15db,
    kGain18db,
    kGain21db,
    kGain24db,
    kGain27db,
    kGain30db,
  };

  enum class SerialPortMode {
    kSlave,
    kMaster,
  };

  enum class AdcDataFormat {
    kAdcAdc = 0,
    kAdcNone,
    kNoneAdc,
    kNoneNone,
    kDaclAdc,
    kAdcDacr,
    kDaclDacr,
    NA,
  };

  struct PowerStatus {
    // 控制设置，[true]：启动，[false]：关闭
    struct {
      bool analog_circuits = false;                // 模拟电路
      bool analog_bias_circuits = false;           // 模拟偏置电路
      bool analog_adc_bias_circuits = false;       // 模拟ADC偏置电路
      bool analog_adc_reference_circuits = false;  // 模拟ADC参考电路
      bool analog_dac_reference_circuit = false;   // 模拟DAC参考电路
      bool internal_reference_circuits = false;    // 内部参考电路
    } contorl;

    Vmid vmid = Vmid::kPowerDown;
  };

  // 低功耗状态
  // 控制设置，[true]：启动低功耗，[false]：关闭低功耗
  struct LowPowerStatus {
    bool dac = false;
    bool pga = false;
    bool pga_output = false;
    bool adc = false;
    bool adc_reference = false;
    bool dac_reference = false;
    bool flash = false;  // 闪存
    bool int1 = false;   // 中断
  };

  explicit Es8311(std::shared_ptr<BusI2cGuide> i2c_bus,
      std::shared_ptr<BusI2sGuide> i2s_bus, int16_t i2c_address,
      int32_t rst = kDefaultValue)
      : ChipI2cGuide(i2c_bus, i2c_address), ChipI2sGuide(i2s_bus), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;
  bool Init(uint16_t mclk_multiple, uint32_t sample_rate_hz,
      uint8_t data_bit_width) override;
  uint16_t GetDeviceId();

  /**
   * @brief 软件复位
   * ，在编解码器准备好待机或睡眠时，将所有复位位设置为“1”，并将CSM_ON清除为“0”，以最大限度地降低功耗
   * @param enable true 表示复位，false 表示解除复位
   * @return 成功返回 true，失败返回 false
   */
  bool SoftwareReset(bool enable);

  /**
   * @brief 设置主时钟源
   * @param clock 时钟源
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetMasterClockSource(ClockSource clock);

  /**
   * @brief 设置时钟启动
   * @param clock 时钟源
   * @param enalbe [true]：启动，[false]：关闭
   * @param invert [true]：反转，[false]：不反转
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetClock(ClockSource clock, bool enalbe, bool invert = false);

  /**
   * @brief 设置DAC的音量
   * @param volume 值范围0~255，增益范围 -95.5dB 到 +32dB，步进0.5dB，0dB为191
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDacVolume(uint8_t volume);

  /**
   * @brief 设置 ADC 音量；启用自动音量控制后该设置不生效
   * @param volume 值范围0~255，增益范围 -95.5dB 到 +32dB，步进0.5dB，0dB为191
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcVolume(uint8_t volume);

  /**
   * @brief 设置ADC自动控制音量
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcAutoVolumeControl(bool enable);

  /**
   * @brief 设置MIC1P引脚的麦克风使用类型
   * @param type 麦克风类型
   * @param input 麦克风输入通道
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetMic(MicType type, MicInput input);

  /**
   * @brief 设置电源状态
   * @param status 使用 PowerStatus 结构体配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetPowerStatus(PowerStatus status);

  /**
   * @brief 设置低功耗状态；启用后可降低功耗，但会略微降低音频性能
   * @param status 使用 LowPowerStatus 结构体配置
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetLowPowerStatus(LowPowerStatus status);

  /**
   * @brief 根据主时钟倍频和采样率选择最佳时钟系数
   * @param mclk_multiple 主时钟倍频
   * @param sample_rate_hz 采样率
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetClockCoeff(uint16_t mclk_multiple, uint32_t sample_rate_hz);

  /**
   * @brief 设置SDP字节长度
   * @param sdp 串行数据端口
   * @param length 每个采样的数据位数
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetSdpDataBitLength(Sdp sdp, BitsPerSample length);

  /**
   * @brief 设置PGA电源
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetPgaPower(bool enable);

  /**
   * @brief 设置ADC电源
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcPower(bool enable);

  /**
   * @brief 设置 DAC 电源
   * @param enable true 表示启用，false 表示关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDacPower(bool enable);

  /**
   * @brief 设置输出到HP驱动器
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetOutputToHpDrive(bool enable);

  /**
   * @brief 设置ADC处理信号中的直流偏置和高频噪声的模式
   * @param offset_freeze ADC 直流偏置冻结模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcOffsetFreeze(AdcOffsetFreeze offset_freeze);

  /**
   * @brief 设置ADC的HPF第二系数
   * @param coeff 值范围0~31
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcHpfStage2Coeff(uint8_t coeff);

  /**
   * @brief 设置DAC的均衡器（EQ）
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetDacEqualizer(bool enable);

#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ESPIDF)
  /**
   * @brief 读取I2s数据
   * @param data 接收数据指针
   * @param byte 字节长度
   * @return 实际读取到的字节数
   */
  size_t ReadI2s(void* data, size_t byte);

  /**
   * @brief 写入I2s数据
   * @param data 待写入数据指针
   * @param byte 字节长度
   * @return 实际写入的字节数
   */
  size_t WriteI2s(const void* data, size_t byte);

  /**
   * @brief 重新配置时钟信息
   * @param mclk_multiple mclk倍速
   * @param sample_rate_hz 采样率
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetClockReconfig(uint16_t mclk_multiple, uint32_t sample_rate_hz);

  /**
   * @brief 设置i2s通道使能
   * @param enable [true]：启用i2s，[false]：关闭i2s
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetI2sChannelEnable(bool enable);
#elif defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)

  /**
   * @brief 开始 I2S 数据流传输
   * @param write_buffer 写数据缓冲区；为空时不发送，且必须位于 RAM
   * @param read_buffer 读数据缓冲区；为空时不接收，且必须位于 RAM
   * @param max_buffer_length 数据流缓存最大长度
   * @return 操作成功返回 true，失败返回 false
   */
  bool StartTransmitI2s(
      uint32_t* write_buffer, uint32_t* read_buffer, size_t max_buffer_length);

  /**
   * @brief 停止I2s数据流传输
   */
  void StopTransmitI2s();

  /**
   * @brief 设置下一个读取的I2s指针
   * @param data 数据指针
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetNextReadI2s(uint32_t* data);

  /**
   * @brief 设置下一个写入的I2s指针
   * @param data 数据指针
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetNextWriteI2s(uint32_t* data);

  /**
   * @brief 获取读取I2s事件标志
   * @return [true]：有数据可读，[false]：无数据可读
   */
  bool GetReadI2sEventFlag();

  /**
   * @brief 获取写入I2s事件标志
   * @return [true]：可以继续写入数据，[false]：不能写入数据
   */
  bool GetWriteI2sEventFlag();

#endif

  /**
   * @brief ADC增益
   * @param gain ADC 增益
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcGain(AdcGain gain);

  /**
   * @brief 设置ADC数据全部输出到DAC上
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcDataToDac(bool enable);

  /**
   * @brief ADC的PGA增益
   * @param gain ADC PGA 增益
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcPgaGain(AdcPgaGain gain);

  /**
   * @brief 设置串行模式
   * @param mode 串行音频端口模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetSerialPortMode(SerialPortMode mode);

  /**
   * @brief 设置ADC数据传输格式，用于回声消除
   * @param format ADC 数据格式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAdcDataFormat(AdcDataFormat format);

 private:
  enum class Cmd {
    kRoDeviceIdStart = 0xFD,  // 连续读取两次返回芯片ID 0x8311

    kRwResetSerialPortModeControl = 0x00,
    kRwClockManager1,
    kRwClockManager2,
    kRwClockManager3,
    kRwClockManager4,
    kRwClockManager5,
    kRwClockManager6,
    kRwClockManager7,
    kRwClockManager8,
    kRwSdpInFormat,
    kRwSdpOutFormat,

    kRwPowerUpPowerDownContorl = 0x0D,
    kRwPgaAdcModulatorPowerControl,
    kRwLowPowerControl,

    kRwDacPowerControl = 0x12,
    kRwOutputToHpDriveControl,

    kRwAdcDmicPgaGain = 0x14,
    kRwAdcGainScaleUp = 0x16,
    kRwAdcVolume,
    kRwAdcAlc,

    kRwAdcEqualizerBypass = 0x1C,
    kRwDacVolume = 0x32,
    kRwDacRamprateEqbypass = 0x37,
    kRwAdcDacControlAdcdatSel = 0x44,

  };

  // 时钟系数结构体
  struct ClockCoeff {
    uint32_t mclk_multiple;  // mclk倍速
    uint32_t sample_rate;    // 采样率
    uint8_t pre_div;         // 前分频器，范围从1到8
    uint8_t pre_multi;       // 前倍频器选择，0: 1x, 1: 2x, 2: 4x, 3: 8x
    uint8_t adc_div;         // adcclk分频器
    uint8_t dac_div;         // dacclk分频器
    uint8_t fs_mode;         // 双速或单速，=0, 单速, =1, 双速
    uint8_t lrck_h;          // adclrck分频器和daclrck分频器高字节
    uint8_t lrck_l;          // adclrck分频器和daclrck分频器低字节
    uint8_t bclk_div;        // sclk分频器
    uint8_t adc_osr;         // adc过采样率
    uint8_t dac_osr;         // dac过采样率
  };

  static constexpr uint8_t kDeviceI2cAddress1 = 0x18;
  static constexpr uint8_t kDeviceI2cAddress2 = 0x19;
  static constexpr uint8_t kDeviceI2cAddressDefault = kDeviceI2cAddress1;
  static constexpr uint16_t kDeviceId = 0x8311;
  // 时钟分配系数列表
  static constexpr ClockCoeff kClockCoeffTable_[] = {
      // 每项字段的排列顺序与 ClockCoeff 声明一致。

      // 8 kHz 采样率
      {2304, 8000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x10},
      {2048, 8000, 0x08, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {1536, 8000, 0x06, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {1024, 8000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {768, 8000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {512, 8000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {384, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {192, 8000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 11.025 kHz 采样率
      {1024, 11025, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {512, 11025, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 11025, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 12 kHz 采样率
      {1024, 12000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {512, 12000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 12000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 16 kHz 采样率
      {1152, 16000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
      {1024, 16000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {768, 16000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {512, 16000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {384, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 16000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {192, 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {96, 16000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 22.05 kHz 采样率
      {512, 22050, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 22050, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {32, 22050, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 24 kHz 采样率
      {768, 24000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {512, 24000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 24000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 32 kHz 采样率
      {576, 32000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
      {512, 32000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {384, 32000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 32000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {192, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {96, 32000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {48, 32000, 0x03, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
      {32, 32000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 44.1 kHz 采样率
      {256, 44100, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {32, 44100, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 48 kHz 采样率
      {384, 48000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {256, 48000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {32, 48000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

      // 64 kHz 采样率
      {288, 64000, 0x03, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
      {256, 64000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {192, 64000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {96, 64000, 0x01, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
      {64, 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {48, 64000, 0x01, 0x03, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
      {32, 64000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {24, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
      {16, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

      // 88.2 kHz 采样率
      {128, 88200, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {32, 88200, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {16, 88200, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

      // 96 kHz 采样率
      {192, 96000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {128, 96000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {64, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {32, 96000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
      {16, 96000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
  };

  /**
   * @brief 搜索时钟系数
   * @param mclk_multiple mclk倍速
   * @param sample_rate_hz 采样率
   * @param library 待搜索的 ClockCoeff 系数表
   * @param library_length 查找的库的长度
   * @param search_index 搜索结果索引
   * @return 成功返回 true，失败返回 false
   */
  bool SearchClockCoeff(uint16_t mclk_multiple, uint32_t sample_rate_hz,
      const ClockCoeff* library, size_t library_length,
      size_t* search_index = nullptr);

  ClockCoeff clock_coeff_;
  int32_t rst_;
};
}  // namespace cpp_bus_driver
