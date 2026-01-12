/******************************************************************************
 * 文件名: CN_bytecode_loader.h
 * 功能: CN_Language 字节码加载器模块头文件
 * 
 * 定义字节码加载器的公共接口，提供字节码加载、解析和验证功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language架构委员会
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_LOADER_H
#define CN_BYTECODE_LOADER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 字节码模块结构体
// ============================================================================

/**
 * @brief 字节码模块结构体
 * 
 * 表示已加载的字节码模块，包含模块信息和字节码数据。
 */
typedef struct Stru_BytecodeModule_t {
    char* name;                     ///< 模块名称
    uint8_t* bytecode_data;         ///< 字节码数据（完整副本）
    size_t bytecode_size;           ///< 字节码大小
    
    // 解析后的段信息
    struct {
        uint32_t magic;             ///< 魔数
        uint16_t version_major;     ///< 主版本号
        uint16_t version_minor;     ///< 次版本号
        uint32_t code_size;         ///< 代码段大小
        uint32_t data_size;         ///< 数据段大小
        uint32_t constant_pool_size;///< 常量池大小
        uint32_t entry_point_offset;///< 入口点偏移
        uint32_t checksum;          ///< 校验和
    } header;                       ///< 文件头信息
    
    const uint8_t* code_segment;    ///< 代码段指针
    const uint8_t* data_segment;    ///< 数据段指针
    const char** constant_pool;     ///< 常量池指针
    size_t constant_count;          ///< 常量池项数
    
    void* user_data;                ///< 用户数据
} Stru_BytecodeModule_t;

// ============================================================================
// 字节码加载器接口函数
// ============================================================================

/**
 * @brief 从内存加载字节码
 * 
 * 从内存缓冲区加载字节码数据，验证格式并解析模块信息。
 * 
 * @param bytecode 字节码数据指针
 * @param bytecode_size 字节码数据大小
 * @param module_name 模块名称（可选）
 * @return Stru_BytecodeModule_t* 加载的字节码模块，失败返回NULL
 */
Stru_BytecodeModule_t* F_load_bytecode_from_memory(const uint8_t* bytecode, 
                                                  size_t bytecode_size,
                                                  const char* module_name);

/**
 * @brief 从文件加载字节码
 * 
 * 从文件加载字节码数据，验证格式并解析模块信息。
 * 
 * @param filename 字节码文件路径
 * @return Stru_BytecodeModule_t* 加载的字节码模块，失败返回NULL
 */
Stru_BytecodeModule_t* F_load_bytecode_from_file(const char* filename);

/**
 * @brief 卸载字节码模块
 * 
 * 释放字节码模块占用的所有资源。
 * 
 * @param module 要卸载的字节码模块
 */
void F_unload_bytecode_module(Stru_BytecodeModule_t* module);

/**
 * @brief 获取模块信息
 * 
 * 获取字节码模块的基本信息。
 * 
 * @param module 字节码模块
 * @param name_buffer 输出参数：模块名称缓冲区
 * @param name_buffer_size 模块名称缓冲区大小
 * @param code_size 输出参数：代码段大小
 * @param data_size 输出参数：数据段大小
 * @param constant_count 输出参数：常量池项数
 * @param entry_point 输出参数：入口点偏移
 */
void F_get_module_info(const Stru_BytecodeModule_t* module,
                      char* name_buffer, size_t name_buffer_size,
                      uint32_t* code_size, uint32_t* data_size,
                      uint32_t* constant_count, uint32_t* entry_point);

/**
 * @brief 验证字节码格式
 * 
 * 验证字节码数据的格式是否正确。
 * 
 * @param bytecode 字节码数据指针
 * @param bytecode_size 字节码数据大小
 * @return 格式正确返回true，否则返回false
 */
bool F_validate_bytecode_format(const uint8_t* bytecode, size_t bytecode_size);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_LOADER_H */
