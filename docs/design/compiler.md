# CN_Language 编译器架构设计

## 1. 概述

本文档详细描述 CN_Language 编译器的架构设计，包括编译器的各个组件、数据流、错误处理机制以及优化策略。

## 2. 编译器整体架构

### 2.1 编译器组件

```
源代码 (.cn)
    ↓
[词法分析器 Lexer]
    ↓
Token 流
    ↓
[语法分析器 Parser]
    ↓
抽象语法树 (AST)
    ↓
[语义分析器 Semantic Analyzer]
    ↓
类型检查后的 AST
    ↓
[IR 生成器 IR Generator]
    ↓
中间表示 (IR)
    ↓
[优化器 Optimizer]
    ↓
优化后的 IR
    ↓
[代码生成器 Code Generator]
    ↓
目标代码 (汇编/机器码)
    ↓
[链接器 Linker]
    ↓
可执行文件
```

### 2.2 编译器主结构

```c
// 编译器结构
typedef struct CN_Compiler {
    // 编译器配置
    CN_CompilerConfig* config;

    // 编译器组件
    CN_Lexer* lexer;
    CN_Parser* parser;
    CN_SemanticAnalyzer* semantic_analyzer;
    CN_IRGenerator* ir_generator;
    CN_Optimizer* optimizer;
    CN_CodeGenerator* code_generator;

    // 编译器状态
    CN_CompilerState* state;

    // 错误处理器
    CN_ErrorHandler* error_handler;

    // 符号表
    CN_SymbolTable* symbol_table;

    // 类型系统
    CN_TypeSystem* type_system;
} CN_Compiler;

// 编译器配置
typedef struct CN_CompilerConfig {
    const char* target_triple;    // 目标平台三元组
    int optimization_level;       // 优化级别 (0-3)
    int debug_info;               // 是否生成调试信息
    const char* output_file;      // 输出文件名
    const char** include_paths;   // 包含路径
    size_t include_path_count;    // 包含路径数量
} CN_CompilerConfig;

// 编译器状态
typedef struct CN_CompilerState {
    int error_count;              // 错误数量
    int warning_count;            // 警告数量
    int current_phase;            // 当前编译阶段
    CN_SourceFile* current_file;  // 当前源文件
} CN_CompilerState;
```

## 3. 词法分析器（Lexer）

### 3.1 词法分析器架构

```
源代码输入
    ↓
[字符读取器 Character Reader]
    ↓
字符流
    ↓
[状态机 State Machine]
    ↓
Token 识别
    ↓
[Token 构造器 Token Builder]
    ↓
Token 流输出
```

### 3.2 词法分析器组件

#### 3.2.1 字符读取器（Character Reader）

**职责**：从源代码中读取字符，处理编码和换行

**接口**：
```c
// 字符读取器结构
typedef struct CN_CharReader {
    const char* source;          // 源代码
    size_t length;               // 源代码长度
    size_t position;             // 当前位置
    size_t line;                 // 当前行号
    size_t column;               // 当前列号
} CN_CharReader;

// 字符读取器接口
CN_CharReader* CN_CharReaderCreate(const char* source);
void CN_CharReaderDestroy(CN_CharReader* reader);
char CN_CharReaderPeek(CN_CharReader* reader);
char CN_CharReaderNext(CN_CharReader* reader);
int CN_CharReaderHasNext(CN_CharReader* reader);
```

#### 3.2.2 状态机（State Machine）

**职责**：使用有限状态机识别 Token

**状态定义**：
```c
// 词法分析器状态
typedef enum CN_LexerState {
    STATE_START,                 // 初始状态
    STATE_IDENTIFIER,            // 标识符
    STATE_KEYWORD,               // 关键字
    STATE_INTEGER,               // 整数
    STATE_FLOAT,                 // 浮点数
    STATE_STRING,                // 字符串
    STATE_CHAR,                  // 字符
    STATE_OPERATOR,              // 运算符
    STATE_PUNCTUATOR,            // 标点符号
    STATE_COMMENT,               // 注释
    STATE_ERROR                  // 错误状态
} CN_LexerState;
```

#### 3.2.3 Token 构造器（Token Builder）

**职责**：构造 Token 对象，记录位置信息

**接口**：
```c
// Token 构造器结构
typedef struct CN_TokenBuilder {
    CN_TokenType type;           // Token 类型
    char* value;                 // Token 值
    size_t capacity;             // 值缓冲区容量
    size_t length;               // 值长度
    size_t line;                 // 行号
    size_t column;               // 列号
} CN_TokenBuilder;

// Token 构造器接口
CN_TokenBuilder* CN_TokenBuilderCreate(void);
void CN_TokenBuilderDestroy(CN_TokenBuilder* builder);
void CN_TokenBuilderAppend(CN_TokenBuilder* builder, char c);
CN_Token CN_TokenBuilderBuild(CN_TokenBuilder* builder);
```

