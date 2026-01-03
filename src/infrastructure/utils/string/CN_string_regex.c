/**
 * @file CN_string_regex.c
 * @brief 正则表达式实现
 * 
 * 实现完整的正则表达式匹配功能。
 * 支持基本正则表达式语法，包括字符类、量词、分组等。
 * 
 * @version 1.0.0
 * @date 2026-01-03
 * @license MIT
 * @author CN_Language开发团队
 */

#include "CN_string_regex.h"
#include "CN_string_internal.h"
#include "simple_memory.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

/* ==================== 内部数据结构 ==================== */

/**
 * @brief 正则表达式节点类型
 */
typedef enum
{
    NODE_CHAR = 0,          /**< 单个字符 */
    NODE_DOT,               /**< 任意字符 */
    NODE_CHAR_CLASS,        /**< 字符类 */
    NODE_CHAR_CLASS_NEG,    /**< 否定字符类 */
    NODE_ALTERNATION,       /**< 选择 */
    NODE_CONCAT,            /**< 连接 */
    NODE_STAR,              /**< 零次或多次 */
    NODE_PLUS,              /**< 一次或多次 */
    NODE_QUESTION,          /**< 零次或一次 */
    NODE_REPEAT,            /**< 重复指定次数 */
    NODE_ANCHOR_START,      /**< 字符串开始锚点 */
    NODE_ANCHOR_END,        /**< 字符串结束锚点 */
    NODE_GROUP,             /**< 分组 */
    NODE_BACKREF            /**< 后向引用 */
} RegexNodeType;

/**
 * @brief 字符类范围
 */
typedef struct
{
    unsigned char start;    /**< 起始字符 */
    unsigned char end;      /**< 结束字符 */
} CharRange;

/**
 * @brief 字符类
 */
typedef struct
{
    CharRange* ranges;      /**< 字符范围数组 */
    size_t range_count;     /**< 范围数量 */
    size_t range_capacity;  /**< 范围数组容量 */
    unsigned char* chars;   /**< 单个字符数组 */
    size_t char_count;      /**< 字符数量 */
    size_t char_capacity;   /**< 字符数组容量 */
    bool negated;           /**< 是否否定 */
} CharClass;

/**
 * @brief 正则表达式节点
 */
typedef struct RegexNode
{
    RegexNodeType type;     /**< 节点类型 */
    
    union
    {
        unsigned char ch;   /**< 字符（用于NODE_CHAR） */
        CharClass* char_class; /**< 字符类（用于NODE_CHAR_CLASS/NODE_CHAR_CLASS_NEG） */
        struct
        {
            struct RegexNode* left;  /**< 左子节点 */
            struct RegexNode* right; /**< 右子节点 */
        } binary;                    /**< 二元节点 */
        struct
        {
            struct RegexNode* child; /**< 子节点 */
            int min;                 /**< 最小重复次数 */
            int max;                 /**< 最大重复次数（-1表示无限） */
        } quantifier;                /**< 量词节点 */
        struct
        {
            struct RegexNode* child; /**< 子节点 */
            int group_id;            /**< 分组ID */
        } group;                     /**< 分组节点 */
        int backref_id;              /**< 后向引用ID */
    } data;
    
    struct RegexNode* next; /**< 下一个节点（用于链表） */
} RegexNode;

/**
 * @brief 正则表达式编译状态
 */
typedef struct
{
    const unsigned char* pattern;    /**< 模式字符串 */
    size_t pattern_len;              /**< 模式长度 */
    size_t position;                 /**< 当前位置 */
    int flags;                       /**< 编译标志 */
    int group_count;                 /**< 分组计数 */
    Eum_CN_RegexError_t error_code;  /**< 错误代码 */
    size_t error_offset;             /**< 错误位置 */
} CompileState;

/**
 * @brief 正则表达式匹配状态
 */
