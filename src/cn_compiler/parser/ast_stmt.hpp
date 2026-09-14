// AST 语句节点族（D1 125-a 拆分）
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/source_location.hpp"

#include "cn_compiler/parser/ast_base.hpp"
#include "cn_compiler/parser/ast_expr.hpp"

namespace cn_compiler {

// ==================== 语句节点 ====================

// 表达式语句：仅由表达式构成（如函数调用、自增表达式）
class ExprStmt : public Stmt {
public:
    explicit ExprStmt(std::unique_ptr<Expr> expr)
        : Stmt(NodeType::ExprStmt), expr(std::move(expr)) {}
    void accept(AstVisitor& visitor) override { visitor.visitExprStmt(this); }

    std::unique_ptr<Expr> expr;  // 表达式
};

// 函数指针类型信息：C风格 返回类型(*名)(参数类型列表)（规格书5.8）
// 用于变量声明（整32(*回调)(整32, 整32)）与参数声明（整32(*func)(整32, 整32)）
struct FuncPtrTypeInfo {
    std::string returnType;                    // 返回类型（如 整32）
    std::vector<std::string> paramTypes;       // 参数类型列表（如 [整32, 整32]）
    std::string name;                          // 函数指针变量名（C风格声明内嵌：整32(*名)(...)）

    bool isFunctionPtr() const { return !returnType.empty(); }  // 是否为函数指针类型
    // 生成规范化类型字符串：函数指针<返回>(参数1,参数2,...)（语义层类型比较用）
    std::string toString() const;
};

// 结构体字段：字段名 + 类型 + 对齐/偏移（语义层布局计算回填，Task 2.7）
struct StructField {
    std::string name;      // 字段名
    std::string type;      // 字段类型（源码类型名，规范化后）
    int offset = 0;        // 字段偏移（字节，语义层计算）
    // 164-a（A4 方案D·plans/023 §十二）：联合体成员「手动释放」标注——语义层放行
    //   拥有型成员但编译器不生成自动释放（对标 Rust ManuallyDrop<T>）；仅联合体
    //   成员位识别；用户须在 不安全 函数 内显式释放（字符串释放 等）
    bool manualRelease = false;
    SourceLocation location;   // 字段声明位置（164-a：联合体成员类型诊断精确位置）
};

// 结构体/联合体声明：结构体 名 { 类型 字段; ... } / 联合体 名 { ... }（Task 2.7）
// 联合体为 isUnion=true：所有字段从偏移0开始，大小 = 最大字段大小（按最大对齐）
class StructDecl : public AstNode {
public:
    StructDecl() : AstNode(NodeType::StructDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitStructDecl(this); }

    std::string name;                        // 类型名（结构体名/联合体名）
    bool isUnion = false;                    // true 表示联合体（共享内存布局）
    std::vector<StructField> fields;         // 字段列表（含布局偏移回填）
    int totalSize = 0;                       // 总大小（字节，语义层计算回填）
    int align = 1;                           // 对齐（字节，语义层计算回填）
    bool layoutComputed = false;             // 布局是否已计算（语义层回填标记）
    // 模块级可见性（Task 3.6，v2.0）：公开: -> Public / 私有: -> Private（默认 Private）
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 枚举成员：成员名 + 值（显式赋值或自动递增，Task 2.7）
struct EnumMember {
    std::string name;      // 成员名
    std::int64_t value;    // 成员值
    bool explicitValue = false;  // 是否为显式赋值
};

// 枚举声明：枚举 名 { 成员, 成员 = 值, ... }（Task 2.7）
// 未赋值成员自动按前一个成员值+1递增（首个默认0）；支持负数值
class EnumDecl : public AstNode {
public:
    EnumDecl() : AstNode(NodeType::EnumDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitEnumDecl(this); }

    std::string name;                        // 枚举类型名
    std::vector<EnumMember> members;         // 成员列表（值已求值）
    // 模块级可见性（Task 3.6，v2.0）：公开: -> Public / 私有: -> Private（默认 Private）
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 变量声明：变量/常量/静态 类型前置（CN规范唯一形态；冒号后置已 A8 收口拒绝）
class VarDecl : public Stmt {
public:
    VarDecl() : Stmt(NodeType::VarDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitVarDecl(this); }

    bool isConst = false;                        // 是否为常量（常量 x = 值）
    bool isStatic = false;                       // 是否为静态变量（静态 类型 x = 值）
    std::string name;                            // 变量名
    std::string typeName;                        // 类型名（为空表示类型推断：变量 x = 10）
    std::unique_ptr<Expr> initializer;           // 初始值（可为空）
    FuncPtrTypeInfo funcPtr;                     // 函数指针类型信息（非空表示本变量为函数指针）
    // ---- 第 4 层（v2.0 决策8/9，P1-4/P3-8）：顶层常量/静态模块级可见性 ----
    // 顶层 常量/静态 声明记录模块级可见性（公开:/私有: 标签段），
    // 供 mergeModules 跨模块合并（公开 常量/静态 才跨模块可见）与链接前缀。
    AccessSpecifier access = AccessSpecifier::Private;
    std::string moduleName;                      // 所属模块（crate 域）名，合并阶段写入
};

// 代码块：{ 语句列表 }
class BlockStmt : public Stmt {
public:
    BlockStmt() : Stmt(NodeType::BlockStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitBlockStmt(this); }

