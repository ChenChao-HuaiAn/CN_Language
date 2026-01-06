/**
 * @file CN_target_codegen_interface.h
 * @brief CN_Language 目标代码生成器抽象接口
 * 
 * 定义目标代码生成器的抽象接口，支持将中间表示转换为特定目标平台的代码。
 * 遵循项目架构规范，使用接口模式实现多目标平台支持。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_TARGET_CODEGEN_INTERFACE_H
#define CN_TARGET_CODEGEN_INTERFACE_H

#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../ast/CN_ast.h"
#include "CN_codegen_interface.h"

/**
 * @brief 目标平台枚举
 * 
 * 定义支持的目标平台。
 */
typedef enum Eum_TargetPlatform {
    Eum_TARGET_PLATFORM_GENERIC,    ///< 通用平台
    Eum_TARGET_PLATFORM_X86,        ///< x86平台
    Eum_TARGET_PLATFORM_X86_64,     ///< x86-64平台
    Eum_TARGET_PLATFORM_ARM,        ///< ARM平台
    Eum_TARGET_PLATFORM_ARM64,      ///< ARM64平台
    Eum_TARGET_PLATFORM_MIPS,       ///< MIPS平台
    Eum_TARGET_PLATFORM_POWERPC,    ///< PowerPC平台
    Eum_TARGET_PLATFORM_RISCV,      ///< RISC-V平台
    Eum_TARGET_PLATFORM_WASM,       ///< WebAssembly平台
    Eum_TARGET_PLATFORM_JVM,        ///< Java虚拟机平台
    Eum_TARGET_PLATFORM_DOTNET,     ///< .NET平台
    Eum_TARGET_PLATFORM_CUSTOM      ///< 自定义平台
} Eum_TargetPlatform;

/**
 * @brief 目标操作系统枚举
 * 
 * 定义支持的目标操作系统。
 */
typedef enum Eum_TargetOS {
    Eum_TARGET_OS_GENERIC,          ///< 通用操作系统
    Eum_TARGET_OS_LINUX,            ///< Linux
    Eum_TARGET_OS_WINDOWS,          ///< Windows
    Eum_TARGET_OS_MACOS,            ///< macOS
    Eum_TARGET_OS_ANDROID,          ///< Android
    Eum_TARGET_OS_IOS,              ///< iOS
    Eum_TARGET_OS_FREEBSD,          ///< FreeBSD
    Eum_TARGET_OS_EMBEDDED,         ///< 嵌入式系统
    Eum_TARGET_OS_BARE_METAL,       ///< 裸机系统
    Eum_TARGET_OS_CUSTOM            ///< 自定义操作系统
} Eum_TargetOS;

/**
 * @brief 目标ABI枚举
 * 
 * 定义支持的目标ABI（应用程序二进制接口）。
 */
typedef enum Eum_TargetABI {
    Eum_TARGET_ABI_GENERIC,         ///< 通用ABI
    Eum_TARGET_ABI_SYSV,            ///< System V ABI
    Eum_TARGET_ABI_WIN32,           ///< Win32 ABI
    Eum_TARGET_ABI_MACH_O,          ///< Mach-O ABI
    Eum_TARGET_ABI_ELF,             ///< ELF ABI
    Eum_TARGET_ABI_COFF,            ///< COFF ABI
    Eum_TARGET_ABI_WASM,            ///< WebAssembly ABI
    Eum_TARGET_ABI_CUSTOM           ///< 自定义ABI
} Eum_TargetABI;

/**
 * @brief 目标代码格式枚举
 * 
 * 定义支持的目标代码格式。
 */
typedef enum Eum_TargetFormat {
    Eum_TARGET_FORMAT_ASSEMBLY,     ///< 汇编代码
    Eum_TARGET_FORMAT_OBJECT,       ///< 目标文件
    Eum_TARGET_FORMAT_EXECUTABLE,   ///< 可执行文件
    Eum_TARGET_FORMAT_LIBRARY,      ///< 库文件
    Eum_TARGET_FORMAT_SHARED_LIB,   ///< 共享库
    Eum_TARGET_FORMAT_BYTECODE,     ///< 字节码
    Eum_TARGET_FORMAT_IR,           ///< 中间表示
    Eum_TARGET_FORMAT_CUSTOM        ///< 自定义格式
} Eum_TargetFormat;

/**
 * @brief 目标配置结构体
 * 
 * 定义目标代码生成的完整配置。
 */