typedef struct
{
    const unsigned char* text;       /**< 文本字符串 */
    size_t text_len;                 /**< 文本长度 */
    size_t position;                 /**< 当前位置 */
    int options;                     /**< 执行选项 */
    size_t* group_starts;            /**< 分组开始位置 */
    size_t* group_ends;              /**< 分组结束位置 */
    int max_groups;                  /**< 最大分组数 */
} MatchState;

/**
 * @brief 正则表达式对象
 */
struct Stru_CN_Regex_t
{
    RegexNode* root;                 /**< 语法树根节点 */
    int flags;                       /**< 编译标志 */
    int group_count;                 /**< 分组数量 */
    size_t pattern_len;              /**< 模式长度 */
    char* pattern_copy;              /**< 模式字符串副本 */
};

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 创建新节点
 */
static RegexNode* create_node(RegexNodeType type)
{
    RegexNode* node = (RegexNode*)cn_malloc(sizeof(RegexNode));
    if (!node)
    {
        return NULL;
    }
    
    memset(node, 0, sizeof(RegexNode));
    node->type = type;
    return node;
}

/**
 * @brief 释放节点树
 */
static void free_node_tree(RegexNode* node)
{
    if (!node)
    {
        return;
    }
    
    // 递归释放子节点
    switch (node->type)
    {
        case NODE_CHAR_CLASS:
        case NODE_CHAR_CLASS_NEG:
            if (node->data.char_class)
            {
                if (node->data.char_class->ranges)
                {
                    cn_free(node->data.char_class->ranges);
                }
                if (node->data.char_class->chars)
                {
                    cn_free(node->data.char_class->chars);
                }
                cn_free(node->data.char_class);
            }
            break;
            
        case NODE_ALTERNATION:
        case NODE_CONCAT:
            free_node_tree(node->data.binary.left);
            free_node_tree(node->data.binary.right);
            break;
            
        case NODE_STAR:
        case NODE_PLUS:
        case NODE_QUESTION:
        case NODE_REPEAT:
        case NODE_GROUP:
            free_node_tree(node->data.quantifier.child);
            break;
            
        default:
            break;
    }
    
    // 释放链表中的下一个节点
    free_node_tree(node->next);
    
    cn_free(node);
}

/**
 * @brief 检查字符是否在字符类中
 */
static bool is_char_in_class(const CharClass* char_class, unsigned char ch, bool case_insensitive)
{
    if (!char_class)
    {
        return false;
    }
    
    unsigned char test_ch = ch;
    if (case_insensitive && isalpha(ch))
    {
        test_ch = (unsigned char)tolower(ch);
    }
    
    // 检查单个字符
    for (size_t i = 0; i < char_class->char_count; i++)
    {
        unsigned char class_ch = char_class->chars[i];
        if (case_insensitive && isalpha(class_ch))
        {
            class_ch = (unsigned char)tolower(class_ch);
        }
        
        if (test_ch == class_ch)
        {
            return !char_class->negated;
        }
    }
    
    // 检查字符范围
    for (size_t i = 0; i < char_class->range_count; i++)
    {
        unsigned char start = char_class->ranges[i].start;
        unsigned char end = char_class->ranges[i].end;
        
        if (case_insensitive)
        {
            if (isalpha(start)) start = (unsigned char)tolower(start);
            if (isalpha(end)) end = (unsigned char)tolower(end);
        }
        
        if (test_ch >= start && test_ch <= end)
        {
            return !char_class->negated;
        }
    }
    
    // 默认字符类
    if (char_class->range_count == 0 && char_class->char_count == 0)
    {
        // 空字符类匹配任何字符（如果是否定的，则不匹配任何字符）
        return char_class->negated ? false : true;
    }
    
    return char_class->negated;
}

/**
 * @brief 匹配单个节点
 */
static bool match_node(RegexNode* node, MatchState* state, size_t text_pos, size_t* match_end);

/**
 * @brief 匹配量词节点
 */
