#include "cnlang/frontend/preprocessor.h"
#include "cnlang/support/diagnostics.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ========== 内部辅助函数声明 ========== */

static char current_char(const CnPreprocessor *pp);
static char peek_char(const CnPreprocessor *pp, size_t offset);
static void advance(CnPreprocessor *pp);
static void skip_whitespace(CnPreprocessor *pp);
static void skip_line(CnPreprocessor *pp);
static bool is_identifier_start(char c);
static bool is_identifier_continue(char c);
static void append_output(CnPreprocessor *pp, const char *text, size_t length);
static void append_char(CnPreprocessor *pp, char c);
static void report_error(CnPreprocessor *pp, const char *message);

static bool process_directive(CnPreprocessor *pp);
static bool process_define(CnPreprocessor *pp);
static bool process_ifdef(CnPreprocessor *pp, bool negate);
static bool process_else(CnPreprocessor *pp);
static bool process_endif(CnPreprocessor *pp);
static bool process_undef(CnPreprocessor *pp);

static CnMacro* find_macro(const CnPreprocessor *pp, const char *name, size_t name_length);
static bool add_macro(CnPreprocessor *pp, CnMacro *macro);
static void free_macro(CnMacro *macro);
static bool expand_macro(CnPreprocessor *pp, const char *name, size_t name_length);
static bool parse_function_macro_args(CnPreprocessor *pp, char ***out_args, size_t *out_count);
static char* substitute_macro_params(const CnMacro *macro, char **args, size_t arg_count);

static bool is_condition_active(const CnPreprocessor *pp);
static bool push_condition(CnPreprocessor *pp, bool active);
static bool pop_condition(CnPreprocessor *pp);

/* ========== 公共API实现 ========== */

void cn_frontend_preprocessor_init(
    CnPreprocessor *preprocessor,
    const char *source,
    size_t source_length,
    const char *filename)
{
    if (!preprocessor) {
        return;
    }

    preprocessor->source = source;
    preprocessor->source_length = source_length;
    preprocessor->filename = filename;
    
    preprocessor->output = NULL;
    preprocessor->output_length = 0;
    preprocessor->output_capacity = 0;
    
    preprocessor->macros = NULL;
    preprocessor->macro_count = 0;
    preprocessor->macro_capacity = 0;
    
    preprocessor->condition_stack = NULL;
    preprocessor->condition_depth = 0;
    preprocessor->condition_capacity = 0;
    
    preprocessor->current_offset = 0;
    preprocessor->current_line = 1;
    preprocessor->current_column = 1;
    
    preprocessor->diagnostics = NULL;
}

void cn_frontend_preprocessor_set_diagnostics(
    CnPreprocessor *preprocessor,
    struct CnDiagnostics *diagnostics)
{
    if (preprocessor) {
        preprocessor->diagnostics = diagnostics;
    }
}

