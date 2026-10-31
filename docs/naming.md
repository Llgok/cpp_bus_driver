# 命名约定

按实际语义选择名称，不要将 `Device` 全局替换为 `Chip`。

- 表示芯片自身的地址、ID、版本和状态时使用 `chip`，例如芯片驱动中的 `chip_address`、`chip_id`、`chip_revision` 和 `ResetChip()`。
- I2C 总线接口已经由类型和命名空间表明协议时，地址参数直接命名为 `address`，未绑定地址标记使用 `kNoI2cAddress`；扫描结果是地址集合时使用 `addresses`。
- 类方法已经由芯片类型和方法语义明确地址对象时，参数也直接命名为 `address`；成员字段应按协议语义命名。例如 `Cc1101::SetAddressCheck(..., uint8_t address)` 使用 `address`，而配置结构中的无线协议本机地址使用 `local_address`。只有确实需要区分芯片物理地址、总线地址或其他地址时，才使用带限定词的名称。
- ESP-IDF API 中固定的名称保持不变，例如 `device_address`、`device_handle` 和 `spi_device_handle_t`。
- 芯片驱动对外提供的 API 按对象语义命名，例如使用 `GetChipErrors()`；底层协议命令、寄存器和子命令按手册保留原名，例如 `kRoGetDeviceErrors`、`kDeviceNumber` 和 `kRwDeviceControl`。

默认配置常量使用 `kDefault` 前缀，并包含总线或单位上下文，例如 `kDefaultI2cAddress`、`kDefaultI2cFrequencyHz` 和 `kDefaultMipiFrequencyMhz`。默认地址、默认频率和默认缓冲区长度通常只服务构造函数或函数默认参数，应放在私有区；只有公共 API 明确要求调用方查询、选择或复用时才公开。容量、能力和协议选项也按同一原则判断。
