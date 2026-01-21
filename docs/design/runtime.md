# CN_Language 运行时架构设计

## 1. 概述

本文档详细描述 CN_Language 运行时系统的架构设计，包括虚拟机、垃圾回收、内存管理、类型系统等核心组件。

## 2. 运行时整体架构

### 2.1 运行时组件

```
字节码输入
    ↓
[虚拟机 VM]
    ↓
[解释器 Interpreter]
    ↓
[操作数栈 Operand Stack]
    ↓
[调用栈 Call Stack]
    ↓
[垃圾回收器 GC]
    ↓
[内存管理器 Memory Manager]
    ↓
[类型系统 Type System]
    ↓
执行结果输出
```

### 2.2 运行时主结构

```c
// 运行时结构
typedef struct CN_Runtime {
    // 虚拟机
    CN_VM* vm;

    // 垃圾回收器
    CN_GC* gc;

    // 内存管理器
    CN_Memory* memory;

    // 类型系统
    CN_TypeSystem* type_system;

    // 标准库
    CN_StdLib* stdlib;

    // 配置
    CN_RuntimeConfig* config;

    // 状态
    CN_RuntimeState* state;
} CN_Runtime;

// 运行时配置
typedef struct CN_RuntimeConfig {
    size_t stack_size;          // 栈大小
    size_t heap_size;           // 堆大小
    size_t gc_threshold;        // GC 触发阈值
    int enable_gc;              // 是否启用 GC
    int enable_profiling;       // 是否启用性能分析
} CN_RuntimeConfig;

// 运行时状态
typedef struct CN_RuntimeState {
    int is_running;             // 是否正在运行
    int error_count;            // 错误数量
    CN_Value* exception;        // 当前异常
} CN_RuntimeState;
```

## 3. 虚拟机（VM）

### 3.1 虚拟机架构

```
字节码加载
    ↓
[指令解码器 Instruction Decoder]
    ↓
[指令分发器 Instruction Dispatcher]
    ↓
[指令执行器 Instruction Executor]
    ↓
[状态更新 State Update]
    ↓
循环执行
```

### 3.2 虚拟机结构

```c
// 虚拟机结构
typedef struct CN_VM {
    // 字节码
    CN_Bytecode* bytecode;

    // 操作数栈
    CN_Value* stack;
    size_t stack_size;
    size_t stack_top;

    // 调用栈
    CN_CallFrame* call_stack;
    size_t call_stack_size;
    size_t call_stack_top;

    // 全局变量
    CN_Value* globals;
    size_t global_count;

    // 运行时环境
    CN_Runtime* runtime;

    // 当前帧
    CN_CallFrame* current_frame;

    // 配置
    CN_VMConfig* config;
} CN_VM;

// 虚拟机配置
typedef struct CN_VMConfig {
    size_t stack_size;          // 操作数栈大小
    size_t call_stack_size;     // 调用栈大小
    size_t max_call_depth;      // 最大调用深度
} CN_VMConfig;
```

### 3.3 字节码（Bytecode）

```c
// 字节码结构
typedef struct CN_Bytecode {
    unsigned char* code;        // 代码数据
    size_t code_size;          // 代码大小

    CN_ConstantPool* constants; // 常量池
    CN_Function** functions;   // 函数列表
    size_t function_count;     // 函数数量

    CN_DebugInfo* debug_info;  // 调试信息
} CN_Bytecode;

// 常量池
typedef struct CN_ConstantPool {
    CN_Value** constants;      // 常量列表
    size_t constant_count;      // 常量数量
    size_t capacity;           // 容量
} CN_ConstantPool;

// 函数信息
typedef struct CN_Function {
    const char* name;          // 函数名
    unsigned char* code;       // 函数代码
    size_t code_size;         // 代码大小
    size_t param_count;       // 参数数量
    size_t local_count;       // 局部变量数量
    CN_Type* return_type;     // 返回类型
} CN_Function;
```

### 3.4 调用帧（Call Frame）

```c
// 调用帧结构
typedef struct CN_CallFrame {
    CN_Function* function;     // 函数
    unsigned char* ip;         // 指令指针
    CN_Value* locals;          // 局部变量
    size_t stack_base;         // 栈基址
    CN_CallFrame* caller;      // 调用者
} CN_CallFrame;
```

### 3.5 操作数栈（Operand Stack）

```c
// 操作数栈操作
void CN_VMStackPush(CN_VM* vm, CN_Value value);
CN_Value CN_VMStackPop(CN_VM* vm);
CN_Value CN_VMStackPeek(CN_VM* vm, size_t offset);
void CN_VMStackReset(CN_VM* vm, size_t new_top);
```

### 3.6 指令集（Instruction Set）

#### 3.6.1 基本指令

