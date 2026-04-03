/**
 * @file module_loader.h
 * @brief CN语言 Python 风格模块系统 - 模块加载器
 * 
 * 本文件定义模块加载器的接口，用于实现跨文件模块导入功能。
 * 
 * 功能包括：
 * - 模块标识符系统（完全限定名：包.子包.模块）
 * - 模块元数据管理
 * - 模块搜索路径配置
 * - 模块缓存和依赖管理
 * 
 * @version 1.0
 * @date 2026-01-29
 */

#ifndef CN_FRONTEND_MODULE_LOADER_H
#define CN_FRONTEND_MODULE_LOADER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 前向声明
// ============================================================================

struct CnAstFileModule;
struct CnAstPackageDecl;
struct CnAstModulePath;
struct CnDiagnostics;

// ============================================================================
// 模块标识符系统（A4）
// ============================================================================

/**
 * @brief 模块类型枚举
 */
typedef enum CnModuleType {
    CN_MODULE_TYPE_FILE,      ///< 文件模块（.cn 文件）
    CN_MODULE_TYPE_PACKAGE,   ///< 包（包含 __包__.cn 的目录）
    CN_MODULE_TYPE_INLINE,    ///< 内联模块（同文件内定义的模块）
    CN_MODULE_TYPE_BUILTIN    ///< 内建模块
} CnModuleType;

/**
 * @brief 模块加载状态枚举
 */
typedef enum CnModuleLoadState {
    CN_MODULE_STATE_UNLOADED,    ///< 未加载
    CN_MODULE_STATE_LOADING,     ///< 加载中（用于检测循环导入）
    CN_MODULE_STATE_LOADED,      ///< 已加载
    CN_MODULE_STATE_ERROR        ///< 加载失败
} CnModuleLoadState;

/**
 * @brief 模块标识符结构
 * 
 * 表示一个完全限定的模块名，如 "工具.数学.高级"
 */
typedef struct CnModuleId {
    char *qualified_name;        ///< 完全限定名（如 "工具.数学.高级"）
    size_t qualified_name_length; ///< 完全限定名长度
    char **segments;             ///< 路径段列表（如 ["工具", "数学", "高级"]）
    size_t segment_count;        ///< 路径段数量
} CnModuleId;

// ============================================================================
// 模块元数据结构（A5）
// ============================================================================

/**
 * @brief 导出符号信息
 */
typedef struct CnExportedSymbol {
    char *name;                  ///< 符号名称
    size_t name_length;          ///< 符号名称长度
    int is_function;             ///< 是否为函数
    int is_constant;             ///< 是否为常量
    void *symbol_info;           ///< 指向符号表条目的指针
} CnExportedSymbol;

/**
 * @brief 模块依赖信息
 */
typedef struct CnModuleDependency {
    struct CnModuleId *module_id; ///< 依赖的模块标识
    int is_required;              ///< 是否为必需依赖
    int is_circular;              ///< 是否形成循环依赖
} CnModuleDependency;

/**
 * @brief 模块元数据
 */
typedef struct CnModuleMetadata {
    CnModuleId *id;                  ///< 模块标识符
    CnModuleType type;               ///< 模块类型
    CnModuleLoadState state;         ///< 加载状态
    
    // 文件信息
    char *file_path;                 ///< 源文件绝对路径
    size_t file_path_length;         ///< 文件路径长度
    uint64_t file_mtime;             ///< 文件修改时间（用于增量编译）
    
    // 导出符号
    CnExportedSymbol *exports;       ///< 导出符号列表
    size_t export_count;             ///< 导出符号数量
    
    // 依赖信息
    CnModuleDependency *dependencies; ///< 依赖列表
    size_t dependency_count;          ///< 依赖数量
    
    // AST 引用
    struct CnAstFileModule *file_module;   ///< 文件模块 AST
    struct CnAstPackageDecl *package_decl; ///< 包声明 AST
} CnModuleMetadata;

// ============================================================================
// 模块搜索路径配置（A6）
// ============================================================================

/**
 * @brief 搜索路径条目
 */
typedef struct CnSearchPathEntry {
    char *path;                  ///< 路径字符串
    size_t path_length;          ///< 路径长度
    int priority;                ///< 优先级（数字越小优先级越高）
} CnSearchPathEntry;

/**
 * @brief 模块搜索路径配置
 */
