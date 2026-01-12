/**
 * @file CN_target_codegen_utils.c
 * @brief CN_Language 目标代码生成器工具函数模块
 * 
 * 提供目标代码生成器的工具函数，如枚举转字符串等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 */

#include "../CN_target_codegen_interface.h"

/**
 * @brief 目标平台转字符串
 * 
 * 将目标平台转换为可读的字符串表示。
 * 
 * @param platform 目标平台
 * @return const char* 平台字符串表示
 */
const char* F_target_platform_to_string(Eum_TargetPlatform platform)
{
    switch (platform) {
        case Eum_TARGET_PLATFORM_GENERIC: return "Generic";
        case Eum_TARGET_PLATFORM_X86: return "x86";
        case Eum_TARGET_PLATFORM_X86_64: return "x86-64";
        case Eum_TARGET_PLATFORM_ARM: return "ARM";
        case Eum_TARGET_PLATFORM_ARM64: return "ARM64";
        case Eum_TARGET_PLATFORM_MIPS: return "MIPS";
        case Eum_TARGET_PLATFORM_POWERPC: return "PowerPC";
        case Eum_TARGET_PLATFORM_RISCV: return "RISC-V";
        case Eum_TARGET_PLATFORM_WASM: return "WebAssembly";
        case Eum_TARGET_PLATFORM_JVM: return "Java Virtual Machine";
        case Eum_TARGET_PLATFORM_DOTNET: return ".NET";
        case Eum_TARGET_PLATFORM_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

/**
 * @brief 目标操作系统转字符串
 * 
 * 将目标操作系统转换为可读的字符串表示。
 * 
 * @param os 目标操作系统
 * @return const char* 操作系统字符串表示
 */
const char* F_target_os_to_string(Eum_TargetOS os)
{
    switch (os) {
        case Eum_TARGET_OS_GENERIC: return "Generic";
        case Eum_TARGET_OS_LINUX: return "Linux";
        case Eum_TARGET_OS_WINDOWS: return "Windows";
        case Eum_TARGET_OS_MACOS: return "macOS";
        case Eum_TARGET_OS_ANDROID: return "Android";
        case Eum_TARGET_OS_IOS: return "iOS";
        case Eum_TARGET_OS_FREEBSD: return "FreeBSD";
        case Eum_TARGET_OS_EMBEDDED: return "Embedded";
        case Eum_TARGET_OS_BARE_METAL: return "Bare Metal";
        case Eum_TARGET_OS_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

/**
 * @brief 目标ABI转字符串
 * 
 * 将目标ABI转换为可读的字符串表示。
 * 
 * @param abi 目标ABI
 * @return const char* ABI字符串表示
 */
const char* F_target_abi_to_string(Eum_TargetABI abi)
{
    switch (abi) {
        case Eum_TARGET_ABI_GENERIC: return "Generic";
        case Eum_TARGET_ABI_SYSV: return "System V";
        case Eum_TARGET_ABI_WIN32: return "Win32";
        case Eum_TARGET_ABI_MACH_O: return "Mach-O";
        case Eum_TARGET_ABI_ELF: return "ELF";
        case Eum_TARGET_ABI_COFF: return "COFF";
        case Eum_TARGET_ABI_WASM: return "WebAssembly";
        case Eum_TARGET_ABI_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

/**
 * @brief 目标格式转字符串
 * 
 * 将目标格式转换为可读的字符串表示。
 * 
 * @param format 目标格式
 * @return const char* 格式字符串表示
 */
const char* F_target_format_to_string(Eum_TargetFormat format)
{
    switch (format) {
        case Eum_TARGET_FORMAT_ASSEMBLY: return "Assembly";
        case Eum_TARGET_FORMAT_OBJECT: return "Object File";
        case Eum_TARGET_FORMAT_EXECUTABLE: return "Executable";
        case Eum_TARGET_FORMAT_LIBRARY: return "Library";
        case Eum_TARGET_FORMAT_SHARED_LIB: return "Shared Library";
        case Eum_TARGET_FORMAT_BYTECODE: return "Bytecode";
        case Eum_TARGET_FORMAT_IR: return "Intermediate Representation";
        case Eum_TARGET_FORMAT_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}
