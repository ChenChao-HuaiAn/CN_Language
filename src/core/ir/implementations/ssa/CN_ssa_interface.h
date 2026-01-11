/******************************************************************************
 * 文件名: CN_ssa_interface.h
 * 功能: CN_Language 静态单赋值形式（SSA）接口定义
 * 
 * 定义静态单赋值形式（Static Single Assignment Form）的接口和数据结构。
 * SSA是一种中间代码表示形式，每个变量只被赋值一次，便于优化。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_SSA_INTERFACE_H
#define CN_SSA_INTERFACE_H

#include "../../CN_ir_interface.h"

// ============================================================================
// SSA特定类型定义
// ============================================================================

/**
 * @brief SSA操作码枚举
 * 
 * 定义SSA特有的操作码。
 */
typedef enum {
    Eum_SSA_OP_PHI,              ///< φ函数: x = φ(x1, x2, ..., xn)
    Eum_SSA_OP_COPY,             ///< 复制: x = y
    Eum_SSA_OP_ADD,              ///< 加法: x = y + z
    Eum_SSA_OP_SUB,              ///< 减法: x = y - z
    Eum_SSA_OP_MUL,              ///< 乘法: x = y * z
    Eum_SSA_OP_DIV,              ///< 除法: x = y / z
    Eum_SSA_OP_MOD,              ///< 取模: x = y % z
    Eum_SSA_OP_NEG,              ///< 取负: x = -y
    Eum_SSA_OP_NOT,              ///< 逻辑非: x = !y
    Eum_SSA_OP_AND,              ///< 逻辑与: x = y && z
    Eum_SSA_OP_OR,               ///< 逻辑或: x = y || z
    Eum_SSA_OP_EQ,               ///< 等于: x = y == z
    Eum_SSA_OP_NE,               ///< 不等于: x = y != z
    Eum_SSA_OP_LT,               ///< 小于: x = y < z
    Eum_SSA_OP_LE,               ///< 小于等于: x = y <= z
    Eum_SSA_OP_GT,               ///< 大于: x = y > z
    Eum_SSA_OP_GE,               ///< 大于等于: x = y >= z
    Eum_SSA_OP_BIT_AND,          ///< 位与: x = y & z
    Eum_SSA_OP_BIT_OR,           ///< 位或: x = y | z
    Eum_SSA_OP_BIT_XOR,          ///< 位异或: x = y ^ z
    Eum_SSA_OP_BIT_NOT,          ///< 位非: x = ~y
    Eum_SSA_OP_SHL,              ///< 左移: x = y << z
    Eum_SSA_OP_SHR,              ///< 右移: x = y >> z
    Eum_SSA_OP_LOAD,             ///< 加载: x = *y
    Eum_SSA_OP_STORE,            ///< 存储: *x = y
    Eum_SSA_OP_LOAD_ADDR,        ///< 加载地址: x = &y
    Eum_SSA_OP_CALL,             ///< 函数调用: x = call f(args...)
    Eum_SSA_OP_RETURN,           ///< 返回: return x
    Eum_SSA_OP_BRANCH,           ///< 条件分支: br cond, label1, label2
    Eum_SSA_OP_JUMP,             ///< 无条件跳转: jump label
    Eum_SSA_OP_LABEL,            ///< 标签: label:
    Eum_SSA_OP_FUNCTION,         ///< 函数定义: function f
    Eum_SSA_OP_END_FUNCTION,     ///< 函数结束: end_function
    Eum_SSA_OP_ALLOCA,           ///< 栈分配: x = alloca size
    Eum_SSA_OP_GET_ELEMENT_PTR   ///< 获取元素指针: x = &y[z]
} Eum_SsaOpcode;

/**
 * @brief SSA变量结构体
 * 
 * SSA特有的变量结构体，包含版本信息。
 */
typedef struct {
    char* name;                 ///< 变量名（不含版本号）
    int version;                ///< 版本号（从0开始）
    char* full_name;            ///< 完整变量名（包含版本号）
    bool is_temporary;          ///< 是否为临时变量
    size_t definition_point;    ///< 定义点（指令索引）
    void* type_info;            ///< 类型信息
} Stru_SsaVariable_t;

/**
 * @brief φ函数参数结构体
 * 
 * φ函数的参数结构体，包含变量和来源基本块。
 */
typedef struct {
    Stru_SsaVariable_t* variable;   ///< 变量
    char* block_name;               ///< 来源基本块名称
    size_t block_index;             ///< 来源基本块索引
} Stru_PhiArgument_t;

