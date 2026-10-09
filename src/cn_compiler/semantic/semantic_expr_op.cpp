// CN 语义分析器——表达式检查（D1 行数整改 116-a：自 semantic_expr.cpp 按族拆出）
//   族 = 运算符/赋值表达式检查（visitBinaryExpr + visitUnaryExpr + visitTernaryExpr + reportNonLvalueTarget + visitAssignmentExpr）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp）。
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

    // 320-a（T42·方案甲·Rust () 同款）：空类型操作数禁止参与二元运算——
    //   空类型调用「无返回值却有值」（原 commonNumericType 兜底整32 静默
    //   产出垃圾值：5+无返回() 出 15 实锤）；赋值/传参/返回面由 canConvertType
    //   空类型单点拒绝覆盖。
    if ((isComparisonOp(node->op) || isArithmeticOp(node->op) || isBitwiseOp(node->op)) &&
        (leftType == "空类型" || rightType == "空类型")) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            std::string("空类型（无返回值）不能参与运算：") +
                            (leftType == "空类型" ? "左" : "右") +
                            " 操作数是空类型调用结果（Rust () 同款——无返回值"
                            "却有值违反类型安全；如需值请改函数返回类型）");
        lastType_ = "未知";
        return;
    }

    // 319-a（C18①+T8·用户批量裁决方案甲）：编译期整型常量求值安全检查——
    //   两侧均为字面量（含一元负号字面量）时按结果类型域预演：
    //   ①÷/%：常量除零编译期硬错误（T8：原 10/0 编过、运行期才报）；
    //   ②+/-/*：常量溢出编译期硬错误（原 2147483647+1 静默回绕——Rust debug
    //     溢出检查同款严格面；变量面回绕语义由 B12 运行时通道承载）。
    //   比较类不涉溢出；无符号/整128 结果域宽于 int64 预演承载，跳过（不误报）。
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
    }

    if (isComparisonOp(node->op)) {
        // 比较运算：要求可互相转换的同类操作数，结果为布尔
        if (leftType == "未知" || rightType == "未知") {
            lastType_ = "布尔";
            return;
        }
        // 962（166 立法·2026-10-02 用户裁）：字符串×字符串 比较放行为内容/
        //   字典序比较（Rust/Go/Python/C++ 主流对齐·001 §比较语义修订版）——
        //   旧「显式拒绝」废止（与 019「默认路径零规则」冲突）。IR 层按操作数
        //   类型分派 helper。字符*×字符* 维持真指针拒绝（地址语义留 不安全 域）；
        //   与 空类型* 判空比较保留（Task 6.2）。
        if (leftType == "字符串" && rightType == "字符串") {
            lastType_ = "布尔";
            return;
        }
        const bool leftStrOnly = (leftType == "字符串" || leftType == "字符*");
        const bool rightStrOnly = (rightType == "字符串" || rightType == "字符*");
        if (leftStrOnly && rightStrOnly) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "字符指针之间不支持 ==/!=/</> 比较（地址语义收 不安全 域·"
                                "字符串值比较请用 字符串 类型）");
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
    // 172-a→175-a：本函数 349 行按「左值白名单 / 借出视图 / 复合赋值 / 局部
    //   地址逃逸」两层提取为 7 个族子方法（纯搬运零行为变更——逐行核验）。
    checkConstMethodMemberAssign(node);
    // 检查左值——可写左值统一白名单（缺陷②根治，2026-09-03 用户裁决立案：
    //   非左值赋值静默接受 → 硬错误，Rust E0070 / C++ 赋值约束同款）。
    //   合法形态：标识符（非常量）/下标/解引用（*，Star 一元）/成员/引用返回调用；
    //   其余（二元运算结果/字面量/三元/强转/嵌套赋值/非 Star 一元等）显式拒绝。
    bool lvalueOk = false;
    std::string targetType = checkAssignLvalueTarget(node, lvalueOk);
    if (!lvalueOk) {
        // 左值性已失败：右值仍检查（级联诊断更完整），类型按未知处理并提前返回
        checkExpr(node->value.get());
        lastType_ = "未知";
        return;
    }

    // 检查右值
    // 574-a：构造器目标上下文压栈（r = 正常(...) 赋值位推断按目标 T/E）
    if (!targetType.empty() && targetType != "未知") {
        ctorTargetStack_.push_back(targetType);
    }
    inAggregateReadCtx_ = true;  // 010（def-init）：右值=聚合值读语境（数组整体赋值右值）
    std::string valueType = checkExpr(node->value.get());
    inAggregateReadCtx_ = false;
    if (!targetType.empty() && targetType != "未知") {
        ctorTargetStack_.pop_back();
    }

    // 借出视图登记 + 字符* 借用收紧（A21/A2 族）：true = 已诊断并终止
    if (checkBorrowViewAssign(node, targetType, valueType)) return;

    // 复合赋值：+= -= 等要求数值：true = 已处理并终止
    if (checkCompoundAssign(node, targetType, valueType)) return;

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
    // 任务 094（008 波 4）：赋值位污染登记＋出参移交源禁用——与 IR 层字符串
    //   赋值三路径＋下标污染逐形态同构（identifierString{ByRef,Transfer,Own}
    //   Assign+markIndexStringElemTainted·注释互指）。消费点=装箱 move 判定
    //   （污染源=借用装箱 IR 不清零→语义不 markMovedVar·q1/q3 实证）。
    if (!isCompoundAssign(node->op)) {
        // ①出参移交（IR byRef 路径·72-a）：目标=字符串引用（T&）·右值=标识符
        //   →IR 句柄直写调用方槽＋清零源槽（真 move）——语义层 markMovedVar
        //   挂既有已转移拦截（探针 p5c 实锤：移交后源静默读空串）。判据对齐：
        //   目标非污染/源非污染（污染移交=借用句柄不清零）＋排除类字段。
        if (node->target->getType() == NodeType::IdentifierExpr &&
            node->value->getType() == NodeType::IdentifierExpr) {
            const std::string& tgtName =
                static_cast<const IdentifierExpr*>(node->target.get())->name;
            const std::string& srcName =
                static_cast<const IdentifierExpr*>(node->value.get())->name;
            // 引用判定用 currentRefParams_（符号表类型经 canonicalType 已剥
            //   "&"——插桩实证 [dbg886] 输出形参类型=[字符串]·isRef=0；
            //   currentRefParams_ 由 checkFunctionBody 按 param->typeName 原文
            //   isReference 收集=准确引用形参名集，与 IR 层 isByRefCapture 同源）。
            std::string tgtSrcType;
            if (currentRefParams_.count(tgtName) > 0 &&
                lookupVar(tgtName, tgtSrcType) &&
                types::canonical(tgtSrcType) == "字符串" &&
                !isStrTainted(srcName) && !isCurrentClassFieldName(srcName)) {
                markMovedVar(srcName, node->location.getLine());
            }
        }
        // ②转移污染传播（IR identifierStringTransferAssign 723）：甲 = 转移(乙)
        //   且乙为借用污染 → 移交的是借用句柄 → 甲同污染（登记）。源禁用已由
        //   转移调用通用路径 markMovedVar 覆盖（semantic_call.cpp）。
        if (node->target->getType() == NodeType::IdentifierExpr &&
            node->value->getType() == NodeType::CallExpr) {
            const CallExpr* tr =
                static_cast<const CallExpr*>(node->value.get());
            if (isTransferCall(tr) && !tr->arguments.empty() &&
                tr->arguments[0]->getType() == NodeType::IdentifierExpr) {
                const std::string& srcName =
                    static_cast<const IdentifierExpr*>(tr->arguments[0].get())
                        ->name;
                const std::string& tgtName =
                    static_cast<const IdentifierExpr*>(node->target.get())->name;
                if (isStrTainted(srcName)) markStrTainted(tgtName);
            }
        }
        // ③下标元素写浅存（IR markIndexStringElemTainted 同构）：串数组[i] = s
        //   →右值句柄被外部槽持有 → s 污染退出 RAII（q3 实证：污染后装箱源
        //   仍可用=合法形态）。
        if (node->target->getType() == NodeType::IndexExpr &&
            node->value->getType() == NodeType::IdentifierExpr) {
            const IndexExpr* idx =
                static_cast<const IndexExpr*>(node->target.get());
            if (idx->object->getType() == NodeType::IdentifierExpr) {
                const std::string& objName =
                    static_cast<const IdentifierExpr*>(idx->object.get())->name;
                std::string objType;
                if (lookupVar(objName, objType)) {
                    std::string elemType;
                    if (types::isArray(objType)) elemType = types::arrayElemOf(objType);
                    else if (types::isPointer(objType)) elemType = types::pointeeOf(objType);
                    if (types::canonical(elemType) == "字符串") {
                        markStrTainted(
                            static_cast<const IdentifierExpr*>(node->value.get())
                                ->name);
                    }
                }
            }
        }
        // ④一般赋值非拥有来源污染（IR identifierStringOwnAssign else 分支）：
        //   目标=字符串局部·右值非三拥有形态（字面量/标识符/ownRet∪白名单调用）
        //   →markStrTainted。右值=转移调用已由②处理（IR transfer 分支先行
        //   return·不走 ownAssign）·成员/下标/解引用借出赋值已被
        //   checkBorrowViewAssign 拒绝（q1 实证·IR 不可达此点）。
        if (node->target->getType() == NodeType::IdentifierExpr &&
            !(node->value->getType() == NodeType::CallExpr &&
              isTransferCall(static_cast<const CallExpr*>(node->value.get())))) {
            const std::string& tgtName =
                static_cast<const IdentifierExpr*>(node->target.get())->name;
            std::string tgtSrcType;
            if (lookupVar(tgtName, tgtSrcType) &&
                types::canonical(tgtSrcType) == "字符串" &&
                !types::isReference(tgtSrcType) &&
                !isOwnedStrRvalue(node->value.get())) {
                markStrTainted(tgtName);
            }
        }
    }
    // plans/019 阶段2（2026-09-10）：局部地址逃逸检查——右值求值为当前函数
    //   局部的地址（&局部 / 引用局部绑局部）而赋值目标是比其寿命长的存储
    //   （静态/全局变量、静态/全局对象的字段或元素）时编译期拒绝（悬垂防线
    //   前移）+ 局部指针指向登记（返回检查依据）。
    checkLocalAddressEscapeAssign(node);
    // 010（def-init）：写点置位——左值 place（整体/字段链/常量下标）初始化
    if (lvalueOk) {
        defInitMarkAssignTarget(node->target.get());
    }
    // 981 赋值复活（001 条文⑥·用户裁决甲）：**纯赋值**成功路径尾部对标识符
    //   目标清除已转移标记=复活点（写不读旧值·Rust assign-to-move 同款）。
    //   成员/下标目标不复活（需读旧值·仍拦）；复合赋值已在左值检查拦。
    if (lvalueOk && !isCompoundAssign(node->op) &&
        node->target->getType() == NodeType::IdentifierExpr) {
        reviveMovedVar(static_cast<const IdentifierExpr*>(node->target.get())->name);
    }
}

