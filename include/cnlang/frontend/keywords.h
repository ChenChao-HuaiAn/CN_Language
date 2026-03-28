#ifndef CN_FRONTEND_KEYWORDS_H
#define CN_FRONTEND_KEYWORDS_H

#include "cnlang/frontend/token.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CN_Language 关键字定义数据结构
 * 用于lexer词法分析和LSP代码补全等工具的统一关键字管理
 */
typedef struct CnKeywordEntry {
    const char *text;        // 关键字文本（UTF-8编码）
    CnTokenKind kind;        // 对应的Token类型
    const char *category;    // 关键字分类（用于LSP补全提示）
} CnKeywordEntry;

/**
 * 获取CN_Language所有关键字列表
 * @param out_count 输出参数，返回关键字数量
 * @return 关键字数组指针（静态存储，不需要释放）
 */
const CnKeywordEntry *cn_frontend_get_keywords(size_t *out_count);

/**
 * 根据词素查找对应的关键字Token类型
 * @param begin 词素起始指针
 * @param length 词素长度
 * @return 对应的Token类型，如果不是关键字返回CN_TOKEN_INVALID
 */
CnTokenKind cn_frontend_lookup_keyword(const char *begin, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_KEYWORDS_H */
