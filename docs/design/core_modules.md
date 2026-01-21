# CN_Language 核心模块架构设计

## 1. 概述

本文档详细描述 CN_Language 项目的核心模块架构，包括编译器、运行时、标准库和工具库的详细设计。

## 2. 编译器模块（compiler）

### 2.1 词法分析器（lexer）

#### 2.1.1 模块职责
- 将源代码转换为 Token 流
- 识别中文关键字和标识符
- 处理注释和空白字符
- 记录 Token 位置信息

#### 2.1.2 目录结构
```
src/compiler/lexer/
├── include/
│   └── lexer.h              # 词法分析器公共接口
├── src/
│   ├── lexer_core.c         # 词法分析器核心实现
│   ├── token.c              # Token 定义和操作
│   ├── keyword.c            # 关键字识别
│   ├── identifier.c         # 标识符识别
│   ├── literal.c            # 字面量识别
│   └── comment.c            # 注释处理
└── README.md                # 模块文档
```

#### 2.1.3 接口设计
```c
// 词法分析器结构
typedef struct CN_Lexer {
    const char* source;           // 源代码
    size_t position;              // 当前位置
    size_t line;                  // 当前行号
    size_t column;                // 当前列号
    CN_Token current_token;       // 当前 Token
    CN_ErrorHandler* error_handler; // 错误处理器
} CN_Lexer;

// Token 类型
typedef enum CN_TokenType {
    TOKEN_EOF,                    // 文件结束
    TOKEN_KEYWORD,                // 关键字
    TOKEN_IDENTIFIER,             // 标识符
    TOKEN_INTEGER,                // 整数
    TOKEN_FLOAT,                  // 浮点数
    TOKEN_STRING,                 // 字符串
    TOKEN_CHAR,                   // 字符
    TOKEN_OPERATOR,               // 运算符
    TOKEN_PUNCTUATOR,             // 标点符号
    // ... 更多 Token 类型
} CN_TokenType;

// Token 结构
typedef struct CN_Token {
    CN_TokenType type;            // Token 类型
    const char* value;            // Token 值
    size_t length;                // Token 长度
    size_t line;                  // 行号
    size_t column;                // 列号
} CN_Token;

// 词法分析器接口
CN_Lexer* CN_LexerCreate(const char* source);
void CN_LexerDestroy(CN_Lexer* lexer);
CN_Token CN_LexerNextToken(CN_Lexer* lexer);
CN_Token CN_LexerPeekToken(CN_Lexer* lexer);
int CN_LexerHasError(CN_Lexer* lexer);
```

#### 2.1.4 子模块说明

**lexer_core.c**
- 词法分析器核心逻辑
- 状态机实现
- Token 生成

**token.c**
- Token 结构定义
- Token 操作函数
- Token 类型判断

**keyword.c**
- 中文关键字表
- 关键字识别
- 关键字到 Token 类型映射

**identifier.c**
- 标识符识别
- 标识符命名规则检查
- 标识符有效性验证

**literal.c**
- 整数字面量识别
- 浮点数字面量识别
- 字符串字面量识别
- 字符字面量识别

**comment.c**
- 单行注释处理
- 多行注释处理
- 注略注释内容

### 2.2 语法分析器（parser）

#### 2.2.1 模块职责
- 将 Token 流转换为抽象语法树（AST）
- 语法错误检测和报告
- 作用域管理
- 符号表构建

#### 2.2.2 目录结构
```
src/compiler/parser/
├── include/
│   └── parser.h              # 语法分析器公共接口
├── src/
│   ├── parser_core.c         # 语法分析器核心实现
│   ├── expression.c          # 表达式解析
│   ├── statement.c           # 语句解析
│   ├── declaration.c         # 声明解析
│   ├── scope.c               # 作用域管理
│   └── symbol_table.c        # 符号表管理
└── README.md                 # 模块文档
```

#### 2.2.3 接口设计
```c
// 语法分析器结构
typedef struct CN_Parser {
    CN_Lexer* lexer;               // 词法分析器
    CN_Token current_token;        // 当前 Token
    CN_Token peek_token;           // 预读 Token
    CN_AST* ast;                   // 抽象语法树
    CN_Scope* current_scope;       // 当前作用域
    CN_SymbolTable* symbol_table;  // 符号表
    CN_ErrorHandler* error_handler; // 错误处理器
} CN_Parser;

// 语法分析器接口
CN_Parser* CN_ParserCreate(CN_Lexer* lexer);
void CN_ParserDestroy(CN_Parser* parser);
CN_AST* CN_ParserParse(CN_Parser* parser);
int CN_ParserHasError(CN_Parser* parser);
```

#### 2.2.4 子模块说明

**parser_core.c**
- 语法分析器核心逻辑
- 递归下降分析实现
- 错误恢复机制

**expression.c**
- 表达式解析
- 运算符优先级处理
- 类型推断

**statement.c**
- 语句解析
- 控制流语句
- 块语句

**declaration.c**
- 声明解析
- 变量声明
- 函数声明
- 类型声明

