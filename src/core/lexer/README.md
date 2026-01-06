# CN_Language 词法分析器模块

## 概述

词法分析器（Lexer）是CN_Language编译器的第一个阶段，负责将源代码字符流转换为令牌（Token）序列。本模块实现了对70个中文关键字的完整支持，遵循项目架构规范，采用接口模式实现模块解耦。

## 模块结构

### 文件说明

1. **CN_lexer_interface.h** - 词法分析器抽象接口定义
   - 定义`Stru_LexerInterface_t`接口结构体
   - 声明所有公共接口函数
   - 提供工厂函数声明

2. **CN_lexer_interface.c** - 接口适配器
   - 实现工厂函数`F_create_lexer_interface()`
   - 提供接口到具体实现的适配

3. **CN_lexer_impl.c** - 词法分析器核心实现
   - 实现所有词法分析功能
   - 包含中文关键字识别、运算符识别、字面量识别等
   - 遵循单一职责原则：每个函数不超过50行

### 依赖关系

- **基础设施层**：依赖`CN_dynamic_array.h`用于令牌数组管理
- **核心层**：依赖`CN_token.h`和`CN_token_types.h`用于令牌定义
- **应用层**：通过接口使用词法分析器功能

## 接口设计

### Stru_LexerInterface_t 结构体

词法分析器采用面向接口的编程模式，定义完整的抽象接口：

```c
typedef struct Stru_LexerInterface_t {
    // 初始化函数
    bool (*initialize)(struct Stru_LexerInterface_t* interface, 
                      const char* source, size_t length, 
                      const char* source_name);
    
    // 核心功能
    Stru_Token_t* (*next_token)(struct Stru_LexerInterface_t* interface);
    bool (*has_more_tokens)(struct Stru_LexerInterface_t* interface);
    Stru_DynamicArray_t* (*tokenize_all)(struct Stru_LexerInterface_t* interface);
    
    // 状态查询
    void (*get_position)(struct Stru_LexerInterface_t* interface, 
                        size_t* line, size_t* column);
    const char* (*get_source_name)(struct Stru_LexerInterface_t* interface);
    
    // 错误处理
    bool (*has_errors)(struct Stru_LexerInterface_t* interface);
    const char* (*get_last_error)(struct Stru_LexerInterface_t* interface);
    
    // 资源管理
    void (*reset)(struct Stru_LexerInterface_t* interface);
    void (*destroy)(struct Stru_LexerInterface_t* interface);
    
    // 内部状态
    void* internal_state;
} Stru_LexerInterface_t;
```

### 工厂函数

```c
Stru_LexerInterface_t* F_create_lexer_interface(void);
```

## 功能特性

### 1. 中文关键字支持
支持70个中文关键字，分为9个类别：
- 变量和类型声明：变量、常量、类型、类、对象等
- 控制结构：如果、否则、当、循环、中断、继续等
- 函数相关：函数、返回、主程序、无等
- 逻辑运算符：与、或、非等
- 字面量：真、假、空等
- 模块系统：模块、导入、导出等
- 运算符关键字：加、减、乘、除等
- 类型关键字：整数、浮点数、字符串、布尔等
- 其他关键字：尝试、捕获、抛出等

### 2. 令牌类型识别
- **关键字**：70个中文关键字
- **标识符**：变量名、函数名等
- **字面量**：整数、浮点数、字符串
- **运算符**：算术、比较、赋值等运算符
- **分隔符**：括号、逗号、分号等

### 3. 错误处理
- 详细的错误信息报告
- 错误位置追踪（行号、列号）
- 错误恢复机制

### 4. 性能优化
- 高效的字符扫描算法
- 最小化的内存分配
- 支持批量令牌化

## 使用示例

### 基本使用

```c
#include "src/core/lexer/CN_lexer_interface.h"

int main(void) {
    // 创建词法分析器
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    
    // 初始化
    const char* source = "变量 x = 42 + 3.14";
    lexer->initialize(lexer, source, strlen(source), "test.cn");
    
    // 逐个获取令牌
    while (lexer->has_more_tokens(lexer)) {
        Stru_Token_t* token = lexer->next_token(lexer);
        // 处理令牌...
        F_destroy_token(token);
    }
    
    // 批量令牌化
    lexer->reset(lexer);
    Stru_DynamicArray_t* tokens = lexer->tokenize_all(lexer);
    
    // 清理资源
    lexer->destroy(lexer);
    return 0;
}
```

### 测试程序

项目包含完整的测试程序：
- `test_lexer_simple.c` - 基本功能测试
- 编译命令：`make test`

## 架构合规性

### 单一职责原则
- 每个.c文件不超过500行
- 每个函数不超过50行
- 每个模块只负责一个功能领域

### 接口隔离原则
- 通过抽象接口提供功能
- 接口最小化，只暴露必要的方法
- 支持依赖注入

### 开闭原则
- 对扩展开放：可以通过实现新接口添加功能
- 对修改封闭：现有接口稳定，不轻易修改

### 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过工厂函数创建实例

## 开发指南

### 添加新关键字
1. 在`src/core/token/CN_token_types.h`中添加枚举值
2. 在`src/core/token/CN_token.c`的关键字表中添加条目
3. 在`src/core/lexer/CN_lexer_impl.c`的`F_identify_keyword`函数中添加识别逻辑

### 扩展功能
1. 在接口中定义新方法
2. 在实现文件中提供具体实现
3. 更新工厂函数设置函数指针
4. 编写相应的测试用例

### 调试技巧
- 使用`CN_LOG`宏输出调试信息
- 检查错误信息：`lexer->get_last_error(lexer)`
- 验证位置信息：`lexer->get_position(lexer, &line, &column)`

## 性能基准

### 内存使用
- 每个令牌：约64字节
- 内部状态：约256字节
- 动态数组：按需增长

### 处理速度
- 简单表达式：< 1毫秒
- 1000行代码：< 10毫秒
- 支持大文件处理

## 版本历史

### v1.0.0 (2026-01-06)
- 初始版本发布
- 支持70个中文关键字
- 完整的接口设计
- 通过所有单元测试

## 维护者

- CN_Language架构委员会
- 联系方式：通过项目Issue跟踪系统

## 许可证

MIT License - 详见项目根目录LICENSE文件