typedef struct Stru_TargetConfig_t {
    Eum_TargetPlatform platform;    ///< 目标平台
    Eum_TargetOS os;                ///< 目标操作系统
    Eum_TargetABI abi;              ///< 目标ABI
    Eum_TargetFormat format;        ///< 目标格式
    Eum_TargetCodeType code_type;   ///< 目标代码类型（来自CN_codegen_interface.h）
    
    const char* cpu;                ///< CPU型号（可选）
    const char* features;           ///< CPU特性（可选）
    const char* triple;             ///< LLVM目标三元组（可选）
    
    bool position_independent;      ///< 是否生成位置无关代码
    bool stack_protector;           ///< 是否启用栈保护
    bool omit_frame_pointer;        ///< 是否省略帧指针
    bool optimize_for_size;         ///< 是否优化代码大小
    bool optimize_for_speed;        ///< 是否优化执行速度
    
    int alignment;                  ///< 对齐要求（字节）
    int stack_alignment;            ///< 栈对齐要求（字节）
    int red_zone_size;              ///< 红区大小（字节）
    
    Stru_DynamicArray_t* defines;   ///< 预定义宏
    Stru_DynamicArray_t* includes;  ///< 包含路径
    Stru_DynamicArray_t* libraries; ///< 链接库
    Stru_DynamicArray_t* lib_paths; ///< 库路径
} Stru_TargetConfig_t;

/**
 * @brief 目标代码生成结果结构体
 * 
 * 包含目标代码生成的详细结果。
 */
typedef struct Stru_TargetCodeGenResult_t {
    bool success;                   ///< 是否成功
    
    // 生成的代码
    const char* assembly_code;      ///< 汇编代码（如果生成汇编）
    const char* object_code;        ///< 目标代码（二进制）
    size_t code_size;               ///< 代码大小（字节）
    
    // 元数据
    Stru_DynamicArray_t* symbols;   ///< 符号表
    Stru_DynamicArray_t* relocations; ///< 重定位信息
    Stru_DynamicArray_t* sections;  ///< 段信息
    Stru_DynamicArray_t* debug_info; ///< 调试信息
    
    // 统计信息
    size_t instruction_count;       ///< 指令数量
    size_t data_size;               ///< 数据段大小
    size_t bss_size;                ///< BSS段大小
    size_t total_size;              ///< 总大小
    
    // 错误和警告
    Stru_DynamicArray_t* errors;    ///< 错误信息
    Stru_DynamicArray_t* warnings;  ///< 警告信息
    
    // 性能信息
    double generation_time;         ///< 生成时间（秒）
    size_t memory_used;             ///< 内存使用量（字节）
} Stru_TargetCodeGenResult_t;

/**
 * @brief 目标代码生成器抽象接口结构体
 * 
 * 定义目标代码生成器的完整接口，包含配置、代码生成、链接和资源管理功能。
 * 所有函数指针必须由具体实现提供。
 */
