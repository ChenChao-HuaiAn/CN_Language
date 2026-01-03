/******************************************************************************
 * 文件名：CN_platform_linux_thread.c
 * 功能：Linux平台线程接口实现
 * 描述：基于POSIX线程（pthread）API实现的线程接口，提供线程创建、同步、
 *       互斥锁、条件变量等功能。
 * 作者：CN_Language开发团队
 * 创建日期：2026年1月3日
 * 修改历史：
 *   [2026-01-03] 初始版本创建
 * 版权：MIT许可证
 ******************************************************************************/

#include "CN_platform_linux.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <time.h>

/* 内部数据结构定义 */

/**
 * @brief Linux线程句柄包装结构
 */
typedef struct {
    pthread_t thread_id;          /**< POSIX线程ID */
    bool is_detached;             /**< 是否已分离 */
    bool is_canceled;             /**< 是否已取消 */
} Stru_CN_LinuxThreadHandle_t;

/**
 * @brief Linux互斥锁包装结构
 */
typedef struct {
    pthread_mutex_t mutex;        /**< POSIX互斥锁 */
    bool is_initialized;          /**< 是否已初始化 */
} Stru_CN_LinuxMutexHandle_t;

/**
 * @brief Linux条件变量包装结构
 */
typedef struct {
    pthread_cond_t cond;          /**< POSIX条件变量 */
    bool is_initialized;          /**< 是否已初始化 */
} Stru_CN_LinuxCondHandle_t;

/* 静态函数声明 */

static bool linux_thread_initialize(void);
static void linux_thread_cleanup(void);
static Stru_CN_ThreadInterface_t* linux_thread_get_interface(void);

static int linux_thread_create(CN_ThreadHandle_t* thread, const void* attr,
                               void* (*start_routine)(void*), void* arg);
static int linux_thread_join(CN_ThreadHandle_t thread, void** retval);
static int linux_thread_detach(CN_ThreadHandle_t thread);
static int linux_thread_cancel(CN_ThreadHandle_t thread);
static CN_ThreadHandle_t linux_thread_self(void);
static void linux_thread_exit(void* retval);

static int linux_mutex_init(CN_MutexHandle_t* mutex, const void* attr);
static int linux_mutex_destroy(CN_MutexHandle_t* mutex);
static int linux_mutex_lock(CN_MutexHandle_t* mutex);
static int linux_mutex_trylock(CN_MutexHandle_t* mutex);
static int linux_mutex_unlock(CN_MutexHandle_t* mutex);

static int linux_cond_init(CN_CondHandle_t* cond, const void* attr);
static int linux_cond_destroy(CN_CondHandle_t* cond);
static int linux_cond_wait(CN_CondHandle_t* cond, CN_MutexHandle_t* mutex);
static int linux_cond_timedwait(CN_CondHandle_t* cond, CN_MutexHandle_t* mutex,
                                const Stru_CN_TimeSpec_t* abstime);
static int linux_cond_signal(CN_CondHandle_t* cond);
static int linux_cond_broadcast(CN_CondHandle_t* cond);

/* 全局线程接口变量 */

static Stru_CN_ThreadInterface_t g_linux_thread_interface = {
    .thread_create = linux_thread_create,
    .thread_join = linux_thread_join,
    .thread_detach = linux_thread_detach,
    .thread_cancel = linux_thread_cancel,
    .thread_self = linux_thread_self,
    .thread_exit = linux_thread_exit,
    .mutex_init = linux_mutex_init,
    .mutex_destroy = linux_mutex_destroy,
    .mutex_lock = linux_mutex_lock,
    .mutex_trylock = linux_mutex_trylock,
    .mutex_unlock = linux_mutex_unlock,
    .cond_init = linux_cond_init,
    .cond_destroy = linux_cond_destroy,
    .cond_wait = linux_cond_wait,
    .cond_timedwait = linux_cond_timedwait,
    .cond_signal = linux_cond_signal,
    .cond_broadcast = linux_cond_broadcast
};

