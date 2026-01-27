#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

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

// 处理科学计数法的指数部分：e[+|-]digits
// 返回 1 表示成功识别指数部分，0 表示没有指数部分
static int scan_exponent(CnLexer *lexer)
{
    char c = current_char(lexer);
    
    // 检查是否有 e 或 E
    if (c != 'e' && c != 'E') {
        return 0;
    }
    
    advance(lexer);  // 跳过 'e' 或 'E'
    
    // 检查可选的正负号
    c = current_char(lexer);
    if (c == '+' || c == '-') {
        advance(lexer);
    }
    
    // 必须至少有一个数字
    if (!isdigit((unsigned char)current_char(lexer))) {
        return 0;  // 无效的科学计数法
    }
    
    // 扫描所有指数数字
    while (isdigit((unsigned char)current_char(lexer))) {
        advance(lexer);
    }
    
    return 1;
}

// 扫描数字字面量的类型后缀
// 返回值：0=无后缀, 1=float(f/F), 2=long(L/l), 3=long long(LL/ll), 
//         4=unsigned(U/u), 5=unsigned long(UL/ul), 6=unsigned long long(ULL/ull)
static int scan_number_suffix(CnLexer *lexer)
{
    char c = current_char(lexer);
    
    // 浮点后缀：f 或 F
    if (c == 'f' || c == 'F') {
        advance(lexer);
        return 1;  // float suffix
    }
    
    // 整数后缀：U/u, L/l, LL/ll 及其组合
    int has_unsigned = 0;
    int long_count = 0;
    
    // 可以是 U L LL UL ULL 等组合
    for (int i = 0; i < 3; i++) {  // 最多3个字符（ULL）
        c = current_char(lexer);
        
        if (c == 'u' || c == 'U') {
            if (has_unsigned) {
                break;  // 已经有 U 了
            }
            has_unsigned = 1;
            advance(lexer);
        } else if (c == 'l' || c == 'L') {
            if (long_count >= 2) {
                break;  // 最多两个 L
            }
            long_count++;
            advance(lexer);
        } else {
            break;
        }
    }
    
    // 根据组合返回类型标记
    if (has_unsigned && long_count == 2) {
        return 6;  // ULL
    } else if (has_unsigned && long_count == 1) {
        return 5;  // UL
    } else if (has_unsigned) {
        return 4;  // U
    } else if (long_count == 2) {
        return 3;  // LL
    } else if (long_count == 1) {
        return 2;  // L
    }
    
    return 0;  // 无后缀
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
    
    // 打印 (内置函数标识符，虽然这里处理关键字，但可以参考)
    // 打印: \xe6\x89\x93\xe5\x8d\xb0 (6 bytes)

    // ... 为了简洁，其他关键字暂时保持原样或按需转换 ...
    // 小数: \xe5\xb0\x8f\xe6\x95\xb0
    if (length == 6 && memcmp(begin, "\xe5\xb0\x8f\xe6\x95\xb0", 6) == 0) {
        return CN_TOKEN_KEYWORD_FLOAT;
    }

    // 字符串: \xe5\xad\x97\xe7\xac\xa6\xe4\xb8\xb2
    if (length == 9 && memcmp(begin, "\xe5\xad\x97\xe7\xac\xa6\xe4\xb8\xb2", 9) == 0) {
        return CN_TOKEN_KEYWORD_STRING;
    }

    // 布尔: \xe5\xb8\x83\xe5\xb0\x94
    if (length == 6 && memcmp(begin, "\xe5\xb8\x83\xe5\xb0\x94", 6) == 0) {
        return CN_TOKEN_KEYWORD_BOOL;
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

    if (length == strlen("选择") && strncmp(begin, "选择", length) == 0) {
        return CN_TOKEN_KEYWORD_SWITCH;
    }

    if (length == strlen("情况") && strncmp(begin, "情况", length) == 0) {
        return CN_TOKEN_KEYWORD_CASE;
    }

    if (length == strlen("默认") && strncmp(begin, "默认", length) == 0) {
        return CN_TOKEN_KEYWORD_DEFAULT;
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

    // 预留关键字（仅为语法检查保留）
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
        // 扫描数字：支持整数、浮点数、十六进制、二进制、八进制
        
        // 检查是否为特殊进制前缀（0x, 0X, 0b, 0B, 0o, 0O）
        if (c == '0') {
            char next = peek_char(lexer);
            
            // 十六进制：0x 或 0X
            if (next == 'x' || next == 'X') {
                advance(lexer);  // 跳过 '0'
                advance(lexer);  // 跳过 'x' 或 'X'
                
                // 必须至少有一个十六进制数字
                if (!isxdigit((unsigned char)current_char(lexer))) {
                    report_lex_error(lexer, CN_DIAG_CODE_LEX_INVALID_HEX, 
                                   "十六进制字面量必须包含至少一个有效数字（0-9, a-f, A-F）");
                    out_token->kind = CN_TOKEN_INVALID;
                } else {
                    // 扫描所有十六进制数字
                    while (isxdigit((unsigned char)current_char(lexer))) {
                        advance(lexer);
                    }
                    // 检查整数后缀
                    out_token->number_suffix = scan_number_suffix(lexer);
                    out_token->kind = CN_TOKEN_INTEGER;
                }
            }
            // 二进制：0b 或 0B
            else if (next == 'b' || next == 'B') {
                advance(lexer);  // 跳过 '0'
                advance(lexer);  // 跳过 'b' 或 'B'
                
                // 必须至少有一个二进制数字
                char bin_c = current_char(lexer);
                if (bin_c != '0' && bin_c != '1') {
                    report_lex_error(lexer, CN_DIAG_CODE_LEX_INVALID_BINARY,
                                   "二进制字面量必须包含至少一个有效数字（0或1）");
                    out_token->kind = CN_TOKEN_INVALID;
                } else {
                    // 扫描所有二进制数字
                    while ((bin_c = current_char(lexer)) == '0' || bin_c == '1') {
                        advance(lexer);
                    }
                    // 检查是否有非法字符
                    if (isdigit((unsigned char)bin_c)) {
                        report_lex_error(lexer, CN_DIAG_CODE_LEX_INVALID_BINARY,
                                       "二进制字面量只能包含0和1");
                        out_token->kind = CN_TOKEN_INVALID;
                    } else {
                        // 检查整数后缀
                        out_token->number_suffix = scan_number_suffix(lexer);
                        out_token->kind = CN_TOKEN_INTEGER;
                    }
                }
            }
            // 八进制：0o 或 0O
            else if (next == 'o' || next == 'O') {
                advance(lexer);  // 跳过 '0'
                advance(lexer);  // 跳过 'o' 或 'O'
                
                // 必须至少有一个八进制数字
                char oct_c = current_char(lexer);
                if (oct_c < '0' || oct_c > '7') {
                    report_lex_error(lexer, CN_DIAG_CODE_LEX_INVALID_OCTAL,
                                   "八进制字面量必须包含至少一个有效数字（0-7）");
                    out_token->kind = CN_TOKEN_INVALID;
                } else {
                    // 扫描所有八进制数字
                    while ((oct_c = current_char(lexer)) >= '0' && oct_c <= '7') {
                        advance(lexer);
                    }
                    // 检查是否有非法字符
                    if (isdigit((unsigned char)oct_c)) {
                        report_lex_error(lexer, CN_DIAG_CODE_LEX_INVALID_OCTAL,
                                       "八进制字面量只能包含0-7");
                        out_token->kind = CN_TOKEN_INVALID;
                    } else {
                        // 检查整数后缀
                        out_token->number_suffix = scan_number_suffix(lexer);
                        out_token->kind = CN_TOKEN_INTEGER;
                    }
                }
            }
            // 普通数字（以0开头）
            else {
                while (isdigit((unsigned char)current_char(lexer))) {
                    advance(lexer);
                }
                
                // 检查是否为浮点数（有小数点）
                if (current_char(lexer) == '.' && isdigit((unsigned char)peek_char(lexer))) {
                    advance(lexer);  // 跳过小数点
                    while (isdigit((unsigned char)current_char(lexer))) {
                        advance(lexer);
                    }
                    
                    // 检查科学计数法：e[+|-]digits
                    scan_exponent(lexer);
                    
                    // 检查浮点后缀 f/F
                    out_token->number_suffix = scan_number_suffix(lexer);
                    
                    out_token->kind = CN_TOKEN_FLOAT_LITERAL;
                } else {
                    // 检查整数是否有科学计数法（如 1e10）
                    if (scan_exponent(lexer)) {
                        // 有科学计数法，变成浮点数
                        out_token->number_suffix = scan_number_suffix(lexer);  // 检查 f/F 后缀
                        out_token->kind = CN_TOKEN_FLOAT_LITERAL;
                    } else {
                        // 普通整数，检查整数后缀 L/LL/U/UL/ULL
                        out_token->number_suffix = scan_number_suffix(lexer);
                        out_token->kind = CN_TOKEN_INTEGER;
                    }
                }
            }
        }
        // 非0开头的普通数字
        else {
            while (isdigit((unsigned char)current_char(lexer))) {
                advance(lexer);
            }

            // 检查是否为浮点数（有小数点）
            if (current_char(lexer) == '.' && isdigit((unsigned char)peek_char(lexer))) {
                advance(lexer);  // 跳过小数点
                while (isdigit((unsigned char)current_char(lexer))) {
                    advance(lexer);
                }
                
                // 检查科学计数法：e[+|-]digits
                scan_exponent(lexer);
                
                // 检查浮点后缀 f/F
                out_token->number_suffix = scan_number_suffix(lexer);
                
                out_token->kind = CN_TOKEN_FLOAT_LITERAL;
            } else {
                // 检查整数是否有科学计数法（如 1e10）
                if (scan_exponent(lexer)) {
                    // 有科学计数法，变成浮点数
                    out_token->number_suffix = scan_number_suffix(lexer);  // 检查 f/F 后缀
                    out_token->kind = CN_TOKEN_FLOAT_LITERAL;
                } else {
                    // 普通整数，检查整数后缀 L/LL/U/UL/ULL
                    out_token->number_suffix = scan_number_suffix(lexer);
                    out_token->kind = CN_TOKEN_INTEGER;
                }
            }
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
            if (current_char(lexer) == '+') {
                advance(lexer);
                out_token->kind = CN_TOKEN_PLUS_PLUS;
            } else {
                out_token->kind = CN_TOKEN_PLUS;
            }
            break;
        case '-':
            advance(lexer);
            if (current_char(lexer) == '>') {
                advance(lexer);
                out_token->kind = CN_TOKEN_ARROW;
            } else if (current_char(lexer) == '-') {
                advance(lexer);
                out_token->kind = CN_TOKEN_MINUS_MINUS;
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
            if (current_char(lexer) == '<') {
                advance(lexer);
                out_token->kind = CN_TOKEN_LEFT_SHIFT;
            } else if (current_char(lexer) == '=') {
                advance(lexer);
                out_token->kind = CN_TOKEN_LESS_EQUAL;
            } else {
                out_token->kind = CN_TOKEN_LESS;
            }
            break;
        case '>':
            advance(lexer);
            if (current_char(lexer) == '>') {
                advance(lexer);
                out_token->kind = CN_TOKEN_RIGHT_SHIFT;
            } else if (current_char(lexer) == '=') {
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
                out_token->kind = CN_TOKEN_BITWISE_OR;
            }
            break;
        case '^':
            advance(lexer);
            out_token->kind = CN_TOKEN_BITWISE_XOR;
            break;
        case '~':
            advance(lexer);
            out_token->kind = CN_TOKEN_BITWISE_NOT;
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
        case ':':
            advance(lexer);
            out_token->kind = CN_TOKEN_COLON;
            break;
        case '?':
            advance(lexer);
            out_token->kind = CN_TOKEN_QUESTION;
            break;
        default:
            {
                char error_msg[256];
                unsigned char invalid_ch = (unsigned char)c;
                if (invalid_ch == '#') {
                    snprintf(error_msg, sizeof(error_msg), 
                            "意外的预处理指令符 '#' (行 %d:列 %d，这可能表示预处理器未正确处理该指令)", 
                            lexer->line, lexer->column);
                } else if (invalid_ch >= 32 && invalid_ch < 127) {
                    snprintf(error_msg, sizeof(error_msg), "非法字符 '%c' (ASCII: %d, 行 %d:列 %d)", 
                            invalid_ch, invalid_ch, lexer->line, lexer->column);
                } else if (invalid_ch >= 0x80) {
                    /* UTF-8 多字节字符的开始 */
                    size_t remaining = lexer->length - lexer->offset;
                    size_t utf8_len = 1;
                    if ((invalid_ch & 0xE0) == 0xC0 && remaining >= 2) utf8_len = 2;
                    else if ((invalid_ch & 0xF0) == 0xE0 && remaining >= 3) utf8_len = 3;
                    else if ((invalid_ch & 0xF8) == 0xF0 && remaining >= 4) utf8_len = 4;
                    
                    char utf8_bytes[16] = {0};
                    for (size_t i = 0; i < utf8_len && i < 8; i++) {
                        sprintf(utf8_bytes + i*5, "0x%02X ", (unsigned char)lexer->source[lexer->offset + i]);
                    }
                    snprintf(error_msg, sizeof(error_msg), "非法UTF-8字符 [%s] (行 %d:列 %d)", 
                            utf8_bytes, lexer->line, lexer->column);
                } else {
                    snprintf(error_msg, sizeof(error_msg), "非法字符 0x%02X (行 %d:列 %d)", 
                            invalid_ch, lexer->line, lexer->column);
                }
                report_lex_error(lexer, CN_DIAG_CODE_LEX_INVALID_CHAR, error_msg);
            }
            advance(lexer);
            out_token->kind = CN_TOKEN_INVALID;
            break;
        }
    }
    
    out_token->lexeme_begin = lexer->source + start_offset;
    out_token->lexeme_length = lexer->offset - start_offset;
    out_token->line = start_line;
    out_token->column = start_column;
    // 对于非数字token，确保 number_suffix 为 0
    if (out_token->kind != CN_TOKEN_INTEGER && out_token->kind != CN_TOKEN_FLOAT_LITERAL) {
        out_token->number_suffix = 0;
    }

    return true;
}
