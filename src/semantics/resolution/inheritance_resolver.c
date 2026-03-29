/**
 * @file inheritance_resolver.c
 * @brief CN语言面向对象编程 - 继承关系解析器实现
 * 
 * 实现继承关系解析器的功能：
 * - 类注册与管理
 * - 继承关系构建
 * - 循环继承检测
 * - 继承链遍历查询
 */

#include <stdlib.h>
#include <string.h>
#include <cnlang/semantics/inheritance_resolver.h>

/* ============================================================================
 * 内部常量定义
 * ============================================================================ */

/** 初始节点容量 */
#define INITIAL_NODE_CAPACITY 16

/** 初始派生类容量 */
#define INITIAL_DERIVED_CAPACITY 4

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 比较类名是否相等
 * 
 * @param name1 类名1
 * @param len1 类名1长度
 * @param name2 类名2
 * @param len2 类名2长度
 * @return bool 相等返回true
 */
static bool class_name_equals(const char *name1, size_t len1,
                               const char *name2, size_t len2) {
    if (len1 != len2) return false;
    return memcmp(name1, name2, len1) == 0;
}

/**
 * @brief 比较类名与C字符串是否相等
 * 
 * @param name 类名
 * @param len 类名长度
 * @param cstr C字符串（以NULL结尾）
 * @return bool 相等返回true
 */
static bool class_name_equals_cstr(const char *name, size_t len, const char *cstr) {
    size_t cstr_len = strlen(cstr);
    return class_name_equals(name, len, cstr, cstr_len);
}

/**
 * @brief 初始化继承节点
 * 
 * @param node 节点指针
 * @param class_decl 类声明
 * @return bool 成功返回true
 */
static bool inheritance_node_init(CnInheritanceNode *node, CnAstClassDecl *class_decl) {
    if (!node || !class_decl) return false;
    
    node->class_decl = class_decl;
    node->derived_classes = NULL;
    node->derived_count = 0;
    node->derived_capacity = 0;
    node->depth = 0;
    node->visited = false;
    
    return true;
}

/**
 * @brief 清理继承节点
 * 
 * @param node 节点指针
 */
static void inheritance_node_cleanup(CnInheritanceNode *node) {
    if (!node) return;
    
    if (node->derived_classes) {
        free(node->derived_classes);
        node->derived_classes = NULL;
    }
    node->derived_count = 0;
    node->derived_capacity = 0;
}

/**
 * @brief 添加派生类到节点
 * 
 * @param node 父类节点
 * @param derived 派生类节点
 * @return bool 成功返回true
 */
static bool inheritance_node_add_derived(CnInheritanceNode *node,
                                          CnInheritanceNode *derived) {
    if (!node || !derived) return false;
    
    /* 检查是否需要扩容 */
    if (node->derived_count >= node->derived_capacity) {
        size_t new_capacity = node->derived_capacity == 0 
                              ? INITIAL_DERIVED_CAPACITY 
                              : node->derived_capacity * 2;
        CnInheritanceNode **new_array = realloc(node->derived_classes,
                                                 new_capacity * sizeof(CnInheritanceNode*));
        if (!new_array) return false;
        
        node->derived_classes = new_array;
        node->derived_capacity = new_capacity;
    }
    
    node->derived_classes[node->derived_count++] = derived;
    return true;
}

/* ============================================================================
 * 创建和销毁函数实现
 * ============================================================================ */

CnInheritanceResolver *cn_inheritance_resolver_create(CnDiagnostics *diag) {
    CnInheritanceResolver *resolver = malloc(sizeof(CnInheritanceResolver));
    if (!resolver) return NULL;
    
    resolver->nodes = malloc(INITIAL_NODE_CAPACITY * sizeof(CnInheritanceNode));
    if (!resolver->nodes) {
        free(resolver);
        return NULL;
    }
    
    resolver->node_count = 0;
    resolver->node_capacity = INITIAL_NODE_CAPACITY;
    resolver->diagnostics = diag;
    
    return resolver;
}

