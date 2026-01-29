/**
 * @file module_loader_test.c
 * @brief 模块加载器单元测试 (G1, G2)
 * 
 * 测试模块标识符系统、搜索路径配置、缓存和依赖图。
 */
#include "cnlang/frontend/module_loader.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  测试: %s ... ", #name); \
    test_##name(); \
} while(0)
#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("失败 (行 %d)\n", __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)
#define PASS() do { printf("通过\n"); tests_passed++; } while(0)

// ============================================================================
// G1: 文件模块单元测试 - 模块标识符系统
// ============================================================================

TEST(module_id_create) {
    // 测试创建简单模块标识符
    CnModuleId *id = cn_module_id_create("工具");
    ASSERT(id != NULL);
    ASSERT(id->qualified_name != NULL);
    ASSERT(strcmp(id->qualified_name, "工具") == 0);
    ASSERT(id->segment_count == 1);
    cn_module_id_free(id);
    
    // 测试创建点分模块标识符
    id = cn_module_id_create("工具.数学.高级");
    ASSERT(id != NULL);
    ASSERT(strcmp(id->qualified_name, "工具.数学.高级") == 0);
    ASSERT(id->segment_count == 3);
    cn_module_id_free(id);
    
    PASS();
}

TEST(module_id_from_segments) {
    const char *segments[] = {"工具", "数学"};
    CnModuleId *id = cn_module_id_create_from_segments(segments, 2);
    ASSERT(id != NULL);
    ASSERT(id->segment_count == 2);
    ASSERT(strstr(id->qualified_name, "工具") != NULL);
    cn_module_id_free(id);
    
    PASS();
}

TEST(module_id_equals) {
    CnModuleId *id1 = cn_module_id_create("工具.数学");
    CnModuleId *id2 = cn_module_id_create("工具.数学");
    CnModuleId *id3 = cn_module_id_create("工具.其他");
    
    ASSERT(cn_module_id_equals(id1, id2) == 1);
    ASSERT(cn_module_id_equals(id1, id3) == 0);
    ASSERT(cn_module_id_equals(NULL, NULL) == 1);
    ASSERT(cn_module_id_equals(id1, NULL) == 0);
    
    cn_module_id_free(id1);
    cn_module_id_free(id2);
    cn_module_id_free(id3);
    
    PASS();
}

TEST(module_id_hash) {
    CnModuleId *id1 = cn_module_id_create("工具.数学");
    CnModuleId *id2 = cn_module_id_create("工具.数学");
    CnModuleId *id3 = cn_module_id_create("其他.模块");
    
    // 相同模块应该有相同哈希
    ASSERT(cn_module_id_hash(id1) == cn_module_id_hash(id2));
    // 不同模块哈希应该不同（大概率）
    // 不做严格断言，因为可能有碰撞
    
    cn_module_id_free(id1);
    cn_module_id_free(id2);
    cn_module_id_free(id3);
    
    PASS();
}

// ============================================================================
// G2: 包系统单元测试 - 搜索路径和缓存
// ============================================================================

TEST(search_config_create) {
    CnModuleSearchConfig *config = cn_search_config_create();
    ASSERT(config != NULL);
    ASSERT(config->path_count == 0);
    cn_search_config_free(config);
    
    PASS();
}

TEST(search_config_add_path) {
    CnModuleSearchConfig *config = cn_search_config_create();
    ASSERT(config != NULL);
    
    int result = cn_search_config_add_path(config, "./modules", 1);
    ASSERT(result == 1);
    ASSERT(config->path_count == 1);
    
    result = cn_search_config_add_path(config, "./lib", 2);
    ASSERT(result == 1);
    ASSERT(config->path_count == 2);
    
    cn_search_config_free(config);
    
    PASS();
}

TEST(search_config_project_root) {
    CnModuleSearchConfig *config = cn_search_config_create();
    ASSERT(config != NULL);
    
    cn_search_config_set_project_root(config, "/home/user/project");
    ASSERT(config->project_root != NULL);
    ASSERT(strcmp(config->project_root, "/home/user/project") == 0);
    
    cn_search_config_free(config);
    
    PASS();
}

TEST(module_cache_create) {
    CnModuleCache *cache = cn_module_cache_create(16);
    ASSERT(cache != NULL);
    cn_module_cache_free(cache);
    
    PASS();
}

