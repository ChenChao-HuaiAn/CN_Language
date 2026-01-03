/******************************************************************************
 * 文件名：CN_platform_linux_special.c
 * 功能：Linux平台特有功能接口实现
 * 描述：基于Linux特有API实现的特殊功能接口，提供inotify文件监控、
 *       epoll事件多路复用、timerfd定时器、signalfd信号处理、
 *       eventfd事件通知、memfd匿名文件等功能。
 * 作者：CN_Language开发团队
 * 创建日期：2026年1月3日
 * 修改历史：
 *   [2026-01-03] 初始版本创建
 * 版权：MIT许可证
 ******************************************************************************/

#include "CN_platform_linux.h"
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

/* 内部数据结构定义 */

/**
 * @brief Linux inotify句柄包装结构
 */
typedef struct {
    int inotify_fd;               /**< inotify文件描述符 */
    int watch_count;              /**< 监控数量 */
} Stru_CN_LinuxInotifyHandle_t;

/**
 * @brief Linux epoll句柄包装结构
 */
typedef struct {
    int epoll_fd;                 /**< epoll文件描述符 */
    int max_events;               /**< 最大事件数 */
} Stru_CN_LinuxEpollHandle_t;

/**
 * @brief Linux timerfd句柄包装结构
 */
typedef struct {
    int timer_fd;                 /**< timerfd文件描述符 */
    bool is_armed;                /**< 是否已启动 */
} Stru_CN_LinuxTimerfdHandle_t;

/**
 * @brief Linux signalfd句柄包装结构
 */
typedef struct {
    int signal_fd;                /**< signalfd文件描述符 */
    sigset_t signal_mask;         /**< 信号掩码 */
} Stru_CN_LinuxSignalfdHandle_t;

/**
 * @brief Linux eventfd句柄包装结构
 */
typedef struct {
    int event_fd;                 /**< eventfd文件描述符 */
    uint64_t counter;             /**< 计数器 */
} Stru_CN_LinuxEventfdHandle_t;

/**
 * @brief Linux memfd句柄包装结构
 */
typedef struct {
    int memfd;                    /**< memfd文件描述符 */
    size_t size;                  /**< 内存大小 */
    void* mapping;                /**< 内存映射地址 */
} Stru_CN_LinuxMemfdHandle_t;

/* 静态函数声明 */

static bool linux_special_initialize(void);
static void linux_special_cleanup(void);
static Stru_CN_LinuxSpecialInterface_t* linux_special_get_interface(void);

/* inotify相关函数 */
static void* linux_inotify_create(void);
static int linux_inotify_add_watch(void* handle, const char* pathname, uint32_t mask);
static int linux_inotify_rm_watch(void* handle, int wd);
static int linux_inotify_read_events(void* handle, Stru_CN_InotifyEvent_t* events, int max_events);
static void linux_inotify_destroy(void* handle);

/* epoll相关函数 */
static void* linux_epoll_create(int size);
static int linux_epoll_ctl(void* handle, int op, int fd, Stru_CN_EpollEvent_t* event);
static int linux_epoll_wait(void* handle, Stru_CN_EpollEvent_t* events, int maxevents, int timeout);
static void linux_epoll_destroy(void* handle);

/* timerfd相关函数 */
static void* linux_timerfd_create(int clockid, int flags);
static int linux_timerfd_settime(void* handle, int flags, 
                                 const Stru_CN_ItimerSpec_t* new_value,
                                 Stru_CN_ItimerSpec_t* old_value);
static int linux_timerfd_gettime(void* handle, Stru_CN_ItimerSpec_t* curr_value);
static uint64_t linux_timerfd_read(void* handle);
static void linux_timerfd_destroy(void* handle);

/* signalfd相关函数 */
static void* linux_signalfd_create(const sigset_t* mask, int flags);
static int linux_signalfd_read(void* handle, Stru_CN_SignalfdSiginfo_t* info);
static int linux_signalfd_add_signals(void* handle, const sigset_t* mask);
static int linux_signalfd_remove_signals(void* handle, const sigset_t* mask);
static void linux_signalfd_destroy(void* handle);

