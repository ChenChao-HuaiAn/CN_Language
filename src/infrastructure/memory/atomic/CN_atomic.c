/******************************************************************************
 * 文件名: CN_atomic.c
 * 功能: CN_Language原子操作和内存屏障原语实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现原子操作和内存屏障接口
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_atomic.h"
#include "../../platform/CN_platform.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 内部数据结构
// ============================================================================

// 默认原子操作接口实例
static const Stru_CN_AtomicInterface_t* g_default_atomic_interface = NULL;

// 自定义原子操作接口实例
static Stru_CN_AtomicInterface_t* g_custom_atomic_interface = NULL;

// ============================================================================
// 编译器内置函数适配
// ============================================================================

/**
 * @brief 将CN内存顺序转换为编译器内置函数的内存顺序
 */
static int convert_to_compiler_memory_order(Eum_CN_MemoryOrder_t order)
{
    switch (order)
    {
        case Eum_MEMORY_ORDER_RELAXED:
#if defined(__GNUC__) || defined(__clang__)
            return __ATOMIC_RELAXED;
#elif defined(_MSC_VER)
            return 0; // MSVC没有明确的relaxed顺序
#endif
        case Eum_MEMORY_ORDER_CONSUME:
#if defined(__GNUC__) || defined(__clang__)
            return __ATOMIC_CONSUME;
#elif defined(_MSC_VER)
            return 1;
#endif
        case Eum_MEMORY_ORDER_ACQUIRE:
#if defined(__GNUC__) || defined(__clang__)
            return __ATOMIC_ACQUIRE;
#elif defined(_MSC_VER)
            return 2;
#endif
        case Eum_MEMORY_ORDER_RELEASE:
#if defined(__GNUC__) || defined(__clang__)
            return __ATOMIC_RELEASE;
#elif defined(_MSC_VER)
            return 3;
#endif
        case Eum_MEMORY_ORDER_ACQ_REL:
#if defined(__GNUC__) || defined(__clang__)
            return __ATOMIC_ACQ_REL;
#elif defined(_MSC_VER)
            return 4;
#endif
        case Eum_MEMORY_ORDER_SEQ_CST:
        default:
#if defined(__GNUC__) || defined(__clang__)
            return __ATOMIC_SEQ_CST;
#elif defined(_MSC_VER)
            return 5;
#endif
    }
}

/**
 * @brief 使用编译器内置函数的原子操作实现
 */
