// CN 语义分析器——一元表达式检查（D1 行数整改 181-a：自 semantic_expr_op.cpp 按族拆出）
//   族 = 一元表达式检查（visitUnaryExpr 主分派 + 5 个操作符族子方法：P2-14
//   运算符重载/取地址 &/解引用 */错误传播 ?/自增自减）；纯重构零行为变更
//   （成员函数实现搬迁——声明仍在 semantic.hpp）。
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

void SemanticAnalyzer::visitUnaryExpr(UnaryExpr* node) {
    std::string operandType = checkExpr(node->operand.get());
    // P2-14：单目运算符重载（- ! ~）——类类型操作数先查 运算符X（0 参数）成员
    //   （重载决议：类重载优先；无重载则落入下方内置校验/报错）
    // 181-a：本函数 204 行按操作符 case 提取为 5 个族子方法（纯搬运零行为
    //   变更——case 体逐行搬运，break 语义=方法返回，多重集核验先行于构建）。
    if (checkUnaryOperatorOverload(node, operandType)) {
        return;
    }
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
        case Operator::AddressOf:
            // 取地址 &：操作数须为左值（标识符/下标/解引用），结果为指向其类型的指针
            // 数组取地址 &数组：数组名退化后取首元素地址（语义层数组名已是地址值）
            checkAddressOfUnary(node, operandType);
            break;
        case Operator::Deref:
            // 解引用 *：操作数须为指针类型，结果为所指元素类型（可写左值）
            checkDerefUnary(node, operandType);
            break;
        case Operator::Propagate:
            // C-1（错误传播运算符，2026-08）：表达式? ——操作数须为
            //   结果<T,E>/可选<T>；当前函数返回类型须兼容（结果->结果、
            //   可选->可选）；传播后值类型 = T（正常分支取 .值）。
            //   失败分支由 IR 层生成"构造错误结果并返回"（Rust ? 语义）
            checkPropagateUnary(node, operandType);
            break;
        case Operator::Increment:
        case Operator::Decrement:
            // 缺陷②同族（2026-09-03）：自增/自减是隐式赋值——目标须为可写左值
            checkIncDecUnary(node, operandType);
            break;
        default:
            lastType_ = operandType;
            break;
    }
}

// ==================== 181-a 族子方法（原 visitUnaryExpr 277~473 段） ====================

// 族①：单目运算符重载检查（P2-14·原 277~291 段）。true = 类重载命中已处理。
bool SemanticAnalyzer::checkUnaryOperatorOverload(UnaryExpr* node,
                                                  const std::string& operandType) {
    // P2-14：单目运算符重载（- ! ~）——类类型操作数先查 运算符X（0 参数）成员
    //   （重载决议：类重载优先；无重载则落入下方内置校验/报错）
    if (isClassType(canonicalType(operandType))) {
        const std::string opSym = (node->op == Operator::Bang) ? "!" :
                                  (node->op == Operator::Tilde) ? "~" :
                                  (node->op == Operator::Subtract) ? "-" : "";
        if (!opSym.empty()) {
            const ClassMemberInfo* mi = resolveOperatorOverload(
                opSym, operandType, {}, node->location);
            if (mi != nullptr) {
                lastType_ = mi->type;
                return true;
            }
        }
    }
    return false;
}

