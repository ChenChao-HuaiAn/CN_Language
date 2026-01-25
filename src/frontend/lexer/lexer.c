#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"

#include <ctype.h>
#include <string.h>

static char current_char(const CnLexer *lexer)
{
    if (lexer->offset >= lexer->length) {
        return '\0';
    }

    return lexer->source[lexer->offset];
}

static char peek_char(const CnLexer *lexer)
{
    if (lexer->offset + 1 >= lexer->length) {
        return '\0';
    }

    return lexer->source[lexer->offset + 1];
}

static void advance(CnLexer *lexer)
{
    char c;

    if (lexer->offset >= lexer->length) {
        return;
    }

    c = lexer->source[lexer->offset];
    lexer->offset += 1;

    if (c == '\n') {
        lexer->line += 1;
        lexer->column = 1;
    } else {
        lexer->column += 1;
    }
}

static void skip_whitespace(CnLexer *lexer)
{
    char c;

    c = current_char(lexer);
    while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
        advance(lexer);
        c = current_char(lexer);
    }
}

static void report_lex_error(CnLexer *lexer, CnDiagCode code, const char *message)
{
    if (!lexer || !lexer->diagnostics) {
        return;
    }

    cn_support_diagnostics_report(lexer->diagnostics,
                                  CN_DIAG_SEVERITY_ERROR,
                                  code,
                                  lexer->filename,
                                  lexer->line,
                                  lexer->column,
                                  message);
}

static int is_identifier_start(unsigned char c)
{
    if (c == '_' || isalpha(c)) {
        return 1;
    }

    if (c >= 0x80) {
        return 1;
    }

    return 0;
}

static int is_identifier_continue(unsigned char c)
{
    if (isalnum(c) || c == '_') {
        return 1;
    }

    if (c >= 0x80) {
        return 1;
    }

    return 0;
}

