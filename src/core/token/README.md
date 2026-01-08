# CN_Language 令牌模块

## 概述

令牌（Token）模块是CN_Language编译器的核心数据模型，定义了源代码中所有基本语法单元的结构和类型。本模块实现了对70个中文关键字的完整类型定义，为词法分析器、语法分析器等后续阶段提供统一的数据接口。

## 模块化架构

CN_Language 1.2.0版本引入了模块化架构，将令牌模块拆分为多个子模块，每个子模块专注于单一职责。这种设计遵循SOLID原则，提高了代码的可维护性、可测试性和可扩展性。

### 模块化结构

```
src/core/token/
├── CN_token.h                    # 主头文件（向后兼容）
├── CN_token.c                    # 适配器实现（向后兼容）
├── CN_token_interface.h          # 抽象接口定义
├── CN_token_types.h              # 类型定义（70个中文关键字）
├── README.md                     # 主模块文档
├── lifecycle/                    # 令牌生命周期管理
│   ├── CN_token_lifecycle.h
│   ├── CN_token_lifecycle.c
│   └── README.md
├── values/                       # 字面量值操作
│   ├── CN_token_values.h
│   ├── CN_token_values.c
│   └── README.md
├── query/                        # 类型查询和分类
│   ├── CN_token_query.h
│   ├── CN_token_query.c
│   └── README.md
├── keywords/                     # 关键字信息管理
│   ├── CN_token_keywords.h
│   ├── CN_token_keywords.c
│   └── README.md
├── tools/                        # 工具函数
│   ├── CN_token_tools.h
│   ├── CN_token_tools.c
│   └── README.md
└── interface/                    # 接口实现
    ├── CN_token_interface_impl.c
    └── README.md
```

### 子模块职责

1. **生命周期管理模块** (`lifecycle/`)
   - 令牌创建、销毁、克隆、比较
   - 内存管理和资源释放
   - 哈希值计算

2. **字面量值操作模块** (`values/`)
   - 设置和获取各种字面量值（整数、浮点数、字符串等）
   - 类型兼容性验证
   - 安全的字面量值访问

3. **类型查询和分类模块** (`query/`)
   - 令牌类型分类（关键字、字面量、运算符等）
   - 关键字详细分类（9个类别）
   - 类型转换和验证

4. **关键字信息管理模块** (`keywords/`)
   - 70个中文关键字的信息管理
   - 关键字查询（通过中文、英文、类型）
   - 关键字统计和枚举

5. **工具函数模块** (`tools/`)
   - 令牌格式化输出和字符串表示
   - 调试和日志输出支持
   - 序列化和反序列化辅助

6. **接口实现模块** (`interface/`)
   - 抽象接口实现
   - 模块集成和连接
   - 依赖注入支持

### 向后兼容性

模块化架构完全向后兼容：
- 现有代码无需修改
- `CN_token.h`和`CN_token.c`作为适配器层
- 所有原有API保持不变

### 新架构优势

1. **单一职责原则**：每个子模块不超过500行代码，每个函数不超过50行
2. **更好的可维护性**：模块间通过抽象接口通信，降低耦合度
3. **可测试性**：每个子模块有独立的测试文件
4. **可扩展性**：支持依赖注入，可替换模块实现
5. **性能优化**：可按需加载和使用模块

### 使用方式

```c
// 传统方式（向后兼容）
#include "src/core/token/CN_token.h"

// 新架构方式（模块化）
#include "src/core/token/CN_token_interface.h"
#include "src/core/token/lifecycle/CN_token_lifecycle.h"
#include "src/core/token/values/CN_token_values.h"
// ... 其他需要的子模块
```

### 依赖关系

- **基础设施层**：无直接依赖（自包含）
- **核心层**：被词法分析器、语法分析器等模块使用
- **应用层**：通过令牌接口访问令牌数据
- **内部依赖**：子模块间通过抽象接口通信

## 数据结构

### Eum_TokenType 枚举

定义了所有可能的令牌类型，包括70个中文关键字：