**scope.c**
- 作用域管理
- 作用域嵌套
- 作用域查找

**symbol_table.c**
- 符号表管理
- 符号注册
- 符号查找
- 符号类型检查

### 2.3 抽象语法树（ast）

#### 2.3.1 模块职责
- 表示程序的结构
- 提供 AST 节点类型
- 提供 AST 遍历接口
- AST 序列化/反序列化

#### 2.3.2 目录结构
```
src/compiler/ast/
├── include/
│   └── ast.h                 # AST 公共接口
├── src/
│   ├── ast_core.c            # AST 核心实现
│   ├── ast_node.c            # AST 节点定义
│   ├── ast_visitor.c         # AST 访问者模式
│   ├── ast_printer.c         # AST 打印器
│   └── ast_serializer.c      # AST 序列化
└── README.md                 # 模块文档
```

#### 2.3.3 接口设计
```c
// AST 节点类型
typedef enum CN_ASTNodeType {
    AST_PROGRAM,               // 程序节点
    AST_FUNCTION_DECL,         // 函数声明
    AST_VARIABLE_DECL,         // 变量声明
    AST_BLOCK,                 // 块语句
    AST_IF_STATEMENT,          // if 语句
    AST_WHILE_STATEMENT,       // while 语句
    AST_FOR_STATEMENT,         // for 语句
    AST_RETURN_STATEMENT,      // return 语句
    AST_EXPRESSION_STMT,       // 表达式语句
    AST_BINARY_EXPR,           // 二元表达式
    AST_UNARY_EXPR,            // 一元表达式
    AST_CALL_EXPR,             // 函数调用
    AST_IDENTIFIER,            // 标识符
    AST_LITERAL,               // 字面量
    // ... 更多节点类型
} CN_ASTNodeType;

// AST 节点基类
typedef struct CN_ASTNode {
    CN_ASTNodeType type;       // 节点类型
    const char* source_file;   // 源文件名
    size_t line;               // 行号
    size_t column;             // 列号
} CN_ASTNode;

// AST 结构
typedef struct CN_AST {
    CN_ASTNode* root;          // 根节点
    CN_SymbolTable* symbols;   // 符号表
} CN_AST;

// AST 接口
CN_AST* CN_ASTCreate(void);
void CN_ASTDestroy(CN_AST* ast);
void CN_ASTAddNode(CN_AST* ast, CN_ASTNode* node);
CN_ASTNode* CN_ASTGetRoot(CN_AST* ast);
```

#### 2.3.4 子模块说明

**ast_core.c**
- AST 核心管理
- 节点生命周期管理
- AST 遍历

**ast_node.c**
- AST 节点定义
- 节点构造函数
- 节点访问函数

**ast_visitor.c**
- 访问者模式实现
- AST 遍历接口
- 节点访问回调

**ast_printer.c**
- AST 打印器
- 格式化输出
- 调试支持

**ast_serializer.c**
- AST 序列化
- AST 反序列化
- 二进制格式

### 2.4 中间表示（ir）

#### 2.4.1 模块职责
- 提供平台无关的中间代码
- IR 指令定义
- IR 生成
- IR 优化

#### 2.4.2 目录结构
```
src/compiler/ir/
├── include/
│   └── ir.h                  # IR 公共接口
├── src/
│   ├── ir_core.c             # IR 核心实现
│   ├── ir_instruction.c      # IR 指令定义
│   ├── ir_builder.c          # IR 构建器
│   ├── ir_optimizer.c        # IR 优化器
│   └── ir_printer.c          # IR 打印器
└── README.md                 # 模块文档
```

#### 2.4.3 接口设计
```c
// IR 指令类型
typedef enum CN_IRInstructionType {
    IR_NOP,                    // 空操作
    IR_LOAD,                   // 加载
    IR_STORE,                  // 存储
    IR_ADD,                    // 加法
    IR_SUB,                    // 减法
    IR_MUL,                    // 乘法
    IR_DIV,                    // 除法
    IR_MOD,                    // 取模
    IR_CALL,                   // 函数调用
    IR_RET,                    // 返回
    IR_JUMP,                   // 跳转
    IR_BRANCH,                 // 条件跳转
    // ... 更多指令类型
} CN_IRInstructionType;

// IR 指令
typedef struct CN_IRInstruction {
    CN_IRInstructionType type;  // 指令类型
    CN_Value* operands[3];     // 操作数
    CN_Value* result;          // 结果
    CN_BasicBlock* block;      // 所属基本块
} CN_IRInstruction;

// IR 函数
typedef struct CN_IRFunction {
    const char* name;          // 函数名
    CN_Type* return_type;      // 返回类型
    CN_Type** param_types;     // 参数类型
    size_t param_count;        // 参数数量
    CN_BasicBlock* entry;      // 入口基本块
    CN_BasicBlock** blocks;    // 基本块列表
    size_t block_count;        // 基本块数量
} CN_IRFunction;

// IR 模块
typedef struct CN_IRModule {
    CN_IRFunction** functions;  // 函数列表
    size_t function_count;     // 函数数量
    CN_GlobalVariable** globals; // 全局变量列表
    size_t global_count;       // 全局变量数量
} CN_IRModule;

// IR 接口
CN_IRModule* CN_IRModuleCreate(void);
void CN_IRModuleDestroy(CN_IRModule* module);
CN_IRFunction* CN_IRModuleAddFunction(CN_IRModule* module, const char* name);
```

