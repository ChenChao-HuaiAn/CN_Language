# 工具函数模块

## 概述

工具函数模块提供了内存调试工具所需的各种辅助函数和工具函数。这些函数包括字节转换、字符检查、内存信息格式化、地址格式化和对齐检查等，为其他调试工具模块提供基础支持。

## 功能特性

### 1. 字节和字符处理
- **字节转十六进制**: 将字节转换为十六进制字符串
- **可打印字符检查**: 检查字符是否为可打印ASCII字符
- **字符转义**: 处理特殊字符的显示

### 2. 内存信息格式化
- **内存信息字符串**: 生成格式化的内存信息描述
- **地址格式化**: 将内存地址格式化为标准字符串
- **大小格式化**: 将字节大小格式化为易读的字符串

### 3. 对齐和边界检查
- **内存对齐检查**: 检查内存地址是否符合对齐要求
- **边界验证**: 验证内存访问是否在有效边界内
- **页面对齐**: 计算页面对齐的地址和大小

### 4. 工具函数
- **安全字符串操作**: 提供安全的字符串格式化函数
- **缓冲区管理**: 辅助缓冲区操作和管理
- **错误处理工具**: 辅助错误处理和报告

## 接口说明

### 主要函数

#### 1. 字节和字符处理
```c
/**
 * @brief 将单个字节转换为两位十六进制字符串
 * @param byte 要转换的字节
 * @param buffer 输出缓冲区（至少3字节，包含终止空字符）
 * @note 输出格式为两位十六进制数字，小写字母
 */
void F_byte_to_hex(uint8_t byte, char* buffer);

/**
 * @brief 检查字符是否为可打印ASCII字符
 * @param ch 要检查的字符
 * @return 可打印返回true，否则返回false
 * @note 可打印字符范围：0x20-0x7E（空格到波浪号）
 */
bool F_is_printable_char(char ch);
```

#### 2. 内存信息格式化
```c
/**
 * @brief 获取内存信息的格式化字符串
 * @param address 内存地址
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止空字符）
 * @note 格式："Address: 0xXXXX, Size: YYYY bytes"
 */
size_t F_get_memory_info_string(const void* address,
                               char* buffer, size_t buffer_size);

/**
 * @brief 格式化内存地址为字符串
 * @param address 内存地址
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止空字符）
 * @note 格式："0xXXXXXXXX"（32位）或"0xXXXXXXXXXXXXXXXX"（64位）
 */
size_t F_format_memory_address(const void* address,
                              char* buffer, size_t buffer_size);
```

#### 3. 对齐检查
```c
/**
 * @brief 检查内存地址是否符合对齐要求
 * @param address 内存地址
 * @param alignment 对齐要求（必须是2的幂）
 * @return 对齐返回true，否则返回false
 * @note 对齐要求示例：4（4字节对齐）、8（8字节对齐）、16（16字节对齐）
 */
bool F_check_memory_alignment(const void* address, size_t alignment);
```

## 使用示例

### 基本使用
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <stdint.h>

