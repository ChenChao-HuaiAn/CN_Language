# CN语言的标准库函数

本文档基于 CN_Language 项目实际实现的标准库函数编写，所有函数均已在运行时库中实现。

## 文档说明

- **[FS]**: 表示该函数在 Freestanding 模式下可用
- **[HOSTED]**: 表示该函数仅在宿主模式下可用
- **[OPTIONAL]**: 表示该函数为可选功能

---

## 1. 运行时生命周期管理 [FS]

### 1.1 `cn_rt_init`

**函数签名**:
```c
void cn_rt_init(void);
```

**功能描述**: 初始化运行时系统，包括内存管理、中断系统等。

**参数**: 无

**返回值**: 无

**使用示例**:
```c
cn_rt_init();  // 程序启动时调用
```

---

### 1.2 `cn_rt_exit`

**函数签名**:
```c
void cn_rt_exit(void);
```

**功能描述**: 退出运行时系统，清理资源。

**参数**: 无

**返回值**: 无

**使用示例**:
```c
cn_rt_exit();  // 程序退出前调用
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

**参数**:
- `size`: 要分配的字节数

**返回值**: 指向分配内存的指针，失败返回 NULL

**使用示例**:
```c
int* arr = (int*)分配内存(10 * sizeof(int));
if (arr != NULL) {
    // 使用内存
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

**参数**:
- `count`: 元素个数
- `size`: 每个元素的字节数

**返回值**: 指向分配内存的指针，失败返回 NULL

**使用示例**:
```c
int* arr = (int*)分配清零内存(10, sizeof(int));
// arr 中所有元素已初始化为 0
```

---

#### 2.1.3 `cn_rt_realloc` / `重新分配内存`

**函数签名**:
```c
void* cn_rt_realloc(void* ptr, size_t new_size);
void* 重新分配内存(void* ptr, size_t new_size);  // 中文别名
```

**功能描述**: 改变已分配内存块的大小。

**参数**:
- `ptr`: 原内存指针
- `new_size`: 新的字节数

**返回值**: 指向新内存的指针，失败返回 NULL

**使用示例**:
```c
int* arr = (int*)分配内存(5 * sizeof(int));
arr = (int*)重新分配内存(arr, 10 * sizeof(int));
```

---

#### 2.1.4 `cn_rt_free` / `释放内存`

**函数签名**:
```c
void cn_rt_free(void* ptr);
void 释放内存(void* ptr);  // 中文别名
```

**功能描述**: 释放之前分配的内存块。

**参数**:
- `ptr`: 要释放的内存指针

**返回值**: 无

**使用示例**:
```c
int* arr = (int*)分配内存(10 * sizeof(int));
// 使用 arr...
释放内存(arr);
```

---

### 2.2 内存操作函数

#### 2.2.1 `cn_rt_memcpy` / `复制内存`

**函数签名**:
```c
void* cn_rt_memcpy(void* dest, const void* src, size_t n);
void* 复制内存(void* dest, const void* src, size_t n);  // 中文别名
```

**功能描述**: 复制内存块。

**参数**:
- `dest`: 目标内存
- `src`: 源内存
- `n`: 要复制的字节数

**返回值**: 目标内存指针

**使用示例**:
```c
char src[10] = "Hello";
char dest[10];
复制内存(dest, src, 10);
```

---

#### 2.2.2 `cn_rt_memset` / `设置内存`

**函数签名**:
```c
void* cn_rt_memset(void* s, int c, size_t n);
void* 设置内存(void* s, int c, size_t n);  // 中文别名
```

**功能描述**: 将内存块设置为指定值。

**参数**:
- `s`: 目标内存
- `c`: 设置的值（字节）
- `n`: 设置的字节数

**返回值**: 目标内存指针

**使用示例**:
```c
char buffer[100];
设置内存(buffer, 0, 100);  // 将 buffer 全部清零
```

---

#### 2.2.3 `cn_rt_memmove`

**函数签名**:
```c
void* cn_rt_memmove(void* dest, const void* src, size_t n);
```

**功能描述**: 移动内存块（正确处理重叠）。

**参数**:
- `dest`: 目标内存地址
- `src`: 源内存地址
- `n`: 要移动的字节数

**返回值**: 目标内存指针

**使用示例**:
```c
char buffer[20] = "Hello World";
cn_rt_memmove(buffer + 2, buffer, 10);  // 安全处理重叠
```

---

#### 2.2.4 `cn_rt_memcmp` / `比较内存`

**函数签名**:
```c
int cn_rt_memcmp(const void* s1, const void* s2, size_t n);
int 比较内存(const void* s1, const void* s2, size_t n);  // 中文别名
```

**功能描述**: 比较两个内存块。

**参数**:
- `s1`: 第一个内存块
- `s2`: 第二个内存块
- `n`: 比较的字节数

**返回值**: 0表示相等，<0表示s1<s2，>0表示s1>s2

**使用示例**:
```c
char buf1[10] = "Hello";
char buf2[10] = "World";
int result = 比较内存(buf1, buf2, 5);
```

---

### 2.3 内存安全检查 [FS]

#### 2.3.1 `cn_rt_memory_set_safety_check`

**函数签名**:
```c
void cn_rt_memory_set_safety_check(int enabled);
```

**功能描述**: 启用/禁用内存安全检查。

**参数**:
- `enabled`: 1启用，0禁用

**返回值**: 无

---

#### 2.3.2 `cn_rt_memory_check_alignment`

**函数签名**:
```c
int cn_rt_memory_check_alignment(uintptr_t addr, size_t alignment);
```

**功能描述**: 检查内存地址是否对齐。

**参数**:
- `addr`: 内存地址
- `alignment`: 对齐要求

**返回值**: 对齐返回1，否则返回0

---

#### 2.3.3 `cn_rt_memory_read_safe`

**函数签名**:
```c
uintptr_t cn_rt_memory_read_safe(uintptr_t addr, size_t size);
```

**功能描述**: 安全地读取内存。

**参数**:
- `addr`: 内存地址
- `size`: 读取大小(1/2/4/8字节)

**返回值**: 读取的值

---

#### 2.3.4 `cn_rt_memory_write_safe`

**函数签名**:
```c
void cn_rt_memory_write_safe(uintptr_t addr, uintptr_t value, size_t size);
```

**功能描述**: 安全地写入内存。

**参数**:
- `addr`: 内存地址
- `value`: 写入的值
- `size`: 写入大小(1/2/4/8字节)

**返回值**: 无

---

#### 2.3.5 `cn_rt_memory_copy_safe`

**函数签名**:
```c
void cn_rt_memory_copy_safe(void* dest, const void* src, size_t size);
```

**功能描述**: 安全地复制内存。

**参数**:
- `dest`: 目标地址
- `src`: 源地址
- `size`: 复制大小

**返回值**: 无

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

**参数**:
- `str`/`s`: 字符串指针

**返回值**: 字符串长度

**使用示例**:
```c
const char* str = "Hello";
size_t len = 获取字符串长度(str);  // len = 5
```

---

#### 3.1.2 `cn_rt_strcpy` / `复制字符串`

**函数签名**:
```c
char* cn_rt_strcpy(char* dest, const char* src);
char* 复制字符串(char* dest, const char* src);  // 中文别名
```

**功能描述**: 将源字符串复制到目标缓冲区。

**参数**:
- `dest`: 目标缓冲区
- `src`: 源字符串

**返回值**: 目标缓冲区指针

**使用示例**:
```c
char dest[20];
复制字符串(dest, "Hello");
```

---

#### 3.1.3 `cn_rt_strncpy` / `限长复制字符串`

**函数签名**:
```c
char* cn_rt_strncpy(char* dest, const char* src, size_t n);
char* 限长复制字符串(char* dest, const char* src, size_t n);  // 中文别名
```

**功能描述**: 复制至多n个字符。

**参数**:
- `dest`: 目标缓冲区
- `src`: 源字符串
- `n`: 最大复制字符数

**返回值**: 目标缓冲区指针

**使用示例**:
```c
char dest[20];
限长复制字符串(dest, "Hello World", 5);  // dest = "Hello"
```

---

#### 3.1.4 `cn_rt_strcat` / `连接字符串`

**函数签名**:
```c
char* cn_rt_strcat(char* dest, const char* src);
char* 连接字符串(char* dest, const char* src);  // 中文别名
```

**功能描述**: 将源字符串追加到目标字符串末尾。

**参数**:
- `dest`: 目标字符串（需有足够空间）
- `src`: 源字符串

**返回值**: 目标字符串指针

**使用示例**:
```c
char dest[20] = "Hello";
连接字符串(dest, " World");  // dest = "Hello World"
```

---

#### 3.1.5 `cn_rt_strncat` / `限长连接字符串`

**函数签名**:
```c
char* cn_rt_strncat(char* dest, const char* src, size_t n);
char* 限长连接字符串(char* dest, const char* src, size_t n);  // 中文别名
```

**功能描述**: 追加至多n个字符。

**参数**:
- `dest`: 目标字符串
- `src`: 源字符串
- `n`: 最大追加字符数

**返回值**: 目标字符串指针

---

#### 3.1.6 `cn_rt_strcmp` / `比较字符串`

**函数签名**:
```c
int cn_rt_strcmp(const char* s1, const char* s2);
int 比较字符串(const char* s1, const char* s2);  // 中文别名
```

**功能描述**: 比较两个字符串。

**参数**:
- `s1`: 第一个字符串
- `s2`: 第二个字符串

**返回值**: 0表示相等，<0表示s1<s2，>0表示s1>s2

**使用示例**:
```c
if (比较字符串("abc", "abc") == 0) {
    // 字符串相等
}
```

---

#### 3.1.7 `cn_rt_strncmp` / `限长比较字符串`

**函数签名**:
```c
int cn_rt_strncmp(const char* s1, const char* s2, size_t n);
int 限长比较字符串(const char* s1, const char* s2, size_t n);  // 中文别名
```

**功能描述**: 比较两个字符串的前n个字符。

**参数**:
- `s1`: 第一个字符串
- `s2`: 第二个字符串
- `n`: 比较的最大字符数

**返回值**: 0表示相等，<0表示s1<s2，>0表示s1>s2

---

### 3.2 字符串查找 [HOSTED]

#### 3.2.1 `查找字符`

**函数签名**:
```c
char* 查找字符(const char* s, int c);
```

**功能描述**: 在字符串中查找字符首次出现的位置。

**参数**:
- `s`: 字符串
- `c`: 要查找的字符

**返回值**: 指向找到的字符的指针，未找到返回NULL

---

#### 3.2.2 `反向查找字符`

**函数签名**:
```c
char* 反向查找字符(const char* s, int c);
```

**功能描述**: 在字符串中查找字符最后出现的位置。

**参数**:
- `s`: 字符串
- `c`: 要查找的字符

**返回值**: 指向找到的字符的指针，未找到返回NULL

---

#### 3.2.3 `查找子串`

**函数签名**:
```c
char* 查找子串(const char* haystack, const char* needle);
```

**功能描述**: 在字符串中查找子字符串。

**参数**:
- `haystack`: 被查找的字符串
- `needle`: 要查找的子串

**返回值**: 指向找到的子串的指针，未找到返回NULL

---

### 3.3 运行时字符串函数 [FS]

#### 3.3.1 `cn_rt_string_concat`

**函数签名**:
```c
char* cn_rt_string_concat(const char* a, const char* b);
```

**功能描述**: 连接两个字符串（分配新内存）。

**参数**:
- `a`: 第一个字符串
- `b`: 第二个字符串

**返回值**: 新分配的连接后的字符串

**使用示例**:
```c
char* result = cn_rt_string_concat("Hello", " World");
// 使用 result...
cn_rt_free(result);
```

---

#### 3.3.2 `cn_rt_string_length`

**函数签名**:
```c
size_t cn_rt_string_length(const char* str);
```

**功能描述**: 获取字符串长度。

**参数**:
- `str`: 字符串

**返回值**: 字符串长度

---

#### 3.3.3 `cn_rt_int_to_string`

**函数签名**:
```c
char* cn_rt_int_to_string(long long val);
```

**功能描述**: 将整数转换为字符串。

**参数**:
- `val`: 整数值

**返回值**: 新分配的字符串

---

#### 3.3.4 `cn_rt_float_to_string`

**函数签名**:
```c
char* cn_rt_float_to_string(double val);
```

**功能描述**: 将浮点数转换为字符串。

**参数**:
- `val`: 浮点数值

**返回值**: 新分配的字符串

---

#### 3.3.5 `cn_rt_bool_to_string`

**函数签名**:
```c
char* cn_rt_bool_to_string(int val);
```

**功能描述**: 将布尔值转换为字符串。

**参数**:
- `val`: 布尔值（0或非0）

**返回值**: "true" 或 "false" 字符串

---

### 3.4 高级字符串操作 [HOSTED]

#### 3.4.1 `cn_rt_string_substring`

**函数签名**:
```c
char* cn_rt_string_substring(const char* str, size_t start, size_t length);
```

**功能描述**: 提取子字符串。

**参数**:
- `str`: 源字符串
- `start`: 起始位置
- `length`: 子串长度

**返回值**: 新分配的子字符串

---

#### 3.4.2 `cn_rt_string_compare`

**函数签名**:
```c
int cn_rt_string_compare(const char* a, const char* b);
```

**功能描述**: 比较两个字符串。

**参数**:
- `a`: 第一个字符串
- `b`: 第二个字符串

**返回值**: 0表示相等，<0表示a<b，>0表示a>b

---

#### 3.4.3 `cn_rt_string_index_of`

**函数签名**:
```c
int cn_rt_string_index_of(const char* str, const char* target);
```

**功能描述**: 查找子串位置。

**参数**:
- `str`: 被查找的字符串
- `target`: 要查找的子串

**返回值**: 子串位置索引，未找到返回-1

---

#### 3.4.4 `cn_rt_string_trim`

**函数签名**:
```c
char* cn_rt_string_trim(const char* str);
```

**功能描述**: 去除字符串首尾空白。

**参数**:
- `str`: 源字符串

**返回值**: 新分配的去除空白后的字符串

---

#### 3.4.5 `cn_rt_string_format`

**函数签名**:
```c
char* cn_rt_string_format(const char* format, ...);
```

**功能描述**: 格式化字符串（类似sprintf）。

**参数**:
- `format`: 格式字符串
- `...`: 可变参数

**返回值**: 新分配的格式化后的字符串

---

## 4. 数组支持函数 [FS]

### 4.1 `cn_rt_array_alloc`

**函数签名**:
```c
void* cn_rt_array_alloc(size_t elem_size, size_t count);
```

**功能描述**: 分配数组内存。

**参数**:
- `elem_size`: 元素大小
- `count`: 元素个数

**返回值**: 指向数组的指针

**使用示例**:
```c
int* arr = (int*)cn_rt_array_alloc(sizeof(int), 10);
```

---

### 4.2 `cn_rt_array_length`

**函数签名**:
```c
size_t cn_rt_array_length(void* arr);
```

**功能描述**: 获取数组长度。

**参数**:
- `arr`: 数组指针

**返回值**: 数组长度

**使用示例**:
```c
int* arr = (int*)cn_rt_array_alloc(sizeof(int), 10);
size_t len = cn_rt_array_length(arr);  // len = 10
```

---

### 4.3 `cn_rt_array_bounds_check`

**函数签名**:
```c
int cn_rt_array_bounds_check(void* arr, size_t index);
```

**功能描述**: 检查数组索引是否越界。

**参数**:
- `arr`: 数组指针
- `index`: 索引

**返回值**: 索引有效返回1，否则返回0

---

### 4.4 `cn_rt_array_free`

**函数签名**:
```c
void cn_rt_array_free(void* arr);
```

**功能描述**: 释放数组内存。

**参数**:
- `arr`: 数组指针

**返回值**: 无

---

### 4.5 `cn_rt_array_set_element`

**函数签名**:
```c
int cn_rt_array_set_element(void* arr, size_t index, const void* element, size_t elem_size);
```

**功能描述**: 设置数组元素。

**参数**:
- `arr`: 数组指针
- `index`: 索引
- `element`: 元素指针
- `elem_size`: 元素大小

**返回值**: 成功返回1，失败返回0

---

### 4.6 `cn_rt_array_get_element`

**函数签名**:
```c
void* cn_rt_array_get_element(void* arr, size_t index, size_t elem_size);
```

**功能描述**: 获取数组元素指针。

**参数**:
- `arr`: 数组指针
- `index`: 索引
- `elem_size`: 元素大小

**返回值**: 指向元素的指针

---

## 5. 输出函数 [OPTIONAL]

### 5.1 `cn_rt_print_int`

**函数签名**:
```c
void cn_rt_print_int(long long val);
```

**功能描述**: 打印整数。

**参数**:
- `val`: 整数值

**返回值**: 无

---

### 5.2 `cn_rt_print_float`

**函数签名**:
```c
void cn_rt_print_float(double val);
```

**功能描述**: 打印浮点数。

**参数**:
- `val`: 浮点数值

**返回值**: 无

---

### 5.3 `cn_rt_print_bool`

**函数签名**:
```c
void cn_rt_print_bool(int val);
```

**功能描述**: 打印布尔值。

**参数**:
- `val`: 布尔值

**返回值**: 无

---

### 5.4 `cn_rt_print_string`

**函数签名**:
```c
void cn_rt_print_string(const char* str);
```

**功能描述**: 打印字符串。

**参数**:
- `str`: 字符串

**返回值**: 无

---

### 5.5 `cn_rt_print_newline`

**函数签名**:
```c
void cn_rt_print_newline(void);
```

**功能描述**: 打印换行符。

**参数**: 无

**返回值**: 无

---

### 5.6 `打印字符串` [HOSTED]

**函数签名**:
```c
int 打印字符串(const char* str);
```

**功能描述**: 输出字符串到标准输出（不含换行）。

**参数**:
- `str`: 要输出的字符串

**返回值**: 成功返回非负数，失败返回EOF

---

### 5.7 `打印行` [HOSTED]

**函数签名**:
```c
int 打印行(const char* str);
```

**功能描述**: 输出字符串到标准输出（含换行）。

**参数**:
- `str`: 要输出的字符串

**返回值**: 成功返回非负数，失败返回EOF

---

### 5.8 `格式化打印` [HOSTED]

**函数签名**:
```c
int 格式化打印(const char* format, ...);
```

**功能描述**: 格式化输出到标准输出。

**参数**:
- `format`: 格式字符串
- `...`: 可变参数

**返回值**: 输出的字符数，失败返回负数

---

## 6. 输入函数 [HOSTED]

### 6.1 通用智能读取

#### 6.1.1 `cn_rt_read` / `读取`

**函数签名**:
```c
CnInputValue cn_rt_read(void);
输入值 读取(void);  // 中文别名
```

**功能描述**: 通用智能读取函数，自动识别输入类型。

**识别规则**:
- 纯数字（无小数点）→ 整数
- 带小数点或科学计数法 → 小数
- 其他 → 字符串

**返回值**: 包含类型和值的输入值结构

**使用示例**:
```c
输入值 val = 读取();
if (是整数(&val)) {
    long long n = 取整数(&val);
    // 处理整数
}
释放输入(&val);
```

---

#### 6.1.2 `cn_rt_input_is_int` / `是整数`

**函数签名**:
```c
int cn_rt_input_is_int(const CnInputValue* val);
int 是整数(const 输入值* val);  // 中文别名
```

**功能描述**: 判断输入值是否为整数类型。

**参数**:
- `val`: 输入值指针

**返回值**: 是整数返回1，否则返回0

---

#### 6.1.3 `cn_rt_input_is_float` / `是小数`

**函数签名**:
```c
int cn_rt_input_is_float(const CnInputValue* val);
int 是小数(const 输入值* val);  // 中文别名
```

**功能描述**: 判断输入值是否为浮点数类型。

**参数**:
- `val`: 输入值指针

**返回值**: 是小数返回1，否则返回0

---

#### 6.1.4 `cn_rt_input_is_string` / `是字符串`

**函数签名**:
```c
int cn_rt_input_is_string(const CnInputValue* val);
int 是字符串(const 输入值* val);  // 中文别名
```

**功能描述**: 判断输入值是否为字符串类型。

**参数**:
- `val`: 输入值指针

**返回值**: 是字符串返回1，否则返回0

---

#### 6.1.5 `cn_rt_input_is_number` / `是数值`

**函数签名**:
```c
int cn_rt_input_is_number(const CnInputValue* val);
int 是数值(const 输入值* val);  // 中文别名
```

**功能描述**: 判断输入值是否为数值类型（整数或小数）。

**参数**:
- `val`: 输入值指针

**返回值**: 是数值返回1，否则返回0

---

#### 6.1.6 `cn_rt_input_to_int` / `取整数`

**函数签名**:
```c
long long cn_rt_input_to_int(const CnInputValue* val);
long long 取整数(const 输入值* val);  // 中文别名
```

**功能描述**: 从输入值获取整数。

**参数**:
- `val`: 输入值指针

**返回值**: 整数值（自动转换）

---

#### 6.1.7 `cn_rt_input_to_float` / `取小数`

**函数签名**:
```c
double cn_rt_input_to_float(const CnInputValue* val);
double 取小数(const 输入值* val);  // 中文别名
```

**功能描述**: 从输入值获取浮点数。

**参数**:
- `val`: 输入值指针

**返回值**: 浮点数值（自动转换）

---

#### 6.1.8 `cn_rt_input_to_string` / `取文本`

**函数签名**:
```c
const char* cn_rt_input_to_string(const CnInputValue* val);
const char* 取文本(const 输入值* val);  // 中文别名
```

**功能描述**: 从输入值获取字符串表示。

**参数**:
- `val`: 输入值指针

**返回值**: 字符串指针

---

#### 6.1.9 `cn_rt_input_free` / `释放输入`

**函数签名**:
```c
void cn_rt_input_free(CnInputValue* val);
void 释放输入(输入值* val);  // 中文别名
```

**功能描述**: 释放输入值占用的资源。

**参数**:
- `val`: 输入值指针

**返回值**: 无

---

### 6.2 特定类型读取

#### 6.2.1 `cn_rt_read_line` / `读取行`

**函数签名**:
```c
char* cn_rt_read_line(void);
char* 读取行(void);  // 中文别名
```

**功能描述**: 从标准输入读取一行（需调用者释放内存）。

**返回值**: 包含读取内容的字符串指针，失败返回NULL

---

#### 6.2.2 `cn_rt_read_int` / `读取整数`

**函数签名**:
```c
int cn_rt_read_int(long long* out_val);
int 读取整数(long long* out_val);  // 中文别名
```

**功能描述**: 从标准输入读取一个整数。

**参数**:
- `out_val`: 输出参数，存储读取的整数

**返回值**: 成功返回1，失败返回0

---

#### 6.2.3 `cn_rt_read_float` / `读取小数`

**函数签名**:
```c
int cn_rt_read_float(double* out_val);
int 读取小数(double* out_val);  // 中文别名
```

**功能描述**: 从标准输入读取一个浮点数。

**参数**:
- `out_val`: 输出参数，存储读取的浮点数

**返回值**: 成功返回1，失败返回0

---

#### 6.2.4 `cn_rt_read_string` / `读取字符串`

**函数签名**:
```c
int cn_rt_read_string(char* buffer, size_t size);
int 读取字符串(char* buffer, size_t size);  // 中文别名
```

**功能描述**: 从标准输入读取字符串到指定缓冲区。

**参数**:
- `buffer`: 目标缓冲区
- `size`: 缓冲区大小

**返回值**: 成功返回1，失败返回0

---

#### 6.2.5 `cn_rt_read_char` / `读取字符`

**函数签名**:
```c
int cn_rt_read_char(char* out_char);
int 读取字符(char* out_char);  // 中文别名
```

**功能描述**: 从标准输入读取一个字符。

**参数**:
- `out_char`: 输出参数，存储读取的字符

**返回值**: 成功返回1，失败返回0（EOF）

---

### 6.3 字符串转换辅助

#### 6.3.1 `cn_rt_str_to_int` / `转整数`

**函数签名**:
```c
long long cn_rt_str_to_int(const char* str);
long long 转整数(const char* str);  // 中文别名
```

**功能描述**: 将字符串转换为整数。

**参数**:
- `str`: 要转换的字符串

**返回值**: 转换后的整数值，转换失败返回0

---

#### 6.3.2 `cn_rt_str_to_float` / `转小数`

**函数签名**:
```c
double cn_rt_str_to_float(const char* str);
double 转小数(const char* str);  // 中文别名
```

**功能描述**: 将字符串转换为浮点数。

**参数**:
- `str`: 要转换的字符串

**返回值**: 转换后的浮点数值，转换失败返回0.0

---

#### 6.3.3 `cn_rt_is_numeric_str` / `是数字文本`

**函数签名**:
```c
int cn_rt_is_numeric_str(const char* str);
int 是数字文本(const char* str);  // 中文别名
```

**功能描述**: 判断字符串是否为有效数字。

**参数**:
- `str`: 要判断的字符串

**返回值**: 是数字返回1，否则返回0

---

#### 6.3.4 `cn_rt_is_int_str` / `是整数文本`

**函数签名**:
```c
int cn_rt_is_int_str(const char* str);
int 是整数文本(const char* str);  // 中文别名
```

**功能描述**: 判断字符串是否为整数格式。

**参数**:
- `str`: 要判断的字符串

**返回值**: 是整数格式返回1，否则返回0

---

## 7. 文件操作 [HOSTED]

### 7.1 `cn_rt_file_open` / `打开文件`

**函数签名**:
```c
CnRtFile cn_rt_file_open(const char* path, const char* mode);
void* 打开文件(const char* path, const char* mode);  // 中文别名
```

**功能描述**: 以指定模式打开文件。

**参数**:
- `path`: 文件路径
- `mode`: 打开模式（"r":读, "w":写, "a":追加, "r+":读写等）

**返回值**: 文件句柄，失败返回NULL

**使用示例**:
```c
void* file = 打开文件("test.txt", "r");
if (file != NULL) {
    // 读取文件
    关闭文件(file);
}
```

---

### 7.2 `cn_rt_file_close` / `关闭文件`

**函数签名**:
```c
void cn_rt_file_close(CnRtFile file);
void 关闭文件(void* file);  // 中文别名
```

**功能描述**: 关闭已打开的文件。

**参数**:
- `file`: 文件句柄

**返回值**: 无

---

### 7.3 `cn_rt_file_read` / `读取文件`

**函数签名**:
```c
size_t cn_rt_file_read(CnRtFile file, void* buffer, size_t size);
size_t 读取文件(void* file, void* buffer, size_t size);  // 中文别名
```

**功能描述**: 从文件读取数据到缓冲区。

**参数**:
- `file`: 文件句柄
- `buffer`: 目标缓冲区
- `size`: 要读取的字节数

**返回值**: 实际读取的字节数

---

### 7.4 `cn_rt_file_write` / `写入文件`

**函数签名**:
```c
size_t cn_rt_file_write(CnRtFile file, const void* buffer, size_t size);
size_t 写入文件(void* file, const void* buffer, size_t size);  // 中文别名
```

**功能描述**: 将缓冲区数据写入文件。

**参数**:
- `file`: 文件句柄
- `buffer`: 源数据缓冲区
- `size`: 要写入的字节数

**返回值**: 实际写入的字节数

---

### 7.5 `cn_rt_file_eof` / `判断文件结束`

**函数签名**:
```c
int cn_rt_file_eof(CnRtFile file);
int 判断文件结束(void* file);  // 中文别名
```

**功能描述**: 检查是否到达文件末尾。

**参数**:
- `file`: 文件句柄

**返回值**: 到达末尾返回非零值，否则返回0

---

### 7.6 `文件定位` [HOSTED]

**函数签名**:
```c
int 文件定位(void* file, long offset, int whence);
```

**功能描述**: 设置文件读写位置。

**参数**:
- `file`: 文件句柄
- `offset`: 偏移量
- `whence`: 起始位置（0:文件开头, 1:当前位置, 2:文件末尾）

**返回值**: 成功返回0，失败返回非零值

---

### 7.7 `获取文件位置` [HOSTED]

**函数签名**:
```c
long 获取文件位置(void* file);
```

**功能描述**: 获取当前文件读写位置。

**参数**:
- `file`: 文件句柄

**返回值**: 当前位置偏移量，失败返回-1

---

### 7.8 `刷新文件缓冲` [HOSTED]

**函数签名**:
```c
int 刷新文件缓冲(void* file);
```

**功能描述**: 将缓冲区数据立即写入文件。

**参数**:
- `file`: 文件句柄

**返回值**: 成功返回0，失败返回EOF

---

### 7.9 `cn_rt_file_setbuf` [HOSTED]

**函数签名**:
```c
int cn_rt_file_setbuf(CnRtFile file, char* buffer, int mode, size_t size);
```

**功能描述**: 设置文件缓冲区。

**参数**:
- `file`: 文件句柄
- `buffer`: 缓冲区指针（为NULL则使用默认缓冲）
- `mode`: 缓冲模式（0:全缓冲, 1:行缓冲, 2:无缓冲）
- `size`: 缓冲区大小

**返回值**: 成功返回0，失败返回非零值

---

### 7.10 `cn_rt_flush_all` / `刷新输出` [HOSTED]

**函数签名**:
```c
int cn_rt_flush_all(void);
int 刷新输出(void);  // 中文别名（用于标准输出）
```

**功能描述**: 刷新所有打开的文件流缓冲区。

**返回值**: 成功返回0，失败返回EOF

---

## 8. 数学函数 [FS]

### 8.1 `cn_rt_abs` / `获取绝对值`

**函数签名**:
```c
long long cn_rt_abs(long long val);
long long 获取绝对值(long long n);  // 中文别名
```

**功能描述**: 返回整数的绝对值。

**参数**:
- `val`/`n`: 整数

**返回值**: 绝对值

**使用示例**:
```c
long long result = 获取绝对值(-10);  // result = 10
```

---

### 8.2 `cn_rt_min` / `求最小值`

**函数签名**:
```c
long long cn_rt_min(long long a, long long b);
long long 求最小值(long long a, long long b);  // 中文别名
```

**功能描述**: 返回两个整数中的较小值。

**参数**:
- `a`: 第一个整数
- `b`: 第二个整数

**返回值**: 较小值

---

### 8.3 `cn_rt_max` / `求最大值`

**函数签名**:
```c
long long cn_rt_max(long long a, long long b);
long long 求最大值(long long a, long long b);  // 中文别名
```

**功能描述**: 返回两个整数中的较大值。

**参数**:
- `a`: 第一个整数
- `b`: 第二个整数

**返回值**: 较大值

---

### 8.4 `cn_rt_pow`

**函数签名**:
```c
double cn_rt_pow(double base, double exp);
```

**功能描述**: 计算幂运算（base^exp）。

**参数**:
- `base`: 底数
- `exp`: 指数

**返回值**: 幂运算结果

**使用示例**:
```c
double result = cn_rt_pow(2.0, 3.0);  // result = 8.0
```

---

### 8.5 `cn_rt_sqrt`

**函数签名**:
```c
double cn_rt_sqrt(double val);
```

**功能描述**: 计算平方根。

**参数**:
- `val`: 数值

**返回值**: 平方根

**使用示例**:
```c
double result = cn_rt_sqrt(9.0);  // result = 3.0
```

---

## 9. 中断处理 [HOSTED]

### 9.1 `cn_rt_interrupt_init`

**函数签名**:
```c
void cn_rt_interrupt_init(void);
```

**功能描述**: 初始化中断管理系统。

**参数**: 无

**返回值**: 无

---

### 9.2 `cn_rt_interrupt_register`

**函数签名**:
```c
int cn_rt_interrupt_register(uint32_t vector, CnRtInterruptHandler handler, const char* name);
```

**功能描述**: 注册中断服务程序。

**参数**:
- `vector`: 中断向量号
- `handler`: 中断处理函数
- `name`: 中断名称（用于调试）

**返回值**: 成功返回0，失败返回负数

**使用示例**:
```c
void timer_handler(void) {
    // 处理定时器中断
}
cn_rt_interrupt_register(0, timer_handler, "Timer");
```

---

### 9.3 `cn_rt_interrupt_unregister`

**函数签名**:
```c
int cn_rt_interrupt_unregister(uint32_t vector);
```

**功能描述**: 注销中断服务程序。

**参数**:
- `vector`: 中断向量号

**返回值**: 成功返回0，失败返回负数

---

### 9.4 `cn_rt_interrupt_enable`

**函数签名**:
```c
void cn_rt_interrupt_enable(uint32_t vector);
```

**功能描述**: 启用特定中断。

**参数**:
- `vector`: 中断向量号

**返回值**: 无

---

### 9.5 `cn_rt_interrupt_disable`

**函数签名**:
```c
void cn_rt_interrupt_disable(uint32_t vector);
```

**功能描述**: 禁用特定中断。

**参数**:
- `vector`: 中断向量号

**返回值**: 无

---

### 9.6 `cn_rt_interrupt_enable_all`

**函数签名**:
```c
void cn_rt_interrupt_enable_all(void);
```

**功能描述**: 全局启用所有中断。

**参数**: 无

**返回值**: 无

---

### 9.7 `cn_rt_interrupt_disable_all`

**函数签名**:
```c
void cn_rt_interrupt_disable_all(void);
```

**功能描述**: 全局禁用所有中断。

**参数**: 无

**返回值**: 无

---

### 9.8 `cn_rt_interrupt_trigger`

**函数签名**:
```c
void cn_rt_interrupt_trigger(uint32_t vector);
```

**功能描述**: 触发中断（用于测试或软件中断）。

**参数**:
- `vector`: 中断向量号

**返回值**: 无

---

## 10. 原子操作与同步 [FS]

### 10.1 原子操作 - 32位

#### 10.1.1 `cn_rt_atomic32_init`

**函数签名**:
```c
void cn_rt_atomic32_init(CnAtomic32* atomic, int32_t value);
```

**功能描述**: 初始化原子变量。

**参数**:
- `atomic`: 原子变量指针
- `value`: 初始值

**返回值**: 无

---

#### 10.1.2 `cn_rt_atomic32_load`

**函数签名**:
```c
int32_t cn_rt_atomic32_load(CnAtomic32* atomic, CnMemoryOrder order);
```

**功能描述**: 原子读取。

**参数**:
- `atomic`: 原子变量指针
- `order`: 内存顺序

**返回值**: 当前值

---

#### 10.1.3 `cn_rt_atomic32_store`

**函数签名**:
```c
void cn_rt_atomic32_store(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);
```

**功能描述**: 原子写入。

**参数**:
- `atomic`: 原子变量指针
- `value`: 新值
- `order`: 内存顺序

**返回值**: 无

---

#### 10.1.4 `cn_rt_atomic32_exchange`

**函数签名**:
```c
int32_t cn_rt_atomic32_exchange(CnAtomic32* atomic, int32_t new_value, CnMemoryOrder order);
```

**功能描述**: 原子交换（返回旧值）。

**参数**:
- `atomic`: 原子变量指针
- `new_value`: 新值
- `order`: 内存顺序

**返回值**: 旧值

---

#### 10.1.5 `cn_rt_atomic32_compare_exchange`

**函数签名**:
```c
int cn_rt_atomic32_compare_exchange(CnAtomic32* atomic, int32_t* expected, 
                                     int32_t desired, CnMemoryOrder order);
```

**功能描述**: 原子比较并交换（CAS）。

**参数**:
- `atomic`: 原子变量指针
- `expected`: 期望值指针
- `desired`: 目标值
- `order`: 内存顺序

**返回值**: 如果 *atomic == expected，则设置 *atomic = desired，返回1；否则设置 *expected = *atomic，返回0

---

#### 10.1.6 `cn_rt_atomic32_fetch_add`

**函数签名**:
```c
int32_t cn_rt_atomic32_fetch_add(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);
```

**功能描述**: 原子加法（返回旧值）。

**参数**:
- `atomic`: 原子变量指针
- `value`: 要加的值
- `order`: 内存顺序

**返回值**: 旧值

---

#### 10.1.7 `cn_rt_atomic32_fetch_sub`

**函数签名**:
```c
int32_t cn_rt_atomic32_fetch_sub(CnAtomic32* atomic, int32_t value, CnMemoryOrder order);
```

**功能描述**: 原子减法（返回旧值）。

**参数**:
- `atomic`: 原子变量指针
- `value`: 要减的值
- `order`: 内存顺序

**返回值**: 旧值

---

### 10.2 互斥锁

#### 10.2.1 `cn_rt_mutex_init`

**函数签名**:
```c
void cn_rt_mutex_init(CnMutex* mutex);
```

**功能描述**: 初始化互斥锁。

**参数**:
- `mutex`: 互斥锁指针

**返回值**: 无

---

#### 10.2.2 `cn_rt_mutex_lock`

**函数签名**:
```c
void cn_rt_mutex_lock(CnMutex* mutex);
```

**功能描述**: 加锁（阻塞直到获取锁）。

**参数**:
- `mutex`: 互斥锁指针

**返回值**: 无

---

#### 10.2.3 `cn_rt_mutex_trylock`

**函数签名**:
```c
int cn_rt_mutex_trylock(CnMutex* mutex);
```

**功能描述**: 尝试加锁（非阻塞）。

**参数**:
- `mutex`: 互斥锁指针

**返回值**: 成功返回1，失败返回0

---

#### 10.2.4 `cn_rt_mutex_unlock`

**函数签名**:
```c
void cn_rt_mutex_unlock(CnMutex* mutex);
```

**功能描述**: 解锁。

**参数**:
- `mutex`: 互斥锁指针

**返回值**: 无

---

### 10.3 自旋锁

#### 10.3.1 `cn_rt_spinlock_init`

**函数签名**:
```c
void cn_rt_spinlock_init(CnSpinlock* lock);
```

**功能描述**: 初始化自旋锁。

**参数**:
- `lock`: 自旋锁指针

**返回值**: 无

---

#### 10.3.2 `cn_rt_spinlock_lock`

**函数签名**:
```c
void cn_rt_spinlock_lock(CnSpinlock* lock);
```

**功能描述**: 加锁（自旋等待）。

**参数**:
- `lock`: 自旋锁指针

**返回值**: 无

---

#### 10.3.3 `cn_rt_spinlock_unlock`

**函数签名**:
```c
void cn_rt_spinlock_unlock(CnSpinlock* lock);
```

**功能描述**: 解锁。

**参数**:
- `lock`: 自旋锁指针

**返回值**: 无

---

### 10.4 内存屏障

#### 10.4.1 `cn_rt_memory_barrier`

**函数签名**:
```c
void cn_rt_memory_barrier(void);
```

**功能描述**: 完整内存屏障。

**参数**: 无

**返回值**: 无

---

#### 10.4.2 `cn_rt_compiler_barrier`

**函数签名**:
```c
void cn_rt_compiler_barrier(void);
```

**功能描述**: 编译器屏障。

**参数**: 无

**返回值**: 无

---

## 11. 系统编程 API

### 11.1 `cn_rt_mem_read`

**函数签名**:
```c
static inline uintptr_t cn_rt_mem_read(uintptr_t addr, size_t size);
```

**功能描述**: 读取内存（替代已删除的"读取内存"关键字）。

**参数**:
- `addr`: 内存地址
- `size`: 读取大小(1/2/4/8字节)

**返回值**: 读取的值

---

### 11.2 `cn_rt_mem_write`

**函数签名**:
```c
static inline void cn_rt_mem_write(uintptr_t addr, uintptr_t value, size_t size);
```

**功能描述**: 写入内存（替代已删除的"写入内存"关键字）。

**参数**:
- `addr`: 内存地址
- `value`: 写入的值
- `size`: 写入大小(1/2/4/8字节)

**返回值**: 无

---

### 11.3 `cn_rt_mem_copy`

**函数签名**:
```c
static inline void cn_rt_mem_copy(void* dest, const void* src, size_t size);
```

**功能描述**: 内存复制（替代已删除的"内存复制"关键字）。

**参数**:
- `dest`: 目标地址
- `src`: 源地址
- `size`: 复制大小

**返回值**: 无

---

### 11.4 `cn_rt_mem_set`

**函数签名**:
```c
static inline void cn_rt_mem_set(void* addr, int value, size_t size);
```

**功能描述**: 内存设置（替代已删除的"内存设置"关键字）。

**参数**:
- `addr`: 内存地址
- `value`: 填充值
- `size`: 设置大小

**返回值**: 无

---

### 11.5 `cn_rt_mem_map`

**函数签名**:
```c
static inline void* cn_rt_mem_map(void* addr, size_t size, int prot, int flags);
```

**功能描述**: 映射内存（替代已删除的"映射内存"关键字）。

**参数**:
- `addr`: 建议地址
- `size`: 映射大小
- `prot`: 保护标志（PROT_READ/PROT_WRITE/PROT_EXEC）
- `flags`: 映射标志（MAP_SHARED/MAP_PRIVATE/MAP_ANONYMOUS）

**返回值**: 映射后的地址，失败返回NULL

---

### 11.6 `cn_rt_mem_unmap`

**函数签名**:
```c
static inline int cn_rt_mem_unmap(void* addr, size_t size);
```

**功能描述**: 解除映射（替代已删除的"解除映射"关键字）。

**参数**:
- `addr`: 映射地址
- `size`: 映射大小

**返回值**: 0表示成功，-1表示失败

---

## 12. 格式化字符串 [HOSTED]

### 12.1 `格式化字符串`

**函数签名**:
```c
int 格式化字符串(char* str, const char* format, ...);
```

**功能描述**: 格式化输出到字符串缓冲区。

**参数**:
- `str`: 目标缓冲区
- `format`: 格式字符串
- `...`: 可变参数

**返回值**: 输出的字符数（不含'\0'），失败返回负数

**注意**: 确保缓冲区足够大，建议使用安全格式化字符串

---

### 12.2 `安全格式化字符串`

**函数签名**:
```c
int 安全格式化字符串(char* str, size_t size, const char* format, ...);
```

**功能描述**: 格式化输出到字符串缓冲区（带长度限制）。

**参数**:
- `str`: 目标缓冲区
- `size`: 缓冲区大小
- `format`: 格式字符串
- `...`: 可变参数

**返回值**: 输出的字符数（不含'\0'），失败返回负数

---

## 附录：数据类型定义

### A.1 输入值类型 [HOSTED]

```c
// 输入值类型枚举
typedef enum {
    CN_INPUT_TYPE_NONE = 0,     // 无效/空输入
    CN_INPUT_TYPE_INT,          // 整数类型
    CN_INPUT_TYPE_FLOAT,        // 浮点数类型
    CN_INPUT_TYPE_STRING        // 字符串类型
} CnInputType;

// 通用输入值结构
typedef struct {
    CnInputType type;           // 值的类型
    union {
        long long int_val;      // 整数值
        double float_val;       // 浮点数值
        char* string_val;       // 字符串值（动态分配）
    } data;
} CnInputValue;

// 中文别名
typedef CnInputValue 输入值;
```

### A.2 原子类型 [FS]

```c
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
```

### A.3 同步类型 [FS]

```c
// 互斥锁
typedef struct {
    CnAtomic32 locked;
    uint32_t owner_thread_id;  // 用于调试和死锁检测
    uint32_t lock_count;       // 用于可重入锁
} CnMutex;

// 自旋锁
typedef struct {
    CnAtomic32 locked;
} CnSpinlock;

// 读写锁
typedef struct {
    CnAtomic32 state;  // 正数表示读者数量，-1表示写者持有
    CnAtomic32 waiting_writers;  // 等待的写者数量
} CnRwLock;
```

### A.4 中断类型 [HOSTED]

```c
// 中断处理函数指针类型
typedef void (*CnRtInterruptHandler)(void);

// 中断向量表项
typedef struct {
    uint32_t vector_num;              // 中断向量号
    CnRtInterruptHandler handler;     // 中断服务程序
    const char* name;                 // 中断名称（用于调试）
} CnRtInterruptVectorEntry;

// 中断管理状态
typedef struct {
    CnRtInterruptVectorEntry vectors[32];  // 中断向量表
    uint32_t enabled_mask;                 // 中断使能掩码
    uint32_t pending_mask;                 // 中断挂起掩码
    uint32_t nesting_level;                // 中断嵌套级别
    int global_enabled;                    // 全局中断使能状态
} CnRtInterruptState;
```

### A.5 文件类型 [HOSTED]

```c
// 文件句柄类型
typedef void* CnRtFile;
```

---

## 附录：常量定义

### B.1 中断向量号 [HOSTED]

```c
#define CN_RT_IRQ_TIMER_0        0
#define CN_RT_IRQ_TIMER_1        1
#define CN_RT_IRQ_KEYBOARD       2
#define CN_RT_IRQ_MOUSE          3
#define CN_RT_IRQ_SERIAL         4
#define CN_RT_IRQ_MAX            32
```

### B.2 内存保护标志

```c
#define PROT_NONE   0x0  // 页不可访问
#define PROT_READ   0x1  // 页可读
#define PROT_WRITE  0x2  // 页可写
#define PROT_EXEC   0x4  // 页可执行
```

### B.3 映射标志

```c
#define MAP_SHARED     0x01  // 共享映射
#define MAP_PRIVATE    0x02  // 私有映射
#define MAP_ANONYMOUS  0x20  // 匿名映射
```

---

## 使用示例

### 示例1：基础内存管理

```c
#include <cnrt.h>

int main() {
    // 分配内存
    int* arr = (int*)分配内存(10 * sizeof(int));
    
    if (arr != NULL) {
        // 设置内存
        设置内存(arr, 0, 10 * sizeof(int));
        
        // 使用数组
        for (int i = 0; i < 10; i++) {
            arr[i] = i * 2;
        }
        
        // 释放内存
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
    
    // 连接字符串
    连接字符串(str1, str2);
    
    // 获取长度
    size_t len = 获取字符串长度(str1);
    
    // 打印
    cn_rt_print_string(str1);
    cn_rt_print_newline();
    
    return 0;
}
```

### 示例3：文件读写 [HOSTED]

```c
#include <cnrt.h>

int main() {
    // 打开文件
    void* file = 打开文件("test.txt", "w");
    
    if (file != NULL) {
        const char* data = "Hello, CN Language!";
        
        // 写入文件
        写入文件(file, data, 获取字符串长度(data));
        
        // 刷新缓冲
        刷新文件缓冲(file);
        
        // 关闭文件
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
    
    // 初始化原子变量
    cn_rt_atomic32_init(&counter, 0);
    
    // 原子加法
    for (int i = 0; i < 10; i++) {
        cn_rt_atomic32_fetch_add(&counter, 1, CN_MEMORY_ORDER_SEQ_CST);
    }
    
    // 读取结果
    int32_t result = cn_rt_atomic32_load(&counter, CN_MEMORY_ORDER_SEQ_CST);
    
    cn_rt_print_int(result);  // 输出: 10
    cn_rt_print_newline();
    
    return 0;
}
```

### 示例6：互斥锁 [FS]

```c
#include <cnrt.h>

CnMutex global_mutex;
int shared_data = 0;

void increment_shared_data() {
    cn_rt_mutex_lock(&global_mutex);
    shared_data++;
    cn_rt_mutex_unlock(&global_mutex);
}

int main() {
    cn_rt_mutex_init(&global_mutex);
    
    // 使用共享数据
    for (int i = 0; i < 100; i++) {
        increment_shared_data();
    }
    
    cn_rt_print_int(shared_data);  // 输出: 100
    cn_rt_print_newline();
    
    cn_rt_mutex_destroy(&global_mutex);
    
    return 0;
}
```

---

## 注意事项

1. **内存管理**: 所有通过 `分配内存`、`分配清零内存`、`重新分配内存` 分配的内存必须使用 `释放内存` 释放。

2. **字符串函数**: 字符串操作函数（如 `cn_rt_string_concat`、`cn_rt_int_to_string` 等）返回的字符串需要调用者释放内存。

3. **Freestanding模式**: 标记为 [HOSTED] 的函数在 Freestanding 模式下不可用。在 Freestanding 模式下使用这些函数会导致编译错误。

4. **中断处理**: 中断处理函数仅在宿主模式下可用，适用于操作系统开发和系统编程。

5. **原子操作**: 原子操作和同步原语在两种模式下都可用，但具体实现可能根据平台定制。

6. **文件操作**: 所有文件操作函数仅在宿主模式下可用。打开的文件必须关闭以释放资源。

7. **输入函数**: 通过 `读取()` 函数获取的 `输入值` 结构在使用完毕后必须调用 `释放输入()` 释放资源。

8. **线程安全**: 除非特别说明，标准库函数不保证线程安全。在多线程环境中使用时需要额外的同步机制。

---

## 版本信息

- **文档版本**: 1.0
- **更新日期**: 2026-01-28
- **适用于**: CN_Language 1.0 及以上版本

---

## 参考

- [CN_Language 语言规范草案（核心子集）](./CN_Language%20语言规范草案（核心子集）.md)
- [CN_Language 运行时绑定规范](./CN_Language%20运行时绑定规范.md)
- [CN_Language C 代码风格规范](./CN_Language%20C%20代码风格规范.md)