/* 模块初始化状态 */

static bool g_thread_module_initialized = false;

/******************************************************************************
 * 函数名：linux_thread_initialize
 * 功能：初始化Linux线程模块
 * 参数：无
 * 返回值：初始化成功返回true，失败返回false
 ******************************************************************************/
static bool linux_thread_initialize(void)
{
    if (g_thread_module_initialized) {
        return true;
    }
    
    // 检查pthread库是否可用
    // 这里可以添加更详细的系统功能检测
    
    g_thread_module_initialized = true;
    return true;
}

/******************************************************************************
 * 函数名：linux_thread_cleanup
 * 功能：清理Linux线程模块
 * 参数：无
 * 返回值：无
 ******************************************************************************/
static void linux_thread_cleanup(void)
{
    if (!g_thread_module_initialized) {
        return;
    }
    
    g_thread_module_initialized = false;
}

/******************************************************************************
 * 函数名：linux_thread_get_interface
 * 功能：获取Linux线程接口
 * 参数：无
 * 返回值：线程接口指针
 ******************************************************************************/
static Stru_CN_ThreadInterface_t* linux_thread_get_interface(void)
{
    if (!g_thread_module_initialized) {
        if (!linux_thread_initialize()) {
            return NULL;
        }
    }
    
    return &g_linux_thread_interface;
}

/******************************************************************************
 * 线程创建和管理函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_thread_create
 * 功能：创建新线程
 * 参数：
 *   thread - 线程句柄指针
 *   attr - 线程属性指针（可为NULL）
 *   start_routine - 线程入口函数
 *   arg - 线程参数
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_thread_create(CN_ThreadHandle_t* thread, const void* attr,
                               void* (*start_routine)(void*), void* arg)
{
    if (!thread || !start_routine) {
        return EINVAL;
    }
    
    // 分配线程句柄内存
    Stru_CN_LinuxThreadHandle_t* linux_thread = 
        (Stru_CN_LinuxThreadHandle_t*)cn_malloc(sizeof(Stru_CN_LinuxThreadHandle_t));
    if (!linux_thread) {
        return ENOMEM;
    }
    
    // 初始化线程句柄
    memset(linux_thread, 0, sizeof(Stru_CN_LinuxThreadHandle_t));
    
    // 创建线程
    int result = pthread_create(&linux_thread->thread_id, 
                                (const pthread_attr_t*)attr,
                                start_routine, arg);
    
    if (result != 0) {
        cn_free(linux_thread);
        return result;
    }
    
    // 设置线程句柄
    *thread = (CN_ThreadHandle_t)linux_thread;
    return 0;
}

/******************************************************************************
 * 函数名：linux_thread_join
 * 功能：等待线程结束
 * 参数：
 *   thread - 线程句柄
 *   retval - 线程返回值指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_thread_join(CN_ThreadHandle_t thread, void** retval)
{
    if (!thread) {
        return EINVAL;
    }
    
    Stru_CN_LinuxThreadHandle_t* linux_thread = 
        (Stru_CN_LinuxThreadHandle_t*)thread;
    
    // 检查线程是否已分离
    if (linux_thread->is_detached) {
        return EINVAL;  // 不能等待已分离的线程
    }
    
    // 等待线程结束
    int result = pthread_join(linux_thread->thread_id, retval);
    
    if (result == 0) {
        // 释放线程句柄内存
        cn_free(linux_thread);
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_thread_detach
 * 功能：分离线程
 * 参数：
 *   thread - 线程句柄
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_thread_detach(CN_ThreadHandle_t thread)
{
    if (!thread) {
        return EINVAL;
    }
    
    Stru_CN_LinuxThreadHandle_t* linux_thread = 
        (Stru_CN_LinuxThreadHandle_t*)thread;
    
    // 分离线程
    int result = pthread_detach(linux_thread->thread_id);
    
    if (result == 0) {
        linux_thread->is_detached = true;
        // 分离后不能join，但需要在线程退出时释放内存
        // 这里可以设置回调或使用其他机制
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_thread_cancel
 * 功能：取消线程
 * 参数：
 *   thread - 线程句柄
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_thread_cancel(CN_ThreadHandle_t thread)
{
    if (!thread) {
        return EINVAL;
    }
    
    Stru_CN_LinuxThreadHandle_t* linux_thread = 
        (Stru_CN_LinuxThreadHandle_t*)thread;
    
    // 取消线程
    int result = pthread_cancel(linux_thread->thread_id);
    
    if (result == 0) {
        linux_thread->is_canceled = true;
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_thread_self
 * 功能：获取当前线程ID
 * 参数：无
 * 返回值：当前线程ID
 ******************************************************************************/
