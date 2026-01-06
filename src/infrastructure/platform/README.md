# 平台抽象层 (Platform Abstraction Layer)

## 概述

平台抽象层是CN_Language项目的基础设施层核心模块之一，提供跨平台的操作系统接口抽象。通过统一的接口定义，实现上层代码的平台无关性，支持Windows、Linux、macOS等主流操作系统。

## 设计原则

### 1. 单一职责原则
- 每个接口只负责一个功能领域（文件系统、进程管理、时间管理）
- 每个函数功能明确，不超过50行代码
- 每个源文件不超过500行代码

### 2. 开闭原则
- 通过抽象接口支持扩展
- 新增平台支持无需修改现有接口
- 支持运行时平台检测和接口切换

### 3. 接口隔离原则
- 为不同客户端提供专用接口
- 文件系统、进程、时间管理接口相互独立
- 避免"胖接口"，接口功能最小化

### 4. 依赖倒置原则
- 高层模块定义接口（`CN_platform_interface.h`）
- 低层模块实现接口（各平台具体实现）
- 通过工厂函数获取平台接口实例

## 模块结构

```
src/infrastructure/platform/
├── CN_platform_interface.h      # 平台抽象层接口定义
├── CN_platform_windows.c        # Windows平台实现
├── CN_platform_linux.c          # Linux平台实现
├── CN_platform_macos.c          # macOS平台实现
├── CN_platform_stub.c           # 测试桩实现（用于单元测试）
├── CN_platform_default.c        # 默认平台接口实现（自动检测）
└── README.md                    # 本文件
```

## 接口分类

### 1. 文件系统接口 (`Stru_FileSystemInterface_t`)
提供跨平台的文件和目录操作：
- 文件打开、关闭、读写、定位
- 文件信息获取、删除、重命名、复制
- 目录创建、删除、遍历
- 路径检查、工作目录管理

### 2. 进程管理接口 (`Stru_ProcessInterface_t`)
提供跨平台的进程管理功能：
- 进程创建、等待、终止
- 进程信息获取
- 环境变量管理
- 系统命令执行

### 3. 时间管理接口 (`Stru_TimeInterface_t`)
提供跨平台的时间管理功能：
- 时间戳获取（毫秒、纳秒精度）
- 时间格式转换（本地时间、UTC时间）
- 时间字符串格式化与解析
- 系统启动时间、CPU频率获取

### 4. 平台主接口 (`Stru_PlatformInterface_t`)
整合所有子接口，提供统一的平台访问入口：
- 平台初始化与清理
- 平台信息获取（名称、版本、架构）
- 系统资源信息（内存、CPU核心数）

## 使用示例

### 基本使用

```c
#include "CN_platform_interface.h"

int main()
{
    // 获取默认平台接口（自动检测当前平台）
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    
    if (!platform || !platform->initialize())
    {
        printf("平台初始化失败\n");
        return -1;
    }
    
    // 使用文件系统接口
    void* file = platform->file_system->open_file("test.txt", Eum_FILE_MODE_READ);
    if (file != CN_INVALID_FILE_HANDLE)
    {
        char buffer[1024];
        int64_t bytes_read = platform->file_system->read_file(file, buffer, sizeof(buffer));
        platform->file_system->close_file(file);
    }
    
    // 使用时间管理接口
    uint64_t timestamp = platform->time->get_current_timestamp();
    printf("当前时间戳: %llu\n", timestamp);
    
    // 清理资源
    platform->cleanup();
    
    return 0;
}
```

### 单元测试中使用测试桩

```c
#include "CN_platform_interface.h"
#include "unity.h"

void setUp(void)
{
    // 使用测试桩接口进行单元测试
    const Stru_PlatformInterface_t* platform = F_get_stub_platform_interface();
    platform->initialize();
}

void test_file_operations(void)
{
    const Stru_PlatformInterface_t* platform = F_get_stub_platform_interface();
    
    // 测试文件操作（不依赖实际文件系统）
    void* file = platform->file_system->open_file("test.txt", Eum_FILE_MODE_READ);
    TEST_ASSERT_NOT_NULL(file);
    
    char buffer[100];
    int64_t bytes_read = platform->file_system->read_file(file, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT64(sizeof(buffer), bytes_read);
    
    platform->file_system->close_file(file);
}

void tearDown(void)
{
    const Stru_PlatformInterface_t* platform = F_get_stub_platform_interface();
    platform->cleanup();
}
```

## 平台支持

### 已实现平台
1. **Windows平台** (`CN_platform_windows.c`)
   - 基于Windows API实现
   - 支持Windows 7及以上版本
   - 使用Unicode字符集（通过A版本API兼容）

2. **Linux平台** (`CN_platform_linux.c`)
   - 基于POSIX标准实现
   - 支持主流Linux发行版
   - 使用标准C库和系统调用