// ==================== 175-a 族子方法（原 visitAssignmentExpr 541~886 段） ====================

// 族①：常量成员函数检查（原 541~570 段）——常量方法体内修改成员/直接字段 -> 错误。
void SemanticAnalyzer::checkConstMethodMemberAssign(AssignmentExpr* node) {
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
}

// ==================== 族②：左值白名单 switch（原 571~702 段） ====================
// 主分派（switch 薄化）：各 case 体提取为三族子方法。返回 targetType；
//   lvalueOk=false = 目标不可写（调用方做右值级联检查后提前返回）。
std::string SemanticAnalyzer::checkAssignLvalueTarget(AssignmentExpr* node,
                                                      bool& lvalueOk) {
    std::string targetType = "未知";
    lvalueOk = false;
    switch (node->target->getType()) {
        case NodeType::IdentifierExpr:
            checkIdentifierAssignTarget(node, targetType, lvalueOk);
            break;
        case NodeType::IndexExpr:
        case NodeType::MemberExpr:
            // 下标访问（数组[i]）/成员访问（对象.字段）均为可写左值
            // （IndexExpr→MemberExpr 原为 [[fallthrough]]——子方法内按形态
            // 分派等价实现）
            checkIndexMemberAssignTarget(node, targetType, lvalueOk);
            break;
        case NodeType::UnaryExpr:
        case NodeType::CallExpr:
            // 解引用（*p）与引用返回调用为可写左值；其余一元结果不可赋值
            checkUnaryCallAssignTarget(node, targetType, lvalueOk);
            break;
        default:
            // 缺陷②：二元运算结果/字面量/三元/强转/嵌套赋值等均非可写左值
            reportNonLvalueTarget(node);
            break;
    }
    return targetType;
}

