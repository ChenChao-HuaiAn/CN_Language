### CN_Language 运行时接口 Freestanding 模式分类

本文档详细说明了 CN_Language 运行时库在 freestanding 模式下的接口分类与支持策略。

---

## 一、分类标准

- **[FS]** - Freestanding 必需：在 freestanding 模式下必须可用的核心接口
- **[OPTIONAL]** - 可选：可通过条件编译宏禁用的功能
- **[HOSTED]** - 仅宿主模式：仅在宿主环境下可用，freestanding 模式下不包含

---

## 二、条件编译宏定义

### 全局模式控制
- `CN_FREESTANDING`: 定义后启用 freestanding 模式，禁用所有 [HOSTED] 接口

### 功能级控制
- `CN_RT_NO_PRINT`: 禁用打印函数（需要用户自定义输出实现）
- `CN_RT_NO_MEMORY_STATS`: 禁用内存统计功能

---

## 三、接口分类详情

### 3.1 运行时生命周期管理 [FS]

**文件**: `include/cnlang/runtime/runtime.h`

```c
void cn_rt_init(void);   // 初始化运行时
void cn_rt_exit(void);   // 清理运行时资源
```

**说明**: 必须在所有模式下可用，freestanding 下可简化实现。

---

### 3.2 基础打印函数 [OPTIONAL]

**文件**: `include/cnlang/runtime/runtime.h`

```c
#ifndef CN_RT_NO_PRINT
void cn_rt_print_int(long long val);
void cn_rt_print_bool(int val);
void cn_rt_print_string(const char *str);
void cn_rt_print_newline();
#endif
```

**说明**: 
- 在宿主模式下默认使用 `printf` 实现
- 在 freestanding 模式下，用户需提供自定义输出回调（如串口输出）
- 通过 `CN_RT_NO_PRINT` 宏可完全禁用

---

### 3.3 字符串核心支持 [FS]

**文件**: `include/cnlang/runtime/runtime.h`

```c
char* cn_rt_string_concat(const char *a, const char *b);
size_t cn_rt_string_length(const char *str);
```

**说明**: 核心字符串操作，freestanding 模式必须支持。

---

### 3.4 字符串高级操作 [HOSTED]

**文件**: `include/cnlang/runtime/runtime.h`

```c
#ifndef CN_FREESTANDING
char* cn_rt_string_substring(const char *str, size_t start, size_t length);
int cn_rt_string_compare(const char *a, const char *b);
int cn_rt_string_index_of(const char *str, const char *target);
char* cn_rt_string_trim(const char *str);
char* cn_rt_string_format(const char *format, ...);
#endif
```

**说明**: 高级字符串操作仅在宿主模式下可用。

---

### 3.5 数组支持函数 [FS]

**文件**: `include/cnlang/runtime/runtime.h`

```c
void* cn_rt_array_alloc(size_t elem_size, size_t count);
size_t cn_rt_array_length(void *arr);
int cn_rt_array_bounds_check(void *arr, size_t index);
void cn_rt_array_free(void *arr);
int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size);
void* cn_rt_array_get_element(void *arr, size_t index, size_t elem_size);
```

**说明**: 核心数组操作，freestanding 模式必须支持。

---

### 3.6 内存管理 [FS]

**文件**: `include/cnlang/runtime/memory.h`

#### 3.6.1 自定义分配器接口 [FS]

```c
typedef void* (*CnRtAllocFunc)(size_t size, void* user_data);
typedef void  (*CnRtFreeFunc)(void* ptr, void* user_data);
typedef void* (*CnRtReallocFunc)(void* ptr, size_t new_size, void* user_data);

typedef struct {
    CnRtAllocFunc alloc_func;
    CnRtFreeFunc free_func;
    CnRtReallocFunc realloc_func;
    void* user_data;
} CnRtMemoryConfig;

void cn_rt_memory_init(CnRtMemoryConfig* config);
```

**说明**: 
- Freestanding 模式下**必须**通过 `cn_rt_memory_init` 提供自定义分配器
- 宿主模式下可传入 `NULL` 使用默认 `malloc/free`

#### 3.6.2 基础分配接口 [FS]

```c
void* cn_rt_malloc(size_t size);
void* cn_rt_calloc(size_t count, size_t size);
void* cn_rt_realloc(void* ptr, size_t new_size);
void  cn_rt_free(void* ptr);
char* cn_rt_string_dup(const char* str);
```

**说明**: 所有模式下必须可用，实现依赖 `CnRtMemoryConfig`。

#### 3.6.3 内存统计 [OPTIONAL]

```c
#ifndef CN_RT_NO_MEMORY_STATS
typedef struct {
    uint64_t current_allocated_bytes;
    uint64_t total_allocated_bytes;
    uint64_t allocation_count;
    uint64_t free_count;
} CnRtMemoryStats;

void cn_rt_memory_get_stats(CnRtMemoryStats* stats);
void cn_rt_memory_dump_stats(void);
#endif
```

