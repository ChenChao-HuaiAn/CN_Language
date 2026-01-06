/**
 * @file CN_platform_macos.c
 * @brief macOS平台实现
 
 * 本文件实现了macOS平台的具体功能，包括文件系统、进程、时间等接口。
 * macOS是基于Unix的系统，与Linux有很多相似之处，但也有一些macOS特有的API。
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
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <libproc.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 将macOS文件模式转换为CN文件属性
 * @param mode macOS文件模式
 * @return CN文件属性
 */
static uint32_t F_convert_macos_attributes(mode_t mode)
{
    uint32_t cn_attr = 0;
    
    if (S_ISDIR(mode))
        cn_attr |= Eum_FILE_ATTR_DIRECTORY;
    if (!(mode & S_IWUSR))
        cn_attr |= Eum_FILE_ATTR_READONLY;
    if (mode & S_IXUSR)
        cn_attr |= Eum_FILE_ATTR_ARCHIVE;
    
    // macOS扩展属性处理
    // 可以通过xattr API获取更多属性，这里简化处理
    if (mode & S_IFREG)
        cn_attr |= Eum_FILE_ATTR_NORMAL;
    
    return cn_attr;
}

/**
 * @brief 将CN文件打开模式转换为macOS打开标志
 * @param mode CN文件打开模式
 * @return macOS打开标志
 */
static int F_convert_open_mode(Eum_FileOpenMode mode)
{
    switch (mode)
    {
        case Eum_FILE_MODE_READ:
            return O_RDONLY;
        case Eum_FILE_MODE_WRITE:
            return O_WRONLY | O_CREAT | O_TRUNC;
        case Eum_FILE_MODE_APPEND:
            return O_WRONLY | O_CREAT | O_APPEND;
        case Eum_FILE_MODE_READ_WRITE:
            return O_RDWR;
        case Eum_FILE_MODE_CREATE:
            return O_WRONLY | O_CREAT | O_TRUNC;
        default:
            return O_RDONLY;
    }
}

// ============================================================================
// 文件系统接口实现
// ============================================================================

static void* F_macos_open_file(const char* path, Eum_FileOpenMode mode)
{
    int flags = F_convert_open_mode(mode);
    mode_t permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644
    
    int fd = open(path, flags, permissions);
    if (fd == -1)
        return CN_INVALID_FILE_HANDLE;
    
    int* file_handle = (int*)malloc(sizeof(int));
    if (!file_handle)
    {
        close(fd);
        return CN_INVALID_FILE_HANDLE;
    }
    
    *file_handle = fd;
    return file_handle;
}

static bool F_macos_close_file(void* file_handle)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return false;
    
    int* fd = (int*)file_handle;
    bool result = close(*fd) == 0;
    free(fd);
    
    return result;
}

static int64_t F_macos_read_file(void* file_handle, void* buffer, size_t size)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    int* fd = (int*)file_handle;
    ssize_t bytes_read = read(*fd, buffer, size);
    
    return (int64_t)bytes_read;
}

static int64_t F_macos_write_file(void* file_handle, const void* buffer, size_t size)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    int* fd = (int*)file_handle;
    ssize_t bytes_written = write(*fd, buffer, size);
    
    return (int64_t)bytes_written;
}

static int64_t F_macos_seek_file(void* file_handle, int64_t offset, int origin)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    int* fd = (int*)file_handle;
    int whence;
    
    switch (origin)
    {
        case 0: whence = SEEK_SET; break;
        case 1: whence = SEEK_CUR; break;
        case 2: whence = SEEK_END; break;
        default: whence = SEEK_SET; break;
    }
    
    off_t result = lseek(*fd, (off_t)offset, whence);
    return (int64_t)result;
}

static int64_t F_macos_get_file_size(void* file_handle)
{
    if (file_handle == CN_INVALID_FILE_HANDLE)
        return -1;
    
    int* fd = (int*)file_handle;
    struct stat st;
    
    if (fstat(*fd, &st) == -1)
        return -1;
    
    return (int64_t)st.st_size;
}

