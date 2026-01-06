/**
 * @file CN_platform_windows.c
 * @brief Windows平台实现
 * 
 * 本文件实现了Windows平台的具体功能，包括文件系统、进程、时间等接口。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 */

#include "CN_platform_interface.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <sys/stat.h>
#include <time.h>
#include <psapi.h>
#include <tlhelp32.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 将Windows文件属性转换为CN文件属性
 * @param win_attr Windows文件属性
 * @return CN文件属性
 */
static uint32_t F_convert_windows_attributes(DWORD win_attr)
{
    uint32_t cn_attr = 0;
    
    if (win_attr & FILE_ATTRIBUTE_READONLY)
        cn_attr |= Eum_FILE_ATTR_READONLY;
    if (win_attr & FILE_ATTRIBUTE_HIDDEN)
        cn_attr |= Eum_FILE_ATTR_HIDDEN;
    if (win_attr & FILE_ATTRIBUTE_SYSTEM)
        cn_attr |= Eum_FILE_ATTR_SYSTEM;
    if (win_attr & FILE_ATTRIBUTE_DIRECTORY)
        cn_attr |= Eum_FILE_ATTR_DIRECTORY;
    if (win_attr & FILE_ATTRIBUTE_ARCHIVE)
        cn_attr |= Eum_FILE_ATTR_ARCHIVE;
    if (win_attr & FILE_ATTRIBUTE_NORMAL)
        cn_attr |= Eum_FILE_ATTR_NORMAL;
    
    return cn_attr;
}

/**
 * @brief 将CN文件打开模式转换为Windows打开模式
 * @param mode CN文件打开模式
 * @return Windows打开标志
 */
static DWORD F_convert_open_mode(Eum_FileOpenMode mode)
{
    switch (mode)
    {
        case Eum_FILE_MODE_READ:
            return GENERIC_READ;
        case Eum_FILE_MODE_WRITE:
            return GENERIC_WRITE;
        case Eum_FILE_MODE_APPEND:
            return GENERIC_WRITE | FILE_APPEND_DATA;
        case Eum_FILE_MODE_READ_WRITE:
            return GENERIC_READ | GENERIC_WRITE;
        case Eum_FILE_MODE_CREATE:
            return GENERIC_WRITE | CREATE_ALWAYS;
        default:
            return GENERIC_READ;
    }
}

/**
 * @brief 将Windows FILETIME转换为Unix时间戳（毫秒）
 * @param filetime Windows FILETIME
 * @return Unix时间戳（毫秒）
 */
static uint64_t F_filetime_to_unix_milliseconds(const FILETIME* filetime)
{
    // Windows FILETIME是1601年1月1日以来的100纳秒间隔数
    // Unix时间戳是1970年1月1日以来的秒数
    const uint64_t WINDOWS_TICK = 10000000;
    const uint64_t SEC_TO_UNIX_EPOCH = 11644473600LL;
    
    ULARGE_INTEGER large_int;
    large_int.LowPart = filetime->dwLowDateTime;
    large_int.HighPart = filetime->dwHighDateTime;
    
    // 转换为秒，然后减去Windows到Unix的偏移
    uint64_t seconds = large_int.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH;
    uint64_t milliseconds = seconds * 1000;
    
    return milliseconds;
}

// ============================================================================
// 文件系统接口实现
// ============================================================================

static void* F_windows_open_file(const char* path, Eum_FileOpenMode mode)
{
    DWORD dwDesiredAccess = F_convert_open_mode(mode);
    DWORD dwCreationDisposition = OPEN_EXISTING;
    
    if (mode == Eum_FILE_MODE_CREATE)
        dwCreationDisposition = CREATE_ALWAYS;
    else if (mode == Eum_FILE_MODE_WRITE)
        dwCreationDisposition = CREATE_NEW;
    
    HANDLE hFile = CreateFileA(
        path,
        dwDesiredAccess,
        FILE_SHARE_READ,
        NULL,
        dwCreationDisposition,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE)
        return CN_INVALID_FILE_HANDLE;
    
    return (void*)hFile;
}

static bool F_windows_close_file(void* file_handle)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return false;
    
    return CloseHandle((HANDLE)file_handle) != 0;
}

static int64_t F_windows_read_file(void* file_handle, void* buffer, size_t size)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    DWORD bytes_read = 0;
    if (!ReadFile((HANDLE)file_handle, buffer, (DWORD)size, &bytes_read, NULL))
        return -1;
    
    return (int64_t)bytes_read;
}

