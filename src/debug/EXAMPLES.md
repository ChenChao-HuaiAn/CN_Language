# CN_debug 使用示例

## 示例1: 基本调试功能

```c
#include "CN_debug.h"
#include <stdio.h>

int main()
{
    // 初始化调试系统，设置为DEBUG级别
    cn_debug_init(CN_DEBUG_LEVEL_DEBUG);
    
    printf("=== 基本调试功能示例 ===\n");
    
    // 输出不同级别的调试信息
    CN_DEBUG(CN_DEBUG_LEVEL_ERROR, "错误: 无法连接到数据库");
    CN_DEBUG(CN_DEBUG_LEVEL_WARN, "警告: 内存使用率达到85%%");
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "信息: 用户登录成功，用户名: %s", "张三");
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "调试: 计算结果: %d + %d = %d", 10, 20, 30);
    
    return 0;
}
```

**编译和运行：**
```bash
gcc -DCN_DEBUG_MODE -I. -o basic_debug basic_debug.c CN_debug.c
./basic_debug
```

**输出：**
```
=== 基本调试功能示例 ===
[2026-01-02 10:30:15] [ERROR] [basic_debug.c:10] 错误: 无法连接到数据库
[2026-01-02 10:30:15] [WARN] [basic_debug.c:11] 警告: 内存使用率达到85%
[2026-01-02 10:30:15] [INFO] [basic_debug.c:12] 信息: 用户登录成功，用户名: 张三
[2026-01-02 10:30:15] [DEBUG] [basic_debug.c:13] 调试: 计算结果: 10 + 20 = 30
```

## 示例2: 文件重定向

```c
#include "CN_debug.h"
#include <stdio.h>

int main()
{
    // 初始化调试系统
    cn_debug_init(CN_DEBUG_LEVEL_INFO);
    
    printf("=== 文件重定向示例 ===\n");
    
    // 将调试信息重定向到文件
    cn_debug_set_output_file("application.log");
    
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "应用程序启动");
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "初始化配置完成");
    
    // 恢复到标准错误输出
    cn_debug_set_output_file(NULL);
    
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "恢复到标准错误输出");
    
    return 0;
}
```

**编译和运行：**
```bash
gcc -DCN_DEBUG_MODE -I. -o file_redirect file_redirect.c CN_debug.c
./file_redirect
```

**输出（stderr）：**
```
=== 文件重定向示例 ===
[2026-01-02 10:30:15] [INFO] [file_redirect.c:15] 恢复到标准错误输出
```

**application.log文件内容：**
```
[2026-01-02 10:30:15] [INFO] [file_redirect.c:11] 应用程序启动
[2026-01-02 10:30:15] [DEBUG] [file_redirect.c:12] 初始化配置完成
```

## 示例3: 断言检查

```c
#include "CN_debug.h"
#include <stdio.h>
#include <stdlib.h>

int divide(int a, int b)
{
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "执行除法运算: %d / %d", a, b);
    
    // 断言检查除数不为零
    CN_ASSERT(b != 0, "除数不能为零");
    
    return a / b;
}

int main()
{
    cn_debug_init(CN_DEBUG_LEVEL_DEBUG);
    
    printf("=== 断言检查示例 ===\n");
    
    // 正常情况
    int result = divide(100, 5);
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "100 / 5 = %d", result);
    
    // 这里会导致程序崩溃
    printf("尝试除以零...\n");
    result = divide(100, 0);
    
    return 0;
}
```

**编译和运行：**
```bash
gcc -DCN_DEBUG_MODE -I. -o assert_demo assert_demo.c CN_debug.c
./assert_demo
```

**输出：**
```
=== 断言检查示例 ===
[2026-01-02 10:30:15] [DEBUG] [assert_demo.c:5] 执行除法运算: 100 / 5
[2026-01-02 10:30:15] [INFO] [assert_demo.c:15] 100 / 5 = 20
尝试除以零...
[2026-01-02 10:30:15] [DEBUG] [assert_demo.c:5] 执行除法运算: 100 / 0
ASSERTION FAILED: 除数不能为零
File: assert_demo.c, Line: 8
```

## 示例4: 调试级别控制

```c
#include "CN_debug.h"
#include <stdio.h>

void demonstrate_levels()
{
    printf("不同调试级别的演示:\n");
    
    CN_DEBUG(CN_DEBUG_LEVEL_ERROR, "这是错误信息");
    CN_DEBUG(CN_DEBUG_LEVEL_WARN, "这是警告信息");
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "这是普通信息");
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "这是调试信息");
}

int main()
{
    printf("=== 调试级别控制示例 ===\n\n");
    
    // 测试不同级别的设置
    printf("1. 设置为NONE级别:\n");
    cn_debug_init(CN_DEBUG_LEVEL_NONE);
    demonstrate_levels();
    
    printf("\n2. 设置为ERROR级别:\n");
    cn_debug_init(CN_DEBUG_LEVEL_ERROR);
    demonstrate_levels();
    
    printf("\n3. 设置为WARN级别:\n");
    cn_debug_init(CN_DEBUG_LEVEL_WARN);
    demonstrate_levels();
    
    printf("\n4. 设置为INFO级别:\n");
    cn_debug_init(CN_DEBUG_LEVEL_INFO);
    demonstrate_levels();
    
    printf("\n5. 设置为DEBUG级别:\n");
    cn_debug_init(CN_DEBUG_LEVEL_DEBUG);
    demonstrate_levels();
    
    return 0;
}
```

**编译和运行：**
```bash
gcc -DCN_DEBUG_MODE -I. -o level_control level_control.c CN_debug.c
./level_control
```

