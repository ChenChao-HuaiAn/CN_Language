# CN_Language Linux平台API文档

## 文档概述

本文档详细描述了CN_Language项目Linux平台实现模块的API接口。Linux平台模块提供了基于POSIX API的完整平台抽象层实现，包括文件系统、线程、网络、时间、系统信息等核心功能，以及Linux特有的inotify、epoll、文件描述符工具等功能。

## API分类

### 1. 核心平台接口

#### 1.1 平台初始化和管理

```c
/**
 * @brief 初始化Linux平台实现
 * 
 * 初始化Linux平台所需的所有资源，包括检测系统功能等。
 * 必须在调用任何其他Linux平台函数之前调用此函数。
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_linux_initialize(void);

/**
 * @brief 清理Linux平台实现
 * 
 * 释放Linux平台使用的所有资源。
 * 在程序退出前调用此函数。
 */
void CN_platform_linux_cleanup(void);

/**
 * @brief 获取完整的Linux平台接口
 * 
 * 返回包含所有平台接口的完整平台接口结构体。
 * 
 * @return Linux平台接口指针，失败返回NULL
 */
Stru_CN_PlatformInterface_t* CN_platform_linux_get_interface(void);
```

#### 1.2 各功能模块接口获取

```c
/**
 * @brief 获取Linux平台文件系统接口
 * 
 * @return Linux平台文件系统接口指针
 */
Stru_CN_FileSystemInterface_t* CN_platform_linux_get_filesystem(void);

/**
 * @brief 获取Linux平台线程接口
 * 
 * @return Linux平台线程接口指针
 */
Stru_CN_ThreadInterface_t* CN_platform_linux_get_thread(void);

/**
 * @brief 获取Linux平台网络接口
 * 
 * @return Linux平台网络接口指针
 */
Stru_CN_NetworkInterface_t* CN_platform_linux_get_network(void);

/**
 * @brief 获取Linux平台时间接口
 * 
 * @return Linux平台时间接口指针
 */
Stru_CN_TimeInterface_t* CN_platform_linux_get_time(void);

/**
 * @brief 获取Linux平台系统接口
 * 
 * @return Linux平台系统接口指针
 */
Stru_CN_SystemInterface_t* CN_platform_linux_get_system(void);
```

#### 1.3 Linux特有功能接口获取

```c
/**
 * @brief 获取Linux inotify接口
 * 
 * @return Linux inotify接口指针
 */
Stru_CN_LinuxInotifyInterface_t* CN_platform_linux_get_inotify(void);

/**
 * @brief 获取Linux epoll接口
 * 
 * @return Linux epoll接口指针
 */
Stru_CN_LinuxEpollInterface_t* CN_platform_linux_get_epoll(void);

/**
 * @brief 获取Linux文件描述符工具接口
 * 
 * @return Linux文件描述符工具接口指针
 */
Stru_CN_LinuxFDInterface_t* CN_platform_linux_get_fd_tools(void);

/**
 * @brief 获取Linux系统调用接口
 * 
 * @return Linux系统调用接口指针
 */
Stru_CN_LinuxSyscallInterface_t* CN_platform_linux_get_syscall(void);
```

#### 1.4 系统功能检测

```c
/**
 * @brief 检查Linux特定功能是否可用
 * 
 * 检测当前Linux系统是否支持特定功能，如inotify、epoll等。
 * 
 * @param feature 功能标志（CN_LINUX_FEATURE_*）
 * @return 如果功能可用返回true，否则返回false
 */
bool CN_platform_linux_check_feature(uint32_t feature);

/**
 * @brief 获取Linux内核版本
 * 
 * 获取当前运行Linux内核的版本信息。
 * 
 * @param major 主版本号输出参数
 * @param minor 次版本号输出参数
 * @param patch 修订号输出参数
 * @return 如果成功获取返回true，否则返回false
 */
bool CN_platform_linux_get_kernel_version(int* major, int* minor, int* patch);

/**
 * @brief 获取Linux发行版信息
 * 
 * 获取当前Linux发行版的名称和版本信息。
 * 
 * @param name 发行版名称缓冲区
 * @param name_size 名称缓冲区大小
 * @param version 版本号缓冲区
 * @param version_size 版本号缓冲区大小
 * @return 如果成功获取返回true，否则返回false
 */
bool CN_platform_linux_get_distro_info(char* name, size_t name_size, 
                                       char* version, size_t version_size);
```

### 2. 文件系统接口 (`Stru_CN_FileSystemInterface_t`)

Linux平台文件系统接口基于POSIX文件API实现，提供完整的文件系统操作功能。

#### 2.1 文件操作

```c
/**
 * @brief 检查文件是否存在
 * 
 * 使用stat系统调用检查文件是否存在。
 * 
 * @param path 文件路径（UTF-8编码）
 * @return 文件存在返回true，否则返回false
 */
bool (*file_exists)(const char* path);

/**
 * @brief 打开文件
 * 
 * 使用open系统调用打开文件，支持多种打开模式。
 * 
 * @param path 文件路径（UTF-8编码）
 * @param mode 打开模式
 * @return 文件句柄，失败返回NULL
 */
CN_FileHandle_t (*file_open)(const char* path, Eum_CN_FileMode_t mode);

/**
 * @brief 关闭文件
 * 
 * 使用close系统调用关闭文件。
 * 
 * @param handle 文件句柄
 * @return 关闭成功返回true，失败返回false
 */
bool (*file_close)(CN_FileHandle_t handle);

/**
 * @brief 读取文件
 * 
 * 使用read系统调用读取文件数据。
 * 
 * @param handle 文件句柄
 * @param buffer 读取缓冲区
 * @param size 要读取的字节数
 * @return 实际读取的字节数
 */
size_t (*file_read)(CN_FileHandle_t handle, void* buffer, size_t size);

/**
 * @brief 写入文件
 * 
 * 使用write系统调用写入文件数据。
 * 
 * @param handle 文件句柄
 * @param buffer 写入缓冲区
 * @param size 要写入的字节数
 * @return 实际写入的字节数
 */
size_t (*file_write)(CN_FileHandle_t handle, const void* buffer, size_t size);
```

#### 2.2 文件定位

```c
/**
 * @brief 文件定位
 * 
 * 使用lseek系统调用定位文件位置。
 * 
 * @param handle 文件句柄
 * @param offset 偏移量
 * @param whence 起始位置（SEEK_SET, SEEK_CUR, SEEK_END）
 * @return 定位成功返回true，失败返回false
 */
bool (*file_seek)(CN_FileHandle_t handle, int64_t offset, int whence);

/**
 * @brief 获取文件当前位置
 * 
 * 使用lseek系统调用获取文件当前位置。
 * 
 * @param handle 文件句柄
 * @return 当前位置，失败返回-1
 */
int64_t (*file_tell)(CN_FileHandle_t handle);
```

