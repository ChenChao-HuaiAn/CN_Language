// 语义分析器实现：内建调用特判族（1283 轮自 semantic_call.cpp 机械搬移——
//   该文件 955 行触警戒线，「先拆再加」纪律拆出；纯搬移零行为变更）。
//   族 = 显式转移 转移()/复制() 克隆内置/回绕加·回绕减·回绕乘（任务 474）——
//   成员函数声明仍在 semantic.hpp；共享 helper 随族迁入（wrapLiteralOperandType/
//   wrapBuiltinIndexOf 本段私有·匿名/静态域）。
#define _CRT_SECURE_NO_WARNINGS
#include <string>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

// 族1：显式转移 转移(变量)——表达式位特判（原 visitCallExpr 115~161 段）
//   （声明初始化位由 visitVarDecl 先行拦截改写，不会到达此处）。表达式位仅放行
//   指针/字符串（值交接无 RAII，ir_call 按 resolvedType 特判展开为实参值加载）；
//   容器/类等拥有资源类型拒绝（防浅柄接管撞 RAII 双析构），随阶段3 放开。
bool SemanticAnalyzer::checkTransferCall(CallExpr* node) {
    if (isTransferCall(node)) {
        Expr* arg = node->arguments[0].get();
        // 959（008 总攻·转移位余二②）：**成员源放行**（乙 = 转移(h.桶)）——
        //   旧名禁用按成员路径推导（成员被转移后再用 h.桶 于 IR 层清零后
        //   读空句柄/空安全跳过；编译期拦截随 LUE 007 轮精确化）。
        //   IR 层 ir_call 转移展开同步支持 MemberExpr 源（成员地址→句柄→
        //   成员槽清零）。〔基准=019〕第二句：显式转移=成员所有权交接。
        if (arg->getType() == NodeType::MemberExpr) {
            MemberExpr* movedMemberSrc = static_cast<MemberExpr*>(arg);
            std::string mt = checkExpr(movedMemberSrc);
            // 静默豁免上游已诊断: mt 推导 miss=显式「未知」上行（checkExpr 已诊断恢复面·消费点 转移诊断 分流）
            if (mt.empty() || mt == "未知") {
                lastType_ = "未知";
                return true;
            }
            node->resolvedType = mt;
            lastType_ = mt;
            return true;
        }
        if (arg->getType() != NodeType::IdentifierExpr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "转移目标须为变量（标识符）或成员——下标/解引用形态"
                                "的转移随后续轮支持");
            lastType_ = "未知";
            // 静默豁免上游已诊断: 形态不符已报错·此为错误恢复面置未知
            return true;
        }
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(arg);
        std::string varType;
        if (!lookupVar(ident->name, varType)) {
            diagnostics_.report(DiagnosticLevel::Error, ident->location,
                                "未声明的标识符 '" + ident->name + "'");
            lastType_ = "未知";
            return true;
        }
        const int kind = transferArgKind(varType);
        if (kind == 1) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "变量 '" + ident->name + "'（类型 '" + varType +
                                    "'）具有复制语义，无需转移");
            lastType_ = "未知";
            return true;
        }
        // plans/022 波 4 首件（459-a·206-d）：拥有资源类型（容器/类/结构体/
        //   结果/可选/数组）表达式位转移**放行**——IR 层展开=句柄直拷+源槽清零
        //   （真槽位交接·与声明初始化位 ir_stmt_decl 浅交接分派同构）。原
        //   「仅声明初始化位」拒绝解除：实参位/返回位等表达式位自此可显式移交
        //   拥有型资源（Rust move 语义对照：所有权移交+源失效）。
        if (reportMovedUse(ident->name, node->location)) {  // 再转移=使用已转移变量
            lastType_ = "未知";
            return true;
        }
        markMovedVar(ident->name, node->location.getLine());
        // 102 甲案（2026-09-29 用户裁决·〔基准=019〕）：转移(字符串形参)＝消耗
        //   （move 发射会清零形参槽＝清掉调用方缓冲区句柄——IR 层须先深拷）。
        if (types::canonicalEquals(varType, "字符串")) {
            markStringParamConsumed(ident->name);
        }
        node->resolvedType = varType;  // IR 层展开识别（ir_call 特判）
        lastType_ = varType;
        return true;
    }
    return false;
}