int main() {
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        return 1;
    }
    
    // 字节转十六进制示例
    uint8_t test_byte = 0xAB;
    char hex_buffer[3];
    F_byte_to_hex(test_byte, hex_buffer);
    printf("Byte 0x%02X in hex: %s\n", test_byte, hex_buffer);
    
    // 可打印字符检查
    char test_chars[] = {'A', '\n', '\t', 'z', 0x1F, ' '};
    printf("Printable character check:\n");
    for (int i = 0; i < sizeof(test_chars); i++) {
        bool printable = F_is_printable_char(test_chars[i]);
        printf("  '%c' (0x%02X): %s\n", 
               test_chars[i] >= ' ' ? test_chars[i] : '.',
               (uint8_t)test_chars[i],
               printable ? "printable" : "non-printable");
    }
    
    // 内存信息字符串
    int test_array[100];
    char info_buffer[256];
    size_t info_len = F_get_memory_info_string(test_array, info_buffer, sizeof(info_buffer));
    printf("\nMemory info: %s\n", info_buffer);
    
    // 地址格式化
    char addr_buffer[32];
    size_t addr_len = F_format_memory_address(test_array, addr_buffer, sizeof(addr_buffer));
    printf("Formatted address: %s\n", addr_buffer);
    
    // 对齐检查
    void* aligned_ptr = (void*)((uintptr_t)test_array & ~0xF);  // 16字节对齐
    void* unaligned_ptr = (void*)((uintptr_t)test_array + 1);
    
    printf("\nAlignment checks:\n");
    printf("  %p is 4-byte aligned: %s\n", aligned_ptr, 
           F_check_memory_alignment(aligned_ptr, 4) ? "YES" : "NO");
    printf("  %p is 8-byte aligned: %s\n", aligned_ptr,
           F_check_memory_alignment(aligned_ptr, 8) ? "YES" : "NO");
    printf("  %p is 16-byte aligned: %s\n", aligned_ptr,
           F_check_memory_alignment(aligned_ptr, 16) ? "YES" : "NO");
    printf("  %p is 4-byte aligned: %s\n", unaligned_ptr,
           F_check_memory_alignment(unaligned_ptr, 4) ? "YES" : "NO");
    
    // 清理
    F_cleanup_debug_tools();
    
    return 0;
}
```

### 集成到内存转储函数
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <string.h>

// 自定义内存转储函数，使用工具函数
void custom_memory_dump(const void* address, size_t size) {
    const uint8_t* ptr = (const uint8_t*)address;
    char line_buffer[256];
    char hex_buffer[3];
    char ascii_buffer[17];
    size_t ascii_pos = 0;
    
    // 格式化地址信息
    char addr_info[64];
    F_format_memory_address(address, addr_info, sizeof(addr_info));
    printf("Memory dump at %s, size: %zu bytes\n", addr_info, size);
    printf("Offset   Hexadecimal                             ASCII\n");
    printf("------   ------------------------------------    ----------------\n");
    
    for (size_t i = 0; i < size; i++) {
        // 每16字节换行
        if (i % 16 == 0) {
            // 输出上一行的ASCII部分
            if (i > 0) {
                ascii_buffer[ascii_pos] = '\0';
                printf("   %s\n", ascii_buffer);
                ascii_pos = 0;
            }
            
            // 输出偏移量
            printf("%06zX   ", i);
        }
        
        // 输出十六进制
        F_byte_to_hex(ptr[i], hex_buffer);
        printf("%s ", hex_buffer);
        
        // 在8字节后添加额外空格
        if (i % 8 == 7) {
            printf(" ");
        }
        
        // 构建ASCII行
        if (F_is_printable_char(ptr[i])) {
            ascii_buffer[ascii_pos] = ptr[i];
        } else {
            ascii_buffer[ascii_pos] = '.';
        }
        ascii_pos++;
    }
    
    // 输出最后一行
    if (ascii_pos > 0) {
        // 填充剩余的十六进制位置
        size_t remaining = 16 - ascii_pos;
        for (size_t i = 0; i < remaining; i++) {
            printf("   ");
            if ((ascii_pos + i) % 8 == 7) {
                printf(" ");
            }
        }
        
        ascii_buffer[ascii_pos] = '\0';
        printf("   %s\n", ascii_buffer);
    }
}
```

### 内存调试辅助工具
```c
#include "CN_memory_debug_tools.h"
#include <stdlib.h>
#include <string.h>

// 内存分配验证工具
void* debug_alloc_aligned(size_t size, size_t alignment) {
    // 验证对齐要求是2的幂
    if ((alignment & (alignment - 1)) != 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg),
                 "Invalid alignment: %zu (must be power of 2)", alignment);
        F_debug_output("%s", error_msg);
        return NULL;
    }
    
    // 分配额外空间用于对齐和存储原始指针
    size_t total_size = size + alignment + sizeof(void*);
    void* raw_ptr = malloc(total_size);
    if (raw_ptr == NULL) {
        F_debug_output("Memory allocation failed: size=%zu, alignment=%zu", 
                      size, alignment);
        return NULL;
    }
    
    // 计算对齐后的地址
    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + sizeof(void*) + alignment - 1) & ~(alignment - 1);
    void* aligned_ptr = (void*)aligned_addr;
    
    // 在对齐地址前存储原始指针
    void** stored_ptr = (void**)(aligned_addr - sizeof(void*));
    *stored_ptr = raw_ptr;
    
    // 验证对齐
    if (!F_check_memory_alignment(aligned_ptr, alignment)) {
        F_debug_output("Alignment failed: requested %zu, got misaligned pointer", 
                      alignment);
        free(raw_ptr);
        return NULL;
    }
    
    // 记录分配信息
    char info_buffer[256];
    size_t info_len = F_get_memory_info_string(aligned_ptr, info_buffer, sizeof(info_buffer));
    F_debug_output("Aligned allocation: %s, alignment=%zu", info_buffer, alignment);
    
    return aligned_ptr;
}

// 对齐内存释放
void debug_free_aligned(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    // 获取存储的原始指针
    void** stored_ptr = (void**)((uintptr_t)ptr - sizeof(void*));
    void* raw_ptr = *stored_ptr;
    
    // 验证指针有效性（简单检查）
    if (raw_ptr == NULL || (uintptr_t)raw_ptr > (uintptr_t)ptr) {
        F_debug_output("Invalid aligned pointer: possible corruption");
        return;
    }
    
    // 记录释放信息
    char info_buffer[256];
    size_t info_len = F_get_memory_info_string(ptr, info_buffer, sizeof(info_buffer));
    F_debug_output("Aligned free: %s", info_buffer);
    
    free(raw_ptr);
}
```

