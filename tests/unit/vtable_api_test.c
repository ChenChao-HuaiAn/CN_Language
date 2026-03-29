/**
 * @file vtable_api_test.c
 * @brief vtable API单元测试 - 简化版
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/semantics/vtable_builder.h"

int main(void) {
    printf("\n========================================\n");
    printf("vtable API Unit Test\n");
    printf("========================================\n\n");
    
    int passed = 0;
    int failed = 0;
    
    /* 测试1: 创建vtable */
    printf("Test 1: Create vtable\n");
    CnVTable *base_vtable = cn_vtable_create("Animal", strlen("Animal"));
    if (base_vtable != NULL) {
        printf("  [PASS] Created base vtable\n");
        passed++;
    } else {
        printf("  [FAIL] Failed to create base vtable\n");
        failed++;
        goto end_test;
    }
    
    /* 测试2: 添加条目 */
    printf("Test 2: Add entry to vtable\n");
    CnClassMember base_method = {0};
    base_method.name = "speak";
    base_method.name_length = strlen("speak");
    base_method.kind = CN_MEMBER_METHOD;
    base_method.is_virtual = true;
    base_method.is_pure_virtual = false;
    
    bool add_result = cn_vtable_add_entry_ex(base_vtable, &base_method, "Animal", strlen("Animal"));
    if (add_result) {
        printf("  [PASS] Added entry to vtable\n");
        passed++;
    } else {
        printf("  [FAIL] Failed to add entry\n");
        failed++;
    }
    
    /* 测试3: 查找条目 */
    printf("Test 3: Find entry in vtable\n");
    int idx = cn_vtable_find_entry(base_vtable, "speak", strlen("speak"));
    if (idx >= 0) {
        printf("  [PASS] Found entry at index %d\n", idx);
        passed++;
    } else {
        printf("  [FAIL] Entry not found\n");
        failed++;
    }
    
    /* 测试4: 创建派生类vtable */
    printf("Test 4: Create derived vtable\n");
    CnVTable *derived_vtable = cn_vtable_create("Dog", strlen("Dog"));
    if (derived_vtable != NULL) {
        printf("  [PASS] Created derived vtable\n");
        passed++;
    } else {
        printf("  [FAIL] Failed to create derived vtable\n");
        failed++;
        cn_vtable_destroy(base_vtable);
        goto end_test;
    }
    
    /* 测试5: 添加重写方法 */
    printf("Test 5: Add override method\n");
    CnClassMember derived_method = {0};
    derived_method.name = "speak";
    derived_method.name_length = strlen("speak");
    derived_method.kind = CN_MEMBER_METHOD;
    derived_method.is_virtual = true;
    derived_method.is_override = true;
    
    add_result = cn_vtable_add_entry_ex(derived_vtable, &derived_method, "Dog", strlen("Dog"));
    if (add_result) {
        printf("  [PASS] Added override method\n");
        passed++;
    } else {
        printf("  [FAIL] Failed to add override method\n");
        failed++;
    }
    
    /* 测试6: 合并vtable */
    printf("Test 6: Merge vtables\n");
    bool merge_result = cn_vtable_merge_base(derived_vtable, base_vtable);
    if (merge_result) {
        printf("  [PASS] Merged vtables\n");
        passed++;
    } else {
        printf("  [FAIL] Failed to merge vtables\n");
        failed++;
    }
    
    /* 测试7: 验证合并结果 */
    printf("Test 7: Verify merge result\n");
    if (derived_vtable->entry_count == 1) {
        printf("  [PASS] Entry count is correct (1)\n");
        passed++;
    } else {
        printf("  [FAIL] Wrong entry count: %zu\n", derived_vtable->entry_count);
        failed++;
    }
    
    /* 测试8: 验证重写标记 */
    printf("Test 8: Verify override flag\n");
    idx = cn_vtable_find_entry(derived_vtable, "speak", strlen("speak"));
    if (idx >= 0 && derived_vtable->entries[idx].is_override) {
        printf("  [PASS] Override flag is set correctly\n");
        passed++;
    } else {
        printf("  [FAIL] Override flag not set\n");
        failed++;
    }
    
    /* 清理 */
    cn_vtable_destroy(derived_vtable);
    cn_vtable_destroy(base_vtable);
    printf("\nCleanup completed\n");
    
end_test:
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("========================================\n");
    
    return (failed > 0) ? 1 : 0;
}