```c
// 指令操作码
typedef enum CN_Instruction {
    // 常量加载
    OP_LOAD_CONST,             // 加载常量
    OP_LOAD_GLOBAL,            // 加载全局变量
    OP_LOAD_LOCAL,             // 加载局部变量
    OP_STORE_GLOBAL,           // 存储全局变量
    OP_STORE_LOCAL,            // 存储局部变量

    // 算术运算
    OP_ADD,                    // 加法
    OP_SUB,                    // 减法
    OP_MUL,                    // 乘法
    OP_DIV,                    // 除法
    OP_MOD,                    // 取模
    OP_NEG,                    // 取负

    // 位运算
    OP_BIT_AND,                // 按位与
    OP_BIT_OR,                 // 按位或
    OP_BIT_XOR,                // 按位异或
    OP_BIT_NOT,                // 按位取反
    OP_SHL,                    // 左移
    OP_SHR,                    // 右移

    // 比较运算
    OP_EQ,                     // 相等
    OP_NE,                     // 不等
    OP_LT,                     // 小于
    OP_GT,                     // 大于
    OP_LE,                     // 小于等于
    OP_GE,                     // 大于等于

    // 逻辑运算
    OP_LOGICAL_AND,            // 逻辑与
    OP_LOGICAL_OR,             // 逻辑或
    OP_LOGICAL_NOT,            // 逻辑非

    // 控制流
    OP_JUMP,                   // 跳转
    OP_JUMP_IF_FALSE,          // 条件跳转
    OP_JUMP_IF_TRUE,           // 条件跳转
    OP_CALL,                   // 函数调用
    OP_RETURN,                 // 返回

    // 类型转换
    OP_CAST_INT_TO_FLOAT,      // 整数转浮点
    OP_CAST_FLOAT_TO_INT,      // 浮点转整数
    OP_CAST_TO_STRING,         // 转字符串

    // 数组操作
    OP_ARRAY_NEW,              // 创建数组
    OP_ARRAY_GET,              // 获取数组元素
    OP_ARRAY_SET,              // 设置数组元素
    OP_ARRAY_LENGTH,           // 获取数组长度

    // 对象操作
    OP_OBJECT_NEW,             // 创建对象
    OP_OBJECT_GET,             // 获取对象属性
    OP_OBJECT_SET,             // 设置对象属性
    OP_OBJECT_METHOD,          // 调用对象方法

    // 其他
    OP_NOP,                    // 空操作
    OP_DUP,                    // 复制栈顶
    OP_POP,                    // 弹出栈顶
    OP_SWAP,                   // 交换栈顶两个元素

    // 调试
    OP_DEBUG_LINE,             // 设置调试行号
    OP_DEBUG_FILE              // 设置调试文件
} CN_Instruction;
```

#### 3.6.2 指令格式

```c
// 指令格式
typedef struct CN_InstructionFormat {
    CN_Instruction opcode;      // 操作码
    CN_OperandType operand1;   // 操作数1类型
    CN_OperandType operand2;   // 操作数2类型
    CN_OperandType operand3;   // 操作数3类型
} CN_InstructionFormat;

// 操作数类型
typedef enum CN_OperandType {
    OPERAND_NONE,              // 无操作数
    OPERAND_IMMEDIATE,         // 立即数
    OPERAND_REGISTER,          // 寄存器
    OPERAND_ADDRESS,           // 地址
    OPERAND_OFFSET             // 偏移量
} CN_OperandType;
```

### 3.7 指令执行（Instruction Execution）

```c
// 指令执行器
typedef int (*CN_InstructionHandler)(CN_VM* vm);

// 指令处理器表
static CN_InstructionHandler CN_INSTRUCTION_HANDLERS[] = {
    [OP_LOAD_CONST] = CN_HandleLoadConst,
    [OP_LOAD_GLOBAL] = CN_HandleLoadGlobal,
    [OP_LOAD_LOCAL] = CN_HandleLoadLocal,
    [OP_STORE_GLOBAL] = CN_HandleStoreGlobal,
    [OP_STORE_LOCAL] = CN_HandleStoreLocal,
    [OP_ADD] = CN_HandleAdd,
    [OP_SUB] = CN_HandleSub,
    [OP_MUL] = CN_HandleMul,
    [OP_DIV] = CN_HandleDiv,
    [OP_MOD] = CN_HandleMod,
    [OP_NEG] = CN_HandleNeg,
    // ... 更多指令处理器
};

// 主执行循环
int CN_VMExecute(CN_VM* vm) {
    while (vm->current_frame != NULL) {
        CN_Instruction opcode = *vm->current_frame->ip++;
        CN_InstructionHandler handler = CN_INSTRUCTION_HANDLERS[opcode];
        if (handler == NULL) {
            CN_ReportError(vm, "Unknown instruction: %d", opcode);
            return -1;
        }
        int result = handler(vm);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}
```

