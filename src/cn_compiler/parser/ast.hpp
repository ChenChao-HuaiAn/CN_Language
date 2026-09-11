// AST节点定义：语法分析器的输出、语义分析器（Task 1.5）的输入（Task 1.3）
// 设计要点：
//   1. 基类 AstNode + 派生类体系，std::unique_ptr 管理所有权（无拷贝语义）
//   2. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
//   3. 访问者模式 AstVisitor：遍历AST的统一入口（可选基类，供打印/分析器复用）
//   4. 每个节点携带 SourceLocation 源码位置，用于错误报告
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// 前向声明：所有节点类（供访问者基类引用，避免循环依赖）
class Program;
class FunctionDecl;
class ParamDecl;
class VarDecl;
class StructDecl;
class EnumDecl;
class ClassDecl;
class ClassMember;
class InterfaceDecl;
class ImportDecl;
class GenericDecl;
class BlockStmt;
class IntegerLiteral;
class FloatLiteral;
class StringLiteral;
class CharLiteral;
class BoolLiteral;
class IdentifierExpr;
class SelfExpr;
class SuperExpr;
class BinaryExpr;
class UnaryExpr;
class AssignmentExpr;
class CallExpr;
class MemberExpr;
class IndexExpr;
class InitListExpr;
class StructInitExpr;
class NullLiteral;
class TernaryExpr;
class CastExpr;
class LambdaExpr;
class SizeofExpr;
class ExprStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class RangeForStmt;
class SwitchStmt;
class CaseLabel;
class DefaultLabel;
class Type;

// ==================== 枚举定义 ====================

// 访问控制枚举：类/接口成员的可见性标签（规格书06-二 标签式）
// 同时用于模块级顶层声明的可见性（规格书08-三 标签式，Task 3.6）：
//   公开: -> Public（跨模块可导入）；私有: -> Private（仅本模块可见）。
// 定义在文件顶部（先于 FunctionDecl/StructDecl 等声明节点，供其 access 字段使用）。
enum class AccessSpecifier {
    Public,         // 公开：
    Protected,      // 保护：/ 公开：模块级等价 Public（模块级无保护）
    Private         // 私有：
};

// 节点类型：标识每个AST节点的种类（语义分析器/打印器按类型分发）
enum class NodeType {
    // 声明节点
    Program,           // 程序（顶层函数声明集合）
    FunctionDecl,      // 函数声明
    ParamDecl,         // 参数声明
    VarDecl,           // 变量/常量声明
    StructDecl,        // 结构体/联合体声明（Task 2.7）
    EnumDecl,          // 枚举声明（Task 2.7）
    ClassDecl,         // 类声明（Task 3.1，阶段3 OOP）
    ClassMember,       // 类成员（字段/方法/构造/析构/运算符重载/友元，Task 3.1）
    InterfaceDecl,     // 接口声明（Task 3.3）
    ImportDecl,        // 导入声明（Task 3.6）
    GenericDecl,       // 泛型声明（包装被泛型修饰的类/函数，Task 3.8）
    // 语句节点
    BlockStmt,         // 代码块 { ... }
    ExprStmt,          // 表达式语句
    IfStmt,            // 如果/否则如果/否则
    WhileStmt,         // 当循环
    ForStmt,           // 循环（for风格 / 无限循环）
    ReturnStmt,        // 返回语句
    BreakStmt,         // 中断语句
    ContinueStmt,      // 继续语句
    RangeForStmt,      // 遍历...中每个 迭代语句（C-2，语义层降级为 循环）
    SwitchStmt,        // 选择语句（switch风格）
    CaseLabel,         // 情况标签（case分支头）
    DefaultLabel,      // 默认标签（default分支头）
    // 表达式节点
    IntegerLiteral,    // 整数字面量
    FloatLiteral,      // 浮点字面量
    StringLiteral,     // 字符串字面量
    CharLiteral,       // 字符字面量
    BoolLiteral,       // 布尔字面量
    NullLiteral,       // 空指针字面量（无，Task 2.4）
    IdentifierExpr,    // 标识符表达式
    SelfExpr,          // 自身（this 指针，Task 3.1）
    SuperExpr,         // 父类（限定调用 父类.方法()，Task 3.1）
    BinaryExpr,        // 二元运算
    UnaryExpr,         // 一元运算
    AssignmentExpr,    // 赋值表达式
    CallExpr,          // 函数调用
    MemberExpr,        // 成员访问（. / ->）
    IndexExpr,         // 下标访问（数组[i]，Task 2.4）
    InitListExpr,      // 初始化列表（{ 1, 2, 3 }，Task 2.4）
    StructInitExpr,    // 结构体初始化（点{ x = 1, y = 2 }，Task 2.7）
    TernaryExpr,       // 三元条件表达式（条件 ? 真值 : 假值，Task 2.9）
    CastExpr,          // 强制类型转换（类型名(表达式)，Task 2.10）
    LambdaExpr,        // lambda表达式（[捕获](参数) -> 返回 { 体 }，Task 2.10）
    SizeofExpr,        // 类型大小（类型大小(类型)，A-3 2026-08，C++ sizeof 等价物）
    // 类型节点
    TypeNode,          // 类型
};