## 实现细节

### 1. 字节转换算法
1. **十六进制转换**: 使用查表法或位操作实现高效转换
2. **字符分类**: 使用范围检查判断可打印字符
3. **缓冲区安全**: 确保输出缓冲区足够并正确终止

### 2. 格式化函数实现
1. **地址格式化**: 根据平台指针大小选择合适格式
2. **信息组合**: 组合多个信息字段为完整字符串
3. **缓冲区管理**: 处理缓冲区不足的安全截断

### 3. 对齐检查优化
1. **幂次验证**: 快速检查对齐值是否为2的幂
2. **位操作检查**: 使用位掩码检查对齐
3. **平台适配**: 处理不同平台的地址表示

### 4. 性能考虑
- 工具函数设计为轻量级和高效
- 避免不必要的内存分配
- 使用内联函数优化频繁调用

## 性能考虑

### 1. 函数开销
- 字节转换函数非常轻量（查表或位操作）
- 格式化函数有字符串处理开销
- 对齐检查是简单的位操作，开销极小

### 2. 优化建议
- 频繁调用的函数考虑内联实现
- 批量处理数据减少函数调用次数
- 重用缓冲区避免重复分配

### 3. 内存使用
- 工具函数通常使用栈上缓冲区
- 格式化函数需要调用者提供输出缓冲区
- 避免在工具函数内部进行动态内存分配

## 扩展指南

### 1. 添加新工具函数
1. 在头文件中声明函数原型
2. 实现函数功能，确保高效和安全
3. 添加详细的文档注释
4. 创建测试用例验证功能

### 2. 增强现有函数
1. 添加更多格式化选项
2. 支持不同进制表示（二进制、八进制）
3. 添加国际化支持（本地化字符串）
4. 增强错误处理和边界检查

### 3. 添加平台特定优化
1. 识别平台特定的性能优化机会
2. 使用平台特定的内联汇编或内置函数
3. 添加平台检测和条件编译
4. 更新平台兼容性文档

## 相关模块

### 1. 直接依赖
- C标准库（字符串处理、格式化）
- 平台特定头文件（stdint.h, stddef.h等）

### 2. 服务模块
- 内存转储模块（使用字节转换和格式化）
- 调用栈跟踪模块（使用地址格式化）
- 内存验证模块（使用对齐检查）

### 3. 协同模块
- 调试工具核心模块（共享工具函数）
- 错误处理模块（使用格式化函数）

## 注意事项

1. **缓冲区安全**: 所有接受缓冲区的函数必须确保不越界
2. **线程安全**: 工具函数应该是可重入和线程安全的
3. **性能影响**: 在性能关键路径上谨慎使用格式化函数
4. **平台兼容性**: 注意不同平台的指针大小和表示差异

## 测试要点

### 1. 功能测试
- 测试所有工具函数的正确性
- 测试边界情况和特殊输入
- 测试缓冲区不足的情况
- 测试不同数据类型的处理

### 2. 性能测试
- 测试高频调用的性能影响
- 测试不同缓冲区大小的性能
- 测试与标准库函数的性能对比
- 测试内存使用情况

### 3. 安全测试
- 测试缓冲区溢出防护
- 测试无效输入处理
- 测试空指针和零大小处理
- 测试对齐值的有效性检查

### 4. 集成测试
- 测试与其他调试工具模块的集成
- 测试在真实场景中的使用
- 测试错误处理和恢复
- 测试平台兼容性

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
