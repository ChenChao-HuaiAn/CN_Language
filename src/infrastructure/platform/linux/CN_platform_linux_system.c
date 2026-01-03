/******************************************************************************
 * 文件名：CN_platform_linux_system.c
 * 功能：Linux平台系统接口实现
 * 描述：基于POSIX系统API实现的系统接口，提供系统信息获取、
 *       进程管理、环境变量操作等功能。
 * 作者：CN_Language开发团队
 * 创建日期：2026年1月3日
 * 修改历史：
 *   [2026-01-03] 初始版本创建
 * 版权：MIT许可证
 ******************************************************************************/

#include "CN_platform_linux.h"
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/* 内部数据结构定义 */

/**
 * @brief Linux进程句柄包装结构
 */
typedef struct {
    pid_t pid;                    /**< 进程ID */
    bool is_running;              /**< 是否在运行 */
    int exit_status;              /**< 退出状态 */
} Stru_CN_LinuxProcessHandle_t;

/* 静态函数声明 */

static bool linux_system_initialize(void);
static void linux_system_cleanup(void);
static Stru_CN_SystemInterface_t* linux_system_get_interface(void);

static int linux_get_system_info(Stru_CN_SystemInfo_t* system_info);
static int linux_get_memory_info(Stru_CN_MemoryInfo_t* memory_info);
static int linux_get_cpu_info(Stru_CN_CpuInfo_t* cpu_info);

static char* linux_get_environment_variable(const char* name);
static bool linux_set_environment_variable(const char* name, const char* value);
static bool linux_unset_environment_variable(const char* name);

static void* linux_process_create(const char* command, const char* const* argv, 
                                  const char* const* envp);
static bool linux_process_wait(void* process_handle, int* exit_status, int timeout_ms);
static bool linux_process_terminate(void* process_handle, int signal);
static bool linux_process_destroy(void* process_handle);

static int linux_get_current_process_id(void);
static int linux_get_parent_process_id(void);

static bool linux_signal_register(int signal, void (*handler)(int));
static bool linux_signal_send(int pid, int signal);

static int linux_get_resource_limit(int resource, Stru_CN_ResourceLimit_t* limit);
static int linux_set_resource_limit(int resource, const Stru_CN_ResourceLimit_t* limit);

/* 全局系统接口变量 */

static Stru_CN_SystemInterface_t g_linux_system_interface = {
    .get_system_info = linux_get_system_info,
    .get_memory_info = linux_get_memory_info,
    .get_cpu_info = linux_get_cpu_info,
    .get_environment_variable = linux_get_environment_variable,
    .set_environment_variable = linux_set_environment_variable,
    .unset_environment_variable = linux_unset_environment_variable,
    .process_create = linux_process_create,
    .process_wait = linux_process_wait,
    .process_terminate = linux_process_terminate,
    .process_destroy = linux_process_destroy,
    .get_current_process_id = linux_get_current_process_id,
    .get_parent_process_id = linux_get_parent_process_id,
    .signal_register = linux_signal_register,
    .signal_send = linux_signal_send,
    .get_resource_limit = linux_get_resource_limit,
    .set_resource_limit = linux_set_resource_limit
};

/* 模块初始化状态 */

static bool g_system_module_initialized = false;

/******************************************************************************
 * 函数名：linux_system_initialize
 * 功能：初始化Linux系统模块
 * 参数：无
 * 返回值：初始化成功返回true，失败返回false
 ******************************************************************************/
static bool linux_system_initialize(void)
{
    if (g_system_module_initialized) {
        return true;
    }
    
    // 检查系统功能是否可用
    // 这里可以添加更详细的系统功能检测
    
    g_system_module_initialized = true;
    return true;
}

/******************************************************************************
 * 函数名：linux_system_cleanup
 * 功能：清理Linux系统模块
 * 参数：无
 * 返回值：无
 ******************************************************************************/
static void linux_system_cleanup(void)
{
    if (!g_system_module_initialized) {
        return;
    }
    
    g_system_module_initialized = false;
}

/******************************************************************************
 * 函数名：linux_system_get_interface
 * 功能：获取Linux系统接口
 * 参数：无
 * 返回值：系统接口指针
 ******************************************************************************/
static Stru_CN_SystemInterface_t* linux_system_get_interface(void)
{
    if (!g_system_module_initialized) {
        if (!linux_system_initialize()) {
            return NULL;
        }
    }
    
    return &g_linux_system_interface;
}

