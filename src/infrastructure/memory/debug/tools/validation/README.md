# 内存验证模块

## 概述

内存验证模块提供了内存完整性检查、模式验证和校验和计算等功能。该模块用于检测内存错误、验证内存内容一致性，以及进行内存模式分析和比较。

## 功能特性

### 1. 内存完整性检查
- **块验证**: 验证内存块的完整性和可访问性
- **模式检查**: 检查内存是否匹配指定模式
- **边界检查**: 检测内存访问越界问题

### 2. 模式操作
- **模式填充**: 用指定模式填充内存区域
- **模式查找**: 在内存中查找指定模式
- **模式比较**: 比较内存区域与模式的匹配程度

### 3. 校验和计算
- **简单校验和**: 计算内存区域的简单校验和
- **CRC校验**: 支持CRC校验计算（如果可用）
- **哈希计算**: 计算内存内容的哈希值

### 4. 内存比较
- **区域比较**: 比较两个内存区域的内容
- **差异分析**: 分析内存区域之间的差异
- **相似度计算**: 计算内存区域的相似度

## 接口说明

### 主要函数

#### 1. 内存验证
```c
/**
 * @brief 验证内存块的完整性
 * @param address 内存起始地址
 * @param size 内存大小（字节）
 * @param pattern 验证模式，如果为NULL则不进行模式检查
 * @param pattern_size 模式大小（字节）
 * @return 验证通过返回true，失败返回false
 * @note 此函数检查内存是否可读，并可选择检查模式匹配
 */
bool F_validate_memory_block(const void* address, size_t size,
                            const void* pattern, size_t pattern_size);
```

#### 2. 模式操作
```c
/**
 * @brief 用指定模式填充内存区域
 * @param address 内存起始地址
 * @param size 内存大小（字节）
 * @param pattern 填充模式
 * @param pattern_size 模式大小（字节）
 * @note 模式会重复应用直到填满整个内存区域
 */
void F_fill_memory_pattern(void* address, size_t size,
                          const void* pattern, size_t pattern_size);

/**
 * @brief 检查内存是否匹配指定模式
 * @param address 内存起始地址
 * @param size 内存大小（字节）
 * @param pattern 检查模式
 * @param pattern_size 模式大小（字节）
 * @return 完全匹配返回true，否则返回false
 */
bool F_check_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);

/**
 * @brief 在内存中查找指定模式
 * @param address 内存起始地址
 * @param size 内存大小（字节）
 * @param pattern 查找模式
 * @param pattern_size 模式大小（字节）
 * @return 找到模式返回首次出现的位置，未找到返回NULL
 */
void* F_find_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);
```

#### 3. 校验和计算
```c
/**
 * @brief 计算内存区域的校验和
 * @param address 内存起始地址
 * @param size 内存大小（字节）
 * @return 32位校验和值
 * @note 使用简单的加法校验和算法
 */
uint32_t F_calculate_memory_checksum(const void* address, size_t size);
```

#### 4. 内存比较
```c
/**
 * @brief 比较两个内存区域
 * @param addr1 第一个内存区域起始地址
 * @param addr2 第二个内存区域起始地址
 * @param size 比较的大小（字节）
 * @return 相同返回0，不同返回第一个不同字节的偏移量+1
 * @note 返回值范围：0表示完全相同，1-size表示第一个不同位置
 */
int F_compare_memory_regions(const void* addr1, const void* addr2, size_t size);
```

## 使用示例

