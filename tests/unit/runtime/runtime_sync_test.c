#include <stdio.h>
#include <assert.h>
#include "cnlang/runtime/sync.h"

// 测试原子操作
void test_atomic32_basic() {
    printf("测试原子32位整数操作...\n");
    
    CnAtomic32 counter;
    cn_rt_atomic32_init(&counter, 0);
    
    // 测试加载和存储
    assert(cn_rt_atomic32_load(&counter, CN_MEMORY_ORDER_SEQ_CST) == 0);
    cn_rt_atomic32_store(&counter, 42, CN_MEMORY_ORDER_SEQ_CST);
    assert(cn_rt_atomic32_load(&counter, CN_MEMORY_ORDER_SEQ_CST) == 42);
    
    // 测试原子加法
    int32_t old_val = cn_rt_atomic32_fetch_add(&counter, 10, CN_MEMORY_ORDER_SEQ_CST);
    assert(old_val == 42);
    assert(cn_rt_atomic32_load(&counter, CN_MEMORY_ORDER_SEQ_CST) == 52);
    
    // 测试原子减法
    old_val = cn_rt_atomic32_fetch_sub(&counter, 12, CN_MEMORY_ORDER_SEQ_CST);
    assert(old_val == 52);
    assert(cn_rt_atomic32_load(&counter, CN_MEMORY_ORDER_SEQ_CST) == 40);
    
    // 测试原子交换
    old_val = cn_rt_atomic32_exchange(&counter, 100, CN_MEMORY_ORDER_SEQ_CST);
    assert(old_val == 40);
    assert(cn_rt_atomic32_load(&counter, CN_MEMORY_ORDER_SEQ_CST) == 100);
    
    printf("✓ 原子32位整数操作测试通过\n\n");
}

// 测试CAS操作
void test_atomic32_cas() {
    printf("测试原子CAS操作...\n");
    
    CnAtomic32 value;
    cn_rt_atomic32_init(&value, 10);
    
    // CAS成功
    int32_t expected = 10;
    int success = cn_rt_atomic32_compare_exchange(&value, &expected, 20, CN_MEMORY_ORDER_SEQ_CST);
    assert(success == 1);
    assert(cn_rt_atomic32_load(&value, CN_MEMORY_ORDER_SEQ_CST) == 20);
    
    // CAS失败
    expected = 10;  // 不匹配当前值20
    success = cn_rt_atomic32_compare_exchange(&value, &expected, 30, CN_MEMORY_ORDER_SEQ_CST);
    assert(success == 0);
    assert(expected == 20);  // expected被更新为当前值
    assert(cn_rt_atomic32_load(&value, CN_MEMORY_ORDER_SEQ_CST) == 20);
    
    printf("✓ 原子CAS操作测试通过\n\n");
}

// 测试位操作
void test_atomic32_bitops() {
    printf("测试原子位操作...\n");
    
    CnAtomic32 flags;
    cn_rt_atomic32_init(&flags, 0);
    
    // 测试原子OR
    int32_t old = cn_rt_atomic32_fetch_or(&flags, 0x01, CN_MEMORY_ORDER_SEQ_CST);
    assert(old == 0);
    assert(cn_rt_atomic32_load(&flags, CN_MEMORY_ORDER_SEQ_CST) == 0x01);
    
    old = cn_rt_atomic32_fetch_or(&flags, 0x04, CN_MEMORY_ORDER_SEQ_CST);
    assert(old == 0x01);
    assert(cn_rt_atomic32_load(&flags, CN_MEMORY_ORDER_SEQ_CST) == 0x05);
    
    // 测试原子AND
    old = cn_rt_atomic32_fetch_and(&flags, 0x05, CN_MEMORY_ORDER_SEQ_CST);
    assert(old == 0x05);
    assert(cn_rt_atomic32_load(&flags, CN_MEMORY_ORDER_SEQ_CST) == 0x05);
    
    // 测试原子XOR
    old = cn_rt_atomic32_fetch_xor(&flags, 0x01, CN_MEMORY_ORDER_SEQ_CST);
    assert(old == 0x05);
    assert(cn_rt_atomic32_load(&flags, CN_MEMORY_ORDER_SEQ_CST) == 0x04);
    
    printf("✓ 原子位操作测试通过\n\n");
}

// 测试互斥锁
void test_mutex() {
    printf("测试互斥锁...\n");
    
    CnMutex mutex;
    cn_rt_mutex_init(&mutex);
    
    // 测试基本加锁解锁
    cn_rt_mutex_lock(&mutex);
    cn_rt_mutex_unlock(&mutex);
    
    // 测试trylock
    int success = cn_rt_mutex_trylock(&mutex);
    assert(success == 1);
    cn_rt_mutex_unlock(&mutex);
    
    // 测试可重入
    cn_rt_mutex_lock(&mutex);
    cn_rt_mutex_lock(&mutex);  // 可重入
    cn_rt_mutex_unlock(&mutex);
    cn_rt_mutex_unlock(&mutex);
    
    cn_rt_mutex_destroy(&mutex);
    printf("✓ 互斥锁测试通过\n\n");
}

// 测试自旋锁
void test_spinlock() {
    printf("测试自旋锁...\n");
    
    CnSpinlock lock;
    cn_rt_spinlock_init(&lock);
    
    // 测试基本加锁解锁
    cn_rt_spinlock_lock(&lock);
    cn_rt_spinlock_unlock(&lock);
    
    // 测试trylock
    int success = cn_rt_spinlock_trylock(&lock);
    assert(success == 1);
    cn_rt_spinlock_unlock(&lock);
    
    cn_rt_spinlock_destroy(&lock);
    printf("✓ 自旋锁测试通过\n\n");
}

// 测试读写锁
void test_rwlock() {
    printf("测试读写锁...\n");
    
    CnRwLock lock;
    cn_rt_rwlock_init(&lock);
    
    // 测试读锁
    cn_rt_rwlock_read_lock(&lock);
    cn_rt_rwlock_read_unlock(&lock);
    
    // 测试多个读锁
    cn_rt_rwlock_read_lock(&lock);
    cn_rt_rwlock_read_lock(&lock);
    cn_rt_rwlock_read_unlock(&lock);
    cn_rt_rwlock_read_unlock(&lock);
    
    // 测试写锁
    cn_rt_rwlock_write_lock(&lock);
    cn_rt_rwlock_write_unlock(&lock);
    
    cn_rt_rwlock_destroy(&lock);
    printf("✓ 读写锁测试通过\n\n");
}

int main() {
    printf("===== CN语言运行时库 - 同步原语测试 =====\n\n");
    
    test_atomic32_basic();
    test_atomic32_cas();
    test_atomic32_bitops();
    test_mutex();
    test_spinlock();
    test_rwlock();
    
    printf("===== 所有测试通过! =====\n");
    return 0;
}