/******************************************************************************
 * 系统信息函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_get_system_info
 * 功能：获取系统信息
 * 参数：
 *   system_info - 系统信息结构体指针
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_get_system_info(Stru_CN_SystemInfo_t* system_info)
{
    if (!system_info) {
        return -1;
    }
    
    struct utsname uts;
    if (uname(&uts) != 0) {
        return -1;
    }
    
    // 填充系统信息
    strncpy(system_info->sysname, uts.sysname, sizeof(system_info->sysname) - 1);
    strncpy(system_info->nodename, uts.nodename, sizeof(system_info->nodename) - 1);
    strncpy(system_info->release, uts.release, sizeof(system_info->release) - 1);
    strncpy(system_info->version, uts.version, sizeof(system_info->version) - 1);
    strncpy(system_info->machine, uts.machine, sizeof(system_info->machine) - 1);
    
    // 确保字符串以null结尾
    system_info->sysname[sizeof(system_info->sysname) - 1] = '\0';
    system_info->nodename[sizeof(system_info->nodename) - 1] = '\0';
    system_info->release[sizeof(system_info->release) - 1] = '\0';
    system_info->version[sizeof(system_info->version) - 1] = '\0';
    system_info->machine[sizeof(system_info->machine) - 1] = '\0';
    
    // 获取其他系统信息
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        system_info->uptime = si.uptime;
        system_info->total_ram = si.totalram;
        system_info->free_ram = si.freeram;
        system_info->shared_ram = si.sharedram;
        system_info->buffer_ram = si.bufferram;
        system_info->total_swap = si.totalswap;
        system_info->free_swap = si.freeswap;
        system_info->procs = si.procs;
    }
    
    return 0;
}

/******************************************************************************
 * 函数名：linux_get_memory_info
 * 功能：获取内存信息
 * 参数：
 *   memory_info - 内存信息结构体指针
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_get_memory_info(Stru_CN_MemoryInfo_t* memory_info)
{
    if (!memory_info) {
        return -1;
    }
    
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        return -1;
    }
    
    memory_info->total_physical = si.totalram;
    memory_info->free_physical = si.freeram;
    memory_info->used_physical = si.totalram - si.freeram;
    memory_info->total_swap = si.totalswap;
    memory_info->free_swap = si.freeswap;
    memory_info->used_swap = si.totalswap - si.freeswap;
    memory_info->shared_memory = si.sharedram;
    memory_info->buffer_memory = si.bufferram;
    
    // 计算内存使用率
    if (memory_info->total_physical > 0) {
        memory_info->physical_usage_percent = 
            (double)memory_info->used_physical * 100.0 / (double)memory_info->total_physical;
    } else {
        memory_info->physical_usage_percent = 0.0;
    }
    
    if (memory_info->total_swap > 0) {
        memory_info->swap_usage_percent = 
            (double)memory_info->used_swap * 100.0 / (double)memory_info->total_swap;
    } else {
        memory_info->swap_usage_percent = 0.0;
    }
    
    return 0;
}

/******************************************************************************
 * 函数名：linux_get_cpu_info
 * 功能：获取CPU信息
 * 参数：
 *   cpu_info - CPU信息结构体指针
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_get_cpu_info(Stru_CN_CpuInfo_t* cpu_info)
{
    if (!cpu_info) {
        return -1;
    }
    
    // 从/proc/cpuinfo读取CPU信息
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        return -1;
    }
    
    memset(cpu_info, 0, sizeof(Stru_CN_CpuInfo_t));
    
    char line[256];
    int processor_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "processor", 9) == 0) {
            processor_count++;
        } else if (strncmp(line, "model name", 10) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                colon++; // 跳过冒号
                while (*colon == ' ' || *colon == '\t') colon++;
                strncpy(cpu_info->model_name, colon, sizeof(cpu_info->model_name) - 1);
                cpu_info->model_name[sizeof(cpu_info->model_name) - 1] = '\0';
                
                // 移除换行符
                char* newline = strchr(cpu_info->model_name, '\n');
                if (newline) *newline = '\0';
            }
        } else if (strncmp(line, "cpu MHz", 7) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                sscanf(colon + 1, "%f", &cpu_info->clock_speed_mhz);
            }
        } else if (strncmp(line, "cache size", 10) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                sscanf(colon + 1, "%d", &cpu_info->cache_size_kb);
            }
        } else if (strncmp(line, "cpu cores", 9) == 0) {
            char* colon = strchr(line, ':');
            if (colon) {
                sscanf(colon + 1, "%d", &cpu_info->cores_per_cpu);
            }
        }
    }
    
    fclose(fp);
    
    cpu_info->processor_count = processor_count;
    cpu_info->total_cores = processor_count * cpu_info->cores_per_cpu;
    
    // 从/proc/stat读取CPU使用率
    fp = fopen("/proc/stat", "r");
    if (fp) {
        unsigned long long user, nice, system, idle, iowait, irq, softirq;
        
        if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq) == 7) {
            unsigned long long total = user + nice + system + idle + iowait + irq + softirq;
            unsigned long long used = total - idle;
            
            if (total > 0) {
                cpu_info->usage_percent = (double)used * 100.0 / (double)total;
            }
        }
        
        fclose(fp);
    }
    
    return 0;
}

/******************************************************************************
 * 环境变量函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_get_environment_variable
 * 功能：获取环境变量
 * 参数：
 *   name - 环境变量名
 * 返回值：环境变量值，不存在返回NULL
 ******************************************************************************/