/* eventfd相关函数 */
static void* linux_eventfd_create(unsigned int initval, int flags);
static int linux_eventfd_write(void* handle, uint64_t value);
static uint64_t linux_eventfd_read(void* handle);
static void linux_eventfd_destroy(void* handle);

/* memfd相关函数 */
static void* linux_memfd_create(const char* name, unsigned int flags);
static int linux_memfd_set_size(void* handle, off_t size);
static void* linux_memfd_map(void* handle, size_t length, int prot, int flags, off_t offset);
static int linux_memfd_unmap(void* handle, void* addr, size_t length);
static void linux_memfd_destroy(void* handle);

/* 全局特殊功能接口变量 */

static Stru_CN_LinuxSpecialInterface_t g_linux_special_interface = {
    .inotify_create = linux_inotify_create,
    .inotify_add_watch = linux_inotify_add_watch,
    .inotify_rm_watch = linux_inotify_rm_watch,
    .inotify_read_events = linux_inotify_read_events,
    .inotify_destroy = linux_inotify_destroy,
    
    .epoll_create = linux_epoll_create,
    .epoll_ctl = linux_epoll_ctl,
    .epoll_wait = linux_epoll_wait,
    .epoll_destroy = linux_epoll_destroy,
    
    .timerfd_create = linux_timerfd_create,
    .timerfd_settime = linux_timerfd_settime,
    .timerfd_gettime = linux_timerfd_gettime,
    .timerfd_read = linux_timerfd_read,
    .timerfd_destroy = linux_timerfd_destroy,
    
    .signalfd_create = linux_signalfd_create,
    .signalfd_read = linux_signalfd_read,
    .signalfd_add_signals = linux_signalfd_add_signals,
    .signalfd_remove_signals = linux_signalfd_remove_signals,
    .signalfd_destroy = linux_signalfd_destroy,
    
    .eventfd_create = linux_eventfd_create,
    .eventfd_write = linux_eventfd_write,
    .eventfd_read = linux_eventfd_read,
    .eventfd_destroy = linux_eventfd_destroy,
    
    .memfd_create = linux_memfd_create,
    .memfd_set_size = linux_memfd_set_size,
    .memfd_map = linux_memfd_map,
    .memfd_unmap = linux_memfd_unmap,
    .memfd_destroy = linux_memfd_destroy
};

/* 模块初始化状态 */

static bool g_special_module_initialized = false;

/******************************************************************************
 * 函数名：linux_special_initialize
 * 功能：初始化Linux特殊功能模块
 * 参数：无
 * 返回值：初始化成功返回true，失败返回false
 ******************************************************************************/
static bool linux_special_initialize(void)
{
    if (g_special_module_initialized) {
        return true;
    }
    
    // 检查Linux特有功能是否可用
    // 这里可以添加更详细的系统功能检测
    
    g_special_module_initialized = true;
    return true;
}

/******************************************************************************
 * 函数名：linux_special_cleanup
 * 功能：清理Linux特殊功能模块
 * 参数：无
 * 返回值：无
 ******************************************************************************/
static void linux_special_cleanup(void)
{
    if (!g_special_module_initialized) {
        return;
    }
    
    g_special_module_initialized = false;
}

/******************************************************************************
 * 函数名：linux_special_get_interface
 * 功能：获取Linux特殊功能接口
 * 参数：无
 * 返回值：特殊功能接口指针
 ******************************************************************************/
static Stru_CN_LinuxSpecialInterface_t* linux_special_get_interface(void)
{
    if (!g_special_module_initialized) {
        if (!linux_special_initialize()) {
            return NULL;
        }
    }
    
    return &g_linux_special_interface;
}

/******************************************************************************
 * inotify函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_inotify_create
 * 功能：创建inotify实例
 * 参数：无
 * 返回值：inotify句柄，失败返回NULL
 ******************************************************************************/
