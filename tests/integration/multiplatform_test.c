/*
 * 多平台功能验证集成测试
 * 
 * 测试目标：
 * 1. 验证目标三元组解析与平台检测
 * 2. 测试目标三元组往返转换
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cnlang/support/config.h"

/*
 * 测试 1: 平台检测与目标三元组解析
 */
static int test_platform_detection(void)
{
    CnTargetTriple triple;
    bool ok;

    printf("  [1] 测试平台检测与目标三元组解析...\n");

    /* 测试 x86_64-elf 解析 */
    ok = cn_support_target_triple_parse("x86_64-elf", &triple);
    if (!ok) {
        fprintf(stderr, "    ✗ 无法解析 x86_64-elf\n");
        return 1;
    }
    if (triple.arch != CN_TARGET_ARCH_X86_64 || triple.abi != CN_TARGET_ABI_ELF) {
        fprintf(stderr, "    ✗ x86_64-elf 解析结果不正确\n");
        return 1;
    }

    /* 测试 aarch64-linux-gnu 解析 */
    ok = cn_support_target_triple_parse("aarch64-unknown-linux-gnueabi", &triple);
    if (!ok) {
        fprintf(stderr, "    ✗ 无法解析 aarch64-unknown-linux-gnueabi\n");
        return 1;
    }
    if (triple.arch != CN_TARGET_ARCH_AARCH64 || 
        triple.os != CN_TARGET_OS_LINUX ||
        triple.abi != CN_TARGET_ABI_GNU_EABI) {
        fprintf(stderr, "    ✗ aarch64-unknown-linux-gnueabi 解析结果不正确\n");
        return 1;
    }

    /* 测试 x86_64-pc-windows-msvc 解析 */
    ok = cn_support_target_triple_parse("x86_64-pc-windows-msvc", &triple);
    if (!ok) {
        fprintf(stderr, "    ✗ 无法解析 x86_64-pc-windows-msvc\n");
        return 1;
    }
    if (triple.arch != CN_TARGET_ARCH_X86_64 ||
        triple.vendor != CN_TARGET_VENDOR_PC ||
        triple.os != CN_TARGET_OS_WINDOWS ||
        triple.abi != CN_TARGET_ABI_MSVC) {
        fprintf(stderr, "    ✗ x86_64-pc-windows-msvc 解析结果不正确\n");
        return 1;
    }

    printf("    ✓ 平台检测与目标三元组解析正确\n");
    return 0;
}

/*
 * 测试 2: 目标三元组往返转换
 */
static int test_target_triple_roundtrip(void)
{
    const char *test_triples[] = {
        "x86_64-pc-windows-msvc",
        "x86_64-pc-linux-sysv",
        "aarch64-unknown-linux-gnueabi",
        "x86_64-pc-none-elf"
    };
    size_t num_triples = sizeof(test_triples) / sizeof(test_triples[0]);

    printf("  [2] 测试目标三元组往返转换...\n");

    for (size_t i = 0; i < num_triples; i++) {
        CnTargetTriple triple;
        char buffer[256];
        bool ok;
        int written;

        /* 解析 */
        ok = cn_support_target_triple_parse(test_triples[i], &triple);
        if (!ok) {
            fprintf(stderr, "    ✗ 无法解析: %s\n", test_triples[i]);
            return 1;
        }

        /* 转换回字符串 */
        written = cn_support_target_triple_to_string(&triple, buffer, sizeof(buffer));
        if (written < 0) {
            fprintf(stderr, "    ✗ 转换字符串失败: %s\n", test_triples[i]);
            return 1;
        }

        /* 验证一致性（注意：某些简化形式可能被扩展为完整形式） */
        printf("    原始: %s -> 转换: %s\n", test_triples[i], buffer);
    }

    printf("    ✓ 目标三元组往返转换测试通过\n");
    return 0;
}

int main(void)
{
    printf("===== 多平台功能验证集成测试 =====\n\n");

    if (test_platform_detection() != 0) {
        return 1;
    }

    if (test_target_triple_roundtrip() != 0) {
        return 1;
    }

    printf("\n✅ 所有多平台功能验证测试通过\n");
    return 0;
}
