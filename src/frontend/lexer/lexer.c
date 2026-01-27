#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/keywords.h"
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
    // 使用统一的关键字查找函数
    return cn_frontend_lookup_keyword(begin, length);
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
