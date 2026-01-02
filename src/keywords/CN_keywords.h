/******************************************************************************
 * 文件名: CN_keywords.h
 * 功能: CN_Language中文关键字定义和管理
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义所有中文关键字
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_KEYWORDS_H
#define CN_KEYWORDS_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// 关键字分类枚举
// ============================================================================

/**
 * @brief 关键字分类枚举
 * @note 用于标识关键字的功能类别
 */
typedef enum Eum_CN_KeywordCategory_t
{
    Eum_KEYWORD_CATEGORY_VARIABLE,      /**< 变量和类型声明关键字 */
    Eum_KEYWORD_CATEGORY_CONTROL,       /**< 控制结构关键字 */
    Eum_KEYWORD_CATEGORY_FUNCTION,      /**< 函数相关关键字 */
    Eum_KEYWORD_CATEGORY_LOGICAL,       /**< 逻辑运算符关键字 */
    Eum_KEYWORD_CATEGORY_LITERAL,       /**< 字面量关键字 */
    Eum_KEYWORD_CATEGORY_MODULE,        /**< 模块系统关键字 */
    Eum_KEYWORD_CATEGORY_OPERATOR,      /**< 运算符关键字 */
    Eum_KEYWORD_CATEGORY_TYPE,          /**< 类型关键字 */
    Eum_KEYWORD_CATEGORY_OTHER          /**< 其他关键字 */
} Eum_CN_KeywordCategory_t;

// ============================================================================
// 关键字信息结构体
// ============================================================================

/**
 * @brief 关键字信息结构体
 * @note 存储单个关键字的完整信息
 */
typedef struct Stru_CN_KeywordInfo_t
{
    const char* keyword;                /**< 关键字字符串（UTF-8编码） */
    size_t length;                      /**< 关键字长度（字节数） */
    Eum_CN_KeywordCategory_t category;  /**< 关键字分类 */
    const char* description;            /**< 关键字功能描述 */
    const char* english_equivalent;     /**< 对应的英文关键字 */
    bool is_reserved;                   /**< 是否为保留关键字 */
    int precedence;                     /**< 运算符优先级（仅对运算符有效） */
} Stru_CN_KeywordInfo_t;

// ============================================================================
// 关键字表结构体
// ============================================================================

/**
 * @brief 关键字表结构体
 * @note 管理所有关键字的集合
 */
typedef struct Stru_CN_KeywordTable_t
{
    Stru_CN_KeywordInfo_t* keywords;    /**< 关键字数组 */
    size_t count;                       /**< 关键字数量 */
    size_t capacity;                    /**< 数组容量 */
} Stru_CN_KeywordTable_t;

// ============================================================================
// 常量定义
// ============================================================================

/**
 * @brief 最大关键字长度（UTF-8字节数）
 */
#define CN_MAX_KEYWORD_LENGTH 32

/**
 * @brief 默认关键字表容量
 */
#define CN_DEFAULT_KEYWORD_TABLE_CAPACITY 64

/**
 * @brief 关键字描述最大长度
 */
#define CN_MAX_KEYWORD_DESCRIPTION_LENGTH 256

// ============================================================================
// 关键字表管理函数
// ============================================================================

/**
 * @brief 创建关键字表
 * @return 新创建的关键字表，失败返回NULL
 * @note 初始化时包含所有预定义的关键字
 */
Stru_CN_KeywordTable_t* F_create_keyword_table(void);

/**
 * @brief 销毁关键字表
 * @param table 要销毁的关键字表
 */
void F_destroy_keyword_table(Stru_CN_KeywordTable_t* table);

/**
 * @brief 向关键字表添加新关键字
 * @param table 关键字表
 * @param keyword 关键字字符串
 * @param length 关键字长度（字节数），如果为0则自动计算
 * @param category 关键字分类
 * @param description 关键字描述
 * @param english_equivalent 对应的英文关键字
 * @param is_reserved 是否为保留关键字
 * @param precedence 运算符优先级（仅对运算符有效）
 * @return 添加成功返回true，失败返回false
 */
bool F_add_keyword(Stru_CN_KeywordTable_t* table,
                   const char* keyword,
                   size_t length,
                   Eum_CN_KeywordCategory_t category,
                   const char* description,
                   const char* english_equivalent,
                   bool is_reserved,
                   int precedence);

/**
 * @brief 从关键字表移除关键字
 * @param table 关键字表
 * @param keyword 要移除的关键字
 * @param length 关键字长度（字节数）
 * @return 移除成功返回true，关键字不存在返回false
 */
bool F_remove_keyword(Stru_CN_KeywordTable_t* table,
                      const char* keyword,
                      size_t length);

// ============================================================================
// 关键字查询函数
// ============================================================================

/**
 * @brief 检查字符串是否为关键字
 * @param table 关键字表
 * @param str 要检查的字符串
 * @param length 字符串长度（字节数）
 * @return 如果是关键字返回true，否则返回false
 */
bool F_is_keyword(Stru_CN_KeywordTable_t* table,
                  const char* str,
                  size_t length);

/**
 * @brief 获取关键字信息
 * @param table 关键字表
 * @param keyword 关键字字符串
 * @param length 关键字长度（字节数）
 * @return 关键字信息指针，如果关键字不存在返回NULL
 */
const Stru_CN_KeywordInfo_t* F_get_keyword_info(Stru_CN_KeywordTable_t* table,
                                                const char* keyword,
                                                size_t length);