### 3.8 函数调用（Function Call）

```c
// 函数调用处理
int CN_HandleCall(CN_VM* vm) {
    // 读取函数索引
    uint16_t func_index = READ_UINT16(vm->current_frame->ip);
    vm->current_frame->ip += 2;

    // 读取参数数量
    uint8_t arg_count = *vm->current_frame->ip++;

    // 获取函数
    CN_Function* func = vm->bytecode->functions[func_index];

    // 检查调用栈空间
    if (vm->call_stack_top >= vm->call_stack_size) {
        CN_ReportError(vm, "Stack overflow");
        return -1;
    }

    // 创建新帧
    CN_CallFrame* new_frame = &vm->call_stack[vm->call_stack_top++];
    new_frame->function = func;
    new_frame->ip = func->code;
    new_frame->locals = CN_VMStackPeek(vm, arg_count);
    new_frame->stack_base = vm->stack_top - arg_count;
    new_frame->caller = vm->current_frame;

    // 设置当前帧
    vm->current_frame = new_frame;

    return 0;
}

// 返回处理
int CN_HandleReturn(CN_VM* vm) {
    // 获取返回值
    CN_Value return_value = CN_VMStackPop(vm);

    // 恢复调用者帧
    CN_CallFrame* caller = vm->current_frame->caller;
    CN_CallFrame* current = vm->current_frame;

    // 恢复栈
    vm->stack_top = current->stack_base;

    // 压入返回值
    CN_VMStackPush(vm, return_value);

    // 设置当前帧
    vm->current_frame = caller;

    return 0;
}
```

## 4. 垃圾回收（GC）

### 4.1 垃圾回收架构

```
对象分配
    ↓
[分配器 Allocator]
    ↓
[对象池 Object Pool]
    ↓
[根集合 Root Set]
    ↓
[标记阶段 Mark Phase]
    ↓
[清除阶段 Sweep Phase]
    ↓
内存回收
```

### 4.2 垃圾回收器结构

```c
// 垃圾回收器结构
typedef struct CN_GC {
    // 分配器
    CN_Allocator* allocator;

    // 对象列表
    CN_GCObject** objects;
    size_t object_count;
    size_t capacity;

    // 根集合
    CN_Value** roots;
    size_t root_count;
    size_t root_capacity;

    // 统计信息
    CN_GCStats* stats;

    // 配置
    CN_GCConfig* config;

    // 运行时
    CN_Runtime* runtime;
} CN_GC;

// GC 配置
typedef struct CN_GCConfig {
    size_t heap_size;           // 堆大小
    size_t gc_threshold;        // GC 触发阈值
    int enable_generational;    // 是否启用分代回收
    int enable_compaction;      // 是否启用压缩
} CN_GCConfig;

// GC 统计信息
typedef struct CN_GCStats {
    size_t allocated_bytes;     // 已分配字节数
    size_t freed_bytes;         // 已释放字节数
    size_t collection_count;    // 回收次数
    size_t collection_time;     // 回收时间
} CN_GCStats;
```

### 4.3 GC 对象（GC Object）

```c
// GC 对象结构
typedef struct CN_GCObject {
    // 对象头
    size_t size;                // 对象大小
    int marked;                 // 标记位
    CN_GCObject* next;          // 下一个对象
    CN_GCObject* prev;          // 前一个对象

    // 对象类型
    CN_ObjectType type;         // 对象类型

    // 对象数据
    union {
        struct {
            CN_Value* elements;  // 数组元素
            size_t length;      // 数组长度
        } array;
        struct {
            CN_Value* fields;   // 对象字段
            size_t field_count; // 字段数量
        } object;
        struct {
            char* data;         // 字符串数据
            size_t length;      // 字符串长度
        } string;
        struct {
            CN_Value* elements; // 列表元素
            size_t capacity;    // 列表容量
            size_t size;        // 列表大小
        } list;
        struct {
            CN_Value** buckets; // 哈希桶
            size_t bucket_count; // 桶数量
            size_t size;        // 元素数量
        } map;
    };

    // 终结器
    void (*finalizer)(struct CN_GCObject* obj);
} CN_GCObject;

// 对象类型
typedef enum CN_ObjectType {
    OBJECT_ARRAY,               // 数组
    OBJECT_OBJECT,              // 对象
    OBJECT_STRING,              // 字符串
    OBJECT_LIST,                // 列表
    OBJECT_MAP,                 // 映射
    OBJECT_FUNCTION,            // 函数
    OBJECT_CLOSURE,             // 闭包
    OBJECT_CLASS,               // 类
    OBJECT_INSTANCE             // 实例
} CN_ObjectType;
```

### 4.4 对象分配（Object Allocation）

