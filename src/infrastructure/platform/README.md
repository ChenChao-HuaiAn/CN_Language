# CN_Language 平台抽象层模块

## 概述

平台抽象层模块（Platform Abstraction Layer）是CN_Language项目基础设施层的重要组成部分，提供跨平台的系统接口抽象。该模块允许上层代码以统一的方式访问操作系统功能，而无需关心底层平台的差异。

## 设计目标

1. **跨平台兼容性**：支持Windows、Linux、macOS等主流操作系统
2. **统一接口**：为不同平台提供一致的API接口
3. **模块化设计**：各功能模块独立，可按需使用
4. **高性能**：最小化抽象层带来的性能开销
5. **可扩展性**：支持新平台和功能的扩展

## 模块结构

### 核心接口文件
- `CN_platform.h` - 平台抽象层主接口定义

### 接口分类

#### 1. 文件系统接口 (`Stru_CN_FileSystemInterface_t`)
- 文件操作（创建、读取、写入、删除）
- 目录操作（创建、遍历、删除）
- 路径操作（组合、解析、规范化）
- 文件属性获取和设置
- 磁盘空间查询

#### 2. 线程和同步接口 (`Stru_CN_ThreadInterface_t`)
- 线程创建和管理
- 互斥锁、信号量、条件变量
- 线程本地存储
- 原子操作和内存屏障

#### 3. 网络接口 (`Stru_CN_NetworkInterface_t`)
- TCP/UDP套接字操作
- 网络地址解析
- 非阻塞IO和事件选择器
- 网络信息查询

#### 4. 时间和日期接口 (`Stru_CN_TimeInterface_t`)
- 时间获取和转换
- 高精度计时器
- 时区和日历操作
- 时间格式化和解析

#### 5. 系统信息接口 (`Stru_CN_SystemInterface_t`)
- 系统硬件信息
- 内存和CPU使用情况
- 环境变量管理
- 进程和用户信息
- 系统控制和通知

## 使用示例

### 基本使用

```c
#include "CN_platform.h"

int main()
{
    // 初始化平台抽象层
    if (!CN_platform_initialize()) {
        return -1;
    }
    
    // 获取默认平台接口
    Stru_CN_PlatformInterface_t* platform = CN_platform_get_default();
    if (!platform) {
        return -1;
    }
    
    // 使用文件系统接口
    if (platform->filesystem->file_exists("/path/to/file")) {
        CN_FileHandle_t file = platform->filesystem->file_open(
            "/path/to/file", Eum_FILE_MODE_READ);
        if (file) {
            // 读取文件内容
            char buffer[1024];
            size_t bytes_read = platform->filesystem->file_read(
                file, buffer, sizeof(buffer));
            platform->filesystem->file_close(file);
        }
    }
    
    // 清理平台抽象层
    CN_platform_cleanup();
    return 0;
}
```

### 自定义平台接口

```c
// 创建自定义平台接口（例如用于测试）
Stru_CN_PlatformInterface_t* create_mock_platform(void)
{
    Stru_CN_FileSystemInterface_t* mock_fs = create_mock_filesystem();
    Stru_CN_ThreadInterface_t* mock_thread = create_mock_thread();
    
    return CN_platform_create_custom(
        mock_fs,    // 自定义文件系统
        mock_thread, // 自定义线程接口
        NULL,       // 使用默认网络接口
        NULL,       // 使用默认时间接口
        NULL        // 使用默认系统接口
    );
}
```

## 平台实现

### 平台特定目录结构

```
src/infrastructure/platform/
├── windows/          # Windows平台实现
│   ├── CN_platform_windows.c
│   └── CN_platform_windows.h
├── linux/            # Linux平台实现
│   ├── CN_platform_linux.c
│   └── CN_platform_linux.h
├── macos/            # macOS平台实现
│   ├── CN_platform_macos.c
│   └── CN_platform_macos.h
└── common/           # 跨平台通用代码
    ├── CN_platform_common.c
    └── CN_platform_common.h
```

### 平台检测机制

平台抽象层在初始化时会自动检测当前运行平台，并加载相应的实现：

```c
const char* platform_type = CN_platform_get_type();
if (CN_platform_is_windows()) {
    // Windows特定逻辑
} else if (CN_platform_is_linux()) {
    // Linux特定逻辑
} else if (CN_platform_is_macos()) {
    // macOS特定逻辑
}
```

