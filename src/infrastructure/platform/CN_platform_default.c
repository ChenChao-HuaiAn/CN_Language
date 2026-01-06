/**
 * @file CN_platform_default.c
 * @brief 默认平台接口实现
 * 
 * 本文件实现了默认平台接口的自动检测逻辑，根据当前编译的平台
 * 自动选择Windows、Linux或macOS实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 */

#include "CN_platform_interface.h"

// 声明各个平台的工厂函数
const Stru_PlatformInterface_t* F_get_windows_platform_interface(void);
const Stru_PlatformInterface_t* F_get_linux_platform_interface(void);
const Stru_PlatformInterface_t* F_get_macos_platform_interface(void);
const Stru_PlatformInterface_t* F_get_stub_platform_interface(void);

/**
 * @brief 获取默认平台接口
 * @return 平台接口指针，失败返回NULL
 * 
 * 此函数根据当前编译的平台自动选择正确的实现：
 * - Windows: 返回Windows平台接口
 * - Linux: 返回Linux平台接口  
 * - macOS: 返回macOS平台接口
 * - 其他: 返回测试桩接口
 * 
 * @note 此函数是线程安全的，可以在多线程环境中调用。
 */
const Stru_PlatformInterface_t* F_get_default_platform_interface(void)
{
    // 根据编译时的平台定义选择正确的实现
    #if defined(_WIN32) || defined(_WIN64)
        // Windows平台
        return F_get_windows_platform_interface();
    
    #elif defined(__linux__)
        // Linux平台
        return F_get_linux_platform_interface();
    
    #elif defined(__APPLE__) && defined(__MACH__)
        // macOS平台
        return F_get_macos_platform_interface();
    
    #else
        // 未知平台，返回测试桩接口
        // 这可以用于单元测试或跨平台开发
        return F_get_stub_platform_interface();
    
    #endif
}

/**
 * @brief 平台抽象层初始化函数
 * @return 成功返回true，失败返回false
 * 
 * 此函数初始化平台抽象层，调用默认平台接口的initialize方法。
 * 应该在程序启动时调用一次。
 */
bool CN_platform_initialize(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform)
        return false;
    
    if (platform->initialize)
        return platform->initialize();
    
    return true;
}

/**
 * @brief 平台抽象层清理函数
 * 
 * 此函数清理平台抽象层资源，调用默认平台接口的cleanup方法。
 * 应该在程序退出时调用一次。
 */
void CN_platform_cleanup(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform)
        return;
    
    if (platform->cleanup)
        platform->cleanup();
}

/**
 * @brief 获取当前平台名称
 * @return 平台名称字符串
 */
const char* CN_platform_get_name(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->get_platform_name)
        return "Unknown";
    
    return platform->get_platform_name();
}

/**
 * @brief 获取当前平台版本
 * @return 平台版本字符串
 */
const char* CN_platform_get_version(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->get_platform_version)
        return "Unknown";
    
    return platform->get_platform_version();
}

/**
 * @brief 获取CPU架构信息
 * @return CPU架构字符串
 */
const char* CN_platform_get_cpu_architecture(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->get_cpu_architecture)
        return "Unknown";
    
    return platform->get_cpu_architecture();
}

/**
 * @brief 获取系统内存信息
 * @param total_memory 总内存大小（字节）
 * @param available_memory 可用内存大小（字节）
 * @return 成功返回true，失败返回false
 */
bool CN_platform_get_memory_info(uint64_t* total_memory, uint64_t* available_memory)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->get_system_memory_info)
        return false;
    
    return platform->get_system_memory_info(total_memory, available_memory);
}

/**
 * @brief 获取CPU核心数
 * @return CPU核心数
 */
uint32_t CN_platform_get_cpu_core_count(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->get_cpu_core_count)
        return 1;
    
    return platform->get_cpu_core_count();
}

/**
 * @brief 获取系统信息
 * @param info_buffer 信息缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回true，失败返回false
 */
bool CN_platform_get_system_info(char* info_buffer, size_t buffer_size)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->get_system_info)
        return false;
    
    return platform->get_system_info(info_buffer, buffer_size);
}

/**
 * @brief 获取文件系统接口
 * @return 文件系统接口指针，失败返回NULL
 */
const Stru_FileSystemInterface_t* CN_platform_get_file_system_interface(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform)
        return NULL;
    
    return platform->file_system;
}

/**
 * @brief 获取进程管理接口
 * @return 进程管理接口指针，失败返回NULL
 */
const Stru_ProcessInterface_t* CN_platform_get_process_interface(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform)
        return NULL;
    
    return platform->process;
}

/**
 * @brief 获取时间管理接口
 * @return 时间管理接口指针，失败返回NULL
 */
const Stru_TimeInterface_t* CN_platform_get_time_interface(void)
{
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform)
        return NULL;
    
    return platform->time;
}