static CnTokenKind keyword_kind(const char *begin, size_t length)
{
    // 使用 hex 编码确保在不同编译环境下 UTF-8 匹配的一致性
    // 如果: \xe5\xa6\x82\xe6\x9e\x9c
    if (length == 6 && memcmp(begin, "\xe5\xa6\x82\xe6\x9e\x9c", 6) == 0) {
        return CN_TOKEN_KEYWORD_IF;
    }

    // 否则: \xe5\x90\xa6\xe5\x88\x99
    if (length == 6 && memcmp(begin, "\xe5\x90\xa6\xe5\x88\x99", 6) == 0) {
        return CN_TOKEN_KEYWORD_ELSE;
    }

    // 函数: \xe5\x87\xbd\xe6\x95\xb0
    if (length == 6 && memcmp(begin, "\xe5\x87\xbd\xe6\x95\xb0", 6) == 0) {
        return CN_TOKEN_KEYWORD_FN;
    }

    // 返回: \xe8\xbf\x94\xe5\x9b\x9e
    if (length == 6 && memcmp(begin, "\xe8\xbf\x94\xe5\x9b\x9e", 6) == 0) {
        return CN_TOKEN_KEYWORD_RETURN;
    }

    // 变量: \xe5\x8f\x98\xe9\x87\x8f
    if (length == 6 && memcmp(begin, "\xe5\x8f\x98\xe9\x87\x8f", 6) == 0) {
        return CN_TOKEN_KEYWORD_VAR;
    }

    // 整数: \xe6\x95\xb4\xe6\x95\xb0
    if (length == 6 && memcmp(begin, "\xe6\x95\xb4\xe6\x95\xb0", 6) == 0) {
        return CN_TOKEN_KEYWORD_INT;
    }

    // 主程序: \xe4\xb8\xbb\xe7\xa8\x8b\xe5\xba\x8f
    if (length == 9 && memcmp(begin, "\xe4\xb8\xbb\xe7\xa8\x8b\xe5\xba\x8f", 9) == 0) {
        return CN_TOKEN_KEYWORD_MAIN;
    }
    
    // 打印 (内置函数标识符，虽然这里处理关键字，但可以参考)
    // 打印: \xe6\x89\x93\xe5\x8d\xb0 (6 bytes)

    // ... 为了简洁，其他关键字暂时保持原样或按需转换 ...
    if (length == strlen("小数") && strncmp(begin, "小数", length) == 0) {
        return CN_TOKEN_KEYWORD_FLOAT;
    }

    if (length == strlen("字符串") && strncmp(begin, "字符串", length) == 0) {
        return CN_TOKEN_KEYWORD_STRING;
    }

    if (length == strlen("布尔") && strncmp(begin, "布尔", length) == 0) {
        return CN_TOKEN_KEYWORD_BOOL;
    }

    if (length == strlen("数组") && strncmp(begin, "数组", length) == 0) {
        return CN_TOKEN_KEYWORD_ARRAY;
    }

    if (length == strlen("结构体") && strncmp(begin, "结构体", length) == 0) {
        return CN_TOKEN_KEYWORD_STRUCT;
    }

    if (length == strlen("枚举") && strncmp(begin, "枚举", length) == 0) {
        return CN_TOKEN_KEYWORD_ENUM;
    }

    // 当: \xe5\xbd\x93
    if (length == 3 && memcmp(begin, "\xe5\xbd\x93", 3) == 0) {
        return CN_TOKEN_KEYWORD_WHILE;
    }

    // 循环: \xe5\xbe\xaa\xe7\x8e\xaf
    if (length == 6 && memcmp(begin, "\xe5\xbe\xaa\xe7\x8e\xaf", 6) == 0) {
        return CN_TOKEN_KEYWORD_FOR;
    }

    // 中断: \xe4\xb8\xad\xe6\x96\xad
    if (length == 6 && memcmp(begin, "\xe4\xb8\xad\xe6\x96\xad", 6) == 0) {
        return CN_TOKEN_KEYWORD_BREAK;
    }

    // 继续: \xe7\xbb\xa7\xe7\xbb\xad
    if (length == 6 && memcmp(begin, "\xe7\xbb\xa7\xe7\xbb\xad", 6) == 0) {
        return CN_TOKEN_KEYWORD_CONTINUE;
    }

    // 从 (用于 for 循环): \xe4\xbb\x8e
    if (length == 3 && memcmp(begin, "\xe4\xbb\x8e", 3) == 0) {
        return CN_TOKEN_KEYWORD_FOR;
    }

    if (length == strlen("选择") && strncmp(begin, "选择", length) == 0) {
        return CN_TOKEN_KEYWORD_SWITCH;
    }

    if (length == strlen("情况") && strncmp(begin, "情况", length) == 0) {
        return CN_TOKEN_KEYWORD_CASE;
    }

    if (length == strlen("默认") && strncmp(begin, "默认", length) == 0) {
        return CN_TOKEN_KEYWORD_DEFAULT;
    }

    if (length == strlen("主程序") && strncmp(begin, "主程序", length) == 0) {
        return CN_TOKEN_KEYWORD_MAIN;
    }

    if (length == strlen("真") && strncmp(begin, "真", length) == 0) {
        return CN_TOKEN_KEYWORD_TRUE;
    }

    if (length == strlen("假") && strncmp(begin, "假", length) == 0) {
        return CN_TOKEN_KEYWORD_FALSE;
    }

    if (length == strlen("空") && strncmp(begin, "空", length) == 0) {
        return CN_TOKEN_KEYWORD_NULL;
    }

    if (length == strlen("无") && strncmp(begin, "无", length) == 0) {
        return CN_TOKEN_KEYWORD_VOID;
    }

    if (length == strlen("模块") && strncmp(begin, "模块", length) == 0) {
        return CN_TOKEN_KEYWORD_MODULE;
    }

    if (length == strlen("导入") && strncmp(begin, "导入", length) == 0) {
        return CN_TOKEN_KEYWORD_IMPORT;
    }

    if (length == strlen("命名空间") && strncmp(begin, "命名空间", length) == 0) {
        return CN_TOKEN_KEYWORD_NAMESPACE;
    }

    if (length == strlen("接口") && strncmp(begin, "接口", length) == 0) {
        return CN_TOKEN_KEYWORD_INTERFACE;
    }

    if (length == strlen("类") && strncmp(begin, "类", length) == 0) {
        return CN_TOKEN_KEYWORD_CLASS;
    }

    if (length == strlen("模板") && strncmp(begin, "模板", length) == 0) {
        return CN_TOKEN_KEYWORD_TEMPLATE;
    }

    if (length == strlen("常量") && strncmp(begin, "常量", length) == 0) {
        return CN_TOKEN_KEYWORD_CONST;
    }

    if (length == strlen("静态") && strncmp(begin, "静态", length) == 0) {
        return CN_TOKEN_KEYWORD_STATIC;
    }

    if (length == strlen("公开") && strncmp(begin, "公开", length) == 0) {
        return CN_TOKEN_KEYWORD_PUBLIC;
    }

    if (length == strlen("私有") && strncmp(begin, "私有", length) == 0) {
        return CN_TOKEN_KEYWORD_PRIVATE;
    }

    if (length == strlen("保护") && strncmp(begin, "保护", length) == 0) {
        return CN_TOKEN_KEYWORD_PROTECTED;
    }

    if (length == strlen("虚拟") && strncmp(begin, "虚拟", length) == 0) {
        return CN_TOKEN_KEYWORD_VIRTUAL;
    }

    if (length == strlen("重写") && strncmp(begin, "重写", length) == 0) {
        return CN_TOKEN_KEYWORD_OVERRIDE;
    }

    if (length == strlen("抽象") && strncmp(begin, "抽象", length) == 0) {
        return CN_TOKEN_KEYWORD_ABSTRACT;
    }

    if (length == strlen("与") && strncmp(begin, "与", length) == 0) {
        return CN_TOKEN_KEYWORD_AND;
    }

    if (length == strlen("且") && strncmp(begin, "且", length) == 0) {
        return CN_TOKEN_KEYWORD_AND;
    }

    if (length == strlen("或") && strncmp(begin, "或", length) == 0) {
        return CN_TOKEN_KEYWORD_OR;
    }

    return CN_TOKEN_INVALID;
}

