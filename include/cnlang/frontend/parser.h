#ifndef CN_FRONTEND_PARSER_H
#define CN_FRONTEND_PARSER_H

#include <stdbool.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CnDiagnostics;

// 前向声明解析器上下文
typedef struct CnParser CnParser;

// 创建 / 销毁解析器上下文
CnParser *cn_frontend_parser_new(CnLexer *lexer);
void cn_frontend_parser_free(CnParser *parser);
void cn_frontend_parser_set_diagnostics(CnParser *parser, struct CnDiagnostics *diagnostics);

// 解析整个程序，生成 AST 根节点
// 返回值：true 表示解析成功（可能带有可恢复错误），false 表示发生致命错误
bool cn_frontend_parse_program(CnParser *parser, CnAstProgram **out_program);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_PARSER_H */