void cn_inheritance_resolver_destroy(CnInheritanceResolver *resolver) {
    if (!resolver) return;
    
    /* 清理所有节点 */
    for (size_t i = 0; i < resolver->node_count; i++) {
        inheritance_node_cleanup(&resolver->nodes[i]);
    }
    
    if (resolver->nodes) {
        free(resolver->nodes);
    }
    
    free(resolver);
}

/* ============================================================================
 * 类注册函数实现
 * ============================================================================ */

bool cn_inheritance_resolver_register(CnInheritanceResolver *resolver,
                                       CnAstClassDecl *class_decl) {
    if (!resolver || !class_decl) return false;
    
    /* 检查是否已注册 */
    for (size_t i = 0; i < resolver->node_count; i++) {
        if (resolver->nodes[i].class_decl == class_decl) {
            return true;  /* 已注册 */
        }
    }
    
    /* 检查是否需要扩容 */
    if (resolver->node_count >= resolver->node_capacity) {
        size_t new_capacity = resolver->node_capacity * 2;
        CnInheritanceNode *new_nodes = realloc(resolver->nodes,
                                                new_capacity * sizeof(CnInheritanceNode));
        if (!new_nodes) return false;
        
        resolver->nodes = new_nodes;
        resolver->node_capacity = new_capacity;
    }
    
    /* 初始化新节点 */
    CnInheritanceNode *node = &resolver->nodes[resolver->node_count];
    if (!inheritance_node_init(node, class_decl)) {
        return false;
    }
    
    resolver->node_count++;
    return true;
}

/* ============================================================================
 * 继承关系解析函数实现
 * ============================================================================ */

bool cn_inheritance_resolver_resolve(CnInheritanceResolver *resolver) {
    if (!resolver) return false;
    
    /* 第一遍：建立类名到节点的映射，计算继承深度 */
    for (size_t i = 0; i < resolver->node_count; i++) {
        CnInheritanceNode *node = &resolver->nodes[i];
        CnAstClassDecl *class_decl = node->class_decl;
        
        /* 处理每个基类 */
        for (size_t j = 0; j < class_decl->base_count; j++) {
            CnInheritanceInfo *base_info = &class_decl->bases[j];
            
            /* 查找基类节点 */
            CnInheritanceNode *base_node = NULL;
            for (size_t k = 0; k < resolver->node_count; k++) {
                CnAstClassDecl *potential_base = resolver->nodes[k].class_decl;
                if (class_name_equals(base_info->base_class_name,
                                       base_info->base_class_name_length,
                                       potential_base->name,
                                       potential_base->name_length)) {
                    base_node = &resolver->nodes[k];
                    break;
                }
            }
            
            if (base_node) {
                /* 添加派生类关系 */
                if (!inheritance_node_add_derived(base_node, node)) {
                    return false;
                }
                
                /* 更新继承深度 */
                if (node->depth <= base_node->depth) {
                    node->depth = base_node->depth + 1;
                }
            }
        }
    }
    
    return true;
}

/**
 * @brief 内部循环检测辅助函数（深度优先搜索）
 * 
 * @param node 当前节点
 * @return bool 存在循环返回true
 */
static bool check_circular_dfs(CnInheritanceNode *node) {
    if (!node) return false;
    
    /* 如果已经在当前路径上访问过，则存在循环 */
    if (node->visited) {
        return true;
    }
    
    node->visited = true;
    
    /* 递归检查所有派生类 */
    for (size_t i = 0; i < node->derived_count; i++) {
        if (check_circular_dfs(node->derived_classes[i])) {
            return true;
        }
    }
    
    /* 回溯时清除访问标记 */
    node->visited = false;
    return false;
}

