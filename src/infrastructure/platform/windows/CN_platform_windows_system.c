/******************************************************************************
 * 文件名: CN_platform_windows_system.c
 * 功能: CN_Language Windows平台系统信息子系统
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows系统信息功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows.h"
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <shlobj.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 获取Windows版本信息
 */
static void get_windows_version_info(char* os_name, char* os_version, size_t size)
{
    if (!os_name || !os_version || size == 0) return;
    
    OSVERSIONINFOEXW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    
    // 使用RtlGetVersion获取准确的版本信息
    typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
    
    if (RtlGetVersion)
    {
        RTL_OSVERSIONINFOW rtl_osvi = {0};
        rtl_osvi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
        RtlGetVersion(&rtl_osvi);
        
        osvi.dwMajorVersion = rtl_osvi.dwMajorVersion;
        osvi.dwMinorVersion = rtl_osvi.dwMinorVersion;
        osvi.dwBuildNumber = rtl_osvi.dwBuildNumber;
    }
    else
    {
        // 回退到GetVersionEx
        GetVersionExW((OSVERSIONINFOW*)&osvi);
    }
    
    // 确定Windows版本名称
    const char* version_name = "Windows";
    if (osvi.dwMajorVersion == 10)
    {
        if (osvi.dwBuildNumber >= 22000)
            version_name = "Windows 11";
        else
            version_name = "Windows 10";
    }
    else if (osvi.dwMajorVersion == 6)
    {
        if (osvi.dwMinorVersion == 3)
            version_name = "Windows 8.1";
        else if (osvi.dwMinorVersion == 2)
            version_name = "Windows 8";
        else if (osvi.dwMinorVersion == 1)
            version_name = "Windows 7";
        else if (osvi.dwMinorVersion == 0)
            version_name = "Windows Vista";
    }
    else if (osvi.dwMajorVersion == 5)
    {
        if (osvi.dwMinorVersion == 2)
            version_name = "Windows Server 2003";
        else if (osvi.dwMinorVersion == 1)
            version_name = "Windows XP";
        else if (osvi.dwMinorVersion == 0)
            version_name = "Windows 2000";
    }
    
    strncpy(os_name, version_name, size - 1);
    os_name[size - 1] = '\0';
    
    // 构建版本字符串
    snprintf(os_version, size, "%lu.%lu.%lu", 
             osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    os_version[size - 1] = '\0';
}

/**
 * @brief 获取系统架构信息
 */
static void get_system_architecture(char* architecture, size_t size)
{
    if (!architecture || size == 0) return;
    
    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);
    
    switch (sys_info.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:
            strncpy(architecture, "x64", size - 1);
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            strncpy(architecture, "ARM", size - 1);
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            strncpy(architecture, "ARM64", size - 1);
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            strncpy(architecture, "IA64", size - 1);
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            strncpy(architecture, "x86", size - 1);
            break;
        default:
            strncpy(architecture, "Unknown", size - 1);
            break;
    }
    architecture[size - 1] = '\0';
}

/**
 * @brief 获取处理器名称
 */
static void get_processor_name(char* processor_name, size_t size)
{
    if (!processor_name || size == 0) return;
    
    HKEY hKey;
    DWORD buffer_size = size;
    
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        wchar_t wbuffer[256];
        DWORD wbuffer_size = sizeof(wbuffer);
        
        if (RegQueryValueExW(hKey, L"ProcessorNameString", NULL, NULL,
                            (LPBYTE)wbuffer, &wbuffer_size) == ERROR_SUCCESS)
        {
            // 转换为UTF-8
            int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1,
                                              NULL, 0, NULL, NULL);
            if (utf8_len > 0 && utf8_len <= (int)size)
            {
                WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1,
                                   processor_name, size, NULL, NULL);
            }
            else
            {
                strncpy(processor_name, "Unknown Processor", size - 1);
                processor_name[size - 1] = '\0';
            }
        }
        else
        {
            strncpy(processor_name, "Unknown Processor", size - 1);
            processor_name[size - 1] = '\0';
        }
        
        RegCloseKey(hKey);
    }
    else
    {
        strncpy(processor_name, "Unknown Processor", size - 1);
        processor_name[size - 1] = '\0';
    }
}

// ============================================================================
// 系统信息接口实现
// ============================================================================