// 运算符枚举：表达式中的运算种类（对应词法层39个运算符中的可解析部分）
enum class Operator {
    // 算术(5)
    Add,               // +
    Subtract,          // -（二元减法 / 一元负号）
    Multiply,          // *
    Divide,            // /
    Modulo,            // %
    // 比较(6)
    EqualEqual,        // ==
    BangEqual,         // !=
    Less,              // <
    Greater,           // >
    LessEqual,         // <=
    GreaterEqual,      // >=
    // 逻辑(3)
    AndAnd,            // &&
    OrOr,              // ||
    Bang,              // !
    // 位运算(6)
    Amp,               // &（二元按位与）
    Pipe,              // |（按位或）
    Caret,             // ^（按位异或）
    Tilde,             // ~（按位非）
    LessLess,          // <<（左移）
    GreaterGreater,    // >>（右移）
    // 指针一元运算(2)（规格书4.4：&取地址、*解引用，Task 2.3 一元上下文解析）
    AddressOf,         // &（取地址，一元）
    Deref,             // *（解引用，一元）
    // 赋值(11)
    Assign,            // =
    PlusAssign,        // +=
    MinusAssign,       // -=
    StarAssign,        // *=
    SlashAssign,       // /=
    PercentAssign,     // %=
    AmpAssign,         // &=
    PipeAssign,        // |=
    CaretAssign,       // ^=
    LessLessAssign,    // <<=
    GreaterGreaterAssign, // >>=
    // 自增自减(2)
    Increment,         // ++（前缀/后缀）
    Decrement,         // --（前缀/后缀）
    // 错误传播（C-1 2026-08，规格书07 Rust ? 运算符等价物）：后缀 ?——
    //   结果<T,E>/可选<T> 表达式的值提取：正常 -> 值；否则从当前函数返回错误
    Propagate,         // ?（后缀错误传播，postfix=true）
    // 成员访问(1)
    Dot,               // .（v2.1 统一：指针自动解引用一级，-> 已废除）
};

// ==================== 访问者基类 ====================

// AST访问者：遍历AST的统一入口（每个节点调用 accept(visitor) 分发）
// 派生类（如 AST 打印器、语义检查器）按需重写 visitXxx 方法
class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    // 声明节点
    virtual void visitProgram(Program* node) = 0;
    virtual void visitFunctionDecl(FunctionDecl* node) = 0;
    virtual void visitParamDecl(ParamDecl* node) = 0;
    virtual void visitVarDecl(VarDecl* node) = 0;
    virtual void visitStructDecl(StructDecl* node) = 0;
    virtual void visitEnumDecl(EnumDecl* node) = 0;
    // 阶段3 OOP/错误处理/模块/泛型 声明节点（默认空实现，语义/IR 层按需重写）
    virtual void visitClassDecl(ClassDecl* node);          // 类声明（Task 3.1）
    virtual void visitClassMember(ClassMember* node);      // 类成员（Task 3.1）
    virtual void visitInterfaceDecl(InterfaceDecl* node);  // 接口声明（Task 3.3）
    virtual void visitImportDecl(ImportDecl* node);        // 导入声明（Task 3.6）
    virtual void visitGenericDecl(GenericDecl* node);      // 泛型声明（Task 3.8）
    // 语句节点
    virtual void visitBlockStmt(BlockStmt* node) = 0;
    virtual void visitExprStmt(ExprStmt* node) = 0;
    virtual void visitIfStmt(IfStmt* node) = 0;
    virtual void visitWhileStmt(WhileStmt* node) = 0;
    virtual void visitForStmt(ForStmt* node) = 0;
    virtual void visitReturnStmt(ReturnStmt* node) = 0;
    virtual void visitBreakStmt(BreakStmt* node) = 0;
    virtual void visitContinueStmt(ContinueStmt* node) = 0;
    // C-2（2026-08）：遍历...中每个 迭代语句——默认空实现（语义层降级为 循环）
    //   （与阶段3 声明节点同策略：按需重写，IR 层经 desugared 生成）
    virtual void visitRangeForStmt(RangeForStmt* node);
    virtual void visitSwitchStmt(SwitchStmt* node) = 0;
    virtual void visitCaseLabel(CaseLabel* node) = 0;
    virtual void visitDefaultLabel(DefaultLabel* node) = 0;
    // 表达式节点
    virtual void visitIntegerLiteral(IntegerLiteral* node) = 0;
    virtual void visitFloatLiteral(FloatLiteral* node) = 0;
    virtual void visitStringLiteral(StringLiteral* node) = 0;
    virtual void visitCharLiteral(CharLiteral* node) = 0;
    virtual void visitBoolLiteral(BoolLiteral* node) = 0;
    virtual void visitNullLiteral(NullLiteral* node) = 0;
    virtual void visitIdentifierExpr(IdentifierExpr* node) = 0;
    virtual void visitSelfExpr(SelfExpr* node);            // 自身（this，Task 3.1）
    virtual void visitSuperExpr(SuperExpr* node);          // 父类（Task 3.1）
    virtual void visitBinaryExpr(BinaryExpr* node) = 0;
    virtual void visitUnaryExpr(UnaryExpr* node) = 0;
    virtual void visitAssignmentExpr(AssignmentExpr* node) = 0;
    virtual void visitCallExpr(CallExpr* node) = 0;
    virtual void visitMemberExpr(MemberExpr* node) = 0;
    virtual void visitIndexExpr(IndexExpr* node) = 0;
    virtual void visitInitListExpr(InitListExpr* node) = 0;
    virtual void visitStructInitExpr(StructInitExpr* node) = 0;
    virtual void visitTernaryExpr(TernaryExpr* node) = 0;
    virtual void visitCastExpr(CastExpr* node) = 0;
    virtual void visitLambdaExpr(LambdaExpr* node) = 0;
    virtual void visitSizeofExpr(SizeofExpr* node);  // 类型大小（A-3，默认空实现）
    // 类型节点
    virtual void visitType(Type* node) = 0;
};