bool cn_inheritance_resolver_check_circular(CnInheritanceResolver *resolver) {
    if (!resolver) return false;
    
    /* 清除所有访问标记 */
    for (size_t i = 0; i < resolver->node_count; i++) {
        resolver->nodes[i].visited = false;
    }
    
    /* 从每个根类（没有基类的类）开始检测 */
    for (size_t i = 0; i < resolver->node_count; i++) {
        CnInheritanceNode *node = &resolver->nodes[i];
        
        /* 只从根类开始 */
        if (node->class_decl->base_count == 0) {
            if (check_circular_dfs(node)) {
                return true;
            }
        }
    }
    
    /* 如果没有根类，从所有节点检测 */
    for (size_t i = 0; i < resolver->node_count; i++) {
        if (check_circular_dfs(&resolver->nodes[i])) {
            return true;
        }
    }
    
    return false;
}

/* ============================================================================
 * 查询函数实现
 * ============================================================================ */

CnInheritanceNode *cn_inheritance_resolver_get_node(CnInheritanceResolver *resolver,
                                                     const char *class_name) {
    if (!resolver || !class_name) return NULL;
    
    size_t name_len = strlen(class_name);
    
    for (size_t i = 0; i < resolver->node_count; i++) {
        CnAstClassDecl *class_decl = resolver->nodes[i].class_decl;
        if (class_name_equals(class_decl->name, class_decl->name_length,
                               class_name, name_len)) {
            return &resolver->nodes[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 收集所有基类的辅助函数
 * 
 * @param node 当前节点
 * @param result 结果数组
 * @param result_count 结果数量
 * @param capacity 结果容量
 * @return bool 成功返回true
 */
static bool collect_all_bases(CnInheritanceNode *node,
                               CnAstClassDecl **result,
                               size_t *result_count,
                               size_t capacity) {
    if (!node || !result || !result_count) return false;
    
    CnAstClassDecl *class_decl = node->class_decl;
    
    /* 处理每个直接基类 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 查找基类节点 */
        CnInheritanceNode *base_node = NULL;
        for (size_t k = 0; k < node->derived_count + 1; k++) {
            /* 这里需要遍历所有节点来查找基类 */
            /* 简化实现：通过基类名查找 */
        }
        
        /* 检查是否已添加 */
        bool already_added = false;
        for (size_t j = 0; j < *result_count; j++) {
            if (result[j] && class_name_equals_cstr(result[j]->name,
                                                     result[j]->name_length,
                                                     base_info->base_class_name)) {
                already_added = true;
                break;
            }
        }
        
        if (!already_added && *result_count < capacity) {
            /* 这里需要实际的基类指针，简化实现中返回NULL */
        }
    }
    
    return true;
}

CnAstClassDecl **cn_inheritance_resolver_get_all_bases(CnInheritanceResolver *resolver,
                                                        CnAstClassDecl *class_decl,
                                                        size_t *count) {
    if (!resolver || !class_decl || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    /* 计算最大可能的基类数量（继承链长度） */
    size_t max_bases = 16;  /* 假设最大继承深度 */
    CnAstClassDecl **result = malloc(max_bases * sizeof(CnAstClassDecl*));
    if (!result) {
        *count = 0;
        return NULL;
    }
    
    size_t result_count = 0;
    
    /* 使用队列进行广度优先遍历收集基类 */
    CnAstClassDecl **queue = malloc(max_bases * sizeof(CnAstClassDecl*));
    if (!queue) {
        free(result);
        *count = 0;
        return NULL;
    }
    
    size_t queue_head = 0;
    size_t queue_tail = 0;
    
    /* 将直接基类加入队列 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
            resolver, base_info->base_class_name);
        
        if (base_node && queue_tail < max_bases) {
            queue[queue_tail++] = base_node->class_decl;
        }
    }
    
    /* 广度优先遍历 */
    while (queue_head < queue_tail && result_count < max_bases) {
        CnAstClassDecl *current = queue[queue_head++];
        
        /* 检查是否已添加 */
        bool already_added = false;
        for (size_t i = 0; i < result_count; i++) {
            if (result[i] == current) {
                already_added = true;
                break;
            }
        }
        
        if (!already_added) {
            result[result_count++] = current;
            
            /* 将当前类的基类加入队列 */
            for (size_t i = 0; i < current->base_count; i++) {
                CnInheritanceInfo *base_info = &current->bases[i];
                CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
                    resolver, base_info->base_class_name);
                
                if (base_node && queue_tail < max_bases) {
                    queue[queue_tail++] = base_node->class_decl;
                }
            }
        }
    }
    
    free(queue);
    *count = result_count;
    return result;
}

bool cn_inheritance_resolver_is_derived_from(CnInheritanceResolver *resolver,
                                              const char *derived,
                                              const char *base) {
    if (!resolver || !derived || !base) return false;
    
    /* 获取派生类节点 */
    CnInheritanceNode *derived_node = cn_inheritance_resolver_get_node(resolver, derived);
    if (!derived_node) return false;
    
    /* 如果派生类名和基类名相同，返回false */
    if (strcmp(derived, base) == 0) return false;
    
    /* 获取所有基类 */
    size_t base_count = 0;
    CnAstClassDecl **all_bases = cn_inheritance_resolver_get_all_bases(
        resolver, derived_node->class_decl, &base_count);
    
    if (!all_bases) return false;
    
    /* 检查基类是否在列表中 */
    bool found = false;
    for (size_t i = 0; i < base_count; i++) {
        if (class_name_equals_cstr(all_bases[i]->name,
                                    all_bases[i]->name_length,
                                    base)) {
            found = true;
            break;
        }
    }
    
    free(all_bases);
    return found;
}

int cn_inheritance_resolver_get_depth(CnInheritanceResolver *resolver,
                                       const char *class_name) {
    if (!resolver || !class_name) return -1;
    
    CnInheritanceNode *node = cn_inheritance_resolver_get_node(resolver, class_name);
    if (!node) return -1;
    
    return node->depth;
}

CnAstClassDecl **cn_inheritance_resolver_get_direct_bases(CnInheritanceResolver *resolver,
                                                           CnAstClassDecl *class_decl,
                                                           size_t *count) {
    if (!resolver || !class_decl || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    size_t base_count = class_decl->base_count;
    if (base_count == 0) {
        *count = 0;
        return NULL;
    }
    
    CnAstClassDecl **result = malloc(base_count * sizeof(CnAstClassDecl*));
    if (!result) {
        *count = 0;
        return NULL;
    }
    
    size_t result_count = 0;
    
    for (size_t i = 0; i < base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
            resolver, base_info->base_class_name);
        
        if (base_node) {
            result[result_count++] = base_node->class_decl;
        }
    }
    
    *count = result_count;
    return result;
}

/* ============================================================================
 * 多继承语义检查函数实现
 * ============================================================================ */

bool cn_inheritance_resolver_check_bases_exist(CnInheritanceResolver *resolver,
                                                CnAstClassDecl *class_decl,
                                                const char **missing_base_name,
                                                size_t *missing_base_len) {
    if (!resolver || !class_decl) return false;
    
    /* 遍历所有基类 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 查找基类节点 */
        CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
            resolver, base_info->base_class_name);
        
        if (!base_node) {
            /* 基类不存在 */
            if (missing_base_name) {
                *missing_base_name = base_info->base_class_name;
            }
            if (missing_base_len) {
                *missing_base_len = base_info->base_class_name_length;
            }
            return false;
        }
    }
    
    return true;
}

bool cn_inheritance_resolver_check_duplicate_bases(CnInheritanceResolver *resolver,
                                                    CnAstClassDecl *class_decl,
                                                    const char **duplicate_name,
                                                    size_t *duplicate_len) {
    if (!resolver || !class_decl) return true;  /* 无基类或单基类不可能重复 */
    
    /* 检查直接基类列表中的重复 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_i = &class_decl->bases[i];
        
        for (size_t j = i + 1; j < class_decl->base_count; j++) {
            CnInheritanceInfo *base_j = &class_decl->bases[j];
            
            /* 比较基类名 */
            if (class_name_equals(base_i->base_class_name,
                                   base_i->base_class_name_length,
                                   base_j->base_class_name,
                                   base_j->base_class_name_length)) {
                /* 发现重复基类 */
                if (duplicate_name) {
                    *duplicate_name = base_i->base_class_name;
                }
                if (duplicate_len) {
                    *duplicate_len = base_i->base_class_name_length;
                }
                return false;
            }
        }
    }
    
    return true;
}

