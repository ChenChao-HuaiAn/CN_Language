// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <string>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

namespace {

// 比较运算符（== != < > <= >=）
bool isComparisonOp(Operator op) {
    switch (op) {
        case Operator::EqualEqual: case Operator::BangEqual:
        case Operator::Less: case Operator::Greater:
        case Operator::LessEqual: case Operator::GreaterEqual:
            return true;
        default:
            return false;
    }
}

// 逻辑运算符（&& || !）
bool isLogicalOp(Operator op) {
    switch (op) {
        case Operator::AndAnd: case Operator::OrOr: case Operator::Bang:
            return true;
        default:
            return false;
    }
}

// 位运算符（& | ^ ~ << >>）
bool isBitwiseOp(Operator op) {
    switch (op) {
        case Operator::Amp: case Operator::Pipe: case Operator::Caret:
        case Operator::Tilde: case Operator::LessLess: case Operator::GreaterGreater:
            return true;
        default:
            return false;
    }
}

} // namespace

// ==================== 符号表管理 ====================

// 进入新作用域（压栈一个空作用域）
void SemanticAnalyzer::pushScope() {
    scopes_.emplace_back();
}

// 退出当前作用域（弹栈，作用域栈始终至少保留全局层）
void SemanticAnalyzer::popScope() {
    if (scopes_.size() > 1) scopes_.pop_back();
}

// 声明变量：同作用域重复声明返回false并报告错误
bool SemanticAnalyzer::declareVar(const std::string& name, const std::string& type,
                                  const SourceLocation& loc) {
    if (scopes_.empty()) pushScope();
    auto& current = scopes_.back();
    if (current.find(name) != current.end()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "重复声明变量 '" + name + "'");
        return false;
    }
    current[name] = type;
    return true;
}

// 查找变量类型：从内到外遍历作用域栈
bool SemanticAnalyzer::lookupVar(const std::string& name, std::string& type) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            type = found->second;
            return true;
        }
    }
    return false;
}

// ==================== 类型工具 ====================

// 是否数值类型（整N/正N/浮N/整数/小数）
bool SemanticAnalyzer::isNumeric(const std::string& type) {
    return isInteger(type) || isFloat(type);
}

// 是否整数类型（整8~整128/正8~正128/整数）
bool SemanticAnalyzer::isInteger(const std::string& type) {
    if (type == "整数") return true;  // 整32别名
    if (type == "整8" || type == "整16" || type == "整32" ||
        type == "整64" || type == "整128") return true;
    if (type == "正8" || type == "正16" || type == "正32" ||
        type == "正64" || type == "正128") return true;
    return false;
}

// 是否浮点类型（浮32/浮64/小数）
bool SemanticAnalyzer::isFloat(const std::string& type) {
    return type == "浮32" || type == "浮64" || type == "小数";
}

// 是否能够隐式转换（CN规范：相同类型、整型宽化、浮点宽化、字符↔整型）
bool SemanticAnalyzer::canConvert(const std::string& from, const std::string& to) {
    if (from == to) return true;
    // 字符 ↔ 整数
    if ((from == "字符" && isInteger(to)) || (isInteger(from) && to == "字符")) return true;
    // 整型宽化：小位宽 -> 大位宽（含 整数=整32 别名）
    if (isInteger(from) && isInteger(to)) {
        static const std::unordered_map<std::string, int> intRank = {
            {"正8", 1}, {"整8", 1},
            {"正16", 2}, {"整16", 2},
            {"正32", 3}, {"整32", 3}, {"整数", 3},
            {"正64", 4}, {"整64", 4},
            {"正128", 5}, {"整128", 5},
        };
        int rankFrom = 0, rankTo = 0;
        auto itFrom = intRank.find(from);
        auto itTo = intRank.find(to);
        if (itFrom != intRank.end()) rankFrom = itFrom->second;
        if (itTo != intRank.end()) rankTo = itTo->second;
        // 无符号 -> 有符号（同秩）不允许隐式；允许严格宽化
        // 注：'正' 为多字节UTF-8字符，需用字符串前缀比较而非单字节char
        bool fromUnsigned = from.compare(0, 3, "正") == 0;
        bool toUnsigned = to.compare(0, 3, "正") == 0;
        if (rankFrom == rankTo && fromUnsigned != toUnsigned) return false;
        return rankFrom < rankTo;
    }
    // 浮点宽化：浮32 -> 浮64/小数
    if (from == "浮32" && (to == "浮64" || to == "小数")) return true;
    // 整数 -> 浮点（允许整数隐式转浮点，浮点不可隐式转整数）
    if (isInteger(from) && isFloat(to)) return true;
    return false;
}

