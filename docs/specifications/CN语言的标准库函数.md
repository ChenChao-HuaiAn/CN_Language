# CN语言的标准库函数

本文档基于 CN_Language 项目实际实现的标准库函数编写，所有函数均已在运行时库中实现。

## 文档说明

- **[FS]**: 表示该函数在 Freestanding 模式下可用
- **[HOSTED]**: 表示该函数仅在宿主模式下可用
- **[OPTIONAL]**: 表示该函数为可选功能

### 中文别名支持

CN语言标准库支持**双轨制命名体系**，每个函数都有英文函数名和中文别名两种调用方式：

1. **英文函数名**：`cn_rt_xxx()` 格式，与C语言运行时兼容
2. **中文别名**：通过宏定义实现，可直接使用中文函数名

**启用/禁用中文别名**：
- 默认启用中文别名
- 定义 `CN_NO_CHINESE_NAMES` 宏可禁用中文别名

---

## 中文函数名汇总表

### 运行时生命周期管理 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_init` | `初始化运行时` | 初始化运行时系统 |
| `cn_rt_exit` | `退出运行时` | 退出运行时系统 |

### 内存管理函数 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_malloc` | `分配内存` | 分配指定字节数的内存块 |
| `cn_rt_calloc` | `分配清零内存` | 分配内存并初始化为0 |
| `cn_rt_realloc` | `重新分配内存` | 改变已分配内存块的大小 |
| `cn_rt_free` | `释放内存` | 释放之前分配的内存块 |
| `cn_rt_memcpy` | `复制内存` | 复制内存块 |
| `cn_rt_memset` | `设置内存` | 将内存块设置为指定值 |
| `cn_rt_memcmp` | `比较内存` | 比较两个内存块 |
| `cn_rt_string_dup` | `复制字符串副本` | 复制字符串副本 |
| `cn_rt_memory_set_safety_check` | `设置内存安全检查` | 启用/禁用内存安全检查 |
| `cn_rt_memory_check_alignment` | `检查内存对齐` | 检查内存地址是否对齐 |
| `cn_rt_memory_check_readable` | `检查内存可读` | 检查内存地址是否可读 |
| `cn_rt_memory_check_writable` | `检查内存可写` | 检查内存地址是否可写 |
| `cn_rt_memory_read_safe` | `安全读取内存` | 安全地读取内存 |
| `cn_rt_memory_write_safe` | `安全写入内存` | 安全地写入内存 |
| `cn_rt_memory_copy_safe` | `安全复制内存` | 安全地复制内存 |
| `cn_rt_memory_set_safe` | `安全设置内存` | 安全地设置内存 |
| `cn_rt_memory_map_safe` | `安全映射内存` | 内存映射（带安全检查） |
| `cn_rt_memory_unmap_safe` | `安全解除映射` | 解除内存映射（带安全检查） |

### 字符串处理函数 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_strlen` | `获取字符串长度` | 返回字符串长度 |
| `cn_rt_strcpy` | `复制字符串` | 复制字符串 |
| `cn_rt_strncpy` | `限长复制字符串` | 复制至多n个字符 |
| `cn_rt_strcat` | `连接字符串` | 连接字符串 |
| `cn_rt_strncat` | `限长连接字符串` | 限长连接字符串 |
| `cn_rt_strcmp` | `比较字符串` | 比较两个字符串 |
| `cn_rt_strncmp` | `限长比较字符串` | 比较前n个字符 |
| `cn_rt_string_concat` | `连接字符串新` | 连接两个字符串（分配新内存） |
| `cn_rt_string_length` | `字符串长度` | 获取字符串长度 |
| `cn_rt_int_to_string` | `整数转字符串` | 整数转字符串 |
| `cn_rt_float_to_string` | `小数转字符串` | 浮点数转字符串 |
| `cn_rt_bool_to_string` | `布尔转字符串` | 布尔值转字符串 |

### 字符串高级操作 [HOSTED]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_string_substring` | `取子串` | 提取子字符串 |
| `cn_rt_string_compare` | `字符串比较` | 比较两个字符串 |
| `cn_rt_string_index_of` | `查找子串位置` | 查找子串位置 |
| `cn_rt_string_trim` | `去除空白` | 去除字符串首尾空白 |
| `cn_rt_string_format` | `格式化字符串新` | 格式化字符串 |