### 基本使用
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main() {
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        return 1;
    }
    
    // 创建测试内存区域
    uint8_t buffer1[256];
    uint8_t buffer2[256];
    
    // 用模式填充内存
    uint8_t pattern1[] = {0xAA, 0xBB, 0xCC, 0xDD};
    F_fill_memory_pattern(buffer1, sizeof(buffer1), pattern1, sizeof(pattern1));
    
    // 复制相同内容到buffer2
    memcpy(buffer2, buffer1, sizeof(buffer1));
    
    // 验证内存完整性
    bool valid = F_validate_memory_block(buffer1, sizeof(buffer1), NULL, 0);
    printf("Memory block validation: %s\n", valid ? "PASS" : "FAIL");
    
    // 检查模式匹配
    bool pattern_match = F_check_memory_pattern(buffer1, sizeof(buffer1), 
                                               pattern1, sizeof(pattern1));
    printf("Pattern check: %s\n", pattern_match ? "MATCH" : "MISMATCH");
    
    // 计算校验和
    uint32_t checksum1 = F_calculate_memory_checksum(buffer1, sizeof(buffer1));
    uint32_t checksum2 = F_calculate_memory_checksum(buffer2, sizeof(buffer2));
    printf("Checksum 1: 0x%08X\n", checksum1);
    printf("Checksum 2: 0x%08X\n", checksum2);
    printf("Checksums match: %s\n", checksum1 == checksum2 ? "YES" : "NO");
    
    // 比较内存区域
    int compare_result = F_compare_memory_regions(buffer1, buffer2, sizeof(buffer1));
    if (compare_result == 0) {
        printf("Memory regions are identical\n");
    } else {
        printf("Memory regions differ at offset %d\n", compare_result - 1);
    }
    
    // 修改buffer2中的一个字节
    buffer2[100] = 0xFF;
    
    // 重新比较
    compare_result = F_compare_memory_regions(buffer1, buffer2, sizeof(buffer1));
    if (compare_result == 0) {
        printf("Memory regions are identical\n");
    } else {
        printf("Memory regions differ at offset %d\n", compare_result - 1);
    }
    
    // 查找模式
    uint8_t search_pattern[] = {0xAA, 0xBB};
    void* found = F_find_memory_pattern(buffer1, sizeof(buffer1),
                                       search_pattern, sizeof(search_pattern));
    if (found != NULL) {
        size_t offset = (uint8_t*)found - buffer1;
        printf("Pattern found at offset %zu\n", offset);
    } else {
        printf("Pattern not found\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    return 0;
}
```

### 内存错误检测
```c
#include "CN_memory_debug_tools.h"
#include <stdlib.h>

void detect_memory_errors(void) {
    // 分配测试内存
    size_t block_size = 1024;
    uint8_t* memory_block = (uint8_t*)malloc(block_size);
    if (memory_block == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    
    // 用已知模式填充
    uint8_t fill_pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
    F_fill_memory_pattern(memory_block, block_size, fill_pattern, sizeof(fill_pattern));
    
    // 验证填充结果
    bool pattern_ok = F_check_memory_pattern(memory_block, block_size,
                                            fill_pattern, sizeof(fill_pattern));
    printf("Pattern verification: %s\n", pattern_ok ? "OK" : "CORRUPTED");
    
    // 计算初始校验和
    uint32_t initial_checksum = F_calculate_memory_checksum(memory_block, block_size);
    printf("Initial checksum: 0x%08X\n", initial_checksum);
    
    // 模拟内存损坏（写入越界）
    // 注意：这是为了演示，实际中不应该这样做
    memory_block[block_size] = 0xFF;  // 越界写入
    
    // 重新计算校验和（可能崩溃或得到错误结果）
    uint32_t corrupted_checksum = F_calculate_memory_checksum(memory_block, block_size);
    printf("Corrupted checksum: 0x%08X\n", corrupted_checksum);
    
    // 验证内存块（可能检测到错误）
    bool still_valid = F_validate_memory_block(memory_block, block_size, NULL, 0);
    printf("Memory still valid: %s\n", still_valid ? "YES" : "NO");
    
    free(memory_block);
}
```

### 集成到内存分配器
```c
#include "CN_memory_debug_tools.h"
#include <stdlib.h>

// 调试内存分配器
void* debug_malloc(size_t size) {
    // 分配额外空间用于模式保护
    size_t total_size = size + 32;  // 额外32字节用于保护模式
    uint8_t* block = (uint8_t*)malloc(total_size);
    if (block == NULL) {
        return NULL;
    }
    
    // 在内存块前后添加保护模式
    uint8_t guard_pattern[] = {0xBA, 0xAD, 0xF0, 0x0D};
    
    // 前保护区域（16字节）
    F_fill_memory_pattern(block, 16, guard_pattern, sizeof(guard_pattern));
    
    // 用户区域（初始化为0xCD）
    uint8_t init_pattern = 0xCD;
    F_fill_memory_pattern(block + 16, size, &init_pattern, 1);
    
    // 后保护区域（剩余空间）
    F_fill_memory_pattern(block + 16 + size, total_size - 16 - size,
                         guard_pattern, sizeof(guard_pattern));
    
    // 返回用户区域指针
    return block + 16;
}

// 调试内存释放
void debug_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    uint8_t* block = (uint8_t*)ptr - 16;
    
    // 检查保护模式是否完好
    uint8_t guard_pattern[] = {0xBA, 0xAD, 0xF0, 0x0D};
    
    bool front_guard_ok = F_check_memory_pattern(block, 16,
                                                guard_pattern, sizeof(guard_pattern));
    
    // 获取分配时的大小信息（这里简化处理，实际需要存储大小）
    size_t user_size = 1024;  // 假设已知大小
    
    bool back_guard_ok = F_check_memory_pattern(block + 16 + user_size, 16,
                                               guard_pattern, sizeof(guard_pattern));
    
    if (!front_guard_ok || !back_guard_ok) {
        printf("Memory corruption detected!\n");
        // 可以在这里转储内存内容用于分析
    }
    
    free(block);
}
```

## 实现细节

### 1. 验证算法
1. **地址有效性检查**: 验证地址是否非NULL
2. **大小合理性检查**: 验证大小是否合理（非零，不过大）
3. **模式匹配算法**: 使用高效的模式匹配算法
4. **校验和计算**: 使用简单的加法校验和

### 2. 模式匹配优化
- 使用内存对齐优化访问
- 批量处理减少循环次数
- 使用平台特定的优化指令（如果可用）

### 3. 错误检测策略
- 越界访问检测（通过保护模式）
- 内存损坏检测（通过校验和）
- 模式完整性检查（通过定期验证）

### 4. 性能考虑
- 验证操作有性能开销
- 模式匹配复杂度O(n)
- 校验和计算需要遍历整个内存区域

## 性能考虑

### 1. 验证开销
- 内存验证需要读取整个内存区域
- 模式匹配有额外的计算开销
- 保护模式检查增加内存访问

### 2. 优化建议
- 生产环境选择性启用验证
- 使用抽样检查而非全量检查
- 优化热路径上的验证频率

### 3. 内存使用
- 保护模式增加内存开销
- 校验和计算需要临时变量
- 模式匹配需要模式缓冲区

## 扩展指南

### 1. 添加新验证算法
1. 在头文件中声明新函数
2. 实现验证算法
3. 添加性能测试
4. 更新文档说明

### 2. 支持更多校验算法
1. 添加CRC32、MD5、SHA等支持
2. 实现算法选择接口
3. 添加算法性能比较
4. 更新使用示例

### 3. 添加内存分析功能
1. 添加内存统计功能
2. 实现内存泄漏检测
3. 添加内存碎片分析
4. 更新集成示例

## 相关模块

### 1. 直接依赖
- 调试工具核心模块（用于输出和状态管理）
- C标准库（内存操作、字符串处理）

### 2. 被依赖模块
- 内存调试器模块（用于内存错误检测）
- 内存分配器模块（用于内存保护）

### 3. 协同模块
- 内存转储模块（转储验证结果）
- 调用栈跟踪模块（记录错误发生位置）

## 注意事项

1. **性能影响**: 内存验证可能显著影响性能
2. **内存开销**: 保护模式增加内存使用
3. **平台差异**: 某些验证可能依赖平台特性
4. **线程安全**: 当前实现假设单线程环境

## 测试要点

### 1. 功能测试
- 测试各种内存大小的验证
- 测试所有模式操作函数
- 测试校验和计算正确性
- 测试内存比较功能

### 2. 边界测试
- 测试零字节验证
- 测试无效地址处理
- 测试模式大小为零的情况
- 测试内存边界情况

### 3. 性能测试
- 测试大内存验证性能
- 测试不同模式大小的性能影响
- 测试校验和计算性能
- 测试内存比较性能

### 4. 错误检测测试
- 测试内存损坏检测
- 测试越界访问检测
- 测试保护模式有效性
- 测试错误恢复机制

### 5. 集成测试
- 测试与内存分配器的集成
- 测试在真实调试场景中的使用
- 测试与其他调试工具的协同工作
- 测试错误报告和日志记录

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
