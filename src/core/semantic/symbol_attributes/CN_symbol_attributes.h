/**
 * @file CN_symbol_attributes.h
 * @brief 符号属性扩展模块头文件
 * 
 * 本模块提供符号属性扩展功能，包括：
 * 1. 符号可见性级别（私有、保护、公有、内部、外部）
 * 2. 符号生命周期（静态、自动、动态、线程、临时）
 * 3. 符号访问权限（只读、只写、读写、只执行、无访问）
 * 4. 符号存储类别（自动、静态、寄存器、外部、线程局部）
 * 5. 符号依赖关系跟踪
 * 6. 符号重命名支持
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SYMBOL_ATTRIBUTES_H
#define CN_SYMBOL_ATTRIBUTES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 符号属性枚举定义
 * ============================================================================ */

/**
 * @enum Eum_SymbolVisibility
 * @brief 符号可见性级别枚举
 */
enum Eum_SymbolVisibility
{
    Eum_VISIBILITY_PRIVATE = 0,    /**< 私有：仅在定义模块内可见 */
    Eum_VISIBILITY_PROTECTED,      /**< 保护：在定义模块及其子模块内可见 */
    Eum_VISIBILITY_PUBLIC,         /**< 公有：在所有模块内可见 */
    Eum_VISIBILITY_INTERNAL,       /**< 内部：在包内可见，包外不可见 */
    Eum_VISIBILITY_EXTERNAL        /**< 外部：外部链接，可被其他模块引用 */
};

/**
 * @enum Eum_SymbolLifetime
 * @brief 符号生命周期枚举
 */
enum Eum_SymbolLifetime
{
    Eum_LIFETIME_STATIC = 0,       /**< 静态：程序整个运行期间存在 */
    Eum_LIFETIME_AUTOMATIC,        /**< 自动：函数调用期间存在 */
    Eum_LIFETIME_DYNAMIC,          /**< 动态：手动分配和释放 */
    Eum_LIFETIME_THREAD,           /**< 线程：线程生命周期内存在 */
    Eum_LIFETIME_TEMPORARY         /**< 临时：表达式求值期间存在 */
};

/**
 * @enum Eum_SymbolAccess
 * @brief 符号访问权限枚举
 */
enum Eum_SymbolAccess
{
    Eum_ACCESS_READ_ONLY = 0,      /**< 只读：只能读取，不能修改 */
    Eum_ACCESS_WRITE_ONLY,         /**< 只写：只能写入，不能读取 */
    Eum_ACCESS_READ_WRITE,         /**< 读写：可以读取和写入 */
    Eum_ACCESS_EXECUTE_ONLY,       /**< 只执行：只能执行，不能读取或写入 */
    Eum_ACCESS_NO_ACCESS           /**< 无访问：不能访问 */
};

/**
 * @enum Eum_SymbolStorage
 * @brief 符号存储类别枚举
 */
enum Eum_SymbolStorage
{
    Eum_STORAGE_AUTO = 0,          /**< 自动：自动存储期 */
    Eum_STORAGE_STATIC,            /**< 静态：静态存储期 */
    Eum_STORAGE_REGISTER,          /**< 寄存器：建议存储在寄存器中 */
    Eum_STORAGE_EXTERN,            /**< 外部：外部链接 */
    Eum_STORAGE_THREAD_LOCAL       /**< 线程局部：每个线程有自己的副本 */
};

/**
 * @enum Eum_DependencyType
 * @brief 符号依赖类型枚举
 */
enum Eum_DependencyType
{
    Eum_DEPENDENCY_USES = 0,       /**< 使用：符号使用其他符号 */
    Eum_DEPENDENCY_DEFINES,        /**< 定义：符号定义其他符号 */
    Eum_DEPENDENCY_REFERENCES,     /**< 引用：符号引用其他符号 */
    Eum_DEPENDENCY_ALIAS,          /**< 别名：符号是其他符号的别名 */
    Eum_DEPENDENCY_INHERITS,       /**< 继承：符号继承其他符号 */
    Eum_DEPENDENCY_IMPLEMENTS      /**< 实现：符号实现其他符号 */
};

/* ============================================================================
 * 符号属性结构体定义
 * ============================================================================ */

/**
 * @struct Stru_SymbolDependency
 * @brief 符号依赖关系结构体
 */