#### 2.3 文件属性操作

```c
/**
 * @brief 获取文件信息
 * 
 * 使用stat系统调用获取文件详细信息。
 * 
 * @param path 文件路径
 * @param info 文件信息结构体指针
 * @return 获取成功返回true，失败返回false
 */
bool (*file_get_info)(const char* path, Stru_CN_FileInfo_t* info);

/**
 * @brief 设置文件属性
 * 
 * 使用chmod系统调用设置文件权限。
 * 
 * @param path 文件路径
 * @param attributes 文件属性（Eum_CN_FileAttribute_t的位组合）
 * @return 设置成功返回true，失败返回false
 */
bool (*file_set_attributes)(const char* path, uint32_t attributes);
```

#### 2.4 文件操作

```c
/**
 * @brief 删除文件
 * 
 * 使用unlink系统调用删除文件。
 * 
 * @param path 文件路径
 * @return 删除成功返回true，失败返回false
 */
bool (*file_delete)(const char* path);

/**
 * @brief 重命名文件
 * 
 * 使用rename系统调用重命名文件。
 * 
 * @param old_path 原文件路径
 * @param new_path 新文件路径
 * @return 重命名成功返回true，失败返回false
 */
bool (*file_rename)(const char* old_path, const char* new_path);

/**
 * @brief 复制文件
 * 
 * 使用read/write系统调用复制文件。
 * 
 * @param src_path 源文件路径
 * @param dst_path 目标文件路径
 * @return 复制成功返回true，失败返回false
 */
bool (*file_copy)(const char* src_path, const char* dst_path);
```

#### 2.5 目录操作

```c
/**
 * @brief 创建目录
 * 
 * 使用mkdir系统调用创建目录。
 * 
 * @param path 目录路径
 * @return 创建成功返回true，失败返回false
 */
bool (*directory_create)(const char* path);

/**
 * @brief 删除目录
 * 
 * 使用rmdir系统调用删除空目录。
 * 
 * @param path 目录路径
 * @return 删除成功返回true，失败返回false
 */
bool (*directory_delete)(const char* path);

/**
 * @brief 检查目录是否存在
 * 
 * 使用stat系统调用检查目录是否存在。
 * 
 * @param path 目录路径
 * @return 目录存在返回true，否则返回false
 */
bool (*directory_exists)(const char* path);
```

#### 2.6 目录遍历

```c
/**
 * @brief 打开目录
 * 
 * 使用opendir系统调用打开目录。
 * 
 * @param path 目录路径
 * @return 目录句柄，失败返回NULL
 */
void* (*directory_open)(const char* path);

/**
 * @brief 读取目录项
 * 
 * 使用readdir系统调用读取目录项。
 * 
 * @param dir_handle 目录句柄
 * @param info 文件信息结构体指针
 * @return 读取成功返回true，没有更多项返回false
 */
bool (*directory_read)(void* dir_handle, Stru_CN_FileInfo_t* info);

/**
 * @brief 关闭目录
 * 
 * 使用closedir系统调用关闭目录。
 * 
 * @param dir_handle 目录句柄
 * @return 关闭成功返回true，失败返回false
 */
bool (*directory_close)(void* dir_handle);
```

#### 2.7 路径操作

```c
/**
 * @brief 检查路径是否为绝对路径
 * 
 * 检查路径是否以'/'开头。
 * 
 * @param path 路径字符串
 * @return 是绝对路径返回true，否则返回false
 */
bool (*path_is_absolute)(const char* path);

/**
 * @brief 检查路径是否为相对路径
 * 
 * 检查路径是否不以'/'开头。
 * 
 * @param path 路径字符串
 * @return 是相对路径返回true，否则返回false
 */
bool (*path_is_relative)(const char* path);

/**
 * @brief 组合路径
 * 
 * 智能组合两个路径，正确处理路径分隔符。
 * 
 * @param path1 第一个路径
 * @param path2 第二个路径
 * @return 组合后的路径字符串，需要调用者释放
 */
char* (*path_combine)(const char* path1, const char* path2);

/**
 * @brief 获取目录部分
 * 
 * 从完整路径中提取目录部分。
 * 
 * @param path 完整路径
 * @return 目录部分字符串，需要调用者释放
 */
char* (*path_get_directory)(const char* path);

/**
 * @brief 获取文件名部分
 * 
 * 从完整路径中提取文件名部分。
 * 
 * @param path 完整路径
 * @return 文件名部分字符串，需要调用者释放
 */
char* (*path_get_filename)(const char* path);

/**
 * @brief 获取文件扩展名
 * 
 * 从完整路径中提取文件扩展名。
 * 
 * @param path 完整路径
 * @return 扩展名字符串（包含点），需要调用者释放
 */
char* (*path_get_extension)(const char* path);
```

#### 2.8 特殊路径

```c
/**
 * @brief 获取当前工作目录
 * 
 * 使用getcwd系统调用获取当前工作目录。
 * 
 * @return 当前目录字符串，需要调用者释放
 */
char* (*get_current_directory)(void);

/**
 * @brief 设置当前工作目录
 * 
 * 使用chdir系统调用设置当前工作目录。
 * 
 * @param path 目录路径
 * @return 设置成功返回true，失败返回false
 */
bool (*set_current_directory)(const char* path);

/**
 * @brief 获取用户主目录
 * 
 * 从HOME环境变量获取用户主目录。
 * 
 * @return 用户主目录字符串，需要调用者释放
 */
char* (*get_home_directory)(void);

/**
 * @brief 获取临时目录
 * 
 * 从TMPDIR环境变量获取临时目录，默认为/tmp。
 * 
 * @return 临时目录字符串，需要调用者释放
 */
char* (*get_temp_directory)(void);
```

#### 2.9 磁盘信息

```c
/**
 * @brief 获取磁盘可用空间
 * 
 * 使用statvfs系统调用获取磁盘可用空间。
 * 
 * @param path 磁盘路径
 * @return 可用空间字节数
 */
uint64_t (*get_disk_free_space)(const char* path);

/**
 * @brief 获取磁盘总空间
 * 
 * 使用statvfs系统调用获取磁盘总空间。
 * 
 * @param path 磁盘路径
 * @return 总空间字节数
 */
uint64_t (*get_disk_total_space)(const char* path);
```

### 3. Linux特有功能接口

#### 3.1 inotify文件监控接口 (`Stru_CN_LinuxInotifyInterface_t`)

