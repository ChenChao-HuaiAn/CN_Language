#ifndef CNLANG_RUNTIME_SYNC_H
#define CNLANG_RUNTIME_SYNC_H

#include <stddef.h>
#include <stdint.h>

/*
 * CN Language 运行时同步原语
 * 提供原子操作和锁机制，支持并发控制
 *
 * Freestanding 模式支持：
 * - [FS] 原子操作和锁机制必须可用
 * - [OPTIONAL] 具体实现可根据平台定制
 */

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 原子类型定义 [FS - 必须支持]
// =============================================================================

// 原子整数类型 (32位)
typedef struct {
    volatile int32_t value;
} CnAtomic32;

// 原子整数类型 (64位)
typedef struct {
    volatile int64_t value;
} CnAtomic64;

// 原子指针类型
typedef struct {
    void* volatile value;
} CnAtomicPtr;

// 内存顺序枚举
typedef enum {
    CN_MEMORY_ORDER_RELAXED,  // 松弛顺序
    CN_MEMORY_ORDER_ACQUIRE,  // 获取顺序
    CN_MEMORY_ORDER_RELEASE,  // 释放顺序
    CN_MEMORY_ORDER_SEQ_CST   // 顺序一致性
} CnMemoryOrder;

// =============================================================================
// 原子操作接口 - 32位整数 [FS - 必须支持]
// =============================================================================

// 初始化原子变量
void cn_rt_atomic32_init(CnAtomic32* atomic, int32_t value);

// 原子读取
int32_t cn_rt_atomic32_load(CnAtomic32* atomic, CnMemoryOrder order);

// 原子写入
void cn_rt_atomic32_store(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);

// 原子交换 (返回旧值)
int32_t cn_rt_atomic32_exchange(CnAtomic32* atomic, int32_t new_value, CnMemoryOrder order);

// 原子比较并交换 (CAS)
// 如果 *atomic == expected，则设置 *atomic = desired，返回 1
// 否则设置 *expected = *atomic，返回 0
int cn_rt_atomic32_compare_exchange(CnAtomic32* atomic, int32_t* expected, 
                                     int32_t desired, CnMemoryOrder order);

// 原子加法 (返回旧值)
int32_t cn_rt_atomic32_fetch_add(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);

// 原子减法 (返回旧值)
int32_t cn_rt_atomic32_fetch_sub(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);

// 原子按位或 (返回旧值)
int32_t cn_rt_atomic32_fetch_or(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);

// 原子按位与 (返回旧值)
int32_t cn_rt_atomic32_fetch_and(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);

// 原子按位异或 (返回旧值)
int32_t cn_rt_atomic32_fetch_xor(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);

// =============================================================================
// 原子操作接口 - 64位整数 [FS - 必须支持]
// =============================================================================

void cn_rt_atomic64_init(CnAtomic64* atomic, int64_t value);
int64_t cn_rt_atomic64_load(CnAtomic64* atomic, CnMemoryOrder order);
void cn_rt_atomic64_store(CnAtomic64* atomic, int64_t value, CnMemoryOrder order);
int64_t cn_rt_atomic64_exchange(CnAtomic64* atomic, int64_t new_value, CnMemoryOrder order);
int cn_rt_atomic64_compare_exchange(CnAtomic64* atomic, int64_t* expected, 
                                     int64_t desired, CnMemoryOrder order);
int64_t cn_rt_atomic64_fetch_add(CnAtomic64* atomic, int64_t value, CnMemoryOrder order);
int64_t cn_rt_atomic64_fetch_sub(CnAtomic64* atomic, int64_t value, CnMemoryOrder order);

// =============================================================================
// 原子操作接口 - 指针 [FS - 必须支持]
// =============================================================================

void cn_rt_atomic_ptr_init(CnAtomicPtr* atomic, void* value);
void* cn_rt_atomic_ptr_load(CnAtomicPtr* atomic, CnMemoryOrder order);
void cn_rt_atomic_ptr_store(CnAtomicPtr* atomic, void* value, CnMemoryOrder order);
void* cn_rt_atomic_ptr_exchange(CnAtomicPtr* atomic, void* new_value, CnMemoryOrder order);
int cn_rt_atomic_ptr_compare_exchange(CnAtomicPtr* atomic, void** expected, 
                                       void* desired, CnMemoryOrder order);

// =============================================================================
// 内存屏障 [FS - 必须支持]
// =============================================================================

// 完整内存屏障
void cn_rt_memory_barrier(void);

// 编译器屏障
void cn_rt_compiler_barrier(void);

// =============================================================================
// 互斥锁 (Mutex) [FS - 必须支持]
// =============================================================================

typedef struct {
    CnAtomic32 locked;
    uint32_t owner_thread_id;  // 用于调试和死锁检测
    uint32_t lock_count;       // 用于可重入锁
} CnMutex;

// 初始化互斥锁
void cn_rt_mutex_init(CnMutex* mutex);

// 销毁互斥锁
void cn_rt_mutex_destroy(CnMutex* mutex);

// 加锁 (阻塞直到获取锁)
void cn_rt_mutex_lock(CnMutex* mutex);