// 族③：标识符左值 case 体（原 580~617 段）——静态写警告/常量/常量引用/
//   已转移拒绝；未声明报错。
void SemanticAnalyzer::checkIdentifierAssignTarget(AssignmentExpr* node,
                                                   std::string& targetType,
                                                   bool& lvalueOk) {
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
        // 102 甲案（2026-09-29 用户裁决·〔基准=019〕）：对字符串形参的赋值＝
        //   消耗标记（IR 层 prologue 深拷兑现「默认=值副本」——原借用直传下
        //   此赋值会释放调用方缓冲区）。非字符串形参名不命中，零影响。
        if (types::canonical(varType) == "字符串") {
            markStringParamConsumed(ident->name);
        }
        // 188-a（D6 B11 变量常量传播）：赋值位登记——RHS 形态分级（`无` 字面量=
        //   种子／标识符=传播边／其余（复合赋值/调用/运算…）=失格），仅指针/字符串
        //   类型参与（非指针变量的 `无` 初始化与空指针判定无关）
        if (isNullConstEligibleType(varType)) {
            noteNullAssign(ident->name, node->value.get());
        }
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
        // plans/019 阶段1（2026-09-10）立→981 甲裁决修正（001 条文⑥赋值复活）：
        //   **纯赋值**（v = x）目标不再因已转移拒绝——写不读旧值=复活点
        //   （Rust assign-to-move 同款·写点清标记见 visitAssignmentExpr 尾部）；
        //   **复合赋值**（+= 等）读旧值照拦（条文③读性使用）。
        else if (isCompoundAssign(node->op) &&
                 reportMovedUse(ident->name, ident->location)) {
            lvalueOk = false;
        }
    } else {
        diagnostics_.report(DiagnosticLevel::Error, ident->location,
                            "赋值目标未声明：'" + ident->name + "'");
    }
}