// 数值运算结果类型：整型取宽者，含浮点则取浮点
std::string SemanticAnalyzer::commonNumericType(const std::string& a, const std::string& b) {
    if (a == b) return a;
    // 含浮点：取较宽浮点
    if (isFloat(a) || isFloat(b)) {
        bool aFloat64 = (a == "浮64" || a == "小数");
        bool bFloat64 = (b == "浮64" || b == "小数");
        if (aFloat64 || bFloat64) return "浮64";
        return "浮32";
    }
    // 均为整型：取秩高者（宽化）
    static const std::unordered_map<std::string, int> intRank = {
        {"正8", 1}, {"整8", 1},
        {"正16", 2}, {"整16", 2},
        {"正32", 3}, {"整32", 3}, {"整数", 3},
        {"正64", 4}, {"整64", 4},
        {"正128", 5}, {"整128", 5},
    };
    auto itA = intRank.find(a);
    auto itB = intRank.find(b);
    if (itA != intRank.end() && itB != intRank.end()) {
        return (itA->second >= itB->second) ? a : b;
    }
    return "整32";  // 未知类型回退
}

// 是否复合赋值运算符
bool SemanticAnalyzer::isCompoundAssign(Operator op) {
    switch (op) {
        case Operator::PlusAssign: case Operator::MinusAssign:
        case Operator::StarAssign: case Operator::SlashAssign:
        case Operator::PercentAssign:
        case Operator::AmpAssign: case Operator::PipeAssign:
        case Operator::CaretAssign: case Operator::LessLessAssign:
        case Operator::GreaterGreaterAssign:
            return true;
        default:
            return false;
    }
}

// ==================== 检查辅助 ====================

// 检查条件表达式类型：必须为布尔类型
void SemanticAnalyzer::checkCondition(const std::string& type, const SourceLocation& loc,
                                      const std::string& ctx) {
    if (type != "布尔") {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            ctx + "条件必须为布尔类型，实际为 '" + type + "'");
    }
}

// 第一趟：注册函数符号（支持前向调用与重名检测）
void SemanticAnalyzer::registerFunction(FunctionDecl* node) {
    FunctionInfo info;
    info.returnType = node->returnType.empty() ? "空类型" : node->returnType;
    info.hasBody = (node->body != nullptr);
    for (auto& param : node->params) {
        info.paramTypes.push_back(param->typeName);
    }
    auto it = functions_.find(node->name);
    if (it != functions_.end()) {
        // 重名：允许"原型声明 + 定义"组合，其余为重复定义
        bool isProtoPlusDef = !it->second.hasBody && info.hasBody;
        if (!isProtoPlusDef) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "重复定义函数 '" + node->name + "'");
        }
        return;
    }
    functions_[node->name] = info;
}

// 函数体是否保证有返回：最后一条语句为返回语句或无限循环
bool SemanticAnalyzer::bodyGuaranteesReturn(BlockStmt* body) const {
    if (body == nullptr || body->statements.empty()) return false;
    Stmt* last = body->statements.back().get();
    if (last->getType() == NodeType::ReturnStmt) return true;
    // 无限循环：循环 ( ; ; ) { } 或 循环 { }（无条件表达式）
    if (last->getType() == NodeType::ForStmt) {
        ForStmt* forStmt = static_cast<ForStmt*>(last);
        if (forStmt->condition == nullptr) return true;
    }
    return false;
}

// ==================== 声明节点 ====================

// 程序入口：两趟处理
void SemanticAnalyzer::visitProgram(Program* node) {
    pushScope();  // 全局作用域
    // 第一趟：注册全部函数符号（含前向调用）
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            registerFunction(decl.get());
        }
    }
    // 第二趟：逐个检查函数体
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            checkFunctionBody(static_cast<FunctionDecl*>(decl.get()));
        }
    }
    popScope();
}

// 主入口：分析程序AST，返回是否成功
bool SemanticAnalyzer::analyze(Program* program) {
    if (program == nullptr) return false;
    visitProgram(program);
    return !diagnostics_.hasErrors();
}