static int64_t F_windows_write_file(void* file_handle, const void* buffer, size_t size)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    DWORD bytes_written = 0;
    if (!WriteFile((HANDLE)file_handle, buffer, (DWORD)size, &bytes_written, NULL))
        return -1;
    
    return (int64_t)bytes_written;
}

static int64_t F_windows_seek_file(void* file_handle, int64_t offset, int origin)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    DWORD dwMoveMethod;
    switch (origin)
    {
        case 0: dwMoveMethod = FILE_BEGIN; break;
        case 1: dwMoveMethod = FILE_CURRENT; break;
        case 2: dwMoveMethod = FILE_END; break;
        default: dwMoveMethod = FILE_BEGIN; break;
    }
    
    LARGE_INTEGER liOffset;
    liOffset.QuadPart = offset;
    
    LARGE_INTEGER liNewPosition;
    liNewPosition.QuadPart = 0;
    
    if (!SetFilePointerEx((HANDLE)file_handle, liOffset, &liNewPosition, dwMoveMethod))
        return -1;
    
    return (int64_t)liNewPosition.QuadPart;
}

static int64_t F_windows_get_file_size(void* file_handle)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx((HANDLE)file_handle, &file_size))
        return -1;
    
    return (int64_t)file_size.QuadPart;
}

static bool F_windows_get_file_info(const char* path, Stru_FileInfo_t* info)
{
    if (!path || !info)
        return false;
    
    WIN32_FILE_ATTRIBUTE_DATA file_attr;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &file_attr))
        return false;
    
    // 获取文件名
    const char* filename = strrchr(path, '\\');
    if (filename)
        filename++;
    else
        filename = path;
    
    strncpy(info->filename, filename, CN_MAX_FILENAME_LENGTH - 1);
    info->filename[CN_MAX_FILENAME_LENGTH - 1] = '\0';
    
    // 文件大小
    LARGE_INTEGER file_size;
    file_size.LowPart = file_attr.nFileSizeLow;
    file_size.HighPart = file_attr.nFileSizeHigh;
    info->size = file_size.QuadPart;
    
    // 时间信息
    info->creation_time = F_filetime_to_unix_milliseconds(&file_attr.ftCreationTime);
    info->modification_time = F_filetime_to_unix_milliseconds(&file_attr.ftLastWriteTime);
    info->access_time = F_filetime_to_unix_milliseconds(&file_attr.ftLastAccessTime);
    
    // 文件属性
    info->attributes = F_convert_windows_attributes(file_attr.dwFileAttributes);
    
    return true;
}

static bool F_windows_delete_file(const char* path)
{
    return DeleteFileA(path) != 0;
}

static bool F_windows_rename_file(const char* old_path, const char* new_path)
{
    return MoveFileA(old_path, new_path) != 0;
}

static bool F_windows_copy_file(const char* source_path, const char* dest_path)
{
    return CopyFileA(source_path, dest_path, FALSE) != 0;
}

static bool F_windows_create_directory(const char* path)
{
    return CreateDirectoryA(path, NULL) != 0;
}

static bool F_windows_delete_directory(const char* path)
{
    return RemoveDirectoryA(path) != 0;
}

static void* F_windows_open_directory(const char* path)
{
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE)
        return NULL;
    
    // 分配目录句柄结构
    typedef struct
    {
        HANDLE hFind;
        char path[MAX_PATH];
        WIN32_FIND_DATAA current_data;
        bool first_entry;
    } WindowsDirHandle;
    
    WindowsDirHandle* dir_handle = (WindowsDirHandle*)malloc(sizeof(WindowsDirHandle));
    if (!dir_handle)
    {
        FindClose(hFind);
        return NULL;
    }
    
    dir_handle->hFind = hFind;
    strncpy(dir_handle->path, path, MAX_PATH - 1);
    dir_handle->path[MAX_PATH - 1] = '\0';
    dir_handle->current_data = find_data;
    dir_handle->first_entry = true;
    
    return dir_handle;
}