// ==================== 节点基类 ====================

// AST节点基类：所有节点的共同接口（类型查询 + 访问者分发 + 源码位置）
class AstNode {
public:
    explicit AstNode(NodeType type) : type_(type) {}
    virtual ~AstNode() = default;

    // 获取节点类型
    NodeType getType() const { return type_; }
    // 访问者分发入口（每个派生类实现，调用 visitor 对应方法）
    virtual void accept(AstVisitor& visitor) = 0;

    SourceLocation location;  // 节点源码位置（错误报告用）

private:
    NodeType type_;  // 节点类型
};

// 表达式基类：所有表达式节点的公共父类
class Expr : public AstNode {
public:
    using AstNode::AstNode;  // 继承构造函数
    ~Expr() override = default;
};

// 语句基类：所有语句节点的公共父类
class Stmt : public AstNode {
public:
    using AstNode::AstNode;
    ~Stmt() override = default;
};

// ==================== 表达式节点 ====================

// 整数字面量：42 / 0xFF / 0b1010 / 0o777（含后缀）
class IntegerLiteral : public Expr {
public:
    // 构造：解析后的整数值 + 原始文本
    IntegerLiteral(std::int64_t value, std::string raw)
        : Expr(NodeType::IntegerLiteral), value(value), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitIntegerLiteral(this); }

    std::int64_t value;  // 解析后的整数值
    std::string raw;     // 原始字面量文本
};

// 浮点字面量：3.14 / 1.5e10
class FloatLiteral : public Expr {
public:
    FloatLiteral(double value, std::string raw)
        : Expr(NodeType::FloatLiteral), value(value), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitFloatLiteral(this); }

    double value;        // 解析后的浮点值
    std::string raw;     // 原始字面量文本
};

// 字符串字面量：保留完整原始文本（含引号与前缀），语义阶段统一解码
class StringLiteral : public Expr {
public:
    explicit StringLiteral(std::string raw)
        : Expr(NodeType::StringLiteral), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitStringLiteral(this); }

    std::string raw;  // 原始字面量文本（含引号）
};

// 字符字面量：'A' / '中' / '\u{4E2D}'
class CharLiteral : public Expr {
public:
    explicit CharLiteral(std::string raw)
        : Expr(NodeType::CharLiteral), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitCharLiteral(this); }

    std::string raw;  // 原始字面量文本（含单引号）
};

// 布尔字面量：真 / 假
class BoolLiteral : public Expr {
public:
    BoolLiteral(bool value, std::string raw)
        : Expr(NodeType::BoolLiteral), value(value), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitBoolLiteral(this); }

    bool value;        // 布尔值
    std::string raw;   // 原始字面量文本
};

// 空指针字面量：无（Task 2.4）
// 语义：作为指针字面量，值为0（等价C的NULL/空指针）；
//       可赋给任意指针类型（含 空类型*）；也可作为空指针与指针比较
class NullLiteral : public Expr {
public:
    explicit NullLiteral(SourceLocation loc) : Expr(NodeType::NullLiteral) {
        location = loc;
    }
    void accept(AstVisitor& visitor) override { visitor.visitNullLiteral(this); }
};

// 标识符表达式：变量名/函数名引用
class IdentifierExpr : public Expr {
public:
    explicit IdentifierExpr(std::string name)
        : Expr(NodeType::IdentifierExpr), name(std::move(name)) {}
    void accept(AstVisitor& visitor) override { visitor.visitIdentifierExpr(this); }

    std::string name;  // 标识符名
};

// 自身表达式：自身（对应 C++ 的 this 指针，Task 3.1，规格书06-七）
// 语义：指向当前类实例的指针；通常经 自身.成员 访问实例成员。
// 语法层仅记录关键字位置，语义层按类方法上下文解析为 this 指针。
class SelfExpr : public Expr {
public:
    explicit SelfExpr(SourceLocation loc) : Expr(NodeType::SelfExpr) {
        location = loc;
    }
    void accept(AstVisitor& visitor) override { visitor.visitSelfExpr(this); }
};

// 父类表达式：父类（对应 C++ 的 父类:: 限定调用，Task 3.1，规格书06-七）
// 语法形式：父类.方法名(实参)（限定调用父类实现）；本节点为 父类 关键字本身，
//           后续由 parseCallOrMember 组装为 MemberExpr/CallExpr（object=SuperExpr）。
class SuperExpr : public Expr {
public:
    explicit SuperExpr(SourceLocation loc) : Expr(NodeType::SuperExpr) {
        location = loc;
    }
    void accept(AstVisitor& visitor) override { visitor.visitSuperExpr(this); }
};