static char* linux_get_environment_variable(const char* name)
{
    if (!name) {
        return NULL;
    }
    
    return getenv(name);
}

/******************************************************************************
 * 函数名：linux_set_environment_variable
 * 功能：设置环境变量
 * 参数：
 *   name - 环境变量名
 *   value - 环境变量值
 * 返回值：成功返回true，失败返回false
 ******************************************************************************/
static bool linux_set_environment_variable(const char* name, const char* value)
{
    if (!name) {
        return false;
    }
    
    if (value) {
        return setenv(name, value, 1) == 0;
    } else {
        return unsetenv(name) == 0;
    }
}

/******************************************************************************
 * 函数名：linux_unset_environment_variable
 * 功能：删除环境变量
 * 参数：
 *   name - 环境变量名
 * 返回值：成功返回true，失败返回false
 ******************************************************************************/
static bool linux_unset_environment_variable(const char* name)
{
    if (!name) {
        return false;
    }
    
    return unsetenv(name) == 0;
}

/******************************************************************************
 * 进程管理函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_process_create
 * 功能：创建进程
 * 参数：
 *   command - 命令路径
 *   argv - 参数数组
 *   envp - 环境变量数组
 * 返回值：进程句柄，失败返回NULL
 ******************************************************************************/
static void* linux_process_create(const char* command, const char* const* argv, 
                                  const char* const* envp)
{
    if (!command) {
        return NULL;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        return NULL;
    }
    
    if (pid == 0) {
        // 子进程
        if (envp) {
            execve(command, (char* const*)argv, (char* const*)envp);
        } else {
            execv(command, (char* const*)argv);
        }
        
        // 如果exec失败，子进程退出
        _exit(127);
    }
    
    // 父进程
    Stru_CN_LinuxProcessHandle_t* handle = 
        (Stru_CN_LinuxProcessHandle_t*)malloc(sizeof(Stru_CN_LinuxProcessHandle_t));
    if (!handle) {
        return NULL;
    }
    
    handle->pid = pid;
    handle->is_running = true;
    handle->exit_status = 0;
    
    return handle;
}

/******************************************************************************
 * 函数名：linux_process_wait
 * 功能：等待进程结束
 * 参数：
 *   process_handle - 进程句柄
 *   exit_status - 退出状态指针
 *   timeout_ms - 超时时间（毫秒）
 * 返回值：成功返回true，失败返回false
 ******************************************************************************/
static bool linux_process_wait(void* process_handle, int* exit_status, int timeout_ms)
{
    if (!process_handle) {
        return false;
    }
    
    Stru_CN_LinuxProcessHandle_t* handle = (Stru_CN_LinuxProcessHandle_t*)process_handle;
    
    if (!handle->is_running) {
        if (exit_status) {
            *exit_status = handle->exit_status;
        }
        return true;
    }
    
    int status;
    pid_t result;
    
    if (timeout_ms <= 0) {
        // 无限等待
        result = waitpid(handle->pid, &status, 0);
    } else {
        // 使用非阻塞等待和超时
        // 简化实现：使用非阻塞等待
        result = waitpid(handle->pid, &status, WNOHANG);
        if (result == 0) {
            // 进程还在运行
            return false;
        }
    }
    
    if (result == handle->pid) {
        handle->is_running = false;
        if (WIFEXITED(status)) {
            handle->exit_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            handle->exit_status = WTERMSIG(status);
        }
        
        if (exit_status) {
            *exit_status = handle->exit_status;
        }
        return true;
    }
    
    return false;
}

