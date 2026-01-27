#include "cnlang/frontend/keywords.h"
#include <string.h>

/**
 * CN_Language 关键字定义表
 * 注意：用户已修改lsp_server.c中"空类型"为"空"，此处与用户修改保持一致
 */
static const CnKeywordEntry g_keywords[] = {
    // 控制流关键字 (10个)
    {"如果", CN_TOKEN_KEYWORD_IF, "控制流关键字"},
    {"否则", CN_TOKEN_KEYWORD_ELSE, "控制流关键字"},
    {"当", CN_TOKEN_KEYWORD_WHILE, "控制流关键字"},
    {"循环", CN_TOKEN_KEYWORD_FOR, "控制流关键字"},
    {"返回", CN_TOKEN_KEYWORD_RETURN, "控制流关键字"},
    {"中断", CN_TOKEN_KEYWORD_BREAK, "控制流关键字"},
    {"继续", CN_TOKEN_KEYWORD_CONTINUE, "控制流关键字"},
    {"选择", CN_TOKEN_KEYWORD_SWITCH, "控制流关键字"},
    {"情况", CN_TOKEN_KEYWORD_CASE, "控制流关键字"},
    {"默认", CN_TOKEN_KEYWORD_DEFAULT, "控制流关键字"},

    // 类型关键字 (7个)
    {"整数", CN_TOKEN_KEYWORD_INT, "类型关键字"},
    {"小数", CN_TOKEN_KEYWORD_FLOAT, "类型关键字"},
    {"字符串", CN_TOKEN_KEYWORD_STRING, "类型关键字"},
    {"布尔", CN_TOKEN_KEYWORD_BOOL, "类型关键字"},
    {"空类型", CN_TOKEN_KEYWORD_VOID, "类型关键字"},  // 注意：空类型关键字是"空类型"
    {"结构体", CN_TOKEN_KEYWORD_STRUCT, "类型关键字"},
    {"枚举", CN_TOKEN_KEYWORD_ENUM, "类型关键字"},

    // 声明关键字 (6个)
    {"函数", CN_TOKEN_KEYWORD_FN, "声明关键字"},
    {"变量", CN_TOKEN_KEYWORD_VAR, "声明关键字"},
    {"模块", CN_TOKEN_KEYWORD_MODULE, "声明关键字"},
    {"导入", CN_TOKEN_KEYWORD_IMPORT, "声明关键字"},
    {"公开", CN_TOKEN_KEYWORD_PUBLIC, "声明关键字"},
    {"私有", CN_TOKEN_KEYWORD_PRIVATE, "声明关键字"},

    // 常量关键字 (3个)
    {"真", CN_TOKEN_KEYWORD_TRUE, "常量关键字"},
    {"假", CN_TOKEN_KEYWORD_FALSE, "常量关键字"},
    {"无", CN_TOKEN_KEYWORD_NULL, "常量关键字"},  // 注意：空值常量是"无"

    // 预留关键字 (7个)
    {"命名空间", CN_TOKEN_KEYWORD_NAMESPACE, "预留关键字"},
    {"接口", CN_TOKEN_KEYWORD_INTERFACE, "预留关键字"},
    {"类", CN_TOKEN_KEYWORD_CLASS, "预留关键字"},
    {"模板", CN_TOKEN_KEYWORD_TEMPLATE, "预留关键字"},
    {"常量", CN_TOKEN_KEYWORD_CONST, "预留关键字"},
    {"静态", CN_TOKEN_KEYWORD_STATIC, "预留关键字"},
    {"保护", CN_TOKEN_KEYWORD_PROTECTED, "预留关键字"},
    {"虚拟", CN_TOKEN_KEYWORD_VIRTUAL, "预留关键字"},
    {"重写", CN_TOKEN_KEYWORD_OVERRIDE, "预留关键字"},
    {"抽象", CN_TOKEN_KEYWORD_ABSTRACT, "预留关键字"},
};

const CnKeywordEntry *cn_frontend_get_keywords(size_t *out_count)
{
    if (out_count) {
        *out_count = sizeof(g_keywords) / sizeof(g_keywords[0]);
    }
    return g_keywords;
}

CnTokenKind cn_frontend_lookup_keyword(const char *begin, size_t length)
{
    if (!begin || length == 0) {
        return CN_TOKEN_INVALID;
    }

    // 遍历关键字表进行匹配
    size_t count = sizeof(g_keywords) / sizeof(g_keywords[0]);
    for (size_t i = 0; i < count; i++) {
        size_t keyword_len = strlen(g_keywords[i].text);
        if (length == keyword_len && 
            memcmp(begin, g_keywords[i].text, length) == 0) {
            return g_keywords[i].kind;
        }
    }

    return CN_TOKEN_INVALID;
}
