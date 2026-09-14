// CN 语义分析器——表达式检查（D1 行数整改 116-a：自 semantic_expr.cpp 按族拆出）
//   族 = 运算符/赋值表达式检查（visitBinaryExpr + visitUnaryExpr + visitTernaryExpr + reportNonLvalueTarget + visitAssignmentExpr）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp）。
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

namespace cn_compiler {

void SemanticAnalyzer::visitBinaryExpr(BinaryExpr* node) {
    std::string leftType = checkExpr(node->left.get());
    std::string rightType = checkExpr(node->right.get());
    // 数组名退化（C语义，Task 2.7 集成修复）：数组类型作为值参与运算时
    // 退化为指向首元素的指针（整32[5] -> 整32*；学生[5] -> 学生*），
    // 使 名单 + 人数（指针算术）与 指针比较 等组合可用
    if (isArrayType(leftType)) leftType = types::arrayElemOf(leftType) + "*";
    if (isArrayType(rightType)) rightType = types::arrayElemOf(rightType) + "*";

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

    // 混合符号二元运算拒绝（2026-09-10 方案A 用户裁决，Rust 对齐）：
    //   有符号（整N）与无符号（正M）的「变量间」比较/算术/位运算编译期拒绝——
    //   隐式宽化在混合符号下静默改变值语义（探针实证 target/p50x64l/mixed_cmp：
    //   正64(2^63) vs 整64(-1) 比较值域反转；正32 vs 整64 负值形态宿主 rc=9 /
    //   v2 rc=11 / 数学真值 13 双侧分叉）。须显式 类型名(表达式) 构造转换。
    //   字面量豁免：一侧为整数字面量（含一元负号字面量）按另一侧类型参与
    //   （Rust 字面量推断同款惯例，E2E 191「大 > 100」锚保留）；同符号混合
    //   宽度（正32 vs 正64）维持既有宽化。规范 plans/001 §3.7。
    // plans/019 阶段4（2026-09-10）：安全区边界观察期——指针算术（指针 +/- 整数
    //   产生新指针=可越出对象边界）应在 不安全 函数 内
    // plans/022 波 1（2026-09-13 用户裁决）：字符串拼接不是指针算术——+ 涉及
    //   字符串语义类型（字符串/字符*）时下方算术分支按拼接分派（字符* 是字符串
    //   视图，IR 层同为 ptr）；原按类型文本 '*' 后缀判指针对该形态误报（宿主
    //   check v2 树 75 处实证全部为 驻留文本(...)/字符串变量 拼接）。字符* 的
    //   -（指针步进，语义层按指针算术分派）与真指针（整N*/类*）的 +/- 保持原判。
    const bool concatAdd =
        node->op == Operator::Add &&
        (isStringSemanticType(leftType) || isStringSemanticType(rightType));
    if (!concatAdd &&
        (node->op == Operator::Add || node->op == Operator::Subtract) &&
        (types::isPointer(leftType) || types::isPointer(rightType))) {
        reportUnsafeBoundary(node->location, "指针算术",
                           "指针 +/- 整数");
    }
    if ((isComparisonOp(node->op) || isArithmeticOp(node->op) || isBitwiseOp(node->op)) &&
        leftType != "未知" && rightType != "未知" &&
        isInteger(leftType) && isInteger(rightType) &&
        types::isUnsigned(leftType) != types::isUnsigned(rightType) &&
        !isIntLiteralExpr(node->left.get()) && !isIntLiteralExpr(node->right.get())) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "混合符号二元运算禁止：'" + leftType + "' 与 '" + rightType +
                            "' —— 须显式转换（如 整64(表达式)/正64(表达式)）；"
                            "字面量豁免（Rust 对齐，2026-09-10 方案A）");
    }

    if (isComparisonOp(node->op)) {
        // 比较运算：要求可互相转换的同类操作数，结果为布尔
        if (leftType == "未知" || rightType == "未知") {
            lastType_ = "布尔";
            return;
        }
        // 灰色点②方案A（2026-09-04 用户裁决）：字符串/字符* 之间的比较运算符
        //   显式拒绝——原「指针间（含字符串）比较按地址」放行 = 同内容异地址的
        //   字符串 ==/!= 恒假（静默陷阱，拼接产物必踩）。规格书 Task 2.8（比较
        //   运算符仅定义整型与浮点变体，字符串不参与运算符比较）；等价能力由
        //   字符串比较（相等）与 字符串字典序（全序）提供。字符串/字符* 与
        //   空类型*（无）的判空比较保留（Task 6.2，133 EOF 载体）；真指针类型
        //   间按地址比较保留（判空/同址判定）。
        const bool leftStrOnly = (leftType == "字符串" || leftType == "字符*");
        const bool rightStrOnly = (rightType == "字符串" || rightType == "字符*");
        if (leftStrOnly && rightStrOnly) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "字符串不支持 ==/!=/</> 等比较运算符（按地址比较而非内容）——"
                                "相等用 字符串比较(a,b)，全序用 字符串字典序(a,b)");
            lastType_ = "布尔";
            return;
        }
        // 指针比较（Task 2.4）：两指针（或指针与空指针）按地址比较；
        // 指针与整型禁止隐式比较（规格书3.7：指针与整数禁止隐式转换）
        // Task 6.2（IO/文件库）：字符串/字符* 本质是 char* 指针，与 空类型*（无）
        //   比较合法（读取行 返回字符串，EOF 返回 nullptr 判定）；视为指针比较。
        const bool leftPtr = isPointerType(leftType) ||
                             leftType == "字符串" || leftType == "字符*";
        const bool rightPtr = isPointerType(rightType) ||
                              rightType == "字符串" || rightType == "字符*";
        if ((leftPtr || rightPtr) && !(leftPtr && rightPtr)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "指针只能与指针或空指针比较，实际为 '" + leftType +
                                "' 与 '" + rightType + "'");
            lastType_ = "布尔";
            return;
        }
        // 指针间（含字符串）比较按地址，无需类型转换检查（字符串 vs 空类型*
        //   均以 ptr 表示，地址比较合法）
        if (leftPtr && rightPtr) {
            lastType_ = "布尔";
            return;
        }
        if (!canConvert(leftType, rightType) && !canConvert(rightType, leftType)) {
            // 55-c 方案A：混合符号+字面量豁免形态（如 大 > 100——canConvertType
            //   对跨符号拒绝后，无字面量豁免会把既有惯用形态误报「类型不兼容」；
            //   该形态已过上方混合符号检查=字面量豁免分支，按另一侧类型参与）
            if (!(isInteger(leftType) && isInteger(rightType) &&
                  (isIntLiteralExpr(node->left.get()) || isIntLiteralExpr(node->right.get())))) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "比较运算操作数类型不兼容：'" + leftType + "' 与 '" +
                                    rightType + "'");
            }
        }
        lastType_ = "布尔";
        return;
    }

    if (isBitwiseOp(node->op)) {
        // 位运算/移位（Task 2.3）：要求整数操作数，结果为两操作数公共整数类型
        // （整型取秩高者；整32 & 整64 -> 整64，与算术推导一致）
        if (!isInteger(leftType) || !isInteger(rightType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "位运算要求整数操作数，实际为 '" + leftType + "' 与 '" +
                                rightType + "'");
        }
        lastType_ = commonNumericType(leftType, rightType);
        if (lastType_ == "浮64" || lastType_ == "浮32") lastType_ = leftType;  // 防御：位运算结果必须整数
        return;
    }

    // 算术运算（+ - * / %）：要求数值操作数；指针算术（Task 2.4）；字符串连接（Task 2.5）
    if (isArithmeticOp(node->op)) {
        // ---- 字符串连接（Task 2.5）：两个字符串/字符* 的 + -> 连接，结果为字符串 ----
        // 说明：字符串与字符* 在 IR 层均为 ptr；语义层需区分"字符串连接"与"指针算术"。
        //       字符串类型（字符串/字符*）的 + 视为连接（字符* 也承载字符串语义）；
        //       判定与顶部安全区边界检查共用 isStringSemanticType（plans/022 波 1）。
        const bool leftStr = isStringSemanticType(leftType);
        const bool rightStr = isStringSemanticType(rightType);
        if (node->op == Operator::Add && leftStr && rightStr) {
            lastType_ = "字符串";  // 连接结果为字符串
            return;
        }
        // ---- 字符串 + 数值 隐式拼接（Task 2.9，规格书3.7 数值→字符串 仅 + 拼接语境）----
        // 左操作数为 字符串/字符*，右操作数为 整数/浮点/布尔/字符/枚举 -> 隐式转字符串再连接。
        // 多操作数左结合："a" + 1 + 2 = ("a"+1)+2（右操作数类型为字符串结果）。
        // 布尔转 "真"/"假"（新增 __cn_str_from_bool）；枚举按整32转（isNumeric 已含整128/正128）。
        const bool rightConcatable =
            isNumeric(rightType) || rightType == "布尔" || rightType == "字符" ||
            isEnumType(rightType);
        if (node->op == Operator::Add && leftStr && rightConcatable) {
            lastType_ = "字符串";
            return;
        }
        // 指针算术：指针 + 整数 / 指针 - 整数（按元素大小偏移，规格书4.4指针运算符）
        const bool leftPtr = isPointerType(leftType);
        const bool rightPtr = isPointerType(rightType);
        if (leftPtr && !rightPtr) {
            // 指针 ± 整数（仅 + - 允许；* / % 不允许指针操作数）
            if (node->op != Operator::Add && node->op != Operator::Subtract) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "指针只能做加减运算，不能做 '" +
                                    std::string(node->op == Operator::Multiply ? "*" :
                                                node->op == Operator::Divide ? "/" : "%") + "'");
            } else if (!isInteger(rightType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "指针算术要求整型偏移，实际为 '" + rightType + "'");
            }
            lastType_ = leftType;  // 结果仍为指针
            return;
        }
        if (!leftPtr && rightPtr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "整数不能与指针做算术运算（仅支持 指针 ± 整数）");
            lastType_ = rightType;
            return;
        }
        if (leftPtr && rightPtr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "两个指针不能做算术运算");
            lastType_ = leftType;
            return;
        }
        // 阶段3（Task 3.7）：类类型左操作数先查运算符重载（重载决议顺序②）
        // 内置算术分支先执行到此（非数值类类型）；命中重载则返回，否则报错。
        if ((!isNumeric(leftType) || !isNumeric(rightType)) && isClassType(canonicalType(leftType))) {
            const std::string opSym = [node]() -> std::string {
                switch (node->op) {
                    case Operator::Add: return "+";
                    case Operator::Subtract: return "-";
                    case Operator::Multiply: return "*";
                    case Operator::Divide: return "/";
                    case Operator::Modulo: return "%";
                    default: return "";
                }
            }();
            if (!opSym.empty()) {
                const ClassMemberInfo* mi = resolveOperatorOverload(
                    opSym, leftType, {rightType}, node->location);
                if (mi != nullptr) {
                    lastType_ = mi->type;
                    // 缺陷2 修复：写回重载结果类型，供 IR 层链式运算符重载识别
                    node->resolvedType = mi->type;
                    return;
                }
            }
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

    // ---- 阶段3：运算符重载决议（Task 3.7，规格书01b）----
    // 重载决议顺序：① 优先内置运算符（上方已处理）；② 无内置匹配时按左操作数
    //   类型查成员 运算符X；③ 无匹配报"类型不兼容"（由上方报错）。
    // 此处拦截：左操作数为类类型（非内置可处理）时查成员运算符。
    if (isClassType(canonicalType(leftType))) {
        const std::string opSym = [node]() -> std::string {
            switch (node->op) {
                case Operator::Add: return "+";
                case Operator::Subtract: return "-";
                case Operator::Multiply: return "*";
                case Operator::Divide: return "/";
                case Operator::Modulo: return "%";
                case Operator::EqualEqual: return "==";
                case Operator::BangEqual: return "!=";
                case Operator::Less: return "<";
                case Operator::Greater: return ">";
                case Operator::LessEqual: return "<=";
                case Operator::GreaterEqual: return ">=";
                default: return "";
            }
        }();
        if (!opSym.empty()) {
            const ClassMemberInfo* mi = resolveOperatorOverload(
                opSym, leftType, {rightType}, node->location);
            if (mi != nullptr) {
                // 运算符重载命中：结果为重载方法返回类型
                lastType_ = mi->type;
                // 缺陷2 修复：写回重载结果类型，供 IR 层链式运算符重载识别
                node->resolvedType = mi->type;
                return;
            }
            // 无重载匹配且非内置：报"类型不兼容"
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "运算符 '" + opSym + "' 与类型 '" + leftType +
                                    "' 不兼容（无内置匹配且类无对应运算符重载）");
            lastType_ = "未知";
            return;
        }
    }
    // 其他运算符（阶段一不支持，回退左操作数类型）
    lastType_ = leftType;
}
void SemanticAnalyzer::visitUnaryExpr(UnaryExpr* node) {
    std::string operandType = checkExpr(node->operand.get());
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
                return;
            }
        }
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
            if (isArrayType(operandType)) {
                // &数组 -> 指向数组的指针（此处简化为指向元素指针，数组退化语义）
                lastType_ = types::arrayElemOf(operandType) + "*";
            } else if (node->operand->getType() == NodeType::IdentifierExpr ||
                       node->operand->getType() == NodeType::IndexExpr ||
                       node->operand->getType() == NodeType::MemberExpr) {
                lastType_ = operandType + "*";
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
            break;
        case Operator::Deref:
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
                } else if (assignmentTargetDepth_ == 0 &&
                           !isStringSemanticType(operandType)) {
                    reportUnsafeBoundary(node->location, "裸指针解引用读",
                                       "指针解引用（*p）");
                }
            } else {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'*'解引用要求指针操作数，实际为 '" + operandType + "'");
                lastType_ = "未知";
            }
            break;
        case Operator::Propagate:
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
            break;
        case Operator::Increment:
        case Operator::Decrement: {
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
                break;
            }
            if (ot == NodeType::IdentifierExpr) {
                const IdentifierExpr* ident =
                    static_cast<IdentifierExpr*>(node->operand.get());
                if (isConstVarName(ident->name)) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "不能对常量 '" + ident->name + "' 自增/自减（常量初始化后不可修改）");
                    lastType_ = operandType;
                    break;
                }
            }
            // 自增/自减：数值 或 指针（Task 2.4 指针 ++/-- 按元素大小步进）
            if (!isNumeric(operandType) && !isPointerType(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "自增/自减要求数值或指针操作数，实际为 '" +
                                    operandType + "'");
            }
            lastType_ = operandType;
            break;
        }
        default:
            lastType_ = operandType;
            break;
    }
}
void SemanticAnalyzer::visitTernaryExpr(TernaryExpr* node) {
    std::string condType = checkExpr(node->condition.get());
    checkCondition(condType, node->condition->location, "三元表达式");
    std::string trueType = checkExpr(node->trueValue.get());
    std::string falseType = checkExpr(node->falseValue.get());
    // 数组名退化（与二元运算一致）：数组类型作为值参与三元时退化为元素指针
    if (isArrayType(trueType)) trueType = types::arrayElemOf(trueType) + "*";
    if (isArrayType(falseType)) falseType = types::arrayElemOf(falseType) + "*";
    if (trueType == "未知" || falseType == "未知") {
        lastType_ = (trueType == "未知") ? falseType : trueType;
        return;
    }
    // 数值类型：宽化合并（整8/整32/浮32 等按 commonNumericType 提升）
    if (isNumeric(trueType) && isNumeric(falseType)) {
        lastType_ = commonNumericType(trueType, falseType);
        return;
    }
    // 非数值类型：两分支须完全一致（字符串/字符*/指针/结构体/枚举/布尔）
    if (trueType != falseType) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "三元表达式两个分支类型不一致：'" + trueType + "' 与 '" +
                            falseType + "'");
        lastType_ = trueType;
        return;
    }
    lastType_ = trueType;
}
// 缺陷②（2026-09-03 用户裁决立案）：赋值目标非左值的统一拒绝诊断。
//   附诊断③（顺带裁决）：赋值运算符行号大于目标起始行号 = 跨行赋值语句——
//   换行≡空格（01a §三）规范行为下行首运算符会并入上一行（如 b = a 换行
//   *a = 7 粘连为 b = ((a * a) = 7)），多为本意为两条语句的粘连形态，
//   按 rustc「you might have meant to write a semicolon here」同款附提示。
void SemanticAnalyzer::reportNonLvalueTarget(AssignmentExpr* node) {
    std::string kindName = "该表达式";
    switch (node->target->getType()) {
        case NodeType::BinaryExpr: kindName = "运算结果表达式"; break;
        case NodeType::IntegerLiteral:
        case NodeType::FloatLiteral:
        case NodeType::StringLiteral:
        case NodeType::CharLiteral:
        case NodeType::BoolLiteral:
        case NodeType::NullLiteral:
            kindName = "字面量";
            break;
        case NodeType::TernaryExpr: kindName = "三元条件表达式"; break;
        case NodeType::CastExpr: kindName = "类型转换结果"; break;
        case NodeType::AssignmentExpr: kindName = "嵌套赋值表达式"; break;
        case NodeType::UnaryExpr: kindName = "一元运算结果"; break;
        default: break;
    }
    std::string msg =
        "赋值目标必须是可赋值的左值（变量/成员/下标/解引用/引用返回调用），不能给" +
        kindName + "赋值";
    const int targetLine = node->target->location.getLine();
    if (targetLine > 0 && node->location.getLine() > targetLine) {
        msg += "\n  提示：赋值语句跨行——CN 换行等同空格，行首运算符会并入上一行；"
               "若本意为两条语句，请在上一行末尾加分号";
    }
    diagnostics_.report(DiagnosticLevel::Error, node->target->location, msg);
}
void SemanticAnalyzer::visitAssignmentExpr(AssignmentExpr* node) {
    // 常量成员函数检查（Task 3.9）：常量方法体内修改成员 -> 错误
    if (isConstMethodContext()) {
        // 赋值目标为 自身.字段 或 直接字段引用（类方法体内）
        if (node->target->getType() == NodeType::MemberExpr) {
            MemberExpr* mem = static_cast<MemberExpr*>(node->target.get());
            if (mem->object->getType() == NodeType::SelfExpr ||
                (!contextClassStack_.empty() &&
                 mem->object->getType() == NodeType::IdentifierExpr)) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "常量成员函数内不能修改成员 '" + mem->memberName + "'");
            }
        }
        if (node->target->getType() == NodeType::IdentifierExpr && !contextClassStack_.empty()) {
            // 直接字段赋值（无 自身. 前缀，如 值 = v）：方法体内标识符可能是字段
            // （字段已入方法作用域，故不能用 lookupVar 失败判断；直接查类字段表）
            const std::string& name =
                static_cast<IdentifierExpr*>(node->target.get())->name;
            const ClassInfo* cls = currentContextClass();
            if (cls != nullptr) {
                std::string owner;
                const ClassMemberInfo* member =
                    lookupClassMember(cls->name, name, owner);
                if (member != nullptr && !member->isStatic) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "常量成员函数内不能修改成员 '" + name + "'");
                }
            }
        }
    }
    // 检查左值——可写左值统一白名单（缺陷②根治，2026-09-03 用户裁决立案：
    //   非左值赋值静默接受 → 硬错误，Rust E0070 / C++ 赋值约束同款）。
    //   合法形态：标识符（非常量）/下标/解引用（*，Star 一元）/成员/引用返回调用；
    //   其余（二元运算结果/字面量/三元/强转/嵌套赋值/非 Star 一元等）显式拒绝。
    //   原 else 分支「其他左值形式（成员访问等）：后续Task实现」只查类型不查
    //   左值性——a + 1 = 7 静默通过、存储被丢弃（半实现形态），本轮收口。
    std::string targetType = "未知";
    bool lvalueOk = false;
    switch (node->target->getType()) {
        case NodeType::IdentifierExpr: {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
            // 150-a（plans/023 B10 实施）：可变静态变量写观察期警告（读安全——
            //   plans/023 §四 B10：Rust static mut 对照的 CN 裁剪）
            if (isGlobalStatic(ident->name)) {
                reportUnsafeBoundary(node->location, "静态变量写", "静态变量赋值");
            }
            std::string varType;
            if (lookupVar(ident->name, varType)) {
                targetType = varType;
                lvalueOk = true;
                // 缺陷②同族：常量初始化后不可修改（局部 常量 / 顶层常量）
                if (isConstVarName(ident->name)) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, ident->location,
                        "不能给常量 '" + ident->name + "' 赋值（常量初始化后不可修改）");
                    lvalueOk = false;
                }
                // plans/019 阶段3（2026-09-10）：常量引用参数是只读借用——不可
                //   作赋值目标（写=可变借用，与只读冲突）
                else if (currentConstRefParams_.count(ident->name) > 0) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, ident->location,
                        "常量引用参数 '" + ident->name +
                            "' 是只读借用，不能赋值");
                    lvalueOk = false;
                }
                // plans/019 阶段1（2026-09-10）：已转移变量不可作赋值目标
                //   （赋值=使用；转移后获得新值请使用新变量名）
                else if (reportMovedUse(ident->name, ident->location)) {
                    lvalueOk = false;
                }
            } else {
                diagnostics_.report(DiagnosticLevel::Error, ident->location,
                                    "赋值目标未声明：'" + ident->name + "'");
            }
            break;
        }
        case NodeType::IndexExpr: {
            // plans/019 阶段4：安全区边界观察期——指针下标写（p[i] = x 经裸
            //   指针偏移写=可越界）应在 不安全 函数 内（数组下标写=运行时越界
            //   检查保护，不在此列）
            if (node->target->getType() == NodeType::IndexExpr) {
                const Expr* iobj =
                    static_cast<IndexExpr*>(node->target.get())->object.get();
                if (iobj->getType() == NodeType::IdentifierExpr) {
                    std::string iot;
                    if (lookupVar(static_cast<const IdentifierExpr*>(iobj)->name, iot) &&
                        types::isPointer(iot)) {
                        reportUnsafeBoundary(node->location, "指针下标写",
                            static_cast<const IdentifierExpr*>(iobj)->name + "[i] = ...");
                    }
                }
            }
            [[fallthrough]];
        }
        case NodeType::MemberExpr: {
            // 下标访问（数组[i]）/成员访问（对象.字段）均为可写左值；
            // plans/019 阶段3：对象为常量引用参数（只读借用经成员链写=写借用
            //   对象）——拒绝
            if (node->target->getType() == NodeType::MemberExpr) {
                const Expr* obj =
                    static_cast<MemberExpr*>(node->target.get())->object.get();
                if (obj->getType() == NodeType::IdentifierExpr &&
                    currentConstRefParams_.count(
                        static_cast<const IdentifierExpr*>(obj)->name) > 0) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->target->location,
                        "常量引用参数 '" +
                            static_cast<const IdentifierExpr*>(obj)->name +
                            "' 是只读借用，不能经成员访问赋值");
                }
            }
            assignmentTargetDepth_++;  // 150-a：抑制 target 求值中的 B6/B8/B9 读判
            targetType = checkExpr(node->target.get());
            assignmentTargetDepth_--;
            lvalueOk = true;
            break;
        }
        case NodeType::UnaryExpr: {
            // 解引用（*p，Deref）为可写左值；其余一元结果（负号/逻辑非/取地址）
            // 不可赋值——原实现整类放行为缺陷②形态
            UnaryExpr* u = static_cast<UnaryExpr*>(node->target.get());
            if (u->op == Operator::Deref) {
                // 150-a（plans/023 B7 实施）：解引用写观察期警告（B6 在 Deref
                //   分支被 assignmentTargetDepth_ 抑制——同一形态单报）。
                assignmentTargetDepth_++;
                targetType = checkExpr(node->target.get());
                assignmentTargetDepth_--;
                if (!isStringSemanticType(targetType)) {
                    // B7-a（plans/023 待裁决子项）：字符* 解引用写暂不豁免（默认）。
                    //   注：targetType=所指元素类型——用操作数（指针）类型判定语义族。
                    const std::string uPtr = checkExpr(u->operand.get());
                    if (!isStringSemanticType(uPtr)) {
                        reportUnsafeBoundary(node->location, "裸指针解引用写",
                                           "指针解引用写（*p = x）");
                    }
                }
                lvalueOk = true;
            } else {
                reportNonLvalueTarget(node);
            }
            break;
        }
        case NodeType::CallExpr: {
            // P3-18 补完：引用返回调用可作赋值目标（获取() = 值 写回被引用对象）
            lastExprIsRefReturn_ = false;
            targetType = checkExpr(node->target.get());
            if (lastExprIsRefReturn_) {
                lvalueOk = true;
            } else {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "赋值目标须为可写左值（标识符/下标/解引用/成员/引用返回调用）");
                targetType = "未知";
            }
            break;
        }
        default:
            // 缺陷②：二元运算结果/字面量/三元/强转/嵌套赋值等均非可写左值
            reportNonLvalueTarget(node);
            break;
    }
    if (!lvalueOk) {
        // 左值性已失败：右值仍检查（级联诊断更完整），类型按未知处理并提前返回
        checkExpr(node->value.get());
        lastType_ = "未知";
        return;
    }

    // 检查右值
    std::string valueType = checkExpr(node->value.get());

    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：赋值位借出绑定
    //   登记（借出视图转入既有变量：s = 表.元素(0)——容器内句柄浅拷；重新绑定
    //   时更新活跃区间起点=新借用取代旧借用，NLL 语义同款）。
    if (lastExprIsBorrowView_ && node->value.get() == lastBorrowCallNode_ &&
        node->target->getType() == NodeType::IdentifierExpr) {
        registerBorrowView(
            static_cast<IdentifierExpr*>(node->target.get())->name,
            node->location);
        lastExprIsBorrowView_ = false;
    }

    // plans/019 阶段4' A2（2026-09-11 方案甲）：赋值位字符* 借用收紧——
    //   字符串（拥有）变量赋值 字符* 借用视图须显式 字符串复制(...)（与
    //   初始化位 A1/A2 同款；拥有→借用自动、借用→拥有显式）。赋值给
    //   字符* 目标=纯借用视图存储，不受限。
    if (targetType == "字符串" && valueType == "字符*") {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "字符串（拥有）变量不能以字符* 借用视图隐式赋值——须 字符串复制(...) "
            "显式落堆拥有化（借用→拥有显式；拥有→借用自动）");
        return;
    }

    // plans/019 阶段4' A2 补全：赋值位借出装入拒绝（A1 同款收紧到赋值位）——
    //   「字符串 变量=恒拥有槽」不变量（Rust String 槽恒拥有/&str 承担借用）。
    //   原赋值位借出靠 IR 污染（不 free 保安全）——但污染变量可经 返回 位
    //   移出（语义层无污染状态）→调用方按返回类型登记 free 借用指针=悬垂
    //   （v2p 全解析瘫痪实测：free 损坏驻留/静态区）。收紧后赋值位借出
    //   一律显式拥有化，污染路径成为不可达防御。**泛型单态化体内豁免**：
    //   实例化类方法体（genericTypeParams_ 非空）的 T 元素搬移（容器拷贝/
    //   移位的 数据[i]=其他.数据[i]）=容器内部存储管理（析构元素 特判
    //   释放），Rust Vec 内部 ptr::write 同类。
    if (targetType == "字符串" && genericTypeParams_.empty()) {
        const NodeType valKindA2 = node->value->getType();
        bool borrowAssignA2 = valKindA2 == NodeType::IndexExpr ||
                              valKindA2 == NodeType::MemberExpr;
        if (valKindA2 == NodeType::UnaryExpr &&
            static_cast<const UnaryExpr*>(node->value.get())->op ==
                Operator::Deref) {
            borrowAssignA2 = true;
        }
        if (borrowAssignA2) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "字符串（拥有）变量不能以下标/成员/解引用借出隐式赋值——须 "
                "字符串复制(...) 显式落堆拥有化（字符* 借用视图另用 字符* 变量）");
            return;
        }
    }

    // 复合赋值：+= -= 等要求数值
    if (isCompoundAssign(node->op)) {
        // plans/019 阶段3 扩展（A21，第七十七轮）：复合赋值读旧值=使用借出视图
        //   （活跃区间右端；字符串 += 拼接重析为二元同覆盖）
        if (node->target->getType() == NodeType::IdentifierExpr) {
            noteBorrowViewUse(
                static_cast<IdentifierExpr*>(node->target.get())->name,
                node->location);
        }
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

    // 简单赋值 =：要求右值可隐式转换为左值类型（55-c 方案A：字面量豁免走
    //   canConvertWithLiteral——跨符号拒绝后 `正32 a; a = 5` 等字面量赋值保留）
    if (targetType != "未知" && valueType != "未知" &&
        !canConvertWithLiteral(node->value.get(), valueType, targetType)) {
        if (!reportMixedSignAssign(node->value.get(), valueType, targetType,
                                   node->location)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "无法将 '" + valueType + "' 隐式转换为 '" + targetType + "'");
        }
    }
    // 方案A 强制规则（2026-08-25）：类对象赋值拷贝（乙 = 甲，两者为类变量）——
    //   有析构类须有拷贝构造（函数 类名(类名& 其他) 深拷贝），否则浅拷贝裸指针
    //   字段析构双释放 0xC0000374。引用目标（类& 乙 = 甲 后 乙 = 丙）为指针
    //   写回非拷贝，跳过。
    if (targetType != "未知" && valueType != "未知" &&
        !types::isReference(targetType) &&
        isClassType(types::canonical(targetType)) &&
        isClassType(types::canonical(valueType))) {
        checkCopyRequiresCtor(types::canonical(targetType), node->location);
    }
    lastType_ = targetType == "未知" ? valueType : targetType;
    // plans/019 阶段2（2026-09-10）：局部地址逃逸检查——右值求值为当前函数
    //   局部的地址（&局部 / 引用局部绑局部）而赋值目标是比其寿命长的存储
    //   （静态/全局变量、静态/全局对象的字段或元素）时编译期拒绝（悬垂防线
    //   前移）。局部指针/局部对象字段/局部数组元素接收局部地址合法（随所在
    //   作用域消亡）；局部指针指向登记供返回检查（直接 &局部 形态）。
    {
        std::string escBase;
        if (isLocalAddressValue(node->value.get(), escBase)) {
            bool escapes = false;
            std::string dstDesc;
            switch (node->target->getType()) {
                case NodeType::IdentifierExpr: {
                    const std::string& name =
                        static_cast<IdentifierExpr*>(node->target.get())->name;
                    std::string vt;
                    if (lookupVar(name, vt) && !isCurrentFnLocal(name)) {
                        escapes = true;
                        dstDesc = name;
                    }
                    break;
                }
                case NodeType::MemberExpr:
                case NodeType::IndexExpr: {
                    // 自行解剖对象链基础名（refReturnLvalueBase 对指针解引用
                    // 访问短路返回空=放行；静态指针 p.f 的存储=静态指向对象，
                    // 须按 p 判定逃逸）
                    std::string objBase;
                    const Expr* obj =
                        node->target->getType() == NodeType::MemberExpr
                            ? static_cast<MemberExpr*>(node->target.get())->object.get()
                            : static_cast<IndexExpr*>(node->target.get())->object.get();
                    while (obj != nullptr &&
                           (obj->getType() == NodeType::MemberExpr ||
                            obj->getType() == NodeType::IndexExpr)) {
                        obj = obj->getType() == NodeType::MemberExpr
                                  ? static_cast<const MemberExpr*>(obj)->object.get()
                                  : static_cast<const IndexExpr*>(obj)->object.get();
                    }
                    if (obj != nullptr &&
                        obj->getType() == NodeType::IdentifierExpr) {
                        objBase = static_cast<const IdentifierExpr*>(obj)->name;
                    }
                    std::string objVt;
                    if (!objBase.empty() && lookupVar(objBase, objVt) &&
                        !isCurrentFnLocal(objBase)) {
                        escapes = true;
                        dstDesc = objBase + " 的字段/元素";
                    }
                    break;
                }
                default:
                    break;
            }
            if (escapes) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "不能将局部变量 '" + escBase + "' 的地址存入静态/全局存储（'" +
                        dstDesc + "' 比其寿命长，将成悬垂）");
            }
        }
        // 指向登记：局部指针 = &局部（直接形态）；指针间传递（p = q）不跟踪
        // = 诚实边界（返回检查只认直接赋值登记的指向）
        if (node->target->getType() == NodeType::IdentifierExpr) {
            const std::string& name =
                static_cast<IdentifierExpr*>(node->target.get())->name;
            std::string vt;
            if (isCurrentFnLocal(name) && lookupVar(name, vt) &&
                types::isPointer(vt) &&
                node->value->getType() == NodeType::UnaryExpr &&
                static_cast<UnaryExpr*>(node->value.get())->op ==
                    Operator::AddressOf) {
                std::string bn;
                if (refReturnLvalueBase(
                        static_cast<UnaryExpr*>(node->value.get())->operand.get(),
                        bn) &&
                    !bn.empty()) {
                    ptrLocalPointees_[name] = bn;
                }
            }
        }
    }
}
} // namespace cn_compiler
