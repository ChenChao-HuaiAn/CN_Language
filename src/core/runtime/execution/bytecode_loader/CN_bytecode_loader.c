/******************************************************************************
 * 文件名: CN_bytecode_loader.c
 * 功能: CN_Language 字节码加载器模块
 * 
 * 负责加载和解析字节码数据，验证字节码格式，准备执行环境。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * 作者: CN_Language架构委员会
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_bytecode_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ============================================================================
// 内部常量定义
// ============================================================================

#define CN_BYTECODE_MAGIC_NUMBER 0x434E4C4E  // "CNLN" ASCII码
#define CN_BYTECODE_VERSION_MAJOR 1
#define CN_BYTECODE_VERSION_MINOR 0

// ============================================================================
// 字节码文件头结构体
// ============================================================================

/**
 * @brief 字节码文件头
 * 
 * 字节码文件的头部信息，包含魔数、版本和段信息。
 */
typedef struct Stru_BytecodeHeader_t {
    uint32_t magic;                 ///< 魔数：0x434E4C4E ("CNLN")
    uint16_t version_major;         ///< 主版本号
    uint16_t version_minor;         ///< 次版本号
    uint32_t code_size;             ///< 代码段大小（字节）
    uint32_t data_size;             ///< 数据段大小（字节）
    uint32_t constant_pool_size;    ///< 常量池大小（字节）
    uint32_t entry_point_offset;    ///< 入口点偏移（指令索引）
    uint32_t checksum;              ///< 校验和
} Stru_BytecodeHeader_t;

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static bool F_validate_header(const Stru_BytecodeHeader_t* header);
static uint32_t F_calculate_checksum(const uint8_t* data, size_t size);

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 从内存加载字节码
 */
Stru_BytecodeModule_t* F_load_bytecode_from_memory(const uint8_t* bytecode, 
                                                  size_t bytecode_size,
                                                  const char* module_name)
{
    if (!bytecode || bytecode_size < sizeof(Stru_BytecodeHeader_t)) {
        return NULL;
    }
    
    // 解析文件头
    const Stru_BytecodeHeader_t* header = (const Stru_BytecodeHeader_t*)bytecode;
    
    // 验证文件头
    if (!F_validate_header(header)) {
        return NULL;
    }
    
    // 验证数据大小
    size_t expected_size = sizeof(Stru_BytecodeHeader_t) + 
                          header->code_size + 
                          header->data_size + 
                          header->constant_pool_size;
    
    if (bytecode_size < expected_size) {
        return NULL;
    }
    
    // 验证校验和
    uint32_t calculated_checksum = F_calculate_checksum(
        bytecode + sizeof(Stru_BytecodeHeader_t),
        bytecode_size - sizeof(Stru_BytecodeHeader_t));
    
    if (calculated_checksum != header->checksum) {
        return NULL;
    }
    
    // 分配模块结构体
    Stru_BytecodeModule_t* module = (Stru_BytecodeModule_t*)malloc(sizeof(Stru_BytecodeModule_t));
    if (!module) {
        return NULL;
    }
    
    memset(module, 0, sizeof(Stru_BytecodeModule_t));
    
    // 设置模块信息
    if (module_name) {
        module->name = strdup(module_name);
    } else {
        module->name = strdup("anonymous");
    }
    
    // 设置字节码信息
    module->bytecode_data = (uint8_t*)malloc(bytecode_size);
    if (!module->bytecode_data) {
        free(module->name);
        free(module);
        return NULL;
    }
    
    memcpy(module->bytecode_data, bytecode, bytecode_size);
    module->bytecode_size = bytecode_size;
    
    // 解析段信息
    module->header.magic = header->magic;
    module->header.version_major = header->version_major;
    module->header.version_minor = header->version_minor;
    module->header.code_size = header->code_size;
    module->header.data_size = header->data_size;
    module->header.constant_pool_size = header->constant_pool_size;
    module->header.entry_point_offset = header->entry_point_offset;
    module->header.checksum = header->checksum;
    
    // 计算段指针
    const uint8_t* data_ptr = bytecode + sizeof(Stru_BytecodeHeader_t);
    module->code_segment = data_ptr;
    module->data_segment = data_ptr + header->code_size;
    module->constant_pool = (const char**)(data_ptr + header->code_size + header->data_size);
    
    // 计算常量池项数
    module->constant_count = header->constant_pool_size / sizeof(const char*);
    
    return module;
}

/**
 * @brief 从文件加载字节码
 */
Stru_BytecodeModule_t* F_load_bytecode_from_file(const char* filename)
{
    if (!filename) {
        return NULL;
    }
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return NULL;
    }
    
    // 读取文件内容
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        return NULL;
    }
    
    // 从内存加载
    Stru_BytecodeModule_t* module = F_load_bytecode_from_memory(buffer, file_size, filename);
    free(buffer);
    
    return module;
}

/**
 * @brief 卸载字节码模块
 */
void F_unload_bytecode_module(Stru_BytecodeModule_t* module)
{
    if (!module) {
        return;
    }
    
    if (module->name) {
        free(module->name);
    }
    
    if (module->bytecode_data) {
        free(module->bytecode_data);
    }
    
    free(module);
}

/**
 * @brief 获取模块信息
 */
void F_get_module_info(const Stru_BytecodeModule_t* module,
                      char* name_buffer, size_t name_buffer_size,
                      uint32_t* code_size, uint32_t* data_size,
                      uint32_t* constant_count, uint32_t* entry_point)
{
    if (!module) {
        return;
    }
    
    if (name_buffer && name_buffer_size > 0) {
        strncpy(name_buffer, module->name, name_buffer_size - 1);
        name_buffer[name_buffer_size - 1] = '\0';
    }
    
    if (code_size) {
        *code_size = module->header.code_size;
    }
    
    if (data_size) {
        *data_size = module->header.data_size;
    }
    
    if (constant_count) {
        *constant_count = module->constant_count;
    }
    
    if (entry_point) {
        *entry_point = module->header.entry_point_offset;
    }
}

/**
 * @brief 验证字节码格式
 */
bool F_validate_bytecode_format(const uint8_t* bytecode, size_t bytecode_size)
{
    if (!bytecode || bytecode_size < sizeof(Stru_BytecodeHeader_t)) {
        return false;
    }
    
    const Stru_BytecodeHeader_t* header = (const Stru_BytecodeHeader_t*)bytecode;
    return F_validate_header(header);
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 验证文件头
 */
static bool F_validate_header(const Stru_BytecodeHeader_t* header)
{
    if (!header) {
        return false;
    }
    
    // 检查魔数
    if (header->magic != CN_BYTECODE_MAGIC_NUMBER) {
        return false;
    }
    
    // 检查版本号
    if (header->version_major != CN_BYTECODE_VERSION_MAJOR ||
        header->version_minor > CN_BYTECODE_VERSION_MINOR) {
        return false;
    }
    
    // 检查段大小（简单验证）
    if (header->code_size == 0) {
        return false;  // 代码段不能为空
    }
    
    return true;
}

/**
 * @brief 计算校验和
 */
static uint32_t F_calculate_checksum(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return 0;
    }
    
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 3) ^ data[i] ^ (checksum >> 29);
    }
    
    return checksum;
}
