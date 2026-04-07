#ifndef CN_BACKEND_CGEN_H
#define CN_BACKEND_CGEN_H

#include "cnlang/ir/ir.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/runtime/collections.h"  // CnMap 用于函数声明去重
#include <stdio.h>

// 前向声明
struct CnModuleLoader;
struct CnSemScope;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C代码生成器上下文
 * 用于在生成过程中维护状态，例如当前正在处理的函数、基本块等。
 */
typedef struct CnCCodeGenContext {
    FILE *output_file;          ///< 输出文件指针
    CnIrModule *module;         ///< 当前 IR 模块（用于获取编译模式）
    CnIrFunction *current_func; ///< 当前正在处理的 IR 函数
    CnIrBasicBlock *current_block; ///< 当前正在处理的 IR 基本块
    int label_counter;          ///< 用于生成唯一标签名称的计数器
    int temp_var_counter;       ///< 用于生成唯一临时变量名称的计数器
    void *module_init_infos;    ///< 模块初始化信息（内部使用）
    size_t module_init_count;   ///< 模块初始化信息数量
    struct CnModuleId *module_id; ///< 当前模块ID（用于生成带模块前缀的函数名）
    struct CnAstProgram *program; ///< AST程序（用于查找类定义，支持继承vtable生成）
    struct CnAstClassDecl *current_class; ///< 当前正在处理的类声明（用于基类访问代码生成）
    
    /* 类成员代码生成专用字段 */
    struct CnClassMember *current_method; ///< 当前正在处理的方法/构造函数（用于参数查找）
    
    /* 寄存器类型信息（用于成员访问时判断指针类型） */
    struct CnType **reg_types;  ///< 寄存器类型数组
    int reg_types_count;        ///< 寄存器类型数组大小
    
    /* 函数声明去重（避免多个模块导入同一函数时重复声明） */
    CnMap *declared_functions;  ///< 已声明的函数签名集合
} CnCCodeGenContext;

/**
 * @brief 将 IR 模块转换为 C 代码并写入指定文件
 * 
 * @param module IR 模块
 * @param filename 输出的 C 文件名
 * @return 0 成功，非 0 失败
 */
int cn_cgen_module_to_file(CnIrModule *module, const char *filename);

/**
 * @brief 将 IR 模块和结构体定义转换为 C 代码并写入指定文件
 * 
 * @param module IR 模块
 * @param program AST 程序（包含结构体定义）
 * @param filename 输出的 C 文件名
 * @return 0 成功，非 0 失败
 */
int cn_cgen_module_with_structs_to_file(CnIrModule *module, CnAstProgram *program, const char *filename);

/**
 * @brief 将 IR 模块和结构体定义转换为 C 代码，支持模块加载器
 *
 * @param module IR 模块
 * @param program AST 程序（包含结构体定义和导入语句）
 * @param loader 模块加载器（可为 NULL）
 * @param global_scope 全局作用域（用于获取导入符号的类型信息）
 * @param module_id 当前模块ID（用于生成带模块前缀的函数名，可为 NULL）
 * @param filename 输出的 C 文件名
 * @return 0 成功，非 0 失败
 */
int cn_cgen_module_with_imports_to_file(CnIrModule *module, CnAstProgram *program,
                                         struct CnModuleLoader *loader,
                                         struct CnSemScope *global_scope,
                                         struct CnModuleId *module_id,
                                         const char *filename);

/**
 * @brief 将 IR 模块的函数声明生成为 C 头文件并写入指定文件
 * 
 * @param module IR 模块
 * @param filename 输出的 H 文件名
 * @return 0 成功，非 0 失败
 */
int cn_cgen_header_to_file(CnIrModule *module, const char *filename);

/**
 * @brief 将 IR 模块转换为 C 代码字符串
 * 
 * @param module IR 模块
 * @return char* C 代码字符串，调用者负责释放内存
 */
char *cn_cgen_module_to_string(CnIrModule *module);

/**
 * @brief 生成单个函数的 C 代码
 * 
 * @param ctx 生成器上下文
 * @param func IR 函数
 */
void cn_cgen_function(CnCCodeGenContext *ctx, CnIrFunction *func);

/**
 * @brief 生成单个基本块的 C 代码
 * 
 * @param ctx 生成器上下文
 * @param block IR 基本块
 */
void cn_cgen_block(CnCCodeGenContext *ctx, CnIrBasicBlock *block);

/**
 * @brief 生成单条 IR 指令的 C 代码
 * 
 * @param ctx 生成器上下文
 * @param inst IR 指令
 */
void cn_cgen_inst(CnCCodeGenContext *ctx, CnIrInst *inst);

/**
 * @brief 生成结构体定义的 C 代码
 *
 * @param ctx 生成器上下文
 * @param struct_stmt AST 结构体声明语句
 */
void cn_cgen_struct_decl(CnCCodeGenContext *ctx, CnAstStmt *struct_stmt);

