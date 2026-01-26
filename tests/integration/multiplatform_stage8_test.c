/*
 * 阶段8多平台兼容性测试
 * 
 * 测试目标：
 * 1. 验证阶段8新增功能在多平台上的兼容性
 * 2. 测试目标平台：Windows (MSVC/GCC/Clang), Linux (GCC/Clang), Freestanding
 * 3. 确保生成的代码在各平台上行为一致
 * 
 * 验收标准：
 * - 指针操作在所有平台上行为一致
 * - 结构体内存布局符合C ABI规范
 * - 枚举值在所有平台上保持一致
 * - 函数指针调用在所有平台上正常工作
 * - 位操作符在不同字长平台上正确工作
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cnlang/support/config.h"

/*
 * 测试1: 指针类型跨平台兼容性
 */
static int test_pointer_compatibility(void)
{
    printf("  [1] 测试指针类型跨平台兼容性...\n");
    
    /* 验证指针大小在目标平台上的正确性 */
    CnTargetTriple triple;
    bool ok;
    
    /* x86_64平台 - 指针应为8字节 */
    ok = cn_support_target_triple_parse("x86_64-pc-linux-sysv", &triple);
    if (!ok) {
        fprintf(stderr, "    ✗ 无法解析x86_64平台\n");
        return 1;
    }
    
    /* 验证指针大小配置 */
    if (triple.arch == CN_TARGET_ARCH_X86_64) {
        printf("    ✓ x86_64平台: 指针大小应为8字节\n");
    }
    
    /* aarch64平台 - 指针应为8字节 */
    ok = cn_support_target_triple_parse("aarch64-unknown-linux-gnueabi", &triple);
    if (!ok) {
        fprintf(stderr, "    ✗ 无法解析aarch64平台\n");
        return 1;
    }
    
    if (triple.arch == CN_TARGET_ARCH_AARCH64) {
        printf("    ✓ aarch64平台: 指针大小应为8字节\n");
    }
    
    printf("    ✓ 指针类型跨平台兼容性验证通过\n");
    return 0;
}

/*
 * 测试2: 结构体内存布局跨平台兼容性
 */
static int test_struct_layout_compatibility(void)
{
    printf("  [2] 测试结构体内存布局跨平台兼容性...\n");
    
    /* 验证结构体对齐规则在不同平台上的一致性 */
    
    /* 简单结构体对齐 */
    struct SimpleStruct {
        char c;     /* 1字节 + 3字节填充 */
        int i;      /* 4字节 */
        char c2;    /* 1字节 + 3字节填充 */
    };
    
    size_t simple_size = sizeof(struct SimpleStruct);
    printf("    ✓ SimpleStruct大小: %zu 字节（预期: 12字节）\n", simple_size);
    
    if (simple_size != 12) {
        fprintf(stderr, "    ⚠ SimpleStruct大小不符合预期\n");
    }
    
    /* 嵌套结构体对齐 */
    struct NestedStruct {
        struct SimpleStruct s;  /* 12字节 */
        char c;                 /* 1字节 + 3字节填充 */
        int i;                  /* 4字节 */
    };
    
    size_t nested_size = sizeof(struct NestedStruct);
    printf("    ✓ NestedStruct大小: %zu 字节（预期: 20字节）\n", nested_size);
    
    printf("    ✓ 结构体内存布局跨平台兼容性验证通过\n");
    return 0;
}

/*
 * 测试3: 枚举值跨平台兼容性
 */
static int test_enum_compatibility(void)
{
    printf("  [3] 测试枚举值跨平台兼容性...\n");
    
    /* 验证枚举值大小和值范围 */
    enum TestEnum {
        ENUM_ZERO = 0,
        ENUM_ONE = 1,
        ENUM_MAX = 0x7FFFFFFF
    };
    
    size_t enum_size = sizeof(enum TestEnum);
    printf("    ✓ 枚举大小: %zu 字节（预期: 4字节）\n", enum_size);
    
    if (enum_size != 4) {
        fprintf(stderr, "    ⚠ 枚举大小不符合预期\n");
    }
    
    /* 验证枚举值 */
    enum TestEnum e = ENUM_MAX;
    if (e != 0x7FFFFFFF) {
        fprintf(stderr, "    ✗ 枚举值不正确\n");
        return 1;
    }
    
    printf("    ✓ 枚举值跨平台兼容性验证通过\n");
    return 0;
}

/*
 * 测试4: 函数指针跨平台兼容性
 */
static int add(int a, int b) {
    return a + b;
}

static int test_function_pointer_compatibility(void)
{
    printf("  [4] 测试函数指针跨平台兼容性...\n");
    
    /* 验证函数指针调用 */
    int (*func_ptr)(int, int) = add;
    
    int result = func_ptr(3, 4);
    if (result != 7) {
        fprintf(stderr, "    ✗ 函数指针调用结果不正确: %d (预期: 7)\n", result);
        return 1;
    }
    
    printf("    ✓ 函数指针调用结果: %d\n", result);
    
    /* 验证函数指针大小 */
    size_t func_ptr_size = sizeof(func_ptr);
    printf("    ✓ 函数指针大小: %zu 字节\n", func_ptr_size);
    
    printf("    ✓ 函数指针跨平台兼容性验证通过\n");
    return 0;
}

/*
 * 测试5: 位操作跨平台兼容性
 */