// 尝试加锁 (非阻塞，返回1表示成功，0表示失败)
int cn_rt_mutex_trylock(CnMutex* mutex);

// 解锁
void cn_rt_mutex_unlock(CnMutex* mutex);

// =============================================================================
// 自旋锁 (Spinlock) [FS - 必须支持]
// =============================================================================

typedef struct {
    CnAtomic32 locked;
} CnSpinlock;

// 初始化自旋锁
void cn_rt_spinlock_init(CnSpinlock* lock);

// 销毁自旋锁
void cn_rt_spinlock_destroy(CnSpinlock* lock);

// 加锁 (自旋等待)
void cn_rt_spinlock_lock(CnSpinlock* lock);

// 尝试加锁 (非阻塞)
int cn_rt_spinlock_trylock(CnSpinlock* lock);

// 解锁
void cn_rt_spinlock_unlock(CnSpinlock* lock);

// =============================================================================
// 读写锁 (RWLock) [FS - 必须支持]
// =============================================================================

typedef struct {
    CnAtomic32 state;  // 正数表示读者数量，-1表示写者持有
    CnAtomic32 waiting_writers;  // 等待的写者数量
} CnRwLock;

// 初始化读写锁
void cn_rt_rwlock_init(CnRwLock* lock);

// 销毁读写锁
void cn_rt_rwlock_destroy(CnRwLock* lock);

// 读锁定
void cn_rt_rwlock_read_lock(CnRwLock* lock);

// 尝试读锁定
int cn_rt_rwlock_try_read_lock(CnRwLock* lock);

// 读解锁
void cn_rt_rwlock_read_unlock(CnRwLock* lock);

// 写锁定
void cn_rt_rwlock_write_lock(CnRwLock* lock);

// 尝试写锁定
int cn_rt_rwlock_try_write_lock(CnRwLock* lock);

// 写解锁
void cn_rt_rwlock_write_unlock(CnRwLock* lock);

// =============================================================================
// 辅助函数 [FS - 必须支持]
// =============================================================================

// 获取当前线程ID (简化版，用于锁的所有权标识)
uint32_t cn_rt_get_thread_id(void);

// CPU暂停指令 (用于自旋锁优化)
void cn_rt_cpu_pause(void);

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

// 原子操作 - 32位
#define 初始化原子32 cn_rt_atomic32_init
#define 原子读取32 cn_rt_atomic32_load
#define 原子写入32 cn_rt_atomic32_store
#define 原子交换32 cn_rt_atomic32_exchange
#define 原子比较交换32 cn_rt_atomic32_compare_exchange
#define 原子加法32 cn_rt_atomic32_fetch_add
#define 原子减法32 cn_rt_atomic32_fetch_sub
#define 原子或32 cn_rt_atomic32_fetch_or
#define 原子与32 cn_rt_atomic32_fetch_and
#define 原子异或32 cn_rt_atomic32_fetch_xor

// 原子操作 - 64位
#define 初始化原子64 cn_rt_atomic64_init
#define 原子读取64 cn_rt_atomic64_load
#define 原子写入64 cn_rt_atomic64_store
#define 原子交换64 cn_rt_atomic64_exchange
#define 原子比较交换64 cn_rt_atomic64_compare_exchange
#define 原子加法64 cn_rt_atomic64_fetch_add
#define 原子减法64 cn_rt_atomic64_fetch_sub

// 原子操作 - 指针
#define 初始化原子指针 cn_rt_atomic_ptr_init
#define 原子读取指针 cn_rt_atomic_ptr_load
#define 原子写入指针 cn_rt_atomic_ptr_store
#define 原子交换指针 cn_rt_atomic_ptr_exchange
#define 原子比较交换指针 cn_rt_atomic_ptr_compare_exchange

// 内存屏障
#define 内存屏障 cn_rt_memory_barrier
#define 编译器屏障 cn_rt_compiler_barrier

// 互斥锁
#define 初始化互斥锁 cn_rt_mutex_init
#define 销毁互斥锁 cn_rt_mutex_destroy
#define 加互斥锁 cn_rt_mutex_lock
#define 尝试加互斥锁 cn_rt_mutex_trylock
#define 解互斥锁 cn_rt_mutex_unlock

// 自旋锁
#define 初始化自旋锁 cn_rt_spinlock_init
#define 销毁自旋锁 cn_rt_spinlock_destroy
#define 加自旋锁 cn_rt_spinlock_lock
#define 尝试加自旋锁 cn_rt_spinlock_trylock
#define 解自旋锁 cn_rt_spinlock_unlock

// 读写锁
#define 初始化读写锁 cn_rt_rwlock_init
#define 销毁读写锁 cn_rt_rwlock_destroy
#define 读锁定 cn_rt_rwlock_read_lock
#define 尝试读锁定 cn_rt_rwlock_try_read_lock
#define 读解锁 cn_rt_rwlock_read_unlock
#define 写锁定 cn_rt_rwlock_write_lock
#define 尝试写锁定 cn_rt_rwlock_try_write_lock
#define 写解锁 cn_rt_rwlock_write_unlock

// 辅助函数
#define 获取线程标识 cn_rt_get_thread_id
#define CPU暂停 cn_rt_cpu_pause

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_SYNC_H */