static bool F_windows_read_directory(void* dir_handle, Stru_DirectoryEntry_t* entry)
{
    if (!dir_handle || !entry)
        return false;
    
    typedef struct
    {
        HANDLE hFind;
        char path[MAX_PATH];
        WIN32_FIND_DATAA current_data;
        bool first_entry;
    } WindowsDirHandle;
    
    WindowsDirHandle* handle = (WindowsDirHandle*)dir_handle;
    
    // 跳过"."和".."目录
    while (1)
    {
        if (!handle->first_entry)
        {
            if (!FindNextFileA(handle->hFind, &handle->current_data))
                return false;
        }
        else
        {
            handle->first_entry = false;
        }
        
        // 跳过"."和".."
        if (strcmp(handle->current_data.cFileName, ".") != 0 &&
            strcmp(handle->current_data.cFileName, "..") != 0)
        {
            break;
        }
    }
    
    // 填充目录条目
    strncpy(entry->name, handle->current_data.cFileName, CN_MAX_FILENAME_LENGTH - 1);
    entry->name[CN_MAX_FILENAME_LENGTH - 1] = '\0';
    
    entry->is_directory = (handle->current_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    
    if (!entry->is_directory)
    {
        LARGE_INTEGER file_size;
        file_size.LowPart = handle->current_data.nFileSizeLow;
        file_size.HighPart = handle->current_data.nFileSizeHigh;
        entry->size = file_size.QuadPart;
    }
    else
    {
        entry->size = 0;
    }
    
    entry->modification_time = F_filetime_to_unix_milliseconds(&handle->current_data.ftLastWriteTime);
    
    return true;
}

static bool F_windows_close_directory(void* dir_handle)
{
    if (!dir_handle)
        return false;
    
    typedef struct
    {
        HANDLE hFind;
        char path[MAX_PATH];
        WIN32_FIND_DATAA current_data;
        bool first_entry;
    } WindowsDirHandle;
    
    WindowsDirHandle* handle = (WindowsDirHandle*)dir_handle;
    
    bool result = FindClose(handle->hFind) != 0;
    free(handle);
    
    return result;
}

static bool F_windows_path_exists(const char* path)
{
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
}

static bool F_windows_is_directory(const char* path)
{
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return false;
    
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static bool F_windows_get_current_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;
    
    DWORD length = GetCurrentDirectoryA((DWORD)size, buffer);
    return length > 0 && length < size;
}

static bool F_windows_set_current_directory(const char* path)
{
    return SetCurrentDirectoryA(path) != 0;
}

static bool F_windows_get_temp_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;
    
    DWORD length = GetTempPathA((DWORD)size, buffer);
    return length > 0 && length < size;
}

// ============================================================================
// 进程管理接口实现
// ============================================================================

static void* F_windows_create_process(const char* command, const char* const* arguments, const char* working_directory)
{
    // 构建命令行
    char cmd_line[4096] = {0};
    strncpy(cmd_line, command, sizeof(cmd_line) - 1);
    
    if (arguments)
    {
        for (int i = 0; arguments[i] != NULL; i++)
        {
            strcat(cmd_line, " ");
            strcat(cmd_line, arguments[i]);
        }
    }
    
    STARTUPINFOA startup_info = {0};
    PROCESS_INFORMATION process_info = {0};
    startup_info.cb = sizeof(startup_info);
    
    BOOL success = CreateProcessA(
        NULL,                   // 应用程序名（使用命令行）
        cmd_line,               // 命令行
        NULL,                   // 进程安全属性
        NULL,                   // 线程安全属性
        FALSE,                  // 句柄继承选项
        0,                      // 创建标志
        NULL,                   // 环境变量
        working_directory,      // 工作目录
        &startup_info,          // STARTUPINFO
        &process_info           // PROCESS_INFORMATION
    );
    
    if (!success)
        return CN_INVALID_PROCESS_HANDLE;
    
    // 关闭不需要的句柄
    CloseHandle(process_info.hThread);
    
    // 分配进程句柄结构
    typedef struct
    {
        HANDLE hProcess;
        DWORD dwProcessId;
    } WindowsProcessHandle;
    
    WindowsProcessHandle* process_handle = (WindowsProcessHandle*)malloc(sizeof(WindowsProcessHandle));
    if (!process_handle)
    {
        CloseHandle(process_info.hProcess);
        return CN_INVALID_PROCESS_HANDLE;
    }
    
    process_handle->hProcess = process_info.hProcess;
    process_handle->dwProcessId = process_info.dwProcessId;
    
    return process_handle;
}

