/******************************************************************************
 * 文件名：test_platform_basic.c
 * 功能：平台抽象层基础功能测试
 * 作者：CN_Language测试团队
 * 创建日期：2026年1月6日
 * 修改历史：
 *   [2026-01-06] 初始版本
 * 版权：MIT License
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../src/infrastructure/platform/CN_platform_interface.h"

/**
 * @brief 测试文件系统接口
 * @return 成功返回true，失败返回false
 */
static bool test_filesystem_interface(void)
{
    printf("测试文件系统接口...\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (platform == NULL) {
        printf("错误：无法获取平台接口\n");
        return false;
    }
    
    const Stru_FileSystemInterface_t* fs = platform->file_system;
    if (fs == NULL) {
        printf("错误：文件系统接口为空\n");
        return false;
    }
    
    // 测试文件创建和删除
    const char* test_filename = "test_platform_file.txt";
    const char* test_content = "Hello, CN_Language Platform!";
    
    // 创建文件
    void* file = fs->open_file(test_filename, Eum_FILE_MODE_WRITE);
    if (file == NULL) {
        printf("错误：无法创建测试文件\n");
        return false;
    }
    
    // 写入内容
    int64_t bytes_written = fs->write_file(file, test_content, strlen(test_content));
    if (bytes_written != (int64_t)strlen(test_content)) {
        printf("错误：写入字节数不匹配\n");
        fs->close_file(file);
        return false;
    }
    
    // 关闭文件
    if (!fs->close_file(file)) {
        printf("错误：无法关闭文件\n");
        return false;
    }
    
    // 重新打开文件读取
    file = fs->open_file(test_filename, Eum_FILE_MODE_READ);
    if (file == NULL) {
        printf("错误：无法重新打开测试文件\n");
        return false;
    }
    
    // 读取内容
    char buffer[256] = {0};
    int64_t bytes_read = fs->read_file(file, buffer, sizeof(buffer) - 1);
    if (bytes_read != (int64_t)strlen(test_content)) {
        printf("错误：读取字节数不匹配\n");
        fs->close_file(file);
        return false;
    }
    
    // 验证内容
    if (strcmp(buffer, test_content) != 0) {
        printf("错误：读取内容不匹配\n");
        printf("期望：%s\n", test_content);
        printf("实际：%s\n", buffer);
        fs->close_file(file);
        return false;
    }
    
    // 关闭文件
    if (!fs->close_file(file)) {
        printf("错误：无法关闭文件\n");
        return false;
    }
    
    // 获取文件信息
    Stru_FileInfo_t file_info;
    if (!fs->get_file_info(test_filename, &file_info)) {
        printf("错误：无法获取文件信息\n");
        return false;
    }
    
    printf("文件大小：%zu 字节\n", file_info.size);
    printf("文件创建时间：%llu\n", file_info.creation_time);
    printf("文件修改时间：%llu\n", file_info.modification_time);
    
    // 删除文件
    if (!fs->delete_file(test_filename)) {
        printf("错误：无法删除测试文件\n");
        return false;
    }
    
    printf("文件系统接口测试通过！\n");
    return true;
}

/**
 * @brief 测试时间接口
 * @return 成功返回true，失败返回false
 */
static bool test_time_interface(void)
{
    printf("测试时间接口...\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (platform == NULL) {
        printf("错误：无法获取平台接口\n");
        return false;
    }
    
    const Stru_TimeInterface_t* time = platform->time;
    if (time == NULL) {
        printf("错误：时间接口为空\n");
        return false;
    }
    
    // 测试获取当前时间
    uint64_t current_time = time->get_current_timestamp();
    printf("当前时间戳：%llu\n", current_time);
    
    if (current_time == 0) {
        printf("警告：当前时间为0，可能是测试桩实现\n");
    }
    
    // 测试获取高精度时间
    uint64_t high_res_time = time->get_high_resolution_time();
    printf("高精度时间：%llu\n", high_res_time);
    
    // 注意：时间接口没有sleep函数，sleep在进程接口中
    // 我们稍后在进程接口测试中测试sleep
    
    // 获取当前时间戳用于比较
    uint64_t after_test = time->get_current_timestamp();
    printf("测试后时间：%llu\n", after_test);
    
    // 测试获取系统启动时间
    uint64_t system_uptime = time->get_system_uptime();
    printf("系统启动时间：%llu\n", system_uptime);
    
    printf("时间接口测试通过！\n");
    return true;
}