static void* linux_inotify_create(void)
{
    int inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotify_fd < 0) {
        return NULL;
    }
    
    Stru_CN_LinuxInotifyHandle_t* handle = 
        (Stru_CN_LinuxInotifyHandle_t*)malloc(sizeof(Stru_CN_LinuxInotifyHandle_t));
    if (!handle) {
        close(inotify_fd);
        return NULL;
    }
    
    handle->inotify_fd = inotify_fd;
    handle->watch_count = 0;
    
    return handle;
}

/******************************************************************************
 * 函数名：linux_inotify_add_watch
 * 功能：添加inotify监控
 * 参数：
 *   handle - inotify句柄
 *   pathname - 路径名
 *   mask - 监控掩码
 * 返回值：监控描述符，失败返回-1
 ******************************************************************************/
static int linux_inotify_add_watch(void* handle, const char* pathname, uint32_t mask)
{
    if (!handle || !pathname) {
        return -1;
    }
    
    Stru_CN_LinuxInotifyHandle_t* inotify_handle = (Stru_CN_LinuxInotifyHandle_t*)handle;
    int wd = inotify_add_watch(inotify_handle->inotify_fd, pathname, mask);
    
    if (wd >= 0) {
        inotify_handle->watch_count++;
    }
    
    return wd;
}

/******************************************************************************
 * 函数名：linux_inotify_rm_watch
 * 功能：移除inotify监控
 * 参数：
 *   handle - inotify句柄
 *   wd - 监控描述符
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_inotify_rm_watch(void* handle, int wd)
{
    if (!handle || wd < 0) {
        return -1;
    }
    
    Stru_CN_LinuxInotifyHandle_t* inotify_handle = (Stru_CN_LinuxInotifyHandle_t*)handle;
    int result = inotify_rm_watch(inotify_handle->inotify_fd, wd);
    
    if (result == 0) {
        inotify_handle->watch_count--;
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_inotify_read_events
 * 功能：读取inotify事件
 * 参数：
 *   handle - inotify句柄
 *   events - 事件数组
 *   max_events - 最大事件数
 * 返回值：实际读取的事件数，失败返回-1
 ******************************************************************************/
static int linux_inotify_read_events(void* handle, Stru_CN_InotifyEvent_t* events, int max_events)
{
    if (!handle || !events || max_events <= 0) {
        return -1;
    }
    
    Stru_CN_LinuxInotifyHandle_t* inotify_handle = (Stru_CN_LinuxInotifyHandle_t*)handle;
    
    char buffer[4096];
    ssize_t len = read(inotify_handle->inotify_fd, buffer, sizeof(buffer));
    
    if (len <= 0) {
        return 0;
    }
    
    int event_count = 0;
    char* ptr = buffer;
    
    while (ptr < buffer + len && event_count < max_events) {
        struct inotify_event* event = (struct inotify_event*)ptr;
        
        events[event_count].wd = event->wd;
        events[event_count].mask = event->mask;
        events[event_count].cookie = event->cookie;
        
        if (event->len > 0) {
            strncpy(events[event_count].name, event->name, sizeof(events[event_count].name) - 1);
            events[event_count].name[sizeof(events[event_count].name) - 1] = '\0';
        } else {
            events[event_count].name[0] = '\0';
        }
        
        event_count++;
        ptr += sizeof(struct inotify_event) + event->len;
    }
    
    return event_count;
}

/******************************************************************************
 * 函数名：linux_inotify_destroy
 * 功能：销毁inotify实例
 * 参数：
 *   handle - inotify句柄
 * 返回值：无
 ******************************************************************************/
static void linux_inotify_destroy(void* handle)
{
    if (!handle) {
        return;
    }
    
    Stru_CN_LinuxInotifyHandle_t* inotify_handle = (Stru_CN_LinuxInotifyHandle_t*)handle;
    
    close(inotify_handle->inotify_fd);
    free(inotify_handle);
}

/******************************************************************************
 * epoll函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_epoll_create
 * 功能：创建epoll实例
 * 参数：
 *   size - 监控的文件描述符数量（提示值）
 * 返回值：epoll句柄，失败返回NULL
 ******************************************************************************/