/**
 * @brief SSA指令结构体
 * 
 * SSA特有的指令结构体。
 */
typedef struct {
    Eum_SsaOpcode opcode;           ///< SSA操作码
    Stru_SsaVariable_t* result;     ///< 结果变量（可为NULL）
    Stru_SsaVariable_t** operands;  ///< 操作数变量数组
    size_t operand_count;           ///< 操作数数量
    Stru_PhiArgument_t** phi_args;  ///< φ函数参数数组（仅用于φ指令）
    size_t phi_arg_count;           ///< φ函数参数数量
    char** labels;                  ///< 标签数组（用于跳转指令）
    size_t label_count;             ///< 标签数量
    size_t line;                    ///< 源代码行号
    size_t column;                  ///< 源代码列号
    void* extra_data;               ///< 额外数据
} Stru_SsaInstruction_t;

/**
 * @brief SSA基本块结构体
 * 
 * SSA特有的基本块结构体，包含φ函数。
 */
typedef struct {
    char* name;                     ///< 基本块名称
    Stru_SsaInstruction_t** instructions;  ///< 指令数组
    size_t instruction_count;       ///< 指令数量
    Stru_SsaInstruction_t** phi_functions; ///< φ函数数组（基本块开头）
    size_t phi_count;               ///< φ函数数量
    
    // 控制流信息
    struct Stru_SsaBasicBlock_t** predecessors;  ///< 前驱基本块
    size_t predecessor_count;       ///< 前驱数量
    struct Stru_SsaBasicBlock_t** successors;    ///< 后继基本块
    size_t successor_count;         ///< 后继数量
    
    // 支配关系
    struct Stru_SsaBasicBlock_t** dominators;    ///< 支配者集合
    size_t dominator_count;         ///< 支配者数量
    struct Stru_SsaBasicBlock_t* immediate_dominator; ///< 直接支配者
    
    // 数据流信息
    void* in_set;                   ///< 到达定义入集
    void* out_set;                  ///< 到达定义出集
    void* def_set;                  ///< 定义集合
    void* use_set;                  ///< 使用集合
} Stru_SsaBasicBlock_t;

/**
 * @brief SSA函数结构体
 * 
 * SSA特有的函数结构体。
 */
typedef struct {
    char* name;                     ///< 函数名
    Stru_SsaBasicBlock_t** blocks;  ///< 基本块数组
    size_t block_count;             ///< 基本块数量
    Stru_SsaBasicBlock_t* entry_block; ///< 入口基本块
    Stru_SsaBasicBlock_t* exit_block;  ///< 出口基本块
    
    // 变量信息
    Stru_SsaVariable_t** parameters; ///< 参数变量数组
    size_t param_count;             ///< 参数数量
    Stru_SsaVariable_t** local_vars; ///< 局部变量数组
    size_t local_var_count;         ///< 局部变量数量
    Stru_SsaVariable_t** temporaries; ///< 临时变量数组
    size_t temp_count;              ///< 临时变量数量
    
    // 控制流图
    void* cfg;                      ///< 控制流图
} Stru_SsaFunction_t;

/**
 * @brief SSA数据结构体
 * 
 * 包含SSA程序的所有信息。
 */
typedef struct {
    Stru_SsaFunction_t** functions; ///< 函数数组
    size_t function_count;          ///< 函数数量
    size_t capacity;                ///< 数组容量
    
    // 全局信息
    Stru_SsaVariable_t** global_vars; ///< 全局变量数组
    size_t global_var_count;       ///< 全局变量数量
    
    // 错误处理
    char* errors;                  ///< 错误信息
    bool has_errors;               ///< 是否有错误
    
    // 内部状态
    void* internal_state;          ///< 内部状态
} Stru_SsaData_t;

// ============================================================================
// SSA接口函数声明
// ============================================================================

/**
 * @brief 创建SSA接口实例
 * 
 * 创建SSA（静态单赋值形式）的IR接口实例。
 * 
 * @return Stru_IrInterface_t* SSA接口实例
 */
Stru_IrInterface_t* F_create_ssa_interface(void);

/**
 * @brief 创建SSA变量
 * 
 * 创建新的SSA变量。
 * 
 * @param name 变量名（不含版本号）
 * @param version 版本号
 * @param is_temporary 是否为临时变量
 * @return Stru_SsaVariable_t* 新创建的SSA变量
 */
