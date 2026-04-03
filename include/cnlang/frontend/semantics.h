#ifndef CN_FRONTEND_SEMANTICS_H
#define CN_FRONTEND_SEMANTICS_H

#include <stddef.h>
#include <stdbool.h>
#include "cnlang/frontend/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// 语义分析阶段使用的符号种类
typedef enum CnSemSymbolKind {
    CN_SEM_SYMBOL_VARIABLE,
    CN_SEM_SYMBOL_FUNCTION,
    CN_SEM_SYMBOL_STRUCT,   // 结构体类型
    CN_SEM_SYMBOL_ENUM,     // 枚举类型
    CN_SEM_SYMBOL_ENUM_MEMBER, // 枚举成员
    CN_SEM_SYMBOL_MODULE,   // 模块
    CN_SEM_SYMBOL_CLASS     // 类类型（用于静态成员访问）
} CnSemSymbolKind;

// 类型种类
typedef enum CnTypeKind {
    CN_TYPE_VOID,   // 无类型
    CN_TYPE_INT,    // 整型（默认，等价于 int32）
    CN_TYPE_FLOAT,  // 浮点型（默认，等价于 double）
    CN_TYPE_BOOL,   // 布尔型
    CN_TYPE_STRING, // 字符串型
    CN_TYPE_POINTER,// 指针类型
    CN_TYPE_ARRAY,  // 数组类型
    CN_TYPE_STRUCT, // 结构体类型
    CN_TYPE_ENUM,   // 枚举类型
    CN_TYPE_FUNCTION, // 函数类型
    CN_TYPE_MEMORY_ADDRESS, // 内存地址类型
    
    // 扩展的数字类型（用于支持数字字面量后缀）
    CN_TYPE_INT32,      // 32位有符号整数（L后缀）
    CN_TYPE_INT64,      // 64位有符号整数（LL后缀）
    CN_TYPE_UINT32,     // 32位无符号整数（U后缀）
    CN_TYPE_UINT64,     // 64位无符号整数（UL后缀）
    CN_TYPE_UINT64_LL,  // 64位无符号整数（ULL后缀）
    CN_TYPE_FLOAT32,    // 单精度浮点数（f/F后缀）
    CN_TYPE_FLOAT64,    // 双精度浮点数（等价于 CN_TYPE_FLOAT）
    
    CN_TYPE_SELF,       // 自身类型（表示当前类/接口的类型）
    CN_TYPE_UNKNOWN // 未知类型
} CnTypeKind;

// 前向声明：作用域、符号和结构体字段
typedef struct CnSemScope CnSemScope;
typedef struct CnSemSymbol CnSemSymbol;
typedef struct CnStructField CnStructField;
struct CnDiagnostics;

// 类型描述结构
typedef struct CnType {
    CnTypeKind kind;
    union {
        struct CnType *pointer_to;      // 指针指向的元素类型
        struct {
            struct CnType *element_type; // 数组元素类型
            size_t length;              // 数组长度（0 表示动态或未知）
        } array;
        struct {
            const char *name;           // 结构体名称
            size_t name_length;         // 结构体名称长度
            struct CnStructField *fields; // 结构体字段列表
            size_t field_count;         // 字段数量
            CnSemScope *decl_scope;     // 结构体定义所在的作用域(用于区分局部/全局)
            const char *owner_func_name;   // 局部结构体所属函数名(用于C代码生成)
            size_t owner_func_name_length; // 函数名长度
        } struct_type;
        struct {
            const char *name;           // 枚举名称
            size_t name_length;         // 枚举名称长度
            CnSemScope *enum_scope;     // 枚举成员的作用域
        } enum_type;
        struct {
            struct CnType *return_type;
            struct CnType **param_types;
            size_t param_count;
        } function;
    } as;
} CnType;

// 作用域类型（全局 / 函数 / 语句块 / 模块 / 枚举 / 文件模块）
typedef enum CnSemScopeKind {
    CN_SEM_SCOPE_GLOBAL,
    CN_SEM_SCOPE_FUNCTION,
    CN_SEM_SCOPE_BLOCK,
    CN_SEM_SCOPE_MODULE,        // 内联模块作用域（模块 模块名 { ... }）
    CN_SEM_SCOPE_ENUM,          // 枚举作用域
    CN_SEM_SCOPE_FILE_MODULE    // 文件模块作用域（Python风格，一个文件一个模块）
} CnSemScopeKind;