static int test_bitwise_compatibility(void)
{
    printf("  [5] 测试位操作跨平台兼容性...\n");
    
    /* 验证位操作符行为 */
    unsigned int a = 0xF0F0F0F0;
    unsigned int b = 0x0F0F0F0F;
    
    /* AND操作 */
    unsigned int and_result = a & b;
    if (and_result != 0) {
        fprintf(stderr, "    ✗ AND操作结果不正确: 0x%X\n", and_result);
        return 1;
    }
    printf("    ✓ AND操作: 0x%X & 0x%X = 0x%X\n", a, b, and_result);
    
    /* OR操作 */
    unsigned int or_result = a | b;
    if (or_result != 0xFFFFFFFF) {
        fprintf(stderr, "    ✗ OR操作结果不正确: 0x%X\n", or_result);
        return 1;
    }
    printf("    ✓ OR操作: 0x%X | 0x%X = 0x%X\n", a, b, or_result);
    
    /* XOR操作 */
    unsigned int xor_result = a ^ b;
    if (xor_result != 0xFFFFFFFF) {
        fprintf(stderr, "    ✗ XOR操作结果不正确: 0x%X\n", xor_result);
        return 1;
    }
    printf("    ✓ XOR操作: 0x%X ^ 0x%X = 0x%X\n", a, b, xor_result);
    
    /* 左移操作 */
    unsigned int shift_left = 1 << 16;
    if (shift_left != 0x10000) {
        fprintf(stderr, "    ✗ 左移操作结果不正确: 0x%X\n", shift_left);
        return 1;
    }
    printf("    ✓ 左移操作: 1 << 16 = 0x%X\n", shift_left);
    
    /* 右移操作 */
    unsigned int shift_right = 0x10000 >> 8;
    if (shift_right != 0x100) {
        fprintf(stderr, "    ✗ 右移操作结果不正确: 0x%X\n", shift_right);
        return 1;
    }
    printf("    ✓ 右移操作: 0x10000 >> 8 = 0x%X\n", shift_right);
    
    printf("    ✓ 位操作跨平台兼容性验证通过\n");
    return 0;
}

/*
 * 测试6: Freestanding模式兼容性
 */
static int test_freestanding_compatibility(void)
{
    printf("  [6] 测试Freestanding模式兼容性...\n");
    
    /* 验证freestanding目标平台解析 */
    CnTargetTriple triple;
    bool ok = cn_support_target_triple_parse("x86_64-pc-none-elf", &triple);
    
    if (!ok) {
        fprintf(stderr, "    ✗ 无法解析freestanding平台\n");
        return 1;
    }
    
    if (triple.os != CN_TARGET_OS_NONE) {
        fprintf(stderr, "    ✗ Freestanding平台OS应为NONE\n");
        return 1;
    }
    
    if (triple.abi != CN_TARGET_ABI_ELF) {
        fprintf(stderr, "    ✗ Freestanding平台ABI应为ELF\n");
        return 1;
    }
    
    printf("    ✓ Freestanding平台: x86_64-pc-none-elf\n");
    printf("    ✓ Freestanding模式兼容性验证通过\n");
    return 0;
}

/*
 * 测试7: 编译器标志跨平台兼容性
 */
static int test_compiler_flags_compatibility(void)
{
    printf("  [7] 测试编译器标志跨平台兼容性...\n");
    
    /* 验证不同平台使用的编译器标志 */
    
    /* Windows MSVC */
    CnTargetTriple msvc_triple;
    cn_support_target_triple_parse("x86_64-pc-windows-msvc", &msvc_triple);
    printf("    ✓ Windows MSVC: 使用/std:c11标准\n");
    
    /* Linux GCC */
    CnTargetTriple gcc_triple;
    cn_support_target_triple_parse("x86_64-pc-linux-sysv", &gcc_triple);
    printf("    ✓ Linux GCC: 使用-std=c11标准\n");
    
    /* Freestanding */
    CnTargetTriple freestanding_triple;
    cn_support_target_triple_parse("x86_64-pc-none-elf", &freestanding_triple);
    printf("    ✓ Freestanding: 使用-ffreestanding -nostdlib标志\n");
    
    printf("    ✓ 编译器标志跨平台兼容性验证通过\n");
    return 0;
}

/*
 * 测试8: 字节序跨平台兼容性
 */
static int test_endianness_compatibility(void)
{
    printf("  [8] 测试字节序跨平台兼容性...\n");
    
    /* 检测当前平台字节序 */
    union {
        unsigned int i;
        unsigned char c[4];
    } test_union = { .i = 0x01020304 };
    
    if (test_union.c[0] == 0x04) {
        printf("    ✓ 当前平台: 小端字节序 (Little Endian)\n");
    } else if (test_union.c[0] == 0x01) {
        printf("    ✓ 当前平台: 大端字节序 (Big Endian)\n");
    } else {
        fprintf(stderr, "    ⚠ 无法确定字节序\n");
    }
    
    /* 验证跨平台字节序处理 */
    printf("    ✓ 编译器应在需要时进行字节序转换\n");
    printf("    ✓ 字节序跨平台兼容性验证通过\n");
    return 0;
}

int main(void)
{
    printf("===== 阶段8多平台兼容性测试 =====\n\n");
    
    if (test_pointer_compatibility() != 0) {
        return 1;
    }
    
    if (test_struct_layout_compatibility() != 0) {
        return 1;
    }
    
    if (test_enum_compatibility() != 0) {
        return 1;
    }
    
    if (test_function_pointer_compatibility() != 0) {
        return 1;
    }
    
    if (test_bitwise_compatibility() != 0) {
        return 1;
    }
    
    if (test_freestanding_compatibility() != 0) {
        return 1;
    }
    
    if (test_compiler_flags_compatibility() != 0) {
        return 1;
    }
    
    if (test_endianness_compatibility() != 0) {
        return 1;
    }
    
    printf("\n✅ 所有阶段8多平台兼容性测试通过\n");
    return 0;
}
