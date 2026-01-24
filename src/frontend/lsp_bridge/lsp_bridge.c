#include "cnlang/frontend/lsp_bridge.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include <stdlib.h>
#include <string.h>

// 分析文档：执行 Lexer + Parser + Semantics，返回分析结果
CnLspDocumentAnalysis *cn_lsp_analyze_document(
    const char *source,
    size_t source_length,
    const char *uri)
{
    if (!source || !uri) {
        return NULL;
    }

    // 分配分析结果结构
    CnLspDocumentAnalysis *analysis = (CnLspDocumentAnalysis *)malloc(sizeof(CnLspDocumentAnalysis));
    if (!analysis) {
        return NULL;
    }

    analysis->program = NULL;
    analysis->global_scope = NULL;
    analysis->diagnostics = NULL;
    analysis->diagnostic_count = 0;
    analysis->source = source;
    analysis->source_length = source_length;

    // 初始化诊断系统
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    // 词法分析
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, source_length, uri);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 语法分析
    CnParser *parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        cn_support_diagnostics_free(&diagnostics);
        free(analysis);
        return NULL;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    analysis->program = program;

    // 语义分析（如果解析成功）
    if (program && cn_support_diagnostics_error_count(&diagnostics) == 0) {
        CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
        if (global_scope) {
            cn_sem_resolve_names(global_scope, program, &diagnostics);
            cn_sem_check_types(global_scope, program, &diagnostics);
            analysis->global_scope = global_scope;
        }
    }

    // 转换诊断信息
    cn_lsp_convert_diagnostics(&diagnostics, &analysis->diagnostics, &analysis->diagnostic_count);

    // 清理
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);

    return analysis;
}

// 释放分析结果
void cn_lsp_free_analysis(CnLspDocumentAnalysis *analysis)
{
    if (!analysis) {
        return;
    }

    if (analysis->program) {
        cn_frontend_ast_program_free(analysis->program);
    }

    if (analysis->global_scope) {
        cn_sem_scope_free(analysis->global_scope);
    }

    if (analysis->diagnostics) {
        // 释放每条诊断的消息字符串（如果是动态分配的）
        for (size_t i = 0; i < analysis->diagnostic_count; i++) {
            // message 和 source 是静态字符串，不需要释放
        }
        free(analysis->diagnostics);
    }

    free(analysis);
}

// 查找指定位置的符号定义
bool cn_lsp_find_definition(
    CnLspDocumentAnalysis *analysis,
    CnLspPosition position,
    CnLspSymbolInfo *out_symbol)
{
    if (!analysis || !analysis->source || !out_symbol) {
        return false;
    }

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, analysis->source, analysis->source_length, "<lsp>");

    CnToken token;
    CnToken ident_token;
    int found = 0;

    while (cn_frontend_lexer_next_token(&lexer, &token)) {
        if (token.kind == CN_TOKEN_EOF) {
            break;
        }

        int line0 = (token.line > 0) ? token.line - 1 : 0;
        int col0 = (token.column > 0) ? token.column - 1 : 0;
        int col1 = col0 + (int)token.lexeme_length;

        if (position.line == line0 && position.column >= col0 && position.column < col1) {
            if (token.kind != CN_TOKEN_IDENT) {
                return false;
            }
            ident_token = token;
            found = 1;
            break;
        }
    }

    if (!found) {
        return false;
    }

    out_symbol->name = ident_token.lexeme_begin;
    out_symbol->kind = CN_SEM_SYMBOL_VARIABLE;
    out_symbol->type = NULL;

    out_symbol->definition_range.start.line = (ident_token.line > 0) ? ident_token.line - 1 : 0;
    out_symbol->definition_range.start.column = (ident_token.column > 0) ? ident_token.column - 1 : 0;
    out_symbol->definition_range.end.line = out_symbol->definition_range.start.line;
    out_symbol->definition_range.end.column = out_symbol->definition_range.start.column + (int)ident_token.lexeme_length;

    return true;
}

