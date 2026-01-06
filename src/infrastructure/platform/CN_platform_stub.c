/**
 * @file CN_platform_stub.c
 * @brief 平台抽象层测试桩实现
 * 
 * 本文件提供了平台抽象层的测试桩实现，用于单元测试。
 * 所有函数都返回默认值或模拟数据，不依赖实际操作系统。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 */

#include "CN_platform_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// 内部状态管理
// ============================================================================

/** @brief 测试桩全局状态 */
typedef struct
{
    bool initialized;
    uint64_t mock_timestamp;
    uint64_t mock_high_res_time;
    uint64_t system_uptime;
    uint64_t total_memory;
    uint64_t available_memory;
    uint32_t cpu_core_count;
    char platform_name[64];
    char platform_version[64];
    char cpu_architecture[32];
} StubGlobalState;

static StubGlobalState g_stub_state = {0};

// ============================================================================
// 文件系统接口实现（测试桩）
// ============================================================================

static void* F_stub_open_file(const char* path, Eum_FileOpenMode mode)
{
    // 模拟文件句柄：简单的递增ID
    static uint64_t next_handle_id = 1;
    uint64_t* handle = (uint64_t*)malloc(sizeof(uint64_t));
    if (handle)
    {
        *handle = next_handle_id++;
        return handle;
    }
    return CN_INVALID_FILE_HANDLE;
}

static bool F_stub_close_file(void* file_handle)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return false;
    
    free(file_handle);
    return true;
}

static int64_t F_stub_read_file(void* file_handle, void* buffer, size_t size)
{
    if (file_handle == CN_INVALID_FILE_HANDLE || !buffer || size == 0)
        return -1;
    
    // 模拟读取：填充零
    memset(buffer, 0, size);
    return (int64_t)size;
}

static int64_t F_stub_write_file(void* file_handle, const void* buffer, size_t size)
{
    if (file_handle == CN_INVALID_FILE_HANDLE || !buffer || size == 0)
        return -1;
    
    // 模拟写入：总是成功
    return (int64_t)size;
}

static int64_t F_stub_seek_file(void* file_handle, int64_t offset, int origin)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    // 模拟寻址：总是返回0
    return 0;
}

static int64_t F_stub_get_file_size(void* file_handle)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    // 模拟文件大小：1KB
    return 1024;
}

static bool F_stub_get_file_info(const char* path, Stru_FileInfo_t* info)
{
    if (!path || !info)
        return false;
    
    // 模拟文件信息
    const char* filename = strrchr(path, '/');
    if (!filename)
        filename = strrchr(path, '\\');
    if (filename)
        filename++;
    else
        filename = path;
    
    strncpy(info->filename, filename, CN_MAX_FILENAME_LENGTH - 1);
    info->filename[CN_MAX_FILENAME_LENGTH - 1] = '\0';
    
    info->size = 1024; // 1KB
    info->creation_time = g_stub_state.mock_timestamp - 3600000; // 1小时前
    info->modification_time = g_stub_state.mock_timestamp - 1800000; // 30分钟前
    info->access_time = g_stub_state.mock_timestamp - 900000; // 15分钟前
    info->attributes = Eum_FILE_ATTR_NORMAL;
    
    return true;
}

static bool F_stub_delete_file(const char* path)
{
    // 模拟删除：总是成功
    return true;
}

static bool F_stub_rename_file(const char* old_path, const char* new_path)
{
    // 模拟重命名：总是成功
    return true;
}

static bool F_stub_copy_file(const char* source_path, const char* dest_path)
{
    // 模拟复制：总是成功
    return true;
}

static bool F_stub_create_directory(const char* path)
{
    // 模拟创建目录：总是成功
    return true;
}

static bool F_stub_delete_directory(const char* path)
{
    // 模拟删除目录：总是成功
    return true;
}

static void* F_stub_open_directory(const char* path)
{
    // 模拟目录句柄：简单的递增ID
    static uint64_t next_dir_handle_id = 1000;
    uint64_t* handle = (uint64_t*)malloc(sizeof(uint64_t));
    if (handle)
    {
        *handle = next_dir_handle_id++;
        return handle;
    }
    return NULL;
}

static bool F_stub_read_directory(void* dir_handle, Stru_DirectoryEntry_t* entry)
{
    if (!dir_handle || !entry)
        return false;
    
    static int entry_count = 0;
    
    if (entry_count >= 3)
        return false; // 模拟只有3个条目
    
    // 模拟目录条目
    const char* names[] = {"file1.txt", "file2.txt", "subdir"};
    bool is_dir[] = {false, false, true};
    uint64_t sizes[] = {1024, 2048, 0};
    
    strncpy(entry->name, names[entry_count], CN_MAX_FILENAME_LENGTH - 1);
    entry->name[CN_MAX_FILENAME_LENGTH - 1] = '\0';
    entry->is_directory = is_dir[entry_count];
    entry->size = sizes[entry_count];
    entry->modification_time = g_stub_state.mock_timestamp - (3 - entry_count) * 600000; // 递减时间
    
    entry_count++;
    if (entry_count >= 3)
        entry_count = 0; // 重置以便下次使用
    
    return true;
}

