# CN_Language Windows平台实现模块

## 概述

Windows平台实现模块是CN_Language项目基础设施层的重要组成部分，提供基于Win32 API的Windows平台特定功能实现。该模块实现了平台抽象层定义的所有接口，并提供了Windows特有的功能扩展。

## 功能特性

### 1. 完整平台抽象层实现
- **文件系统接口**：基于Win32文件API的完整实现
- **线程和同步接口**：使用Windows线程API和同步原语
- **网络接口**：基于Winsock 2.0的网络功能
- **时间和日期接口**：Windows高精度计时器支持
- **系统信息接口**：Windows系统信息查询

### 2. Windows特有功能
- **注册表操作**：完整的Windows注册表读写支持
- **GUI界面**：基于Win32窗口系统的GUI支持
- **COM组件**：Windows COM组件集成支持
- **UTF-8编码支持**：完整的Unicode字符集支持

### 3. 性能优化
- **异步I/O支持**：使用IOCP（I/O完成端口）的高性能I/O
- **内存映射文件**：大文件处理优化
- **线程池**：高效的线程管理和调度

## 模块结构

### 核心文件
- `CN_platform_windows.h` - Windows平台接口声明
- `CN_platform_windows.c` - Windows平台实现

### 接口实现分类

#### 1. 文件系统接口 (`Stru_CN_FileSystemInterface_t`)
- **文件操作**：CreateFileW、ReadFile、WriteFile等
- **目录操作**：CreateDirectoryW、RemoveDirectoryW等
- **路径处理**：支持Windows路径分隔符和UNC路径
- **磁盘信息**：GetDiskFreeSpaceExW等

#### 2. 线程和同步接口 (`Stru_CN_ThreadInterface_t`)
- **线程管理**：CreateThread、_beginthreadex等
- **同步原语**：临界区、互斥锁、信号量、事件等
- **线程本地存储**：TLS API支持
- **原子操作**：Interlocked系列函数

#### 3. 网络接口 (`Stru_CN_NetworkInterface_t`)
- **套接字操作**：Winsock 2.0 API
- **网络地址**：IPv4/IPv6支持
- **异步I/O**：WSAAsyncSelect、WSAEventSelect
- **选择器**：select函数实现

#### 4. 时间和日期接口 (`Stru_CN_TimeInterface_t`)
- **高精度计时**：QueryPerformanceCounter
- **系统时间**：GetSystemTime、GetLocalTime
- **时间转换**：FileTime与Unix时间戳转换
- **时区支持**：GetTimeZoneInformation

#### 5. 系统信息接口 (`Stru_CN_SystemInterface_t`)
- **硬件信息**：GetSystemInfo、GlobalMemoryStatusEx
- **进程信息**：GetCurrentProcessId、GetProcessMemoryInfo
- **环境变量**：GetEnvironmentVariable
- **用户信息**：GetUserName、GetUserProfileDirectory

### 4. Windows特有接口

#### 注册表接口 (`Stru_CN_WindowsRegistryInterface_t`)
```c
// 注册表操作示例
Stru_CN_WindowsRegistryInterface_t* registry = CN_platform_windows_get_registry();
void* key = registry->reg_open_key("HKEY_CURRENT_USER\\Software\\CN_Language", KEY_READ);
if (key) {
    char value[256];
    if (registry->reg_get_value_string(key, "InstallPath", value, sizeof(value))) {
        printf("安装路径: %s\n", value);
    }
    registry->reg_close_key(key);
}
```

#### GUI接口 (`Stru_CN_WindowsGUIInterface_t`)
```c
// GUI创建示例
Stru_CN_WindowsGUIInterface_t* gui = CN_platform_windows_get_gui();
void* window = gui->window_create("CN_Language IDE", 800, 600);
if (window) {
    void* button = gui->create_button(window, "运行", 10, 10, 80, 30);
    void* textbox = gui->create_textbox(window, 10, 50, 780, 400);
    gui->window_show(window);
    gui->process_messages(window);
}
```

#### COM接口 (`Stru_CN_WindowsCOMInterface_t`)
```c
// COM组件使用示例
Stru_CN_WindowsCOMInterface_t* com = CN_platform_windows_get_com();
if (com->com_initialize()) {
    void* excel = com->com_create_instance("Excel.Application", "{00020813-0000-0000-C000-000000000046}");
    if (excel) {
        // 操作Excel
        com->com_set_property(excel, "Visible", &true_value);
        com->com_release_instance(excel);
    }
    com->com_uninitialize();
}
```

## 使用示例