// 获取系统信息
bool windows_get_system_info(Stru_CN_SystemInfo_t* info)
{
    if (!info) return false;
    
    // 获取操作系统信息
    get_windows_version_info(info->os_name, info->os_version, 
                            sizeof(info->os_name));
    
    // 获取系统架构
    get_system_architecture(info->architecture, sizeof(info->architecture));
    
    // 获取内存页大小
    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);
    info->page_size = sys_info.dwPageSize;
    
    // 获取内存信息
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&mem_status))
    {
        info->total_physical_memory = mem_status.ullTotalPhys;
        info->available_physical_memory = mem_status.ullAvailPhys;
    }
    else
    {
        info->total_physical_memory = 0;
        info->available_physical_memory = 0;
    }
    
    // 获取处理器信息
    info->processor_count = sys_info.dwNumberOfProcessors;
    
    // 获取处理器名称
    get_processor_name(info->processor_name, sizeof(info->processor_name));
    
    // 获取处理器速度（简化实现）
    HKEY hKey;
    DWORD mhz = 0;
    DWORD mhz_size = sizeof(mhz);
    
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueExW(hKey, L"~MHz", NULL, NULL,
                        (LPBYTE)&mhz, &mhz_size);
        RegCloseKey(hKey);
    }
    
    info->processor_speed_mhz = mhz;
    
    return true;
}

// 获取总内存
uint64_t windows_get_total_memory(void)
{
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&mem_status))
    {
        return mem_status.ullTotalPhys;
    }
    
    return 0;
}

// 获取可用内存
uint64_t windows_get_available_memory(void)
{
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&mem_status))
    {
        return mem_status.ullAvailPhys;
    }
    
    return 0;
}

// 获取进程内存使用
uint64_t windows_get_process_memory_usage(void)
{
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
    
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        return pmc.WorkingSetSize;
    }
    
    return 0;
}

// 获取CPU核心数
uint32_t windows_get_cpu_count(void)
{
    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);
    return sys_info.dwNumberOfProcessors;
}

// 获取CPU使用率（简化实现）
double windows_get_cpu_usage(void)
{
    // 简化实现：返回0.0
    // 实际实现需要使用PDH API或性能计数器
    return 0.0;
}

// 获取环境变量
char* windows_get_environment_variable(const char* name)
{
    if (!name) return NULL;
    
    // 转换为宽字符
    wchar_t* wname = NULL;
    int wname_len = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (wname_len <= 0) return NULL;
    
    wname = (wchar_t*)malloc(wname_len * sizeof(wchar_t));
    if (!wname) return NULL;
    
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, wname_len);
    
    // 获取环境变量值
    DWORD buffer_size = GetEnvironmentVariableW(wname, NULL, 0);
    if (buffer_size == 0)
    {
        free(wname);
        return NULL;
    }
    
    wchar_t* wvalue = (wchar_t*)malloc(buffer_size * sizeof(wchar_t));
    if (!wvalue)
    {
        free(wname);
        return NULL;
    }
    
    GetEnvironmentVariableW(wname, wvalue, buffer_size);
    free(wname);
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wvalue, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0)
    {
        free(wvalue);
        return NULL;
    }
    
    char* value = (char*)malloc(utf8_len);
    if (!value)
    {
        free(wvalue);
        return NULL;
    }
    
    WideCharToMultiByte(CP_UTF8, 0, wvalue, -1, value, utf8_len, NULL, NULL);
    free(wvalue);
    
    return value;
}

// 设置环境变量
bool windows_set_environment_variable(const char* name, const char* value)
{
    if (!name) return false;
    
    // 转换为宽字符
    wchar_t* wname = NULL;
    wchar_t* wvalue = NULL;
    
    int wname_len = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (wname_len <= 0) return false;
    
    wname = (wchar_t*)malloc(wname_len * sizeof(wchar_t));
    if (!wname) return false;
    
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, wname_len);
    
    if (value)
    {
        int wvalue_len = MultiByteToWideChar(CP_UTF8, 0, value, -1, NULL, 0);
        if (wvalue_len <= 0)
        {
            free(wname);
            return false;
        }
        
        wvalue = (wchar_t*)malloc(wvalue_len * sizeof(wchar_t));
        if (!wvalue)
        {
            free(wname);
            return false;
        }
        
        MultiByteToWideChar(CP_UTF8, 0, value, -1, wvalue, wvalue_len);
    }
    
    // 设置环境变量
    BOOL result = SetEnvironmentVariableW(wname, wvalue);
    
    free(wname);
    if (wvalue) free(wvalue);
    
    return result != FALSE;
}

// 获取进程ID
uint32_t windows_get_process_id(void)
{
    return GetCurrentProcessId();
}

// 获取父进程ID
uint32_t windows_get_parent_process_id(void)
{
    // Windows没有直接获取父进程ID的API
    // 需要使用NtQueryInformationProcess
    return 0;
}