// 第二趟：检查函数体（参数入作用域 + 语句检查 + 返回类型检查）
void SemanticAnalyzer::checkFunctionBody(FunctionDecl* node) {
    // 函数符号必须已注册（原型声明无函数体）
    auto it = functions_.find(node->name);
    if (it == functions_.end()) return;
    if (node->body == nullptr) return;  // 函数原型声明：无需检查体

    currentReturnType_ = it->second.returnType;
    pushScope();  // 参数作用域
    for (auto& param : node->params) {
        if (!declareVar(param->name, param->typeName, param->location)) {
            // 重复声明参数
        }
    }
    // 检查函数体
    for (auto& stmt : node->body->statements) {
        checkStmt(stmt.get());
    }
    // 缺少返回语句检查：有返回类型且函数体不保证返回
    if (currentReturnType_ != "空类型" && !bodyGuaranteesReturn(node->body.get())) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "函数 '" + node->name + "' 缺少返回语句，返回类型为 '" +
                            currentReturnType_ + "'");
    }
    currentReturnType_.clear();
    popScope();
}

void SemanticAnalyzer::visitFunctionDecl(FunctionDecl* node) {
    // 由 visitProgram 两趟驱动；单独访问时仅注册（防御性）
    registerFunction(node);
}

void SemanticAnalyzer::visitParamDecl(ParamDecl* node) {
    // 参数由 checkFunctionBody 统一入符号表，这里不做处理
    (void)node;
}

// 变量声明：类型检查 + 类型推断 + 入符号表
void SemanticAnalyzer::visitVarDecl(VarDecl* node) {
    std::string varType = node->typeName;
    if (varType.empty() && node->initializer != nullptr) {
        // 类型推断：无显式类型时从初始值推断
        varType = checkExpr(node->initializer.get());
    } else if (node->initializer != nullptr) {
        // 显式类型：检查初始值可隐式转换
        std::string initType = checkExpr(node->initializer.get());
        if (!canConvert(initType, varType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "无法将 '" + initType + "' 隐式转换为 '" + varType + "'");
        }
    }
    if (varType.empty()) {
        // 无类型标注且无初始值：类型未知
        varType = "未知";
    }
    declareVar(node->name, varType, node->location);
}

// ==================== 语句节点 ====================

// 代码块：进入子作用域检查
void SemanticAnalyzer::visitBlockStmt(BlockStmt* node) {
    pushScope();
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
    popScope();
}

void SemanticAnalyzer::visitExprStmt(ExprStmt* node) {
    checkExpr(node->expr.get());
}

// 如果语句：条件必须为布尔；分支各自进入子作用域
void SemanticAnalyzer::visitIfStmt(IfStmt* node) {
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'如果'");
    if (node->thenBranch != nullptr) checkBlock(node->thenBranch.get());
    if (node->elseBranch != nullptr) checkStmt(node->elseBranch.get());
}

// 当循环：条件必须为布尔；循环体进入子作用域且允许中断/继续
void SemanticAnalyzer::visitWhileStmt(WhileStmt* node) {
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'当'");
    loopDepth_++;
    if (node->body != nullptr) checkBlock(node->body.get());
    loopDepth_--;
}

// 循环语句：初始化/条件/更新检查；循环体进入子作用域且允许中断/继续
void SemanticAnalyzer::visitForStmt(ForStmt* node) {
    if (node->init != nullptr) checkStmt(node->init.get());
    if (node->condition != nullptr) {
        checkCondition(checkExpr(node->condition.get()), node->condition->location, "'循环'");
    }
    if (node->update != nullptr) checkExpr(node->update.get());
    loopDepth_++;
    if (node->body != nullptr) checkBlock(node->body.get());
    loopDepth_--;
}

// 返回语句：检查返回值类型与函数返回类型匹配
void SemanticAnalyzer::visitReturnStmt(ReturnStmt* node) {
    if (currentReturnType_.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'返回'语句只能出现在函数体内");
        return;
    }
    if (node->value == nullptr) {
        // 无返回值：要求函数返回类型为空类型
        if (currentReturnType_ != "空类型") {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数返回类型为 '" + currentReturnType_ +
                                "'，但返回语句未携带值");
        }
        return;
    }
    std::string valueType = checkExpr(node->value.get());
    if (currentReturnType_ == "空类型") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "空类型函数不允许返回值");
    } else if (!canConvert(valueType, currentReturnType_)) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + valueType + "' 隐式转换为返回类型 '" +
                            currentReturnType_ + "'");
    }
}

// 中断语句：只能在循环内使用
void SemanticAnalyzer::visitBreakStmt(BreakStmt* node) {
    if (loopDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'中断'语句只能出现在循环体内");
    }
}