```c
/**
 * @brief 初始化inotify实例
 * 
 * 使用inotify_init系统调用初始化inotify。
 * 
 * @return inotify文件描述符，失败返回-1
 */
int (*inotify_init)(void);

/**
 * @brief 初始化inotify实例（带标志）
 * 
 * 使用inotify_init1系统调用初始化inotify。
 * 
 * @param flags 初始化标志（IN_CLOEXEC等）
 * @return inotify文件描述符，失败返回-1
 */
int (*inotify_init1)(int flags);

/**
 * @brief 关闭inotify实例
 * 
 * 关闭inotify文件描述符。
 * 
 * @param fd inotify文件描述符
 */
void (*inotify_close)(int fd);

/**
 * @brief 添加文件监视
 * 
 * 使用inotify_add_watch系统调用添加文件监视。
 * 
 * @param fd inotify文件描述符
 * @param pathname 要监视的路径
 * @param mask 事件掩码
 * @return 监视描述符，失败返回-1
 */
int (*inotify_add_watch)(int fd, const char* pathname, uint32_t mask);

/**
 * @brief 移除文件监视
 * 
 * 使用inotify_rm_watch系统调用移除文件监视。
 * 
 * @param fd inotify文件描述符
 * @param wd 监视描述符
 * @return 成功返回0，失败返回-1
 */
int (*inotify_rm_watch)(int fd, int wd);

/**
 * @brief 读取inotify事件
 * 
 * 读取inotify事件到提供的缓冲区。
 * 
 * @param fd inotify文件描述符
 * @param events 事件缓冲区
 * @param max_events 最大事件数
 * @return 读取的事件数，失败返回-1
 */
ssize_t (*inotify_read_events)(int fd, Stru_CN_LinuxInotifyEvent_t* events, 
                               size_t max_events);
```

#### 3.2 epoll事件通知接口 (`Stru_CN_LinuxEpollInterface_t`)

```c
/**
 * @brief 创建epoll实例
 * 
 * 使用epoll_create系统调用创建epoll实例。
 * 
 * @param size epoll实例大小（Linux 2.6.8后忽略）
 * @return epoll文件描述符，失败返回-1
 */
int (*epoll_create)(int size);

/**
 * @brief 创建epoll实例（带标志）
 * 
 * 使用epoll_create1系统调用创建epoll实例。
 * 
 * @param flags 创建标志（EPOLL_CLOEXEC等）
 * @return epoll文件描述符，失败返回-1
 */
int (*epoll_create1)(int flags);

/**
 * @brief 关闭epoll实例
 * 
 * 关闭epoll文件描述符。
 * 
 * @param epfd epoll文件描述符
 */
void (*epoll_close)(int epfd);

/**
 * @brief 控制epoll事件
 * 
 * 使用epoll_ctl系统调用控制epoll事件。
 * 
 * @param epfd epoll文件描述符
 * @param op 操作（EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL）
 * @param fd 要控制的文件描述符
 * @param event 事件结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*epoll_ctl)(int epfd, int op, int fd, Stru_CN_LinuxEpollEvent_t* event);

/**
 * @brief 等待epoll事件
 * 
 * 使用epoll_wait系统调用等待epoll事件。
 * 
 * @param epfd epoll文件描述符
 * @param events 事件缓冲区
 * @param maxevents 最大事件数
 * @param timeout 超时时间（毫秒，-1表示无限等待）
 * @return 就绪的事件数，失败返回-1
 */
int (*epoll_wait)(int epfd, Stru_CN_LinuxEpollEvent_t* events, 
                  int maxevents, int timeout);
```

#### 3.3 Linux文件描述符工具接口 (`Stru_CN_LinuxFDInterface_t`)

```c
/**
 * @brief 创建匿名文件描述符
 * 
 * 使用memfd_create系统调用创建匿名文件描述符。
 * 
 * @param name 文件描述符名称
 * @param flags 创建标志（MFD_CLOEXEC, MFD_ALLOW_SEALING等）
 * @return 文件描述符，失败返回-1
 */
int (*fd_memfd_create)(const char* name, unsigned int flags);

/**
 * @brief 创建事件文件描述符
 * 
 * 使用eventfd系统调用创建事件文件描述符。
 * 
 * @param initval 初始计数器值
 * @param flags 创建标志（EFD_CLOEXEC, EFD_NONBLOCK, EFD_SEMAPHORE）
 * @return 文件描述符，失败返回-1
 */
int (*fd_eventfd)(unsigned int initval, int flags);

/**
 * @brief 创建定时器文件描述符
 * 
 * 使用timerfd_create系统调用创建定时器文件描述符。
 * 
 * @param clockid 时钟ID（CLOCK_REALTIME, CLOCK_MONOTONIC等）
 * @param flags 创建标志（TFD_CLOEXEC, TFD_NONBLOCK）
 * @return 文件描述符，失败返回-1
 */
int (*fd_timerfd_create)(int clockid, int flags);

/**
 * @brief 创建信号文件描述符
 * 
 * 使用signalfd系统调用创建信号文件描述符。
 * 
 * @param sigmask 信号掩码
 * @param flags 创建标志（SFD_CLOEXEC, SFD_NONBLOCK）
 * @return 文件描述符，失败返回-1
 */
int (*fd_signalfd)(const sigset_t* sigmask, int flags);

/**
 * @brief 设置文件描述符标志
 * 
 * 使用fcntl系统调用设置文件描述符标志。
 * 
 * @param fd 文件描述符
 * @param cmd 命令（F_GETFD, F_SETFD, F_GETFL, F_SETFL等）
 * @param arg 参数
 * @return 成功返回0，失败返回-1
 */
int (*fd_fcntl)(int fd, int cmd, long arg);

/**
 * @brief 复制文件描述符
 * 
 * 使用dup/dup2/dup3系统调用复制文件描述符。
 * 
 * @param oldfd 原文件描述符
 * @param newfd 新文件描述符（-1表示自动分配）
 * @param flags 复制标志（O_CLOEXEC）
 * @return 新文件描述符，失败返回-1
 */
int (*fd_dup)(int oldfd, int newfd, int flags);

/**
 * @brief 创建管道
 * 
 * 使用pipe/pipe2系统调用创建管道。
 * 
 * @param pipefd 管道文件描述符数组（pipefd[0]为读端，pipefd[1]为写端）
 * @param flags 创建标志（O_CLOEXEC, O_NONBLOCK）
 * @return 成功返回0，失败返回-1
 */
int (*fd_pipe)(int pipefd[2], int flags);

/**
 * @brief 创建socketpair
 * 
 * 使用socketpair系统调用创建socket对。
 * 
 * @param domain 协议域（AF_UNIX, AF_INET等）
 * @param type socket类型（SOCK_STREAM, SOCK_DGRAM等）
 * @param protocol 协议
 * @param sv socket文件描述符数组
 * @return 成功返回0，失败返回-1
 */
int (*fd_socketpair)(int domain, int type, int protocol, int sv[2]);
```