/******************************************************************************
 * 函数名：linux_process_terminate
 * 功能：终止进程
 * 参数：
 *   process_handle - 进程句柄
 *   signal - 信号
 * 返回值：成功返回true，失败返回false
 ******************************************************************************/
static bool linux_process_terminate(void* process_handle, int signal)
{
    if (!process_handle) {
        return false;
    }
    
    Stru_CN_LinuxProcessHandle_t* handle = (Stru_CN_LinuxProcessHandle_t*)process_handle;
    
    if (!handle->is_running) {
        return true;
    }
    
    if (kill(handle->pid, signal) == 0) {
        return true;
    }
    
    return false;
}

/******************************************************************************
 * 函数名：linux_process_destroy
 * 功能：销毁进程句柄
 * 参数：
 *   process_handle - 进程句柄
 * 返回值：成功返回true，失败返回false
 ******************************************************************************/
static bool linux_process_destroy(void* process_handle)
{
    if (!process_handle) {
        return false;
    }
    
    Stru_CN_LinuxProcessHandle_t* handle = (Stru_CN_LinuxProcessHandle_t*)process_handle;
    
    if (handle->is_running) {
        // 如果进程还在运行，先终止它
        linux_process_terminate(handle, SIGTERM);
        
        // 等待进程结束
        int status;
        linux_process_wait(handle, &status, 1000);
    }
    
    free(handle);
    return true;
}

/******************************************************************************
 * 进程ID函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_get_current_process_id
 * 功能：获取当前进程ID
 * 参数：无
 * 返回值：当前进程ID
 ******************************************************************************/
static int linux_get_current_process_id(void)
{
    return getpid();
}

/******************************************************************************
 * 函数名：linux_get_parent_process_id
 * 功能：获取父进程ID
 * 参数：无
 * 返回值：父进程ID
 ******************************************************************************/
static int linux_get_parent_process_id(void)
{
    return getppid();
}

/******************************************************************************
 * 信号处理函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_signal_register
 * 功能：注册信号处理函数
 * 参数：
 *   signal - 信号
 *   handler - 处理函数
 * 返回值：成功返回true，失败返回false
 ******************************************************************************/
static bool linux_signal_register(int signal, void (*handler)(int))
{
    if (!handler) {
        return false;
    }
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    return sigaction(signal, &sa, NULL) == 0;
}

/******************************************************************************
 * 函数名：linux_signal_send
 * 功能：发送信号
 * 参数：
 *   pid - 进程ID
 *   signal - 信号
 * 返回值：成功返回true，失败返回false
 ******************************************************************************/
static bool linux_signal_send(int pid, int signal)
{
    return kill(pid, signal) == 0;
}

/******************************************************************************
 * 资源限制函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_get_resource_limit
 * 功能：获取资源限制
 * 参数：
 *   resource - 资源类型
 *   limit - 资源限制结构体指针
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_get_resource_limit(int resource, Stru_CN_ResourceLimit_t* limit)
{
    if (!limit) {
        return -1;
    }
    
    struct rlimit rlim;
    if (getrlimit(resource, &rlim) != 0) {
        return -1;
    }
    
    limit->soft_limit = rlim.rlim_cur;
    limit->hard_limit = rlim.rlim_max;
    return 0;
}

/******************************************************************************
 * 函数名：linux_set_resource_limit
 * 功能：设置资源限制
 * 参数：
 *   resource - 资源类型
 *   limit - 资源限制结构体指针
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_set_resource_limit(int resource, const Stru_CN_ResourceLimit_t* limit)
{
    if (!limit) {
        return -1;
    }
    
    struct rlimit rlim;
    rlim.rlim_cur = limit->soft_limit;
    rlim.rlim_max = limit->hard_limit;
    
    return setrlimit(resource, &rlim);
}

/******************************************************************************
 * 外部接口函数
 ******************************************************************************/

/**
 * @brief 获取Linux系统接口
 * 
 * @return Stru_CN_SystemInterface_t* 系统接口指针
 */
Stru_CN_SystemInterface_t* CN_platform_linux_get_system(void)
{
    return linux_system_get_interface();
}

/**
 * @brief 初始化Linux系统模块
 * 
 * @return bool 初始化成功返回true，失败返回false
 */
bool CN_platform_linux_system_initialize(void)
{
    return linux_system_initialize();
}

/**
 * @brief 清理Linux系统模块
 */
void CN_platform_linux_system_cleanup(void)
{
    linux_system_cleanup();
}
