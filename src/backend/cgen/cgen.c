#include "cnlang/backend/cgen.h"
#include "cnlang/backend/cgen/class_cgen.h"  // 类代码生成接口
#include "cnlang/backend/template_cgen.h"    // 模板代码生成接口
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/config.h"
#include "cnlang/frontend/semantics.h"  // 引入作用域和类型信息
#include "cnlang/frontend/ast/class_node.h"  // 类AST节点定义
#include "cnlang/runtime/cli.h"              // 命令行参数接口
#include "cnlang/frontend/module_loader.h"   // 模块加载器接口
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/mman.h>
#endif

/* 性能优化：输出缓冲区大小 */
#define CGEN_BUFFER_SIZE 8192

static CnTargetDataLayout g_target_layout;
static bool g_target_layout_valid = false;

/* ============================================================================
 * CN自举编译器类型系统支持
 * CN编译器使用 struct 类型节点 结构，其中 指针层级 字段表示指针类型
 * 而C代码生成器期望 CnType 结构，使用 kind 字段
 * 以下函数提供统一的指针类型检测接口
 * ============================================================================ */

/* ============================================================================
 * CN基本类型关键字检测
 * 自举编译时，解析器/语义分析器可能将CN源码中的基本类型关键字（整数、字符串、布尔等）
 * 错误标记为 CN_TYPE_STRUCT 类型（名称为中文关键字字符串）。
 * 以下函数检测这种情况，并返回对应的C类型字符串。
 * ============================================================================ */

/**
 * @brief 检查结构体类型名是否是CN基本类型关键字，并返回对应的C类型字符串
 *
 * 当自举编译时，CN基本类型关键字可能被错误标记为CN_TYPE_STRUCT，
 * 此函数检测这种情况并返回正确的C类型字符串。
 *
 * @param name 类型名称
 * @param name_len 类型名称长度（字节数）
 * @return 对应的C类型字符串，如果不是CN基本类型关键字则返回NULL
 *
 * UTF-8编码下中文字符串长度：
 * - "整数" = 6字节, "小数" = 6字节, "字符" = 6字节
 * - "字符串" = 9字节, "布尔" = 6字节, "空类型" = 9字节
 */
static const char *try_cn_primitive_type_name(const char *name, size_t name_len) {
    if (!name || name_len == 0) return NULL;

    /* 使用长度先做快速筛选，再用strncmp精确匹配
     * UTF-8编码下中文字符串长度：
     * - 1个汉字 = 3字节, 2个汉字 = 6字节, 3个汉字 = 9字节
     * 包含：CN基本类型关键字 + CN元类型关键字（函数/无/结构体）
     * 【P0-3a修复】扩展映射：添加 函数→void*、无→void*、结构体→void*
     */
    switch (name_len) {
        case 3: /* 1个汉字 × 3字节 = 3字节 */
            /* 【P0-3a】"无"是CN的null/none关键字，被错误标记为结构体类型时映射为void* */
            if (strncmp(name, "无", 3) == 0) return "void*";
            break;
        case 6: /* 2个汉字 × 3字节 = 6字节 */
            if (strncmp(name, "整数", 6) == 0) return "long long";
            if (strncmp(name, "小数", 6) == 0) return "double";
            if (strncmp(name, "字符", 6) == 0) return "char";
            if (strncmp(name, "布尔", 6) == 0) return "_Bool";
            /* 【P0-3a】"函数"作为类型名使用时，映射为通用函数指针void* */
            if (strncmp(name, "函数", 6) == 0) return "void*";
            break;
        case 9: /* 3个汉字 × 3字节 = 9字节 */
            if (strncmp(name, "字符串", 9) == 0) return "char*";
            if (strncmp(name, "空类型", 9) == 0) return "void";
            /* 【P0-3a】"结构体"本身作为类型名时是元类型，不应生成struct定义 */
            if (strncmp(name, "结构体", 9) == 0) return "void*";
            break;
        default:
            break;
    }
    return NULL;
}

/**
 * @brief 检查类型名是否是CN基本类型关键字（布尔版本）
 *
 * @param name 类型名称
 * @param name_len 类型名称长度（字节数）
 * @return true 如果是CN基本类型关键字，false 否则
 */
static bool is_cn_primitive_type_name(const char *name, size_t name_len) {
    return try_cn_primitive_type_name(name, name_len) != NULL;
}

/* 检测类型是否为指针类型 - 支持 CnType 和 struct 类型节点 两种类型系统
 * 参数 type: 类型指针（可能是 CnType* 或 struct 类型节点*）
 * 返回: true 如果是指针类型，false 否则
 */