TEST(module_cache_put_get) {
    CnModuleCache *cache = cn_module_cache_create(16);
    ASSERT(cache != NULL);
    
    CnModuleMetadata *meta = cn_module_metadata_create();
    ASSERT(meta != NULL);
    
    meta->id = cn_module_id_create("测试模块");
    meta->type = CN_MODULE_TYPE_FILE;
    meta->state = CN_MODULE_STATE_LOADED;
    
    int result = cn_module_cache_put(cache, meta);
    ASSERT(result == 1);
    
    CnModuleMetadata *found = cn_module_cache_get(cache, meta->id);
    ASSERT(found != NULL);
    ASSERT(found == meta);
    
    // 查找不存在的模块
    CnModuleId *nonexistent = cn_module_id_create("不存在的模块");
    CnModuleMetadata *not_found = cn_module_cache_get(cache, nonexistent);
    ASSERT(not_found == NULL);
    cn_module_id_free(nonexistent);
    
    cn_module_cache_free(cache);
    
    PASS();
}

TEST(dependency_graph_create) {
    CnDependencyGraph *graph = cn_dependency_graph_create();
    ASSERT(graph != NULL);
    cn_dependency_graph_free(graph);
    
    PASS();
}

TEST(dependency_graph_add_edge) {
    CnDependencyGraph *graph = cn_dependency_graph_create();
    ASSERT(graph != NULL);
    
    CnModuleId *a = cn_module_id_create("模块A");
    CnModuleId *b = cn_module_id_create("模块B");
    
    int result = cn_dependency_graph_add_edge(graph, a, b);
    ASSERT(result == 1);
    
    cn_module_id_free(a);
    cn_module_id_free(b);
    cn_dependency_graph_free(graph);
    
    PASS();
}

TEST(dependency_graph_topological_sort) {
    CnDependencyGraph *graph = cn_dependency_graph_create();
    ASSERT(graph != NULL);
    
    CnModuleId *a = cn_module_id_create("A");
    CnModuleId *b = cn_module_id_create("B");
    CnModuleId *c = cn_module_id_create("C");
    
    // A -> B -> C (A 依赖 B，B 依赖 C)
    cn_dependency_graph_add_edge(graph, a, b);
    cn_dependency_graph_add_edge(graph, b, c);
    
    CnModuleId **order = NULL;
    size_t order_count = 0;
    
    int result = cn_dependency_graph_topological_sort(graph, &order, &order_count);
    ASSERT(result == 1);  // 无循环，应该成功
    ASSERT(order_count == 3);
    
    // 顺序应该是 C, B, A（依赖的先初始化）
    if (order) {
        free(order);
    }
    
    cn_module_id_free(a);
    cn_module_id_free(b);
    cn_module_id_free(c);
    cn_dependency_graph_free(graph);
    
    PASS();
}

TEST(module_loader_create) {
    CnModuleLoader *loader = cn_module_loader_create();
    ASSERT(loader != NULL);
    ASSERT(loader->search_config != NULL);
    ASSERT(loader->cache != NULL);
    ASSERT(loader->dep_graph != NULL);
    cn_module_loader_free(loader);
    
    PASS();
}

TEST(package_init_file_check) {
    // 测试包初始化文件名检测
    ASSERT(cn_is_package_init_file("__包__.cn") == 1);
    ASSERT(cn_is_package_init_file("普通文件.cn") == 0);
    ASSERT(cn_is_package_init_file("__init__.cn") == 0);
    
    PASS();
}

// ============================================================================
// 主函数
// ============================================================================

int main(void) {
    printf("=== 模块加载器单元测试 (G1, G2) ===\n\n");
    
    printf("G1: 模块标识符系统测试\n");
    RUN_TEST(module_id_create);
    RUN_TEST(module_id_from_segments);
    RUN_TEST(module_id_equals);
    RUN_TEST(module_id_hash);
    
    printf("\nG2: 搜索路径和缓存测试\n");
    RUN_TEST(search_config_create);
    RUN_TEST(search_config_add_path);
    RUN_TEST(search_config_project_root);
    RUN_TEST(module_cache_create);
    RUN_TEST(module_cache_put_get);
    RUN_TEST(dependency_graph_create);
    RUN_TEST(dependency_graph_add_edge);
    RUN_TEST(dependency_graph_topological_sort);
    RUN_TEST(module_loader_create);
    RUN_TEST(package_init_file_check);
    
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