static bool F_windows_wait_process(void* process_handle, uint32_t timeout_ms, CN_ProcessExitStatus* exit_status)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE)
        return false;
    
    typedef struct
    {
        HANDLE hProcess;
        DWORD dwProcessId;
    } WindowsProcessHandle;
    
    WindowsProcessHandle* handle = (WindowsProcessHandle*)process_handle;
    
    DWORD wait_result = WaitForSingleObject(handle->hProcess, timeout_ms);
    if (wait_result == WAIT_TIMEOUT)
        return false;
    if (wait_result == WAIT_FAILED)
        return false;
    
    if (exit_status)
    {
        DWORD exit_code = 0;
        GetExitCodeProcess(handle->hProcess, &exit_code);
        *exit_status = (CN_ProcessExitStatus)exit_code;
    }
    
    return true;
}

static bool F_windows_terminate_process(void* process_handle)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE)
        return false;
    
    typedef struct
    {
        HANDLE hProcess;
        DWORD dwProcessId;
    } WindowsProcessHandle;
    
    WindowsProcessHandle* handle = (WindowsProcessHandle*)process_handle;
    
    return TerminateProcess(handle->hProcess, 1) != 0;
}

static bool F_windows_get_process_info(void* process_handle, Stru_ProcessInfo_t* info)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE || !info)
        return false;
    
    typedef struct
    {
        HANDLE hProcess;
        DWORD dwProcessId;
    } WindowsProcessHandle;
    
    WindowsProcessHandle* handle = (WindowsProcessHandle*)process_handle;
    
    // 获取进程创建时间
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (!GetProcessTimes(handle->hProcess, &creation_time, &exit_time, &kernel_time, &user_time))
        return false;
    
    // 获取内存使用信息
    PROCESS_MEMORY_COUNTERS mem_counters;
    if (!GetProcessMemoryInfo(handle->hProcess, &mem_counters, sizeof(mem_counters)))
        return false;
    
    // 填充进程信息
    info->process_id = handle->dwProcessId;
    info->parent_process_id = 0; // Windows需要额外API获取父进程ID
    info->creation_time = F_filetime_to_unix_milliseconds(&creation_time);
    
    // 计算CPU时间（用户时间+内核时间）
    uint64_t user_ms = F_filetime_to_unix_milliseconds(&user_time);
    uint64_t kernel_ms = F_filetime_to_unix_milliseconds(&kernel_time);
    info->cpu_time = user_ms + kernel_ms;
    
    info->memory_usage = mem_counters.WorkingSetSize;
    
    return true;
}

static bool F_windows_close_process(void* process_handle)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE)
        return false;
    
    typedef struct
    {
        HANDLE hProcess;
        DWORD dwProcessId;
    } WindowsProcessHandle;
    
    WindowsProcessHandle* handle = (WindowsProcessHandle*)process_handle;
    
    bool result = CloseHandle(handle->hProcess) != 0;
    free(handle);
    
    return result;
}

static uint32_t F_windows_get_current_process_id(void)
{
    return GetCurrentProcessId();
}

static uint32_t F_windows_get_current_thread_id(void)
{
    return GetCurrentThreadId();
}

