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

namespace {
// 505（G1·501 采样轮）：小型常量折叠求值——编译期预演面专用纯函数（零副作用）。
//   背景：checkConstIntExprSafety/checkShiftConstWidth 旧取值通道只认直接子节点
//   字面量（litValue 两处手写提取），嵌套折叠形态 ((30000+30000)*30000000)、
//   甲<<(700+66)、折叠零除数 10/(5-5) 双侧一致漏编译期检查（运行期错误码 8 兜底）
//   ——预演发生在折叠之前。修法=子树先递归折叠出常量闭包再按域预演（检查提前+
//   精确·不改变运行期兜底语义：无法折叠的形态〔含变量/中间溢出/除零〕返回 假=
//   保持现状走运行期错误码；错误形态由该子表达式自身节点的预演报告——
//   visitBinaryExpr 逐节点递归遍历，无遗漏面）。
//   折叠域：int64 安全闭包——加减乘用溢出条件检测（hi-vR 技巧·同
//   reportConstOverflowOrDivZero 家族手法），除零/INT64_MIN÷-1/移位量越 [0,63]
//   域/负值左移 返回 假（不可折叠——这些形态各自的节点级检查〔编译期除零/
//   INT_MIN÷-1 溢出/移位量超宽〕已按现状口径覆盖，折叠不与之竞争）。
//   归纳注：本函数取代 checkConstIntExprSafety 与 checkShiftConstWidth 两处
//   litValue 手写提取（第 2 处重复→提炼·Rule of Three）。
//   503 rebase 归纳：编译期除零定性（除/余右操作数折叠值==0）同样复用本通道，
//   不再另设 constIntFoldValue503（双折叠器归纳为一——零叉路口）。
bool foldConstIntExpr505(const Expr* e, std::int64_t& out, int depth = 0) {
    if (e == nullptr || depth > 64) return false;  // 深度守卫（防病态深嵌套）
    if (e->getType() == NodeType::IntegerLiteral) {
        out = static_cast<const IntegerLiteral*>(e)->value;
        return true;
    }
    if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->op == Operator::Subtract && !u->postfix) {
            std::int64_t v = 0;
            if (!foldConstIntExpr505(u->operand.get(), v, depth + 1)) return false;
            // 取负按两补码回绕位模式（INT64_MIN 取负=自身·与旧 litValue 同口径）
            out = static_cast<std::int64_t>(~static_cast<std::uint64_t>(v) + 1);
            return true;
        }
        return false;
    }
    if (e->getType() != NodeType::BinaryExpr) return false;
    const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
    std::int64_t l = 0, r = 0;
    if (!foldConstIntExpr505(b->left.get(), l, depth + 1)) return false;
    if (!foldConstIntExpr505(b->right.get(), r, depth + 1)) return false;
    switch (b->op) {
        case Operator::Add:
            // 溢出安全加：INT64_MAX-r（r>0）/INT64_MIN-r（r<0）恒良定义
            if ((r > 0 && l > INT64_MAX - r) || (r < 0 && l < INT64_MIN - r)) return false;
            out = l + r;
            return true;
        case Operator::Subtract:
            if ((r < 0 && l > INT64_MAX + r) || (r > 0 && l < INT64_MIN + r)) return false;
            out = l - r;
            return true;
        case Operator::Multiply: {
            const std::uint64_t magL =
                (l < 0) ? static_cast<std::uint64_t>(~static_cast<std::uint64_t>(l)) + 1
                        : static_cast<std::uint64_t>(l);
            const std::uint64_t magR =
                (r < 0) ? static_cast<std::uint64_t>(~static_cast<std::uint64_t>(r)) + 1
                        : static_cast<std::uint64_t>(r);
            const bool negResult = (l < 0) != (r < 0);
            // 乘积域界：正结果=INT64_MAX；负结果=|INT64_MIN|=2^63
            const std::uint64_t magHi =
                negResult ? static_cast<std::uint64_t>(INT64_MAX) + 1
                          : static_cast<std::uint64_t>(INT64_MAX);
            if (magL != 0 && magR != 0 &&
                (magL > magHi / magR || magL * magR > magHi)) return false;
            out = negResult ? static_cast<std::int64_t>(~static_cast<std::uint64_t>(magL * magR) + 1)
                            : static_cast<std::int64_t>(magL * magR);
            return true;
        }
        case Operator::Divide:
        case Operator::Modulo:
            if (r == 0) return false;  // 除零：由该节点预演报编译期除零
            if (l == INT64_MIN && r == -1) return false;  // 结果 2^63 超 int64：该节点预演报
            out = (b->op == Operator::Divide) ? l / r : l % r;
            return true;
        case Operator::LessLess:
            if (l < 0 || r < 0 || r > 62) return false;  // 负左移/溢出位 UB 面：不折叠
            if (static_cast<std::uint64_t>(l) >
                (static_cast<std::uint64_t>(INT64_MAX) >> r)) return false;
            out = l << r;
            return true;
        case Operator::GreaterGreater:
            if (r < 0 || r > 63) return false;
            out = l >> r;  // 算术右移（与发射层面口径一致）
            return true;
        default:
            return false;  // 比较/位运算/逻辑等不进折叠闭包（预演面只管算术+移位）
    }
}

}  // namespace

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
    checkShiftConstWidth(node, leftType, rightType);  // 任务 474：常量移位量超宽编译期硬错误
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
//   两侧均可折叠为常量（字面量/一元负链/纯常量子树——505 折叠通道）时按结果
//   类型域预演：
//   ①÷/%：常量除零编译期硬错误（T8：原 10/0 编过、运行期才报）——503 解绑
//     扩面：除/余右操作数为编译期可折叠表达式且值为 0 即拦，左操作数变量
//     无关（001 §1.1a①「编译期能证明的必须在编译期拦·不得静默放行到运行
//     期」+§3.7a「常量折叠面同样拒绝」·v2 575-a 右字面量零口径对齐·501 采
//     样轮分叉实锤；浮点除零不拦=IEEE 语义；不安全 函数内同拦=编译期确定
//     性错误无条件·114 同款；常量名/变量常量初值=常量传播面「随常量折叠
//     后续波」另案立项不在本轮）；
//   ②+/-/*：常量溢出编译期硬错误（原 2147483647+1 静默回绕——Rust debug
//     溢出检查同款严格面；变量面回绕语义由 B12 运行时通道承载）。
//   比较类不涉溢出；无符号/整128 结果域宽于 int64 预演承载，跳过（不误报）。
void SemanticAnalyzer::checkConstIntExprSafety(BinaryExpr* node, const std::string& leftType,
                                               const std::string& rightType) {
    if (isArithmeticOp(node->op) && isInteger(leftType) && isInteger(rightType) &&
        leftType != "未知" && rightType != "未知") {
        // 505：取值通道升级——直接字面量（旧 litValue 口径）扩为递归常量折叠
        //   （嵌套形态 ((30000+30000)*30000000)/折叠零 10/(5-5) 由此进预演；
        //   折叠失败〔变量/中间溢出〕=保持现状走运行期错误码兜底）
        // 503：编译期除零检测从对称预演前提解绑——除/余右操作数经同一折叠
        //   通道求值==0 即编译期硬错误（与 505 共用 foldConstIntExpr505 单折
        //   叠器·rebase 归纳零双份；除零定性后与溢出面互斥不重复预演）。
        if (node->op == Operator::Divide || node->op == Operator::Modulo) {
            std::int64_t foldR = 0;
            if (foldConstIntExpr505(node->right.get(), foldR) && foldR == 0) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "编译期除零：常量表达式除数为零（运行期除零已有"
                                    "错误码防护，常量折叠面编译期硬错误）");
                return;  // 已定性除零·除零与溢出面互斥·不重复进入对称预演
            }
        }
        // 319-a 对称常量预演（§3.7a）：两侧均编译期可折叠时按结果类型域预演
        //   ——溢出面（含 474 INT_MIN/-1 除法溢出形态）。
        bool okL = false, okR = false;
        std::int64_t vL = 0, vR = 0;
        okL = foldConstIntExpr505(node->left.get(), vL);
        okR = foldConstIntExpr505(node->right.get(), vR);
        if (okL && okR) {
            reportConstOverflow(node, vL, vR, leftType, rightType);
        }
    }
}

