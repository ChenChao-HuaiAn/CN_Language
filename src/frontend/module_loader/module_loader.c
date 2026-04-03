/**
 * @file module_loader.c
 * @brief CN语言 Python 风格模块系统 - 模块加载器实现
 * 
 * 本文件实现模块加载器的核心功能。
 * 
 * @version 1.0
 * @date 2026-01-29
 */

#include "cnlang/frontend/module_loader.h"
#include "cnlang/frontend/ast.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#include <sys/stat.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

// ============================================================================
// 包初始化文件识别（B7）
// ============================================================================

/**
 * @brief 检查文件名是否为包初始化文件
 */
int cn_is_package_init_file(const char *filename)
{
    if (!filename) {
        return 0;
    }
    
    // 比较文件名是否为 "__包__.cn"
    return strcmp(filename, CN_PACKAGE_INIT_FILENAME) == 0;
}

/**
 * @brief 从完整路径中提取文件名
 */
static const char *get_filename_from_path(const char *file_path)
{
    if (!file_path) {
        return NULL;
    }
    
    // 查找最后一个路径分隔符
    const char *last_sep = strrchr(file_path, PATH_SEPARATOR);
    
#ifdef _WIN32
    // Windows 也支持正斜杠
    const char *last_forward_sep = strrchr(file_path, '/');
    if (last_forward_sep > last_sep) {
        last_sep = last_forward_sep;
    }
#endif
    
    if (last_sep) {
        return last_sep + 1;
    }
    
    // 没有路径分隔符，整个字符串就是文件名
    return file_path;
}

/**
 * @brief 检查完整路径是否指向包初始化文件
 */
int cn_is_package_init_path(const char *file_path)
{
    if (!file_path) {
        return 0;
    }
    
    const char *filename = get_filename_from_path(file_path);
    return cn_is_package_init_file(filename);
}

/**
 * @brief 检查目录是否为包（包含 __包__.cn 文件）
 */