typedef struct CnModuleSearchConfig {
    CnSearchPathEntry *paths;    ///< 搜索路径列表
    size_t path_count;           ///< 搜索路径数量
    size_t path_capacity;        ///< 搜索路径容量
    
    // 特殊路径
    char *project_root;          ///< 项目根目录
    size_t project_root_length;  ///< 项目根目录长度
    char *stdlib_path;           ///< 标准库路径
    size_t stdlib_path_length;   ///< 标准库路径长度
} CnModuleSearchConfig;

// ============================================================================
// 模块缓存
// ============================================================================

/**
 * @brief 模块缓存条目
 */
typedef struct CnModuleCacheEntry {
    CnModuleId *id;              ///< 模块标识符
    CnModuleMetadata *metadata;  ///< 模块元数据
    struct CnModuleCacheEntry *next; ///< 链表下一项
} CnModuleCacheEntry;

/**
 * @brief 模块缓存（哈希表实现）
 */
typedef struct CnModuleCache {
    CnModuleCacheEntry **buckets; ///< 哈希桶数组
    size_t bucket_count;          ///< 桶数量
    size_t entry_count;           ///< 条目数量
} CnModuleCache;

// ============================================================================
// 模块依赖图
// ============================================================================

/**
 * @brief 依赖图节点
 */
typedef struct CnDependencyNode {
    CnModuleId *id;                  ///< 模块标识符
    struct CnDependencyNode **edges; ///< 出边列表（依赖的模块）
    size_t edge_count;               ///< 出边数量
    int visited;                     ///< 遍历标记
    int in_stack;                    ///< 是否在递归栈中（用于检测循环）
} CnDependencyNode;

/**
 * @brief 模块依赖图
 */
typedef struct CnDependencyGraph {
    CnDependencyNode **nodes;    ///< 节点列表
    size_t node_count;           ///< 节点数量
    size_t node_capacity;        ///< 节点容量
} CnDependencyGraph;

// ============================================================================
// 模块加载器
// ============================================================================

/**
 * @brief 模块加载器
 */
typedef struct CnModuleLoader {
    CnModuleSearchConfig *search_config; ///< 搜索路径配置
    CnModuleCache *cache;                ///< 模块缓存
    CnDependencyGraph *dep_graph;        ///< 依赖图
    struct CnDiagnostics *diagnostics;   ///< 诊断信息
    
    // 当前加载上下文
    char *current_file;                  ///< 当前正在处理的文件
    size_t current_file_length;          ///< 当前文件路径长度
} CnModuleLoader;

// ============================================================================
// API 函数声明
// ============================================================================

// --- 模块标识符操作 ---

/**
 * @brief 创建模块标识符
 * @param qualified_name 完全限定名（如 "工具.数学"）
 * @return 新创建的模块标识符，失败返回 NULL
 */
CnModuleId *cn_module_id_create(const char *qualified_name);

/**
 * @brief 从路径段创建模块标识符
 * @param segments 路径段数组
 * @param segment_count 路径段数量
 * @return 新创建的模块标识符，失败返回 NULL
 */
CnModuleId *cn_module_id_create_from_segments(const char **segments, size_t segment_count);

/**
 * @brief 释放模块标识符
 * @param id 要释放的模块标识符
 */
void cn_module_id_free(CnModuleId *id);

/**
 * @brief 比较两个模块标识符是否相等
 * @param a 第一个模块标识符
 * @param b 第二个模块标识符
 * @return 相等返回 1，不相等返回 0
 */
int cn_module_id_equals(const CnModuleId *a, const CnModuleId *b);

/**
 * @brief 计算模块标识符的哈希值
 * @param id 模块标识符
 * @return 哈希值
 */
uint32_t cn_module_id_hash(const CnModuleId *id);

// --- 模块搜索路径配置 ---

/**
 * @brief 创建搜索路径配置
 * @return 新创建的配置，失败返回 NULL
 */
CnModuleSearchConfig *cn_search_config_create(void);

/**
 * @brief 释放搜索路径配置
 * @param config 要释放的配置
 */
void cn_search_config_free(CnModuleSearchConfig *config);

/**
 * @brief 添加搜索路径
 * @param config 配置对象
 * @param path 要添加的路径
 * @param priority 优先级
 * @return 成功返回 1，失败返回 0
 */
int cn_search_config_add_path(CnModuleSearchConfig *config, const char *path, int priority);

