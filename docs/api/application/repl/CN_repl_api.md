# CN_Language 交互式环境（REPL）模块 API 文档

## 概述

交互式环境（REPL）模块是CN_Language项目的用户交互界面，提供读取-求值-打印循环功能，支持逐行执行CN代码，并提供历史记录、自动补全等增强功能。本模块采用模块化设计，遵循SOLID原则和分层架构，属于应用层组件。

## 模块架构

REPL模块采用三层设计，实现关注点分离：

```
┌─────────────────────────────────────────┐
│     交互式环境接口（CN_repl_interface） │ ← 公共接口层
├─────────────────────────────────────────┤
│     交互式环境实现（CN_repl_impl）      │ ← 业务逻辑层
├─────────────────────────────────────────┤
│     历史记录管理器（HistoryManager）    │ ← 数据管理层
│     自动补全管理器（CompletionManager） │
└─────────────────────────────────────────┘
```

### 1. 交互式环境接口（REPL Interface）

**职责**：定义REPL模块的抽象接口，遵循依赖倒置原则。

#### 接口定义

```c
// 文件：CN_repl_interface.h

/**
 * @brief 交互式环境接口结构体
 * 
 * 定义了交互式环境模块的抽象接口，遵循依赖倒置原则。
 * 高层模块通过此接口与REPL模块交互，实现模块间的解耦。
 */
typedef struct Stru_ReplInterface_t Stru_ReplInterface_t;

/**
 * @brief 交互式环境接口方法定义
 */
struct Stru_ReplInterface_t {
    /**
     * @brief 初始化交互式环境
     * 
     * @param self 接口指针
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_ReplInterface_t* self);
    
    /**
     * @brief 启动交互式环境
     * 
     * 启动读取-求值-打印循环，等待用户输入并执行。
     * 
     * @param self 接口指针
     * @return int 退出码，0表示正常退出，非0表示错误
     */
    int (*start)(Stru_ReplInterface_t* self);
    
    /**
     * @brief 执行单行代码
     * 
     * 执行用户输入的单行CN代码，并返回结果。
     * 
     * @param self 接口指针
     * @param line 要执行的代码行
     * @return char* 执行结果字符串，需要调用者释放
     */
    char* (*execute_line)(Stru_ReplInterface_t* self, const char* line);
    
    /**
     * @brief 获取历史记录
     * 
     * 获取交互式环境的历史命令记录。
     * 
     * @param self 接口指针
     * @param index 历史记录索引（0表示最近的一条）
     * @return const char* 历史命令，NULL表示索引无效
     */
    const char* (*get_history)(Stru_ReplInterface_t* self, size_t index);
    
    /**
     * @brief 添加自动补全建议
     * 
     * 为当前输入添加自动补全建议。
     * 
     * @param self 接口指针
     * @param input 当前输入
     * @param suggestions 建议数组
     * @param count 建议数量
     */
    void (*add_completions)(Stru_ReplInterface_t* self, const char* input, 
                           const char** suggestions, size_t count);
    
    /**
     * @brief 显示欢迎信息
     * 
     * 显示交互式环境的欢迎信息和帮助提示。
     * 
     * @param self 接口指针
     */
    void (*show_welcome)(Stru_ReplInterface_t* self);
    
    /**
     * @brief 销毁交互式环境
     * 
     * 清理交互式环境占用的资源。
     * 
     * @param self 接口指针
     */
    void (*destroy)(Stru_ReplInterface_t* self);
};

/**
 * @brief 创建交互式环境实例
 * 
 * 工厂函数，创建并返回交互式环境接口的实例。
 * 
 * @return Stru_ReplInterface_t* 交互式环境接口指针，失败返回NULL
 */
Stru_ReplInterface_t* F_create_repl_interface(void);
```

#### 使用示例