// 二元运算：左操作数 运算符 右操作数（Pratt解析的中间/最终结果）
class BinaryExpr : public Expr {
public:
    BinaryExpr(Operator op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : Expr(NodeType::BinaryExpr), op(op), left(std::move(left)), right(std::move(right)) {}
    void accept(AstVisitor& visitor) override { visitor.visitBinaryExpr(this); }

    Operator op;                  // 运算符
    std::unique_ptr<Expr> left;   // 左操作数
    std::unique_ptr<Expr> right;  // 右操作数
    // 运算符重载结果源码类型（Task 3.7，缺陷2 修复）：
    //   语义层在 visitBinaryExpr 命中成员 运算符X 时写回重载方法返回类型；
    //   IR 层对"链式运算符重载"（甲+乙+丙）的内层 BinaryExpr 经 exprSrcType
    //   读取此字段得到类类型，避免落入 ptr+ptr 字符串连接分支（字段错乱）。
    std::string resolvedType;
};

// 一元运算：运算符 操作数（含前缀/后缀自增自减）
class UnaryExpr : public Expr {
public:
    // 构造：运算符 + 操作数 + 是否为后缀（i++ 为 true，++i 为 false）
    UnaryExpr(Operator op, std::unique_ptr<Expr> operand, bool postfix = false)
        : Expr(NodeType::UnaryExpr), op(op), operand(std::move(operand)), postfix(postfix) {}
    void accept(AstVisitor& visitor) override { visitor.visitUnaryExpr(this); }

    Operator op;                   // 运算符
    std::unique_ptr<Expr> operand; // 操作数
    bool postfix;                  // 是否为后缀形式（++ -- 后缀为true）
    // C-1（错误传播运算符，2026-08）：op==Propagate 时记录操作数类型
    //   （结果<T,E> / 可选<T>），语义层回填、IR 层按此降级（正常取 .值 /
    //   失败构造错误结果并返回）
    std::string propagateType;
};

// 赋值表达式：目标 赋值运算符 值（右结合）
class AssignmentExpr : public Expr {
public:
    AssignmentExpr(std::unique_ptr<Expr> target, Operator op, std::unique_ptr<Expr> value)
        : Expr(NodeType::AssignmentExpr), target(std::move(target)),
          op(op), value(std::move(value)) {}
    void accept(AstVisitor& visitor) override { visitor.visitAssignmentExpr(this); }

    std::unique_ptr<Expr> target;  // 赋值目标（左值表达式）
    Operator op;                   // 赋值运算符
    std::unique_ptr<Expr> value;   // 赋值值
};

// 调用表达式：被调者(实参列表)
class CallExpr : public Expr {
public:
    // 便捷构造：按函数名创建（内部构造标识符被调者）
    explicit CallExpr(std::string calleeName)
        : Expr(NodeType::CallExpr),
          callee(std::make_unique<IdentifierExpr>(std::move(calleeName))) {}
    // 通用构造：任意被调者表达式（支持成员函数调用等）
    explicit CallExpr(std::unique_ptr<Expr> callee)
        : Expr(NodeType::CallExpr), callee(std::move(callee)) {}
    void accept(AstVisitor& visitor) override { visitor.visitCallExpr(this); }

    std::unique_ptr<Expr> callee;                 // 被调者（通常是标识符）
    std::vector<std::unique_ptr<Expr>> arguments; // 实参列表
    std::string resolvedSignature;                // 重载决议后的签名 key（Task 2.10，
                                                  //   语义层写回；IR 层按此生成 mangled 符号）
    // P3-18 补完（2026-08）：调用是否为"引用返回"调用（解析结果返回类型含 &）。
    // 语义层决议时写回；IR 层据此把调用结果当"左值地址"处理（整32& r = 获取() /
    // 获取() = 值 / &获取()）。函数指针间接调用不适用（无法静态知返回类型）。
    bool isRefReturnCall = false;
    // plans/019 阶段4' A2（2026-09-11 方案甲 用户批准）：调用返回=拥有字符串
    //   标记——按被调者返回类型决议写回（返回 字符串=拥有，调用方登记 RAII；
    //   返回 字符*=借用不登记）。普通函数/类方法/接口方法/内置全路径统一置位；
    //   IR 层初始化位/赋值位消费（白名单之外的登记依据——签名即契约，
    //   Rust fn f() -> String / &str 两分法同款）。
    bool retOwnedString = false;
    std::string resolvedType;                     // 内置构造器推导的 结果<T,E>/可选<T> 类型
                                                  //   （Task 3.5，语义层写回；IR 层按此
                                                  //    降级为合成结构体构造）
    // ---- 第 4 层（crate 隔离）：限定调用模块过滤 ----
    // 模块::函数(实参) 限定调用重写时记录所属模块（数学::双倍 -> 数学），
    // 重载决议 resolveOverload 按此过滤（跨模块同名函数不歧义）。
    std::string moduleFilter;
};

// 成员访问：对象.成员（v2.1 成员访问统一 .：对象为指针时自动解引用一级，≡ (*对象).成员）
class MemberExpr : public Expr {
public:
    MemberExpr(std::unique_ptr<Expr> object, std::string memberName,
               bool derefAccess = false)
        : Expr(NodeType::MemberExpr), object(std::move(object)),
          memberName(std::move(memberName)), isDerefAccess(derefAccess) {}
    void accept(AstVisitor& visitor) override { visitor.visitMemberExpr(this); }

