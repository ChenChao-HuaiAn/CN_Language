/******************************************************************************
 * 文件名: CN_platform_linux.c
 * 功能: CN_Language Linux平台实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Linux平台功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_linux.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// ============================================================================
// 全局接口变量定义
// ============================================================================

/** Linux文件系统接口全局变量 */
Stru_CN_FileSystemInterface_t g_linux_filesystem_interface = {0};

/** Linux线程接口全局变量 */
Stru_CN_ThreadInterface_t g_linux_thread_interface = {0};

/** Linux网络接口全局变量 */
Stru_CN_NetworkInterface_t g_linux_network_interface = {0};

/** Linux时间接口全局变量 */
Stru_CN_TimeInterface_t g_linux_time_interface = {0};

/** Linux系统接口全局变量 */
Stru_CN_SystemInterface_t g_linux_system_interface = {0};

/** Linux inotify接口全局变量 */
Stru_CN_LinuxInotifyInterface_t g_linux_inotify_interface = {0};

/** Linux epoll接口全局变量 */
Stru_CN_LinuxEpollInterface_t g_linux_epoll_interface = {0};

/** Linux文件描述符工具接口全局变量 */
Stru_CN_LinuxFDInterface_t g_linux_fd_tools_interface = {0};

/** Linux系统调用接口全局变量 */
Stru_CN_LinuxSyscallInterface_t g_linux_syscall_interface = {0};

// ============================================================================
// 内部状态管理
// ============================================================================

/** 平台初始化状态 */
static bool g_linux_platform_initialized = false;

/** 平台功能支持位掩码 */
static uint32_t g_linux_features = 0;

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 检测Linux系统功能
 */
static void detect_linux_features(void)
{
    g_linux_features = 0;
    
    // 检测inotify支持
#ifdef __linux__
    g_linux_features |= CN_LINUX_FEATURE_INOTIFY;
#endif
    
    // 检测epoll支持
#ifdef EPOLL_CLOEXEC
    g_linux_features |= CN_LINUX_FEATURE_EPOLL;
#endif
    
    // 检测signalfd支持
#ifdef SFD_CLOEXEC
    g_linux_features |= CN_LINUX_FEATURE_SIGNALFD;
#endif
    
    // 检测timerfd支持
#ifdef TFD_CLOEXEC
    g_linux_features |= CN_LINUX_FEATURE_TIMERFD;
#endif
    
    // 检测eventfd支持
#ifdef EFD_CLOEXEC
    g_linux_features |= CN_LINUX_FEATURE_EVENTFD;
#endif
    
    // 检测memfd支持
#ifdef MFD_CLOEXEC
    g_linux_features |= CN_LINUX_FEATURE_MEMFD;
#endif
}

/**
 * @brief 初始化文件系统接口
 */
static bool initialize_filesystem_interface(void)
{
    // 文件系统接口已经在CN_platform_linux_filesystem.c中定义
    // 这里只需要确保它被正确链接
    return true;
}

/**
 * @brief 初始化线程接口
 */
static bool initialize_thread_interface(void)
{
    // 这里将填充线程接口函数指针
    // 由于这是框架代码，实际实现将在单独的文件中
    return true;
}

/**
 * @brief 初始化网络接口
 */
static bool initialize_network_interface(void)
{
    // 这里将填充网络接口函数指针
    // 由于这是框架代码，实际实现将在单独的文件中
    return true;
}

/**
 * @brief 初始化时间接口
 */
static bool initialize_time_interface(void)
{
    // 这里将填充时间接口函数指针
    // 由于这是框架代码，实际实现将在单独的文件中
    return true;
}

/**
 * @brief 初始化系统接口
 */
static bool initialize_system_interface(void)
{
    // 这里将填充系统接口函数指针
    // 由于这是框架代码，实际实现将在单独的文件中
    return true;
}

/**
 * @brief 初始化Linux特定功能接口
 */
static bool initialize_linux_specific_interfaces(void)
{
    // 这里将填充Linux特定功能接口函数指针
    // 由于这是框架代码，实际实现将在单独的文件中
    return true;
}

// ============================================================================
// 公共接口函数实现
// ============================================================================

/**
 * @brief 获取Linux平台文件系统接口
 */
Stru_CN_FileSystemInterface_t* CN_platform_linux_get_filesystem(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_filesystem_interface;
}

