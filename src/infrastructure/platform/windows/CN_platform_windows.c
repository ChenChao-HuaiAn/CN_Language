/******************************************************************************
 * 文件名: CN_platform_windows.c
 * 功能: CN_Language Windows平台管理文件
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 重构为平台管理文件，集成所有子系统
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// 子系统接口声明
// ============================================================================

// 文件系统子系统接口
extern Stru_CN_FileSystemInterface_t* CN_platform_windows_get_filesystem_impl(void);

// 线程子系统接口
extern Stru_CN_ThreadInterface_t* CN_platform_windows_get_thread_impl(void);

// 网络子系统接口
extern Stru_CN_NetworkInterface_t* CN_platform_windows_get_network_impl(void);

// 时间子系统接口
extern Stru_CN_TimeInterface_t* CN_platform_windows_get_time_impl(void);

// 系统信息子系统接口
extern Stru_CN_SystemInterface_t* CN_platform_windows_get_system_impl(void);

// 注册表子系统接口
extern Stru_CN_WindowsRegistryInterface_t* CN_platform_windows_get_registry_impl(void);

// GUI子系统接口
extern Stru_CN_WindowsGUIInterface_t* CN_platform_windows_get_gui_impl(void);

// COM子系统接口
extern Stru_CN_WindowsCOMInterface_t* CN_platform_windows_get_com_impl(void);

// ============================================================================
// 全局平台接口实例
// ============================================================================

static Stru_CN_PlatformInterface_t g_windows_platform_interface = {0};
static bool g_windows_platform_initialized = false;

// ============================================================================
// 平台管理函数
// ============================================================================

/**
 * @brief 初始化Windows平台实现
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_windows_initialize(void)
{
    if (g_windows_platform_initialized)
    {
        return true;
    }
    
    // 初始化Winsock（用于网络子系统）
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        // 网络初始化失败，但其他子系统仍可用
        // 可以继续初始化其他子系统
    }
    
    // 获取各个子系统接口
    g_windows_platform_interface.filesystem = CN_platform_windows_get_filesystem_impl();
    g_windows_platform_interface.thread = CN_platform_windows_get_thread_impl();
    g_windows_platform_interface.network = CN_platform_windows_get_network_impl();
    g_windows_platform_interface.time = CN_platform_windows_get_time_impl();
    g_windows_platform_interface.system = CN_platform_windows_get_system_impl();
    
    // 检查必需接口是否可用
    if (!g_windows_platform_interface.filesystem ||
        !g_windows_platform_interface.thread ||
        !g_windows_platform_interface.time ||
        !g_windows_platform_interface.system)
    {
        // 必需接口缺失，清理并返回失败
        CN_platform_windows_cleanup();
        return false;
    }
    
    g_windows_platform_initialized = true;
    return true;
}

/**
 * @brief 清理Windows平台实现
 */
void CN_platform_windows_cleanup(void)
{
    if (!g_windows_platform_initialized)
    {
        return;
    }
    
    // 清理网络子系统
    if (g_windows_platform_interface.network && 
        g_windows_platform_interface.network->network_cleanup)
    {
        g_windows_platform_interface.network->network_cleanup();
    }
    
    // 清理Winsock
    WSACleanup();
    
    // 重置接口
    g_windows_platform_interface.filesystem = NULL;
    g_windows_platform_interface.thread = NULL;
    g_windows_platform_interface.network = NULL;
    g_windows_platform_interface.time = NULL;
    g_windows_platform_interface.system = NULL;
    
    g_windows_platform_initialized = false;
}

/**
 * @brief 获取Windows平台文件系统接口
 * 
 * @return Windows平台文件系统接口指针
 */
Stru_CN_FileSystemInterface_t* CN_platform_windows_get_filesystem(void)
{
    // 实际实现在CN_platform_windows_filesystem.c中
    extern Stru_CN_FileSystemInterface_t g_windows_filesystem_interface;
    return &g_windows_filesystem_interface;
}

/**
 * @brief 获取Windows平台线程接口
 * 
 * @return Windows平台线程接口指针
 */
Stru_CN_ThreadInterface_t* CN_platform_windows_get_thread(void)
{
    // 实际实现在CN_platform_windows_thread.c中
    extern Stru_CN_ThreadInterface_t g_windows_thread_interface;
    return &g_windows_thread_interface;
}

/**
 * @brief 获取Windows平台网络接口
 * 
 * @return Windows平台网络接口指针
 */
Stru_CN_NetworkInterface_t* CN_platform_windows_get_network(void)
{
    // 实际实现在CN_platform_windows_network.c中
    extern Stru_CN_NetworkInterface_t g_windows_network_interface;
    return &g_windows_network_interface;
}