bool cn_frontend_preprocessor_process(CnPreprocessor *preprocessor)
{
    if (!preprocessor) {
        return false;
    }

    /* 初始化输出缓冲区 */
    preprocessor->output_capacity = preprocessor->source_length + 1024;
    preprocessor->output = (char *)malloc(preprocessor->output_capacity);
    if (!preprocessor->output) {
        return false;
    }
    preprocessor->output_length = 0;

    /* 主处理循环 */
    while (preprocessor->current_offset < preprocessor->source_length) {
        char c = current_char(preprocessor);
        
        /* 处理行注释 */
        if (c == '/' && peek_char(preprocessor, 1) == '/') {
            /* 跳过注释内容,保留换行 */
            while (current_char(preprocessor) != '\n' && current_char(preprocessor) != '\0') {
                advance(preprocessor);
            }
            if (current_char(preprocessor) == '\n') {
                append_char(preprocessor, '\n');
                advance(preprocessor);
            }
            continue;
        }
        
        /* 处理块注释 */
        if (c == '/' && peek_char(preprocessor, 1) == '*') {
            advance(preprocessor);  /* 跳过 / */
            advance(preprocessor);  /* 跳过 * */
            /* 找到结束标记,保留换行符 */
            while (current_char(preprocessor) != '\0') {
                if (current_char(preprocessor) == '*' && peek_char(preprocessor, 1) == '/') {
                    advance(preprocessor);  /* 跳过 * */
                    advance(preprocessor);  /* 跳过 / */
                    break;
                }
                if (current_char(preprocessor) == '\n') {
                    append_char(preprocessor, '\n');
                }
                advance(preprocessor);
            }
            continue;
        }
        
        /* 检查是否是预处理指令 (行首的 #) */
        if (c == '#' && preprocessor->current_column == 1) {
            if (!process_directive(preprocessor)) {
                return false;
            }
            continue;
        }
        
        /* 跳过条件编译不活跃的代码 */
        if (!is_condition_active(preprocessor)) {
            if (c == '\n') {
                advance(preprocessor);
            } else {
                skip_line(preprocessor);
            }
            continue;
        }
        
        /* 识别标识符,检查是否是宏 */
        if (is_identifier_start(c)) {
            size_t start_offset = preprocessor->current_offset;
            while (is_identifier_continue(current_char(preprocessor))) {
                advance(preprocessor);
            }
            size_t name_length = preprocessor->current_offset - start_offset;
            const char *name = preprocessor->source + start_offset;
            
            /* 尝试展开宏 */
            if (!expand_macro(preprocessor, name, name_length)) {
                /* 不是宏或展开失败,保留原文本 */
                append_output(preprocessor, name, name_length);
            }
            continue;
        }
        
        /* 普通字符,直接复制 */
        append_char(preprocessor, c);
        advance(preprocessor);
    }

    /* 检查是否有未闭合的条件编译块 */
    if (preprocessor->condition_depth > 0) {
        report_error(preprocessor, "未闭合的条件编译指令");
        return false;
    }

    /* 添加终止符 */
    append_char(preprocessor, '\0');
    preprocessor->output_length--;  /* 不计入终止符 */

    return true;
}

void cn_frontend_preprocessor_free(CnPreprocessor *preprocessor)
{
    size_t i;

    if (!preprocessor) {
        return;
    }

    /* 释放输出缓冲区 */
    free(preprocessor->output);
    preprocessor->output = NULL;
    preprocessor->output_length = 0;
    preprocessor->output_capacity = 0;

    /* 释放宏定义 */
    for (i = 0; i < preprocessor->macro_count; ++i) {
        free_macro(&preprocessor->macros[i]);
    }
    free(preprocessor->macros);
    preprocessor->macros = NULL;
    preprocessor->macro_count = 0;
    preprocessor->macro_capacity = 0;

    /* 释放条件栈 */
    free(preprocessor->condition_stack);
    preprocessor->condition_stack = NULL;
    preprocessor->condition_depth = 0;
    preprocessor->condition_capacity = 0;
}

bool cn_frontend_preprocessor_define_macro(
    CnPreprocessor *preprocessor,
    const char *name,
    const char *value)
{
    CnMacro macro;
    size_t name_len, value_len;

    if (!preprocessor || !name) {
        return false;
    }

    name_len = strlen(name);
    value_len = value ? strlen(value) : 0;

    macro.name = (char *)malloc(name_len + 1);
    if (!macro.name) {
        return false;
    }
    memcpy(macro.name, name, name_len + 1);
    macro.name_length = name_len;

    if (value) {
        macro.replacement = (char *)malloc(value_len + 1);
        if (!macro.replacement) {
            free(macro.name);
            return false;
        }
        memcpy(macro.replacement, value, value_len + 1);
        macro.replacement_length = value_len;
    } else {
        macro.replacement = NULL;
        macro.replacement_length = 0;
    }

    macro.params = NULL;
    macro.param_count = 0;
    macro.is_function_like = false;
    macro.defined_line = preprocessor->current_line;

    return add_macro(preprocessor, &macro);
}

bool cn_frontend_preprocessor_undefine_macro(
    CnPreprocessor *preprocessor,
    const char *name)
{
    size_t i;
    size_t name_len;

    if (!preprocessor || !name) {
        return false;
    }

    name_len = strlen(name);
    
    for (i = 0; i < preprocessor->macro_count; ++i) {
        if (preprocessor->macros[i].name_length == name_len &&
            memcmp(preprocessor->macros[i].name, name, name_len) == 0) {
            /* 找到宏,释放并移除 */
            free_macro(&preprocessor->macros[i]);
            
            /* 将后续元素前移 */
            if (i + 1 < preprocessor->macro_count) {
                memmove(&preprocessor->macros[i],
                        &preprocessor->macros[i + 1],
                        (preprocessor->macro_count - i - 1) * sizeof(CnMacro));
            }
            preprocessor->macro_count--;
            return true;
        }
    }

    return false;
}