Stru_SsaVariable_t* F_create_ssa_variable(const char* name, int version, bool is_temporary);

/**
 * @brief 销毁SSA变量
 * 
 * 销毁SSA变量及其所有资源。
 * 
 * @param variable SSA变量
 */
void F_destroy_ssa_variable(Stru_SsaVariable_t* variable);

/**
 * @brief 获取SSA变量的完整名称
 * 
 * 获取包含版本号的SSA变量完整名称。
 * 
 * @param variable SSA变量
 * @return const char* 完整变量名
 */
const char* F_ssa_variable_get_full_name(const Stru_SsaVariable_t* variable);

/**
 * @brief 创建SSA指令
 * 
 * 创建新的SSA指令。
 * 
 * @param opcode SSA操作码
 * @param result 结果变量（可为NULL）
 * @param line 源代码行号
 * @param column 源代码列号
 * @return Stru_SsaInstruction_t* 新创建的SSA指令
 */
Stru_SsaInstruction_t* F_create_ssa_instruction(Eum_SsaOpcode opcode,
                                                Stru_SsaVariable_t* result,
                                                size_t line, size_t column);

/**
 * @brief 销毁SSA指令
 * 
 * 销毁SSA指令及其所有资源。
 * 
 * @param instruction SSA指令
 */
void F_destroy_ssa_instruction(Stru_SsaInstruction_t* instruction);

/**
 * @brief 添加操作数到SSA指令
 * 
 * 向SSA指令添加操作数变量。
 * 
 * @param instruction SSA指令
 * @param operand 操作数变量
 * @return bool 成功返回true，失败返回false
 */
bool F_ssa_instruction_add_operand(Stru_SsaInstruction_t* instruction, 
                                   Stru_SsaVariable_t* operand);

/**
 * @brief 添加φ参数到SSA指令
 * 
 * 向φ指令添加参数。
 * 
 * @param instruction φ指令
 * @param variable 变量
 * @param block_name 来源基本块名称
 * @param block_index 来源基本块索引
 * @return bool 成功返回true，失败返回false
 */
bool F_ssa_instruction_add_phi_argument(Stru_SsaInstruction_t* instruction,
                                        Stru_SsaVariable_t* variable,
                                        const char* block_name,
                                        size_t block_index);

/**
 * @brief 添加标签到SSA指令
 * 
 * 向跳转指令添加标签。
 * 
 * @param instruction SSA指令
 * @param label 标签
 * @return bool 成功返回true，失败返回false
 */
bool F_ssa_instruction_add_label(Stru_SsaInstruction_t* instruction, const char* label);

/**
 * @brief SSA操作码转字符串
 * 
 * 将SSA操作码转换为可读的字符串表示。
 * 
 * @param opcode SSA操作码
 * @return const char* 操作码字符串表示
 */
const char* F_ssa_opcode_to_string(Eum_SsaOpcode opcode);

/**
 * @brief 字符串转SSA操作码
 * 
 * 将字符串转换为SSA操作码。
 * 
 * @param str 操作码字符串
 * @return Eum_SsaOpcode SSA操作码，无效返回Eum_SSA_OP_COPY
 */
Eum_SsaOpcode F_string_to_ssa_opcode(const char* str);

/**
 * @brief 格式化SSA指令
 * 
 * 将SSA指令格式化为字符串。
 * 
 * @param instruction SSA指令
 * @return char* 格式化后的字符串，需要调用者释放
 */
char* F_format_ssa_instruction(const Stru_SsaInstruction_t* instruction);

/**
 * @brief 创建SSA数据
 * 
 * 创建新的SSA数据结构体。
 * 
 * @return Stru_SsaData_t* 新创建的SSA数据
 */
Stru_SsaData_t* F_create_ssa_data(void);

/**
 * @brief 销毁SSA数据
 * 
 * 销毁SSA数据及其所有资源。
 * 
 * @param data SSA数据
 */
void F_destroy_ssa_data(Stru_SsaData_t* data);

/**
 * @brief 添加函数到SSA数据
 * 
 * 向SSA数据添加函数。
 * 
 * @param data SSA数据
 * @param function SSA函数
 * @return bool 成功返回true，失败返回false
 */
bool F_ssa_data_add_function(Stru_SsaData_t* data, Stru_SsaFunction_t* function);

/**
 * @brief 从SSA数据获取函数
 * 
 * 从SSA数据获取指定索引的函数。
 * 
 * @param data SSA数据
 * @param index 函数索引
 * @return Stru_SsaFunction_t* SSA函数，索引无效返回NULL
 */