static bool F_macos_get_file_info(const char* path, Stru_FileInfo_t* info)
{
    if (!path || !info)
        return false;
    
    struct stat st;
    if (stat(path, &st) == -1)
        return false;
    
    // 获取文件名
    const char* filename = strrchr(path, '/');
    if (filename)
        filename++;
    else
        filename = path;
    
    strncpy(info->filename, filename, CN_MAX_FILENAME_LENGTH - 1);
    info->filename[CN_MAX_FILENAME_LENGTH - 1] = '\0';
    
    // 文件大小
    info->size = (uint64_t)st.st_size;
    
    // 时间信息（转换为毫秒）
    info->creation_time = (uint64_t)st.st_birthtimespec.tv_sec * 1000 + 
                         (uint64_t)st.st_birthtimespec.tv_nsec / 1000000;
    info->modification_time = (uint64_t)st.st_mtimespec.tv_sec * 1000 + 
                             (uint64_t)st.st_mtimespec.tv_nsec / 1000000;
    info->access_time = (uint64_t)st.st_atimespec.tv_sec * 1000 + 
                       (uint64_t)st.st_atimespec.tv_nsec / 1000000;
    
    // 文件属性
    info->attributes = F_convert_macos_attributes(st.st_mode);
    
    return true;
}

static bool F_macos_delete_file(const char* path)
{
    return unlink(path) == 0;
}

static bool F_macos_rename_file(const char* old_path, const char* new_path)
{
    return rename(old_path, new_path) == 0;
}

static bool F_macos_copy_file(const char* source_path, const char* dest_path)
{
    // macOS可以使用copyfile API，这里简化使用系统命令
    char command[CN_MAX_PATH_LENGTH * 2 + 10];
    snprintf(command, sizeof(command), "cp \"%s\" \"%s\"", source_path, dest_path);
    
    return system(command) == 0;
}

static bool F_macos_create_directory(const char* path)
{
    return mkdir(path, 0755) == 0;
}

static bool F_macos_delete_directory(const char* path)
{
    return rmdir(path) == 0;
}

static void* F_macos_open_directory(const char* path)
{
    DIR* dir = opendir(path);
    if (!dir)
        return NULL;
    
    return dir;
}

static bool F_macos_read_directory(void* dir_handle, Stru_DirectoryEntry_t* entry)
{
    if (!dir_handle || !entry)
        return false;
    
    DIR* dir = (DIR*)dir_handle;
    struct dirent* dirent = readdir(dir);
    
    if (!dirent)
        return false;
    
    // 跳过"."和".."
    while (dirent && (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0))
    {
        dirent = readdir(dir);
        if (!dirent)
            return false;
    }
    
    // 填充目录条目
    strncpy(entry->name, dirent->d_name, CN_MAX_FILENAME_LENGTH - 1);
    entry->name[CN_MAX_FILENAME_LENGTH - 1] = '\0';
    
    // 判断是否为目录
    entry->is_directory = (dirent->d_type == DT_DIR);
    
    // 如果是文件，获取文件大小
    if (dirent->d_type == DT_REG)
    {
        char full_path[CN_MAX_PATH_LENGTH];
        struct stat st;
        
        // 需要获取完整路径才能得到文件大小
        // 简化处理：这里不实现
        entry->size = 0;
    }
    else
    {
        entry->size = 0;
    }
    
    // 修改时间（简化处理）
    entry->modification_time = 0;
    
    return true;
}

static bool F_macos_close_directory(void* dir_handle)
{
    if (!dir_handle)
        return false;
    
    DIR* dir = (DIR*)dir_handle;
    return closedir(dir) == 0;
}

static bool F_macos_path_exists(const char* path)
{
    return access(path, F_OK) == 0;
}

static bool F_macos_is_directory(const char* path)
{
    struct stat st;
    if (stat(path, &st) == -1)
        return false;
    
    return S_ISDIR(st.st_mode);
}

static bool F_macos_get_current_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;
    
    char* cwd = getcwd(buffer, size);
    return cwd != NULL;
}

static bool F_macos_set_current_directory(const char* path)
{
    return chdir(path) == 0;
}

static bool F_macos_get_temp_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;
    
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";
    
    strncpy(buffer, tmpdir, size - 1);
    buffer[size - 1] = '\0';
    
    return true;
}