    std::unique_ptr<Expr> object;  // 对象表达式
    std::string memberName;        // 成员名
    // v2.1（2026-09-03，用户裁决废除 ->）：经指针访问标记——语义层
    // visitMemberExpr 按对象类型写回（对象为指针 = true，≡ (*对象).成员，
    // IR 层据此选基址：指针值 / 对象地址）；解析层恒 false（语法上只有 .）。
    bool isDerefAccess;
    // P3-23 补完（2026-08）：实例方法作值标记（对象.实例方法 非调用上下文）。
    // 语义层 visitMemberExpr 写回；IR 合成"绑定 this"闭包并在变量绑定态登记。
    bool isMethodValue = false;
};

// 下标访问：对象[index]（Task 2.4）
// 对象为数组（数组名/数组元素地址）或指针；index 为整型表达式。
// 语义层展开为：基址 + index * 元素大小 的内存访问（越界检查错误码2）
class IndexExpr : public Expr {
public:
    IndexExpr(std::unique_ptr<Expr> object, std::unique_ptr<Expr> index)
        : Expr(NodeType::IndexExpr), object(std::move(object)), index(std::move(index)) {}
    void accept(AstVisitor& visitor) override { visitor.visitIndexExpr(this); }

    std::unique_ptr<Expr> object;  // 被下标对象（数组/指针表达式）
    std::unique_ptr<Expr> index;   // 下标表达式（整型）
};

// 初始化列表：{ 表达式, 表达式, ... }（Task 2.4）
// 用于数组声明初始化（整32[5] 数据 = { 1, 2, 3 }），部分初始化剩余元素补零（C语义）。
// 结构体/联合体初始化列表在后续Task（2.7/2.8）复用本节点。
class InitListExpr : public Expr {
public:
    InitListExpr() : Expr(NodeType::InitListExpr) {}
    void accept(AstVisitor& visitor) override { visitor.visitInitListExpr(this); }

    std::vector<std::unique_ptr<Expr>> elements;  // 初始化元素列表
};

// 结构体初始化：类型名{ 字段 = 值, ... }（规格书05，Task 2.7）
// 例：点 p = 点{ x = 1, y = 2 }；字段按名赋值（顺序任意）
class StructInitExpr : public Expr {
public:
    explicit StructInitExpr(std::string typeName)
        : Expr(NodeType::StructInitExpr), typeName(std::move(typeName)) {}
    void accept(AstVisitor& visitor) override { visitor.visitStructInitExpr(this); }

    std::string typeName;                        // 结构体/联合体类型名
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> fields;  // 字段名 -> 值
};

// 三元条件表达式：条件 ? 真值 : 假值（规格书4.5 优先级1.5，右结合，Task 2.9）
// 语义：条件为布尔；结果为真值/假值的公共类型（数值宽化合并；字符串/指针/结构体须一致）；
//       IR 层用条件跳转 CFG 实现惰性求值（不选中分支不执行）
class TernaryExpr : public Expr {
public:
    TernaryExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> trueValue,
                std::unique_ptr<Expr> falseValue)
        : Expr(NodeType::TernaryExpr), condition(std::move(condition)),
          trueValue(std::move(trueValue)), falseValue(std::move(falseValue)) {}
    void accept(AstVisitor& visitor) override { visitor.visitTernaryExpr(this); }

    std::unique_ptr<Expr> condition;   // 条件表达式（须为布尔）
    std::unique_ptr<Expr> trueValue;   // 条件为真时的值
    std::unique_ptr<Expr> falseValue;  // 条件为假时的值
};

// 强制类型转换：类型名(表达式)（规格书04-一E，Task 2.10）
// 语义同 static_cast/位重解释组合：宽化/窄化/浮整/指针↔整数 均显式触发。
// 解析判据：'(' 前 token 为类型关键字/已声明类型名 -> CastExpr；否则 -> CallExpr。
class CastExpr : public Expr {
public:
    CastExpr(std::string targetType, std::unique_ptr<Expr> operand)
        : Expr(NodeType::CastExpr), targetType(std::move(targetType)),
          operand(std::move(operand)) {}
    void accept(AstVisitor& visitor) override { visitor.visitCastExpr(this); }

    std::string targetType;        // 目标类型名（整32/浮64/指针/自定义类型等）
    std::unique_ptr<Expr> operand; // 被转换表达式
};

// 捕获规格：[] 不捕获 / [=] 值捕获 / [&] 引用捕获 / [变量] 显式捕获（Task 2.10）
enum class LambdaCaptureKind { None, ByValue, ByRef, Explicit };

// lambda 表达式：[] 或 [捕获](参数) [-> 返回类型] { 函数体 }（规格书04-一D，Task 2.10）
// 语义：降级为匿名函数 + 闭包捕获环境；无捕获可赋给函数指针；有捕获存入闭包结构体。
class LambdaExpr : public Expr {
public:
    LambdaExpr() : Expr(NodeType::LambdaExpr) {}
    void accept(AstVisitor& visitor) override { visitor.visitLambdaExpr(this); }