#### 2.4.4 子模块说明

**ir_core.c**
- IR 模块管理
- IR 函数管理
- IR 基本块管理

**ir_instruction.c**
- IR 指令定义
- 指令操作
- 指令验证

**ir_builder.c**
- IR 构建器
- 指令生成
- SSA 形式转换

**ir_optimizer.c**
- 常量折叠
- 死代码消除
- 公共子表达式消除
- 循环优化

**ir_printer.c**
- IR 打印器
- 格式化输出
- 调试支持

### 2.5 代码生成器（codegen）

#### 2.5.1 模块职责
- 将 IR 转换为目标代码
- 目标代码生成
- 寄存器分配
- 栈帧管理

#### 2.5.2 目录结构
```
src/compiler/codegen/
├── include/
│   └── codegen.h             # 代码生成器公共接口
├── src/
│   ├── codegen_core.c        # 代码生成器核心实现
│   ├── target.c              # 目标平台
│   ├── register_allocator.c  # 寄存器分配
│   ├── stack_frame.c         # 栈帧管理
│   ├── calling_convention.c  # 调用约定
│   └── machine_code.c        # 机器代码生成
└── README.md                 # 模块文档
```

#### 2.5.3 接口设计
```c
// 代码生成器结构
typedef struct CN_CodeGen {
    CN_IRModule* ir_module;    // IR 模块
    CN_Target* target;         // 目标平台
    CN_RegisterAllocator* reg_allocator; // 寄存器分配器
    CN_CodeBuffer* code_buffer; // 代码缓冲区
} CN_CodeGen;

// 目标平台
typedef struct CN_Target {
    const char* name;         // 平台名称
    const char* triple;        // 目标三元组
    size_t pointer_size;      // 指针大小
    size_t word_size;         // 字大小
    int endianness;           // 字节序
} CN_Target;

// 代码生成器接口
CN_CodeGen* CN_CodeGenCreate(CN_IRModule* ir_module, CN_Target* target);
void CN_CodeGenDestroy(CN_CodeGen* codegen);
int CN_CodeGenGenerate(CN_CodeGen* codegen, const char* output_file);
```

#### 2.5.4 子模块说明

**codegen_core.c**
- 代码生成器核心逻辑
- IR 到目标代码转换
- 代码生成流程控制

**target.c**
- 目标平台定义
- 平台特性
- 平台配置

**register_allocator.c**
- 寄存器分配算法
- 寄存器冲突图
- 寄存器溢出处理

**stack_frame.c**
- 栈帧布局
- 局部变量分配
- 参数传递

**calling_convention.c**
- 调用约定实现
- 参数传递规则
- 返回值处理

**machine_code.c**
- 机器代码生成
- 指令编码
- 重定位

### 2.6 优化器（optimizer）

#### 2.6.1 模块职责
- 优化生成的代码
- 提高代码性能
- 减少代码体积

#### 2.6.2 目录结构
```
src/compiler/optimizer/
├── include/
│   └── optimizer.h           # 优化器公共接口
├── src/
│   ├── optimizer_core.c      # 优化器核心实现
│   ├── const_fold.c          # 常量折叠
│   ├── dead_code_elim.c      # 死代码消除
│   ├── loop_opt.c            # 循环优化
│   ├── inline.c              // 内联展开
│   └── strength_reduction.c  // 强度削减
└── README.md                 # 模块文档
```

#### 2.6.3 接口设计
```c
// 优化器结构
typedef struct CN_Optimizer {
    CN_IRModule* ir_module;    // IR 模块
    CN_OptimizationPass** passes; // 优化遍
    size_t pass_count;         // 优化遍数量
} CN_Optimizer;

// 优化遍
typedef struct CN_OptimizationPass {
    const char* name;          // 优化遍名称
    int (*run)(CN_IRModule* module); // 运行函数
    int enabled;               // 是否启用
} CN_OptimizationPass;

// 优化器接口
CN_Optimizer* CN_OptimizerCreate(CN_IRModule* ir_module);
void CN_OptimizerDestroy(CN_Optimizer* optimizer);
int CN_OptimizerRun(CN_Optimizer* optimizer);
void CN_OptimizerAddPass(CN_Optimizer* optimizer, CN_OptimizationPass* pass);
```

#### 2.6.4 子模块说明

**optimizer_core.c**
- 优化器核心逻辑
- 优化遍管理
- 优化流程控制

**const_fold.c**
- 常量表达式求值
- 常量传播
- 常量折叠

**dead_code_elim.c**
- 死代码检测
- 不可达代码消除
- 未使用变量消除

**loop_opt.c**
- 循环展开
- 循环不变代码外提
- 循环强度削减