### 3.3 中文关键字处理

**关键字表**：
```c
// 中文关键字表
typedef struct CN_Keyword {
    const char* keyword;         // 关键字
    CN_TokenType token_type;     // 对应的 Token 类型
} CN_Keyword;

// 关键字表
static const CN_Keyword CN_KEYWORDS[] = {
    {"如果", TOKEN_IF},
    {"否则", TOKEN_ELSE},
    {"当", TOKEN_WHILE},
    {"循环", TOKEN_FOR},
    {"函数", TOKEN_FUNCTION},
    {"返回", TOKEN_RETURN},
    {"类", TOKEN_CLASS},
    {"结构体", TOKEN_STRUCT},
    {"整数", TOKEN_INT},
    {"浮点", TOKEN_FLOAT},
    {"字符串", TOKEN_STRING},
    {"布尔", TOKEN_BOOL},
    {"真", TOKEN_TRUE},
    {"假", TOKEN_FALSE},
    {"空", TOKEN_NULL},
    // ... 更多关键字
    {NULL, TOKEN_EOF}
};
```

### 3.4 词法错误处理

**错误类型**：
```c
// 词法错误类型
typedef enum CN_LexerError {
    LEX_ERROR_INVALID_CHAR,      // 无效字符
    LEX_ERROR_UNTERMINATED_STRING, // 未终止的字符串
    LEX_ERROR_UNTERMINATED_CHAR,  // 未终止的字符
    LEX_ERROR_INVALID_NUMBER,     // 无效数字
    LEX_ERROR_UNKNOWN_TOKEN       // 未知 Token
} CN_LexerError;
```

## 4. 语法分析器（Parser）

### 4.1 语法分析器架构

```
Token 流输入
    ↓
[Token 缓冲器 Token Buffer]
    ↓
[递归下降分析器 Recursive Descent Parser]
    ↓
AST 节点构造
    ↓
[AST 构建器 AST Builder]
    ↓
AST 输出
```

### 4.2 语法分析器组件

#### 4.2.1 Token 缓冲器（Token Buffer）

**职责**：缓存 Token，支持预读

**接口**：
```c
// Token 缓冲器结构
typedef struct CN_TokenBuffer {
    CN_Token* tokens;            // Token 数组
    size_t capacity;             // 容量
    size_t size;                 // 大小
    size_t position;             // 当前位置
} CN_TokenBuffer;

// Token 缓冲器接口
CN_TokenBuffer* CN_TokenBufferCreate(size_t capacity);
void CN_TokenBufferDestroy(CN_TokenBuffer* buffer);
void CN_TokenBufferPush(CN_TokenBuffer* buffer, CN_Token token);
CN_Token CN_TokenBufferPeek(CN_TokenBuffer* buffer, size_t offset);
CN_Token CN_TokenBufferNext(CN_TokenBuffer* buffer);
```

#### 4.2.2 递归下降分析器（Recursive Descent Parser）

**职责**：使用递归下降方法解析语法

**解析函数**：
```c
// 语法分析器接口
CN_ASTNode* CN_ParseProgram(CN_Parser* parser);
CN_ASTNode* CN_ParseFunctionDecl(CN_Parser* parser);
CN_ASTNode* CN_ParseVariableDecl(CN_Parser* parser);
CN_ASTNode* CN_ParseStatement(CN_Parser* parser);
CN_ASTNode* CN_ParseExpression(CN_Parser* parser);
CN_ASTNode* CN_ParseTerm(CN_Parser* parser);
CN_ASTNode* CN_ParseFactor(CN_Parser* parser);
```

#### 4.2.3 AST 构建器（AST Builder）

**职责**：构建 AST 节点

**接口**：
```c
// AST 构建器接口
CN_ASTNode* CN_ASTCreateProgram(CN_ASTNode** declarations, size_t count);
CN_ASTNode* CN_ASTCreateFunctionDecl(const char* name, CN_Type* return_type,
                                     CN_ASTNode** params, size_t param_count,
                                     CN_ASTNode* body);
CN_ASTNode* CN_ASTCreateVariableDecl(const char* name, CN_Type* type, CN_ASTNode* init);
CN_ASTNode* CN_ASTCreateBinaryExpr(CN_ASTNode* left, CN_TokenType op, CN_ASTNode* right);
CN_ASTNode* CN_ASTCreateUnaryExpr(CN_TokenType op, CN_ASTNode* operand);
CN_ASTNode* CN_ASTCreateCallExpr(const char* name, CN_ASTNode** args, size_t arg_count);
```

### 4.3 语法规则

#### 4.3.1 表达式语法