static int32_t builtin_load_i32(const volatile int32_t* ptr, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_load_n(ptr, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
    int32_t value = *ptr;
    _ReadWriteBarrier();
    (void)order;
    return value;
#else
    // 回退到普通加载
    (void)order;
    return *ptr;
#endif
}

static void builtin_store_i32(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    __atomic_store_n(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
    *ptr = value;
    _ReadWriteBarrier();
    (void)order;
#else
    // 回退到普通存储
    (void)order;
    *ptr = value;
#endif
}

static int32_t builtin_exchange_i32(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_exchange_n(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    return InterlockedExchange((volatile LONG*)ptr, value);
#else
    // 非原子版本
    (void)order;
    int32_t old_value = *ptr;
    *ptr = value;
    return old_value;
#endif
}

static bool builtin_compare_exchange_i32(volatile int32_t* ptr, int32_t* expected, 
                                        int32_t desired, Eum_CN_MemoryOrder_t success_order,
                                        Eum_CN_MemoryOrder_t failure_order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_compare_exchange_n(ptr, expected, desired, false,
                                      convert_to_compiler_memory_order(success_order),
                                      convert_to_compiler_memory_order(failure_order));
#elif defined(_MSC_VER)
    int32_t comparand = *expected;
    int32_t result = InterlockedCompareExchange((volatile LONG*)ptr, desired, comparand);
    if (result == comparand)
    {
        return true;
    }
    else
    {
        *expected = result;
        return false;
    }
#else
    // 非原子版本
    (void)success_order;
    (void)failure_order;
    if (*ptr == *expected)
    {
        *ptr = desired;
        return true;
    }
    else
    {
        *expected = *ptr;
        return false;
    }
#endif
}

static int32_t builtin_fetch_add_i32(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_fetch_add(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    return InterlockedExchangeAdd((volatile LONG*)ptr, value);
#else
    // 非原子版本
    (void)order;
    int32_t old_value = *ptr;
    *ptr += value;
    return old_value;
#endif
}

static int32_t builtin_fetch_sub_i32(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_fetch_sub(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    return InterlockedExchangeAdd((volatile LONG*)ptr, -value);
#else
    // 非原子版本
    (void)order;
    int32_t old_value = *ptr;
    *ptr -= value;
    return old_value;
#endif
}

static int32_t builtin_fetch_and_i32(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_fetch_and(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    int32_t old_value;
    int32_t new_value;
    do {
        old_value = *ptr;
        new_value = old_value & value;
    } while (InterlockedCompareExchange((volatile LONG*)ptr, new_value, old_value) != old_value);
    return old_value;
#else
    // 非原子版本
    (void)order;
    int32_t old_value = *ptr;
    *ptr &= value;
    return old_value;
#endif
}

static int32_t builtin_fetch_or_i32(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_fetch_or(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    int32_t old_value;
    int32_t new_value;
    do {
        old_value = *ptr;
        new_value = old_value | value;
    } while (InterlockedCompareExchange((volatile LONG*)ptr, new_value, old_value) != old_value);
    return old_value;
#else
    // 非原子版本
    (void)order;
    int32_t old_value = *ptr;
    *ptr |= value;
    return old_value;
#endif
}

static int32_t builtin_fetch_xor_i32(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_fetch_xor(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    int32_t old_value;
    int32_t new_value;
    do {
        old_value = *ptr;
        new_value = old_value ^ value;
    } while (InterlockedCompareExchange((volatile LONG*)ptr, new_value, old_value) != old_value);
    return old_value;
#else
    // 非原子版本
    (void)order;
    int32_t old_value = *ptr;
    *ptr ^= value;
    return old_value;
#endif
}

// 64位原子操作（如果平台支持）
static int64_t builtin_load_i64(const volatile int64_t* ptr, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_load_n(ptr, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER) && defined(_WIN64)
    _ReadWriteBarrier();
    int64_t value = *ptr;
    _ReadWriteBarrier();
    (void)order;
    return value;
#else
    // 回退到普通加载或不支持
    (void)order;
    return *ptr;
#endif
}

static void builtin_store_i64(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    __atomic_store_n(ptr, value, convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER) && defined(_WIN64)
    _ReadWriteBarrier();
    *ptr = value;
    _ReadWriteBarrier();
    (void)order;
#else
    // 回退到普通存储
    (void)order;
    *ptr = value;
#endif
}

// 内存屏障实现
static void builtin_thread_fence(Eum_CN_MemoryOrder_t order)
{
#if defined(__GNUC__) || defined(__clang__)
    __atomic_thread_fence(convert_to_compiler_memory_order(order));
#elif defined(_MSC_VER)
    switch (order)
    {
        case Eum_MEMORY_ORDER_ACQUIRE:
            _ReadBarrier();
            break;
        case Eum_MEMORY_ORDER_RELEASE:
            _WriteBarrier();
            break;
        case Eum_MEMORY_ORDER_ACQ_REL:
            _ReadWriteBarrier();
            break;
        case Eum_MEMORY_ORDER_SEQ_CST:
            MemoryBarrier();
            break;
        default:
            _ReadWriteBarrier();
            break;
    }
#else
    // 空实现
    (void)order;
#endif
}

static void builtin_compiler_barrier(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" ::: "memory");
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
#else
    // 空实现
#endif
}

static void builtin_full_memory_barrier(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __sync_synchronize();
#elif defined(_MSC_VER)
    MemoryBarrier();
#else
    // 空实现
#endif
}

static void builtin_load_load_barrier(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
#elif defined(_MSC_VER)
    _ReadBarrier();
#else
    // 空实现
#endif
}

static void builtin_store_store_barrier(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __atomic_thread_fence(__ATOMIC_RELEASE);
#elif defined(_MSC_VER)
    _WriteBarrier();
#else
    // 空实现
#endif
}

static void builtin_load_store_barrier(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __atomic_thread_fence(__ATOMIC_ACQ_REL);
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
#else
    // 空实现
#endif
}

static void builtin_store_load_barrier(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif defined(_MSC_VER)
    MemoryBarrier();
#else
    // 空实现
#endif
}

// ============================================================================
// 默认原子操作接口实现
// ============================================================================

static const Stru_CN_AtomicInterface_t g_default_interface = {
    // 32位整数操作
    .load_i32 = builtin_load_i32,
    .store_i32 = builtin_store_i32,
    .exchange_i32 = builtin_exchange_i32,
    .compare_exchange_i32 = builtin_compare_exchange_i32,
    .fetch_add_i32 = builtin_fetch_add_i32,
    .fetch_sub_i32 = builtin_fetch_sub_i32,
    .fetch_and_i32 = builtin_fetch_and_i32,
    .fetch_or_i32 = builtin_fetch_or_i32,
    .fetch_xor_i32 = builtin_fetch_xor_i32,
    
    // 64位整数操作
    .load_i64 = builtin_load_i64,
    .store_i64 = builtin_store_i64,
    .exchange_i64 = NULL, // 暂未实现
    .compare_exchange_i64 = NULL, // 暂未实现
    .fetch_add_i64 = NULL, // 暂未实现
    .fetch_sub_i64 = NULL, // 暂未实现
    
    // 指针操作（暂未实现）
    .load_ptr = NULL,
    .store_ptr = NULL,
    .exchange_ptr = NULL,
    .compare_exchange_ptr = NULL,
    .fetch_add_ptr = NULL,
    .fetch_sub_ptr = NULL,
    
    // 内存屏障操作
    .thread_fence = builtin_thread_fence,
    .signal_fence = NULL, // 暂未实现
    .compiler_barrier = builtin_compiler_barrier,
    .full_memory_barrier = builtin_full_memory_barrier,
    .load_load_barrier = builtin_load_load_barrier,
    .store_store_barrier = builtin_store_store_barrier,
    .load_store_barrier = builtin_load_store_barrier,
    .store_load_barrier = builtin_store_load_barrier
};

// ============================================================================
// 原子操作管理函数实现
// ============================================================================

const Stru_CN_AtomicInterface_t* CN_atomic_get_default(void)
{
    if (g_custom_atomic_interface != NULL)
    {
        return g_custom_atomic_interface;
    }
    
    if (g_default_atomic_interface == NULL)
    {
        g_default_atomic_interface = &g_default_interface;
    }
    
    return g_default_atomic_interface;
}

Stru_CN_AtomicInterface_t* CN_atomic_create_custom(const Stru_CN_AtomicInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 分配内存
    Stru_CN_AtomicInterface_t* new_interface = malloc(sizeof(Stru_CN_AtomicInterface_t));
    if (new_interface == NULL)
    {
        return NULL;
    }
    
    // 复制接口
    memcpy(new_interface, interface, sizeof(Stru_CN_AtomicInterface_t));
    
    // 设置自定义接口
    if (g_custom_atomic_interface != NULL)
    {
        free(g_custom_atomic_interface);
    }
    
    g_custom_atomic_interface = new_interface;
    return new_interface;
}

void CN_atomic_destroy(Stru_CN_AtomicInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface == g_custom_atomic_interface)
    {
        g_custom_atomic_interface = NULL;
    }
    
    free(interface);
}

// ============================================================================
// 便捷函数实现（使用默认接口）
// ============================================================================

int32_t CN_atomic_load_i32(const volatile int32_t* ptr)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->load_i32)
    {
        return atomic->load_i32(ptr, Eum_MEMORY_ORDER_SEQ_CST);
    }
    return *ptr;
}

void CN_atomic_store_i32(volatile int32_t* ptr, int32_t value)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->store_i32)
    {
        atomic->store_i32(ptr, value, Eum_MEMORY_ORDER_SEQ_CST);
        return;
    }
    *ptr = value;
}

int32_t CN_atomic_exchange_i32(volatile int32_t* ptr, int32_t value)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->exchange_i32)
    {
        return atomic->exchange_i32(ptr, value, Eum_MEMORY_ORDER_SEQ_CST);
    }
    
    int32_t old_value = *ptr;
    *ptr = value;
    return old_value;
}

bool CN_atomic_compare_exchange_i32(volatile int32_t* ptr, int32_t* expected, int32_t desired)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->compare_exchange_i32)
    {
        return atomic->compare_exchange_i32(ptr, expected, desired, 
                                           Eum_MEMORY_ORDER_SEQ_CST, Eum_MEMORY_ORDER_SEQ_CST);
    }
    
    if (*ptr == *expected)
    {
        *ptr = desired;
        return true;
    }
    else
    {
        *expected = *ptr;
        return false;
    }
}