**inline.c**
- 函数内联
- 内联决策
- 内联优化

**strength_reduction.c**
- 强度削减
- 乘法转移位
- 除法转乘法

## 3. 运行时模块（runtime）

### 3.1 垃圾回收（gc）

#### 3.1.1 模块职责
- 自动内存管理
- 对象分配
- 引用计数
- 标记清除

#### 3.1.2 目录结构
```
src/runtime/gc/
├── include/
│   └── gc.h                  # GC 公共接口
├── src/
│   ├── gc_core.c             # GC 核心实现
│   ├── allocator.c           # 对象分配器
│   ├── ref_count.c           # 引用计数
│   ├── mark_sweep.c          # 标记清除
│   └── gc_stats.c            # GC 统计
└── README.md                 # 模块文档
```

#### 3.1.3 接口设计
```c
// GC 结构
typedef struct CN_GC {
    CN_Allocator* allocator;  // 分配器
    CN_Object** roots;        // 根集合
    size_t root_count;        // 根数量
    size_t allocated_bytes;   // 已分配字节数
    size_t threshold;         // GC 触发阈值
} CN_GC;

// GC 对象
typedef struct CN_GCObject {
    size_t size;              // 对象大小
    int marked;               // 标记位
    CN_GCObject* next;        // 下一个对象
    void (*finalizer)(void*); // 终结器
} CN_GCObject;

// GC 接口
CN_GC* CN_GCCreate(void);
void CN_GCDestroy(CN_GC* gc);
void* CN_GCAllocate(CN_GC* gc, size_t size);
void CN_GCCollect(CN_GC* gc);
void CN_GCAddRoot(CN_GC* gc, void* root);
void CN_GCRemoveRoot(CN_GC* gc, void* root);
```

#### 3.1.4 子模块说明

**gc_core.c**
- GC 核心逻辑
- GC 触发机制
- GC 调度

**allocator.c**
- 对象分配
- 内存池管理
- 分配策略

**ref_count.c**
- 引用计数实现
- 引用增减
- 循环引用检测

**mark_sweep.c**
- 标记阶段
- 清除阶段
- 分代回收

**gc_stats.c**
- GC 统计信息
- 性能监控
- 调试支持

### 3.2 内存管理（memory）

#### 3.2.1 模块职责
- 底层内存操作
- 内存池管理
- 内存对齐
- 内存保护

#### 3.2.2 目录结构
```
src/runtime/memory/
├── include/
│   └── memory.h              # 内存管理公共接口
├── src/
│   ├── memory_core.c         # 内存管理核心实现
│   ├── pool.c                # 内存池
│   ├── alignment.c           # 内存对齐
│   ├── protection.c          # 内存保护
│   └── stats.c               # 内存统计
└── README.md                 # 模块文档
```

#### 3.2.3 接口设计
```c
// 内存管理器结构
typedef struct CN_Memory {
    CN_MemoryPool** pools;     // 内存池列表
    size_t pool_count;        // 内存池数量
    size_t total_allocated;   // 总分配量
    size_t total_freed;       // 总释放量
} CN_Memory;

// 内存池结构
typedef struct CN_MemoryPool {
    size_t block_size;        // 块大小
    size_t block_count;       // 块数量
    void** free_blocks;       // 空闲块列表
    size_t free_count;        // 空闲块数量
} CN_MemoryPool;

// 内存管理接口
CN_Memory* CN_MemoryCreate(void);
void CN_MemoryDestroy(CN_Memory* memory);
void* CN_MemoryAllocate(CN_Memory* memory, size_t size);
void CN_MemoryFree(CN_Memory* memory, void* ptr);
CN_MemoryPool* CN_MemoryCreatePool(CN_Memory* memory, size_t block_size, size_t block_count);
```

#### 3.2.4 子模块说明

**memory_core.c**
- 内存管理核心逻辑
- 内存分配/释放
- 内存统计

**pool.c**
- 内存池实现
- 块分配
- 块回收

**alignment.c**
- 内存对齐计算
- 对齐分配
- 对齐检查

**protection.c**
- 内存保护
- 读写保护
- 执行保护

**stats.c**
- 内存统计
- 使用情况
- 泄漏检测

### 3.3 类型系统（types）

#### 3.3.1 模块职责
- 类型定义
- 类型检查
- 类型转换
- 类型推断

#### 3.3.2 目录结构
```
src/runtime/types/
├── include/
│   └── types.h               # 类型系统公共接口
├── src/
│   ├── types_core.c          # 类型系统核心实现
│   ├── type.c                # 类型定义
│   ├── type_check.c          # 类型检查
│   ├── type_convert.c        # 类型转换
│   └── type_infer.c          # 类型推断
└── README.md                 # 模块文档
```