void cn_frontend_lexer_init(CnLexer *lexer, const char *source, size_t length, const char *filename)
{
    if (lexer == NULL) {
        return;
    }

    lexer->source = source;
    lexer->filename = filename;
    lexer->length = length;
    lexer->offset = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->diagnostics = NULL;
}

void cn_frontend_lexer_set_diagnostics(CnLexer *lexer, struct CnDiagnostics *diagnostics)
{
    if (!lexer) {
        return;
    }

    lexer->diagnostics = diagnostics;
}

bool cn_frontend_lexer_next_token(CnLexer *lexer, CnToken *out_token)
{
    char c;
    size_t start_offset;
    int start_line;
    int start_column;

    if (lexer == NULL || out_token == NULL) {
        return false;
    }

    skip_whitespace(lexer);
    
    for (;;) {
        c = current_char(lexer);
        if (c == '/' && peek_char(lexer) == '/') {
            while (c != '\n' && c != '\0') {
                advance(lexer);
                c = current_char(lexer);
            }
            skip_whitespace(lexer);
            continue;
        }
        break;
    }
    
    start_offset = lexer->offset;
    start_line = lexer->line;
    start_column = lexer->column;
    
    c = current_char(lexer);
            
    if (c == '\0') {
        out_token->kind = CN_TOKEN_EOF;
        out_token->lexeme_begin = lexer->source + start_offset;
        out_token->lexeme_length = 0;
        out_token->line = start_line;
        out_token->column = start_column;
        return true;
    }
        
    if (c == '"') {
        advance(lexer);
        c = current_char(lexer);
        while (c != '"' && c != '\0') {
            if (c == '\\') {
                advance(lexer);
                c = current_char(lexer);
                if (c == '\0') {
                    break;
                }
            }
            advance(lexer);
            c = current_char(lexer);
        }
    
        if (c == '"') {
            advance(lexer);
            out_token->kind = CN_TOKEN_STRING_LITERAL;
        } else {
            report_lex_error(lexer, CN_DIAG_CODE_LEX_UNTERMINATED_STRING, "未终止的字符串字面量");
            out_token->kind = CN_TOKEN_INVALID;
        }
    } else if (isdigit((unsigned char)c)) {
        // 扫描数字：支持整数和浮点数
        while (isdigit((unsigned char)current_char(lexer))) {
            advance(lexer);
        }

        // 检查是否为浮点数（有小数点）
        if (current_char(lexer) == '.' && isdigit((unsigned char)peek_char(lexer))) {
            advance(lexer);  // 跳过小数点
            while (isdigit((unsigned char)current_char(lexer))) {
                advance(lexer);
            }
            out_token->kind = CN_TOKEN_FLOAT_LITERAL;
        } else {
            out_token->kind = CN_TOKEN_INTEGER;
        }
    } else if (is_identifier_start((unsigned char)c)) {
        CnTokenKind kind;

        while (is_identifier_continue((unsigned char)current_char(lexer))) {
            advance(lexer);
        }

        kind = keyword_kind(lexer->source + start_offset,
                            lexer->offset - start_offset);
        if (kind == CN_TOKEN_INVALID) {
            kind = CN_TOKEN_IDENT;
        }

        out_token->kind = kind;
    } else {
        switch (c) {
        case '+':
            advance(lexer);
            out_token->kind = CN_TOKEN_PLUS;
            break;
        case '-':
            advance(lexer);
            if (current_char(lexer) == '>') {
                advance(lexer);
                out_token->kind = CN_TOKEN_ARROW;
            } else {
                out_token->kind = CN_TOKEN_MINUS;
            }
            break;
        case '*':
            advance(lexer);
            out_token->kind = CN_TOKEN_STAR;
            break;
        case '/':
            advance(lexer);
            out_token->kind = CN_TOKEN_SLASH;
            break;
        case '%':
            advance(lexer);
            out_token->kind = CN_TOKEN_PERCENT;
            break;
        case '=':
            advance(lexer);
            if (current_char(lexer) == '=') {
                advance(lexer);
                out_token->kind = CN_TOKEN_EQUAL_EQUAL;
            } else {
                out_token->kind = CN_TOKEN_EQUAL;
            }
            break;
        case '!':
            advance(lexer);
            if (current_char(lexer) == '=') {
                advance(lexer);
                out_token->kind = CN_TOKEN_BANG_EQUAL;
            } else {
                out_token->kind = CN_TOKEN_BANG;
            }
            break;
        case '<':
            advance(lexer);
            if (current_char(lexer) == '=') {
                advance(lexer);
                out_token->kind = CN_TOKEN_LESS_EQUAL;
            } else {
                out_token->kind = CN_TOKEN_LESS;
            }
            break;
        case '>':
            advance(lexer);
            if (current_char(lexer) == '=') {
                advance(lexer);
                out_token->kind = CN_TOKEN_GREATER_EQUAL;
            } else {
                out_token->kind = CN_TOKEN_GREATER;
            }
            break;
        case '&':
            advance(lexer);
            if (current_char(lexer) == '&') {
                advance(lexer);
                out_token->kind = CN_TOKEN_LOGICAL_AND;
            } else {
                out_token->kind = CN_TOKEN_AMPERSAND;
            }
            break;
        case '|':
            advance(lexer);
            if (current_char(lexer) == '|') {
                advance(lexer);
                out_token->kind = CN_TOKEN_LOGICAL_OR;
            } else {
                out_token->kind = CN_TOKEN_INVALID;
            }
            break;
        case '(':
            advance(lexer);
            out_token->kind = CN_TOKEN_LPAREN;
            break;
        case ')':
            advance(lexer);
            out_token->kind = CN_TOKEN_RPAREN;
            break;
        case '{':
            advance(lexer);
            out_token->kind = CN_TOKEN_LBRACE;
            break;
        case '}':
            advance(lexer);
            out_token->kind = CN_TOKEN_RBRACE;
            break;
        case '[':
            advance(lexer);
            out_token->kind = CN_TOKEN_LBRACKET;
            break;
        case ']':
            advance(lexer);
            out_token->kind = CN_TOKEN_RBRACKET;
            break;
        case ';':
            advance(lexer);
            out_token->kind = CN_TOKEN_SEMICOLON;
            break;
        case ',':
            advance(lexer);
            out_token->kind = CN_TOKEN_COMMA;
            break;
        case '.':
            advance(lexer);
            out_token->kind = CN_TOKEN_DOT;
            break;
        default:
            report_lex_error(lexer, CN_DIAG_CODE_LEX_INVALID_CHAR, "非法字符");
            advance(lexer);
            out_token->kind = CN_TOKEN_INVALID;
            break;
        }
    }
    
    out_token->lexeme_begin = lexer->source + start_offset;
    out_token->lexeme_length = lexer->offset - start_offset;
    out_token->line = start_line;
    out_token->column = start_column;

    return true;
}