    LambdaCaptureKind captureKind = LambdaCaptureKind::None;  // 捕获方式
    std::vector<std::string> explicitCaptures; // 显式捕获变量名列表（[x, y]）
    std::vector<std::unique_ptr<ParamDecl>> params; // 参数列表
    std::string returnType;                    // 返回类型（为空表示推导）
    std::unique_ptr<BlockStmt> body;           // 函数体
};

// 类型大小表达式：类型大小(类型)（A-3 2026-08，C++ sizeof 等价物）
// 参数是类型名（可为 泛型 T / 限定类型 甲::记录 / 复合类型 整32[5]），
// 编译期求值为字节大小（语义层计算回填 size，IR 层生成常量）。
class SizeofExpr : public Expr {
public:
    explicit SizeofExpr(std::string typeName)
        : Expr(NodeType::SizeofExpr), typeName(std::move(typeName)) {}
    void accept(AstVisitor& visitor) override { visitor.visitSizeofExpr(this); }

    std::string typeName;  // 目标类型名（解析后）
    int size = 0;          // 字节大小（语义层回填）
};

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

// 变量声明：变量/常量/静态 类型前置或冒号后置（CN规范类型前置，兼容冒号后置）
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

// ==================== 声明节点 ====================

// 类型节点：类型名（阶段一仅基本类型名，阶段二扩展复合类型）
class Type : public AstNode {
public:
    explicit Type(std::string name) : AstNode(NodeType::TypeNode), name(std::move(name)) {}
    void accept(AstVisitor& visitor) override { visitor.visitType(this); }

    std::string name;  // 类型名（如 整32 / 字符串 / 自定义类型名）
};

// 参数声明：类型 参数名 [= 默认值]（CN规范类型前置；兼容 参数名: 类型 冒号后置）
// 默认参数（规格书04-一C，Task 2.10）：从右向左连续声明；默认值须编译期常量。
//   defaultExpr 为表达式节点（整/浮/字符串/布尔/字符字面量、一元负号常量）；
//   hasDefault 标记是否给出默认值（缺省实参补全用）。
class ParamDecl : public AstNode {
public:
    ParamDecl() : AstNode(NodeType::ParamDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitParamDecl(this); }

    std::string name;      // 参数名
    std::string typeName;  // 参数类型
    FuncPtrTypeInfo funcPtr;  // 函数指针类型信息（非空表示本参数为函数指针）
    bool hasDefault = false;  // 是否有默认值（Task 2.10）
    std::unique_ptr<Expr> defaultExpr;  // 默认值表达式（编译期常量，Task 2.10）
    // plans/019 阶段3（2026-09-10）：常量 只读引用参数（常量 T& 名）——只读
    //   借用（Rust &T 对标）；体内赋值/传可变引用/与可变借用互斥均拒绝。
    bool isConstParam = false;
};

// 函数声明：函数 名称(参数列表) [-> 返回类型] { 函数体 }
class FunctionDecl : public AstNode {
public:
    FunctionDecl() : AstNode(NodeType::FunctionDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitFunctionDecl(this); }

    std::string name;                                  // 函数名
    std::vector<std::unique_ptr<ParamDecl>> params;    // 参数列表
    std::string returnType;                            // 返回类型（为空表示无返回值）
    std::unique_ptr<BlockStmt> body;                   // 函数体（为空表示函数原型声明）
    std::string sigKey;                                // 重载签名 key（Task 2.10，
                                                       //   语义层注册时写回；IR/codegen 按此 mangling）
    // ---- 模块级可见性（Task 3.6，规格书08-四 标签式） ----
    // 顶层声明的模块可见性：公开: 后声明 -> Public（跨模块可导入）；
    //   私有: 后声明 -> Private（仅本模块可见）。默认 Private（v2.0 变更）。
    // 与类内成员访问标签（ClassMember::access）同语法不同作用域：
    //   顶层标签由 parser.cpp 顶层循环维护；类内标签由 parseClassDecl 维护。
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4） ----
    // 所属模块（crate 域）名：mergeModules 合并阶段写入（入口模块=主）。
    // 用途：① 函数重复定义按模块分桶（跨模块同名允许——crate 隔离）；
    //       ② IR/codegen 链接符号加 模块名$ 前缀（防跨包链接冲突）。
    std::string moduleName;
    // ---- C-3（2026-08）FFI 最小集：外部 函数 声明 ----
    // 外部 函数 名(参数) -> 类型：声明 C 链接外部函数（无函数体）。
    // 语义/IR 差异：链接符号 = 纯名（不做重载 mangling、不加模块前缀），
    //   调用侧经 externFuncs 映射到纯名；codegen 自动 EXTERN（链接期解析）。
    bool isExtern = false;
    // plans/019 阶段4（2026-09-10）：不安全 函数 修饰——安全区边界（指针算术/
    //   指针下标写/联合体访问/外部函数调用/裸释放 仅限不安全函数体内；观察期
    //   =警告，分批收口后=错误）。类方法修饰位随收口轮。
    bool isUnsafe = false;
};

// 导入项：花括号导入中的单个符号（可选 作为 别名，v2.0）
struct ImportItem {
    std::string name;      // 符号名
    std::string alias;     // 作为 别名（为空表示无别名）
};