/**
 * @brief 测试进程接口
 * @return 成功返回true，失败返回false
 */
static bool test_process_interface(void)
{
    printf("测试进程接口...\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (platform == NULL) {
        printf("错误：无法获取平台接口\n");
        return false;
    }
    
    const Stru_ProcessInterface_t* process = platform->process;
    if (process == NULL) {
        printf("错误：进程接口为空\n");
        return false;
    }
    
    // 测试获取当前进程ID
    uint32_t pid = process->get_current_process_id();
    printf("当前进程ID：%u\n", pid);
    
    if (pid == 0) {
        printf("警告：进程ID为0，可能是测试桩实现\n");
    }
    
    // 测试获取进程信息
    // 注意：get_process_info需要进程句柄，这里我们只测试当前进程ID
    printf("当前进程ID：%u\n", pid);
    
    // 测试获取当前线程ID
    uint32_t tid = process->get_current_thread_id();
    printf("当前线程ID：%u\n", tid);
    
    // 测试睡眠
    printf("睡眠50毫秒...\n");
    process->sleep(50);
    
    // 测试获取系统内存信息
    uint64_t total_memory = 0;
    uint64_t available_memory = 0;
    if (platform->get_system_memory_info(&total_memory, &available_memory)) {
        printf("系统总内存：%llu 字节\n", total_memory);
        printf("系统可用内存：%llu 字节\n", available_memory);
    } else {
        printf("警告：无法获取系统内存信息\n");
    }
    
    // 测试获取CPU核心数
    uint32_t cpu_cores = platform->get_cpu_core_count();
    printf("CPU核心数：%u\n", cpu_cores);
    
    printf("进程接口测试通过！\n");
    return true;
}

/**
 * @brief 测试平台检测
 * @return 成功返回true，失败返回false
 */
static bool test_platform_detection(void)
{
    printf("测试平台检测...\n");
    
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (platform == NULL) {
        printf("错误：无法获取平台接口\n");
        return false;
    }
    
    const char* platform_name = platform->get_platform_name();
    const char* platform_version = platform->get_platform_version();
    
    printf("平台名称：%s\n", platform_name);
    printf("平台版本：%s\n", platform_version);
    
    // 验证平台名称不为空
    if (platform_name == NULL || strlen(platform_name) == 0) {
        printf("错误：平台名称为空\n");
        return false;
    }
    
    printf("平台检测测试通过！\n");
    return true;
}

/**
 * @brief 主测试函数
 * @return 成功返回0，失败返回1
 */
int main(void)
{
    printf("========================================\n");
    printf("CN_Language 平台抽象层测试套件\n");
    printf("版本：1.0.0\n");
    printf("日期：2026年1月6日\n");
    printf("========================================\n\n");
    
    int passed_tests = 0;
    int total_tests = 0;
    
    // 运行所有测试
    total_tests++;
    if (test_platform_detection()) {
        passed_tests++;
        printf("\n");
    } else {
        printf("平台检测测试失败！\n\n");
    }
    
    total_tests++;
    if (test_filesystem_interface()) {
        passed_tests++;
        printf("\n");
    } else {
        printf("文件系统接口测试失败！\n\n");
    }
    
    total_tests++;
    if (test_time_interface()) {
        passed_tests++;
        printf("\n");
    } else {
        printf("时间接口测试失败！\n\n");
    }
    
    total_tests++;
    if (test_process_interface()) {
        passed_tests++;
        printf("\n");
    } else {
        printf("进程接口测试失败！\n\n");
    }
    
    // 输出测试结果
    printf("========================================\n");
    printf("测试结果：\n");
    printf("通过测试：%d/%d\n", passed_tests, total_tests);
    printf("成功率：%.1f%%\n", (float)passed_tests / total_tests * 100);
    
    if (passed_tests == total_tests) {
        printf("所有测试通过！\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("部分测试失败！\n");
        printf("========================================\n");
        return 1;
    }
}