## 依赖关系

### 内部依赖
- 无（基础设施层不依赖项目其他层）

### 外部依赖
- C标准库（stdlib.h, stdio.h, string.h等）
- 平台特定API：
  - Windows: Win32 API, Winsock
  - Linux: POSIX API, pthreads
  - macOS: POSIX API, Cocoa (部分功能)

## 编译和链接

### 编译选项

```makefile
# 平台检测
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    CFLAGS += -DCN_PLATFORM_WINDOWS
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = linux
        CFLAGS += -DCN_PLATFORM_LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macos
        CFLAGS += -DCN_PLATFORM_MACOS
    endif
endif

# 平台特定源文件
PLATFORM_SOURCES = src/infrastructure/platform/$(PLATFORM)/CN_platform_$(PLATFORM).c
PLATFORM_OBJECTS = $(PLATFORM_SOURCES:.c=.o)
```

### 链接选项

```makefile
# Windows链接选项
ifeq ($(PLATFORM),windows)
    LDFLAGS += -lws2_32 -ladvapi32 -luser32
endif

# Linux链接选项
ifeq ($(PLATFORM),linux)
    LDFLAGS += -lpthread -lrt
endif

# macOS链接选项
ifeq ($(PLATFORM),macos)
    LDFLAGS += -framework Foundation -framework Cocoa
endif
```

## 测试策略

### 单元测试
- 每个接口函数都有对应的单元测试
- 使用模拟对象进行隔离测试
- 测试覆盖率目标：≥85%

### 集成测试
- 跨平台功能一致性测试
- 性能基准测试
- 内存泄漏检测

### 测试工具
- Unity测试框架
- Valgrind（内存检查）
- Cppcheck（静态分析）

## 性能考虑

### 优化策略
1. **延迟加载**：平台特定实现按需加载
2. **缓存机制**：频繁使用的系统信息缓存
3. **批量操作**：支持批量文件/网络操作
4. **零拷贝**：尽可能避免不必要的数据复制

### 性能基准
- 文件IO操作：与原生API性能差异 < 5%
- 线程创建：开销 < 10%
- 网络延迟：额外延迟 < 1ms

## 错误处理

### 错误码定义
```c
typedef enum Eum_CN_PlatformError_t
{
    Eum_PLATFORM_ERROR_NONE = 0,
    Eum_PLATFORM_ERROR_FILE_NOT_FOUND,
    Eum_PLATFORM_ERROR_PERMISSION_DENIED,
    Eum_PLATFORM_ERROR_OUT_OF_MEMORY,
    Eum_PLATFORM_ERROR_NETWORK_FAILURE,
    Eum_PLATFORM_ERROR_TIMEOUT,
    Eum_PLATFORM_ERROR_UNSUPPORTED_OPERATION,
    Eum_PLATFORM_ERROR_PLATFORM_SPECIFIC
} Eum_CN_PlatformError_t;
```

### 错误处理模式
```c
CN_FileHandle_t file = platform->filesystem->file_open(path, mode);
if (!file) {
    Eum_CN_PlatformError_t error = platform->filesystem->get_last_error();
    CN_LOG(Eum_LOG_LEVEL_ERROR, "文件打开失败: %s, 错误码: %d", 
           path, error);
    return false;
}
```

## 扩展指南

### 添加新平台支持
1. 创建平台特定目录：`src/infrastructure/platform/newplatform/`
2. 实现所有接口函数
3. 添加平台检测逻辑
4. 编写平台特定测试
5. 更新构建系统

### 添加新功能接口
1. 在`CN_platform.h`中定义新接口结构
2. 在所有平台实现中添加对应函数
3. 更新统一平台接口结构
4. 编写测试用例
5. 更新文档

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本发布
- 支持Windows、Linux、macOS基础功能
- 提供文件系统、线程、网络、时间、系统信息接口

### 未来计划
- 移动平台支持（Android、iOS）
- 更多高级功能（进程管理、窗口系统）
- 性能优化和内存使用改进

## 贡献指南

1. 遵循项目编码规范
2. 所有新功能必须包含测试
3. 更新相关文档
4. 确保跨平台兼容性
5. 性能影响评估

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
