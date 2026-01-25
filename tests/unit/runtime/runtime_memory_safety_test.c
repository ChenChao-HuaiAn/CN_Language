/*
 * 运行时内存访问安全检查测试
 * 测试内存访问安全检查功能
 */

#include "cnlang/runtime/memory.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 测试内存对齐检查 */
static int test_alignment_check(void) {
    printf("测试内存对齐检查...\n");
    
    // 启用安全检查
    cn_rt_memory_set_safety_check(1);
    
    // 测试对齐的地址
    int aligned_value = 42;
    uintptr_t aligned_addr = (uintptr_t)&aligned_value;
    
    assert(cn_rt_memory_check_alignment(aligned_addr, 1) == 1);
    assert(cn_rt_memory_check_alignment(aligned_addr, 2) == 1);
    assert(cn_rt_memory_check_alignment(aligned_addr, 4) == 1);
    
    // 测试未对齐的地址
    char buffer[16];
    uintptr_t unaligned_addr = ((uintptr_t)buffer) + 1;
    
    assert(cn_rt_memory_check_alignment(unaligned_addr, 1) == 1);
    // 2字节对齐检查可能失败（取决于buffer的对齐情况）
    if (unaligned_addr % 2 != 0) {
        assert(cn_rt_memory_check_alignment(unaligned_addr, 2) == 0);
    }
    
    printf("  ✓ 内存对齐检查通过\n");
    return 0;
}

/* 测试内存可读性检查 */
static int test_readable_check(void) {
    printf("测试内存可读性检查...\n");
    
    cn_rt_memory_set_safety_check(1);
    
    // 测试可读的内存
    int readable_value = 123;
    uintptr_t readable_addr = (uintptr_t)&readable_value;
    
    assert(cn_rt_memory_check_readable(readable_addr, sizeof(int)) == 1);
    
    // 测试空指针（应该失败）
    assert(cn_rt_memory_check_readable(0, sizeof(int)) == 0);
    
    printf("  ✓ 内存可读性检查通过\n");
    return 0;
}

/* 测试内存可写性检查 */
static int test_writable_check(void) {
    printf("测试内存可写性检查...\n");
    
    cn_rt_memory_set_safety_check(1);
    
    // 测试可写的内存
    int writable_value = 456;
    uintptr_t writable_addr = (uintptr_t)&writable_value;
    
    assert(cn_rt_memory_check_writable(writable_addr, sizeof(int)) == 1);
    
    // 测试空指针（应该失败）
    assert(cn_rt_memory_check_writable(0, sizeof(int)) == 0);
    
    printf("  ✓ 内存可写性检查通过\n");
    return 0;
}

/* 测试安全内存读取 */
static int test_safe_read(void) {
    printf("测试安全内存读取...\n");
    
    cn_rt_memory_set_safety_check(1);
    
    // 测试读取不同大小的值
    uint8_t byte_val = 0x12;
    uint16_t word_val = 0x1234;
    uint32_t dword_val = 0x12345678;
    uint64_t qword_val = 0x123456789ABCDEF0ULL;
    
    uintptr_t result;
    
    result = cn_rt_memory_read_safe((uintptr_t)&byte_val, 1);
    assert(result == 0x12);
    
    result = cn_rt_memory_read_safe((uintptr_t)&word_val, 2);
    assert(result == 0x1234);
    
    result = cn_rt_memory_read_safe((uintptr_t)&dword_val, 4);
    assert(result == 0x12345678);
    
    result = cn_rt_memory_read_safe((uintptr_t)&qword_val, 8);
    assert(result == 0x123456789ABCDEF0ULL);
    
    printf("  ✓ 安全内存读取通过\n");
    return 0;
}

/* 测试安全内存写入 */
static int test_safe_write(void) {
    printf("测试安全内存写入...\n");
    
    cn_rt_memory_set_safety_check(1);
    
    // 测试写入不同大小的值
    uint8_t byte_val = 0;
    uint16_t word_val = 0;
    uint32_t dword_val = 0;
    uint64_t qword_val = 0;
    
    cn_rt_memory_write_safe((uintptr_t)&byte_val, 0xAB, 1);
    assert(byte_val == 0xAB);
    
    cn_rt_memory_write_safe((uintptr_t)&word_val, 0xABCD, 2);
    assert(word_val == 0xABCD);
    
    cn_rt_memory_write_safe((uintptr_t)&dword_val, 0xABCDEF01, 4);
    assert(dword_val == 0xABCDEF01);
    
    cn_rt_memory_write_safe((uintptr_t)&qword_val, 0xFEDCBA9876543210ULL, 8);
    assert(qword_val == 0xFEDCBA9876543210ULL);
    
    printf("  ✓ 安全内存写入通过\n");
    return 0;
}