static void* linux_epoll_create(int size)
{
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        return NULL;
    }
    
    Stru_CN_LinuxEpollHandle_t* handle = 
        (Stru_CN_LinuxEpollHandle_t*)malloc(sizeof(Stru_CN_LinuxEpollHandle_t));
    if (!handle) {
        close(epoll_fd);
        return NULL;
    }
    
    handle->epoll_fd = epoll_fd;
    handle->max_events = size > 0 ? size : 64;
    
    return handle;
}

/******************************************************************************
 * 函数名：linux_epoll_ctl
 * 功能：控制epoll监控
 * 参数：
 *   handle - epoll句柄
 *   op - 操作（EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL）
 *   fd - 文件描述符
 *   event - 事件结构体指针
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_epoll_ctl(void* handle, int op, int fd, Stru_CN_EpollEvent_t* event)
{
    if (!handle || fd < 0) {
        return -1;
    }
    
    Stru_CN_LinuxEpollHandle_t* epoll_handle = (Stru_CN_LinuxEpollHandle_t*)handle;
    
    struct epoll_event ev;
    if (event) {
        ev.events = event->events;
        ev.data.u64 = event->data.u64;
    } else {
        memset(&ev, 0, sizeof(ev));
    }
    
    return epoll_ctl(epoll_handle->epoll_fd, op, fd, event ? &ev : NULL);
}

/******************************************************************************
 * 函数名：linux_epoll_wait
 * 功能：等待epoll事件
 * 参数：
 *   handle - epoll句柄
 *   events - 事件数组
 *   maxevents - 最大事件数
 *   timeout - 超时时间（毫秒）
 * 返回值：就绪的文件描述符数量，失败返回-1
 ******************************************************************************/
static int linux_epoll_wait(void* handle, Stru_CN_EpollEvent_t* events, int maxevents, int timeout)
{
    if (!handle || !events || maxevents <= 0) {
        return -1;
    }
    
    Stru_CN_LinuxEpollHandle_t* epoll_handle = (Stru_CN_LinuxEpollHandle_t*)handle;
    
    struct epoll_event* epoll_events = (struct epoll_event*)malloc(maxevents * sizeof(struct epoll_event));
    if (!epoll_events) {
        return -1;
    }
    
    int nfds = epoll_wait(epoll_handle->epoll_fd, epoll_events, maxevents, timeout);
    
    if (nfds > 0) {
        for (int i = 0; i < nfds; i++) {
            events[i].events = epoll_events[i].events;
            events[i].data.u64 = epoll_events[i].data.u64;
        }
    }
    
    free(epoll_events);
    return nfds;
}

/******************************************************************************
 * 函数名：linux_epoll_destroy
 * 功能：销毁epoll实例
 * 参数：
 *   handle - epoll句柄
 * 返回值：无
 ******************************************************************************/
static void linux_epoll_destroy(void* handle)
{
    if (!handle) {
        return;
    }
    
    Stru_CN_LinuxEpollHandle_t* epoll_handle = (Stru_CN_LinuxEpollHandle_t*)handle;
    
    close(epoll_handle->epoll_fd);
    free(epoll_handle);
}

/******************************************************************************
 * timerfd函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_timerfd_create
 * 功能：创建timerfd定时器
 * 参数：
 *   clockid - 时钟ID（CLOCK_REALTIME, CLOCK_MONOTONIC等）
 *   flags - 标志（TFD_NONBLOCK, TFD_CLOEXEC等）
 * 返回值：timerfd句柄，失败返回NULL
 ******************************************************************************/
static void* linux_timerfd_create(int clockid, int flags)
{
    int timer_fd = timerfd_create(clockid, flags);
    if (timer_fd < 0) {
        return NULL;
    }
    
    Stru_CN_LinuxTimerfdHandle_t* handle = 
        (Stru_CN_LinuxTimerfdHandle_t*)malloc(sizeof(Stru_CN_LinuxTimerfdHandle_t));
    if (!handle) {
        close(timer_fd);
        return NULL;
    }
    
    handle->timer_fd = timer_fd;
    handle->is_armed = false;
    
    return handle;
}