#### 3.4 Linux系统调用接口 (`Stru_CN_LinuxSyscallInterface_t`)

```c
/**
 * @brief 直接系统调用
 * 
 * 使用syscall函数进行直接系统调用。
 * 
 * @param number 系统调用号
 * @param ... 系统调用参数
 * @return 系统调用返回值
 */
long (*syscall)(long number, ...);

/**
 * @brief 获取系统信息
 * 
 * 使用sysinfo系统调用获取系统信息。
 * 
 * @param info 系统信息结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*sysinfo)(Stru_CN_LinuxSysInfo_t* info);

/**
 * @brief 获取进程信息
 * 
 * 使用getrusage系统调用获取进程资源使用信息。
 * 
 * @param who 进程类型（RUSAGE_SELF, RUSAGE_CHILDREN等）
 * @param usage 资源使用信息结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*getrusage)(int who, Stru_CN_LinuxRUsage_t* usage);

/**
 * @brief 获取系统配置值
 * 
 * 使用sysconf系统调用获取系统配置值。
 * 
 * @param name 配置名称（_SC_PAGESIZE, _SC_NPROCESSORS_ONLN等）
 * @return 配置值，失败返回-1
 */
long (*sysconf)(int name);

/**
 * @brief 获取进程限制
 * 
 * 使用getrlimit系统调用获取进程资源限制。
 * 
 * @param resource 资源类型（RLIMIT_CPU, RLIMIT_DATA等）
 * @param rlim 资源限制结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*getrlimit)(int resource, Stru_CN_LinuxRLimit_t* rlim);

/**
 * @brief 设置进程限制
 * 
 * 使用setrlimit系统调用设置进程资源限制。
 * 
 * @param resource 资源类型
 * @param rlim 资源限制结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*setrlimit)(int resource, const Stru_CN_LinuxRLimit_t* rlim);

/**
 * @brief 获取进程ID
 * 
 * 获取当前进程ID。
 * 
 * @return 进程ID
 */
pid_t (*getpid)(void);

/**
 * @brief 获取父进程ID
 * 
 * 获取父进程ID。
 * 
 * @return 父进程ID
 */
pid_t (*getppid)(void);

/**
 * @brief 获取用户ID
 * 
 * 获取当前用户ID。
 * 
 * @return 用户ID
 */
uid_t (*getuid)(void);

/**
 * @brief 获取有效用户ID
 * 
 * 获取有效用户ID。
 * 
 * @return 有效用户ID
 */
uid_t (*geteuid)(void);

/**
 * @brief 获取组ID
 * 
 * 获取当前组ID。
 * 
 * @return 组ID
 */
gid_t (*getgid)(void);

/**
 * @brief 获取有效组ID
 * 
 * 获取有效组ID。
 * 
 * @return 有效组ID
 */
gid_t (*getegid)(void);
```

### 4. 线程接口 (`Stru_CN_ThreadInterface_t`)

Linux平台线程接口基于POSIX线程（pthread）API实现。

#### 4.1 线程创建和管理

```c
/**
 * @brief 创建线程
 * 
 * 使用pthread_create创建新线程。
 * 
 * @param thread 线程句柄指针
 * @param attr 线程属性指针（可为NULL）
 * @param start_routine 线程入口函数
 * @param arg 线程参数
 * @return 成功返回0，失败返回错误码
 */
int (*thread_create)(CN_ThreadHandle_t* thread, const void* attr,
                     void* (*start_routine)(void*), void* arg);

/**
 * @brief 等待线程结束
 * 
 * 使用pthread_join等待线程结束。
 * 
 * @param thread 线程句柄
 * @param retval 线程返回值指针
 * @return 成功返回0，失败返回错误码
 */
int (*thread_join)(CN_ThreadHandle_t thread, void** retval);

/**
 * @brief 分离线程
 * 
 * 使用pthread_detach分离线程。
 * 
 * @param thread 线程句柄
 * @return 成功返回0，失败返回错误码
 */
int (*thread_detach)(CN_ThreadHandle_t thread);

/**
 * @brief 取消线程
 * 
 * 使用pthread_cancel取消线程。
 * 
 * @param thread 线程句柄
 * @return 成功返回0，失败返回错误码
 */
int (*thread_cancel)(CN_ThreadHandle_t thread);

/**
 * @brief 获取当前线程ID
 * 
 * 使用pthread_self获取当前线程ID。
 * 
 * @return 当前线程ID
 */
CN_ThreadHandle_t (*thread_self)(void);

/**
 * @brief 线程退出
 * 
 * 使用pthread_exit退出当前线程。
 * 
 * @param retval 退出值
 */
void (*thread_exit)(void* retval);
```

#### 4.2 线程同步