/**
 * @brief 从环境变量加载搜索路径
 * @param config 配置对象
 * @param env_var 环境变量名（如 "CN_MODULE_PATH"）
 * @return 成功返回 1，失败返回 0
 */
int cn_search_config_load_from_env(CnModuleSearchConfig *config, const char *env_var);

/**
 * @brief 设置项目根目录
 * @param config 配置对象
 * @param project_root 项目根目录路径
 */
void cn_search_config_set_project_root(CnModuleSearchConfig *config, const char *project_root);

/**
 * @brief 设置标准库路径
 * @param config 配置对象
 * @param stdlib_path 标准库路径
 */
void cn_search_config_set_stdlib_path(CnModuleSearchConfig *config, const char *stdlib_path);

// --- 模块缓存操作 ---

/**
 * @brief 创建模块缓存
 * @param initial_bucket_count 初始桶数量
 * @return 新创建的缓存，失败返回 NULL
 */
CnModuleCache *cn_module_cache_create(size_t initial_bucket_count);

/**
 * @brief 释放模块缓存
 * @param cache 要释放的缓存
 */
void cn_module_cache_free(CnModuleCache *cache);

/**
 * @brief 在缓存中查找模块
 * @param cache 缓存对象
 * @param id 模块标识符
 * @return 找到的模块元数据，未找到返回 NULL
 */
CnModuleMetadata *cn_module_cache_get(CnModuleCache *cache, const CnModuleId *id);

/**
 * @brief 将模块添加到缓存
 * @param cache 缓存对象
 * @param metadata 模块元数据
 * @return 成功返回 1，失败返回 0
 */
int cn_module_cache_put(CnModuleCache *cache, CnModuleMetadata *metadata);

/**
 * @brief 遍历缓存中的所有模块
 * @param cache 缓存对象
 * @param callback 回调函数，参数为模块元数据，返回 0 继续遍历，非 0 停止
 * @param user_data 用户数据指针
 * @return 遍历的模块数量
 */
size_t cn_module_cache_foreach(CnModuleCache *cache,
                                int (*callback)(CnModuleMetadata *metadata, void *user_data),
                                void *user_data);

/**
 * @brief 获取缓存中的模块数量
 * @param cache 缓存对象
 * @return 模块数量
 */
size_t cn_module_cache_count(CnModuleCache *cache);

// --- 模块加载器操作 ---

/**
 * @brief 创建模块加载器
 * @return 新创建的加载器，失败返回 NULL
 */
CnModuleLoader *cn_module_loader_create(void);

/**
 * @brief 释放模块加载器
 * @param loader 要释放的加载器
 */
void cn_module_loader_free(CnModuleLoader *loader);

/**
 * @brief 设置诊断信息对象
 * @param loader 加载器
 * @param diagnostics 诊断信息对象
 */
void cn_module_loader_set_diagnostics(CnModuleLoader *loader, struct CnDiagnostics *diagnostics);

/**
 * @brief 加载模块
 * @param loader 加载器
 * @param module_id 模块标识符
 * @return 加载的模块元数据，失败返回 NULL
 */
CnModuleMetadata *cn_module_loader_load(CnModuleLoader *loader, const CnModuleId *module_id);

/**
 * @brief 加载相对模块
 * @param loader 加载器
 * @param base_path 基准路径（当前文件所在目录）
 * @param relative_path 相对模块路径
 * @return 加载的模块元数据，失败返回 NULL
 */
CnModuleMetadata *cn_module_loader_load_relative(CnModuleLoader *loader, 
                                                  const char *base_path,
                                                  const struct CnAstModulePath *relative_path);

/**
 * @brief 加载相对模块（带目标类型）
 * @param loader 加载器
 * @param base_path 基准路径（当前文件所在目录）
 * @param relative_path 相对模块路径
 * @param target_type 导入目标类型：0 = 模块（.cn文件），1 = 包（目录中的__包__.cn）
 * @return 加载的模块元数据，失败返回 NULL
 */
CnModuleMetadata *cn_module_loader_load_relative_typed(CnModuleLoader *loader, 
                                                        const char *base_path,
                                                        const struct CnAstModulePath *relative_path,
                                                        int target_type);

/**
 * @brief 解析模块路径为文件路径
 * @param loader 加载器
 * @param module_id 模块标识符
 * @param out_path 输出的文件路径（调用者负责释放）
 * @return 成功返回 1，失败返回 0
 */