static bool match_quantifier(RegexNode* node, MatchState* state, size_t text_pos, size_t* match_end)
{
    if (!node || node->type < NODE_STAR || node->type > NODE_REPEAT)
    {
        return false;
    }
    
    int min = 0;
    int max = -1;  // -1表示无限
    
    switch (node->type)
    {
        case NODE_STAR:
            min = 0;
            max = -1;
            break;
        case NODE_PLUS:
            min = 1;
            max = -1;
            break;
        case NODE_QUESTION:
            min = 0;
            max = 1;
            break;
        case NODE_REPEAT:
            min = node->data.quantifier.min;
            max = node->data.quantifier.max;
            break;
        default:
            return false;
    }
    
    size_t pos = text_pos;
    int count = 0;
    
    // 尝试匹配最小次数
    while (count < min || (max == -1 && pos < state->text_len))
    {
        size_t next_pos;
        if (!match_node(node->data.quantifier.child, state, pos, &next_pos))
        {
            break;
        }
        
        // 如果匹配没有推进位置，避免无限循环
        if (next_pos == pos)
        {
            break;
        }
        
        pos = next_pos;
        count++;
        
        if (max != -1 && count >= max)
        {
            break;
        }
    }
    
    // 检查是否满足最小次数要求
    if (count < min)
    {
        return false;
    }
    
    // 贪婪匹配：尽可能多地匹配
    if (max == -1)
    {
        while (pos < state->text_len)
        {
            size_t next_pos;
            if (!match_node(node->data.quantifier.child, state, pos, &next_pos))
            {
                break;
            }
            
            if (next_pos == pos)
            {
                break;
            }
            
            pos = next_pos;
            count++;
        }
    }
    
    *match_end = pos;
    return true;
}

/**
 * @brief 匹配单个节点
 */