```c
/**
 * @brief 初始化互斥锁
 * 
 * 使用pthread_mutex_init初始化互斥锁。
 * 
 * @param mutex 互斥锁指针
 * @param attr 互斥锁属性指针（可为NULL）
 * @return 成功返回0，失败返回错误码
 */
int (*mutex_init)(CN_MutexHandle_t* mutex, const void* attr);

/**
 * @brief 销毁互斥锁
 * 
 * 使用pthread_mutex_destroy销毁互斥锁。
 * 
 * @param mutex 互斥锁指针
 * @return 成功返回0，失败返回错误码
 */
int (*mutex_destroy)(CN_MutexHandle_t* mutex);

/**
 * @brief 锁定互斥锁
 * 
 * 使用pthread_mutex_lock锁定互斥锁。
 * 
 * @param mutex 互斥锁指针
 * @return 成功返回0，失败返回错误码
 */
int (*mutex_lock)(CN_MutexHandle_t* mutex);

/**
 * @brief 尝试锁定互斥锁
 * 
 * 使用pthread_mutex_trylock尝试锁定互斥锁。
 * 
 * @param mutex 互斥锁指针
 * @return 成功返回0，失败返回错误码
 */
int (*mutex_trylock)(CN_MutexHandle_t* mutex);

/**
 * @brief 解锁互斥锁
 * 
 * 使用pthread_mutex_unlock解锁互斥锁。
 * 
 * @param mutex 互斥锁指针
 * @return 成功返回0，失败返回错误码
 */
int (*mutex_unlock)(CN_MutexHandle_t* mutex);

/**
 * @brief 初始化条件变量
 * 
 * 使用pthread_cond_init初始化条件变量。
 * 
 * @param cond 条件变量指针
 * @param attr 条件变量属性指针（可为NULL）
 * @return 成功返回0，失败返回错误码
 */
int (*cond_init)(CN_CondHandle_t* cond, const void* attr);

/**
 * @brief 销毁条件变量
 * 
 * 使用pthread_cond_destroy销毁条件变量。
 * 
 * @param cond 条件变量指针
 * @return 成功返回0，失败返回错误码
 */
int (*cond_destroy)(CN_CondHandle_t* cond);

/**
 * @brief 等待条件变量
 * 
 * 使用pthread_cond_wait等待条件变量。
 * 
 * @param cond 条件变量指针
 * @param mutex 互斥锁指针
 * @return 成功返回0，失败返回错误码
 */
int (*cond_wait)(CN_CondHandle_t* cond, CN_MutexHandle_t* mutex);

/**
 * @brief 定时等待条件变量
 * 
 * 使用pthread_cond_timedwait定时等待条件变量。
 * 
 * @param cond 条件变量指针
 * @param mutex 互斥锁指针
 * @param abstime 绝对时间
 * @return 成功返回0，失败返回错误码
 */
int (*cond_timedwait)(CN_CondHandle_t* cond, CN_MutexHandle_t* mutex,
                      const Stru_CN_TimeSpec_t* abstime);

/**
 * @brief 通知条件变量
 * 
 * 使用pthread_cond_signal通知一个等待线程。
 * 
 * @param cond 条件变量指针
 * @return 成功返回0，失败返回错误码
 */
int (*cond_signal)(CN_CondHandle_t* cond);

/**
 * @brief 广播条件变量
 * 
 * 使用pthread_cond_broadcast通知所有等待线程。
 * 
 * @param cond 条件变量指针
 * @return 成功返回0，失败返回错误码
 */
int (*cond_broadcast)(CN_CondHandle_t* cond);
```

### 5. 网络接口 (`Stru_CN_NetworkInterface_t`)

Linux平台网络接口基于BSD套接字API实现。

#### 5.1 套接字操作

```c
/**
 * @brief 创建套接字
 * 
 * 使用socket系统调用创建套接字。
 * 
 * @param domain 协议域（AF_INET, AF_INET6, AF_UNIX等）
 * @param type 套接字类型（SOCK_STREAM, SOCK_DGRAM等）
 * @param protocol 协议（0表示默认）
 * @return 套接字描述符，失败返回-1
 */
int (*socket_create)(int domain, int type, int protocol);

/**
 * @brief 关闭套接字
 * 
 * 使用close系统调用关闭套接字。
 * 
 * @param sockfd 套接字描述符
 * @return 成功返回0，失败返回-1
 */
int (*socket_close)(int sockfd);

/**
 * @brief 绑定套接字
 * 
 * 使用bind系统调用绑定套接字。
 * 
 * @param sockfd 套接字描述符
 * @param addr 地址结构体指针
 * @param addrlen 地址结构体长度
 * @return 成功返回0，失败返回-1
 */
int (*socket_bind)(int sockfd, const Stru_CN_SockAddr_t* addr, socklen_t addrlen);

/**
 * @brief 监听套接字
 * 
 * 使用listen系统调用监听套接字。
 * 
 * @param sockfd 套接字描述符
 * @param backlog 等待连接队列的最大长度
 * @return 成功返回0，失败返回-1
 */
int (*socket_listen)(int sockfd, int backlog);

/**
 * @brief 接受连接
 * 
 * 使用accept系统调用接受连接。
 * 
 * @param sockfd 套接字描述符
 * @param addr 客户端地址结构体指针
 * @param addrlen 地址结构体长度指针
 * @return 新连接套接字描述符，失败返回-1
 */
int (*socket_accept)(int sockfd, Stru_CN_SockAddr_t* addr, socklen_t* addrlen);

/**
 * @brief 连接服务器
 * 
 * 使用connect系统调用连接服务器。
 * 
 * @param sockfd 套接字描述符
 * @param addr 服务器地址结构体指针
 * @param addrlen 地址结构体长度
 * @return 成功返回0，失败返回-1
 */
int (*socket_connect)(int sockfd, const Stru_CN_SockAddr_t* addr, socklen_t addrlen);
```

#### 5.2 数据传输

```c
/**
 * @brief 发送数据
 * 
 * 使用send系统调用发送数据。
 * 
 * @param sockfd 套接字描述符
 * @param buf 发送缓冲区
 * @param len 发送数据长度
 * @param flags 发送标志
 * @return 实际发送的字节数，失败返回-1
 */
ssize_t (*socket_send)(int sockfd, const void* buf, size_t len, int flags);

/**
 * @brief 接收数据
 * 
 * 使用recv系统调用接收数据。
 * 
 * @param sockfd 套接字描述符
 * @param buf 接收缓冲区
 * @param len 接收缓冲区长度
 * @param flags 接收标志
 * @return 实际接收的字节数，失败返回-1
 */
ssize_t (*socket_recv)(int sockfd, void* buf, size_t len, int flags);

/**
 * @brief 发送到指定地址
 * 
 * 使用sendto系统调用发送数据到指定地址。
 * 
 * @param sockfd 套接字描述符
 * @param buf 发送缓冲区
 * @param len 发送数据长度
 * @param flags 发送标志
 * @param dest_addr 目标地址结构体指针
 * @param addrlen 地址结构体长度
 * @return 实际发送的字节数，失败返回-1
 */
ssize_t (*socket_sendto)(int sockfd, const void* buf, size_t len, int flags,
                         const Stru_CN_SockAddr_t* dest_addr, socklen_t addrlen);

/**
 * @brief 从指定地址接收
 * 
 * 使用recvfrom系统调用从指定地址接收数据。
 * 
 * @param sockfd 套接字描述符
 * @param buf 接收缓冲区
 * @param len 接收缓冲区长度
 * @param flags 接收标志
 * @param src_addr 源地址结构体指针
 * @param addrlen 地址结构体长度指针
 * @return 实际接收的字节数，失败返回-1
 */
ssize_t (*socket_recvfrom)(int sockfd, void* buf, size_t len, int flags,
                           Stru_CN_SockAddr_t* src_addr, socklen_t* addrlen);
```

#### 5.3 套接字选项