int cn_is_package_directory(const char *dir_path)
{
    if (!dir_path) {
        return 0;
    }
    
    // 构建 __包__.cn 的完整路径
    size_t dir_len = strlen(dir_path);
    size_t filename_len = strlen(CN_PACKAGE_INIT_FILENAME);
    size_t path_len = dir_len + 1 + filename_len + 1;  // dir + sep + filename + null
    
    char *init_path = (char *)malloc(path_len);
    if (!init_path) {
        return 0;
    }
    
    snprintf(init_path, path_len, "%s%s%s", dir_path, PATH_SEPARATOR_STR, CN_PACKAGE_INIT_FILENAME);
    
    // 检查文件是否存在
    int exists = 0;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(init_path);
    exists = (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    exists = (stat(init_path, &st) == 0 && S_ISREG(st.st_mode));
#endif
    
    free(init_path);
    return exists;
}

// ============================================================================
// 模块标识符操作（A4）- 基础实现
// ============================================================================

/**
 * @brief 创建模块标识符
 */
CnModuleId *cn_module_id_create(const char *qualified_name)
{
    if (!qualified_name) {
        return NULL;
    }
    
    CnModuleId *id = (CnModuleId *)malloc(sizeof(CnModuleId));
    if (!id) {
        return NULL;
    }
    
    // 复制完全限定名
    size_t name_len = strlen(qualified_name);
    id->qualified_name = (char *)malloc(name_len + 1);
    if (!id->qualified_name) {
        free(id);
        return NULL;
    }
    strcpy(id->qualified_name, qualified_name);
    id->qualified_name_length = name_len;
    
    // 计算路径段数量
    size_t segment_count = 1;
    for (size_t i = 0; i < name_len; i++) {
        if (qualified_name[i] == '.') {
            segment_count++;
        }
    }
    
    // 分配路径段数组
    id->segments = (char **)malloc(sizeof(char *) * segment_count);
    if (!id->segments) {
        free(id->qualified_name);
        free(id);
        return NULL;
    }
    id->segment_count = segment_count;
    
    // 解析路径段
    const char *start = qualified_name;
    size_t seg_idx = 0;
    for (size_t i = 0; i <= name_len; i++) {
        if (qualified_name[i] == '.' || qualified_name[i] == '\0') {
            size_t seg_len = &qualified_name[i] - start;
            id->segments[seg_idx] = (char *)malloc(seg_len + 1);
            if (!id->segments[seg_idx]) {
                // 释放已分配的段
                for (size_t j = 0; j < seg_idx; j++) {
                    free(id->segments[j]);
                }
                free(id->segments);
                free(id->qualified_name);
                free(id);
                return NULL;
            }
            memcpy(id->segments[seg_idx], start, seg_len);
            id->segments[seg_idx][seg_len] = '\0';
            seg_idx++;
            start = &qualified_name[i + 1];
        }
    }
    
    return id;
}

/**
 * @brief 释放模块标识符
 */
void cn_module_id_free(CnModuleId *id)
{
    if (!id) {
        return;
    }
    
    if (id->segments) {
        for (size_t i = 0; i < id->segment_count; i++) {
            free(id->segments[i]);
        }
        free(id->segments);
    }
    
    free(id->qualified_name);
    free(id);
}

/**
 * @brief 比较两个模块标识符是否相等
 */
int cn_module_id_equals(const CnModuleId *a, const CnModuleId *b)
{
    if (!a || !b) {
        return a == b;
    }
    
    if (a->qualified_name_length != b->qualified_name_length) {
        return 0;
    }
    
    return strcmp(a->qualified_name, b->qualified_name) == 0;
}

/**
 * @brief 计算模块标识符的哈希值
 */
uint32_t cn_module_id_hash(const CnModuleId *id)
{
    if (!id || !id->qualified_name) {
        return 0;
    }
    
    // 使用 djb2 哈希算法
    uint32_t hash = 5381;
    const char *str = id->qualified_name;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

// ============================================================================
// 模块搜索路径配置（A6）- 基础实现
// ============================================================================

/**
 * @brief 创建搜索路径配置
 */
CnModuleSearchConfig *cn_search_config_create(void)
{
    CnModuleSearchConfig *config = (CnModuleSearchConfig *)malloc(sizeof(CnModuleSearchConfig));
    if (!config) {
        return NULL;
    }
    
    config->paths = NULL;
    config->path_count = 0;
    config->path_capacity = 0;
    config->project_root = NULL;
    config->project_root_length = 0;
    config->stdlib_path = NULL;
    config->stdlib_path_length = 0;
    
    return config;
}

/**
 * @brief 释放搜索路径配置
 */
void cn_search_config_free(CnModuleSearchConfig *config)
{
    if (!config) {
        return;
    }
    
    if (config->paths) {
        for (size_t i = 0; i < config->path_count; i++) {
            free(config->paths[i].path);
        }
        free(config->paths);
    }
    
    free(config->project_root);
    free(config->stdlib_path);
    free(config);
}

/**
 * @brief 添加搜索路径
 */
int cn_search_config_add_path(CnModuleSearchConfig *config, const char *path, int priority)
{
    if (!config || !path) {
        return 0;
    }
    
    // 扩容检查
    if (config->path_count >= config->path_capacity) {
        size_t new_capacity = config->path_capacity == 0 ? 4 : config->path_capacity * 2;
        CnSearchPathEntry *new_paths = (CnSearchPathEntry *)realloc(
            config->paths, sizeof(CnSearchPathEntry) * new_capacity);
        if (!new_paths) {
            return 0;
        }
        config->paths = new_paths;
        config->path_capacity = new_capacity;
    }
    
    // 复制路径
    size_t path_len = strlen(path);
    char *path_copy = (char *)malloc(path_len + 1);
    if (!path_copy) {
        return 0;
    }
    strcpy(path_copy, path);
    
    // 添加条目
    config->paths[config->path_count].path = path_copy;
    config->paths[config->path_count].path_length = path_len;
    config->paths[config->path_count].priority = priority;
    config->path_count++;
    
    return 1;
}

/**
 * @brief 设置项目根目录
 */
void cn_search_config_set_project_root(CnModuleSearchConfig *config, const char *project_root)
{
    if (!config) {
        return;
    }
    
    free(config->project_root);
    
    if (project_root) {
        size_t len = strlen(project_root);
        config->project_root = (char *)malloc(len + 1);
        if (config->project_root) {
            strcpy(config->project_root, project_root);
            config->project_root_length = len;
        }
    } else {
        config->project_root = NULL;
        config->project_root_length = 0;
    }
}

/**
 * @brief 设置标准库路径
 */
void cn_search_config_set_stdlib_path(CnModuleSearchConfig *config, const char *stdlib_path)
{
    if (!config) {
        return;
    }
    
    free(config->stdlib_path);
    
    if (stdlib_path) {
        size_t len = strlen(stdlib_path);
        config->stdlib_path = (char *)malloc(len + 1);
        if (config->stdlib_path) {
            strcpy(config->stdlib_path, stdlib_path);
            config->stdlib_path_length = len;
        }
    } else {
        config->stdlib_path = NULL;
        config->stdlib_path_length = 0;
    }
}

// ============================================================================
// 阶段 C：模块解析与加载
// ============================================================================

// --- 辅助函数 ---

/**
 * @brief 检查文件是否存在
 */
static int file_exists(const char *path)
{
    if (!path) {
        return 0;
    }
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
#endif
}

/**
 * @brief 检查目录是否存在
 */
static int dir_exists(const char *path)
{
    if (!path) {
        return 0;
    }
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

/**
 * @brief 拼接路径
 */
static char *path_join(const char *base, const char *name)
{
    if (!base || !name) {
        return NULL;
    }
    
    size_t base_len = strlen(base);
    size_t name_len = strlen(name);
    
    // 移除末尾分隔符
    while (base_len > 0 && (base[base_len - 1] == '/' || base[base_len - 1] == '\\')) {
        base_len--;
    }
    
    size_t total_len = base_len + 1 + name_len + 1;
    char *result = (char *)malloc(total_len);
    if (!result) {
        return NULL;
    }
    
    memcpy(result, base, base_len);
    result[base_len] = PATH_SEPARATOR;
    strcpy(result + base_len + 1, name);
    
    return result;
}

/**
 * @brief 获取文件修改时间
 */
static uint64_t get_file_mtime(const char *path)
{
    if (!path) {
        return 0;
    }
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) {
        ULARGE_INTEGER ull;
        ull.LowPart = fad.ftLastWriteTime.dwLowDateTime;
        ull.HighPart = fad.ftLastWriteTime.dwHighDateTime;
        return ull.QuadPart;
    }
    return 0;
#else
    struct stat st;
    if (stat(path, &st) == 0) {
        return (uint64_t)st.st_mtime;
    }
    return 0;
#endif
}

// ============================================================================
// C1: 模块文件发现
// ============================================================================

/**
 * @brief 在指定目录中查找模块文件
 * @param search_dir 搜索目录
 * @param module_name 模块名（单个段）
 * @return 找到的文件路径（调用者负责释放），未找到返回 NULL
 */
static char *find_module_file_in_dir(const char *search_dir, const char *module_name)
{
    if (!search_dir || !module_name) {
        return NULL;
    }
    
    // 构建模块文件名：模块名.cn
    size_t name_len = strlen(module_name);
    char *filename = (char *)malloc(name_len + 4);  // .cn + null
    if (!filename) {
        return NULL;
    }
    sprintf(filename, "%s.cn", module_name);
    
    // 拼接完整路径
    char *full_path = path_join(search_dir, filename);
    free(filename);
    
    if (!full_path) {
        return NULL;
    }
    
    // 检查文件是否存在
    if (file_exists(full_path)) {
        return full_path;
    }
    
    free(full_path);
    return NULL;
}

/**
 * @brief 根据模块标识符查找模块文件
 */
static char *find_module_file(CnModuleSearchConfig *config, const CnModuleId *id)
{
    if (!config || !id || id->segment_count == 0) {
        return NULL;
    }
    
    // 构建相对路径：包/子包/模块.cn
    char *relative_path = NULL;
    size_t path_len = 0;
    
    // 计算路径长度
    for (size_t i = 0; i < id->segment_count; i++) {
        path_len += strlen(id->segments[i]);
        if (i < id->segment_count - 1) {
            path_len += 1;  // 路径分隔符
        }
    }
    path_len += 4;  // .cn + null
    
    relative_path = (char *)malloc(path_len);
    if (!relative_path) {
        return NULL;
    }
    
    // 构建路径
    char *ptr = relative_path;
    for (size_t i = 0; i < id->segment_count; i++) {
        size_t seg_len = strlen(id->segments[i]);
        memcpy(ptr, id->segments[i], seg_len);
        ptr += seg_len;
        if (i < id->segment_count - 1) {
            *ptr++ = PATH_SEPARATOR;
        }
    }
    strcpy(ptr, ".cn");
    
    // 在搜索路径中查找
    // 1. 先检查项目根目录
    if (config->project_root) {
        char *full_path = path_join(config->project_root, relative_path);
        if (full_path && file_exists(full_path)) {
            free(relative_path);
            return full_path;
        }
        free(full_path);
    }
    
    // 2. 检查自定义搜索路径（按优先级排序）
    for (size_t i = 0; i < config->path_count; i++) {
        char *full_path = path_join(config->paths[i].path, relative_path);
        if (full_path && file_exists(full_path)) {
            free(relative_path);
            return full_path;
        }
        free(full_path);
    }
    
    // 3. 检查标准库路径
    if (config->stdlib_path) {
        char *full_path = path_join(config->stdlib_path, relative_path);
        if (full_path && file_exists(full_path)) {
            free(relative_path);
            return full_path;
        }
        free(full_path);
    }
    
    free(relative_path);
    return NULL;
}

// ============================================================================
// C2: 包目录发现
// ============================================================================

/**
 * @brief 根据模块标识符查找包目录
 */
static char *find_package_dir(CnModuleSearchConfig *config, const CnModuleId *id)
{
    if (!config || !id || id->segment_count == 0) {
        return NULL;
    }
    
    // 构建相对路径：包/子包/模块
    char *relative_path = NULL;
    size_t path_len = 0;
    
    // 计算路径长度
    for (size_t i = 0; i < id->segment_count; i++) {
        path_len += strlen(id->segments[i]);
        if (i < id->segment_count - 1) {
            path_len += 1;
        }
    }
    path_len += 1;  // null
    
    relative_path = (char *)malloc(path_len);
    if (!relative_path) {
        return NULL;
    }
    
    // 构建路径
    char *ptr = relative_path;
    for (size_t i = 0; i < id->segment_count; i++) {
        size_t seg_len = strlen(id->segments[i]);
        memcpy(ptr, id->segments[i], seg_len);
        ptr += seg_len;
        if (i < id->segment_count - 1) {
            *ptr++ = PATH_SEPARATOR;
        }
    }
    *ptr = '\0';
    
    // 在搜索路径中查找
    // 1. 先检查项目根目录
    if (config->project_root) {
        char *full_path = path_join(config->project_root, relative_path);
        if (full_path && cn_is_package_directory(full_path)) {
            free(relative_path);
            return full_path;
        }
        free(full_path);
    }
    
    // 2. 检查自定义搜索路径
    for (size_t i = 0; i < config->path_count; i++) {
        char *full_path = path_join(config->paths[i].path, relative_path);
        if (full_path && cn_is_package_directory(full_path)) {
            free(relative_path);
            return full_path;
        }
        free(full_path);
    }
    
    // 3. 检查标准库路径
    if (config->stdlib_path) {
        char *full_path = path_join(config->stdlib_path, relative_path);
        if (full_path && cn_is_package_directory(full_path)) {
            free(relative_path);
            return full_path;
        }
        free(full_path);
    }
    
    free(relative_path);
    return NULL;
}

// ============================================================================
// C3: 模块搜索路径管理器
// ============================================================================

/**
 * @brief 从环境变量加载搜索路径
 */
int cn_search_config_load_from_env(CnModuleSearchConfig *config, const char *env_var)
{
    if (!config || !env_var) {
        return 0;
    }
    
    const char *env_value = getenv(env_var);
    if (!env_value || *env_value == '\0') {
        return 1;  // 环境变量未设置，不算失败
    }
    
    // 解析路径列表（用 ; 或 : 分隔）
#ifdef _WIN32
    const char path_list_sep = ';';
#else
    const char path_list_sep = ':';
#endif
    
    const char *start = env_value;
    const char *ptr = env_value;
    int priority = 100;  // 环境变量路径的默认优先级
    
    while (*ptr != '\0') {
        if (*ptr == path_list_sep) {
            if (ptr > start) {
                // 提取路径
                size_t path_len = ptr - start;
                char *path = (char *)malloc(path_len + 1);
                if (path) {
                    memcpy(path, start, path_len);
                    path[path_len] = '\0';
                    cn_search_config_add_path(config, path, priority);
                    free(path);
                }
            }
            start = ptr + 1;
        }
        ptr++;
    }
    
    // 处理最后一个路径
    if (ptr > start) {
        cn_search_config_add_path(config, start, priority);
    }
    
    return 1;
}

// ============================================================================
// C4: 模块缓存系统
// ============================================================================

/**
 * @brief 创建模块缓存
 */
CnModuleCache *cn_module_cache_create(size_t initial_bucket_count)
{
    if (initial_bucket_count == 0) {
        initial_bucket_count = 64;
    }
    
    CnModuleCache *cache = (CnModuleCache *)malloc(sizeof(CnModuleCache));
    if (!cache) {
        return NULL;
    }
    
    cache->buckets = (CnModuleCacheEntry **)calloc(initial_bucket_count, sizeof(CnModuleCacheEntry *));
    if (!cache->buckets) {
        free(cache);
        return NULL;
    }
    
    cache->bucket_count = initial_bucket_count;
    cache->entry_count = 0;
    
    return cache;
}

/**
 * @brief 释放模块缓存
 */
void cn_module_cache_free(CnModuleCache *cache)
{
    if (!cache) {
        return;
    }
    
    // 释放所有条目
    for (size_t i = 0; i < cache->bucket_count; i++) {
        CnModuleCacheEntry *entry = cache->buckets[i];
        while (entry) {
            CnModuleCacheEntry *next = entry->next;
            // 注意：id 和 metadata 由外部管理，这里不释放
            free(entry);
            entry = next;
        }
    }
    
    free(cache->buckets);
    free(cache);
}

/**
 * @brief 在缓存中查找模块
 */
CnModuleMetadata *cn_module_cache_get(CnModuleCache *cache, const CnModuleId *id)
{
    if (!cache || !id) {
        return NULL;
    }
    
    uint32_t hash = cn_module_id_hash(id);
    size_t bucket_idx = hash % cache->bucket_count;
    
    CnModuleCacheEntry *entry = cache->buckets[bucket_idx];
    while (entry) {
        if (cn_module_id_equals(entry->id, id)) {
            return entry->metadata;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief 将模块添加到缓存
 */
int cn_module_cache_put(CnModuleCache *cache, CnModuleMetadata *metadata)
{
    if (!cache || !metadata || !metadata->id) {
        return 0;
    }
    
    // 检查是否已存在
    if (cn_module_cache_get(cache, metadata->id)) {
        return 1;  // 已存在
    }
    
    // 创建新条目
    CnModuleCacheEntry *entry = (CnModuleCacheEntry *)malloc(sizeof(CnModuleCacheEntry));
    if (!entry) {
        return 0;
    }
    
    entry->id = metadata->id;
    entry->metadata = metadata;
    
    // 插入到哈希桶
    uint32_t hash = cn_module_id_hash(metadata->id);
    size_t bucket_idx = hash % cache->bucket_count;
    
    entry->next = cache->buckets[bucket_idx];
    cache->buckets[bucket_idx] = entry;
    cache->entry_count++;
    
    return 1;
}

/**
 * @brief 遍历缓存中的所有模块
 */
size_t cn_module_cache_foreach(CnModuleCache *cache,
                                int (*callback)(CnModuleMetadata *metadata, void *user_data),
                                void *user_data)
{
    if (!cache || !callback) {
        return 0;
    }
    
    size_t count = 0;
    for (size_t i = 0; i < cache->bucket_count; i++) {
        CnModuleCacheEntry *entry = cache->buckets[i];
        while (entry) {
            if (callback(entry->metadata, user_data) != 0) {
                // 回调返回非0，停止遍历
                return count + 1;
            }
            count++;
            entry = entry->next;
        }
    }
    
    return count;
}

/**
 * @brief 获取缓存中的模块数量
 */
size_t cn_module_cache_count(CnModuleCache *cache)
{
    if (!cache) {
        return 0;
    }
    return cache->entry_count;
}

// ============================================================================
// C5: 循环导入检测
// ============================================================================

/**
 * @brief 检测循环导入（DFS 检测环）
 */
static int detect_cycle_dfs(CnDependencyGraph *graph, CnDependencyNode *node, 
                             CnModuleId **cycle_path, size_t *cycle_len)
{
    if (!node || node->visited) {
        return 0;
    }
    
    if (node->in_stack) {
        // 发现循环
        if (cycle_path && cycle_len) {
            // 记录循环路径（简化处理）
            *cycle_path = node->id;
            *cycle_len = 1;
        }
        return 1;
    }
    
    node->in_stack = 1;
    
    for (size_t i = 0; i < node->edge_count; i++) {
        if (detect_cycle_dfs(graph, node->edges[i], cycle_path, cycle_len)) {
            return 1;
        }
    }
    
    node->in_stack = 0;
    node->visited = 1;
    
    return 0;
}

// ============================================================================
// C6: 模块依赖图构建
// ============================================================================

/**
 * @brief 创建依赖图
 */
CnDependencyGraph *cn_dependency_graph_create(void)
{
    CnDependencyGraph *graph = (CnDependencyGraph *)malloc(sizeof(CnDependencyGraph));
    if (!graph) {
        return NULL;
    }
    
    graph->nodes = NULL;
    graph->node_count = 0;
    graph->node_capacity = 0;
    
    return graph;
}

/**
 * @brief 释放依赖图
 */
void cn_dependency_graph_free(CnDependencyGraph *graph)
{
    if (!graph) {
        return;
    }
    
    for (size_t i = 0; i < graph->node_count; i++) {
        CnDependencyNode *node = graph->nodes[i];
        if (node) {
            free(node->edges);
            // 注意：id 由外部管理
            free(node);
        }
    }
    
    free(graph->nodes);
    free(graph);
}

/**
 * @brief 在图中查找或创建节点
 */
static CnDependencyNode *get_or_create_node(CnDependencyGraph *graph, const CnModuleId *id)
{
    if (!graph || !id) {
        return NULL;
    }
    
    // 查找现有节点
    for (size_t i = 0; i < graph->node_count; i++) {
        if (cn_module_id_equals(graph->nodes[i]->id, id)) {
            return graph->nodes[i];
        }
    }
    
    // 创建新节点
    CnDependencyNode *node = (CnDependencyNode *)malloc(sizeof(CnDependencyNode));
    if (!node) {
        return NULL;
    }
    
    node->id = (CnModuleId *)id;  // 假设 id 由外部管理
    node->edges = NULL;
    node->edge_count = 0;
    node->visited = 0;
    node->in_stack = 0;
    
    // 添加到图
    if (graph->node_count >= graph->node_capacity) {
        size_t new_capacity = graph->node_capacity == 0 ? 16 : graph->node_capacity * 2;
        CnDependencyNode **new_nodes = (CnDependencyNode **)realloc(
            graph->nodes, sizeof(CnDependencyNode *) * new_capacity);
        if (!new_nodes) {
            free(node);
            return NULL;
        }
        graph->nodes = new_nodes;
        graph->node_capacity = new_capacity;
    }
    
    graph->nodes[graph->node_count++] = node;
    return node;
}

/**
 * @brief 添加依赖边
 */
int cn_dependency_graph_add_edge(CnDependencyGraph *graph,
                                  const CnModuleId *from,
                                  const CnModuleId *to)
{
    if (!graph || !from || !to) {
        return 0;
    }
    
    CnDependencyNode *from_node = get_or_create_node(graph, from);
    CnDependencyNode *to_node = get_or_create_node(graph, to);
    
    if (!from_node || !to_node) {
        return 0;
    }
    
    // 检查边是否已存在
    for (size_t i = 0; i < from_node->edge_count; i++) {
        if (from_node->edges[i] == to_node) {
            return 1;  // 边已存在
        }
    }
    
    // 添加边
    CnDependencyNode **new_edges = (CnDependencyNode **)realloc(
        from_node->edges, sizeof(CnDependencyNode *) * (from_node->edge_count + 1));
    if (!new_edges) {
        return 0;
    }
    
    from_node->edges = new_edges;
    from_node->edges[from_node->edge_count++] = to_node;
    
    return 1;
}

/**
 * @brief 拓扑排序 DFS 辅助函数
 */
static int topo_sort_dfs(CnDependencyNode *node, CnModuleId **result, size_t *idx, size_t max_count)
{
    if (!node || node->visited) {
        return 1;
    }
    
    if (node->in_stack) {
        // 存在循环
        return 0;
    }
    
    node->in_stack = 1;
    
    for (size_t i = 0; i < node->edge_count; i++) {
        if (!topo_sort_dfs(node->edges[i], result, idx, max_count)) {
            return 0;
        }
    }
    
    node->in_stack = 0;
    node->visited = 1;
    
    if (*idx < max_count) {
        result[*idx] = node->id;
        (*idx)++;
    }
    
    return 1;
}

/**
 * @brief 执行拓扑排序
 */
int cn_dependency_graph_topological_sort(CnDependencyGraph *graph,
                                          CnModuleId ***out_order,
                                          size_t *out_count)
{
    if (!graph || !out_order || !out_count) {
        return 0;
    }
    
    if (graph->node_count == 0) {
        *out_order = NULL;
        *out_count = 0;
        return 1;
    }
    
    // 重置所有节点的访问标记
    for (size_t i = 0; i < graph->node_count; i++) {
        graph->nodes[i]->visited = 0;
        graph->nodes[i]->in_stack = 0;
    }
    
    // 分配结果数组
    CnModuleId **result = (CnModuleId **)malloc(sizeof(CnModuleId *) * graph->node_count);
    if (!result) {
        return 0;
    }
    
    size_t idx = 0;
    
    // 对每个未访问的节点进行 DFS
    for (size_t i = 0; i < graph->node_count; i++) {
        if (!graph->nodes[i]->visited) {
            if (!topo_sort_dfs(graph->nodes[i], result, &idx, graph->node_count)) {
                free(result);
                return 0;  // 存在循环
            }
        }
    }
    
    *out_order = result;
    *out_count = idx;
    return 1;
}

// ============================================================================
// C7: 模块加载器
// ============================================================================

/**
 * @brief 创建模块元数据
 */
CnModuleMetadata *cn_module_metadata_create(void)
{
    CnModuleMetadata *metadata = (CnModuleMetadata *)malloc(sizeof(CnModuleMetadata));
    if (!metadata) {
        return NULL;
    }
    
    memset(metadata, 0, sizeof(CnModuleMetadata));
    metadata->state = CN_MODULE_STATE_UNLOADED;
    
    return metadata;
}

/**
 * @brief 释放模块元数据
 */
void cn_module_metadata_free(CnModuleMetadata *metadata)
{
    if (!metadata) {
        return;
    }
    
    cn_module_id_free(metadata->id);
    free(metadata->file_path);
    
    // 释放导出符号
    if (metadata->exports) {
        for (size_t i = 0; i < metadata->export_count; i++) {
            free(metadata->exports[i].name);
        }
        free(metadata->exports);
    }
    
    // 释放依赖列表
    if (metadata->dependencies) {
        for (size_t i = 0; i < metadata->dependency_count; i++) {
            cn_module_id_free(metadata->dependencies[i].module_id);
        }
        free(metadata->dependencies);
    }
    
    free(metadata);
}

/**
 * @brief 添加导出符号
 */
int cn_module_metadata_add_export(CnModuleMetadata *metadata,
                                   const char *name,
                                   int is_function,
                                   int is_constant)
{
    if (!metadata || !name) {
        return 0;
    }
    
    // 扩容
    CnExportedSymbol *new_exports = (CnExportedSymbol *)realloc(
        metadata->exports, sizeof(CnExportedSymbol) * (metadata->export_count + 1));
    if (!new_exports) {
        return 0;
    }
    metadata->exports = new_exports;
    
    // 添加符号
    size_t name_len = strlen(name);
    metadata->exports[metadata->export_count].name = (char *)malloc(name_len + 1);
    if (!metadata->exports[metadata->export_count].name) {
        return 0;
    }
    strcpy(metadata->exports[metadata->export_count].name, name);
    metadata->exports[metadata->export_count].name_length = name_len;
    metadata->exports[metadata->export_count].is_function = is_function;
    metadata->exports[metadata->export_count].is_constant = is_constant;
    metadata->exports[metadata->export_count].symbol_info = NULL;
    
    metadata->export_count++;
    return 1;
}

/**
 * @brief 添加模块依赖
 */
int cn_module_metadata_add_dependency(CnModuleMetadata *metadata, const CnModuleId *dep_id)
{
    if (!metadata || !dep_id) {
        return 0;
    }
    
    // 扩容
    CnModuleDependency *new_deps = (CnModuleDependency *)realloc(
        metadata->dependencies, sizeof(CnModuleDependency) * (metadata->dependency_count + 1));
    if (!new_deps) {
        return 0;
    }
    metadata->dependencies = new_deps;
    
    // 复制依赖 ID
    metadata->dependencies[metadata->dependency_count].module_id = cn_module_id_create(dep_id->qualified_name);
    metadata->dependencies[metadata->dependency_count].is_required = 1;
    metadata->dependencies[metadata->dependency_count].is_circular = 0;
    
    metadata->dependency_count++;
    return 1;
}

/**
 * @brief 创建模块加载器
 */
CnModuleLoader *cn_module_loader_create(void)
{
    CnModuleLoader *loader = (CnModuleLoader *)malloc(sizeof(CnModuleLoader));
    if (!loader) {
        return NULL;
    }
    
    loader->search_config = cn_search_config_create();
    loader->cache = cn_module_cache_create(64);
    loader->dep_graph = cn_dependency_graph_create();
    loader->diagnostics = NULL;
    loader->current_file = NULL;
    loader->current_file_length = 0;
    
    if (!loader->search_config || !loader->cache || !loader->dep_graph) {
        cn_module_loader_free(loader);
        return NULL;
    }
    
    return loader;
}

/**
 * @brief 释放模块加载器
 */
void cn_module_loader_free(CnModuleLoader *loader)
{
    if (!loader) {
        return;
    }
    
    cn_search_config_free(loader->search_config);
    cn_module_cache_free(loader->cache);
    cn_dependency_graph_free(loader->dep_graph);
    free(loader->current_file);
    free(loader);
}

/**
 * @brief 设置诊断信息对象
 */
void cn_module_loader_set_diagnostics(CnModuleLoader *loader, struct CnDiagnostics *diagnostics)
{
    if (loader) {
        loader->diagnostics = diagnostics;
    }
}

/**
 * @brief 解析模块路径为文件路径
 */
int cn_module_loader_resolve_path(CnModuleLoader *loader, 
                                   const CnModuleId *module_id,
                                   char **out_path)
{
    if (!loader || !module_id || !out_path) {
        return 0;
    }
    
    // 先尝试查找模块文件
    char *file_path = find_module_file(loader->search_config, module_id);
    if (file_path) {
        *out_path = file_path;
        return 1;
    }
    
    // 再尝试查找包目录
    char *pkg_dir = find_package_dir(loader->search_config, module_id);
    if (pkg_dir) {
        // 返回包初始化文件路径
        char *init_path = path_join(pkg_dir, CN_PACKAGE_INIT_FILENAME);
        free(pkg_dir);
        if (init_path && file_exists(init_path)) {
            *out_path = init_path;
            return 1;
        }
        free(init_path);
    }
    
    return 0;
}

/**
 * @brief 解析模块路径为文件路径（带目标类型）
 * @param target_type 0 = 模块（.cn文件），1 = 包（目录中的__包__.cn）
 */
int cn_module_loader_resolve_path_typed(CnModuleLoader *loader, 
                                         const CnModuleId *module_id,
                                         char **out_path,
                                         int target_type)
{
    if (!loader || !module_id || !out_path) {
        return 0;
    }
    
    if (target_type == 0) {
        // 模块导入：只查找 .cn 文件
        char *file_path = find_module_file(loader->search_config, module_id);
        if (file_path) {
            *out_path = file_path;
            return 1;
        }
        return 0;
    } else {
        // 包导入：只查找目录中的 __包__.cn
        char *pkg_dir = find_package_dir(loader->search_config, module_id);
        if (pkg_dir) {
            char *init_path = path_join(pkg_dir, CN_PACKAGE_INIT_FILENAME);
            free(pkg_dir);
            if (init_path && file_exists(init_path)) {
                *out_path = init_path;
                return 1;
            }
            free(init_path);
        }
        return 0;
    }
}

/**
 * @brief 加载模块
 */
CnModuleMetadata *cn_module_loader_load(CnModuleLoader *loader, const CnModuleId *module_id)
{
    if (!loader || !module_id) {
        return NULL;
    }
    
    // 检查缓存
    CnModuleMetadata *cached = cn_module_cache_get(loader->cache, module_id);
    if (cached) {
        // 检查是否正在加载（循环导入）
        if (cached->state == CN_MODULE_STATE_LOADING) {
            // 循环导入检测
            return NULL;
        }
        return cached;
    }
    
    // 解析文件路径
    char *file_path = NULL;
    if (!cn_module_loader_resolve_path(loader, module_id, &file_path)) {
        // 模块未找到
        return NULL;
    }
    
    // 创建模块元数据
    CnModuleMetadata *metadata = cn_module_metadata_create();
    if (!metadata) {
        free(file_path);
        return NULL;
    }
    
    // 设置基本信息
    metadata->id = cn_module_id_create(module_id->qualified_name);
    metadata->file_path = file_path;
    metadata->file_path_length = strlen(file_path);
    metadata->file_mtime = get_file_mtime(file_path);
    metadata->state = CN_MODULE_STATE_LOADING;
    
    // 确定模块类型
    if (cn_is_package_init_path(file_path)) {
        metadata->type = CN_MODULE_TYPE_PACKAGE;
    } else {
        metadata->type = CN_MODULE_TYPE_FILE;
    }
    
    // 添加到缓存
    cn_module_cache_put(loader->cache, metadata);
    
    // TODO: 实际解析文件内容，提取导出符号和依赖
    // 这部分需要调用解析器，属于阶段 D 的内容
    
    metadata->state = CN_MODULE_STATE_LOADED;
    return metadata;
}

/**
 * @brief 检测循环导入
 */
int cn_module_loader_check_circular(CnModuleLoader *loader, const CnModuleId *module_id)
{
    if (!loader || !module_id || !loader->cache) {
        return 0;
    }
    
    CnModuleMetadata *metadata = cn_module_cache_get(loader->cache, module_id);
    if (metadata && metadata->state == CN_MODULE_STATE_LOADING) {
        return 1;  // 循环导入
    }
    
    return 0;
}

/**
 * @brief 获取模块初始化顺序
 */
int cn_module_loader_get_init_order(CnModuleLoader *loader,
                                     CnModuleId ***out_order,
                                     size_t *out_count)
{
    if (!loader || !loader->dep_graph) {
        return 0;
    }
    
    return cn_dependency_graph_topological_sort(loader->dep_graph, out_order, out_count);
}

// ============================================================================
// C8: 延迟加载机制
// ============================================================================

/**
 * @brief 获取文件所在目录
 */
static char *get_directory_from_path(const char *file_path)
{
    if (!file_path) {
        return NULL;
    }
    
    // 查找最后一个路径分隔符
    const char *last_sep = strrchr(file_path, PATH_SEPARATOR);
    
#ifdef _WIN32
    // Windows 也支持正斜杠
    const char *last_forward_sep = strrchr(file_path, '/');
    if (last_forward_sep > last_sep) {
        last_sep = last_forward_sep;
    }
#endif
    
    if (!last_sep) {
        // 没有路径分隔符，返回当前目录
        return strdup(".");
    }
    
    // 复制目录部分
    size_t dir_len = last_sep - file_path;
    char *dir = (char *)malloc(dir_len + 1);
    if (!dir) {
        return NULL;
    }
    memcpy(dir, file_path, dir_len);
    dir[dir_len] = '\0';
    
    return dir;
}

/**
 * @brief 解析相对路径
 * @param base_dir 基准目录
 * @param relative_level 相对层级（1 = ./，2 = ../）
 * @return 解析后的目录（调用者负责释放）
 */
static char *resolve_relative_dir(const char *base_dir, size_t relative_level)
{
    if (!base_dir || relative_level == 0) {
        return NULL;
    }
    
    // 复制基准目录
    char *result = strdup(base_dir);
    if (!result) {
        return NULL;
    }
    
    // relative_level = 1 表示 ./（同级）
    // relative_level = 2 表示 ../（父级）
    // 所以需要向上 relative_level - 1 层
    size_t up_levels = relative_level - 1;
    
    for (size_t i = 0; i < up_levels; i++) {
        // 查找最后一个路径分隔符
        char *last_sep = strrchr(result, PATH_SEPARATOR);
        
#ifdef _WIN32
        char *last_forward_sep = strrchr(result, '/');
        if (last_forward_sep > last_sep) {
            last_sep = last_forward_sep;
        }
#endif
        
        if (last_sep) {
            *last_sep = '\0';  // 截断到父目录
        } else {
            // 已经到达根目录，无法再向上
            free(result);
            return strdup(".");
        }
    }
    
    return result;
}

/**
 * @brief 加载相对模块
 */
CnModuleMetadata *cn_module_loader_load_relative(CnModuleLoader *loader, 
                                                  const char *base_path,
                                                  const struct CnAstModulePath *relative_path)
{
    if (!loader || !base_path || !relative_path) {
        return NULL;
    }
    
    // 获取基准文件所在目录
    char *base_dir = get_directory_from_path(base_path);
    if (!base_dir) {
        return NULL;
    }
    
    // 解析相对目录
    char *target_dir = base_dir;
    if (relative_path->is_relative && relative_path->relative_level > 0) {
        target_dir = resolve_relative_dir(base_dir, relative_path->relative_level);
        free(base_dir);
        if (!target_dir) {
            return NULL;
        }
    }
    
    // 构建目标模块路径
    char *module_path = target_dir;
    for (size_t i = 0; i < relative_path->segment_count; i++) {
        const char *segment = relative_path->segments[i].name;
        size_t segment_len = relative_path->segments[i].name_length;
        
        // 复制段名（因为 segment 不以 null 结尾）
        char *segment_str = (char *)malloc(segment_len + 1);
        if (!segment_str) {
            free(module_path);
            return NULL;
        }
        memcpy(segment_str, segment, segment_len);
        segment_str[segment_len] = '\0';
        
        char *new_path = path_join(module_path, segment_str);
        free(segment_str);
        free(module_path);
        
        if (!new_path) {
            return NULL;
        }
        module_path = new_path;
    }
    
    // 基于 module_path 识别包或模块
    CnModuleType module_type = CN_MODULE_TYPE_FILE;
    char *file_path = NULL;

    // 1. 优先尝试包初始化文件: module_path/__包__.cn
    char *package_init_path = path_join(module_path, CN_PACKAGE_INIT_FILENAME);
    if (package_init_path && file_exists(package_init_path)) {
        file_path = package_init_path;
        module_type = CN_MODULE_TYPE_PACKAGE;
    } else {
        if (package_init_path) {
            free(package_init_path);
        }

        // 2. 再尝试模块文件: module_path.cn
        size_t path_len = strlen(module_path);
        file_path = (char *)malloc(path_len + 4);  // .cn + null
        if (!file_path) {
            free(module_path);
            return NULL;
        }
        sprintf(file_path, "%s.cn", module_path);

        if (!file_exists(file_path)) {
            free(file_path);
            free(module_path);
            return NULL;
        }

        module_type = CN_MODULE_TYPE_FILE;
    }

    free(module_path);
    
    // 从文件路径创建模块 ID
    // 使用最后一段作为模块名
    if (relative_path->segment_count > 0) {
        size_t last_idx = relative_path->segment_count - 1;
        const char *module_name = relative_path->segments[last_idx].name;
        size_t module_name_len = relative_path->segments[last_idx].name_length;
        
        char *module_name_str = (char *)malloc(module_name_len + 1);
        if (!module_name_str) {
            free(file_path);
            return NULL;
        }
        memcpy(module_name_str, module_name, module_name_len);
        module_name_str[module_name_len] = '\0';
        
        CnModuleId *module_id = cn_module_id_create(module_name_str);
        free(module_name_str);
        
        if (!module_id) {
            free(file_path);
            return NULL;
        }
        
        // 检查缓存
        CnModuleMetadata *cached = cn_module_cache_get(loader->cache, module_id);
        if (cached) {
            cn_module_id_free(module_id);
            free(file_path);
            return cached;
        }
        
        // 创建模块元数据
        CnModuleMetadata *metadata = cn_module_metadata_create();
        if (!metadata) {
            cn_module_id_free(module_id);
            free(file_path);
            return NULL;
        }
        
        metadata->id = module_id;
        metadata->file_path = file_path;
        metadata->file_path_length = strlen(file_path);
        metadata->file_mtime = get_file_mtime(file_path);
        metadata->state = CN_MODULE_STATE_LOADING;
        metadata->type = module_type;
        
        // 添加到缓存
        cn_module_cache_put(loader->cache, metadata);
        
        // TODO: 实际解析文件内容
        
        metadata->state = CN_MODULE_STATE_LOADED;
        return metadata;
    }
    
    free(file_path);
    return NULL;
}

/**
 * @brief 加载相对模块（带目标类型）
 * @param target_type 0 = 模块（.cn文件），1 = 包（目录中的__包__.cn）
 */
CnModuleMetadata *cn_module_loader_load_relative_typed(CnModuleLoader *loader, 
                                                        const char *base_path,
                                                        const struct CnAstModulePath *relative_path,
                                                        int target_type)
{
    if (!loader || !base_path || !relative_path) {
        return NULL;
    }
    
    // 获取基准文件所在目录
    char *base_dir = get_directory_from_path(base_path);
    if (!base_dir) {
        return NULL;
    }
    
    // 解析相对目录
    char *target_dir = base_dir;
    if (relative_path->is_relative && relative_path->relative_level > 0) {
        target_dir = resolve_relative_dir(base_dir, relative_path->relative_level);
        free(base_dir);
        if (!target_dir) {
            return NULL;
        }
    }
    
    // 构建目标模块路径
    char *module_path = target_dir;
    for (size_t i = 0; i < relative_path->segment_count; i++) {
        const char *segment = relative_path->segments[i].name;
        size_t segment_len = relative_path->segments[i].name_length;
        
        char *segment_str = (char *)malloc(segment_len + 1);
        if (!segment_str) {
            free(module_path);
            return NULL;
        }
        memcpy(segment_str, segment, segment_len);
        segment_str[segment_len] = '\0';
        
        char *new_path = path_join(module_path, segment_str);
        free(segment_str);
        free(module_path);
        
        if (!new_path) {
            return NULL;
        }
        module_path = new_path;
    }
    
    // 根据 target_type 决定查找逻辑
    // 优先按指定类型查找，如果失败则尝试另一种类型（模块/包自动回退）
    CnModuleType module_type;
    char *file_path = NULL;
    
    if (target_type == 1) {
        // 包导入（相对路径 ./xxx）：优先查找目录中的 __包__.cn，找不到再查找 .cn 文件
        char *package_init_path = path_join(module_path, CN_PACKAGE_INIT_FILENAME);
        if (package_init_path && file_exists(package_init_path)) {
            file_path = package_init_path;
            module_type = CN_MODULE_TYPE_PACKAGE;
        } else {
            if (package_init_path) {
                free(package_init_path);
            }
            
            // 包不存在，尝试回退到模块文件查找
            size_t path_len = strlen(module_path);
            file_path = (char *)malloc(path_len + 4);  // .cn + null
            if (file_path) {
                sprintf(file_path, "%s.cn", module_path);
                
                if (file_exists(file_path)) {
                    module_type = CN_MODULE_TYPE_FILE;
                } else {
                    free(file_path);
                    file_path = NULL;
                    free(module_path);
                    return NULL;  // 包和模块都不存在
                }
            } else {
                free(module_path);
                return NULL;
            }
        }
    } else {
        // 模块导入：先查找 .cn 文件
        size_t path_len = strlen(module_path);
        file_path = (char *)malloc(path_len + 4);  // .cn + null
        if (!file_path) {
            free(module_path);
            return NULL;
        }
        sprintf(file_path, "%s.cn", module_path);
        
        if (file_exists(file_path)) {
            module_type = CN_MODULE_TYPE_FILE;
        } else {
            // 模块文件不存在，尝试回退到包目录查找
            // 这支持「从 ./工具 导入 { 成员 };」语法，当 ./工具.cn 不存在但 ./工具/__包__.cn 存在时
            free(file_path);
            file_path = NULL;
            
            char *package_init_path = path_join(module_path, CN_PACKAGE_INIT_FILENAME);
            if (package_init_path && file_exists(package_init_path)) {
                file_path = package_init_path;
                module_type = CN_MODULE_TYPE_PACKAGE;
            } else {
                if (package_init_path) {
                    free(package_init_path);
                }
                free(module_path);
                return NULL;  // 模块和包都不存在
            }
        }
    }
    
    free(module_path);
    
    // 创建模块元数据
    if (relative_path->segment_count > 0) {
        size_t last_idx = relative_path->segment_count - 1;
        const char *module_name = relative_path->segments[last_idx].name;
        size_t module_name_len = relative_path->segments[last_idx].name_length;
        
        char *module_name_str = (char *)malloc(module_name_len + 1);
        if (!module_name_str) {
            free(file_path);
            return NULL;
        }
        memcpy(module_name_str, module_name, module_name_len);
        module_name_str[module_name_len] = '\0';
        
        CnModuleId *module_id = cn_module_id_create(module_name_str);
        free(module_name_str);
        
        if (!module_id) {
            free(file_path);
            return NULL;
        }
        
        // 检查缓存
        CnModuleMetadata *cached = cn_module_cache_get(loader->cache, module_id);
        if (cached) {
            cn_module_id_free(module_id);
            free(file_path);
            return cached;
        }
        
        CnModuleMetadata *metadata = cn_module_metadata_create();
        if (!metadata) {
            cn_module_id_free(module_id);
            free(file_path);
            return NULL;
        }
        
        metadata->id = module_id;
        metadata->file_path = file_path;
        metadata->file_path_length = strlen(file_path);
        metadata->file_mtime = get_file_mtime(file_path);
        metadata->state = CN_MODULE_STATE_LOADING;
        metadata->type = module_type;
        
        cn_module_cache_put(loader->cache, metadata);
        metadata->state = CN_MODULE_STATE_LOADED;
        return metadata;
    }
    
    free(file_path);
    return NULL;
}

/**
 * @brief 从路径段创建模块标识符
 */
CnModuleId *cn_module_id_create_from_segments(const char **segments, size_t segment_count)
{
    if (!segments || segment_count == 0) {
        return NULL;
    }
    
    // 计算完全限定名长度
    size_t total_len = 0;
    for (size_t i = 0; i < segment_count; i++) {
        total_len += strlen(segments[i]);
        if (i < segment_count - 1) {
            total_len += 1;  // 点号
        }
    }
    
    // 构建完全限定名
    char *qualified_name = (char *)malloc(total_len + 1);
    if (!qualified_name) {
        return NULL;
    }
    
    char *ptr = qualified_name;
    for (size_t i = 0; i < segment_count; i++) {
        size_t seg_len = strlen(segments[i]);
        memcpy(ptr, segments[i], seg_len);
        ptr += seg_len;
        if (i < segment_count - 1) {
            *ptr++ = '.';
        }
    }
    *ptr = '\0';
    
    CnModuleId *id = cn_module_id_create(qualified_name);
    free(qualified_name);
    
    return id;
}
