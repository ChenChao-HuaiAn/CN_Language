/**
 * @file runtime_chinese_names_test.c
 * @brief CN语言标准库中文函数名测试
 * 
 * 测试策略：
 * 1. 验证中文函数名与英文函数名指向相同的函数指针
 * 2. 验证中文函数名的功能正确性
 * 3. 使用条件编译，确保在支持中文标识符的编译器上运行
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

// 包含所有运行时头文件
#include "cnlang/runtime/runtime.h"
#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/sync.h"
#include "cnlang/runtime/math.h"
#include "cnlang/runtime/io.h"

// 测试结果统计
static int total_tests = 0;
static int passed_tests = 0;

// 测试辅助宏
#define TEST_ASSERT(cond, msg) do { \
    total_tests++; \
    if (cond) { \
        passed_tests++; \
    } else { \
        fprintf(stderr, "  [失败] %s\n", msg); \
    } \
} while(0)

// =============================================================================
// 测试1：运行时生命周期函数的中文别名
// =============================================================================
static int test_runtime_lifecycle_chinese_names(void) {
    printf("测试1：运行时生命周期函数的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证中文别名与英文函数名指向相同的地址
    TEST_ASSERT((void*)初始化运行时 == (void*)cn_rt_init, 
                "初始化运行时 应指向 cn_rt_init");
    TEST_ASSERT((void*)退出运行时 == (void*)cn_rt_exit, 
                "退出运行时 应指向 cn_rt_exit");
    
    // 功能测试：使用中文别名初始化和退出运行时
    初始化运行时();
    退出运行时();
    TEST_ASSERT(1, "运行时生命周期中文函数调用成功");
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试2：内存管理函数的中文别名
// =============================================================================
static int test_memory_chinese_names(void) {
    printf("测试2：内存管理函数的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证中文别名与英文函数名指向相同的地址
    TEST_ASSERT((void*)分配内存 == (void*)cn_rt_malloc, 
                "分配内存 应指向 cn_rt_malloc");
    TEST_ASSERT((void*)分配清零内存 == (void*)cn_rt_calloc, 
                "分配清零内存 应指向 cn_rt_calloc");
    TEST_ASSERT((void*)重新分配内存 == (void*)cn_rt_realloc, 
                "重新分配内存 应指向 cn_rt_realloc");
    TEST_ASSERT((void*)释放内存 == (void*)cn_rt_free, 
                "释放内存 应指向 cn_rt_free");
    
    // 功能测试：使用中文别名进行内存操作
    void* ptr = 分配内存(100);
    TEST_ASSERT(ptr != NULL, "分配内存(100) 应返回非空指针");
    
    if (ptr) {
        memset(ptr, 0xAB, 100);
        释放内存(ptr);
        TEST_ASSERT(1, "释放内存 调用成功");
    }
    
    // 测试分配清零内存
    int* arr = (int*)分配清零内存(10, sizeof(int));
    TEST_ASSERT(arr != NULL, "分配清零内存 应返回非空指针");
    
    if (arr) {
        int all_zero = 1;
        for (int i = 0; i < 10; i++) {
            if (arr[i] != 0) {
                all_zero = 0;
                break;
            }
        }
        TEST_ASSERT(all_zero, "分配清零内存 应返回零初始化的内存");
        释放内存(arr);
    }
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试3：字符串函数的中文别名
// =============================================================================
static int test_string_chinese_names(void) {
    printf("测试3：字符串函数的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证中文别名与英文函数名指向相同的地址
    TEST_ASSERT((void*)字符串长度 == (void*)cn_rt_string_length, 
                "字符串长度 应指向 cn_rt_string_length");
    TEST_ASSERT((void*)整数转字符串 == (void*)cn_rt_int_to_string, 
                "整数转字符串 应指向 cn_rt_int_to_string");
    TEST_ASSERT((void*)布尔转字符串 == (void*)cn_rt_bool_to_string, 
                "布尔转字符串 应指向 cn_rt_bool_to_string");
    
    // 功能测试：使用中文别名进行字符串操作
    const char* test_str = "你好世界";
    size_t len = 字符串长度(test_str);
    TEST_ASSERT(len > 0, "字符串长度 应返回正数");
    
    char* int_str = 整数转字符串(42);
    TEST_ASSERT(int_str != NULL, "整数转字符串(42) 应返回非空指针");
    if (int_str) {
        TEST_ASSERT(strcmp(int_str, "42") == 0, "整数转字符串(42) 应返回 \"42\"");
        释放内存(int_str);
    }
    
    char* bool_str = 布尔转字符串(1);
    TEST_ASSERT(bool_str != NULL, "布尔转字符串(1) 应返回非空指针");
    if (bool_str) {
        释放内存(bool_str);
    }
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试4：输出函数的中文别名
// =============================================================================
static int test_io_chinese_names(void) {
    printf("测试4：输出函数的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
#ifndef CN_RT_NO_PRINT
    // 验证中文别名与英文函数名指向相同的地址
    TEST_ASSERT((void*)打印整数 == (void*)cn_rt_print_int, 
                "打印整数 应指向 cn_rt_print_int");
    TEST_ASSERT((void*)打印小数 == (void*)cn_rt_print_float, 
                "打印小数 应指向 cn_rt_print_float");
    TEST_ASSERT((void*)打印布尔 == (void*)cn_rt_print_bool, 
                "打印布尔 应指向 cn_rt_print_bool");
    TEST_ASSERT((void*)打印换行 == (void*)cn_rt_print_newline, 
                "打印换行 应指向 cn_rt_print_newline");
    
    // 功能测试：使用中文别名进行输出操作
    printf("  测试输出: ");
    打印整数(12345);
    打印换行();
    打印小数(3.14159);
    打印换行();
    打印布尔(1);
    打印换行();
    TEST_ASSERT(1, "输出函数中文别名调用成功");
#else
    printf("  [跳过] CN_RT_NO_PRINT 已定义\n");
#endif
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试5：数学函数的中文别名
// =============================================================================
static int test_math_chinese_names(void) {
    printf("测试5：数学函数的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证中文别名与英文函数名指向相同的地址
    TEST_ASSERT((void*)求幂 == (void*)cn_rt_pow, 
                "求幂 应指向 cn_rt_pow");
    TEST_ASSERT((void*)求平方根 == (void*)cn_rt_sqrt, 
                "求平方根 应指向 cn_rt_sqrt");
    
    // 功能测试：使用中文别名进行数学运算
    double result = 求幂(2.0, 10.0);
    TEST_ASSERT(fabs(result - 1024.0) < 0.0001, 
                "求幂(2.0, 10.0) 应返回约 1024.0");
    
    double sqrt_val = 求平方根(16.0);
    TEST_ASSERT(fabs(sqrt_val - 4.0) < 0.0001, 
                "求平方根(16.0) 应返回约 4.0");
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试6：同步原语的中文别名
// =============================================================================
static int test_sync_chinese_names(void) {
    printf("测试6：同步原语的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证互斥锁中文别名
    TEST_ASSERT((void*)初始化互斥锁 == (void*)cn_rt_mutex_init, 
                "初始化互斥锁 应指向 cn_rt_mutex_init");
    TEST_ASSERT((void*)销毁互斥锁 == (void*)cn_rt_mutex_destroy, 
                "销毁互斥锁 应指向 cn_rt_mutex_destroy");
    TEST_ASSERT((void*)加互斥锁 == (void*)cn_rt_mutex_lock, 
                "加互斥锁 应指向 cn_rt_mutex_lock");
    TEST_ASSERT((void*)解互斥锁 == (void*)cn_rt_mutex_unlock, 
                "解互斥锁 应指向 cn_rt_mutex_unlock");
    
    // 验证自旋锁中文别名
    TEST_ASSERT((void*)初始化自旋锁 == (void*)cn_rt_spinlock_init, 
                "初始化自旋锁 应指向 cn_rt_spinlock_init");
    TEST_ASSERT((void*)加自旋锁 == (void*)cn_rt_spinlock_lock, 
                "加自旋锁 应指向 cn_rt_spinlock_lock");
    TEST_ASSERT((void*)解自旋锁 == (void*)cn_rt_spinlock_unlock, 
                "解自旋锁 应指向 cn_rt_spinlock_unlock");
    
    // 验证读写锁中文别名
    TEST_ASSERT((void*)初始化读写锁 == (void*)cn_rt_rwlock_init, 
                "初始化读写锁 应指向 cn_rt_rwlock_init");
    TEST_ASSERT((void*)读锁定 == (void*)cn_rt_rwlock_read_lock, 
                "读锁定 应指向 cn_rt_rwlock_read_lock");
    TEST_ASSERT((void*)写锁定 == (void*)cn_rt_rwlock_write_lock, 
                "写锁定 应指向 cn_rt_rwlock_write_lock");
    
    // 功能测试：使用中文别名操作互斥锁
    CnMutex mutex;
    初始化互斥锁(&mutex);
    加互斥锁(&mutex);
    解互斥锁(&mutex);
    销毁互斥锁(&mutex);
    TEST_ASSERT(1, "互斥锁中文别名操作成功");
    
    // 功能测试：使用中文别名操作自旋锁
    CnSpinlock spinlock;
    初始化自旋锁(&spinlock);
    加自旋锁(&spinlock);
    解自旋锁(&spinlock);
    销毁自旋锁(&spinlock);
    TEST_ASSERT(1, "自旋锁中文别名操作成功");
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试7：原子操作的中文别名
// =============================================================================
static int test_atomic_chinese_names(void) {
    printf("测试7：原子操作的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证32位原子操作中文别名
    TEST_ASSERT((void*)初始化原子32 == (void*)cn_rt_atomic32_init, 
                "初始化原子32 应指向 cn_rt_atomic32_init");
    TEST_ASSERT((void*)原子读取32 == (void*)cn_rt_atomic32_load, 
                "原子读取32 应指向 cn_rt_atomic32_load");
    TEST_ASSERT((void*)原子写入32 == (void*)cn_rt_atomic32_store, 
                "原子写入32 应指向 cn_rt_atomic32_store");
    TEST_ASSERT((void*)原子加法32 == (void*)cn_rt_atomic32_fetch_add, 
                "原子加法32 应指向 cn_rt_atomic32_fetch_add");
    
    // 功能测试：使用中文别名进行原子操作
    CnAtomic32 atomic;
    初始化原子32(&atomic, 0);
    
    原子写入32(&atomic, 100, CN_MEMORY_ORDER_SEQ_CST);
    int32_t val = 原子读取32(&atomic, CN_MEMORY_ORDER_SEQ_CST);
    TEST_ASSERT(val == 100, "原子写入/读取 应正确工作");
    
    int32_t old = 原子加法32(&atomic, 50, CN_MEMORY_ORDER_SEQ_CST);
    TEST_ASSERT(old == 100, "原子加法32 应返回旧值");
    
    val = 原子读取32(&atomic, CN_MEMORY_ORDER_SEQ_CST);
    TEST_ASSERT(val == 150, "原子加法后值应为 150");
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试8：内存屏障的中文别名
// =============================================================================
static int test_barrier_chinese_names(void) {
    printf("测试8：内存屏障的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证内存屏障中文别名
    TEST_ASSERT((void*)内存屏障 == (void*)cn_rt_memory_barrier, 
                "内存屏障 应指向 cn_rt_memory_barrier");
    TEST_ASSERT((void*)编译器屏障 == (void*)cn_rt_compiler_barrier, 
                "编译器屏障 应指向 cn_rt_compiler_barrier");
    
    // 功能测试：调用内存屏障
    内存屏障();
    编译器屏障();
    TEST_ASSERT(1, "内存屏障中文别名调用成功");
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试9：数组函数的中文别名
// =============================================================================
static int test_array_chinese_names(void) {
    printf("测试9：数组函数的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证数组函数中文别名
    TEST_ASSERT((void*)分配数组 == (void*)cn_rt_array_alloc, 
                "分配数组 应指向 cn_rt_array_alloc");
    TEST_ASSERT((void*)获取数组长度 == (void*)cn_rt_array_length, 
                "获取数组长度 应指向 cn_rt_array_length");
    TEST_ASSERT((void*)释放数组 == (void*)cn_rt_array_free, 
                "释放数组 应指向 cn_rt_array_free");
    
    // 功能测试：使用中文别名操作数组
    int* arr = (int*)分配数组(sizeof(int), 10);
    TEST_ASSERT(arr != NULL, "分配数组 应返回非空指针");
    
    if (arr) {
        size_t len = 获取数组长度(arr);
        TEST_ASSERT(len == 10, "获取数组长度 应返回 10");
        释放数组(arr);
    }
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 测试10：内存安全检查函数的中文别名
// =============================================================================
static int test_memory_safety_chinese_names(void) {
    printf("测试10：内存安全检查函数的中文别名\n");
    
#ifndef CN_NO_CHINESE_NAMES
    // 验证内存安全检查函数中文别名
    TEST_ASSERT((void*)设置内存安全检查 == (void*)cn_rt_memory_set_safety_check, 
                "设置内存安全检查 应指向 cn_rt_memory_set_safety_check");
    TEST_ASSERT((void*)检查内存对齐 == (void*)cn_rt_memory_check_alignment, 
                "检查内存对齐 应指向 cn_rt_memory_check_alignment");
    TEST_ASSERT((void*)安全复制内存 == (void*)cn_rt_memory_copy_safe, 
                "安全复制内存 应指向 cn_rt_memory_copy_safe");
    TEST_ASSERT((void*)安全设置内存 == (void*)cn_rt_memory_set_safe, 
                "安全设置内存 应指向 cn_rt_memory_set_safe");
    
    // 功能测试：使用中文别名进行内存安全检查
    int result = 检查内存对齐(0x1000, 4096);
    TEST_ASSERT(result == 1, "检查内存对齐(0x1000, 4096) 应返回 1（对齐）");
    
    char buffer[100];
    安全设置内存(buffer, 0, sizeof(buffer));
    TEST_ASSERT(buffer[0] == 0 && buffer[99] == 0, "安全设置内存 应正确清零");
#else
    printf("  [跳过] CN_NO_CHINESE_NAMES 已定义\n");
#endif
    return 0;
}

// =============================================================================
// 主函数
// =============================================================================
int main(void) {
    printf("========================================\n");
    printf("CN语言标准库中文函数名测试\n");
    printf("========================================\n\n");
    
    // 运行所有测试
    test_runtime_lifecycle_chinese_names();
    test_memory_chinese_names();
    test_string_chinese_names();
    test_io_chinese_names();
    test_math_chinese_names();
    test_sync_chinese_names();
    test_atomic_chinese_names();
    test_barrier_chinese_names();
    test_array_chinese_names();
    test_memory_safety_chinese_names();
    
    // 输出测试结果
    printf("\n========================================\n");
    printf("测试结果: %d 通过, %d 失败 (总计 %d)\n", 
           passed_tests, total_tests - passed_tests, total_tests);
    printf("========================================\n");
    
    return (passed_tests == total_tests) ? 0 : 1;
}