/* 测试安全内存复制 */
static int test_safe_copy(void) {
    printf("测试安全内存复制...\n");
    
    cn_rt_memory_set_safety_check(1);
    
    // 测试普通内存复制
    int source[5] = {1, 2, 3, 4, 5};
    int dest[5] = {0};
    
    cn_rt_memory_copy_safe(dest, source, sizeof(source));
    
    for (int i = 0; i < 5; i++) {
        assert(dest[i] == source[i]);
    }
    
    // 测试字符串复制
    char str_source[] = "Hello, Memory!";
    char str_dest[32] = {0};
    
    cn_rt_memory_copy_safe(str_dest, str_source, strlen(str_source) + 1);
    assert(strcmp(str_dest, str_source) == 0);
    
    printf("  ✓ 安全内存复制通过\n");
    return 0;
}

/* 测试安全内存设置 */
static int test_safe_set(void) {
    printf("测试安全内存设置...\n");
    
    cn_rt_memory_set_safety_check(1);
    
    // 测试将内存设置为特定值
    char buffer[64];
    
    // 设置为0
    cn_rt_memory_set_safe(buffer, 0, sizeof(buffer));
    for (size_t i = 0; i < sizeof(buffer); i++) {
        assert(buffer[i] == 0);
    }
    
    // 设置为0xFF
    cn_rt_memory_set_safe(buffer, 0xFF, sizeof(buffer));
    for (size_t i = 0; i < sizeof(buffer); i++) {
        assert((unsigned char)buffer[i] == 0xFF);
    }
    
    // 设置为0x55
    cn_rt_memory_set_safe(buffer, 0x55, sizeof(buffer));
    for (size_t i = 0; i < sizeof(buffer); i++) {
        assert((unsigned char)buffer[i] == 0x55);
    }
    
    printf("  ✓ 安全内存设置通过\n");
    return 0;
}

/* 测试禁用安全检查 */
static int test_disable_safety(void) {
    printf("测试禁用安全检查...\n");
    
    // 禁用安全检查
    cn_rt_memory_set_safety_check(0);
    
    int value = 999;
    uintptr_t addr = (uintptr_t)&value;
    
    // 读取和写入应该仍然工作
    uintptr_t read_val = cn_rt_memory_read_safe(addr, 4);
    assert(read_val == 999);
    
    cn_rt_memory_write_safe(addr, 777, 4);
    assert(value == 777);
    
    // 重新启用
    cn_rt_memory_set_safety_check(1);
    
    printf("  ✓ 禁用安全检查测试通过\n");
    return 0;
}

/* 测试边界情况 */
static int test_edge_cases(void) {
    printf("测试边界情况...\n");
    
    cn_rt_memory_set_safety_check(1);
    
    // 测试大小为0的操作
    char buffer[16] = {0};
    
    // 复制0字节
    cn_rt_memory_copy_safe(buffer, buffer, 0);
    
    // 设置0字节
    cn_rt_memory_set_safe(buffer, 0xFF, 0);
    
    // 验证buffer没有被修改
    for (size_t i = 0; i < sizeof(buffer); i++) {
        assert(buffer[i] == 0);
    }
    
    printf("  ✓ 边界情况测试通过\n");
    return 0;
}

/* 测试内存映射安全检查（仅在支持的平台上） */
static int test_memory_mapping_safety(void) {
    printf("测试内存映射安全检查...\n");
    
#if !defined(CN_FREESTANDING) && !defined(_WIN32) && !defined(_WIN64)
    cn_rt_memory_set_safety_check(1);
    
    // 测试映射4KB内存
    void* mapped = cn_rt_memory_map_safe(NULL, 4096, 3, 0x22);
    
    if (mapped != NULL) {
        // 测试访问映射的内存
        uintptr_t mapped_addr = (uintptr_t)mapped;
        
        // 写入数据
        cn_rt_memory_write_safe(mapped_addr, 12345, 4);
        
        // 读取数据
        uintptr_t read_val = cn_rt_memory_read_safe(mapped_addr, 4);
        assert(read_val == 12345);
        
        // 解除映射
        int result = cn_rt_memory_unmap_safe(mapped, 4096);
        assert(result == 0);
        
        printf("  ✓ 内存映射安全检查通过\n");
    } else {
        printf("  ⚠ 内存映射不可用（平台不支持）\n");
    }
#else
    printf("  ⚠ 当前平台不支持内存映射\n");
#endif
    
    return 0;
}

int main(void) {
    printf("========== 运行时内存安全检查测试 ==========\n\n");
    
    if (test_alignment_check() != 0) return 1;
    if (test_readable_check() != 0) return 1;
    if (test_writable_check() != 0) return 1;
    if (test_safe_read() != 0) return 1;
    if (test_safe_write() != 0) return 1;
    if (test_safe_copy() != 0) return 1;
    if (test_safe_set() != 0) return 1;
    if (test_disable_safety() != 0) return 1;
    if (test_edge_cases() != 0) return 1;
    if (test_memory_mapping_safety() != 0) return 1;
    
    printf("\n========================================\n");
    printf("所有内存安全测试通过! ✓\n");
    printf("========================================\n");
    
    return 0;
}