```c
/**
 * @brief 获取套接字选项
 * 
 * 使用getsockopt系统调用获取套接字选项。
 * 
 * @param sockfd 套接字描述符
 * @param level 选项级别（SOL_SOCKET, IPPROTO_TCP等）
 * @param optname 选项名称
 * @param optval 选项值缓冲区
 * @param optlen 选项值长度指针
 * @return 成功返回0，失败返回-1
 */
int (*socket_get_option)(int sockfd, int level, int optname,
                         void* optval, socklen_t* optlen);

/**
 * @brief 设置套接字选项
 * 
 * 使用setsockopt系统调用设置套接字选项。
 * 
 * @param sockfd 套接字描述符
 * @param level 选项级别
 * @param optname 选项名称
 * @param optval 选项值指针
 * @param optlen 选项值长度
 * @return 成功返回0，失败返回-1
 */
int (*socket_set_option)(int sockfd, int level, int optname,
                         const void* optval, socklen_t optlen);
```

#### 5.4 地址转换

```c
/**
 * @brief 将IP地址字符串转换为二进制格式
 * 
 * 使用inet_pton系统调用转换IP地址。
 * 
 * @param af 地址族（AF_INET, AF_INET6）
 * @param src IP地址字符串
 * @param dst 二进制地址缓冲区
 * @return 成功返回1，失败返回0或-1
 */
int (*inet_pton)(int af, const char* src, void* dst);

/**
 * @brief 将二进制IP地址转换为字符串格式
 * 
 * 使用inet_ntop系统调用转换IP地址。
 * 
 * @param af 地址族
 * @param src 二进制地址指针
 * @param dst 字符串缓冲区
 * @param size 缓冲区大小
 * @return 成功返回字符串指针，失败返回NULL
 */
const char* (*inet_ntop)(int af, const void* src, char* dst, socklen_t size);
```

#### 5.5 主机名和地址解析

```c
/**
 * @brief 获取主机名
 * 
 * 使用gethostname系统调用获取主机名。
 * 
 * @param name 主机名缓冲区
 * @param len 缓冲区长度
 * @return 成功返回0，失败返回-1
 */
int (*gethostname)(char* name, size_t len);

/**
 * @brief 获取主机信息
 * 
 * 使用gethostbyname或getaddrinfo获取主机信息。
 * 
 * @param name 主机名
 * @return 主机信息结构体指针，失败返回NULL
 */
void* (*gethostbyname)(const char* name);

/**
 * @brief 获取地址信息
 * 
 * 使用getaddrinfo获取地址信息。
 * 
 * @param node 节点名（主机名或IP地址）
 * @param service 服务名或端口号
 * @param hints 提示结构体指针
 * @param res 结果链表指针
 * @return 成功返回0，失败返回错误码
 */
int (*getaddrinfo)(const char* node, const char* service,
                   const Stru_CN_AddrInfo_t* hints, Stru_CN_AddrInfo_t** res);

/**
 * @brief 释放地址信息
 * 
 * 使用freeaddrinfo释放地址信息。
 * 
 * @param res 地址信息链表
 */
void (*freeaddrinfo)(Stru_CN_AddrInfo_t* res);
```

### 6. 时间接口 (`Stru_CN_TimeInterface_t`)

Linux平台时间接口基于POSIX时间API实现。

#### 6.1 时间获取

```c
/**
 * @brief 获取当前时间
 * 
 * 使用clock_gettime系统调用获取当前时间。
 * 
 * @param clock_id 时钟ID（CLOCK_REALTIME, CLOCK_MONOTONIC等）
 * @param tp 时间结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*clock_gettime)(int clock_id, Stru_CN_TimeSpec_t* tp);

/**
 * @brief 获取当前时间（秒级精度）
 * 
 * 使用time系统调用获取当前时间。
 * 
 * @param tloc 时间值指针（可为NULL）
 * @return 当前时间（秒）
 */
time_t (*time)(time_t* tloc);

/**
 * @brief 获取高精度时间戳
 * 
 * 使用gettimeofday系统调用获取高精度时间戳。
 * 
 * @param tv 时间值结构体指针
 * @param tz 时区结构体指针（已废弃，应为NULL）
 * @return 成功返回0，失败返回-1
 */
int (*gettimeofday)(Stru_CN_TimeVal_t* tv, void* tz);
```

#### 6.2 时间转换

```c
/**
 * @brief 将时间转换为本地时间
 * 
 * 使用localtime_r将时间转换为本地时间。
 * 
 * @param timer 时间值指针
 * @param result 结果结构体指针
 * @return 本地时间结构体指针
 */
Stru_CN_TM_t* (*localtime_r)(const time_t* timer, Stru_CN_TM_t* result);

/**
 * @brief 将时间转换为UTC时间
 * 
 * 使用gmtime_r将时间转换为UTC时间。
 * 
 * @param timer 时间值指针
 * @param result 结果结构体指针
 * @return UTC时间结构体指针
 */
Stru_CN_TM_t* (*gmtime_r)(const time_t* timer, Stru_CN_TM_t* result);

/**
 * @brief 将时间结构体转换为时间值
 * 
 * 使用mktime将时间结构体转换为时间值。
 * 
 * @param tm 时间结构体指针
 * @return 时间值
 */
time_t (*mktime)(Stru_CN_TM_t* tm);
```

#### 6.3 时间格式化

```c
/**
 * @brief 格式化时间
 * 
 * 使用strftime格式化时间。
 * 
 * @param s 输出缓冲区
 * @param maxsize 缓冲区大小
 * @param format 格式字符串
 * @param tm 时间结构体指针
 * @return 格式化后的字符数
 */
size_t (*strftime)(char* s, size_t maxsize, const char* format,
                   const Stru_CN_TM_t* tm);

/**
 * @brief 解析时间字符串
 * 
 * 使用strptime解析时间字符串。
 * 
 * @param s 时间字符串
 * @param format 格式字符串
 * @param tm 时间结构体指针
 * @return 解析后的剩余字符串指针
 */
char* (*strptime)(const char* s, const char* format, Stru_CN_TM_t* tm);
```

#### 6.4 定时器

```c
/**
 * @brief 创建定时器
 * 
 * 使用timer_create创建定时器。
 * 
 * @param clockid 时钟ID
 * @param evp 事件结构体指针
 * @param timerid 定时器ID指针
 * @return 成功返回0，失败返回-1
 */
int (*timer_create)(int clockid, Stru_CN_Sigevent_t* evp, timer_t* timerid);

/**
 * @brief 设置定时器
 * 
 * 使用timer_settime设置定时器。
 * 
 * @param timerid 定时器ID
 * @param flags 标志（0或TIMER_ABSTIME）
 * @param value 定时器值结构体指针
 * @param ovalue 原定时器值结构体指针（可为NULL）
 * @return 成功返回0，失败返回-1
 */
int (*timer_settime)(timer_t timerid, int flags,
                     const Stru_CN_ITimerSpec_t* value,
                     Stru_CN_ITimerSpec_t* ovalue);

/**
 * @brief 获取定时器剩余时间
 * 
 * 使用timer_gettime获取定时器剩余时间。
 * 
 * @param timerid 定时器ID
 * @param value 定时器值结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*timer_gettime)(timer_t timerid, Stru_CN_ITimerSpec_t* value);

/**
 * @brief 删除定时器
 * 
 * 使用timer_delete删除定时器。
 * 
 * @param timerid 定时器ID
 * @return 成功返回0，失败返回-1
 */
int (*timer_delete)(timer_t timerid);
```

