#ifndef CN_SUPPORT_CONFIG_H
#define CN_SUPPORT_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 目标平台配置与目标三元组（target triple）抽象
 *
 * 本模块提供基础的目标三元组表示和字符串解析/格式化能力，
 * 后续可在此基础上扩展更多目标相关配置（数据布局、对齐约束等）。
 */

typedef enum CnTargetArch {
    CN_TARGET_ARCH_UNKNOWN = 0,
    CN_TARGET_ARCH_X86_64,
    CN_TARGET_ARCH_AARCH64
} CnTargetArch;

typedef enum CnTargetVendor {
    CN_TARGET_VENDOR_UNKNOWN = 0,
    CN_TARGET_VENDOR_PC
} CnTargetVendor;

typedef enum CnTargetOS {
    CN_TARGET_OS_UNKNOWN = 0,
    CN_TARGET_OS_NONE,     /* 无操作系统 / 裸机环境 */
    CN_TARGET_OS_LINUX,
    CN_TARGET_OS_WINDOWS
} CnTargetOS;

typedef enum CnTargetABI {
    CN_TARGET_ABI_UNKNOWN = 0,
    CN_TARGET_ABI_SYSV,
    CN_TARGET_ABI_MSVC,
    CN_TARGET_ABI_ELF,
    CN_TARGET_ABI_GNU_EABI
} CnTargetABI;

/*
 * 目标三元组：架构 + 厂商 + 操作系统 + ABI
 */
typedef struct CnTargetTriple {
    CnTargetArch arch;
    CnTargetVendor vendor;
    CnTargetOS os;
    CnTargetABI abi;
} CnTargetTriple;

/*
 * 编译模式：区分宿主环境与 freestanding 模式。
 */
typedef enum CnCompileMode {
    CN_COMPILE_MODE_HOSTED = 0,
    CN_COMPILE_MODE_FREESTANDING = 1
} CnCompileMode;

/*
 * 目标数据布局信息：指针大小、对齐约束、整数宽度等。
 */
typedef struct CnTargetDataLayout {
    int pointer_size_in_bits;
    int pointer_alignment_in_bits;
    int int_size_in_bits;
    int int_alignment_in_bits;
    int long_size_in_bits;
    int long_alignment_in_bits;
    int long_long_size_in_bits;
    int long_long_alignment_in_bits;
} CnTargetDataLayout;

/**
 * 获取指定目标三元组对应的预设数据布局信息。
 * 若该目标未定义预设数据布局，则返回 false。
 */
bool cn_support_target_get_data_layout(
    const CnTargetTriple *triple,
    CnTargetDataLayout *out_layout);

/**
 * 从字符串解析目标三元组。
 *
 * 支持的形式包括：
 *   - "arch"
 *   - "arch-vendor-os-abi"
 *   - "arch-vendor-os"
 *   - "arch-vendor"
 *   - "arch-abi"（常见 OS 开发简写形式，如 "x86_64-elf"）
 * 未识别的字段会被解析为 *_UNKNOWN（或 *_NONE），但若架构无法识别则解析失败。
 *
 * @param str  待解析的三元组字符串
 * @param out  输出的目标三元组结构体指针
 * @return     true 表示解析成功，false 表示解析失败
 */
bool cn_support_target_triple_parse(const char *str, CnTargetTriple *out);

/**
 * 从枚举值直接构造目标三元组。
 */
CnTargetTriple cn_support_target_triple_make(
    CnTargetArch arch,
    CnTargetVendor vendor,
    CnTargetOS os,
    CnTargetABI abi);

/**
 * 将目标三元组格式化为字符串。
 *
 * 目前总是以规范形式输出："arch-vendor-os-abi"。
 *
 * @param triple       要格式化的目标三元组
 * @param buffer       输出缓冲区
 * @param buffer_size  缓冲区大小
 * @return             写入的字符数（不含终止符），失败时返回 -1
 */
int cn_support_target_triple_to_string(
    const CnTargetTriple *triple,
    char *buffer,
    size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* CN_SUPPORT_CONFIG_H */