// ============================================================================
// 进程管理接口实现
// ============================================================================

static void* F_macos_create_process(const char* command, const char* const* arguments, const char* working_directory)
{
    // macOS使用fork和exec，与Linux类似
    pid_t pid = fork();
    
    if (pid == -1)
        return CN_INVALID_PROCESS_HANDLE;
    
    if (pid == 0) // 子进程
    {
        if (working_directory && chdir(working_directory) == -1)
        {
            exit(EXIT_FAILURE);
        }
        
        // 构建参数数组
        char** argv = NULL;
        int argc = 0;
        
        // 计算参数数量
        if (arguments)
        {
            while (arguments[argc] != NULL)
                argc++;
        }
        
        // 分配参数数组（命令 + 参数 + NULL）
        argv = (char**)malloc((argc + 2) * sizeof(char*));
        if (!argv)
            exit(EXIT_FAILURE);
        
        argv[0] = (char*)command;
        for (int i = 0; i < argc; i++)
            argv[i + 1] = (char*)arguments[i];
        argv[argc + 1] = NULL;
        
        // 执行命令
        execvp(command, argv);
        
        // 如果execvp失败
        free(argv);
        exit(EXIT_FAILURE);
    }
    
    // 父进程：分配进程句柄
    pid_t* process_handle = (pid_t*)malloc(sizeof(pid_t));
    if (!process_handle)
        return CN_INVALID_PROCESS_HANDLE;
    
    *process_handle = pid;
    return process_handle;
}

static bool F_macos_wait_process(void* process_handle, uint32_t timeout_ms, CN_ProcessExitStatus* exit_status)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE)
        return false;
    
    pid_t* pid = (pid_t*)process_handle;
    int status;
    
    // 简化实现：不支持超时
    if (timeout_ms > 0)
    {
        // 这里应该使用waitpid的非阻塞模式，但简化处理
        return false;
    }
    
    pid_t result = waitpid(*pid, &status, 0);
    if (result == -1)
        return false;
    
    if (exit_status)
    {
        if (WIFEXITED(status))
            *exit_status = WEXITSTATUS(status);
        else
            *exit_status = -1;
    }
    
    return true;
}

static bool F_macos_terminate_process(void* process_handle)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE)
        return false;
    
    pid_t* pid = (pid_t*)process_handle;
    return kill(*pid, SIGTERM) == 0;
}

static bool F_macos_get_process_info(void* process_handle, Stru_ProcessInfo_t* info)
{
    if (process_handle == CN_INVALID_PROCESS_HANDLE || !info)
        return false;
    
    pid_t* pid = (pid_t*)process_handle;
    
    // 使用macOS特有的proc_pidinfo API获取进程信息
    struct proc_bsdinfo proc_info;
    int ret = proc_pidinfo(*pid, PROC_PIDTBSDINFO, 0, &proc_info, sizeof(proc_info));
    
    if (ret <= 0)
        return false;
    
    info->process_id = (uint32_t)*pid;
    info->parent_process_id = proc_info.pbi_ppid;
    info->creation_time = proc_info.pbi_start_tvsec * 1000; // 转换为毫秒
    
    // 获取CPU时间和内存使用
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
        info->cpu_time = (uint64_t)usage.ru_utime.tv_sec * 1000 + 
                        (uint64_t)usage.ru_utime.tv_usec / 1000;
    }
    else
    {
        info->cpu_time = 0;
    }
    
    // 内存使用（简化处理）
    info->memory_usage = 0;
    
    return true;
}

static uint32_t F_macos_get_current_process_id(void)
{
    return (uint32_t)getpid();
}

static uint32_t F_macos_get_current_thread_id(void)
{
    // macOS使用pthread_self，但返回的是pthread_t类型
    // 这里简化处理，转换为uint32_t
    pthread_t thread = pthread_self();
    uint64_t thread_id;
    pthread_threadid_np(thread, &thread_id);
    return (uint32_t)thread_id;
}

static void F_macos_sleep(uint32_t milliseconds)
{
    usleep(milliseconds * 1000);
}

