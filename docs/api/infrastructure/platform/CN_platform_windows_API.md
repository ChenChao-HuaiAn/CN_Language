# CN_Language Windows平台API文档

## 文档概述

本文档详细描述了CN_Language项目Windows平台实现模块的API接口。Windows平台模块提供了基于Win32 API的完整平台抽象层实现，包括文件系统、线程、网络、时间、系统信息等核心功能，以及Windows特有的注册表、GUI、COM等功能。

## API分类

### 1. 核心平台接口

#### 1.1 平台初始化和管理

```c
/**
 * @brief 初始化Windows平台实现
 * 
 * 初始化Windows平台所需的所有资源，包括Winsock、COM等。
 * 必须在调用任何其他Windows平台函数之前调用此函数。
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_windows_initialize(void);

/**
 * @brief 清理Windows平台实现
 * 
 * 释放Windows平台使用的所有资源。
 * 在程序退出前调用此函数。
 */
void CN_platform_windows_cleanup(void);

/**
 * @brief 获取完整的Windows平台接口
 * 
 * 返回包含所有平台接口的完整平台接口结构体。
 * 
 * @return Windows平台接口指针，失败返回NULL
 */
Stru_CN_PlatformInterface_t* CN_platform_windows_get_interface(void);
```

#### 1.2 各功能模块接口获取

```c
/**
 * @brief 获取Windows平台文件系统接口
 * 
 * @return Windows平台文件系统接口指针
 */
Stru_CN_FileSystemInterface_t* CN_platform_windows_get_filesystem(void);

/**
 * @brief 获取Windows平台线程接口
 * 
 * @return Windows平台线程接口指针
 */
Stru_CN_ThreadInterface_t* CN_platform_windows_get_thread(void);

/**
 * @brief 获取Windows平台网络接口
 * 
 * @return Windows平台网络接口指针
 */
Stru_CN_NetworkInterface_t* CN_platform_windows_get_network(void);

/**
 * @brief 获取Windows平台时间接口
 * 
 * @return Windows平台时间接口指针
 */
Stru_CN_TimeInterface_t* CN_platform_windows_get_time(void);

/**
 * @brief 获取Windows平台系统接口
 * 
 * @return Windows平台系统接口指针
 */
Stru_CN_SystemInterface_t* CN_platform_windows_get_system(void);
```

### 2. 文件系统接口 (`Stru_CN_FileSystemInterface_t`)

#### 2.1 文件操作

```c
/**
 * @brief 检查文件是否存在
 * 
 * @param path 文件路径（UTF-8编码）
 * @return 文件存在返回true，否则返回false
 */
bool (*file_exists)(const char* path);

/**
 * @brief 打开文件
 * 
 * @param path 文件路径（UTF-8编码）
 * @param mode 打开模式
 * @return 文件句柄，失败返回NULL
 */
CN_FileHandle_t (*file_open)(const char* path, Eum_CN_FileMode_t mode);

/**
 * @brief 关闭文件
 * 
 * @param handle 文件句柄
 * @return 关闭成功返回true，失败返回false
 */
bool (*file_close)(CN_FileHandle_t handle);

/**
 * @brief 读取文件
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
 * @param handle 文件句柄
 * @param offset 偏移量
 * @param whence 起始位置（SEEK_SET, SEEK_CUR, SEEK_END）
 * @return 定位成功返回true，失败返回false
 */
bool (*file_seek)(CN_FileHandle_t handle, int64_t offset, int whence);

/**
 * @brief 获取文件当前位置
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
 * @param path 文件路径
 * @param info 文件信息结构体指针
 * @return 获取成功返回true，失败返回false
 */
bool (*file_get_info)(const char* path, Stru_CN_FileInfo_t* info);

/**
 * @brief 设置文件属性
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
 * @param path 文件路径
 * @return 删除成功返回true，失败返回false
 */
bool (*file_delete)(const char* path);

/**
 * @brief 重命名文件
 * 
 * @param old_path 原文件路径
 * @param new_path 新文件路径
 * @return 重命名成功返回true，失败返回false
 */
bool (*file_rename)(const char* old_path, const char* new_path);

/**
 * @brief 复制文件
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
 * @param path 目录路径
 * @return 创建成功返回true，失败返回false
 */
bool (*directory_create)(const char* path);

/**
 * @brief 删除目录
 * 
 * @param path 目录路径
 * @return 删除成功返回true，失败返回false
 */
bool (*directory_delete)(const char* path);

/**
 * @brief 检查目录是否存在
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
 * @param path 目录路径
 * @return 目录句柄，失败返回NULL
 */
void* (*directory_open)(const char* path);

/**
 * @brief 读取目录项
 * 
 * @param dir_handle 目录句柄
 * @param info 文件信息结构体指针
 * @return 读取成功返回true，没有更多项返回false
 */
bool (*directory_read)(void* dir_handle, Stru_CN_FileInfo_t* info);

/**
 * @brief 关闭目录
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
 * @param path 路径字符串
 * @return 是绝对路径返回true，否则返回false
 */
bool (*path_is_absolute)(const char* path);

/**
 * @brief 检查路径是否为相对路径
 * 
 * @param path 路径字符串
 * @return 是相对路径返回true，否则返回false
 */
bool (*path_is_relative)(const char* path);

/**
 * @brief 组合路径
 * 
 * @param path1 第一个路径
 * @param path2 第二个路径
 * @return 组合后的路径字符串，需要调用者释放
 */
char* (*path_combine)(const char* path1, const char* path2);

/**
 * @brief 获取目录部分
 * 
 * @param path 完整路径
 * @return 目录部分字符串，需要调用者释放
 */
char* (*path_get_directory)(const char* path);

/**
 * @brief 获取文件名部分
 * 
 * @param path 完整路径
 * @return 文件名部分字符串，需要调用者释放
 */
char* (*path_get_filename)(const char* path);

/**
 * @brief 获取文件扩展名
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
 * @return 当前目录字符串，需要调用者释放
 */
char* (*get_current_directory)(void);

/**
 * @brief 设置当前工作目录
 * 
 * @param path 目录路径
 * @return 设置成功返回true，失败返回false
 */
bool (*set_current_directory)(const char* path);

/**
 * @brief 获取用户主目录
 * 
 * @return 用户主目录字符串，需要调用者释放
 */
char* (*get_home_directory)(void);

/**
 * @brief 获取临时目录
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
 * @param path 磁盘路径
 * @return 可用空间字节数
 */
uint64_t (*get_disk_free_space)(const char* path);

/**
 * @brief 获取磁盘总空间
 * 
 * @param path 磁盘路径
 * @return 总空间字节数
 */
uint64_t (*get_disk_total_space)(const char* path);
```