// 结构体字段定义
struct CnStructField {
    const char *name;          // 字段名称
    size_t name_length;        // 字段名称长度
    CnType *field_type;        // 字段类型
    int is_const;              // 是否为常量字段（使用"常量"关键字）
};

// 符号实体：表示一个变量或函数声明
struct CnSemSymbol {
    const char *name;
    size_t name_length;
    CnSemSymbolKind kind;
    CnSemScope *decl_scope;
    CnType *type; // 符号的类型信息
    int is_public; // 是否为公开成员（用于模块成员）
    int is_const;  // 是否为常量变量（使用"常量"关键字声明）
    int is_static; // 是否为静态局部变量（使用"静态"关键字声明）
    // 枚举成员的常量值和模块作用域
    union {
        long enum_value; // 当kind为CN_SEM_SYMBOL_ENUM_MEMBER时，存储枚举值
        CnSemScope *module_scope; // 当kind为CN_SEM_SYMBOL_MODULE时，指向模块的作用域
    } as;
};

// 类型管理接口
CnType *cn_type_new_primitive(CnTypeKind kind);
CnType *cn_type_new_pointer(CnType *base);
CnType *cn_type_new_array(CnType *element, size_t length);
CnType *cn_type_new_function(CnType *return_type, CnType **param_types, size_t param_count);
CnType *cn_type_new_struct(const char *name, size_t name_length, CnStructField *fields, size_t field_count, CnSemScope *decl_scope, const char *owner_func_name, size_t owner_func_name_length);
CnType *cn_type_new_enum(const char *name, size_t name_length);
CnType *cn_type_new_memory_address(void);
bool cn_type_equals(CnType *a, CnType *b);
bool cn_type_compatible(CnType *a, CnType *b); // 检查 a 是否可以隐式转换为 b 或等价

// 创建 / 销毁作用域
CnSemScope *cn_sem_scope_new(CnSemScopeKind kind, CnSemScope *parent);
void cn_sem_scope_free(CnSemScope *scope);

// 查询作用域关系
CnSemScope *cn_sem_scope_parent(CnSemScope *scope);
CnSemScopeKind cn_sem_scope_get_kind(CnSemScope *scope);  // 获取作用域类型

// 设置作用域名称(对于函数作用域)
void cn_sem_scope_set_name(CnSemScope *scope, const char *name, size_t name_length);

// 获取作用域名称
const char *cn_sem_scope_get_name(CnSemScope *scope, size_t *out_length);

// 在当前作用域中插入符号；如插入成功返回指向符号的指针，若发生重复定义等错误返回 NULL
CnSemSymbol *cn_sem_scope_insert_symbol(CnSemScope *scope,
                                        const char *name,
                                        size_t name_length,
                                        CnSemSymbolKind kind);

// 在从当前作用域向外层逐级查找符号；找到则返回符号指针，找不到返回 NULL
CnSemSymbol *cn_sem_scope_lookup(CnSemScope *scope,
                                 const char *name,
                                 size_t name_length);

// 只在当前作用域中查找符号，不向父作用域提升；找到则返回符号指针，找不到返回 NULL
CnSemSymbol *cn_sem_scope_lookup_shallow(CnSemScope *scope,
                                         const char *name,
                                         size_t name_length);

// 遍历作用域符号的回调函数类型
typedef void (*CnSemScopeSymbolCallback)(CnSemSymbol *symbol, void *user_data);

// 遍历作用域中的所有符号
void cn_sem_scope_foreach_symbol(CnSemScope *scope,
                                  CnSemScopeSymbolCallback callback,
                                  void *user_data);

// 结构体成员查找：在结构体类型中查找成员字段
CnStructField *cn_type_struct_find_field(CnType *struct_type,
                                         const char *field_name,
                                         size_t field_name_length);

// 枚举成员查找：在枚举类型中查找成员
CnSemSymbol *cn_type_enum_find_member(CnType *enum_type,
                                      const char *member_name,
                                      size_t member_name_length);

// 基于当前 AST 构建作用域链，返回全局作用域指针；失败时返回 NULL
CnSemScope *cn_sem_build_scopes(CnAstProgram *program, struct CnDiagnostics *diagnostics);

// 带模块加载器的作用域构建（支持 Python 风格跨文件模块导入）
// source_file 是当前编译的源文件路径，用于设置模块加载器的搜索路径
CnSemScope *cn_sem_build_scopes_with_loader(CnAstProgram *program, 
                                             struct CnDiagnostics *diagnostics,
                                             struct CnModuleLoader *loader,
                                             const char *source_file);