## 示例5: 生产环境与开发环境

### 开发环境版本 (debug_version.c)
```c
#include "CN_debug.h"
#include <stdio.h>

int process_data(int* data, int size)
{
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "开始处理数据，大小: %d", size);
    
    CN_ASSERT(data != NULL, "数据指针不能为空");
    CN_ASSERT(size > 0, "数据大小必须大于0");
    
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "处理元素[%d]: %d", i, data[i]);
        sum += data[i];
    }
    
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "数据处理完成，总和: %d", sum);
    return sum;
}

int main()
{
    cn_debug_init(CN_DEBUG_LEVEL_DEBUG);
    cn_debug_set_output_file("debug.log");
    
    printf("=== 开发环境版本 ===\n");
    
    int numbers[] = {1, 2, 3, 4, 5};
    int result = process_data(numbers, 5);
    
    printf("结果: %d\n", result);
    
    return 0;
}
```

### 生产环境版本 (release_version.c)
```c
#include "CN_debug.h"
#include <stdio.h>

int process_data(int* data, int size)
{
    // 在生产环境中，调试代码会被优化掉
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "开始处理数据，大小: %d", size);
    
    // 断言在生产环境中也会被移除
    CN_ASSERT(data != NULL, "数据指针不能为空");
    CN_ASSERT(size > 0, "数据大小必须大于0");
    
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "处理元素[%d]: %d", i, data[i]);
        sum += data[i];
    }
    
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "数据处理完成，总和: %d", sum);
    return sum;
}

int main()
{
    // 这些调用在生产环境中也不会产生任何开销
    cn_debug_init(CN_DEBUG_LEVEL_DEBUG);
    cn_debug_set_output_file("debug.log");
    
    printf("=== 生产环境版本 ===\n");
    
    int numbers[] = {1, 2, 3, 4, 5};
    int result = process_data(numbers, 5);
    
    printf("结果: %d\n", result);
    
    return 0;
}
```

**编译命令：**
```bash
# 开发环境 - 启用调试
gcc -DCN_DEBUG_MODE -g -I. -o debug_version debug_version.c CN_debug.c

# 生产环境 - 禁用调试
gcc -O2 -DNDEBUG -I. -o release_version release_version.c CN_debug.c
```

## 示例6: 在CN_Language项目中的集成使用

```c
/******************************************************************************
 * CN_Language项目中使用CN_debug的示例
 ******************************************************************************/

#include "CN_debug.h"
#include "CN_lexer.h"
#include "CN_parser.h"
#include <stdio.h>
#include <stdlib.h>

// 词法分析器调试
CN_Token* cn_lexical_analyze(const char* source_code)
{
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "开始词法分析: %s", source_code);
    
    CN_Token* tokens = malloc(sizeof(CN_Token) * MAX_TOKENS);
    CN_ASSERT(tokens != NULL, "词法分析器: 内存分配失败");
    
    // 词法分析逻辑...
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "词法分析完成，生成%d个token", token_count);
    
    return tokens;
}

// 语法分析器调试
CN_ASTNode* cn_parse_tokens(CN_Token* tokens)
{
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "开始语法分析");
    
    CN_ASSERT(tokens != NULL, "语法分析器: token列表不能为空");
    
    CN_ASTNode* ast = cn_create_ast_node(AST_PROGRAM);
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "创建AST根节点: %p", ast);
    
    // 语法分析逻辑...
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "语法分析完成，AST节点数: %d", node_count);
    
    return ast;
}

int main(int argc, char* argv[])
{
    // 根据命令行参数设置调试级别
    if (argc > 1 && strcmp(argv[1], "--debug") == 0)
    {
        cn_debug_init(CN_DEBUG_LEVEL_DEBUG);
        cn_debug_set_output_file("cn_language_debug.log");
        CN_DEBUG(CN_DEBUG_LEVEL_INFO, "CN_Language调试模式启动");
    }
    else
    {
        cn_debug_init(CN_DEBUG_LEVEL_ERROR);  // 生产环境只输出错误
    }
    
    const char* source_code = "整数 变量 = 42;";
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "开始编译源代码: %s", source_code);
    
    // 词法分析
    CN_Token* tokens = cn_lexical_analyze(source_code);
    
    // 语法分析
    CN_ASTNode* ast = cn_parse_tokens(tokens);
    
    CN_DEBUG(CN_DEBUG_LEVEL_INFO, "编译完成");
    
    // 清理资源...
    free(tokens);
    cn_destroy_ast(ast);
    
    return 0;
}
```

## 编译和使用建议

### 1. Makefile集成示例
```makefile
# 调试版本
debug: CFLAGS += -DCN_DEBUG_MODE -g
debug: program

# 发布版本
release: CFLAGS += -O2 -DNDEBUG
release: program

program: main.c src/debug/CN_debug.c
	$(CC) $(CFLAGS) -Isrc/debug -o $@ $^
```

### 2. CMake集成示例
```cmake
# 根据构建类型设置调试标志
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DCN_DEBUG_MODE)
endif()

# 添加调试模块
add_library(cn_debug src/debug/CN_debug.c)
target_include_directories(cn_debug PUBLIC src/debug)

# 链接到主程序
target_link_libraries(main cn_debug)
```

### 3. 条件编译最佳实践
```c
#ifdef CN_DEBUG_MODE
    // 调试版本特有的代码
    CN_DEBUG(CN_DEBUG_LEVEL_DEBUG, "详细调试信息");
#else
    // 生产版本的优化代码
#endif
```

这些示例展示了CN_debug系统在不同场景下的使用方法，从基本的调试输出到复杂的生产环境集成。
