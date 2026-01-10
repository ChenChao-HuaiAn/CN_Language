# 符号属性扩展模块 API 文档

## 概述

本文档描述了CN_Language符号属性扩展模块的应用程序编程接口（API）。符号属性扩展模块提供了符号属性扩展功能，包括符号可见性级别、生命周期、访问权限、存储类别、依赖关系跟踪和重命名支持。

## 文件结构

```
src/core/semantic/symbol_attributes/
├── CN_symbol_attributes.h      # 头文件 - 接口定义
├── CN_symbol_attributes.c      # 源文件 - 接口实现
└── README.md                   # 模块文档
```

## 枚举定义

### 符号可见性枚举

```c
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
```

### 符号生命周期枚举

```c
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
```

### 符号访问权限枚举

```c
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
```

### 符号存储类别枚举

```c
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
```

### 依赖类型枚举

```c
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
```

## 结构体定义

### 符号依赖关系结构体

```c
/**
 * @struct Stru_SymbolDependency_t
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
```

### 符号引用结构体

```c
/**
 * @struct Stru_SymbolReference_t
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
```

### 扩展符号信息结构体

```c
/**
 * @struct Stru_ExtendedSymbolInfo_t
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
```

## 接口定义

### 符号属性接口

```c
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
```

### 符号依赖关系接口

```c
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
```

### 符号重命名接口

```c
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
```

## 工厂函数

### 符号属性接口工厂函数

```c
/**
 * @brief 创建符号属性接口实例
 * @return 符号属性接口指针，失败返回NULL
 */
Stru_SymbolAttributesInterface_t* F_create_symbol_attributes_interface(void);

/**
 * @brief 销毁符号属性接口实例
 * @param interface 接口指针
 */
void F_destroy_symbol_attributes_interface(Stru_SymbolAttributesInterface_t* interface);
```

### 符号依赖关系接口工厂函数

```c
/**
 * @brief 创建符号依赖关系接口实例
 * @return 符号依赖关系接口指针，失败返回NULL
 */
Stru_SymbolDependencyInterface_t* F_create_symbol_dependency_interface(void);

/**
 * @brief 销毁符号依赖关系接口实例
 * @param interface 接口指针
 */
void F_destroy_symbol_dependency_interface(Stru_SymbolDependencyInterface_t* interface);
```

### 符号重命名接口工厂函数

```c
/**
 * @brief 创建符号重命名接口实例
 * @return 符号重命名接口指针，失败返回NULL
 */
Stru_SymbolRenameInterface_t* F_create_symbol_rename_interface(void);

/**
 * @brief 销毁符号重命名接口实例
 * @param interface 接口指针
 */
void F_destroy_symbol_rename_interface(Stru_SymbolRenameInterface_t* interface);
```

## 使用示例

### 基本使用示例

```c
#include "src/core/semantic/symbol_attributes/CN_symbol_attributes.h"

// 创建符号属性接口
Stru_SymbolAttributesInterface_t* attr_interface = F_create_symbol_attributes_interface();
if (!attr_interface) {
    // 处理错误
    return;
}

// 初始化符号属性接口
if (!attr_interface->initialize()) {
    // 处理初始化失败
    F_destroy_symbol_attributes_interface(attr_interface);
    return;
}

// 创建符号信息
Stru_ExtendedSymbolInfo_t* symbol_info = attr_interface->create_symbol_info("myVariable", "int");
if (!symbol_info) {
    // 处理创建失败
    F_destroy_symbol_attributes_interface(attr_interface);
    return;
}

// 设置符号属性
attr_interface->set_visibility(symbol_info, Eum_VISIBILITY_PUBLIC);
attr_interface->set_lifetime(symbol_info, Eum_LIFETIME_STATIC);
attr_interface->set_access(symbol_info, Eum_ACCESS_READ_WRITE);
attr_interface->set_storage(symbol_info, Eum_STORAGE_AUTO);

// 添加依赖关系
attr_interface->add_dependency(symbol_info, "otherFunction", Eum_DEPENDENCY_USES, 10, 5);

// 添加引用
attr_interface->add_reference(symbol_info, 15, 8);
attr_interface->add_reference(symbol_info, 20, 12);

// 重命名符号
attr_interface->rename_symbol(symbol_info, "renamedVariable");

// 添加别名
attr_interface->add_alias(symbol_info, "oldName");
attr_interface->add_alias(symbol_info, "aliasName");

// 获取字符串表示
char* str_repr = attr_interface->to_string(symbol_info);
if (str_repr) {
    printf("符号信息: %s\n", str_repr);
    free(str_repr);
}

// 验证符号属性
bool is_valid = attr_interface->validate(symbol_info);
if (!is_valid) {
    printf("符号属性验证失败\n");
}

// 克隆符号信息
Stru_ExtendedSymbolInfo_t* cloned_info = attr_interface->clone(symbol_info);
if (cloned_info) {
    // 使用克隆的符号信息
    attr_interface->destroy_symbol_info(cloned_info);
}

// 销毁符号信息
attr_interface->destroy_symbol_info(symbol_info);

// 销毁符号属性接口
F_destroy_symbol_attributes_interface(attr_interface);
```