static bool F_macos_get_environment_variable(const char* name, char* buffer, size_t size)
{
    if (!name || !buffer || size == 0)
        return false;
    
    const char* value = getenv(name);
    if (!value)
        return false;
    
    strncpy(buffer, value, size - 1);
    buffer[size - 1] = '\0';
    
    return true;
}

static bool F_macos_set_environment_variable(const char* name, const char* value)
{
    if (!name)
        return false;
    
    if (value)
        return setenv(name, value, 1) == 0;
    else
        return unsetenv(name) == 0;
}

static CN_ProcessExitStatus F_macos_execute_command(const char* command, char* output, size_t output_size)
{
    // 使用popen执行命令
    FILE* fp = popen(command, "r");
    if (!fp)
        return -1;
    
    if (output && output_size > 0)
    {
        size_t bytes_read = fread(output, 1, output_size - 1, fp);
        output[bytes_read] = '\0';
    }
    
    int status = pclose(fp);
    return WEXITSTATUS(status);
}

// ============================================================================
// 时间管理接口实现
// ============================================================================

static uint64_t F_macos_get_current_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

static uint64_t F_macos_get_high_resolution_time(void)
{
    // macOS使用mach_absolute_time获取高精度时间
    static mach_timebase_info_data_t timebase_info = {0};
    
    if (timebase_info.denom == 0)
        mach_timebase_info(&timebase_info);
    
    uint64_t mach_time = mach_absolute_time();
    uint64_t nanoseconds = mach_time * timebase_info.numer / timebase_info.denom;
    
    return nanoseconds;
}

static bool F_macos_get_local_time(uint64_t timestamp, Stru_TimeInfo_t* time_info)
{
    if (!time_info)
        return false;
    
    time_t t = (time_t)(timestamp / 1000);
    struct tm* tm_info = localtime(&t);
    
    if (!tm_info)
        return false;
    
    time_info->year = (uint16_t)(tm_info->tm_year + 1900);
    time_info->month = (uint8_t)(tm_info->tm_mon + 1);
    time_info->day = (uint8_t)tm_info->tm_mday;
    time_info->hour = (uint8_t)tm_info->tm_hour;
    time_info->minute = (uint8_t)tm_info->tm_min;
    time_info->second = (uint8_t)tm_info->tm_sec;
    time_info->millisecond = (uint16_t)(timestamp % 1000);
    
    // 时区偏移
    time_info->timezone_offset = (int8_t)(tm_info->tm_gmtoff / 3600);
    
    return true;
}

static bool F_macos_get_utc_time(uint64_t timestamp, Stru_TimeInfo_t* time_info)
{
    if (!time_info)
        return false;
    
    time_t t = (time_t)(timestamp / 1000);
    struct tm* tm_info = gmtime(&t);
    
    if (!tm_info)
        return false;
    
    time_info->year = (uint16_t)(tm_info->tm_year + 1900);
    time_info->month = (uint8_t)(tm_info->tm_mon + 1);
    time_info->day = (uint8_t)tm_info->tm_mday;
    time_info->hour = (uint8_t)tm_info->tm_hour;
    time_info->minute = (uint8_t)tm_info->tm_min;
    time_info->second = (uint8_t)tm_info->tm_sec;
    time_info->millisecond = (uint16_t)(timestamp % 1000);
    
    // UTC时间，时区偏移为0
    time_info->timezone_offset = 0;
    
    return true;
}

static bool F_macos_format_time(const Stru_TimeInfo_t* time_info, const char* format, char* buffer, size_t size)
{
    if (!time_info || !buffer || size == 0)
        return false;
    
    struct tm tm_info;
    tm_info.tm_year = time_info->year - 1900;
    tm_info.tm_mon = time_info->month - 1;
    tm_info.tm_mday = time_info->day;
    tm_info.tm_hour = time_info->hour;
    tm_info.tm_min = time_info->minute;
    tm_info.tm_sec = time_info->second;
    tm_info.tm_isdst = -1; // 自动判断夏令时
    
    // 如果格式为NULL，使用默认格式
    const char* actual_format = format ? format : "%Y-%m-%d %H:%M:%S";
    
    size_t result = strftime(buffer, size, actual_format, &tm_info);
    return result > 0;
}