/******************************************************************************
 * 函数名：linux_timerfd_settime
 * 功能：设置timerfd定时器时间
 * 参数：
 *   handle - timerfd句柄
 *   flags - 标志（0或TFD_TIMER_ABSTIME）
 *   new_value - 新时间值
 *   old_value - 旧时间值（可选）
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_timerfd_settime(void* handle, int flags, 
                                 const Stru_CN_ItimerSpec_t* new_value,
                                 Stru_CN_ItimerSpec_t* old_value)
{
    if (!handle || !new_value) {
        return -1;
    }
    
    Stru_CN_LinuxTimerfdHandle_t* timer_handle = (Stru_CN_LinuxTimerfdHandle_t*)handle;
    
    struct itimerspec new_its, old_its;
    new_its.it_interval.tv_sec = new_value->it_interval.tv_sec;
    new_its.it_interval.tv_nsec = new_value->it_interval.tv_nsec;
    new_its.it_value.tv_sec = new_value->it_value.tv_sec;
    new_its.it_value.tv_nsec = new_value->it_value.tv_nsec;
    
    int result = timerfd_settime(timer_handle->timer_fd, flags, &new_its, old_value ? &old_its : NULL);
    
    if (result == 0) {
        timer_handle->is_armed = (new_its.it_value.tv_sec != 0 || new_its.it_value.tv_nsec != 0);
        
        if (old_value) {
            old_value->it_interval.tv_sec = old_its.it_interval.tv_sec;
            old_value->it_interval.tv_nsec = old_its.it_interval.tv_nsec;
            old_value->it_value.tv_sec = old_its.it_value.tv_sec;
            old_value->it_value.tv_nsec = old_its.it_value.tv_nsec;
        }
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_timerfd_gettime
 * 功能：获取timerfd定时器时间
 * 参数：
 *   handle - timerfd句柄
 *   curr_value - 当前时间值
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_timerfd_gettime(void* handle, Stru_CN_ItimerSpec_t* curr_value)
{
    if (!handle || !curr_value) {
        return -1;
    }
    
    Stru_CN_LinuxTimerfdHandle_t* timer_handle = (Stru_CN_LinuxTimerfdHandle_t*)handle;
    
    struct itimerspec curr_its;
    int result = timerfd_gettime(timer_handle->timer_fd, &curr_its);
    
    if (result == 0) {
        curr_value->it_interval.tv_sec = curr_its.it_interval.tv_sec;
        curr_value->it_interval.tv_nsec = curr_its.it_interval.tv_nsec;
        curr_value->it_value.tv_sec = curr_its.it_value.tv_sec;
        curr_value->it_value.tv_nsec = curr_its.it_value.tv_nsec;
    }
    
    return result;
}

/******************************************************************************
 * 函数名：linux_timerfd_read
 * 功能：读取timerfd定时器到期次数
 * 参数：
 *   handle - timerfd句柄
 * 返回值：到期次数
 ******************************************************************************/
static uint64_t linux_timerfd_read(void* handle)
{
    if (!handle) {
        return 0;
    }
    
    Stru_CN_LinuxTimerfdHandle_t* timer_handle = (Stru_CN_LinuxTimerfdHandle_t*)handle;
    
    uint64_t expirations;
    ssize_t s = read(timer_handle->timer_fd, &expirations, sizeof(expirations));
    
    if (s != sizeof(expirations)) {
        return 0;
    }
    
    return expirations;
}

/******************************************************************************
 * 函数名：linux_timerfd_destroy
 * 功能：销毁timerfd定时器
 * 参数：
 *   handle - timerfd句柄
 * 返回值：无
 ******************************************************************************/
static void linux_timerfd_destroy(void* handle)
{
    if (!handle) {
        return;
    }
    
    Stru_CN_LinuxTimerfdHandle_t* timer_handle = (Stru_CN_LinuxTimerfdHandle_t*)handle;
    
    close(timer_handle->timer_fd);
    free(timer_handle);
}

/******************************************************************************
 * signalfd函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_signalfd_create
 * 功能：创建signalfd信号处理器
 * 参数：
 *   mask - 信号掩码
 *   flags - 标志（SFD_NONBLOCK, SFD_CLOEXEC等）
 * 返回值：signalfd句柄，失败返回NULL
 ******************************************************************************/