typedef struct Stru_SymbolDependency_t
{
    char* dependent_symbol;        /**< 依赖符号名称 */
    char* depended_symbol;         /**< 被依赖符号名称 */
    enum Eum_DependencyType type;  /**< 依赖类型 */
    uint32_t line_number;          /**< 依赖所在行号 */
    uint32_t column_number;        /**< 依赖所在列号 */
    struct Stru_SymbolDependency_t* next; /**< 下一个依赖关系 */
} Stru_SymbolDependency_t;

/**
 * @struct Stru_SymbolReference
 * @brief 符号引用结构体
 */
typedef struct Stru_SymbolReference_t
{
    char* symbol_name;             /**< 符号名称 */
    uint32_t reference_count;      /**< 引用计数 */
    uint32_t definition_line;      /**< 定义行号 */
    uint32_t definition_column;    /**< 定义列号 */
    struct Stru_SymbolReference_t* next; /**< 下一个引用 */
} Stru_SymbolReference_t;

/**
 * @struct Stru_ExtendedSymbolInfo
 * @brief 扩展的符号信息结构体
 */
typedef struct Stru_ExtendedSymbolInfo_t
{
    char* symbol_name;             /**< 符号名称 */
    char* symbol_type;             /**< 符号类型 */
    
    /* 扩展属性 */
    enum Eum_SymbolVisibility visibility; /**< 可见性级别 */
    enum Eum_SymbolLifetime lifetime;     /**< 生命周期 */
    enum Eum_SymbolAccess access;         /**< 访问权限 */
    enum Eum_SymbolStorage storage;       /**< 存储类别 */
    
    /* 依赖关系信息 */
    Stru_SymbolDependency_t* dependencies; /**< 依赖关系链表 */
    Stru_SymbolReference_t* references;    /**< 引用信息链表 */
    
    /* 重命名支持 */
    char* original_name;           /**< 原始名称（用于重命名） */
    char* aliases;                 /**< 别名列表（逗号分隔） */
    
    /* 其他属性 */
    bool is_constant;              /**< 是否为常量 */
    bool is_volatile;              /**< 是否为易变变量 */
    bool is_external;              /**< 是否为外部符号 */
    bool is_deprecated;            /**< 是否已废弃 */
    
    /* 位置信息 */
    uint32_t line_number;          /**< 定义行号 */
    uint32_t column_number;        /**< 定义列号 */
    char* file_name;               /**< 定义文件名 */
    
    /* 时间戳 */
    uint64_t creation_time;        /**< 创建时间戳 */
    uint64_t modification_time;    /**< 修改时间戳 */
    
    struct Stru_ExtendedSymbolInfo_t* next; /**< 下一个符号信息 */
} Stru_ExtendedSymbolInfo_t;

/* ============================================================================
 * 符号属性接口定义
 * ============================================================================ */

/**
 * @struct Stru_SymbolAttributesInterface_t
 * @brief 符号属性接口结构体
 */
