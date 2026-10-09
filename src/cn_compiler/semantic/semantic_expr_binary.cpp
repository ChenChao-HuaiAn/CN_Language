// CN 语义分析器——二元运算表达式检查（350 重构E2 自 semantic_expr_op.cpp 纯机械搬移）
// 职责（函数体逐字搬移·零语义变化；声明仍在 semantic.hpp）：
//   visitBinaryExpr 分发器 + 族方法（逻辑/混合符号与安全区/空类型/编译期常量除零溢出/比较/位运算/算术（含串拼接与指针算术）/运算符重载）。
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

void SemanticAnalyzer::visitBinaryExpr(BinaryExpr* node) {
    std::string leftType = checkExpr(node->left.get());
    std::string rightType = checkExpr(node->right.get());
    // 数组名退化（C语义，Task 2.7 集成修复）：数组类型作为值参与运算时
    // 退化为指向首元素的指针（整32[5] -> 整32*；学生[5] -> 学生*），
    // 使 名单 + 人数（指针算术）与 指针比较 等组合可用
    if (isArrayType(leftType)) leftType = types::arrayElemOf(leftType) + "*";
    if (isArrayType(rightType)) rightType = types::arrayElemOf(rightType) + "*";

    // 350 重构E2：以下按原单函数逐节顺序分派（各节见同名族方法·零重排）
    if (checkLogicalBinary(node, leftType, rightType)) return;
    reportMixedSignAndUnsafeBoundary(node, leftType, rightType);
    if (checkVoidOperandBinary(node, leftType, rightType)) return;
    checkConstIntExprSafety(node, leftType, rightType);
    if (checkComparisonBinary(node, leftType, rightType)) return;
    if (checkBitwiseBinary(node, leftType, rightType)) return;
    if (checkArithmeticBinary(node, leftType, rightType)) return;
    if (checkClassOperatorOverload(node, leftType, rightType)) return;
    // 其他运算符（阶段一不支持，回退左操作数类型）
    lastType_ = leftType;
}

// 族①（原 visitBinaryExpr 逻辑运算节）：逻辑运算操作数必须为布尔，结果为布尔。
// true = 已处理。
bool SemanticAnalyzer::checkLogicalBinary(BinaryExpr* node, const std::string& leftType,
                                          const std::string& rightType) {
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
        return true;
    }
    return false;
}

// 族②（原 2026-09-10 方案A 混合符号拒绝 + 019 阶段4 安全区边界节）：
//   仅报告不短路（后续各节继续分派）。
void SemanticAnalyzer::reportMixedSignAndUnsafeBoundary(BinaryExpr* node,
                                                        const std::string& leftType,
                                                        const std::string& rightType) {
    // 混合符号二元运算拒绝（2026-09-10 方案A·Rust 对齐）：有符号与无符号的
    //   「变量间」比较/算术/位运算编译期拒绝——隐式宽化静默改变值语义（mixed_cmp
    //   探针：正64(2^63) vs 整64(-1) 值域反转·双侧分叉）。须显式 类型名(表达式)
    //   构造转换。字面量豁免：一侧为整数字面量按另一侧类型参与（E2E 191 锚）；
    //   同符号混合宽度维持既有宽化。规范 plans/001 §3.7。
    // plans/019 阶段4：安全区边界——指针算术（± 整数产生新指针=可越界）应在 不安全 函数 内
    // plans/022 波 1：字符串拼接不是指针算术——+ 涉及字符串语义类型（字符串/
    //   字符*·IR 层同为 ptr）时算术分支按拼接分派；原按 '*' 后缀判指针误报
    //   （v2 树 75 处实证全为拼接）。字符* 的 -（步进）与真指针 +/- 保持原判。
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
}

// 族③（原 320-a T42 空类型节）：空类型操作数禁止参与二元运算——
//   空类型调用「无返回值却有值」（原 commonNumericType 兜底整32 静默
//   产出垃圾值：5+无返回() 出 15 实锤）；赋值/传参/返回面由 canConvertType
//   空类型单点拒绝覆盖。true = 已处理。
bool SemanticAnalyzer::checkVoidOperandBinary(BinaryExpr* node, const std::string& leftType,
                                              const std::string& rightType) {
    if ((isComparisonOp(node->op) || isArithmeticOp(node->op) || isBitwiseOp(node->op)) &&
        (leftType == "空类型" || rightType == "空类型")) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            std::string("空类型（无返回值）不能参与运算：") +
                            (leftType == "空类型" ? "左" : "右") +
                            " 操作数是空类型调用结果（Rust () 同款——无返回值"
                            "却有值违反类型安全；如需值请改函数返回类型）");
        lastType_ = "未知";
        return true;
    }
    return false;
}