static bool F_stub_close_directory(void* dir_handle)
{
    if (!dir_handle)
        return false;
    
    free(dir_handle);
    return true;
}

static bool F_stub_path_exists(const char* path)
{
    // 模拟路径存在：总是返回true
    return true;
}

static bool F_stub_is_directory(const char* path)
{
    // 模拟检查目录：如果路径包含"dir"则是目录
    return strstr(path, "dir") != NULL;
}

static bool F_stub_get_current_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;
    
    // 模拟当前目录
    const char* current_dir = "/mock/current/directory";
    strncpy(buffer, current_dir, size - 1);
    buffer[size - 1] = '\0';
    
    return true;
}

static bool F_stub_set_current_directory(const char* path)
{
    // 模拟设置目录：总是成功
    return true;
}

static bool F_stub_get_temp_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;
    
    // 模拟临时目录
    const char* temp_dir = "/mock/temp/directory";
    strncpy(buffer, temp_dir, size - 1);
    buffer[size - 1] = '\0';
    
    return true;
}

// ============================================================================
// 进程管理接口实现（测试桩）
// ============================================================================

static void* F_stub_create_process(const char* command, const char* const* arguments, const char* working_directory)
{
    // 模拟进程句柄：简单的递增ID
    static uint64_t next_process_id = 10000;
    uint64_t* handle = (uint64_t*)malloc(sizeof(uint64_t));
    if (handle)
    {
        *handle = next_process_id++;
        return handle;
    }
    return CN_INVALID_PROCESS_HANDLE;
}

static bool F_stub_wait_process(void* process_handle, uint32_t timeout_ms, CN_ProcessExitStatus* exit_status)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE)
        return false;
    
    // 模拟等待：总是成功
    if (exit_status)
        *exit_status = 0; // 成功退出
    
    return true;
}

static bool F_stub_terminate_process(void* process_handle)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE)
        return false;
    
    // 模拟终止：总是成功
    return true;
}

static bool F_stub_get_process_info(void* process_handle, Stru_ProcessInfo_t* info)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE || !info)
        return false;
    
    // 模拟进程信息
    info->process_id = 12345;
    info->parent_process_id = 1;
    info->creation_time = g_stub_state.mock_timestamp - 5000;
    info->cpu_time = 100; // 100ms
    info->memory_usage = 1024 * 1024; // 1MB
    
    return true;
}

static uint32_t F_stub_get_current_process_id(void)
{
    // 模拟当前进程ID
    return 12345;
}

static uint32_t F_stub_get_current_thread_id(void)
{
    // 模拟当前线程ID
    return 54321;
}

static void F_stub_sleep(uint32_t milliseconds)
{
    // 模拟睡眠：不实际睡眠，只更新时间戳
    g_stub_state.mock_timestamp += milliseconds;
    g_stub_state.mock_high_res_time += milliseconds * 1000000; // 转换为纳秒
}

static bool F_stub_get_environment_variable(const char* name, char* buffer, size_t size)
{
    if (!name || !buffer || size == 0)
        return false;
    
    // 模拟环境变量
    if (strcmp(name, "PATH") == 0)
    {
        strncpy(buffer, "/mock/bin:/mock/usr/bin", size - 1);
        buffer[size - 1] = '\0';
        return true;
    }
    else if (strcmp(name, "HOME") == 0)
    {
        strncpy(buffer, "/mock/home/user", size - 1);
        buffer[size - 1] = '\0';
        return true;
    }
    
    return false;
}

static bool F_stub_set_environment_variable(const char* name, const char* value)
{
    // 模拟设置环境变量：总是成功
    return true;
}

static CN_ProcessExitStatus F_stub_execute_command(const char* command, char* output, size_t output_size)
{
    // 模拟执行命令
    if (output && output_size > 0)
    {
        const char* mock_output = "Mock command execution completed successfully.\n";
        strncpy(output, mock_output, output_size - 1);
        output[output_size - 1] = '\0';
    }
    
    return 0; // 成功退出
}

// ============================================================================
// 时间管理接口实现（测试桩）
// ============================================================================

static uint64_t F_stub_get_current_timestamp(void)
{
    // 返回模拟时间戳（毫秒）
    return g_stub_state.mock_timestamp;
}