```c
// 对象分配
void* CN_GCAllocate(CN_GC* gc, size_t size, CN_ObjectType type) {
    // 检查是否需要 GC
    if (gc->stats->allocated_bytes + size > gc->config->gc_threshold) {
        CN_GCCollect(gc);
    }

    // 分配对象
    CN_GCObject* obj = (CN_GCObject*)CN_AllocatorAllocate(gc->allocator, size);
    if (obj == NULL) {
        // 内存不足，强制 GC
        CN_GCCollect(gc);
        obj = (CN_GCObject*)CN_AllocatorAllocate(gc->allocator, size);
        if (obj == NULL) {
            return NULL;
        }
    }

    // 初始化对象
    obj->size = size;
    obj->marked = 0;
    obj->type = type;
    obj->next = gc->objects;
    obj->prev = NULL;
    if (gc->objects != NULL) {
        gc->objects->prev = obj;
    }
    gc->objects = obj;

    // 更新统计
    gc->stats->allocated_bytes += size;

    return obj;
}
```

### 4.5 标记阶段（Mark Phase）

```c
// 标记阶段
void CN_GCMark(CN_GC* gc) {
    // 标记根集合
    for (size_t i = 0; i < gc->root_count; i++) {
        CN_MarkValue(gc, gc->roots[i]);
    }

    // 标记虚拟机栈
    CN_MarkVMStack(gc, gc->runtime->vm);

    // 标记全局变量
    CN_MarkGlobals(gc, gc->runtime->vm);
}

// 标记值
void CN_MarkValue(CN_GC* gc, CN_Value value) {
    if (!CN_IsObject(value)) {
        return;
    }

    CN_GCObject* obj = CN_AsObject(value);
    if (obj->marked) {
        return;
    }

    obj->marked = 1;

    // 根据对象类型标记子对象
    switch (obj->type) {
        case OBJECT_ARRAY:
            CN_MarkArray(gc, obj);
            break;
        case OBJECT_OBJECT:
            CN_MarkObject(gc, obj);
            break;
        case OBJECT_STRING:
            // 字符串没有子对象
            break;
        case OBJECT_LIST:
            CN_MarkList(gc, obj);
            break;
        case OBJECT_MAP:
            CN_MarkMap(gc, obj);
            break;
        case OBJECT_FUNCTION:
            CN_MarkFunction(gc, obj);
            break;
        case OBJECT_CLOSURE:
            CN_MarkClosure(gc, obj);
            break;
        case OBJECT_CLASS:
            CN_MarkClass(gc, obj);
            break;
        case OBJECT_INSTANCE:
            CN_MarkInstance(gc, obj);
            break;
    }
}

// 标记数组
void CN_MarkArray(CN_GC* gc, CN_GCObject* obj) {
    for (size_t i = 0; i < obj->array.length; i++) {
        CN_MarkValue(gc, obj->array.elements[i]);
    }
}

// 标记对象
void CN_MarkObject(CN_GC* gc, CN_GCObject* obj) {
    for (size_t i = 0; i < obj->object.field_count; i++) {
        CN_MarkValue(gc, obj->object.fields[i]);
    }
}
```

### 4.6 清除阶段（Sweep Phase）

```c
// 清除阶段
void CN_GCSweep(CN_GC* gc) {
    CN_GCObject** obj_ptr = &gc->objects;
    while (*obj_ptr != NULL) {
        if (!(*obj_ptr)->marked) {
            // 未标记，回收
            CN_GCObject* unreached = *obj_ptr;
            *obj_ptr = unreached->next;
            if (*obj_ptr != NULL) {
                (*obj_ptr)->prev = NULL;
            }

            // 调用终结器
            if (unreached->finalizer != NULL) {
                unreached->finalizer(unreached);
            }

            // 释放内存
            CN_AllocatorFree(gc->allocator, unreached);

            // 更新统计
            gc->stats->freed_bytes += unreached->size;
        } else {
            // 已标记，清除标记位
            (*obj_ptr)->marked = 0;
            obj_ptr = &(*obj_ptr)->next;
        }
    }
}

// 执行 GC
void CN_GCCollect(CN_GC* gc) {
    // 开始计时
    clock_t start = clock();

    // 标记阶段
    CN_GCMark(gc);

    // 清除阶段
    CN_GCSweep(gc);

    // 结束计时
    clock_t end = clock();
    gc->stats->collection_time += (end - start);
    gc->stats->collection_count++;
}
```

### 4.7 分代回收（Generational GC）