### 3. 线程和同步接口 (`Stru_CN_ThreadInterface_t`)

#### 3.1 线程管理

```c
/**
 * @brief 创建线程
 * 
 * @param func 线程函数
 * @param user_data 用户数据
 * @param name 线程名称（可选）
 * @return 线程句柄，失败返回NULL
 */
CN_ThreadHandle_t (*thread_create)(CN_ThreadFunc func, void* user_data, const char* name);

/**
 * @brief 等待线程结束
 * 
 * @param thread 线程句柄
 * @param exit_code 退出码指针（可选）
 * @return 等待成功返回true，失败返回false
 */
bool (*thread_join)(CN_ThreadHandle_t thread, int* exit_code);

/**
 * @brief 分离线程
 * 
 * @param thread 线程句柄
 * @return 分离成功返回true，失败返回false
 */
bool (*thread_detach)(CN_ThreadHandle_t thread);

/**
 * @brief 线程睡眠
 * 
 * @param milliseconds 睡眠毫秒数
 */
void (*thread_sleep)(uint32_t milliseconds);

/**
 * @brief 线程让步
 */
void (*thread_yield)(void);
```

#### 3.2 线程本地存储

```c
/**
 * @brief 分配线程本地存储键
 * 
 * @return TLS键，失败返回NULL
 */
void* (*tls_allocate)(void);

/**
 * @brief 释放线程本地存储键
 * 
 * @param key TLS键
 * @return 释放成功返回true，失败返回false
 */
bool (*tls_free)(void* key);

/**
 * @brief 设置线程本地存储值
 * 
 * @param key TLS键
 * @param value 要设置的值
 * @return 设置成功返回true，失败返回false
 */
bool (*tls_set_value)(void* key, void* value);

/**
 * @brief 获取线程本地存储值
 * 
 * @param key TLS键
 * @return 存储的值
 */
void* (*tls_get_value)(void* key);
```

#### 3.3 互斥锁

```c
/**
 * @brief 创建互斥锁
 * 
 * @return 互斥锁句柄，失败返回NULL
 */
CN_MutexHandle_t (*mutex_create)(void);

/**
 * @brief 销毁互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return 销毁成功返回true，失败返回false
 */
bool (*mutex_destroy)(CN_MutexHandle_t mutex);

/**
 * @brief 锁定互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return 锁定成功返回true，失败返回false
 */
bool (*mutex_lock)(CN_MutexHandle_t mutex);

/**
 * @brief 尝试锁定互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return 锁定成功返回true，已被锁定返回false
 */
bool (*mutex_try_lock)(CN_MutexHandle_t mutex);

/**
 * @brief 解锁互斥锁
 * 
 * @param mutex 互斥锁句柄
 * @return 解锁成功返回true，失败返回false
 */
bool (*mutex_unlock)(CN_MutexHandle_t mutex);
```

