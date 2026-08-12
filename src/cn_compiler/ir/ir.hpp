// CN-IR中间表示定义 + IR生成器（Task 1.6）
// 设计要点：
//   1. 指令为三地址码形式：opcode + 操作数 + 结果虚拟寄存器 + 类型标注
//   2. 基本块为CFG节点，末尾可挂终止信息（跳转/条件跳转/返回）
//   3. 虚拟寄存器采用 %v0/%v1 编号；基本块采用 块0/块1 标签
//   4. 字符串常量统一收集到模块级常量池（常量ID形如 @str0）
//   5. IRGenerator 继承 AstVisitor，将AST降级为三地址码IR
//   6. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {
namespace ir {

// IR指令操作码（规格书7.3指令分类，阶段一子集）
enum class Opcode {
    // ---- 常量加载 ----
    ConstInt,       // 加载整数常量（extra=十进制值）
    ConstFloat,     // 加载浮点常量（extra=文本值）
    ConstString,    // 加载字符串常量（extra=常量池ID，如 @str0）
    ConstBool,      // 加载布尔常量（extra=真/假）

    // ---- 算术运算 ----
    Add,            // 整数加法
    Sub,            // 整数减法
    Mul,            // 整数乘法
    Div,            // 整数除法
    Mod,            // 整数取余

    // ---- 比较运算（结果为布尔） ----
    Eq,             // ==
    Ne,             // !=
    Lt,             // <
    Le,             // <=
    Gt,             // >
    Ge,             // >=

    // ---- 逻辑运算 ----
    And,            // &&（短路，阶段一简化非短路）
    Or,             // ||（短路，阶段一简化非短路）
    Not,            // !

    // ---- 内存操作 ----
    Load,           // 从变量加载（operand[0]=变量名）
    Store,          // 存储到变量（operand[0]=值寄存器, extra=变量名）
    Alloca,         // 栈上分配（extra=变量名）

    // ---- 控制流 ----
    Jump,           // 无条件跳转（extra=目标块标签）
    Branch,         // 条件跳转（operand[0]=条件寄存器, extra=真块|假块）
    Call,           // 函数调用（extra=函数名）
    Return,         // 返回（operand[0]=返回值寄存器，可为空）

    // ---- 其他 ----
    Phi,            // Phi节点（SSA汇合点，阶段一预留）
};

// 操作码转字符串（调试/测试输出，中文描述）
const char* opcodeToString(Opcode opcode);

// IR值（虚拟寄存器/常量/变量引用）
struct IRValue {
    int id = -1;           // 虚拟寄存器编号（-1表示非寄存器）
    std::string type;      // 类型（IR类型：i32/u64/f64/i1/ptr/void）
    std::string extra;     // 附加信息（变量名/常量文本/函数名）
    bool isConstant = false;  // 是否常量（寄存器外形式）

    // 便捷构造：虚拟寄存器引用
    static IRValue reg(int id, const std::string& type) {
        IRValue v;
        v.id = id;
        v.type = type;
        return v;
    }
    // 便捷构造：常量（文本）
    static IRValue constant(const std::string& text, const std::string& type) {
        IRValue v;
        v.type = type;
        v.extra = text;
        v.isConstant = true;
        return v;
    }
    // 便捷构造：变量引用（按名）
    static IRValue var(const std::string& name, const std::string& type) {
        IRValue v;
        v.type = type;
        v.extra = name;
        return v;
    }

