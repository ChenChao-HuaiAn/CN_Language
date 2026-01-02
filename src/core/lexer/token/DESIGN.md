# CN_Language Token模块 设计文档

## 设计概述

Token模块是CN_Language词法分析器的核心组件，负责定义和管理源代码中的词法单元（Token）。本模块采用模块化设计，遵循单一职责原则，提供完整的Token类型定义、创建、销毁、查询和操作功能。

## 设计原则

### 1. 单一职责原则
- Token模块只负责Token相关的功能
- 不包含词法分析逻辑（由词法分析器模块负责）
- 不包含语法分析逻辑（由语法分析器模块负责）

### 2. 开闭原则
- 支持通过枚举扩展新的Token类型
- 接口设计允许未来添加新功能而不修改现有代码
- 通过抽象接口支持不同的Token实现

### 3. 里氏替换原则
- 所有Token操作函数对NULL参数安全
- 一致的错误处理机制
- 可预测的函数行为

### 4. 接口隔离原则
- 提供细粒度的函数接口
- 客户端只依赖它们需要的接口
- 避免"胖接口"问题

### 5. 依赖倒置原则
- 高层模块定义抽象接口
- 低层模块实现具体功能
- 减少模块间的耦合

## 架构设计

### 模块分层

```
应用层 (Application Layer)
    ↓
词法分析器 (Lexer) - 未来实现
    ↓
Token模块 (Token Module) ← 本模块
    ↓
基础设施层 (Infrastructure Layer)
    - 内存管理
    - 字符串处理
    - 标准库
```

### 核心组件

1. **Token类型系统** (`Eum_TokenType`)
   - 定义所有可能的Token类型
   - 支持关键字、标识符、字面量、运算符、分隔符等

2. **Token数据结构** (`Stru_Token_t`)
   - 封装Token的所有信息
   - 包含类型、原始字符串、位置信息、字面量值

3. **Token操作接口**
   - 创建和销毁接口
   - 查询和比较接口
   - 打印和格式化接口

4. **运算符属性系统**
   - 优先级计算
   - 结合性判断

## 数据结构设计

### Stru_Token_t 设计考虑

```c
typedef struct Stru_Token_t
{
    enum Eum_TokenType type;         // Token类型（4字节）
    char* lexeme;                    // 原始字符串指针（8字节）
    size_t lexeme_length;            // 字符串长度（8字节）
    int line_number;                 // 行号（4字节）
    int column_number;               // 列号（4字节）
    union {                          // 字面量值联合体（8字节）
        long integer_value;
        double float_value;
        bool boolean_value;
    } literal_value;
} Stru_Token_t;                      // 总计：36字节（考虑对齐）
```

**设计决策**：
1. **动态字符串存储**：lexeme使用动态分配，避免固定长度限制
2. **位置信息**：包含行号和列号，便于错误定位
3. **字面量联合体**：节省内存，不同类型字面量共享存储空间
4. **类型安全**：通过枚举确保类型安全

### Eum_TokenType 枚举设计

**分类策略**：
1. **关键字类型**：`Eum_TOKEN_KEYWORD_` 前缀
2. **标识符类型**：`Eum_TOKEN_IDENTIFIER`
3. **字面量类型**：`Eum_TOKEN_*_LITERAL` 后缀
4. **运算符类型**：`Eum_TOKEN_OPERATOR_` 前缀
5. **分隔符类型**：`Eum_TOKEN_` + 符号名称
6. **特殊类型**：`Eum_TOKEN_EOF`, `Eum_TOKEN_ERROR` 等

**扩展性考虑**：
- 预留数值范围供未来扩展
- 按功能分组，便于分类查询

## 接口设计

### 创建和销毁接口

```c
// 工厂模式：创建Token
Stru_Token_t* F_token_create(enum Eum_TokenType type, ...);

// 专门工厂：创建字面量Token
Stru_Token_t* F_token_create_literal(enum Eum_TokenType type, ...);

// 资源管理：销毁Token
void F_token_destroy(Stru_Token_t* token);

// 原型模式：复制Token
Stru_Token_t* F_token_copy(const Stru_Token_t* src);
```

**设计模式应用**：
- **工厂模式**：封装Token创建逻辑
- **资源获取即初始化**：谁创建谁销毁
- **原型模式**：支持Token复制

### 查询接口

```c
// 类型信息查询
const char* F_token_type_to_string(enum Eum_TokenType type);
const char* F_token_get_category(enum Eum_TokenType type);

// 类型判断
bool F_token_is_keyword(enum Eum_TokenType type);
bool F_token_is_operator(enum Eum_TokenType type);
bool F_token_is_literal(enum Eum_TokenType type);
bool F_token_is_separator(enum Eum_TokenType type);
```

**查询优化**：
- 使用范围检查而非开关语句
- 常量时间操作
- 缓存友好

### 操作接口

```c
// 输出接口
void F_token_print(const Stru_Token_t* token, bool show_position, bool show_literal_value);
size_t F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);

// 比较接口
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);

// 运算符属性
int F_token_get_precedence(enum Eum_TokenType type);
int F_token_get_associativity(enum Eum_TokenType type);
```

**接口设计原则**：
- **参数默认值**：通过布尔参数控制输出细节
- **缓冲区安全**：`F_token_to_string` 提供缓冲区大小检查
- **NULL安全**：所有函数对NULL参数安全

## 内存管理设计

### 分配策略