### 数组支持函数 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_array_alloc` | `分配数组` | 分配数组内存 |
| `cn_rt_array_length` | `获取数组长度` | 获取数组长度 |
| `cn_rt_array_bounds_check` | `检查数组越界` | 检查数组索引是否越界 |
| `cn_rt_array_free` | `释放数组` | 释放数组内存 |
| `cn_rt_array_set_element` | `设置数组元素` | 设置数组元素 |
| `cn_rt_array_get_element` | `获取数组元素` | 获取数组元素指针 |

### 输出函数 [OPTIONAL]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_print_int` | `打印整数` | 打印整数 |
| `cn_rt_print_float` | `打印小数` | 打印浮点数 |
| `cn_rt_print_bool` | `打印布尔` | 打印布尔值 |
| `cn_rt_print_string` | `打印字符串` | 打印字符串 |
| `cn_rt_print_newline` | `打印换行` | 打印换行符 |

### 输入函数 [HOSTED]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_read` | `读取` | 通用智能读取函数 |
| `cn_rt_input_is_int` | `是整数` | 判断输入值是否为整数 |
| `cn_rt_input_is_float` | `是小数` | 判断输入值是否为浮点数 |
| `cn_rt_input_is_string` | `是字符串` | 判断输入值是否为字符串 |
| `cn_rt_input_is_number` | `是数值` | 判断输入值是否为数值 |
| `cn_rt_input_to_int` | `取整数` | 从输入值获取整数 |
| `cn_rt_input_to_float` | `取小数` | 从输入值获取浮点数 |
| `cn_rt_input_to_string` | `取文本` | 从输入值获取字符串 |
| `cn_rt_input_free` | `释放输入` | 释放输入值资源 |
| `cn_rt_read_line` | `读取行` | 读取一行 |
| `cn_rt_read_int` | `读取整数` | 读取整数 |
| `cn_rt_read_float` | `读取小数` | 读取浮点数 |
| `cn_rt_read_string` | `读取字符串` | 读取字符串 |
| `cn_rt_read_char` | `读取字符` | 读取字符 |
| `cn_rt_str_to_int` | `转整数` | 字符串转整数 |
| `cn_rt_str_to_float` | `转小数` | 字符串转浮点数 |
| `cn_rt_is_numeric_str` | `是数字文本` | 判断是否为数字 |
| `cn_rt_is_int_str` | `是整数文本` | 判断是否为整数格式 |

### 文件操作 [HOSTED]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_file_open` | `打开文件` | 打开文件 |
| `cn_rt_file_close` | `关闭文件` | 关闭文件 |
| `cn_rt_file_read` | `读取文件` | 读取文件 |
| `cn_rt_file_write` | `写入文件` | 写入文件 |
| `cn_rt_file_eof` | `判断文件结束` | 检查是否到达文件末尾 |
| `cn_rt_file_setbuf` | `设置文件缓冲` | 设置文件缓冲区 |
| `cn_rt_flush_all` | `刷新输出` | 刷新所有缓冲 |

### 数学函数 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_abs` | `获取绝对值` | 返回绝对值 |
| `cn_rt_min` | `求最小值` | 返回较小值 |
| `cn_rt_max` | `求最大值` | 返回较大值 |
| `cn_rt_pow` | `求幂` | 计算幂运算 |
| `cn_rt_sqrt` | `求平方根` | 计算平方根 |

### 中断处理 [HOSTED]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_interrupt_init` | `初始化中断` | 初始化中断管理系统 |
| `cn_rt_interrupt_register` | `注册中断` | 注册中断服务程序 |
| `cn_rt_interrupt_unregister` | `注销中断` | 注销中断服务程序 |
| `cn_rt_interrupt_enable` | `启用中断` | 启用特定中断 |
| `cn_rt_interrupt_disable` | `禁用中断` | 禁用特定中断 |
| `cn_rt_interrupt_enable_all` | `全局启用中断` | 全局启用所有中断 |
| `cn_rt_interrupt_disable_all` | `全局禁用中断` | 全局禁用所有中断 |
| `cn_rt_interrupt_get_state` | `获取中断状态` | 获取中断状态 |
| `cn_rt_interrupt_trigger` | `触发中断` | 触发中断 |
| `cn_rt_interrupt_enter` | `进入中断` | 进入中断服务程序 |
| `cn_rt_interrupt_exit` | `退出中断` | 退出中断服务程序 |
| `cn_rt_interrupt_get_current` | `获取当前中断` | 获取当前中断向量号 |
| `cn_rt_interrupt_is_pending` | `中断是否挂起` | 检查中断是否挂起 |
| `cn_rt_interrupt_is_enabled` | `中断是否启用` | 检查中断是否已启用 |