bool cn_inheritance_resolver_check_base_access(CnInheritanceResolver *resolver,
                                                CnAstClassDecl *class_decl,
                                                const char **invalid_base_name,
                                                size_t *invalid_base_len) {
    if (!resolver || !class_decl) return false;
    
    /* 遍历所有基类 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 查找基类节点 */
        CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
            resolver, base_info->base_class_name);
        
        if (base_node) {
            CnAstClassDecl *base_decl = base_node->class_decl;
            
            /* 检查是否为最终类 */
            if (base_decl->is_final) {
                if (invalid_base_name) {
                    *invalid_base_name = base_info->base_class_name;
                }
                if (invalid_base_len) {
                    *invalid_base_len = base_info->base_class_name_length;
                }
                return false;
            }
            
            /* 检查继承访问级别（私有继承不可访问） */
            if (base_info->access == CN_ACCESS_PRIVATE) {
                if (invalid_base_name) {
                    *invalid_base_name = base_info->base_class_name;
                }
                if (invalid_base_len) {
                    *invalid_base_len = base_info->base_class_name_length;
                }
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 内部循环继承检测辅助函数（从指定类开始）
 *
 * @param resolver 解析器
 * @param class_decl 当前类
 * @param visited 已访问类集合
 * @param visited_count 已访问类数量
 * @param circular_name 输出循环类名
 * @param circular_len 输出循环类名长度
 * @return bool 存在循环返回true
 */
static bool check_circular_from_class(CnInheritanceResolver *resolver,
                                       CnAstClassDecl *class_decl,
                                       CnAstClassDecl **visited,
                                       size_t *visited_count,
                                       const char **circular_name,
                                       size_t *circular_len) {
    /* 检查是否已在访问路径中 */
    for (size_t i = 0; i < *visited_count; i++) {
        if (visited[i] == class_decl) {
            /* 发现循环 */
            if (circular_name) {
                *circular_name = class_decl->name;
            }
            if (circular_len) {
                *circular_len = class_decl->name_length;
            }
            return true;
        }
    }
    
    /* 添加到访问路径 */
    if (*visited_count < 32) {  /* 限制最大深度防止栈溢出 */
        visited[(*visited_count)++] = class_decl;
    }
    
    /* 递归检查所有基类 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
            resolver, base_info->base_class_name);
        
        if (base_node) {
            if (check_circular_from_class(resolver, base_node->class_decl,
                                          visited, visited_count,
                                          circular_name, circular_len)) {
                return true;
            }
        }
    }
    
    /* 回溯 */
    (*visited_count)--;
    return false;
}

CnMultiInheritCheckResult cn_inheritance_resolver_check_multi_inherit(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    const char **error_base_name,
    size_t *error_base_len) {
    
    if (!resolver || !class_decl) {
        return CN_MULTI_INHERIT_BASE_NOT_FOUND;
    }
    
    /* 1. 检查所有基类是否存在 */
    if (!cn_inheritance_resolver_check_bases_exist(resolver, class_decl,
                                                    error_base_name, error_base_len)) {
        return CN_MULTI_INHERIT_BASE_NOT_FOUND;
    }
    
    /* 2. 检查重复基类 */
    if (!cn_inheritance_resolver_check_duplicate_bases(resolver, class_decl,
                                                        error_base_name, error_base_len)) {
        return CN_MULTI_INHERIT_DUPLICATE_BASE;
    }
    
    /* 3. 检查基类访问级别 */
    if (!cn_inheritance_resolver_check_base_access(resolver, class_decl,
                                                    error_base_name, error_base_len)) {
        return CN_MULTI_INHERIT_FINAL_BASE;
    }
    
    /* 4. 检查循环继承 */
    CnAstClassDecl *visited[32];
    size_t visited_count = 0;
    if (check_circular_from_class(resolver, class_decl, visited, &visited_count,
                                   error_base_name, error_base_len)) {
        return CN_MULTI_INHERIT_CIRCULAR;
    }
    
    return CN_MULTI_INHERIT_OK;
}

/* ============================================================================
 * 方法解析顺序（MRO）函数实现
 * ============================================================================ */

CnMroList *cn_mro_list_create(size_t capacity) {
    if (capacity == 0) capacity = 8;
    
    CnMroList *list = malloc(sizeof(CnMroList));
    if (!list) return NULL;
    
    list->classes = calloc(capacity, sizeof(CnAstClassDecl*));
    if (!list->classes) {
        free(list);
        return NULL;
    }
    
    list->count = 0;
    list->capacity = capacity;
    return list;
}

void cn_mro_list_destroy(CnMroList *list) {
    if (!list) return;
    
    if (list->classes) {
        free(list->classes);
    }
    free(list);
}

/**
 * @brief 向MRO列表添加类
 *
 * @param list MRO列表
 * @param class_decl 要添加的类
 * @return bool 成功返回true
 */
static bool mro_list_append(CnMroList *list, CnAstClassDecl *class_decl) {
    if (!list || !class_decl) return false;
    
    /* 检查是否需要扩容 */
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        CnAstClassDecl **new_classes = realloc(list->classes,
                                                new_capacity * sizeof(CnAstClassDecl*));
        if (!new_classes) return false;
        
        list->classes = new_classes;
        list->capacity = new_capacity;
    }
    
    list->classes[list->count++] = class_decl;
    return true;
}

/**
 * @brief 检查类是否在MRO列表中
 *
 * @param list MRO列表
 * @param class_decl 要检查的类
 * @return bool 存在返回true
 */
static bool mro_list_contains(CnMroList *list, CnAstClassDecl *class_decl) {
    if (!list || !class_decl) return false;
    
    for (size_t i = 0; i < list->count; i++) {
        if (list->classes[i] == class_decl) {
            return true;
        }
    }
    return false;
}

/**
 * @brief C3线性化合并算法
 *
 * 合并多个MRO列表为一个
 *
 * @param mro_lists MRO列表数组
 * @param list_count 列表数量
 * @param result 结果MRO列表
 * @param resolver 解析器
 * @return bool 成功返回true
 */
static bool c3_merge(CnMroList **mro_lists, size_t list_count,
                     CnMroList *result, CnInheritanceResolver *resolver) {
    /* 简化实现：按顺序合并，跳过重复 */
    for (size_t i = 0; i < list_count; i++) {
        CnMroList *list = mro_lists[i];
        if (!list) continue;
        
        for (size_t j = 0; j < list->count; j++) {
            if (!mro_list_contains(result, list->classes[j])) {
                mro_list_append(result, list->classes[j]);
            }
        }
    }
    return true;
}

CnMroList *cn_inheritance_resolver_compute_mro(CnInheritanceResolver *resolver,
                                                CnAstClassDecl *class_decl) {
    if (!resolver || !class_decl) return NULL;
    
    /* 创建结果列表 */
    CnMroList *result = cn_mro_list_create(16);
    if (!result) return NULL;
    
    /* 首先添加当前类 */
    mro_list_append(result, class_decl);
    
    /* 如果没有基类，直接返回 */
    if (class_decl->base_count == 0) {
        return result;
    }
    
    /* 收集所有基类的MRO */
    CnMroList **base_mros = malloc(class_decl->base_count * sizeof(CnMroList*));
    if (!base_mros) {
        cn_mro_list_destroy(result);
        return NULL;
    }
    
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
            resolver, base_info->base_class_name);
        
        if (base_node) {
            base_mros[i] = cn_inheritance_resolver_compute_mro(resolver,
                                                                base_node->class_decl);
        } else {
            base_mros[i] = NULL;
        }
    }
    
    /* 使用C3线性化合并 */
    c3_merge(base_mros, class_decl->base_count, result, resolver);
    
    /* 清理临时MRO列表 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        if (base_mros[i]) {
            cn_mro_list_destroy(base_mros[i]);
        }
    }
    free(base_mros);
    
    return result;
}

CnClassMember *cn_mro_find_method(CnMroList *mro,
                                   const char *method_name,
                                   size_t method_name_len) {
    if (!mro || !method_name || method_name_len == 0) return NULL;
    
    /* 按MRO顺序查找方法 */
    for (size_t i = 0; i < mro->count; i++) {
        CnAstClassDecl *class_decl = mro->classes[i];
        
        /* 遍历类的所有成员 */
        for (size_t j = 0; j < class_decl->member_count; j++) {
            CnClassMember *member = &class_decl->members[j];
            
            /* 检查是否为方法且名称匹配 */
            if (member->kind == CN_MEMBER_METHOD &&
                class_name_equals(member->name, member->name_length,
                                  method_name, method_name_len)) {
                return member;
            }
        }
    }
    
    return NULL;
}