```
表达式 ::= 赋值表达式
赋值表达式 ::= 逻辑或表达式 ('=' 赋值表达式)?
逻辑或表达式 ::= 逻辑与表达式 ('||' 逻辑与表达式)*
逻辑与表达式 ::= 相等表达式 ('&&' 相等表达式)*
相等表达式 ::= 关系表达式 ('==' | '!=' 关系表达式)*
关系表达式 ::= 移位表达式 ('<' | '>' | '<=' | '>=' 移位表达式)*
移位表达式 ::= 加法表达式 ('<<' | '>>' 加法表达式)*
加法表达式 ::= 乘法表达式 ('+' | '-' 乘法表达式)*
乘法表达式 ::= 一元表达式 ('*' | '/' | '%' 一元表达式)*
一元表达式 ::= ('+' | '-' | '!' | '~') 一元表达式 | 后缀表达式
后缀表达式 ::= 主表达式 ('(' 参数列表? ')' | '[' 表达式 ']' | '.' 标识符)*
主表达式 ::= 标识符 | 字面量 | '(' 表达式 ')'
```

#### 4.3.2 语句语法

```
语句 ::= 表达式语句 | 块语句 | if 语句 | while 语句 | for 语句 | return 语句
表达式语句 ::= 表达式 ';'
块语句 ::= '{' 语句* '}'
if 语句 ::= '如果' '(' 表达式 ')' 语句 ('否则' 语句)?
while 语句 ::= '当' '(' 表达式 ')' 语句
for 语句 ::= '循环' '(' 变量声明? ';' 表达式? ';' 表达式? ')' 语句
return 语句 ::= '返回' 表达式? ';'
```

#### 4.3.3 声明语法

```
声明 ::= 函数声明 | 变量声明
函数声明 ::= '函数' 标识符 '(' 参数列表? ')' '->' 类型 块语句
参数列表 ::= 参数 (',' 参数)*
参数 ::= 标识符 ':' 类型
变量声明 ::= 类型 标识符 ('=' 表达式)? ';'
类型 ::= '整数' | '浮点' | '字符串' | '布尔' | 标识符 | 类型 '[' ']'
```

### 4.4 语法错误处理

**错误恢复策略**：
1. 同步恢复：在遇到错误时跳过到下一个同步点（如分号、右括号）
2. 短语级恢复：插入缺失的 Token
3. 全局纠正：尝试纠正错误

**错误类型**：
```c
// 语法错误类型
typedef enum CN_ParseError {
    PARSE_ERROR_UNEXPECTED_TOKEN,    // 意外的 Token
    PARSE_ERROR_MISSING_TOKEN,       // 缺少 Token
    PARSE_ERROR_INVALID_SYNTAX,      // 无效语法
    PARSE_ERROR_TYPE_MISMATCH,       // 类型不匹配
    PARSE_ERROR_REDECLARATION,        // 重复声明
    PARSE_ERROR_UNDEFINED_SYMBOL     // 未定义符号
} CN_ParseError;
```

## 5. 语义分析器（Semantic Analyzer）

### 5.1 语义分析器架构

```
AST 输入
    ↓
[符号表构建器 Symbol Table Builder]
    ↓
符号表
    ↓
[类型检查器 Type Checker]
    ↓
类型检查后的 AST
    ↓
[作用域解析器 Scope Resolver]
    ↓
作用域解析后的 AST
```

### 5.2 符号表（Symbol Table）

#### 5.2.1 符号表结构

```c
// 符号类型
typedef enum CN_SymbolType {
    SYMBOL_VARIABLE,              // 变量
    SYMBOL_FUNCTION,              // 函数
    SYMBOL_TYPE,                  // 类型
    SYMBOL_PARAMETER,             // 参数
    SYMBOL_CLASS,                 // 类
    SYMBOL_MEMBER                 // 成员
} CN_SymbolType;

// 符号结构
typedef struct CN_Symbol {
    const char* name;             // 符号名
    CN_SymbolType type;           // 符号类型
    CN_Type* data_type;           // 数据类型
    CN_ASTNode* declaration;      // 声明节点
    CN_Scope* scope;              // 所属作用域
    int is_defined;              // 是否已定义
} CN_Symbol;

// 符号表结构
typedef struct CN_SymbolTable {
    CN_Symbol** symbols;          // 符号数组
    size_t capacity;             // 容量
    size_t size;                 // 大小
    CN_SymbolTable* parent;      // 父符号表
} CN_SymbolTable;

// 符号表接口
CN_SymbolTable* CN_SymbolTableCreate(CN_SymbolTable* parent);
void CN_SymbolTableDestroy(CN_SymbolTable* table);
CN_Symbol* CN_SymbolTableLookup(CN_SymbolTable* table, const char* name);
CN_Symbol* CN_SymbolTableInsert(CN_SymbolTable* table, CN_Symbol* symbol);
```

#### 5.2.2 作用域（Scope）