static uint64_t F_stub_get_high_resolution_time(void)
{
    // 返回模拟高精度时间（纳秒）
    return g_stub_state.mock_high_res_time;
}

static bool F_stub_get_local_time(uint64_t timestamp, Stru_TimeInfo_t* time_info)
{
    if (!time_info)
        return false;
    
    // 将时间戳转换为本地时间（模拟）
    time_t raw_time = (time_t)(timestamp / 1000);
    struct tm* time_struct = localtime(&raw_time);
    
    if (!time_struct)
        return false;
    
    time_info->year = time_struct->tm_year + 1900;
    time_info->month = time_struct->tm_mon + 1;
    time_info->day = time_struct->tm_mday;
    time_info->hour = time_struct->tm_hour;
    time_info->minute = time_struct->tm_min;
    time_info->second = time_struct->tm_sec;
    time_info->millisecond = timestamp % 1000;
    time_info->timezone_offset = 8; // 模拟东八区
    
    return true;
}

static bool F_stub_get_utc_time(uint64_t timestamp, Stru_TimeInfo_t* time_info)
{
    if (!time_info)
        return false;
    
    // 将时间戳转换为UTC时间（模拟）
    time_t raw_time = (time_t)(timestamp / 1000);
    struct tm* time_struct = gmtime(&raw_time);
    
    if (!time_struct)
        return false;
    
    time_info->year = time_struct->tm_year + 1900;
    time_info->month = time_struct->tm_mon + 1;
    time_info->day = time_struct->tm_mday;
    time_info->hour = time_struct->tm_hour;
    time_info->minute = time_struct->tm_min;
    time_info->second = time_struct->tm_sec;
    time_info->millisecond = timestamp % 1000;
    time_info->timezone_offset = 0; // UTC时区
    
    return true;
}

static bool F_stub_format_time(const Stru_TimeInfo_t* time_info, const char* format, char* buffer, size_t size)
{
    if (!time_info || !format || !buffer || size == 0)
        return false;
    
    // 模拟格式化时间
    snprintf(buffer, size - 1, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             time_info->year, time_info->month, time_info->day,
             time_info->hour, time_info->minute, time_info->second,
             time_info->millisecond);
    buffer[size - 1] = '\0';
    
    return true;
}

static bool F_stub_parse_time(const char* time_string, const char* format, Stru_TimeInfo_t* time_info)
{
    if (!time_string || !format || !time_info)
        return false;
    
    // 模拟解析时间（简单实现）
    // 这里只解析特定格式：YYYY-MM-DD HH:MM:SS
    int year, month, day, hour, minute, second;
    if (sscanf(time_string, "%d-%d-%d %d:%d:%d",
               &year, &month, &day, &hour, &minute, &second) == 6)
    {
        time_info->year = year;
        time_info->month = month;
        time_info->day = day;
        time_info->hour = hour;
        time_info->minute = minute;
        time_info->second = second;
        time_info->millisecond = 0;
        time_info->timezone_offset = 0;
        return true;
    }
    
    return false;
}

static uint64_t F_stub_get_system_uptime(void)
{
    // 返回模拟系统启动时间
    return g_stub_state.system_uptime;
}

static uint64_t F_stub_get_cpu_frequency(void)
{
    // 返回模拟CPU频率：2.5GHz
    return 2500000000ULL;
}

static bool F_stub_get_timezone_info(char* timezone_name, size_t size, int8_t* offset)
{
    if (!timezone_name || size == 0 || !offset)
        return false;
    
    // 模拟时区信息
    strncpy(timezone_name, "Asia/Shanghai", size - 1);
    timezone_name[size - 1] = '\0';
    *offset = 8; // 东八区
    
    return true;
}

// ============================================================================
// 平台抽象层主接口实现（测试桩）
// ============================================================================

static bool F_stub_initialize(void)
{
    if (g_stub_state.initialized)
        return true;
    
    // 初始化模拟状态
    g_stub_state.initialized = true;
    g_stub_state.mock_timestamp = 1700000000000ULL; // 2023-11-14左右
    g_stub_state.mock_high_res_time = g_stub_state.mock_timestamp * 1000000ULL;
    g_stub_state.system_uptime = 86400000ULL; // 1天
    g_stub_state.total_memory = 16ULL * 1024 * 1024 * 1024; // 16GB
    g_stub_state.available_memory = 8ULL * 1024 * 1024 * 1024; // 8GB
    g_stub_state.cpu_core_count = 8;
    
    strcpy(g_stub_state.platform_name, "StubPlatform");
    strcpy(g_stub_state.platform_version, "1.0.0");
    strcpy(g_stub_state.cpu_architecture, "x86_64");
    
    return true;
}