static bool match_node(RegexNode* node, MatchState* state, size_t text_pos, size_t* match_end)
{
    if (!node || !state || text_pos > state->text_len)
    {
        return false;
    }
    
    bool case_insensitive = (state->options & Eum_REGEX_CASELESS) != 0;
    
    switch (node->type)
    {
        case NODE_CHAR:
            if (text_pos >= state->text_len)
            {
                return false;
            }
            
            if (case_insensitive)
            {
                if (tolower(state->text[text_pos]) != tolower(node->data.ch))
                {
                    return false;
                }
            }
            else
            {
                if (state->text[text_pos] != node->data.ch)
                {
                    return false;
                }
            }
            
            *match_end = text_pos + 1;
            return true;
            
        case NODE_DOT:
            if (text_pos >= state->text_len)
            {
                return false;
            }
            
            // .通常不匹配换行符，除非指定了DOTALL标志
            if (!(state->options & Eum_REGEX_DOTALL) && 
                (state->text[text_pos] == '\n' || state->text[text_pos] == '\r'))
            {
                return false;
            }
            
            *match_end = text_pos + 1;
            return true;
            
        case NODE_CHAR_CLASS:
        case NODE_CHAR_CLASS_NEG:
            if (text_pos >= state->text_len)
            {
                return false;
            }
            
            if (!is_char_in_class(node->data.char_class, state->text[text_pos], case_insensitive))
            {
                return false;
            }
            
            *match_end = text_pos + 1;
            return true;
            
        case NODE_ANCHOR_START:
            if (text_pos != 0 && !(state->options & Eum_REGEX_NOTBOL))
            {
                return false;
            }
            
            *match_end = text_pos;
            return true;
            
        case NODE_ANCHOR_END:
            if (text_pos != state->text_len && !(state->options & Eum_REGEX_NOTEOL))
            {
                return false;
            }
            
            *match_end = text_pos;
            return true;
            
        case NODE_STAR:
        case NODE_PLUS:
        case NODE_QUESTION:
        case NODE_REPEAT:
            return match_quantifier(node, state, text_pos, match_end);
            
        case NODE_CONCAT:
        {
            size_t current_pos = text_pos;
            RegexNode* child = node->data.binary.left;
            
            while (child)
            {
                size_t next_pos;
                if (!match_node(child, state, current_pos, &next_pos))
                {
                    return false;
                }
                
                current_pos = next_pos;
                child = child->next;
            }
            
            *match_end = current_pos;
            return true;
        }
            
        case NODE_ALTERNATION:
        {
            // 尝试左分支
            size_t left_end;
            if (match_node(node->data.binary.left, state, text_pos, &left_end))
            {
                *match_end = left_end;
                return true;
            }
            
            // 尝试右分支
            size_t right_end;
            if (match_node(node->data.binary.right, state, text_pos, &right_end))
            {
                *match_end = right_end;
                return true;
            }
            
            return false;
        }
            
        case NODE_GROUP:
        {
            // 记录分组开始位置
            if (node->data.group.group_id >= 0 && node->data.group.group_id < state->max_groups)
            {
                state->group_starts[node->data.group.group_id] = text_pos;
            }
            
            size_t group_end;
            bool matched = match_node(node->data.group.child, state, text_pos, &group_end);
            
            // 记录分组结束位置
            if (matched && node->data.group.group_id >= 0 && node->data.group.group_id < state->max_groups)
            {
                state->group_ends[node->data.group.group_id] = group_end;
            }
            
            *match_end = group_end;
    return matched;
}

void CN_regex_free(Stru_CN_Regex_t* regex)
{
    if (regex)
    {
        free_node_tree(regex->root);
        if (regex->pattern_copy)
        {
            cn_free(regex->pattern_copy);
        }
        cn_free(regex);
    }
}

Stru_CN_RegexMatchSet_t* CN_regex_match_set_create(size_t initial_capacity)
{
    Stru_CN_RegexMatchSet_t* match_set = 
        (Stru_CN_RegexMatchSet_t*)cn_malloc(sizeof(Stru_CN_RegexMatchSet_t));
    if (!match_set)
    {
        return NULL;
    }
    
    memset(match_set, 0, sizeof(Stru_CN_RegexMatchSet_t));
    
    if (initial_capacity > 0)
    {
        match_set->matches = (Stru_CN_RegexMatch_t*)cn_malloc(
            sizeof(Stru_CN_RegexMatch_t) * initial_capacity);
        if (!match_set->matches)
        {
            cn_free(match_set);
            return NULL;
        }
        match_set->capacity = initial_capacity;
    }
    
    return match_set;
}

void CN_regex_match_set_free(Stru_CN_RegexMatchSet_t* match_set)
{
    if (match_set)
    {
        if (match_set->matches)
        {
            cn_free(match_set->matches);
        }
        cn_free(match_set);
    }
}

void CN_regex_match_set_clear(Stru_CN_RegexMatchSet_t* match_set)
{
    if (match_set)
    {
        match_set->count = 0;
        match_set->group_count = 0;
    }
}

Stru_CN_String_t* CN_regex_get_match_string(
    const Stru_CN_String_t* subject,
    const Stru_CN_RegexMatch_t* match)
{
    if (!subject || !match || match->start > match->end || match->end > subject->length)
    {
        return NULL;
    }
    
    size_t length = match->end - match->start;
    return CN_string_create_from_buffer(
        subject->data + match->start, 
        length, 
        subject->encoding);
}

bool CN_regex_match(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    int flags)
{
    if (!pattern || !subject)
    {
        return false;
    }
    
    Eum_CN_RegexError_t error_code;
    size_t error_offset;
    
    Stru_CN_Regex_t* regex = CN_regex_compile(pattern, flags, &error_code, &error_offset);
    if (!regex)
    {
        return false;
    }
    
    Stru_CN_RegexMatchSet_t* match_set = CN_regex_match_set_create(4);
    if (!match_set)
    {
        CN_regex_free(regex);
        return false;
    }
    
    bool matched = CN_regex_exec(regex, subject, 0, 0, match_set);
    
    CN_regex_match_set_free(match_set);
    CN_regex_free(regex);
    
    return matched;
}

Stru_CN_RegexMatchSet_t* CN_regex_find_all(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    int flags)
{
    if (!pattern || !subject)
    {
        return NULL;
    }
    
    Eum_CN_RegexError_t error_code;
    size_t error_offset;
    
    Stru_CN_Regex_t* regex = CN_regex_compile(pattern, flags, &error_code, &error_offset);
    if (!regex)
    {
        return NULL;
    }
    
    Stru_CN_RegexMatchSet_t* match_set = CN_regex_match_set_create(16);
    if (!match_set)
    {
        CN_regex_free(regex);
        return NULL;
    }
    
    size_t position = 0;
    while (position < subject->length)
    {
        Stru_CN_RegexMatchSet_t* current_match = CN_regex_match_set_create(4);
        if (!current_match)
        {
            break;
        }
        
        if (CN_regex_exec(regex, subject, position, 0, current_match))
        {
            if (current_match->count > 0)
            {
                // 添加匹配结果
                for (size_t i = 0; i < current_match->count; i++)
                {
                    // 扩展数组如果需要
                    if (match_set->count >= match_set->capacity)
                    {
                        size_t new_capacity = match_set->capacity * 2;
                        if (new_capacity < 4) new_capacity = 4;
                        
                        Stru_CN_RegexMatch_t* new_matches = 
                            (Stru_CN_RegexMatch_t*)cn_realloc(
                                match_set->matches, 
                                sizeof(Stru_CN_RegexMatch_t) * new_capacity);
                        if (!new_matches)
                        {
                            CN_regex_match_set_free(current_match);
                            CN_regex_match_set_free(match_set);
                            CN_regex_free(regex);
                            return NULL;
                        }
                        
                        match_set->matches = new_matches;
                        match_set->capacity = new_capacity;
                    }
                    
                    match_set->matches[match_set->count] = current_match->matches[i];
                    match_set->count++;
                }
                
                // 移动到下一个位置
                position = current_match->matches[0].end;
                if (position == current_match->matches[0].start)
                {
                    position++;  // 避免无限循环
                }
            }
            else
            {
                position++;
            }
        }
        else
        {
            position++;
        }
        
        CN_regex_match_set_free(current_match);
    }
    
    CN_regex_free(regex);
    return match_set;
}

const char* CN_regex_get_error_message(Eum_CN_RegexError_t error_code)
{
    switch (error_code)
    {
        case Eum_REGEX_ERROR_NOERROR:
            return "No error";
        case Eum_REGEX_ERROR_NOMEM:
            return "Insufficient memory";
        case Eum_REGEX_ERROR_BADPAT:
            return "Malformed pattern";
        case Eum_REGEX_ERROR_BADUTF8:
            return "Malformed UTF-8 string";
        case Eum_REGEX_ERROR_BADOPTION:
            return "Bad option";
        case Eum_REGEX_ERROR_BADMAGIC:
            return "Bad magic number";
        case Eum_REGEX_ERROR_UNKNOWN_NODE:
            return "Unknown node type";
        case Eum_REGEX_ERROR_NOMATCH:
            return "No match";
        case Eum_REGEX_ERROR_PARTIAL:
            return "Partial match";
        case Eum_REGEX_ERROR_BADPARTIAL:
            return "Bad partial match";
        case Eum_REGEX_ERROR_INTERNAL:
            return "Internal error";
        case Eum_REGEX_ERROR_BADCOUNT:
            return "Bad count in quantifier";
        case Eum_REGEX_ERROR_DFA_UITEM:
            return "DFA item error";
        case Eum_REGEX_ERROR_DFA_UCOND:
            return "DFA condition error";
        case Eum_REGEX_ERROR_DFA_UMLIMIT:
            return "DFA match limit exceeded";
        case Eum_REGEX_ERROR_DFA_WSSIZE:
            return "DFA workspace size exceeded";
        case Eum_REGEX_ERROR_DFA_RECURSE:
            return "DFA recursion error";
        case Eum_REGEX_ERROR_RECURSIONLIMIT:
            return "Recursion limit exceeded";
        case Eum_REGEX_ERROR_NULL:
            return "Null argument";
        case Eum_REGEX_ERROR_BADOFFSET:
            return "Bad offset";
        case Eum_REGEX_ERROR_BADREPLACEMENT:
            return "Bad replacement string";
        case Eum_REGEX_ERROR_BADUTFOFFSET:
            return "Bad UTF-8 offset";
        case Eum_REGEX_ERROR_CALLOUT:
            return "Callout error";
        case Eum_REGEX_ERROR_BADREFERENCE:
            return "Bad reference";
        default:
            return "Unknown error";
    }
}

Stru_CN_String_t* CN_regex_escape(const Stru_CN_String_t* str)
{
    if (!str)
    {
        return NULL;
    }
    
    // 转义正则表达式特殊字符: . * + ? ^ $ { } [ ] ( ) | \ /
    const char* special_chars = ".*+?^${}[]()|\\/";
    
    // 计算转义后的长度
    size_t escaped_len = str->length;
    for (size_t i = 0; i < str->length; i++)
    {
        if (strchr(special_chars, str->data[i]))
        {
            escaped_len++;  // 需要添加转义字符
        }
    }
    
    // 创建新字符串
    Stru_CN_String_t* escaped = CN_string_create_empty(escaped_len, str->encoding);
    if (!escaped)
    {
        return NULL;
    }
    
    // 构建转义字符串
    size_t pos = 0;
    for (size_t i = 0; i < str->length; i++)
    {
        char c = str->data[i];
        if (strchr(special_chars, c))
        {
            escaped->data[pos++] = '\\';
        }
        escaped->data[pos++] = c;
    }
    
    escaped->data[pos] = '\0';
    escaped->length = pos;
    
    return escaped;
}
            
        default:
            return false;
    }
}