```c
// 作用域结构
typedef struct CN_Scope {
    const char* name;             // 作用域名
    CN_ScopeType type;           // 作用域类型
    CN_Scope* parent;            // 父作用域
    CN_Scope** children;         // 子作用域
    size_t child_count;          // 子作用域数量
    CN_SymbolTable* symbols;     // 符号表
} CN_Scope;

// 作用域类型
typedef enum CN_ScopeType {
    SCOPE_GLOBAL,                // 全局作用域
    SCOPE_FUNCTION,              // 函数作用域
    SCOPE_BLOCK,                 // 块作用域
    SCOPE_CLASS                  // 类作用域
} CN_ScopeType;

// 作用域接口
CN_Scope* CN_ScopeCreate(const char* name, CN_ScopeType type, CN_Scope* parent);
void CN_ScopeDestroy(CN_Scope* scope);
CN_Scope* CN_ScopeEnter(CN_Scope* scope);
CN_Scope* CN_ScopeExit(CN_Scope* scope);
CN_Symbol* CN_ScopeLookup(CN_Scope* scope, const char* name);
```

### 5.3 类型检查（Type Checking）

#### 5.3.1 类型检查器

```c
// 类型检查器结构
typedef struct CN_TypeChecker {
    CN_Scope* current_scope;     // 当前作用域
    CN_TypeSystem* type_system;  // 类型系统
    CN_ErrorHandler* error_handler; // 错误处理器
} CN_TypeChecker;

// 类型检查器接口
CN_Type* CN_TypeCheckExpression(CN_TypeChecker* checker, CN_ASTNode* expr);
CN_Type* CN_TypeCheckStatement(CN_TypeChecker* checker, CN_ASTNode* stmt);
CN_Type* CN_TypeCheckDeclaration(CN_TypeChecker* checker, CN_ASTNode* decl);
```

#### 5.3.2 类型转换规则

```c
// 类型转换
typedef enum CN_TypeConversion {
    CONV_IMPLICIT,              // 隐式转换
    CONV_EXPLICIT,              // 显式转换
    CONV_NONE                   // 无转换
} CN_TypeConversion;

// 类型转换检查
CN_TypeConversion CN_CheckTypeConversion(CN_Type* from, CN_Type* to);
int CN_CanConvertImplicitly(CN_Type* from, CN_Type* to);
CN_Type* CN_ApplyTypeConversion(CN_Type* from, CN_Type* to);
```

### 5.4 语义错误处理

**错误类型**：
```c
// 语义错误类型
typedef enum CN_SemanticError {
    SEM_ERROR_TYPE_MISMATCH,     // 类型不匹配
    SEM_ERROR_UNDEFINED_SYMBOL,   // 未定义符号
    SEM_ERROR_REDECLARATION,      // 重复声明
    SEM_ERROR_INVALID_OPERATION,  // 无效操作
    SEM_ERROR_NOT_ASSIGNABLE,     // 不可赋值
    SEM_ERROR_ARGUMENT_COUNT,     // 参数数量错误
    SEM_ERROR_ARGUMENT_TYPE       // 参数类型错误
} CN_SemanticError;
```

## 6. IR 生成器（IR Generator）

### 6.1 IR 生成器架构

```
AST 输入
    ↓
[IR 构建器 IR Builder]
    ↓
IR 指令
    ↓
[IR 模块 IR Module]
    ↓
IR 输出
```

### 6.2 IR 指令集

#### 6.2.1 基本指令

```c
// IR 指令操作码
typedef enum CN_IROpcode {
    // 算术指令
    IR_ADD,                      // 加法
    IR_SUB,                      // 减法
    IR_MUL,                      // 乘法
    IR_DIV,                      // 除法
    IR_MOD,                      // 取模

    // 位运算指令
    IR_AND,                      // 按位与
    IR_OR,                       // 按位或
    IR_XOR,                      // 按位异或
    IR_SHL,                      // 左移
    IR_SHR,                      // 右移

    // 比较指令
    IR_EQ,                       // 相等
    IR_NE,                       // 不等
    IR_LT,                       // 小于
    IR_GT,                       // 大于
    IR_LE,                       // 小于等于
    IR_GE,                       // 大于等于

    // 内存指令
    IR_LOAD,                     // 加载
    IR_STORE,                    // 存储
    IR_ALLOC,                    // 分配
    IR_FREE,                     // 释放

    // 控制流指令
    IR_JUMP,                     // 跳转
    IR_BRANCH,                   // 条件跳转
    IR_CALL,                     // 调用
    IR_RET,                      // 返回

    // 其他指令
    IR_PHI,                      // Phi 指令
    IR_CAST,                     // 类型转换
    IR_NOP                       // 空操作
} CN_IROpcode;
```

#### 6.2.2 IR 指令结构