    // 渲染为字符串（%v0 / 常量文本 / 变量名）
    std::string toString() const {
        if (isConstant) return extra;
        if (id >= 0) return "%v" + std::to_string(id);
        return extra;
    }
};

// IR指令：三地址码形式
struct IRInstruction {
    Opcode opcode;                    // 操作码
    IRValue result;                   // 结果寄存器（无结果的指令保留默认）
    std::vector<IRValue> operands;    // 操作数
    std::string extra;                // 附加信息（常量值/跳转目标/函数名/变量名）
    SourceLocation loc;               // 源码位置
    std::string type;                 // 指令类型（结果类型，无结果为空）
};

// IR基本块：CFG节点（标签 + 指令序列 + 终止信息）
struct IRBlock {
    std::string label;                          // 块标签（块0/块1/...）
    std::vector<IRInstruction> instructions;    // 指令序列（不含终止指令）
    // ---- 终止信息（三者至多其一） ----
    bool terminated = false;                    // 是否已终止
    std::string termKind;                       // "跳转"/"条件跳转"/"返回"
    std::string termTarget;                     // 无条件跳转目标块标签
    std::string termTrueTarget;                 // 条件跳转真分支目标
    std::string termFalseTarget;                // 条件跳转假分支目标
    std::string termReturnValue;                // 返回寄存器名（空=无返回值）
};

// IR函数：签名 + 参数 + 基本块列表 + 变量映射
struct IRFunction {
    std::string name;                          // 函数名
    std::string returnType;                    // 返回类型（IR类型）
    // 参数列表 (源码名, IR类型)：对外接口保持源码名（可读性/测试契约）
    std::vector<std::pair<std::string, std::string>> params;
    // 参数唯一内部名（与 params 一一对应，供代码生成层分配独立栈槽，
    // 解决遮蔽参数/变量的槽冲突）
    std::vector<std::string> paramUniques;
    std::vector<std::unique_ptr<IRBlock>> blocks;             // 基本块列表
    int nextRegId = 0;                         // 下一个虚拟寄存器编号
    std::unordered_map<std::string, std::string> varTypes;   // 变量名 -> IR类型

    // MSVC兼容：含 unique_ptr 的类，隐式拷贝构造会触发 C2280（traits实例化）。
    // 显式声明移动语义（语义与默认一致），删除拷贝。
    IRFunction() = default;
    IRFunction(const IRFunction&) = delete;
    IRFunction& operator=(const IRFunction&) = delete;
    IRFunction(IRFunction&&) = default;
    IRFunction& operator=(IRFunction&&) = default;
};

// IR模块：函数列表 + 字符串常量池
struct IRModule {
    std::vector<IRFunction> functions;                  // 函数列表
    std::vector<std::string> stringConstants;           // 字符串常量池（@str0/@str1...）
    std::unordered_map<std::string, int> stringIndex;   // 文本 -> 常量池ID

    // MSVC兼容：与 IRFunction 相同原因，显式移动语义
    IRModule() = default;
    IRModule(const IRModule&) = delete;
    IRModule& operator=(const IRModule&) = delete;
    IRModule(IRModule&&) = default;
    IRModule& operator=(IRModule&&) = default;
};

} // namespace ir

// ==================== IR生成器 ====================

