#include "cnlang/runtime/sync.h"
#include <stdio.h>
#include <string.h>

// Windows平台特定实现
#ifdef _WIN32
#include <windows.h>
#include <intrin.h>

// CPU暂停指令
#define CN_CPU_PAUSE() _mm_pause()

// 原子操作宏 - 32位
#define CN_ATOMIC32_LOAD(ptr, order) \
    InterlockedCompareExchange((volatile LONG*)(ptr), 0, 0)

#define CN_ATOMIC32_STORE(ptr, val, order) \
    InterlockedExchange((volatile LONG*)(ptr), (LONG)(val))

#define CN_ATOMIC32_EXCHANGE(ptr, val, order) \
    InterlockedExchange((volatile LONG*)(ptr), (LONG)(val))

#define CN_ATOMIC32_CAS(ptr, expected, desired, order) \
    (InterlockedCompareExchange((volatile LONG*)(ptr), (LONG)(desired), (LONG)(expected)) == (LONG)(expected))

#define CN_ATOMIC32_FETCH_ADD(ptr, val, order) \
    InterlockedExchangeAdd((volatile LONG*)(ptr), (LONG)(val))

#define CN_ATOMIC32_FETCH_SUB(ptr, val, order) \
    InterlockedExchangeAdd((volatile LONG*)(ptr), -(LONG)(val))

#define CN_ATOMIC32_FETCH_OR(ptr, val, order) \
    InterlockedOr((volatile LONG*)(ptr), (LONG)(val))

#define CN_ATOMIC32_FETCH_AND(ptr, val, order) \
    InterlockedAnd((volatile LONG*)(ptr), (LONG)(val))

#define CN_ATOMIC32_FETCH_XOR(ptr, val, order) \
    InterlockedXor((volatile LONG*)(ptr), (LONG)(val))

// 原子操作宏 - 64位
#define CN_ATOMIC64_LOAD(ptr, order) \
    InterlockedCompareExchange64((volatile LONG64*)(ptr), 0, 0)

#define CN_ATOMIC64_STORE(ptr, val, order) \
    InterlockedExchange64((volatile LONG64*)(ptr), (LONG64)(val))

#define CN_ATOMIC64_EXCHANGE(ptr, val, order) \
    InterlockedExchange64((volatile LONG64*)(ptr), (LONG64)(val))

#define CN_ATOMIC64_CAS(ptr, expected, desired, order) \
    (InterlockedCompareExchange64((volatile LONG64*)(ptr), (LONG64)(desired), (LONG64)(expected)) == (LONG64)(expected))

#define CN_ATOMIC64_FETCH_ADD(ptr, val, order) \
    InterlockedExchangeAdd64((volatile LONG64*)(ptr), (LONG64)(val))

#define CN_ATOMIC64_FETCH_SUB(ptr, val, order) \
    InterlockedExchangeAdd64((volatile LONG64*)(ptr), -(LONG64)(val))

// 内存屏障
#define CN_MEMORY_BARRIER() MemoryBarrier()
#define CN_COMPILER_BARRIER() _ReadWriteBarrier()

// 获取线程ID
static inline uint32_t cn_get_thread_id_impl(void) {
    return (uint32_t)GetCurrentThreadId();
}

#else
// GCC/Clang平台实现 (Linux, macOS等)
#include <pthread.h>
#include <unistd.h>

// CPU暂停指令
#if defined(__x86_64__) || defined(__i386__)
#define CN_CPU_PAUSE() __builtin_ia32_pause()
#elif defined(__aarch64__)
#define CN_CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
#else
#define CN_CPU_PAUSE() ((void)0)
#endif

// 内存顺序转换
static inline int cn_order_to_gcc(CnMemoryOrder order) {
    switch (order) {
        case CN_MEMORY_ORDER_RELAXED: return __ATOMIC_RELAXED;
        case CN_MEMORY_ORDER_ACQUIRE: return __ATOMIC_ACQUIRE;
        case CN_MEMORY_ORDER_RELEASE: return __ATOMIC_RELEASE;
        case CN_MEMORY_ORDER_SEQ_CST: return __ATOMIC_SEQ_CST;
        default: return __ATOMIC_SEQ_CST;
    }
}