```c
// IR 指令结构
typedef struct CN_IRInstruction {
    CN_IROpcode opcode;          // 操作码
    CN_Value* operands[3];       // 操作数
    CN_Value* result;            // 结果
    CN_BasicBlock* block;        // 所属基本块
    CN_IRInstruction* prev;      // 前一条指令
    CN_IRInstruction* next;      // 后一条指令
} CN_IRInstruction;

// IR 值
typedef enum CN_ValueKind {
    VALUE_CONSTANT,              // 常量
    VALUE_REGISTER,              // 寄存器
    VALUE_PARAMETER,             // 参数
    VALUE_GLOBAL                 // 全局变量
} CN_ValueKind;

typedef struct CN_Value {
    CN_ValueKind kind;           // 值类型
    CN_Type* type;               // 数据类型
    union {
        int64_t int_val;         // 整数值
        double float_val;        // 浮点值
        const char* str_val;     // 字符串值
        int reg_num;             // 寄存器编号
        const char* name;        // 名称
    };
} CN_Value;
```

### 6.3 IR 基本块

```c
// 基本块结构
typedef struct CN_BasicBlock {
    const char* name;            // 基本块名
    CN_IRInstruction* first;     // 第一条指令
    CN_IRInstruction* last;     // 最后一条指令
    CN_BasicBlock** predecessors; // 前驱基本块
    size_t pred_count;          // 前驱数量
    CN_BasicBlock** successors;  // 后继基本块
    size_t succ_count;          // 后继数量
    CN_IRFunction* function;     // 所属函数
} CN_BasicBlock;
```

### 6.4 IR 函数

```c
// IR 函数结构
typedef struct CN_IRFunction {
    const char* name;            // 函数名
    CN_Type* return_type;        // 返回类型
    CN_Value** parameters;       // 参数列表
    size_t param_count;         // 参数数量
    CN_BasicBlock* entry;        // 入口基本块
    CN_BasicBlock** blocks;      // 基本块列表
    size_t block_count;         // 基本块数量
    CN_IRModule* module;         // 所属模块
} CN_IRFunction;
```

### 6.5 IR 模块

```c
// IR 模块结构
typedef struct CN_IRModule {
    const char* name;            // 模块名
    CN_IRFunction** functions;   // 函数列表
    size_t function_count;      // 函数数量
    CN_GlobalVariable** globals; // 全局变量列表
    size_t global_count;        // 全局变量数量
} CN_IRModule;
```

### 6.6 IR 生成策略

#### 6.6.1 表达式生成

```c
// 表达式生成
CN_Value* CN_GenerateExpression(CN_IRBuilder* builder, CN_ASTNode* expr) {
    switch (expr->type) {
        case AST_BINARY_EXPR:
            return CN_GenerateBinaryExpr(builder, expr);
        case AST_UNARY_EXPR:
            return CN_GenerateUnaryExpr(builder, expr);
        case AST_CALL_EXPR:
            return CN_GenerateCallExpr(builder, expr);
        case AST_IDENTIFIER:
            return CN_GenerateIdentifier(builder, expr);
        case AST_LITERAL:
            return CN_GenerateLiteral(builder, expr);
        default:
            return NULL;
    }
}
```

#### 6.6.2 语句生成

```c
// 语句生成
void CN_GenerateStatement(CN_IRBuilder* builder, CN_ASTNode* stmt) {
    switch (stmt->type) {
        case AST_IF_STATEMENT:
            CN_GenerateIfStmt(builder, stmt);
            break;
        case AST_WHILE_STATEMENT:
            CN_GenerateWhileStmt(builder, stmt);
            break;
        case AST_FOR_STATEMENT:
            CN_GenerateForStmt(builder, stmt);
            break;
        case AST_RETURN_STATEMENT:
            CN_GenerateReturnStmt(builder, stmt);
            break;
        case AST_EXPRESSION_STMT:
            CN_GenerateExpressionStmt(builder, stmt);
            break;
        default:
            break;
    }
}
```

## 7. 优化器（Optimizer）

### 7.1 优化器架构

```
IR 输入
    ↓
[优化遍管理器 Pass Manager]
    ↓
[优化遍 Pass 1]
    ↓
[优化遍 Pass 2]
    ↓
...
    ↓
[优化遍 Pass N]
    ↓
优化后的 IR 输出
```

### 7.2 优化遍（Optimization Pass）

#### 7.2.1 优化遍结构

```c
// 优化遍结构
typedef struct CN_OptimizationPass {
    const char* name;            // 优化遍名称
    int (*run)(CN_IRModule* module); // 运行函数
    int enabled;                 // 是否启用
    int (*verify)(CN_IRModule* module); // 验证函数
} CN_OptimizationPass;

// 优化遍管理器结构
typedef struct CN_PassManager {
    CN_OptimizationPass** passes; // 优化遍列表
    size_t pass_count;           // 优化遍数量
    CN_IRModule* module;         // IR 模块
} CN_PassManager;

// 优化遍管理器接口
CN_PassManager* CN_PassManagerCreate(CN_IRModule* module);
void CN_PassManagerDestroy(CN_PassManager* manager);
void CN_PassManagerAddPass(CN_PassManager* manager, CN_OptimizationPass* pass);
int CN_PassManagerRun(CN_PassManager* manager);
```