bool cn_frontend_preprocessor_is_defined(
    const CnPreprocessor *preprocessor,
    const char *name,
    size_t name_length)
{
    return find_macro(preprocessor, name, name_length) != NULL;
}

/* ========== 内部辅助函数实现 ========== */

static char current_char(const CnPreprocessor *pp)
{
    if (pp->current_offset >= pp->source_length) {
        return '\0';
    }
    return pp->source[pp->current_offset];
}

static char peek_char(const CnPreprocessor *pp, size_t offset)
{
    size_t pos = pp->current_offset + offset;
    if (pos >= pp->source_length) {
        return '\0';
    }
    return pp->source[pos];
}

static void advance(CnPreprocessor *pp)
{
    if (pp->current_offset >= pp->source_length) {
        return;
    }

    if (pp->source[pp->current_offset] == '\n') {
        pp->current_line++;
        pp->current_column = 1;
    } else {
        pp->current_column++;
    }
    pp->current_offset++;
}

static void skip_whitespace(CnPreprocessor *pp)
{
    char c = current_char(pp);
    while (c == ' ' || c == '\t' || c == '\r') {
        advance(pp);
        c = current_char(pp);
    }
}

static void skip_line(CnPreprocessor *pp)
{
    while (current_char(pp) != '\n' && current_char(pp) != '\0') {
        advance(pp);
    }
    if (current_char(pp) == '\n') {
        advance(pp);
    }
}

static bool is_identifier_start(char c)
{
    return isalpha((unsigned char)c) || c == '_' || (unsigned char)c >= 0x80;
}

static bool is_identifier_continue(char c)
{
    return isalnum((unsigned char)c) || c == '_' || (unsigned char)c >= 0x80;
}

static void append_output(CnPreprocessor *pp, const char *text, size_t length)
{
    size_t new_capacity;
    char *new_output;

    if (pp->output_length + length >= pp->output_capacity) {
        new_capacity = (pp->output_capacity + length) * 2;
        new_output = (char *)realloc(pp->output, new_capacity);
        if (!new_output) {
            return;
        }
        pp->output = new_output;
        pp->output_capacity = new_capacity;
    }

    memcpy(pp->output + pp->output_length, text, length);
    pp->output_length += length;
}

static void append_char(CnPreprocessor *pp, char c)
{
    append_output(pp, &c, 1);
}

static void report_error(CnPreprocessor *pp, const char *message)
{
    if (pp->diagnostics) {
        cn_support_diagnostics_report(
            pp->diagnostics,
            CN_DIAG_SEVERITY_ERROR,
            CN_DIAG_CODE_UNKNOWN,
            pp->filename,
            pp->current_line,
            pp->current_column,
            message
        );
    }
}

/* ========== 预处理指令处理 ========== */

static bool process_directive(CnPreprocessor *pp)
{
    char c;
    size_t start_offset;
    size_t directive_length;
    const char *directive;

    /* 跳过 # */
    advance(pp);
    skip_whitespace(pp);

    /* 读取指令名称 */
    start_offset = pp->current_offset;
    c = current_char(pp);
    if (!is_identifier_start(c)) {
        report_error(pp, "预处理指令后需要指令名称");
        skip_line(pp);
        return false;
    }

    while (is_identifier_continue(current_char(pp))) {
        advance(pp);
    }

    directive_length = pp->current_offset - start_offset;
    directive = pp->source + start_offset;

    /* 处理不同的指令 (支持中英文) */
    /* #define / #定义 */
    if ((directive_length == 6 && memcmp(directive, "define", 6) == 0) ||
        (directive_length == 6 && memcmp(directive, "\xE5\xAE\x9A\xE4\xB9\x89", 6) == 0)) {  /* UTF-8: 定义 */
        return process_define(pp);
    }
    /* #ifdef / #如果定义 */
    else if ((directive_length == 5 && memcmp(directive, "ifdef", 5) == 0) ||
             (directive_length == 12 && memcmp(directive, "\xE5\xA6\x82\xE6\x9E\x9C\xE5\xAE\x9A\xE4\xB9\x89", 12) == 0)) {  /* UTF-8: 如果定义 */
        return process_ifdef(pp, false);
    }
    /* #ifndef / #如果未定义 */
    else if ((directive_length == 6 && memcmp(directive, "ifndef", 6) == 0) ||
             (directive_length == 15 && memcmp(directive, "\xE5\xA6\x82\xE6\x9E\x9C\xE6\x9C\xAA\xE5\xAE\x9A\xE4\xB9\x89", 15) == 0)) {  /* UTF-8: 如果未定义 */
        return process_ifdef(pp, true);
    }
    /* #else / #否则 */
    else if ((directive_length == 4 && memcmp(directive, "else", 4) == 0) ||
             (directive_length == 6 && memcmp(directive, "\xE5\x90\xA6\xE5\x88\x99", 6) == 0)) {  /* UTF-8: 否则 */
        return process_else(pp);
    }
    /* #endif / #结束如果 */
    else if ((directive_length == 5 && memcmp(directive, "endif", 5) == 0) ||
             (directive_length == 12 && memcmp(directive, "\xE7\xBB\x93\xE6\x9D\x9F\xE5\xA6\x82\xE6\x9E\x9C", 12) == 0)) {  /* UTF-8: 结束如果 */
        return process_endif(pp);
    }
    /* #undef / #未定义 */
    else if ((directive_length == 5 && memcmp(directive, "undef", 5) == 0) ||
             (directive_length == 9 && memcmp(directive, "\xE6\x9C\xAA\xE5\xAE\x9A\xE4\xB9\x89", 9) == 0)) {  /* UTF-8: 未定义 */
        return process_undef(pp);
    }
    else {
        report_error(pp, "未知的预处理指令");
        skip_line(pp);
        return false;
    }
}