/**
 * @brief 获取Linux平台线程接口
 */
Stru_CN_ThreadInterface_t* CN_platform_linux_get_thread(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_thread_interface;
}

/**
 * @brief 获取Linux平台网络接口
 */
Stru_CN_NetworkInterface_t* CN_platform_linux_get_network(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_network_interface;
}

/**
 * @brief 获取Linux平台时间接口
 */
Stru_CN_TimeInterface_t* CN_platform_linux_get_time(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_time_interface;
}

/**
 * @brief 获取Linux平台系统接口
 */
Stru_CN_SystemInterface_t* CN_platform_linux_get_system(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_system_interface;
}

/**
 * @brief 获取完整的Linux平台接口
 */
Stru_CN_PlatformInterface_t* CN_platform_linux_get_interface(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    
    static Stru_CN_PlatformInterface_t platform_interface = {0};
    
    // 只在第一次调用时填充接口
    if (platform_interface.filesystem == NULL)
    {
        platform_interface.filesystem = &g_linux_filesystem_interface;
        platform_interface.thread = &g_linux_thread_interface;
        platform_interface.network = &g_linux_network_interface;
        platform_interface.time = &g_linux_time_interface;
        platform_interface.system = &g_linux_system_interface;
    }
    
    return &platform_interface;
}

/**
 * @brief 初始化Linux平台实现
 */
bool CN_platform_linux_initialize(void)
{
    if (g_linux_platform_initialized)
    {
        return true; // 已经初始化
    }
    
    // 检测系统功能
    detect_linux_features();
    
    // 初始化各个接口
    bool success = true;
    
    success = success && initialize_filesystem_interface();
    success = success && initialize_thread_interface();
    success = success && initialize_network_interface();
    success = success && initialize_time_interface();
    success = success && initialize_system_interface();
    success = success && initialize_linux_specific_interfaces();
    
    if (success)
    {
        g_linux_platform_initialized = true;
    }
    
    return success;
}

/**
 * @brief 清理Linux平台实现
 */
void CN_platform_linux_cleanup(void)
{
    if (!g_linux_platform_initialized)
    {
        return;
    }
    
    // 清理各个接口
    // 这里可以添加资源释放代码
    
    g_linux_platform_initialized = false;
}

/**
 * @brief 获取Linux inotify接口
 */
Stru_CN_LinuxInotifyInterface_t* CN_platform_linux_get_inotify(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_inotify_interface;
}

/**
 * @brief 获取Linux epoll接口
 */
Stru_CN_LinuxEpollInterface_t* CN_platform_linux_get_epoll(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_epoll_interface;
}

/**
 * @brief 获取Linux文件描述符工具接口
 */
Stru_CN_LinuxFDInterface_t* CN_platform_linux_get_fd_tools(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_fd_tools_interface;
}

/**
 * @brief 获取Linux系统调用接口
 */
Stru_CN_LinuxSyscallInterface_t* CN_platform_linux_get_syscall(void)
{
    if (!g_linux_platform_initialized)
    {
        return NULL;
    }
    return &g_linux_syscall_interface;
}

/**
 * @brief 检查Linux特定功能是否可用
 */
bool CN_platform_linux_check_feature(uint32_t feature)
{
    return (g_linux_features & feature) != 0;
}

/**
 * @brief 获取Linux内核版本
 */
bool CN_platform_linux_get_kernel_version(int* major, int* minor, int* patch)
{
    if (!major || !minor || !patch)
    {
        return false;
    }
    
    // 简化实现，实际应该解析/proc/version或调用uname
    *major = 5;
    *minor = 15;
    *patch = 0;
    
    return true;
}

/**
 * @brief 获取Linux发行版信息
 */
bool CN_platform_linux_get_distro_info(char* name, size_t name_size, 
                                       char* version, size_t version_size)
{
    if (!name || name_size == 0 || !version || version_size == 0)
    {
        return false;
    }
    
    // 简化实现，实际应该解析/etc/os-release
    const char* default_name = "Ubuntu";
    const char* default_version = "22.04";
    
    strncpy(name, default_name, name_size - 1);
    name[name_size - 1] = '\0';
    
    strncpy(version, default_version, version_size - 1);
    version[version_size - 1] = '\0';
    
    return true;
}