### 依赖关系分析示例

```c
#include "src/core/semantic/symbol_attributes/CN_symbol_attributes.h"

// 创建符号依赖关系接口
Stru_SymbolDependencyInterface_t* dep_interface = F_create_symbol_dependency_interface();
if (!dep_interface) {
    // 处理错误
    return;
}

// 初始化依赖关系接口
if (!dep_interface->initialize()) {
    // 处理初始化失败
    F_destroy_symbol_dependency_interface(dep_interface);
    return;
}

// 创建符号信息数组
Stru_ExtendedSymbolInfo_t* symbols[3];
// ... 创建和配置符号信息 ...

// 创建依赖关系图
if (dep_interface->create_dependency_graph(symbols, 3)) {
    // 分析依赖关系
    char* analysis = dep_interface->analyze_dependencies("myFunction");
    if (analysis) {
        printf("依赖关系分析: %s\n", analysis);
        free(analysis);
    }
    
    // 检测循环依赖
    char* cycles = dep_interface->detect_cycles();
    if (cycles) {
        printf("循环依赖: %s\n", cycles);
        free(cycles);
    }
    
    // 获取依赖链
    char* chain = dep_interface->get_dependency_chain("myVariable", 5);
    if (chain) {
        printf("依赖链: %s\n", chain);
        free(chain);
    }
    
    // 获取依赖该符号的符号
    char* dependents = dep_interface->get_dependents("myFunction");
    if (dependents) {
        printf("依赖该符号的符号: %s\n", dependents);
        free(dependents);
    }
    
    // 检查是否可安全重命名
    bool can_rename = dep_interface->can_rename_safely("oldName");
    if (can_rename) {
        printf("符号可以安全重命名\n");
    }
    
    // 分析重命名影响
    char* impact = dep_interface->analyze_rename_impact("oldName", "newName");
    if (impact) {
        printf("重命名影响分析: %s\n", impact);
        free(impact);
    }
}

// 销毁依赖关系接口
F_destroy_symbol_dependency_interface(dep_interface);
```

### 符号重命名示例

```c
#include "src/core/semantic/symbol_attributes/CN_symbol_attributes.h"

// 创建符号重命名接口
Stru_SymbolRenameInterface_t* rename_interface = F_create_symbol_rename_interface();
if (!rename_interface) {
    // 处理错误
    return;
}

// 初始化重命名接口
if (!rename_interface->initialize()) {
    // 处理初始化失败
    F_destroy_symbol_rename_interface(rename_interface);
    return;
}

// 创建符号信息数组
Stru_ExtendedSymbolInfo_t* symbols[5];
// ... 创建和配置符号信息 ...

// 验证重命名
bool is_valid = rename_interface->validate_rename("oldName", "newName", symbols, 5);
if (is_valid) {
    // 执行重命名
    bool success = rename_interface->rename_symbol("oldName", "newName", symbols, 5);
    if (success) {
        printf("重命名成功\n");
        
        // 获取重命名报告
        char* report = rename_interface->rename_with_report("oldName", "newName", symbols, 5);
        if (report) {
            printf("重命名报告: %s\n", report);
            free(report);
        }
    }
}

// 获取重命名建议
char* suggestions = rename_interface->get_rename_suggestions("var1", "计数器变量");
if (suggestions) {
    printf("重命名建议: %s\n", suggestions);
    free(suggestions);
}

// 批量重命名
const char* rename_map[][2] = {
    {"oldName1", "newName1"},
    {"oldName2", "newName2"},
    {"oldName3", "newName3"}
};
bool batch_success = rename_interface->batch_rename((const char**)rename_map, 3, symbols, 5);
if (batch_success) {
    printf("批量重命名成功\n");
}

// 销毁重命名接口
F_destroy_symbol_rename_interface(rename_interface);
```