#### 3.4 递归互斥锁

```c
/**
 * @brief 创建递归互斥锁
 * 
 * @return 递归互斥锁句柄，失败返回NULL
 */
CN_MutexHandle_t (*recursive_mutex_create)(void);

/**
 * @brief 销毁递归互斥锁
 * 
 * @param mutex 递归互斥锁句柄
 * @return 销毁成功返回true，失败返回false
 */
bool (*recursive_mutex_destroy)(CN_MutexHandle_t mutex);

/**
 * @brief 锁定递归互斥锁
 * 
 * @param mutex 递归互斥锁句柄
 * @return 锁定成功返回true，失败返回false
 */
bool (*recursive_mutex_lock)(CN_MutexHandle_t mutex);

/**
 * @brief 尝试锁定递归互斥锁
 * 
 * @param mutex 递归互斥锁句柄
 * @return 锁定成功返回true，已被锁定返回false
 */
bool (*recursive_mutex_try_lock)(CN_MutexHandle_t mutex);

/**
 * @brief 解锁递归互斥锁
 * 
 * @param mutex 递归互斥锁句柄
 * @return 解锁成功返回true，失败返回false
 */
bool (*recursive_mutex_unlock)(CN_MutexHandle_t mutex);
```

#### 3.5 信号量

```c
/**
 * @brief 创建信号量
 * 
 * @param initial_count 初始计数
 * @param max_count 最大计数
 * @return 信号量句柄，失败返回NULL
 */
CN_SemaphoreHandle_t (*semaphore_create)(int initial_count, int max_count);

/**
 * @brief 销毁信号量
 * 
 * @param semaphore 信号量句柄
 * @return 销毁成功返回true，失败返回false
 */
bool (*semaphore_destroy)(CN_SemaphoreHandle_t semaphore);

/**
 * @brief 等待信号量
 * 
 * @param semaphore 信号量句柄
 * @param timeout_ms 超时时间（毫秒），0表示无限等待
 * @return 等待成功返回true，超时返回false
 */
bool (*semaphore_wait)(CN_SemaphoreHandle_t semaphore, uint32_t timeout_ms);

/**
 * @brief 释放信号量
 * 
 * @param semaphore 信号量句柄
 * @return 释放成功返回true，失败返回false
 */
bool (*semaphore_post)(CN_SemaphoreHandle_t semaphore);
```

#### 3.6 条件变量

```c
/**
 * @brief 创建条件变量
 * 
 * @return 条件变量句柄，失败返回NULL
 */
CN_ConditionHandle_t (*condition_create)(void);

/**
 * @brief 销毁条件变量
 * 
 * @param condition 条件变量句柄
 * @return 销毁成功返回true，失败返回false
 */
bool (*condition_destroy)(CN_ConditionHandle_t condition);

/**
 * @brief 等待条件变量
 * 
 * @param condition 条件变量句柄
 * @param mutex 互斥锁句柄
 * @return 等待成功返回true，失败返回false
 */
bool (*condition_wait)(CN_ConditionHandle_t condition, CN_MutexHandle_t mutex);

/**
 * @brief 带超时的条件变量等待
 * 
 * @param condition 条件变量句柄
 * @param mutex 互斥锁句柄
 * @param timeout_ms 超时时间（毫秒）
 * @return 等待成功返回true，超时返回false
 */
bool (*condition_timed_wait)(CN_ConditionHandle_t condition, 
                             CN_MutexHandle_t mutex, uint32_t timeout_ms);

/**
 * @brief 通知一个等待条件变量的线程
 * 
 * @param condition 条件变量句柄
 * @return 通知成功返回true，失败返回false
 */
bool (*condition_signal)(CN_ConditionHandle_t condition);

/**
 * @brief 通知所有等待条件变量的线程
 * 
 * @param condition 条件变量句柄
 * @return 通知成功返回true，失败返回false
 */
bool (*condition_broadcast)(CN_ConditionHandle_t condition);
```

#### 3.7 原子操作

```c
/**
 * @brief 原子递增
 * 
 * @param value 要递增的值
 * @return 递增后的值
 */
int32_t (*atomic_increment)(volatile int32_t* value);

/**
 * @brief 原子递减
 * 
 * @param value 要递减的值
 * @return 递减后的值
 */
int32_t (*atomic_decrement)(volatile int32_t* value);

/**
 * @brief 原子加法
 * 
 * @param value 要加的值
 * @param addend 加数
 * @return 加法后的值
 */
int32_t (*atomic_add)(volatile int32_t* value, int32_t addend);

/**
 * @brief 原子比较交换
 * 
 * @param dest 目标值
 * @param exchange 要交换的值
 * @param comparand 比较值
 * @return 原始值
 */
int32_t (*atomic