// 查找符号的所有引用
bool cn_lsp_find_references(
    CnLspDocumentAnalysis *analysis,
    CnLspPosition position,
    CnLspRange **out_ranges,
    size_t *out_count)
{
    if (!analysis || !analysis->source || !out_ranges || !out_count) {
        return false;
    }

    *out_ranges = NULL;
    *out_count = 0;

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, analysis->source, analysis->source_length, "<lsp>");

    CnToken token;
    CnToken ident_token;
    int found = 0;

    // 第一次遍历：找到光标所在的标识符
    while (cn_frontend_lexer_next_token(&lexer, &token)) {
        if (token.kind == CN_TOKEN_EOF) {
            break;
        }

        int line0 = (token.line > 0) ? token.line - 1 : 0;
        int col0 = (token.column > 0) ? token.column - 1 : 0;
        int col1 = col0 + (int)token.lexeme_length;

        if (position.line == line0 && position.column >= col0 && position.column < col1) {
            if (token.kind != CN_TOKEN_IDENT) {
                return false;
            }
            ident_token = token;
            found = 1;
            break;
        }
    }

    if (!found) {
        return false;
    }

    // 第二次遍历：收集所有同名标识符的位置
    cn_frontend_lexer_init(&lexer, analysis->source, analysis->source_length, "<lsp>");

    size_t capacity = 4;
    CnLspRange *ranges = (CnLspRange *)malloc(capacity * sizeof(CnLspRange));
    if (!ranges) {
        return false;
    }

    size_t count = 0;

    while (cn_frontend_lexer_next_token(&lexer, &token)) {
        if (token.kind == CN_TOKEN_EOF) {
            break;
        }

        if (token.kind != CN_TOKEN_IDENT) {
            continue;
        }

        if (token.lexeme_length == ident_token.lexeme_length &&
            memcmp(token.lexeme_begin, ident_token.lexeme_begin, ident_token.lexeme_length) == 0) {
            if (count >= capacity) {
                capacity *= 2;
                CnLspRange *new_ranges = (CnLspRange *)realloc(ranges, capacity * sizeof(CnLspRange));
                if (!new_ranges) {
                    free(ranges);
                    return false;
                }
                ranges = new_ranges;
            }

            CnLspRange *range = &ranges[count++];
            range->start.line = (token.line > 0) ? token.line - 1 : 0;
            range->start.column = (token.column > 0) ? token.column - 1 : 0;
            range->end.line = range->start.line;
            range->end.column = range->start.column + (int)token.lexeme_length;
        }
    }

    if (count == 0) {
        free(ranges);
        return false;
    }

    *out_ranges = ranges;
    *out_count = count;
    return true;
}

// 获取文档符号列表
bool cn_lsp_get_document_symbols(
    CnLspDocumentAnalysis *analysis,
    CnLspSymbolInfo **out_symbols,
    size_t *out_count)
{
    // TODO: 实现文档符号列表提取（从 AST 获取所有函数和变量）
    // 当前为最小原型，暂不实现
    (void)analysis;
    (void)out_symbols;
    (void)out_count;
    return false;
}

// 转换诊断信息：CnDiagnostics -> CnLspDiagnostic[]
void cn_lsp_convert_diagnostics(
    const CnDiagnostics *diagnostics,
    CnLspDiagnostic **out_diagnostics,
    size_t *out_count)
{
    if (!diagnostics || !out_diagnostics || !out_count) {
        return;
    }

    *out_diagnostics = NULL;
    *out_count = 0;

    if (diagnostics->count == 0) {
        return;
    }

    // 分配 LSP 诊断数组
    CnLspDiagnostic *lsp_diags = (CnLspDiagnostic *)malloc(
        diagnostics->count * sizeof(CnLspDiagnostic));
    if (!lsp_diags) {
        return;
    }

    // 转换每条诊断
    for (size_t i = 0; i < diagnostics->count; i++) {
        const CnDiagnostic *diag = &diagnostics->items[i];
        CnLspDiagnostic *lsp_diag = &lsp_diags[i];

        // 转换严重级别
        lsp_diag->severity = (diag->severity == CN_DIAG_SEVERITY_ERROR)
            ? CN_LSP_DIAG_ERROR
            : CN_LSP_DIAG_WARNING;

        // 转换位置信息（编译器使用 1-based，LSP 使用 0-based）
        lsp_diag->range.start.line = (diag->line > 0) ? diag->line - 1 : 0;
        lsp_diag->range.start.column = (diag->column > 0) ? diag->column - 1 : 0;
        
        // 结束位置暂时设为同一位置（精确范围需要 Token 信息）
        lsp_diag->range.end.line = lsp_diag->range.start.line;
        lsp_diag->range.end.column = lsp_diag->range.start.column + 1;

        // 设置消息和来源
        lsp_diag->message = diag->message;
        lsp_diag->source = "CN_Language";
    }

    *out_diagnostics = lsp_diags;
    *out_count = diagnostics->count;
}
