#include "cnlang/support/config.h"

#include <stdio.h>
#include <string.h>

/*
 * 内部帮助函数：字符串到枚举的转换
 */

static CnTargetArch cn_support_target_arch_from_string(const char *str)
{
    if (!str) {
        return CN_TARGET_ARCH_UNKNOWN;
    }

    if (strcmp(str, "x86_64") == 0) {
        return CN_TARGET_ARCH_X86_64;
    }

    if (strcmp(str, "aarch64") == 0) {
        return CN_TARGET_ARCH_AARCH64;
    }

    return CN_TARGET_ARCH_UNKNOWN;
}

static CnTargetVendor cn_support_target_vendor_from_string(const char *str)
{
    if (!str) {
        return CN_TARGET_VENDOR_UNKNOWN;
    }

    if (strcmp(str, "pc") == 0) {
        return CN_TARGET_VENDOR_PC;
    }

    if (strcmp(str, "unknown") == 0) {
        return CN_TARGET_VENDOR_UNKNOWN;
    }

    return CN_TARGET_VENDOR_UNKNOWN;
}

static CnTargetOS cn_support_target_os_from_string(const char *str)
{
    if (!str) {
        return CN_TARGET_OS_UNKNOWN;
    }

    if (strcmp(str, "none") == 0 || strcmp(str, "freestanding") == 0) {
        return CN_TARGET_OS_NONE;
    }

    if (strcmp(str, "linux") == 0) {
        return CN_TARGET_OS_LINUX;
    }

    if (strcmp(str, "windows") == 0) {
        return CN_TARGET_OS_WINDOWS;
    }

    if (strcmp(str, "unknown") == 0) {
        return CN_TARGET_OS_UNKNOWN;
    }

    return CN_TARGET_OS_UNKNOWN;
}

static CnTargetABI cn_support_target_abi_from_string(const char *str)
{
    if (!str) {
        return CN_TARGET_ABI_UNKNOWN;
    }

    if (strcmp(str, "sysv") == 0) {
        return CN_TARGET_ABI_SYSV;
    }

    if (strcmp(str, "msvc") == 0) {
        return CN_TARGET_ABI_MSVC;
    }

    if (strcmp(str, "elf") == 0) {
        return CN_TARGET_ABI_ELF;
    }

    if (strcmp(str, "gnueabi") == 0 || strcmp(str, "eabi") == 0) {
        return CN_TARGET_ABI_GNU_EABI;
    }

    if (strcmp(str, "unknown") == 0) {
        return CN_TARGET_ABI_UNKNOWN;
    }

    return CN_TARGET_ABI_UNKNOWN;
}