// 继续语句：只能在循环内使用
void SemanticAnalyzer::visitContinueStmt(ContinueStmt* node) {
    if (loopDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'继续'语句只能出现在循环体内");
    }
}

// ==================== 表达式节点 ====================

// 整数字面量：推断为整32（阶段一默认）
void SemanticAnalyzer::visitIntegerLiteral(IntegerLiteral* node) {
    lastType_ = "整32";
    (void)node;
}

// 浮点字面量：推断为浮64（阶段一默认）
void SemanticAnalyzer::visitFloatLiteral(FloatLiteral* node) {
    lastType_ = "浮64";
    (void)node;
}

// 字符串字面量：类型为字符串
void SemanticAnalyzer::visitStringLiteral(StringLiteral* node) {
    lastType_ = "字符串";
    (void)node;
}

// 字符字面量：类型为字符
void SemanticAnalyzer::visitCharLiteral(CharLiteral* node) {
    lastType_ = "字符";
    (void)node;
}

// 布尔字面量：类型为布尔
void SemanticAnalyzer::visitBoolLiteral(BoolLiteral* node) {
    lastType_ = "布尔";
    (void)node;
}

// 标识符表达式：从符号表查找变量或函数
void SemanticAnalyzer::visitIdentifierExpr(IdentifierExpr* node) {
    std::string varType;
    if (lookupVar(node->name, varType)) {
        lastType_ = varType;
        return;
    }
    auto it = functions_.find(node->name);
    if (it != functions_.end()) {
        lastType_ = it->second.returnType;
        return;
    }
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "未声明的标识符 '" + node->name + "'");
    lastType_ = "未知";
}

// 二元运算：操作数类型兼容检查 + 结果类型推导
void SemanticAnalyzer::visitBinaryExpr(BinaryExpr* node) {
    std::string leftType = checkExpr(node->left.get());
    std::string rightType = checkExpr(node->right.get());

    if (isLogicalOp(node->op)) {
        // 逻辑运算：操作数必须为布尔，结果为布尔
        if (node->op != Operator::Bang) {  // Bang 由一元表达式处理
            if (leftType != "布尔" || rightType != "布尔") {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "逻辑运算符要求布尔操作数，实际为 '" + leftType +
                                    "' 与 '" + rightType + "'");
            }
        }
        lastType_ = "布尔";
        return;
    }

    if (isComparisonOp(node->op)) {
        // 比较运算：要求可互相转换的同类操作数，结果为布尔
        if (leftType == "未知" || rightType == "未知") {
            lastType_ = "布尔";
            return;
        }
        if (!canConvert(leftType, rightType) && !canConvert(rightType, leftType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "比较运算操作数类型不兼容：'" + leftType + "' 与 '" +
                                rightType + "'");
        }
        lastType_ = "布尔";
        return;
    }

    if (isBitwiseOp(node->op)) {
        // 位运算：要求整数操作数，结果为整数
        if (!isInteger(leftType) || !isInteger(rightType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "位运算要求整数操作数，实际为 '" + leftType + "' 与 '" +
                                rightType + "'");
        }
        lastType_ = isInteger(leftType) ? leftType : "整32";
        return;
    }

    // 算术运算（+ - * / %）：要求数值操作数
    if (node->op == Operator::Add || node->op == Operator::Subtract ||
        node->op == Operator::Multiply || node->op == Operator::Divide ||
        node->op == Operator::Modulo) {
        if (!isNumeric(leftType) || !isNumeric(rightType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "算术运算符要求数值操作数，实际为 '" + leftType + "' 与 '" +
                                rightType + "'");
        }
        // 取余要求整数操作数
        if (node->op == Operator::Modulo && (!isInteger(leftType) || !isInteger(rightType))) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "'%'取余运算要求整数操作数");
        }
        lastType_ = commonNumericType(leftType, rightType);
        return;
    }

    // 其他运算符（阶段一不支持，回退左操作数类型）
    lastType_ = leftType;
}

