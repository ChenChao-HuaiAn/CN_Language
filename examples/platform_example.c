/******************************************************************************
 * 文件名：platform_example.c
 * 功能：平台抽象层使用示例
 * 
 * 本示例演示如何使用CN_Language平台抽象层进行跨平台开发。
 * 展示了文件系统、进程管理和时间管理的基本用法。
 * 
 * @author CN_Language架构委员会
 * @date 2026年1月6日
 * @version 1.0.0
 * @license MIT
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/infrastructure/platform/CN_platform_interface.h"

/**
 * @brief 演示文件系统操作
 */
static void demonstrate_filesystem(void)
{
    printf("=== 文件系统操作演示 ===\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->file_system) {
        printf("错误：无法获取文件系统接口\n");
        return;
    }
    
    const Stru_FileSystemInterface_t* fs = platform->file_system;
    
    // 1. 创建并写入文件
    const char* filename = "example_file.txt";
    const char* content = "这是CN_Language平台抽象层的示例文件内容。\n"
                         "This is example file content for CN_Language platform abstraction layer.\n";
    
    printf("1. 创建文件: %s\n", filename);
    void* file = fs->open_file(filename, Eum_FILE_MODE_WRITE);
    if (!file) {
        printf("   错误：无法创建文件\n");
        return;
    }
    
    int64_t written = fs->write_file(file, content, strlen(content));
    printf("   写入 %lld 字节\n", written);
    fs->close_file(file);
    
    // 2. 读取文件
    printf("2. 读取文件内容\n");
    file = fs->open_file(filename, Eum_FILE_MODE_READ);
    if (!file) {
        printf("   错误：无法打开文件进行读取\n");
        return;
    }
    
    char buffer[1024] = {0};
    int64_t read = fs->read_file(file, buffer, sizeof(buffer) - 1);
    printf("   读取 %lld 字节\n", read);
    printf("   内容: %s\n", buffer);
    fs->close_file(file);
    
    // 3. 获取文件信息
    printf("3. 获取文件信息\n");
    Stru_FileInfo_t file_info;
    if (fs->get_file_info(filename, &file_info)) {
        printf("   文件名: %s\n", file_info.filename);
        printf("   文件大小: %llu 字节\n", file_info.size);
        printf("   创建时间: %llu\n", file_info.creation_time);
        printf("   修改时间: %llu\n", file_info.modification_time);
    } else {
        printf("   错误：无法获取文件信息\n");
    }
    
    // 4. 删除文件
    printf("4. 删除文件\n");
    if (fs->delete_file(filename)) {
        printf("   文件删除成功\n");
    } else {
        printf("   错误：无法删除文件\n");
    }
    
    printf("\n");
}

/**
 * @brief 演示时间操作
 */
static void demonstrate_time(void)
{
    printf("=== 时间操作演示 ===\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->time) {
        printf("错误：无法获取时间接口\n");
        return;
    }
    
    const Stru_TimeInterface_t* time = platform->time;
    
    // 1. 获取当前时间戳
    uint64_t timestamp = time->get_current_timestamp();
    printf("1. 当前时间戳: %llu 毫秒\n", timestamp);
    
    // 2. 获取高精度时间
    uint64_t high_res_time = time->get_high_resolution_time();
    printf("2. 高精度时间: %llu 纳秒\n", high_res_time);
    
    // 3. 获取系统启动时间
    uint64_t uptime = time->get_system_uptime();
    printf("3. 系统启动时间: %llu 毫秒\n", uptime);
    
    // 4. 获取本地时间
    Stru_TimeInfo_t local_time;
    if (time->get_local_time(timestamp, &local_time)) {
        printf("4. 本地时间: %04d-%02d-%02d %02d:%02d:%02d.%03d\n",
               local_time.year, local_time.month, local_time.day,
               local_time.hour, local_time.minute, local_time.second,
               local_time.millisecond);
    }
    
    printf("\n");
}

/**
 * @brief 演示进程操作
 */
static void demonstrate_process(void)
{
    printf("=== 进程操作演示 ===\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform || !platform->process) {
        printf("错误：无法获取进程接口\n");
        return;
    }
    
    const Stru_ProcessInterface_t* process = platform->process;
    
    // 1. 获取当前进程ID
    uint32_t pid = process->get_current_process_id();
    printf("1. 当前进程ID: %u\n", pid);
    
    // 2. 获取当前线程ID
    uint32_t tid = process->get_current_thread_id();
    printf("2. 当前线程ID: %u\n", tid);
    
    // 3. 获取环境变量
    char env_buffer[256] = {0};
    if (process->get_environment_variable("PATH", env_buffer, sizeof(env_buffer))) {
        printf("3. PATH环境变量 (前100字符): %.100s...\n", env_buffer);
    } else {
        printf("3. 无法获取PATH环境变量\n");
    }
    
    // 4. 获取系统信息
    uint64_t total_memory = 0, available_memory = 0;
    if (platform->get_system_memory_info(&total_memory, &available_memory)) {
        printf("4. 系统内存: 总共 %.2f GB, 可用 %.2f GB\n",
               total_memory / 1024.0 / 1024.0 / 1024.0,
               available_memory / 1024.0 / 1024.0 / 1024.0);
    }
    
    uint32_t cpu_cores = platform->get_cpu_core_count();
    printf("5. CPU核心数: %u\n", cpu_cores);
    
    printf("\n");
}

/**
 * @brief 演示平台信息
 */
static void demonstrate_platform_info(void)
{
    printf("=== 平台信息演示 ===\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform) {
        printf("错误：无法获取平台接口\n");
        return;
    }
    
    // 获取平台基本信息
    const char* platform_name = platform->get_platform_name();
    const char* platform_version = platform->get_platform_version();
    const char* cpu_arch = platform->get_cpu_architecture();
    
    printf("平台名称: %s\n", platform_name ? platform_name : "未知");
    printf("平台版本: %s\n", platform_version ? platform_version : "未知");
    printf("CPU架构: %s\n", cpu_arch ? cpu_arch : "未知");
    
    // 获取完整的系统信息
    char system_info[1024] = {0};
    if (platform->get_system_info(system_info, sizeof(system_info))) {
        printf("系统信息: %s\n", system_info);
    }
    
    printf("\n");
}

/**
 * @brief 主函数
 */
int main(void)
{
    printf("========================================\n");
    printf("CN_Language 平台抽象层使用示例\n");
    printf("版本: 1.0.0\n");
    printf("日期: 2026年1月6日\n");
    printf("========================================\n\n");
    
    // 初始化平台抽象层
    if (!CN_platform_initialize()) {
        printf("警告：平台抽象层初始化失败，继续运行...\n");
    }
    
    // 演示各个功能
    demonstrate_platform_info();
    demonstrate_filesystem();
    demonstrate_time();
    demonstrate_process();
    
    // 清理平台抽象层
    CN_platform_cleanup();
    
    printf("========================================\n");
    printf("示例程序执行完成\n");
    printf("========================================\n");
    
    return 0;
}
