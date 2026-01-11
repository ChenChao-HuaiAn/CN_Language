/******************************************************************************
 * 文件名: CN_cfg_basic_block.h
 * 功能: CN_Language 控制流图基本块数据结构定义
 * 
 * 定义控制流图的基本块数据结构，包括基本块属性、前驱后继关系、
 * 支配关系和数据流信息。
 * 
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_CFG_BASIC_BLOCK_H
#define CN_CFG_BASIC_BLOCK_H

#include <stddef.h>
#include <stdbool.h>
#include "../../CN_ir_interface.h"

// ============================================================================
// 类型定义
// ============================================================================

/**
 * @brief 基本块类型枚举
 * 
 * 定义基本块的类型，用于区分不同功能的基本块。
 */
typedef enum {
    Eum_BASIC_BLOCK_NORMAL,      ///< 普通基本块
    Eum_BASIC_BLOCK_ENTRY,       ///< 入口基本块
    Eum_BASIC_BLOCK_EXIT,        ///< 出口基本块
    Eum_BASIC_BLOCK_LOOP_HEADER, ///< 循环头基本块
    Eum_BASIC_BLOCK_LOOP_BODY,   ///< 循环体基本块
    Eum_BASIC_BLOCK_LOOP_EXIT,   ///< 循环退出基本块
    Eum_BASIC_BLOCK_IF_THEN,     ///< if-then分支基本块
    Eum_BASIC_BLOCK_IF_ELSE,     ///< if-else分支基本块
    Eum_BASIC_BLOCK_SWITCH_CASE, ///< switch-case基本块
    Eum_BASIC_BLOCK_HANDLER      ///< 异常处理基本块
} Eum_BasicBlockType;

/**
 * @brief 基本块结构体
 * 
 * 定义控制流图的基本块，包含指令序列、前驱后继关系、
 * 支配关系和数据流信息。
 */
typedef struct Stru_BasicBlock_t {
    // 基本信息
    char* name;                         ///< 基本块名称
    Eum_BasicBlockType type;            ///< 基本块类型
    size_t id;                          ///< 基本块ID（唯一标识）
    
    // 指令序列
    Stru_IrInstruction_t** instructions; ///< 指令数组
    size_t instruction_count;           ///< 指令数量
    size_t instruction_capacity;        ///< 指令数组容量
    
    // 控制流关系
    struct Stru_BasicBlock_t** predecessors; ///< 前驱基本块数组
    size_t predecessor_count;           ///< 前驱基本块数量
    size_t predecessor_capacity;        ///< 前驱数组容量
    
    struct Stru_BasicBlock_t** successors;   ///< 后继基本块数组
    size_t successor_count;             ///< 后继基本块数量
    size_t successor_capacity;          ///< 后继数组容量
    
    // 支配关系
    struct Stru_BasicBlock_t** dominators;   ///< 支配者集合
    size_t dominator_count;             ///< 支配者数量
    size_t dominator_capacity;          ///< 支配者数组容量
    
    struct Stru_BasicBlock_t* immediate_dominator; ///< 直接支配者
    
    struct Stru_BasicBlock_t** dominated;    ///< 被支配者集合
    size_t dominated_count;             ///< 被支配者数量
    size_t dominated_capacity;          ///< 被支配者数组容量
    
    // 数据流信息
    void* in_set;                       ///< 数据流分析入集
    void* out_set;                      ///< 数据流分析出集
    void* gen_set;                      ///< 生成集合
    void* kill_set;                     ///< 杀死集合
    
    // 循环信息
    size_t loop_depth;                  ///< 循环嵌套深度
    struct Stru_BasicBlock_t* loop_header; ///< 所属循环头
    size_t loop_id;                     ///< 循环ID
    
    // 调试信息
    size_t start_line;                  ///< 起始源代码行号
    size_t end_line;                    ///< 结束源代码行号
    size_t start_column;                ///< 起始源代码列号
    size_t end_column;                  ///< 结束源代码列号
    
    // 内部状态
    void* internal_data;                ///< 内部数据（用于算法临时存储）
    bool visited;                       ///< 访问标记（用于遍历算法）
} Stru_BasicBlock_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建基本块
 * 
 * 创建新的基本块实例。
 * 
 * @param name 基本块名称（可空，自动生成）
 * @param type 基本块类型
 * @param id 基本块ID（如果为0，自动生成）
 * @return Stru_BasicBlock_t* 新创建的基本块，失败返回NULL
 */
Stru_BasicBlock_t* F_create_basic_block(const char* name, 
                                        Eum_BasicBlockType type, 
                                        size_t id);

/**
 * @brief 销毁基本块
 * 
 * 销毁基本块及其所有资源。
 * 
 * @param block 基本块
 */