// 一元运算：! 要求布尔，- ~ 要求数值/整数，++ -- 要求左值数值
void SemanticAnalyzer::visitUnaryExpr(UnaryExpr* node) {
    std::string operandType = checkExpr(node->operand.get());
    switch (node->op) {
        case Operator::Bang:
            // 逻辑非：要求布尔
            if (operandType != "布尔") {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'!'逻辑非要求布尔操作数，实际为 '" + operandType + "'");
            }
            lastType_ = "布尔";
            break;
        case Operator::Subtract:
            // 一元负号：要求数值
            if (!isNumeric(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "一元负号要求数值操作数，实际为 '" + operandType + "'");
            }
            lastType_ = operandType;
            break;
        case Operator::Tilde:
            // 按位非：要求整数
            if (!isInteger(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'~'按位非要求整数操作数，实际为 '" + operandType + "'");
            }
            lastType_ = operandType;
            break;
        case Operator::Increment:
        case Operator::Decrement:
            // 自增/自减：要求左值数值（阶段一只做类型检查，左值性在赋值检查中体现）
            if (!isNumeric(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "自增/自减要求数值操作数，实际为 '" + operandType + "'");
            }
            lastType_ = operandType;
            break;
        default:
            lastType_ = operandType;
            break;
    }
}

// 赋值表达式：左值必须为标识符（阶段一），类型兼容检查
void SemanticAnalyzer::visitAssignmentExpr(AssignmentExpr* node) {
    // 检查左值（阶段一只支持标识符左值）
    std::string targetType = "未知";
    if (node->target->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
        std::string varType;
        if (lookupVar(ident->name, varType)) {
            targetType = varType;
        } else {
            diagnostics_.report(DiagnosticLevel::Error, ident->location,
                                "赋值目标未声明：'" + ident->name + "'");
        }
    } else {
        // 其他左值形式（成员访问等）：阶段二实现
        targetType = checkExpr(node->target.get());
    }

    // 检查右值
    std::string valueType = checkExpr(node->value.get());

    // 复合赋值：+= -= 等要求数值
    if (isCompoundAssign(node->op)) {
        if (!isNumeric(targetType) || !isNumeric(valueType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "复合赋值要求数值操作数，实际为 '" + targetType +
                                "' 与 '" + valueType + "'");
        }
        if (targetType == "未知") {
            lastType_ = valueType;
            return;
        }
        lastType_ = targetType;
        return;
    }

    // 简单赋值 =：要求右值可隐式转换为左值类型
    if (targetType != "未知" && valueType != "未知" &&
        !canConvert(valueType, targetType)) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + valueType + "' 隐式转换为 '" + targetType + "'");
    }
    lastType_ = targetType == "未知" ? valueType : targetType;
}

// 函数调用：检查被调者与实参数量/类型
void SemanticAnalyzer::visitCallExpr(CallExpr* node) {
    // 被调者：阶段一只支持直接函数名调用
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
    } else {
        // 其他被调者（成员函数等）：阶段二实现，跳过检查
        lastType_ = "未知";
        return;
    }

    auto it = functions_.find(calleeName);
    if (it == functions_.end()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "未声明的函数 '" + calleeName + "'");
        lastType_ = "未知";
        return;
    }
    const FunctionInfo& info = it->second;

    // 参数数量检查
    if (node->arguments.size() != info.paramTypes.size()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "函数 '" + calleeName + "' 期望 " +
                            std::to_string(info.paramTypes.size()) + " 个参数，实际提供 " +
                            std::to_string(node->arguments.size()) + " 个");
        lastType_ = info.returnType;
        return;
    }
    // 参数类型检查
    for (std::size_t i = 0; i < node->arguments.size(); i++) {
        std::string argType = checkExpr(node->arguments[i].get());
        const std::string& paramType = info.paramTypes[i];
        if (!canConvert(argType, paramType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                "函数 '" + calleeName + "' 第 " + std::to_string(i + 1) +
                                " 个参数无法将 '" + argType + "' 隐式转换为 '" +
                                paramType + "'");
        }
    }
    lastType_ = info.returnType;
}

// 成员访问：阶段一不支持，标记为未知类型
void SemanticAnalyzer::visitMemberExpr(MemberExpr* node) {
    (void)node;
    lastType_ = "未知";
}

// 类型节点：语义阶段不做处理
void SemanticAnalyzer::visitType(Type* node) {
    (void)node;
}

// ==================== 分发辅助 ====================

// 检查表达式，返回推断类型（存到 lastType_）
std::string SemanticAnalyzer::checkExpr(Expr* node) {
    if (node == nullptr) return "未知";
    node->accept(*this);
    return lastType_;
}

// 检查代码块（含作用域进出）
void SemanticAnalyzer::checkBlock(BlockStmt* node) {
    if (node == nullptr) return;
    visitBlockStmt(node);
}

// 检查语句：分发到对应 visit 方法
void SemanticAnalyzer::checkStmt(Stmt* node) {
    if (node == nullptr) return;
    node->accept(*this);
}

} // namespace cn_compiler