// 导入声明（v2.0，规格书08-三 导入语法全形式，Task 3.6）
//   导入 路径 [作为 标识符]              -> 路径导入 / 重命名导入
//   导入 路径 :: { 项1 [作为 别名], ... } -> 花括号导入（替代 v1.0 从...导入）
//   导入 路径 :: *                        -> 通配符导入
//   模块 标识符                           -> 模块声明（isModuleDecl=true，引用 .cn 文件模块）
// 结构化字段：
//   segments     : 路径段（A::B::C -> {A, B, C}；模块 网络 -> {网络}）
//   alias        : 作为 别名（可选，路径导入/模块声明重命名）
//   names        : 花括号导入项（{a, b}；ImportItem.name/.alias）
//   wildcard     : ::* 通配符
//   isModuleDecl : 模块 声明（模块 X 引用文件模块）
// 兼容字段（供 module.cpp/semantic.cpp 最小适配，第 4 层语义改造后移除）：
//   importPath   : 路径文本（:: 分隔，如 数学::平方根；模块声明 = 模块名）
//   fromImport   : 旧 从...导入 标记（v2.0 恒为 false，保留字段避免引用点编译失败）
class ImportDecl : public AstNode {
public:
    ImportDecl() : AstNode(NodeType::ImportDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitImportDecl(this); }

    // ---- v2.0 结构化字段 ----
    std::vector<std::string> segments;          // 路径段（A::B::C -> {A,B,C}）
    std::string alias;                          // 作为 别名（可选）
    std::vector<ImportItem> names;              // 花括号导入项（{a, b}）
    bool wildcard = false;                      // ::* 通配符
    bool isModuleDecl = false;                  // 模块 声明（模块 X）

    // ---- 兼容字段（最小适配；第 4 层语义改造后移除） ----
    std::string importPath;                     // 路径文本（:: 分隔；兼容旧字段名）
    bool fromImport = false;                    // 旧 从...导入 标记（v2.0 恒 false）

    // ---- 模块级可见性（第 5 层，规格书09-三 包.cn 再导出） ----
    // 公开 导入 路径（无冒号形式，包.cn 公共 API 再导出）：
    //   公开 导入 网络::连接 -> access = Public（被导入符号再导出为包级 API）
    //   普通 导入            -> access = Private（仅本 crate 内部使用）
    // parser 顶层循环识别 `公开` 前缀后调用 parseImportDecl 并设置本字段；
    // 语义层 visitImportDecl 依此把公开导入的符号注册到包级导出表。
    AccessSpecifier access = AccessSpecifier::Private;

    // ---- 来源模块归属（plans/018 P6b 工作流2，E0255 冲突检查用）----
    // 声明此导入的文件所属模块名（mergeModules 合并时写入 unit.moduleName）。
    //   合并把全部文件的 ImportDecl 收进单一 Program::imports，丢失「谁导入的」
    //   归属；「导入与本地定义同名」判定须按归属文件的本模块定义比对
    //   （声明 moduleName == ownerModule 的项 = 该文件本地定义）。
    std::string ownerModule;
};

// 类成员：类体内的字段/方法/构造/析构/运算符重载/友元（Task 3.1，规格书06）
// kind 取值（英文枚举，见 ClassMemberKind）：
//   Field       : 字段声明（类型 名称 [= 初始值]）
//   Method      : 方法声明（[虚拟|重写|抽象|常量] 函数 名(...) -> 类型 { 体 }）
//   Constructor : 构造函数（函数 类名(...) { 体 }）
//   Destructor  : 析构函数（函数 ~类名() { 体 }）
//   Operator    : 运算符重载成员（函数 运算符X(右操作数) -> 类型 { 体 }）
//   Friend      : 友元声明（友元 函数 名(...) 或 友元 类 名）
enum class ClassMemberKind {
    Field,          // 字段
    Method,         // 方法
    Constructor,    // 构造函数
    Destructor,     // 析构函数
    Operator,       // 运算符重载（函数 运算符X）
    Friend          // 友元声明
};

// 类成员节点：字段/方法/构造/析构/运算符重载/友元（Task 3.1）
class ClassMember : public AstNode {
public:
    ClassMember() : AstNode(NodeType::ClassMember) {}
    void accept(AstVisitor& visitor) override { visitor.visitClassMember(this); }

    ClassMemberKind kind = ClassMemberKind::Field;  // 成员种类
    // 可见性（当前访问标签段；v2.0 默认私有——parseClassDecl 未出现标签时
    //   按 Private 处理，公共成员须显式 公开:）
    AccessSpecifier access = AccessSpecifier::Private;

    // ---- 字段（kind=Field） ----
    std::string name;                              // 字段名/方法名/友元目标名
    std::string typeName;                          // 字段类型（含 结果<T,E> 等模板类型）
    std::unique_ptr<Expr> initializer;             // 字段初始值（可为空）