static const char *cn_support_target_arch_to_string(CnTargetArch arch)
{
    switch (arch) {
    case CN_TARGET_ARCH_X86_64:
        return "x86_64";
    case CN_TARGET_ARCH_AARCH64:
        return "aarch64";
    case CN_TARGET_ARCH_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *cn_support_target_vendor_to_string(CnTargetVendor vendor)
{
    switch (vendor) {
    case CN_TARGET_VENDOR_PC:
        return "pc";
    case CN_TARGET_VENDOR_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *cn_support_target_os_to_string(CnTargetOS os)
{
    switch (os) {
    case CN_TARGET_OS_NONE:
        return "none";
    case CN_TARGET_OS_LINUX:
        return "linux";
    case CN_TARGET_OS_WINDOWS:
        return "windows";
    case CN_TARGET_OS_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *cn_support_target_abi_to_string(CnTargetABI abi)
{
    switch (abi) {
    case CN_TARGET_ABI_SYSV:
        return "sysv";
    case CN_TARGET_ABI_MSVC:
        return "msvc";
    case CN_TARGET_ABI_ELF:
        return "elf";
    case CN_TARGET_ABI_GNU_EABI:
        return "gnueabi";
    case CN_TARGET_ABI_UNKNOWN:
    default:
        return "unknown";
    }
}

/*
 * 将输入字符串按 '-' 分割为最多 4 段。
 */
static int cn_support_split_target_triple(
    const char *str,
    char parts[4][32],
    int *out_count)
{
    int count = 0;
    const char *p;
    const char *next;
    size_t len;

    if (!str || !out_count) {
        return 0;
    }

    p = str;

    while (*p != '\0' && count < 4) {
        next = strchr(p, '-');
        if (next) {
            len = (size_t)(next - p);
        } else {
            len = strlen(p);
        }

        if (len >= sizeof(parts[0])) {
            len = sizeof(parts[0]) - 1;
        }

        memcpy(parts[count], p, len);
        parts[count][len] = '\0';

        count++;

        if (!next) {
            break;
        }

        p = next + 1;
    }

    *out_count = count;
    return 1;
}

bool cn_support_target_triple_parse(const char *str, CnTargetTriple *out)
{
    char parts[4][32];
    int part_count = 0;
    CnTargetTriple result;

    if (!str || !out) {
        return false;
    }

    if (!cn_support_split_target_triple(str, parts, &part_count) || part_count <= 0) {
        return false;
    }

    result.arch = CN_TARGET_ARCH_UNKNOWN;
    result.vendor = CN_TARGET_VENDOR_UNKNOWN;
    result.os = CN_TARGET_OS_UNKNOWN;
    result.abi = CN_TARGET_ABI_UNKNOWN;

    if (part_count >= 1) {
        result.arch = cn_support_target_arch_from_string(parts[0]);
    }

    if (part_count == 2) {
        /* 形如 "arch-abi" 的简写形式，常用于 OS 开发，如 "x86_64-elf" */
        result.vendor = CN_TARGET_VENDOR_UNKNOWN;
        result.os = CN_TARGET_OS_NONE;
        result.abi = cn_support_target_abi_from_string(parts[1]);
    } else if (part_count == 3) {
        /* 形如 "arch-vendor-os" 的形式，ABI 使用默认值 */
        result.vendor = cn_support_target_vendor_from_string(parts[1]);
        result.os = cn_support_target_os_from_string(parts[2]);
    } else if (part_count >= 4) {
        /* 规范形式：arch-vendor-os-abi */
        result.vendor = cn_support_target_vendor_from_string(parts[1]);
        result.os = cn_support_target_os_from_string(parts[2]);
        result.abi = cn_support_target_abi_from_string(parts[3]);
    }

    /* 要求至少识别出架构，否则视为解析失败 */
    if (result.arch == CN_TARGET_ARCH_UNKNOWN) {
        return false;
    }

    *out = result;
    return true;
}

CnTargetTriple cn_support_target_triple_make(
    CnTargetArch arch,
    CnTargetVendor vendor,
    CnTargetOS os,
    CnTargetABI abi)
{
    CnTargetTriple triple;
    triple.arch = arch;
    triple.vendor = vendor;
    triple.os = os;
    triple.abi = abi;
    return triple;
}

int cn_support_target_triple_to_string(
    const CnTargetTriple *triple,
    char *buffer,
    size_t buffer_size)
{
    const char *arch_str;
    const char *vendor_str;
    const char *os_str;
    const char *abi_str;
    int written;

    if (!triple || !buffer || buffer_size == 0) {
        return -1;
    }

    arch_str = cn_support_target_arch_to_string(triple->arch);
    vendor_str = cn_support_target_vendor_to_string(triple->vendor);
    os_str = cn_support_target_os_to_string(triple->os);
    abi_str = cn_support_target_abi_to_string(triple->abi);

    written = snprintf(buffer,
                       buffer_size,
                       "%s-%s-%s-%s",
                       arch_str,
                       vendor_str,
                       os_str,
                       abi_str);

    if (written < 0 || (size_t)written >= buffer_size) {
        return -1;
    }

    return written;
}