typedef struct Stru_TargetCodeGeneratorInterface_t {
    // ============================================
    // 配置函数
    // ============================================
    
    /**
     * @brief 初始化目标代码生成器
     * 
     * 使用目标配置初始化代码生成器。
     * 
     * @param interface 目标代码生成器接口指针
     * @param config 目标配置
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool (*initialize)(struct Stru_TargetCodeGeneratorInterface_t* interface, const Stru_TargetConfig_t* config);
    
    /**
     * @brief 配置目标平台
     * 
     * 配置目标平台、操作系统和ABI。
     * 
     * @param interface 目标代码生成器接口指针
     * @param platform 目标平台
     * @param os 目标操作系统
     * @param abi 目标ABI
     * @return true 配置成功
     * @return false 配置失败
     */
    bool (*configure_target)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                             Eum_TargetPlatform platform, Eum_TargetOS os, Eum_TargetABI abi);
    
    /**
     * @brief 设置目标三元组
     * 
     * 设置LLVM风格的目标三元组。
     * 
     * @param interface 目标代码生成器接口指针
     * @param triple 目标三元组
     * @return true 设置成功
     * @return false 设置失败
     */
    bool (*set_target_triple)(struct Stru_TargetCodeGeneratorInterface_t* interface, const char* triple);
    
    /**
     * @brief 设置CPU特性
     * 
     * 设置目标CPU型号和特性。
     * 
     * @param interface 目标代码生成器接口指针
     * @param cpu CPU型号
     * @param features CPU特性字符串
     * @return true 设置成功
     * @return false 设置失败
     */
    bool (*set_cpu_features)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                             const char* cpu, const char* features);
    
    // ============================================
    // 代码生成功能
    // ============================================
    
    /**
     * @brief 从IR生成目标代码
     * 
     * 从中间表示生成目标代码。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ir 中间表示
     * @param ir_size 中间表示大小
     * @return Stru_TargetCodeGenResult_t* 生成结果，调用者负责销毁
     */
    Stru_TargetCodeGenResult_t* (*generate_from_ir)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                                    const void* ir, size_t ir_size);
    
    /**
     * @brief 从AST生成目标代码
     * 
     * 直接从AST生成目标代码。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ast AST根节点
     * @return Stru_TargetCodeGenResult_t* 生成结果，调用者负责销毁
     */
    Stru_TargetCodeGenResult_t* (*generate_from_ast)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                                     Stru_AstNode_t* ast);
    
    /**
     * @brief 生成汇编代码
     * 
     * 生成人类可读的汇编代码。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ir 中间表示
     * @param ir_size 中间表示大小
     * @return const char* 汇编代码字符串，调用者不应修改
     */
    const char* (*generate_assembly)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                     const void* ir, size_t ir_size);
    
    /**
     * @brief 生成目标文件
     * 
     * 生成目标文件（.o, .obj等）。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ir 中间表示
     * @param ir_size 中间表示大小
     * @param output_file 输出文件路径
     * @return bool 生成成功
     */
    bool (*generate_object_file)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                 const void* ir, size_t ir_size, const char* output_file);
    
    // ============================================
    // 优化功能
    // ============================================
    
    /**
     * @brief 应用目标特定优化
     * 
     * 应用目标平台特定的优化。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ir 中间表示
     * @param ir_size 中间表示大小
     * @param optimization_level 优化级别
     * @return void* 优化后的中间表示，调用者负责销毁
     */
    void* (*apply_target_optimizations)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                        const void* ir, size_t ir_size, int optimization_level);
    
    /**
     * @brief 指令选择
     * 
     * 执行指令选择，将通用IR转换为目标特定指令。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ir 中间表示
     * @param ir_size 中间表示大小
     * @return void* 目标特定IR，调用者负责销毁
     */
    void* (*select_instructions)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                 const void* ir, size_t ir_size);
    
    /**
     * @brief 寄存器分配
     * 
     * 执行寄存器分配。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ir 中间表示
     * @param ir_size 中间表示大小
     * @return void* 寄存器分配后的IR，调用者负责销毁
     */
    void* (*allocate_registers)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                const void* ir, size_t ir_size);
    
    /**
     * @brief 指令调度
     * 
     * 执行指令调度以优化性能。
     * 
     * @param interface 目标代码生成器接口指针
     * @param ir 中间表示
     * @param ir_size 中间表示大小
     * @return void* 调度后的IR，调用者负责销毁
     */
    void* (*schedule_instructions)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                   const void* ir, size_t ir_size);
    
    // ============================================
    // 链接功能
    // ============================================
    
    /**
     * @brief 链接目标文件
     * 
     * 将多个目标文件链接为可执行文件或库。
     * 
     * @param interface 目标代码生成器接口指针
     * @param object_files 目标文件路径数组
     * @param file_count 文件数量
     * @param output_file 输出文件路径
     * @return bool 链接成功
     */
    bool (*link_objects)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                         const char** object_files, size_t file_count, const char* output_file);
    
    /**
     * @brief 创建静态库
     * 
     * 创建静态库文件。
     * 
     * @param interface 目标代码生成器接口指针
     * @param object_files 目标文件路径数组
     * @param file_count 文件数量
     * @param output_file 输出库文件路径
     * @return bool 创建成功
     */
    bool (*create_static_library)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                  const char** object_files, size_t file_count, const char* output_file);
    
    /**
     * @brief 创建动态库
     * 
     * 创建动态库文件。
     * 
     * @param interface 目标代码生成器接口指针
     * @param object_files 目标文件路径数组
     * @param file_count 文件数量
     * @param output_file 输出库文件路径
     * @return bool 创建成功
     */
    bool (*create_dynamic_library)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                   const char** object_files, size_t file_count, const char* output_file);
    
    // ============================================
    // 查询功能
    // ============================================
    
    /**
     * @brief 获取支持的平台
     * 
     * 获取生成器支持的平台列表。
     * 
     * @param interface 目标代码生成器接口指针
     * @return Stru_DynamicArray_t* 支持的平台数组
     */
    Stru_DynamicArray_t* (*get_supported_platforms)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    /**
     * @brief 获取支持的ABI
     * 
     * 获取生成器支持的ABI列表。
     * 
     * @param interface 目标代码生成器接口指针
     * @return Stru_DynamicArray_t* 支持的ABI数组
     */
    Stru_DynamicArray_t* (*get_supported_abis)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    /**
     * @brief 获取支持的格式
     * 
     * 获取生成器支持的输出格式列表。
     * 
     * @param interface 目标代码生成器接口指针
     * @return Stru_DynamicArray_t* 支持的格式数组
     */
    Stru_DynamicArray_t* (*get_supported_formats)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    /**
     * @brief 检查平台支持
     * 
     * 检查是否支持特定的平台、操作系统和ABI组合。
     * 
     * @param interface 目标代码生成器接口指针
     * @param platform 平台
     * @param os 操作系统
     * @param abi ABI
     * @return bool 是否支持
     */
    bool (*is_platform_supported)(struct Stru_TargetCodeGeneratorInterface_t* interface,
                                  Eum_TargetPlatform platform, Eum_TargetOS os, Eum_TargetABI abi);
    
    // ============================================
    // 错误处理
    // ============================================
    
    /**
     * @brief 检查是否有错误
     * 
     * 检查代码生成过程中是否发生了错误。
     * 
     * @param interface 目标代码生成器接口指针
     * @return true 有错误
     * @return false 没有错误
     */
    bool (*has_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    /**
     * @brief 获取错误信息
     * 
     * 获取所有错误信息的数组。
     * 
     * @param interface 目标代码生成器接口指针
     * @return Stru_DynamicArray_t* 错误信息数组
     */
    Stru_DynamicArray_t* (*get_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    /**
     * @brief 清除错误
     * 
     * 清除所有错误信息。
     * 
     * @param interface 目标代码生成器接口指针
     */
    void (*clear_errors)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 重置目标代码生成器
     * 
     * 重置目标代码生成器到初始状态。
     * 
     * @param interface 目标代码生成器接口指针
     */
    void (*reset)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    /**
     * @brief 销毁目标代码生成器
     * 
     * 释放目标代码生成器占用的所有资源。
     * 
     * @param interface 目标代码生成器接口指针
     */
    void (*destroy)(struct Stru_TargetCodeGeneratorInterface_t* interface);
    
    // ============================================
    // 内部状态（不直接暴露）
    // ============================================
    
    /**
     * @brief 内部状态指针
     * 
     * 具体实现可以使用此指针存储内部状态。
     * 接口使用者不应直接访问此字段。
     */
    void* internal_state;
    
} Stru_TargetCodeGeneratorInterface_t;

// ============================================
// 工厂函数
// ============================================

/**
 * @brief 创建目标代码生成器接口实例
 * 
 * 创建并返回一个新的目标代码生成器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @param platform 目标平台
 * @param os 目标操作系统
 * @return Stru_TargetCodeGeneratorInterface_t* 新创建的目标代码生成器接口实例
 */
Stru_TargetCodeGeneratorInterface_t* F_create_target_codegen_interface(Eum_TargetPlatform platform, Eum_TargetOS os);

/**
 * @brief 创建默认目标配置
 * 
 * 创建并返回默认的目标配置。
 * 
 * @return Stru_TargetConfig_t 默认目标配置
 */
Stru_TargetConfig_t F_create_default_target_config(void);

/**
 * @brief 销毁目标代码生成结果
 * 
 * 释放目标代码生成结果占用的所有资源。
 * 
 * @param result 要销毁的目标代码生成结果
 */
void F_destroy_target_codegen_result(Stru_TargetCodeGenResult_t* result);

/**
 * @brief 目标平台转字符串
 * 
 * 将目标平台转换为可读的字符串表示。
 * 
 * @param platform 目标平台
 * @return const char* 平台字符串表示
 */
const char* F_target_platform_to_string(Eum_TargetPlatform platform);

/**
 * @brief 目标操作系统转字符串
 * 
 * 将目标操作系统转换为可读的字符串表示。
 * 
 * @param os 目标操作系统
 * @return const char* 操作系统字符串表示
 */
const char* F_target_os_to_string(Eum_TargetOS os);

/**
 * @brief 目标ABI转字符串
 * 
 * 将目标ABI转换为可读的字符串表示。
 * 
 * @param abi 目标ABI
 * @return const char* ABI字符串表示
 */
const char* F_target_abi_to_string(Eum_TargetABI abi);

/**
 * @brief 目标格式转字符串
 * 
 * 将目标格式转换为可读的字符串表示。
 * 
 * @param format 目标格式
 * @return const char* 格式字符串表示
 */
const char* F_target_format_to_string(Eum_TargetFormat format);

#endif // CN_TARGET_CODEGEN_INTERFACE_H