static bool F_macos_parse_time(const char* time_string, const char* format, Stru_TimeInfo_t* time_info)
{
    if (!time_string || !format || !time_info)
        return false;
    
    // 简化实现：这里不实现完整的解析
    // 对于生产代码，需要使用strptime等函数
    return false;
}

static uint64_t F_macos_get_system_uptime(void)
{
    // macOS使用sysctl获取系统启动时间
    struct timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) == -1)
        return 0;
    
    struct timeval now;
    gettimeofday(&now, NULL);
    
    uint64_t uptime_seconds = now.tv_sec - boottime.tv_sec;
    return uptime_seconds * 1000; // 转换为毫秒
}

static uint64_t F_macos_get_cpu_frequency(void)
{
    // 使用sysctl获取CPU频率
    uint64_t freq = 0;
    size_t len = sizeof(freq);
    
    if (sysctlbyname("hw.cpufrequency", &freq, &len, NULL, 0) == -1)
        return 0;
    
    return freq;
}

static bool F_macos_get_timezone_info(char* timezone_name, size_t size, int8_t* offset)
{
    if (!timezone_name || !offset)
        return false;
    
    // 获取时区信息
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    
    if (!tm_info)
        return false;
    
    // 时区偏移
    *offset = (int8_t)(tm_info->tm_gmtoff / 3600);
    
    // 时区名称
    const char* tz = getenv("TZ");
    if (tz)
    {
        strncpy(timezone_name, tz, size - 1);
        timezone_name[size - 1] = '\0';
    }
    else
    {
        strncpy(timezone_name, "Unknown", size - 1);
        timezone_name[size - 1] = '\0';
    }
    
    return true;
}

// ============================================================================
// 平台接口实现
// ============================================================================

static bool F_macos_platform_initialize(void)
{
    // macOS平台初始化
    return true;
}

static void F_macos_platform_cleanup(void)
{
    // macOS平台清理
}

static const char* F_macos_get_platform_name(void)
{
    return "macOS";
}

static const char* F_macos_get_platform_version(void)
{
    // 获取macOS版本信息
    static char version[64] = "Unknown";
    
    char os_version[256];
    size_t len = sizeof(os_version);
    
    if (sysctlbyname("kern.osrelease", os_version, &len, NULL, 0) == 0)
    {
        snprintf(version, sizeof(version), "macOS %s", os_version);
    }
    
    return version;
}

static const char* F_macos_get_cpu_architecture(void)
{
    // 获取CPU架构
    static char arch[32] = "Unknown";
    
    #if defined(__x86_64__) || defined(_M_X64)
        strcpy(arch, "x64");
    #elif defined(__i386__) || defined(_M_IX86)
        strcpy(arch, "x86");
    #elif defined(__aarch64__) || defined(_M_ARM64)
        strcpy(arch, "ARM64");
    #elif defined(__arm__) || defined(_M_ARM)
        strcpy(arch, "ARM");
    #else
        strcpy(arch, "Unknown");
    #endif
    
    return arch;
}

static bool F_macos_get_system_memory_info(uint64_t* total_memory, uint64_t* available_memory)
{
    if (!total_memory || !available_memory)
        return false;
    
    // 使用macOS特有的API获取内存信息
    mach_port_t host_port = mach_host_self();
    mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    vm_size_t page_size;
    vm_statistics_data_t vm_stat;
    
    host_page_size(host_port, &page_size);
    
    if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS)
        return false;
    
    // 计算总内存和可用内存
    int64_t mem_used = (int64_t)(vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * (int64_t)page_size;
    int64_t mem_free = (int64_t)vm_stat.free_count * (int64_t)page_size;
    
    // 获取总物理内存
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    size_t length = sizeof(*total_memory);
    sysctl(mib, 2, total_memory, &length, NULL, 0);
    
    *available_memory = mem_free;
    
    return true;
}

static uint32_t F_macos_get_cpu_core_count(void)
{
    int core_count = 0;
    size_t len = sizeof(core_count);
    
    if (sysctlbyname("hw.ncpu", &core_count, &len, NULL, 0) == -1)
        return 1;
    
    return (uint32_t)core_count;
}