#### 3.3.3 接口设计
```c
// 类型种类
typedef enum CN_TypeKind {
    TYPE_VOID,                 // void 类型
    TYPE_BOOL,                 // bool 类型
    TYPE_INT,                  // 整数类型
    TYPE_FLOAT,                // 浮点类型
    TYPE_STRING,               // 字符串类型
    TYPE_ARRAY,                // 数组类型
    TYPE_STRUCT,               // 结构体类型
    TYPE_FUNCTION,             // 函数类型
    TYPE_POINTER,              // 指针类型
    // ... 更多类型
} CN_TypeKind;

// 类型结构
typedef struct CN_Type {
    CN_TypeKind kind;          // 类型种类
    const char* name;          // 类型名称
    size_t size;               // 类型大小
    union {
        struct {
            CN_Type* element_type; // 元素类型
            size_t length;         // 数组长度
        } array;
        struct {
            CN_Type** fields;      // 字段类型
            const char** field_names; // 字段名称
            size_t field_count;    // 字段数量
        } structure;
        struct {
            CN_Type* return_type;  // 返回类型
            CN_Type** param_types; // 参数类型
            size_t param_count;    // 参数数量
        } function;
        struct {
            CN_Type* base_type;    // 基类型
        } pointer;
    };
} CN_Type;

// 类型系统接口
CN_Type* CN_TypeCreatePrimitive(CN_TypeKind kind);
CN_Type* CN_TypeCreateArray(CN_Type* element_type, size_t length);
CN_Type* CN_TypeCreateStruct(const char* name, CN_Type** fields, const char** field_names, size_t field_count);
CN_Type* CN_TypeCreateFunction(CN_Type* return_type, CN_Type** param_types, size_t param_count);
int CN_TypeCheck(CN_Type* type1, CN_Type* type2);
CN_Type* CN_TypeConvert(CN_Type* from_type, CN_Type* to_type);
```

#### 3.3.4 子模块说明

**types_core.c**
- 类型系统核心逻辑
- 类型管理
- 类型缓存

**type.c**
- 类型定义
- 类型构造
- 类型销毁

**type_check.c**
- 类型兼容性检查
- 类型相等判断
- 类型子类型关系

**type_convert.c**
- 隐式类型转换
- 显式类型转换
- 转换规则

**type_infer.c**
- 类型推断算法
- 表达式类型推断
- 变量类型推断

### 3.4 虚拟机（vm）

#### 3.4.1 模块职责
- 执行字节码
- 指令执行
- 栈管理
- 调用栈管理

#### 3.4.2 目录结构
```
src/runtime/vm/
├── include/
│   └── vm.h                  # 虚拟机公共接口
├── src/
│   ├── vm_core.c             # 虚拟机核心实现
│   ├── interpreter.c         # 解释器
│   ├── stack.c               # 栈管理
│   ├── call_stack.c          # 调用栈
│   └── exception.c           # 异常处理
└── README.md                 # 模块文档
```

#### 3.4.3 接口设计
```c
// 虚拟机结构
typedef struct CN_VM {
    CN_Bytecode* bytecode;     // 字节码
    CN_Value* stack;           // 操作数栈
    size_t stack_size;         // 栈大小
    size_t stack_top;          // 栈顶
    CN_CallFrame* call_stack;  // 调用栈
    size_t call_stack_size;    // 调用栈大小
    size_t call_stack_top;     // 调用栈顶
    CN_GC* gc;                 // 垃圾回收器
    CN_Runtime* runtime;       // 运行时环境
} CN_VM;

// 调用帧
typedef struct CN_CallFrame {
    CN_Function* function;     // 函数
    CN_Value* locals;          // 局部变量
    size_t ip;                 // 指令指针
} CN_CallFrame;

// 虚拟机接口
CN_VM* CN_VMCreate(CN_Runtime* runtime);
void CN_VMDestroy(CN_VM* vm);
int CN_VMExecute(CN_VM* vm, CN_Bytecode* bytecode);
void CN_VMReset(CN_VM* vm);
```

#### 3.4.4 子模块说明

**vm_core.c**
- 虚拟机核心逻辑
- 虚拟机初始化
- 虚拟机清理

**interpreter.c**
- 指令解释执行
- 指令分发
- 指令实现

**stack.c**
- 操作数栈管理
- 栈操作
- 栈检查

**call_stack.c**
- 调用栈管理
- 函数调用
- 函数返回

**exception.c**
- 异常处理
- 异常抛出
- 异常捕获

## 4. 标准库模块（stdlib）

### 4.1 输入输出（io）

#### 4.1.1 模块职责
- 标准输入输出
- 文件读写
- 格式化输出
- 缓冲管理

#### 4.1.2 目录结构
```
src/stdlib/io/
├── include/
│   └── io.h                  # IO 公共接口
├── src/
│   ├── io_core.c             # IO 核心实现
│   ├── stdio.c               # 标准输入输出
│   ├── file.c                # 文件操作
│   ├── format.c              // 格式化输出
│   └── buffer.c              // 缓冲管理
└── README.md                 # 模块文档
```