// 获取进程名称
char* windows_get_process_name(void)
{
    wchar_t wpath[MAX_PATH];
    DWORD result = GetModuleFileNameW(NULL, wpath, MAX_PATH);
    
    if (result == 0 || result == MAX_PATH)
    {
        return NULL;
    }
    
    // 提取文件名部分
    wchar_t* wfilename = wcsrchr(wpath, L'\\');
    if (wfilename) wfilename++;
    else wfilename = wpath;
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wfilename, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return NULL;
    
    char* filename = (char*)malloc(utf8_len);
    if (!filename) return NULL;
    
    WideCharToMultiByte(CP_UTF8, 0, wfilename, -1, filename, utf8_len, NULL, NULL);
    
    return filename;
}

// 获取命令行
char* windows_get_command_line(void)
{
    wchar_t* wcmdline = GetCommandLineW();
    if (!wcmdline) return NULL;
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wcmdline, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return NULL;
    
    char* cmdline = (char*)malloc(utf8_len);
    if (!cmdline) return NULL;
    
    WideCharToMultiByte(CP_UTF8, 0, wcmdline, -1, cmdline, utf8_len, NULL, NULL);
    
    return cmdline;
}

// 获取参数数量
int windows_get_argument_count(void)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) LocalFree(argv);
    return argc;
}

// 获取参数
char* windows_get_argument(int index)
{
    int argc = 0;
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (!wargv || index < 0 || index >= argc)
    {
        if (wargv) LocalFree(wargv);
        return NULL;
    }
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wargv[index], -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0)
    {
        LocalFree(wargv);
        return NULL;
    }
    
    char* arg = (char*)malloc(utf8_len);
    if (!arg)
    {
        LocalFree(wargv);
        return NULL;
    }
    
    WideCharToMultiByte(CP_UTF8, 0, wargv[index], -1, arg, utf8_len, NULL, NULL);
    LocalFree(wargv);
    
    return arg;
}

// 获取系统目录
char* windows_get_system_directory(void)
{
    wchar_t wpath[MAX_PATH];
    UINT result = GetSystemDirectoryW(wpath, MAX_PATH);
    
    if (result == 0 || result > MAX_PATH)
    {
        return NULL;
    }
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return NULL;
    
    char* path = (char*)malloc(utf8_len);
    if (!path) return NULL;
    
    WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, utf8_len, NULL, NULL);
    
    return path;
}

// 获取Program Files目录
char* windows_get_program_files_directory(void)
{
    wchar_t wpath[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, wpath);
    
    if (FAILED(result))
    {
        return NULL;
    }
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return NULL;
    
    char* path = (char*)malloc(utf8_len);
    if (!path) return NULL;
    
    WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, utf8_len, NULL, NULL);
    
    return path;
}

// 获取用户名
char* windows_get_user_name(void)
{
    wchar_t wname[256];
    DWORD size = sizeof(wname) / sizeof(wchar_t);
    
    if (!GetUserNameW(wname, &size))
    {
        return NULL;
    }
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wname, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return NULL;
    
    char* name = (char*)malloc(utf8_len);
    if (!name) return NULL;
    
    WideCharToMultiByte(CP_UTF8, 0, wname, -1, name, utf8_len, NULL, NULL);
    
    return name;
}

// 获取用户主目录
char* windows_get_user_home_directory(void)
{
    wchar_t wpath[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, wpath);
    
    if (FAILED(result))
    {
        return NULL;
    }
    
    // 转换为UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return NULL;
    
    char* path = (char*)malloc(utf8_len);
    if (!path) return NULL;
    
    WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, utf8_len, NULL, NULL);
    
    return path;
}

// 显示消息框
bool windows_show_message_box(const char* title, const char* message, uint32_t flags)
{
    if (!title || !message) return false;
    
    // 转换为宽字符
    wchar_t* wtitle = NULL;
    wchar_t* wmessage = NULL;
    
    int wtitle_len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    if (wtitle_len <= 0) return false;
    
    wtitle = (wchar_t*)malloc(wtitle_len * sizeof(wchar_t));
    if (!wtitle) return false;
    
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, wtitle_len);
    
    int wmessage_len = MultiByteToWideChar(CP_UTF8, 0, message, -1, NULL, 0);
    if (wmessage_len <= 0)
    {
        free(wtitle);
        return false;
    }
    
    wmessage = (wchar_t*)malloc(wmessage_len * sizeof(wchar_t));
    if (!wmessage)
    {
        free(wtitle);
        return false;
    }
    
    MultiByteToWideChar(CP_UTF8, 0, message, -1, wmessage, wmessage_len);
    
    // 转换标志
    UINT type = MB_OK;
    if (flags & 0x01) type = MB_OKCANCEL;
    if (flags & 0x02) type = MB_YESNO;
    if (flags & 0x04) type |= MB_ICONINFORMATION;
    if (flags & 0x08) type |= MB_ICONWARNING;
    if (flags & 0x10) type |= MB_ICONERROR;
    if (flags & 0x20) type |= MB_ICONQUESTION;
    
    // 显示消息框
    int result = MessageBoxW(NULL, wmessage, wtitle, type);
    
    free(wtitle);
    free(wmessage);
    
    return result != 0;
}