#### 7.2.2 常用优化遍

**常量折叠（Constant Folding）**：
```c
// 常量折叠优化遍
int CN_ConstantFoldingPass(CN_IRModule* module) {
    int changed = 0;
    for (size_t i = 0; i < module->function_count; i++) {
        CN_IRFunction* func = module->functions[i];
        for (size_t j = 0; j < func->block_count; j++) {
            CN_BasicBlock* block = func->blocks[j];
            CN_IRInstruction* instr = block->first;
            while (instr != NULL) {
                if (CN_IsConstantFoldable(instr)) {
                    CN_Value* result = CN_FoldConstant(instr);
                    if (result != NULL) {
                        CN_ReplaceAllUsesWith(instr->result, result);
                        CN_RemoveInstruction(instr);
                        changed = 1;
                    }
                }
                instr = instr->next;
            }
        }
    }
    return changed;
}
```

**死代码消除（Dead Code Elimination）**：
```c
// 死代码消除优化遍
int CN_DeadCodeEliminationPass(CN_IRModule* module) {
    int changed = 0;
    for (size_t i = 0; i < module->function_count; i++) {
        CN_IRFunction* func = module->functions[i];
        // 标记可达代码
        CN_MarkReachableCode(func);
        // 删除不可达代码
        for (size_t j = 0; j < func->block_count; j++) {
            CN_BasicBlock* block = func->blocks[j];
            if (!block->reachable) {
                CN_RemoveBasicBlock(block);
                changed = 1;
            }
        }
    }
    return changed;
}
```

**公共子表达式消除（Common Subexpression Elimination）**：
```c
// 公共子表达式消除优化遍
int CN_CommonSubexpressionEliminationPass(CN_IRModule* module) {
    int changed = 0;
    for (size_t i = 0; i < module->function_count; i++) {
        CN_IRFunction* func = module->functions[i];
        CN_ValueTable* table = CN_ValueTableCreate();
        for (size_t j = 0; j < func->block_count; j++) {
            CN_BasicBlock* block = func->blocks[j];
            CN_IRInstruction* instr = block->first;
            while (instr != NULL) {
                CN_Value* existing = CN_ValueTableLookup(table, instr);
                if (existing != NULL) {
                    CN_ReplaceAllUsesWith(instr->result, existing);
                    CN_RemoveInstruction(instr);
                    changed = 1;
                } else {
                    CN_ValueTableInsert(table, instr);
                }
                instr = instr->next;
            }
        }
        CN_ValueTableDestroy(table);
    }
    return changed;
}
```

**循环优化（Loop Optimization）**：
```c
// 循环优化遍
int CN_LoopOptimizationPass(CN_IRModule* module) {
    int changed = 0;
    for (size_t i = 0; i < module->function_count; i++) {
        CN_IRFunction* func = module->functions[i];
        // 识别循环
        CN_LoopInfo** loops = CN_IdentifyLoops(func);
        size_t loop_count = CN_GetLoopCount(func);
        // 对每个循环进行优化
        for (size_t j = 0; j < loop_count; j++) {
            CN_LoopInfo* loop = loops[j];
            // 循环不变代码外提
            changed |= CN_LoopInvariantCodeMotion(loop);
            // 循环展开
            changed |= CN_LoopUnrolling(loop);
            // 循环强度削减
            changed |= CN_LoopStrengthReduction(loop);
        }
        CN_FreeLoops(loops, loop_count);
    }
    return changed;
}
```

### 7.3 优化级别

```c
// 优化级别
typedef enum CN_OptimizationLevel {
    OPT_NONE,                    // 无优化
    OPT_BASIC,                   // 基础优化
    OPT_STANDARD,                // 标准优化
    OPT_AGGRESSIVE               // 激进优化
} CN_OptimizationLevel;

// 优化级别配置
void CN_SetOptimizationLevel(CN_PassManager* manager, CN_OptimizationLevel level) {
    switch (level) {
        case OPT_NONE:
            CN_DisableAllPasses(manager);
            break;
        case OPT_BASIC:
            CN_EnableBasicPasses(manager);
            break;
        case OPT_STANDARD:
            CN_EnableStandardPasses(manager);
            break;
        case OPT_AGGRESSIVE:
            CN_EnableAggressivePasses(manager);
            break;
    }
}
```

## 8. 代码生成器（Code Generator）

### 8.1 代码生成器架构

```
IR 输入
    ↓
[目标平台选择 Target Selection]
    ↓
[寄存器分配 Register Allocation]
    ↓
[栈帧分配 Stack Frame Allocation]
    ↓
[指令选择 Instruction Selection]
    ↓
[指令调度 Instruction Scheduling]
    ↓
[代码生成 Code Emission]
    ↓
目标代码输出
```

### 8.2 目标平台（Target）