### 原子操作 - 32位 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_atomic32_init` | `初始化原子32` | 初始化原子变量 |
| `cn_rt_atomic32_load` | `原子读取32` | 原子读取 |
| `cn_rt_atomic32_store` | `原子写入32` | 原子写入 |
| `cn_rt_atomic32_exchange` | `原子交换32` | 原子交换 |
| `cn_rt_atomic32_compare_exchange` | `原子比较交换32` | 原子比较并交换 |
| `cn_rt_atomic32_fetch_add` | `原子加法32` | 原子加法 |
| `cn_rt_atomic32_fetch_sub` | `原子减法32` | 原子减法 |
| `cn_rt_atomic32_fetch_or` | `原子或32` | 原子按位或 |
| `cn_rt_atomic32_fetch_and` | `原子与32` | 原子按位与 |
| `cn_rt_atomic32_fetch_xor` | `原子异或32` | 原子按位异或 |

### 原子操作 - 64位 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_atomic64_init` | `初始化原子64` | 初始化原子变量 |
| `cn_rt_atomic64_load` | `原子读取64` | 原子读取 |
| `cn_rt_atomic64_store` | `原子写入64` | 原子写入 |
| `cn_rt_atomic64_exchange` | `原子交换64` | 原子交换 |
| `cn_rt_atomic64_compare_exchange` | `原子比较交换64` | 原子比较并交换 |
| `cn_rt_atomic64_fetch_add` | `原子加法64` | 原子加法 |
| `cn_rt_atomic64_fetch_sub` | `原子减法64` | 原子减法 |

### 原子操作 - 指针 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_atomic_ptr_init` | `初始化原子指针` | 初始化原子指针变量 |
| `cn_rt_atomic_ptr_load` | `原子读取指针` | 原子读取指针 |
| `cn_rt_atomic_ptr_store` | `原子写入指针` | 原子写入指针 |
| `cn_rt_atomic_ptr_exchange` | `原子交换指针` | 原子交换指针 |
| `cn_rt_atomic_ptr_compare_exchange` | `原子比较交换指针` | 原子比较并交换指针 |

