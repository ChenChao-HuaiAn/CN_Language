// CN 语义分析器——二元运算表达式检查（350 重构E2 自 semantic_expr_op.cpp 纯机械搬移）
// 548：原匿名 ns 折叠器 foldConstIntExpr505 整体提升 model/semantic_helpers.hpp
//   中立层共享（cnFoldConstIntExpr505·扩常量名解析参数）——除零/溢出/移位三消费方
//   与 548 零值种子表跨文件单份（防双实现漂移·语义_helpers inline 中立层惯例）。
// 职责（函数体逐字搬移·零语义变化；声明仍在 semantic.hpp）：
//   visitBinaryExpr 分发器 + 族方法（逻辑/混合符号与安全区/空类型/编译期常量除零溢出/比较/位运算/算术（含串拼接与指针算术）/运算符重载）。
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

namespace {
// 582（604 并案）：编译期溢出诊断「字面量参与域」提示句——481 立法
//   （001 §3.7/§3.7a）后字面量恒按另一侧操作数类型参与·与赋值目标域解耦
//   （`整32 和 = 甲 + 1`/`整64 大 = 2147483647 + 1` 新手困惑「1 明明按更宽
//   域算」而实报字面量恒定级域）。本句在 INT_MIN÷-1 与通用溢出两处消费；
//   v2 语义检查溢出474.cn 溢出字面量参与域提示582 逐字同源（零分叉）。
//   548：原同 ns 折叠器 foldConstIntExpr505 已整体提升 model/semantic_helpers.hpp
//   中立层共享（cnFoldConstIntExpr505·扩常量名解析参数）——除零/溢出/移位
//   三消费方跨文件单份（防双实现漂移）；本 ns 仅留 582 提示句。
constexpr const char kLiteralDomainHint582[] =
    "。字面量按另一侧操作数类型参与·与赋值目标域解耦（001 §3.7/§3.7a）";

// 728s 对称化（2026-10-15 用户裁决批乙·#728s「宿主字面量二元算式结果类型
//   『顺序敏感』（1+2U 拒/1U+2 放行）是否对称化」→ 批乙：对称化·Rust 真对齐）：
//   无后缀整数字面量随另一侧定级——一侧为**无后缀非负**整数字面量、另一侧为
//   「有定级」（非无后缀字面量）的整数族类型时，字面量侧按另一侧类型参与
//   （Rust 字面量推断惯例：1 + 2U → 正32、2U + 1 → 正32 双向对称；另一侧为
//   变量/成员/调用等同规则——001 §3.7「字面量按另一侧类型参与」在结果类型
//   推导面的落实）。边界三条：①两侧均无后缀=无定级对象（各自恒定级整32·
//   既有行为零变化、杜绝互随顺序敏感）；②负号字面量不随侧（无符号域不承载
//   负值·Rust 同口径：-1 + 2U 维持混合符号拒绝——323 解析期把 `-1` 折叠为
//   raw="-1" 的整数字面量，负号判定须查 raw 首字符）；③另一侧非整数族（浮点/
//   字符等）不触发（不在本轮扩面）。
//   「无后缀」判定=literalTypeOf 为 整32（无后缀）或空串（非法组合防御回退
//   ——含 0xFF 十六进制尾 F 被后缀收集的既有回退形态，与语义层字面量恒定级
//   整32 同口径；v2 字面量后缀类型ID==0 同构）。
//   548：原同 ns 折叠器 foldConstIntExpr505 已提升 model/semantic_helpers.hpp
//   中立层共享（cnFoldConstIntExpr505·扩常量名解析参数）——除零/溢出/移位
//   三消费方跨文件单份（防双实现漂移）；本 ns 留 582 提示句与 728s 对称化面。
bool isUnsuffixedIntLiteral728(const Expr* e) {
    if (e == nullptr || e->getType() != NodeType::IntegerLiteral) return false;
    const IntegerLiteral* lit = static_cast<const IntegerLiteral*>(e);
    if (!lit->raw.empty() && lit->raw[0] == '-') return false;  // 负号字面量不随侧
    const std::string lt = types::literalTypeOf(lit->raw, false);
    return lt == "整32" || lt.empty();
}

// 无后缀字面量随另一侧替换（左优先·单侧触发——两侧均无后缀时不替换，杜绝
//   互随顺序敏感）。返回 真=已发生替换。结果类型推导（checkArithmeticBinary/
//   checkBitwiseBinary）与常量溢出预演（reportConstOverflow 域计算）共用同一
//   规则——类型推导与域预演口径必须一致，否则出现「类型正32 而域按整32 误报
//   溢出」的内部双口径（949 q6c 形态 = 2147483647 + 1U）。
bool unifyUnsuffixedLiteralOperand728(const Expr* leftExpr, const Expr* rightExpr,
                                      std::string& leftType, std::string& rightType) {
    const bool lUnsuf = isUnsuffixedIntLiteral728(leftExpr);
    const bool rUnsuf = isUnsuffixedIntLiteral728(rightExpr);
    if (lUnsuf && !rUnsuf && types::isInteger(rightType)) {
        leftType = types::canonical(rightType);
        return true;
    }
    if (rUnsuf && !lUnsuf && types::isInteger(leftType)) {
        rightType = types::canonical(leftType);
        return true;
    }
    return false;
}

// 1064（2026-10-18·#1040 派生缺口·表达式操作数扩展）：指针差操作数判可——
//   语义层已判两侧同型指针（isPointerType 命中）之后，本判据限定**可放行**
//   的操作数形态：标识符（指针变量/数组名退化）或 ± 链（指针侧为标识符或
//   递归 ± 链）——与 IR 侧 ptrOperandSrcType 的步进源回溯同构（genPointer
//   Arithmetic 双指针/单指针分支须解析元素步进），保证放行面在 IR 恒可解析
//   步进（648 显形面零新增）。其余形态（成员链/取地址/调用返回/下标/三元
//   等）=不可判——维持既拒文案「两个指针不能做算术运算」，与 v2 指针值
//   表达式元素1064 边界一致（双侧同拒·立法收窄不放松）。
bool isJudgedPtrDiffOperand1040(const Expr* e) {
    if (e == nullptr) return false;
    if (e->getType() == NodeType::IdentifierExpr) return true;
    if (e->getType() == NodeType::BinaryExpr) {
        const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
        if (b->op == Operator::Add || b->op == Operator::Subtract) {
            return isJudgedPtrDiffOperand1040(b->left.get()) ||
                   isJudgedPtrDiffOperand1040(b->right.get());
        }
    }
    return false;
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
        // 548：折叠通道开常量名解析（resolveConstNameValue：globalConstValues_
        //   顶层常量值·584-a 遮蔽守卫同款——顶层常量名/常量名参与折叠式均进
        //   定性面·001 §1.1a① 常量传播后续波）；右子树折叠失败且为裸标识符
        //   变量=零初值使用点延迟登记（零值种子表收尾统一判定·zp548_）。
        if (node->op == Operator::Divide || node->op == Operator::Modulo) {
            std::function<bool(const std::string&, std::int64_t&)> resolver =
                [this](const std::string& n, std::int64_t& v) {
                    return resolveConstNameValue(n, v);
                };
            // 1137：纯常量形态先走 919 文本级求值判零（大数安全）——原单链
            //   505 值级折叠读 IntegerLiteral::value，对超整64 承载字面量是
            //   parseIntValue stoll 溢出毒化 0（catch 静默面）→ 数学值非零的
            //   右操作数被误判为零：`10/(99999999999999999999-99999999999999999998)`
            //   （数学=1）误报「编译期除零」拒合法程序（1137 探针实证·值级
            //   消费点的毒化出口）。919 判定不了（含变量/常量名）回落 505+
            //   resolver 既有通道（548 面保留·判定口径展开不改文案）。
            std::string r919;
            bool zeroR = false;
            const bool textFoldable1137 =
                cnEvalConstIntExprText919(node->right.get(), r919);
            if (textFoldable1137) {
                // 空串=超 u128 域哨兵（数学值必非零·不报除零）
                zeroR = (r919 == "0");
            }
            if (!textFoldable1137) {
                std::int64_t foldR = 0;
                zeroR = cnFoldConstIntExpr505(node->right.get(), foldR, 0,
                                              &resolver) &&
                        foldR == 0;
            }
            if (zeroR) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "编译期除零：常量表达式除数为零（运行期除零已有"
                                    "错误码防护，常量折叠面编译期硬错误）");
                return;  // 已定性除零·除零与溢出面互斥·不重复进入对称预演
            }
            // 548：变量零初值面——右子树裸标识符且非常量名（折叠失败）→
            //   使用点登记（是否恒零由种子表收尾不动点统一判定·流不敏感保守）
            const Expr* re548 = node->right.get();
            if (re548->getType() == NodeType::IdentifierExpr) {
                const auto* ident548 = static_cast<const IdentifierExpr*>(re548);
                if (!isConstVarName(ident548->name)) {
                    noteZeroUse(ident548->name, node->location,
                                node->op == Operator::Divide ? "除" : "取余");
                }
            }
        }
        // 319-a 对称常量预演（§3.7a）：两侧均编译期可折叠时按结果类型域预演
        //   ——溢出面（含 474 INT_MIN/-1 除法溢出形态）。
        bool okL = false, okR = false;
        std::int64_t vL = 0, vR = 0;
        okL = cnFoldConstIntExpr505(node->left.get(), vL);
        okR = cnFoldConstIntExpr505(node->right.get(), vR);
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
                    // 728g（#506甲·420 负号形态）：数学值域判据含负号——parser
                    //   323 负号折叠后 lit->value 即数学负值，修前只判正域使
                    //   -3000000000 仍按整32 域预演：-3000000000*2 误报「编译期
                    //   整数溢出」（v2 同形放行·-6000000000 整64 域内合法）；
                    //   共用单点判据（-2147483648*2 双侧仍按整32 域拒=零翻转）。
                    if (types::literalValueExceedsInt32Domain(lit->value)) return "整64";
                    return "整32";
                }
                std::int64_t v = 0;
                if (cnFoldConstIntExpr505(e, v) &&
                    types::literalValueExceedsInt32Domain(v)) {
                    return "整64";
                }
                return "";
            };
            const std::string pL = promotedType(node->left.get());
            const std::string pR = promotedType(node->right.get());
            std::string effL = (!pL.empty() && pL != "整32") ? pL : types::canonical(leftType);
            std::string effR = (!pR.empty() && pR != "整32") ? pR : types::canonical(rightType);
            // 728s 对称化（2026-10-15 用户裁决批乙）：域预演与结果类型推导同
            //   规则（无后缀字面量随另一侧）——`2147483647 + 1U` 结果域=正32
            //   （无符号域不预演·跳过）、原按整32 域误报编译期溢出（949 q6c
            //   形态）；`4294967295 + 1U` 同域=正32（与 m2 无符号回绕口径同）；
            //   双侧无后缀/带后缀形态不触发（420 按值提升链既有行为零变化）。
            unifyUnsuffixedLiteralOperand728(node->left.get(), node->right.get(),
                                             effL, effR);
            // 648 归档（commonNumericType 终态消费面）：常量溢出预演面——
            //   effL/effR 为字面量提升类型或已守卫的操作数类型；终态 "整32"
            //   只影响溢出域判定宽度，且方向为收窄（最坏=误报超域硬错误，
            //   非静默放行），不加报错。
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
                                    "任务 474 错误码 8 对齐）" +
                                    kLiteralDomainHint582);
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
                                        "变量面回绕语义由运行时通道承载）" +
                                        kLiteralDomainHint582);
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
    if (!cnFoldConstIntExpr505(node->right.get(), sh)) return;
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
        // 548（编译期除零·常量传播）：比较参与=零值判定失格——零初值变量进入
        //   比较即进入除零防视野（短路守卫惯用形 `除数 != 0 && 10/除数`·125/137
        //   短路语义锁形态），其除法使用点保守放行交运行期兜底（检查序=左子树
        //   先于右：比较失格先于同表达式右侧除法的使用点登记·收尾判定正确）。
        if (isInteger(leftType) && isInteger(rightType)) {
            if (node->left->getType() == NodeType::IdentifierExpr) {
                const auto* l548 = static_cast<const IdentifierExpr*>(node->left.get());
                if (!isConstVarName(l548->name)) noteZeroDisqual(l548->name);
            }
            if (node->right->getType() == NodeType::IdentifierExpr) {
                const auto* r548 = static_cast<const IdentifierExpr*>(node->right.get());
                if (!isConstVarName(r548->name)) noteZeroDisqual(r548->name);
            }
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
        // 648 归档（commonNumericType 终态消费面）：上方 isInteger 守卫已对非
        //   整数操作数报 Error（此处继续取 lastType_=错误恢复值，防下游再扩大
        //   报错）；合法面 isInteger → intRank>0，不落 "整32" miss 终态。
        // 728s 对称化（2026-10-15 用户裁决批乙）：无后缀字面量随另一侧——
        //   位运算（& | ^）与算术面同一助手同一规则（1 | 2U 等形态原落混合
        //   符号误拒，现双向对称）。**移位（<< >>）不随侧**：右操作数不参与
        //   结果类型（结果=左操作数参与公共类型·Rust/C 同口径）——与 v2
        //   740 推导二元结果类型ID 移位面「字面量直译整32」及宿主实测锚
        //   `整8 甲 = 甲8 << 2` 拒「无法将 '整32' 隐式转换为 '整8'」一致。
        std::string lT728 = leftType;
        std::string rT728 = rightType;
        if (node->op != Operator::LessLess &&
            node->op != Operator::GreaterGreater) {
            unifyUnsuffixedLiteralOperand728(node->left.get(), node->right.get(),
                                             lT728, rT728);
        }
        lastType_ = commonNumericType(lT728, rT728);
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
        // 728s 对称化（2026-10-15 用户裁决批乙·#728s）：无后缀字面量随另一侧
        //   定级——285 窄域适配的整数族泛化：正32 同秩场景 `1 + 2U` 原落
        //   「同秩取左=整32」与 `2U + 1`=正32 不对称（编译期混合符号误拒
        //   非对称），判决=双向放行（Rust 真对齐）。另一侧整数族（含变量/
        //   带后缀字面量/成员/调用推导）时字面量侧按另一侧参与；两侧均无后缀
        //   =无定级对象（整32 恒定级既有行为零变化）。
        unifyUnsuffixedLiteralOperand728(node->left.get(), node->right.get(),
                                         leftType, rightType);
        // 648 归档（commonNumericType 终态消费面）：合法消费面=字符参与算术
        //   （979 字符四字节窄整语义·'a'+1=98，canonical("字符") 的 intRank=0
        //   落 "整32" 终态=设计内回退，非 miss）；非法面（布尔/字符串/枚举等）
        //   已由上方 411 算术总守卫报 Error，此处取值为错误恢复值。
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
        // 1040（2026-10-18 任务 1040 用户批甲·001 §5.3 清单 #1 扩充）：同型指针差
        //   （尾-首）＝不安全函数内合法——结果=有符号整64「元素个数」（IR 层按
        //   (地址差)/元素大小 发射·C ptrdiff 同款·尾在首前得负）；安全区边界已由
        //   上方 reportUnsafeBoundary（「指针算术」kind）先行执法，本分支只决定
        //   不安全域内的类型/拒绝面。
        //   同型 = 规范化类型文本全等且非字符串语义类型（字符串/字符*=视图·
        //   001 §5.3 豁免①不适用指针差）；异型 = 编译错误（文案与 v2 逐字一致）。
        //   操作数形态（1064·2026-10-18 任务 1064 扩展）：标识符对（1040 首批）
        //   + 表达式操作数（(p+1)/(p+n) 等 ± 链——isJudgedPtrDiffOperand1040
        //   判可·与 v2 指针值表达式元素1064/IR ptrOperandSrcType 三处同构）；
        //   其余形态（成员/取地址/调用/下标）维持「两个指针不能做算术运算」
        //   （v2 同判·立法收窄不放松）。
        const bool sameTypeDiff1040 =
            (leftType == rightType) && !isStringSemanticType(leftType);
        const bool operandsJudgeable1040 = isJudgedPtrDiffOperand1040(node->left.get()) &&
                                     isJudgedPtrDiffOperand1040(node->right.get());
        if (node->op == Operator::Subtract && sameTypeDiff1040 && operandsJudgeable1040) {
            lastType_ = "整64";
            node->resolvedType = "整64";   // IR 侧 genPointerArithmetic 双指针分支识别
            return true;
        }
        // 指针差三分支的显式拒绝档：减且异型→报「要求同型」；同型但形态
        // 不可判（成员/取地址/调用/下标）落入下方「不能做算术」档。
        // 三档均有显式终态（合法置整64/错误诊断后终止），无静默取值。
        if (node->op == Operator::Subtract && !sameTypeDiff1040) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "指针差要求两个指针同型");
            lastType_ = "整64";
            return true;
        }
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