```c
// 分代回收器
typedef struct CN_GenerationalGC {
    CN_GC* young_gen;           // 年轻代
    CN_GC* old_gen;             // 老年代
    size_t promotion_age;       // 晋升年龄
} CN_GenerationalGC;

// 对象晋升
void CN_PromoteObject(CN_GenerationalGC* gc, CN_GCObject* obj) {
    // 从年轻代移动到老年代
    CN_RemoveFromGeneration(gc->young_gen, obj);
    CN_AddToGeneration(gc->old_gen, obj);
}

// 分代 GC 回收
void CN_GenerationalGCCollect(CN_GenerationalGC* gc) {
    // 只回收年轻代
    CN_GCCollect(gc->young_gen);

    // 如果老年代空间不足，回收老年代
    if (gc->old_gen->stats->allocated_bytes > gc->old_gen->config->gc_threshold) {
        CN_GCCollect(gc->old_gen);
    }
}
```

## 5. 内存管理（Memory Management）

### 5.1 内存管理器结构

```c
// 内存管理器结构
typedef struct CN_Memory {
    // 内存池
    CN_MemoryPool** pools;
    size_t pool_count;

    // 统计信息
    CN_MemoryStats* stats;

    // 配置
    CN_MemoryConfig* config;
} CN_Memory;

// 内存配置
typedef struct CN_MemoryConfig {
    size_t pool_size;           // 内存池大小
    size_t block_size;          // 块大小
    int enable_debug;           // 是否启用调试
} CN_MemoryConfig;

// 内存统计信息
typedef struct CN_MemoryStats {
    size_t total_allocated;     // 总分配量
    size_t total_freed;         // 总释放量
    size_t current_usage;       // 当前使用量
    size_t peak_usage;          // 峰值使用量
} CN_MemoryStats;
```

### 5.2 内存池（Memory Pool）

```c
// 内存池结构
typedef struct CN_MemoryPool {
    void* memory;               // 内存块
    size_t size;                // 内存大小
    size_t block_size;          // 块大小
    void** free_blocks;         // 空闲块列表
    size_t free_count;          // 空闲块数量
    size_t total_blocks;        // 总块数
    CN_Memory* memory_manager;  // 内存管理器
} CN_MemoryPool;

// 内存池接口
CN_MemoryPool* CN_MemoryPoolCreate(CN_Memory* memory, size_t block_size, size_t block_count);
void CN_MemoryPoolDestroy(CN_MemoryPool* pool);
void* CN_MemoryPoolAllocate(CN_MemoryPool* pool);
void CN_MemoryPoolFree(CN_MemoryPool* pool, void* ptr);
```

### 5.3 内存分配（Memory Allocation）

```c
// 内存分配
void* CN_MemoryAllocate(CN_Memory* memory, size_t size) {
    // 查找合适的内存池
    CN_MemoryPool* pool = CN_FindSuitablePool(memory, size);
    if (pool == NULL) {
        // 创建新的内存池
        pool = CN_MemoryPoolCreate(memory, size, 1024);
        if (pool == NULL) {
            return NULL;
        }
        CN_AddPool(memory, pool);
    }

    // 从内存池分配
    void* ptr = CN_MemoryPoolAllocate(pool);
    if (ptr == NULL) {
        return NULL;
    }

    // 更新统计
    memory->stats->total_allocated += size;
    memory->stats->current_usage += size;
    if (memory->stats->current_usage > memory->stats->peak_usage) {
        memory->stats->peak_usage = memory->stats->current_usage;
    }

    return ptr;
}

// 内存释放
void CN_MemoryFree(CN_Memory* memory, void* ptr) {
    // 查找内存池
    CN_MemoryPool* pool = CN_FindPoolForPointer(memory, ptr);
    if (pool == NULL) {
        return;
    }

    // 释放到内存池
    CN_MemoryPoolFree(pool, ptr);

    // 更新统计
    memory->stats->total_freed += pool->block_size;
    memory->stats->current_usage -= pool->block_size;
}
```

## 6. 类型系统（Type System）

### 6.1 类型系统结构

```c
// 类型系统结构
typedef struct CN_TypeSystem {
    // 类型表
    CN_Type** types;
    size_t type_count;
    size_t capacity;

    // 类型缓存
    CN_TypeCache* cache;

    // 运行时
    CN_Runtime* runtime;
} CN_TypeSystem;

// 类型缓存
typedef struct CN_TypeCache {
    CN_Type* int_type;          // 整数类型
    CN_Type* float_type;        // 浮点类型
    CN_Type* bool_type;         // 布尔类型
    CN_Type* string_type;       // 字符串类型
    CN_Type* void_type;         // void 类型
    CN_Type* null_type;         // null 类型
} CN_TypeCache;
```

### 6.2 类型定义（Type Definition）

