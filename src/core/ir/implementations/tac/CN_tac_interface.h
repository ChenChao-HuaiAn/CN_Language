/******************************************************************************
 * 文件名: CN_tac_interface.h
 * 功能: CN_Language 三地址码（TAC）接口定义
 * 
 * 定义三地址码（Three-Address Code）的接口和数据结构。
 * TAC是一种中间代码表示形式，每个指令最多包含三个操作数。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_TAC_INTERFACE_H
#define CN_TAC_INTERFACE_H

#include "../../CN_ir_interface.h"

// ============================================================================
// TAC特定类型定义
// ============================================================================

/**
 * @brief TAC操作码枚举
 * 
 * 定义TAC特有的操作码。
 */
typedef enum {
    Eum_TAC_OP_ASSIGN,       ///< 赋值: x = y
    Eum_TAC_OP_ADD,          ///< 加法: x = y + z
    Eum_TAC_OP_SUB,          ///< 减法: x = y - z
    Eum_TAC_OP_MUL,          ///< 乘法: x = y * z
    Eum_TAC_OP_DIV,          ///< 除法: x = y / z
    Eum_TAC_OP_MOD,          ///< 取模: x = y % z
    Eum_TAC_OP_NEG,          ///< 取负: x = -y
    Eum_TAC_OP_NOT,          ///< 逻辑非: x = !y
    Eum_TAC_OP_AND,          ///< 逻辑与: x = y && z
    Eum_TAC_OP_OR,           ///< 逻辑或: x = y || z
    Eum_TAC_OP_EQ,           ///< 等于: x = y == z
    Eum_TAC_OP_NE,           ///< 不等于: x = y != z
    Eum_TAC_OP_LT,           ///< 小于: x = y < z
    Eum_TAC_OP_LE,           ///< 小于等于: x = y <= z
    Eum_TAC_OP_GT,           ///< 大于: x = y > z
    Eum_TAC_OP_GE,           ///< 大于等于: x = y >= z
    Eum_TAC_OP_BIT_AND,      ///< 位与: x = y & z
    Eum_TAC_OP_BIT_OR,       ///< 位或: x = y | z
    Eum_TAC_OP_BIT_XOR,      ///< 位异或: x = y ^ z
    Eum_TAC_OP_BIT_NOT,      ///< 位非: x = ~y
    Eum_TAC_OP_SHL,          ///< 左移: x = y << z
    Eum_TAC_OP_SHR,          ///< 右移: x = y >> z
    Eum_TAC_OP_LOAD,         ///< 加载: x = *y
    Eum_TAC_OP_STORE,        ///< 存储: *x = y
    Eum_TAC_OP_LOAD_ADDR,    ///< 加载地址: x = &y
    Eum_TAC_OP_CALL,         ///< 函数调用: x = call f, args...
    Eum_TAC_OP_RETURN,       ///< 返回: return x
    Eum_TAC_OP_GOTO,         ///< 无条件跳转: goto label
    Eum_TAC_OP_IF,           ///< 条件跳转: if x goto label
    Eum_TAC_OP_IF_FALSE,     ///< 条件跳转（假）: ifFalse x goto label
    Eum_TAC_OP_PARAM,        ///< 参数: param x
    Eum_TAC_OP_LABEL,        ///< 标签: label:
    Eum_TAC_OP_FUNCTION,     ///< 函数定义: function f
    Eum_TAC_OP_END_FUNCTION, ///< 函数结束: end_function
    Eum_TAC_OP_ALLOCA,       ///< 栈分配: x = alloca size
    Eum_TAC_OP_GET_ELEMENT_PTR ///< 获取元素指针: x = &y[z]
} Eum_TacOpcode;

/**
 * @brief TAC指令结构体
 * 
 * TAC特有的指令结构体。
 */
typedef struct {
    Eum_TacOpcode opcode;        ///< TAC操作码
    char* result;                ///< 结果操作数（可为NULL）
    char* operand1;              ///< 第一个操作数（可为NULL）
    char* operand2;              ///< 第二个操作数（可为NULL）
    char* label;                 ///< 标签（用于跳转指令）
    size_t line;                 ///< 源代码行号
    size_t column;               ///< 源代码列号
    void* extra_data;            ///< 额外数据
} Stru_TacInstruction_t;

/**
 * @brief TAC数据结构体
 * 
 * 包含TAC程序的所有信息。
 */
typedef struct {
    Stru_TacInstruction_t** instructions;  ///< 指令数组
    size_t instruction_count;              ///< 指令数量
    size_t capacity;                       ///< 数组容量
    char** temporaries;                    ///< 临时变量列表
    size_t temp_count;                     ///< 临时变量数量
    char** labels;                         ///< 标签列表
    size_t label_count;                    ///< 标签数量
    char* errors;                          ///< 错误信息
    bool has_errors;                       ///< 是否有错误
    void* internal_state;                  ///< 内部状态
} Stru_TacData_t;

// ============================================================================
// TAC接口函数声明
// ============================================================================