// 播放声音
bool windows_play_sound(const char* sound_name)
{
    if (!sound_name) return false;
    
    // 转换为宽字符
    wchar_t* wsound = NULL;
    int wsound_len = MultiByteToWideChar(CP_UTF8, 0, sound_name, -1, NULL, 0);
    if (wsound_len <= 0) return false;
    
    wsound = (wchar_t*)malloc(wsound_len * sizeof(wchar_t));
    if (!wsound) return false;
    
    MultiByteToWideChar(CP_UTF8, 0, sound_name, -1, wsound, wsound_len);
    
    // 播放声音
    BOOL result = PlaySoundW(wsound, NULL, SND_ALIAS | SND_ASYNC);
    
    free(wsound);
    return result != FALSE;
}

// 获取电源状态
bool windows_get_power_status(uint32_t* battery_percent, bool* is_charging)
{
    SYSTEM_POWER_STATUS power_status;
    
    if (!GetSystemPowerStatus(&power_status))
    {
        return false;
    }
    
    if (battery_percent)
    {
        *battery_percent = power_status.BatteryLifePercent;
        if (*battery_percent > 100) *battery_percent = 100;
    }
    
    if (is_charging)
    {
        *is_charging = (power_status.ACLineStatus == 1) || 
                      (power_status.BatteryFlag & 8); // 充电中
    }
    
    return true;
}

// 设置省电模式
bool windows_set_power_saving_mode(bool enable)
{
    // Windows没有直接的API设置省电模式
    // 可以通过电源计划实现，但这里简化处理
    return false;
}

// 关闭系统
bool windows_shutdown_system(uint32_t timeout_seconds)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    
    // 获取关机权限
    if (!OpenProcessToken(GetCurrentProcess(), 
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }
    
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
    CloseHandle(hToken);
    
    // 关机
    if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER))
    {
        return false;
    }
    
    return true;
}

// 重启系统
bool windows_reboot_system(uint32_t timeout_seconds)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    
    // 获取关机权限
    if (!OpenProcessToken(GetCurrentProcess(), 
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }
    
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
    CloseHandle(hToken);
    
    // 重启
    if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER))
    {
        return false;
    }
    
    return true;
}

// 用户注销
bool windows_logoff_user(void)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    
    // 获取关机权限
    if (!OpenProcessToken(GetCurrentProcess(), 
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }
    
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
    CloseHandle(hToken);
    
    // 注销
    if (!ExitWindowsEx(EWX_LOGOFF | EWX_FORCE, 0))
    {
        return false;
    }
    
    return true;
}

// ============================================================================
// 系统接口结构体
// ============================================================================

Stru_CN_SystemInterface_t g_windows_system_interface = {
    .get_system_info = windows_get_system_info,
    .get_total_memory = windows_get_total_memory,
    .get_available_memory = windows_get_available_memory,
    .get_process_memory_usage = windows_get_process_memory_usage,
    .get_cpu_count = windows_get_cpu_count,
    .get_cpu_usage = windows_get_cpu_usage,
    .get_environment_variable = windows_get_environment_variable,
    .set_environment_variable = windows_set_environment_variable,
    .get_process_id = windows_get_process_id,
    .get_parent_process_id = windows_get_parent_process_id,
    .get_process_name = windows_get_process_name,
    .get_command_line = windows_get_command_line,
    .get_argument_count = windows_get_argument_count,
    .get_argument = windows_get_argument,
    .get_system_directory = windows_get_system_directory,
    .get_program_files_directory = windows_get_program_files_directory,
    .get_user_name = windows_get_user_name,
    .get_user_home_directory = windows_get_user_home_directory,
    .show_message_box = windows_show_message_box,
    .play_sound = windows_play_sound,
    .get_power_status = windows_get_power_status,
    .set_power_saving_mode = windows_set_power_saving_mode,
    .shutdown_system = windows_shutdown_system,
    .reboot_system = windows_reboot_system,
    .logoff_user = windows_logoff_user
};

// ============================================================================
// 公共接口函数
// ============================================================================

/**
 * @brief 获取Windows平台系统接口
 * 
 * @return Windows平台系统接口指针
 */
Stru_CN_SystemInterface_t* CN_platform_windows_get_system_impl(void)
{
    return &g_windows_system_interface;
}
