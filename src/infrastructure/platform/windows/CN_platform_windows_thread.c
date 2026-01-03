/******************************************************************************
 * 文件名: CN_platform_windows_thread.c
 * 功能: CN_Language Windows平台线程和同步子系统
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows线程和同步功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows.h"
#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief Windows线程句柄包装
 */
typedef struct Stru_CN_WindowsThread_t
{
    HANDLE handle;                  /**< Windows线程句柄 */
    DWORD id;                       /**< 线程ID */
    void* user_data;                /**< 用户数据 */
    CN_ThreadFunc func;             /**< 线程函数 */
    char name[CN_MAX_THREAD_NAME];  /**< 线程名称 */
} Stru_CN_WindowsThread_t;

/**
 * @brief Windows互斥锁包装
 */
typedef struct Stru_CN_WindowsMutex_t
{
    CRITICAL_SECTION cs;            /**< Windows临界区 */
    char name[CN_MAX_MUTEX_NAME];   /**< 互斥锁名称 */
} Stru_CN_WindowsMutex_t;

/**
 * @brief Windows信号量包装
 */
typedef struct Stru_CN_WindowsSemaphore_t
{
    HANDLE handle;                  /**< Windows信号量句柄 */
    char name[CN_MAX_SEMAPHORE_NAME]; /**< 信号量名称 */
} Stru_CN_WindowsSemaphore_t;

/**
 * @brief Windows条件变量包装
 */
typedef struct Stru_CN_WindowsCondition_t
{
    CONDITION_VARIABLE cv;          /**< Windows条件变量 */
    char name[CN_MAX_CONDITION_NAME]; /**< 条件变量名称 */
} Stru_CN_WindowsCondition_t;

/**
 * @brief Windows读写锁包装
 */
typedef struct Stru_CN_WindowsRWLock_t
{
    SRWLOCK lock;                   /**< Windows读写锁 */
    char name[CN_MAX_RWLOCK_NAME];  /**< 读写锁名称 */
} Stru_CN_WindowsRWLock_t;

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 线程入口函数
 */
static unsigned __stdcall thread_entry(void* arg)
{
    Stru_CN_WindowsThread_t* thread = (Stru_CN_WindowsThread_t*)arg;
    
    // 设置线程名称（如果支持）
    if (thread->name[0] != '\0')
    {
        // Windows 10及更高版本支持SetThreadDescription
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (kernel32)
        {
            typedef HRESULT (WINAPI *SetThreadDescriptionFunc)(HANDLE, PCWSTR);
            SetThreadDescriptionFunc set_thread_desc = 
                (SetThreadDescriptionFunc)GetProcAddress(kernel32, "SetThreadDescription");
            
            if (set_thread_desc)
            {
                wchar_t* wname = NULL;
                int wname_len = MultiByteToWideChar(CP_UTF8, 0, thread->name, -1, NULL, 0);
                if (wname_len > 0)
                {
                    wname = (wchar_t*)malloc(wname_len * sizeof(wchar_t));
                    if (wname)
                    {
                        MultiByteToWideChar(CP_UTF8, 0, thread->name, -1, wname, wname_len);
                        set_thread_desc(thread->handle, wname);
                        free(wname);
                    }
                }
            }
        }
    }
    
    // 调用用户线程函数
    if (thread->func)
    {
        thread->func(thread->user_data);
    }
    
    return 0;
}

// ============================================================================
// 线程接口实现
// ============================================================================

// 创建线程
CN_ThreadHandle_t windows_thread_create(CN_ThreadFunc func, void* user_data, const char* name)
{
    if (!func) return NULL;
    
    Stru_CN_WindowsThread_t* thread = 
        (Stru_CN_WindowsThread_t*)malloc(sizeof(Stru_CN_WindowsThread_t));
    if (!thread) return NULL;
    
    thread->func = func;
    thread->user_data = user_data;
    thread->handle = NULL;
    thread->id = 0;
    
    if (name)
    {
        strncpy(thread->name, name, sizeof(thread->name) - 1);
        thread->name[sizeof(thread->name) - 1] = '\0';
    }
    else
    {
        thread->name[0] = '\0';
    }
    
    // 创建线程
    thread->handle = (HANDLE)_beginthreadex(NULL, 0, thread_entry, thread, 0, 
                                           (unsigned int*)&thread->id);
    
    if (thread->handle == NULL)
    {
        free(thread);
        return NULL;
    }
    
    return (CN_ThreadHandle_t)thread;
}