// 族④：下标/成员左值 case 体（原 618~658 段，含原 [[fallthrough]] 等价实现）
//   ——指针下标写警告 / 常量引用参数经成员写拒绝 / 目标求值。
void SemanticAnalyzer::checkIndexMemberAssignTarget(AssignmentExpr* node,
                                                    std::string& targetType,
                                                    bool& lvalueOk) {
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
            // 910（112 甲·分层安全①编译期拦）：字符串下标写=编译期拒绝——
            //   字符串不可变拥有型（修改走拼接/字符串子串 重建·原地字节操作用
            //   字符 数组或不安全区指针·069 同语义家族）。原静默无效写（s[1]='x'
            //   读回原值·rc=0）——v2 拒绝既有（174_v2 锚）·本补丁宿主对齐同拒。
            if (lookupVar(static_cast<const IdentifierExpr*>(iobj)->name, iot) &&
                types::canonical(iot) == "字符串") {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->target->location,
                    "下标左值赋值未支持（字符串下标只读）——字符串为不可变"
                    "拥有型，原地修改请用 字符 数组或不安全区指针（112 甲）: " +
                    static_cast<const IdentifierExpr*>(iobj)->name + "[i] = ...");
                lvalueOk = false;
                return;
            }
        }
    }
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
}

// 族⑤：解引用/引用返回调用左值 case 体（原 659~701 段）——Deref 白名单
//   （裸指针解引用写警告）/ 引用返回调用 / 其余拒绝。
void SemanticAnalyzer::checkUnaryCallAssignTarget(AssignmentExpr* node,
                                                  std::string& targetType,
                                                  bool& lvalueOk) {
    if (node->target->getType() == NodeType::UnaryExpr) {
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
        return;
    }
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
}

// ==================== 族⑥：借出视图登记 + 字符* 借用收紧（原 713~761 段） ====================
// true = 已诊断并终止（调用方 return）。
bool SemanticAnalyzer::checkBorrowViewAssign(AssignmentExpr* node,
                                             const std::string& targetType,
                                             const std::string& valueType) {
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
        return true;
    }

    // plans/019 阶段4' A2：赋值位借出装入拒绝（A1 收紧到赋值位）——「字符串
    //   变量=恒拥有槽」不变量。原靠 IR 污染（不 free），但污染变量经 返回 位
    //   移出→调用方按返回类型登记 free 借用指针=悬垂（v2p 全解析瘫痪实测）。
    //   收紧后赋值位借出一律显式拥有化。**泛型单态化体内豁免**：实例化类方法
    //   体（genericTypeParams_ 非空）的 T 元素搬移=容器内部存储管理（析构元素
    //   特判释放·Rust Vec 内部 ptr::write 同类）。
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
            return true;
        }
    }
    return false;
}

// ==================== 族⑦：复合赋值（原 763~783 段） ====================
// true = 已处理并终止（调用方 return）。
bool SemanticAnalyzer::checkCompoundAssign(AssignmentExpr* node,
                                           const std::string& targetType,
                                           const std::string& valueType) {
    if (isCompoundAssign(node->op)) {
        // 010（def-init）：复合赋值=先读后写——左值 place 读判定+写点置位
        defInitCheckReadAssignTarget(node->target.get(), node->location);
        defInitMarkAssignTarget(node->target.get());
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
            return true;
        }
        lastType_ = targetType;
        return true;
    }
    return false;
}

// ==================== 族⑧：局部地址逃逸检查 + 指向登记（原 806~886 段） ====================
// plans/019 阶段2（2026-09-10）：局部地址逃逸检查——右值为当前函数局部的
//   地址而赋值目标是比其寿命长的存储（静态/全局变量·对象字段/元素）时编译期
//   拒绝（悬垂防线前移）。局部指针/局部对象字段/局部数组元素接收局部地址合法
//   （随作用域消亡）；局部指针指向登记供返回检查（直接 &局部 形态）。
void SemanticAnalyzer::checkLocalAddressEscapeAssign(AssignmentExpr* node) {
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