int cn_module_loader_resolve_path(CnModuleLoader *loader, 
                                   const CnModuleId *module_id,
                                   char **out_path);

/**
 * @brief 解析模块路径为文件路径（带目标类型）
 * @param loader 加载器
 * @param module_id 模块标识符
 * @param out_path 输出的文件路径（调用者负责释放）
 * @param target_type 导入目标类型：0 = 模块（.cn文件），1 = 包（目录中的__包__.cn）
 * @return 成功返回 1，失败返回 0
 */
int cn_module_loader_resolve_path_typed(CnModuleLoader *loader, 
                                         const CnModuleId *module_id,
                                         char **out_path,
                                         int target_type);

/**
 * @brief 检测循环导入
 * @param loader 加载器
 * @param module_id 要检测的模块标识符
 * @return 存在循环导入返回 1，否则返回 0
 */
int cn_module_loader_check_circular(CnModuleLoader *loader, const CnModuleId *module_id);

/**
 * @brief 获取模块初始化顺序
 * @param loader 加载器
 * @param out_order 输出的模块标识符数组（调用者负责释放）
 * @param out_count 输出的模块数量
 * @return 成功返回 1，失败返回 0
 */
int cn_module_loader_get_init_order(CnModuleLoader *loader,
                                     CnModuleId ***out_order,
                                     size_t *out_count);

// --- 模块元数据操作 ---

/**
 * @brief 创建模块元数据
 * @return 新创建的元数据，失败返回 NULL
 */
CnModuleMetadata *cn_module_metadata_create(void);

/**
 * @brief 释放模块元数据
 * @param metadata 要释放的元数据
 */
void cn_module_metadata_free(CnModuleMetadata *metadata);

/**
 * @brief 添加导出符号
 * @param metadata 元数据对象
 * @param name 符号名称
 * @param is_function 是否为函数
 * @param is_constant 是否为常量
 * @return 成功返回 1，失败返回 0
 */
int cn_module_metadata_add_export(CnModuleMetadata *metadata,
                                   const char *name,
                                   int is_function,
                                   int is_constant);

/**
 * @brief 添加模块依赖
 * @param metadata 元数据对象
 * @param dep_id 依赖的模块标识符
 * @return 成功返回 1，失败返回 0
 */
int cn_module_metadata_add_dependency(CnModuleMetadata *metadata, const CnModuleId *dep_id);

// --- 依赖图操作 ---

/**
 * @brief 创建依赖图
 * @return 新创建的依赖图，失败返回 NULL
 */
CnDependencyGraph *cn_dependency_graph_create(void);

/**
 * @brief 释放依赖图
 * @param graph 要释放的依赖图
 */
void cn_dependency_graph_free(CnDependencyGraph *graph);

/**
 * @brief 添加依赖边
 * @param graph 依赖图
 * @param from 源模块
 * @param to 目标模块
 * @return 成功返回 1，失败返回 0
 */
int cn_dependency_graph_add_edge(CnDependencyGraph *graph,
                                  const CnModuleId *from,
                                  const CnModuleId *to);

/**
 * @brief 执行拓扑排序
 * @param graph 依赖图
 * @param out_order 输出的排序结果（调用者负责释放）
 * @param out_count 输出的节点数量
 * @return 成功返回 1，存在循环返回 0
 */
int cn_dependency_graph_topological_sort(CnDependencyGraph *graph,
                                          CnModuleId ***out_order,
                                          size_t *out_count);

// ============================================================================
// 包初始化文件识别（B7）
// ============================================================================

/**
 * @brief 包初始化文件名
 * 
 * 当一个目录包含名为 "__包__.cn" 的文件时，该目录被视为一个包。
 */
#define CN_PACKAGE_INIT_FILENAME "__包__.cn"

/**
 * @brief 检查文件名是否为包初始化文件
 * @param filename 文件名（仅文件名，不含路径）
 * @return 是包初始化文件返回 1，否则返回 0
 */
int cn_is_package_init_file(const char *filename);

/**
 * @brief 检查完整路径是否指向包初始化文件
 * @param file_path 文件的完整路径
 * @return 是包初始化文件返回 1，否则返回 0
 */
int cn_is_package_init_path(const char *file_path);

/**
 * @brief 检查目录是否为包（包含 __包__.cn 文件）
 * @param dir_path 目录路径
 * @return 是包返回 1，否则返回 0
 */
int cn_is_package_directory(const char *dir_path);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_MODULE_LOADER_H */
