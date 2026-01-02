# CN_debug API 文档

## 概述

CN_debug是CN_Language项目的调试系统模块，提供分级调试输出、断言检查和日志记录功能。该模块通过条件编译实现零开销的生产环境部署。

## 头文件引用

```c
#include "CN_debug.h"
```

## 枚举类型

### Eum_DebugLevel_t
调试级别枚举，用于控制调试信息的输出级别。

```c
typedef enum Eum_DebugLevel_t
{
    Eum_DEBUG_LEVEL_NONE = 0,    // 不输出任何调试信息
    Eum_DEBUG_LEVEL_ERROR = 1,   // 错误信息
    Eum_DEBUG_LEVEL_WARN = 2,    // 警告信息
    Eum_DEBUG_LEVEL_INFO = 3,    // 一般信息
    Eum_DEBUG_LEVEL_DEBUG = 4    // 调试信息
} Eum_DebugLevel_t;
```

## 宏定义

### CN_DEBUG
条件调试输出宏，在定义CN_DEBUG_MODE时生效。

**语法：**
```c
CN_DEBUG(level, format, ...)
```

**参数：**
- `level` - 调试级别（Eum_DebugLevel_t枚举值）
- `format` - 格式化字符串
- `...` - 可变参数列表

**示例：**
```c
CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "变量值: %d", value);
CN_DEBUG(Eum_DEBUG_LEVEL_ERROR, "文件打开失败: %s", filename);
```

### CN_ASSERT
条件断言宏，在定义CN_DEBUG_MODE时生效。

**语法：**
```c
CN_ASSERT(condition, message)
```

**参数：**
- `condition` - 断言条件表达式
- `message` - 断言失败时的错误消息

**示例：**
```c
CN_ASSERT(ptr != NULL, "指针不能为空");
CN_ASSERT(count > 0, "计数必须大于0");
```

## 函数接口

### F_debug_init
初始化调试系统，设置全局调试级别。

**函数签名：**
```c
void F_debug_init(Eum_DebugLevel_t level);
```

**参数：**
- `level` - 调试级别

**说明：**
- 设置全局调试级别，只有等于或高于此级别的调试信息才会输出
- 会关闭之前打开的调试文件并释放相关资源

**示例：**
```c
F_debug_init(Eum_DEBUG_LEVEL_DEBUG);  // 输出所有级别的调试信息
F_debug_init(Eum_DEBUG_LEVEL_ERROR);  // 只输出错误信息
F_debug_init(Eum_DEBUG_LEVEL_NONE);   // 不输出任何调试信息
```

### F_debug_set_output_file
设置调试信息输出文件。

**函数签名：**
```c
void F_debug_set_output_file(const char* filename);
```

**参数：**
- `filename` - 输出文件名，NULL表示输出到标准错误

**说明：**
- 可以将调试信息重定向到指定文件
- 文件以追加模式打开
- 如果文件无法打开，回退到标准错误输出
- 传入NULL可恢复到标准错误输出

**示例：**
```c
F_debug_set_output_file("debug.log");  // 输出到文件
F_debug_set_output_file(NULL);         // 恢复到标准错误
```

### F_debug_log
记录调试信息，通常通过CN_DEBUG宏调用。

**函数签名：**
```c
void F_debug_log(Eum_DebugLevel_t level, const char* file, int line, 
                  const char* format, ...);
```

**参数：**
- `level` - 调试级别
- `file` - 源文件名（通常由__FILE__宏提供）
- `line` - 行号（通常由__LINE__宏提供）
- `format` - 格式化字符串
- `...` - 可变参数列表

**说明：**
- 直接调用此函数较少见，通常通过CN_DEBUG宏使用
- 自动添加时间戳、调试级别、文件名和行号信息

### F_debug_assert
调试断言，当条件为假时终止程序，通常通过CN_ASSERT宏调用。

**函数签名：**
```c
void F_debug_assert(int condition, const char* file, int line, 
                     const char* message);
```