static bool F_macos_get_system_info(char* info_buffer, size_t buffer_size)
{
    if (!info_buffer || buffer_size == 0)
        return false;
    
    char model[256] = "Unknown";
    size_t len = sizeof(model);
    
    // 获取机器型号
    if (sysctlbyname("hw.model", model, &len, NULL, 0) == -1)
        strcpy(model, "Unknown");
    
    // 获取CPU核心数
    uint32_t cores = F_macos_get_cpu_core_count();
    
    // 获取内存信息
    uint64_t total_memory = 0;
    uint64_t available_memory = 0;
    F_macos_get_system_memory_info(&total_memory, &available_memory);
    
    // 格式化系统信息
    snprintf(info_buffer, buffer_size,
             "Model: %s\n"
             "CPU Cores: %u\n"
             "Total Memory: %.2f GB\n"
             "Available Memory: %.2f GB",
             model,
             cores,
             (double)total_memory / (1024 * 1024 * 1024),
             (double)available_memory / (1024 * 1024 * 1024));
    
    return true;
}

// ============================================================================
// 接口结构体初始化
// ============================================================================

static Stru_FileSystemInterface_t g_macos_file_system_interface = {
    .open_file = F_macos_open_file,
    .close_file = F_macos_close_file,
    .read_file = F_macos_read_file,
    .write_file = F_macos_write_file,
    .seek_file = F_macos_seek_file,
    .get_file_size = F_macos_get_file_size,
    .get_file_info = F_macos_get_file_info,
    .delete_file = F_macos_delete_file,
    .rename_file = F_macos_rename_file,
    .copy_file = F_macos_copy_file,
    .create_directory = F_macos_create_directory,
    .delete_directory = F_macos_delete_directory,
    .open_directory = F_macos_open_directory,
    .read_directory = F_macos_read_directory,
    .close_directory = F_macos_close_directory,
    .path_exists = F_macos_path_exists,
    .is_directory = F_macos_is_directory,
    .get_current_directory = F_macos_get_current_directory,
    .set_current_directory = F_macos_set_current_directory,
    .get_temp_directory = F_macos_get_temp_directory
};

static Stru_ProcessInterface_t g_macos_process_interface = {
    .create_process = F_macos_create_process,
    .wait_process = F_macos_wait_process,
    .terminate_process = F_macos_terminate_process,
    .get_process_info = F_macos_get_process_info,
    .get_current_process_id = F_macos_get_current_process_id,
    .get_current_thread_id = F_macos_get_current_thread_id,
    .sleep = F_macos_sleep,
    .get_environment_variable = F_macos_get_environment_variable,
    .set_environment_variable = F_macos_set_environment_variable,
    .execute_command = F_macos_execute_command
};

static Stru_TimeInterface_t g_macos_time_interface = {
    .get_current_timestamp = F_macos_get_current_timestamp,
    .get_high_resolution_time = F_macos_get_high_resolution_time,
    .get_local_time = F_macos_get_local_time,
    .get_utc_time = F_macos_get_utc_time,
    .format_time = F_macos_format_time,
    .parse_time = F_macos_parse_time,
    .get_system_uptime = F_macos_get_system_uptime,
    .get_cpu_frequency = F_macos_get_cpu_frequency,
    .get_timezone_info = F_macos_get_timezone_info
};

static Stru_PlatformInterface_t g_macos_platform_interface = {
    .initialize = F_macos_platform_initialize,
    .cleanup = F_macos_platform_cleanup,
    .get_platform_name = F_macos_get_platform_name,
    .get_platform_version = F_macos_get_platform_version,
    .get_cpu_architecture = F_macos_get_cpu_architecture,
    .get_system_memory_info = F_macos_get_system_memory_info,
    .get_cpu_core_count = F_macos_get_cpu_core_count,
    .get_system_info = F_macos_get_system_info,
    .file_system = &g_macos_file_system_interface,
    .process = &g_macos_process_interface,
    .time = &g_macos_time_interface
};

// ============================================================================
// 工厂函数
// ============================================================================

/**
 * @brief 获取macOS平台接口
 * @return macOS平台接口指针
 */
const Stru_PlatformInterface_t* F_get_macos_platform_interface(void)
{
    return &g_macos_platform_interface;
}