```c
// 创建REPL实例
Stru_ReplInterface_t* repl = F_create_repl_interface();

// 初始化
if (repl->initialize(repl)) {
    // 启动REPL循环
    int exit_code = repl->start(repl);
    printf("REPL退出码: %d\n", exit_code);
} else {
    fprintf(stderr, "REPL初始化失败\n");
}

// 清理资源
repl->destroy(repl);
```

### 2. 历史记录管理器（History Manager）

**职责**：管理用户输入的历史记录，支持最多100条记录。

#### 数据结构

```c
// 文件：CN_repl_impl.c（内部使用）

/* 历史记录最大数量 */
#define REPL_MAX_HISTORY 100

/**
 * @brief 历史记录条目结构体
 * 
 * 存储单条历史记录，包含命令字符串和时间戳。
 */
typedef struct {
    char* command;          /**< 命令字符串 */
    size_t timestamp;       /**< 时间戳（可选） */
} Stru_HistoryEntry_t;

/**
 * @brief 历史记录管理器结构体
 * 
 * 管理交互式环境的历史记录功能。
 */
typedef struct {
    Stru_HistoryEntry_t entries[REPL_MAX_HISTORY];  /**< 历史记录数组 */
    size_t count;                                   /**< 当前记录数量 */
    size_t current_index;                           /**< 当前浏览索引 */
    size_t max_size;                                /**< 最大记录数量 */
} Stru_HistoryManager_t;
```

#### 主要功能

1. **添加历史记录**：`F_history_add()` - 添加新命令到历史记录
2. **获取历史记录**：`F_history_get()` - 按索引获取历史命令
3. **清空历史记录**：`F_history_clear()` - 清空所有历史记录
4. **历史记录导航**：支持上下箭头键浏览历史（待实现）

### 3. 自动补全管理器（Completion Manager）

**职责**：管理自动补全建议，支持最多20条建议。

#### 数据结构

```c
// 文件：CN_repl_impl.c（内部使用）

/* 自动补全最大建议数量 */
#define REPL_MAX_COMPLETIONS 20

/**
 * @brief 自动补全管理器结构体
 * 
 * 管理交互式环境的自动补全功能。
 */
typedef struct {
    const char* suggestions[REPL_MAX_COMPLETIONS];  /**< 补全建议数组 */
    size_t count;                                   /**< 当前建议数量 */
    size_t max_size;                                /**< 最大建议数量 */
} Stru_CompletionManager_t;
```

#### 主要功能

1. **添加补全建议**：`F_completion_add()` - 添加新的补全建议
2. **清空补全建议**：`F_completion_clear()` - 清空所有补全建议
3. **显示补全建议**：`F_completion_show()` - 显示与当前输入匹配的建议
4. **智能匹配**：基于前缀匹配显示相关建议

## 功能特性

### 1. 读取-求值-打印循环（REPL）

REPL模块的核心功能是提供交互式的代码执行环境：

```c
// REPL主循环伪代码
while (is_running) {
    // 1. 读取：显示提示符，读取用户输入
    char* input = read_input();
    
    // 2. 求值：执行用户输入的代码
    char* result = evaluate(input);
    
    // 3. 打印：显示执行结果
    print_result(result);
    
    // 4. 循环：继续等待下一次输入
}
```

### 2. 历史记录功能

- **容量**：最多存储100条历史记录
- **持久化**：支持历史记录的保存和加载（待实现）
- **导航**：支持使用上下箭头键浏览历史记录
- **搜索**：支持历史记录搜索（待实现）

### 3. 自动补全功能

- **建议数量**：最多显示20条补全建议
- **匹配算法**：基于前缀匹配的智能补全
- **上下文感知**：根据当前输入上下文提供相关建议
- **自定义建议**：支持运行时添加自定义补全建议

### 4. 内置命令

REPL支持以下内置命令：

| 命令 | 描述 | 示例 |
|------|------|------|
| `help` | 显示帮助信息 | `help` |
| `exit` | 退出REPL环境 | `exit` |
| `quit` | 退出REPL环境 | `quit` |
| `clear` | 清屏 | `clear` |
| `history` | 显示历史记录 | `history` |
| `version` | 显示版本信息 | `version` |