#### 4.1.3 接口设计
```c
// 文件结构
typedef struct CN_File {
    void* handle;              // 文件句柄
    const char* mode;          // 打开模式
    CN_Buffer* buffer;         // 缓冲区
} CN_File;

// IO 接口
CN_File* CN_FileOpen(const char* filename, const char* mode);
int CN_FileClose(CN_File* file);
size_t CN_FileRead(CN_File* file, void* buffer, size_t size);
size_t CN_FileWrite(CN_File* file, const void* buffer, size_t size);
int CN_FileSeek(CN_File* file, long offset, int origin);
long CN_FileTell(CN_File* file);
```

### 4.2 网络库（net）

#### 4.2.1 模块职责
- Socket 编程
- HTTP 客户端/服务器
- TCP/UDP 通信
- 异步 I/O

#### 4.2.2 目录结构
```
src/stdlib/net/
├── include/
│   └── net.h                 # 网络库公共接口
├── src/
│   ├── net_core.c            # 网络库核心实现
│   ├── socket.c              # Socket 编程
│   ├── tcp.c                 # TCP 通信
│   ├── udp.c                 # UDP 通信
│   ├── http.c                # HTTP 协议
│   └── async.c               // 异步 I/O
└── README.md                 # 模块文档
```

#### 4.2.3 接口设计
```c
// Socket 结构
typedef struct CN_Socket {
    int fd;                    // 文件描述符
    int domain;                // 地址族
    int type;                  // Socket 类型
    int protocol;              // 协议
} CN_Socket;

// 网络接口
CN_Socket* CN_SocketCreate(int domain, int type, int protocol);
int CN_SocketBind(CN_Socket* socket, const char* address, int port);
int CN_SocketListen(CN_Socket* socket, int backlog);
CN_Socket* CN_SocketAccept(CN_Socket* socket);
int CN_SocketConnect(CN_Socket* socket, const char* address, int port);
int CN_SocketSend(CN_Socket* socket, const void* buffer, size_t size);
int CN_SocketRecv(CN_Socket* socket, void* buffer, size_t size);
void CN_SocketClose(CN_Socket* socket);
```

### 4.3 操作系统接口（os）

#### 4.3.1 模块职责
- 进程管理
- 线程管理
- 文件系统操作
- 系统信息获取

#### 4.3.2 目录结构
```
src/stdlib/os/
├── include/
│   └── os.h                  # OS 公共接口
├── src/
│   ├── os_core.c             # OS 核心实现
│   ├── process.c             # 进程管理
│   ├── thread.c              // 线程管理
│   ├── filesystem.c          # 文件系统操作
│   └── system.c              // 系统信息
└── README.md                 # 模块文档
```

#### 4.3.3 接口设计
```c
// 进程结构
typedef struct CN_Process {
    int pid;                   // 进程 ID
    CN_Thread* main_thread;    // 主线程
} CN_Process;

// 线程结构
typedef struct CN_Thread {
    int tid;                   // 线程 ID
    void* handle;              // 线程句柄
    void* (*routine)(void*);   // 线程函数
    void* arg;                 // 线程参数
} CN_Thread;

// OS 接口
CN_Process* OS_ProcessCreate(const char* command);
int OS_ProcessWait(CN_Process* process, int* status);
void OS_ProcessTerminate(CN_Process* process, int exit_code);
CN_Thread* OS_ThreadCreate(void* (*routine)(void*), void* arg);
int OS_ThreadJoin(CN_Thread* thread);
void OS_ThreadExit(void* result);
```

### 4.4 图形界面（gui）

#### 4.4.1 模块职责
- 窗口管理
- 事件处理
- 绘图接口
- 控件库

#### 4.4.2 目录结构
```
src/stdlib/gui/
├── include/
│   └── gui.h                 # GUI 公共接口
├── src/
│   ├── gui_core.c            # GUI 核心实现
│   ├── window.c              # 窗口管理
│   ├── event.c               // 事件处理
│   ├── graphics.c            # 绘图接口
│   └── widget.c              # 控件库
└── README.md                 # 模块文档
```

#### 4.4.3 接口设计
```c
// 窗口结构
typedef struct CN_Window {
    void* handle;              // 窗口句柄
    const char* title;         // 窗口标题
    int x, y;                  // 窗口位置
    int width, height;         // 窗口大小
    CN_Widget* root_widget;    // 根控件
} CN_Window;

// GUI 接口
CN_Window* CN_WindowCreate(const char* title, int x, int y, int width, int height);
void CN_WindowShow(CN_Window* window);
void CN_WindowHide(CN_Window* window);
void CN_WindowDestroy(CN_Window* window);
void CN_WindowSetSize(CN_Window* window, int width, int height);
void CN_WindowSetTitle(CN_Window* window, const char* title);
```

### 4.5 游戏引擎（game）

#### 4.5.1 模块职责
- 渲染引擎
- 物理引擎
- 音频系统
- 资源管理

#### 4.5.2 目录结构
```
src/stdlib/game/
├── include/
│   └── game.h                # 游戏引擎公共接口
├── src/
│   ├── game_core.c           # 游戏引擎核心实现
│   ├── renderer.c            # 渲染引擎
│   ├── physics.c             # 物理引擎
│   ├── audio.c               # 音频系统
│   └── resource.c            # 资源管理
└── README.md                 # 模块文档
```

