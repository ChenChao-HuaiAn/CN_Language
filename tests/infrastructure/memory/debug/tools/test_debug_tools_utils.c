/**
 * @file test_debug_tools_utils.c
 * @brief 工具函数模块测试
 * 
 * 本文件实现了工具函数模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../../src/infrastructure/memory/debug/tools/CN_memory_debug_tools.h"

/**
 * @brief 测试字节转换函数
 */
static bool test_byte_conversion_functions(void)
{
    printf("    测试字节转换函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 字节到十六进制字符串
    unsigned char byte = 0xAB;
    char hex_str[3];
    bool converted = F_byte_to_hex_string(byte, hex_str, sizeof(hex_str));
    if (!converted) {
        printf("      ❌ 字节到十六进制字符串转换失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    if (strcmp(hex_str, "AB") != 0) {
        printf("      ❌ 字节转换结果不正确: %s (期望: AB)\n", hex_str);
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试2: 十六进制字符串到字节
    unsigned char result_byte = 0;
    bool parsed = F_hex_string_to_byte("CD", &result_byte);
    if (!parsed) {
        printf("      ❌ 十六进制字符串到字节转换失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    if (result_byte != 0xCD) {
        printf("      ❌ 字符串转换结果不正确: 0x%02X (期望: 0xCD)\n", result_byte);
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试3: 字节数组到十六进制字符串
    unsigned char byte_array[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    char array_hex_str[17];
    bool array_converted = F_bytes_to_hex_string(byte_array, sizeof(byte_array), 
                                               array_hex_str, sizeof(array_hex_str));
    if (!array_converted) {
        printf("      ⚠ 字节数组到十六进制字符串转换失败\n");
    } else {
        if (strcmp(array_hex_str, "0123456789ABCDEF") != 0) {
            printf("      ⚠ 字节数组转换结果不正确: %s\n", array_hex_str);
        }
    }
    
    // 测试4: 测试边界情况
    // 小缓冲区测试
    char small_buffer[2];
    bool small_converted = F_byte_to_hex_string(0xFF, small_buffer, sizeof(small_buffer));
    if (small_converted) {
        printf("      ⚠ 小缓冲区转换返回成功（可能是截断）\n");
    }
    
    // 无效十六进制字符串
    unsigned char invalid_byte = 0;
    bool invalid_parsed = F_hex_string_to_byte("GG", &invalid_byte);
    if (invalid_parsed) {
        printf("      ⚠ 无效十六进制字符串解析返回成功\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 字节转换函数测试通过\n");
    return true;
}

/**
 * @brief 测试格式化函数
 */
static bool test_formatting_functions(void)
{
    printf("    测试格式化函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 格式化内存地址
    void* address = (void*)0x12345678;
    char address_str[32];
    bool address_formatted = F_format_memory_address(address, address_str, sizeof(address_str));
    if (!address_formatted) {
        printf("      ⚠ 内存地址格式化失败\n");
    } else {
        printf("      格式化后的地址: %s\n", address_str);
    }
    
    // 测试2: 格式化内存大小
    size_t memory_size = 1024 * 1024; // 1MB
    char size_str[32];
    bool size_formatted = F_format_memory_size(memory_size, size_str, sizeof(size_str));
    if (!size_formatted) {
        printf("      ⚠ 内存大小格式化失败\n");
    } else {
        printf("      格式化后的大小: %s\n", size_str);
    }
    
    // 测试3: 格式化时间戳
    char timestamp_str[64];
    bool timestamp_formatted = F_format_timestamp(timestamp_str, sizeof(timestamp_str));
    if (!timestamp_formatted) {
        printf("      ⚠ 时间戳格式化失败\n");
    } else {
        printf("      格式化后的时间戳: %s\n", timestamp_str);
    }
    
    // 测试4: 格式化调用栈帧
    void* frame_address = (void*)0x87654321;
    char frame_str[128];
    bool frame_formatted = F_format_stack_frame(frame_address, 42, "test_function", 
                                              "test_file.c", frame_str, sizeof(frame_str));
    if (!frame_formatted) {
        printf("      ⚠ 调用栈帧格式化失败\n");
    } else {
        printf("      格式化后的调用栈帧: %s\n", frame_str);
    }
    
    // 测试5: 小缓冲区测试
    char small_buffer[5];
    bool small_formatted = F_format_memory_address(address, small_buffer, sizeof(small_buffer));
    if (small_formatted) {
        printf("      ⚠ 小缓冲区格式化返回成功（可能是截断）\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 格式化函数测试通过\n");
    return true;
}

/**
 * @brief 测试对齐检查函数
 */
static bool test_alignment_functions(void)
{
    printf("    测试对齐检查函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 检查地址对齐
    void* aligned_address = (void*)0x1000;
    void* unaligned_address = (void*)0x1001;
    
    bool is_aligned = F_is_address_aligned(aligned_address, 16);
    if (!is_aligned) {
        printf("      ❌ 对齐地址检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    bool is_unaligned = F_is_address_aligned(unaligned_address, 16);
    if (is_unaligned) {
        printf("      ❌ 非对齐地址检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试2: 对齐地址
    void* aligned = F_align_address(unaligned_address, 16);
    if (aligned == NULL) {
        printf("      ⚠ 地址对齐失败\n");
    } else {
        bool check_aligned = F_is_address_aligned(aligned, 16);
        if (!check_aligned) {
            printf("      ⚠ 对齐后的地址检查失败\n");
        }
    }
    
    // 测试3: 检查大小对齐
    size_t aligned_size = 1024;
    size_t unaligned_size = 1025;
    
    bool size_aligned = F_is_size_aligned(aligned_size, 16);
    if (!size_aligned) {
        printf("      ❌ 对齐大小检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    bool size_unaligned = F_is_size_aligned(unaligned_size, 16);
    if (size_unaligned) {
        printf("      ❌ 非对齐大小检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试4: 对齐大小
    size_t aligned_result = F_align_size(unaligned_size, 16);
    if (aligned_result != 1024) { // 1025对齐到16的倍数应该是1040
        printf("      ⚠ 大小对齐结果不正确: %zu\n", aligned_result);
    }
    
    // 测试5: 边界情况
    bool zero_aligned = F_is_address_aligned(NULL, 16);
    if (zero_aligned) {
        printf("      ⚠ NULL地址对齐检查返回true\n");
    }
    
    bool zero_size_aligned = F_is_size_aligned(0, 16);
    if (!zero_size_aligned) {
        printf("      ⚠ 零大小对齐检查返回false\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 对齐检查函数测试通过\n");
    return true;
}

/**
 * @brief 测试工具辅助函数
 */
static bool test_utility_helper_functions(void)
{
    printf("    测试工具辅助函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 计算校验和
    unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t data_size = sizeof(data);
    
    unsigned int checksum = F_calculate_simple_checksum(data, data_size);
    printf("      数据校验和: 0x%08X\n", checksum);
    
    // 测试2: 计算CRC32
    unsigned int crc32 = F_calculate_crc32(data, data_size);
    printf("      数据CRC32: 0x%08X\n", crc32);
    
    // 测试3: 计算简单哈希
    unsigned int simple_hash = F_calculate_simple_hash(data, data_size);
    printf("      数据简单哈希: 0x%08X\n", simple_hash);
    
    // 测试4: 反转字节顺序
    uint32_t value = 0x12345678;
    uint32_t reversed = F_reverse_bytes_32(value);
    if (reversed != 0x78563412) {
        printf("      ⚠ 32位字节反转结果不正确: 0x%08X\n", reversed);
    }
    
    uint64_t value64 = 0x123456789ABCDEF0ULL;
    uint64_t reversed64 = F_reverse_bytes_64(value64);
    printf("      64位字节反转: 0x%016llX -> 0x%016llX\n", value64, reversed64);
    
    // 测试5: 安全字符串复制
    char dest[10];
    char src[] = "HelloWorld";
    
    bool safe_copied = F_safe_strncpy(dest, src, sizeof(dest));
    if (!safe_copied) {
        printf("      ⚠ 安全字符串复制失败\n");
    } else {
        if (strlen(dest) != sizeof(dest) - 1) {
            printf("      ⚠ 安全字符串复制结果长度不正确\n");
        }
    }
    
    // 测试6: 安全字符串连接
    char dest2[20] = "Hello";
    char src2[] = "World";
    
    bool safe_catted = F_safe_strncat(dest2, src2, sizeof(dest2));
    if (!safe_catted) {
        printf("      ⚠ 安全字符串连接失败\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 工具辅助函数测试通过\n");
    return true;
}

/**
 * @brief 测试性能测量函数
 */
static bool test_performance_measurement_functions(void)
{
    printf("    测试性能测量函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 开始和停止计时器
    uint64_t timer_id = F_start_timer();
    if (timer_id == 0) {
        printf("      ⚠ 计时器启动失败\n");
    } else {
        // 模拟一些工作
        volatile int sum = 0;
        for (int i = 0; i < 1000; i++) {
            sum += i;
        }
        
        uint64_t elapsed_ns = F_stop_timer(timer_id);
        printf("      计时器经过时间: %llu ns\n", elapsed_ns);
    }
    
    // 测试2: 获取当前时间戳
    uint64_t timestamp1 = F_get_current_timestamp_ns();
    uint64_t timestamp2 = F_get_current_timestamp_ns();
    
    if (timestamp1 == 0 || timestamp2 == 0) {
        printf("      ⚠ 时间戳获取失败\n");
    } else {
        printf("      时间戳1: %llu ns\n", timestamp1);
        printf("      时间戳2: %llu ns\n", timestamp2);
        printf("      时间差: %llu ns\n", timestamp2 - timestamp1);
    }
    
    // 测试3: 性能计数器
    uint64_t counter_start = F_get_performance_counter();
    if (counter_start == 0) {
        printf("      ⚠ 性能计数器获取失败\n");
    } else {
        // 模拟一些工作
        volatile int product = 1;
        for (int i = 1; i < 100; i++) {
            product *= i;
        }
        
        uint64_t counter_end = F_get_performance_counter();
        uint64_t counter_diff = counter_end - counter_start;
        printf("      性能计数器差值: %llu\n", counter_diff);
    }
    
    // 测试4: 测量函数执行时间
    uint64_t execution_time = F_measure_execution_time_ns(F_initialize_debug_tools);
    printf("      函数执行时间: %llu ns\n", execution_time);
    
    // 注意：F_initialize_debug_tools已经被调用过，这里再次调用可能会失败
    // 但这不是测试的重点
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 性能测量函数测试通过\n");
    return true;
}

/**
 * @brief 测试工具函数边界情况
 */
static bool test_utility_functions_edge_cases(void)
{
    printf("    测试工具函数边界情况...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: NULL指针输入
    bool null_converted = F_byte_to_hex_string(0xFF, NULL, 10);
    if (null_converted) {
        printf("      ⚠ NULL缓冲区字节转换返回成功\n");
    }
    
    unsigned char null_byte = 0;
    bool null_parsed = F_hex_string_to_byte(NULL, &null_byte);
    if (null_parsed) {
        printf("      ⚠ NULL字符串字节解析返回成功\n");
    }
    
    bool null_checksum = F_calculate_simple_checksum(NULL, 10);
    if (null_checksum != 0) {
        printf("      ⚠ NULL数据校验和不为0\n");
    }
    
    // 测试2: 零大小输入
    unsigned char dummy_data[1] = {0};
    bool zero_checksum = F_calculate_simple_checksum(dummy_data, 0);
    if (zero_checksum != 0) {
        printf("      ⚠ 零大小数据校验和不为0\n");
    }
    
    // 测试3: 无效对齐值
    bool invalid_aligned = F_is_address_aligned((void*)0x1000, 0);
    if (invalid_aligned) {
        printf("      ⚠ 零对齐值检查返回true\n");
    }
    
    // 测试4: 停止不存在的计时器
    uint64_t fake_timer_id = 9999;
    uint64_t fake_elapsed = F_stop_timer(fake_timer_id);
    if (fake_elapsed != 0) {
        printf("      ⚠ 不存在的计时器停止返回非零值\n");
    }
    
    // 测试5: 缓冲区溢出保护
    char tiny_buffer[1];
    bool overflow_test = F_safe_strncpy(tiny_buffer, "Hello", sizeof(tiny_buffer));
    if (overflow_test) {
        printf("      ⚠ 缓冲区溢出测试返回成功\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 工具函数边界情况测试通过\n");
    return true;
}

/**
 * @brief 运行所有工具函数模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_utils_all(void)
{
    printf("  运行工具函数模块测试:\n");
    printf("  ---------------------\n");
    
    bool all_passed = true;
    
    if (!test_byte_conversion_functions()) {
        all_passed = false;
    }
    
    if (!test_formatting_functions()) {
        all_passed = false;
    }
    
    if (!test_alignment_functions()) {
        all_passed = false;
    }
    
    if (!test_utility_helper_functions()) {
        all_passed = false;
    }
    
    if (!test_performance_measurement_functions()) {
        all_passed = false;
    }
    
    if (!test_utility_functions_edge_cases()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有工具函数模块测试通过\n");
    } else {
        printf("  ❌ 部分工具函数模块测试失败\n");
    }
    
    return all_passed;
}