static bool process_define(CnPreprocessor *pp)
{
    size_t name_start, name_length;
    size_t replacement_start, replacement_end;
    CnMacro macro;
    char *name_str;
    char *replacement_str;
    bool is_function_like = false;

    skip_whitespace(pp);

    /* 如果当前条件不激活,跳过此指令 */
    if (!is_condition_active(pp)) {
        skip_line(pp);
        return true;
    }

    /* 读取宏名称 */
    name_start = pp->current_offset;
    if (!is_identifier_start(current_char(pp))) {
        report_error(pp, "#define 后需要宏名称");
        skip_line(pp);
        return false;
    }

    while (is_identifier_continue(current_char(pp))) {
        advance(pp);
    }
    name_length = pp->current_offset - name_start;

    /* 检查是否是函数宏 (紧跟着的左括号,没有空格) */
    if (current_char(pp) == '(') {
        is_function_like = true;
        advance(pp);  /* 跳过 ( */
        
        /* 解析参数列表 */
        macro.params = NULL;
        macro.param_count = 0;
        macro.is_function_like = true;
        
        size_t param_capacity = 4;
        macro.params = (char **)malloc(param_capacity * sizeof(char *));
        if (!macro.params) {
            return false;
        }
        
        skip_whitespace(pp);
        
        /* 空参数列表 */
        if (current_char(pp) == ')') {
            advance(pp);
        } else {
            /* 读取参数 */
            while (current_char(pp) != ')' && current_char(pp) != '\0') {
                skip_whitespace(pp);
                
                size_t param_start = pp->current_offset;
                if (!is_identifier_start(current_char(pp))) {
                    report_error(pp, "函数宏参数必须是标识符");
                    skip_line(pp);
                    /* 释放已分配的参数 */
                    for (size_t i = 0; i < macro.param_count; ++i) {
                        free(macro.params[i]);
                    }
                    free(macro.params);
                    return false;
                }
                
                while (is_identifier_continue(current_char(pp))) {
                    advance(pp);
                }
                
                size_t param_length = pp->current_offset - param_start;
                
                /* 扩展参数数组 */
                if (macro.param_count >= param_capacity) {
                    param_capacity *= 2;
                    char **new_params = (char **)realloc(macro.params, param_capacity * sizeof(char *));
                    if (!new_params) {
                        for (size_t i = 0; i < macro.param_count; ++i) {
                            free(macro.params[i]);
                        }
                        free(macro.params);
                        return false;
                    }
                    macro.params = new_params;
                }
                
                /* 复制参数名 */
                macro.params[macro.param_count] = (char *)malloc(param_length + 1);
                if (!macro.params[macro.param_count]) {
                    for (size_t i = 0; i < macro.param_count; ++i) {
                        free(macro.params[i]);
                    }
                    free(macro.params);
                    return false;
                }
                memcpy(macro.params[macro.param_count], pp->source + param_start, param_length);
                macro.params[macro.param_count][param_length] = '\0';
                macro.param_count++;
                
                skip_whitespace(pp);
                
                if (current_char(pp) == ',') {
                    advance(pp);
                } else if (current_char(pp) != ')') {
                    report_error(pp, "函数宏参数列表格式错误");
                    skip_line(pp);
                    for (size_t i = 0; i < macro.param_count; ++i) {
                        free(macro.params[i]);
                    }
                    free(macro.params);
                    return false;
                }
            }
            
            if (current_char(pp) == ')') {
                advance(pp);
            } else {
                report_error(pp, "函数宏参数列表未闭合");
                skip_line(pp);
                for (size_t i = 0; i < macro.param_count; ++i) {
                    free(macro.params[i]);
                }
                free(macro.params);
                return false;
            }
        }
    } else {
        macro.params = NULL;
        macro.param_count = 0;
        macro.is_function_like = false;
    }

    skip_whitespace(pp);

    /* 读取替换文本 (到行尾) */
    replacement_start = pp->current_offset;
    while (current_char(pp) != '\n' && current_char(pp) != '\0') {
        advance(pp);
    }
    replacement_end = pp->current_offset;

    /* 去除替换文本末尾的空白 */
    while (replacement_end > replacement_start &&
           (pp->source[replacement_end - 1] == ' ' ||
            pp->source[replacement_end - 1] == '\t' ||
            pp->source[replacement_end - 1] == '\r')) {
        replacement_end--;
    }

    /* 创建宏定义 */
    name_str = (char *)malloc(name_length + 1);
    if (!name_str) {
        if (is_function_like) {
            for (size_t i = 0; i < macro.param_count; ++i) {
                free(macro.params[i]);
            }
            free(macro.params);
        }
        return false;
    }
    memcpy(name_str, pp->source + name_start, name_length);
    name_str[name_length] = '\0';

    if (replacement_end > replacement_start) {
        size_t replacement_length = replacement_end - replacement_start;
        replacement_str = (char *)malloc(replacement_length + 1);
        if (!replacement_str) {
            free(name_str);
            if (is_function_like) {
                for (size_t i = 0; i < macro.param_count; ++i) {
                    free(macro.params[i]);
                }
                free(macro.params);
            }
            return false;
        }
        memcpy(replacement_str, pp->source + replacement_start, replacement_length);
        replacement_str[replacement_length] = '\0';
        macro.replacement = replacement_str;
        macro.replacement_length = replacement_length;
    } else {
        macro.replacement = NULL;
        macro.replacement_length = 0;
    }

    macro.name = name_str;
    macro.name_length = name_length;
    macro.defined_line = pp->current_line;

    /* 跳过换行 */
    if (current_char(pp) == '\n') {
        advance(pp);
    }

    return add_macro(pp, &macro);
}