/* ============================================================================
 * 虚继承语义检查函数实现
 * ============================================================================ */

/**
 * @brief 检查基类是否为虚基类
 */
bool cn_inheritance_is_virtual_base(const CnAstClassDecl *class_decl,
                                     const char *base_name,
                                     size_t base_name_len) {
    if (!class_decl || !base_name) return false;
    
    for (size_t i = 0; i < class_decl->base_count; i++) {
        const CnInheritanceInfo *base_info = &class_decl->bases[i];
        if (class_name_equals(base_info->base_class_name, base_info->base_class_name_length,
                              base_name, base_name_len)) {
            return base_info->is_virtual;
        }
    }
    
    return false;
}

/**
 * @brief 内部辅助函数：收集类的所有间接基类
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param bases 输出基类数组
 * @param base_virtual 输出基类是否虚继承数组
 * @param count 输出基类数量
 * @param capacity 数组容量
 * @return bool 成功返回true
 */
static bool collect_all_bases_internal(CnInheritanceResolver *resolver,
                                        CnAstClassDecl *class_decl,
                                        char **bases,
                                        bool *base_virtual,
                                        size_t *count,
                                        size_t capacity) {
    if (!resolver || !class_decl || !bases || !base_virtual || !count) return false;
    
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 检查是否已存在 */
        bool found = false;
        for (size_t j = 0; j < *count; j++) {
            if (class_name_equals(bases[j], strlen(bases[j]),
                                  base_info->base_class_name, base_info->base_class_name_length)) {
                found = true;
                /* 如果已有记录且当前是虚继承，更新标记 */
                if (base_info->is_virtual) {
                    base_virtual[j] = true;
                }
                break;
            }
        }
        
        if (!found && *count < capacity) {
            /* 添加新基类 */
            char *name_copy = malloc(base_info->base_class_name_length + 1);
            if (name_copy) {
                memcpy(name_copy, base_info->base_class_name, base_info->base_class_name_length);
                name_copy[base_info->base_class_name_length] = '\0';
                bases[*count] = name_copy;
                base_virtual[*count] = base_info->is_virtual;
                (*count)++;
            }
        }
        
        /* 递归收集间接基类 */
        CnInheritanceNode *base_node = cn_inheritance_resolver_get_node(
            resolver, base_info->base_class_name);
        if (base_node) {
            collect_all_bases_internal(resolver, base_node->class_decl,
                                        bases, base_virtual, count, capacity);
        }
    }
    
    return true;
}