### 5. 错误处理

- **输入验证**：验证用户输入的语法和语义
- **错误恢复**：提供清晰的错误信息和恢复建议
- **资源管理**：确保内存和资源的正确释放

## 使用示例

### 基本使用

```c
#include "CN_repl_interface.h"

int main(void) {
    // 创建REPL实例
    Stru_ReplInterface_t* repl = F_create_repl_interface();
    if (repl == NULL) {
        fprintf(stderr, "创建REPL实例失败\n");
        return 1;
    }
    
    // 初始化REPL
    if (!repl->initialize(repl)) {
        fprintf(stderr, "REPL初始化失败\n");
        repl->destroy(repl);
        return 1;
    }
    
    // 启动REPL循环
    int exit_code = repl->start(repl);
    
    // 清理资源
    repl->destroy(repl);
    
    return exit_code;
}
```

### 集成到主程序

```c
// 在主程序中集成REPL
int main(int argc, char** argv) {
    // 检查命令行参数
    if (argc == 1) {
        // 无参数，启动REPL模式
        Stru_ReplInterface_t* repl = F_create_repl_interface();
        if (repl != NULL && repl->initialize(repl)) {
            return repl->start(repl);
        }
        return 1;
    } else {
        // 有参数，使用CLI模式
        // ... CLI处理逻辑 ...
    }
}
```

### 自定义自动补全

```c
// 添加自定义补全建议
const char* custom_suggestions[] = {
    "变量", "函数", "如果", "循环", "返回",
    "打印", "输入", "类型", "结构", "枚举"
};

Stru_ReplInterface_t* repl = F_create_repl_interface();
repl->initialize(repl);

// 添加自定义补全建议
repl->add_completions(repl, "变", custom_suggestions, 
                     sizeof(custom_suggestions) / sizeof(custom_suggestions[0]));
```

## 集成核心层

### 与词法分析器集成

REPL模块需要与核心层的词法分析器集成以执行CN代码：

```c
// 在execute_line方法中集成词法分析器
static char* F_repl_execute_line(Stru_ReplInterface_t* self, const char* line)
{
    // 1. 调用词法分析器
    // Stru_LexerInterface_t* lexer = F_create_lexer();
    // lexer->initialize(lexer, line);
    
    // 2. 调用语法分析器
    // Stru_ParserInterface_t* parser = F_create_parser();
    // Stru_AstNode_t* ast = parser->parse(parser, tokens);
    
    // 3. 调用语义分析器
    // Stru_SemanticAnalyzer_t* semantic = F_create_semantic_analyzer();
    // semantic->analyze(semantic, ast);
    
    // 4. 调用代码生成器
    // Stru_CodeGenerator_t* codegen = F_create_code_generator();
    // Stru_Executable_t* executable = codegen->generate(codegen, ast);
    
    // 5. 调用运行时系统
    // Stru_Runtime_t* runtime = F_create_runtime();
    // Stru_Value_t* result = runtime->execute(runtime, executable);
    
    // 目前返回示例结果
    char* result = (char*)malloc(256);
    if (result != NULL) {
        snprintf(result, 256, "执行: %s (核心层集成待实现)", line);
    }
    return result;
}
```

### 与运行时系统集成

REPL模块最终需要与运行时系统集成以执行生成的代码：

```c
// 集成运行时系统的伪代码
static char* F_execute_with_runtime(const char* code)
{
    // 1. 编译代码
    // Stru_Compiler_t* compiler = F_create_compiler();
    // Stru_Executable_t* executable = compiler->compile(compiler, code);
    
    // 2. 执行代码
    // Stru_Runtime_t* runtime = F_create_runtime();
    // runtime->load(runtime, executable);
    // Stru_Value_t* result = runtime->run(runtime);
    
    // 3. 格式化结果
    // char* formatted_result = F_format_value(result);
    
    // 返回结果
    return strdup("执行结果待实现");
}
```