static void F_stub_cleanup(void)
{
    g_stub_state.initialized = false;
}

static const char* F_stub_get_platform_name(void)
{
    return g_stub_state.platform_name;
}

static const char* F_stub_get_platform_version(void)
{
    return g_stub_state.platform_version;
}

static const char* F_stub_get_cpu_architecture(void)
{
    return g_stub_state.cpu_architecture;
}

static bool F_stub_get_system_memory_info(uint64_t* total_memory, uint64_t* available_memory)
{
    if (!total_memory || !available_memory)
        return false;
    
    *total_memory = g_stub_state.total_memory;
    *available_memory = g_stub_state.available_memory;
    
    return true;
}

static uint32_t F_stub_get_cpu_core_count(void)
{
    return g_stub_state.cpu_core_count;
}

static bool F_stub_get_system_info(char* info_buffer, size_t buffer_size)
{
    if (!info_buffer || buffer_size == 0)
        return false;
    
    snprintf(info_buffer, buffer_size - 1,
             "Platform: %s %s\n"
             "CPU Architecture: %s\n"
             "CPU Cores: %u\n"
             "Total Memory: %llu MB\n"
             "Available Memory: %llu MB\n"
             "System Uptime: %llu hours",
             g_stub_state.platform_name,
             g_stub_state.platform_version,
             g_stub_state.cpu_architecture,
             g_stub_state.cpu_core_count,
             g_stub_state.total_memory / (1024 * 1024),
             g_stub_state.available_memory / (1024 * 1024),
             g_stub_state.system_uptime / 3600000);
    
    info_buffer[buffer_size - 1] = '\0';
    return true;
}

// ============================================================================
// 接口结构体定义
// ============================================================================

static const Stru_FileSystemInterface_t g_stub_file_system_interface = {
    .open_file = F_stub_open_file,
    .close_file = F_stub_close_file,
    .read_file = F_stub_read_file,
    .write_file = F_stub_write_file,
    .seek_file = F_stub_seek_file,
    .get_file_size = F_stub_get_file_size,
    .get_file_info = F_stub_get_file_info,
    .delete_file = F_stub_delete_file,
    .rename_file = F_stub_rename_file,
    .copy_file = F_stub_copy_file,
    .create_directory = F_stub_create_directory,
    .delete_directory = F_stub_delete_directory,
    .open_directory = F_stub_open_directory,
    .read_directory = F_stub_read_directory,
    .close_directory = F_stub_close_directory,
    .path_exists = F_stub_path_exists,
    .is_directory = F_stub_is_directory,
    .get_current_directory = F_stub_get_current_directory,
    .set_current_directory = F_stub_set_current_directory,
    .get_temp_directory = F_stub_get_temp_directory
};

static const Stru_ProcessInterface_t g_stub_process_interface = {
    .create_process = F_stub_create_process,
    .wait_process = F_stub_wait_process,
    .terminate_process = F_stub_terminate_process,
    .get_process_info = F_stub_get_process_info,
    .get_current_process_id = F_stub_get_current_process_id,
    .get_current_thread_id = F_stub_get_current_thread_id,
    .sleep = F_stub_sleep,
    .get_environment_variable = F_stub_get_environment_variable,
    .set_environment_variable = F_stub_set_environment_variable,
    .execute_command = F_stub_execute_command
};

static const Stru_TimeInterface_t g_stub_time_interface = {
    .get_current_timestamp = F_stub_get_current_timestamp,
    .get_high_resolution_time = F_stub_get_high_resolution_time,
    .get_local_time = F_stub_get_local_time,
    .get_utc_time = F_stub_get_utc_time,
    .format_time = F_stub_format_time,
    .parse_time = F_stub_parse_time,
    .get_system_uptime = F_stub_get_system_uptime,
    .get_cpu_frequency = F_stub_get_cpu_frequency,
    .get_timezone_info = F_stub_get_timezone_info
};

static const Stru_PlatformInterface_t g_stub_platform_interface = {
    .file_system = &g_stub_file_system_interface,
    .process = &g_stub_process_interface,
    .time = &g_stub_time_interface,
    .initialize = F_stub_initialize,
    .cleanup = F_stub_cleanup,
    .get_platform_name = F_stub_get_platform_name,
    .get_platform_version = F_stub_get_platform_version,
    .get_cpu_architecture = F_stub_get_cpu_architecture,
    .get_system_memory_info = F_stub_get_system_memory_info,
    .get_cpu_core_count = F_stub_get_cpu_core_count,
    .get_system_info = F_stub_get_system_info
};

// ============================================================================
// 工厂函数实现
// ============================================================================

const Stru_PlatformInterface_t* F_get_stub_platform_interface(void)
{
    return &g_stub_platform_interface;
}