## 错误处理

### 返回值约定

1. **布尔返回值**：
   - `true`：操作成功
   - `false`：操作失败

2. **指针返回值**：
   - 非NULL：操作成功，返回有效指针
   - NULL：操作失败

3. **字符串返回值**：
   - 非NULL：操作成功，返回动态分配的字符串，需要调用者释放
   - NULL：操作失败

### 错误类型

1. **内存分配错误**：当malloc失败时返回NULL
2. **参数错误**：当输入参数无效时返回false或NULL
3. **初始化错误**：当模块初始化失败时返回false
4. **状态错误**：当模块处于无效状态时返回false
5. **依赖关系错误**：当依赖关系分析失败时返回false或NULL

### 错误恢复

1. **资源清理**：所有创建的资源都必须通过对应的销毁函数释放
2. **状态重置**：通过重新初始化模块重置状态
3. **错误传播**：错误通过返回值传递，不依赖全局状态

## 性能考虑

### 内存管理

1. **接口创建**：每个工厂函数都会分配内存，必须通过对应的销毁函数释放
2. **符号信息**：符号信息结构体包含动态分配的字符串，必须通过destroy_symbol_info释放
3. **链表管理**：依赖关系和引用使用链表存储，需要遍历释放
4. **字符串返回**：返回字符串的函数分配内存，调用者负责释放

### 时间复杂度

1. **符号属性操作**：O(1) 对于大多数属性设置操作
2. **依赖关系添加**：O(1) 添加到链表头部
3. **依赖关系分析**：O(n + e)，n为符号数，e为依赖边数
4. **循环依赖检测**：O(n + e)，使用深度优先搜索
5. **符号重命名**：O(n)，n为符号数
6. **批量重命名**：O(m × n)，m为重命名映射大小，n为符号数

### 空间复杂度

1. **符号信息存储**：O(n)，n为符号数
2. **依赖关系图**：O(n + e)，n为符号数，e为依赖边数
3. **引用信息**：O(r)，r为引用总数
4. **重命名映射**：O(m)，m为重命名映射大小

### 线程安全

1. **非线程安全**：当前实现不支持多线程并发访问
2. **独立实例**：每个线程应创建独立的接口实例
3. **共享数据**：符号信息结构体不是线程安全的，需要外部同步

## 扩展指南

### 添加新的符号属性

1. **定义枚举**：在CN_symbol_attributes.h中添加新的枚举值
2. **扩展结构体**：在Stru_ExtendedSymbolInfo_t中添加新的字段
3. **添加接口函数**：在Stru_SymbolAttributesInterface_t中添加新的函数指针
4. **实现功能**：在CN_symbol_attributes.c中实现新的功能
5. **更新文档**：更新API文档和README

### 自定义依赖关系分析

1. **继承接口**：创建新的结构体继承现有接口
2. **重写函数**：重写特定的分析函数
3. **算法扩展**：实现新的依赖关系分析算法
4. **性能优化**：优化现有算法的性能

### 集成到其他模块

1. **语义分析器集成**：在语义分析过程中收集符号属性
2. **优化器集成**：使用符号属性进行优化决策
3. **代码生成器集成**：根据符号属性生成不同的代码
4. **调试器集成**：在调试过程中显示符号属性

## 版本兼容性

### API版本

- **v1.0.0**：初始版本，提供符号属性扩展、依赖关系跟踪和重命名支持
- **向后兼容**：后续版本保持API向后兼容
- **扩展接口**：通过添加新函数扩展接口，不影响现有代码

### 二进制兼容性

- **结构体布局**：保持结构体布局不变
- **函数指针顺序**：保持函数指针顺序不变
- **枚举值顺序**：保持枚举值顺序不变

## 相关文档

- [模块README](../../../src/core/semantic/symbol_attributes/README.md)
- [架构设计文档](../../../../docs/architecture/)
- [编码规范](../../../../docs/specifications/)
- [语义分析接口文档](../CN_semantic_interface.md)

## 维护者

- CN_Language架构委员会

## 许可证

MIT License