```c
typedef enum {
    // 文件结束标记
    Eum_TOKEN_EOF = 0,
    
    // 错误标记
    Eum_TOKEN_ERROR,
    
    // 标识符
    Eum_TOKEN_IDENTIFIER,
    
    // 字面量
    Eum_TOKEN_LITERAL_INTEGER,
    Eum_TOKEN_LITERAL_FLOAT,
    Eum_TOKEN_LITERAL_STRING,
    Eum_TOKEN_LITERAL_CHAR,
    Eum_TOKEN_LITERAL_BOOLEAN,
    
    // 运算符
    Eum_TOKEN_OPERATOR_PLUS,        // +
    Eum_TOKEN_OPERATOR_MINUS,       // -
    Eum_TOKEN_OPERATOR_MULTIPLY,    // *
    Eum_TOKEN_OPERATOR_DIVIDE,      // /
    Eum_TOKEN_OPERATOR_MODULO,      // %
    Eum_TOKEN_OPERATOR_ASSIGN,      // =
    Eum_TOKEN_OPERATOR_EQUAL,       // ==
    Eum_TOKEN_OPERATOR_NOT_EQUAL,   // !=
    Eum_TOKEN_OPERATOR_LESS,        // <
    Eum_TOKEN_OPERATOR_GREATER,     // >
    Eum_TOKEN_OPERATOR_LESS_EQUAL,  // <=
    Eum_TOKEN_OPERATOR_GREATER_EQUAL, // >=
    
    // 分隔符
    Eum_TOKEN_DELIMITER_COMMA,      // ,
    Eum_TOKEN_DELIMITER_SEMICOLON,  // ;
    Eum_TOKEN_DELIMITER_LPAREN,     // (
    Eum_TOKEN_DELIMITER_RPAREN,     // )
    Eum_TOKEN_DELIMITER_LBRACE,     // {
    Eum_TOKEN_DELIMITER_RBRACE,     // }
    Eum_TOKEN_DELIMITER_LBRACKET,   // [
    Eum_TOKEN_DELIMITER_RBRACKET,   // ]
    
    // 中文关键字（70个）
    Eum_TOKEN_KEYWORD_VAR,          // 变量
    Eum_TOKEN_KEYWORD_CONST,        // 常量
    Eum_TOKEN_KEYWORD_TYPE,         // 类型
    Eum_TOKEN_KEYWORD_CLASS,        // 类
    Eum_TOKEN_KEYWORD_OBJECT,       // 对象
    // ... 其他65个关键字
} Eum_TokenType;
```

### Stru_Token_t 结构体

令牌的核心数据结构：

```c
typedef struct Stru_Token_t {
    Eum_TokenType type;             ///< 令牌类型
    char* lexeme;                   ///< 词素（源代码中的原始字符串）
    size_t line;                    ///< 行号（从1开始）
    size_t column;                  ///< 列号（从1开始）
    
    // 字面量值（根据类型使用其中一个）
    union {
        long int_value;             ///< 整数值
        double float_value;         ///< 浮点数值
        char* string_value;         ///< 字符串值
        char char_value;            ///< 字符值
        bool bool_value;            ///< 布尔值
    } literal;
    
    // 扩展信息
    void* extra_data;               ///< 扩展数据指针
    size_t extra_data_size;         ///< 扩展数据大小
} Stru_Token_t;
```

## 功能特性

### 1. 完整的中文关键字支持
支持70个中文关键字，分为9个逻辑类别：
1. **变量和类型声明**：变量、常量、类型、类、对象等
2. **控制结构**：如果、否则、当、循环、中断、继续等
3. **函数相关**：函数、返回、主程序、无等
4. **逻辑运算符**：与、或、非等
5. **字面量**：真、假、空等
6. **模块系统**：模块、导入、导出等
7. **运算符关键字**：加、减、乘、除等
8. **类型关键字**：整数、浮点数、字符串、布尔等
9. **其他关键字**：尝试、捕获、抛出等

### 2. 类型分类系统
提供辅助函数对令牌类型进行分类：
- `F_is_keyword_token()` - 检查是否为关键字
- `F_is_literal_token()` - 检查是否为字面量
- `F_is_operator_token()` - 检查是否为运算符
- `F_is_delimiter_token()` - 检查是否为分隔符
- `F_is_identifier_token()` - 检查是否为标识符

### 3. 关键字信息查询
通过关键字表提供详细信息：
- 中文关键字字符串
- 对应的英文名称
- 功能描述
- 所属类别
- 优先级信息

### 4. 内存管理
- 自动内存分配和释放
- 支持深拷贝和浅拷贝
- 引用计数（可选）

## 使用示例

### 创建和销毁令牌