CnVirtualInheritCheckResult cn_inheritance_resolver_check_diamond(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    const char **ambiguous_base_name,
    size_t *ambiguous_base_len) {
    
    if (!resolver || !class_decl) {
        return CN_VIRTUAL_INHERIT_BASE_NOT_FOUND;
    }
    
    /* 收集所有基类（包括间接基类） */
    char *all_bases[64];
    bool base_is_virtual[64];
    size_t base_count = 0;
    
    /* 初始化数组 */
    memset(all_bases, 0, sizeof(all_bases));
    memset(base_is_virtual, 0, sizeof(base_is_virtual));
    
    collect_all_bases_internal(resolver, class_decl, all_bases, base_is_virtual, &base_count, 64);
    
    /* 检查是否有重复的非虚基类 */
    for (size_t i = 0; i < base_count; i++) {
        for (size_t j = i + 1; j < base_count; j++) {
            if (all_bases[i] && all_bases[j] &&
                strcmp(all_bases[i], all_bases[j]) == 0) {
                /* 找到重复基类，检查是否为虚继承 */
                if (!base_is_virtual[i]) {
                    /* 非虚继承的重复基类，存在歧义 */
                    if (ambiguous_base_name) {
                        *ambiguous_base_name = all_bases[i];
                    }
                    if (ambiguous_base_len) {
                        *ambiguous_base_len = strlen(all_bases[i]);
                    }
                    
                    /* 清理 */
                    for (size_t k = 0; k < base_count; k++) {
                        if (all_bases[k]) free((void*)all_bases[k]);
                    }
                    
                    return CN_VIRTUAL_INHERIT_AMBIGUOUS_BASE;
                }
            }
        }
    }
    
    /* 清理 */
    for (size_t k = 0; k < base_count; k++) {
        if (all_bases[k]) free((void*)all_bases[k]);
    }
    
    return CN_VIRTUAL_INHERIT_OK;
}