### 7. 系统接口 (`Stru_CN_SystemInterface_t`)

Linux平台系统接口提供系统信息和配置功能。

#### 7.1 系统信息

```c
/**
 * @brief 获取系统名称
 * 
 * 使用uname系统调用获取系统名称。
 * 
 * @param name 系统信息结构体指针
 * @return 成功返回0，失败返回-1
 */
int (*uname)(Stru_CN_UtsName_t* name);

/**
 * @brief 获取系统页面大小
 * 
 * 使用sysconf获取系统页面大小。
 * 
 * @return 页面大小（字节）
 */
long (*get_page_size)(void);

/**
 * @brief 获取物理内存大小
 * 
 * 使用sysconf获取物理内存大小。
 * 
 * @return 物理内存大小（字节）
 */
long long (*get_phys_pages)(void);

/**
 * @brief 获取可用物理内存大小
 * 
 * 使用sysconf获取可用物理内存大小。
 * 
 * @return 可用物理内存大小（字节）
 */
long long (*get_avail_phys_pages)(void);

/**
 * @brief 获取CPU核心数
 * 
 * 使用sysconf获取CPU核心数。
 * 
 * @return CPU核心数
 */
long (*get_nprocs_conf)(void);

/**
 * @brief 获取在线CPU核心数
 * 
 * 使用sysconf获取在线CPU核心数。
 * 
 * @return 在线CPU核心数
 */
long (*get_nprocs)(void);
```

#### 7.2 环境变量

```c
/**
 * @brief 获取环境变量值
 * 
 * 使用getenv获取环境变量值。
 * 
 * @param name 环境变量名
 * @return 环境变量值，不存在返回NULL
 */
char* (*getenv)(const char* name);

/**
 * @brief 设置环境变量
 * 
 * 使用setenv设置环境变量。
 * 
 * @param name 环境变量名
 * @param value 环境变量值
 * @param overwrite 是否覆盖（非0表示覆盖）
 * @return 成功返回0，失败返回-1
 */
int (*setenv)(const char* name, const char* value, int overwrite);

/**
 * @brief 删除环境变量
 * 
 * 使用unsetenv删除环境变量。
 * 
 * @param name 环境变量名
 * @return 成功返回0，失败返回-1
 */
int (*unsetenv)(const char* name);

/**
 * @brief 清空环境变量
 * 
 * 使用clearenv清空环境变量。
 * 
 * @return 成功返回0，失败返回-1
 */
int (*clearenv)(void);
```

#### 7.3 进程控制

```c
/**
 * @brief 创建子进程
 * 
 * 使用fork创建子进程。
 * 
 * @return 子进程返回0，父进程返回子进程ID，失败返回-1
 */
pid_t (*fork)(void);

/**
 * @brief 执行程序
 * 
 * 使用exec系列函数执行程序。
 * 
 * @param path 程序路径
 * @param argv 参数数组
 * @param envp 环境变量数组
 * @return 成功不返回，失败返回-1
 */
int (*execve)(const char* path, char* const argv[], char* const envp[]);

/**
 * @brief 等待子进程
 * 
 * 使用waitpid等待子进程。
 * 
 * @param pid 进程ID
 * @param status 状态指针
 * @param options 选项
 * @return 成功返回进程ID，失败返回-1
 */
pid_t (*waitpid)(pid_t pid, int* status, int options);

/**
 * @brief 退出进程
 * 
 * 使用exit退出进程。
 * 
 * @param status 退出状态
 */
void (*exit)(int status);

/**
 * @brief 获取进程组ID
 * 
 * 使用getpgid获取进程组ID。
 * 
 * @param pid 进程ID
 * @return 进程组ID，失败返回-1
 */
pid_t (*getpgid)(pid_t pid);

/**
 * @brief 设置进程组ID
 * 
 * 使用setpgid设置进程组ID。
 * 
 * @param pid 进程ID
 * @param pgid 进程组ID
 * @return 成功返回0，失败返回-1
 */
int (*setpgid)(pid_t pid, pid_t pgid);
```

#### 7.4 信号处理

```c
/**
 * @brief 设置信号处理函数
 * 
 * 使用sigaction设置信号处理函数。
 * 
 * @param signum 信号编号
 * @param act 新动作结构体指针
 * @param oldact 原动作结构体指针（可为NULL）
 * @return 成功返回0，失败返回-1
 */
int (*sigaction)(int signum, const Stru_CN_SigAction_t* act,
                 Stru_CN_SigAction_t* oldact);

/**
 * @brief 发送信号
 * 
 * 使用kill发送信号。
 * 
 * @param pid 进程ID
 * @param sig 信号编号
 * @return 成功返回0，失败返回-1
 */
int (*kill)(pid_t pid, int sig);

/**
 * @brief 阻塞信号
 * 
 * 使用sigprocmask阻塞信号。
 * 
 * @param how 操作方式（SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK）
 * @param set 信号集指针
 * @param oldset 原信号集指针（可为NULL）
 * @return 成功返回0，失败返回-1
 */
int (*sigprocmask)(int how, const sigset_t* set, sigset_t* oldset);

/**
 * @brief 等待信号
 * 
 * 使用sigsuspend等待信号。
 * 
 * @param mask 信号掩码指针
 * @return 总是返回-1，errno设置为EINTR
 */
int (*sigsuspend)(const sigset_t* mask);
```

## 使用示例

### 示例1：初始化Linux平台

```c
#include "CN_platform_linux.h"

int main() {
    // 初始化Linux平台
    if (!CN_platform_linux_initialize()) {
        printf("Failed to initialize Linux platform\n");
        return 1;
    }
    
    // 获取平台接口
    Stru_CN_PlatformInterface_t* platform = CN_platform_linux_get_interface();
    if (!platform) {
        printf("Failed to get platform interface\n");
        return 1;
    }
    
    // 使用文件系统接口
    Stru_CN_FileSystemInterface_t* fs = platform->filesystem;
    if (fs->file_exists("/etc/passwd")) {
        printf("/etc/passwd exists\n");
    }
    
    // 清理平台
    CN_platform_linux_cleanup();
    return 0;
}
```