// IR生成器：将AST降级为三地址码IR
class IRGenerator : public AstVisitor {
public:
    // 构造函数：绑定诊断引擎引用
    explicit IRGenerator(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    // 主入口：生成IR模块
    ir::IRModule generate(Program* program);

    // ==================== AstVisitor 接口实现 ====================
    // 声明节点
    void visitProgram(Program* node) override;
    void visitFunctionDecl(FunctionDecl* node) override;
    void visitParamDecl(ParamDecl* node) override;
    void visitVarDecl(VarDecl* node) override;
    // 语句节点
    void visitBlockStmt(BlockStmt* node) override;
    void visitExprStmt(ExprStmt* node) override;
    void visitIfStmt(IfStmt* node) override;
    void visitWhileStmt(WhileStmt* node) override;
    void visitForStmt(ForStmt* node) override;
    void visitReturnStmt(ReturnStmt* node) override;
    void visitBreakStmt(BreakStmt* node) override;
    void visitContinueStmt(ContinueStmt* node) override;
    // 表达式节点
    void visitIntegerLiteral(IntegerLiteral* node) override;
    void visitFloatLiteral(FloatLiteral* node) override;
    void visitStringLiteral(StringLiteral* node) override;
    void visitCharLiteral(CharLiteral* node) override;
    void visitBoolLiteral(BoolLiteral* node) override;
    void visitIdentifierExpr(IdentifierExpr* node) override;
    void visitBinaryExpr(BinaryExpr* node) override;
    void visitUnaryExpr(UnaryExpr* node) override;
    void visitAssignmentExpr(AssignmentExpr* node) override;
    void visitCallExpr(CallExpr* node) override;
    void visitMemberExpr(MemberExpr* node) override;
    // 类型节点
    void visitType(Type* node) override;

private:
    // ==================== IR构建辅助 ====================
    ir::IRValue newReg();                       // 新建虚拟寄存器
    // 添加指令到当前块
    void emit(ir::Opcode opcode,
              const std::vector<ir::IRValue>& operands,
              const ir::IRValue& result,
              const std::string& extra,
              const std::string& type,
              const SourceLocation& loc);
    // 添加带结果寄存器的指令
    ir::IRValue emitResult(ir::Opcode opcode,
                           const std::vector<ir::IRValue>& operands,
                           const std::string& type,
                           const std::string& extra,
                           const SourceLocation& loc);
    // 生成零常量（一元负号/按位非辅助）
    ir::IRValue zeroConst(const std::string& type);
    // 终结当前块：无条件跳转
    void endJump(const std::string& target);
    // 终结当前块：条件跳转
    void endBranch(const std::string& condReg, const std::string& trueTarget,
                   const std::string& falseTarget);
    // 终结当前块：返回
    void endReturn(const std::string& valueReg);

    // ==================== 语句/表达式生成 ====================
    void genStmt(Stmt* node);                   // 语句分发
    void genBlock(BlockStmt* node);             // 代码块（顺序生成语句）
    ir::IRValue genExpr(Expr* node);            // 表达式生成，返回结果寄存器
    void genVarDecl(VarDecl* node);             // 变量声明（Alloca + Store）
    // 分配变量寄存器：Alloca并登记映射
    ir::IRValue allocVar(const std::string& name, const std::string& irType,
                         const SourceLocation& loc);
    // 查找变量寄存器（未找到返回id=-1）
    ir::IRValue lookupVar(const std::string& name);

    // ==================== 控制流 ====================
    void genIf(IfStmt* node);                   // 如果/否则如果/否则
    void genWhile(WhileStmt* node);             // 当循环
    void genFor(ForStmt* node);                 // 循环（for风格/无限）
    ir::IRBlock* newBlock(const std::string& label);  // 新建基本块并加入函数
    void setCurrentBlock(ir::IRBlock* block);   // 设置当前生成块

    // ==================== 类型与辅助 ====================
    // 源码类型 -> IR类型映射
    static std::string mapType(const std::string& type);
    // 运算符 -> IR操作码映射（返回false表示不支持）
    static bool mapBinaryOp(Operator op, bool isFloat, ir::Opcode& out);
    // 是否复合赋值运算符
    static bool isCompoundAssignOp(Operator op);
    // 复合赋值的基础运算符（+= -> +）
    static Operator baseOpOfCompound(Operator op);
    // 字符串字面量解码（剥离引号，阶段一简单解码）
    static std::string decodeString(const std::string& raw);

    // 查找变量的唯一内部名（未找到返回空串）
    std::string lookupVarName(const std::string& name) const;
    // 查找变量的IR类型（未找到返回空串）
    std::string lookupVarType(const std::string& name) const;

    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;                  // 诊断引擎
    ir::IRModule* module_ = nullptr;            // 当前模块
    ir::IRFunction* function_ = nullptr;        // 当前函数
    ir::IRBlock* currentBlock_ = nullptr;       // 当前生成块
    ir::IRValue lastExpr_;                      // 最近一次表达式生成的结果
    int regCounter_ = 0;                        // 虚拟寄存器编号（全局递增）
    int blockCounter_ = 0;                      // 基本块编号（全局递增）
    int varCounter_ = 0;                        // 变量唯一名计数器（函数级递增）
    // 变量作用域栈（BlockStmt 进入压栈/退出弹栈，支持同名遮蔽）。
    // 每个条目：源码名 -> {寄存器ID, 唯一内部名（name$N，遮蔽时分配独立槽）, IR类型}
    struct VarEntry {
        int regId = -1;
        std::string uniqueName;
        std::string type;
    };
    std::vector<std::unordered_map<std::string, VarEntry>> varStack_;
    // 循环控制流：中断/继续跳转目标栈
    struct LoopContext {
        std::string breakTarget;    // 中断跳转目标块标签
        std::string continueTarget; // 继续跳转目标块标签
    };
    std::vector<LoopContext> loopStack_;
};

} // namespace cn_compiler