```c
// 目标平台结构
typedef struct CN_Target {
    const char* name;            // 平台名称
    const char* triple;          // 目标三元组
    size_t pointer_size;         // 指针大小
    size_t word_size;            // 字大小
    int endianness;              // 字节序 (0: 小端, 1: 大端)
    CN_Register** registers;     // 寄存器列表
    size_t register_count;       // 寄存器数量
    CN_CallingConvention* calling_convention; // 调用约定
} CN_Target;

// 寄存器结构
typedef struct CN_Register {
    const char* name;            // 寄存器名
    int number;                  // 寄存器编号
    int is_float;                // 是否为浮点寄存器
    int is_callee_saved;         // 是否为被调用者保存
} CN_Register;

// 目标平台接口
CN_Target* CN_TargetCreate(const char* triple);
void CN_TargetDestroy(CN_Target* target);
CN_Target* CN_GetDefaultTarget(void);
```

### 8.3 寄存器分配（Register Allocation）

#### 8.3.1 寄存器分配器

```c
// 寄存器分配器结构
typedef struct CN_RegisterAllocator {
    CN_Target* target;           // 目标平台
    CN_Register** available;     // 可用寄存器
    size_t available_count;      // 可用寄存器数量
    CN_Value** reg_map;          // 值到寄存器的映射
    size_t reg_map_size;         // 映射表大小
    CN_StackSlot** spills;       // 溢出到栈的值
    size_t spill_count;          // 溢出数量
} CN_RegisterAllocator;

// 寄存器分配器接口
CN_RegisterAllocator* CN_RegisterAllocatorCreate(CN_Target* target);
void CN_RegisterAllocatorDestroy(CN_RegisterAllocator* allocator);
CN_Register* CN_AllocateRegister(CN_RegisterAllocator* allocator, CN_Value* value);
void CN_FreeRegister(CN_RegisterAllocator* allocator, CN_Register* reg);
CN_StackSlot* CN_SpillToStack(CN_RegisterAllocator* allocator, CN_Value* value);
```

#### 8.3.2 寄存器分配算法

**线性扫描（Linear Scan）**：
```c
// 线性扫描寄存器分配
int CN_LinearScanAllocation(CN_RegisterAllocator* allocator, CN_IRFunction* func) {
    // 1. 计算每个值的活跃区间
    CN_LiveInterval** intervals = CN_ComputeLiveIntervals(func);
    size_t interval_count = CN_GetIntervalCount(func);

    // 2. 按起始位置排序
    CN_SortLiveIntervals(intervals, interval_count);

    // 3. 线性扫描分配寄存器
    CN_Register** active = CN_CreateActiveList();
    size_t active_count = 0;
    for (size_t i = 0; i < interval_count; i++) {
        CN_LiveInterval* interval = intervals[i];
        // 移除已结束的活跃区间
        CN_ExpireOldIntervals(active, &active_count, interval);
        // 尝试分配寄存器
        CN_Register* reg = CN_FindFreeRegister(allocator);
        if (reg != NULL) {
            CN_AssignRegister(interval, reg);
            CN_InsertActive(active, &active_count, interval);
        } else {
            // 溢出到栈
            CN_SpillInterval(allocator, interval);
        }
    }

    CN_FreeActiveList(active, active_count);
    CN_FreeLiveIntervals(intervals, interval_count);
    return 0;
}
```

### 8.4 栈帧分配（Stack Frame Allocation）

```c
// 栈帧结构
typedef struct CN_StackFrame {
    size_t size;                // 栈帧大小
    size_t locals_size;          // 局部变量大小
    size_t spills_size;         // 溢出变量大小
    CN_StackSlot** locals;      // 局部变量槽位
    size_t local_count;         // 局部变量数量
    CN_StackSlot** spills;      // 溢出变量槽位
    size_t spill_count;         // 溢出变量数量
} CN_StackFrame;

// 栈槽结构
typedef struct CN_StackSlot {
    int offset;                 // 偏移量
    CN_Type* type;              // 类型
    CN_Value* value;            // 对应的值
} CN_StackSlot;

// 栈帧分配器接口
CN_StackFrame* CN_StackFrameCreate(CN_IRFunction* func);
void CN_StackFrameDestroy(CN_StackFrame* frame);
CN_StackSlot* CN_AllocateStackSlot(CN_StackFrame* frame, CN_Type* type);
int CN_GetStackOffset(CN_StackFrame* frame, CN_StackSlot* slot);
```

### 8.5 指令选择（Instruction Selection）

```c
// 指令选择器结构
typedef struct CN_InstructionSelector {
    CN_Target* target;           // 目标平台
    CN_CodeBuffer* code_buffer; // 代码缓冲区
} CN_InstructionSelector;

// 指令选择器接口
CN_InstructionSelector* CN_InstructionSelectorCreate(CN_Target* target);
void CN_InstructionSelectorDestroy(CN_InstructionSelector* selector);
int CN_SelectInstruction(CN_InstructionSelector* selector, CN_IRInstruction* instr);
```