```c
// 类型种类
typedef enum CN_TypeKind {
    TYPE_VOID,                  // void 类型
    TYPE_NULL,                  // null 类型
    TYPE_BOOL,                  // bool 类型
    TYPE_INT,                   // int 类型
    TYPE_FLOAT,                 // float 类型
    TYPE_STRING,                // string 类型
    TYPE_ARRAY,                 // array 类型
    TYPE_OBJECT,                // object 类型
    TYPE_FUNCTION,              // function 类型
    TYPE_CLASS,                 // class 类型
    TYPE_INTERFACE              // interface 类型
} CN_TypeKind;

// 类型结构
typedef struct CN_Type {
    CN_TypeKind kind;           // 类型种类
    const char* name;           // 类型名称
    size_t size;                // 类型大小
    union {
        struct {
            CN_Type* element_type; // 元素类型
        } array;
        struct {
            CN_Class* class_def;    // 类定义
        } class_type;
        struct {
            CN_Type* return_type;  // 返回类型
            CN_Type** param_types;  // 参数类型
            size_t param_count;    // 参数数量
        } function;
    };
} CN_Type;
```

### 6.3 值（Value）

```c
// 值类型
typedef enum CN_ValueType {
    VALUE_NULL,                 // null
    VALUE_BOOL,                 // bool
    VALUE_INT,                  // int
    VALUE_FLOAT,                // float
    VALUE_STRING,               // string
    VALUE_ARRAY,                // array
    VALUE_OBJECT,               // object
    VALUE_FUNCTION,             // function
    VALUE_CLASS                 // class
} CN_ValueType;

// 值结构
typedef struct CN_Value {
    CN_ValueType type;          // 值类型
    union {
        int bool_val;           // bool 值
        int64_t int_val;         // int 值
        double float_val;        // float 值
        CN_GCObject* obj_ptr;    // 对象指针
        CN_Function* func_ptr;  // 函数指针
        CN_Class* class_ptr;     // 类指针
    };
} CN_Value;

// 值操作
CN_Value CN_NullValue(void);
CN_Value CN_BoolValue(int value);
CN_Value CN_IntValue(int64_t value);
CN_Value CN_FloatValue(double value);
CN_Value CN_StringValue(const char* str);
CN_Value CN_ArrayValue(CN_GCObject* array);
CN_Value CN_ObjectValue(CN_GCObject* obj);
int CN_IsNull(CN_Value value);
int CN_IsObject(CN_Value value);
CN_GCObject* CN_AsObject(CN_Value value);
```

### 6.4 类型检查（Type Checking）

```c
// 类型检查
int CN_TypeCheck(CN_TypeSystem* type_system, CN_Value value, CN_Type* expected_type) {
    CN_Type* actual_type = CN_GetValueType(type_system, value);
    return CN_TypeEquals(type_system, actual_type, expected_type);
}

// 获取值的类型
CN_Type* CN_GetValueType(CN_TypeSystem* type_system, CN_Value value) {
    switch (value.type) {
        case VALUE_NULL:
            return type_system->cache->null_type;
        case VALUE_BOOL:
            return type_system->cache->bool_type;
        case VALUE_INT:
            return type_system->cache->int_type;
        case VALUE_FLOAT:
            return type_system->cache->float_type;
        case VALUE_STRING:
            return type_system->cache->string_type;
        case VALUE_ARRAY:
            return CN_GetArrayType(type_system, value.obj_ptr);
        case VALUE_OBJECT:
            return CN_GetObjectType(type_system, value.obj_ptr);
        case VALUE_FUNCTION:
            return CN_GetFunctionType(type_system, value.func_ptr);
        case VALUE_CLASS:
            return CN_GetClassType(type_system, value.class_ptr);
        default:
            return NULL;
    }
}

// 类型相等
int CN_TypeEquals(CN_TypeSystem* type_system, CN_Type* type1, CN_Type* type2) {
    if (type1 == type2) {
        return 1;
    }
    if (type1->kind != type2->kind) {
        return 0;
    }
    switch (type1->kind) {
        case TYPE_ARRAY:
            return CN_TypeEquals(type_system, type1->array.element_type,
                                   type2->array.element_type);
        case TYPE_CLASS:
            return type1->class_type.class_def == type2->class_type.class_def;
        case TYPE_FUNCTION:
            if (!CN_TypeEquals(type_system, type1->function.return_type,
                                 type2->function.return_type)) {
                return 0;
            }
            if (type1->function.param_count != type2->function.param_count) {
                return 0;
            }
            for (size_t i = 0; i < type1->function.param_count; i++) {
                if (!CN_TypeEquals(type_system, type1->function.param_types[i],
                                     type2->function.param_types[i])) {
                    return 0;
                }
            }
            return 1;
        default:
            return 1;
    }
}
```

### 6.5 类型转换（Type Conversion）