// 族④（原 319-a C18①+T8 编译期整型常量求值安全检查节·用户批量裁决方案甲）：
//   两侧均为字面量（含一元负号字面量）时按结果类型域预演：
//   ①÷/%：常量除零编译期硬错误（T8：原 10/0 编过、运行期才报）；
//   ②+/-/*：常量溢出编译期硬错误（原 2147483647+1 静默回绕——Rust debug
//     溢出检查同款严格面；变量面回绕语义由 B12 运行时通道承载）。
//   比较类不涉溢出；无符号/整128 结果域宽于 int64 预演承载，跳过（不误报）。
void SemanticAnalyzer::checkConstIntExprSafety(BinaryExpr* node, const std::string& leftType,
                                               const std::string& rightType) {
    if (isArithmeticOp(node->op) && isInteger(leftType) && isInteger(rightType) &&
        leftType != "未知" && rightType != "未知" &&
        isIntLiteralExpr(node->left.get()) && isIntLiteralExpr(node->right.get())) {
        auto litValue = [](const Expr* e, bool& ok) -> std::int64_t {
            ok = true;
            if (e->getType() == NodeType::IntegerLiteral) {
                return static_cast<const IntegerLiteral*>(e)->value;
            }
            if (e->getType() == NodeType::UnaryExpr) {
                const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
                if (u->op == Operator::Subtract && !u->postfix &&
                    u->operand != nullptr &&
                    u->operand->getType() == NodeType::IntegerLiteral) {
                    const std::int64_t v =
                        static_cast<const IntegerLiteral*>(u->operand.get())->value;
                    // 取负按两补码回绕位模式（INT64_MIN 取负=自身）
                    return static_cast<std::int64_t>(
                        ~static_cast<std::uint64_t>(v) + 1);
                }
            }
            ok = false;
            return 0;
        };
        bool okL = false, okR = false;
        const std::int64_t vL = litValue(node->left.get(), okL);
        const std::int64_t vR = litValue(node->right.get(), okR);
        if (okL && okR) {
            reportConstOverflowOrDivZero(node, vL, vR, leftType, rightType);
        }
    }
}