void F_destroy_basic_block(Stru_BasicBlock_t* block);

/**
 * @brief 复制基本块
 * 
 * 创建基本块的深拷贝。
 * 
 * @param src 源基本块
 * @return Stru_BasicBlock_t* 新创建的基本块副本，失败返回NULL
 */
Stru_BasicBlock_t* F_copy_basic_block(const Stru_BasicBlock_t* src);

// ============================================================================
// 基本块操作函数
// ============================================================================

/**
 * @brief 添加指令到基本块
 * 
 * 向基本块添加指令。
 * 
 * @param block 基本块
 * @param instruction 指令
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_add_instruction(Stru_BasicBlock_t* block, 
                                   Stru_IrInstruction_t* instruction);

/**
 * @brief 从基本块移除指令
 * 
 * 从基本块移除指定索引的指令。
 * 
 * @param block 基本块
 * @param index 指令索引
 * @return Stru_IrInstruction_t* 被移除的指令，失败返回NULL
 */
Stru_IrInstruction_t* F_basic_block_remove_instruction(Stru_BasicBlock_t* block, 
                                                       size_t index);

/**
 * @brief 获取基本块指令
 * 
 * 获取基本块的指定指令。
 * 
 * @param block 基本块
 * @param index 指令索引
 * @return Stru_IrInstruction_t* 指令，索引无效返回NULL
 */
Stru_IrInstruction_t* F_basic_block_get_instruction(const Stru_BasicBlock_t* block, 
                                                    size_t index);

/**
 * @brief 清空基本块指令
 * 
 * 清空基本块的所有指令。
 * 
 * @param block 基本块
 */
void F_basic_block_clear_instructions(Stru_BasicBlock_t* block);

// ============================================================================
// 控制流关系函数
// ============================================================================

/**
 * @brief 添加前驱基本块
 * 
 * 向基本块添加前驱基本块。
 * 
 * @param block 基本块
 * @param predecessor 前驱基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_add_predecessor(Stru_BasicBlock_t* block, 
                                   Stru_BasicBlock_t* predecessor);

/**
 * @brief 移除前驱基本块
 * 
 * 从基本块移除前驱基本块。
 * 
 * @param block 基本块
 * @param predecessor 前驱基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_remove_predecessor(Stru_BasicBlock_t* block, 
                                      Stru_BasicBlock_t* predecessor);

/**
 * @brief 添加后继基本块
 * 
 * 向基本块添加后继基本块。
 * 
 * @param block 基本块
 * @param successor 后继基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_add_successor(Stru_BasicBlock_t* block, 
                                 Stru_BasicBlock_t* successor);

/**
 * @brief 移除后继基本块
 * 
 * 从基本块移除后继基本块。
 * 
 * @param block 基本块
 * @param successor 后继基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_remove_successor(Stru_BasicBlock_t* block, 
                                    Stru_BasicBlock_t* successor);

/**
 * @brief 检查基本块是否为前驱
 * 
 * 检查指定基本块是否为当前基本块的前驱。
 * 
 * @param block 基本块
 * @param predecessor 待检查的前驱基本块
 * @return bool 是前驱返回true，否则返回false
 */
bool F_basic_block_is_predecessor(const Stru_BasicBlock_t* block, 
                                  const Stru_BasicBlock_t* predecessor);

/**
 * @brief 检查基本块是否为后继
 * 
 * 检查指定基本块是否为当前基本块的后继。
 * 
 * @param block 基本块
 * @param successor 待检查的后继基本块
 * @return bool 是后继返回true，否则返回false
 */
bool F_basic_block_is_successor(const Stru_BasicBlock_t* block, 
                                const Stru_BasicBlock_t* successor);

// ============================================================================
// 支配关系函数
// ============================================================================

/**
 * @brief 添加支配者
 * 
 * 向基本块添加支配者。
 * 
 * @param block 基本块
 * @param dominator 支配者基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_add_dominator(Stru_BasicBlock_t* block, 
                                 Stru_BasicBlock_t* dominator);

/**
 * @brief 移除支配者
 * 
 * 从基本块移除支配者。
 * 
 * @param block 基本块
 * @param dominator 支配者基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_remove_dominator(Stru_BasicBlock_t* block, 
                                    Stru_BasicBlock_t* dominator);

/**
 * @brief 添加被支配者
 * 
 * 向基本块添加被支配者。
 * 
 * @param block 基本块
 * @param dominated 被支配者基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_add_dominated(Stru_BasicBlock_t* block, 
                                 Stru_BasicBlock_t* dominated);

/**
 * @brief 移除被支配者
 * 
 * 从基本块移除被支配者。
 * 
 * @param block 基本块
 * @param dominated 被支配者基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_remove_dominated(Stru_BasicBlock_t* block, 
                                    Stru_BasicBlock_t* dominated);

/**
 * @brief 检查基本块是否支配另一个基本块
 * 
 * 检查当前基本块是否支配指定基本块。
 * 
 * @param dominator 支配者基本块
 * @param dominated 被支配者基本块
 * @return bool 支配返回true，否则返回false
 */