### 同步原语 [FS]

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_memory_barrier` | `内存屏障` | 完整内存屏障 |
| `cn_rt_compiler_barrier` | `编译器屏障` | 编译器屏障 |
| `cn_rt_mutex_init` | `初始化互斥锁` | 初始化互斥锁 |
| `cn_rt_mutex_destroy` | `销毁互斥锁` | 销毁互斥锁 |
| `cn_rt_mutex_lock` | `加互斥锁` | 加锁 |
| `cn_rt_mutex_trylock` | `尝试加互斥锁` | 尝试加锁 |
| `cn_rt_mutex_unlock` | `解互斥锁` | 解锁 |
| `cn_rt_spinlock_init` | `初始化自旋锁` | 初始化自旋锁 |
| `cn_rt_spinlock_destroy` | `销毁自旋锁` | 销毁自旋锁 |
| `cn_rt_spinlock_lock` | `加自旋锁` | 加锁 |
| `cn_rt_spinlock_trylock` | `尝试加自旋锁` | 尝试加锁 |
| `cn_rt_spinlock_unlock` | `解自旋锁` | 解锁 |
| `cn_rt_rwlock_init` | `初始化读写锁` | 初始化读写锁 |
| `cn_rt_rwlock_destroy` | `销毁读写锁` | 销毁读写锁 |
| `cn_rt_rwlock_read_lock` | `读锁定` | 读锁定 |
| `cn_rt_rwlock_try_read_lock` | `尝试读锁定` | 尝试读锁定 |
| `cn_rt_rwlock_read_unlock` | `读解锁` | 读解锁 |
| `cn_rt_rwlock_write_lock` | `写锁定` | 写锁定 |
| `cn_rt_rwlock_try_write_lock` | `尝试写锁定` | 尝试写锁定 |
| `cn_rt_rwlock_write_unlock` | `写解锁` | 写解锁 |
| `cn_rt_get_thread_id` | `获取线程标识` | 获取当前线程ID |
| `cn_rt_cpu_pause` | `CPU暂停` | CPU暂停指令 |

### 系统编程 API

| 英文函数名 | 中文别名 | 功能描述 |
|-----------|---------|---------|
| `cn_rt_mem_read` | `读取内存` | 读取内存 |
| `cn_rt_mem_write` | `写入内存` | 写入内存 |
| `cn_rt_mem_copy` | `内存复制` | 内存复制 |
| `cn_rt_mem_set` | `内存设置` | 内存设置 |
| `cn_rt_mem_map` | `映射内存` | 映射内存 |
| `cn_rt_mem_unmap` | `解除映射` | 解除映射 |
| `cn_rt_inline_asm` | `内联汇编` | 内联汇编执行 |

---

## 1. 运行时生命周期管理 [FS]

### 1.1 `cn_rt_init` / `初始化运行时`

**函数签名**:
```c
void cn_rt_init(void);
void 初始化运行时(void);  // 中文别名
```

**功能描述**: 初始化运行时系统，包括内存管理、中断系统等。

**使用示例**:
```c
初始化运行时();  // 程序启动时调用
```

---

### 1.2 `cn_rt_exit` / `退出运行时`

**函数签名**:
```c
void cn_rt_exit(void);
void 退出运行时(void);  // 中文别名
```

**功能描述**: 退出运行时系统，清理资源。

**使用示例**:
```c
退出运行时();  // 程序退出前调用
```

---

## 2. 内存管理函数 [FS]

### 2.1 基础内存分配

#### 2.1.1 `cn_rt_malloc` / `分配内存`

**函数签名**:
```c
void* cn_rt_malloc(size_t size);
void* 分配内存(size_t size);  // 中文别名
```

**功能描述**: 分配指定字节数的内存块。

**使用示例**:
```c
int* arr = (int*)分配内存(10 * sizeof(int));
if (arr != NULL) {
    释放内存(arr);
}
```

---

#### 2.1.2 `cn_rt_calloc` / `分配清零内存`

**函数签名**:
```c
void* cn_rt_calloc(size_t count, size_t size);
void* 分配清零内存(size_t count, size_t size);  // 中文别名
```

**功能描述**: 分配内存并初始化为0。

---

#### 2.1.3 `cn_rt_realloc` / `重新分配内存`

**函数签名**:
```c
void* cn_rt_realloc(void* ptr, size_t new_size);
void* 重新分配内存(void* ptr, size_t new_size);  // 中文别名
```

**功能描述**: 改变已分配内存块的大小。

---

#### 2.1.4 `cn_rt_free` / `释放内存`

**函数签名**:
```c
void cn_rt_free(void* ptr);
void 释放内存(void* ptr);  // 中文别名
```

**功能描述**: 释放之前分配的内存块。

---

### 2.2 内存操作函数

#### 2.2.1 `cn_rt_memcpy` / `复制内存`

**函数签名**:
```c
void* cn_rt_memcpy(void* dest, const void* src, size_t n);
void* 复制内存(void* dest, const void* src, size_t n);  // 中文别名
```

**功能描述**: 复制内存块。

---

#### 2.2.2 `cn_rt_memset` / `设置内存`

**函数签名**:
```c
void* cn_rt_memset(void* s, int c, size_t n);
void* 设置内存(void* s, int c, size_t n);  // 中文别名
```

**功能描述**: 将内存块设置为指定值。

---

#### 2.2.3 `cn_rt_memcmp` / `比较内存`

**函数签名**:
```c
int cn_rt_memcmp(const void* s1, const void* s2, size_t n);
int 比较内存(const void* s1, const void* s2, size_t n);  // 中文别名
```

**功能描述**: 比较两个内存块。

---

## 3. 字符串处理函数 [FS]

### 3.1 基础字符串操作

#### 3.1.1 `cn_rt_strlen` / `获取字符串长度`

**函数签名**:
```c
size_t cn_rt_strlen(const char* str);
size_t 获取字符串长度(const char* s);  // 中文别名
```

**功能描述**: 返回字符串中的字符数（不含'\0'）。

---

#### 3.1.2 `cn_rt_strcpy` / `复制字符串`

**函数签名**:
```c
char* cn_rt_strcpy(char* dest, const char* src);
char* 复制字符串(char* dest, const char* src);  // 中文别名
```

**功能描述**: 将源字符串复制到目标缓冲区。

---

#### 3.1.3 `cn_rt_strcat` / `连接字符串`

**函数签名**:
```c
char* cn_rt_strcat(char* dest, const char* src);
char* 连接字符串(char* dest, const char* src);  // 中文别名
```

**功能描述**: 将源字符串追加到目标字符串末尾。

---

#### 3.1.4 `cn_rt_strcmp` / `比较字符串`

**函数签名**:
```c
int cn_rt_strcmp(const char* s1, const char* s2);
int 比较字符串(const char* s1, const char* s2);  // 中文别名
```

**功能描述**: 比较两个字符串。

---

### 3.2 运行时字符串函数 [FS]

#### 3.2.1 `cn_rt_string_concat` / `连接字符串新`

**函数签名**:
```c
char* cn_rt_string_concat(const char* a, const char* b);
char* 连接字符串新(const char* a, const char* b);  // 中文别名
```

**功能描述**: 连接两个字符串（分配新内存）。

---

#### 3.2.2 `cn_rt_int_to_string` / `整数转字符串`

**函数签名**:
```c
char* cn_rt_int_to_string(long long val);
char* 整数转字符串(long long val);  // 中文别名
```

**功能描述**: 将整数转换为字符串。

---

## 4. 数组支持函数 [FS]

### 4.1 `cn_rt_array_alloc` / `分配数组`

**函数签名**:
```c
void* cn_rt_array_alloc(size_t elem_size, size_t count);
void* 分配数组(size_t elem_size, size_t count);  // 中文别名
```

**功能描述**: 分配数组内存。

---

### 4.2 `cn_rt_array_length` / `获取数组长度`

**函数签名**:
```c
size_t cn_rt_array_length(void* arr);
size_t 获取数组长度(void* arr);  // 中文别名
```

**功能描述**: 获取数组长度。

---

## 5. 输出函数 [OPTIONAL]

### 5.1 `cn_rt_print_int` / `打印整数`

**函数签名**:
```c
void cn_rt_print_int(long long val);
void 打印整数(long long val);  // 中文别名
```

**功能描述**: 打印整数。

---

### 5.2 `cn_rt_print_string` / `打印字符串`

**函数签名**:
```c
void cn_rt_print_string(const char* str);
void 打印字符串(const char* str);  // 中文别名
```

**功能描述**: 打印字符串。

---

## 6. 输入函数 [HOSTED]

### 6.1 `cn_rt_read` / `读取`

**函数签名**:
```c
CnInputValue cn_rt_read(void);
输入值 读取(void);  // 中文别名
```

**功能描述**: 通用智能读取函数，自动识别输入类型。

**使用示例**:
```c
输入值 val = 读取();
if (是整数(&val)) {
    long long n = 取整数(&val);
}
释放输入(&val);
```

---

## 7. 文件操作 [HOSTED]

### 7.1 `cn_rt_file_open` / `打开文件`

**函数签名**:
```c
CnRtFile cn_rt_file_open(const char* path, const char* mode);
void* 打开文件(const char* path, const char* mode);  // 中文别名
```

**功能描述**: 以指定模式打开文件。

---

### 7.2 `cn_rt_file_close` / `关闭文件`

**函数签名**:
```c
void cn_rt_file_close(CnRtFile file);
void 关闭文件(void* file);  // 中文别名
```

**功能描述**: 关闭已打开的文件。

---

## 8. 数学函数 [FS]

### 8.1 `cn_rt_abs` / `获取绝对值`

**函数签名**:
```c
long long cn_rt_abs(long long val);
long long 获取绝对值(long long n);  // 中文别名
```

**功能描述**: 返回整数的绝对值。

---

### 8.2 `cn_rt_pow` / `求幂`

**函数签名**:
```c
double cn_rt_pow(double base, double exp);
double 求幂(double base, double exp);  // 中文别名
```

**功能描述**: 计算幂运算。

---

### 8.3 `cn_rt_sqrt` / `求平方根`

**函数签名**:
```c
double cn_rt_sqrt(double val);
double 求平方根(double val);  // 中文别名
```

**功能描述**: 计算平方根。

---

## 9. 中断处理 [HOSTED]

### 9.1 `cn_rt_interrupt_init` / `初始化中断`

**函数签名**:
```c
void cn_rt_interrupt_init(void);
void 初始化中断(void);  // 中文别名
```

**功能描述**: 初始化中断管理系统。

---

### 9.2 `cn_rt_interrupt_register` / `注册中断`

**函数签名**:
```c
int cn_rt_interrupt_register(uint32_t vector, CnRtInterruptHandler handler, const char* name);
int 注册中断(uint32_t vector, CnRtInterruptHandler handler, const char* name);  // 中文别名
```

**功能描述**: 注册中断服务程序。

---

## 10. 原子操作与同步 [FS]

### 10.1 `cn_rt_atomic32_init` / `初始化原子32`

**函数签名**:
```c
void cn_rt_atomic32_init(CnAtomic32* atomic, int32_t value);
void 初始化原子32(CnAtomic32* atomic, int32_t value);  // 中文别名
```

**功能描述**: 初始化原子变量。

---

### 10.2 `cn_rt_mutex_init` / `初始化互斥锁`

**函数签名**:
```c
void cn_rt_mutex_init(CnMutex* mutex);
void 初始化互斥锁(CnMutex* mutex);  // 中文别名
```

**功能描述**: 初始化互斥锁。

---

### 10.3 `cn_rt_memory_barrier` / `内存屏障`

**函数签名**:
```c
void cn_rt_memory_barrier(void);
void 内存屏障(void);  // 中文别名
```

**功能描述**: 完整内存屏障。

---

## 11. 系统编程 API

### 11.1 `cn_rt_mem_read` / `读取内存`

**函数签名**:
```c
uintptr_t cn_rt_mem_read(uintptr_t addr, size_t size);
uintptr_t 读取内存(uintptr_t addr, size_t size);  // 中文别名
```

**功能描述**: 读取内存。

---

### 11.2 `cn_rt_mem_write` / `写入内存`

**函数签名**:
```c
void cn_rt_mem_write(uintptr_t addr, uintptr_t value, size_t size);
void 写入内存(uintptr_t addr, uintptr_t value, size_t size);  // 中文别名
```

**功能描述**: 写入内存。

---

## 附录：数据类型定义

### A.1 输入值类型 [HOSTED]

```c
typedef enum {
    CN_INPUT_TYPE_NONE = 0,
    CN_INPUT_TYPE_INT,
    CN_INPUT_TYPE_FLOAT,
    CN_INPUT_TYPE_STRING
} CnInputType;