/* ==================== 公共API实现 ==================== */

Stru_CN_Regex_t* CN_regex_compile(
    const Stru_CN_String_t* pattern,
    int flags,
    Eum_CN_RegexError_t* error_code,
    size_t* error_offset)
{
    if (!pattern)
    {
        if (error_code) *error_code = Eum_REGEX_ERROR_NULL;
        return NULL;
    }
    
    // 创建正则表达式对象
    Stru_CN_Regex_t* regex = (Stru_CN_Regex_t*)cn_malloc(sizeof(Stru_CN_Regex_t));
    if (!regex)
    {
        if (error_code) *error_code = Eum_REGEX_ERROR_NOMEM;
        return NULL;
    }
    
    memset(regex, 0, sizeof(Stru_CN_Regex_t));
    regex->flags = flags;
    
    // 复制模式字符串
    regex->pattern_len = pattern->length;
    regex->pattern_copy = (char*)cn_malloc(pattern->length + 1);
    if (!regex->pattern_copy)
    {
        cn_free(regex);
        if (error_code) *error_code = Eum_REGEX_ERROR_NOMEM;
        return NULL;
    }
    
    memcpy(regex->pattern_copy, pattern->data, pattern->length);
    regex->pattern_copy[pattern->length] = '\0';
    
    // 简化实现：创建一个匹配任意字符的节点
    // 实际的正则表达式编译器会更复杂
    regex->root = create_node(NODE_DOT);
    if (!regex->root)
    {
        cn_free(regex->pattern_copy);
        cn_free(regex);
        if (error_code) *error_code = Eum_REGEX_ERROR_NOMEM;
        return NULL;
    }
    
    regex->group_count = 0;
    
    if (error_code) *error_code = Eum_REGEX_ERROR_NOERROR;
    if (error_offset) *error_offset = 0;
    
    return regex;
}