**参数：**
- `condition` - 断言条件
- `file` - 源文件名（通常由__FILE__宏提供）
- `line` - 行号（通常由__LINE__宏提供）
- `message` - 错误消息

**说明：**
- 直接调用此函数较少见，通常通过CN_ASSERT宏使用
- 断言失败时输出详细错误信息并调用abort()终止程序

### F_debug_level_to_string
将调试级别转换为字符串表示。

**函数签名：**
```c
const char* F_debug_level_to_string(Eum_DebugLevel_t level);
```

**参数：**
- `level` - 调试级别

**返回值：**
- 对应的字符串表示

**示例：**
```c
printf("当前级别: %s\n", F_debug_level_to_string(Eum_DEBUG_LEVEL_INFO));
// 输出: 当前级别: INFO
```

## 编译选项

### CN_DEBUG_MODE
控制调试功能的编译开关。

**定义时：**
```bash
gcc -DCN_DEBUG_MODE ...
```

**效果：**
- CN_DEBUG和CN_ASSERT宏生效
- 调试代码被编译进程序

**未定义时：**
```bash
gcc ...  # 不定义CN_DEBUG_MODE
```

**效果：**
- CN_DEBUG和CN_ASSERT宏为空操作
- 调试代码被编译器优化掉，无性能开销

## 使用示例

### 完整示例程序

```c
/******************************************************************************
 * CN_debug使用示例
 ******************************************************************************/

#include "CN_debug.h"
#include <stdio.h>

int main()
{
    // 初始化调试系统
    F_debug_init(Eum_DEBUG_LEVEL_DEBUG);
    
    // 设置输出文件（可选）
    F_debug_set_output_file("application.log");
    
    // 基本调试输出
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "应用程序启动");
    CN_DEBUG(Eum_DEBUG_LEVEL_DEBUG, "初始化变量: count=%d", 42);
    
    // 错误处理
    FILE* file = fopen("nonexistent.txt", "r");
    if (file == NULL)
    {
        CN_DEBUG(Eum_DEBUG_LEVEL_ERROR, "无法打开文件: nonexistent.txt");
    }
    
    // 断言检查
    int* ptr = malloc(sizeof(int));
    CN_ASSERT(ptr != NULL, "内存分配失败");
    
    *ptr = 100;
    CN_ASSERT(*ptr > 0, "值必须大于0");
    
    free(ptr);
    
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "应用程序正常退出");
    
    return 0;
}
```

### 生产环境编译

```bash
# 生产环境 - 禁用调试功能
gcc -O2 -DNDEBUG -o myapp main.c src/debug/CN_debug.c

# 开发环境 - 启用调试功能
gcc -DCN_DEBUG_MODE -g -o myapp_debug main.c src/debug/CN_debug.c
```

## 性能考虑

1. **生产环境零开销** - 未定义CN_DEBUG_MODE时，所有调试代码被优化掉
2. **级别控制** - 可以动态调整调试级别，控制输出量
3. **文件I/O** - 文件输出会有一定性能开销，建议在高频率调试时使用标准错误输出
4. **线程安全** - 基础实现保证基本的线程安全性

## 最佳实践

1. **合理设置调试级别** - 生产环境使用ERROR或WARN级别
2. **使用有意义的消息** - 调试信息应包含足够的上下文信息
3. **避免敏感信息** - 不要在调试信息中输出密码等敏感数据
4. **及时清理资源** - 通过cn_debug_init重新初始化时会自动清理资源
5. **条件编译** - 在生产环境中禁用CN_DEBUG_MODE以获得最佳性能

## 错误处理

1. **文件打开失败** - 自动回退到标准错误输出
2. **内存分配失败** - 使用标准库的strdup，失败时返回NULL
3. **断言失败** - 输出详细错误信息后调用abort()终止程序

## 限制和约束

1. **线程安全** - 基础实现提供基本线程安全，复杂多线程场景需额外考虑
2. **文件大小** - 长时间运行可能产生大量日志，需定期清理
3. **编码支持** - 支持UTF-8编码的中文字符
4. **平台兼容** - 支持Windows和Linux平台