// 族④ 子方法（原 if (okL && okR) 预演体）：结果域提升 + 除零/溢出编译期硬错误。
void SemanticAnalyzer::reportConstOverflowOrDivZero(BinaryExpr* node, std::int64_t vL,
                                                    std::int64_t vR,
                                                    const std::string& leftType,
                                                    const std::string& rightType) {
            // 预演结果域：字面量操作数按后缀+值提升（对齐 visitIntegerLiteral
            // 的提升链——语义层 lastType/literalTypeOf 对无后缀字面量恒整32，
            // 若按其取域会把 int64 值字面量运算误判超整32 域〔420 用例实锤〕）；
            // 提升至整128（超 int64 无后缀）时域宽于 int64 预演承载，跳过。
            auto promotedType = [](const Expr* e) -> std::string {
                if (e->getType() != NodeType::IntegerLiteral) return "";
                const IntegerLiteral* lit =
                    static_cast<const IntegerLiteral*>(e);
                const std::string lt = types::literalTypeOf(lit->raw, false);
                if (lt != "整32") return lt;  // 带后缀/其他：按后缀类型
                // 无后缀：按值提升（超 int32 -> 整64；超 int64 -> 整128）
                if (types::textExceedsInt64(types::stripLiteralSuffix(lit->raw))) {
                    return "整128";
                }
                if (lit->value > 2147483647LL) return "整64";
                return "整32";
            };
            const std::string pL = promotedType(node->left.get());
            const std::string pR = promotedType(node->right.get());
            const std::string effL = (!pL.empty() && pL != "整32") ? pL : types::canonical(leftType);
            const std::string effR = (!pR.empty() && pR != "整32") ? pR : types::canonical(rightType);
            const std::string rt = types::canonical(
                types::commonNumericType(effL, effR));
            auto rangeOf = [](const std::string& t, std::int64_t& lo,
                              std::int64_t& hi) {
                if (t == "整8") { lo = -128; hi = 127; return true; }
                if (t == "整16") { lo = -32768; hi = 32767; return true; }
                if (t == "整32" || t == "整数") { lo = -2147483648LL; hi = 2147483647LL; return true; }
                if (t == "整64") { lo = (-9223372036854775807LL - 1); hi = 9223372036854775807LL; return true; }
                return false;
            };
            std::int64_t lo = 0, hi = 0;
            const bool hasRange = rangeOf(rt, lo, hi);
            if ((node->op == Operator::Divide || node->op == Operator::Modulo) &&
                vR == 0) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "编译期除零：常量表达式除数为零（运行期除零已有"
                                    "错误码防护，常量折叠面编译期硬错误）");
            } else if (hasRange && node->op != Operator::Divide &&
                       node->op != Operator::Modulo) {
                bool overflow = false;
                switch (node->op) {
                    case Operator::Add:
                        overflow = (vR > 0 && vL > hi - vR) ||
                                   (vR < 0 && vL < lo - vR);
                        break;
                    case Operator::Subtract:
                        overflow = (vR < 0 && vL > hi + vR) ||
                                   (vR > 0 && vL < lo + vR);
                        break;
                    case Operator::Multiply: {
                        auto absOf = [](std::int64_t v, std::uint64_t& out) {
                            out = (v < 0)
                                ? static_cast<std::uint64_t>(
                                      ~static_cast<std::uint64_t>(v)) + 1
                                : static_cast<std::uint64_t>(v);
                        };
                        std::uint64_t magL = 0, magR = 0;
                        absOf(vL, magL);
                        absOf(vR, magR);
                        const bool negResult = (vL < 0) != (vR < 0);
                        const std::uint64_t magHi = negResult
                            ? static_cast<std::uint64_t>(-(lo + 1)) + 1  // |lo|
                            : static_cast<std::uint64_t>(hi);
                        overflow = (magL != 0 && magR != 0) &&
                                   (magL > magHi / magR ||
                                    (magL * magR) > magHi);
                        break;
                    }
                    default:
                        break;
                }
                if (overflow) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "编译期整数溢出：常量表达式结果超出类型 '" +
                                        rt + "' 范围（Rust debug 溢出检查对齐；"
                                        "变量面回绕语义由运行时通道承载）");
                }
            }
}

// 族⑤（原 visitBinaryExpr 比较运算节）：要求可互相转换的同类操作数，结果为布尔。
// true = 已处理。
bool SemanticAnalyzer::checkComparisonBinary(BinaryExpr* node, const std::string& leftType,
                                             const std::string& rightType) {
    if (isComparisonOp(node->op)) {
        // 比较运算：要求可互相转换的同类操作数，结果为布尔
        if (leftType == "未知" || rightType == "未知") {
            lastType_ = "布尔";
            return true;
        }
        // 962（166 立法·2026-10-02 用户裁）：字符串×字符串 比较放行为内容/
        //   字典序比较（Rust/Go/Python/C++ 主流对齐·001 §比较语义修订版）——
        //   旧「显式拒绝」废止（与 019「默认路径零规则」冲突）。IR 层按操作数
        //   类型分派 helper。字符*×字符* 维持真指针拒绝（地址语义留 不安全 域）；
        //   与 空类型* 判空比较保留（Task 6.2）。
        if (leftType == "字符串" && rightType == "字符串") {
            lastType_ = "布尔";
            return true;
        }
        const bool leftStrOnly = (leftType == "字符串" || leftType == "字符*");
        const bool rightStrOnly = (rightType == "字符串" || rightType == "字符*");
        if (leftStrOnly && rightStrOnly) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "字符指针之间不支持 ==/!=/</> 比较（地址语义收 不安全 域·"
                                "字符串值比较请用 字符串 类型）");
            lastType_ = "布尔";
            return true;
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
            return true;
        }
        // 指针间（含字符串）比较按地址，无需类型转换检查（字符串 vs 空类型*
        //   均以 ptr 表示，地址比较合法）
        if (leftPtr && rightPtr) {
            lastType_ = "布尔";
            return true;
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
        return true;
    }
    return false;
}

// 族⑥（原 visitBinaryExpr 位运算节，Task 2.3）：要求整数操作数，结果为两操作数
//   公共整数类型（整型取秩高者；整32 & 整64 -> 整64，与算术推导一致）。
// true = 已处理。
bool SemanticAnalyzer::checkBitwiseBinary(BinaryExpr* node, const std::string& leftType,
                                          const std::string& rightType) {
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
        return true;
    }
    return false;
}