/**
 * @brief 根据分类获取关键字列表
 * @param table 关键字表
 * @param category 关键字分类
 * @param count 输出参数，返回找到的关键字数量
 * @return 关键字信息指针数组，调用者需要释放内存
 */
Stru_CN_KeywordInfo_t** F_get_keywords_by_category(Stru_CN_KeywordTable_t* table,
                                                   Eum_CN_KeywordCategory_t category,
                                                   size_t* count);

/**
 * @brief 查找对应的英文关键字
 * @param table 关键字表
 * @param chinese_keyword 中文关键字
 * @param length 关键字长度（字节数）
 * @return 对应的英文关键字，如果不存在返回NULL
 */
const char* F_get_english_equivalent(Stru_CN_KeywordTable_t* table,
                                     const char* chinese_keyword,
                                     size_t length);

/**
 * @brief 查找对应的中文关键字
 * @param table 关键字表
 * @param english_keyword 英文关键字
 * @return 对应的中文关键字，如果不存在返回NULL
 */
const char* F_get_chinese_equivalent(Stru_CN_KeywordTable_t* table,
                                     const char* english_keyword);

// ============================================================================
// 关键字表迭代函数
// ============================================================================

/**
 * @brief 获取关键字表迭代器（开始）
 * @param table 关键字表
 * @return 第一个关键字的索引，如果表为空返回0
 */
size_t F_keyword_table_begin(Stru_CN_KeywordTable_t* table);

/**
 * @brief 获取关键字表迭代器（结束）
 * @param table 关键字表
 * @return 最后一个关键字的索引+1
 */
size_t F_keyword_table_end(Stru_CN_KeywordTable_t* table);

/**
 * @brief 获取下一个关键字
 * @param table 关键字表
 * @param index 当前索引
 * @return 下一个关键字的索引，如果到达末尾返回F_keyword_table_end(table)
 */
size_t F_keyword_table_next(Stru_CN_KeywordTable_t* table, size_t index);

/**
 * @brief 获取关键字信息（通过索引）
 * @param table 关键字表
 * @param index 关键字索引
 * @return 关键字信息指针，如果索引无效返回NULL
 */
const Stru_CN_KeywordInfo_t* F_get_keyword_by_index(Stru_CN_KeywordTable_t* table,
                                                    size_t index);

// ============================================================================
// 关键字表统计函数
// ============================================================================

/**
 * @brief 获取关键字总数
 * @param table 关键字表
 * @return 关键字总数
 */
size_t F_get_keyword_count(Stru_CN_KeywordTable_t* table);

/**
 * @brief 获取分类关键字数量
 * @param table 关键字表
 * @param category 关键字分类
 * @return 该分类的关键字数量
 */
size_t F_get_keyword_count_by_category(Stru_CN_KeywordTable_t* table,
                                       Eum_CN_KeywordCategory_t category);

/**
 * @brief 获取保留关键字数量
 * @param table 关键字表
 * @return 保留关键字数量
 */
size_t F_get_reserved_keyword_count(Stru_CN_KeywordTable_t* table);

// ============================================================================
// 关键字表导入导出函数
// ============================================================================

/**
 * @brief 从文件加载关键字表
 * @param filename 文件名
 * @return 加载的关键字表，失败返回NULL
 * @note 文件格式为JSON或自定义格式
 */
Stru_CN_KeywordTable_t* F_load_keyword_table_from_file(const char* filename);

/**
 * @brief 保存关键字表到文件
 * @param table 关键字表
 * @param filename 文件名
 * @return 保存成功返回true，失败返回false
 */
bool F_save_keyword_table_to_file(Stru_CN_KeywordTable_t* table,
                                  const char* filename);

/**
 * @brief 导出关键字表为C代码
 * @param table 关键字表
 * @param filename 输出文件名
 * @return 导出成功返回true，失败返回false
 * @note 生成可以直接编译的C代码文件
 */
bool F_export_keyword_table_to_c_code(Stru_CN_KeywordTable_t* table,
                                      const char* filename);

// ============================================================================
// 预定义关键字访问函数
// ============================================================================

/**
 * @brief 获取预定义的关键字表（只读）
 * @return 预定义的关键字表指针
 * @note 返回的表是只读的，不要修改
 */
const Stru_CN_KeywordTable_t* F_get_predefined_keyword_table(void);

/**
 * @brief 初始化预定义关键字
 * @param table 关键字表
 * @return 初始化成功返回true，失败返回false
 * @note 将CN语言的所有预定义关键字添加到表中
 */
bool F_initialize_predefined_keywords(Stru_CN_KeywordTable_t* table);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 打印关键字信息
 * @param keyword_info 关键字信息
 */
void F_print_keyword_info(const Stru_CN_KeywordInfo_t* keyword_info);

/**
 * @brief 打印整个关键字表
 * @param table 关键字表
 */
void F_print_keyword_table(Stru_CN_KeywordTable_t* table);

/**
 * @brief 按分类打印关键字表
 * @param table 关键字表
 */
void F_print_keyword_table_by_category(Stru_CN_KeywordTable_t* table);

/**
 * @brief 比较两个关键字（用于排序）
 * @param a 第一个关键字
 * @param b 第二个关键字
 * @return 比较结果：<0 if a<b, 0 if a==b, >0 if a>b
 */
int F_compare_keywords(const void* a, const void* b);

/**
 * @brief 对关键字表排序
 * @param table 关键字表
 * @param compare_func 比较函数，如果为NULL使用默认比较函数
 */
void F_sort_keyword_table(Stru_CN_KeywordTable_t* table,
                          int (*compare_func)(const void*, const void*));

#endif // CN_KEYWORDS_H
