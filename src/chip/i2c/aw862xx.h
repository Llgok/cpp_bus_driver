/*
 * @Description: AW862xx 触觉反馈驱动芯片接口
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2026-06-03 09:45:41
 * @License: GPL 3.0
 */
#pragma once

#include "../chip_guide.h"

namespace cpp_bus_driver {

class Aw862xx final : public ChipI2cGuide {
 public:
  enum class PlayMode {
    kRam = 0,
    kRtp,
    kCont,
    kNoPlay,
  };

  enum class GlobalStatus {
    kFalse = -1,

    kStandby = 0,
    kCont = 6,
    kRam,
    kRtp,
    kTrig,
    kBrake = 11,
  };

  enum class SampleRate {
    kRate24Khz = 0,
    kRate48Khz,
    kRate12Khz,
  };

  enum class D2sGain {
    kGain1 = 0,
    kGain2,
    kGain4,
    kGain5,
    kGain8,
    kGain10,
    kGain20,
    kGain40,
  };

  enum class ChipType {
    kUnknown = 0,  // 未知芯片
    kAw8623,
    kAw8624,
    kAw86214,
    kAw86223,
    kAw86224,
    kAw86225,
    kAw86233,
    kAw86234,
    kAw86235,
    kAw86243,
    kAw86245,
  };

  enum class RamWaveformLibrary {
    kRam12k101635_130,
    kRam12k0809_170,
    kRam12k0815_170,
    kRam12k9595_170,
    kRam12k0832_205,
    kRam12k0832_235,
    kRam12k041230_235,
    kRam12k041235_240,
    kRam12k0832_260,
    kRam24k101635_130,
    kRam24k0619_170,
    kRam24k0809_170,
    kRam24k0815_170,
    kRam24k1010_170,
    kRam24k1040_170,
    kRam24k9595_170,
    kRam24k0832_205,
    kRam24k0832_235,
    kRam24k041230_235,
    kRam24k041235_240,
    kRam24k0832_260,
  };

  enum class ForceMode {
    kActive,   // 激活模式
    kStandby,  // 待机模式
  };

  struct SystemStatus {
    bool uvls_flag = false;              // VDD电压低于UV电压标志
    bool rtp_fifo_empty = true;          // RTP模式FIFO为空标志
    bool rtp_fifo_full = false;          // RTP模式FIFO为满标志
    bool over_current_flag = false;      // 过流标志
    bool over_temperature_flag = false;  // 过温标志
    bool playback_flag = false;          // 回放标志
  };

#include "aw862xx_haptic_waveform_table.h"

  struct RamWaveformInfo {
#if defined(CPP_BUS_DRIVER_DEVELOPMENT_FRAMEWORK_ARDUINO_NRF)
    RamWaveformInfo() = default;

    RamWaveformInfo(const char* name, const uint8_t* data, size_t length,
        SampleRate sample_rate, uint16_t rated_f0_hz, uint8_t waveform_count)
        : name(name),
          data(data),
          length(length),
          sample_rate(sample_rate),
          rated_f0_hz(rated_f0_hz),
          waveform_count(waveform_count) {}
#endif

    const char* name = nullptr;
    const uint8_t* data = nullptr;
    size_t length = 0;
    SampleRate sample_rate = SampleRate::kRate12Khz;
    uint16_t rated_f0_hz = 0;
    uint8_t waveform_count = 0;
  };

  explicit Aw862xx(std::shared_ptr<BusI2cGuide> bus,
      int16_t address = kDeviceI2cAddressDefault, int32_t rst = kDefaultValue)
      : ChipI2cGuide(bus, address), rst_(rst) {}

  bool Init(int32_t freq_hz = kDefaultValue) override;
  bool Deinit(bool delete_bus = true) override;

  /**
   * @brief 读取并识别AW862xx芯片类型
   * @return 返回芯片类型，无法识别返回ChipType::kUnknown
   */
  ChipType GetDeviceId();

  /**
   * @brief 软件复位
   * @return 成功返回 true，失败返回 false
   */
  bool SoftwareReset();

  /**
   * @brief 获取输入电压
   * @return 返回读取到的数值
   */
  float GetInputVoltage();