// 等待线程结束
bool windows_thread_join(CN_ThreadHandle_t handle, int* exit_code)
{
    if (!handle) return false;
    
    Stru_CN_WindowsThread_t* thread = (Stru_CN_WindowsThread_t*)handle;
    
    DWORD result = WaitForSingleObject(thread->handle, INFINITE);
    if (result == WAIT_OBJECT_0)
    {
        if (exit_code)
        {
            DWORD exit_code_value = 0;
            GetExitCodeThread(thread->handle, &exit_code_value);
            *exit_code = (int)exit_code_value;
        }
        return true;
    }
    return false;
}

// 分离线程
bool windows_thread_detach(CN_ThreadHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsThread_t* thread = (Stru_CN_WindowsThread_t*)handle;
    
    // Windows线程创建时默认是可连接的，需要关闭句柄来分离
    BOOL result = CloseHandle(thread->handle);
    thread->handle = NULL; // 标记为已分离
    
    return result != FALSE;
}

// 获取线程ID
uint64_t windows_thread_get_id(CN_ThreadHandle_t handle)
{
    if (!handle) return 0;
    
    Stru_CN_WindowsThread_t* thread = (Stru_CN_WindowsThread_t*)handle;
    return (uint64_t)thread->id;
}

// 获取当前线程ID
uint64_t windows_thread_get_current_id(void)
{
    return (uint64_t)GetCurrentThreadId();
}

// 线程休眠
void windows_thread_sleep(uint32_t milliseconds)
{
    Sleep((DWORD)milliseconds);
}

// 线程让出CPU
void windows_thread_yield(void)
{
    SwitchToThread();
}

// 销毁线程句柄
void windows_thread_destroy(CN_ThreadHandle_t handle)
{
    if (!handle) return;
    
    Stru_CN_WindowsThread_t* thread = (Stru_CN_WindowsThread_t*)handle;
    
    if (thread->handle)
    {
        CloseHandle(thread->handle);
    }
    
    free(thread);
}

// ============================================================================
// 互斥锁接口实现
// ============================================================================

// 创建互斥锁
CN_MutexHandle_t windows_mutex_create(void)
{
    Stru_CN_WindowsMutex_t* mutex = 
        (Stru_CN_WindowsMutex_t*)malloc(sizeof(Stru_CN_WindowsMutex_t));
    if (!mutex) return NULL;
    
    InitializeCriticalSection(&mutex->cs);
    mutex->name[0] = '\0';
    
    return (CN_MutexHandle_t)mutex;
}

// 锁定互斥锁
bool windows_mutex_lock(CN_MutexHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsMutex_t* mutex = (Stru_CN_WindowsMutex_t*)handle;
    EnterCriticalSection(&mutex->cs);
    return true;
}

// 尝试锁定互斥锁
bool windows_mutex_try_lock(CN_MutexHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsMutex_t* mutex = (Stru_CN_WindowsMutex_t*)handle;
    return TryEnterCriticalSection(&mutex->cs) != FALSE;
}

// 解锁互斥锁
bool windows_mutex_unlock(CN_MutexHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsMutex_t* mutex = (Stru_CN_WindowsMutex_t*)handle;
    LeaveCriticalSection(&mutex->cs);
    return true;
}

// 销毁互斥锁
bool windows_mutex_destroy(CN_MutexHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsMutex_t* mutex = (Stru_CN_WindowsMutex_t*)handle;
    DeleteCriticalSection(&mutex->cs);
    free(mutex);
    return true;
}

// ============================================================================
// 信号量接口实现
// ============================================================================

// 创建信号量
CN_SemaphoreHandle_t windows_semaphore_create(int initial_count, int max_count)
{
    if (initial_count < 0 || max_count <= 0 || initial_count > max_count)
    {
        return NULL;
    }
    
    Stru_CN_WindowsSemaphore_t* semaphore = 
        (Stru_CN_WindowsSemaphore_t*)malloc(sizeof(Stru_CN_WindowsSemaphore_t));
    if (!semaphore) return NULL;
    
    semaphore->handle = CreateSemaphoreW(NULL, (LONG)initial_count, (LONG)max_count, NULL);
    if (!semaphore->handle)
    {
        free(semaphore);
        return NULL;
    }
    
    semaphore->name[0] = '\0';
    
    return (CN_SemaphoreHandle_t)semaphore;
}