**说明**: 
- 调试与统计功能，可通过 `CN_RT_NO_MEMORY_STATS` 禁用
- Freestanding 模式下建议禁用以减小代码体积

---

### 3.7 I/O 操作 [HOSTED]

**文件**: `include/cnlang/runtime/io.h`

```c
#ifndef CN_FREESTANDING
char* cn_rt_read_line(void);
int   cn_rt_read_int(long long* out_val);

typedef void* CnRtFile;
CnRtFile cn_rt_file_open(const char* path, const char* mode);
void     cn_rt_file_close(CnRtFile file);
size_t   cn_rt_file_read(CnRtFile file, void* buffer, size_t size);
size_t   cn_rt_file_write(CnRtFile file, const void* buffer, size_t size);
int      cn_rt_file_eof(CnRtFile file);
#endif
```

**说明**: 
- 所有 I/O 接口仅在宿主模式下可用
- Freestanding 模式下完全不包含此模块

---

### 3.8 数学函数 [FS]

**文件**: `include/cnlang/runtime/math.h`

```c
// 整数数学
long long cn_rt_abs(long long val);
long long cn_rt_min(long long a, long long b);
long long cn_rt_max(long long a, long long b);

// 浮点数学（需要硬件支持或软件实现）
double cn_rt_pow(double base, double exp);
double cn_rt_sqrt(double val);
```

**说明**: 
- 整数数学函数在所有模式下可用
- 浮点函数在 freestanding 模式下需要：
  - 硬件 FPU 支持，或
  - 软件浮点库（如 compiler-rt）

---

## 四、Freestanding 模式使用指南

### 4.1 编译选项

```bash
# 启用 freestanding 模式
cnc --freestanding --target=x86_64-elf program.cn -o kernel.elf

# 禁用打印和统计功能
gcc -DCN_FREESTANDING -DCN_RT_NO_PRINT -DCN_RT_NO_MEMORY_STATS ...
```

### 4.2 自定义内存分配器示例

```c
// OS 内核侧实现
void* kernel_alloc(size_t size, void* user_data) {
    return my_kernel_heap_alloc(size);
}

void kernel_free(void* ptr, void* user_data) {
    my_kernel_heap_free(ptr);
}

void* kernel_realloc(void* ptr, size_t new_size, void* user_data) {
    return my_kernel_heap_realloc(ptr, new_size);
}

// 初始化运行时
void kernel_main() {
    CnRtMemoryConfig mem_config = {
        .alloc_func = kernel_alloc,
        .free_func = kernel_free,
        .realloc_func = kernel_realloc,
        .user_data = NULL
    };
    
    cn_rt_memory_init(&mem_config);
    cn_rt_init();
    
    // ... 运行 CN Language 代码 ...
    
    cn_rt_exit();
}
```

### 4.3 自定义输出回调示例

```c
// 提供串口输出实现
void cn_rt_print_string(const char *str) {
    while (*str) {
        serial_write_byte(*str++);
    }
}

void cn_rt_print_int(long long val) {
    char buf[32];
    // 实现整数转字符串
    // ...
    cn_rt_print_string(buf);
}
```

---

## 五、实现状态

| 模块 | Freestanding 支持 | 条件编译 | 状态 |
|------|-----------------|---------|-----|
| 运行时生命周期 | ✅ [FS] | - | ✅ 已实现 |
| 基础打印 | ✅ [OPTIONAL] | `CN_RT_NO_PRINT` | ✅ 已实现 |
| 字符串核心 | ✅ [FS] | - | ✅ 已实现 |
| 字符串高级 | ❌ [HOSTED] | `CN_FREESTANDING` | ✅ 已实现 |
| 数组操作 | ✅ [FS] | - | ✅ 已实现 |
| 内存管理核心 | ✅ [FS] | - | ✅ 已实现 |
| 内存统计 | ✅ [OPTIONAL] | `CN_RT_NO_MEMORY_STATS` | ✅ 已实现 |
| I/O 操作 | ❌ [HOSTED] | `CN_FREESTANDING` | ✅ 已实现 |
| 数学函数 | ✅ [FS] | - | ✅ 已实现 |

---

## 六、后续工作

### 6.1 待实现功能（TODO 2.2）

- [ ] 为 freestanding 模式提供独立的简化实现版本
- [ ] 实现静态内存分配策略（无堆环境）
- [ ] 提供软件浮点库集成方案

### 6.2 待验证场景（TODO 2.3）

- [ ] 无堆环境下的运行模式验证
- [ ] 无文件系统环境下的 I/O 策略
- [ ] 裸机启动测试

---

**最后更新**: 2026-01-22  
**相关文档**: 
- [阶段 5 TODO 列表](./阶段 5：OS 开发支持与 freestanding 模式 TODO 列表（细分版）.md)
- [CN_Language 运行时绑定规范](../../specifications/CN_Language 运行时绑定规范.md)