int32_t CN_atomic_fetch_add_i32(volatile int32_t* ptr, int32_t value)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->fetch_add_i32)
    {
        return atomic->fetch_add_i32(ptr, value, Eum_MEMORY_ORDER_SEQ_CST);
    }
    
    int32_t old_value = *ptr;
    *ptr += value;
    return old_value;
}

int32_t CN_atomic_fetch_sub_i32(volatile int32_t* ptr, int32_t value)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->fetch_sub_i32)
    {
        return atomic->fetch_sub_i32(ptr, value, Eum_MEMORY_ORDER_SEQ_CST);
    }
    
    int32_t old_value = *ptr;
    *ptr -= value;
    return old_value;
}

int32_t CN_atomic_fetch_increment_i32(volatile int32_t* ptr)
{
    return CN_atomic_fetch_add_i32(ptr, 1);
}

int32_t CN_atomic_fetch_decrement_i32(volatile int32_t* ptr)
{
    return CN_atomic_fetch_sub_i32(ptr, 1);
}

void CN_atomic_full_memory_barrier(void)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->full_memory_barrier)
    {
        atomic->full_memory_barrier();
        return;
    }
    
    builtin_full_memory_barrier();
}

void CN_atomic_compiler_barrier(void)
{
    const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();
    if (atomic && atomic->compiler_barrier)
    {
        atomic->compiler_barrier();
        return;
    }
    
    builtin_compiler_barrier();
}