// 等待信号量
bool windows_semaphore_wait(CN_SemaphoreHandle_t handle, uint32_t timeout_ms)
{
    if (!handle) return false;
    
    Stru_CN_WindowsSemaphore_t* semaphore = (Stru_CN_WindowsSemaphore_t*)handle;
    
    DWORD timeout = (timeout_ms == UINT32_MAX) ? INFINITE : (DWORD)timeout_ms;
    DWORD result = WaitForSingleObject(semaphore->handle, timeout);
    
    return result == WAIT_OBJECT_0;
}

// 释放信号量
bool windows_semaphore_release(CN_SemaphoreHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsSemaphore_t* semaphore = (Stru_CN_WindowsSemaphore_t*)handle;
    
    BOOL result = ReleaseSemaphore(semaphore->handle, 1, NULL);
    return result != FALSE;
}

// 获取信号量当前计数
int windows_semaphore_get_count(CN_SemaphoreHandle_t handle)
{
    if (!handle) return -1;
    
    Stru_CN_WindowsSemaphore_t* semaphore = (Stru_CN_WindowsSemaphore_t*)handle;
    
    // Windows没有直接获取信号量计数的API
    // 这里使用尝试等待然后立即释放的方式来检查
    DWORD result = WaitForSingleObject(semaphore->handle, 0);
    if (result == WAIT_OBJECT_0)
    {
        // 有可用计数，立即释放
        ReleaseSemaphore(semaphore->handle, 1, NULL);
        return 1; // 至少有一个可用
    }
    else if (result == WAIT_TIMEOUT)
    {
        return 0; // 没有可用计数
    }
    else
    {
        return -1; // 错误
    }
}

// 销毁信号量
bool windows_semaphore_destroy(CN_SemaphoreHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsSemaphore_t* semaphore = (Stru_CN_WindowsSemaphore_t*)handle;
    
    if (semaphore->handle)
    {
        CloseHandle(semaphore->handle);
    }
    
    free(semaphore);
    return true;
}

// ============================================================================
// 条件变量接口实现
// ============================================================================

// 创建条件变量
CN_ConditionHandle_t windows_condition_create(void)
{
    Stru_CN_WindowsCondition_t* condition = 
        (Stru_CN_WindowsCondition_t*)malloc(sizeof(Stru_CN_WindowsCondition_t));
    if (!condition) return NULL;
    
    InitializeConditionVariable(&condition->cv);
    condition->name[0] = '\0';
    
    return (CN_ConditionHandle_t)condition;
}

// 等待条件变量
bool windows_condition_wait(CN_ConditionHandle_t handle, CN_MutexHandle_t mutex_handle)
{
    if (!handle || !mutex_handle) return false;
    
    Stru_CN_WindowsCondition_t* condition = (Stru_CN_WindowsCondition_t*)handle;
    Stru_CN_WindowsMutex_t* mutex = (Stru_CN_WindowsMutex_t*)mutex_handle;
    
    BOOL result = SleepConditionVariableCS(&condition->cv, &mutex->cs, INFINITE);
    
    return result != FALSE;
}

// 通知一个等待线程
bool windows_condition_notify_one(CN_ConditionHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsCondition_t* condition = (Stru_CN_WindowsCondition_t*)handle;
    WakeConditionVariable(&condition->cv);
    return true;
}

// 通知所有等待线程
bool windows_condition_notify_all(CN_ConditionHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsCondition_t* condition = (Stru_CN_WindowsCondition_t*)handle;
    WakeAllConditionVariable(&condition->cv);
    return true;
}

// 销毁条件变量
bool windows_condition_destroy(CN_ConditionHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsCondition_t* condition = (Stru_CN_WindowsCondition_t*)handle;
    // Windows条件变量不需要显式销毁
    free(condition);
    return true;
}

// ============================================================================
// 读写锁接口实现
// ============================================================================

// 创建读写锁
CN_RWLockHandle_t windows_rwlock_create(const char* name)
{
    Stru_CN_WindowsRWLock_t* rwlock = 
        (Stru_CN_WindowsRWLock_t*)malloc(sizeof(Stru_CN_WindowsRWLock_t));
    if (!rwlock) return NULL;
    
    InitializeSRWLock(&rwlock->lock);
    
    if (name)
    {
        strncpy(rwlock->name, name, sizeof(rwlock->name) - 1);
        rwlock->name[sizeof(rwlock->name) - 1] = '\0';
    }
    else
    {
        rwlock->name[0] = '\0';
    }
    
    return (CN_RWLockHandle_t)rwlock;
}

// 获取读锁
bool windows_rwlock_lock_read(CN_RWLockHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsRWLock_t* rwlock = (Stru_CN_WindowsRWLock_t*)handle;
    AcquireSRWLockShared(&rwlock->lock);
    return true;
}