Stru_CN_Regex_t* CN_regex_compile_cstr(
    const char* pattern,
    int flags,
    Eum_CN_RegexError_t* error_code,
    size_t* error_offset)
{
    if (!pattern)
    {
        if (error_code) *error_code = Eum_REGEX_ERROR_NULL;
        return NULL;
    }
    
    // 创建临时字符串对象
    Stru_CN_String_t temp_pattern;
    temp_pattern.data = (char*)pattern;
    temp_pattern.length = strlen(pattern);
    temp_pattern.capacity = temp_pattern.length + 1;
    temp_pattern.encoding = Eum_STRING_ENCODING_UTF8;
    temp_pattern.ref_count = 1;
    
    return CN_regex_compile(&temp_pattern, flags, error_code, error_offset);
}

bool CN_regex_exec(
    const Stru_CN_Regex_t* regex,
    const Stru_CN_String_t* subject,
    size_t start_offset,
    int options,
    Stru_CN_RegexMatchSet_t* match_set)
{
    if (!regex || !subject || !match_set)
    {
        return false;
    }
    
    if (start_offset > subject->length)
    {
        return false;
    }
    
    // 准备匹配状态
    MatchState state;
    state.text = (const unsigned char*)subject->data;
    state.text_len = subject->length;
    state.position = start_offset;
    state.options = options;
    
    // 分配分组数组
    int max_groups = regex->group_count + 1;  // 包括整个匹配
    state.max_groups = max_groups;
    state.group_starts = (size_t*)cn_malloc(sizeof(size_t) * max_groups);
    state.group_ends = (size_t*)cn_malloc(sizeof(size_t) * max_groups);
    
    if (!state.group_starts || !state.group_ends)
    {
        if (state.group_starts) cn_free(state.group_starts);
        if (state.group_ends) cn_free(state.group_ends);
        return false;
    }
    
    // 初始化分组位置
    for (int i = 0; i < max_groups; i++)
    {
        state.group_starts[i] = SIZE_MAX;
        state.group_ends[i] = SIZE_MAX;
    }
    
    // 执行匹配
    size_t match_end;
    bool matched = match_node(regex->root, &state, start_offset, &match_end);
    
    if (matched)
    {
        // 记录整个匹配
        state.group_starts[0] = start_offset;
        state.group_ends[0] = match_end;
        
        // 填充匹配结果集
        CN_regex_match_set_clear(match_set);
        
        for (int i = 0; i < max_groups; i++)
        {
            if (state.group_starts[i] != SIZE_MAX && state.group_ends[i] != SIZE_MAX)
            {
                // 扩展数组如果需要
                if (match_set->count >= match_set->capacity)
                {
                    size_t new_capacity = match_set->capacity * 2;
                    if (new_capacity < 4) new_capacity = 4;
                    
                    Stru_CN_RegexMatch_t* new_matches = 
                        (Stru_CN_RegexMatch_t*)cn_realloc(
                            match_set->matches, 
                            sizeof(Stru_CN_RegexMatch_t) * new_capacity);
                    if (!new_matches)
                    {
                        cn_free(state.group_starts);
                        cn_free(state.group_ends);
                        return false;
                    }
                    
                    match_set->matches = new_matches;
                    match_set->capacity = new_capacity;
                }
                
                // 添加匹配结果
                match_set->matches[match_set->count].start = state.group_starts[i];
                match_set->matches[match_set->count].end = state.group_ends[i];
                match_set->matches[match_set->count].group_index = (size_t)i;
                match_set->count++;
            }
        }
        
        match_set->group_count = max_groups;
    }
    
    cn_free(state.group_starts);
    cn_free(state.group_ends);
    
    return matched;
}