typedef struct {
    CnInputType type;
    union {
        long long int_val;
        double float_val;
        char* string_val;
    } data;
} CnInputValue;

typedef CnInputValue 输入值;
```

### A.2 原子类型 [FS]

```c
typedef struct { volatile int32_t value; } CnAtomic32;
typedef struct { volatile int64_t value; } CnAtomic64;
typedef struct { void* volatile value; } CnAtomicPtr;

typedef enum {
    CN_MEMORY_ORDER_RELAXED,
    CN_MEMORY_ORDER_ACQUIRE,
    CN_MEMORY_ORDER_RELEASE,
    CN_MEMORY_ORDER_SEQ_CST
} CnMemoryOrder;
```

### A.3 同步类型 [FS]

```c
typedef struct { CnAtomic32 locked; uint32_t owner_thread_id; uint32_t lock_count; } CnMutex;
typedef struct { CnAtomic32 locked; } CnSpinlock;
typedef struct { CnAtomic32 state; CnAtomic32 waiting_writers; } CnRwLock;
```

---

## 使用示例

### 示例1：基础内存管理

```c
#include <cnrt.h>

int main() {
    // 使用中文别名分配内存
    int* arr = (int*)分配内存(10 * sizeof(int));
    
    if (arr != NULL) {
        设置内存(arr, 0, 10 * sizeof(int));
        
        for (int i = 0; i < 10; i++) {
            arr[i] = i * 2;
        }
        
        释放内存(arr);
    }
    
    return 0;
}
```

### 示例2：字符串操作

```c
#include <cnrt.h>