## 扩展性

### 添加新内置命令

要添加新内置命令，需要：

1. 在`F_repl_execute_line()`函数中添加命令识别
2. 实现命令逻辑
3. 更新帮助信息

```c
// 示例：添加"debug"命令
if (strcmp(line, "debug") == 0) {
    return strdup("调试模式已启动（功能待实现）");
}
```

### 自定义提示符

支持自定义REPL提示符：

```c
// 在上下文中修改提示符
static void F_set_prompt(Stru_ReplContext_t* context, const char* prompt)
{
    if (context->prompt != NULL) {
        free(context->prompt);
    }
    context->prompt = strdup(prompt);
}
```

### 插件系统

支持通过插件扩展REPL功能（待实现）：

```c
// 插件接口定义
typedef struct {
    const char* name;
    bool (*initialize)(void* context);
    char* (*handle_command)(const char* command);
    void (*cleanup)(void);
} Stru_ReplPlugin_t;

// 插件注册
void F_register_plugin(Stru_ReplInterface_t* self, Stru_ReplPlugin_t* plugin);
```

## 性能考虑

### 内存管理

1. **历史记录内存**：使用固定大小数组，避免动态内存碎片
2. **输入缓冲区**：使用动态增长缓冲区，适应不同长度的输入
3. **结果缓存**：缓存最近的计算结果，提高重复执行性能

### 响应时间

1. **输入响应**：使用非阻塞I/O，确保及时响应用户输入
2. **补全建议**：使用前缀树（Trie）数据结构，提高补全匹配速度
3. **历史搜索**：使用高效搜索算法，快速定位历史记录

### 资源清理

1. **自动清理**：在退出时自动清理所有分配的资源
2. **错误恢复**：在发生错误时确保资源正确释放
3. **内存泄漏检测**：集成内存调试工具，检测潜在的内存泄漏

## 兼容性

### 支持的平台

- **Linux**：完全支持，包括ANSI转义序列
- **Windows**：支持通过MinGW或Cygwin，需要终端模拟器支持
- **macOS**：完全支持，包括ANSI转义序列

### 终端要求

- **ANSI转义序列**：支持颜色、光标控制和清屏
- **UTF-8编码**：支持中文和其他Unicode字符
- **行编辑功能**：支持退格、删除和光标移动

### 编译器要求

- **C11标准**：需要C11兼容的编译器
- **标准库**：需要完整的C标准库支持
- **平台API**：需要基本的平台API支持（文件I/O、内存分配等）

## 测试

### 单元测试

每个模块都有对应的单元测试：

1. **接口测试**：测试接口方法的正确性
2. **历史记录测试**：测试历史记录的添加、获取和清理
3. **自动补全测试**：测试自动补全的匹配和显示
4. **命令测试**：测试内置命令的执行

### 集成测试

测试REPL与核心层的集成：

```bash
# 启动REPL测试
make test-repl

# 测试历史记录功能
make test-repl-history

# 测试自动补全功能
make test-repl-completion
```

### 端到端测试

完整的REPL工作流程测试：

```bash
# 启动REPL并执行测试脚本
echo "help\nexit" | ./bin/CN_Language repl

# 测试错误恢复
echo "无效命令\nexit" | ./bin/CN_Language repl
```

## 参考

- [CN_Language架构设计文档](../../architecture/001-中文编程语言CN_Language开发规划.md)
- [CN_Language技术规范文档](../../specifications/CN_Language项目%20技术规范和编码标准.md)
- [CN_Language构建方法文档](../../specifications/CN_Language项目%20构建方法和编译流程.md)
- [CLI模块API文档](../cli/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-09 | 初始版本，基本REPL功能、历史记录、自动补全 |
| 1.1.0 | 待定 | 计划添加插件系统、持久化历史记录、语法高亮 |

## 维护者

- CN_Language架构委员会
- 问题反馈：项目仓库 Issues 页面

##