bool F_basic_block_dominates(const Stru_BasicBlock_t* dominator, 
                             const Stru_BasicBlock_t* dominated);

/**
 * @brief 检查基本块是否严格支配另一个基本块
 * 
 * 检查当前基本块是否严格支配指定基本块（不包括自身）。
 * 
 * @param dominator 支配者基本块
 * @param dominated 被支配者基本块
 * @return bool 严格支配返回true，否则返回false
 */
bool F_basic_block_strictly_dominates(const Stru_BasicBlock_t* dominator, 
                                      const Stru_BasicBlock_t* dominated);

// ============================================================================
// 查询函数
// ============================================================================

/**
 * @brief 获取基本块名称
 * 
 * 获取基本块的名称。
 * 
 * @param block 基本块
 * @return const char* 基本块名称
 */
const char* F_basic_block_get_name(const Stru_BasicBlock_t* block);

/**
 * @brief 设置基本块名称
 * 
 * 设置基本块的名称。
 * 
 * @param block 基本块
 * @param name 新名称
 * @return bool 成功返回true，失败返回false
 */
bool F_basic_block_set_name(Stru_BasicBlock_t* block, const char* name);

/**
 * @brief 获取基本块类型
 * 
 * 获取基本块的类型。
 * 
 * @param block 基本块
 * @return Eum_BasicBlockType 基本块类型
 */
Eum_BasicBlockType F_basic_block_get_type(const Stru_BasicBlock_t* block);

/**
 * @brief 设置基本块类型
 * 
 * 设置基本块的类型。
 * 
 * @param block 基本块
 * @param type 新类型
 */
void F_basic_block_set_type(Stru_BasicBlock_t* block, Eum_BasicBlockType type);

/**
 * @brief 获取基本块ID
 * 
 * 获取基本块的ID。
 * 
 * @param block 基本块
 * @return size_t 基本块ID
 */
size_t F_basic_block_get_id(const Stru_BasicBlock_t* block);

/**
 * @brief 检查基本块是否为空
 * 
 * 检查基本块是否为空（无指令）。
 * 
 * @param block 基本块
 * @return bool 为空返回true，否则返回false
 */
bool F_basic_block_is_empty(const Stru_BasicBlock_t* block);

/**
 * @brief 检查基本块是否只有跳转指令
 * 
 * 检查基本块是否只包含跳转指令。
 * 
 * @param block 基本块
 * @return bool 只有跳转指令返回true，否则返回false
 */
bool F_basic_block_is_jump_only(const Stru_BasicBlock_t* block);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 基本块类型转字符串
 * 
 * 将基本块类型转换为可读的字符串表示。
 * 
 * @param type 基本块类型
 * @return const char* 类型字符串表示
 */
const char* F_basic_block_type_to_string(Eum_BasicBlockType type);

/**
 * @brief 生成基本块名称
 * 
 * 根据基本块ID生成默认名称。
 * 
 * @param id 基本块ID
 * @return char* 生成的名称，调用者负责释放
 */
char* F_generate_basic_block_name(size_t id);

/**
 * @brief 重置基本块访问标记
 * 
 * 重置基本块的访问标记。
 * 
 * @param block 基本块
 */
void F_basic_block_reset_visited(Stru_BasicBlock_t* block);

/**
 * @brief 设置基本块访问标记
 * 
 * 设置基本块的访问标记。
 * 
 * @param block 基本块
 * @param visited 访问标记
 */
void F_basic_block_set_visited(Stru_BasicBlock_t* block, bool visited);

/**
 * @brief 检查基本块是否已访问
 * 
 * 检查基本块是否已访问。
 * 
 * @param block 基本块
 * @return bool 已访问返回true，否则返回false
 */
bool F_basic_block_is_visited(const Stru_BasicBlock_t* block);

// ============================================================================
// 验证函数
// ============================================================================

/**
 * @brief 验证基本块
 * 
 * 验证基本块的完整性。
 * 
 * @param block 基本块
 * @param error_message 输出参数，错误信息
 * @return bool 有效返回true，无效返回false
 */
bool F_validate_basic_block(const Stru_BasicBlock_t* block, char** error_message);

#endif /* CN_CFG_BASIC_BLOCK_H */