int main() {
    char str1[50] = "Hello";
    char str2[50] = " World";
    
    连接字符串(str1, str2);
    
    size_t len = 获取字符串长度(str1);
    
    打印字符串(str1);
    打印换行();
    
    return 0;
}
```

### 示例3：文件读写 [HOSTED]

```c
#include <cnrt.h>

int main() {
    void* file = 打开文件("test.txt", "w");
    
    if (file != NULL) {
        const char* data = "Hello, CN Language!";
        写入文件(file, data, 获取字符串长度(data));
        刷新文件缓冲(file);
        关闭文件(file);
    }
    
    return 0;
}
```

### 示例4：智能输入 [HOSTED]

```c
#include <cnrt.h>

int main() {
    打印字符串("请输入一个值: ");
    
    输入值 val = 读取();
    
    if (是整数(&val)) {
        long long n = 取整数(&val);
        格式化打印("你输入了整数: %lld\n", n);
    } else if (是小数(&val)) {
        double d = 取小数(&val);
        格式化打印("你输入了小数: %f\n", d);
    } else if (是字符串(&val)) {
        const char* s = 取文本(&val);
        格式化打印("你输入了字符串: %s\n", s);
    }
    
    释放输入(&val);
    
    return 0;
}
```

### 示例5：原子操作 [FS]

```c
#include <cnrt.h>

