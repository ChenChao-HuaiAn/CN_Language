# 测试框架文档

## 概述

CN_Language 项目采用多层次的测试策略，包括单元测试、集成测试和系统测试，确保编译器和运行时库的正确性。

## 测试目录结构

```
tests/
├── unit/                    # 单元测试
│   ├── lexer_*.c           # 词法分析器测试
│   ├── parser_*.c          # 语法分析器测试
│   ├── semantics_*.c       # 语义分析测试
│   ├── runtime_*.c         # 运行时库测试
│   └── ...
├── integration/             # 集成测试
│   ├── integration_*.c     # 集成测试
│   ├── format/             # 格式化测试
│   ├── compiler/           # 编译器集成测试
│   ├── os/                 # 操作系统测试
│   └── perf/               # 性能测试
└── system/                  # 系统测试
    └── ...
```

## 测试分类

### 单元测试 (Unit Tests)

测试单个组件的功能：

| 测试类别 | 文件模式 | 描述 |
|----------|----------|------|
| 词法分析器 | `lexer_*.c` | 测试词法分析功能 |
| 语法分析器 | `parser_*.c` | 测试语法分析功能 |
| 语义分析 | `semantics_*.c` | 测试语义检查功能 |
| 运行时库 | `runtime_*.c` | 测试运行时功能 |
| 内存管理 | `memory_*.c` | 测试内存管理功能 |

### 集成测试 (Integration Tests)

测试多个组件的协同工作：

| 测试类别 | 目录 | 描述 |
|----------|------|------|
| 前端集成 | `integration_*.c` | 测试前端整体功能 |
| 编译器集成 | `compiler/` | 测试完整编译流程 |
| 格式化 | `format/` | 测试代码格式化 |
| 性能 | `perf/` | 测试性能基准 |
| 操作系统 | `os/` | 测试内核功能 |

### 系统测试 (System Tests)

测试整个系统的功能：

- 端到端编译测试
- 跨平台兼容性测试
- 实际应用场景测试

## 测试运行

### CTest 配置

```cmake
# CMakeLists.txt
enable_testing()

# 添加单元测试
add_test(NAME LexerTest COMMAND lexer_test)
add_test(NAME ParserTest COMMAND parser_test)
add_test(NAME SemanticsTest COMMAND semantics_test)

# 添加集成测试
add_test(NAME IntegrationTest COMMAND integration_test)
```

### 运行所有测试

```bash
# 配置并构建
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 运行测试
cd build
ctest -C Debug --output-on-failure
```

### 运行特定测试

```bash
# 运行特定测试
ctest -R LexerTest --output-on-failure

# 运行词法分析器相关测试
ctest -R "Lexer.*" --output-on-failure

# 并行运行测试
ctest -j4 --output-on-failure
```

## 测试示例

### 单元测试示例

```c
#include <stdio.h>
#include <string.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"

// 测试词法分析器基本功能
void test_lexer_basic() {
    const char *source = "函数 主程序 () { 返回 0; }";
    CnLexer lexer;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn", &diagnostics);
    
    CnToken token;
    int token_count = 0;
    
    while (cn_frontend_lexer_next_token(&lexer, &token)) {
        token_count++;
    }
    
    // 验证标记数量
    if (token_count != 9) {
        printf("FAIL: 期望 9 个标记，实际 %d\n", token_count);
        return;
    }
    
    printf("PASS: test_lexer_basic\n");
}

int main() {
    test_lexer_basic();
    return 0;
}
```

### 集成测试示例

```c
#include <stdio.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"

// 测试完整前端流程
void test_full_frontend() {
    const char *source = 
        "函数 加法 (整数 a, 整数 b) {\n"
        "    返回 a + b;\n"
        "}\n"
        "\n"
        "函数 主程序 () {\n"
        "    整数 结果 = 加法 (5, 3);\n"
        "    打印 (结果);\n"
        "    返回 0;\n"
        "}";
    
    CnLexer lexer;
    CnParser *parser;
    CnDiagnostics diagnostics;
    CnAstProgram *program;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn", &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    if (!cn_frontend_parse_program(parser, &program)) {
        printf("FAIL: 解析失败\n");
        cn_frontend_parser_free(parser);
        return;
    }
    
    // 验证 AST
    if (program->function_count != 2) {
        printf("FAIL: 期望 2 个函数，实际 %zu\n", program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return;
    }
    
    printf("PASS: test_full_frontend\n");
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
}

int main() {
    test_full_frontend();
    return 0;
}
```

## 测试工具

### PowerShell 测试脚本

```powershell
# tests/run_keyword_tests.ps1
# 运行关键字测试

$testFiles = Get-ChildItem -Path "tests/unit" -Filter "*_test.c"

foreach ($file in $testFiles) {
    Write-Host "Running $($file.Name)..."
    $result = .\build\tests\unit\$($file.BaseName).exe
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: $($file.Name)" -ForegroundColor Red
    } else {
        Write-Host "PASSED: $($file.Name)" -ForegroundColor Green
    }
}
```

### 测试覆盖率

```bash
# 使用 gcov 生成覆盖率报告
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="--coverage"
cmake --build build

# 运行测试
cd build
ctest -C Debug

# 生成报告
gcovr -r .. --html --html-details -o coverage.html
```

## 测试最佳实践

### 1. 测试命名

```c
// 好的命名
void test_lexer_identifies_chinese_keywords();
void test_parser_handles_nested_blocks();
void test_semantic_check_detects_type_mismatch();

// 避免的命名
void test1();
void test_thing();
```

### 2. 测试结构

```c
// 推荐的测试结构
void test_feature() {
    // 1. 设置 (Arrange)
    CnLexer lexer;
    setup_lexer(&lexer, "source code");
    
    // 2. 执行 (Act)
    CnToken token = get_next_token(&lexer);
    
    // 3. 验证 (Assert)
    assert(token.kind == CN_TOKEN_KEYWORD_FUNC);
    
    // 4. 清理 (Cleanup)
    cleanup_lexer(&lexer);
}
```

### 3. 错误报告

```c
// 详细的错误报告
void test_with_detailed_error() {
    int expected = 10;
    int actual = compute_value();
    
    if (actual != expected) {
        printf("FAIL: %s:%d\n", __FILE__, __LINE__);
        printf("  Expected: %d\n", expected);
        printf("  Actual:   %d\n", actual);
        return;
    }
    
    printf("PASS: %s\n", __func__);
}
```

## 相关文档

- [构建指南](../08-构建指南/README.md)
- [调试指南](../08-构建指南/08.2-调试指南.md)
- [CI/CD](../08-构建指南/08.4-CI-CD.md)