/**
 * @brief 获取C类型字符串
 *
 * 将CN语言的类型转换为C语言的类型字符串
 *
 * @param type CN语言类型
 * @return const char* C语言类型字符串
 */
const char *get_c_type_string(CnType *type);

// ============================================================================
// 阶段E：多文件模块代码生成 API
// ============================================================================

struct CnModuleId;
struct CnModuleLoader;
struct CnModuleMetadata;

/**
 * @brief 模块编译单元
 */
typedef struct CnModuleCompileUnit {
    struct CnModuleId *module_id;    ///< 模块标识符
    char *source_path;               ///< 源文件路径
    char *header_path;               ///< 生成的头文件路径
    char *impl_path;                 ///< 生成的实现文件路径
    CnAstProgram *program;           ///< 解析后的 AST
    CnIrModule *ir_module;           ///< 生成的 IR 模块
    uint64_t source_mtime;           ///< 源文件修改时间
    uint64_t output_mtime;           ///< 输出文件修改时间
    int needs_rebuild;               ///< 是否需要重新编译
    int is_entry;                    ///< 是否是入口模块
} CnModuleCompileUnit;

/**
 * @brief 多文件编译上下文
 */
typedef struct CnMultiFileCompileContext {
    CnModuleCompileUnit *units;      ///< 编译单元数组
    size_t unit_count;               ///< 编译单元数量
    size_t unit_capacity;            ///< 编译单元容量
    
    char *output_dir;                ///< 输出目录
    size_t output_dir_length;        ///< 输出目录长度
    
    struct CnModuleId **init_order;  ///< 初始化顺序
    size_t init_order_count;         ///< 初始化顺序数量
    
    struct CnModuleLoader *loader;   ///< 模块加载器
    void *diagnostics;               ///< 诊断信息
} CnMultiFileCompileContext;

// --- E1: 多文件编译单元管理 ---

/**
 * @brief 创建多文件编译上下文
 * @param output_dir 输出目录
 * @param loader 模块加载器
 * @return 新创建的编译上下文
 */
CnMultiFileCompileContext *cn_multi_compile_ctx_create(const char *output_dir, 
                                                        struct CnModuleLoader *loader);

/**
 * @brief 释放多文件编译上下文
 */
void cn_multi_compile_ctx_free(CnMultiFileCompileContext *ctx);

/**
 * @brief 添加编译单元
 * @param ctx 编译上下文
 * @param module_id 模块标识符
 * @param source_path 源文件路径
 * @param is_entry 是否是入口模块
 * @return 成功返回1，失败返回0
 */
int cn_multi_compile_ctx_add_unit(CnMultiFileCompileContext *ctx,
                                   struct CnModuleId *module_id,
                                   const char *source_path,
                                   int is_entry);

// --- E2: 模块头文件生成 ---

/**
 * @brief 生成模块头文件
 * @param ctx 编译上下文
 * @param unit 编译单元
 * @return 成功返回0，失败返回非0
 */
int cn_cgen_module_header(CnMultiFileCompileContext *ctx, CnModuleCompileUnit *unit);

// --- E3: 模块实现文件生成 ---

/**
 * @brief 生成模块实现文件
 * @param ctx 编译上下文
 * @param unit 编译单元
 * @return 成功返回0，失败返回非0
 */
int cn_cgen_module_impl(CnMultiFileCompileContext *ctx, CnModuleCompileUnit *unit);

// --- E4: 模块初始化函数生成 ---

/**
 * @brief 生成模块初始化函数
 * @param ctx 代码生成上下文
 * @param unit 编译单元
 */
void cn_cgen_module_init_func(CnCCodeGenContext *ctx, CnModuleCompileUnit *unit);

// --- E5: 模块初始化调用顺序 ---

/**
 * @brief 生成模块初始化调用代码
 * @param ctx 多文件编译上下文
 * @param output_file 输出文件
 * @return 成功返回0，失败返回非0
 */
int cn_cgen_init_call_sequence(CnMultiFileCompileContext *ctx, FILE *output_file);

// --- E6: 符号命名规则 ---

/**
 * @brief 生成模块限定的符号名称
 * @param module_id 模块标识符
 * @param symbol_name 符号名称
 * @param symbol_name_length 符号名称长度
 * @param out_buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数
 */
size_t cn_cgen_qualified_symbol_name(struct CnModuleId *module_id,
                                      const char *symbol_name,
                                      size_t symbol_name_length,
                                      char *out_buffer,
                                      size_t buffer_size);

// --- E7: 增量编译支持 ---

/**
 * @brief 检查模块是否需要重新编译
 * @param unit 编译单元
 * @return 需要重新编译返回1，否则返回0
 */
int cn_cgen_needs_rebuild(CnModuleCompileUnit *unit);

/**
 * @brief 执行多文件编译
 * @param ctx 编译上下文
 * @return 成功返回0，失败返回非0
 */
int cn_multi_compile_execute(CnMultiFileCompileContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CN_BACKEND_CGEN_H */