    // ---- 方法/构造/析构/运算符重载（kind=Method/Constructor/Destructor/Operator） ----
    bool isVirtual = false;                        // 虚拟 修饰（虚函数）
    bool isOverride = false;                       // 重写 修饰
    bool isAbstract = false;                       // 抽象 修饰（纯虚函数，无实现体）
    bool isConstMethod = false;                    // 常量 修饰（常量成员函数，Task 3.9）
    bool isStatic = false;                         // 静态 修饰（静态方法/字段，Task 3.9）
    bool isUnsafe = false;                         // 不安全 修饰（安全区边界，plans/019 阶段4 第二层第一批）
    std::vector<std::unique_ptr<ParamDecl>> params;// 参数列表
    std::string returnType;                        // 返回类型（方法/运算符重载）
    std::string operatorSym;                       // 运算符符号（kind=Operator，如 "+"）
    std::unique_ptr<BlockStmt> body;               // 方法体（抽象/接口签名为空）
    // P3-20：父类构造初始化列表（函数 子(...) : 父(实参)）——仅构造函数有意义
    std::string ctorInitBase;                        // 初始化列表中的父类名（可空）
    std::vector<std::unique_ptr<Expr>> ctorInitArgs; // 父类构造实参

    // ---- 友元（kind=Friend） ----
    bool isFriendClass = false;                    // 友元 类 名（true）或 友元 函数（false）
};

// 类声明：类 名 [: 父类名|接口名] { 访问标签段* }（Task 3.1，规格书06-一）
//   baseName    : 父类名（单继承；可为空）
//   interfaces  : 实现的接口名列表（可多个，规格书06-六 接口实现）
//   members     : 成员列表（按声明顺序；语义层按访问标签段分组）
class ClassDecl : public AstNode {
public:
    ClassDecl() : AstNode(NodeType::ClassDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitClassDecl(this); }

    std::string name;                              // 类名
    std::string baseName;                          // 父类名（可为空 = 无继承）
    std::vector<std::string> interfaces;           // 实现的接口名列表
    std::vector<std::unique_ptr<ClassMember>> members; // 类成员列表
    // 模块级可见性（Task 3.6，v2.0）：公开: -> Public / 私有: -> Private（默认 Private）
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 接口声明：接口 名 { 虚拟函数签名列表 }（Task 3.3，规格书06-六）
// 接口只含虚函数签名（无实现体）；实现类通过 重写 提供实现。
class InterfaceDecl : public AstNode {
public:
    InterfaceDecl() : AstNode(NodeType::InterfaceDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitInterfaceDecl(this); }

    std::string name;                              // 接口名
    std::vector<std::unique_ptr<ClassMember>> members; // 方法签名列表（kind=Method，body 为空）
    // 模块级可见性（Task 3.6，v2.0）：公开: -> Public / 私有: -> Private（默认 Private）
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 泛型声明：泛型 <类型 T> 类/函数（Task 3.8，规格书06-十三）
//   typeParams   : 类型参数名列表（如 [T, U]）
//   constraints  : 类型参数接口约束（constraints[T] = 接口名；空表示无约束）
//   innerClass   : 被泛型修饰的类声明（泛型类）
//   innerFunc    : 被泛型修饰的函数声明（泛型函数；与 innerClass 二选一）
class GenericDecl : public AstNode {
public:
    GenericDecl() : AstNode(NodeType::GenericDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitGenericDecl(this); }

    std::vector<std::string> typeParams;           // 类型参数名列表
    std::vector<std::string> constraints;          // 接口约束（与 typeParams 一一对应，空串=无约束）
    std::unique_ptr<ClassDecl> innerClass;         // 泛型类（可为空）
    std::unique_ptr<FunctionDecl> innerFunc;       // 泛型函数（可为空）
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 程序：顶层声明集合（函数/结构体/枚举/联合体/类/接口/导入/泛型）
class Program : public AstNode {
public:
    Program() : AstNode(NodeType::Program) {}
    void accept(AstVisitor& visitor) override { visitor.visitProgram(this); }

    std::vector<std::unique_ptr<FunctionDecl>> declarations;  // 顶层函数声明
    std::vector<std::unique_ptr<StructDecl>> structs;         // 结构体/联合体声明（Task 2.7）
    std::vector<std::unique_ptr<EnumDecl>> enums;             // 枚举声明（Task 2.7）
    std::vector<std::unique_ptr<ClassDecl>> classes;          // 类声明（Task 3.1）
    std::vector<std::unique_ptr<InterfaceDecl>> interfaces;   // 接口声明（Task 3.3）
    std::vector<std::unique_ptr<ImportDecl>> imports;         // 导入声明（Task 3.6）
    std::vector<std::unique_ptr<GenericDecl>> generics;       // 泛型声明（Task 3.8）
    // ---- 第 4 层（v2.0 决策8/9，P1-4/P3-8）：顶层常量/静态 ----
    // 顶层声明：常量 名 = 值（crate 级常量，编译期求值）与 静态 [类型] 名 [= 值]
    // （crate 级静态变量）。VarDecl 承载（isConst/isStatic），含模块级可见性。
    std::vector<std::unique_ptr<VarDecl>> globals;            // 顶层常量/静态变量声明
    // ---- plans/018 呈报一B（2026-09-07 用户终裁）：已加载模块名清单 ----
    // driver（runModulePipeline）注入：依赖图内全部模块名 + 货舱 [依赖] 包名。
    //   P1-1 废止后限定调用按「模块已加载」放行——crate/包名（如 工具库）不
    //   产生声明（其模块声明名为文件主干），须由此清单提供给语义层
    //   （knownModules_）。单文件管线（runPipeline）不填：无加载概念。
    std::vector<std::string> loadedModules;
};

} // namespace cn_compiler
