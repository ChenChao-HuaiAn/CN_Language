#ifndef TEST_MEMORY_H
#define TEST_MEMORY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 测试函数声明

// 分配器测试
bool test_system_allocator_basic(void);
bool test_system_allocator_alignment(void);
bool test_system_allocator_reallocate(void);
bool test_debug_allocator_basic(void);
bool test_debug_allocator_validation(void);
bool test_debug_allocator_leak_detection(void);

// 工具函数测试
bool test_memory_operations_copy(void);
bool test_memory_operations_move(void);
bool test_memory_operations_compare(void);
bool test_memory_operations_set(void);
bool test_memory_safety_secure_zero(void);
bool test_memory_safety_validate_range(void);
bool test_memory_safety_initialize_pattern(void);

// 上下文测试
bool test_memory_context_create_destroy(void);
bool test_memory_context_allocation(void);
bool test_memory_context_hierarchy(void);
bool test_memory_context_statistics(void);
bool test_memory_context_reset(void);

// 调试器测试
bool test_memory_debugger_initialization(void);
bool test_memory_debugger_monitoring(void);
bool test_memory_debugger_leak_detection(void);
bool test_memory_debugger_validation(void);
bool test_memory_debugger_statistics(void);

// 便捷函数测试（向后兼容）
bool test_cn_malloc_free(void);
bool test_cn_malloc_aligned(void);
bool test_cn_realloc(void);
bool test_cn_memcpy(void);
bool test_cn_memset(void);
bool test_cn_memcmp(void);

// 测试运行器
bool test_memory_all(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_MEMORY_H