// 原子操作宏 - 32位
#define CN_ATOMIC32_LOAD(ptr, order) \
    __atomic_load_n((int32_t*)(ptr), cn_order_to_gcc(order))

#define CN_ATOMIC32_STORE(ptr, val, order) \
    __atomic_store_n((int32_t*)(ptr), (int32_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC32_EXCHANGE(ptr, val, order) \
    __atomic_exchange_n((int32_t*)(ptr), (int32_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC32_CAS(ptr, expected, desired, order) \
    __atomic_compare_exchange_n((int32_t*)(ptr), (int32_t*)(expected), (int32_t)(desired), \
                                 0, cn_order_to_gcc(order), cn_order_to_gcc(order))

#define CN_ATOMIC32_FETCH_ADD(ptr, val, order) \
    __atomic_fetch_add((int32_t*)(ptr), (int32_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC32_FETCH_SUB(ptr, val, order) \
    __atomic_fetch_sub((int32_t*)(ptr), (int32_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC32_FETCH_OR(ptr, val, order) \
    __atomic_fetch_or((int32_t*)(ptr), (int32_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC32_FETCH_AND(ptr, val, order) \
    __atomic_fetch_and((int32_t*)(ptr), (int32_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC32_FETCH_XOR(ptr, val, order) \
    __atomic_fetch_xor((int32_t*)(ptr), (int32_t)(val), cn_order_to_gcc(order))

// 原子操作宏 - 64位
#define CN_ATOMIC64_LOAD(ptr, order) \
    __atomic_load_n((int64_t*)(ptr), cn_order_to_gcc(order))

#define CN_ATOMIC64_STORE(ptr, val, order) \
    __atomic_store_n((int64_t*)(ptr), (int64_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC64_EXCHANGE(ptr, val, order) \
    __atomic_exchange_n((int64_t*)(ptr), (int64_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC64_CAS(ptr, expected, desired, order) \
    __atomic_compare_exchange_n((int64_t*)(ptr), (int64_t*)(expected), (int64_t)(desired), \
                                 0, cn_order_to_gcc(order), cn_order_to_gcc(order))

#define CN_ATOMIC64_FETCH_ADD(ptr, val, order) \
    __atomic_fetch_add((int64_t*)(ptr), (int64_t)(val), cn_order_to_gcc(order))

#define CN_ATOMIC64_FETCH_SUB(ptr, val, order) \
    __atomic_fetch_sub((int64_t*)(ptr), (int64_t)(val), cn_order_to_gcc(order))

// 内存屏障
#define CN_MEMORY_BARRIER() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define CN_COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")

// 获取线程ID
static inline uint32_t cn_get_thread_id_impl(void) {
    return (uint32_t)pthread_self();
}

#endif

// =============================================================================
// 原子操作实现 - 32位整数
// =============================================================================

void cn_rt_atomic32_init(CnAtomic32* atomic, int32_t value) {
    atomic->value = value;
}

int32_t cn_rt_atomic32_load(CnAtomic32* atomic, CnMemoryOrder order) {
    return CN_ATOMIC32_LOAD(&atomic->value, order);
}

void cn_rt_atomic32_store(CnAtomic32* atomic, int32_t value, CnMemoryOrder order) {
    CN_ATOMIC32_STORE(&atomic->value, value, order);
}

int32_t cn_rt_atomic32_exchange(CnAtomic32* atomic, int32_t new_value, CnMemoryOrder order) {
    return CN_ATOMIC32_EXCHANGE(&atomic->value, new_value, order);
}

int cn_rt_atomic32_compare_exchange(CnAtomic32* atomic, int32_t* expected, 
                                     int32_t desired, CnMemoryOrder order) {
    return CN_ATOMIC32_CAS(&atomic->value, *expected, desired, order);
}

int32_t cn_rt_atomic32_fetch_add(CnAtomic32* atomic, int32_t value, CnMemoryOrder order) {
    return CN_ATOMIC32_FETCH_ADD(&atomic->value, value, order);
}

int32_t cn_rt_atomic32_fetch_sub(CnAtomic32* atomic, int32_t value, CnMemoryOrder order) {
    return CN_ATOMIC32_FETCH_SUB(&atomic->value, value, order);
}

int32_t cn_rt_atomic32_fetch_or(CnAtomic32* atomic, int32_t value, CnMemoryOrder order) {
    return CN_ATOMIC32_FETCH_OR(&atomic->value, value, order);
}

int32_t cn_rt_atomic32_fetch_and(CnAtomic32* atomic, int32_t value, CnMemoryOrder order) {
    return CN_ATOMIC32_FETCH_AND(&atomic->value, value, order);
}

int32_t cn_rt_atomic32_fetch_xor(CnAtomic32* atomic, int32_t value, CnMemoryOrder order) {
    return CN_ATOMIC32_FETCH_XOR(&atomic->value, value, order);
}

// =============================================================================
// 原子操作实现 - 64位整数
// =============================================================================

void cn_rt_atomic64_init(CnAtomic64* atomic, int64_t value) {
    atomic->value = value;
}

int64_t cn_rt_atomic64_load(CnAtomic64* atomic, CnMemoryOrder order) {
    return CN_ATOMIC64_LOAD(&atomic->value, order);
}

void cn_rt_atomic64_store(CnAtomic64* atomic, int64_t value, CnMemoryOrder order) {
    CN_ATOMIC64_STORE(&atomic->value, value, order);
}

int64_t cn_rt_atomic64_exchange(CnAtomic64* atomic, int64_t new_value, CnMemoryOrder order) {
    return CN_ATOMIC64_EXCHANGE(&atomic->value, new_value, order);
}

int cn_rt_atomic64_compare_exchange(CnAtomic64* atomic, int64_t* expected, 
                                     int64_t desired, CnMemoryOrder order) {
    return CN_ATOMIC64_CAS(&atomic->value, *expected, desired, order);
}

int64_t cn_rt_atomic64_fetch_add(CnAtomic64* atomic, int64_t value, CnMemoryOrder order) {
    return CN_ATOMIC64_FETCH_ADD(&atomic->value, value, order);
}

int64_t cn_rt_atomic64_fetch_sub(CnAtomic64* atomic, int64_t value, CnMemoryOrder order) {
    return CN_ATOMIC64_FETCH_SUB(&atomic->value, value, order);
}

// =============================================================================
// 原子操作实现 - 指针
// =============================================================================

void cn_rt_atomic_ptr_init(CnAtomicPtr* atomic, void* value) {
    atomic->value = value;
}

void* cn_rt_atomic_ptr_load(CnAtomicPtr* atomic, CnMemoryOrder order) {
#ifdef _WIN32
    return (void*)InterlockedCompareExchangePointer(&atomic->value, NULL, NULL);
#else
    return __atomic_load_n(&atomic->value, cn_order_to_gcc(order));
#endif
}

void cn_rt_atomic_ptr_store(CnAtomicPtr* atomic, void* value, CnMemoryOrder order) {
#ifdef _WIN32
    InterlockedExchangePointer(&atomic->value, value);
#else
    __atomic_store_n(&atomic->value, value, cn_order_to_gcc(order));
#endif
}

void* cn_rt_atomic_ptr_exchange(CnAtomicPtr* atomic, void* new_value, CnMemoryOrder order) {
#ifdef _WIN32
    return InterlockedExchangePointer(&atomic->value, new_value);
#else
    return __atomic_exchange_n(&atomic->value, new_value, cn_order_to_gcc(order));
#endif
}

int cn_rt_atomic_ptr_compare_exchange(CnAtomicPtr* atomic, void** expected, 
                                       void* desired, CnMemoryOrder order) {
#ifdef _WIN32
    void* old = InterlockedCompareExchangePointer(&atomic->value, desired, *expected);
    if (old == *expected) {
        return 1;
    } else {
        *expected = old;
        return 0;
    }
#else
    return __atomic_compare_exchange_n(&atomic->value, expected, desired, 
                                        0, cn_order_to_gcc(order), cn_order_to_gcc(order));
#endif
}

// =============================================================================
// 内存屏障实现
// =============================================================================

void cn_rt_memory_barrier(void) {
    CN_MEMORY_BARRIER();
}

void cn_rt_compiler_barrier(void) {
    CN_COMPILER_BARRIER();
}

// =============================================================================
// 互斥锁实现
// =============================================================================

void cn_rt_mutex_init(CnMutex* mutex) {
    cn_rt_atomic32_init(&mutex->locked, 0);
    mutex->owner_thread_id = 0;
    mutex->lock_count = 0;
}

void cn_rt_mutex_destroy(CnMutex* mutex) {
    // 确保锁已解锁
    if (cn_rt_atomic32_load(&mutex->locked, CN_MEMORY_ORDER_RELAXED) != 0) {
        fprintf(stderr, "警告: 销毁仍被持有的互斥锁\n");
    }
}

void cn_rt_mutex_lock(CnMutex* mutex) {
    uint32_t tid = cn_rt_get_thread_id();
    
    // 支持可重入
    if (mutex->owner_thread_id == tid) {
        mutex->lock_count++;
        return;
    }
    
    // 自旋等待锁释放
    int32_t expected = 0;
    while (!cn_rt_atomic32_compare_exchange(&mutex->locked, &expected, 1, 
                                             CN_MEMORY_ORDER_ACQUIRE)) {
        expected = 0;
        cn_rt_cpu_pause();
    }
    
    mutex->owner_thread_id = tid;
    mutex->lock_count = 1;
}

int cn_rt_mutex_trylock(CnMutex* mutex) {
    uint32_t tid = cn_rt_get_thread_id();
    
    // 支持可重入
    if (mutex->owner_thread_id == tid) {
        mutex->lock_count++;
        return 1;
    }
    
    int32_t expected = 0;
    if (cn_rt_atomic32_compare_exchange(&mutex->locked, &expected, 1, 
                                         CN_MEMORY_ORDER_ACQUIRE)) {
        mutex->owner_thread_id = tid;
        mutex->lock_count = 1;
        return 1;
    }
    return 0;
}

void cn_rt_mutex_unlock(CnMutex* mutex) {
    uint32_t tid = cn_rt_get_thread_id();
    
    if (mutex->owner_thread_id != tid) {
        fprintf(stderr, "错误: 试图解锁不属于当前线程的互斥锁\n");
        return;
    }
    
    if (mutex->lock_count > 1) {
        mutex->lock_count--;
        return;
    }
    
    mutex->owner_thread_id = 0;
    mutex->lock_count = 0;
    cn_rt_atomic32_store(&mutex->locked, 0, CN_MEMORY_ORDER_RELEASE);
}

// =============================================================================
// 自旋锁实现
// =============================================================================

void cn_rt_spinlock_init(CnSpinlock* lock) {
    cn_rt_atomic32_init(&lock->locked, 0);
}

void cn_rt_spinlock_destroy(CnSpinlock* lock) {
    if (cn_rt_atomic32_load(&lock->locked, CN_MEMORY_ORDER_RELAXED) != 0) {
        fprintf(stderr, "警告: 销毁仍被持有的自旋锁\n");
    }
}

void cn_rt_spinlock_lock(CnSpinlock* lock) {
    int32_t expected = 0;
    while (!cn_rt_atomic32_compare_exchange(&lock->locked, &expected, 1, 
                                             CN_MEMORY_ORDER_ACQUIRE)) {
        expected = 0;
        cn_rt_cpu_pause();
    }
}

int cn_rt_spinlock_trylock(CnSpinlock* lock) {
    int32_t expected = 0;
    return cn_rt_atomic32_compare_exchange(&lock->locked, &expected, 1, 
                                            CN_MEMORY_ORDER_ACQUIRE);
}

void cn_rt_spinlock_unlock(CnSpinlock* lock) {
    cn_rt_atomic32_store(&lock->locked, 0, CN_MEMORY_ORDER_RELEASE);
}

// =============================================================================
// 读写锁实现
// =============================================================================

void cn_rt_rwlock_init(CnRwLock* lock) {
    cn_rt_atomic32_init(&lock->state, 0);
    cn_rt_atomic32_init(&lock->waiting_writers, 0);
}

void cn_rt_rwlock_destroy(CnRwLock* lock) {
    int32_t state = cn_rt_atomic32_load(&lock->state, CN_MEMORY_ORDER_RELAXED);
    if (state != 0) {
        fprintf(stderr, "警告: 销毁仍被持有的读写锁\n");
    }
}

void cn_rt_rwlock_read_lock(CnRwLock* lock) {
    // 等待所有写者完成
    while (1) {
        int32_t state = cn_rt_atomic32_load(&lock->state, CN_MEMORY_ORDER_RELAXED);
        if (state >= 0 && cn_rt_atomic32_load(&lock->waiting_writers, 
                                               CN_MEMORY_ORDER_RELAXED) == 0) {
            // 尝试增加读者数量
            if (cn_rt_atomic32_compare_exchange(&lock->state, &state, state + 1, 
                                                 CN_MEMORY_ORDER_ACQUIRE)) {
                return;
            }
        }
        cn_rt_cpu_pause();
    }
}

int cn_rt_rwlock_try_read_lock(CnRwLock* lock) {
    int32_t state = cn_rt_atomic32_load(&lock->state, CN_MEMORY_ORDER_RELAXED);
    if (state >= 0 && cn_rt_atomic32_load(&lock->waiting_writers, 
                                           CN_MEMORY_ORDER_RELAXED) == 0) {
        return cn_rt_atomic32_compare_exchange(&lock->state, &state, state + 1, 
                                                CN_MEMORY_ORDER_ACQUIRE);
    }
    return 0;
}

void cn_rt_rwlock_read_unlock(CnRwLock* lock) {
    cn_rt_atomic32_fetch_sub(&lock->state, 1, CN_MEMORY_ORDER_RELEASE);
}

void cn_rt_rwlock_write_lock(CnRwLock* lock) {
    // 标记有写者在等待
    cn_rt_atomic32_fetch_add(&lock->waiting_writers, 1, CN_MEMORY_ORDER_RELAXED);
    
    // 等待所有读者和写者完成
    int32_t expected = 0;
    while (!cn_rt_atomic32_compare_exchange(&lock->state, &expected, -1, 
                                             CN_MEMORY_ORDER_ACQUIRE)) {
        expected = 0;
        cn_rt_cpu_pause();
    }
    
    // 获取锁后减少等待写者计数
    cn_rt_atomic32_fetch_sub(&lock->waiting_writers, 1, CN_MEMORY_ORDER_RELAXED);
}

int cn_rt_rwlock_try_write_lock(CnRwLock* lock) {
    int32_t expected = 0;
    return cn_rt_atomic32_compare_exchange(&lock->state, &expected, -1, 
                                            CN_MEMORY_ORDER_ACQUIRE);
}

void cn_rt_rwlock_write_unlock(CnRwLock* lock) {
    cn_rt_atomic32_store(&lock->state, 0, CN_MEMORY_ORDER_RELEASE);
}

// =============================================================================
// 辅助函数实现
// =============================================================================

uint32_t cn_rt_get_thread_id(void) {
    return cn_get_thread_id_impl();
}

void cn_rt_cpu_pause(void) {
    CN_CPU_PAUSE();
}
