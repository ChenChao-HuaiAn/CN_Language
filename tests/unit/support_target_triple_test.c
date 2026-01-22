#include "cnlang/support/config.h"

#include <stdio.h>
#include <string.h>

static int test_parse_simple_arch_abi(void)
{
    CnTargetTriple triple;
    bool ok;

    ok = cn_support_target_triple_parse("x86_64-elf", &triple);
    if (!ok) {
        fprintf(stderr, "support_target_triple_test: 解析 x86_64-elf 失败\n");
        return 1;
    }

    if (triple.arch != CN_TARGET_ARCH_X86_64 ||
        triple.vendor != CN_TARGET_VENDOR_UNKNOWN ||
        triple.os != CN_TARGET_OS_NONE ||
        triple.abi != CN_TARGET_ABI_ELF) {
        fprintf(stderr,
                "support_target_triple_test: 解析结果不符合预期 (arch-abi)\n");
        return 1;
    }

    return 0;
}

static int test_parse_full_triple(void)
{
    CnTargetTriple triple;
    bool ok;

    ok = cn_support_target_triple_parse("aarch64-unknown-linux-gnueabi", &triple);
    if (!ok) {
        fprintf(stderr,
                "support_target_triple_test: 解析 aarch64-unknown-linux-gnueabi 失败\n");
        return 1;
    }

    if (triple.arch != CN_TARGET_ARCH_AARCH64 ||
        triple.vendor != CN_TARGET_VENDOR_UNKNOWN ||
        triple.os != CN_TARGET_OS_LINUX ||
        triple.abi != CN_TARGET_ABI_GNU_EABI) {
        fprintf(stderr,
                "support_target_triple_test: 解析结果不符合预期 (full triple)\n");
        return 1;
    }

    return 0;
}

static int test_parse_invalid_arch(void)
{
    CnTargetTriple triple;
    bool ok;

    ok = cn_support_target_triple_parse("mips-unknown-elf", &triple);
    if (ok) {
        fprintf(stderr,
                "support_target_triple_test: 非法架构应当解析失败\n");
        return 1;
    }

    (void)triple;
    return 0;
}

static int test_to_string_round_trip(void)
{
    CnTargetTriple triple;
    CnTargetTriple parsed;
    char buffer[128];
    int written;
    bool ok;

    triple = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);

    written = cn_support_target_triple_to_string(&triple, buffer, sizeof(buffer));
    if (written <= 0) {
        fprintf(stderr,
                "support_target_triple_test: to_string 写入失败\n");
        return 1;
    }

    ok = cn_support_target_triple_parse(buffer, &parsed);
    if (!ok) {
        fprintf(stderr,
                "support_target_triple_test: 解析 to_string 输出失败\n");
        return 1;
    }

    if (parsed.arch != triple.arch ||
        parsed.vendor != triple.vendor ||
        parsed.os != triple.os ||
        parsed.abi != triple.abi) {
        fprintf(stderr,
                "support_target_triple_test: round-trip 结果不一致\n");
        return 1;
    }

    return 0;
}

static int test_preset_x86_64_elf_layout(void)
{
    CnTargetTriple triple;
    CnTargetDataLayout layout;
    bool ok;

    ok = cn_support_target_triple_parse("x86_64-elf", &triple);
    if (!ok) {
        fprintf(stderr,
                "support_target_triple_test: 预设目标 x86_64-elf 解析失败\n");
        return 1;
    }

    ok = cn_support_target_get_data_layout(&triple, &layout);
    if (!ok) {
        fprintf(stderr,
                "support_target_triple_test: 未找到 x86_64-elf 的预设数据布局\n");
        return 1;
    }

    if (layout.pointer_size_in_bits != 64 ||
        layout.pointer_alignment_in_bits != 64 ||
        layout.int_size_in_bits != 32 ||
        layout.int_alignment_in_bits != 32 ||
        layout.long_size_in_bits != 64 ||
        layout.long_alignment_in_bits != 64 ||
        layout.long_long_size_in_bits != 64 ||
        layout.long_long_alignment_in_bits != 64) {
        fprintf(stderr,
                "support_target_triple_test: x86_64-elf 数据布局不符合预期\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_parse_simple_arch_abi() != 0) {
        return 1;
    }

    if (test_parse_full_triple() != 0) {
        return 1;
    }

    if (test_parse_invalid_arch() != 0) {
        return 1;
    }

    if (test_to_string_round_trip() != 0) {
        return 1;
    }

    if (test_preset_x86_64_elf_layout() != 0) {
        return 1;
    }

    printf("support_target_triple_test: OK\n");
    return 0;
}