// 名称解析：在已构建的作用域链上执行名称绑定与基本重复/未定义检查；返回 true 表示成功（无致命错误）
bool cn_sem_resolve_names(CnSemScope *global_scope,
                          CnAstProgram *program,
                          struct CnDiagnostics *diagnostics);

// 类型检查：在名称解析之后，对表达式和语句进行基础类型检查；返回 true 表示成功（无致命错误）
bool cn_sem_check_types(CnSemScope *global_scope,
                        CnAstProgram *program,
                        struct CnDiagnostics *diagnostics);

// Freestanding 模式检查：检查程序是否符合 freestanding 模式约束；返回 true 表示成功
bool cn_sem_check_freestanding(CnAstProgram *program,
                               struct CnDiagnostics *diagnostics,
                               bool enable_check);

// ============================================================================
// 模块缓存访问 API
// ============================================================================

/**
 * @brief 获取缓存的模块数量
 * @return 缓存中的模块数量
 */
int cn_sem_get_cached_module_count(void);

/**
 * @brief 获取缓存的模块文件路径
 * @param index 模块索引（0 到 cn_sem_get_cached_module_count() - 1）
 * @return 模块文件路径，索引无效时返回 NULL
 */
const char *cn_sem_get_cached_module_path(int index);

// ============================================================================
// 阶段D：跨文件模块语义分析 API
// ============================================================================

// 前向声明
struct CnModuleLoader;
struct CnModuleId;
struct CnModuleMetadata;
struct CnAstModulePath;

// --- D1: 跨文件符号引用结构 ---

/**
 * @brief 导入的符号引用
 */
typedef struct CnImportedSymbol {
    const char *local_name;       ///< 本地使用的名称（可能是别名）
    size_t local_name_length;     ///< 本地名称长度
    const char *original_name;    ///< 原始符号名称
    size_t original_name_length;  ///< 原始名称长度
    struct CnModuleId *source_module; ///< 来源模块
    CnSemSymbol *resolved_symbol; ///< 解析后的符号指针
    int is_public;                ///< 是否为公开符号
} CnImportedSymbol;

/**
 * @brief 文件模块的语义信息
 */
typedef struct CnFileModuleSemInfo {
    struct CnModuleId *module_id;     ///< 模块标识符
    CnSemScope *file_scope;           ///< 文件模块作用域
    
    // 导入的符号列表
    CnImportedSymbol *imported_symbols;
    size_t imported_symbol_count;
    size_t imported_symbol_capacity;
    
    // 导出的符号列表（公开成员）
    CnSemSymbol **exported_symbols;
    size_t exported_symbol_count;
    size_t exported_symbol_capacity;
    
    // 依赖的模块列表
    struct CnModuleId **dependencies;
    size_t dependency_count;
    size_t dependency_capacity;
} CnFileModuleSemInfo;

// --- D2: 文件模块作用域管理 ---

/**
 * @brief 创建文件模块作用域
 * @param module_id 模块标识符
 * @param parent 父作用域（通常为全局作用域）
 * @return 新创建的文件模块作用域
 */
CnSemScope *cn_sem_file_module_scope_new(struct CnModuleId *module_id, CnSemScope *parent);

/**
 * @brief 获取文件模块语义信息
 * @param scope 文件模块作用域
 * @return 文件模块语义信息，如果scope不是文件模块作用域则返回NULL
 */
CnFileModuleSemInfo *cn_sem_scope_get_file_module_info(CnSemScope *scope);

// --- D3: 跨文件名称解析 ---

/**
 * @brief 解析模块路径引用（如 工具.数学.计算）
 * @param loader 模块加载器
 * @param current_scope 当前作用域
 * @param module_path 模块路径
 * @param diagnostics 诊断信息
 * @return 解析到的符号，失败返回NULL
 */
CnSemSymbol *cn_sem_resolve_module_path(struct CnModuleLoader *loader,
                                         CnSemScope *current_scope,
                                         struct CnAstModulePath *module_path,
                                         struct CnDiagnostics *diagnostics);

/**
 * @brief 解析跨文件成员访问（如 模块.成员）
 * @param loader 模块加载器
 * @param module_scope 模块作用域
 * @param member_name 成员名称
 * @param member_name_length 成员名称长度
 * @param diagnostics 诊断信息
 * @return 解析到的符号，失败返回NULL
 */