```c
// 类型转换
CN_Value CN_TypeConvert(CN_Runtime* runtime, CN_Value value, CN_Type* target_type) {
    CN_Type* source_type = CN_GetValueType(runtime->type_system, value);

    // 相同类型，直接返回
    if (CN_TypeEquals(runtime->type_system, source_type, target_type)) {
        return value;
    }

    // 隐式转换
    switch (source_type->kind) {
        case TYPE_INT:
            if (target_type->kind == TYPE_FLOAT) {
                return CN_FloatValue((double)value.int_val);
            }
            break;
        case TYPE_FLOAT:
            if (target_type->kind == TYPE_INT) {
                return CN_IntValue((int64_t)value.float_val);
            }
            break;
        case TYPE_STRING:
            // 尝试从字符串转换
            return CN_StringToValue(runtime, value.obj_ptr->string.data, target_type);
        default:
            break;
    }

    // 无法转换
    CN_ReportError(runtime, "Cannot convert type %s to %s",
                   source_type->name, target_type->name);
    return CN_NullValue();
}
```

## 7. 异常处理（Exception Handling）

### 7.1 异常结构

```c
// 异常结构
typedef struct CN_Exception {
    const char* message;        // 异常消息
    const char* type;           // 异常类型
    CN_Value* stack_trace;      // 堆栈跟踪
    size_t stack_trace_size;   // 堆栈跟踪大小
    CN_Exception* cause;        // 原因异常
} CN_Exception;

// 异常类型
typedef enum CN_ExceptionType {
    EXCEPTION_RUNTIME,          // 运行时异常
    EXCEPTION_ARITHMETIC,       // 算术异常
    EXCEPTION_INDEX,            // 索引异常
    EXCEPTION_TYPE,             // 类型异常
    EXCEPTION_MEMORY,           // 内存异常
    EXCEPTION_IO                // IO 异常
} CN_ExceptionType;
```

### 7.2 异常抛出（Exception Throwing）

```c
// 抛出异常
void CN_ThrowException(CN_Runtime* runtime, const char* message, CN_ExceptionType type) {
    // 创建异常对象
    CN_Exception* exception = (CN_Exception*)CN_MemoryAllocate(runtime->memory, sizeof(CN_Exception));
    exception->message = CN_StringCopy(message);
    exception->type = CN_GetExceptionTypeName(type);
    exception->stack_trace = CN_CaptureStackTrace(runtime);
    exception->stack_trace_size = CN_GetStackTraceSize(runtime);
    exception->cause = NULL;

    // 设置当前异常
    runtime->state->exception = CN_ObjectValue((CN_GCObject*)exception);

    // 查找异常处理器
    CN_ExceptionHandler* handler = CN_FindExceptionHandler(runtime);
    if (handler != NULL) {
        // 跳转到异常处理器
        CN_JumpToExceptionHandler(runtime, handler);
    } else {
        // 未处理的异常
        CN_PrintUncaughtException(runtime, exception);
        CN_RuntimeExit(runtime, 1);
    }
}

// 捕获堆栈跟踪
CN_Value* CN_CaptureStackTrace(CN_Runtime* runtime) {
    CN_VM* vm = runtime->vm;
    size_t depth = vm->call_stack_top;
    CN_Value* stack_trace = (CN_Value*)CN_MemoryAllocate(runtime->memory, sizeof(CN_Value) * depth);

    for (size_t i = 0; i < depth; i++) {
        CN_CallFrame* frame = &vm->call_stack[i];
        stack_trace[i] = CN_StringValue(frame->function->name);
    }

    return stack_trace;
}
```

### 7.3 异常处理（Exception Handling）

```c
// 异常处理器结构
typedef struct CN_ExceptionHandler {
    CN_ExceptionType type;      // 异常类型
    size_t handler_address;     // 处理器地址
    CN_CallFrame* frame;         // 调用帧
    CN_ExceptionHandler* next;  // 下一个处理器
} CN_ExceptionHandler;

// 查找异常处理器
CN_ExceptionHandler* CN_FindExceptionHandler(CN_Runtime* runtime) {
    CN_VM* vm = runtime->vm;
    CN_CallFrame* frame = vm->current_frame;

    while (frame != NULL) {
        // 检查帧的异常处理器
        CN_ExceptionHandler* handler = frame->exception_handlers;
        while (handler != NULL) {
            if (handler->type == EXCEPTION_RUNTIME ||
                handler->type == CN_GetCurrentExceptionType(runtime)) {
                return handler;
            }
            handler = handler->next;
        }
        frame = frame->caller;
    }

    return NULL;
}

// 跳转到异常处理器
void CN_JumpToExceptionHandler(CN_Runtime* runtime, CN_ExceptionHandler* handler) {
    CN_VM* vm = runtime->vm;

    // 恢复到处理器所在的帧
    while (vm->current_frame != handler->frame) {
        CN_VMStackReset(vm, vm->current_frame->stack_base);
        vm->call_stack_top--;
        vm->current_frame = vm->current_frame->caller;
    }

    // 跳转到处理器地址
    vm->current_frame->ip = vm->current_frame->function->code + handler->handler_address;
}
```

## 8. 性能分析（Profiling）

### 8.1 性能分析器结构