  /**
   * @brief 设置GO TRIG的波形播放模式，kRam/kRtp/kCont/NO kPlay 模式
   * @param mode 播放模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetPlayMode(PlayMode mode);

  /**
   * @brief kRam/kRtp/kCont 模式播放触发位，当设置为 1
   * 时，芯片将播放其中一种播放模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetGoFlag();

  /**
   * @brief 获取全局状态
   * @return 返回状态枚举值
   */
  GlobalStatus GetGlobalStatus();

  /**
   * @brief 在 RTP 模式下播放波形库数据
   * @param waveform_data 波形数据指针
   * @param length 波形输出长度
   * @return 操作成功返回 true，失败返回 false
   */
  bool RunRtpPlaybackWaveform(const uint8_t* waveform_data, size_t length);

  /**
   * @brief 设置波形库数据的播放采样率
   * @param rate 目标采样率
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetWaveformDataSampleRate(SampleRate rate);

  /**
   * @brief 获取内置 RAM 波形库信息
   * @param library RAM 波形库类型
   * @return 返回信息结构体
   */
  static RamWaveformInfo GetRamWaveformInfo(RamWaveformLibrary library);

  /**
   * @brief 设置在播放的时候改变增益
   * @param enable [true]：在播放的时候改变增益，[false]：在播放的时候不改变增益
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetPlayingChangedGainBypass(bool enable);

  /**
   * @brief 设置 D2S 增益
   * @param gain D2S 增益档位
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetD2sGain(D2sGain gain);

  /**
   * @brief 调整LRA（线性振动马达）的频率，以适应LRA共振频率的偏差
   * @param freq_hz 值范围：0~63
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetLraOscFrequency(uint8_t freq_hz);

  /**
   * @brief 设置是否启动f0校验模式
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetF0DetectionMode(bool enable);

  /**
   * @brief 设置振动马达开关
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetTrackSwitch(bool enable);

  /**
   * @brief 设置在RTP/kRam/CONT播放模式停止后启用自动制动（当设置为1时）
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetAutoBrakeStop(bool enable);

  /**
   * @brief 设置第一个连续驱动阶段的输出电压级别
   * @param level 值范围：0~127
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetContDrive1Level(uint8_t level);

  /**
   * @brief 设置第二个连续驱动阶段的输出电压级别
   * @param level 值范围：0~127
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetContDrive2Level(uint8_t level);

  /**
   * @brief 设置第1次连续驱动的半循环次数
   * @param times 值范围：0~255
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetContDrive1Times(uint8_t times);

  /**
   * @brief 设置第2次连续驱动的半循环次数
   * @param times 值范围：0~255
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetContDrive2Times(uint8_t times);

  /**
   * @brief 设置跟踪余量值，跟踪余量值越小，跟踪精度越高
   * @param value 值范围：0~255
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetContTrackMargin(uint8_t value);

  /**
   * @brief 设置制动器的半周期驱动时间（code/48K
   * s），该值必须小于一半F0的循环时间，
   * 建议将DRV_WIDTH配置为（24k/F0）-8-kTrackMargin-kBrkGain
   * @param value 值范围：0~255
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetContDriveWidth(uint8_t value);

  /**
   * @brief 获取f0检测的值
   * @return 返回读取到的数值
   */
  uint32_t GetF0Detection();

  /**
   * @brief 设置F0校准参考值，单位为0.1Hz
   * @param f0_0p1_hz F0参考值，例如1700表示170.0Hz
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetF0Preset(uint32_t f0_0p1_hz);

  /**
   * @brief 获取当前F0校准参考值，单位为0.1Hz
   * @return 返回读取到的数值
   */
  uint32_t f0_preset() const { return f0_value_; }

  /**
   * @brief 输入f0的值开始进行f0校准
   * @param f0_value GetF0Detection()函数获取的f0的值
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetF0Calibrate(uint32_t f0_value);

  /**
   * @brief 获取系统状态
   * @param status 使用System_Status结构体配置
   * @return 读取成功返回 true，失败返回 false
   */
  bool GetSystemStatus(SystemStatus& status);

  /**
   * @brief 设置数字时钟启动
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetClock(bool enable);

  /**
   * @brief 设置RAM初始化模式
   * @param enable [true]：进入RAM初始化模式，[false]：退出RAM初始化模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRamInit(bool enable);

  /**
   * @brief 设置在RTP、kRam、TRIG模式下的增益
   * @param gain 值范围0~255
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRrtModeGain(uint8_t gain);

  /**
   * @brief 初始化 RAM 模式
   * @param waveform_data 波形数据指针
   * @param length 波形输出长度
   * @return 初始化成功返回 true，失败返回 false
   */
  bool InitRamMode(const uint8_t* waveform_data, size_t length);