static bool is_pointer_type_unified(void *type) {
    if (!type) return false;
    
    /* 首先尝试作为 CnType 检测
     * CnTypeKind 枚举值范围是 0-51（CN_TYPE_VOID 到 CN_TYPE_UNKNOWN）
     * 如果 kind 值在此范围内，按 CnType 处理
     */
    CnType *cn_type = (CnType *)type;
    
    /* 检查 kind 值是否在 CnTypeKind 有效范围内 */
    if (cn_type->kind >= 0 && cn_type->kind <= CN_TYPE_UNKNOWN) {
        /* 作为 CnType 处理 */
        if (cn_type->kind == CN_TYPE_POINTER) {
            return true;
        }
        return false;
    }
    
    /* kind 值超出 CnTypeKind 范围，可能是 struct 类型节点
     * struct 类型节点 的布局：
     * - 第一个字段是 enum 节点类型 类型 (4 字节)
     * - 第二个字段是 char* 名称 (8 字节，64位)
     * - 第三个字段是 struct 类型节点* 元素类型 (8 字节，64位)
     * - 第四个字段是 long long 指针层级 (8 字节)
     *
     * 在 64 位系统上，指针层级 字段偏移量 = 4 + 4(填充) + 8 + 8 = 24 字节
     */
    
    /* 尝试访问 指针层级 字段 */
    long long *pointer_level_ptr = (long long *)((char *)type + 24);
    if (*pointer_level_ptr > 0) {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查类型是否为指针或类似指针的类型
 *
 * 包括：CN_TYPE_POINTER, CN_TYPE_STRING (char*),
 * 以及指针层级>0的struct 类型节点
 * 用于P6修复中的ADD/SUB指令指针算术类型推断
 */
static bool is_pointer_like_type(CnType *type) {
    if (!type) return false;
    /* CN_TYPE_STRING 是 char*，在C中是指针类型 */
    if (type->kind == CN_TYPE_STRING) return true;
    /* 委托给 is_pointer_type_unified 处理 CN_TYPE_POINTER 和 struct 类型节点 */
    return is_pointer_type_unified(type);
}

/* 运行时宏冲突检测：检测函数名是否与运行时宏定义冲突 */
static bool is_runtime_macro_conflict(const char *func_name) {
    if (!func_name) return false;
    
    /* 运行时宏定义的中文函数名列表（来自 runtime.h） */
    static const char *runtime_macros[] = {
        "初始化运行时", "退出运行时",
        "打印整数", "打印小数", "打印布尔", "打印换行",
        "连接字符串新", "字符串长度", "整数转字符串", "小数转字符串", "布尔转字符串",
        "分配数组", "获取数组长度", "检查数组越界", "释放数组", "设置数组元素", "获取数组元素",
        "取子串", "字符串比较", "查找子串位置", "去除空白", "格式化字符串新",
        NULL
    };
    
    for (int i = 0; runtime_macros[i]; i++) {
        if (strcmp(func_name, runtime_macros[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* 符号名匹配（考虑局部变量名后缀和前缀）
 * 检查两个符号名是否匹配，考虑：
 * 1. 局部变量名的 _数字序号 后缀（例如：cn_var_上下文_0 和 cn_var_上下文）
 * 2. cn_var_ 前缀（例如：cn_var_上下文 和 上下文）
 *
 * 这处理了IR生成器为参数和局部变量添加 cn_var_ 前缀的情况
 */
/**
 * 检查字符串末尾是否是 _数字 后缀
 * 返回去掉后缀后的长度，如果没有数字后缀返回0
 */
static size_t get_base_name_len(const char *name) {
    const char *last_underscore;
    const char *after;
    
    if (!name) return 0;
    
    last_underscore = strrchr(name, '_');
    if (!last_underscore || last_underscore == name) return 0;
    
    after = last_underscore + 1;
    if (*after == '\0') return 0; // 下划线后没有内容
    
    // 检查是否全是数字
    for (const char *p = after; *p; p++) {
        if (*p < '0' || *p > '9') return 0;
    }
    return last_underscore - name; // 返回基础名称的长度
}

/* 【P8修复】ALLOCA变量类型映射表条目 - 移到文件作用域供指令生成使用
 * 原来定义在 cn_cgen_function() 内部的 if 块中，
 * 指令生成函数无法访问。P8修复需要 MEMBER_ACCESS 指令
 * 能查找 alloca_types 来判断变量是否为指针类型。
 */
typedef struct AllocaTypeEntry {
    const char *sym_name;    // 符号名（不拥有内存）
    CnType *type;            // 类型指针
    struct AllocaTypeEntry *next;
} AllocaTypeEntry;

/**
 * 比较两个名称是否匹配（考虑 cn_var_ 前缀和 _数字 后缀）
 *
 * 匹配规则：
 * 1. 精确匹配：name1 == name2
 * 2. 前缀匹配：cn_var_XXX 与 XXX 匹配
 * 3. 后缀匹配：XXX_0 与 XXX 匹配
 * 4. 前缀+后缀匹配：cn_var_XXX_0 与 XXX 匹配，cn_var_XXX_0 与 cn_var_XXX 匹配
 */
static bool names_match_with_suffix(const char *name1, const char *name2) {
    size_t len1, len2;
    size_t base_len1, base_len2;
    const char *cn_var_prefix = "cn_var_";
    size_t cn_prefix_len = 7; // strlen("cn_var_")
    
    if (!name1 || !name2) return false;
    
    // 首先尝试精确匹配
    if (strcmp(name1, name2) == 0) return true;
    
    len1 = strlen(name1);
    len2 = strlen(name2);
    
    // 获取去掉数字后缀后的基础名称长度
    base_len1 = get_base_name_len(name1);
    base_len2 = get_base_name_len(name2);
    
    // 使用有效长度（如果有数字后缀，使用基础长度；否则使用原长度）
    size_t effective_len1 = (base_len1 > 0) ? base_len1 : len1;
    size_t effective_len2 = (base_len2 > 0) ? base_len2 : len2;
    
    // 情况1：去掉数字后缀后精确匹配
    // 例如：cn_var_函数声明_7 与 cn_var_函数声明 匹配
    if (effective_len1 == effective_len2 &&
        strncmp(name1, name2, effective_len1) == 0) {
        return true;
    }
    
    // 【修复】情况2：name1 有 cn_var_ 前缀，name2 没有
    // 需要先检查是否有 cn_var_ 前缀，然后再计算去掉前缀后的有效长度
    bool name1_has_cn_var_prefix = (len1 > cn_prefix_len &&
                                     strncmp(name1, cn_var_prefix, cn_prefix_len) == 0);
    bool name2_has_cn_var_prefix = (len2 > cn_prefix_len &&
                                     strncmp(name2, cn_var_prefix, cn_prefix_len) == 0);
    
    if (name1_has_cn_var_prefix && !name2_has_cn_var_prefix) {
        // name1 有 cn_var_ 前缀，name2 没有
        // 计算去掉前缀后的有效长度
        size_t name1_after_prefix_len = effective_len1 - cn_prefix_len;
        // 如果有数字后缀，需要从去掉前缀后的部分计算
        if (base_len1 > 0) {
            name1_after_prefix_len = base_len1 - cn_prefix_len;
        }
        
        // 检查去掉前缀后是否与 name2 匹配
        if (name1_after_prefix_len == effective_len2 &&
            strncmp(name1 + cn_prefix_len, name2, effective_len2) == 0) {
            return true;
        }
        // 也检查 name2 是否有数字后缀的情况
        if (base_len2 > 0 && name1_after_prefix_len == len2 &&
            strncmp(name1 + cn_prefix_len, name2, len2) == 0) {
            return true;
        }
        // 【新增】检查完整长度匹配（处理没有数字后缀的情况）
        if (name1_after_prefix_len == len2 &&
            strncmp(name1 + cn_prefix_len, name2, len2) == 0) {
            return true;
        }
    }
    
    // 【修复】情况3：name2 有 cn_var_ 前缀，name1 没有
    if (name2_has_cn_var_prefix && !name1_has_cn_var_prefix) {
        // name2 有 cn_var_ 前缀，name1 没有
        size_t name2_after_prefix_len = effective_len2 - cn_prefix_len;
        if (base_len2 > 0) {
            name2_after_prefix_len = base_len2 - cn_prefix_len;
        }
        
        if (name2_after_prefix_len == effective_len1 &&
            strncmp(name2 + cn_prefix_len, name1, effective_len1) == 0) {
            return true;
        }
        if (base_len1 > 0 && name2_after_prefix_len == len1 &&
            strncmp(name2 + cn_prefix_len, name1, len1) == 0) {
            return true;
        }
        // 【新增】检查完整长度匹配（处理没有数字后缀的情况）
        if (name2_after_prefix_len == len1 &&
            strncmp(name2 + cn_prefix_len, name1, len1) == 0) {
            return true;
        }
    }
    
    // 情况4：name1 有数字后缀，name2 没有
    // 例如：cn_var_函数声明_7 与 cn_var_函数声明 匹配
    if (base_len1 > 0 && base_len2 == 0) {
        // 检查基础部分是否与 name2 精确匹配
        if (base_len1 == len2 && strncmp(name1, name2, base_len1) == 0) {
            return true;
        }
    }
    
    // 情况5：name2 有数字后缀，name1 没有
    if (base_len2 > 0 && base_len1 == 0) {
        if (base_len2 == len1 && strncmp(name2, name1, base_len2) == 0) {
            return true;
        }
    }
    
    // 情况6：两个名称都有数字后缀，检查基础部分是否匹配
    if (base_len1 > 0 && base_len2 > 0) {
        if (base_len1 == base_len2 && strncmp(name1, name2, base_len1) == 0) {
            return true;
        }
    }
    
    return false;
}

/* 运行时库函数冲突检测：检测函数名是否与运行时库函数冲突 */
static bool is_runtime_function_conflict(const char *func_name) {
    if (!func_name) return false;
    
    /* 运行时库中已定义的中文函数名列表（来自 stdlib.h） */
    static const char *runtime_functions[] = {
        /* 内存管理函数 */
        "分配内存", "释放内存", "重新分配内存", "分配清零内存",
        /* 内存操作函数 */
        "设置内存", "复制内存", "比较内存",
        /* 字符串函数 */
        "比较字符串", "限长比较字符串", "获取字符串长度",
        "复制字符串", "限长复制字符串", "连接字符串", "限长连接字符串",
        "查找字符", "反向查找字符", "查找子串",
        /* 文件操作函数 */
        "打开文件", "关闭文件", "读取文件", "写入文件",
        "判断文件结束", "文件定位", "获取文件位置", "刷新文件缓冲",
        /* 控制台输入输出函数 */
        "打印字符串", "打印行", "读取整数", "读取小数", "读取字符串", "读取字符",
        "刷新输出", "格式化打印", "格式化字符串", "安全格式化字符串",
        "打印格式",  /* 格式化打印的别名 */
        /* 类型检查函数 */
        "是整数", "是小数", "是字符串", "是数值", "取小数", "释放输入",
        /* 类型转换函数 */
        "转整数", "转小数", "是数字文本", "是整数文本",
        /* 动态数组函数 */
        "创建数组", "销毁数组", "数组添加", "数组长度", "清空数组",
        /* 哈希表函数 */
        "创建哈希表", "销毁哈希表", "哈希表插入", "哈希表获取",
        "哈希表包含", "哈希表大小", "清空哈希表",
        NULL
    };
    
    for (int i = 0; runtime_functions[i]; i++) {
        if (strcmp(func_name, runtime_functions[i]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查表达式是否为字符串类型
 */
static bool cgen_is_string_expr(CnAstExpr *expr) {
    if (!expr) return false;
    
    // 检查表达式本身的类型
    if (expr->type && expr->type->kind == CN_TYPE_STRING) {
        return true;
    }
    
    // 对于字符串字面量，直接返回true
    if (expr->kind == CN_AST_EXPR_STRING_LITERAL) {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查二元表达式是否为字符串拼接
 */
static bool cgen_is_string_concat(CnAstExpr *expr) {
    if (!expr || expr->kind != CN_AST_EXPR_BINARY) return false;
    if (expr->as.binary.op != CN_AST_BINARY_OP_ADD) return false;
    
    // 检查表达式类型
    if (expr->type && expr->type->kind == CN_TYPE_STRING) {
        return true;
    }
    
    // 检查任一操作数是否为字符串
    if (cgen_is_string_expr(expr->as.binary.left) || cgen_is_string_expr(expr->as.binary.right)) {
        return true;
    }
    
    return false;
}

// --- 辅助函数 ---

// 前向声明
static bool is_enum_type_name(const char *name, size_t name_len);
static bool is_type_already_generated(const char *name, size_t name_len);
static void mark_type_as_generated(const char *name, size_t name_len);

/**
 * @brief 获取类型的C类型字符串（内部实现）
 * @param type 类型指针
 * @param is_param 是否是函数参数（用于决定字符串类型是否添加const）
 * @return C类型字符串
 */
static const char *get_c_type_string_internal(CnType *type, bool is_param) {
    if (!type) return "void";
    switch (type->kind) {
        case CN_TYPE_INT:
            // CN语言规范：整数类型建议对应 C 的 long long 以保证 64 位
            return "long long";
        case CN_TYPE_FLOAT: return "double";
        case CN_TYPE_CHAR: return "char";
        case CN_TYPE_BOOL: return "_Bool";
        case CN_TYPE_STRING:
            // 函数参数使用 const char* 以匹配运行时库签名
            // 变量和返回值使用 char* 以支持修改操作
            return is_param ? "const char*" : "char*";
        
        // 新增的数字类型（支持数字字面量后缀）
        case CN_TYPE_INT32: return "int";
        case CN_TYPE_INT64: return "long long";
        case CN_TYPE_UINT32: return "unsigned int";
        case CN_TYPE_UINT64: return "unsigned long long";
        case CN_TYPE_UINT64_LL: return "unsigned long long";
        case CN_TYPE_FLOAT32: return "float";
        case CN_TYPE_FLOAT64: return "double";
        
        case CN_TYPE_POINTER: {
            // 检查是否是函数指针类型
            if (type->as.pointer_to && type->as.pointer_to->kind == CN_TYPE_FUNCTION) {
                // 函数指针类型：返回类型(*)(param1_type, param2_type, ...)
                static _Thread_local char buffer[512];
                CnType *func_type = type->as.pointer_to;
                
                char params_str[256] = {0};
                if (func_type->as.function.param_count == 0) {
                    snprintf(params_str, sizeof(params_str), "void");
                } else {
                    size_t offset = 0;
                    for (size_t i = 0; i < func_type->as.function.param_count; i++) {
                        const char *param_type = get_c_type_string(func_type->as.function.param_types[i]);
                        offset += snprintf(params_str + offset, sizeof(params_str) - offset, "%s", param_type);
                        if (i < func_type->as.function.param_count - 1) {
                            offset += snprintf(params_str + offset, sizeof(params_str) - offset, ", ");
                        }
                    }
                }
                
                snprintf(buffer, sizeof(buffer), "%s(*)(%s)",
                         get_c_type_string(func_type->as.function.return_type),
                         params_str);
                return buffer;
            }
            
            /* 普通指针类型 */
            /* 性能优化：使用线程局部缓冲区避免静态缓冲区竞争 */
            static _Thread_local char buffer[256];
            
            // 【修复】检查指针指向的类型是否有效
            if (!type->as.pointer_to) {
                // 指针指向的类型未知，使用 void*
                snprintf(buffer, sizeof(buffer), "void*");
                return buffer;
            }
            
            // 【修复】对于结构体指针类型，确保生成正确的类型字符串
            // 例如：作用域* 应该生成 "struct 作用域*"
            CnType *pointee_type = type->as.pointer_to;
            const char *pointee_c_type = get_c_type_string(pointee_type);
            snprintf(buffer, sizeof(buffer), "%s*", pointee_c_type);
            return buffer;
        }
        case CN_TYPE_VOID: return "void";
        case CN_TYPE_ARRAY: {
            /* 数组类型：生成元素指针类型 */
            /* 对于结构体字段，动态数组（长度为0）生成指针类型 */
            /* 固定大小数组也生成指针类型，因为C结构体不支持变长数组字段 */
            static _Thread_local char buffer[256];
            
            // 【修复】检查元素类型是否有效
            if (!type->as.array.element_type) {
                // 元素类型未知，使用 void* 作为默认
                snprintf(buffer, sizeof(buffer), "void*");
                return buffer;
            }
            
            // 【修复】对于结构体指针数组（如 作用域*[]），生成正确的类型
            // 数组类型在C代码中应该生成元素指针类型
            // 例如：作用域*[] 应该生成 struct 作用域**
            // 因为数组字段在C中被转换为指针
            CnType *elem_type = type->as.array.element_type;
            
            // 【关键修复】直接生成元素指针类型
            // 对于 作用域*[]，元素类型是 作用域*（指针类型）
            // get_c_type_string(作用域*) 返回 "struct 作用域*"
            // 我们需要再添加一个 *，得到 "struct 作用域**"
            const char *elem_c_type = get_c_type_string(elem_type);
            
            // 【关键修复】检查生成的类型字符串是否包含结构体定义
            // 如果 elem_c_type 包含 "{" 或 ";"，说明它包含了结构体定义，这是错误的
            // 我们需要只使用结构体名称
            if (strstr(elem_c_type, "{") != NULL || strstr(elem_c_type, ";") != NULL) {
                // 类型字符串包含了结构体定义，这是错误的
                // 使用 void* 作为默认
                snprintf(buffer, sizeof(buffer), "void*");
                return buffer;
            }
            
            snprintf(buffer, sizeof(buffer), "%s*", elem_c_type);
            return buffer;
        }
        case CN_TYPE_STRUCT: {
            /* 结构体类型：检查是否是局部结构体 */
            static _Thread_local char buffer[512];
            
            // 【修复】检查结构体名称是否有效
            if (!type->as.struct_type.name || type->as.struct_type.name_length == 0) {
                // 结构体名称未知，使用匿名结构体
                snprintf(buffer, sizeof(buffer), "struct");
                return buffer;
            }
            
            // 【P0-1修复】检查是否是CN基本类型关键字被错误标记为结构体
            // 自举编译时，语义分析器可能将 整数/字符串/布尔 等基本类型
            // 错误创建为 CN_TYPE_STRUCT 类型，需要在此翻译为正确的C类型
            {
                const char *cn_prim = try_cn_primitive_type_name(
                    type->as.struct_type.name, type->as.struct_type.name_length);
                if (cn_prim) {
                    return cn_prim;
                }
            }
            
            // 【P0-3a修复】防御性检查：检测类型名是否腐败
            // 如果类型名包含结构体定义内容（含 {; \n } 等字符），
            // 说明 struct_type.name 指向了结构体定义的源文本而非仅类型名，
            // 类型信息已损坏，应回退到 void*
            {
                const char *_name = type->as.struct_type.name;
                int _name_len = (int)type->as.struct_type.name_length;
                int _corrupted = 0;
                for (int _i = 0; _i < _name_len; _i++) {
                    if (_name[_i] == '{' || _name[_i] == ';' ||
                        _name[_i] == '\n' || _name[_i] == '}') {
                        _corrupted = 1;
                        break;
                    }
                }
                if (_corrupted) {
                    // 类型信息损坏，回退到 void*
                    return "void*";
                }
            }
            
            // 【关键修复】只生成结构体类型名称，不输出结构体定义
            // 无论结构体是否有字段信息，都只返回类型名称
            // 结构体定义应该在文件开头输出，而不是在类型转换表达式中
            
            // 检查是否是局部结构体（通过owner_func_name判断）
            // 注意：不再依赖decl_scope指针，因为它可能指向已释放的内存
            if (type->as.struct_type.owner_func_name) {
                // 局部结构体：生成 __local_函数名_结构体名
                snprintf(buffer, sizeof(buffer), "struct __local_%.*s_%.*s",
                         (int)type->as.struct_type.owner_func_name_length,
                         type->as.struct_type.owner_func_name,
                         (int)type->as.struct_type.name_length,
                         type->as.struct_type.name);
            } else {
                // 全局/模块结构体
                // 检查是否是枚举类型名称（语义分析可能将枚举错误标记为结构体）
                if (is_enum_type_name(type->as.struct_type.name, type->as.struct_type.name_length)) {
                    snprintf(buffer, sizeof(buffer), "enum %.*s",
                             (int)type->as.struct_type.name_length,
                             type->as.struct_type.name);
                } else {
                    snprintf(buffer, sizeof(buffer), "struct %.*s",
                             (int)type->as.struct_type.name_length,
                             type->as.struct_type.name);
                }
            }
            return buffer;
        }
        case CN_TYPE_ENUM: {
            /* 枚举类型：返回枚举名 */
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "enum %.*s",
                     (int)type->as.enum_type.name_length,
                     type->as.enum_type.name);
            return buffer;
        }
        case CN_TYPE_CLASS: {
            /* 类类型：返回类名 */
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "struct %.*s*",
                     (int)type->as.struct_type.name_length,
                     type->as.struct_type.name);
            return buffer;
        }
        case CN_TYPE_INTERFACE: {
            /* 接口类型：返回接口指针 */
            return "void*";
        }
        case CN_TYPE_MEMORY_ADDRESS: {
            /* 内存地址类型：对应 uintptr_t */
            return "uintptr_t";
        }
        case CN_TYPE_SELF:
            /* 自身类型：接口方法中使用 void* 作为占位符 */
            return "void*";
        case CN_TYPE_PARAM:
            /* 类型参数：使用 void* 作为占位符 */
            return "void*";
        default: return "int";
    }
}

/**
 * @brief 获取类型的C类型字符串（公共接口）
 * @param type 类型指针
 * @return C类型字符串（默认不添加const修饰符）
 */
const char *get_c_type_string(CnType *type) {
    return get_c_type_string_internal(type, false);
}

/**
 * @brief 获取函数参数类型的C类型字符串
 * @param type 类型指针
 * @return C类型字符串（字符串类型添加const修饰符）
 */
const char *get_c_param_type_string(CnType *type) {
    return get_c_type_string_internal(type, true);
}

/**
 * 【P5修复】将CnType转换为extern声明安全的C类型字符串
 * 对于结构体/指针/枚举等可能未在当前模块定义的类型，使用void*替代
 * 避免生成不完整类型导致的GCC错误
 * is_param: true时字符串类型使用const char*（参数位置），false时使用char*（返回值位置）
 */
static const char *get_extern_type_string(CnType *type, bool is_param) {
    if (!type) return is_param ? "void*" : "void*";
    switch (type->kind) {
        case CN_TYPE_INT:
        case CN_TYPE_INT32:
        case CN_TYPE_INT64:
        case CN_TYPE_UINT32:
        case CN_TYPE_UINT64:
        case CN_TYPE_UINT64_LL:
            return "long long";
        case CN_TYPE_FLOAT:
        case CN_TYPE_FLOAT32:
        case CN_TYPE_FLOAT64:
            return "double";
        case CN_TYPE_CHAR: return "char";
        case CN_TYPE_BOOL: return "_Bool";
        case CN_TYPE_STRING: return is_param ? "const char*" : "char*";
        case CN_TYPE_VOID: return "void";
        /* 指针、结构体、枚举、类、接口等使用void*避免不完整类型错误 */
        case CN_TYPE_POINTER:
        case CN_TYPE_STRUCT:
        case CN_TYPE_ENUM:
        case CN_TYPE_CLASS:
        case CN_TYPE_INTERFACE:
        case CN_TYPE_ARRAY:
        case CN_TYPE_MEMORY_ADDRESS:
        case CN_TYPE_SELF:
        case CN_TYPE_PARAM:
            return "void*";
        default: return "long long";
    }
}

static const char *get_c_function_name(const char *name) {
    if (!name) return "unnamed_func";
    
    /* 性能优化：提前返回已知的运行时函数名 */
    if (strncmp(name, "cn_rt_", 6) == 0) {
        return name;
    }
    
    /* 性能优化：使用查找表加速常用中文函数名映射 */
    static const struct {
        const char *utf8_name;
        const char *c_name;
    } function_map[] = {
        {"\xe6\x89\x93\xe5\x8d\xb0", "cn_rt_print_string"},  /* 打印 */
        {"\xe9\x95\xbf\xe5\xba\xa6", "cn_rt_string_length"}, /* 长度 */
        {"\xe4\xb8\xbb\xe7\xa8\x8b\xe5\xba\x8f", "main"},      /* 主程序 */
        /* 输入函数映射 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe8\xa1\x8c", "cn_rt_read_line"},         /* 读取行 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_read_int"},     /* 读取整数 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_read_float"},   /* 读取小数 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe5\xad\x97\xe7\xac\xa6\xe4\xb8\xb2", "cn_rt_read_string"}, /* 读取字符串 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe5\xad\x97\xe7\xac\xa6", "cn_rt_read_char"},   /* 读取字符 */
        /* 通用读取函数映射 */
        {"\xe8\xaf\xbb\xe5\x8f\x96", "cn_rt_read"},           /* 读取 */
        {"\xe6\x98\xaf\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_input_is_int"},     /* 是整数 */
        {"\xe6\x98\xaf\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_input_is_float"},   /* 是小数 */
        {"\xe6\x98\xaf\xe5\xad\x97\xe7\xac\xa6\xe4\xb8\xb2", "cn_rt_input_is_string"}, /* 是字符串 */
        {"\xe6\x98\xaf\xe6\x95\xb0\xe5\x80\xbc", "cn_rt_input_is_number"},  /* 是数值 */
        {"\xe5\x8f\x96\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_input_to_int"},     /* 取整数 */
        {"\xe5\x8f\x96\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_input_to_float"},   /* 取小数 */
        {"\xe5\x8f\x96\xe6\x96\x87\xe6\x9c\xac", "cn_rt_input_to_string"},  /* 取文本 */
        {"\xe9\x87\x8a\xe6\x94\xbe\xe8\xbe\x93\xe5\x85\xa5", "cn_rt_input_free"},  /* 释放输入 */
        /* 字符串转换函数映射 */
        {"\xe8\xbd\xac\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_str_to_int"},      /* 转整数 */
        {"\xe8\xbd\xac\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_str_to_float"},    /* 转小数 */
        {"\xe6\x98\xaf\xe6\x95\xb0\xe5\xad\x97\xe6\x96\x87\xe6\x9c\xac", "cn_rt_is_numeric_str"},  /* 是数字文本 */
        {"\xe6\x98\xaf\xe6\x95\xb4\xe6\x95\xb0\xe6\x96\x87\xe6\x9c\xac", "cn_rt_is_int_str"},      /* 是整数文本 */
        {NULL, NULL}
    };
    
    for (int i = 0; function_map[i].utf8_name; i++) {
        if (strcmp(name, function_map[i].utf8_name) == 0) {
            return function_map[i].c_name;
        }
    }
    
    /* 性能优化：使用线程局部缓冲区 */
    static _Thread_local char buffer[256];
    /* 方案C：未映射的中文函数直接使用原名称，不添加 cn_func_ 前缀 */
    /* 这样 stdlib.h 中定义的中文函数（如 分配内存、释放内存）可以直接被调用 */
    snprintf(buffer, sizeof(buffer), "%s", name);
    return buffer;
}

// 获取静态变量的C名称
static const char *get_static_var_name(const char *func_name, const char *var_name) {
    static _Thread_local char buffer[256];
    snprintf(buffer, sizeof(buffer), "cn_static_%s_%s", func_name, var_name);
    return buffer;
}

static const char *get_c_variable_name(const char *name) {
    /* 性能优化：使用线程局部缓冲区 */
    static _Thread_local char buffer[256];
    
    // 检查是否已经有 cn_module_、cn_var_ 或 cn_static_ 前缀（避免重复加前缀）
    if (strncmp(name, "cn_module_", 10) == 0 ||
        strncmp(name, "cn_var_", 7) == 0 ||
        strncmp(name, "cn_static_", 10) == 0) {
        return name;
    }
    
    // 检查是否为模块成员名（包含双下划线 "__"）
    const char *delimiter = strstr(name, "__");
    if (delimiter) {
        // 模块成员名：模块名__成员名 -> cn_module_模块名__成员名
        snprintf(buffer, sizeof(buffer), "cn_module_%s", name);
    } else {
        // 普通变量名
        snprintf(buffer, sizeof(buffer), "cn_var_%s", name);
    }
    return buffer;
}

// 前向声明
static void cn_cgen_expr_simple(CnCCodeGenContext *ctx, CnAstExpr *expr);

static void print_operand(CnCCodeGenContext *ctx, CnIrOperand op) {
    switch (op.kind) {
        case CN_IR_OP_NONE: fprintf(ctx->output_file, "/* NONE */"); break;
        case CN_IR_OP_REG: fprintf(ctx->output_file, "r%d", op.as.reg_id); break;
        case CN_IR_OP_IMM_INT: fprintf(ctx->output_file, "%lld", op.as.imm_int); break;
        case CN_IR_OP_IMM_FLOAT: fprintf(ctx->output_file, "%f", op.as.imm_float); break;
        case CN_IR_OP_IMM_STR:
            // 字符串字面量：需要加引号并处理转义
            fprintf(ctx->output_file, "\"");
            for (const char *p = op.as.imm_str; p && *p; p++) {
                switch (*p) {
                    case '\\': fprintf(ctx->output_file, "\\\\"); break;
                    case '\"': fprintf(ctx->output_file, "\\\""); break;
                    case '\n': fprintf(ctx->output_file, "\\n"); break;
                    case '\r': fprintf(ctx->output_file, "\\r"); break;
                    case '\t': fprintf(ctx->output_file, "\\t"); break;
                    default: fprintf(ctx->output_file, "%c", *p); break;
                }
            }
            fprintf(ctx->output_file, "\"");
            break;
        case CN_IR_OP_SYMBOL:
            // 检查是否为枚举成员符号
            // 枚举成员符号名格式为 "枚举类型名_成员名"，不需要添加 cn_var_ 前缀
            // 【调试】输出符号信息
            
            // 【修复】检查是否为局部变量名格式（原名_数字序号）
            // 局部变量名格式为 "原名_序号"，序号是数字
            // 枚举成员名格式为 "枚举类型名_成员名"，成员名不是纯数字
            bool is_local_var_by_name = false;
            if (op.as.sym_name && strchr(op.as.sym_name, '_') != NULL) {
                const char *last_underscore = strrchr(op.as.sym_name, '_');
                if (last_underscore) {
                    const char *after_underscore = last_underscore + 1;
                    bool is_number = true;
                    for (const char *p = after_underscore; *p; p++) {
                        if (*p < '0' || *p > '9') {
                            is_number = false;
                            break;
                        }
                    }
                    // 如果下划线后面是纯数字，则是局部变量名
                    if (is_number && strlen(after_underscore) > 0) {
                        is_local_var_by_name = true;
                    }
                }
            }
            
            // 只有当类型是枚举且不是局部变量名格式时，才当作枚举成员处理
            if (op.type && op.type->kind == CN_TYPE_ENUM && !is_local_var_by_name) {
                // 枚举成员：直接输出符号名
                fprintf(ctx->output_file, "%s", op.as.sym_name);
            } else {
                // 【调试】检查符号名是否包含下划线（可能是枚举成员但类型信息丢失）
                // 枚举成员名格式为 "枚举类型名_成员名"
                bool is_enum_member_by_name = false;
                if (op.as.sym_name && strchr(op.as.sym_name, '_') != NULL) {
                    // 符号名包含下划线，可能是枚举成员
                    // 检查是否在全局作用域中作为枚举成员存在
                    if (ctx->global_scope) {
                        CnSemSymbol *sym = cn_sem_scope_lookup(ctx->global_scope, op.as.sym_name, strlen(op.as.sym_name));
                        if (sym && sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            is_enum_member_by_name = true;
                            }
                    }
                    
                    // 【修复】检查符号名是否符合枚举成员命名模式
                    // 枚举成员符号名格式为 "枚举类型名_成员名"
                    // 局部变量名格式为 "原名_序号"（序号是数字）
                    // 关键区别：枚举成员的下划线后面不是数字，局部变量的下划线后面是数字
                    if (!is_enum_member_by_name) {
                        size_t name_len = strlen(op.as.sym_name);
                        if (name_len > 3) {
                            // 查找最后一个下划线的位置
                            const char *last_underscore = strrchr(op.as.sym_name, '_');
                            if (last_underscore) {
                                // 检查下划线后面是否是数字
                                const char *after_underscore = last_underscore + 1;
                                bool is_number_after_underscore = true;
                                for (const char *p = after_underscore; *p; p++) {
                                    if (*p < '0' || *p > '9') {
                                        is_number_after_underscore = false;
                                        break;
                                    }
                                }
                                
                                // 如果下划线后面是数字，则是局部变量，不是枚举成员
                                if (is_number_after_underscore && strlen(after_underscore) > 0) {
                                    // 这是局部变量名格式（如 "名称长度_0"），不是枚举成员
                                    is_enum_member_by_name = false;
                                } else {
                                    // 下划线后面不是数字，检查是否包含中文
                                    bool has_chinese = false;
                                    for (size_t i = 0; i < name_len; i++) {
                                        if ((unsigned char)op.as.sym_name[i] > 127) {
                                            has_chinese = true;
                                            break;
                                        }
                                    }
                                    // 如果包含中文且下划线后面不是数字，可能是枚举成员
                                    if (has_chinese) {
                                        is_enum_member_by_name = true;
                                        }
                                }
                            }
                        }
                    }
                }
                
                if (is_enum_member_by_name) {
                    // 枚举成员：直接输出符号名
                    fprintf(ctx->output_file, "%s", op.as.sym_name);
                } else {
                    // 普通变量：添加 cn_var_ 前缀
                    fprintf(ctx->output_file, "%s", get_c_variable_name(op.as.sym_name));
                }
            }
            break;
        case CN_IR_OP_LABEL: fprintf(ctx->output_file, "%s", op.as.label->name ? op.as.label->name : "unnamed_label"); break;
        case CN_IR_OP_AST_EXPR:
            // AST表达式：直接生成 C 代码（用于结构体字面量等）
            cn_cgen_expr_simple(ctx, op.as.ast_expr);
            break;
        default: fprintf(ctx->output_file, "/* UNKNOWN */"); break;
    }
}

// 生成简单表达式的 C 代码（用于模块变量初始化）
static void cn_cgen_expr_simple(CnCCodeGenContext *ctx, CnAstExpr *expr) {
    if (!ctx || !expr) return;
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(ctx->output_file, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(ctx->output_file, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            // 输出字符串字面量，需要处理转义
            fprintf(ctx->output_file, "\"");
            for (const char *p = expr->as.string_literal.value; *p; p++) {
                switch (*p) {
                    case '\\': fprintf(ctx->output_file, "\\\\"); break;
                    case '\"': fprintf(ctx->output_file, "\\\""); break;
                    case '\n': fprintf(ctx->output_file, "\\n"); break;
                    case '\r': fprintf(ctx->output_file, "\\r"); break;
                    case '\t': fprintf(ctx->output_file, "\\t"); break;
                    default: fprintf(ctx->output_file, "%c", *p); break;
                }
            }
            fprintf(ctx->output_file, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(ctx->output_file, "%s", expr->as.bool_literal.value ? "true" : "false");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            // 检查是否为自身指针（this/self）
            if (expr->is_this_pointer) {
                // 自身指针直接输出 "self"
                fprintf(ctx->output_file, "self");
            } else if (expr->is_base_pointer) {
                // 基类指针：输出 "base" 标识符
                fprintf(ctx->output_file, "base");
            } else {
                // 检查是否为枚举成员引用
                // 枚举成员的类型是 CN_TYPE_ENUM，可以通过 expr->type 获取枚举类型信息
                bool is_enum_member = false;
                if (expr->type && expr->type->kind == CN_TYPE_ENUM) {
                    // 这是一个枚举成员引用
                    // 生成格式：枚举类型名_成员名
                    const char *enum_name = expr->type->as.enum_type.name;
                    size_t enum_name_len = expr->type->as.enum_type.name_length;
                    const char *member_name = expr->as.identifier.name;
                    size_t member_name_len = expr->as.identifier.name_length;
                    
                    fprintf(ctx->output_file, "%.*s_%.*s",
                            (int)enum_name_len, enum_name,
                            (int)member_name_len, member_name);
                    is_enum_member = true;
                }
                
                // 如果不是枚举成员，检查全局作用域中是否有同名的枚举成员符号
                if (!is_enum_member && ctx->global_scope) {
                    CnSemSymbol *sym = cn_sem_scope_lookup(ctx->global_scope,
                                                           expr->as.identifier.name,
                                                           expr->as.identifier.name_length);
                    if (sym && sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                        // 找到枚举成员符号
                        // 枚举成员的符号名格式为 "枚举类型名_成员名"
                        // 直接使用符号名作为枚举值引用
                        fprintf(ctx->output_file, "%.*s",
                                (int)sym->name_length, sym->name);
                        is_enum_member = true;
                    }
                }
                
                // 如果不是枚举成员，检查是否为函数名
                if (!is_enum_member && ctx->global_scope) {
                    CnSemSymbol *sym = cn_sem_scope_lookup(ctx->global_scope,
                                                           expr->as.identifier.name,
                                                           expr->as.identifier.name_length);
                    if (sym && sym->kind == CN_SEM_SYMBOL_FUNCTION) {
                        // 函数名：直接输出函数名，不添加 cn_var_ 前缀
                        fprintf(ctx->output_file, "%.*s",
                                (int)expr->as.identifier.name_length,
                                expr->as.identifier.name);
                        break;
                    }
                }
                
                // 如果不是枚举成员也不是函数名，按普通变量处理
                if (!is_enum_member) {
                    // 支持引用其他模块变量
                    // 先简单处理：直接输出变量名（需要结合符号表处理模块前缀）
                    fprintf(ctx->output_file, "cn_var_%.*s",
                            (int)expr->as.identifier.name_length,
                            expr->as.identifier.name);
                }
            }
            break;
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 成员访问表达式：obj.member 或 ptr->member 或 类名.静态成员 或 基类.成员 或 枚举类型.成员
            {
                // 【新增】检查是否为枚举成员访问（枚举类型.成员名）
                // 枚举成员访问的对象类型是枚举类型
                if (expr->as.member.object && expr->as.member.object->type) {
                    CnType *obj_type = expr->as.member.object->type;
                    if (obj_type->kind == CN_TYPE_ENUM) {
                        // 枚举成员访问：生成 枚举类型名_成员名
                        const char *enum_name = obj_type->as.enum_type.name;
                        size_t enum_name_len = obj_type->as.enum_type.name_length;
                        fprintf(ctx->output_file, "%.*s_%.*s",
                                (int)enum_name_len, enum_name,
                                (int)expr->as.member.member_name_length,
                                expr->as.member.member_name);
                        break;  // 枚举成员访问处理完成
                    }
                }
                
                // 检查是否为静态成员访问
                if (expr->as.member.is_static_member && expr->as.member.class_name) {
                    // 静态成员访问：生成 类名_成员名
                    fprintf(ctx->output_file, "%.*s_%.*s",
                            (int)expr->as.member.class_name_length, expr->as.member.class_name,
                            (int)expr->as.member.member_name_length, expr->as.member.member_name);
                }
                // 检查对象是否为自身指针
                else if (expr->as.member.object &&
                    expr->as.member.object->is_this_pointer) {
                    // 自身.成员 -> self->成员
                    fprintf(ctx->output_file, "self->%.*s",
                            (int)expr->as.member.member_name_length,
                            expr->as.member.member_name);
                }
                // 检查对象是否为基类指针
                else if (expr->as.member.object &&
                    expr->as.member.object->is_base_pointer) {
                    // 基类访问：base->member
                    // 生成基类指针转换代码
                    if (ctx->current_class && ctx->current_class->base_count > 0) {
                        CnInheritanceInfo *base_info = &ctx->current_class->bases[0];
                        fprintf(ctx->output_file, "((struct %.*s*)((char*)self + %.*s_%.*s_OFFSET))->%.*s",
                                (int)base_info->base_class_name_length, base_info->base_class_name,
                                (int)ctx->current_class->name_length, ctx->current_class->name,
                                (int)base_info->base_class_name_length, base_info->base_class_name,
                                (int)expr->as.member.member_name_length,
                                expr->as.member.member_name);
                    } else {
                        // 没有基类，生成错误占位符
                        fprintf(ctx->output_file, "/* 错误: 当前类没有基类 */ %.*s",
                                (int)expr->as.member.member_name_length,
                                expr->as.member.member_name);
                    }
                } else {
                    // 普通成员访问
                    cn_cgen_expr_simple(ctx, expr->as.member.object);
                    
                    // 【修改点】根据对象类型判断使用 "." 还是 "->"
                    // CN语言中，指针类型的成员访问使用 "." 语法，但生成C代码时需要转换为 "->"
                    bool use_arrow = expr->as.member.is_arrow;  // 显式使用箭头语法
                    
                    // 【新增】检查对象类型是否为指针
                    if (!use_arrow && expr->as.member.object && expr->as.member.object->type) {
                        CnType *obj_type = expr->as.member.object->type;
                        if (obj_type->kind == CN_TYPE_POINTER) {
                            use_arrow = true;  // 指针类型自动使用箭头
                        }
                    }
                    
                    // 【新增】如果对象类型信息缺失，尝试从符号表查找
                    // 这处理了函数参数类型信息丢失的情况
                    if (!use_arrow && expr->as.member.object &&
                        expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                        // 尝试从全局作用域查找变量类型
                        if (ctx->global_scope) {
                            CnSemSymbol *sym = cn_sem_scope_lookup(ctx->global_scope,
                                expr->as.member.object->as.identifier.name,
                                expr->as.member.object->as.identifier.name_length);
                            if (sym && sym->type && sym->type->kind == CN_TYPE_POINTER) {
                                use_arrow = true;
                            }
                        }
                        // 尝试从当前函数参数查找
                        if (!use_arrow && ctx->current_func && ctx->current_func->params) {
                            const char *obj_name = expr->as.member.object->as.identifier.name;
                            size_t obj_name_len = expr->as.member.object->as.identifier.name_length;
                            for (size_t p = 0; p < ctx->current_func->param_count && !use_arrow; p++) {
                                CnIrOperand *param = &ctx->current_func->params[p];
                                if (param->kind == CN_IR_OP_SYMBOL && param->as.sym_name) {
                                    // 检查参数名是否匹配（考虑 cn_var_ 前缀）
                                    const char *param_name = param->as.sym_name;
                                    size_t param_name_len = strlen(param_name);
                                    bool matches = false;
                                    // 直接匹配
                                    if (param_name_len == obj_name_len &&
                                        strncmp(param_name, obj_name, obj_name_len) == 0) {
                                        matches = true;
                                    }
                                    // cn_var_ 前缀匹配
                                    else if (param_name_len > 7 &&
                                             strncmp(param_name, "cn_var_", 7) == 0 &&
                                             param_name_len - 7 == obj_name_len &&
                                             strncmp(param_name + 7, obj_name, obj_name_len) == 0) {
                                        matches = true;
                                    }
                                    if (matches && param->type && param->type->kind == CN_TYPE_POINTER) {
                                        use_arrow = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (use_arrow) {
                        fprintf(ctx->output_file, "->");
                    } else {
                        fprintf(ctx->output_file, ".");
                    }
                    
                    fprintf(ctx->output_file, "%.*s",
                            (int)expr->as.member.member_name_length,
                            expr->as.member.member_name);
                }
            }
            break;
        case CN_AST_EXPR_BINARY:
            // 检查是否为字符串拼接
            if (cgen_is_string_concat(expr)) {
                fprintf(ctx->output_file, "cn_rt_string_concat(");
                cn_cgen_expr_simple(ctx, expr->as.binary.left);
                fprintf(ctx->output_file, ", ");
                cn_cgen_expr_simple(ctx, expr->as.binary.right);
                fprintf(ctx->output_file, ")");
                break;
            }
            fprintf(ctx->output_file, "(");
            cn_cgen_expr_simple(ctx, expr->as.binary.left);
            
            // 生成运算符
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(ctx->output_file, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(ctx->output_file, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(ctx->output_file, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(ctx->output_file, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(ctx->output_file, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(ctx->output_file, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(ctx->output_file, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(ctx->output_file, " < "); break;
                case CN_AST_BINARY_OP_GT: fprintf(ctx->output_file, " > "); break;
                case CN_AST_BINARY_OP_LE: fprintf(ctx->output_file, " <= "); break;
                case CN_AST_BINARY_OP_GE: fprintf(ctx->output_file, " >= "); break;
                case CN_AST_BINARY_OP_BITWISE_AND: fprintf(ctx->output_file, " & "); break;
                case CN_AST_BINARY_OP_BITWISE_OR: fprintf(ctx->output_file, " | "); break;
                case CN_AST_BINARY_OP_BITWISE_XOR: fprintf(ctx->output_file, " ^ "); break;
                case CN_AST_BINARY_OP_LEFT_SHIFT: fprintf(ctx->output_file, " << "); break;
                case CN_AST_BINARY_OP_RIGHT_SHIFT: fprintf(ctx->output_file, " >> "); break;
                default: fprintf(ctx->output_file, " ? "); break;
            }
            
            cn_cgen_expr_simple(ctx, expr->as.binary.right);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_UNARY:
            // 一元运算
            switch (expr->as.unary.op) {
                case CN_AST_UNARY_OP_MINUS: fprintf(ctx->output_file, "-"); break;
                case CN_AST_UNARY_OP_NOT: fprintf(ctx->output_file, "!"); break;
                case CN_AST_UNARY_OP_BITWISE_NOT: fprintf(ctx->output_file, "~"); break;
                case CN_AST_UNARY_OP_PRE_INC:
                    fprintf(ctx->output_file, "++");
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    return;
                case CN_AST_UNARY_OP_PRE_DEC:
                    fprintf(ctx->output_file, "--");
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    return;
                case CN_AST_UNARY_OP_POST_INC:
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    fprintf(ctx->output_file, "++");
                    return;
                case CN_AST_UNARY_OP_POST_DEC:
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    fprintf(ctx->output_file, "--");
                    return;
                default: break;
            }
            cn_cgen_expr_simple(ctx, expr->as.unary.operand);
            break;
        case CN_AST_EXPR_TERNARY:
            // 三元运算符: condition ? true_expr : false_expr
            fprintf(ctx->output_file, "(");
            cn_cgen_expr_simple(ctx, expr->as.ternary.condition);
            fprintf(ctx->output_file, " ? ");
            cn_cgen_expr_simple(ctx, expr->as.ternary.true_expr);
            fprintf(ctx->output_file, " : ");
            cn_cgen_expr_simple(ctx, expr->as.ternary.false_expr);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_MEMORY_READ:
            // 读取内存: 读取内存(地址) -> *(uintptr_t*)addr
            fprintf(ctx->output_file, "*(uintptr_t*)");
            cn_cgen_expr_simple(ctx, expr->as.memory_read.address);
            break;
        case CN_AST_EXPR_MEMORY_WRITE:
            // 写入内存: 写入内存(地址, 值) -> *(uintptr_t*)addr = value
            fprintf(ctx->output_file, "*(uintptr_t*)");
            cn_cgen_expr_simple(ctx, expr->as.memory_write.address);
            fprintf(ctx->output_file, " = ");
            cn_cgen_expr_simple(ctx, expr->as.memory_write.value);
            break;
        case CN_AST_EXPR_MEMORY_COPY:
            // 内存复制: 内存复制(目标, 源, 大小) -> memcpy(dest, src, size)
            fprintf(ctx->output_file, "memcpy(");
            cn_cgen_expr_simple(ctx, expr->as.memory_copy.dest);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_copy.src);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_copy.size);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_MEMORY_SET:
            // 内存设置: 内存设置(地址, 值, 大小) -> memset(addr, value, size)
            fprintf(ctx->output_file, "memset(");
            cn_cgen_expr_simple(ctx, expr->as.memory_set.address);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_set.value);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_set.size);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_MEMORY_MAP:
            // 内存映射: 映射内存(地址, 大小, 保护, 标志) -> mmap(addr, size, prot, flags, -1, 0)
            fprintf(ctx->output_file, "mmap(");
            if (expr->as.memory_map.address) {
                cn_cgen_expr_simple(ctx, expr->as.memory_map.address);
            } else {
                fprintf(ctx->output_file, "NULL");
            }
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_map.size);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_map.prot);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_map.flags);
            fprintf(ctx->output_file, ", -1, 0)");
            break;
        case CN_AST_EXPR_MEMORY_UNMAP:
            // 解除内存映射: 解除映射(地址, 大小) -> munmap(addr, size)
            fprintf(ctx->output_file, "munmap(");
            cn_cgen_expr_simple(ctx, expr->as.memory_unmap.address);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_unmap.size);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_INLINE_ASM:
            // 内联汇编: inline_asm("code", outputs, inputs, clobbers)
            fprintf(ctx->output_file, "asm volatile (");
            cn_cgen_expr_simple(ctx, expr->as.inline_asm.asm_code);
            fprintf(ctx->output_file, "\n");
            
            // 输出操作数
            if (expr->as.inline_asm.outputs && expr->as.inline_asm.output_count > 0) {
                fprintf(ctx->output_file, "    : ");
                for (size_t i = 0; i < expr->as.inline_asm.output_count; i++) {
                    if (i > 0) {
                        fprintf(ctx->output_file, ", ");
                    }
                    fprintf(ctx->output_file, "\"=r\"(");
                    cn_cgen_expr_simple(ctx, expr->as.inline_asm.outputs[i]);
                    fprintf(ctx->output_file, ")");
                }
                fprintf(ctx->output_file, "\n");
            }
            
            // 输入操作数
            if (expr->as.inline_asm.inputs && expr->as.inline_asm.input_count > 0) {
                fprintf(ctx->output_file, "    : ");
                for (size_t i = 0; i < expr->as.inline_asm.input_count; i++) {
                    if (i > 0) {
                        fprintf(ctx->output_file, ", ");
                    }
                    fprintf(ctx->output_file, "\"r\"(");
                    cn_cgen_expr_simple(ctx, expr->as.inline_asm.inputs[i]);
                    fprintf(ctx->output_file, ")");
                }
                fprintf(ctx->output_file, "\n");
            }
            
            // 破坏列表
            if (expr->as.inline_asm.clobbers) {
                fprintf(ctx->output_file, "    : ");
                cn_cgen_expr_simple(ctx, expr->as.inline_asm.clobbers);
                fprintf(ctx->output_file, "\n");
            }
            
            fprintf(ctx->output_file, ");");
            break;
        case CN_AST_EXPR_STRUCT_LITERAL:
            // 结构体字面量：点 { x: 10, y: 20 } -> (struct 点){.x = 10, .y = 20}
            // 使用expr->type中的类型信息来决定如何生成
            {
                if (expr->type && expr->type->kind == CN_TYPE_STRUCT) {
                    // 使用get_c_type_string来生成结构体类型名（已经考虑了局部/全局区分）
                    const char *c_type = get_c_type_string(expr->type);
                    fprintf(ctx->output_file, "(%s){", c_type);
                } else {
                    // fallback: 使用结构体名
                    fprintf(ctx->output_file, "(struct %.*s){",
                            (int)expr->as.struct_lit.struct_name_length,
                            expr->as.struct_lit.struct_name);
                }
                for (size_t i = 0; i < expr->as.struct_lit.field_count; i++) {
                    if (i > 0) {
                        fprintf(ctx->output_file, ", ");
                    }
                    fprintf(ctx->output_file, ".%.*s = ",
                            (int)expr->as.struct_lit.fields[i].field_name_length,
                            expr->as.struct_lit.fields[i].field_name);
                    cn_cgen_expr_simple(ctx, expr->as.struct_lit.fields[i].value);
                }
                fprintf(ctx->output_file, "}");
            }
            break;
        case CN_AST_EXPR_CAST:
            // 类型转换表达式：(目标类型)操作数
            {
                fprintf(ctx->output_file, "(");
                if (expr->as.cast.target_type) {
                    // 【修复】对于数组类型，直接使用元素指针类型
                    // 例如：作用域*[] 应该转换为 struct 作用域**
                    // 因为数组字段在C中被转换为指针
                    const char *c_type = get_c_type_string(expr->as.cast.target_type);
                    
                    // 【关键修复】检查类型字符串是否包含结构体定义
                    // 如果包含 "{" 或 ";" 或 "子作用域列表" 等字段名，说明类型字符串错误
                    // 这种情况下，使用 void* 替代
                    if (strstr(c_type, "{") != NULL ||
                        strstr(c_type, ";") != NULL ||
                        strstr(c_type, "子作用域列表") != NULL ||
                        strstr(c_type, "子作用域数量") != NULL ||
                        strstr(c_type, "子作用域容量") != NULL ||
                        strstr(c_type, "是循环体") != NULL) {
                        fprintf(ctx->output_file, "void*");
                    } else {
                        fprintf(ctx->output_file, "%s", c_type);
                    }
                } else {
                    fprintf(ctx->output_file, "void*");  // 默认使用 void*
                }
                fprintf(ctx->output_file, ")");
                cn_cgen_expr_simple(ctx, expr->as.cast.operand);
            }
            break;
        case CN_AST_EXPR_CALL:
            // 函数调用表达式：函数名(参数列表)
            {
                CnAstCallExpr *call = &expr->as.call;
                // 输出函数名（callee 是被调用的函数表达式）
                if (call->callee) {
                    cn_cgen_expr_simple(ctx, call->callee);
                }
                fprintf(ctx->output_file, "(");
                // 输出参数
                for (size_t i = 0; i < call->argument_count; i++) {
                    if (i > 0) {
                        fprintf(ctx->output_file, ", ");
                    }
                    cn_cgen_expr_simple(ctx, call->arguments[i]);
                }
                fprintf(ctx->output_file, ")");
            }
            break;
        default:
            fprintf(ctx->output_file, "0"); // 不支持的表达式，用0作为默认值
            break;
    }
}

// --- 核心生成函数 ---

void cn_cgen_inst(CnCCodeGenContext *ctx, CnIrInst *inst) {
    if (!ctx || !inst) return;
    switch (inst->kind) {
        case CN_IR_INST_LABEL: fprintf(ctx->output_file, "  %s:\n", inst->dest.as.sym_name); break;
        case CN_IR_INST_ALLOCA: fprintf(ctx->output_file, "  %s %s;\n", get_c_type_string(inst->dest.type), get_c_variable_name(inst->dest.as.sym_name)); break;
        case CN_IR_INST_LOAD:
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = ");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;
        case CN_IR_INST_STORE: {
            // 【P3修复】如果源操作数为NONE，跳过该指令
            // 这避免了生成 "变量 = /* NONE */;" 的无效C代码
            // 根因：函数调用返回值未被捕获时，STORE的源操作数为NONE
            if (inst->src1.kind == CN_IR_OP_NONE) break;
            
            // 检查目标是否需要解引用
            // 情况1：目标操作数是寄存器（通过 GET_ELEMENT_PTR 获取的地址）- 需要解引用
            // 情况2：目标操作数是符号（变量名）- 不需要解引用
            bool need_deref = (inst->dest.kind == CN_IR_OP_REG &&
                              inst->dest.type &&
                              inst->dest.type->kind == CN_TYPE_POINTER);
            fprintf(ctx->output_file, "  ");
            if (need_deref) {
                fprintf(ctx->output_file, "*");
            }
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = ");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;
        }
        case CN_IR_INST_ADD:
        case CN_IR_INST_SUB:
        case CN_IR_INST_MUL:
        case CN_IR_INST_DIV:
        case CN_IR_INST_MOD:
        case CN_IR_INST_EQ:
        case CN_IR_INST_NE:
        case CN_IR_INST_LT:
        case CN_IR_INST_GT:
        case CN_IR_INST_LE:
        case CN_IR_INST_GE:
        case CN_IR_INST_AND:
        case CN_IR_INST_OR:
        case CN_IR_INST_XOR:
        case CN_IR_INST_SHL:
        case CN_IR_INST_SHR:
            fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); print_operand(ctx, inst->src1);
            switch (inst->kind) {
                case CN_IR_INST_ADD: fprintf(ctx->output_file, " + "); break;
                case CN_IR_INST_SUB: fprintf(ctx->output_file, " - "); break;
                case CN_IR_INST_MUL: fprintf(ctx->output_file, " * "); break;
                case CN_IR_INST_DIV: fprintf(ctx->output_file, " / "); break;
                case CN_IR_INST_MOD: fprintf(ctx->output_file, " %% "); break;
                case CN_IR_INST_EQ: fprintf(ctx->output_file, " == "); break;
                case CN_IR_INST_NE: fprintf(ctx->output_file, " != "); break;
                case CN_IR_INST_LT: fprintf(ctx->output_file, " < "); break;
                case CN_IR_INST_GT: fprintf(ctx->output_file, " > "); break;
                case CN_IR_INST_LE: fprintf(ctx->output_file, " <= "); break;
                case CN_IR_INST_GE: fprintf(ctx->output_file, " >= "); break;
                case CN_IR_INST_AND: fprintf(ctx->output_file, " & "); break;
                case CN_IR_INST_OR: fprintf(ctx->output_file, " | "); break;
                case CN_IR_INST_XOR: fprintf(ctx->output_file, " ^ "); break;
                case CN_IR_INST_SHL: fprintf(ctx->output_file, " << "); break;
                case CN_IR_INST_SHR: fprintf(ctx->output_file, " >> "); break;
                default: break;
            }
            print_operand(ctx, inst->src2); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_RET: fprintf(ctx->output_file, "  return"); if (inst->src1.kind != CN_IR_OP_NONE) { fprintf(ctx->output_file, " "); print_operand(ctx, inst->src1); } fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_JUMP: fprintf(ctx->output_file, "  goto %s;\n", inst->dest.as.label->name); break;
        case CN_IR_INST_BRANCH:
            // 检查条件操作数是否有效
            if (inst->src1.kind == CN_IR_OP_NONE) {
                // 无条件分支，直接跳转到 true 分支
                fprintf(ctx->output_file, "  goto %s;\n", inst->dest.as.label->name);
            } else {
                fprintf(ctx->output_file, "  if ("); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ") goto %s; else goto %s;\n", inst->dest.as.label->name, inst->src2.as.label->name);
            }
            break;
        case CN_IR_INST_SIZEOF:
            // sizeof 运算符：生成 sizeof(type) 或 sizeof(变量)
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = sizeof(");
            
            // 检查是否是类型名（通过 extra_args_count 标记）
            bool is_type_name = (inst->extra_args_count > 0);
            
            if (is_type_name && inst->src1.kind == CN_IR_OP_SYMBOL) {
                // 参数是类型名：生成 sizeof(类型)
                // 【P0-1修复】统一使用 get_c_type_string() 获取C类型字符串
                // 该函数已内置CN基本类型关键字到C类型的映射
                if (inst->src1.type) {
                    fprintf(ctx->output_file, "%s", get_c_type_string(inst->src1.type));
                } else {
                    // 没有类型信息，默认当作结构体
                    const char *type_name = inst->src1.as.sym_name;
                    fprintf(ctx->output_file, "struct %s", type_name);
                }
            } else if (inst->src1.kind == CN_IR_OP_SYMBOL) {
                // 参数是变量名：生成 sizeof(变量类型)
                // 需要从变量类型推断
                if (inst->src1.type) {
                    fprintf(ctx->output_file, "%s", get_c_type_string(inst->src1.type));
                } else {
                    // 【P4修复】没有类型信息时，检查符号名是否可能是类型名
                    // 如果符号名不匹配当前函数的任何ALLOCA变量，则当作类型名处理
                    // 这解决了跨模块类型名在SIZEOF中被错误添加cn_var_前缀的问题
                    const char *sym_name = inst->src1.as.sym_name;
                    bool is_local_var = false;
                    if (ctx->current_func) {
                        CnIrBasicBlock *check_block = ctx->current_func->first_block;
                        while (check_block && !is_local_var) {
                            CnIrInst *check_inst = check_block->first_inst;
                            while (check_inst) {
                                if (check_inst->kind == CN_IR_INST_ALLOCA &&
                                    check_inst->dest.kind == CN_IR_OP_SYMBOL &&
                                    check_inst->dest.as.sym_name &&
                                    names_match_with_suffix(check_inst->dest.as.sym_name, sym_name)) {
                                    is_local_var = true;
                                    break;
                                }
                                check_inst = check_inst->next;
                            }
                            check_block = check_block->next;
                        }
                    }
                    if (is_local_var) {
                        // 是局部变量，使用print_operand（会添加cn_var_前缀）
                        print_operand(ctx, inst->src1);
                    } else {
                        // 不是局部变量，可能是类型名，生成 struct 类型名
                        fprintf(ctx->output_file, "struct %s", sym_name);
                    }
                }
            } else {
                // 其他情况：直接打印操作数
                print_operand(ctx, inst->src1);
            }
            
            fprintf(ctx->output_file, ");\n");
            break;
        case CN_IR_INST_CALL:
            fprintf(ctx->output_file, "  ");
            
            // 特殊处理运行时系统API函数
            // 这些函数替代了已删除的关键字(读取内存、写入内存、内存复制等)
            
            // 1. cn_rt_mem_read: 内存读取 (替代"读取内存"关键字)
            if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_read") == 0) {
                // 生成: dest = (type)cn_rt_mem_read(addr, size)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = (%s)", get_c_type_string(inst->dest.type));
                }
                fprintf(ctx->output_file, "cn_rt_mem_read(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 2. cn_rt_mem_write: 内存写入 (替代"写入内存"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_write") == 0) {
                // 生成: cn_rt_mem_write(addr, value, size)
                fprintf(ctx->output_file, "cn_rt_mem_write(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 3. cn_rt_mem_copy: 内存复制 (替代"内存复制"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_copy") == 0) {
                // 生成: cn_rt_mem_copy(dest, src, size)
                fprintf(ctx->output_file, "cn_rt_mem_copy(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 4. cn_rt_mem_set: 内存设置 (替代"内存设置"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_set") == 0) {
                // 生成: cn_rt_mem_set(addr, value, size)
                fprintf(ctx->output_file, "cn_rt_mem_set(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 5. cn_rt_mem_map: 内存映射 (替代"映射内存"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_map") == 0) {
                // 生成: dest = cn_rt_mem_map(addr, size, prot, flags)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "cn_rt_mem_map(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 6. cn_rt_mem_unmap: 解除内存映射 (替代"解除映射"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_unmap") == 0) {
                // 生成: result = cn_rt_mem_unmap(addr, size)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "cn_rt_mem_unmap(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 7. cn_rt_inline_asm: 内联汇编 (替代"内联汇编"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_inline_asm") == 0) {
                // 生成平台特定的内联汇编代码
                // 参数: asm_code, outputs, inputs, clobbers
                if (inst->extra_args_count >= 1 && 
                    inst->extra_args[0].kind == CN_IR_OP_IMM_STR) {
                    // 使用GCC内联汇编语法
                    fprintf(ctx->output_file, "__asm__ volatile (");
                    fprintf(ctx->output_file, "\"");
                    // 打印汇编代码(第一个参数)
                    for (const char *p = inst->extra_args[0].as.imm_str; p && *p; p++) {
                        if (*p == '\\') fprintf(ctx->output_file, "\\\\");
                        else if (*p == '\"') fprintf(ctx->output_file, "\\\"");
                        else fprintf(ctx->output_file, "%c", *p);
                    }
                    fprintf(ctx->output_file, "\"\n");
                    
                    // outputs (第二个参数)
                    fprintf(ctx->output_file, "  : ");
                    if (inst->extra_args_count >= 2 && 
                        inst->extra_args[1].kind == CN_IR_OP_IMM_STR &&
                        inst->extra_args[1].as.imm_str != NULL) {
                        fprintf(ctx->output_file, "\"%s\"", inst->extra_args[1].as.imm_str);
                    }
                    fprintf(ctx->output_file, "\n");
                    
                    // inputs (第三个参数)
                    fprintf(ctx->output_file, "  : ");
                    if (inst->extra_args_count >= 3 && 
                        inst->extra_args[2].kind == CN_IR_OP_IMM_STR &&
                        inst->extra_args[2].as.imm_str != NULL) {
                        fprintf(ctx->output_file, "\"%s\"", inst->extra_args[2].as.imm_str);
                    }
                    fprintf(ctx->output_file, "\n");
                    
                    // clobbers (第四个参数)
                    fprintf(ctx->output_file, "  : ");
                    if (inst->extra_args_count >= 4 && 
                        inst->extra_args[3].kind == CN_IR_OP_IMM_STR &&
                        inst->extra_args[3].as.imm_str != NULL) {
                        fprintf(ctx->output_file, "\"%s\"", inst->extra_args[3].as.imm_str);
                    }
                    fprintf(ctx->output_file, "\n);");
                } else {
                    // 汇编代码不是字符串字面量,生成占位注释
                    fprintf(ctx->output_file, "/* 内联汇编: 参数类型错误 */");
                }
                fprintf(ctx->output_file, "\n");
            }
            // 8. cn_rt_interrupt_register: 中断注册 (替代"中断处理"关键字)
            // 注意: 这个函数在main函数中自动生成(见L630),这里只处理显式调用
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_register") == 0) {
                // 生成: result = cn_rt_interrupt_register(vector, handler, name)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "cn_rt_interrupt_register(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 9. 其他中断管理函数
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                (strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_init") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_enable") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_disable") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_enable_all") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_disable_all") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_trigger") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_unregister") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_is_enabled") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_is_pending") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_get_current") == 0)) {
                // 标准运行时函数调用,直接透传
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "%s(", inst->src1.as.sym_name);
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 特殊处理 cn_rt_array_get_element：需要解引用返回的指针
            else if (inst->src1.kind == CN_IR_OP_SYMBOL &&
                strcmp(inst->src1.as.sym_name, "cn_rt_array_get_element") == 0 &&
                inst->extra_args_count >= 3 &&
                inst->dest.kind != CN_IR_OP_NONE) {
                
                // 对于结构体指针类型，返回的是指针，不需要解引用
                // 对于基本类型，需要解引用
                // 【修复】检查目标类型是否为指针类型，如果是则获取其指向的类型
                bool is_pointer_to_struct = false;
                bool is_pointer_type = false;
                CnType *target_type = inst->dest.type;
                
                // 如果目标类型是指针，获取其指向的类型
                if (target_type && target_type->kind == CN_TYPE_POINTER) {
                    is_pointer_type = true;
                    target_type = target_type->as.pointer_to;
                }
                
                // 检查目标类型是否为结构体
                is_pointer_to_struct = target_type && target_type->kind == CN_TYPE_STRUCT;
                
                print_operand(ctx, inst->dest);
                if (is_pointer_to_struct) {
                    // 【修复】结构体指针类型：数组元素访问返回的是指向结构体的指针
                    // dest = (type*)cn_rt_array_get_element(arr, idx, size)
                    // 【P0-1修复】使用 get_c_type_string() 统一获取C类型字符串
                    // 该函数已内置CN基本类型关键字到C类型的映射
                    if (target_type) {
                        fprintf(ctx->output_file, " = (%s*)", get_c_type_string(target_type));
                    } else {
                        fprintf(ctx->output_file, " = (void*)");
                    }
                } else if (is_pointer_type) {
                    // 【修复】指针类型：数组元素访问返回的是指向元素的指针
                    // 如果元素类型是指针，则返回的是指针的指针
                    // 例如：int*[] 数组，访问元素返回 int**
                    if (target_type && target_type->kind == CN_TYPE_POINTER) {
                        // 元素本身是指针类型，返回指针的指针
                        CnType *elem_pointee = target_type->as.pointer_to;
                        if (elem_pointee) {
                            // 【P0-1修复】使用 get_c_type_string() 统一获取C类型字符串
                            fprintf(ctx->output_file, " = (%s**)", get_c_type_string(elem_pointee));
                        } else {
                            fprintf(ctx->output_file, " = (void**)");
                        }
                    } else if (target_type) {
                        // 【P0-1修复】使用 get_c_type_string() 统一获取C类型字符串
                        fprintf(ctx->output_file, " = (%s*)", get_c_type_string(target_type));
                    } else {
                        // 未知类型，使用 void*
                        fprintf(ctx->output_file, " = (void*)");
                    }
                } else {
                    // 基本类型：dest = *(type*)cn_rt_array_get_element(arr, idx, size)
                    fprintf(ctx->output_file, " = *(%s)", get_c_type_string(inst->dest.type));
                }
                fprintf(ctx->output_file, "%s(", get_c_function_name(inst->src1.as.sym_name));
                
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            } else if (inst->src1.kind == CN_IR_OP_SYMBOL &&
                strcmp(inst->src1.as.sym_name, "cn_rt_array_set_element") == 0 &&
                inst->extra_args_count >= 3) {
                // 特殊处理 cn_rt_array_set_element：第3个参数需要是指针
                // 使用MSVC兼容的方式：先声明临时变量，再取地址
                
                CnIrOperand elem = inst->extra_args[2];
                
                // 根据元素类型生成临时变量并赋值
                if (elem.kind == CN_IR_OP_IMM_INT) {
                    // 整数常量：使用long long临时变量
                    static int temp_int_counter = 0;
                    fprintf(ctx->output_file, "  { long long _tmp_i%d = ", temp_int_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_i%d", temp_int_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_int_counter++;
                } else if (elem.kind == CN_IR_OP_IMM_FLOAT) {
                    // 浮点常量：使用double临时变量
                    static int temp_float_counter = 0;
                    fprintf(ctx->output_file, "  { double _tmp_f%d = ", temp_float_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_f%d", temp_float_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_float_counter++;
                } else if (elem.kind == CN_IR_OP_IMM_STR) {
                    // 字符串常量：使用char*临时变量
                    static int temp_str_counter = 0;
                    fprintf(ctx->output_file, "  { char* _tmp_s%d = ", temp_str_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_s%d", temp_str_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_str_counter++;
                } else if (elem.kind == CN_IR_OP_REG) {
                    // 寄存器变量：根据类型生成临时变量
                    static int temp_reg_counter = 0;
                    const char *tmp_type = "long long";
                    char struct_type_buf[256] = {0};  // 用于结构体类型名
                    if (elem.type) {
                        switch (elem.type->kind) {
                            case CN_TYPE_FLOAT: tmp_type = "double"; break;
                            case CN_TYPE_STRING: tmp_type = "char*"; break;
                            case CN_TYPE_STRUCT:
                                // 结构体类型：生成 "struct 结构体名"
                                if (elem.type->as.struct_type.name) {
                                    snprintf(struct_type_buf, sizeof(struct_type_buf), "struct %.*s",
                                        (int)elem.type->as.struct_type.name_length,
                                        elem.type->as.struct_type.name);
                                    tmp_type = struct_type_buf;
                                } else {
                                    tmp_type = "struct void";
                                }
                                break;
                            default: tmp_type = "long long"; break;
                        }
                    }
                    fprintf(ctx->output_file, "  { %s _tmp_r%d = ", tmp_type, temp_reg_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_r%d", temp_reg_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_reg_counter++;
                } else {
                    // 已经是符号类型，直接取地址
                    fprintf(ctx->output_file, "  %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &");
                    print_operand(ctx, elem);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, ");\n");
                }
            } else {
                // 普通函数调用
                if (inst->dest.kind != CN_IR_OP_NONE) { print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); }
                
                if (inst->src1.kind == CN_IR_OP_SYMBOL) {
                    fprintf(ctx->output_file, "%s(", get_c_function_name(inst->src1.as.sym_name));
                } else {
                    print_operand(ctx, inst->src1);
                    fprintf(ctx->output_file, "(");
                }
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            break;
        case CN_IR_INST_MOV: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_SELECT: 
            // 三元运算符：dest = condition ? true_val : false_val
            // 指令格式：dest, src1=condition, src2=true_val, extra_args[0]=false_val
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = (");
            print_operand(ctx, inst->src1);  // condition
            fprintf(ctx->output_file, " ? ");
            print_operand(ctx, inst->src2);  // true_val
            fprintf(ctx->output_file, " : ");
            if (inst->extra_args_count > 0) {
                print_operand(ctx, inst->extra_args[0]);  // false_val
            } else {
                fprintf(ctx->output_file, "0");  // fallback
            }
            fprintf(ctx->output_file, ");\n");
            break;
        case CN_IR_INST_ADDRESS_OF: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = &"); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_DEREF: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = *"); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_GET_ELEMENT_PTR: {
            // 数组索引访问（静态数组）：dest = &array[index]
            // 生成 C 风格的数组索引访问：dest = &array[index]
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = &");
            print_operand(ctx, inst->src1);  // 数组
            fprintf(ctx->output_file, "[");
            print_operand(ctx, inst->src2);  // 索引
            fprintf(ctx->output_file, "];\n");
            break;
        }
        case CN_IR_INST_MEMBER_ACCESS: {
            // 结构体成员访问：dest = obj.member 或 dest = ptr->member
            // CN语言统一使用'.'访问成员，编译器自动根据类型选择'.'或'->'
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = ");
            print_operand(ctx, inst->src1);
            
            // 【P8修复】多层指针类型检测，决定使用 -> 还是 .
            // 关键改进：不再使用if/else if链式结构，而是逐层检查。
            // 当高层级返回低精度类型（INT/VOID/UNKNOWN）时，继续检查低层级，
            // 因为alloca_types和reg_types经过P6多遍扫描后可能有更精确的POINTER类型。
            // 只有高层级返回高精度类型（POINTER/STRUCT/STRING/ENUM）时才直接信任。
            bool is_pointer = false;
            bool has_precise_type = false;  // 是否已获得高精度类型（无需继续检查）
            
            // 层级1：操作数自带类型信息
            if (inst->src1.type) {
                is_pointer = is_pointer_type_unified(inst->src1.type);
                // 判断是否为无歧义的高精度指针类型，可直接信任
                // 关键：CN_TYPE_STRUCT/ENUM 不应阻止后续检查，因为IR生成器可能
                // 将指针变量标记为STRUCT而非POINTER，alloca_types经过P6类型传播后更准确
                // 同时要区分CnType和struct类型节点：只有CnType才能用kind判断
                CnType *cn_type_check = (CnType *)inst->src1.type;
                if (cn_type_check->kind >= 0 && cn_type_check->kind <= CN_TYPE_UNKNOWN) {
                    /* 有效的CnType */
                    CnTypeKind kind = cn_type_check->kind;
                    has_precise_type = (kind == CN_TYPE_POINTER || kind == CN_TYPE_STRING);
                }
                /* 如果是struct类型节点（kind超出CnTypeKind范围），不设置has_precise_type，
                 * 允许后续层级检查，因为类型节点的指针层级可能不准确 */
            }
            
            // 层级2：符号操作数 - 检查函数参数和变量类型
            // 当层级1没有类型，或层级1类型为低精度时，继续检查
            if (!has_precise_type && inst->src1.kind == CN_IR_OP_SYMBOL && inst->src1.as.sym_name) {
                const char *sym_name = inst->src1.as.sym_name;
                
                // 层级2a：检查是否为函数参数（参数不会有ALLOCA指令）
                if (!is_pointer && ctx->current_func && ctx->current_func->params) {
                    for (size_t i = 0; i < ctx->current_func->param_count && !is_pointer; i++) {
                        CnIrOperand *param = &ctx->current_func->params[i];
                        if (param->kind == CN_IR_OP_SYMBOL &&
                            param->as.sym_name &&
                            names_match_with_suffix(param->as.sym_name, sym_name)) {
                            if (param->type && is_pointer_type_unified(param->type)) {
                                is_pointer = true;
                            }
                        }
                    }
                }
                
                // 层级2b：【P8修复】从alloca_types映射表查找（P6多遍扫描更新后的类型）
                // 这比遍历ALLOCA指令更可靠，因为alloca_types包含了STORE类型传播的更新
                if (!is_pointer && ctx->alloca_types) {
                    AllocaTypeEntry *entry = (AllocaTypeEntry *)ctx->alloca_types;
                    while (entry && !is_pointer) {
                        if (entry->sym_name && names_match_with_suffix(entry->sym_name, sym_name)) {
                            if (entry->type && is_pointer_type_unified(entry->type)) {
                                is_pointer = true;
                            }
                        }
                        entry = entry->next;
                    }
                }
                
                // 层级2c：遍历ALLOCA指令查找（回退方案）
                if (!is_pointer) {
                    CnIrBasicBlock *block = ctx->current_func->first_block;
                    while (block && !is_pointer) {
                        CnIrInst *alloca_inst = block->first_inst;
                        while (alloca_inst && !is_pointer) {
                            if (alloca_inst->kind == CN_IR_INST_ALLOCA &&
                                alloca_inst->dest.kind == CN_IR_OP_SYMBOL &&
                                alloca_inst->dest.as.sym_name &&
                                names_match_with_suffix(alloca_inst->dest.as.sym_name, sym_name) &&
                                alloca_inst->dest.type) {
                                if (is_pointer_type_unified(alloca_inst->dest.type)) {
                                    is_pointer = true;
                                }
                            }
                            alloca_inst = alloca_inst->next;
                        }
                        block = block->next;
                    }
                }
                
                // 层级2d：【P8修复】启发式变量名检测 - 当类型信息完全缺失时使用
                // CN语言中变量名包含"指针"关键词的通常是指针类型
                if (!is_pointer) {
                    size_t sym_len = strlen(sym_name);
                    // 检查变量名是否包含"指针"关键词（UTF-8编码）
                    if (strstr(sym_name, "\xe6\x8c\x87\xe9\x92\x88") != NULL) {
                        is_pointer = true;
                    }
                    // 检查变量名是否以"_ptr"或"Ptr"结尾（C风格指针命名）
                    else if (sym_len > 4 && strcmp(sym_name + sym_len - 4, "_ptr") == 0) {
                        is_pointer = true;
                    }
                    else if (sym_len > 3 && strcmp(sym_name + sym_len - 3, "Ptr") == 0) {
                        is_pointer = true;
                    }
                }
            }
            
            // 层级3：寄存器操作数 - 从reg_types中获取类型信息
            // 当层级1/2没有获得高精度类型时，检查reg_types
            if (!has_precise_type && !is_pointer && inst->src1.kind == CN_IR_OP_REG && ctx->reg_types) {
                int reg_id = inst->src1.as.reg_id;
                if (reg_id < ctx->reg_types_count && ctx->reg_types[reg_id]) {
                    is_pointer = is_pointer_type_unified(ctx->reg_types[reg_id]);
                    #ifdef CN_DEBUG_TYPE_PROPAGATION
                    fprintf(stderr, "[DEBUG] MEMBER_ACCESS: src1 是寄存器 r%d, is_pointer=%s\n",
                            reg_id, is_pointer ? "true" : "false");
                    #endif
                } else {
                    #ifdef CN_DEBUG_TYPE_PROPAGATION
                    fprintf(stderr, "[DEBUG] MEMBER_ACCESS: src1 是寄存器 r%d, 但 reg_types 中没有类型信息\n", reg_id);
                    #endif
                }
            }
            
            if (is_pointer) {
                fprintf(ctx->output_file, "->");
            } else {
                fprintf(ctx->output_file, ".");
            }
            // src2 为成员名（符号类型）
            if (inst->src2.kind == CN_IR_OP_SYMBOL) {
                fprintf(ctx->output_file, "%s", inst->src2.as.sym_name);
            }
            fprintf(ctx->output_file, ";\n");
            break;
        }
        case CN_IR_INST_STRUCT_INIT:
            // 结构体初始化（构造函数调用）：dest = (struct 类型名){arg1, arg2, ...}
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = (struct ");
            // src1 为类型名（符号类型）
            if (inst->src1.kind == CN_IR_OP_SYMBOL) {
                fprintf(ctx->output_file, "%s", inst->src1.as.sym_name);
            }
            fprintf(ctx->output_file, "){");
            // 打印参数
            for (size_t i = 0; i < inst->extra_args_count; i++) {
                if (i > 0) fprintf(ctx->output_file, ", ");
                print_operand(ctx, inst->extra_args[i]);
            }
            fprintf(ctx->output_file, "};\n");
            break;
        case CN_IR_INST_NEG:
            // 一元负号：dest = -src1
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = -");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;
        case CN_IR_INST_NOT:
            // 逻辑非：dest = !src1
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = !");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;
        default: fprintf(ctx->output_file, "  /* Unsupported inst %d */\n", inst->kind); break;
    }
}

void cn_cgen_block(CnCCodeGenContext *ctx, CnIrBasicBlock *block) {
    if (!ctx || !block) return;
    if (block->name) fprintf(ctx->output_file, "\n  %s:\n", block->name);
    CnIrInst *inst = block->first_inst;
    while (inst) { cn_cgen_inst(ctx, inst); inst = inst->next; }
}

void cn_cgen_function(CnCCodeGenContext *ctx, CnIrFunction *func) {
    if (!ctx || !func) return;
    ctx->current_func = func;
    
    // 生成函数名：直接使用原始函数名（不再使用模块前缀编码）
    // 模块系统通过作用域管理避免命名冲突
    const char *c_func_name = get_c_function_name(func->name);
    
    bool is_main = strcmp(c_func_name, "main") == 0;
    
    // 注意：运行时宏冲突的 #undef 已在前向声明之前统一处理，此处不再重复
    
    // 中断处理函数：生成注释和属性
    if (func->is_interrupt_handler) {
        fprintf(ctx->output_file, "// 中断处理函数 (ISR) - 中断向量号: %u\n", func->interrupt_vector);
    }
    
    // 生成函数签名
    fprintf(ctx->output_file, "%s %s(", get_c_type_string(func->return_type), c_func_name);
    for (size_t i = 0; i < func->param_count; i++) {
        fprintf(ctx->output_file, "%s %s", get_c_param_type_string(func->params[i].type), get_c_variable_name(func->params[i].as.sym_name));
        if (i < func->param_count - 1) fprintf(ctx->output_file, ", ");
    }
    
    // 检查是否为函数原型声明（无函数体）
    if (func->is_prototype) {
        // 函数原型声明：只生成声明，不生成函数体
        fprintf(ctx->output_file, ");\n");
        return;
    }
    
    fprintf(ctx->output_file, ") {\n");
    
    // 生成静态局部变量声明（在函数体开头）
    if (func->first_static_var) {
        fprintf(ctx->output_file, "    /* 静态局部变量 */\n");
        CnIrStaticVar *static_var = func->first_static_var;
        while (static_var) {
            const char *type_str = get_c_type_string(static_var->type);
            // 生成静态变量名：cn_static_{函数名}_{变量名}
            fprintf(ctx->output_file, "    static %s cn_static_%s_%s",
                    type_str, func->name, static_var->name);
            
            // 如果有初始化值，生成初始化
            if (static_var->initializer.kind != CN_IR_OP_NONE) {
                fprintf(ctx->output_file, " = ");
                if (static_var->initializer.kind == CN_IR_OP_IMM_INT) {
                    fprintf(ctx->output_file, "%lld", static_var->initializer.as.imm_int);
                } else if (static_var->initializer.kind == CN_IR_OP_IMM_FLOAT) {
                    fprintf(ctx->output_file, "%f", static_var->initializer.as.imm_float);
                } else if (static_var->initializer.kind == CN_IR_OP_IMM_STR) {
                    // 字符串字面量：需要加引号并处理转义
                    fprintf(ctx->output_file, "\"");
                    for (const char *p = static_var->initializer.as.imm_str; p && *p; p++) {
                        switch (*p) {
                            case '\\': fprintf(ctx->output_file, "\\\\"); break;
                            case '\"': fprintf(ctx->output_file, "\\\""); break;
                            case '\n': fprintf(ctx->output_file, "\\n"); break;
                            case '\r': fprintf(ctx->output_file, "\\r"); break;
                            case '\t': fprintf(ctx->output_file, "\\t"); break;
                            default: fprintf(ctx->output_file, "%c", *p); break;
                        }
                    }
                    fprintf(ctx->output_file, "\"");
                }
            }
            fprintf(ctx->output_file, ";\n");
            static_var = static_var->next;
        }
        fprintf(ctx->output_file, "\n");
    }
    
    // 注入运行时初始化（仅 hosted 模式）
    if (is_main && ctx->module && ctx->module->compile_mode != CN_COMPILE_MODE_FREESTANDING) {
        fprintf(ctx->output_file, "  cn_rt_init();\n");
        
        // 自动注册ISR函数
        CnIrFunction *isr_func = ctx->module->first_func;
        while (isr_func) {
            if (isr_func->is_interrupt_handler) {
                const char *isr_c_name = get_c_function_name(isr_func->name);
                fprintf(ctx->output_file, "  cn_rt_interrupt_register(%u, %s, \"%s\");\n",
                        isr_func->interrupt_vector, isr_c_name, isr_c_name);
            }
            isr_func = isr_func->next;
        }
    }
    
    // 声明虚拟寄存器：根据IR指令中的类型信息收集寄存器类型
    if (func->next_reg_id > 0) {
        /* 首先扫描所有指令以找出实际使用的最大寄存器ID */
        int max_reg_id = -1;
        CnIrBasicBlock *prescan_block = func->first_block;
        while (prescan_block) {
            CnIrInst *prescan_inst = prescan_block->first_inst;
            while (prescan_inst) {
                if (prescan_inst->dest.kind == CN_IR_OP_REG && prescan_inst->dest.as.reg_id > max_reg_id) {
                    max_reg_id = prescan_inst->dest.as.reg_id;
                }
                if (prescan_inst->src1.kind == CN_IR_OP_REG && prescan_inst->src1.as.reg_id > max_reg_id) {
                    max_reg_id = prescan_inst->src1.as.reg_id;
                }
                if (prescan_inst->src2.kind == CN_IR_OP_REG && prescan_inst->src2.as.reg_id > max_reg_id) {
                    max_reg_id = prescan_inst->src2.as.reg_id;
                }
                for (size_t i = 0; i < prescan_inst->extra_args_count; i++) {
                    if (prescan_inst->extra_args[i].kind == CN_IR_OP_REG && prescan_inst->extra_args[i].as.reg_id > max_reg_id) {
                        max_reg_id = prescan_inst->extra_args[i].as.reg_id;
                    }
                }
                prescan_inst = prescan_inst->next;
            }
            prescan_block = prescan_block->next;
        }
        
        // 使用实际的最大寄存器ID+1或func->next_reg_id中的较大值
        int scan_reg_count = (max_reg_id >= 0) ? (max_reg_id + 1) : 0;
        int actual_reg_count = (scan_reg_count > func->next_reg_id) ? scan_reg_count : func->next_reg_id;
        
        /* 性能优化：使用栈分配替代 calloc，避免堆分配开销 */
        CnType *reg_types_stack[256];  /* 大多数函数的寄存器数量 < 256 */
        CnType **reg_types = NULL;
        bool use_heap = false;
        
        if (actual_reg_count <= 256) {
            /* 小寄存器集合使用栈分配 */
            memset(reg_types_stack, 0, actual_reg_count * sizeof(CnType*));
            reg_types = reg_types_stack;
        } else {
            /* 大寄存器集合使用堆分配 */
            reg_types = calloc(actual_reg_count, sizeof(CnType*));
            use_heap = true;
            if (!reg_types) {
                /* 内存分配失败，使用默认类型 */
                fprintf(ctx->output_file, "  long long r0");
                for (int i = 1; i < actual_reg_count; i++) {
                    fprintf(ctx->output_file, ", r%d", i);
                }
                fprintf(ctx->output_file, ";\n");
                goto skip_register_type_scan;
            }
        }
        
        /* 【修复】预扫描ALLOCA指令，建立符号名到类型的映射表
         * 这解决了MOV指令类型传播问题：当MOV指令从局部变量赋值时，
         * 需要知道变量的指针类型。通过预扫描ALLOCA指令，我们可以
         * 在多遍扫描开始前就建立完整的符号类型映射。
         * 【P8修复】AllocaTypeEntry已移到文件作用域，供MEMBER_ACCESS指令使用
         */
        AllocaTypeEntry *alloca_types = NULL;  // 符号类型映射链表
        
        // 预扫描所有ALLOCA指令
        CnIrBasicBlock *prescan_alloca_block = func->first_block;
        while (prescan_alloca_block) {
            CnIrInst *prescan_alloca_inst = prescan_alloca_block->first_inst;
            while (prescan_alloca_inst) {
                if (prescan_alloca_inst->kind == CN_IR_INST_ALLOCA &&
                    prescan_alloca_inst->dest.kind == CN_IR_OP_SYMBOL &&
                    prescan_alloca_inst->dest.as.sym_name &&
                    prescan_alloca_inst->dest.type) {
                    // 添加到映射表
                    AllocaTypeEntry *entry = (AllocaTypeEntry *)malloc(sizeof(AllocaTypeEntry));
                    if (entry) {
                        entry->sym_name = prescan_alloca_inst->dest.as.sym_name;
                        entry->type = prescan_alloca_inst->dest.type;
                        entry->next = alloca_types;
                        alloca_types = entry;
                    }
                }
                prescan_alloca_inst = prescan_alloca_inst->next;
            }
            prescan_alloca_block = prescan_alloca_block->next;
        }
        
        /* 【P1修复】预扫描STORE指令，当STORE将CALL结果存入ALLOCA变量时，
         * 如果CALL返回指针/结构体类型但ALLOCA是INT类型，更新ALLOCA类型。
         * 这解决了变量类型推断错误：如 "变量 解析器 = 创建解析器()"
         * 创建解析器返回指针，但解析器被声明为 long long int 的问题。
         */
        {
            CnIrBasicBlock *prescan_store_block = func->first_block;
            while (prescan_store_block) {
                CnIrInst *prescan_store_inst = prescan_store_block->first_inst;
                while (prescan_store_inst) {
                    if (prescan_store_inst->kind == CN_IR_INST_STORE &&
                        prescan_store_inst->dest.kind == CN_IR_OP_SYMBOL &&
                        prescan_store_inst->dest.as.sym_name &&
                        prescan_store_inst->src1.kind == CN_IR_OP_REG &&
                        prescan_store_inst->src1.type) {
                        /* 检查源寄存器类型是否为指针/结构体/枚举 */
                        CnType *src_type = prescan_store_inst->src1.type;
                        bool src_is_pointer_or_struct = (src_type->kind == CN_TYPE_POINTER ||
                                                         src_type->kind == CN_TYPE_STRUCT ||
                                                         src_type->kind == CN_TYPE_ENUM ||
                                                         src_type->kind == CN_TYPE_STRING);
                        if (src_is_pointer_or_struct) {
                            /* 在alloca_types中查找目标变量 */
                            const char *dest_name = prescan_store_inst->dest.as.sym_name;
                            AllocaTypeEntry *entry = alloca_types;
                            while (entry) {
                                if (entry->sym_name && names_match_with_suffix(entry->sym_name, dest_name)) {
                                    bool should_update = false;
                                    /* 如果ALLOCA当前类型是INT/VOID/UNKNOWN，更新为源类型 */
                                    if (entry->type &&
                                        (entry->type->kind == CN_TYPE_INT ||
                                         entry->type->kind == CN_TYPE_VOID ||
                                         entry->type->kind == CN_TYPE_UNKNOWN)) {
                                        should_update = true;
                                    }
                                    /* 【P8修复】POINTER类型可覆盖STRUCT/ENUM类型
                                     * 例如：变量声明为struct X但赋值为(struct X*)指针，
                                     * 应更新为POINTER类型以正确生成 -> 访问 */
                                    else if (src_type->kind == CN_TYPE_POINTER &&
                                             entry->type &&
                                             (entry->type->kind == CN_TYPE_STRUCT ||
                                              entry->type->kind == CN_TYPE_ENUM)) {
                                        should_update = true;
                                    }
                                    if (should_update) {
                                        entry->type = src_type;
                                    }
                                    break;
                                }
                                entry = entry->next;
                            }
                        }
                    }
                    prescan_store_inst = prescan_store_inst->next;
                }
                prescan_store_block = prescan_store_block->next;
            }
        }
        
        /* 性能优化：多遍扫描收集寄存器类型，确保 MEMBER_ACCESS 指令能获取正确的类型 */
        bool types_changed = true;
        int max_iterations = 10;  // 防止无限循环
        int iteration = 0;
        while (types_changed && iteration < max_iterations) {
            types_changed = false;
            iteration++;
            
        CnIrBasicBlock *scan_block = func->first_block;
        while (scan_block) {
            CnIrInst *scan_inst = scan_block->first_inst;
            while (scan_inst) {
                // 收集dest寄存器类型
                // 注意：指针类型优先于整型类型，因为指针类型需要正确的成员访问语法
                // 同时，结构体类型也优先于整型类型
                if (scan_inst->dest.kind == CN_IR_OP_REG) {
                    if (scan_inst->dest.as.reg_id < actual_reg_count) {
                        int reg_id = scan_inst->dest.as.reg_id;
                        // 对于 LOAD 指令，如果 dest.type 为 NULL，尝试从 src1.type 获取类型
                        // 对于 MEMBER_ACCESS 指令，结果类型应该是成员的类型
                        CnType *new_type = scan_inst->dest.type;
                        if (scan_inst->kind == CN_IR_INST_LOAD) {
                            // 【重要修复】对于LOAD指令，优先从ALLOCA映射表获取类型
                            // 这确保了从局部变量加载时，类型信息正确传播
                            // 特别是对于指针类型变量，ALLOCA映射表中存储了正确的指针类型
                            if (scan_inst->src1.kind == CN_IR_OP_SYMBOL && scan_inst->src1.as.sym_name) {
                                const char *sym_name = scan_inst->src1.as.sym_name;
                                
                                // 【最高优先级】从预扫描的ALLOCA映射表中查找
                                // 这解决了LOAD指令类型传播不正确的问题
                                AllocaTypeEntry *entry = alloca_types;
                                while (entry) {
                                    if (names_match_with_suffix(entry->sym_name, sym_name)) {
                                        new_type = entry->type;
                                        break;
                                    }
                                    entry = entry->next;
                                }
                            }
                            
                            // 如果ALLOCA映射表中没有找到，尝试其他来源
                            if (!new_type) {
                                // 从 src1.type 获取
                                if (scan_inst->src1.type) {
                                    new_type = scan_inst->src1.type;
                                }
                                // 首先尝试从 dest.type 获取
                                else if (scan_inst->dest.type) {
                                    new_type = scan_inst->dest.type;
                                }
                                // 如果 src1 是寄存器，尝试从 reg_types 中获取
                                else if (scan_inst->src1.kind == CN_IR_OP_REG &&
                                         scan_inst->src1.as.reg_id < actual_reg_count) {
                                    new_type = reg_types[scan_inst->src1.as.reg_id];
                                }
                            }
                            
                            // 【备用】如果仍然没有找到类型，检查函数参数和全局作用域
                            if (!new_type && scan_inst->src1.kind == CN_IR_OP_SYMBOL && scan_inst->src1.as.sym_name) {
                                const char *sym_name = scan_inst->src1.as.sym_name;
                                
                                // 检查函数参数
                                for (size_t p = 0; p < func->param_count; p++) {
                                    if (func->params[p].as.sym_name) {
                                        // 使用 names_match_with_suffix 匹配（考虑局部变量名后缀）
                                        if (names_match_with_suffix(func->params[p].as.sym_name, sym_name)) {
                                            new_type = func->params[p].type;
                                            break;
                                        }
                                    }
                                }
                                
                                // 【新增】如果仍然没有找到，尝试从全局作用域获取变量类型
                                if (!new_type && ctx->global_scope) {
                                    // 去掉 cn_var_ 前缀后查找原始变量名
                                    const char *var_name = sym_name;
                                    size_t var_name_len = strlen(sym_name);
                                    if (strncmp(sym_name, "cn_var_", 7) == 0) {
                                        var_name = sym_name + 7;
                                        var_name_len -= 7;
                                    }
                                    CnSemSymbol *global_sym = cn_sem_scope_lookup(ctx->global_scope, var_name, var_name_len);
                                    if (global_sym && global_sym->kind == CN_SEM_SYMBOL_VARIABLE && global_sym->type) {
                                        new_type = global_sym->type;
                                    }
                                }
                            }
                        }
                        
                        /* 【P6修复增强】MOV指令类型传播
                         * 移除!new_type限制：当dest.type已设为INT时，
                         * 仍允许从src1获取更精确的指针/结构体类型覆盖
                         * 同时添加IMM_STR(char*)类型传播支持
                         */
                        if (scan_inst->kind == CN_IR_INST_MOV) {
                            CnType *mov_src_type = NULL;
                            
                            /* 优先级1：从src1.type直接获取 */
                            if (scan_inst->src1.type) {
                                mov_src_type = scan_inst->src1.type;
                            }
                            /* 优先级2：IMM_STR视为char*类型 */
                            if (!mov_src_type && scan_inst->src1.kind == CN_IR_OP_IMM_STR) {
                                mov_src_type = cn_type_new_primitive(CN_TYPE_STRING);
                            }
                            /* 优先级3：从符号名查找ALLOCA映射表/函数参数/全局作用域 */
                            if (!mov_src_type && scan_inst->src1.kind == CN_IR_OP_SYMBOL && scan_inst->src1.as.sym_name) {
                                const char *sym_name = scan_inst->src1.as.sym_name;
                                
                                // 首先从预扫描的ALLOCA映射表中查找（最高优先级）
                                AllocaTypeEntry *entry = alloca_types;
                                while (entry) {
                                    if (names_match_with_suffix(entry->sym_name, sym_name)) {
                                        mov_src_type = entry->type;
                                        break;
                                    }
                                    entry = entry->next;
                                }
                                
                                // 如果映射表中没有，检查函数参数
                                if (!mov_src_type) {
                                    for (size_t p = 0; p < func->param_count; p++) {
                                        if (func->params[p].as.sym_name) {
                                            if (names_match_with_suffix(func->params[p].as.sym_name, sym_name)) {
                                                mov_src_type = func->params[p].type;
                                                break;
                                            }
                                        }
                                    }
                                }
                                
                                // 尝试从全局作用域获取变量类型
                                if (!mov_src_type && ctx->global_scope) {
                                    const char *var_name = sym_name;
                                    size_t var_name_len = strlen(sym_name);
                                    if (strncmp(sym_name, "cn_var_", 7) == 0) {
                                        var_name = sym_name + 7;
                                        var_name_len -= 7;
                                    }
                                    CnSemSymbol *global_sym = cn_sem_scope_lookup(ctx->global_scope, var_name, var_name_len);
                                    if (global_sym && global_sym->kind == CN_SEM_SYMBOL_VARIABLE && global_sym->type) {
                                        mov_src_type = global_sym->type;
                                    }
                                }
                            }
                            /* 优先级4：从reg_types获取寄存器类型 */
                            if (!mov_src_type && scan_inst->src1.kind == CN_IR_OP_REG &&
                                     scan_inst->src1.as.reg_id < actual_reg_count) {
                                mov_src_type = reg_types[scan_inst->src1.as.reg_id];
                            }
                            
                            /* 仅当mov_src_type比当前new_type更精确时才覆盖
                             * 指针/结构体/字符串类型可以覆盖INT/UNKNOWN/VOID/CHAR/BOOL */
                            if (mov_src_type && is_pointer_like_type(mov_src_type) &&
                                (!new_type ||
                                 new_type->kind == CN_TYPE_INT ||
                                 new_type->kind == CN_TYPE_UNKNOWN ||
                                 new_type->kind == CN_TYPE_VOID ||
                                 new_type->kind == CN_TYPE_CHAR ||
                                 new_type->kind == CN_TYPE_BOOL)) {
                                new_type = mov_src_type;
                            }
                            /* 结构体类型也可以覆盖低精度类型 */
                            else if (mov_src_type && mov_src_type->kind == CN_TYPE_STRUCT &&
                                (!new_type ||
                                 new_type->kind == CN_TYPE_INT ||
                                 new_type->kind == CN_TYPE_UNKNOWN ||
                                 new_type->kind == CN_TYPE_VOID)) {
                                new_type = mov_src_type;
                            }
                            /* 如果没有new_type，使用mov_src_type */
                            else if (!new_type && mov_src_type) {
                                new_type = mov_src_type;
                            }
                        }
                        
                        // 【修复】对于 CALL 指令，优先使用 dest.type 作为返回类型
                        // 这是类型传播问题的关键修复点
                        // 注意：这里不使用 !new_type 条件，因为我们要强制使用 dest.type
                        if (scan_inst->kind == CN_IR_INST_CALL) {
                            // CALL 指令的 dest.type 应该已经被 IR 生成器设置为返回类型
                            // 优先使用 dest.type，因为它是最准确的类型信息
                            if (scan_inst->dest.type) {
                                new_type = scan_inst->dest.type;
                                
                                // 【调试】输出类型传播信息
                                #ifdef CN_DEBUG_TYPE_PROPAGATION
                                if (scan_inst->dest.type->kind == CN_TYPE_POINTER) {
                                    fprintf(stderr, "[DEBUG] CALL 指令类型传播: dest.type 是指针类型, reg_id=%d\n", reg_id);
                                } else if (scan_inst->dest.type->kind == CN_TYPE_STRUCT) {
                                    fprintf(stderr, "[DEBUG] CALL 指令类型传播: dest.type 是结构体类型, reg_id=%d\n", reg_id);
                                } else {
                                    fprintf(stderr, "[DEBUG] CALL 指令类型传播: dest.type 是其他类型 (kind=%d), reg_id=%d\n", scan_inst->dest.type->kind, reg_id);
                                }
                                #endif
                            }
                            // 如果 dest.type 为 NULL，尝试从被调用函数的符号信息获取返回类型
                            else if (scan_inst->src1.kind == CN_IR_OP_SYMBOL && scan_inst->src1.as.sym_name && ctx->global_scope) {
                                const char *func_name = scan_inst->src1.as.sym_name;
                                // 去掉可能的前缀
                                const char *lookup_name = func_name;
                                size_t lookup_len = strlen(func_name);
                                // 查找函数符号
                                CnSemSymbol *func_sym = cn_sem_scope_lookup(ctx->global_scope, lookup_name, lookup_len);
                                if (func_sym && func_sym->kind == CN_SEM_SYMBOL_FUNCTION && func_sym->type) {
                                    // 函数符号的 type 是函数类型，从中提取返回类型
                                    if (func_sym->type->kind == CN_TYPE_FUNCTION && func_sym->type->as.function.return_type) {
                                        new_type = func_sym->type->as.function.return_type;
                                    }
                                }
                            }
                        }
                        
                        // 【新增】对于 GET_ELEMENT_PTR 指令，处理静态数组索引访问的类型
                        // GET_ELEMENT_PTR 返回指向数组元素的指针
                        if (!new_type && scan_inst->kind == CN_IR_INST_GET_ELEMENT_PTR) {
                            // dest = &array[index]，dest 的类型是指向数组元素类型的指针
                            if (scan_inst->dest.type) {
                                new_type = scan_inst->dest.type;
                            } else if (scan_inst->src1.type) {
                                // 从数组类型推断元素指针类型
                                CnType *array_type = scan_inst->src1.type;
                                if (array_type->kind == CN_TYPE_ARRAY && array_type->as.array.element_type) {
                                    // 元素指针类型
                                    new_type = cn_type_new_pointer(array_type->as.array.element_type);
                                } else if (array_type->kind == CN_TYPE_POINTER) {
                                    // 如果是指针类型（动态数组），直接使用
                                    new_type = array_type;
                                }
                            }
                        }
                        
                        // 对于 MEMBER_ACCESS 指令，优先使用 dest.type（IR生成器设置的成员类型）
                        // 这是最可靠的类型来源，特别是对于指针成员
                        if (scan_inst->kind == CN_IR_INST_MEMBER_ACCESS) {
                            // 【优先】使用 dest.type（IR生成器设置的成员类型）
                            if (scan_inst->dest.type) {
                                new_type = scan_inst->dest.type;
                            }
                            
                            // 如果 dest.type 为 NULL，尝试从 src1.type 推断成员类型
                            if (!new_type) {
                                // src1 是对象，src2 是成员名
                                // 需要从对象的类型中推断成员的类型
                                CnType *obj_type = scan_inst->src1.type;
                                
                                // 如果 src1.type 为 NULL，尝试从 reg_types 中获取
                                if (!obj_type && scan_inst->src1.kind == CN_IR_OP_REG &&
                                    scan_inst->src1.as.reg_id < actual_reg_count) {
                                    obj_type = reg_types[scan_inst->src1.as.reg_id];
                                }
                                
                                // 【修复】如果 src1 是符号（变量名），检查变量是否为指针类型
                                // 这处理了 MEMBER_ACCESS 指令从指针变量访问成员时类型信息丢失的问题
                                if (!obj_type && scan_inst->src1.kind == CN_IR_OP_SYMBOL && scan_inst->src1.as.sym_name) {
                                    const char *sym_name = scan_inst->src1.as.sym_name;
                                    // 首先检查函数参数
                                    for (size_t p = 0; p < func->param_count; p++) {
                                        if (func->params[p].as.sym_name) {
                                            if (names_match_with_suffix(func->params[p].as.sym_name, sym_name)) {
                                                obj_type = func->params[p].type;
                                                break;
                                            }
                                        }
                                    }
                                    // 如果不是参数，检查局部变量（从 ALLOCA 指令中查找）
                                    if (!obj_type) {
                                        CnIrBasicBlock *search_block = func->first_block;
                                        while (search_block && !obj_type) {
                                            CnIrInst *search_inst = search_block->first_inst;
                                            while (search_inst && !obj_type) {
                                                if (search_inst->kind == CN_IR_INST_ALLOCA &&
                                                    search_inst->dest.kind == CN_IR_OP_SYMBOL &&
                                                    search_inst->dest.as.sym_name &&
                                                    names_match_with_suffix(search_inst->dest.as.sym_name, sym_name)) {
                                                    obj_type = search_inst->dest.type;
                                                    break;
                                                }
                                                search_inst = search_inst->next;
                                            }
                                            search_block = search_block->next;
                                        }
                                    }
                                }
                                
                            if (obj_type) {
                                // 如果是指针类型，获取指向的类型
                                if (obj_type->kind == CN_TYPE_POINTER && obj_type->as.pointer_to) {
                                    obj_type = obj_type->as.pointer_to;
                                }
                                // 如果是结构体类型但没有字段信息，尝试从 dest.type 或全局作用域获取
                                if (obj_type->kind == CN_TYPE_STRUCT && !obj_type->as.struct_type.fields) {
                                    // 首先尝试从 dest.type 获取
                                    if (scan_inst->dest.type && scan_inst->dest.type->kind == CN_TYPE_STRUCT && scan_inst->dest.type->as.struct_type.fields) {
                                        obj_type = scan_inst->dest.type;
                                    }
                                    // 然后尝试从全局作用域查找
                                    else if (ctx->global_scope && obj_type->as.struct_type.name) {
                                        CnSemSymbol *struct_sym = cn_sem_scope_lookup(ctx->global_scope,
                                                obj_type->as.struct_type.name, obj_type->as.struct_type.name_length);
                                        if (struct_sym && struct_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                                            struct_sym->type && struct_sym->type->as.struct_type.fields) {
                                            obj_type = struct_sym->type;
                                        }
                                    }
                                }
                                // 如果是结构体类型，查找成员类型
                                if (!new_type && obj_type->kind == CN_TYPE_STRUCT && obj_type->as.struct_type.fields) {
                                    const char *member_name = NULL;
                                    size_t member_name_length = 0;
                                    if (scan_inst->src2.kind == CN_IR_OP_SYMBOL) {
                                        member_name = scan_inst->src2.as.sym_name;
                                        member_name_length = strlen(member_name);
                                    }
                                    if (member_name) {
                                        // 遍历结构体成员查找类型
                                        // 使用 name_length 和 memcmp 比较，而不是 strcmp
                                        for (size_t f = 0; f < obj_type->as.struct_type.field_count; f++) {
                                            if (obj_type->as.struct_type.fields[f].name &&
                                                obj_type->as.struct_type.fields[f].name_length == member_name_length &&
                                                memcmp(obj_type->as.struct_type.fields[f].name, member_name, member_name_length) == 0) {
                                                new_type = obj_type->as.struct_type.fields[f].field_type;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        }
                        
                        // 【修复】对于 ADDRESS_OF 指令，目标寄存器类型应该是指针类型
                        if (!new_type && scan_inst->kind == CN_IR_INST_ADDRESS_OF) {
                            // ADDRESS_OF 指令：dest = &src1
                            // dest 的类型应该是指向 src1 类型的指针
                            CnType *src_type = NULL;
                            
                            // 从 src1.type 获取类型
                            if (scan_inst->src1.type) {
                                src_type = scan_inst->src1.type;
                            }
                            // 如果 src1 是寄存器，从 reg_types 中获取
                            else if (scan_inst->src1.kind == CN_IR_OP_REG &&
                                     scan_inst->src1.as.reg_id < actual_reg_count) {
                                src_type = reg_types[scan_inst->src1.as.reg_id];
                            }
                            
                            // 如果有源类型，创建指针类型
                            if (src_type) {
                                new_type = cn_type_new_pointer(src_type);
                            }
                            // 如果 dest.type 已经是指针类型，直接使用
                            else if (scan_inst->dest.type && scan_inst->dest.type->kind == CN_TYPE_POINTER) {
                                new_type = scan_inst->dest.type;
                            }
                        }
                        
                        /* 【P6修复】ADD/SUB指针算术类型推断
                         * 指针 + 整数 = 指针；整数 + 指针 = 指针
                         * 指针 - 整数 = 指针
                         * 这修复了 "char* + int 赋值给 long long int" 的类型不匹配错误
                         * 注意：使用"new_type可覆盖"条件而非!new_type，
                         * 因为dest.type可能已被设为INT，但指针类型更精确应覆盖
                         */
                        if ((scan_inst->kind == CN_IR_INST_ADD ||
                             scan_inst->kind == CN_IR_INST_SUB)) {
                            CnType *src1_type = NULL;
                            CnType *src2_type = NULL;
                            
                            /* 获取src1类型：优先从reg_types，其次从操作数自带类型 */
                            if (scan_inst->src1.kind == CN_IR_OP_REG &&
                                scan_inst->src1.as.reg_id < actual_reg_count) {
                                src1_type = reg_types[scan_inst->src1.as.reg_id];
                            }
                            if (!src1_type && scan_inst->src1.type) {
                                src1_type = scan_inst->src1.type;
                            }
                            /* 【P6增强】IMM_STR操作数视为char*类型 */
                            if (!src1_type && scan_inst->src1.kind == CN_IR_OP_IMM_STR) {
                                src1_type = cn_type_new_primitive(CN_TYPE_STRING);
                            }
                            
                            /* 获取src2类型 */
                            if (scan_inst->src2.kind == CN_IR_OP_REG &&
                                scan_inst->src2.as.reg_id < actual_reg_count) {
                                src2_type = reg_types[scan_inst->src2.as.reg_id];
                            }
                            if (!src2_type && scan_inst->src2.type) {
                                src2_type = scan_inst->src2.type;
                            }
                            if (!src2_type && scan_inst->src2.kind == CN_IR_OP_IMM_STR) {
                                src2_type = cn_type_new_primitive(CN_TYPE_STRING);
                            }
                            
                            /* 如果任一操作数是指针/字符串类型，结果也是该指针类型
                             * 仅当新类型比当前new_type更精确时才覆盖 */
                            CnType *pointer_result = NULL;
                            if (is_pointer_like_type(src1_type)) {
                                pointer_result = src1_type;
                            } else if (is_pointer_like_type(src2_type)) {
                                pointer_result = src2_type;
                            }
                            /* 指针类型可以覆盖INT/UNKNOWN/VOID/CHAR/BOOL类型 */
                            if (pointer_result && (!new_type ||
                                new_type->kind == CN_TYPE_INT ||
                                new_type->kind == CN_TYPE_UNKNOWN ||
                                new_type->kind == CN_TYPE_VOID ||
                                new_type->kind == CN_TYPE_CHAR ||
                                new_type->kind == CN_TYPE_BOOL)) {
                                new_type = pointer_result;
                            }
                        }
                        
                        /* 【P6修复】DEREF指令类型推断
                         * 解引用 *ptr 的结果类型是指针指向的类型
                         * 如果无法确定指向类型，保持指针类型（避免降级为int）
                         */
                        if (!new_type && scan_inst->kind == CN_IR_INST_DEREF) {
                            CnType *src_type = NULL;
                            if (scan_inst->src1.kind == CN_IR_OP_REG &&
                                scan_inst->src1.as.reg_id < actual_reg_count) {
                                src_type = reg_types[scan_inst->src1.as.reg_id];
                            }
                            if (!src_type && scan_inst->src1.type) {
                                src_type = scan_inst->src1.type;
                            }
                            /* 如果源是指针类型，解引用得到指向的类型 */
                            if (src_type && src_type->kind == CN_TYPE_POINTER &&
                                src_type->as.pointer_to) {
                                new_type = src_type->as.pointer_to;
                            }
                            /* 如果源是STRING类型(char*)，解引用得到char */
                            else if (src_type && src_type->kind == CN_TYPE_STRING) {
                                new_type = cn_type_new_primitive(CN_TYPE_CHAR);
                            }
                        }
                        
                        if (new_type) {
                            CnType *old_type = reg_types[reg_id];
                            
                            // 判断是否应该更新类型
                            // 重要：指针类型和结构体类型优先级高于INT类型，不应该被降级
                            bool should_update = false;
                            if (!old_type) {
                                // 之前没有类型，直接更新
                                should_update = true;
                                types_changed = true;  // 标记类型发生变化
                            } else if (new_type->kind == CN_TYPE_POINTER && old_type->kind != CN_TYPE_POINTER) {
                                // 新类型是指针类型，旧类型不是指针类型，更新
                                should_update = true;
                                types_changed = true;  // 标记类型发生变化
                            } else if (new_type->kind == CN_TYPE_STRUCT && old_type->kind != CN_TYPE_STRUCT) {
                                // 新类型是结构体类型，旧类型不是结构体类型，更新
                                should_update = true;
                                types_changed = true;  // 标记类型发生变化
                            }
                            // 防止指针类型被降级为INT类型
                            else if (old_type->kind == CN_TYPE_POINTER && new_type->kind != CN_TYPE_POINTER) {
                                // 旧类型是指针类型，新类型不是指针类型，不更新
                                should_update = false;
                            }
                            // 防止结构体类型被降级为INT类型
                            else if (old_type->kind == CN_TYPE_STRUCT && new_type->kind != CN_TYPE_STRUCT) {
                                // 旧类型是结构体类型，新类型不是结构体类型，不更新
                                should_update = false;
                            }
                            // 新增：如果新旧类型相同，也更新（确保类型信息正确传播）
                            else if (new_type->kind == old_type->kind) {
                                should_update = true;
                                // 不设置 types_changed，因为类型没有变化
                            }
                            // 新增：其他情况（如从INT到其他类型），允许更新
                            else {
                                should_update = true;
                                types_changed = true;
                            }
                            
                            // 【修复】对于LOAD指令，如果dest.type是指针类型，强制更新
                            // 这确保LOAD指令的目标寄存器类型正确传播
                            if (scan_inst->kind == CN_IR_INST_LOAD &&
                                scan_inst->dest.type &&
                                scan_inst->dest.type->kind == CN_TYPE_POINTER) {
                                should_update = true;
                                if (!old_type || old_type->kind != CN_TYPE_POINTER) {
                                    types_changed = true;
                                }
                            }
                            
                            // 【修复】对于CALL指令，如果dest.type是指针类型，强制更新
                            // 这确保动态数组索引访问返回的指针类型正确传播
                            // 例如：cn_rt_array_get_element 返回 void*，但实际类型应该是 符号*
                            if (scan_inst->kind == CN_IR_INST_CALL &&
                                scan_inst->dest.type &&
                                scan_inst->dest.type->kind == CN_TYPE_POINTER) {
                                should_update = true;
                                if (!old_type || old_type->kind != CN_TYPE_POINTER) {
                                    types_changed = true;
                                }
                            }
                            
                            // 【修复】对于MEMBER_ACCESS指令，如果dest.type是指针类型，强制更新
                            // 这确保成员访问的指针类型正确传播
                            if (scan_inst->kind == CN_IR_INST_MEMBER_ACCESS &&
                                scan_inst->dest.type &&
                                scan_inst->dest.type->kind == CN_TYPE_POINTER) {
                                should_update = true;
                                if (!old_type || old_type->kind != CN_TYPE_POINTER) {
                                    types_changed = true;
                                }
                            }
                            
                            // 【修复】对于MEMBER_ACCESS指令，如果dest.type是结构体类型，强制更新
                            // 这确保成员访问的结构体类型正确传播（如 cn_var_信息->位置 返回 struct 源位置）
                            if (scan_inst->kind == CN_IR_INST_MEMBER_ACCESS &&
                                scan_inst->dest.type &&
                                scan_inst->dest.type->kind == CN_TYPE_STRUCT) {
                                should_update = true;
                                if (!old_type || old_type->kind != CN_TYPE_STRUCT) {
                                    types_changed = true;
                                }
                            }
                            
                            // 【修复】对于MOV指令，如果dest.type是指针类型，强制更新
                            // 这确保MOV指令的指针类型正确传播（如 声明.作为函数声明 返回 函数声明*）
                            if (scan_inst->kind == CN_IR_INST_MOV &&
                                scan_inst->dest.type &&
                                scan_inst->dest.type->kind == CN_TYPE_POINTER) {
                                should_update = true;
                                if (!old_type || old_type->kind != CN_TYPE_POINTER) {
                                    types_changed = true;
                                }
                            }
                            
                            // 【修复】对于CALL指令，如果dest.type是指针类型，强制更新
                            // 这确保函数调用的指针返回类型正确传播
                            if (scan_inst->kind == CN_IR_INST_CALL &&
                                scan_inst->dest.type &&
                                scan_inst->dest.type->kind == CN_TYPE_POINTER) {
                                should_update = true;
                                if (!old_type || old_type->kind != CN_TYPE_POINTER) {
                                    types_changed = true;
                                }
                            }
                            
                            if (should_update) {
                                reg_types[reg_id] = new_type;
                            }
                        } else {
                            // 【修复】对于 MEMBER_ACCESS 指令，如果无法推断类型，不要设置默认类型
                            // 因为后续扫描可能从 src1.type 推断出正确的类型
                            // 只有非 MEMBER_ACCESS 指令才设置默认类型
                            if (scan_inst->kind != CN_IR_INST_MEMBER_ACCESS) {
                                // 修复：即使无法推断类型，也要标记寄存器为已使用（使用默认类型）
                                if (!reg_types[reg_id]) {
                                    reg_types[reg_id] = cn_type_new_primitive(CN_TYPE_INT);
                                    types_changed = true;
                                }
                            }
                        }
                    }
                }
                
                /* 【P6修复】STORE指令类型传播到ALLOCA变量
                 * 当STORE将值存入ALLOCA变量（符号）时，
                 * 如果源有指针/结构体/字符串类型，但ALLOCA变量是INT/CHAR类型，
                 * 则更新ALLOCA变量的类型。这解决了变量声明类型与赋值类型不匹配的问题。
                 *
                 * 与P1修复的区别：P1只在预扫描阶段处理src1.type已设置的情况，
                 * P6在多遍扫描中处理通过类型传播获得的reg_types类型和IMM_STR。
                 */
                if (scan_inst->kind == CN_IR_INST_STORE &&
                    scan_inst->dest.kind == CN_IR_OP_SYMBOL &&
                    scan_inst->dest.as.sym_name) {
                    
                    CnType *src_store_type = NULL;
                    
                    /* 从REG源获取类型 */
                    if (scan_inst->src1.kind == CN_IR_OP_REG &&
                        scan_inst->src1.as.reg_id < actual_reg_count) {
                        src_store_type = reg_types[scan_inst->src1.as.reg_id];
                    }
                    /* 【P6增强】从IMM_STR源获取char*类型 */
                    if (!src_store_type && scan_inst->src1.kind == CN_IR_OP_IMM_STR) {
                        src_store_type = cn_type_new_primitive(CN_TYPE_STRING);
                    }
                    /* 从src1.type直接获取 */
                    if (!src_store_type && scan_inst->src1.type) {
                        src_store_type = scan_inst->src1.type;
                    }
                    
                    /* 检查源类型是否为指针/结构体/字符串/枚举 */
                    bool src_is_precise = (src_store_type &&
                        (src_store_type->kind == CN_TYPE_POINTER ||
                         src_store_type->kind == CN_TYPE_STRUCT ||
                         src_store_type->kind == CN_TYPE_STRING ||
                         src_store_type->kind == CN_TYPE_ENUM));
                    
                    if (src_is_precise) {
                        const char *dest_name = scan_inst->dest.as.sym_name;
                        AllocaTypeEntry *entry = alloca_types;
                        while (entry) {
                            if (entry->sym_name && names_match_with_suffix(entry->sym_name, dest_name)) {
                                /* 判断是否应该更新ALLOCA类型 */
                                bool should_update_alloca = false;
                                if (!entry->type) {
                                    should_update_alloca = true;
                                }
                                /* 低精度类型(INT/VOID/UNKNOWN/CHAR/BOOL)可被任何精确类型覆盖 */
                                else if (entry->type->kind == CN_TYPE_INT ||
                                         entry->type->kind == CN_TYPE_VOID ||
                                         entry->type->kind == CN_TYPE_UNKNOWN ||
                                         entry->type->kind == CN_TYPE_CHAR ||
                                         entry->type->kind == CN_TYPE_BOOL) {
                                    should_update_alloca = true;
                                }
                                /* 【P6增强】POINTER类型可覆盖STRING类型
                                 * 例如：char* (STRING) → char** (POINTER) */
                                else if (src_store_type->kind == CN_TYPE_POINTER &&
                                         entry->type->kind == CN_TYPE_STRING) {
                                    should_update_alloca = true;
                                }
                                /* 【P6增强】POINTER类型可覆盖STRUCT类型
                                 * 例如：struct X (STRUCT) → struct X* (POINTER) */
                                else if (src_store_type->kind == CN_TYPE_POINTER &&
                                         entry->type->kind == CN_TYPE_STRUCT) {
                                    should_update_alloca = true;
                                }
                                /* 【P6增强】POINTER类型可覆盖ENUM类型 */
                                else if (src_store_type->kind == CN_TYPE_POINTER &&
                                         entry->type->kind == CN_TYPE_ENUM) {
                                    should_update_alloca = true;
                                }
                                
                                if (should_update_alloca) {
                                    entry->type = src_store_type;
                                    types_changed = true;
                                }
                                break;
                            }
                            entry = entry->next;
                        }
                    }
                }
                
                // 收集src1寄存器类型
                // 修复：即使类型为NULL，也要标记寄存器为已使用（使用默认类型）
                // 但不能覆盖已有的指针类型或结构体类型
                if (scan_inst->src1.kind == CN_IR_OP_REG) {
                    if (scan_inst->src1.as.reg_id < actual_reg_count) {
                        int src1_reg_id = scan_inst->src1.as.reg_id;
                        CnType *old_src1_type = reg_types[src1_reg_id];
                        if (scan_inst->src1.type) {
                            // 如果新类型是指针类型，或者之前没有类型，则更新
                            if (scan_inst->src1.type->kind == CN_TYPE_POINTER ||
                                !old_src1_type) {
                                reg_types[src1_reg_id] = scan_inst->src1.type;
                            }
                        } else if (!old_src1_type) {
                            // 类型为NULL，但寄存器被使用，标记为INT类型（默认）
                            reg_types[src1_reg_id] = cn_type_new_primitive(CN_TYPE_INT);
                            types_changed = true;
                        }
                        // 如果已有指针类型或结构体类型，不要用INT类型覆盖
                    }
                }
                // 收集src2寄存器类型
                if (scan_inst->src2.kind == CN_IR_OP_REG) {
                    if (scan_inst->src2.as.reg_id < actual_reg_count) {
                        int src2_reg_id = scan_inst->src2.as.reg_id;
                        CnType *old_src2_type = reg_types[src2_reg_id];
                        if (scan_inst->src2.type) {
                            // 如果新类型是指针类型，或者之前没有类型，则更新
                            if (scan_inst->src2.type->kind == CN_TYPE_POINTER ||
                                !old_src2_type) {
                                reg_types[src2_reg_id] = scan_inst->src2.type;
                            }
                        } else if (!old_src2_type) {
                            // 类型为NULL，但寄存器被使用，标记为INT类型（默认）
                            reg_types[src2_reg_id] = cn_type_new_primitive(CN_TYPE_INT);
                            types_changed = true;
                        }
                    }
                }
                // 收集extra_args中的寄存器类型
                for (size_t i = 0; i < scan_inst->extra_args_count; i++) {
                    if (scan_inst->extra_args[i].kind == CN_IR_OP_REG) {
                        if (scan_inst->extra_args[i].as.reg_id < actual_reg_count) {
                            int extra_reg_id = scan_inst->extra_args[i].as.reg_id;
                            CnType *old_extra_type = reg_types[extra_reg_id];
                            if (scan_inst->extra_args[i].type) {
                                // 如果新类型是指针类型，或者之前没有类型，则更新
                                if (scan_inst->extra_args[i].type->kind == CN_TYPE_POINTER ||
                                    !old_extra_type) {
                                    reg_types[extra_reg_id] = scan_inst->extra_args[i].type;
                                }
                            } else if (!old_extra_type) {
                                // 类型为NULL，但寄存器被使用，标记为INT类型（默认）
                                reg_types[extra_reg_id] = cn_type_new_primitive(CN_TYPE_INT);
                                types_changed = true;
                            }
                        }
                    }
                }
                scan_inst = scan_inst->next;
            }
            scan_block = scan_block->next;
        }
        }  // 结束多遍扫描循环
        
        /* 【P6修复】将更新后的alloca_types回写到ALLOCA指令的dest.type
         * 多遍扫描可能通过STORE指令类型传播更新了alloca_types中的类型，
         * 需要将这些更新反映到实际的ALLOCA指令中，这样在后续生成
         * ALLOCA指令的变量声明时（cn_cgen_inst中的CN_IR_INST_ALLOCA分支），
         * 会使用正确的类型字符串。
         */
        {
            AllocaTypeEntry *writeback_entry = alloca_types;
            while (writeback_entry) {
                if (writeback_entry->sym_name && writeback_entry->type) {
                    /* 在函数的所有基本块中查找匹配的ALLOCA指令 */
                    CnIrBasicBlock *wb_block = func->first_block;
                    while (wb_block) {
                        CnIrInst *wb_inst = wb_block->first_inst;
                        while (wb_inst) {
                            if (wb_inst->kind == CN_IR_INST_ALLOCA &&
                                wb_inst->dest.kind == CN_IR_OP_SYMBOL &&
                                wb_inst->dest.as.sym_name &&
                                names_match_with_suffix(wb_inst->dest.as.sym_name, writeback_entry->sym_name)) {
                                /* 判断是否应该更新ALLOCA指令的类型 */
                                bool should_wb_update = false;
                                if (!wb_inst->dest.type) {
                                    should_wb_update = true;
                                }
                                /* 低精度类型可被任何精确类型覆盖 */
                                else if (wb_inst->dest.type->kind == CN_TYPE_INT ||
                                         wb_inst->dest.type->kind == CN_TYPE_VOID ||
                                         wb_inst->dest.type->kind == CN_TYPE_UNKNOWN ||
                                         wb_inst->dest.type->kind == CN_TYPE_CHAR ||
                                         wb_inst->dest.type->kind == CN_TYPE_BOOL) {
                                    should_wb_update = true;
                                }
                                /* 【P6增强】POINTER类型可覆盖STRING/STRUCT/ENUM
                                 * 例如：char*(STRING) → char**(POINTER)
                                 * 例如：struct X(STRUCT) → struct X*(POINTER) */
                                else if (writeback_entry->type->kind == CN_TYPE_POINTER &&
                                         (wb_inst->dest.type->kind == CN_TYPE_STRING ||
                                          wb_inst->dest.type->kind == CN_TYPE_STRUCT ||
                                          wb_inst->dest.type->kind == CN_TYPE_ENUM)) {
                                    should_wb_update = true;
                                }
                                
                                if (should_wb_update) {
                                    wb_inst->dest.type = writeback_entry->type;
                                }
                                break;
                            }
                            wb_inst = wb_inst->next;
                        }
                        wb_block = wb_block->next;
                    }
                }
                writeback_entry = writeback_entry->next;
            }
        }
        
        /* 性能优化：按类型分组声明寄存器，减少 fprintf 调用 */
        /* 修复：确保所有使用的寄存器都被声明，即使类型信息缺失 */
        bool has_int_regs = false;
        for (int i = 0; i < actual_reg_count; i++) {
            // 【调试】输出最终类型
            // 对于NULL、CN_TYPE_VOID、CN_TYPE_INT或CN_TYPE_UNKNOWN类型，都使用long long
            // 注意：NULL类型表示类型信息缺失，VOID类型可能是函数调用返回值未正确设置，但仍需声明寄存器
            if (!reg_types[i] || reg_types[i]->kind == CN_TYPE_VOID || reg_types[i]->kind == CN_TYPE_INT || reg_types[i]->kind == CN_TYPE_UNKNOWN) {
                if (!has_int_regs) {
                    fprintf(ctx->output_file, "  long long ");
                    has_int_regs = true;
                } else {
                    fprintf(ctx->output_file, ", ");
                }
                fprintf(ctx->output_file, "r%d", i);
            }
        }
        if (has_int_regs) fprintf(ctx->output_file, ";\n");
        
        /* 声明字符串寄存器（每个寄存器单独声明以确保每个都是指针类型） */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_STRING) {
                fprintf(ctx->output_file, "  char* r%d;\n", i);
            }
        }
        
        /* 声明数组寄存器（使用元素指针类型，而非void*） */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_ARRAY) {
                // 数组类型应该声明为元素指针类型，以便支持指针算术
                // 例如：关键字条目[40] 应该声明为 struct 关键字条目*
                fprintf(ctx->output_file, "  %s r%d;\n", get_c_type_string(reg_types[i]), i);
            }
        }
        
        /* 声明指针寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_POINTER) {
                fprintf(ctx->output_file, "  %s r%d;\n", get_c_type_string(reg_types[i]), i);
            }
        }
        
        /* 声明布尔寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_BOOL) {
                fprintf(ctx->output_file, "  _Bool r%d;\n", i);
            }
        }
        
        /* 声明字符寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_CHAR) {
                fprintf(ctx->output_file, "  char r%d;\n", i);
            }
        }
        
        /* 声明浮点寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_FLOAT) {
                fprintf(ctx->output_file, "  double r%d;\n", i);
            }
        }
        
        /* 声明结构体寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_STRUCT) {
                fprintf(ctx->output_file, "  %s r%d;\n", get_c_type_string(reg_types[i]), i);
            }
        }
        
        /* 声明枚举寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_ENUM) {
                fprintf(ctx->output_file, "  %s r%d;\n", get_c_type_string(reg_types[i]), i);
            }
        }
        
        // 将reg_types保存到ctx中，供指令生成时使用
        // 注意：如果是栈分配的reg_types，需要复制到堆上以便在函数生成期间保持有效
        if (!use_heap && actual_reg_count > 0) {
            // 栈分配的reg_types需要复制到堆上
            CnType **heap_reg_types = malloc(actual_reg_count * sizeof(CnType*));
            if (heap_reg_types) {
                memcpy(heap_reg_types, reg_types, actual_reg_count * sizeof(CnType*));
                ctx->reg_types = heap_reg_types;
                ctx->reg_types_count = actual_reg_count;
            }
        } else {
            ctx->reg_types = reg_types;
            ctx->reg_types_count = actual_reg_count;
        }
        
        // 【P8修复】将alloca_types保存到ctx中，供指令生成时MEMBER_ACCESS使用
        // 延迟释放：在指令生成完成后才释放
        ctx->alloca_types = alloca_types;
        
    skip_register_type_scan:;
    }

    CnIrBasicBlock *block = func->first_block;
    while (block) { cn_cgen_block(ctx, block); block = block->next; }
    
    // 清除ctx中的reg_types（总是释放，因为要么是堆分配的，要么是从栈复制的）
    if (ctx->reg_types) {
        free(ctx->reg_types);
    }
    ctx->reg_types = NULL;
    ctx->reg_types_count = 0;
    
    // 【P8修复】释放alloca_types映射表（延迟到指令生成完成后）
    if (ctx->alloca_types) {
        AllocaTypeEntry *entry = (AllocaTypeEntry *)ctx->alloca_types;
        while (entry) {
            AllocaTypeEntry *next = entry->next;
            free(entry);
            entry = next;
        }
        ctx->alloca_types = NULL;
    }

    // 检查函数是否以返回语句结束
    bool has_return = false;
    if (func->last_block && func->last_block->last_inst) {
        has_return = (func->last_block->last_inst->kind == CN_IR_INST_RET);
    }

    // 如果函数没有返回语句，添加默认返回
    if (!has_return) {
        // 中断处理函数默认返回0
        if (func->is_interrupt_handler) {
            fprintf(ctx->output_file, "  return 0;\n");
        }
        // 根据返回类型添加默认返回
        else if (func->return_type) {
            switch (func->return_type->kind) {
                case CN_TYPE_VOID:
                    // void函数不需要返回值
                    fprintf(ctx->output_file, "  return;\n");
                    break;
                case CN_TYPE_INT:
                case CN_TYPE_INT32:
                case CN_TYPE_INT64:
                case CN_TYPE_UINT32:
                case CN_TYPE_UINT64_LL:
                case CN_TYPE_BOOL:
                case CN_TYPE_CHAR:
                    fprintf(ctx->output_file, "  return 0;\n");
                    break;
                case CN_TYPE_FLOAT:
                case CN_TYPE_FLOAT32:
                case CN_TYPE_FLOAT64:
                    fprintf(ctx->output_file, "  return 0.0;\n");
                    break;
                case CN_TYPE_STRING:
                case CN_TYPE_POINTER:
                    fprintf(ctx->output_file, "  return NULL;\n");
                    break;
                case CN_TYPE_STRUCT:
                    // 结构体类型：返回零初始化的结构体
                    fprintf(ctx->output_file, "  {\n");
                    fprintf(ctx->output_file, "    static %s _zero = {0};\n", get_c_type_string(func->return_type));
                    fprintf(ctx->output_file, "    return _zero;\n");
                    fprintf(ctx->output_file, "  }\n");
                    break;
                case CN_TYPE_ENUM:
                    // 枚举类型：返回0
                    fprintf(ctx->output_file, "  return 0;\n");
                    break;
                default:
                    // 其他类型默认返回0
                    fprintf(ctx->output_file, "  return 0;\n");
                    break;
            }
        } else {
            // 无返回类型声明的函数，默认返回0
            fprintf(ctx->output_file, "  return 0;\n");
        }
    }

    // 注入运行时退出（仅 hosted 模式）
    if (is_main && ctx->module && ctx->module->compile_mode != CN_COMPILE_MODE_FREESTANDING) {
        fprintf(ctx->output_file, "  cn_rt_exit();\n");
    }
    
    fprintf(ctx->output_file, "}\n\n");
}

// 用于存储局部结构体与其所属函数的映射关系
typedef struct {
    CnAstStmt *struct_stmt;      // 结构体定义语句
    const char *function_name;   // 所属函数名
    size_t function_name_length;
    const char *struct_name;     // 结构体名
    size_t struct_name_length;
} LocalStructInfo;

// 全局局部结构体信息表（用于 C 代码生成时查找）
static LocalStructInfo *g_local_struct_infos = NULL;
static size_t g_local_struct_count = 0;

// 查找局部结构体信息
static LocalStructInfo *find_local_struct_info(const char *struct_name, size_t struct_name_len) {
    for (size_t i = 0; i < g_local_struct_count; i++) {
        if (g_local_struct_infos[i].struct_name_length == struct_name_len &&
            strncmp(g_local_struct_infos[i].struct_name, struct_name, struct_name_len) == 0) {
            return &g_local_struct_infos[i];
        }
    }
    return NULL;
}

// 递归收集函数体内的结构体定义，记录所属函数
static void collect_local_structs_from_block(CnAstBlockStmt *block, LocalStructInfo **struct_infos, size_t *count, size_t *capacity, const char *func_name, size_t func_name_len) {
    if (!block || !struct_infos || !count || !capacity) return;
    
    for (size_t i = 0; i < block->stmt_count; i++) {
        CnAstStmt *stmt = block->stmts[i];
        if (!stmt) continue;
        
        // 如果是结构体定义，添加到列表并记录所属函数
        if (stmt->kind == CN_AST_STMT_STRUCT_DECL) {
            if (*count >= *capacity) {
                *capacity = (*capacity == 0) ? 4 : (*capacity * 2);
                LocalStructInfo *new_array = (LocalStructInfo *)realloc(*struct_infos, *capacity * sizeof(LocalStructInfo));
                if (!new_array) return;
                *struct_infos = new_array;
            }
            (*struct_infos)[*count].struct_stmt = stmt;
            (*struct_infos)[*count].function_name = func_name;
            (*struct_infos)[*count].function_name_length = func_name_len;
            // 保存结构体名称
            (*struct_infos)[*count].struct_name = stmt->as.struct_decl.name;
            (*struct_infos)[*count].struct_name_length = stmt->as.struct_decl.name_length;
            (*count)++;
        }
        
        // 递归处理嵌套块
        if (stmt->kind == CN_AST_STMT_IF) {
            collect_local_structs_from_block(stmt->as.if_stmt.then_block, struct_infos, count, capacity, func_name, func_name_len);
            collect_local_structs_from_block(stmt->as.if_stmt.else_block, struct_infos, count, capacity, func_name, func_name_len);
        } else if (stmt->kind == CN_AST_STMT_WHILE) {
            collect_local_structs_from_block(stmt->as.while_stmt.body, struct_infos, count, capacity, func_name, func_name_len);
        } else if (stmt->kind == CN_AST_STMT_FOR) {
            collect_local_structs_from_block(stmt->as.for_stmt.body, struct_infos, count, capacity, func_name, func_name_len);
        } else if (stmt->kind == CN_AST_STMT_SWITCH) {
            for (size_t j = 0; j < stmt->as.switch_stmt.case_count; j++) {
                collect_local_structs_from_block(stmt->as.switch_stmt.cases[j].body, struct_infos, count, capacity, func_name, func_name_len);
            }
        }
    }
}

// 从函数声明中收集局部结构体定义
static void collect_local_structs_from_function(CnAstFunctionDecl *func, LocalStructInfo **struct_infos, size_t *count, size_t *capacity) {
    if (!func || !func->body) return;
    collect_local_structs_from_block(func->body, struct_infos, count, capacity, func->name, func->name_length);
}

// 生成结构体定义（从 AST 结构体声明）
// 如果提供了函数名前缀，则生成局部结构体的唯一名称
void cn_cgen_struct_decl_with_prefix(CnCCodeGenContext *ctx, CnAstStmt *struct_stmt, const char *func_prefix, size_t func_prefix_len) {
    if (!ctx || !struct_stmt || struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) return;
    
    CnAstStructDecl *decl = &struct_stmt->as.struct_decl;
    
    // 【修复】对于全局结构体，检查是否已生成（避免与导入模块的同名结构体重定义）
    // 局部结构体（有函数前缀）名称已唯一化，不需要检查
    if (!func_prefix || func_prefix_len == 0) {
        if (is_type_already_generated(decl->name, decl->name_length)) {
            return;  // 已生成，跳过
        }
        // 标记为已生成
        mark_type_as_generated(decl->name, decl->name_length);
    }
    
    // 生成结构体定义
    // 注意：前向声明已在第一遍扫描中统一输出，此处不再重复输出
    // 如果有函数前缀，生成: struct __local_函数名_结构体名
    // 否则生成: struct 结构体名
    if (func_prefix && func_prefix_len > 0) {
        fprintf(ctx->output_file, "struct __local_%.*s_%.*s {\n",
                (int)func_prefix_len, func_prefix,
                (int)decl->name_length, decl->name);
    } else {
        fprintf(ctx->output_file, "struct %.*s {\n",
                (int)decl->name_length, decl->name);
    }
    
    // 生成字段
    for (size_t i = 0; i < decl->field_count; i++) {
        // 获取字段类型
        CnType *field_type = decl->fields[i].field_type;
        
        // 如果字段类型是结构体类型，需要检查是否实际上是枚举类型
        // （因为解析器将所有自定义类型都创建为结构体类型）
        if (field_type && field_type->kind == CN_TYPE_STRUCT &&
            field_type->as.struct_type.name && ctx->program) {
            // 在程序的枚举列表中查找是否有同名枚举
            for (size_t j = 0; j < ctx->program->enum_count; j++) {
                CnAstStmt *enum_stmt = ctx->program->enums[j];
                if (enum_stmt && enum_stmt->kind == CN_AST_STMT_ENUM_DECL) {
                    CnAstEnumDecl *enum_decl = &enum_stmt->as.enum_decl;
                    if (enum_decl->name_length == field_type->as.struct_type.name_length &&
                        memcmp(enum_decl->name, field_type->as.struct_type.name,
                               enum_decl->name_length) == 0) {
                        // 找到匹配的枚举，创建枚举类型
                        field_type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
                        break;
                    }
                }
            }
        }
        
        fprintf(ctx->output_file, "    %s %.*s;\n",
                get_c_type_string(field_type),
                (int)decl->fields[i].name_length,
                decl->fields[i].name);
    }
    
    fprintf(ctx->output_file, "};\n\n");
}

// 兼容旧接口：生成全局结构体定义
void cn_cgen_struct_decl(CnCCodeGenContext *ctx, CnAstStmt *struct_stmt) {
    cn_cgen_struct_decl_with_prefix(ctx, struct_stmt, NULL, 0);
}

// 前向声明：类型去重检查函数（定义在后面）
static bool is_type_already_generated(const char *name, size_t name_len);
static void mark_type_as_generated(const char *name, size_t name_len);
static void mark_enum_type_as_generated(const char *name, size_t name_len);

// 生成枚举定义（从 AST 枚举声明）
// 为枚举成员自动添加枚举类型名称作为前缀，避免C语言中枚举成员全局命名空间冲突
void cn_cgen_enum_decl(CnCCodeGenContext *ctx, CnAstStmt *enum_stmt) {
    if (!ctx || !enum_stmt || enum_stmt->kind != CN_AST_STMT_ENUM_DECL) return;
    
    CnAstEnumDecl *decl = &enum_stmt->as.enum_decl;
    
    // 【修复】检查枚举是否已生成（避免与导入模块的同名枚举重定义）
    if (is_type_already_generated(decl->name, decl->name_length)) {
        return;  // 已生成，跳过
    }
    
    // 标记为已生成
    mark_type_as_generated(decl->name, decl->name_length);
    mark_enum_type_as_generated(decl->name, decl->name_length);
    
    // 生成枚举定义
    fprintf(ctx->output_file, "enum %.*s {\n",
            (int)decl->name_length, decl->name);
    
    // 生成枚举成员（添加枚举类型名作为前缀）
    for (size_t i = 0; i < decl->member_count; i++) {
        // 输出格式：枚举类型名_成员名
        fprintf(ctx->output_file, "    %.*s_%.*s",
                (int)decl->name_length, decl->name,
                (int)decl->members[i].name_length,
                decl->members[i].name);
        
        // 如果有显式值，生成赋值
        if (decl->members[i].has_value) {
            fprintf(ctx->output_file, " = %ld", decl->members[i].value);
        }
        
        // 添加逗号（最后一个除外）
        if (i < decl->member_count - 1) {
            fprintf(ctx->output_file, ",\n");
        } else {
            fprintf(ctx->output_file, "\n");
        }
    }
    
    fprintf(ctx->output_file, "};\n\n");
}

int cn_cgen_module_to_file(CnIrModule *module, const char *filename) {
    return cn_cgen_module_with_structs_to_file(module, NULL, filename);
}

// 完整的代码生成函数，包含结构体定义
int cn_cgen_module_with_structs_to_file(CnIrModule *module, CnAstProgram *program, const char *filename) {
    if (!module || !filename) return -1;

    /* 根据 IR 模块上的目标三元组获取预设数据布局（若存在）。 */
    CnTargetDataLayout layout;
    bool layout_ok = cn_support_target_get_data_layout(&module->target, &layout);
    if (layout_ok) {
        g_target_layout = layout;
        g_target_layout_valid = true;
    } else {
        g_target_layout_valid = false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) return -1;
    
    /* 性能优化：设置较大的缓冲区，减少系统调用 */
    char *buffer = malloc(CGEN_BUFFER_SIZE);
    if (buffer) {
        setvbuf(file, buffer, _IOFBF, CGEN_BUFFER_SIZE);
    }
    
    CnCCodeGenContext ctx = {0};
    ctx.output_file = file;
    ctx.module = module;
    ctx.program = program;  // 设置程序AST，用于查找类定义

    // 根据编译模式生成不同的头文件
    if (module->compile_mode == CN_COMPILE_MODE_FREESTANDING) {
        // Freestanding 模式：不包含运行时库头文件，但声明必要的运行时函数
        fprintf(file, "#include <stddef.h>\n#include <stdbool.h>\n#include <stdint.h>\n\n");
        fprintf(file, "// CN Language Runtime Function Declarations (Freestanding Mode)\n");
        fprintf(file, "void cn_rt_print_string(const char *str);\n");
        fprintf(file, "void cn_rt_print_int(long long val);\n");
        fprintf(file, "void cn_rt_print_float(double val);\n");
        fprintf(file, "void cn_rt_print_bool(int val);\n");
        fprintf(file, "size_t cn_rt_string_length(const char *str);\n");
        fprintf(file, "char* cn_rt_string_concat(const char *a, const char *b);\n");
        fprintf(file, "char* cn_rt_int_to_string(long long val);\n");
        fprintf(file, "char* cn_rt_bool_to_string(int val);\n");
        fprintf(file, "char* cn_rt_float_to_string(double val);\n");
        fprintf(file, "void* cn_rt_array_alloc(size_t elem_size, size_t count);\n");
        fprintf(file, "size_t cn_rt_array_length(void *arr);\n");
        fprintf(file, "int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size);\n");
        fprintf(file, "\n");
    } else {
        // Hosted 模式：包含完整运行时库
        fprintf(file, "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include \"cnrt.h\"\n");
        // 包含系统API头文件(替代已删除关键字的运行时函数)
        fprintf(file, "#include \"cnlang/runtime/system_api.h\"\n\n");
    }
    
    // 生成结构体定义（如果提供了 AST）
    // 首先收集全局结构体和局部结构体
    LocalStructInfo *local_struct_infos = NULL;
    size_t local_struct_count = 0;
    size_t local_struct_capacity = 0;
    
    if (program) {
        // =============================================================================
        // 先输出枚举定义（必须在结构体之前，因为结构体可能使用枚举类型）
        // =============================================================================
        if (program->enum_count > 0) {
            fprintf(file, "// CN Language Enum Definitions\n");
            for (size_t i = 0; i < program->enum_count; i++) {
                cn_cgen_enum_decl(&ctx, program->enums[i]);
            }
        }
        
        // =============================================================================
        // 输出全局结构体定义（在枚举之后）
        // 采用两遍扫描：第一遍输出前向声明，第二遍输出完整定义
        // 这样可以解决结构体之间的依赖问题
        // =============================================================================
        if (program->struct_count > 0) {
            // 第一遍：输出所有结构体的前向声明
            fprintf(file, "// CN Language Global Struct Forward Declarations\n");
            for (size_t i = 0; i < program->struct_count; i++) {
                CnAstStmt *struct_stmt = program->structs[i];
                if (struct_stmt && struct_stmt->kind == CN_AST_STMT_STRUCT_DECL) {
                    CnAstStructDecl *decl = &struct_stmt->as.struct_decl;
                    fprintf(file, "struct %.*s;\n", (int)decl->name_length, decl->name);
                }
            }
            fprintf(file, "\n");
            
            // 第二遍：输出所有结构体的完整定义
            fprintf(file, "// CN Language Global Struct Definitions\n");
            for (size_t i = 0; i < program->struct_count; i++) {
                cn_cgen_struct_decl(&ctx, program->structs[i]);
            }
        }
        
        // 收集所有函数中的局部结构体（带函数名信息）
        for (size_t i = 0; i < program->function_count; i++) {
            collect_local_structs_from_function(program->functions[i], &local_struct_infos, &local_struct_count, &local_struct_capacity);
        }
        
        // 输出局部结构体定义（提升到全局，但使用特殊命名）
        // 同样采用两遍扫描：先输出前向声明，再输出完整定义
        if (local_struct_count > 0) {
            // 第一遍：输出所有局部结构体的前向声明
            fprintf(file, "// CN Language Local Struct Forward Declarations (hoisted for C compatibility)\n");
            for (size_t i = 0; i < local_struct_count; i++) {
                CnAstStmt *struct_stmt = local_struct_infos[i].struct_stmt;
                const char *func_name = local_struct_infos[i].function_name;
                size_t func_name_len = local_struct_infos[i].function_name_length;
                if (struct_stmt && struct_stmt->kind == CN_AST_STMT_STRUCT_DECL) {
                    CnAstStructDecl *decl = &struct_stmt->as.struct_decl;
                    fprintf(file, "struct __local_%.*s_%.*s;\n",
                            (int)func_name_len, func_name,
                            (int)decl->name_length, decl->name);
                }
            }
            fprintf(file, "\n");
            
            // 第二遍：输出所有局部结构体的完整定义
            fprintf(file, "// CN Language Local Struct Definitions (hoisted for C compatibility)\n");
            fprintf(file, "// Note: These are local to their respective functions in CN semantics\n");
            for (size_t i = 0; i < local_struct_count; i++) {
                cn_cgen_struct_decl_with_prefix(&ctx,
                                               local_struct_infos[i].struct_stmt,
                                               local_struct_infos[i].function_name,
                                               local_struct_infos[i].function_name_length);
            }
            
            // 设置全局查找表
            g_local_struct_infos = local_struct_infos;
            g_local_struct_count = local_struct_count;
        }
    }
    
    // 生成接口定义（如果提供了 AST）- 必须在类定义之前，因为类的vtable引用接口vtable
    if (program && program->interface_count > 0) {
        fprintf(file, "// CN Language Interface Definitions\n\n");
        for (size_t i = 0; i < program->interface_count; i++) {
            CnAstStmt *interface_stmt = program->interfaces[i];
            if (interface_stmt && interface_stmt->kind == CN_AST_STMT_INTERFACE_DECL) {
                cn_cgen_interface_decl(&ctx, interface_stmt->as.interface_decl);
            }
        }
    }
    
    // 生成类定义（如果提供了 AST）- 阶段11 面向对象编程支持
    if (program && program->class_count > 0) {
        fprintf(file, "// CN Language Class Definitions\n\n");
        for (size_t i = 0; i < program->class_count; i++) {
            CnAstStmt *class_stmt = program->classes[i];
            if (class_stmt && class_stmt->kind == CN_AST_STMT_CLASS_DECL) {
                cn_cgen_class_decl(&ctx, class_stmt->as.class_decl);
            }
        }
    }
    
    // 生成全局变量声明
    fprintf(file, "// Global Variables\n");
    CnIrGlobalVar *global = module->first_global;
    while (global) {
        // 【修复】检查是否为静态数组类型，生成正确的C数组语法
        if (global->type && global->type->kind == CN_TYPE_ARRAY && global->type->as.array.length > 0) {
            // 静态数组：生成 "元素类型 变量名[长度];"
            fprintf(file, "%s cn_var_%s[%zu]",
                    get_c_type_string(global->type->as.array.element_type),
                    global->name,
                    global->type->as.array.length);
        } else {
            // 非数组类型：正常生成
            fprintf(file, "%s cn_var_%s",
                    get_c_type_string(global->type),
                    global->name);
            
            // 生成初始化值
            if (global->initializer.kind != CN_IR_OP_NONE) {
                fprintf(file, " = ");
                if (global->initializer.kind == CN_IR_OP_IMM_INT) {
                    fprintf(file, "%lld", global->initializer.as.imm_int);
                } else if (global->initializer.kind == CN_IR_OP_IMM_FLOAT) {
                    fprintf(file, "%f", global->initializer.as.imm_float);
                } else if (global->initializer.kind == CN_IR_OP_IMM_STR) {
                    // 字符串字面量初始化
                    fprintf(file, "\"%s\"", global->initializer.as.imm_str ? global->initializer.as.imm_str : "");
                } else if (global->initializer.kind == CN_IR_OP_AST_EXPR && global->initializer.as.ast_expr) {
                    // 结构体字面量初始化
                    CnAstExpr *struct_lit = global->initializer.as.ast_expr;
                    // 生成结构体字面量初始化
                    if (struct_lit->kind == CN_AST_EXPR_STRUCT_LITERAL) {
                        if (struct_lit->as.struct_lit.struct_name && struct_lit->as.struct_lit.struct_name_length > 0) {
                            fprintf(file, "(struct %.*s){",
                                    (int)struct_lit->as.struct_lit.struct_name_length,
                                    struct_lit->as.struct_lit.struct_name);
                        } else {
                            fprintf(file, "{");
                        }
                        for (size_t i = 0; i < struct_lit->as.struct_lit.field_count; i++) {
                            if (i > 0) {
                                fprintf(file, ", ");
                            }
                            // 生成字段值
                            CnAstExpr *field_value = struct_lit->as.struct_lit.fields[i].value;
                            if (field_value) {
                                if (field_value->kind == CN_AST_EXPR_INTEGER_LITERAL) {
                                    fprintf(file, "%lld", field_value->as.integer_literal.value);
                                } else if (field_value->kind == CN_AST_EXPR_FLOAT_LITERAL) {
                                    fprintf(file, "%f", field_value->as.float_literal.value);
                                } else if (field_value->kind == CN_AST_EXPR_STRING_LITERAL) {
                                    fprintf(file, "\"%s\"", field_value->as.string_literal.value ? field_value->as.string_literal.value : "");
                                } else if (field_value->kind == CN_AST_EXPR_CHAR_LITERAL) {
                                    fprintf(file, "'%c'", field_value->as.char_literal.value);
                                } else if (field_value->kind == CN_AST_EXPR_BOOL_LITERAL) {
                                    fprintf(file, "%d", field_value->as.bool_literal.value ? 1 : 0);
                                } else if (field_value->kind == CN_AST_EXPR_IDENTIFIER) {
                                    // 检查是否为 "无" (NULL)
                                    if (field_value->as.identifier.name_length == 1 &&
                                        field_value->as.identifier.name[0] == '无') {
                                        fprintf(file, "NULL");
                                    } else {
                                        fprintf(file, "0");  // 其他标识符默认为0
                                    }
                                } else {
                                    fprintf(file, "0");  // 其他类型默认为0
                                }
                            }
                        }
                        fprintf(file, "}");
                    } else {
                        fprintf(file, "0");  // 无效的结构体字面量，使用0初始化
                    }
                }
            }
        }
        
        fprintf(file, ";\n");
        global = global->next;
    }
    fprintf(file, "\n");
    
    // 生成函数前置声明（Forward Declarations）
    fprintf(file, "// Forward Declarations\n");
    CnIrFunction *func = module->first_func;
    
    // 首先收集所有需要 #undef 的运行时宏冲突函数名
    // 在前向声明之前统一生成 #undef，避免宏展开导致重定义
    bool has_macro_conflict = false;
    CnIrFunction *temp_func = func;
    while (temp_func) {
        if (is_runtime_macro_conflict(temp_func->name)) {
            if (!has_macro_conflict) {
                fprintf(file, "// 取消运行时宏定义，避免与用户函数名冲突\n");
                has_macro_conflict = true;
            }
            fprintf(file, "#undef %s\n", temp_func->name);
        }
        temp_func = temp_func->next;
    }
    if (has_macro_conflict) {
        fprintf(file, "\n");
    }
    
    // 生成前向声明（跳过运行时库已定义的函数）
    while (func) {
        // 跳过与运行时库函数冲突的函数声明
        if (is_runtime_function_conflict(func->name)) {
            func = func->next;
            continue;
        }
        fprintf(file, "%s %s(", get_c_type_string(func->return_type), get_c_function_name(func->name));
        for (size_t i = 0; i < func->param_count; i++) {
            fprintf(file, "%s", get_c_param_type_string(func->params[i].type));
            if (i < func->param_count - 1) fprintf(file, ", ");
        }
        fprintf(file, ");\n");
        func = func->next;
    }
    fprintf(file, "\n");
    
    // 【P5修复】扫描所有CALL指令，收集被调用但未在当前模块声明的函数
    // 生成ANSI原型风格的extern声明，包含参数类型信息
    // 解决K&R风格声明(extern type name();)导致的"函数参数数量不匹配"GCC错误
    {
        #define MAX_EXTERN_FUNCS_A 512
        const char *extern_func_names_a[MAX_EXTERN_FUNCS_A];
        CnType *extern_func_ret_types_a[MAX_EXTERN_FUNCS_A];
        CnIrInst *extern_call_insts_a[MAX_EXTERN_FUNCS_A]; // 【P5】保存CALL指令以获取参数信息
        size_t extern_func_count_a = 0;
        
        CnIrFunction *scan_func_a = module->first_func;
        while (scan_func_a) {
            CnIrBasicBlock *scan_block_a = scan_func_a->first_block;
            while (scan_block_a) {
                CnIrInst *scan_inst_a = scan_block_a->first_inst;
                while (scan_inst_a) {
                    if (scan_inst_a->kind == CN_IR_INST_CALL &&
                        scan_inst_a->src1.kind == CN_IR_OP_SYMBOL &&
                        scan_inst_a->src1.as.sym_name) {
                        const char *called_name = scan_inst_a->src1.as.sym_name;
                        // 跳过运行时库函数（cn_rt_前缀或与运行时库冲突的函数名）
                        if (strncmp(called_name, "cn_rt_", 6) == 0 ||
                            is_runtime_function_conflict(called_name) ||
                            is_runtime_macro_conflict(called_name)) {
                            scan_inst_a = scan_inst_a->next;
                            continue;
                        }
                        // 检查是否已在当前模块的函数列表中声明
                        bool in_module_a = false;
                        CnIrFunction *cf = module->first_func;
                        while (cf) {
                            if (cf->name && strcmp(cf->name, called_name) == 0) {
                                in_module_a = true;
                                break;
                            }
                            cf = cf->next;
                        }
                        if (in_module_a) {
                            scan_inst_a = scan_inst_a->next;
                            continue;
                        }
                        // 检查是否已收集到extern列表中
                        size_t existing_idx_a = 0;
                        bool in_extern_a = false;
                        for (size_t ei = 0; ei < extern_func_count_a; ei++) {
                            if (strcmp(extern_func_names_a[ei], called_name) == 0) {
                                in_extern_a = true;
                                existing_idx_a = ei;
                                break;
                            }
                        }
                        if (!in_extern_a && extern_func_count_a < MAX_EXTERN_FUNCS_A) {
                            // 新函数，添加到extern列表
                            extern_func_names_a[extern_func_count_a] = called_name;
                            extern_func_ret_types_a[extern_func_count_a] =
                                (scan_inst_a->dest.kind == CN_IR_OP_REG && scan_inst_a->dest.type)
                                ? scan_inst_a->dest.type : NULL;
                            extern_call_insts_a[extern_func_count_a] = scan_inst_a;
                            extern_func_count_a++;
                        } else if (in_extern_a) {
                            // 【P5】已存在，如果新调用有更多参数则更新指令指针
                            if (scan_inst_a->extra_args_count > extern_call_insts_a[existing_idx_a]->extra_args_count) {
                                extern_call_insts_a[existing_idx_a] = scan_inst_a;
                            }
                        }
                    }
                    scan_inst_a = scan_inst_a->next;
                }
                scan_block_a = scan_block_a->next;
            }
            scan_func_a = scan_func_a->next;
        }
        
        if (extern_func_count_a > 0) {
            fprintf(file, "// Extern Declarations - 跨模块调用的函数（ANSI原型风格）\n");
            for (size_t ei = 0; ei < extern_func_count_a; ei++) {
                // 【P5】使用get_extern_type_string获取安全的类型字符串
                const char *ret_type_str = get_extern_type_string(extern_func_ret_types_a[ei], false);
                CnIrInst *call_inst_a = extern_call_insts_a[ei];
                fprintf(file, "extern %s %s(", ret_type_str,
                        get_c_function_name(extern_func_names_a[ei]));
                // 【P5】生成ANSI原型参数列表
                if (call_inst_a && call_inst_a->extra_args_count > 0) {
                    for (size_t pi = 0; pi < call_inst_a->extra_args_count; pi++) {
                        CnType *param_type = call_inst_a->extra_args[pi].type;
                        fprintf(file, "%s", get_extern_type_string(param_type, true));
                        if (pi < call_inst_a->extra_args_count - 1) fprintf(file, ", ");
                    }
                } else {
                    fprintf(file, "void");  // 无参数时使用void而非空列表
                }
                fprintf(file, ");\n");
            }
            fprintf(file, "\n");
        }
        #undef MAX_EXTERN_FUNCS_A
    }

    func = module->first_func;
    while (func) {
        // 跳过与运行时库函数冲突的函数定义（它们已在运行时库中实现）
        if (is_runtime_function_conflict(func->name)) {
            func = func->next;
            continue;
        }
        cn_cgen_function(&ctx, func);
        func = func->next;
    }
    
    // 清理全局查找表
    g_local_struct_infos = NULL;
    g_local_struct_count = 0;
    
    // 释放局部结构体信息列表
    if (local_struct_infos) {
        free(local_struct_infos);
    }
    
    fclose(file);
    
    /* 释放缓冲区 */
    if (buffer) {
        free(buffer);
    }
    
    return 0;
}

int cn_cgen_header_to_file(CnIrModule *module, const char *filename) {
    if (!module || !filename) return -1;
    FILE *file = fopen(filename, "w");
    if (!file) return -1;
    fprintf(file, "#ifndef CN_MODULE_AUTOGEN_H\n#define CN_MODULE_AUTOGEN_H\n\n#include \"cnrt.h\"\n\n");
    CnIrFunction *func = module->first_func;
    while (func) {
        fprintf(file, "%s %s(", get_c_type_string(func->return_type), get_c_function_name(func->name));
        for (size_t i = 0; i < func->param_count; i++) {
            fprintf(file, "%s %s", get_c_param_type_string(func->params[i].type), get_c_variable_name(func->params[i].as.sym_name));
            if (i < func->param_count - 1) fprintf(file, ", ");
        }
        fprintf(file, ");\n");
        func = func->next;
    }
    fprintf(file, "\n#endif\n");
    fclose(file);
    return 0;
}

char *cn_cgen_module_to_string(CnIrModule *module) { return strdup("// Not implemented"); }

// ============================================================================
// 导入模块前向声明生成
// ============================================================================

// 前向声明
static void cn_cgen_function_forward_declaration(FILE *file, const char *func_name, size_t func_name_len, CnType *func_type, CnMap *declared_functions);

// 通配符导入回调上下文结构
typedef struct {
    FILE *file;
    const char *module_name;      // 模块名（用于生成前缀）
    size_t module_name_len;       // 模块名长度
    CnMap *declared_functions;    // 已声明函数集合（用于去重）
} WildcardImportContext;

// 枚举成员收集回调上下文
typedef struct {
    FILE *file;
    bool *first;
    const char *enum_name;      // 枚举类型名称（用于添加前缀）
    size_t enum_name_len;       // 枚举类型名称长度
} EnumMemberContext;

// 枚举成员收集回调函数
// 为枚举成员自动添加枚举类型名称作为前缀，避免C语言中枚举成员全局命名空间冲突
static void enum_member_callback(CnSemSymbol *sym, void *user_data) {
    EnumMemberContext *ctx = (EnumMemberContext *)user_data;
    if (sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
        if (!(*ctx->first)) fprintf(ctx->file, ",\n");
        // 输出格式：枚举类型名_成员名 = 值
        fprintf(ctx->file, "    %.*s_%.*s = %ld",
                (int)ctx->enum_name_len, ctx->enum_name,
                (int)sym->name_length, sym->name,
                sym->as.enum_value);
        *ctx->first = false;
    }
}

// 已生成类型名称集合（用于避免重复生成完整定义）
#define MAX_GENERATED_TYPES 256
static const char *g_generated_type_names[MAX_GENERATED_TYPES];
static size_t g_generated_type_count = 0;

// 已生成前向声明的类型名称集合（用于区分前向声明和完整定义）
static const char *g_forward_decl_types[MAX_GENERATED_TYPES];
static size_t g_forward_decl_count = 0;

// 已生成枚举类型名称集合（用于区分枚举和结构体）
static const char *g_enum_type_names[MAX_GENERATED_TYPES];
static size_t g_enum_type_count = 0;

// 检查类型是否已生成完整定义
static bool is_type_already_generated(const char *name, size_t name_len) {
    for (size_t i = 0; i < g_generated_type_count; i++) {
        if (strncmp(g_generated_type_names[i], name, name_len) == 0 &&
            g_generated_type_names[i][name_len] == '\0') {
            return true;
        }
    }
    return false;
}

// 检查类型是否已生成前向声明
static bool is_type_forward_declared(const char *name, size_t name_len) {
    for (size_t i = 0; i < g_forward_decl_count; i++) {
        if (strncmp(g_forward_decl_types[i], name, name_len) == 0 &&
            g_forward_decl_types[i][name_len] == '\0') {
            return true;
        }
    }
    return false;
}

// 检查是否是枚举类型名称
static bool is_enum_type_name(const char *name, size_t name_len) {
    for (size_t i = 0; i < g_enum_type_count; i++) {
        if (strncmp(g_enum_type_names[i], name, name_len) == 0 &&
            g_enum_type_names[i][name_len] == '\0') {
            return true;
        }
    }
    return false;
}

// 标记类型已生成完整定义
static void mark_type_as_generated(const char *name, size_t name_len) {
    if (g_generated_type_count >= MAX_GENERATED_TYPES) return;
    
    // 分配内存并复制名称
    char *name_copy = malloc(name_len + 1);
    if (!name_copy) return;
    memcpy(name_copy, name, name_len);
    name_copy[name_len] = '\0';
    
    g_generated_type_names[g_generated_type_count++] = name_copy;
}

// 标记类型已生成前向声明
static void mark_type_as_forward_declared(const char *name, size_t name_len) {
    if (g_forward_decl_count >= MAX_GENERATED_TYPES) return;
    
    // 分配内存并复制名称
    char *name_copy = malloc(name_len + 1);
    if (!name_copy) return;
    memcpy(name_copy, name, name_len);
    name_copy[name_len] = '\0';
    
    g_forward_decl_types[g_forward_decl_count++] = name_copy;
}

// 标记枚举类型已生成
static void mark_enum_type_as_generated(const char *name, size_t name_len) {
    if (g_enum_type_count >= MAX_GENERATED_TYPES) return;
    
    // 分配内存并复制名称
    char *name_copy = malloc(name_len + 1);
    if (!name_copy) return;
    memcpy(name_copy, name, name_len);
    name_copy[name_len] = '\0';
    
    g_enum_type_names[g_enum_type_count++] = name_copy;
}

// 重置已生成类型集合（每个文件编译开始时调用）
static void reset_generated_types(void) {
    for (size_t i = 0; i < g_generated_type_count; i++) {
        free((void*)g_generated_type_names[i]);
    }
    for (size_t i = 0; i < g_forward_decl_count; i++) {
        free((void*)g_forward_decl_types[i]);
    }
    for (size_t i = 0; i < g_enum_type_count; i++) {
        free((void*)g_enum_type_names[i]);
    }
    g_generated_type_count = 0;
    g_forward_decl_count = 0;
    g_enum_type_count = 0;
}

// 前向声明
static void cn_cgen_enum_type_definition(FILE *file, CnType *type);

// 前向声明
static void cn_cgen_struct_type_definition_with_forward(FILE *file, CnType *type, bool emit_forward);

// 生成结构体类型定义（从 CnType 生成）
// 这个版本会先输出前向声明，然后输出定义
static void cn_cgen_struct_type_definition(FILE *file, CnType *type) {
    cn_cgen_struct_type_definition_with_forward(file, type, true);
}

// 生成结构体类型定义（内部实现）
// emit_forward: 是否在定义前输出前向声明
static void cn_cgen_struct_type_definition_with_forward(FILE *file, CnType *type, bool emit_forward) {
    if (!file || !type || type->kind != CN_TYPE_STRUCT) {
        return;
    }
    
    // 结构体名称
    const char *name = type->as.struct_type.name;
    size_t name_len = type->as.struct_type.name_length;
    if (!name) return;
    
    // 【P0-1修复+P0-3a扩展】跳过CN基本类型关键字（如 整数、字符串、布尔 等）
    // 这些不是真正的结构体，不应生成结构体定义
    // 【P0-3a】扩展：也跳过 函数、无、结构体 等元类型关键字
    if (is_cn_primitive_type_name(name, name_len)) {
        return;
    }
    
    // 【P0-3a修复】防御性检查：检测类型名是否腐败
    // 如果类型名包含结构体定义内容（含 {; \n } 等字符），
    // 说明 struct_type.name 指向了结构体定义的源文本而非仅类型名，
    // 不应生成结构体定义
    {
        int _corrupted = 0;
        for (size_t _i = 0; _i < name_len; _i++) {
            if (name[_i] == '{' || name[_i] == ';' ||
                name[_i] == '\n' || name[_i] == '}') {
                _corrupted = 1;
                break;
            }
        }
        if (_corrupted) {
            // 类型名损坏，跳过此结构体定义
            return;
        }
    }
    
    // 检查是否已生成
    if (is_type_already_generated(name, name_len)) {
        return;
    }
    
    // 第一步：收集所有需要前向声明的结构体类型（指针字段引用的结构体）
    // 对于指针类型，只需要前向声明，不需要完整定义
    for (size_t i = 0; i < type->as.struct_type.field_count; i++) {
        CnStructField *field = &type->as.struct_type.fields[i];
        if (field->field_type) {
            if (field->field_type->kind == CN_TYPE_POINTER &&
                field->field_type->as.pointer_to &&
                field->field_type->as.pointer_to->kind == CN_TYPE_STRUCT) {
                // 指针指向结构体：只需要前向声明
                CnType *pointee = field->field_type->as.pointer_to;
                const char *pointee_name = pointee->as.struct_type.name;
                size_t pointee_name_len = pointee->as.struct_type.name_length;
                // 只有在没有生成前向声明和完整定义时才输出前向声明
                // 【P0-3a】同时跳过CN基本类型关键字和腐败的类型名
                if (pointee_name && !is_type_forward_declared(pointee_name, pointee_name_len) &&
                    !is_type_already_generated(pointee_name, pointee_name_len) &&
                    !is_cn_primitive_type_name(pointee_name, pointee_name_len)) {
                    // 【P0-3a】检查pointee_name是否腐败
                    int _pcorrupted = 0;
                    for (size_t _pi = 0; _pi < pointee_name_len; _pi++) {
                        if (pointee_name[_pi] == '{' || pointee_name[_pi] == ';' ||
                            pointee_name[_pi] == '\n' || pointee_name[_pi] == '}') {
                            _pcorrupted = 1;
                            break;
                        }
                    }
                    if (!_pcorrupted) {
                        // 输出前向声明
                        fprintf(file, "struct %.*s;\n", (int)pointee_name_len, pointee_name);
                        // 标记为已生成前向声明（但不标记为完整定义）
                        mark_type_as_forward_declared(pointee_name, pointee_name_len);
                    }
                }
            }
        }
    }
    
    // 第二步：生成依赖的类型定义（枚举和非指针结构体）
    for (size_t i = 0; i < type->as.struct_type.field_count; i++) {
        CnStructField *field = &type->as.struct_type.fields[i];
        if (field->field_type) {
            if (field->field_type->kind == CN_TYPE_ENUM) {
                // 字段类型是枚举，需要先生成枚举定义
                cn_cgen_enum_type_definition(file, field->field_type);
            } else if (field->field_type->kind == CN_TYPE_STRUCT) {
                // 字段类型是结构体（非指针），需要先生成该结构体定义（不输出前向声明）
                cn_cgen_struct_type_definition_with_forward(file, field->field_type, false);
            }
            // 注意：指针类型已在第一步处理，这里不需要再处理
        }
    }
    
    // 标记为已生成
    mark_type_as_generated(name, name_len);
    
    // 如果需要，输出前向声明（用于解决循环依赖）
    if (emit_forward) {
        fprintf(file, "struct %.*s;\n", (int)name_len, name);
    }
    
    // 生成结构体定义
    fprintf(file, "struct %.*s {\n", (int)name_len, name);
    
    // 生成字段
    for (size_t i = 0; i < type->as.struct_type.field_count; i++) {
        CnStructField *field = &type->as.struct_type.fields[i];
        const char *field_type_str = get_c_type_string(field->field_type);
        fprintf(file, "    %s %.*s;\n", field_type_str, (int)field->name_length, field->name);
    }
    
    fprintf(file, "};\n");
}

// 生成枚举类型定义（从 CnType 生成）
static void cn_cgen_enum_type_definition(FILE *file, CnType *type) {
    if (!file || !type || type->kind != CN_TYPE_ENUM) return;
    
    // 枚举名称
    const char *name = type->as.enum_type.name;
    size_t name_len = type->as.enum_type.name_length;
    if (!name) return;
    
    // 检查是否已生成
    if (is_type_already_generated(name, name_len)) return;
    
    // 标记为已生成（普通类型集合）
    mark_type_as_generated(name, name_len);
    // 同时标记为枚举类型（用于区分枚举和结构体）
    mark_enum_type_as_generated(name, name_len);
    
    // 枚举作用域
    CnSemScope *enum_scope = type->as.enum_type.enum_scope;
    if (!enum_scope) return;
    
    // 生成枚举定义
    fprintf(file, "enum %.*s {\n", (int)name_len, name);
    
    // 使用回调遍历枚举成员
    bool first = true;
    EnumMemberContext ctx = { file, &first, name, name_len };
    cn_sem_scope_foreach_symbol(enum_scope, enum_member_callback, &ctx);
    
    fprintf(file, "\n};\n");
}

// 已访问模块作用域集合（用于防止无限递归）
#define MAX_VISITED_SCOPES 256
static CnSemScope *g_visited_scopes[MAX_VISITED_SCOPES];
static size_t g_visited_scope_count = 0;

// 检查作用域是否已访问
static bool is_scope_visited(CnSemScope *scope) {
    for (size_t i = 0; i < g_visited_scope_count; i++) {
        if (g_visited_scopes[i] == scope) {
            return true;
        }
    }
    return false;
}

// 标记作用域为已访问
static void mark_scope_visited(CnSemScope *scope) {
    if (g_visited_scope_count < MAX_VISITED_SCOPES) {
        g_visited_scopes[g_visited_scope_count++] = scope;
    }
}

// 重置已访问作用域集合
static void reset_visited_scopes(void) {
    g_visited_scope_count = 0;
}

// 枚举优先遍历回调上下文结构（第一阶段：只输出枚举）
typedef struct {
    FILE *file;
} EnumFirstPassContext;

// 枚举优先遍历回调函数（只输出枚举定义）
// 前向声明，用于递归调用
static void enum_first_pass_callback(CnSemSymbol *sym, void *user_data);

// 递归遍历模块作用域输出枚举定义
static void enum_first_pass_recursive(FILE *file, CnSemScope *scope) {
    if (!scope) return;
    
    // 防止无限递归：检查是否已访问过此作用域
    if (is_scope_visited(scope)) {
        return;
    }
    mark_scope_visited(scope);
    
    EnumFirstPassContext ctx = { file };
    cn_sem_scope_foreach_symbol(scope, enum_first_pass_callback, &ctx);
}

static void enum_first_pass_callback(CnSemSymbol *sym, void *user_data) {
    EnumFirstPassContext *ctx = (EnumFirstPassContext *)user_data;
    // 只导出公开的符号
    if (!sym->is_public) {
        return;
    }
    
    if (sym->kind == CN_SEM_SYMBOL_ENUM && sym->type) {
        cn_cgen_enum_type_definition(ctx->file, sym->type);
        // 如果枚举符号有 module_scope，说明它是由模块符号替换而来的
        // 需要递归处理嵌套导入的模块
        if (sym->as.module_scope) {
            enum_first_pass_recursive(ctx->file, sym->as.module_scope);
        }
    } else if (sym->kind == CN_SEM_SYMBOL_MODULE && sym->as.module_scope) {
        // 递归处理嵌套导入的模块
        enum_first_pass_recursive(ctx->file, sym->as.module_scope);
    }
}

// 结构体优先遍历回调上下文结构（第二阶段：只输出结构体）
typedef struct {
    FILE *file;
} StructFirstPassContext;

// 前向声明，用于递归调用
static void struct_first_pass_callback(CnSemSymbol *sym, void *user_data);

// 递归遍历模块作用域输出结构体定义
static void struct_first_pass_recursive(FILE *file, CnSemScope *scope) {
    if (!scope) return;
    
    // 防止无限递归：检查是否已访问过此作用域
    if (is_scope_visited(scope)) {
        return;
    }
    mark_scope_visited(scope);
    
    StructFirstPassContext ctx = { file };
    cn_sem_scope_foreach_symbol(scope, struct_first_pass_callback, &ctx);
}

// 结构体优先遍历回调函数（只输出结构体定义）
static void struct_first_pass_callback(CnSemSymbol *sym, void *user_data) {
    StructFirstPassContext *ctx = (StructFirstPassContext *)user_data;
    
    // 只导出公开的符号
    if (!sym->is_public) {
        return;
    }
    
    if (sym->kind == CN_SEM_SYMBOL_STRUCT && sym->type) {
        cn_cgen_struct_type_definition(ctx->file, sym->type);
        // 如果结构体符号有 module_scope，说明它是由模块符号替换而来的
        // 需要递归处理嵌套导入的模块
        if (sym->as.module_scope) {
            struct_first_pass_recursive(ctx->file, sym->as.module_scope);
        }
    } else if (sym->kind == CN_SEM_SYMBOL_MODULE && sym->as.module_scope) {
        // 递归处理嵌套导入的模块
        struct_first_pass_recursive(ctx->file, sym->as.module_scope);
    }
}

// 通配符导入回调函数（遍历模块作用域符号时调用）
// 注意：此回调只输出函数前向声明和变量声明，不再输出类型定义
static void wildcard_import_callback(CnSemSymbol *sym, void *user_data) {
    WildcardImportContext *ctx = (WildcardImportContext *)user_data;
    // 只导出公开的符号
    if (!sym->is_public) {
        return;
    }
    
    if (sym->kind == CN_SEM_SYMBOL_FUNCTION && sym->type) {
        // 直接使用原始函数名生成前向声明（不再使用编码名称）
        // 模块系统通过作用域管理避免命名冲突
        cn_cgen_function_forward_declaration(ctx->file, sym->name, sym->name_length, sym->type, ctx->declared_functions);
    } else if (sym->kind == CN_SEM_SYMBOL_VARIABLE && sym->type) {
        // 生成变量的 extern 声明
        const char *var_type = get_c_type_string(sym->type);
        fprintf(ctx->file, "extern %s cn_var_%.*s;\n", var_type, (int)sym->name_length, sym->name);
    }
    // 注意：结构体和枚举定义已在第一、二阶段输出，此处不再输出
}

// 全量导入回调上下文结构
typedef struct {
    FILE *file;
    const char *module_name;
    size_t module_name_len;
    CnMap *declared_functions;    // 已声明函数集合（用于去重）
} FullImportContext;

// 全量导入回调函数（遍历模块作用域符号时调用）
// 注意：此回调只输出函数前向声明和变量声明，不再输出类型定义
static void full_import_callback(CnSemSymbol *sym, void *user_data) {
    FullImportContext *ctx = (FullImportContext *)user_data;
    // 只导出公开的符号
    if (!sym->is_public) {
        return;
    }
    
    if (sym->kind == CN_SEM_SYMBOL_FUNCTION && sym->type) {
        // 直接使用原始函数名生成前向声明（不再使用编码名称）
        // 模块系统通过作用域管理避免命名冲突
        cn_cgen_function_forward_declaration(ctx->file, sym->name, sym->name_length, sym->type, ctx->declared_functions);
    } else if (sym->kind == CN_SEM_SYMBOL_VARIABLE && sym->type) {
        // 生成变量的 extern 声明
        const char *var_type = get_c_type_string(sym->type);
        fprintf(ctx->file, "extern %s cn_var_%.*s;\n", var_type, (int)sym->name_length, sym->name);
    }
    // 注意：结构体和枚举定义已在第一、二阶段输出，此处不再输出
}

/**
 * @brief 收集函数类型中需要前向声明的结构体
 *
 * @param func_type 函数类型
 * @param struct_names 结构体名称数组（输出）
 * @param struct_name_lens 结构体名称长度数组（输出）
 * @param max_count 最大数量
 * @param count 实际数量（输出）
 */
static void collect_struct_forward_decls_from_func_type(CnType *func_type,
                                                         const char **struct_names,
                                                         size_t *struct_name_lens,
                                                         size_t max_count,
                                                         size_t *count) {
    if (!func_type || func_type->kind != CN_TYPE_FUNCTION || !struct_names || !struct_name_lens || !count) {
        *count = 0;
        return;
    }
    
    *count = 0;
    
    // 检查返回类型
    CnType *ret_type = func_type->as.function.return_type;
    if (ret_type) {
        if (ret_type->kind == CN_TYPE_STRUCT) {
            if (*count < max_count) {
                struct_names[*count] = ret_type->as.struct_type.name;
                struct_name_lens[*count] = ret_type->as.struct_type.name_length;
                (*count)++;
            }
        } else if (ret_type->kind == CN_TYPE_POINTER && ret_type->as.pointer_to &&
                   ret_type->as.pointer_to->kind == CN_TYPE_STRUCT) {
            // 指针指向结构体，需要前向声明
            if (*count < max_count) {
                struct_names[*count] = ret_type->as.pointer_to->as.struct_type.name;
                struct_name_lens[*count] = ret_type->as.pointer_to->as.struct_type.name_length;
                (*count)++;
            }
        }
    }
    
    // 检查参数类型
    for (size_t i = 0; i < func_type->as.function.param_count; i++) {
        CnType *param_type = func_type->as.function.param_types[i];
        if (param_type) {
            if (param_type->kind == CN_TYPE_STRUCT) {
                // 检查是否已存在
                bool exists = false;
                for (size_t j = 0; j < *count; j++) {
                    if (struct_name_lens[j] == param_type->as.struct_type.name_length &&
                        strncmp(struct_names[j], param_type->as.struct_type.name, struct_name_lens[j]) == 0) {
                        exists = true;
                        break;
                    }
                }
                if (!exists && *count < max_count) {
                    struct_names[*count] = param_type->as.struct_type.name;
                    struct_name_lens[*count] = param_type->as.struct_type.name_length;
                    (*count)++;
                }
            } else if (param_type->kind == CN_TYPE_POINTER && param_type->as.pointer_to &&
                       param_type->as.pointer_to->kind == CN_TYPE_STRUCT) {
                // 指针指向结构体，需要前向声明
                CnType *pointee = param_type->as.pointer_to;
                // 检查是否已存在
                bool exists = false;
                for (size_t j = 0; j < *count; j++) {
                    if (struct_name_lens[j] == pointee->as.struct_type.name_length &&
                        strncmp(struct_names[j], pointee->as.struct_type.name, struct_name_lens[j]) == 0) {
                        exists = true;
                        break;
                    }
                }
                if (!exists && *count < max_count) {
                    struct_names[*count] = pointee->as.struct_type.name;
                    struct_name_lens[*count] = pointee->as.struct_type.name_length;
                    (*count)++;
                }
            }
        }
    }
}

/**
 * @brief 生成单个函数的前向声明（带去重检查）
 *
 * @param file 输出文件
 * @param func_name 函数名称（非空字符结尾）
 * @param func_name_len 函数名称长度
 * @param func_type 函数类型
 * @param declared_functions 已声明函数集合（用于去重，可为NULL）
 */
static void cn_cgen_function_forward_declaration(FILE *file, const char *func_name, size_t func_name_len, CnType *func_type, CnMap *declared_functions) {
    if (!file || !func_name || !func_type || func_type->kind != CN_TYPE_FUNCTION) {
        return;
    }
    
    // 创建临时空字符结尾的字符串用于检测
    char temp_name[256];
    size_t safe_len = func_name_len < sizeof(temp_name) - 1 ? func_name_len : sizeof(temp_name) - 1;
    memcpy(temp_name, func_name, safe_len);
    temp_name[safe_len] = '\0';
    
    // 检测运行时函数冲突：如果函数名与运行时库函数同名，跳过前向声明
    // 运行时库已提供这些函数的实现，无需重复声明
    if (is_runtime_function_conflict(temp_name)) {
        return;
    }
    
    // 去重检查：如果已声明过该函数，跳过
    if (declared_functions && cn_rt_map_contains(declared_functions, temp_name)) {
        return;
    }
    
    // 检测运行时宏冲突：如果函数名与运行时宏同名，需要先 #undef 取消宏定义
    if (is_runtime_macro_conflict(temp_name)) {
        fprintf(file, "#undef %s\n", temp_name);
    }
    
    // 返回类型
    const char *ret_type = get_c_type_string(func_type->as.function.return_type);
    fprintf(file, "%s %.*s(", ret_type, (int)func_name_len, func_name);
    
    // 参数列表（使用 get_c_param_type_string 为字符串类型添加 const 修饰符）
    if (func_type->as.function.param_count == 0) {
        fprintf(file, "void");
    } else {
        for (size_t i = 0; i < func_type->as.function.param_count; i++) {
            if (i > 0) fprintf(file, ", ");
            const char *param_type = get_c_param_type_string(func_type->as.function.param_types[i]);
            fprintf(file, "%s", param_type);
        }
    }
    
    fprintf(file, ");\n");
    
    // 记录已声明的函数
    if (declared_functions) {
        // 使用strdup复制键名，因为哈希表会持有该指针
        char *key = strdup(temp_name);
        if (key) {
            cn_rt_map_insert(declared_functions, key, (void*)1);
        }
    }
}

/**
 * @brief 从全局作用域获取导入符号的类型信息并生成前向声明
 *
 * 修复：分三个阶段输出，确保类型依赖顺序正确：
 * 1. 先输出所有枚举定义（枚举不依赖其他类型）
 * 2. 再输出所有结构体定义（结构体可能依赖枚举）
 * 3. 最后输出函数前向声明（函数可能依赖结构体和枚举）
 *
 * @param file 输出文件
 * @param program AST程序节点
 * @param global_scope 全局作用域
 * @param declared_functions 已声明函数集合（用于去重）
 */
static void cn_cgen_import_forward_declarations(FILE *file, CnAstProgram *program, CnSemScope *global_scope, CnMap *declared_functions) {
    if (!file || !program || !global_scope) {
        return;
    }
    
    // 重置已访问作用域集合（防止无限递归）
    reset_visited_scopes();
    
    // 如果没有导入语句，直接返回
    if (program->import_count == 0) {
        return;
    }
    
    // =========================================================================
    // 第一阶段：输出所有枚举定义（枚举不依赖其他类型，必须最先输出）
    // =========================================================================
    fprintf(file, "// Enum Definitions - 从导入模块\n");
    
    for (size_t i = 0; i < program->import_count; i++) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }
        
        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 处理选择性导入
        if (import->kind == CN_IMPORT_FROM_SELECTIVE && import->members && import->member_count > 0) {
            for (size_t j = 0; j < import->member_count; j++) {
                CnAstImportMember *member = &import->members[j];
                CnSemSymbol *symbol = cn_sem_scope_lookup(global_scope, member->name, member->name_length);
                if (symbol && symbol->kind == CN_SEM_SYMBOL_ENUM && symbol->type) {
                    cn_cgen_enum_type_definition(file, symbol->type);
                }
            }
        }
        // 处理通配符导入
        else if (import->kind == CN_IMPORT_FROM_WILDCARD) {
            CnSemSymbol *module_sym = cn_sem_scope_lookup(global_scope, import->module_name, import->module_name_length);
            if (module_sym && module_sym->kind == CN_SEM_SYMBOL_MODULE && module_sym->as.module_scope) {
                // 只输出枚举定义
                EnumFirstPassContext ctx = { file };
                cn_sem_scope_foreach_symbol(module_sym->as.module_scope, enum_first_pass_callback, &ctx);
            }
        }
        // 处理全量导入和从模块导入
        else if (import->kind == CN_IMPORT_FULL || import->kind == CN_IMPORT_FROM_MODULE) {
            const char *module_name = import->module_name;
            size_t module_name_len = import->module_name_length;
            
            if (!module_name && import->module_path && import->module_path->segment_count > 0) {
                CnAstModulePathSegment *last_seg = &import->module_path->segments[import->module_path->segment_count - 1];
                module_name = last_seg->name;
                module_name_len = last_seg->name_length;
            }
            
            if (!module_name) continue;
            
            CnSemSymbol *sym = cn_sem_scope_lookup(global_scope, module_name, module_name_len);
            if (sym) {
                if (sym->kind == CN_SEM_SYMBOL_MODULE && sym->as.module_scope) {
                    // 使用递归函数处理嵌套导入
                    enum_first_pass_recursive(file, sym->as.module_scope);
                } else if (sym->kind == CN_SEM_SYMBOL_ENUM && sym->type) {
                    cn_cgen_enum_type_definition(file, sym->type);
                } else if (sym->kind == CN_SEM_SYMBOL_STRUCT && sym->type) {
                    // 如果符号是结构体，检查是否有 module_scope（嵌套导入）
                    cn_cgen_struct_type_definition(file, sym->type);
                    if (sym->as.module_scope) {
                        enum_first_pass_recursive(file, sym->as.module_scope);
                    }
                }
            }
        }
    }
    
    // =========================================================================
    // 第二阶段：输出所有结构体定义（结构体可能依赖枚举）
    // =========================================================================
    // 重置已访问作用域集合，允许结构体阶段重新访问枚举阶段已访问的作用域
    reset_visited_scopes();
    
    fprintf(file, "\n// Struct Definitions - 从导入模块\n");
    
    for (size_t i = 0; i < program->import_count; i++) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }
        
        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 处理选择性导入
        if (import->kind == CN_IMPORT_FROM_SELECTIVE && import->members && import->member_count > 0) {
            for (size_t j = 0; j < import->member_count; j++) {
                CnAstImportMember *member = &import->members[j];
                CnSemSymbol *symbol = cn_sem_scope_lookup(global_scope, member->name, member->name_length);
                if (symbol && symbol->kind == CN_SEM_SYMBOL_STRUCT && symbol->type) {
                    cn_cgen_struct_type_definition(file, symbol->type);
                }
            }
        }
        // 处理通配符导入
        else if (import->kind == CN_IMPORT_FROM_WILDCARD) {
            CnSemSymbol *module_sym = cn_sem_scope_lookup(global_scope, import->module_name, import->module_name_length);
            if (module_sym && module_sym->kind == CN_SEM_SYMBOL_MODULE && module_sym->as.module_scope) {
                StructFirstPassContext ctx = { file };
                cn_sem_scope_foreach_symbol(module_sym->as.module_scope, struct_first_pass_callback, &ctx);
            }
        }
        // 处理全量导入和从模块导入
        else if (import->kind == CN_IMPORT_FULL || import->kind == CN_IMPORT_FROM_MODULE) {
            const char *module_name = import->module_name;
            size_t module_name_len = import->module_name_length;
            
            if (!module_name && import->module_path && import->module_path->segment_count > 0) {
                CnAstModulePathSegment *last_seg = &import->module_path->segments[import->module_path->segment_count - 1];
                module_name = last_seg->name;
                module_name_len = last_seg->name_length;
            }
            
            if (!module_name) continue;
            
            CnSemSymbol *sym = cn_sem_scope_lookup(global_scope, module_name, module_name_len);
            if (sym) {
                if (sym->kind == CN_SEM_SYMBOL_MODULE && sym->as.module_scope) {
                    // 使用递归函数处理嵌套导入
                    struct_first_pass_recursive(file, sym->as.module_scope);
                } else if (sym->kind == CN_SEM_SYMBOL_STRUCT) {
                    if (sym->type) {
                        cn_cgen_struct_type_definition(file, sym->type);
                    }
                    // 如果结构体有 module_scope，递归处理嵌套导入
                    if (sym->as.module_scope) {
                        struct_first_pass_recursive(file, sym->as.module_scope);
                    }
                }
            }
        }
    }
    
    // =========================================================================
    // 第2.5阶段：输出结构体前向声明（用于函数参数中的结构体指针）
    // =========================================================================
    // 收集所有函数参数和返回值中使用的结构体类型
    #define MAX_STRUCT_FORWARD_DECLS 256
    const char *struct_forward_names[MAX_STRUCT_FORWARD_DECLS];
    size_t struct_forward_name_lens[MAX_STRUCT_FORWARD_DECLS];
    size_t struct_forward_count = 0;
    
    for (size_t i = 0; i < program->import_count; i++) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }
        
        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 收集函数类型中的结构体
        // 处理选择性导入
        if (import->kind == CN_IMPORT_FROM_SELECTIVE && import->members && import->member_count > 0) {
            for (size_t j = 0; j < import->member_count; j++) {
                CnAstImportMember *member = &import->members[j];
                CnSemSymbol *symbol = cn_sem_scope_lookup(global_scope, member->name, member->name_length);
                if (symbol && symbol->kind == CN_SEM_SYMBOL_FUNCTION && symbol->type) {
                    size_t count = 0;
                    collect_struct_forward_decls_from_func_type(symbol->type,
                                                                struct_forward_names + struct_forward_count,
                                                                struct_forward_name_lens + struct_forward_count,
                                                                MAX_STRUCT_FORWARD_DECLS - struct_forward_count,
                                                                &count);
                    struct_forward_count += count;
                }
            }
        }
        // 处理通配符导入 - 使用回调函数遍历
        // 注意：通配符导入的结构体前向声明已在 struct_first_pass_callback 中处理
        // 这里不需要额外处理，因为结构体定义已在第二阶段输出
        else if (import->kind == CN_IMPORT_FROM_WILDCARD) {
            // 结构体定义已在第二阶段输出，无需额外处理
        }
        // 处理全量导入和从模块导入 - 使用回调函数遍历
        // 注意：全量导入的结构体前向声明已在 struct_first_pass_callback 中处理
        else if (import->kind == CN_IMPORT_FULL || import->kind == CN_IMPORT_FROM_MODULE) {
            // 结构体定义已在第二阶段输出，无需额外处理
        }
    }
    
    // 输出结构体前向声明（去重）
    if (struct_forward_count > 0) {
        fprintf(file, "\n// Struct Forward Declarations - 用于函数参数\n");
        for (size_t i = 0; i < struct_forward_count; i++) {
            // 检查是否已输出过
            bool already_output = false;
            for (size_t j = 0; j < i; j++) {
                if (struct_forward_name_lens[j] == struct_forward_name_lens[i] &&
                    strncmp(struct_forward_names[j], struct_forward_names[i], struct_forward_name_lens[i]) == 0) {
                    already_output = true;
                    break;
                }
            }
            if (!already_output) {
                fprintf(file, "struct %.*s;\n", (int)struct_forward_name_lens[i], struct_forward_names[i]);
            }
        }
    }
    #undef MAX_STRUCT_FORWARD_DECLS
    
    // =========================================================================
    // 第三阶段：输出函数前向声明和变量声明（可能依赖结构体和枚举）
    // =========================================================================
    fprintf(file, "\n// Forward Declarations - 从导入模块\n");
    
    for (size_t i = 0; i < program->import_count; i++) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }
        
        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 处理选择性导入
        if (import->kind == CN_IMPORT_FROM_SELECTIVE && import->members && import->member_count > 0) {
            for (size_t j = 0; j < import->member_count; j++) {
                CnAstImportMember *member = &import->members[j];
                const char *symbol_name = member->name;
                size_t symbol_name_len = member->name_length;
                
                CnSemSymbol *symbol = cn_sem_scope_lookup(global_scope, symbol_name, symbol_name_len);
                if (symbol && symbol->type) {
                    if (symbol->kind == CN_SEM_SYMBOL_FUNCTION) {
                        cn_cgen_function_forward_declaration(file, symbol_name, symbol_name_len, symbol->type, declared_functions);
                    } else if (symbol->kind == CN_SEM_SYMBOL_VARIABLE) {
                        const char *var_type = get_c_type_string(symbol->type);
                        fprintf(file, "extern %s cn_var_%.*s;\n", var_type, (int)symbol_name_len, symbol_name);
                    }
                }
            }
        }
        // 处理通配符导入
        else if (import->kind == CN_IMPORT_FROM_WILDCARD) {
            CnSemSymbol *module_sym = cn_sem_scope_lookup(global_scope, import->module_name, import->module_name_length);
            if (module_sym && module_sym->kind == CN_SEM_SYMBOL_MODULE && module_sym->as.module_scope) {
                WildcardImportContext ctx = { file, import->module_name, import->module_name_length, declared_functions };
                cn_sem_scope_foreach_symbol(module_sym->as.module_scope, wildcard_import_callback, &ctx);
            }
        }
        // 处理全量导入和从模块导入
        else if (import->kind == CN_IMPORT_FULL || import->kind == CN_IMPORT_FROM_MODULE) {
            const char *module_name = import->module_name;
            size_t module_name_len = import->module_name_length;
            
            if (!module_name && import->module_path && import->module_path->segment_count > 0) {
                CnAstModulePathSegment *last_seg = &import->module_path->segments[import->module_path->segment_count - 1];
                module_name = last_seg->name;
                module_name_len = last_seg->name_length;
            }
            
            if (!module_name) continue;
            
            CnSemSymbol *sym = cn_sem_scope_lookup(global_scope, module_name, module_name_len);
            if (sym && sym->kind == CN_SEM_SYMBOL_MODULE && sym->as.module_scope) {
                FullImportContext fctx = { file, module_name, module_name_len, declared_functions };
                cn_sem_scope_foreach_symbol(sym->as.module_scope, full_import_callback, &fctx);
            }
        }
    }
    
    fprintf(file, "\n");
}

/**
 * @brief 带导入支持的完整代码生成函数
 */
int cn_cgen_module_with_imports_to_file(CnIrModule *module, CnAstProgram *program,
                                         CnModuleLoader *loader,
                                         CnSemScope *global_scope,
                                         CnModuleId *module_id,
                                         const char *filename) {
    // 首先调用原有的生成函数（不包含导入声明）
    // 然后在适当位置插入导入声明
    // 为了简化实现，我们直接修改现有函数的行为
    
    if (!module || !filename) return -1;
    
    // 重置已生成类型集合，避免重复生成
    reset_generated_types();

    /* 根据 IR 模块上的目标三元组获取预设数据布局（若存在）。 */
    CnTargetDataLayout layout;
    bool layout_ok = cn_support_target_get_data_layout(&module->target, &layout);
    if (layout_ok) {
        g_target_layout = layout;
        g_target_layout_valid = true;
    } else {
        g_target_layout_valid = false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) return -1;
    
    /* 性能优化：设置较大的缓冲区，减少系统调用 */
    char *buffer = malloc(CGEN_BUFFER_SIZE);
    if (buffer) {
        setvbuf(file, buffer, _IOFBF, CGEN_BUFFER_SIZE);
    }
    
    CnCCodeGenContext ctx = {0};
    ctx.output_file = file;
    ctx.module = module;
    ctx.module_id = module_id;  // 设置模块ID用于生成带前缀的函数名
    ctx.program = program;      // 设置程序AST，用于查找类定义
    ctx.global_scope = global_scope; // 设置全局作用域，用于查找结构体类型的完整定义
    
    // 初始化已声明函数集合（用于去重）
    ctx.declared_functions = cn_rt_map_create(64);

    // 根据编译模式生成不同的头文件
    if (module->compile_mode == CN_COMPILE_MODE_FREESTANDING) {
        fprintf(file, "#include <stddef.h>\n#include <stdbool.h>\n#include <stdint.h>\n\n");
        fprintf(file, "// CN Language Runtime Function Declarations (Freestanding Mode)\n");
        fprintf(file, "void cn_rt_print_string(const char *str);\n");
        fprintf(file, "void cn_rt_print_int(long long val);\n");
        fprintf(file, "void cn_rt_print_float(double val);\n");
        fprintf(file, "void cn_rt_print_bool(int val);\n");
        fprintf(file, "size_t cn_rt_string_length(const char *str);\n");
        fprintf(file, "char* cn_rt_string_concat(const char *a, const char *b);\n");
        fprintf(file, "char* cn_rt_int_to_string(long long val);\n");
        fprintf(file, "char* cn_rt_bool_to_string(int val);\n");
        fprintf(file, "char* cn_rt_float_to_string(double val);\n");
        fprintf(file, "void* cn_rt_array_alloc(size_t elem_size, size_t count);\n");
        fprintf(file, "size_t cn_rt_array_length(void *arr);\n");
        fprintf(file, "int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size);\n");
        fprintf(file, "\n");
    } else {
        fprintf(file, "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include \"cnrt.h\"\n");
        fprintf(file, "#include \"cnlang/runtime/system_api.h\"\n\n");
    }
    
    // 生成结构体定义（如果提供了 AST）
    LocalStructInfo *local_struct_infos = NULL;
    size_t local_struct_count = 0;
    size_t local_struct_capacity = 0;
    
    if (program) {
        // =============================================================================
        // 先输出导入模块的枚举和结构体定义（必须在当前模块定义之前）
        // =============================================================================
        if (global_scope) {
            cn_cgen_import_forward_declarations(file, program, global_scope, ctx.declared_functions);
        }
        
        // =============================================================================
        // 先输出枚举定义（必须在结构体之前，因为结构体可能使用枚举类型）
        // =============================================================================
        if (program->enum_count > 0) {
            fprintf(file, "// CN Language Enum Definitions\n");
            for (size_t i = 0; i < program->enum_count; i++) {
                cn_cgen_enum_decl(&ctx, program->enums[i]);
            }
        }
        
        // =============================================================================
        // 输出全局结构体定义（在枚举之后）
        // 采用两遍扫描：第一遍输出前向声明，第二遍输出完整定义
        // 这样可以解决结构体之间的依赖问题
        // =============================================================================
        if (program->struct_count > 0) {
            // 第一遍：输出所有结构体的前向声明
            fprintf(file, "// CN Language Global Struct Forward Declarations\n");
            for (size_t i = 0; i < program->struct_count; i++) {
                CnAstStmt *struct_stmt = program->structs[i];
                if (struct_stmt && struct_stmt->kind == CN_AST_STMT_STRUCT_DECL) {
                    CnAstStructDecl *decl = &struct_stmt->as.struct_decl;
                    fprintf(file, "struct %.*s;\n", (int)decl->name_length, decl->name);
                }
            }
            fprintf(file, "\n");
            
            // 第二遍：输出所有结构体的完整定义
            fprintf(file, "// CN Language Global Struct Definitions\n");
            for (size_t i = 0; i < program->struct_count; i++) {
                cn_cgen_struct_decl(&ctx, program->structs[i]);
            }
        }
        
        for (size_t i = 0; i < program->function_count; i++) {
            collect_local_structs_from_function(program->functions[i], &local_struct_infos, &local_struct_count, &local_struct_capacity);
        }
        
        // 输出局部结构体定义（提升到全局，但使用特殊命名）
        // 同样采用两遍扫描：先输出前向声明，再输出完整定义
        if (local_struct_count > 0) {
            // 第一遍：输出所有局部结构体的前向声明
            fprintf(file, "// CN Language Local Struct Forward Declarations (hoisted for C compatibility)\n");
            for (size_t i = 0; i < local_struct_count; i++) {
                CnAstStmt *struct_stmt = local_struct_infos[i].struct_stmt;
                const char *func_name = local_struct_infos[i].function_name;
                size_t func_name_len = local_struct_infos[i].function_name_length;
                if (struct_stmt && struct_stmt->kind == CN_AST_STMT_STRUCT_DECL) {
                    CnAstStructDecl *decl = &struct_stmt->as.struct_decl;
                    fprintf(file, "struct __local_%.*s_%.*s;\n",
                            (int)func_name_len, func_name,
                            (int)decl->name_length, decl->name);
                }
            }
            fprintf(file, "\n");
            
            // 第二遍：输出所有局部结构体的完整定义
            fprintf(file, "// CN Language Local Struct Definitions (hoisted for C compatibility)\n");
            fprintf(file, "// Note: These are local to their respective functions in CN semantics\n");
            for (size_t i = 0; i < local_struct_count; i++) {
                cn_cgen_struct_decl_with_prefix(&ctx,
                                               local_struct_infos[i].struct_stmt,
                                               local_struct_infos[i].function_name,
                                               local_struct_infos[i].function_name_length);
            }
            
            g_local_struct_infos = local_struct_infos;
            g_local_struct_count = local_struct_count;
        }
    }
    
    // 生成接口定义（必须在类定义之前，因为类的vtable引用接口vtable）
    if (program && program->interface_count > 0) {
        fprintf(file, "// CN Language Interface Definitions\n\n");
        for (size_t i = 0; i < program->interface_count; i++) {
            CnAstStmt *interface_stmt = program->interfaces[i];
            if (interface_stmt && interface_stmt->kind == CN_AST_STMT_INTERFACE_DECL) {
                cn_cgen_interface_decl(&ctx, interface_stmt->as.interface_decl);
            }
        }
    }
    
    if (program && program->class_count > 0) {
        fprintf(file, "// CN Language Class Definitions\n\n");
        for (size_t i = 0; i < program->class_count; i++) {
            CnAstStmt *class_stmt = program->classes[i];
            if (class_stmt && class_stmt->kind == CN_AST_STMT_CLASS_DECL) {
                cn_cgen_class_decl(&ctx, class_stmt->as.class_decl);
            }
        }
    }
    
    // 生成全局变量声明
    fprintf(file, "// Global Variables\n");
    CnIrGlobalVar *global = module->first_global;
    while (global) {
        // 【修复】检查是否为静态数组类型，生成正确的C数组语法
        if (global->type && global->type->kind == CN_TYPE_ARRAY && global->type->as.array.length > 0) {
            // 静态数组：生成 "元素类型 变量名[长度];"
            fprintf(file, "%s cn_var_%s[%zu]",
                    get_c_type_string(global->type->as.array.element_type),
                    global->name,
                    global->type->as.array.length);
        } else {
            // 非数组类型：正常生成
            fprintf(file, "%s cn_var_%s",
                    get_c_type_string(global->type),
                    global->name);
            
            if (global->initializer.kind != CN_IR_OP_NONE) {
                fprintf(file, " = ");
                if (global->initializer.kind == CN_IR_OP_IMM_INT) {
                    fprintf(file, "%lld", global->initializer.as.imm_int);
                } else if (global->initializer.kind == CN_IR_OP_IMM_FLOAT) {
                    fprintf(file, "%f", global->initializer.as.imm_float);
                } else if (global->initializer.kind == CN_IR_OP_IMM_STR) {
                    // 字符串字面量初始化
                    fprintf(file, "\"%s\"", global->initializer.as.imm_str ? global->initializer.as.imm_str : "");
                } else if (global->initializer.kind == CN_IR_OP_AST_EXPR && global->initializer.as.ast_expr) {
                    // 结构体字面量初始化
                    CnAstExpr *struct_lit = global->initializer.as.ast_expr;
                    if (struct_lit->kind == CN_AST_EXPR_STRUCT_LITERAL) {
                        // 使用 C89 兼容的初始化语法：{ .field = value, ... }
                        // MSVC 对复合字面量有限制，使用指定成员初始化更兼容
                        if (struct_lit->as.struct_lit.struct_name && struct_lit->as.struct_lit.struct_name_length > 0) {
                            fprintf(file, "{");
                        } else {
                            fprintf(file, "{");
                        }
                        for (size_t i = 0; i < struct_lit->as.struct_lit.field_count; i++) {
                            if (i > 0) {
                                fprintf(file, ", ");
                            }
                            CnAstExpr *field_value = struct_lit->as.struct_lit.fields[i].value;
                            if (field_value) {
                                if (field_value->kind == CN_AST_EXPR_INTEGER_LITERAL) {
                                    fprintf(file, "%lld", field_value->as.integer_literal.value);
                                } else if (field_value->kind == CN_AST_EXPR_FLOAT_LITERAL) {
                                    fprintf(file, "%f", field_value->as.float_literal.value);
                                } else if (field_value->kind == CN_AST_EXPR_STRING_LITERAL) {
                                    fprintf(file, "\"%s\"", field_value->as.string_literal.value ? field_value->as.string_literal.value : "");
                                } else if (field_value->kind == CN_AST_EXPR_CHAR_LITERAL) {
                                    fprintf(file, "'%c'", field_value->as.char_literal.value);
                                } else if (field_value->kind == CN_AST_EXPR_BOOL_LITERAL) {
                                    fprintf(file, "%d", field_value->as.bool_literal.value ? 1 : 0);
                                } else if (field_value->kind == CN_AST_EXPR_IDENTIFIER) {
                                    if (field_value->as.identifier.name_length == 1 &&
                                        field_value->as.identifier.name[0] == '无') {
                                        fprintf(file, "NULL");
                                    } else {
                                        fprintf(file, "0");
                                    }
                                } else {
                                    fprintf(file, "0");
                                }
                            }
                        }
                        fprintf(file, "}");
                    } else {
                        fprintf(file, "0");
                    }
                }
            }
        }
        
        fprintf(file, ";\n");
        global = global->next;
    }
    fprintf(file, "\n");
    
    // 生成函数前置声明（Forward Declarations）
    fprintf(file, "// Forward Declarations\n");
    CnIrFunction *func = module->first_func;
    
    // 首先收集所有需要 #undef 的运行时宏冲突函数名
    // 在前向声明之前统一生成 #undef，避免宏展开导致重定义
    bool has_macro_conflict = false;
    CnIrFunction *temp_func = func;
    while (temp_func) {
        if (is_runtime_macro_conflict(temp_func->name)) {
            if (!has_macro_conflict) {
                fprintf(file, "// 取消运行时宏定义，避免与用户函数名冲突\n");
                has_macro_conflict = true;
            }
            fprintf(file, "#undef %s\n", temp_func->name);
        }
        temp_func = temp_func->next;
    }
    if (has_macro_conflict) {
        fprintf(file, "\n");
    }
    
    // 生成前向声明（跳过运行时库已定义的函数）
    while (func) {
        // 跳过与运行时库函数冲突的函数声明
        if (is_runtime_function_conflict(func->name)) {
            func = func->next;
            continue;
        }
        // 直接使用原始函数名（不再使用编码名称）
        const char *c_func_name = get_c_function_name(func->name);
        
        fprintf(file, "%s %s(", get_c_type_string(func->return_type), c_func_name);
        for (size_t i = 0; i < func->param_count; i++) {
            fprintf(file, "%s", get_c_param_type_string(func->params[i].type));
            if (i < func->param_count - 1) fprintf(file, ", ");
        }
        fprintf(file, ");\n");
        func = func->next;
    }
    fprintf(file, "\n");
    
    // 【P5修复】扫描所有CALL指令，收集被调用但未在当前模块声明的函数
    // 生成ANSI原型风格的extern声明，包含参数类型信息
    // 解决K&R风格声明(extern type name();)导致的"函数参数数量不匹配"GCC错误
    {
        // 最大可收集的外部函数数量
        #define MAX_EXTERN_FUNCS 512
        const char *extern_func_names[MAX_EXTERN_FUNCS];
        CnType *extern_func_ret_types[MAX_EXTERN_FUNCS];
        CnIrInst *extern_call_insts[MAX_EXTERN_FUNCS]; // 【P5】保存CALL指令以获取参数信息
        size_t extern_func_count = 0;
        
        // 遍历所有函数的所有基本块，收集CALL指令中被调用的函数名
        CnIrFunction *scan_func = module->first_func;
        while (scan_func) {
            CnIrBasicBlock *scan_block = scan_func->first_block;
            while (scan_block) {
                CnIrInst *scan_inst = scan_block->first_inst;
                while (scan_inst) {
                    if (scan_inst->kind == CN_IR_INST_CALL &&
                        scan_inst->src1.kind == CN_IR_OP_SYMBOL &&
                        scan_inst->src1.as.sym_name) {
                        const char *called_func_name = scan_inst->src1.as.sym_name;
                        
                        // 跳过运行时库函数（cn_rt_前缀或与运行时库冲突的函数名）
                        if (strncmp(called_func_name, "cn_rt_", 6) == 0 ||
                            is_runtime_function_conflict(called_func_name) ||
                            is_runtime_macro_conflict(called_func_name)) {
                            scan_inst = scan_inst->next;
                            continue;
                        }
                        
                        // 检查是否已在当前模块的函数列表中
                        bool in_module_func = false;
                        CnIrFunction *check_func = module->first_func;
                        while (check_func) {
                            if (check_func->name && strcmp(check_func->name, called_func_name) == 0) {
                                in_module_func = true;
                                break;
                            }
                            check_func = check_func->next;
                        }
                        
                        // 检查是否已在declared_functions集合中
                        if (!in_module_func && ctx.declared_functions) {
                            if (cn_rt_map_contains(ctx.declared_functions, called_func_name)) {
                                in_module_func = true;
                            }
                        }
                        
                        if (in_module_func) {
                            scan_inst = scan_inst->next;
                            continue;
                        }
                        
                        // 检查是否已收集到extern列表中
                        size_t existing_idx = 0;
                        bool in_extern_list = false;
                        for (size_t ei = 0; ei < extern_func_count; ei++) {
                            if (strcmp(extern_func_names[ei], called_func_name) == 0) {
                                in_extern_list = true;
                                existing_idx = ei;
                                break;
                            }
                        }
                        
                        // 如果未声明，添加到extern列表
                        if (!in_extern_list && extern_func_count < MAX_EXTERN_FUNCS) {
                            extern_func_names[extern_func_count] = called_func_name;
                            // 尝试从CALL指令的dest.type获取返回类型
                            extern_func_ret_types[extern_func_count] =
                                (scan_inst->dest.kind == CN_IR_OP_REG && scan_inst->dest.type)
                                ? scan_inst->dest.type : NULL;
                            // 【P5】保存CALL指令指针以获取参数类型信息
                            extern_call_insts[extern_func_count] = scan_inst;
                            extern_func_count++;
                        } else if (in_extern_list) {
                            // 【P5】已存在，如果新调用有更多参数则更新指令指针
                            if (scan_inst->extra_args_count > extern_call_insts[existing_idx]->extra_args_count) {
                                extern_call_insts[existing_idx] = scan_inst;
                            }
                        }
                    }
                    scan_inst = scan_inst->next;
                }
                scan_block = scan_block->next;
            }
            scan_func = scan_func->next;
        }
        
        // 生成ANSI原型风格的extern声明
        if (extern_func_count > 0) {
            fprintf(file, "// Extern Declarations - 跨模块调用的函数（ANSI原型风格）\n");
            for (size_t ei = 0; ei < extern_func_count; ei++) {
                // 【P5】使用get_extern_type_string获取安全的类型字符串
                const char *ret_type_str = get_extern_type_string(extern_func_ret_types[ei], false);
                CnIrInst *call_inst = extern_call_insts[ei];
                fprintf(file, "extern %s %s(", ret_type_str,
                        get_c_function_name(extern_func_names[ei]));
                // 【P5】生成ANSI原型参数列表
                if (call_inst && call_inst->extra_args_count > 0) {
                    for (size_t pi = 0; pi < call_inst->extra_args_count; pi++) {
                        CnType *param_type = call_inst->extra_args[pi].type;
                        fprintf(file, "%s", get_extern_type_string(param_type, true));
                        if (pi < call_inst->extra_args_count - 1) fprintf(file, ", ");
                    }
                } else {
                    fprintf(file, "void");  // 无参数时使用void而非空列表
                }
                fprintf(file, ");\n");
            }
            fprintf(file, "\n");
        }
        #undef MAX_EXTERN_FUNCS
    }

    func = module->first_func;
    while (func) {
        // 跳过与运行时库函数冲突的函数定义（它们已在运行时库中实现）
        if (is_runtime_function_conflict(func->name)) {
            func = func->next;
            continue;
        }
        cn_cgen_function(&ctx, func);
        func = func->next;
    }
    
    // 清理全局查找表
    g_local_struct_infos = NULL;
    g_local_struct_count = 0;
    
    // 释放局部结构体信息列表
    if (local_struct_infos) {
        free(local_struct_infos);
    }
    
    // 释放已声明函数集合
    if (ctx.declared_functions) {
        cn_rt_map_destroy(ctx.declared_functions);
        ctx.declared_functions = NULL;
    }
    
    fclose(file);
    
    /* 释放缓冲区 */
    if (buffer) {
        free(buffer);
    }
    
    return 0;
}
