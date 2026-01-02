/******************************************************************************
 * 文件名: CN_keyword_predefined.c
 * 功能: CN_Language预定义关键字初始化
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义所有预定义中文关键字
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_keywords.h"
#include <string.h>

// ============================================================================
// 预定义关键字数据
// ============================================================================

// 变量和类型声明关键字
static const char* PREDEFINED_VARIABLE_KEYWORDS[][4] = {
    {"变量", "variable", "变量声明（类型推断）", "var"},
    {"整数", "integer", "整数类型", "int"},
    {"小数", "decimal", "浮点数类型", "float"},
    {"字符串", "string", "字符串类型", "str"},
    {"布尔", "boolean", "布尔类型", "bool"},
    {"数组", "array", "数组类型", "array"},
    {"结构体", "struct", "结构体类型定义", "struct"},
    {"枚举", "enum", "枚举类型定义", "enum"},
    {"指针", "pointer", "指针类型", "pointer"},
    {"引用", "reference", "引用类型", "reference"}
};

// 控制结构关键字
static const char* PREDEFINED_CONTROL_KEYWORDS[][4] = {
    {"如果", "if", "条件语句", "if"},
    {"否则", "else", "条件语句的else分支", "else"},
    {"当", "while", "while循环", "while"},
    {"循环", "for", "for循环", "for"},
    {"中断", "break", "跳出循环", "break"},
    {"继续", "continue", "继续下一次循环", "continue"},
    {"选择", "switch", "多分支选择语句", "switch"},
    {"情况", "case", "switch语句的分支", "case"},
    {"默认", "default", "switch语句的默认分支", "default"}
};

// 函数相关关键字
static const char* PREDEFINED_FUNCTION_KEYWORDS[][4] = {
    {"函数", "function", "函数声明", "func"},
    {"返回", "return", "返回值", "return"},
    {"主程序", "main", "主函数声明", "main"},
    {"参数", "parameter", "函数参数", "param"},
    {"递归", "recursive", "递归函数", "recursive"}
};

// 逻辑运算符关键字
static const char* PREDEFINED_LOGICAL_KEYWORDS[][4] = {
    {"与", "and", "逻辑与", "and"},
    {"且", "and", "逻辑与（同义词）", "and"},
    {"或", "or", "逻辑或", "or"},
    {"非", "not", "逻辑非", "not"}
};

// 字面量关键字
static const char* PREDEFINED_LITERAL_KEYWORDS[][4] = {
    {"真", "true", "布尔真值", "true"},
    {"假", "false", "布尔假值", "false"},
    {"空", "null", "空值", "null"},
    {"无", "void", "无类型/无返回值", "void"}
};

// 模块系统关键字
static const char* PREDEFINED_MODULE_KEYWORDS[][4] = {
    {"模块", "module", "模块定义", "module"},
    {"导入", "import", "导入模块", "import"},
    {"导出", "export", "导出模块成员", "export"},
    {"包", "package", "包定义", "package"},
    {"命名空间", "namespace", "命名空间定义", "namespace"}
};

// 运算符关键字（有优先级）
static const struct {
    const char* keyword;
    const char* english;
    const char* description;
    const char* short_name;
    int precedence;
} PREDEFINED_OPERATOR_KEYWORDS[] = {
    {"加", "add", "加法运算符", "+", 4},
    {"减", "subtract", "减法运算符", "-", 4},
    {"乘", "multiply", "乘法运算符", "*", 3},
    {"除", "divide", "除法运算符", "/", 3},
    {"取模", "modulo", "取模运算符", "%", 3},
    {"等于", "equal", "等于比较运算符", "==", 7},
    {"不等于", "not_equal", "不等于比较运算符", "!=", 7},
    {"小于", "less", "小于比较运算符", "<", 6},
    {"大于", "greater", "大于比较运算符", ">", 6},
    {"小于等于", "less_equal", "小于等于比较运算符", "<=", 6},
    {"大于等于", "greater_equal", "大于等于比较运算符", ">=", 6}
};

// 类型关键字
static const char* PREDEFINED_TYPE_KEYWORDS[][4] = {
    {"类型", "type", "类型定义", "type"},
    {"接口", "interface", "接口定义", "interface"},
    {"类", "class", "类定义", "class"},
    {"对象", "object", "对象类型", "object"},
    {"泛型", "generic", "泛型类型", "generic"},
    {"模板", "template", "模板类型", "template"}
};

// 其他关键字
static const char* PREDEFINED_OTHER_KEYWORDS[][4] = {
    {"常量", "const", "常量声明", "const"},
    {"静态", "static", "静态声明", "static"},
    {"公开", "public", "公开访问修饰符", "public"},
    {"私有", "private", "私有访问修饰符", "private"},
    {"保护", "protected", "保护访问修饰符", "protected"},
    {"虚拟", "virtual", "虚拟函数", "virtual"},
    {"重写", "override", "函数重写", "override"},
    {"抽象", "abstract", "抽象类/方法", "abstract"},
    {"最终", "final", "最终类/方法", "final"},
    {"同步", "synchronized", "同步方法", "synchronized"},
    {"异步", "async", "异步函数", "async"},
    {"等待", "await", "等待异步操作", "await"},
    {"抛出", "throw", "抛出异常", "throw"},
    {"捕获", "catch", "捕获异常", "catch"},
    {"尝试", "try", "尝试执行代码块", "try"},
    {"最终", "finally", "最终执行代码块", "finally"}
};

// ============================================================================
// 全局预定义关键字表（只读）
// ============================================================================

static Stru_CN_KeywordTable_t g_predefined_keyword_table = {0};
static bool g_predefined_keywords_initialized = false;

// ============================================================================
// 预定义关键字初始化函数
// ============================================================================

bool F_initialize_predefined_keywords(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL)
    {
        return false;
    }

    // 添加变量和类型声明关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_VARIABLE_KEYWORDS) / sizeof(PREDEFINED_VARIABLE_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_VARIABLE_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_VARIABLE,
                          PREDEFINED_VARIABLE_KEYWORDS[i][2],
                          PREDEFINED_VARIABLE_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    // 添加控制结构关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_CONTROL_KEYWORDS) / sizeof(PREDEFINED_CONTROL_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_CONTROL_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_CONTROL,
                          PREDEFINED_CONTROL_KEYWORDS[i][2],
                          PREDEFINED_CONTROL_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    // 添加函数相关关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_FUNCTION_KEYWORDS) / sizeof(PREDEFINED_FUNCTION_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_FUNCTION_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_FUNCTION,
                          PREDEFINED_FUNCTION_KEYWORDS[i][2],
                          PREDEFINED_FUNCTION_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    // 添加逻辑运算符关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_LOGICAL_KEYWORDS) / sizeof(PREDEFINED_LOGICAL_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_LOGICAL_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_LOGICAL,
                          PREDEFINED_LOGICAL_KEYWORDS[i][2],
                          PREDEFINED_LOGICAL_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    // 添加字面量关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_LITERAL_KEYWORDS) / sizeof(PREDEFINED_LITERAL_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_LITERAL_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_LITERAL,
                          PREDEFINED_LITERAL_KEYWORDS[i][2],
                          PREDEFINED_LITERAL_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    // 添加模块系统关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_MODULE_KEYWORDS) / sizeof(PREDEFINED_MODULE_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_MODULE_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_MODULE,
                          PREDEFINED_MODULE_KEYWORDS[i][2],
                          PREDEFINED_MODULE_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    // 添加运算符关键字（有优先级）
    for (size_t i = 0; i < sizeof(PREDEFINED_OPERATOR_KEYWORDS) / sizeof(PREDEFINED_OPERATOR_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_OPERATOR_KEYWORDS[i].keyword,
                          0,
                          Eum_KEYWORD_CATEGORY_OPERATOR,
                          PREDEFINED_OPERATOR_KEYWORDS[i].description,
                          PREDEFINED_OPERATOR_KEYWORDS[i].english,
                          true,
                          PREDEFINED_OPERATOR_KEYWORDS[i].precedence))
        {
            return false;
        }
    }

    // 添加类型关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_TYPE_KEYWORDS) / sizeof(PREDEFINED_TYPE_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_TYPE_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_TYPE,
                          PREDEFINED_TYPE_KEYWORDS[i][2],
                          PREDEFINED_TYPE_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    // 添加其他关键字
    for (size_t i = 0; i < sizeof(PREDEFINED_OTHER_KEYWORDS) / sizeof(PREDEFINED_OTHER_KEYWORDS[0]); i++)
    {
        if (!F_add_keyword(table,
                          PREDEFINED_OTHER_KEYWORDS[i][0],
                          0,
                          Eum_KEYWORD_CATEGORY_OTHER,
                          PREDEFINED_OTHER_KEYWORDS[i][2],
                          PREDEFINED_OTHER_KEYWORDS[i][1],
                          true,
                          0))
        {
            return false;
        }
    }

    return true;
}

const Stru_CN_KeywordTable_t* F_get_predefined_keyword_table(void)
{
    // 如果尚未初始化，初始化全局预定义关键字表
    if (!g_predefined_keywords_initialized)
    {
        // 创建临时表
        Stru_CN_KeywordTable_t* temp_table = F_create_keyword_table();
        if (temp_table == NULL)
        {
            return NULL;
        }

        // 复制到全局表
        g_predefined_keyword_table = *temp_table;
        
        // 注意：这里我们"窃取"了temp_table的内存，所以不需要释放temp_table
        // 但需要确保temp_table的指针被清空，避免双重释放
        temp_table->keywords = NULL;
        temp_table->count = 0;
        temp_table->capacity = 0;
        F_destroy_keyword_table(temp_table);

        g_predefined_keywords_initialized = true;
    }

    return &g_predefined_keyword_table;
}

// ============================================================================
// 辅助函数：获取预定义关键字数量统计
// ============================================================================

size_t F_get_predefined_keyword_count_by_category(Eum_CN_KeywordCategory_t category)
{
    switch (category)
    {
        case Eum_KEYWORD_CATEGORY_VARIABLE:
            return sizeof(PREDEFINED_VARIABLE_KEYWORDS) / sizeof(PREDEFINED_VARIABLE_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_CONTROL:
            return sizeof(PREDEFINED_CONTROL_KEYWORDS) / sizeof(PREDEFINED_CONTROL_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_FUNCTION:
            return sizeof(PREDEFINED_FUNCTION_KEYWORDS) / sizeof(PREDEFINED_FUNCTION_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_LOGICAL:
            return sizeof(PREDEFINED_LOGICAL_KEYWORDS) / sizeof(PREDEFINED_LOGICAL_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_LITERAL:
            return sizeof(PREDEFINED_LITERAL_KEYWORDS) / sizeof(PREDEFINED_LITERAL_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_MODULE:
            return sizeof(PREDEFINED_MODULE_KEYWORDS) / sizeof(PREDEFINED_MODULE_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_OPERATOR:
            return sizeof(PREDEFINED_OPERATOR_KEYWORDS) / sizeof(PREDEFINED_OPERATOR_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_TYPE:
            return sizeof(PREDEFINED_TYPE_KEYWORDS) / sizeof(PREDEFINED_TYPE_KEYWORDS[0]);
        
        case Eum_KEYWORD_CATEGORY_OTHER:
            return sizeof(PREDEFINED_OTHER_KEYWORDS) / sizeof(PREDEFINED_OTHER_KEYWORDS[0]);
        
        default:
            return 0;
    }
}

size_t F_get_total_predefined_keyword_count(void)
{
    size_t total = 0;
    
    total += sizeof(PREDEFINED_VARIABLE_KEYWORDS) / sizeof(PREDEFINED_VARIABLE_KEYWORDS[0]);
    total += sizeof(PREDEFINED_CONTROL_KEYWORDS) / sizeof(PREDEFINED_CONTROL_KEYWORDS[0]);
    total += sizeof(PREDEFINED_FUNCTION_KEYWORDS) / sizeof(PREDEFINED_FUNCTION_KEYWORDS[0]);
    total += sizeof(PREDEFINED_LOGICAL_KEYWORDS) / sizeof(PREDEFINED_LOGICAL_KEYWORDS[0]);
    total += sizeof(PREDEFINED_LITERAL_KEYWORDS) / sizeof(PREDEFINED_LITERAL_KEYWORDS[0]);
    total += sizeof(PREDEFINED_MODULE_KEYWORDS) / sizeof(PREDEFINED_MODULE_KEYWORDS[0]);
    total += sizeof(PREDEFINED_OPERATOR_KEYWORDS) / sizeof(PREDEFINED_OPERATOR_KEYWORDS[0]);
    total += sizeof(PREDEFINED_TYPE_KEYWORDS) / sizeof(PREDEFINED_TYPE_KEYWORDS[0]);
    total += sizeof(PREDEFINED_OTHER_KEYWORDS) / sizeof(PREDEFINED_OTHER_KEYWORDS[0]);
    
    return total;
}