static bool process_ifdef(CnPreprocessor *pp, bool negate)
{
    size_t name_start, name_length;
    bool is_defined;
    bool active;

    skip_whitespace(pp);

    /* 读取宏名称 */
    name_start = pp->current_offset;
    if (!is_identifier_start(current_char(pp))) {
        report_error(pp, negate ? "#ifndef 后需要宏名称" : "#ifdef 后需要宏名称");
        skip_line(pp);
        return false;
    }

    while (is_identifier_continue(current_char(pp))) {
        advance(pp);
    }
    name_length = pp->current_offset - name_start;

    /* 检查宏是否已定义 */
    is_defined = cn_frontend_preprocessor_is_defined(pp, pp->source + name_start, name_length);
    
    /* 确定条件是否激活 */
    active = negate ? !is_defined : is_defined;
    
    /* 只有在父条件激活时,才考虑当前条件 */
    if (!is_condition_active(pp)) {
        active = false;
    }

    skip_line(pp);
    return push_condition(pp, active);
}

static bool process_else(CnPreprocessor *pp)
{
    if (pp->condition_depth == 0) {
        report_error(pp, "#else 没有对应的 #ifdef 或 #ifndef");
        skip_line(pp);
        return false;
    }

    /* 切换条件状态 */
    CnConditionFrame *frame = &pp->condition_stack[pp->condition_depth - 1];
    
    /* 只有在父条件激活且之前没有执行过时,才激活 else 分支 */
    bool parent_active = (pp->condition_depth == 1) || 
                         pp->condition_stack[pp->condition_depth - 2].active;
    
    if (parent_active && !frame->has_executed) {
        frame->active = true;
        frame->has_executed = true;
    } else {
        frame->active = false;
    }

    skip_line(pp);
    return true;
}