// 族⑦（原 visitBinaryExpr 算术运算节）：+ - * / % 数值操作数；指针算术（Task 2.4）；
//   字符串连接（Task 2.5）；类运算符重载（Task 3.7）；窄算术保持窄域（285）。
//   true = 已处理（isArithmeticOp 命中后全部路径以本方法收尾）。
bool SemanticAnalyzer::checkArithmeticBinary(BinaryExpr* node, std::string& leftType,
                                             std::string& rightType) {
    if (!isArithmeticOp(node->op)) return false;
    // 算术运算（+ - * / %）：要求数值操作数；指针算术（Task 2.4）；字符串连接（Task 2.5）
    if (checkStringConcatAndPtrArithmetic(node, leftType, rightType)) return true;
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
                    return true;
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
        // 285（001 §3.7a 窄算术保持窄域·甲案·p1007_05）：字面量按另一侧窄类型
        //   参与（§3.7 推导面落实·v2 零改动锚）——原 a+1 字面量按整32 参与→提升
        //   整32→赋回整8 拒；现另一侧窄整数时结果保持窄域（回绕由发射层窄槽存回
        //   截断承载·227→-29 实证）·宽侧参与/混宽宽化维持既有链零改动。
        if (isInteger(leftType) && isInteger(rightType)) {
            const auto isNarrow280 = [](const std::string& t) {
                return t == "整8" || t == "整16" || t == "正8" || t == "正16";
            };
            if (isNarrow280(rightType) && !isNarrow280(leftType) &&
                isIntLiteralExpr(node->left.get())) {
                leftType = rightType;
            } else if (isNarrow280(leftType) && !isNarrow280(rightType) &&
                       isIntLiteralExpr(node->right.get())) {
                rightType = leftType;
            }
        }
        lastType_ = commonNumericType(leftType, rightType);
        return true;
}

// 族⑦ 子方法（原算术节前半）：字符串连接（Task 2.5）+ 字符串+数值 隐式拼接
//   （Task 2.9）+ 指针算术（Task 2.4）。true = 已处理。
bool SemanticAnalyzer::checkStringConcatAndPtrArithmetic(BinaryExpr* node,
                                                         const std::string& leftType,
                                                         const std::string& rightType) {
    // ---- 字符串连接（Task 2.5）：两个字符串/字符* 的 + -> 连接，结果为字符串 ----
    // 说明：字符串与字符* 在 IR 层均为 ptr；语义层需区分"字符串连接"与"指针算术"。
    //       字符串类型（字符串/字符*）的 + 视为连接（字符* 也承载字符串语义）；
    //       判定与顶部安全区边界检查共用 isStringSemanticType（plans/022 波 1）。
    const bool leftStr = isStringSemanticType(leftType);
    const bool rightStr = isStringSemanticType(rightType);
    if (node->op == Operator::Add && leftStr && rightStr) {
        lastType_ = "字符串";  // 连接结果为字符串
        return true;
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
        return true;
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
        return true;
    }
    if (!leftPtr && rightPtr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "整数不能与指针做算术运算（仅支持 指针 ± 整数）");
        lastType_ = rightType;
        return true;
    }
    if (leftPtr && rightPtr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "两个指针不能做算术运算");
        lastType_ = leftType;
        return true;
    }
    return false;
}

// 族⑧（原 visitBinaryExpr 运算符重载决议节·阶段3 Task 3.7 规格书01b）：
//   重载决议顺序：① 优先内置运算符（上方已处理）；② 无内置匹配时按左操作数
//   类型查成员 运算符X；③ 无匹配报"类型不兼容"（由上方报错）。
//   此处拦截：左操作数为类类型（非内置可处理）时查成员运算符。
//   true = 已处理（opSym 为空或左操作数非类类型=false，交回兜底）。
bool SemanticAnalyzer::checkClassOperatorOverload(BinaryExpr* node, const std::string& leftType,
                                                  const std::string& rightType) {
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
                return true;
            }
            // 无重载匹配且非内置：报"类型不兼容"
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "运算符 '" + opSym + "' 与类型 '" + leftType +
                                    "' 不兼容（无内置匹配且类无对应运算符重载）");
            lastType_ = "未知";
            return true;
        }
    }
    return false;
}

} // namespace cn_compiler