CnSemSymbol *cn_sem_resolve_module_member(struct CnModuleLoader *loader,
                                           CnSemScope *module_scope,
                                           const char *member_name,
                                           size_t member_name_length,
                                           struct CnDiagnostics *diagnostics);

// --- D4: 导入符号可见性检查 ---

/**
 * @brief 检查符号是否可以被外部访问
 * @param symbol 要检查的符号
 * @return 可访问返回1，不可访问返回0
 */
int cn_sem_symbol_is_accessible(CnSemSymbol *symbol);

/**
 * @brief 检查导入的符号是否都是公开的
 * @param file_info 文件模块语义信息
 * @param diagnostics 诊断信息
 * @return 全部可访问返回true，有私有符号被导入返回false
 */
bool cn_sem_check_import_visibility(CnFileModuleSemInfo *file_info,
                                     struct CnDiagnostics *diagnostics);

// --- D5: 模块初始化顺序分析 ---

/**
 * @brief 分析并获取模块初始化顺序
 * @param loader 模块加载器
 * @param entry_module 入口模块标识
 * @param out_order 输出的初始化顺序（调用者负责释放）
 * @param out_count 输出的模块数量
 * @param diagnostics 诊断信息
 * @return 成功返回true，存在循环依赖返回false
 */
bool cn_sem_analyze_init_order(struct CnModuleLoader *loader,
                                struct CnModuleId *entry_module,
                                struct CnModuleId ***out_order,
                                size_t *out_count,
                                struct CnDiagnostics *diagnostics);

// --- D6: 导入冲突检测 ---

/**
 * @brief 检测导入的符号是否存在名称冲突
 * @param file_info 文件模块语义信息
 * @param diagnostics 诊断信息
 * @return 无冲突返回true，有冲突返回false
 */
bool cn_sem_check_import_conflicts(CnFileModuleSemInfo *file_info,
                                    struct CnDiagnostics *diagnostics);

/**
 * @brief 注册导入的符号到文件模块
 * @param file_info 文件模块语义信息
 * @param local_name 本地名称
 * @param local_name_length 本地名称长度
 * @param original_name 原始名称
 * @param original_name_length 原始名称长度
 * @param source_module 来源模块
 * @param resolved_symbol 解析后的符号
 * @param is_public 是否公开
 * @return 成功返回1，失败返回0
 */
int cn_sem_register_imported_symbol(CnFileModuleSemInfo *file_info,
                                     const char *local_name,
                                     size_t local_name_length,
                                     const char *original_name,
                                     size_t original_name_length,
                                     struct CnModuleId *source_module,
                                     CnSemSymbol *resolved_symbol,
                                     int is_public);

// --- D7: 相对导入路径解析 ---

/**
 * @brief 解析相对导入路径
 * @param loader 模块加载器
 * @param current_module 当前模块标识
 * @param relative_path 相对路径AST
 * @param out_resolved_id 输出解析后的模块标识（调用者负责释放）
 * @param diagnostics 诊断信息
 * @return 成功返回true，失败返回false
 */
bool cn_sem_resolve_relative_import(struct CnModuleLoader *loader,
                                     struct CnModuleId *current_module,
                                     struct CnAstModulePath *relative_path,
                                     struct CnModuleId **out_resolved_id,
                                     struct CnDiagnostics *diagnostics);

// --- 文件模块语义信息管理 ---

/**
 * @brief 创建文件模块语义信息
 * @param module_id 模块标识符
 * @return 新创建的语义信息
 */
CnFileModuleSemInfo *cn_file_module_sem_info_create(struct CnModuleId *module_id);

/**
 * @brief 释放文件模块语义信息
 * @param info 要释放的语义信息
 */
void cn_file_module_sem_info_free(CnFileModuleSemInfo *info);

/**
 * @brief 添加导出符号
 * @param info 文件模块语义信息
 * @param symbol 要导出的符号
 * @return 成功返回1，失败返回0
 */
int cn_file_module_sem_info_add_export(CnFileModuleSemInfo *info, CnSemSymbol *symbol);

/**
 * @brief 添加依赖模块
 * @param info 文件模块语义信息
 * @param dep_id 依赖的模块标识
 * @return 成功返回1，失败返回0
 */
int cn_file_module_sem_info_add_dependency(CnFileModuleSemInfo *info, struct CnModuleId *dep_id);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_SEMANTICS_H */