// 206-b（2026-09-15 波 4·plans/022 §四.5）：复制(表达式) 泛型克隆内置——
//   与 转移(x) 构成显式「复制/移动」双内置（C++ copy/move 对照物）。返回类型=
//   实参类型（泛型内置无法用固定签名注册 functions_ 表 → 调用处特判）；字符串
//   实参置 retOwnedString（A2 拥有契约：克隆产物归调用方拥有，接收位不复制）。
//   类实参要求类型定义拷贝构造（无则拒绝——复制 语义要求独立拥有，浅柄共享
//   违背克隆契约）；IR 层 genCopyBuiltin 按类型分派发射（ir_fields.cpp）。
bool SemanticAnalyzer::checkCopyBuiltinCall(CallExpr* node) {
    if (node == nullptr || node->callee->getType() != NodeType::IdentifierExpr)
        return false;
    if (static_cast<IdentifierExpr*>(node->callee.get())->name != "复制")
        return false;
    // 静默豁免闭集穷举: 复制 内置 callee 恒为标识符（语法面保证·非本通道）
    if (node->arguments.size() != 1) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "复制(表达式) 恰好接收 1 个实参");
        lastType_ = "未知";
        return true;
    }
    Expr* arg = node->arguments[0].get();
    lastType_ = checkExpr(arg);
    // 类实参：须有拷贝构造（克隆契约——无拷贝构造类型无法独立拥有副本）
    if (!lastType_.empty() && !types::isArray(lastType_) &&
        isClassType(types::canonical(lastType_)) &&
        !findCopyConstructor(types::canonical(lastType_)).has_value()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类型 '" + lastType_ + "' 未定义拷贝构造，无法 复制");
        lastType_ = "未知";
    }
    node->resolvedType = lastType_;  // IR 层 genCopyBuiltin 分派依据
    node->retOwnedString = (lastType_ == "字符串");  // A2：克隆产物归调用方拥有
    // 874（任务 083·〔基准=019〕）：LUE 判定——字符串/容器面（与 v2 限面同构）；
    //   078（934·008 总攻第二轮）：结构体面扩面（复槽分配+整块搬迁+源槽清零
    //   在 IR 层 genCopyBuiltin 结构体 LUE 分支）；通过 → move 发射。
    if (lastType_ == "字符串" || isClassType(types::canonical(lastType_)) ||
        isStructType(types::canonical(lastType_))) {
        node->lueMove = lueEligible(node, arg);
    }
    return true;
}

// ==================== 任务 474：回绕内置 回绕加/回绕减/回绕乘 ====================