static CN_ThreadHandle_t linux_thread_self(void)
{
    pthread_t self_id = pthread_self();
    
    // 查找或创建线程句柄
    // 注意：这里简化实现，实际可能需要线程本地存储
    
    // 创建临时线程句柄
    Stru_CN_LinuxThreadHandle_t* linux_thread = 
        (Stru_CN_LinuxThreadHandle_t*)cn_malloc(sizeof(Stru_CN_LinuxThreadHandle_t));
    if (!linux_thread) {
        return NULL;
    }
    
    memset(linux_thread, 0, sizeof(Stru_CN_LinuxThreadHandle_t));
    linux_thread->thread_id = self_id;
    
    return (CN_ThreadHandle_t)linux_thread;
}

/******************************************************************************
 * 函数名：linux_thread_exit
 * 功能：退出当前线程
 * 参数：
 *   retval - 退出值
 * 返回值：无
 ******************************************************************************/
static void linux_thread_exit(void* retval)
{
    pthread_exit(retval);
}

/******************************************************************************
 * 互斥锁函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_mutex_init
 * 功能：初始化互斥锁
 * 参数：
 *   mutex - 互斥锁指针
 *   attr - 互斥锁属性指针（可为NULL）
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_mutex_init(CN_MutexHandle_t* mutex, const void* attr)
{
    if (!mutex) {
        return EINVAL;
    }
    
    // 分配互斥锁句柄内存
    Stru_CN_LinuxMutexHandle_t* linux_mutex = 
        (Stru_CN_LinuxMutexHandle_t*)cn_malloc(sizeof(Stru_CN_LinuxMutexHandle_t));
    if (!linux_mutex) {
        return ENOMEM;
    }
    
    // 初始化互斥锁
    memset(linux_mutex, 0, sizeof(Stru_CN_LinuxMutexHandle_t));
    
    int result = pthread_mutex_init(&linux_mutex->mutex, 
                                    (const pthread_mutexattr_t*)attr);
    
    if (result != 0) {
        cn_free(linux_mutex);
        return result;
    }
    
    linux_mutex->is_initialized = true;
    *mutex = (CN_MutexHandle_t)linux_mutex;
    return 0;
}

/******************************************************************************
 * 函数名：linux_mutex_destroy
 * 功能：销毁互斥锁
 * 参数：
 *   mutex - 互斥锁指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_mutex_destroy(CN_MutexHandle_t* mutex)
{
    if (!mutex || !*mutex) {
        return EINVAL;
    }
    
    Stru_CN_LinuxMutexHandle_t* linux_mutex = 
        (Stru_CN_LinuxMutexHandle_t*)*mutex;
    
    if (!linux_mutex->is_initialized) {
        cn_free(linux_mutex);
        *mutex = NULL;
        return 0;
    }
    
    // 销毁互斥锁
    int result = pthread_mutex_destroy(&linux_mutex->mutex);
    
    if (result == 0) {
        linux_mutex->is_initialized = false;
        cn_free(linux_mutex);
        *mutex = NULL;
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_mutex_lock
 * 功能：锁定互斥锁
 * 参数：
 *   mutex - 互斥锁指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_mutex_lock(CN_MutexHandle_t* mutex)
{
    if (!mutex || !*mutex) {
        return EINVAL;
    }
    
    Stru_CN_LinuxMutexHandle_t* linux_mutex = 
        (Stru_CN_LinuxMutexHandle_t*)*mutex;
    
    if (!linux_mutex->is_initialized) {
        return EINVAL;
    }
    
    return pthread_mutex_lock(&linux_mutex->mutex);
}

/******************************************************************************
 * 函数名：linux_mutex_trylock
 * 功能：尝试锁定互斥锁
 * 参数：
 *   mutex - 互斥锁指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_mutex_trylock(CN_MutexHandle_t* mutex)
{
    if (!mutex || !*mutex) {
        return EINVAL;
    }
    
    Stru_CN_LinuxMutexHandle_t* linux_mutex = 
        (Stru_CN_LinuxMutexHandle_t*)*mutex;
    
    if (!linux_mutex->is_initialized) {
        return EINVAL;
    }
    
    return pthread_mutex_trylock(&linux_mutex->mutex);
}

/******************************************************************************
 * 函数名：linux_mutex_unlock
 * 功能：解锁互斥锁
 * 参数：
 *   mutex - 互斥锁指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_mutex_unlock(CN_MutexHandle_t* mutex)
{
    if (!mutex || !*mutex) {
        return EINVAL;
    }
    
    Stru_CN_LinuxMutexHandle_t* linux_mutex = 
        (Stru_CN_LinuxMutexHandle_t*)*mutex;
    
    if (!linux_mutex->is_initialized) {
        return EINVAL;
    }
    
    return pthread_mutex_unlock(&linux_mutex->mutex);
}

/******************************************************************************
 * 条件变量函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_cond_init
 * 功能：初始化条件变量
 * 参数：
 *   cond - 条件变量指针
 *   attr - 条件变量属性指针（可为NULL）
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_cond_init(CN_CondHandle_t* cond, const void* attr)
{
    if (!cond) {
        return EINVAL;
    }
    
    // 分配条件变量句柄内存
    Stru_CN_LinuxCondHandle_t* linux_cond = 
        (Stru_CN_LinuxCondHandle_t*)cn_malloc(sizeof(Stru_CN_LinuxCondHandle_t));
    if (!linux_cond) {
        return ENOMEM;
    }
    
    // 初始化条件变量
    memset(linux_cond, 0, sizeof(Stru_CN_LinuxCondHandle_t));
    
    int result = pthread_cond_init(&linux_cond->cond, 
                                   (const pthread_condattr_t*)attr);
    
    if (result != 0) {
        cn_free(linux_cond);
        return result;
    }
    
    linux_cond->is_initialized = true;
    *cond = (CN_CondHandle_t)linux_cond;
    return 0;
}

/******************************************************************************
 * 函数名：linux_cond_destroy
 * 功能：销毁条件变量
 * 参数：
 *   cond - 条件变量指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_cond_destroy(CN_CondHandle_t* cond)
{
    if (!cond || !*cond) {
        return EINVAL;
    }
    
    Stru_CN_LinuxCondHandle_t* linux_cond = 
        (Stru_CN_LinuxCondHandle_t*)*cond;
    
    if (!linux_cond->is_initialized) {
        cn_free(linux_cond);
        *cond = NULL;
        return 0;
    }
    
    // 销毁条件变量
    int result = pthread_cond_destroy(&linux_cond->cond);
    
    if (result == 0) {
        linux_cond->is_initialized = false;
        cn_free(linux_cond);
        *cond = NULL;
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_cond_wait
 * 功能：等待条件变量
 * 参数：
 *   cond - 条件变量指针
 *   mutex - 互斥锁指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_cond_wait(CN_CondHandle_t* cond, CN_MutexHandle_t* mutex)
{
    if (!cond || !*cond || !mutex || !*mutex) {
        return EINVAL;
    }
    
    Stru_CN_LinuxCondHandle_t* linux_cond = 
        (Stru_CN_LinuxCondHandle_t*)*cond;
    Stru_CN_LinuxMutexHandle_t* linux_mutex = 
        (Stru_CN_LinuxMutexHandle_t*)*mutex;
    
    if (!linux_cond->is_initialized || !linux_mutex->is_initialized) {
        return EINVAL;
    }
    
    return pthread_cond_wait(&linux_cond->cond, &linux_mutex->mutex);
}

/******************************************************************************
 * 函数名：linux_cond_timedwait
 * 功能：定时等待条件变量
 * 参数：
 *   cond - 条件变量指针
 *   mutex - 互斥锁指针
 *   abstime - 绝对时间
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_cond_timedwait(CN_CondHandle_t* cond, CN_MutexHandle_t* mutex,
                                const Stru_CN_TimeSpec_t* abstime)
{
    if (!cond || !*cond || !mutex || !*mutex || !abstime) {
        return EINVAL;
    }
    
    Stru_CN_LinuxCondHandle_t* linux_cond = 
        (Stru_CN_LinuxCondHandle_t*)*cond;
    Stru_CN_LinuxMutexHandle_t* linux_mutex = 
        (Stru_CN_LinuxMutexHandle_t*)*mutex;
    
    if (!linux_cond->is_initialized || !linux_mutex->is_initialized) {
        return EINVAL;
    }
    
    // 转换时间结构体
    struct timespec ts;
    ts.tv_sec = abstime->tv_sec;
    ts.tv_nsec = abstime->tv_nsec;
    
    return pthread_cond_timedwait(&linux_cond->cond, &linux_mutex->mutex, &ts);
}

/******************************************************************************
 * 函数名：linux_cond_signal
 * 功能：通知条件变量
 * 参数：
 *   cond - 条件变量指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_cond_signal(CN_CondHandle_t* cond)
{
    if (!cond || !*cond) {
        return EINVAL;
    }
    
    Stru_CN_LinuxCondHandle_t* linux_cond = 
        (Stru_CN_LinuxCondHandle_t*)*cond;
    
    if (!linux_cond->is_initialized) {
        return EINVAL;
    }
    
    return pthread_cond_signal(&linux_cond->cond);
}

/******************************************************************************
 * 函数名：linux_cond_broadcast
 * 功能：广播条件变量
 * 参数：
 *   cond - 条件变量指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_cond_broadcast(CN_CondHandle_t* cond)
{
    if (!cond || !*cond) {
        return EINVAL;
    }
    
    Stru_CN_LinuxCondHandle_t* linux_cond = 
        (Stru_CN_LinuxCondHandle_t*)*cond;
    
    if (!linux_cond->is_initialized) {
        return EINVAL;
    }
    
    return pthread_cond_broadcast(&linux_cond->cond);
}

/******************************************************************************
 * 函数名：CN_platform_linux_get_thread
 * 功能：获取Linux线程接口（外部接口）
 * 参数：无
 * 返回值：线程接口指针
 ******************************************************************************/
Stru_CN_ThreadInterface_t* CN_platform_linux_get_thread(void)
{
    return linux_thread_get_interface();
}

/******************************************************************************
 * 函数名：CN_platform_linux_thread_initialize
 * 功能：初始化Linux线程模块（外部接口）
 * 参数：无
 * 返回值：初始化成功返回true，失败返回false
 ******************************************************************************/
bool CN_platform_linux_thread_initialize(void)
{
    return linux_thread_initialize();
}

/******************************************************************************
 * 函数名：CN_platform_linux_thread_cleanup
 * 功能：清理Linux线程模块（外部接口）
 * 参数：无
 * 返回值：无
 ******************************************************************************/
void CN_platform_linux_thread_cleanup(void)
{
    linux_thread_cleanup();
}