/**
 * @brief 获取Windows平台时间接口
 * 
 * @return Windows平台时间接口指针
 */
Stru_CN_TimeInterface_t* CN_platform_windows_get_time(void)
{
    // 实际实现在CN_platform_windows_time.c中
    extern Stru_CN_TimeInterface_t g_windows_time_interface;
    return &g_windows_time_interface;
}

/**
 * @brief 获取Windows平台系统接口
 * 
 * @return Windows平台系统接口指针
 */
Stru_CN_SystemInterface_t* CN_platform_windows_get_system(void)
{
    // 实际实现在CN_platform_windows_system.c中
    extern Stru_CN_SystemInterface_t g_windows_system_interface;
    return &g_windows_system_interface;
}

/**
 * @brief 获取完整的Windows平台接口
 * 
 * @return Windows平台接口指针
 */
Stru_CN_PlatformInterface_t* CN_platform_windows_get_interface(void)
{
    if (!g_windows_platform_initialized)
    {
        // 自动初始化
        if (!CN_platform_windows_initialize())
        {
            return NULL;
        }
    }
    
    return &g_windows_platform_interface;
}

/**
 * @brief 获取Windows注册表接口
 * 
 * @return Windows注册表接口指针
 */
Stru_CN_WindowsRegistryInterface_t* CN_platform_windows_get_registry(void)
{
    // 实际实现在CN_platform_windows_special.c中
    extern Stru_CN_WindowsRegistryInterface_t* CN_platform_windows_get_registry_impl(void);
    return CN_platform_windows_get_registry_impl();
}

/**
 * @brief 获取Windows GUI接口
 * 
 * @return Windows GUI接口指针
 */
Stru_CN_WindowsGUIInterface_t* CN_platform_windows_get_gui(void)
{
    // 实际实现在CN_platform_windows_special.c中
    extern Stru_CN_WindowsGUIInterface_t* CN_platform_windows_get_gui_impl(void);
    return CN_platform_windows_get_gui_impl();
}

/**
 * @brief 获取Windows COM接口
 * 
 * @return Windows COM接口指针
 */
Stru_CN_WindowsCOMInterface_t* CN_platform_windows_get_com(void)
{
    // 实际实现在CN_platform_windows_special.c中
    extern Stru_CN_WindowsCOMInterface_t* CN_platform_windows_get_com_impl(void);
    return CN_platform_windows_get_com_impl();
}

// ============================================================================
// 平台检测函数
// ============================================================================

/**
 * @brief 检查是否为Windows平台
 * 
 * @return 如果是Windows平台返回true，否则返回false
 */
bool CN_platform_is_windows(void)
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

/**
 * @brief 获取当前平台类型
 * 
 * @return 平台类型字符串
 */
const char* CN_platform_get_type(void)
{
    return "windows";
}

/**
 * @brief 检查是否为Linux平台
 * 
 * @return 如果是Linux平台返回true，否则返回false
 */
bool CN_platform_is_linux(void)
{
    return false;
}

/**
 * @brief 检查是否为macOS平台
 * 
 * @return 如果是macOS平台返回true，否则返回false
 */
bool CN_platform_is_macos(void)
{
    return false;
}

/**
 * @brief 检查是否为Unix-like平台
 * 
 * @return 如果是Unix-like平台返回true，否则返回false
 */
bool CN_platform_is_unix(void)
{
    return false;
}

// ============================================================================
// 平台抽象层接口实现
// ============================================================================

/**
 * @brief 获取默认平台接口
 * 
 * 根据当前运行平台返回相应的平台接口实现。
 * 
 * @return 平台接口指针，失败返回NULL
 */
Stru_CN_PlatformInterface_t* CN_platform_get_default(void)
{
    if (!CN_platform_is_windows())
    {
        // 非Windows平台，返回NULL
        return NULL;
    }
    
    return CN_platform_windows_get_interface();
}

/**
 * @brief 初始化平台抽象层
 * 
 * 必须在调用任何平台函数之前调用此函数。
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_initialize(void)
{
    if (!CN_platform_is_windows())
    {
        // 非Windows平台，返回失败
        return false;
    }
    
    return CN_platform_windows_initialize();
}

/**
 * @brief 清理平台抽象层
 * 
 * 在程序退出前调用此函数释放平台资源。
 */
void CN_platform_cleanup(void)
{
    if (!CN_platform_is_windows())
    {
        return;
    }
    
    CN_platform_windows_cleanup();
}