static void* linux_signalfd_create(const sigset_t* mask, int flags)
{
    int signal_fd = signalfd(-1, mask, flags);
    if (signal_fd < 0) {
        return NULL;
    }
    
    Stru_CN_LinuxSignalfdHandle_t* handle = 
        (Stru_CN_LinuxSignalfdHandle_t*)malloc(sizeof(Stru_CN_LinuxSignalfdHandle_t));
    if (!handle) {
        close(signal_fd);
        return NULL;
    }
    
    handle->signal_fd = signal_fd;
    if (mask) {
        handle->signal_mask = *mask;
    } else {
        sigemptyset(&handle->signal_mask);
    }
    
    return handle;
}

/******************************************************************************
 * 函数名：linux_signalfd_read
 * 功能：读取signalfd信号信息
 * 参数：
 *   handle - signalfd句柄
 *   info - 信号信息结构体
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_signalfd_read(void* handle, Stru_CN_SignalfdSiginfo_t* info)
{
    if (!handle || !info) {
        return -1;
    }
    
    Stru_CN_LinuxSignalfdHandle_t* signal_handle = (Stru_CN_LinuxSignalfdHandle_t*)handle;
    
    struct signalfd_siginfo fdsi;
    ssize_t s = read(signal_handle->signal_fd, &fdsi, sizeof(fdsi));
    
    if (s != sizeof(fdsi)) {
        return -1;
    }
    
    info->ssi_signo = fdsi.ssi_signo;
    info->ssi_errno = fdsi.ssi_errno;
    info->ssi_code = fdsi.ssi_code;
    info->ssi_pid = fdsi.ssi_pid;
    info->ssi_uid = fdsi.ssi_uid;
    info->ssi_fd = fdsi.ssi_fd;
    info->ssi_tid = fdsi.ssi_tid;
    info->ssi_band = fdsi.ssi_band;
    info->ssi_overrun = fdsi.ssi_overrun;
    info->ssi_trapno = fdsi.ssi_trapno;
    info->ssi_status = fdsi.ssi_status;
    info->ssi_int = fdsi.ssi_int;
    info->ssi_ptr = fdsi.ssi_ptr;
    info->ssi_utime = fdsi.ssi_utime;
    info->ssi_stime = fdsi.ssi_stime;
    info->ssi_addr = fdsi.ssi_addr;
    
    return 0;
}

/******************************************************************************
 * 函数名：linux_signalfd_add_signals
 * 功能：向signalfd添加信号
 * 参数：
 *   handle - signalfd句柄
 *   mask - 要添加的信号掩码
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_signalfd_add_signals(void* handle, const sigset_t* mask)
{
    if (!handle || !mask) {
        return -1;
    }
    
    Stru_CN_LinuxSignalfdHandle_t* signal_handle = (Stru_CN_LinuxSignalfdHandle_t*)handle;
    
    sigset_t new_mask = signal_handle->signal_mask;
    for (int sig = 1; sig < NSIG; sig++) {
        if (sigismember(mask, sig)) {
            sigaddset(&new_mask, sig);
        }
    }
    
    int result = signalfd(signal_handle->signal_fd, &new_mask, 0);
    if (result >= 0) {
        signal_handle->signal_mask = new_mask;
    }
    
    return result >= 0 ? 0 : -1;
}

/******************************************************************************
 * 函数名：linux_signalfd_remove_signals
 * 功能：从signalfd移除信号
 * 参数：
 *   handle - signalfd句柄
 *   mask - 要移除的信号掩码
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_signalfd_remove_signals(void* handle, const sigset_t* mask)
{
    if (!handle || !mask) {
        return -1;
    }
    
    Stru_CN_LinuxSignalfdHandle_t* signal_handle = (Stru_CN_LinuxSignalfdHandle_t*)handle;
    
    sigset_t new_mask = signal_handle->signal_mask;
    for (int sig = 1; sig < NSIG; sig++) {
        if (sigismember(mask, sig)) {
            sigdelset(&new_mask, sig);
        }
    }
    
    int result = signalfd(signal_handle->signal_fd, &new_mask, 0);
    if (result >= 0) {
        signal_handle->signal_mask = new_mask;
    }
    
    return result >= 0 ? 0 : -1;
}

/******************************************************************************
 * 函数名：linux_signalfd_destroy
 * 功能：销毁signalfd信号处理器
 * 参数：
 *   handle - signalfd句柄
 * 返回值：无
 ******************************************************************************/