### 基本使用
```c
#include "CN_platform_windows.h"

int main()
{
    // 初始化Windows平台
    if (!CN_platform_windows_initialize()) {
        printf("Windows平台初始化失败\n");
        return -1;
    }
    
    // 获取Windows平台接口
    Stru_CN_PlatformInterface_t* platform = CN_platform_windows_get_interface();
    if (!platform) {
        printf("获取平台接口失败\n");
        return -1;
    }
    
    // 使用文件系统接口
    if (platform->filesystem->file_exists("C:\\test.txt")) {
        CN_FileHandle_t file = platform->filesystem->file_open(
            "C:\\test.txt", Eum_FILE_MODE_READ);
        if (file) {
            char buffer[1024];
            size_t bytes = platform->filesystem->file_read(file, buffer, sizeof(buffer));
            printf("读取了 %zu 字节\n", bytes);
            platform->filesystem->file_close(file);
        }
    }
    
    // 使用Windows特有功能
    Stru_CN_WindowsRegistryInterface_t* registry = CN_platform_windows_get_registry();
    if (registry) {
        void* key = registry->reg_open_key("HKEY_CURRENT_USER\\Software", KEY_READ);
        if (key) {
            // 枚举注册表键
            char name[256];
            uint32_t index = 0;
            while (registry->reg_enum_keys(key, index, name, sizeof(name))) {
                printf("注册表键: %s\n", name);
                index++;
            }
            registry->reg_close_key(key);
        }
    }
    
    // 清理Windows平台
    CN_platform_windows_cleanup();
    return 0;
}
```

### 高级功能示例
```c
// 异步文件I/O示例
void async_file_operation()
{
    Stru_CN_PlatformInterface_t* platform = CN_platform_windows_get_interface();
    
    // 创建I/O完成端口
    HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    
    // 打开文件进行异步读取
    CN_FileHandle_t file = platform->filesystem->file_open(
        "large_file.bin", Eum_FILE_MODE_READ);
    
    if (file) {
        // 关联文件到IOCP
        Stru_CN_WindowsFileHandle_t* win_file = (Stru_CN_WindowsFileHandle_t*)file;
        CreateIoCompletionPort(win_file->handle, iocp, (ULONG_PTR)file, 0);
        
        // 发起异步读取
        OVERLAPPED overlapped = {0};
        char buffer[4096];
        ReadFile(win_file->handle, buffer, sizeof(buffer), NULL, &overlapped);
        
        // 等待I/O完成
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        OVERLAPPED* lpOverlapped;
        GetQueuedCompletionStatus(iocp, &bytes_transferred, &completion_key, &lpOverlapped, INFINITE);
        
        printf("异步读取完成: %lu 字节\n", bytes_transferred);
        
        platform->filesystem->file_close(file);
    }
    
    CloseHandle(iocp);
}
```

## 编译和链接

### 编译要求
- **Windows SDK**：Windows 10 SDK或更高版本
- **编译器**：MSVC、MinGW或Clang
- **C标准**：C99或更高

### 编译选项
```makefile
# Windows平台编译选项
WINDOWS_CFLAGS = -D_WIN32_WINNT=0x0A00  # Windows 10
WINDOWS_CFLAGS += -DWIN32_LEAN_AND_MEAN
WINDOWS_CFLAGS += -DUNICODE -D_UNICODE

# 链接库
WINDOWS_LIBS = -lws2_32 -ladvapi32 -luser32 -lgdi32 -lcomdlg32
WINDOWS_LIBS += -lshell32 -lole32 -loleaut32 -luuid

# 编译命令
$(CC) $(CFLAGS) $(WINDOWS_CFLAGS) -c CN_platform_windows.c -o CN_platform_windows.o
$(CC) -o program.exe program.o CN_platform_windows.o $(LDFLAGS) $(WINDOWS_LIBS)
```

### Visual Studio项目配置
```xml
<PropertyGroup>
  <CharacterSet>Unicode</CharacterSet>
  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
</PropertyGroup>
<ItemDefinitionGroup>
  <ClCompile>
    <PreprocessorDefinitions>_WIN32_WINNT=0x0A00;WIN32_LEAN_AND_MEAN;UNICODE;_UNICODE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  </ClCompile>
  <Link>
    <AdditionalDependencies>ws2_32.lib;advapi32.lib;user32.lib;gdi32.lib;comdlg32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib</AdditionalDependencies>
  </Link>
</ItemDefinitionGroup>
```

## 编码处理

### UTF-8支持
Windows平台实现完全支持UTF-8编码：
- 所有字符串参数都假定为UTF-8编码
- 内部使用宽字符（UTF-16）与Windows API交互
- 自动进行UTF-8 ↔ UTF-16转换

### 路径分隔符处理
- 自动将'/'转换为'\'以符合Windows约定
- 支持UNC路径（\\server\share）
- 支持长路径（\\?\前缀）

### 编码转换函数
```c
// UTF-8转宽字符
wchar_t* wpath = utf8_to_wide("C:\\测试\\文件.txt");

// 宽字符转UTF-8
char* upath = wide_to_utf8(L"C:\\测试\\文件.txt");
```

## 错误处理

### Windows错误码转换
```c
// 获取最后错误信息
DWORD error = GetLastError();
if (error != ERROR_SUCCESS) {
    LPSTR message = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&message, 0, NULL);
    
    if (message) {
        printf("Windows错误: %s\n", message);
        LocalFree(message);
    }
}
```