#### 4.5.3 接口设计
```c
// 游戏引擎结构
typedef struct CN_GameEngine {
    CN_Renderer* renderer;     // 渲染器
    CN_Physics* physics;      // 物理引擎
    CN_Audio* audio;          // 音频系统
    CN_ResourceManager* resources; // 资源管理器
} CN_GameEngine;

// 游戏引擎接口
CN_GameEngine* CN_GameEngineCreate(void);
void CN_GameEngineDestroy(CN_GameEngine* engine);
int CN_GameEngineInitialize(CN_GameEngine* engine);
void CN_GameEngineRun(CN_GameEngine* engine);
void CN_GameEngineStop(CN_GameEngine* engine);
```

### 4.6 数学库（math）

#### 4.6.1 模块职责
- 基础数学运算
- 三角函数
- 矩阵运算
- 向量运算

#### 4.6.2 目录结构
```
src/stdlib/math/
├── include/
│   └── math.h                # 数学库公共接口
├── src/
│   ├── math_core.c           # 数学库核心实现
│   ├── basic.c               # 基础数学运算
│   ├── trig.c                # 三角函数
│   ├── matrix.c              # 矩阵运算
│   └── vector.c              # 向量运算
└── README.md                 # 模块文档
```

#### 4.6.3 接口设计
```c
// 向量结构
typedef struct CN_Vector3 {
    float x, y, z;
} CN_Vector3;

// 矩阵结构
typedef struct CN_Matrix4 {
    float m[4][4];
} CN_Matrix4;

// 数学库接口
float CN_Sin(float x);
float CN_Cos(float x);
float CN_Tan(float x);
CN_Vector3 CN_Vector3Add(CN_Vector3 a, CN_Vector3 b);
CN_Vector3 CN_Vector3Sub(CN_Vector3 a, CN_Vector3 b);
CN_Vector3 CN_Vector3Mul(CN_Vector3 v, float s);
float CN_Vector3Dot(CN_Vector3 a, CN_Vector3 b);
CN_Vector3 CN_Vector3Cross(CN_Vector3 a, CN_Vector3 b);
CN_Matrix4 CN_Matrix4Identity(void);
CN_Matrix4 CN_Matrix4Multiply(CN_Matrix4 a, CN_Matrix4 b);
```

### 4.7 工具函数（utils）

#### 4.7.1 模块职责
- 字符串处理
- 时间日期
- 随机数生成
- 编码转换

#### 4.7.2 目录结构
```
src/stdlib/utils/
├── include/
│   └── utils.h               # 工具函数公共接口
├── src/
│   ├── utils_core.c          # 工具函数核心实现
│   ├── string.c              # 字符串处理
│   ├── time.c                # 时间日期
│   ├── random.c              # 随机数生成
│   └── encoding.c            # 编码转换
└── README.md                 # 模块文档
```

#### 4.7.3 接口设计
```c
// 字符串接口
char* CN_StringCopy(const char* str);
char* CN_StringConcat(const char* str1, const char* str2);
int CN_StringCompare(const char* str1, const char* str2);
char* CN_StringSubstr(const char* str, int start, int length);

// 时间接口
CN_Time CN_TimeNow(void);
CN_Time CN_TimeAdd(CN_Time t, CN_Duration d);
CN_Duration CN_TimeDiff(CN_Time t1, CN_Time t2);

// 随机数接口
void CN_RandomSeed(unsigned int seed);
int CN_RandomInt(int min, int max);
float CN_RandomFloat(float min, float max);
```

## 5. 工具库模块（utils）

### 5.1 字符串处理（string）

#### 5.1.1 模块职责
- 字符串拼接
- 字符串查找
- 字符串替换
- 字符串分割

#### 5.1.2 目录结构
```
src/utils/string/
├── include/
│   └── string.h              # 字符串处理公共接口
├── src/
│   ├── string_core.c         # 字符串处理核心实现
│   ├── concat.c              # 字符串拼接
│   ├── search.c              # 字符串查找
│   ├── replace.c             # 字符串替换
│   └── split.c               # 字符串分割
└── README.md                 # 模块文档
```

#### 5.1.3 接口设计
```c
// 字符串结构
typedef struct CN_String {
    char* data;                // 字符串数据
    size_t length;             // 字符串长度
    size_t capacity;           // 字符串容量
} CN_String;

// 字符串接口
CN_String* CN_StringCreate(const char* str);
void CN_StringDestroy(CN_String* str);
void CN_StringAppend(CN_String* str, const char* append);
int CN_StringFind(CN_String* str, const char* substr);
void CN_StringReplace(CN_String* str, const char* old, const char* new);
CN_String** CN_StringSplit(CN_String* str, const char* delimiter, size_t* count);
```

### 5.2 容器（container）

#### 5.2.1 模块职责
- 动态数组
- 链表
- 哈希表
- 树结构