// 535：整数字面量（含一元负号形态）的「参与域」——按值提升。420 立法（族④
//   预演域 promotedType 同源）：无后缀字面量标记域恒整32、求值域按值——超
//   int32 → 整64、数字文本超 int64 → 整128（m27 提升链）；带后缀先按后缀
//   定级再值域提升；一元负号按数学负值判定（-2147483648 参与域=整64——与
//   IR 层 visitIntegerLiteral 装载提升、v2 字面量寄存器值域选型三方同判）。
//   修前回绕内置实参面直接用标记域（恒整32）：回绕乘(3000000000, 1) 按整32
//   位域截断折叠出 -1294967296，与同型普通算术 3000000000+0=3000000000（420
//   面）及 v2 分叉——违 001 §3.7a「回绕与算术运算符同类型规则」。非字面量
//   返回 ""（调用方保持 checkExpr 原类型）。仅回绕实参面调用点（语义层其他面
//   维持标记域口径·420 预演域预演体独立），故驻本文件匿名/静态域不动 .hpp。
//   1068：判定本体已切 types::integerLiteralRuntimeType 单点（见函数内注释·
//   进制文本/带后缀超域形态收口·910 五面同判据）。
//   1360 结果化批3B：无值=非整数字面量形态（原空串哨兵退役）；有值=运行时
//   参与域类型文本（值恒非空）。调用方 value_or 显形回落保持原类型。
cn::Optional<std::string> wrapLiteralOperandType(const Expr* e) {
    if (e == nullptr) return std::nullopt;
    const IntegerLiteral* lit = nullptr;
    bool outerNeg = false;
    if (e->getType() == NodeType::IntegerLiteral) {
        lit = static_cast<const IntegerLiteral*>(e);
    } else if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->op == Operator::Subtract && !u->postfix &&
            u->operand != nullptr &&
            u->operand->getType() == NodeType::IntegerLiteral) {
            lit = static_cast<const IntegerLiteral*>(u->operand.get());
            outerNeg = true;
        }
    }
    if (lit == nullptr) return std::nullopt;
    // 一元负号非折叠形态（parser 323 折叠后不可达·防御面）：负号归一进文本，
    //   与 323 折叠的符号归一规则同构（-(-5) 取负抵消等）。
    std::string raw = lit->raw;
    if (outerNeg && (raw.empty() || raw[0] != '-')) raw = "-" + raw;
    // 1068（910 邻接·遗留收口）：判定切 types::integerLiteralRuntimeType 单点
    //   （910 就绪·声明位/赋值/返回/静态/推断五面已收）——全形态（十/0x/0b/0o
    //   + 后缀 + 符号）经 normalizeIntLiteralToDecimal 归一按数学值域提升：
    //   无后缀 超 int32 双域→整64、超 int64 双域→整128（728g 负号判据含于
    //   归一文本）；后缀族 整64→整128（超 int64）、正32→正64→正128（超 u32/
    //   u64）。原链对进制文本误判：textExceedsInt64 十进制长度判据
    //   （0x8000000000000000 18 字符<19 判不超）+ literalValueExceedsInt32Domain
    //   (lit->value) 值级毒化（parser parseIntValue stoll 对超承载进制文本
    //   溢出静默回 0）→ 回绕乘(0x8000000000000000, 1) 参与域留整32、IR 按
    //   resolvedType Cast 截断出 0（应 2^63=9223372036854775808·910 后 IR/物化
    //   面已按单点整128 装载=域错位）；转义超 u128 域（归一空串）由
    //   visitIntegerLiteral 统一「超出…范围」诊断拒（单点同判据·零分叉）。
    return types::integerLiteralRuntimeType(raw, nullptr);
}

// 回绕内置识别（表达式位特判，占位注册已防重定义）：true = 本调用是回绕内置，
//   wrapOpIdx 写 0/1/2（加/减/乘）。
static bool wrapBuiltinIndexOf(const std::string& name, int& opIdx) {
    if (name == "回绕加") { opIdx = 0; return true; }
    if (name == "回绕减") { opIdx = 1; return true; }
    if (name == "回绕乘") { opIdx = 2; return true; }
    return false;
}