// 族②：取地址 & 左值校验（原 317~343 段）——数组退化/左值形态/&引用返回调用。
void SemanticAnalyzer::checkAddressOfUnary(UnaryExpr* node,
                                           const std::string& operandType) {
            // 取地址 &：操作数须为左值（标识符/下标/解引用），结果为指向其类型的指针
            // 数组取地址 &数组：数组名退化后取首元素地址（语义层数组名已是地址值）
            if (isArrayType(operandType)) {
                // &数组 -> 指向数组的指针（此处简化为指向元素指针，数组退化语义）
                lastType_ = types::arrayElemOf(operandType) + "*";
            } else if (node->operand->getType() == NodeType::IdentifierExpr ||
                       node->operand->getType() == NodeType::IndexExpr ||
                       node->operand->getType() == NodeType::MemberExpr) {
                lastType_ = operandType + "*";
                // 188-a（D6·plans/023 B11 变量常量传播）：取地址=别名逃逸——外部
                //   可经别名改写该变量 → 恒空判定失格（保守，防假阳性）
                if (node->operand->getType() == NodeType::IdentifierExpr) {
                    noteNullEscape(
                        static_cast<IdentifierExpr*>(node->operand.get())->name);
                }
            } else if (node->operand->getType() == NodeType::CallExpr) {
                // P3-18 补完：&引用返回调用 = 取得被引用者的地址（须确认为引用返回）
                if (lastExprIsRefReturn_) {
                    lastType_ = node->operand->getType() == NodeType::CallExpr
                                    ? types::canonical(operandType) + "*"
                                    : operandType + "*";
                } else {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "'&'取地址要求左值操作数");
                    lastType_ = "未知";
                }
            } else {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'&'取地址要求左值操作数");
                lastType_ = "未知";
            }
}

// 族③：解引用 * 校验（原 344~366 段）——指针要求 + B11 常量空指针硬错误 +
//   B6 裸指针解引用读警告（排除字符串语义与赋值目标位）。
void SemanticAnalyzer::checkDerefUnary(UnaryExpr* node,
                                       const std::string& operandType) {
            // 解引用 *：操作数须为指针类型，结果为所指元素类型（可写左值）
            if (isPointerType(operandType)) {
                lastType_ = types::pointeeOf(operandType);
                // 150-a（plans/023 B6/B11 实施）：安全区边界族群——
                //   B11（硬错误）：编译期常量空指针解引用（操作数=无 字面量）；
                //   B6（观察期警告）：裸指针解引用读（排除字符串语义=字符串视图；
                //     赋值目标位（assignmentTargetDepth_>0）由 target case 报 B7）。
                if (node->operand->getType() == NodeType::NullLiteral) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "编译期常量空指针解引用（确定性错误；plans/023 B11）");
                } else if (node->operand->getType() == NodeType::IdentifierExpr) {
                    // 188-a（D6 B11 变量常量传播）：标识符操作数 → 使用点登记
                    //   （判定在函数体检查收尾统一做——恒空表见 semantic.hpp）
                    noteNullUse(
                        static_cast<IdentifierExpr*>(node->operand.get())->name,
                        node->location, "解引用");
                }
                if (node->operand->getType() != NodeType::NullLiteral &&
                    assignmentTargetDepth_ == 0 &&
                    !isStringSemanticType(operandType)) {
                    reportUnsafeBoundary(node->location, "裸指针解引用读",
                                       "指针解引用（*p）");
                }
            } else {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'*'解引用要求指针操作数，实际为 '" + operandType + "'");
                lastType_ = "未知";
            }
}