// 尝试获取读锁
bool windows_rwlock_try_lock_read(CN_RWLockHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsRWLock_t* rwlock = (Stru_CN_WindowsRWLock_t*)handle;
    return TryAcquireSRWLockShared(&rwlock->lock) != FALSE;
}

// 获取写锁
bool windows_rwlock_lock_write(CN_RWLockHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsRWLock_t* rwlock = (Stru_CN_WindowsRWLock_t*)handle;
    AcquireSRWLockExclusive(&rwlock->lock);
    return true;
}

// 尝试获取写锁
bool windows_rwlock_try_lock_write(CN_RWLockHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsRWLock_t* rwlock = (Stru_CN_WindowsRWLock_t*)handle;
    return TryAcquireSRWLockExclusive(&rwlock->lock) != FALSE;
}

// 释放读锁
bool windows_rwlock_unlock_read(CN_RWLockHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsRWLock_t* rwlock = (Stru_CN_WindowsRWLock_t*)handle;
    ReleaseSRWLockShared(&rwlock->lock);
    return true;
}

// 释放写锁
bool windows_rwlock_unlock_write(CN_RWLockHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsRWLock_t* rwlock = (Stru_CN_WindowsRWLock_t*)handle;
    ReleaseSRWLockExclusive(&rwlock->lock);
    return true;
}

// 销毁读写锁
void windows_rwlock_destroy(CN_RWLockHandle_t handle)
{
    if (!handle) return;
    
    Stru_CN_WindowsRWLock_t* rwlock = (Stru_CN_WindowsRWLock_t*)handle;
    // Windows SRW锁不需要显式销毁
    free(rwlock);
}

// ============================================================================
// 原子操作接口实现
// ============================================================================

// 原子整数加载
int32_t windows_atomic_load_i32(volatile int32_t* ptr)
{
    if (!ptr) return 0;
    return InterlockedCompareExchange((volatile LONG*)ptr, 0, 0);
}

// 原子整数存储
void windows_atomic_store_i32(volatile int32_t* ptr, int32_t value)
{
    if (!ptr) return;
    InterlockedExchange((volatile LONG*)ptr, (LONG)value);
}

// 原子整数加法
int32_t windows_atomic_add_i32(volatile int32_t* ptr, int32_t value)
{
    if (!ptr) return 0;
    return InterlockedExchangeAdd((volatile LONG*)ptr, (LONG)value);
}

// 原子整数减法
int32_t windows_atomic_sub_i32(volatile int32_t* ptr, int32_t value)
{
    if (!ptr) return 0;
    return InterlockedExchangeAdd((volatile LONG*)ptr, -(LONG)value);
}

// 原子整数与运算
int32_t windows_atomic_and_i32(volatile int32_t* ptr, int32_t value)
{
    if (!ptr) return 0;
    return InterlockedAnd((volatile LONG*)ptr, (LONG)value);
}

// 原子整数或运算
int32_t windows_atomic_or_i32(volatile int32_t* ptr, int32_t value)
{
    if (!ptr) return 0;
    return InterlockedOr((volatile LONG*)ptr, (LONG)value);
}

// 原子整数异或运算
int32_t windows_atomic_xor_i32(volatile int32_t* ptr, int32_t value)
{
    if (!ptr) return 0;
    return InterlockedXor((volatile LONG*)ptr, (LONG)value);
}

// 原子比较并交换
bool windows_atomic_compare_exchange_i32(volatile int32_t* ptr, int32_t* expected, int32_t desired)
{
    if (!ptr || !expected) return false;
    
    LONG old_value = (LONG)*expected;
    LONG new_value = (LONG)desired;
    
    LONG result = InterlockedCompareExchange((volatile LONG*)ptr, new_value, old_value);
    
    *expected = (int32_t)result;
    return result == old_value;
}

// 原子指针加载
void* windows_atomic_load_ptr(volatile void** ptr)
{
    if (!ptr) return NULL;
    
    // 使用InterlockedCompareExchangePointer
    return InterlockedCompareExchangePointer((volatile PVOID*)ptr, NULL, NULL);
}

// 原子指针存储
void windows_atomic_store_ptr(volatile void** ptr, void* value)
{
    if (!ptr) return;
    
    InterlockedExchangePointer((volatile PVOID*)ptr, value);
}

// 原子指针比较并交换
bool windows_atomic_compare_exchange_ptr(volatile void** ptr, void** expected, void* desired)
{
    if (!ptr || !expected) return false;
    
    PVOID old_value = *expected;
    PVOID new_value = desired;
    
    PVOID result = InterlockedCompareExchangePointer((volatile PVOID*)ptr, new_value, old_value);
    
    *expected = result;
    return result == old_value;
}