static bool process_endif(CnPreprocessor *pp)
{
    if (pp->condition_depth == 0) {
        report_error(pp, "#endif 没有对应的 #ifdef 或 #ifndef");
        skip_line(pp);
        return false;
    }

    skip_line(pp);
    return pop_condition(pp);
}

static bool process_undef(CnPreprocessor *pp)
{
    size_t name_start, name_length;
    char *name_str;
    bool result;

    skip_whitespace(pp);

    /* 如果当前条件不激活,跳过此指令 */
    if (!is_condition_active(pp)) {
        skip_line(pp);
        return true;
    }

    /* 读取宏名称 */
    name_start = pp->current_offset;
    if (!is_identifier_start(current_char(pp))) {
        report_error(pp, "#undef 后需要宏名称");
        skip_line(pp);
        return false;
    }

    while (is_identifier_continue(current_char(pp))) {
        advance(pp);
    }
    name_length = pp->current_offset - name_start;

    name_str = (char *)malloc(name_length + 1);
    if (!name_str) {
        return false;
    }
    memcpy(name_str, pp->source + name_start, name_length);
    name_str[name_length] = '\0';

    result = cn_frontend_preprocessor_undefine_macro(pp, name_str);
    free(name_str);

    skip_line(pp);
    return true;
}

/* ========== 宏管理 ========== */

static CnMacro* find_macro(const CnPreprocessor *pp, const char *name, size_t name_length)
{
    size_t i;

    for (i = 0; i < pp->macro_count; ++i) {
        if (pp->macros[i].name_length == name_length &&
            memcmp(pp->macros[i].name, name, name_length) == 0) {
            return &pp->macros[i];
        }
    }

    return NULL;
}

static bool add_macro(CnPreprocessor *pp, CnMacro *macro)
{
    CnMacro *existing;
    CnMacro *new_macros;
    size_t new_capacity;

    /* 检查是否已存在同名宏 */
    existing = find_macro(pp, macro->name, macro->name_length);
    if (existing) {
        /* 替换现有宏 */
        free_macro(existing);
        *existing = *macro;
        return true;
    }

    /* 扩展宏表 */
    if (pp->macro_count >= pp->macro_capacity) {
        new_capacity = pp->macro_capacity == 0 ? 16 : pp->macro_capacity * 2;
        new_macros = (CnMacro *)realloc(pp->macros, new_capacity * sizeof(CnMacro));
        if (!new_macros) {
            free_macro(macro);
            return false;
        }
        pp->macros = new_macros;
        pp->macro_capacity = new_capacity;
    }

    pp->macros[pp->macro_count++] = *macro;
    return true;
}

static void free_macro(CnMacro *macro)
{
    size_t i;

    if (!macro) {
        return;
    }

    free(macro->name);
    free(macro->replacement);
    
    if (macro->params) {
        for (i = 0; i < macro->param_count; ++i) {
            free(macro->params[i]);
        }
        free(macro->params);
    }
}

static bool expand_macro(CnPreprocessor *pp, const char *name, size_t name_length)
{
    CnMacro *macro = find_macro(pp, name, name_length);
    char **args = NULL;
    size_t arg_count = 0;
    char *expanded = NULL;
    
    if (!macro) {
        return false;
    }

    /* 处理函数宏 */
    if (macro->is_function_like) {
        /* 检查后面是否有左括号 */
        skip_whitespace(pp);
        if (current_char(pp) != '(') {
            /* 函数宏后面没有括号,不展开 */
            return false;
        }
        
        /* 解析实参 */
        if (!parse_function_macro_args(pp, &args, &arg_count)) {
            return false;
        }
        
        /* 检查参数数量 */
        if (arg_count != macro->param_count) {
            report_error(pp, "宏调用的参数数量不匹配");
            /* 释放参数 */
            for (size_t i = 0; i < arg_count; ++i) {
                free(args[i]);
            }
            free(args);
            return false;
        }
        
        /* 执行参数替换 */
        expanded = substitute_macro_params(macro, args, arg_count);
        
        /* 释放参数 */
        for (size_t i = 0; i < arg_count; ++i) {
            free(args[i]);
        }
        free(args);
        
        if (expanded) {
            append_output(pp, expanded, strlen(expanded));
            free(expanded);
            return true;
        }
        return false;
    }

    /* 展开对象宏 */
    if (macro->replacement) {
        append_output(pp, macro->replacement, macro->replacement_length);
    }

    return true;
}