CnVirtualInheritCheckResult cn_inheritance_resolver_check_virtual_consistency(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    const char **conflict_base_name,
    size_t *conflict_base_len) {
    
    if (!resolver || !class_decl) {
        return CN_VIRTUAL_INHERIT_BASE_NOT_FOUND;
    }
    
    /* 检查每个直接基类的虚继承一致性 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 检查其他继承路径 */
        for (size_t j = i + 1; j < class_decl->base_count; j++) {
            CnInheritanceInfo *other_info = &class_decl->bases[j];
            
            /* 如果两个基类同名但虚继承方式不同 */
            if (class_name_equals(base_info->base_class_name, base_info->base_class_name_length,
                                  other_info->base_class_name, other_info->base_class_name_length)) {
                if (base_info->is_virtual != other_info->is_virtual) {
                    /* 虚继承与非虚继承冲突 */
                    if (conflict_base_name) {
                        *conflict_base_name = base_info->base_class_name;
                    }
                    if (conflict_base_len) {
                        *conflict_base_len = base_info->base_class_name_length;
                    }
                    return CN_VIRTUAL_INHERIT_CONFLICT_VIRTUAL;
                }
            }
        }
    }
    
    return CN_VIRTUAL_INHERIT_OK;
}

CnAstClassDecl **cn_inheritance_resolver_get_virtual_bases(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    size_t *count) {
    
    if (!resolver || !class_decl || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    *count = 0;
    
    /* 计算虚基类数量 */
    size_t vb_count = 0;
    for (size_t i = 0; i < class_decl->base_count; i++) {
        if (class_decl->bases[i].is_virtual) {
            vb_count++;
        }
    }
    
    if (vb_count == 0) {
        return NULL;
    }
    
    /* 分配数组 */
    CnAstClassDecl **result = malloc(vb_count * sizeof(CnAstClassDecl*));
    if (!result) return NULL;
    
    /* 收集虚基类 */
    size_t idx = 0;
    for (size_t i = 0; i < class_decl->base_count; i++) {
        if (class_decl->bases[i].is_virtual) {
            CnInheritanceNode *node = cn_inheritance_resolver_get_node(
                resolver, class_decl->bases[i].base_class_name);
            if (node) {
                result[idx++] = node->class_decl;
            }
        }
    }
    
    *count = idx;
    return result;
}