/**
 * @brief 创建TAC接口实例
 * 
 * 创建TAC（三地址码）的IR接口实例。
 * 
 * @return Stru_IrInterface_t* TAC接口实例
 */
Stru_IrInterface_t* F_create_tac_interface(void);

/**
 * @brief 创建TAC指令
 * 
 * 创建新的TAC指令。
 * 
 * @param opcode TAC操作码
 * @param result 结果操作数
 * @param operand1 第一个操作数
 * @param operand2 第二个操作数
 * @param label 标签（用于跳转指令）
 * @param line 源代码行号
 * @param column 源代码列号
 * @return Stru_TacInstruction_t* 新创建的TAC指令
 */
Stru_TacInstruction_t* F_create_tac_instruction(Eum_TacOpcode opcode,
                                                const char* result,
                                                const char* operand1,
                                                const char* operand2,
                                                const char* label,
                                                size_t line, size_t column);

/**
 * @brief 销毁TAC指令
 * 
 * 销毁TAC指令及其所有资源。
 * 
 * @param instruction TAC指令
 */
void F_destroy_tac_instruction(Stru_TacInstruction_t* instruction);

/**
 * @brief TAC操作码转字符串
 * 
 * 将TAC操作码转换为可读的字符串表示。
 * 
 * @param opcode TAC操作码
 * @return const char* 操作码字符串表示
 */
const char* F_tac_opcode_to_string(Eum_TacOpcode opcode);

/**
 * @brief 字符串转TAC操作码
 * 
 * 将字符串转换为TAC操作码。
 * 
 * @param str 操作码字符串
 * @return Eum_TacOpcode TAC操作码，无效返回Eum_TAC_OP_ASSIGN
 */
Eum_TacOpcode F_string_to_tac_opcode(const char* str);

/**
 * @brief 格式化TAC指令
 * 
 * 将TAC指令格式化为字符串。
 * 
 * @param instruction TAC指令
 * @return char* 格式化后的字符串，需要调用者释放
 */
char* F_format_tac_instruction(const Stru_TacInstruction_t* instruction);

/**
 * @brief 创建TAC数据
 * 
 * 创建新的TAC数据结构体。
 * 
 * @return Stru_TacData_t* 新创建的TAC数据
 */
Stru_TacData_t* F_create_tac_data(void);

/**
 * @brief 销毁TAC数据
 * 
 * 销毁TAC数据及其所有资源。
 * 
 * @param data TAC数据
 */
void F_destroy_tac_data(Stru_TacData_t* data);

/**
 * @brief 添加TAC指令到数据
 * 
 * 向TAC数据添加指令。
 * 
 * @param data TAC数据
 * @param instruction TAC指令
 * @return bool 成功返回true，失败返回false
 */
bool F_tac_data_add_instruction(Stru_TacData_t* data, Stru_TacInstruction_t* instruction);

/**
 * @brief 从TAC数据获取指令
 * 
 * 从TAC数据获取指定索引的指令。
 * 
 * @param data TAC数据
 * @param index 指令索引
 * @return Stru_TacInstruction_t* TAC指令，索引无效返回NULL
 */
Stru_TacInstruction_t* F_tac_data_get_instruction(Stru_TacData_t* data, size_t index);

/**
 * @brief 获取TAC数据指令数量
 * 
 * 获取TAC数据中的指令数量。
 * 
 * @param data TAC数据
 * @return size_t 指令数量
 */
size_t F_tac_data_get_instruction_count(const Stru_TacData_t* data);

/**
 * @brief 添加错误到TAC数据
 * 
 * 向TAC数据添加错误信息。
 * 
 * @param data TAC数据
 * @param error 错误信息
 */
void F_tac_data_add_error(Stru_TacData_t* data, const char* error);

/**
 * @brief 清除TAC数据错误
 * 
 * 清除TAC数据中的所有错误信息。
 * 
 * @param data TAC数据
 */
void F_tac_data_clear_errors(Stru_TacData_t* data);

/**
 * @brief 检查TAC数据是否有错误
 * 
 * 检查TAC数据是否有错误。
 * 
 * @param data TAC数据
 * @return bool 有错误返回true，否则返回false
 */
bool F_tac_data_has_errors(const Stru_TacData_t* data);

/**
 * @brief 获取TAC数据错误信息
 * 
 * 获取TAC数据的错误信息。
 * 
 * @param data TAC数据
 * @return const char* 错误信息，没有错误返回NULL
 */
const char* F_tac_data_get_errors(const Stru_TacData_t* data);

/**
 * @brief 复制TAC指令
 * 
 * 创建TAC指令的深拷贝。
 * 
 * @param instruction 要复制的TAC指令
 * @return Stru_TacInstruction_t* 新复制的TAC指令，失败返回NULL
 */
Stru_TacInstruction_t* F_copy_tac_instruction(const Stru_TacInstruction_t* instruction);

#endif /* CN_TAC_INTERFACE_H */
