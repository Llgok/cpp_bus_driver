# 类布局

## 格式化工具

代码格式以仓库根目录的 `.clang-format` 为准，不凭个人编辑器默认配置调整缩进、换行或对齐。当前配置基于 Google C++ 风格，并设置 `AlignAfterOpenBracket: DontAlign`，因此多行函数参数、初始化列表和调用参数按固定缩进换行，不与左括号后的首个参数对齐。

例如：

```cpp
bool WriteRegister(Register reg, const uint8_t* data,
    size_t length);
```

修改 C++ 文件后，格式化或检查时指定仓库配置和涉及文件，例如：

```powershell
clang-format -style=file -i src/chip/i2c/rx8130ce.h
clang-format -style=file --dry-run --Werror src/chip/i2c/rx8130ce.h
```

只对本次修改涉及的 C++ 文件执行格式化，不批量重排无关文件；格式化工具只处理空格、缩进、换行和对齐，不替代访问区、命名或注释规则。

## 访问区和成员顺序

类内访问区遵循 Google C++ 风格，按 `public:`、`protected:`、`private:` 的顺序组织。

每个访问区内依次放置类型声明、必要的常量、构造函数和函数，数据成员放在末尾。仅供实现使用的默认地址、默认频率、默认缓冲区长度、超时、寄存器掩码和边界值放在 `private:`。能力枚举或协议类型只有在调用方需要查询或选择时才放入 `public:`。

用一个空行分隔枚举、结构体、类定义和不同类别的成员组，类型自身的注释与类型一起分组。不同语义或不同类型的变量组之间也留空行；同一配置组中的多个常量可以连续排列，即使整数类型不同，也不需要逐个拆开。类内枚举定义结束后，与后续函数或另一类型之间留空行。

访问区标签前留一个空行；类开头的首个 `public:` 紧随类声明。标签与首个成员之间不额外插入空行。不要把“类型之间留空行”理解为在 `class` 与首个访问区之间添加空行。

常量块结束后，如果接下来是另一种声明（例如 `enum class Register`），先留空行：

```cpp
 private:
  static constexpr int32_t kDefaultI2cFrequencyHz = 100000;
  static constexpr uint8_t kDefaultI2cAddress = 0x20;

  enum class Register : uint8_t {
    kStatus = 0x00,
  };
```

函数注释按 [注释约定](comments.md) 处理：带注释函数之间留空行；没有注释且连续相关的简单函数通常不额外插入空行。

声明与定义的顺序应保持一致。移动成员时，同时移动其说明和相邻空行。