static void linux_signalfd_destroy(void* handle)
{
    if (!handle) {
        return;
    }
    
    Stru_CN_LinuxSignalfdHandle_t* signal_handle = (Stru_CN_LinuxSignalfdHandle_t*)handle;
    
    close(signal_handle->signal_fd);
    free(signal_handle);
}

/******************************************************************************
 * eventfd函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_eventfd_create
 * 功能：创建eventfd事件通知器
 * 参数：
 *   initval - 初始计数器值
 *   flags - 标志（EFD_NONBLOCK, EFD_CLOEXEC, EFD_SEMAPHORE等）
 * 返回值：eventfd句柄，失败返回NULL
 ******************************************************************************/
static void* linux_eventfd_create(unsigned int initval, int flags)
{
    int event_fd = eventfd(initval, flags);
    if (event_fd < 0) {
        return NULL;
    }
    
    Stru_CN_LinuxEventfdHandle_t* handle = 
        (Stru_CN_LinuxEventfdHandle_t*)malloc(sizeof(Stru_CN_LinuxEventfdHandle_t));
    if (!handle) {
        close(event_fd);
        return NULL;
    }
    
    handle->event_fd = event_fd;
    handle->counter = initval;
    
    return handle;
}

/******************************************************************************
 * 函数名：linux_eventfd_write
 * 功能：向eventfd写入值
 * 参数：
 *   handle - eventfd句柄
 *   value - 要写入的值
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_eventfd_write(void* handle, uint64_t value)
{
    if (!handle) {
        return -1;
    }
    
    Stru_CN_LinuxEventfdHandle_t* event_handle = (Stru_CN_LinuxEventfdHandle_t*)handle;
    
    ssize_t s = write(event_handle->event_fd, &value, sizeof(value));
    if (s == sizeof(value)) {
        event_handle->counter += value;
        return 0;
    }
    
    return -1;
}

/******************************************************************************
 * 函数名：linux_eventfd_read
 * 功能：从eventfd读取值
 * 参数：
 *   handle - eventfd句柄
 * 返回值：读取的值，失败返回0
 ******************************************************************************/
static uint64_t linux_eventfd_read(void* handle)
{
    if (!handle) {
        return 0;
    }
    
    Stru_CN_LinuxEventfdHandle_t* event_handle = (Stru_CN_LinuxEventfdHandle_t*)handle;
    
    uint64_t value;
    ssize_t s = read(event_handle->event_fd, &value, sizeof(value));
    
    if (s == sizeof(value)) {
        event_handle->counter -= value;
        return value;
    }
    
    return 0;
}

/******************************************************************************
 * 函数名：linux_eventfd_destroy
 * 功能：销毁eventfd事件通知器
 * 参数：
 *   handle - eventfd句柄
 * 返回值：无
 ******************************************************************************/
static void linux_eventfd_destroy(void* handle)
{
    if (!handle) {
        return;
    }
    
    Stru_CN_LinuxEventfdHandle_t* event_handle = (Stru_CN_LinuxEventfdHandle_t*)handle;
    
    close(event_handle->event_fd);
    free(event_handle);
}

/******************************************************************************
 * memfd函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_memfd_create
 * 功能：创建memfd匿名文件
 * 参数：
 *   name - 文件名（用于调试）
 *   flags - 标志（MFD_CLOEXEC, MFD_ALLOW_SEALING等）
 * 返回值：memfd句柄，失败返回NULL
 ******************************************************************************/