```c
#include "src/core/token/CN_token.h"

int main(void) {
    // 创建令牌
    Stru_Token_t* token = F_create_token(
        Eum_TOKEN_KEYWORD_VAR,      // 类型：变量关键字
        "变量",                     // 词素
        1,                          // 行号
        1                           // 列号
    );
    
    // 设置字面量值
    F_token_set_int_value(token, 42);
    
    // 使用令牌...
    
    // 销毁令牌
    F_destroy_token(token);
    
    return 0;
}
```

### 查询关键字信息

```c
#include "src/core/token/CN_token.h"

void print_keyword_info(const char* chinese_keyword) {
    const Stru_KeywordInfo_t* info = F_get_keyword_info(chinese_keyword);
    if (info != NULL) {
        printf("关键字: %s\n", info->chinese);
        printf("英文: %s\n", info->english);
        printf("描述: %s\n", info->description);
        printf("类别: %s\n", info->category);
        printf("优先级: %d\n", info->precedence);
    }
}
```

### 类型检查

```c
bool is_control_structure(Eum_TokenType type) {
    // 检查是否为控制结构关键字
    return type == Eum_TOKEN_KEYWORD_IF ||      // 如果
           type == Eum_TOKEN_KEYWORD_ELSE ||    // 否则
           type == Eum_TOKEN_KEYWORD_WHILE ||   // 当
           type == Eum_TOKEN_KEYWORD_FOR ||     // 循环
           type == Eum_TOKEN_KEYWORD_BREAK ||   // 中断
           type == Eum_TOKEN_KEYWORD_CONTINUE;  // 继续
}
```

## API参考

### 核心函数

| 函数 | 描述 |
|------|------|
| `F_create_token()` | 创建新令牌 |
| `F_destroy_token()` | 销毁令牌 |
| `F_clone_token()` | 克隆令牌 |
| `F_token_set_int_value()` | 设置整数值 |
| `F_token_set_float_value()` | 设置浮点数值 |
| `F_token_set_string_value()` | 设置字符串值 |
| `F_token_set_bool_value()` | 设置布尔值 |
| `F_token_set_char_value()` | 设置字符值 |

### 查询函数

| 函数 | 描述 |
|------|------|
| `F_get_token_type_name()` | 获取类型名称字符串 |
| `F_is_keyword_token()` | 检查是否为关键字 |
| `F_is_literal_token()` | 检查是否为字面量 |
| `F_is_operator_token()` | 检查是否为运算符 |
| `F_is_delimiter_token()` | 检查是否为分隔符 |
| `F_get_keyword_info()` | 获取关键字信息 |
| `F_get_keyword_by_english()` | 通过英文名获取关键字 |

### 工具函数

| 函数 | 描述 |
|------|------|
| `F_token_to_string()` | 将令牌转换为字符串 |
| `F_print_token()` | 打印令牌信息 |
| `F_compare_tokens()` | 比较两个令牌 |
| `F_token_hash()` | 计算令牌哈希值 |

## 架构合规性

### 单一职责原则
- 每个文件专注于一个功能领域
- 函数功能单一明确
- 清晰的模块边界

### 数据封装
- 内部数据结构对外部不可见
- 通过函数接口访问数据
- 隐藏实现细节

### 接口最小化
- 只暴露必要的API
- 稳定的公共接口
- 向后兼容

## 开发指南

### 添加新令牌类型
1. 在`CN_token_types.h`的枚举中添加新类型
2. 如果是关键字，在`CN_token.c`的关键字表中添加条目
3. 更新类型分类函数（如果需要）
4. 编写相应的测试用例

### 扩展令牌数据
1. 在`Stru_Token_t`结构体中添加新字段
2. 提供相应的设置和获取函数
3. 更新内存管理函数
4. 确保向后兼容性

### 性能优化
- 使用内存池减少分配开销
- 缓存常用令牌
- 优化字符串处理

## 测试策略

### 单元测试
- 测试所有公共API函数
- 验证内存管理正确性
- 检查边界条件

### 集成测试
- 与词法分析器集成测试
- 验证关键字识别正确性
- 测试类型分类功能

### 性能测试
- 内存使用分析
- 创建/销毁性能
- 查询性能基准

## 版本历史

### v1.0.0 (2026-01-06)
- 初始版本发布
- 支持70个中文关键字
- 完整的令牌类型系统
- 通过所有单元测试

## 维护者

- CN_Language架构委员会
- 联系方式：通过项目Issue跟踪系统

## 许可证

MIT License - 详见项目根目录LICENSE文件