typedef struct Stru_SymbolAttributesInterface_t
{
    /**
     * @brief 初始化符号属性模块
     * @return 成功返回true，失败返回false
     */
    bool (*initialize)(void);
    
    /**
     * @brief 销毁符号属性模块
     */
    void (*destroy)(void);
    
    /**
     * @brief 创建扩展符号信息
     * @param symbol_name 符号名称
     * @param symbol_type 符号类型
     * @return 创建的符号信息指针，失败返回NULL
     */
    Stru_ExtendedSymbolInfo_t* (*create_symbol_info)(const char* symbol_name, const char* symbol_type);
    
    /**
     * @brief 销毁扩展符号信息
     * @param symbol_info 符号信息指针
     */
    void (*destroy_symbol_info)(Stru_ExtendedSymbolInfo_t* symbol_info);
    
    /**
     * @brief 设置符号可见性
     * @param symbol_info 符号信息指针
     * @param visibility 可见性级别
     * @return 成功返回true，失败返回false
     */
    bool (*set_visibility)(Stru_ExtendedSymbolInfo_t* symbol_info, enum Eum_SymbolVisibility visibility);
    
    /**
     * @brief 设置符号生命周期
     * @param symbol_info 符号信息指针
     * @param lifetime 生命周期
     * @return 成功返回true，失败返回false
     */
    bool (*set_lifetime)(Stru_ExtendedSymbolInfo_t* symbol_info, enum Eum_SymbolLifetime lifetime);
    
    /**
     * @brief 设置符号访问权限
     * @param symbol_info 符号信息指针
     * @param access 访问权限
     * @return 成功返回true，失败返回false
     */
    bool (*set_access)(Stru_ExtendedSymbolInfo_t* symbol_info, enum Eum_SymbolAccess access);
    
    /**
     * @brief 设置符号存储类别
     * @param symbol_info 符号信息指针
     * @param storage 存储类别
     * @return 成功返回true，失败返回false
     */
    bool (*set_storage)(Stru_ExtendedSymbolInfo_t* symbol_info, enum Eum_SymbolStorage storage);
    
    /**
     * @brief 添加符号依赖关系
     * @param symbol_info 符号信息指针
     * @param depended_symbol 被依赖符号名称
     * @param type 依赖类型
     * @param line 行号
     * @param column 列号
     * @return 成功返回true，失败返回false
     */
    bool (*add_dependency)(Stru_ExtendedSymbolInfo_t* symbol_info, 
                          const char* depended_symbol, 
                          enum Eum_DependencyType type,
                          uint32_t line, 
                          uint32_t column);
    
    /**
     * @brief 添加符号引用
     * @param symbol_info 符号信息指针
     * @param reference_line 引用行号
     * @param reference_column 引用列号
     * @return 成功返回true，失败返回false
     */
    bool (*add_reference)(Stru_ExtendedSymbolInfo_t* symbol_info, 
                         uint32_t reference_line, 
                         uint32_t reference_column);
    
    /**
     * @brief 重命名符号
     * @param symbol_info 符号信息指针
     * @param new_name 新名称
     * @return 成功返回true，失败返回false
     */
    bool (*rename_symbol)(Stru_ExtendedSymbolInfo_t* symbol_info, const char* new_name);
    
    /**
     * @brief 添加符号别名
     * @param symbol_info 符号信息指针
     * @param alias 别名
     * @return 成功返回true，失败返回false
     */
    bool (*add_alias)(Stru_ExtendedSymbolInfo_t* symbol_info, const char* alias);
    
    /**
     * @brief 获取符号信息字符串表示
     * @param symbol_info 符号信息指针
     * @return 字符串表示，需要调用者释放
     */
    char* (*to_string)(const Stru_ExtendedSymbolInfo_t* symbol_info);
    
    /**
     * @brief 验证符号属性一致性
     * @param symbol_info 符号信息指针
     * @return 验证通过返回true，失败返回false
     */
    bool (*validate)(const Stru_ExtendedSymbolInfo_t* symbol_info);
    
    /**
     * @brief 克隆符号信息
     * @param symbol_info 源符号信息指针
     * @return 克隆的符号信息指针，失败返回NULL
     */
    Stru_ExtendedSymbolInfo_t* (*clone)(const Stru_ExtendedSymbolInfo_t* symbol_info);
    
} Stru_SymbolAttributesInterface_t;

/* ============================================================================
 * 符号依赖关系接口定义
 * ============================================================================ */

/**
 * @struct Stru_SymbolDependencyInterface_t
 * @brief 符号依赖关系接口结构体
 */
typedef struct Stru_SymbolDependencyInterface_t
{
    /**
     * @brief 初始化依赖关系模块
     * @return 成功返回true，失败返回false
     */
    bool (*initialize)(void);
    
    /**
     * @brief 销毁依赖关系模块
     */
    void (*destroy)(void);
    
    /**
     * @brief 创建依赖关系图
     * @param symbols 符号信息数组
     * @param count 符号数量
     * @return 成功返回true，失败返回false
     */
    bool (*create_dependency_graph)(Stru_ExtendedSymbolInfo_t** symbols, uint32_t count);
    
    /**
     * @brief 分析依赖关系
     * @param symbol_name 符号名称
     * @return 依赖关系分析结果字符串，需要调用者释放
     */
    char* (*analyze_dependencies)(const char* symbol_name);
    
    /**
     * @brief 检测循环依赖
     * @return 检测到的循环依赖列表字符串，需要调用者释放
     */
    char* (*detect_cycles)(void);
    
    /**
     * @brief 获取符号的依赖链
     * @param symbol_name 符号名称
     * @param max_depth 最大深度
     * @return 依赖链字符串，需要调用者释放
     */
    char* (*get_dependency_chain)(const char* symbol_name, uint32_t max_depth);
    
    /**
     * @brief 获取依赖符号的符号
     * @param symbol_name 符号名称
     * @return 依赖该符号的符号列表字符串，需要调用者释放
     */
    char* (*get_dependents)(const char* symbol_name);
    
    /**
     * @brief 检查符号是否可安全重命名
     * @param symbol_name 符号名称
     * @return 可安全重命名返回true，否则返回false
     */
    bool (*can_rename_safely)(const char* symbol_name);
    
    /**
     * @brief 获取重命名影响分析
     * @param symbol_name 符号名称
     * @param new_name 新名称
     * @return 影响分析报告字符串，需要调用者释放
     */
    char* (*analyze_rename_impact)(const char* symbol_name, const char* new_name);
    
} Stru_SymbolDependencyInterface_t;