  /**
   * @brief 初始化内置 RAM 波形库
   * @param library RAM 波形库类型
   * @return 初始化成功返回 true，失败返回 false
   */
  bool InitRamMode(RamWaveformLibrary library);

  /**
   * @brief 获取 RAM 波形数据中的序列数量
   * @param waveform_data RAM 波形数据指针
   * @param length RAM 波形数据长度
   * @return 返回读取到的数值
   */
  static uint8_t GetRamWaveformCount(
      const uint8_t* waveform_data, size_t length);

  /**
   * @brief 设置 RAM 模式的波形数据并启动播放
   * @param waveform_sequence_number 波形库序列号，范围为 0～127
   * @param loop_count 循环次数，范围为 0～15；15 表示无限循环
   * @param gain 值范围0~255，配置增益
   * @param auto_brake [true]：启动，[false]：关闭，自动制动
   * @param gain_bypass 是否允许播放期间改变增益
   * @return 操作成功返回 true，失败返回 false
   */
  bool RunRamPlaybackWaveform(uint8_t waveform_sequence_number,
      uint8_t loop_count, uint8_t gain = 127, bool auto_brake = true,
      bool gain_bypass = true);

  /**
   * @brief 配置RAM模式播放参数但不启动播放
   * @param waveform_sequence_number RAM波形sequence编号
   * @param loop_count 寄存器循环值，范围0~15，15为无限循环
   * @param gain 增益，范围0~255
   * @param auto_brake [true]：启用自动制动，[false]：关闭自动制动
   * @param gain_bypass [true]：播放时允许改变增益，[false]：播放时不改变增益
   * @return 操作成功返回 true，失败返回 false
   */
  bool ConfigureRamPlaybackWaveform(uint8_t waveform_sequence_number,
      uint8_t loop_count, uint8_t gain = 127, bool auto_brake = true,
      bool gain_bypass = true);

  /**
   * @brief 启动当前已配置的RAM模式播放
   * @return 操作成功返回 true，失败返回 false
   */
  bool StartRamPlaybackWaveform();

  /**
   * @brief 播放RAM波形，loop_count为实际播放次数
   * @param waveform_sequence_number RAM波形sequence编号
   * @param loop_count 播放次数，范围1~16
   * @param gain 增益，范围0~255
   * @param auto_brake [true]：启用自动制动，[false]：关闭自动制动
   * @param gain_bypass [true]：播放时允许改变增益，[false]：播放时不改变增益
   * @return 操作成功返回 true，失败返回 false
   */
  bool PlayRamWaveform(uint8_t waveform_sequence_number, uint8_t loop_count = 1,
      uint8_t gain = 255, bool auto_brake = false, bool gain_bypass = true);

  /**
   * @brief 无限循环播放RAM波形
   * @param waveform_sequence_number RAM波形sequence编号
   * @param gain 增益，范围0~255
   * @param auto_brake [true]：启用自动制动，[false]：关闭自动制动
   * @param gain_bypass [true]：播放时允许改变增益，[false]：播放时不改变增益
   * @return 操作成功返回 true，失败返回 false
   */
  bool PlayRamLoopWaveform(uint8_t waveform_sequence_number, uint8_t gain = 255,
      bool auto_brake = true, bool gain_bypass = true);

  /**
   * @brief 设置RAM播放sequence槽位
   * @param slot 槽位编号，范围0~7
   * @param waveform_sequence_number RAM波形sequence编号
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRamWaveformSequence(uint8_t slot, uint8_t waveform_sequence_number);

  /**
   * @brief 设置RAM播放sequence槽位循环次数寄存器值
   * @param slot 槽位编号，范围0~7
   * @param loop_count 寄存器循环值，范围0~15，15为无限循环
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRamWaveformLoop(uint8_t slot, uint8_t loop_count);

  /**
   * @brief 设置停止标志位，设置该标志将停止当前的振动模式
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetStopFlag();

  /**
   * @brief 设置强制进入的模式
   * @param mode 强制控制模式
   * @param enable [true]：启动，[false]：关闭
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetForceEnterMode(ForceMode mode, bool enable);

  /**
   * @brief RAM模式播放停止
   * @return 操作成功返回 true，失败返回 false
   */
  bool StopRamPlaybackWaveform();

