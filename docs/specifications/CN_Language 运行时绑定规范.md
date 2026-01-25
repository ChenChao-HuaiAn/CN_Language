# CN_Language 运行时绑定规范

本文档明确了 CN_Language 语言特性与运行时库（Runtime）之间的绑定关系。

## 1. 核心类型绑定

| 语言类型 | C 映射类型 | 运行时依赖 | 说明 |
| :--- | :--- | :--- | :--- |
| `整数` (int) | `long long` | 无 | 基础算术运算由 C 编译器直接支持 |
| `小数` (float) | `double` | 无 | 64位双精度浮点数，支持与整数混合运算 |
| `布尔` (bool) | `_Bool` | 无 | C99 布尔类型 |
| `字符串` (string) | `char*` | `cn_rt_string_*` | 字符串为不可变的字节序列（UTF-8） |
| `数组` (array) | `CnArray*` | `cn_rt_array_*` | 运行时数组包含长度元数据，见下详 |

## 2. 内置函数映射

| 语言内置函数 | 运行时 C 函数 | 作用 |
| :--- | :--- | :--- |
| `打印(内容)` | `cn_rt_print_string` | 打印字符串（目前仅支持字符串） |
| `长度(对象)` | `cn_rt_string_length` 或 `cn_rt_array_length` | 返回字符串字符数或数组元素个数 |

## 3. 内存管理绑定

### 3.1 数组分配与释放

**数组类型语法支持**：
- 显式类型声明：`数组 整数 a = [1, 2, 3];`
- 显式类型声明（元素类型可选）：`数组 a = [1, 2, 3];`
- 类型推断：`变量 a = [1, 2, 3];`

**运行时表示**：

数组对应运行时结构体 `CnArray`：

```c
typedef struct CnArray {
    void *data;           // 数据指针
    size_t length;        // 数组长度（元素个数）
    size_t capacity;      // 容量（分配的元素个数）
    size_t element_size;  // 元素大小（字节）
} CnArray;
```

**运行时函数绑定**：

| 语言操作 | 运行时 C 函数 | 功能说明 |
| :--- | :--- | :--- |
| `数组 整数 a = [1, 2, 3];` | `cn_rt_array_new(size_t element_size, size_t length)` | 创建数组 |
| `a[i]` (读取) | `cn_rt_array_get(CnArray *arr, size_t index)` | 获取元素（带边界检查） |
| `a[i] = 值` (写入) | `cn_rt_array_set(CnArray *arr, size_t index, void *value)` | 设置元素（带边界检查） |
| `长度(a)` | `cn_rt_array_length(CnArray *arr)` | 获取数组长度 |
| 作用域结束 | `cn_rt_array_free(CnArray *arr)` | 释放数组内存 |

**操作详织**：
- **创建**：`变量 a = [1, 2, 3];` 映射为 `cn_rt_array_new`。
- **释放**：超出作用域或显式销毁时映射为 `cn_rt_array_free`。
- **边界检查**：每次索引访问 `a[i]` 必须在 `cn_rt_array_get`/`cn_rt_array_set` 内部进行边界检查。

**当前实现状态**：
- ✅ 支持一维数组
- ✅ 支持基础类型数组（整数、小数、布尔、字符串）
- ✅ 支持作为函数参数和结构体字段
- ❌ 多维数组需通过嵌套数组实现

### 3.2 字符串操作
- **拼接**：`"A" + "B"` 映射为 `cn_rt_string_concat`。
- **字面量**：源文件中的字符串字面量直接映射为 C 字符串常量。

## 4. 程序生命周期
- 每个生成的 C 程序 `main` 函数必须：
  1. 首先调用 `cn_rt_init()`。
  2. 在返回前调用 `cn_rt_exit()`。