// 族④ 子方法（原 if (okL && okR) 预演体·503 改名：除零面已解绑上移，仅承载
//   溢出面）：结果域提升 + 溢出编译期硬错误。
void SemanticAnalyzer::reportConstOverflow(BinaryExpr* node, std::int64_t vL,
                                           std::int64_t vR,
                                           const std::string& leftType,
                                           const std::string& rightType) {
            // 预演结果域：字面量操作数按后缀+值提升（对齐 visitIntegerLiteral
            // 的提升链——语义层 lastType/literalTypeOf 对无后缀字面量恒整32，
            // 若按其取域会把 int64 值字面量运算误判超整32 域〔420 用例实锤〕）；
            // 提升至整128（超 int64 无后缀）时域宽于 int64 预演承载，跳过。
            // 505：折叠子树（一元负链/嵌套常量算术）无 raw 文本——按折叠值提升
            //   （420 按值提升链的折叠推广：|v| 超 int32 域→整64。语义类型推导
            //   对嵌套子表达式恒窄域〔无后缀字面量整32·commonNumericType 整32〕，
            //   按它取域会把 int64 量级折叠值误判超域——v2 自举源码
            //   0-9223372036854775807 惯用形态 6 处实锤，修后全绿）。
            auto promotedType = [](const Expr* e) -> std::string {
                if (e->getType() == NodeType::IntegerLiteral) {
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
                }
                std::int64_t v = 0;
                if (foldConstIntExpr505(e, v) &&
                    (v > 2147483647LL || v < -2147483648LL)) {
                    return "整64";
                }
                return "";
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
            // 503：除零分支已解绑上移至 checkConstIntExprSafety 折叠面（除/余
            //   右折叠值==0 先行报告并 return·此处 vR==0 不可达）——本方法仅
            //   承载溢出面（与除零互斥·防双报）。
            if ((node->op == Operator::Divide || node->op == Operator::Modulo) &&
                       hasRange && vR == -1 && vL == lo) {
                // 任务 474：INT_MIN / -1（与 INT_MIN % -1）=整数除法唯一溢出形态
                //（x86 idiv 溢出陷阱·arm64 sdiv 回绕）——运行期改报错误码 8，
                //   常量面同款编译期硬错误（与 +/-/* 常量溢出同面）
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "编译期整数溢出：INT_MIN / -1（或 % -1）结果超出"
                                    "类型 '" + rt + "' 范围（除法唯一溢出形态·"
                                    "任务 474 错误码 8 对齐）");
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

// 族④b（任务 474）：常量移位量超宽编译期硬错误——移位量（右操作数）可折叠为
//   常量（505 折叠通道：直接字面量/一元负/折叠形态如 甲<<(700+66)）且值不在
//   [0, 左操作数类型位宽) 域 → 编译期拒绝。原语义=按位宽取模掩码
//   （2026-09-09 规范化·Rust release 同构）随 474 裁决废止：运行期超宽移位改报
//   错误码 8『溢出错误』（三后端发射层），常量面在此先行硬错误（Rust debug
//   compile_error 对齐：`1i32 << 33` 编译失败）。左操作数可为变量（x << 33）。
void SemanticAnalyzer::checkShiftConstWidth(BinaryExpr* node,
                                            const std::string& leftType,
                                            const std::string& rightType) {
    if (node->op != Operator::LessLess && node->op != Operator::GreaterGreater) {
        return;
    }
    if (!isInteger(leftType) || !isInteger(rightType) ||
        leftType == "未知" || rightType == "未知") {
        return;
    }
    // 移位量取值：505 折叠通道（递归常量闭包——取代旧 litValue 手写提取；
    //   折叠失败=非常量移位量，保持现状走运行期同域检查）
    std::int64_t sh = 0;
    // 裸字面量形态识别（#547 巨字面量预检面）：直接字面量/一元负号字面量
    //   抓 raw 原始文本与负号形态，供下方 18 位预检（parser parseIntValue 对
    //   超 int64 域字面量静默回 0 毒化 value·预检只依赖 raw 文本不依赖值）；
    //   折叠形态无单一 raw 文本·不在 547 预检边界（547 提交声明）。
    std::string rawText;
    bool negForm = false;
    const Expr* re = node->right.get();
    if (re->getType() == NodeType::IntegerLiteral) {
        rawText = static_cast<const IntegerLiteral*>(re)->raw;
    } else if (re->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(re);
        if (u->op == Operator::Subtract && !u->postfix && u->operand != nullptr &&
            u->operand->getType() == NodeType::IntegerLiteral) {
            rawText = static_cast<const IntegerLiteral*>(u->operand.get())->raw;
            negForm = true;
        }
    }
    // 取值统一走 505 折叠通道（a6526fcd 单折叠器归属·禁双通道取值分叉）：
    //   折叠失败=非常量移位量，保持现状走运行期同域检查。
    if (!foldConstIntExpr505(node->right.get(), sh)) return;
    // 左操作数类型位宽（整8/正8=8 … 整128/正128=128）
    const std::string lt = types::canonical(leftType);
    int width = 0;
    if (lt == "整8" || lt == "正8") width = 8;
    else if (lt == "整16" || lt == "正16") width = 16;
    else if (lt == "整32" || lt == "正32" || lt == "整数") width = 32;
    else if (lt == "整64" || lt == "正64") width = 64;
    else if (lt == "整128" || lt == "正128") width = 128;
    if (width == 0) return;
    // 巨字面量预检（#547·v2 检查常量移位超宽474 同构）：原始文本有效位 >18
    //   （剥 0x 前缀计位）=值必超任一整型域宽——文本位 >18 的最小值=2^18
    //   （0b 形态 1 后 18 个 0）=262144 > 128（最大整128 位宽）恒真下界；
    //   parser parseIntValue 对超 int64 表示域的字面量 stoll 失败静默回 0
    //   （巨移位量被毒化为 0 放行·v2 拒宿主放行分叉实锤 505 探针），故按
    //   raw 文本先行硬错误，防毒化值流入下方域检查（18 位以内值已可安全
    //   解析，交由下方既有域检查精确裁决）。
    {
        std::size_t digits = rawText.size();
        if (digits > 2 && rawText[0] == '0' &&
            (rawText[1] == 'x' || rawText[1] == 'X')) {
            digits -= 2;
        }
        if (digits > 18) {
            const std::string shown = negForm ? "-" + rawText : rawText;
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "编译期移位量超宽：常量移位量字面量 '" + shown +
                                "' 超任一整型域宽（类型 '" + leftType +
                                "' 位宽 " + std::to_string(width) +
                                "·有效数字超 18 位必超域·v2 同构预检）");
            return;
        }
    }
    if (sh < 0 || sh >= width) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "编译期移位量超宽：移位量 " + std::to_string(sh) +
                            " 超出类型 '" + leftType + "' 位宽 " +
                            std::to_string(width) + "（移位量须在 [0, " +
                            std::to_string(width) + ") 域·任务 474 错误码 8 对齐；"
                            "变量面运行期同域检查）");
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
        // 411（406 采样轮·算术总守卫·左类已由上方重载分支管辖故排除防双报）：
        //   拼接/指针算术/重载分支均未命中后，四则操作数必须均为数值（整数/浮点
        //   /字符——979 字符四字节窄整语义·'a'+1=98 合法面 690 锚）。修前布尔/
        //   字符串/枚举等非法面经 commonNumericType 兜底静默放行（布尔四则布尔化
        //   绕过条件布尔检查/字符串减法放行产物段错误/枚举算术放行·v2 同病 411）。
        //   一元负/比较/位运算/%/复合赋值/自增自减通道守卫本就健全——缺口唯此处。
        if (!isClassType(canonicalType(leftType)) &&
            ((!isNumeric(leftType) && leftType != "字符") ||
             (!isNumeric(rightType) && rightType != "字符"))) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "算术运算符要求数值操作数，实际为 '" + leftType + "' 与 '" +
                                rightType + "'");
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