 private:
  enum class Cmd {
    kRoDeviceId = 0x00,
    kWoSrst = kRoDeviceId,
    kRoSysst,
    kRcSysint,
    kRwSysintm,
    kRoSyssst2,

    kRwPlaycfg2 = 0x07,
    kRwPlaycfg3,
    kRwPlaycfg4,
    kRwWavcfg1,
    kRwWavcfg2,
    kRwWavcfg3,
    kRwWavcfg4,
    kRwWavcfg5,
    kRwWavcfg6,
    kRwWavcfg7,
    kRwWavcfg8,
    kRwWavcfg9,
    kRwWavcfg10,
    kRwWavcfg11,
    kRwWavcfg12,
    kRwWavcfg13,

    kRwContcfg1 = 0x18,
    kRwContcfg2,
    kRwContcfg3,
    kRwContcfg4,
    kRwContcfg5,
    kRwContcfg6,
    kRwContcfg7,
    kRwContcfg8,
    kRwContcfg9,
    kRwContcfg10,
    kRwContcfg11,

    kRoContrd14 = 0x25,
    kRoContrd15,
    kRoContrd16,
    kRoContrd17,

    kRwRtpcfg1 = 0x2D,
    kRwRtpcfg2,
    kRwRtpcfg3,
    kRwRtpcfg4,
    kRwRtpcfg5,
    kRwRtpdata,
    kRwTrgcfg1,

    kRwTrgcfg4 = 0x36,
    kRwTrgcfg7 = 0x39,
    kRwTrgcfg8,

    kRwGlbcfg2 = 0x3C,
    kRwGlbcfg4 = 0x3E,
    kRoGlbrd5,
    kRwRamaddrh,
    kRwRamaddrl,
    kRwRamadata,
    kRwSysctrl1,
    kRwSysctrl2,
    kRwSysctrl3,
    kRwSysctrl4,
    kRwSysctrl5,
    kRwSysctrl6,

    kRwSysctrl7 = 0x49,
    kRwPwmcfg1 = 0x4C,
    kRwPwmcfg2,
    kRwPwmcfg3,
    kRwPwmcfg4,

    kRwDetcfg1 = 0x51,
    kRwDetcfg2,
    kRwDetRl,

    kRwDetVbat = 0x55,
    kRoChipIdHigh = 0x57,
    kRwDetLo = 0x57,
    kRoAw8624xChipIdLow = 0x58,
    kRwTrimcfg3 = 0x5A,
    kRoEfId = 0x64,
    kRoAw8623xChipIdLow = 0x69,
    kRwAnacfg8 = 0X77,
  };

  static constexpr uint8_t kDeviceI2cAddressDefault = 0x58;

  /**
   * @brief 解析RAM波形头，获取base地址和sequence数量
   * @param waveform_data RAM 波形数据指针
   * @param length RAM波形数据长度
   * @param base_addr 返回RAM base地址
   * @param waveform_count 返回sequence数量
   * @return 解析成功返回 true，失败返回 false
   */
  static bool ParseRamHeader(const uint8_t* waveform_data, size_t length,
      uint16_t& base_addr, uint8_t& waveform_count);

  /**
   * @brief 获取芯片类型名称
   * @param chip_type 芯片类型
   * @return 芯片类型名称字符串
   */
  static const char* ChipTypeToString(ChipType chip_type);

  /**
   * @brief 获取RAM波形采样率名称。
   * @param sample_rate RAM波形采样率。
   * @return RAM波形采样率名称字符串。
   */
  static const char* SampleRateToString(SampleRate sample_rate);

  /**
   * @brief 设置RAM base地址
   * @param base_addr RAM base地址
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRamBaseAddress(uint16_t base_addr);

  /**
   * @brief 设置RAM/RTP FIFO almost empty和almost full阈值
   * @param base_addr RAM base地址
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRamFifoThreshold(uint16_t base_addr);

  /**
   * @brief 设置RAM读写地址
   * @param ram_addr RAM读写地址
   * @return 设置成功返回 true，失败返回 false
   */
  bool SetRamAddress(uint16_t ram_addr);

  int32_t rst_;
  uint32_t f0_value_ = 1700;
  RamWaveformInfo ram_waveform_info_;
  ChipType chip_type_ = ChipType::kUnknown;
};
}  // namespace cpp_bus_driver