#### 5.2.2 目录结构
```
src/utils/container/
├── include/
│   └── container.h           # 容器公共接口
├── src/
│   ├── container_core.c       # 容器核心实现
│   ├── array.c               # 动态数组
│   ├── list.c                # 链表
│   ├── hashtable.c           # 哈希表
│   └── tree.c                # 树结构
└── README.md                 # 模块文档
```

#### 5.2.3 接口设计
```c
// 动态数组结构
typedef struct CN_Array {
    void** data;              // 数据指针
    size_t size;              // 元素数量
    size_t capacity;          // 容量
} CN_Array;

// 链表结构
typedef struct CN_ListNode {
    void* data;               // 数据
    struct CN_ListNode* next; // 下一个节点
} CN_ListNode;

typedef struct CN_List {
    CN_ListNode* head;        // 头节点
    CN_ListNode* tail;        // 尾节点
    size_t size;              // 节点数量
} CN_List;

// 容器接口
CN_Array* CN_ArrayCreate(size_t initial_capacity);
void CN_ArrayDestroy(CN_Array* array);
void CN_ArrayPush(CN_Array* array, void* element);
void* CN_ArrayPop(CN_Array* array);
void* CN_ArrayGet(CN_Array* array, size_t index);

CN_List* CN_ListCreate(void);
void CN_ListDestroy(CN_List* list);
void CN_ListPushBack(CN_List* list, void* data);
void* CN_ListPopBack(CN_List* list);
```

### 5.3 文件操作（file）

#### 5.3.1 模块职责
- 文件路径处理
- 文件属性获取
- 目录遍历
- 文件监控

#### 5.3.2 目录结构
```
src/utils/file/
├── include/
│   └── file.h                # 文件操作公共接口
├── src/
│   ├── file_core.c           # 文件操作核心实现
│   ├── path.c                # 文件路径处理
│   ├── attribute.c           # 文件属性获取
│   ├── directory.c           # 目录遍历
│   └── watch.c               # 文件监控
└── README.md                 # 模块文档
```

#### 5.3.3 接口设计
```c
// 文件路径接口
char* CN_PathJoin(const char* path1, const char* path2);
char* CN_PathDirname(const char* path);
char* CN_PathBasename(const char* path);
int CN_PathExists(const char* path);

// 文件属性接口
CN_FileInfo* CN_FileGetInfo(const char* path);
void CN_FileInfoDestroy(CN_FileInfo* info);
int CN_FileIsDirectory(const char* path);
int CN_FileIsFile(const char* path);

// 目录遍历接口
CN_Directory* CN_DirectoryOpen(const char* path);
void CN_DirectoryClose(CN_Directory* dir);
char* CN_DirectoryRead(CN_Directory* dir);
```

### 5.4 日志系统（log）

#### 5.4.1 模块职责
- 日志级别管理
- 日志格式化
- 日志输出
- 日志轮转

#### 5.4.2 目录结构
```
src/utils/log/
├── include/
│   └── log.h                 # 日志系统公共接口
├── src/
│   ├── log_core.c            # 日志系统核心实现
│   ├── level.c               # 日志级别管理
│   ├── formatter.c           # 日志格式化
│   ├── output.c              # 日志输出
│   └── rotation.c            # 日志轮转
└── README.md                 # 模块文档
```

#### 5.4.3 接口设计
```c
// 日志级别
typedef enum CN_LogLevel {
    LOG_DEBUG,                // 调试
    LOG_INFO,                 // 信息
    LOG_WARNING,              // 警告
    LOG_ERROR,                // 错误
    LOG_FATAL                 // 致命
} CN_LogLevel;

// 日志结构
typedef struct CN_Logger {
    CN_LogLevel level;        // 日志级别
    CN_LogFormatter* formatter; // 格式化器
    CN_LogOutput** outputs;   // 输出目标
    size_t output_count;      // 输出数量
} CN_Logger;

// 日志接口
CN_Logger* CN_LoggerCreate(void);
void CN_LoggerDestroy(CN_Logger* logger);
void CN_LoggerSetLevel(CN_Logger* logger, CN_LogLevel level);
void CN_LoggerLog(CN_Logger* logger, CN_LogLevel level, const char* format, ...);
```

## 6. 模块间依赖关系

### 6.1 编译器模块依赖
```
lexer → (无依赖)
parser → lexer, ast
ast → (无依赖)
ir → ast
codegen → ir, target
optimizer → ir
```

### 6.2 运行时模块依赖
```
gc → memory
memory → (无依赖)
types → (无依赖)
vm → gc, types, memory
```

### 6.3 标准库模块依赖
```
io → (无依赖)
net → io
os → io
gui → (无依赖)
game → gui, math
math → (无依赖)
utils → (无依赖)
```

### 6.4 工具库模块依赖
```
string → (无依赖)
container → (无依赖)
file → (无依赖)
log → io
```

## 7. 总结

CN_Language 核心模块采用模块化设计，各模块职责明确、接口清晰。通过严格的模块划分和接口定义，确保代码的可维护性和可扩展性。每个模块都遵循单一职责原则，代码量控制在合理范围内，便于开发和维护。