Stru_SsaFunction_t* F_ssa_data_get_function(Stru_SsaData_t* data, size_t index);

/**
 * @brief 获取SSA数据函数数量
 * 
 * 获取SSA数据中的函数数量。
 * 
 * @param data SSA数据
 * @return size_t 函数数量
 */
size_t F_ssa_data_get_function_count(const Stru_SsaData_t* data);

/**
 * @brief 添加错误到SSA数据
 * 
 * 向SSA数据添加错误信息。
 * 
 * @param data SSA数据
 * @param error 错误信息
 */
void F_ssa_data_add_error(Stru_SsaData_t* data, const char* error);

/**
 * @brief 清除SSA数据错误
 * 
 * 清除SSA数据中的所有错误信息。
 * 
 * @param data SSA数据
 */
void F_ssa_data_clear_errors(Stru_SsaData_t* data);

/**
 * @brief 检查SSA数据是否有错误
 * 
 * 检查SSA数据是否有错误。
 * 
 * @param data SSA数据
 * @return bool 有错误返回true，否则返回false
 */
bool F_ssa_data_has_errors(const Stru_SsaData_t* data);

/**
 * @brief 获取SSA数据错误信息
 * 
 * 获取SSA数据的错误信息。
 * 
 * @param data SSA数据
 * @return const char* 错误信息，没有错误返回NULL
 */
const char* F_ssa_data_get_errors(const Stru_SsaData_t* data);

// ============================================================================
// SSA基本块函数声明
// ============================================================================

/**
 * @brief 创建SSA基本块
 * 
 * 创建新的SSA基本块。
 * 
 * @param name 基本块名称
 * @return Stru_SsaBasicBlock_t* 新创建的SSA基本块
 */
Stru_SsaBasicBlock_t* F_create_ssa_basic_block(const char* name);

/**
 * @brief 销毁SSA基本块
 * 
 * 销毁SSA基本块及其所有资源。
 * 
 * @param block SSA基本块
 */
void F_destroy_ssa_basic_block(Stru_SsaBasicBlock_t* block);

/**
 * @brief 添加指令到SSA基本块
 * 
 * 向SSA基本块添加指令。
 * 
 * @param block SSA基本块
 * @param instruction SSA指令
 * @return bool 成功返回true，失败返回false
 */
bool F_ssa_basic_block_add_instruction(Stru_SsaBasicBlock_t* block, 
                                       Stru_SsaInstruction_t* instruction);

// ============================================================================
// SSA函数函数声明
// ============================================================================

/**
 * @brief 创建SSA函数
 * 
 * 创建新的SSA函数。
 * 
 * @param name 函数名
 * @return Stru_SsaFunction_t* 新创建的SSA函数
 */
Stru_SsaFunction_t* F_create_ssa_function(const char* name);

/**
 * @brief 销毁SSA函数
 * 
 * 销毁SSA函数及其所有资源。
 * 
 * @param function SSA函数
 */
void F_destroy_ssa_function(Stru_SsaFunction_t* function);

/**
 * @brief 添加基本块到SSA函数
 * 
 * 向SSA函数添加基本块。
 * 
 * @param function SSA函数
 * @param block SSA基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_ssa_function_add_block(Stru_SsaFunction_t* function, Stru_SsaBasicBlock_t* block);

// ============================================================================
// SSA转换函数声明
// ============================================================================

/**
 * @brief 从TAC转换为SSA
 * 
 * 将三地址码（TAC）转换为静态单赋值形式（SSA）。
 * 
 * @param tac_data TAC数据
 * @return Stru_SsaData_t* 转换后的SSA数据，失败返回NULL
 */
Stru_SsaData_t* F_convert_tac_to_ssa(const void* tac_data);

/**
 * @brief 从AST转换为SSA
 * 
 * 将抽象语法树（AST）直接转换为静态单赋值形式（SSA）。
 * 
 * @param ast AST根节点
 * @return Stru_SsaData_t* 转换后的SSA数据，失败返回NULL
 */
Stru_SsaData_t* F_convert_ast_to_ssa(Stru_AstNode_t* ast);

/**
 * @brief 插入φ函数
 * 
 * 在控制流图的汇合点插入φ函数。
 * 
 * @param ssa_data SSA数据
 * @return bool 成功返回true，失败返回false
 */
bool F_insert_phi_functions(Stru_SsaData_t* ssa_data);

#endif /* CN_SSA_INTERFACE_H */