### 8.6 代码生成（Code Emission）

```c
// 代码缓冲区结构
typedef struct CN_CodeBuffer {
    unsigned char* data;         // 代码数据
    size_t size;                 // 当前大小
    size_t capacity;             // 容量
    CN_Relocation** relocations; // 重定位表
    size_t relocation_count;     // 重定位数量
} CN_CodeBuffer;

// 重定位结构
typedef struct CN_Relocation {
    size_t offset;               // 重定位偏移
    CN_RelocationType type;      // 重定位类型
    const char* symbol;          // 符号名
    int64_t addend;              // 加数
} CN_Relocation;

// 代码缓冲区接口
CN_CodeBuffer* CN_CodeBufferCreate(size_t initial_capacity);
void CN_CodeBufferDestroy(CN_CodeBuffer* buffer);
void CN_CodeBufferEmitByte(CN_CodeBuffer* buffer, unsigned char byte);
void CN_CodeBufferEmitWord(CN_CodeBuffer* buffer, uint16_t word);
void CN_CodeBufferEmitDWord(CN_CodeBuffer* buffer, uint32_t dword);
void CN_CodeBufferEmitQWord(CN_CodeBuffer* buffer, uint64_t qword);
void CN_CodeBufferAddRelocation(CN_CodeBuffer* buffer, CN_Relocation* reloc);
```

## 9. 错误处理（Error Handling）

### 9.1 错误处理器

```c
// 错误级别
typedef enum CN_ErrorLevel {
    ERROR_INFO,                  // 信息
    ERROR_WARNING,               // 警告
    ERROR_ERROR,                 // 错误
    ERROR_FATAL                  // 致命错误
} CN_ErrorLevel;

// 错误信息结构
typedef struct CN_ErrorInfo {
    CN_ErrorLevel level;         // 错误级别
    const char* message;         // 错误消息
    const char* file;            // 文件名
    size_t line;                 // 行号
    size_t column;               // 列号
    CN_ErrorCode code;           // 错误代码
} CN_ErrorInfo;

// 错误处理器结构
typedef struct CN_ErrorHandler {
    CN_ErrorInfo** errors;       // 错误列表
    size_t error_count;         // 错误数量
    size_t capacity;            // 容量
    int error_limit;             // 错误限制
    int stop_on_error;           // 遇到错误是否停止
} CN_ErrorHandler;

// 错误处理器接口
CN_ErrorHandler* CN_ErrorHandlerCreate(void);
void CN_ErrorHandlerDestroy(CN_ErrorHandler* handler);
void CN_ReportError(CN_ErrorHandler* handler, CN_ErrorLevel level,
                    const char* file, size_t line, size_t column,
                    CN_ErrorCode code, const char* format, ...);
int CN_HasErrors(CN_ErrorHandler* handler);
void CN_PrintErrors(CN_ErrorHandler* handler);
```

### 9.2 错误恢复策略

1. **恐慌模式（Panic Mode）**：在遇到错误时跳过到下一个同步点
2. **错误产生器（Error Productions）**：在语法中添加错误产生式
3. **全局纠正（Global Correction）**：尝试纠正错误

## 10. 调试信息（Debug Information）

### 10.1 调试信息生成

```c
// 调试信息结构
typedef struct CN_DebugInfo {
    CN_SourceFile** files;       // 源文件列表
    size_t file_count;          // 文件数量
    CN_FunctionDebug** functions; // 函数调试信息
    size_t function_count;      // 函数数量
} CN_DebugInfo;

// 源文件结构
typedef struct CN_SourceFile {
    const char* filename;       // 文件名
    char** lines;               // 行列表
    size_t line_count;          // 行数量
} CN_SourceFile;

// 函数调试信息结构
typedef struct CN_FunctionDebug {
    const char* name;           // 函数名
    CN_LineNumberEntry** line_numbers; // 行号表
    size_t line_number_count;   // 行号数量
    CN_LocalVariable** locals;  // 局部变量
    size_t local_count;         // 局部变量数量
} CN_FunctionDebug;

// 行号表项结构
typedef struct CN_LineNumberEntry {
    uint64_t address;           // 地址
    size_t line;                // 行号
    CN_SourceFile* file;        // 源文件
} CN_LineNumberEntry;

// 局部变量结构
typedef struct CN_LocalVariable {
    const char* name;           // 变量名
    CN_Type* type;              // 类型
    int location;               // 位置 (寄存器或栈偏移)
    CN_LocationKind location_kind; // 位置类型
} CN_LocalVariable;
```

## 11. 总结

CN_Language 编译器采用模块化设计，各组件职责明确、接口清晰。通过多阶段编译流程，从源代码到目标代码的转换过程清晰可控。优化器提供了多种优化遍，可以根据需要选择不同的优化级别。错误处理机制完善，支持错误恢复和调试信息生成，为开发者提供良好的开发体验。