```c
// 性能分析器结构
typedef struct CN_Profiler {
    // 函数调用统计
    CN_FunctionStats** function_stats;
    size_t function_count;

    // 内存统计
    CN_MemoryStats* memory_stats;

    // GC 统计
    CN_GCStats* gc_stats;

    // 启用状态
    int enabled;
} CN_Profiler;

// 函数统计信息
typedef struct CN_FunctionStats {
    const char* name;           // 函数名
    size_t call_count;         // 调用次数
    uint64_t total_time;        // 总时间
    uint64_t self_time;         // 自身时间
    uint64_t max_time;          // 最大时间
    uint64_t min_time;          // 最小时间
} CN_FunctionStats;
```

### 8.2 性能分析（Profiling）

```c
// 开始函数调用
void CN_ProfilerBeginFunction(CN_Profiler* profiler, const char* name) {
    if (!profiler->enabled) {
        return;
    }

    // 查找或创建函数统计
    CN_FunctionStats* stats = CN_FindFunctionStats(profiler, name);
    if (stats == NULL) {
        stats = CN_CreateFunctionStats(profiler, name);
    }

    // 记录开始时间
    stats->call_count++;
    stats->start_time = CN_GetCurrentTime();
}

// 结束函数调用
void CN_ProfilerEndFunction(CN_Profiler* profiler, const char* name) {
    if (!profiler->enabled) {
        return;
    }

    // 查找函数统计
    CN_FunctionStats* stats = CN_FindFunctionStats(profiler, name);
    if (stats == NULL) {
        return;
    }

    // 计算执行时间
    uint64_t elapsed = CN_GetCurrentTime() - stats->start_time;
    stats->total_time += elapsed;
    stats->self_time += elapsed;

    // 更新最大/最小时间
    if (elapsed > stats->max_time) {
        stats->max_time = elapsed;
    }
    if (elapsed < stats->min_time || stats->min_time == 0) {
        stats->min_time = elapsed;
    }
}
```

## 9. 调试支持（Debugging）

### 9.1 调试器结构

```c
// 调试器结构
typedef struct CN_Debugger {
    CN_Runtime* runtime;        // 运行时
    CN_Breakpoint** breakpoints; // 断点列表
    size_t breakpoint_count;    // 断点数量
    int is_paused;              // 是否暂停
    CN_DebuggerState state;      // 调试器状态
} CN_Debugger;

// 断点结构
typedef struct CN_Breakpoint {
    const char* file;           // 文件名
    size_t line;                // 行号
    int enabled;                // 是否启用
    int hit_count;              // 命中次数
} CN_Breakpoint;

// 调试器状态
typedef enum CN_DebuggerState {
    DEBUGGER_RUNNING,           // 运行中
    DEBUGGER_PAUSED,            // 已暂停
    DEBUGGER_STEP_OVER,         // 单步跳过
    DEBUGGER_STEP_INTO,         // 单步进入
    DEBUGGER_STEP_OUT           // 单步跳出
} CN_DebuggerState;
```

### 9.2 调试功能

```c
// 设置断点
int CN_DebuggerSetBreakpoint(CN_Debugger* debugger, const char* file, size_t line) {
    // 检查断点是否已存在
    for (size_t i = 0; i < debugger->breakpoint_count; i++) {
        CN_Breakpoint* bp = debugger->breakpoints[i];
        if (strcmp(bp->file, file) == 0 && bp->line == line) {
            return 0;
        }
    }

    // 创建新断点
    CN_Breakpoint* bp = (CN_Breakpoint*)CN_MemoryAllocate(debugger->runtime->memory, sizeof(CN_Breakpoint));
    bp->file = CN_StringCopy(file);
    bp->line = line;
    bp->enabled = 1;
    bp->hit_count = 0;

    // 添加到断点列表
    CN_AddBreakpoint(debugger, bp);

    return 1;
}

// 检查断点
int CN_DebuggerCheckBreakpoint(CN_Debugger* debugger, const char* file, size_t line) {
    for (size_t i = 0; i < debugger->breakpoint_count; i++) {
        CN_Breakpoint* bp = debugger->breakpoints[i];
        if (bp->enabled && strcmp(bp->file, file) == 0 && bp->line == line) {
            bp->hit_count++;
            return 1;
        }
    }
    return 0;
}

// 单步执行
void CN_DebuggerStep(CN_Debugger* debugger, CN_DebuggerState state) {
    debugger->state = state;
    debugger->is_paused = 0;
}
```

## 10. 总结

CN_Language 运行时系统采用模块化设计，各组件职责明确、接口清晰。虚拟机负责字节码执行，垃圾回收器负责自动内存管理，内存管理器提供底层内存操作，类型系统提供类型检查和转换功能。异常处理机制完善，支持异常抛出和捕获。性能分析器和调试器为开发者提供了强大的工具支持。