// 回绕内置（任务 474）：显式两补码回绕算术——运行期错误码 8 溢出检查的显式
//   豁免面（Rust wrapping_add/sub/mul 对照·整16: 回绕加(32767,1)≡-32768）。
// 类型规则与 + - * 完全一致：①公共类型提升（整型取秩高者）；②混合符号拒绝
//   （2026-09-10 方案A·字面量豁免）；③285 窄域字面量豁免（字面量按另一侧窄
//   类型参与→结果保持窄域）。
// true = 已处理（含报错形态）。
bool SemanticAnalyzer::checkWrapBuiltinCall(CallExpr* node) {
    int opIdx = -1;
    if (node->callee->getType() != NodeType::IdentifierExpr) return false;
    if (!wrapBuiltinIndexOf(
            static_cast<IdentifierExpr*>(node->callee.get())->name, opIdx)) {
        return false;
    }
    if (node->arguments.size() != 2) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "回绕内置须恰为两个整数实参：回绕加(a, b)/回绕减(a, b)/"
                            "回绕乘(a, b)");
        lastType_ = "未知";
        return true;
    }
    std::string leftType = checkExpr(node->arguments[0].get());
    std::string rightType = checkExpr(node->arguments[1].get());
    if (isArrayType(leftType)) leftType = types::arrayElemOf(leftType) + "*";
    if (isArrayType(rightType)) rightType = types::arrayElemOf(rightType) + "*";
    // 整数操作数（浮点无回绕语义；指针/布尔/字符串均拒绝）
    if (!isInteger(leftType) || !isInteger(rightType) ||
        leftType == "未知" || rightType == "未知") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "回绕内置要求整数操作数，实际为 '" + leftType +
                            "' 与 '" + rightType + "'");
        lastType_ = "未知";
        return true;
    }
    // 535/1068：字面量实参参与域按值提升（420 立法·001 §3.7a「回绕与算术
    //   运算符同类型规则」）——修前标记域恒整32：回绕乘(3000000000, 1) 按整32
    //   位域截断折叠出 -1294967296，与同型普通算术 3000000000+0=3000000000
    //   （420 面）及 v2（字面量寄存器值域选型）分叉；1068 起进制/后缀超域
    //   形态经 wrapLiteralOperandType 单点收口（整128 档可达·超 128 域由
    //   字面量自身诊断拒）。变量实参不受影响（wrapLiteralOperandType 非字面量
    //   返回 ""，保持 checkExpr 原类型——791 变量矩阵零变化）。
    {
        // 1360 结果化批3B：无值=非字面量形态（保持 checkExpr 原类型——has_value
        //   分流取代原 empty() 判断）
        const cn::Optional<std::string> pL = wrapLiteralOperandType(node->arguments[0].get());
        if (pL.has_value()) leftType = pL.value();
        const cn::Optional<std::string> pR = wrapLiteralOperandType(node->arguments[1].get());
        if (pR.has_value()) rightType = pR.value();
    }
    // 混合符号拒绝对齐 + - *（2026-09-10 方案A·字面量豁免，Rust 对齐）
    if (types::isUnsigned(leftType) != types::isUnsigned(rightType) &&
        !isIntLiteralExpr(node->arguments[0].get()) &&
        !isIntLiteralExpr(node->arguments[1].get())) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "混合符号运算禁止：'" + leftType + "' 与 '" + rightType +
                            "' —— 须显式转换（如 整64(表达式)/正64(表达式)）；"
                            "字面量豁免（Rust 对齐，2026-09-10 方案A）");
    }
    // 285 窄域字面量豁免（与 checkArithmeticBinary 同款）：字面量按另一侧窄
    //   类型参与，结果保持窄域
    const auto isNarrowDomain = [](const std::string& t) {
        return t == "整8" || t == "整16" || t == "正8" || t == "正16";
    };
    if (isNarrowDomain(rightType) && !isNarrowDomain(leftType) &&
        isIntLiteralExpr(node->arguments[0].get())) {
        leftType = rightType;
    } else if (isNarrowDomain(leftType) && !isNarrowDomain(rightType) &&
               isIntLiteralExpr(node->arguments[1].get())) {
        rightType = leftType;
    }
    // 648 归档（commonNumericType 终态消费面）：上方守卫（整数性检查）已把
    //   非整数/"未知" 操作数报 Error 并早退——到此处双操作数均为已验证整数
    //   （isInteger → intRank>0），commonNumericType 的 "整32" miss 终态在本
    //   调用点不可达；非法值面已由守卫报错，非静默。
    lastType_ = commonNumericType(leftType, rightType);
    node->resolvedType = lastType_;  // IR 层 visitCallExpr 回绕展开分派依据
    return true;
}

} // namespace cn_compiler
