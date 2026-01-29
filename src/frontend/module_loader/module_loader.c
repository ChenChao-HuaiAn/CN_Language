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