### CN平台错误映射
```c
// Windows错误码到CN平台错误码的映射
static Eum_CN_PlatformError_t windows_error_to_cn_error(DWORD win_error)
{
    switch (win_error) {
        case ERROR_FILE_NOT_FOUND:
            return Eum_PLATFORM_ERROR_FILE_NOT_FOUND;
        case ERROR_ACCESS_DENIED:
            return Eum_PLATFORM_ERROR_PERMISSION_DENIED;
        case ERROR_OUTOFMEMORY:
            return Eum_PLATFORM_ERROR_OUT_OF_MEMORY;
        default:
            return Eum_PLATFORM_ERROR_PLATFORM_SPECIFIC;
    }
}
```

## 性能优化

### 1. 文件I/O优化
- **缓冲I/O**：使用带缓冲的文件操作
- **内存映射**：大文件使用内存映射文件
- **异步I/O**：支持重叠I/O和IOCP

### 2. 内存管理优化
- **内存池**：频繁分配的小对象使用内存池
- **对齐分配**：确保内存对齐以提高缓存效率
- **预分配**：减少动态分配次数

### 3. 线程优化
- **线程池**：重用线程减少创建开销
- **工作窃取**：平衡线程负载
- **无锁数据结构**：减少锁竞争

## 测试策略

### 单元测试
```c
// 使用Unity测试框架
#include "unity.h"

void test_windows_file_system(void)
{
    TEST_ASSERT_TRUE(windows_file_exists("C:\\Windows\\notepad.exe"));
    TEST_ASSERT_FALSE(windows_file_exists("C:\\nonexistent.file"));
}

void test_windows_registry(void)
{
    Stru_CN_WindowsRegistryInterface_t* registry = CN_platform_windows_get_registry();
    TEST_ASSERT_NOT_NULL(registry);
    
    void* key = registry->reg_open_key("HKEY_CURRENT_USER", KEY_READ);
    TEST_ASSERT_NOT_NULL(key);
    
    registry->reg_close_key(key);
}
```

### 集成测试
- **跨平台一致性测试**：确保与Linux/macOS实现行为一致
- **性能基准测试**：测量关键操作的性能
- **内存泄漏检测**：使用Application Verifier检测内存问题

### 测试工具
- **Application Verifier**：检测内存和句柄泄漏
- **Windows Performance Analyzer**：性能分析
- **DebugDiag**：诊断工具

## 兼容性

### 支持的Windows版本
- Windows 10 (版本 1607 或更高)
- Windows 11
- Windows Server 2016 或更高

### 架构支持
- x86 (32位)
- x64 (64位)
- ARM64 (Windows on ARM)

### 编译器支持
- Microsoft Visual C++ (MSVC) 2017 或更高
- MinGW-w64 (GCC 8.0 或更高)
- Clang for Windows (LLVM 10.0 或更高)

## 安全考虑

### 1. 输入验证
- 所有用户输入都经过严格验证
- 路径规范化防止目录遍历攻击
- 缓冲区溢出防护

### 2. 权限管理
- 最小权限原则运行
- 访问控制列表（ACL）检查
- 用户账户控制（UAC）兼容

### 3. 安全功能
- **地址空间布局随机化（ASLR）**：支持/DYNAMICBASE
- **数据执行保护（DEP）**：支持/NXCOMPAT
- **控制流防护（CFG）**：支持/guard:cf

## 故障排除

### 常见问题

#### 1. 编码问题
**问题**：中文字符显示为乱码
**解决**：确保源代码保存为UTF-8编码，编译时使用/utf-8选项

#### 2. 权限问题
**问题**：文件操作失败，错误码5（ACCESS_DENIED）
**解决**：以管理员身份运行程序，或检查文件权限

#### 3. 路径问题
**问题**：长路径支持
**解决**：使用\\\\?\\前缀或启用长路径支持（Windows 10 1607+）

### 调试技巧
```c
// 启用调试输出
#ifdef _DEBUG
#define CN_WINDOWS_DEBUG 1
#else
#define CN_WINDOWS_DEBUG 0
#endif

#if CN_WINDOWS_DEBUG
#define CN_WINDOWS_TRACE(fmt, ...) \
    OutputDebugStringA("CN_WINDOWS: " fmt "\n", ##__VA_ARGS__)
#else
#define CN_WINDOWS_TRACE(fmt, ...)
#endif
```

## 贡献指南

### 开发环境设置
1. 安装Visual Studio 2022或更高版本
2. 安装Windows 10/11 SDK
3. 配置Git和代码格式化工具

### 代码规范
1. 遵循项目编码标准
2. 所有新功能必须包含测试
3. 更新相关文档
4. 确保向后兼容性

### 提交检查清单
- [ ] 代码通过所有单元测试
- [ ] 新增功能有完整的文档
- [ ] 性能影响评估完成
- [ ] 安全审查通过

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本发布
- 完整实现平台抽象层接口
- 支持Windows特有功能（注册表、GUI、COM）
- UTF-8编码完全支持
- 性能优化和内存管理

### 未来计划
- DirectX图形支持
- Windows Runtime (WinRT) 集成
- PowerShell脚本集成
- Windows服务支持

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 联系方式

- 项目主页：https://github.com/ChenChao-HuaiAn/CN_Language
- 问题跟踪：GitHub Issues
- 讨论论坛：GitHub Discussions