    std::vector<std::unique_ptr<Stmt>> statements;  // 语句列表
};

// 如果语句：如果 (条件) { 真分支 } [否则如果...] [否则 { 否则分支 }]
class IfStmt : public Stmt {
public:
    IfStmt() : Stmt(NodeType::IfStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitIfStmt(this); }

    std::unique_ptr<Expr> condition;      // 条件表达式
    std::unique_ptr<BlockStmt> thenBranch; // 真分支代码块
    std::unique_ptr<Stmt> elseBranch;      // 否则分支：IfStmt（否则如果）或 BlockStmt（否则）
};

// 当循环：当 (条件) { 循环体 }
class WhileStmt : public Stmt {
public:
    WhileStmt() : Stmt(NodeType::WhileStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitWhileStmt(this); }

    std::unique_ptr<Expr> condition;   // 循环条件
    std::unique_ptr<BlockStmt> body;   // 循环体
};

// 循环语句：循环 (初始化; 条件; 更新) { 循环体 }
// 无限循环：循环 { 循环体 }（初始化/条件/更新均为空）
class ForStmt : public Stmt {
public:
    ForStmt() : Stmt(NodeType::ForStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitForStmt(this); }

    std::unique_ptr<Stmt> init;        // 初始化语句（VarDecl 或 ExprStmt，可为空）
    std::unique_ptr<Expr> condition;   // 循环条件（可为空=无限循环）
    std::unique_ptr<Expr> update;      // 更新表达式（可为空）
    std::unique_ptr<BlockStmt> body;   // 循环体
};

// 返回语句：返回 [值]
class ReturnStmt : public Stmt {
public:
    ReturnStmt() : Stmt(NodeType::ReturnStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitReturnStmt(this); }

    std::unique_ptr<Expr> value;  // 返回值（可为空：返回;）
};

// 中断语句：中断（跳出循环/选择）
class BreakStmt : public Stmt {
public:
    BreakStmt() : Stmt(NodeType::BreakStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitBreakStmt(this); }
};

// 继续语句：继续（跳过本次循环剩余部分）
class ContinueStmt : public Stmt {
public:
    ContinueStmt() : Stmt(NodeType::ContinueStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitContinueStmt(this); }
};

// 遍历...中每个 迭代语句（C-2，2026-08）：遍历 容器 中 每个 元素 { 循环体 }
// 语法：遍历 <迭代对象> 中 每个 <变量名> <语句/块>（对标 C++ range-for / Python for-in）
// 语义层（visitRangeForStmt）按容器形态降级为 循环（ForStmt）：
//   - 数组 T[N]        -> 循环 (整64 i=0; i<N; i++) { T 元素 = 容器[i]; 体; }
//   - 类容器（向量<T>）-> 循环 (整64 i=0; i<容器.大小(); i++) { T 元素 = 容器.元素(i); 体; }
//   - 其他形态（须提供 大小()/元素(整64) 方法的类；否则报错）
// 降级树写入 desugared：IR 层 visitRangeForStmt 直接生成降级树
// （名称式迭代对象（标识符/成员/下标）按名重建，无 AST 所有权共享）
class RangeForStmt : public Stmt {
public:
    RangeForStmt() : Stmt(NodeType::RangeForStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitRangeForStmt(this); }

    std::string varName;               // 循环变量名（用户书写）
    std::unique_ptr<Expr> iterable;    // 迭代对象表达式
    std::unique_ptr<Stmt> body;        // 循环体（块或单语句）
    std::unique_ptr<Stmt> desugared;   // 语义层降级树（循环 语句，IR 层生成用）
};

// 情况标签：情况 常量值: 语句*（选择语句的一个分支）
// 每个 case 标签附带其后的语句序列（到下一个标签或右花括号为止）
class CaseLabel : public Stmt {
public:
    explicit CaseLabel(std::int64_t caseValue)
        : Stmt(NodeType::CaseLabel), value(caseValue) {}
    void accept(AstVisitor& visitor) override { visitor.visitCaseLabel(this); }

    std::int64_t value;                          // 情况常量值（编译期整型常量）
    std::string rawValue;                        // 情况常量原始文本（调试输出）
    std::vector<std::unique_ptr<Stmt>> statements;  // 该分支语句体（可为空）
    // ---- C-4（2026-08）模式匹配增强 ----
    bool isString = false;                       // 字符串情况值（rawValue 含引号字面量）
    std::string strValue;                        // 解码后字符串（语义层回填，IR 比较用）
    bool isEnumMember = false;                   // 裸枚举成员名（选择 条件为枚举类型时解析）
};

// 默认标签：默认: 语句*（选择语句的兜底分支，最多一个）
class DefaultLabel : public Stmt {
public:
    DefaultLabel() : Stmt(NodeType::DefaultLabel) {}
    void accept(AstVisitor& visitor) override { visitor.visitDefaultLabel(this); }

    std::vector<std::unique_ptr<Stmt>> statements;  // 该分支语句体（可为空）
};

// 选择语句：选择(值) { 情况 常量: 语句* 默认: 语句* }
// 值类型：整数/字符（阶段二）；枚举（后续Task支持）
class SwitchStmt : public Stmt {
public:
    SwitchStmt() : Stmt(NodeType::SwitchStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitSwitchStmt(this); }

    std::unique_ptr<Expr> condition;                  // 选择表达式（整型/字符）
    std::vector<std::unique_ptr<CaseLabel>> cases;    // 情况分支列表
    std::unique_ptr<DefaultLabel> defaultCase;        // 默认分支（可为空）
};
} // namespace cn_compiler