// 族④：错误传播 ? 校验（C-1·原 367~435 段）——结果/可选 操作数要求 +
//   返回类型兼容（值类型精确一致/错误类型可转换；propagateType=当前返回类型）。
void SemanticAnalyzer::checkPropagateUnary(UnaryExpr* node,
                                           const std::string& operandType) {
            // C-1（错误传播运算符，2026-08）：表达式? ——操作数须为
            //   结果<T,E>/可选<T>；当前函数返回类型须兼容（结果->结果、
            //   可选->可选）；传播后值类型 = T（正常分支取 .值）。
            //   失败分支由 IR 层生成"构造错误结果并返回"（Rust ? 语义）
            {
                // 宿主缺陷根治（2026-08-25）：propagateType 应为当前函数返回类型
                //   （currentReturnType_）而非操作数类型——宽错误码传播（操作数
                //   结果<整32,整32>、函数返回 结果<整32,整64>）原用操作数类型，
                //   IR 层错误临时按 结果$整32$整32（8 字节/偏移 4）分配，返回时
                //   按 16 字节拷贝溢出 -> 主函数读 .错误 错位（4294967296 实测）。
                //   IR 层读侧偏移/类型按操作数类型、写侧按 propagateType 分别取。
                node->propagateType = currentReturnType_;
                if (isResultType(operandType) || isOptionalType(operandType)) {
                    const std::vector<std::string> args = resultTypeArgs(operandType);
                    if (isResultType(operandType) && args.size() == 2) {
                        // 结果<T,E>：返回类型须为 结果<T,E2>（E2 与 E 可转换）
                        lastType_ = canonicalType(args[0]);
                        if (currentReturnType_.empty() ||
                            !isResultType(currentReturnType_)) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->location,
                                "'?'错误传播要求当前函数返回 结果<" +
                                    canonicalType(args[0]) + ", " +
                                    canonicalType(args[1]) + "> 类型（实际返回 '" +
                                    currentReturnType_ + "'）");
                        } else {
                            const std::vector<std::string> retArgs =
                                resultTypeArgs(currentReturnType_);
                            // 值类型须精确一致（IR 层按操作数 T 直接取 .值，
                            //   不自动转换；与 Rust ? 同型传播一致）
                            if (retArgs.size() == 2 &&
                                canonicalType(retArgs[0]) != canonicalType(args[0])) {
                                diagnostics_.report(
                                    DiagnosticLevel::Error, node->location,
                                    "'?'传播的值类型 '" + canonicalType(args[0]) +
                                        "' 与函数返回类型的值类型 '" +
                                        canonicalType(retArgs[0]) + "' 不一致");
                            }
                            if (retArgs.size() == 2 &&
                                !canConvertType(args[1], retArgs[1])) {
                                diagnostics_.report(
                                    DiagnosticLevel::Error, node->location,
                                    "'?'传播的错误类型 '" + canonicalType(args[1]) +
                                        "' 无法转换为函数返回类型的错误类型 '" +
                                        canonicalType(retArgs[1]) + "'");
                            }
                        }
                    } else if (isOptionalType(operandType)) {
                        // 可选<T>：返回类型须为 可选<T>
                        lastType_ = canonicalType(optionalTypeArg(operandType));
                        if (currentReturnType_.empty() ||
                            currentReturnType_ != "可选<" + lastType_ + ">") {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->location,
                                "'?'错误传播要求当前函数返回 可选<" + lastType_ +
                                    "> 类型（实际返回 '" + currentReturnType_ + "'）");
                        }
                    } else {
                        lastType_ = "未知";
                    }
                } else {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "'?'错误传播要求 结果<T,E>/可选<T> 类型操作数，实际为 '" +
                                            operandType + "'");
                    lastType_ = "未知";
                }
            }
}

// 族⑤：自增/自减校验（原 436~473 段）——左值性/常量/数值或指针要求。
void SemanticAnalyzer::checkIncDecUnary(UnaryExpr* node,
                                        const std::string& operandType) {
        
            // 缺陷②同族（2026-09-03）：自增/自减是隐式赋值——目标须为可写左值
            //   （变量[非常量]/成员/下标/解引用；引用返回调用不支持，与 C 一致）。
            //   原实现只查数值/指针类型不查左值性，(a + 1)++ 静默通过。
            const NodeType ot = node->operand->getType();
            bool incLvalue = (ot == NodeType::IdentifierExpr || ot == NodeType::IndexExpr ||
                              ot == NodeType::MemberExpr ||
                              (ot == NodeType::UnaryExpr &&
                               static_cast<UnaryExpr*>(node->operand.get())->op ==
                                   Operator::Deref));
            if (!incLvalue) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "自增/自减目标必须是可赋值的左值（变量/成员/下标/解引用）");
                lastType_ = operandType;
            }
            if (ot == NodeType::IdentifierExpr) {
                const IdentifierExpr* ident =
                    static_cast<IdentifierExpr*>(node->operand.get());
                // 188-a（D6）：自增/自减=隐式非空写入 → 恒空判定失格
                noteNullWriteOther(ident->name);
                if (isConstVarName(ident->name)) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "不能对常量 '" + ident->name + "' 自增/自减（常量初始化后不可修改）");
                    lastType_ = operandType;
                    return;
                }
            }
            // 自增/自减：数值 或 指针（Task 2.4 指针 ++/-- 按元素大小步进）
            if (!isNumeric(operandType) && !isPointerType(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "自增/自减要求数值或指针操作数，实际为 '" +
                                    operandType + "'");
            }
            lastType_ = operandType;
}

} // namespace cn_compiler