static void F_windows_sleep(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

static bool F_windows_yield(void)
{
    SwitchToThread();
    return true;
}

// ============================================================================
// 时间管理接口实现
// ============================================================================

static uint64_t F_windows_get_current_timestamp(void)
{
    // 获取系统时间（毫秒精度）
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    
    return F_filetime_to_unix_milliseconds(&file_time);
}

static uint64_t F_windows_get_high_resolution_time(void)
{
    // 使用QueryPerformanceCounter获取高精度时间
    LARGE_INTEGER counter, frequency;
    
    if (!QueryPerformanceFrequency(&frequency))
        return 0;
    
    if (!QueryPerformanceCounter(&counter))
        return 0;
    
    // 转换为纳秒
    double seconds = (double)counter.QuadPart / (double)frequency.QuadPart;
    return (uint64_t)(seconds * 1e9);
}

static bool F_windows_get_local_time(uint64_t timestamp, Stru_TimeInfo_t* time_info)
{
    if (!time_info)
        return false;
    
    // 将Unix时间戳转换为Windows FILETIME
    const uint64_t WINDOWS_TICK = 10000000;
    const uint64_t SEC_TO_UNIX_EPOCH = 11644473600LL;
    
    uint64_t windows_ticks = (timestamp / 1000 + SEC_TO_UNIX_EPOCH) * WINDOWS_TICK;
    
    FILETIME file_time;
    file_time.dwLowDateTime = (DWORD)(windows_ticks & 0xFFFFFFFF);
    file_time.dwHighDateTime = (DWORD)(windows_ticks >> 32);
    
    // 转换为本地时间
    FILETIME local_file_time;
    SYSTEMTIME sys_time;
    
    if (!FileTimeToLocalFileTime(&file_time, &local_file_time))
        return false;
    
    if (!FileTimeToSystemTime(&local_file_time, &sys_time))
        return false;
    
    time_info->year = sys_time.wYear;
    time_info->month = sys_time.wMonth;
    time_info->day = sys_time.wDay;
    time_info->hour = sys_time.wHour;
    time_info->minute = sys_time.wMinute;
    time_info->second = sys_time.wSecond;
    time_info->millisecond = sys_time.wMilliseconds;
    
    // 获取时区偏移
    TIME_ZONE_INFORMATION tz_info;
    GetTimeZoneInformation(&tz_info);
    time_info->timezone_offset = -(tz_info.Bias / 60); // 转换为小时
    
    return true;
}

static bool F_windows_get_utc_time(uint64_t timestamp, Stru_TimeInfo_t* time_info)
{
    if (!time_info)
        return false;
    
    // 将Unix时间戳转换为Windows FILETIME
    const uint64_t WINDOWS_TICK = 10000000;
    const uint64_t SEC_TO_UNIX_EPOCH = 11644473600LL;
    
    uint64_t windows_ticks = (timestamp / 1000 + SEC_TO_UNIX_EPOCH) * WINDOWS_TICK;
    
    FILETIME file_time;
    file_time.dwLowDateTime = (DWORD)(windows_ticks & 0xFFFFFFFF);
    file_time.dwHighDateTime = (DWORD)(windows_ticks >> 32);
    
    // 转换为系统时间（UTC）
    SYSTEMTIME sys_time;
    
    if (!FileTimeToSystemTime(&file_time, &sys_time))
        return false;
    
    time_info->year = sys_time.wYear;
    time_info->month = sys_time.wMonth;
    time_info->day = sys_time.wDay;
    time_info->hour = sys_time.wHour;
    time_info->minute = sys_time.wMinute;
    time_info->second = sys_time.wSecond;
    time_info->millisecond = sys_time.wMilliseconds;
    
    // UTC时间，时区偏移为0
    time_info->timezone_offset = 0;
    
    return true;
}

static bool F_windows_format_time(const Stru_TimeInfo_t* time_info, const char* format, char* buffer, size_t size)
{
    if (!time_info || !buffer || size == 0)
        return false;
    
    SYSTEMTIME sys_time;
    sys_time.wYear = (WORD)time_info->year;
    sys_time.wMonth = (WORD)time_info->month;
    sys_time.wDay = (WORD)time_info->day;
    sys_time.wHour = (WORD)time_info->hour;
    sys_time.wMinute = (WORD)time_info->minute;
    sys_time.wSecond = (WORD)time_info->second;
    sys_time.wMilliseconds = (WORD)time_info->millisecond;
    // 时区偏移已在time_info中
    // 注意：Windows API需要本地时间，所以这里使用time_info中的值
    // 我们需要将time_info转换为本地时间
    // 简化实现：假设time_info已经是本地时间
    
    // 如果格式为NULL，使用默认格式
    const char* actual_format = format ? format : "%Y-%m-%d %H:%M:%S";
    
    // 转换格式字符串（简化实现）
    char win_format[256];
    strncpy(win_format, actual_format, sizeof(win_format) - 1);
    win_format[sizeof(win_format) - 1] = '\0';
    
    // 这里简化处理，实际应该进行格式转换
    // 对于生产代码，需要实现完整的格式转换
    
    // 使用GetDateFormat和GetTimeFormat进行格式化
    char date_buffer[256], time_buffer[256];
    
    GetDateFormatA(LOCALE_USER_DEFAULT, 0, &sys_time, "yyyy-MM-dd", date_buffer, sizeof(date_buffer));
    GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &sys_time, "HH:mm:ss", time_buffer, sizeof(time_buffer));
    
    snprintf(buffer, size, "%s %s", date_buffer, time_buffer);
    
    return true;
}

// ============================================================================
// 平台接口实现
// ============================================================================

static bool F_windows_platform_initialize(void)
{
    // Windows平台初始化
    // 可以在这里初始化COM、Winsock等
    return true;
}

static void F_windows_platform_cleanup(void)
{
    // Windows平台清理
    // 可以在这里清理COM、Winsock等
}

static const char* F_windows_get_platform_name(void)
{
    return "Windows";
}