/* 解析函数宏的实参列表 */
static bool parse_function_macro_args(CnPreprocessor *pp, char ***out_args, size_t *out_count)
{
    char **args = NULL;
    size_t arg_count = 0;
    size_t arg_capacity = 4;
    int paren_depth = 0;
    
    args = (char **)malloc(arg_capacity * sizeof(char *));
    if (!args) {
        return false;
    }
    
    /* 跳过左括号 */
    advance(pp);
    skip_whitespace(pp);
    
    /* 空参数列表 */
    if (current_char(pp) == ')') {
        advance(pp);
        *out_args = args;
        *out_count = 0;
        return true;
    }
    
    /* 读取参数 */
    while (current_char(pp) != '\0') {
        size_t arg_start = pp->current_offset;
        size_t arg_length = 0;
        paren_depth = 0;
        
        /* 读取一个参数 (到逗号或右括号) */
        while (current_char(pp) != '\0') {
            char c = current_char(pp);
            
            if (c == '(' ) {
                paren_depth++;
                advance(pp);
            } else if (c == ')') {
                if (paren_depth == 0) {
                    break;
                }
                paren_depth--;
                advance(pp);
            } else if (c == ',' && paren_depth == 0) {
                break;
            } else {
                advance(pp);
            }
        }
        
        arg_length = pp->current_offset - arg_start;
        
        /* 去除参数前后的空白 */
        while (arg_length > 0 && 
               (pp->source[arg_start] == ' ' || pp->source[arg_start] == '\t')) {
            arg_start++;
            arg_length--;
        }
        while (arg_length > 0 &&
               (pp->source[arg_start + arg_length - 1] == ' ' ||
                pp->source[arg_start + arg_length - 1] == '\t')) {
            arg_length--;
        }
        
        /* 扩展参数数组 */
        if (arg_count >= arg_capacity) {
            arg_capacity *= 2;
            char **new_args = (char **)realloc(args, arg_capacity * sizeof(char *));
            if (!new_args) {
                for (size_t i = 0; i < arg_count; ++i) {
                    free(args[i]);
                }
                free(args);
                return false;
            }
            args = new_args;
        }
        
        /* 复制参数 */
        args[arg_count] = (char *)malloc(arg_length + 1);
        if (!args[arg_count]) {
            for (size_t i = 0; i < arg_count; ++i) {
                free(args[i]);
            }
            free(args);
            return false;
        }
        memcpy(args[arg_count], pp->source + arg_start, arg_length);
        args[arg_count][arg_length] = '\0';
        arg_count++;
        
        if (current_char(pp) == ',') {
            advance(pp);
            skip_whitespace(pp);
        } else if (current_char(pp) == ')') {
            advance(pp);
            break;
        } else {
            /* 格式错误 */
            for (size_t i = 0; i < arg_count; ++i) {
                free(args[i]);
            }
            free(args);
            return false;
        }
    }
    
    *out_args = args;
    *out_count = arg_count;
    return true;
}