static void* linux_memfd_create(const char* name, unsigned int flags)
{
    int memfd = memfd_create(name, flags);
    if (memfd < 0) {
        return NULL;
    }
    
    Stru_CN_LinuxMemfdHandle_t* handle = 
        (Stru_CN_LinuxMemfdHandle_t*)malloc(sizeof(Stru_CN_LinuxMemfdHandle_t));
    if (!handle) {
        close(memfd);
        return NULL;
    }
    
    handle->memfd = memfd;
    handle->size = 0;
    handle->mapping = NULL;
    
    return handle;
}

/******************************************************************************
 * 函数名：linux_memfd_set_size
 * 功能：设置memfd文件大小
 * 参数：
 *   handle - memfd句柄
 *   size - 文件大小
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_memfd_set_size(void* handle, off_t size)
{
    if (!handle || size < 0) {
        return -1;
    }
    
    Stru_CN_LinuxMemfdHandle_t* memfd_handle = (Stru_CN_LinuxMemfdHandle_t*)handle;
    
    if (ftruncate(memfd_handle->memfd, size) == 0) {
        memfd_handle->size = size;
        return 0;
    }
    
    return -1;
}

/******************************************************************************
 * 函数名：linux_memfd_map
 * 功能：映射memfd到内存
 * 参数：
 *   handle - memfd句柄
 *   length - 映射长度
 *   prot - 保护标志（PROT_READ, PROT_WRITE等）
 *   flags - 映射标志（MAP_SHARED, MAP_PRIVATE等）
 *   offset - 文件偏移
 * 返回值：映射地址，失败返回NULL
 ******************************************************************************/
static void* linux_memfd_map(void* handle, size_t length, int prot, int flags, off_t offset)
{
    if (!handle) {
        return NULL;
    }
    
    Stru_CN_LinuxMemfdHandle_t* memfd_handle = (Stru_CN_LinuxMemfdHandle_t*)handle;
    
    void* mapping = mmap(NULL, length, prot, flags, memfd_handle->memfd, offset);
    if (mapping != MAP_FAILED) {
        memfd_handle->mapping = mapping;
        return mapping;
    }
    
    return NULL;
}

/******************************************************************************
 * 函数名：linux_memfd_unmap
 * 功能：取消memfd内存映射
 * 参数：
 *   handle - memfd句柄
 *   addr - 映射地址
 *   length - 映射长度
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_memfd_unmap(void* handle, void* addr, size_t length)
{
    if (!handle || !addr || length == 0) {
        return -1;
    }
    
    Stru_CN_LinuxMemfdHandle_t* memfd_handle = (Stru_CN_LinuxMemfdHandle_t*)handle;
    
    if (memfd_handle->mapping == addr) {
        memfd_handle->mapping = NULL;
    }
    
    return munmap(addr, length) == 0 ? 0 : -1;
}

/******************************************************************************
 * 函数名：linux_memfd_destroy
 * 功能：销毁memfd匿名文件
 * 参数：
 *   handle - memfd句柄
 * 返回值：无
 ******************************************************************************/
static void linux_memfd_destroy(void* handle)
{
    if (!handle) {
        return;
    }
    
    Stru_CN_LinuxMemfdHandle_t* memfd_handle = (Stru_CN_LinuxMemfdHandle_t*)handle;
    
    if (memfd_handle->mapping) {
        // 注意：这里不调用munmap，因为调用者应该自己管理映射
    }
    
    close(memfd_handle->memfd);
    free(memfd_handle);
}

/******************************************************************************
 * 外部接口函数
 ******************************************************************************/

/**
 * @brief 获取Linux特殊功能接口
 * 
 * @return Stru_CN_LinuxSpecialInterface_t* 特殊功能接口指针
 */
Stru_CN_LinuxSpecialInterface_t* CN_platform_linux_get_special(void)
{
    return linux_special_get_interface();
}

/**
 * @brief 初始化Linux特殊功能模块
 * 
 * @return bool 初始化成功返回true，失败返回false
 */
bool CN_platform_linux_special_initialize(void)
{
    return linux_special_initialize();
}

/**
 * @brief 清理Linux特殊功能模块
 */
void CN_platform_linux_special_cleanup(void)
{
    linux_special_cleanup();
}