3. **macOS平台** (`CN_platform_macos.c`)
   - 基于Darwin/BSD API实现
   - 支持macOS 10.12及以上版本
   - 使用macOS特有API（如mach_absolute_time、proc_pidinfo等）

4. **测试桩平台** (`CN_platform_stub.c`)
   - 用于单元测试
   - 不依赖实际操作系统
   - 提供模拟数据和默认返回值

5. **默认平台接口** (`CN_platform_default.c`)
   - 自动检测当前编译平台
   - 提供统一的平台访问入口
   - 包含便捷的包装函数

## 编译和链接

### 独立编译
平台抽象层模块可以独立编译和测试：

```bash
# 编译平台抽象层
gcc -c src/infrastructure/platform/CN_platform_stub.c -I src/infrastructure/platform -o build/platform_stub.o

# 编译测试程序
gcc -c tests/unit/platform_test.c -I src/infrastructure/platform -o build/platform_test.o

# 链接并运行测试
gcc build/platform_stub.o build/platform_test.o -o platform_test
./platform_test
```

### 项目集成
在CN_Language项目中，平台抽象层通过构建系统自动集成：

```makefile
# Makefile片段
PLATFORM_SOURCES = \
    src/infrastructure/platform/CN_platform_interface.h \
    src/infrastructure/platform/CN_platform_$(PLATFORM).c \
    src/infrastructure/platform/CN_platform_stub.c

PLATFORM_OBJECTS = $(PLATFORM_SOURCES:.c=.o)

# 根据目标平台选择实现
ifeq ($(PLATFORM),windows)
    PLATFORM_IMPL = CN_platform_windows.c
else ifeq ($(PLATFORM),linux)
    PLATFORM_IMPL = CN_platform_linux.c
else ifeq ($(PLATFORM),macos)
    PLATFORM_IMPL = CN_platform_macos.c
else
    PLATFORM_IMPL = CN_platform_stub.c
endif
```

## 性能考虑

### 接口调用开销
- 接口通过函数指针调用，有轻微性能开销
- 关键路径考虑内联或直接函数调用
- 批量操作减少接口调用次数

### 内存管理
- 文件句柄、目录句柄等资源及时释放
- 避免内存泄漏，所有分配的资源都有对应的释放接口
- 使用统一的内存管理接口（cn_malloc/cn_free）

### 线程安全
- 接口设计考虑线程安全性
- 平台实现负责处理线程同步
- 文档明确说明各接口的线程安全级别

## 错误处理

### 错误码定义
- 布尔返回值：true表示成功，false表示失败
- 整数返回值：-1表示错误，其他值表示成功
- 特定错误码通过错误对象传递

### 错误信息
- 重要错误记录日志（使用CN_LOG宏）
- 错误信息包含上下文（文件名、行号、函数名）
- 支持错误链，便于调试

## 测试策略

### 单元测试
- 使用测试桩实现进行接口测试
- 测试覆盖率目标：≥85%
- 每个接口函数都有对应的测试用例

### 集成测试
- 与具体平台实现集成测试
- 测试跨平台一致性
- 性能基准测试

### 端到端测试
- 在实际目标平台上测试
- 验证功能完整性和稳定性
- 性能回归测试

## 扩展指南

### 添加新平台支持
1. 创建新的平台实现文件（如`CN_platform_newos.c`）
2. 实现所有接口函数
3. 添加平台检测逻辑
4. 更新工厂函数支持新平台
5. 编写平台特定测试

### 添加新接口功能
1. 在接口定义文件中添加新函数声明
2. 更新所有平台实现
3. 更新测试桩实现
4. 编写新的测试用例
5. 更新API文档

## 相关文档

- [API文档](../docs/api/platform/README.md)
- [设计文档](../docs/design/platform_abstraction.md)
- [测试文档](../tests/unit/platform/README.md)

## 版本历史

| 版本 | 日期 | 作者 | 修改说明 |
|------|------|------|----------|
| 1.0.0 | 2026-01-06 | CN_Language架构委员会 | 初始版本，包含Windows实现和测试桩 |
| 1.1.0 | 2026-01-06 | CN_Language架构委员会 | 新增Linux和macOS平台实现，添加默认平台接口自动检测 |

## 维护者

- CN_Language架构委员会
- 平台抽象层开发小组

## 许可证

MIT License
```

## 注意事项

1. **平台检测**：工厂函数`F_get_default_platform_interface()`会自动检测当前平台并返回相应的接口实例。
2. **资源管理**：所有通过接口分配的资源（文件句柄、目录句柄、进程句柄）都必须通过对应的接口释放。
3. **线程安全**：接口设计为线程安全，但具体实现可能有限制，请参考各平台实现的文档。
4. **错误处理**：所有函数都提供明确的错误返回值，调用者必须检查返回值并处理错误。
5. **性能优化**：对于性能敏感的场景，可以考虑直接调用平台特定API，但会牺牲可移植性。

---

*最后更新：2026年1月6日*