1. **Token结构体**：使用`malloc`动态分配
2. **lexeme字符串**：根据实际长度动态分配
3. **字面量值**：内联存储，不额外分配

### 释放策略

1. **谁创建谁销毁**：`F_token_destroy` 释放所有关联内存
2. **深拷贝管理**：`F_token_copy` 创建需要单独销毁的副本
3. **NULL安全**：对NULL指针的销毁操作无害

### 内存优化

1. **按需分配**：lexeme字符串按实际长度分配
2. **结构体对齐**：合理布局减少内存浪费
3. **联合体使用**：字面量值共享存储空间

## 错误处理设计

### 错误类型

1. **参数错误**：无效的输入参数
2. **内存错误**：分配失败
3. **逻辑错误**：不支持的Token类型

### 处理策略

1. **返回NULL**：创建函数失败时返回NULL
2. **返回安全值**：查询函数对无效参数返回默认值
3. **静默处理**：销毁函数对NULL参数安全
4. **防御性编程**：所有函数进行参数验证

### 错误传播

```
创建失败 → 返回NULL → 调用者检查 → 适当处理
查询失败 → 返回安全值 → 调用者继续执行
操作失败 → 静默处理 → 避免程序崩溃
```

## 性能考虑

### 时间复杂度

1. **创建操作**：O(n) - 需要复制lexeme字符串
2. **查询操作**：O(1) - 常量时间操作
3. **比较操作**：O(n) - 需要比较字符串
4. **打印操作**：O(n) - 需要格式化输出

### 空间复杂度

1. **Token结构体**：固定大小（约36字节）
2. **lexeme字符串**：变长，按需分配
3. **临时缓冲区**：输出函数使用调用者提供的缓冲区

### 优化措施

1. **字符串长度缓存**：避免重复计算字符串长度
2. **范围检查优化**：使用数值范围而非开关语句
3. **内存池考虑**：未来可添加Token内存池

## 扩展性设计

### Token类型扩展

1. **添加新枚举值**：在`Eum_TokenType`中添加新类型
2. **更新类型字符串**：在`F_token_type_to_string`中添加映射
3. **更新分类逻辑**：在`F_token_get_category`中添加分类规则

### 功能扩展

1. **新创建函数**：支持新的Token创建方式
2. **新查询函数**：添加新的Token属性查询
3. **新操作函数**：支持新的Token操作

### 接口扩展

1. **回调接口**：支持自定义Token处理
2. **迭代器接口**：支持Token序列遍历
3. **序列化接口**：支持Token持久化

## 测试策略

### 单元测试

1. **创建测试**：验证Token创建功能
2. **销毁测试**：验证内存释放
3. **查询测试**：验证类型信息和属性查询
4. **操作测试**：验证比较、打印等操作
5. **边界测试**：测试NULL参数和边界条件

### 集成测试

1. **模块集成**：测试Token模块与其他模块的协作
2. **内存测试**：验证内存管理和泄漏检测
3. **性能测试**：验证性能指标和瓶颈

### 测试工具

1. **Unity测试框架**：用于单元测试
2. **Valgrind**：用于内存泄漏检测
3. **性能分析器**：用于性能测试

## 安全考虑

### 内存安全

1. **缓冲区溢出防护**：所有字符串操作检查缓冲区大小
2. **空指针解引用防护**：所有函数对NULL参数安全
3. **内存泄漏防护**：提供对应的释放函数

### 类型安全

1. **枚举类型**：使用枚举确保类型安全
2. **参数验证**：所有函数验证输入参数
3. **错误处理**：提供一致的错误处理机制

### 线程安全

1. **当前限制**：当前版本不是线程安全的
2. **未来考虑**：可通过互斥锁或线程局部存储实现线程安全

## 兼容性考虑

### 向后兼容性

1. **API稳定性**：公共API一旦发布，保持向后兼容
2. **ABI稳定性**：数据结构布局保持稳定
3. **废弃策略**：废弃的API标记为已废弃，保留至少两个版本

### 平台兼容性

1. **标准C**：使用C89/C99标准，确保跨平台兼容性
2. **字节序**：数据结构不考虑字节序，由调用者处理
3. **编码**：支持UTF-8编码，确保中文兼容性

## 部署考虑

### 编译选项

1. **调试版本**：包含调试信息和断言检查
2. **发布版本**：优化性能和大小
3. **测试版本**：包含测试代码和覆盖率检测

### 依赖管理

1. **标准库依赖**：仅依赖C标准库
2. **项目内部依赖**：不依赖其他项目模块
3. **外部依赖**：无外部依赖

### 打包和分发

1. **静态库**：可编译为静态库供其他项目使用
2. **动态库**：未来可支持动态库
3. **头文件**：提供完整的头文件供包含

## 维护考虑

### 代码质量

1. **编码规范**：遵循项目编码规范
2. **注释规范**：提供完整的API文档注释
3. **测试覆盖率**：目标测试覆盖率≥85%

### 文档维护

1. **API文档**：保持API文档与代码同步
2. **设计文档**：记录重要的设计决策
3. **示例代码**：提供完整的使用示例

### 版本管理

1. **语义化版本**：遵循语义化版本规范
2. **变更日志**：维护详细的变更日志
3. **分支策略**：使用Git流分支策略

## 总结

Token模块是CN_Language词法分析的基础，采用模块化、可扩展的设计，提供完整、安全、高效的Token管理功能。通过遵循SOLID设计原则和现代软件工程实践，确保模块的可靠性、可维护性和可扩展性。