### 示例2：使用epoll进行事件驱动编程

```c
#include "CN_platform_linux.h"

void epoll_example() {
    // 获取epoll接口
    Stru_CN_LinuxEpollInterface_t* epoll = CN_platform_linux_get_epoll();
    
    // 创建epoll实例
    int epfd = epoll->epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        return;
    }
    
    // 添加文件描述符到epoll
    Stru_CN_LinuxEpollEvent_t event;
    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    
    if (epoll->epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &event) < 0) {
        perror("epoll_ctl");
        close(epfd);
        return;
    }
    
    // 等待事件
    Stru_CN_LinuxEpollEvent_t events[10];
    int n = epoll->epoll_wait(epfd, events, 10, 1000);
    if (n > 0) {
        printf("Got %d events\n", n);
    }
    
    // 关闭epoll
    epoll->epoll_close(epfd);
}
```

### 示例3：使用inotify监控文件系统

```c
#include "CN_platform_linux.h"

void inotify_example() {
    // 获取inotify接口
    Stru_CN_LinuxInotifyInterface_t* inotify = CN_platform_linux_get_inotify();
    
    // 初始化inotify
    int fd = inotify->inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return;
    }
    
    // 添加监视
    int wd = inotify->inotify_add_watch(fd, "/tmp", 
                                        IN_CREATE | IN_DELETE | IN_MODIFY);
    if (wd < 0) {
        perror("inotify_add_watch");
        inotify->inotify_close(fd);
        return;
    }
    
    // 读取事件
    Stru_CN_LinuxInotifyEvent_t events[10];
    ssize_t n = inotify->inotify_read_events(fd, events, 10);
    if (n > 0) {
        printf("Got %zd inotify events\n", n);
    }
    
    // 移除监视并关闭
    inotify->inotify_rm_watch(fd, wd);
    inotify->inotify_close(fd);
}
```

### 示例4：使用线程接口

```c
#include "CN_platform_linux.h"

void* thread_function(void* arg) {
    int* value = (int*)arg;
    printf("Thread started with value: %d\n", *value);
    return NULL;
}

void thread_example() {
    // 获取线程接口
    Stru_CN_ThreadInterface_t* thread = CN_platform_linux_get_thread();
    
    // 创建线程
    CN_ThreadHandle_t tid;
    int value = 42;
    
    if (thread->thread_create(&tid, NULL, thread_function, &value) != 0) {
        perror("thread_create");
        return;
    }
    
    // 等待线程结束
    thread->thread_join(tid, NULL);
    printf("Thread finished\n");
}
```

## 编译和链接

### 编译选项

```makefile
# 编译Linux平台模块
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC -I./include
LDFLAGS = -lpthread -lrt

# 目标文件
OBJS = src/infrastructure/platform/linux/CN_platform_linux.o \
       src/infrastructure/platform/linux/CN_platform_linux_filesystem.o \
       src/infrastructure/platform/linux/CN_platform_linux_thread.o \
       src/infrastructure/platform/linux/CN_platform_linux_network.o \
       src/infrastructure/platform/linux/CN_platform_linux_time.o \
       src/infrastructure/platform/linux/CN_platform_linux_system.o \
       src/infrastructure/platform/linux/CN_platform_linux_special.o

# 静态库
libcn_platform_linux.a: $(OBJS)
	ar rcs $@ $(OBJS)

# 共享库
libcn_platform_linux.so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)
```

### 依赖关系

Linux平台模块依赖于以下系统库：
- `libpthread` - POSIX线程库
- `librt` - 实时扩展库（用于定时器和高精度时间）
- `libdl` - 动态链接库（可选，用于动态加载）

## 兼容性说明

### 支持的Linux内核版本

- 最低要求：Linux内核 2.6.32 或更高版本
- 推荐版本：Linux内核 3.2 或更高版本（支持完整的epoll功能）
- 完全支持：Linux内核 4.4 或更高版本（支持所有现代功能）

### 功能检测

Linux平台模块会自动检测系统功能，并提供相应的API：
- 如果系统不支持inotify，相关接口将返回错误
- 如果系统不支持epoll，将回退到select/poll
- 如果系统不支持timerfd，将使用传统的定时器

### 平台特定注意事项

1. **文件描述符限制**：Linux系统有文件描述符限制，默认通常为1024。对于需要大量文件描述符的应用，需要调整`ulimit`设置。

2. **信号处理**：Linux的信号处理与Windows不同，需要注意信号掩码和信号处理函数的线程安全性。

3. **内存管理**：Linux使用虚拟内存管理，大内存分配可能失败，即使物理内存充足。

4. **线程模型**：Linux使用NPTL（Native POSIX Thread Library）线程模型，线程创建和销毁开销较小。

## 性能优化建议

### 文件I/O优化

1. 使用`O_DIRECT`标志进行直接I/O，避免内核缓存
2. 使用`posix_fadvise`提供文件访问模式提示
3. 对于大量小文件，考虑使用内存映射文件（mmap）

### 网络优化

1. 使用`TCP_NODELAY`选项禁用Nagle算法，减少延迟
2. 使用`SO_REUSEPORT`选项允许多个进程绑定同一端口
3. 使用`sendfile`系统调用进行零拷贝文件传输

### 线程优化

1. 使用线程池避免频繁创建和销毁线程
2. 使用`pthread_setaffinity_np`设置线程CPU亲和性
3. 使用读写锁（pthread_rwlock_t）替代互斥锁，提高读多写少场景的性能

## 故障排除

### 常见问题

1. **权限问题**：确保程序有足够的权限执行所需操作
2. **资源限制**：检查文件描述符、内存、线程等资源限制
3. **内核版本**：某些功能需要特定内核版本支持

### 调试技巧

1. 使用`strace`跟踪系统调用
2. 使用`ltrace`跟踪库函数调用
3. 使用`valgrind`检测内存泄漏和错误

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本发布
- 实现完整的POSIX API封装
- 支持Linux特有功能（inotify、epoll等）
- 提供完整的API文档和示例

## 贡献指南

欢迎为Linux平台模块贡献代码。请遵循以下指南：

1. 遵循项目编码规范
2. 添加适当的单元测试
3. 更新相关文档
4. 确保向后兼容性

## 许可证

CN_Language Linux平台模块采用MIT许可证。详见项目根目录的LICENSE文件。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 项目GitHub仓库：https://github.com/ChenChao-HuaiAn/CN_Language
- 项目GitCode仓库：https://gitcode.com/ChenChao_GitCode/CN_Language.git

---
*文档最后更新：2026年1月3日*