/* 在宏替换文本中替换形参为实参,支持 # 字符串化 */
static char* substitute_macro_params(const CnMacro *macro, char **args, size_t arg_count)
{
    size_t output_capacity = 256;
    size_t output_length = 0;
    char *output = (char *)malloc(output_capacity);
    size_t i;
    
    if (!output) {
        return NULL;
    }
    
    if (!macro->replacement) {
        output[0] = '\0';
        return output;
    }
    
    /* 扫描替换文本 */
    i = 0;
    while (i < macro->replacement_length) {
        char c = macro->replacement[i];
        
        /* 检查字符串化操作符 # */
        if (c == '#' && i + 1 < macro->replacement_length) {
            i++;  /* 跳过 # */
            
            /* 跳过空白 */
            while (i < macro->replacement_length && 
                   (macro->replacement[i] == ' ' || macro->replacement[i] == '\t')) {
                i++;
            }
            
            /* 读取参数名 */
            if (i < macro->replacement_length && is_identifier_start(macro->replacement[i])) {
                size_t param_start = i;
                while (i < macro->replacement_length && 
                       is_identifier_continue(macro->replacement[i])) {
                    i++;
                }
                size_t param_length = i - param_start;
                
                /* 查找对应的形参 */
                int param_index = -1;
                for (size_t j = 0; j < macro->param_count; ++j) {
                    if (strlen(macro->params[j]) == param_length &&
                        memcmp(macro->params[j], macro->replacement + param_start, param_length) == 0) {
                        param_index = (int)j;
                        break;
                    }
                }
                
                if (param_index >= 0 && (size_t)param_index < arg_count) {
                    /* 将实参字符串化 */
                    size_t arg_len = strlen(args[param_index]);
                    size_t needed = output_length + arg_len + 3;  /* 包括两个引号 */
                    
                    if (needed >= output_capacity) {
                        output_capacity = needed * 2;
                        char *new_output = (char *)realloc(output, output_capacity);
                        if (!new_output) {
                            free(output);
                            return NULL;
                        }
                        output = new_output;
                    }
                    
                    output[output_length++] = '"';
                    memcpy(output + output_length, args[param_index], arg_len);
                    output_length += arg_len;
                    output[output_length++] = '"';
                }
                continue;
            }
        }
        
        /* 检查是否是参数名 */
        if (is_identifier_start(c)) {
            size_t param_start = i;
            while (i < macro->replacement_length && 
                   is_identifier_continue(macro->replacement[i])) {
                i++;
            }
            size_t param_length = i - param_start;
            
            /* 查找对应的形参 */
            int param_index = -1;
            for (size_t j = 0; j < macro->param_count; ++j) {
                if (strlen(macro->params[j]) == param_length &&
                    memcmp(macro->params[j], macro->replacement + param_start, param_length) == 0) {
                    param_index = (int)j;
                    break;
                }
            }
            
            if (param_index >= 0 && (size_t)param_index < arg_count) {
                /* 替换为实参 */
                size_t arg_len = strlen(args[param_index]);
                size_t needed = output_length + arg_len + 1;
                
                if (needed >= output_capacity) {
                    output_capacity = needed * 2;
                    char *new_output = (char *)realloc(output, output_capacity);
                    if (!new_output) {
                        free(output);
                        return NULL;
                    }
                    output = new_output;
                }
                
                memcpy(output + output_length, args[param_index], arg_len);
                output_length += arg_len;
            } else {
                /* 不是形参,保留原文本 */
                size_t needed = output_length + param_length + 1;
                if (needed >= output_capacity) {
                    output_capacity = needed * 2;
                    char *new_output = (char *)realloc(output, output_capacity);
                    if (!new_output) {
                        free(output);
                        return NULL;
                    }
                    output = new_output;
                }
                memcpy(output + output_length, macro->replacement + param_start, param_length);
                output_length += param_length;
            }
            continue;
        }
        
        /* 普通字符 */
        if (output_length + 1 >= output_capacity) {
            output_capacity *= 2;
            char *new_output = (char *)realloc(output, output_capacity);
            if (!new_output) {
                free(output);
                return NULL;
            }
            output = new_output;
        }
        output[output_length++] = c;
        i++;
    }
    
    output[output_length] = '\0';
    return output;
}

/* ========== 条件编译栈管理 ========== */

static bool is_condition_active(const CnPreprocessor *pp)
{
    /* 没有条件时,默认激活 */
    if (pp->condition_depth == 0) {
        return true;
    }

    /* 检查当前条件是否激活 */
    return pp->condition_stack[pp->condition_depth - 1].active;
}

static bool push_condition(CnPreprocessor *pp, bool active)
{
    CnConditionFrame *new_stack;
    size_t new_capacity;

    if (pp->condition_depth >= pp->condition_capacity) {
        new_capacity = pp->condition_capacity == 0 ? 8 : pp->condition_capacity * 2;
        new_stack = (CnConditionFrame *)realloc(
            pp->condition_stack,
            new_capacity * sizeof(CnConditionFrame)
        );
        if (!new_stack) {
            return false;
        }
        pp->condition_stack = new_stack;
        pp->condition_capacity = new_capacity;
    }

    pp->condition_stack[pp->condition_depth].active = active;
    pp->condition_stack[pp->condition_depth].has_executed = active;
    pp->condition_stack[pp->condition_depth].start_line = pp->current_line;
    pp->condition_depth++;

    return true;
}

static bool pop_condition(CnPreprocessor *pp)
{
    if (pp->condition_depth == 0) {
        return false;
    }

    pp->condition_depth--;
    return true;
}