/* ============================================================================
 * 符号重命名接口定义
 * ============================================================================ */

/**
 * @struct Stru_SymbolRenameInterface_t
 * @brief 符号重命名接口结构体
 */
typedef struct Stru_SymbolRenameInterface_t
{
    /**
     * @brief 初始化重命名模块
     * @return 成功返回true，失败返回false
     */
    bool (*initialize)(void);
    
    /**
     * @brief 销毁重命名模块
     */
    void (*destroy)(void);
    
    /**
     * @brief 重命名符号
     * @param old_name 旧名称
     * @param new_name 新名称
     * @param symbols 符号信息数组
     * @param count 符号数量
     * @return 成功返回true，失败返回false
     */
    bool (*rename_symbol)(const char* old_name, const char* new_name, 
                         Stru_ExtendedSymbolInfo_t** symbols, uint32_t count);
    
    /**
     * @brief 批量重命名符号
     * @param rename_map 重命名映射（旧名称->新名称）
     * @param map_size 映射大小
     * @param symbols 符号信息数组
     * @param count 符号数量
     * @return 成功返回true，失败返回false
     */
    bool (*batch_rename)(const char** rename_map, uint32_t map_size,
                        Stru_ExtendedSymbolInfo_t** symbols, uint32_t count);
    
    /**
     * @brief 验证重命名
     * @param old_name 旧名称
     * @param new_name 新名称
     * @param symbols 符号信息数组
     * @param count 符号数量
     * @return 验证通过返回true，失败返回false
     */
    bool (*validate_rename)(const char* old_name, const char* new_name,
                           Stru_ExtendedSymbolInfo_t** symbols, uint32_t count);
    
    /**
     * @brief 获取重命名建议
     * @param symbol_name 符号名称
     * @param context 上下文信息
     * @return 建议的新名称列表字符串，需要调用者释放
     */
    char* (*get_rename_suggestions)(const char* symbol_name, const char* context);
    
    /**
     * @brief 执行重命名并生成报告
     * @param old_name 旧名称
     * @param new_name 新名称
     * @param symbols 符号信息数组
     * @param count 符号数量
     * @return 重命名报告字符串，需要调用者释放
     */
    char* (*rename_with_report)(const char* old_name, const char* new_name,
                               Stru_ExtendedSymbolInfo_t** symbols, uint32_t count);
    
} Stru_SymbolRenameInterface_t;

/* ============================================================================
 * 工厂函数声明
 * ============================================================================ */

/**
 * @brief 创建符号属性接口实例
 * @return 符号属性接口指针，失败返回NULL
 */
Stru_SymbolAttributesInterface_t* F_create_symbol_attributes_interface(void);

/**
 * @brief 创建符号依赖关系接口实例
 * @return 符号依赖关系接口指针，失败返回NULL
 */
Stru_SymbolDependencyInterface_t* F_create_symbol_dependency_interface(void);

/**
 * @brief 创建符号重命名接口实例
 * @return 符号重命名接口指针，失败返回NULL
 */
Stru_SymbolRenameInterface_t* F_create_symbol_rename_interface(void);

/**
 * @brief 销毁符号属性接口实例
 * @param interface 接口指针
 */
void F_destroy_symbol_attributes_interface(Stru_SymbolAttributesInterface_t* interface);

/**
 * @brief 销毁符号依赖关系接口实例
 * @param interface 接口指针
 */
void F_destroy_symbol_dependency_interface(Stru_SymbolDependencyInterface_t* interface);

/**
 * @brief 销毁符号重命名接口实例
 * @param interface 接口指针
 */
void F_destroy_symbol_rename_interface(Stru_SymbolRenameInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_SYMBOL_ATTRIBUTES_H */