static const char* F_windows_get_platform_version(void)
{
    // 获取Windows版本信息
    OSVERSIONINFOEXA os_info = {0};
    os_info.dwOSVersionInfoSize = sizeof(os_info);
    
    if (!GetVersionExA((OSVERSIONINFOA*)&os_info))
        return "Unknown";
    
    static char version[64];
    snprintf(version, sizeof(version), "%d.%d.%d", 
             os_info.dwMajorVersion, 
             os_info.dwMinorVersion, 
             os_info.dwBuildNumber);
    
    return version;
}

static const char* F_windows_get_cpu_architecture(void)
{
    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);
    
    switch (sys_info.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return "x64";
        case PROCESSOR_ARCHITECTURE_ARM:
            return "ARM";
        case PROCESSOR_ARCHITECTURE_ARM64:
            return "ARM64";
        case PROCESSOR_ARCHITECTURE_INTEL:
            return "x86";
        default:
            return "Unknown";
    }
}

static bool F_windows_get_system_memory_info(uint64_t* total_memory, uint64_t* available_memory)
{
    if (!total_memory || !available_memory)
        return false;
    
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    
    if (!GlobalMemoryStatusEx(&mem_status))
        return false;
    
    *total_memory = mem_status.ullTotalPhys;
    *available_memory = mem_status.ullAvailPhys;
    
    return true;
}

static uint32_t F_windows_get_cpu_core_count(void)
{
    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);
    return sys_info.dwNumberOfProcessors;
}

// ============================================================================
// 接口结构体初始化
// ============================================================================

static Stru_FileSystemInterface_t g_windows_file_system_interface = {
    .open_file = F_windows_open_file,
    .close_file = F_windows_close_file,
    .read_file = F_windows_read_file,
    .write_file = F_windows_write_file,
    .seek_file = F_windows_seek_file,
    .get_file_size = F_windows_get_file_size,
    .get_file_info = F_windows_get_file_info,
    .delete_file = F_windows_delete_file,
    .rename_file = F_windows_rename_file,
    .copy_file = F_windows_copy_file,
    .create_directory = F_windows_create_directory,
    .delete_directory = F_windows_delete_directory,
    .open_directory = F_windows_open_directory,
    .read_directory = F_windows_read_directory,
    .close_directory = F_windows_close_directory,
    .path_exists = F_windows_path_exists,
    .is_directory = F_windows_is_directory,
    .get_current_directory = F_windows_get_current_directory,
    .set_current_directory = F_windows_set_current_directory,
    .get_temp_directory = F_windows_get_temp_directory
};

static Stru_ProcessInterface_t g_windows_process_interface = {
    .create_process = F_windows_create_process,
    .wait_process = F_windows_wait_process,
    .terminate_process = F_windows_terminate_process,
    .get_process_info = F_windows_get_process_info,
    .get_current_process_id = F_windows_get_current_process_id,
    .get_current_thread_id = F_windows_get_current_thread_id,
    .sleep = F_windows_sleep,
    .get_environment_variable = NULL, // 暂未实现
    .set_environment_variable = NULL, // 暂未实现
    .execute_command = NULL // 暂未实现
};

static Stru_TimeInterface_t g_windows_time_interface = {
    .get_current_timestamp = F_windows_get_current_timestamp,
    .get_high_resolution_time = F_windows_get_high_resolution_time,
    .get_local_time = F_windows_get_local_time,
    .get_utc_time = F_windows_get_utc_time,
    .format_time = F_windows_format_time,
    .parse_time = NULL, // 暂未实现
    .get_system_uptime = NULL, // 暂未实现
    .get_cpu_frequency = NULL, // 暂未实现
    .get_timezone_info = NULL // 暂未实现
};

static Stru_PlatformInterface_t g_windows_platform_interface = {
    .initialize = F_windows_platform_initialize,
    .cleanup = F_windows_platform_cleanup,
    .get_platform_name = F_windows_get_platform_name,
    .get_platform_version = F_windows_get_platform_version,
    .get_cpu_architecture = F_windows_get_cpu_architecture,
    .get_system_memory_info = F_windows_get_system_memory_info,
    .get_cpu_core_count = F_windows_get_cpu_core_count,
    .file_system = &g_windows_file_system_interface,
    .process = &g_windows_process_interface,
    .time = &g_windows_time_interface
};

// ============================================================================
// 工厂函数
// ============================================================================

/**
 * @brief 获取Windows平台接口
 * @return Windows平台接口指针
 */
const Stru_PlatformInterface_t* F_get_windows_platform_interface(void)
{
    return &g_windows_platform_interface;
}