// 内存屏障
void windows_memory_barrier(void)
{
    MemoryBarrier();
}

// 获取处理器数量
int windows_get_processor_count(void)
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return (int)sys_info.dwNumberOfProcessors;
}

// 设置线程亲和性
bool windows_set_thread_affinity(CN_ThreadHandle_t handle, uint64_t affinity_mask)
{
    if (!handle) return false;
    
    Stru_CN_WindowsThread_t* thread = (Stru_CN_WindowsThread_t*)handle;
    
    DWORD_PTR result = SetThreadAffinityMask(thread->handle, (DWORD_PTR)affinity_mask);
    return result != 0;
}

// 设置线程优先级
bool windows_set_thread_priority(CN_ThreadHandle_t handle, Eum_CN_ThreadPriority_t priority)
{
    if (!handle) return false;
    
    Stru_CN_WindowsThread_t* thread = (Stru_CN_WindowsThread_t*)handle;
    
    int win_priority;
    switch (priority)
    {
        case Eum_THREAD_PRIORITY_LOWEST:
            win_priority = THREAD_PRIORITY_LOWEST;
            break;
        case Eum_THREAD_PRIORITY_BELOW_NORMAL:
            win_priority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case Eum_THREAD_PRIORITY_NORMAL:
            win_priority = THREAD_PRIORITY_NORMAL;
            break;
        case Eum_THREAD_PRIORITY_ABOVE_NORMAL:
            win_priority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case Eum_THREAD_PRIORITY_HIGHEST:
            win_priority = THREAD_PRIORITY_HIGHEST;
            break;
        case Eum_THREAD_PRIORITY_TIME_CRITICAL:
            win_priority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        default:
            win_priority = THREAD_PRIORITY_NORMAL;
            break;
    }
    
    BOOL result = SetThreadPriority(thread->handle, win_priority);
    return result != FALSE;
}

// ============================================================================
// 线程接口结构定义
// ============================================================================

/**
 * @brief Windows平台线程接口实现
 */
Stru_CN_ThreadInterface_t g_windows_thread_interface = {
    // 线程创建和管理
    .thread_create = windows_thread_create,
    .thread_join = windows_thread_join,
    .thread_detach = windows_thread_detach,
    .thread_sleep = windows_thread_sleep,
    .thread_yield = windows_thread_yield,
    
    // 线程本地存储
    .tls_allocate = NULL, // 暂未实现
    .tls_free = NULL, // 暂未实现
    .tls_set_value = NULL, // 暂未实现
    .tls_get_value = NULL, // 暂未实现
    
    // 互斥锁
    .mutex_create = windows_mutex_create,
    .mutex_destroy = windows_mutex_destroy,
    .mutex_lock = windows_mutex_lock,
    .mutex_try_lock = windows_mutex_try_lock,
    .mutex_unlock = windows_mutex_unlock,
    
    // 递归互斥锁
    .recursive_mutex_create = NULL, // 暂未实现
    .recursive_mutex_destroy = NULL, // 暂未实现
    .recursive_mutex_lock = NULL, // 暂未实现
    .recursive_mutex_try_lock = NULL, // 暂未实现
    .recursive_mutex_unlock = NULL, // 暂未实现
    
    // 信号量
    .semaphore_create = windows_semaphore_create,
    .semaphore_destroy = windows_semaphore_destroy,
    .semaphore_wait = windows_semaphore_wait,
    .semaphore_post = windows_semaphore_release,
    
    // 条件变量
    .condition_create = windows_condition_create,
    .condition_destroy = windows_condition_destroy,
    .condition_wait = windows_condition_wait,
    .condition_timed_wait = NULL, // 暂未实现
    .condition_signal = windows_condition_notify_one,
    .condition_broadcast = windows_condition_notify_all,
    
    // 原子操作
    .atomic_increment = NULL, // 暂未实现
    .atomic_decrement = NULL, // 暂未实现
    .atomic_add = windows_atomic_add_i32,
    .atomic_compare_exchange = NULL, // 暂未实现
    
    // 内存屏障
    .memory_barrier = windows_memory_barrier,
};

/**
 * @brief 获取Windows平台线程接口实现
 * 
 * @return Windows平台线程接口指针
 */
Stru_CN_ThreadInterface_t* CN_platform_windows_get_thread_impl(void)
{
    return &g_windows_thread_interface;
}
