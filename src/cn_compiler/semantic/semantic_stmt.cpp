// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

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

// 算术运算符（+ - * / %）
bool isArithmeticOp(Operator op) {
    return op == Operator::Add || op == Operator::Subtract ||
           op == Operator::Multiply || op == Operator::Divide ||
           op == Operator::Modulo;
}

// 指针类型辅助（Task 2.4）：是否指针类型 / 是否数组类型
bool isPointerType(const std::string& type) {
    return types::isPointer(type);
}
bool isArrayType(const std::string& type) {
    return types::isArray(type);
}
// 计算数组总字节大小（元素大小 × 长度）
// GCC -Wunused-function 下标记 maybe_unused（MSVC 不报，GCC 严格）
[[maybe_unused]] int arrayTotalSize(const std::string& type) {
    const int len = types::arrayLenOf(type);
    const int elemSize = types::typeSize(types::arrayElemOf(type));
    if (len <= 0 || elemSize <= 0) return 0;
    return len * elemSize;
}

// 类型别名规范化：整数 -> 整32、小数 -> 浮64（规格书02-类型系统：默认类型别名）
// Task 2.3：转发到 type_system 子模块（types::canonical），语义与IR共用同一实现
std::string canonicalType(const std::string& type) {
    return types::canonical(type);
}

// ==================== 函数指针类型工具（Task 2.2） ====================

// 判断类型字符串是否为函数指针类型（函数指针<返回>(参数,...)）
bool isFuncPtrTypeStr(const std::string& type) {
    return type.rfind("函数指针<", 0) == 0;
}

// 从函数指针类型字符串提取返回类型（"函数指针<整32>(整32,整32)" -> "整32"）
std::string funcPtrReturn(const std::string& type) {
    std::size_t lt = type.find('<');
    std::size_t gt = type.find('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    return type.substr(lt + 1, gt - lt - 1);
}

// 从函数指针类型字符串提取参数类型列表
// "函数指针<整32>(整32,整32)" -> ["整32","整32"]
std::vector<std::string> funcPtrParams(const std::string& type) {
    std::vector<std::string> result;
    std::size_t lp = type.find('(');
    std::size_t rp = type.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp) return result;
    std::string inner = type.substr(lp + 1, rp - lp - 1);
    // 按逗号分割（参数为基本类型，无嵌套逗号）
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        std::size_t comma = inner.find(',', pos);
        if (comma == std::string::npos) comma = inner.size();
        std::string p = inner.substr(pos, comma - pos);
        // 去除首尾空白
        std::size_t b = p.find_first_not_of(" \t");
        std::size_t e = p.find_last_not_of(" \t");
        if (b != std::string::npos && e != std::string::npos) {
            result.push_back(p.substr(b, e - b + 1));
        }
        pos = comma + 1;
    }
    return result;
}

} // namespace

void SemanticAnalyzer::visitBlockStmt(BlockStmt* node) {
    pushScope();
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
    popScope();
}
void SemanticAnalyzer::visitExprStmt(ExprStmt* node) {
    const std::string exprType = checkExpr(node->expr.get());
    // 规则1（Task 3.5）：结果<T,E> 返回值被丢弃未检查 -> 错误
    // 仅函数调用表达式触发（普通表达式如 结果.正常 读取本身即检查）
    if (node->expr->getType() == NodeType::CallExpr) {
        checkResultDiscard(exprType, node->expr->location);
    }
}
void SemanticAnalyzer::visitIfStmt(IfStmt* node) {
    // 结果/可选 检查跟踪（条件为 结果.正常 / 可选.有值 时专用处理）
    if (node->condition->getType() == NodeType::MemberExpr) {
        MemberExpr* cond = static_cast<MemberExpr*>(node->condition.get());
        const std::string condObjType = checkExpr(cond->object.get());
        const bool isResultCheck =
            isResultType(condObjType) && cond->memberName == "正常";
        const bool isOptionalCheck =
            isOptionalType(condObjType) && cond->memberName == "有值";
        if (isResultCheck || isOptionalCheck) {
            trackIfCheck(node);
            return;
        }
    }
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'如果'");
    if (node->thenBranch != nullptr) checkBlock(node->thenBranch.get());
    if (node->elseBranch != nullptr) checkStmt(node->elseBranch.get());
}
void SemanticAnalyzer::visitWhileStmt(WhileStmt* node) {
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'当'");
    loopDepth_++;
    if (node->body != nullptr) checkBlock(node->body.get());
    loopDepth_--;
}
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
void SemanticAnalyzer::visitReturnStmt(ReturnStmt* node) {
    // lambda 返回类型推导模式（Task 2.10）：currentReturnType_ 为空，
    // 不报"只能出现在函数体内"，而是收集返回表达式类型作为推导候选
    if (currentReturnType_.empty()) {
        if (lambdaInferMode_) {
            if (node->value != nullptr) {
                lambdaReturnCandidate_.push_back(checkExpr(node->value.get()));
            } else {
                lambdaReturnCandidate_.push_back("空类型");
            }
            return;
        }
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
    } else if (!canConvertType(valueType, currentReturnType_)) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + valueType + "' 隐式转换为返回类型 '" +
                            currentReturnType_ + "'");
    }
}
void SemanticAnalyzer::visitBreakStmt(BreakStmt* node) {
    if (loopDepth_ == 0 && switchDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'中断'语句只能出现在循环体内或选择语句内");
    }
}
void SemanticAnalyzer::visitContinueStmt(ContinueStmt* node) {
    if (loopDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'继续'语句只能出现在循环体内");
    }
}
void SemanticAnalyzer::visitSwitchStmt(SwitchStmt* node) {
    std::string condType = checkExpr(node->condition.get());
    // Task 2.7：枚举条件允许（枚举本质为整32，case 值为枚举成员整数值）
    if (!isInteger(condType) && condType != "字符" && !isEnumType(condType)) {
        diagnostics_.report(DiagnosticLevel::Error, node->condition->location,
                            "选择语句的表达式必须是整数或字符类型，实际为 '" +
                            condType + "'");
    }
    // 情况值去重检测（编译期常量，语义层用 set 去重）
    std::unordered_set<std::int64_t> seenValues;
    switchDepth_++;
    for (auto& caseNode : node->cases) {
        // 枚举引用情况值求值：rawValue 形如 "颜色.红"（Task 2.7）
        if (caseNode->rawValue.find('.') != std::string::npos) {
            const std::size_t dotPos = caseNode->rawValue.find('.');
            const std::string enumName = caseNode->rawValue.substr(0, dotPos);
            const std::string memberName = caseNode->rawValue.substr(dotPos + 1);
            std::int64_t enumVal = 0;
            if (enumValueOf(enumName, memberName, enumVal)) {
                caseNode->value = enumVal;
            } else {
                diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                    "情况标签枚举引用无效：'" + caseNode->rawValue + "'");
            }
        }
        // 重复检测：与已见情况值比较
        if (!seenValues.insert(caseNode->value).second) {
            diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                "选择语句中情况值 '" + caseNode->rawValue +
                                "' 重复");
        }
        checkStmt(caseNode.get());
    }
    if (node->defaultCase != nullptr) {
        checkStmt(node->defaultCase.get());
    }
    switchDepth_--;
}
void SemanticAnalyzer::visitCaseLabel(CaseLabel* node) {
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
}
void SemanticAnalyzer::visitDefaultLabel(DefaultLabel* node) {
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
}
} // namespace cn_compiler