int main() {
    CnAtomic32 counter;
    
    初始化原子32(&counter, 0);
    
    for (int i = 0; i < 10; i++) {
        原子加法32(&counter, 1, CN_MEMORY_ORDER_SEQ_CST);
    }
    
    int32_t result = 原子读取32(&counter, CN_MEMORY_ORDER_SEQ_CST);
    
    打印整数(result);
    打印换行();
    
    return 0;
}
```

### 示例6：互斥锁 [FS]

```c
#include <cnrt.h>

CnMutex global_mutex;
int shared_data = 0;

void increment_shared_data() {
    加互斥锁(&global_mutex);
    shared_data++;
    解互斥锁(&global_mutex);
}

int main() {
    初始化互斥锁(&global_mutex);
    
    for (int i = 0; i < 100; i++) {
        increment_shared_data();
    }
    
    打印整数(shared_data);
    打印换行();
    
    销毁互斥锁(&global_mutex);
    
    return 0;
}
```

---

## 注意事项

1. **内存管理**: 所有通过 `分配内存`、`分配清零内存`、`重新分配内存` 分配的内存必须使用 `释放内存` 释放。

2. **字符串函数**: 字符串操作函数（如 `连接字符串新`、`整数转字符串` 等）返回的字符串需要调用者释放内存。

3. **Freestanding模式**: 标记为 [HOSTED] 的函数在 Freestanding 模式下不可用。

4. **中断处理**: 中断处理函数仅在宿主模式下可用，适用于操作系统开发和系统编程。

5. **原子操作**: 原子操作和同步原语在两种模式下都可用，但具体实现可能根据平台定制。

6. **文件操作**: 所有文件操作函数仅在宿主模式下可用。打开的文件必须关闭以释放资源。

7. **输入函数**: 通过 `读取()` 函数获取的 `输入值` 结构在使用完毕后必须调用 `释放输入()` 释放资源。

8. **线程安全**: 除非特别说明，标准库函数不保证线程安全。在多线程环境中使用时需要额外的同步机制。

---

## 版本信息

- **文档版本**: 2.0
- **更新日期**: 2026-03-28
- **适用于**: CN_Language 1.0 及以上版本
- **更新内容**: 添加中文函数名汇总表，所有函数标注中文别名

---

## 参考

- [CN_Language 语言规范草案（核心子集）](./CN_Language%20语言规范草案（核心子集）.md)
- [CN_Language 运行时绑定规范](./CN_Language%20运行时绑定规范.md)
- [CN_Language C 代码风格规范](./CN_Language%20C%20代码风格规范.md)
- [plans/008 CN语言标准库全中文函数名实现方案.md](../../plans/008%20CN语言标准库全中文函数名实现方案.md)
