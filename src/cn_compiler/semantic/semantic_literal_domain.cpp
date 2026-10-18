// CN 语义分析器——整数字面量值域判域族（728g 自 semantic_expr.cpp 拆出：
//   拆前该文件 1000 行贴 §3 硬红线，按「先拆再加」纪律先拆本族再落 #728g
//   修正——拆出后 877 行）。
//   家族 = 319-a T36 值域门槛（intLiteralFitsType） + 558 十进制文本级判域
//   + #728g 负号折叠形态与无符号目标收口 + 756 枚举槽 i32 域（值源=枚举成员
//   常量/目标=枚举类型两枚扩展·2026-10-18 用户裁决）。
//   910（超64域口径收口）：判定核心收口 types::normalizeIntLiteralToDecimal /
//   decTextFits* 单点（原匿名 decimalDigitsOf/decTextLe 族删除——十进制面
//   逐字等价，进制面补齐，见 intLiteralFitsType 注释）。
#define _CRT_SECURE_NO_WARNINGS
#include <functional>
#include <string>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// 919-final（1076·用户裁决「字面量单点判域·Rust 口径」）前置声明：子树扫描
//   自由函数（定义在 intLiteralFitsType 之后·判域经回调复用单点·防散写第二份
//   判域）。回调签名=bool(const Expr*, const std::string&)——调用方传
//   intLiteralFitsType（成员 static 私有，类内 lambda 可访问）。
static bool scanSubtreeLiteralBeyond919(
    const Expr* e, const std::string& to, int depth,
    const std::function<bool(const Expr*, const std::string&)>& fits);

// 319-a（T36·方案甲·用户批量裁决）：整数字面量值域检查——窄化豁免的值域门槛
//（整16 甲=40000 超 32767：原静默截断 -25536 与表达式面〔0+40000 拒〕口径
// 分裂；Rust 统一严格：超域硬错误，域内保留豁免〔整8 a=10 合法〕）。
// 非整数类型恒真（豁免面 isInteger 已前置·#648 豁免边界声明：非整数目标不经
// 本判域，域检查只对整数目标生效）。
// 558（506甲）：文本级判域前置——超 int64 承载字面量（19~20 位）在 parser
//   parseIntValue 已回绕/失真（stoll 失败回 0），值级判定读不到真实值；
//   文本比较不受承载限制（整32/整64/正64 受检面·负号形态按负值判域）。
// 910（超64域口径收口·759 探针实锚）：原「十进制文本级 + 非十进制回退值级」
//   双路径中，进制回退读 lit->value 而 parseIntValue 对超承载进制文本
//   （0x8000000000000000 等 16 位 hex）stoll 溢出静默回 0 → 域外对任何目标
//   恒判域内放行（整64/整32/推断通道各静默错值或巨额立即数）。现全形态
//   （十/0x/0b/0o + 后缀 + 符号）收口 types::normalizeIntLiteralToDecimal
//   归一为规范十进制文本，再走同一「长度+字典序」大数安全判域；超 u128 域
//   文本（归一空串）对一切整型目标超域 → 拒（原「检不出放行」防御面同灭——
//   词法层已保证合法整数 token，能到此处必为真实超域文本）。
bool SemanticAnalyzer::intLiteralFitsType(const Expr* e, const std::string& to) const {
    if (e == nullptr) return true;
    // 提取裸字面量（含一元负号形态——-40000 对 整16 同超域）
    bool neg = false;
    const IntegerLiteral* lit = nullptr;
    if (e->getType() == NodeType::IntegerLiteral) {
        lit = static_cast<const IntegerLiteral*>(e);
    } else if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->op == Operator::Subtract && !u->postfix &&
            u->operand != nullptr &&
            u->operand->getType() == NodeType::IntegerLiteral) {
            lit = static_cast<const IntegerLiteral*>(u->operand.get());
            neg = true;
        }
    }
    // 756（2026-10-18 用户裁决·枚举值域门槛）：值源扩一枚——枚举成员常量
    //   直存形态（枚举名.成员，如 E.甲；E 为枚举类型名且成员存在）与字面量
    //   同族判域：成员值=整64（parseEnumMemberValue/computeEnumValues 两补码
    //   回绕口径），转规范十进制文本后走下方同一 decTextFits* 判据单点。
    //   非枚举形态（结构体字段访问/变量成员等）不属判据面——下落 919 通道
    //   /统一主链（与 756 原「非枚举成员常量形态放行」边界等价）。
    std::string enumDec756;
    if (lit == nullptr && e->getType() == NodeType::MemberExpr) {
        const MemberExpr* m = static_cast<const MemberExpr*>(e);
        if (m->object != nullptr &&
            m->object->getType() == NodeType::IdentifierExpr) {
            const IdentifierExpr* obj =
                static_cast<const IdentifierExpr*>(m->object.get());
            std::int64_t v = 0;
            if (isEnumType(obj->name) &&
                enumValueOf(obj->name, m->memberName, v)) {
                enumDec756 = std::to_string(v);
            }
        }
    }
    std::string dec;
    if (lit != nullptr) {
        dec = types::normalizeIntLiteralToDecimal(lit->raw);
    } else if (!enumDec756.empty()) {
        // 756 枚举成员常量命中：走下方统一判域（不经 919 承载内放行——
        //   枚举槽 i32 域判据须对成员值生效，值域门槛 756 语义。）
        dec = enumDec756;
    } else if (isEnumType(to)) {
        // 静默豁免业务值: 枚举槽非常量直存形态=756 判据面外放行（表达式/标识符/调用交 canConvert 主链·与 v2 枚举槽分支同口径）
        return true;
    } else {  // 静默豁免业务值: 919 轮非常量形态判定（返回真=交既有转换主链判定·非静默默认）
        // 919（超承载字面量参与表达式求值双侧静默分叉根治·640 侦察副产品）：
        //   常量表达式形态扩展——全字面量算术表达式（+ - * / %·一元 ±）经
        //   cnEvalConstIntExprText919 文本级大数安全折叠求值，数学值与目标
        //   类型域按本族既有 decTextFits* 单点判定（禁散写第二份判据）。
        //   启用条件=求值结果超 int64 承载（parseIntValue stoll /
        //   cnFoldConstIntExpr505 值级承载域外——正是静默毒化/回绕发生面：
        //   探针 `静态 整32 槽 = 99999999999999999999+0` 宿主顶层槽 5/函数内
        //   1661992959 vs v2 1661992959 双侧静默错值分叉）；承载内值维持
        //   既有通道（族④a 常量溢出预演/既有隐式转换判定）不动——防与既有
        //   「编译期整数溢出」诊断双报。求值空串=超 u128 域（910 空串口径）
        //   → 落下方统一空串分支（一切整型目标超域拒）。
        // 919-final（2026-10-18 用户裁决「字面量单点判域·Rust 口径」）：
        //   子树扫描加严——表达式（含变量形态）内任一整数字面量数学值超
        //   目标类型域 → 拒（边界2：19 位超 i64 字面量+变量=拒；边界3：
        //   `20位%7` 结果域内不豁免；边界1：超域带后缀字面量同判）。先于
        //   求值面执行（求值面仅剩「字面量全在域内但常量结果超承载」面——
        //   `5000000000*3` 类结果级拒，族④a 链协同）。
        if (scanSubtreeLiteralBeyond919(
                e, to, 0,
                [this](const Expr* x, const std::string& t) {
                    return intLiteralFitsType(x, t);
                })) {
            return false;
        }
        if (!cnEvalConstIntExprText919(e, dec)) return true;  // 静默豁免业务值: 919 轮非常量形态判定（返回真=交既有转换主链判定·非静默默认）
        // 承载内值（int64 域内）：交既有折叠/溢出预演链（族④a 防双报）
        if (!dec.empty() &&
            types::decTextFitsSignedRange(dec, "9223372036854775808",
                                          "9223372036854775807")) {
            return true;
        }
    }
    // 静默豁免业务值: 910 轮整数字面量文本超 u128 域（归一化空串）⇒ 一切整型目标超域 → 拒（业务判定非 miss 默认）
    if (dec.empty()) return false;  // 超 u128 域：一切整型目标皆超（910/919）
    const bool rawNeg = (dec[0] == '-');
    if (rawNeg) dec.erase(0, 1);
    // 728g：负号形态按数学负值判域——parser 323 负号折叠（raw 带 '-'）与
    //   一元负号（值在字面量外）两形态合并为「或」语义（与 728g 同构；
    //   双负形态罕见不作翻转定义）。-2147483648 对 整32 仍域内（855 锚）。
    if (neg || rawNeg) dec.insert(dec.begin(), '-');
    // 756（2026-10-18 用户裁决·枚举值域门槛）：目标域扩一枚——枚举类型按
    //   整32 存储域判（枚举按整32存储；与 v2 侧 枚举槽常量入域756 同口径）。
    const std::string domain = isEnumType(to) ? std::string("整32") : to;
    if (domain == "整8") return types::decTextFitsSignedRange(dec, "128", "127");
    if (domain == "整16") {
        return types::decTextFitsSignedRange(dec, "32768", "32767");
    }
    if (domain == "整32" || domain == "整数") {
        return types::decTextFitsSignedRange(dec, "2147483648", "2147483647");
    }
    if (domain == "整64") {
        return types::decTextFitsSignedRange(dec, "9223372036854775808",
                                             "9223372036854775807");
    }
    // 640（2026-10-16·535 批乙语义恢复扩展）：128 槽文本级域——558/535 原注释
    //   「整128/正128 域宽于任何 int64 承载内文本恒适」对**超 int64 文本**不
    //   成立（39/41 位文本可超 2^127·m642c/d 探针实锚：宿主原仅 IR 层拒/v2
    //   放行错值-1）。整128 合法域 [-2^127, 2^127-1]（负号按负值判）、正128
    //   [0, 2^128-1]。
    if (domain == "整128") {
        return types::decTextFitsSignedRange(
            dec, "170141183460469231731687303715884105728",
            "170141183460469231731687303715884105727");
    }
    if (domain == "正128") {
        return types::decTextFitsUnsignedRange(
            dec, "340282366920938463463374607431768211455");
    }
    if (domain == "正8") return types::decTextFitsUnsignedRange(dec, "255");
    if (domain == "正16") return types::decTextFitsUnsignedRange(dec, "65535");
    if (domain == "正32") {
        return types::decTextFitsUnsignedRange(dec, "4294967295");
    }
    if (domain == "正64") {
        return types::decTextFitsUnsignedRange(dec, "18446744073709551615");
    }
    return true;  // 非整数目标（豁免面 isInteger 已前置）——防御性恒真
}

// 919-final（2026-10-18 用户裁决「字面量单点判域·Rust 口径」·四边界定案）：
//   子树扫描——表达式（含变量等非常量形态）内任一**超 64 位承载**整数字面量
//   （数学值 > 2^63-1 / 负值 < -2^63·parseIntValue 毒化面）超目标类型域即真
//   （调用方拒）。承载内字面量（如 300 对 整8）不属本判据面——由既有链管辖
//   （285/788③ 窄域拉窄=位模式拉窄定义语义·862/1078 锁：`甲8+300`→44 合法；
//   T36 值域门槛/编译期溢出预演同链）——「超 64 位承载」是本轮静默分叉的
//   实质面（1078 回归实锚：全幅度单点判会误伤窄域拉窄）。
//   与既有面关系：
//   ① 判据单点：子节点字面量直接复用 intLiteralFitsType（lit 直路·数学值
//      语义含负号壳与后缀语义），禁散写第二份判域；
//   ② 负号壳（-9223372036854775808=整64::MIN·|值| 恰 2^63≯ 承载上限文本）
//      由 intLiteralFitsType 数学负值面判域（域内=不命中·483 字面量合法面
//      零误伤）；
//   ③ 显式转换（CastExpr）不穿越=用户显式承接/截断意图面（`整64(表达式)`
//      既有截断语义·1106/1318 之外的存量合法面）；调用/标识符/下标/成员
//      不穿越（各自通道独立判域·运行期值）；
//   ④ 覆盖含变量形态=本轮核心扩展（`静态 整64 甲 = 20位 + 变;` 修前放行
//      到运行期窄化，新口径=编译期拒·与「表达式含不含变量无关」）。
static bool scanSubtreeLiteralBeyond919(
    const Expr* e, const std::string& to, int depth,
    const std::function<bool(const Expr*, const std::string&)>& fits) {
    // 静默豁免业务值: 919-final 扫描入口守卫（空节点/深度超限=非扫描面
    // 判定·谓词语义非静默默认；深度上限防病态深嵌套栈溢出）
    if (e == nullptr || depth > 64) {
        return false;
    }
    const NodeType t = e->getType();
    if (t == NodeType::IntegerLiteral ||
        (t == NodeType::UnaryExpr &&
         static_cast<const UnaryExpr*>(e)->operand != nullptr &&
         !static_cast<const UnaryExpr*>(e)->postfix &&
         (static_cast<const UnaryExpr*>(e)->op == Operator::Subtract ||
          static_cast<const UnaryExpr*>(e)->op == Operator::Add) &&
         static_cast<const UnaryExpr*>(e)->operand->getType() ==
             NodeType::IntegerLiteral)) {
        const bool negShell = (t == NodeType::UnaryExpr);
        const auto* lit = static_cast<const IntegerLiteral*>(
            negShell ? static_cast<const UnaryExpr*>(e)->operand.get() : e);
        std::string dec = types::normalizeIntLiteralToDecimal(lit->raw);
        // 静默豁免业务值: 919-final 超 u128 域哨兵（空串=超一切整型域·按超域命中拒·910 口径非静默默认）
        if (dec.empty()) return true;
        // 64 位承载门槛（含负号：负值按 |值| ≤ 2^63）；承载内=非本扫描面
        if (negShell) dec.insert(dec.begin(), '-');
        if (types::decTextFitsSignedRange(dec, "9223372036854775808",
                                          "9223372036854775807")) {
            // 静默豁免业务值: 919-final 承载内字面量（既有链管辖·285 拉窄/
            // T36 值域面·判定语义非静默默认）
            return false;
        }
        return !fits(e, to);
    }
    if (t == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        return scanSubtreeLiteralBeyond919(u->operand.get(), to, depth + 1, fits);
    }
    if (t == NodeType::BinaryExpr) {
        const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
        return scanSubtreeLiteralBeyond919(b->left.get(), to, depth + 1, fits) ||
               scanSubtreeLiteralBeyond919(b->right.get(), to, depth + 1, fits);
    }
    if (t == NodeType::TernaryExpr) {
        const TernaryExpr* t3 = static_cast<const TernaryExpr*>(e);
        return scanSubtreeLiteralBeyond919(t3->condition.get(), to, depth + 1, fits) ||
               scanSubtreeLiteralBeyond919(t3->trueValue.get(), to, depth + 1, fits) ||
               scanSubtreeLiteralBeyond919(t3->falseValue.get(), to, depth + 1, fits);
    }
    if (t == NodeType::AssignmentExpr) {
        const AssignmentExpr* a = static_cast<const AssignmentExpr*>(e);
        return scanSubtreeLiteralBeyond919(a->value.get(), to, depth + 1, fits);
    }
    // 静默豁免业务值: 919-final 扫描闭集穷举（显式转换/调用/标识符/下标/
    // 成员访问=非本扫描面·各自通道独立判域或运行期值）
    return false;
}

// 790（640 面裸全局遗漏补齐·569 采样轮 H1 探针实锚·semantic.cpp/hpp 双
//   1000 红线先拆再加下沉件）：裸全局（569 通道）常量初始化器字面量判域——
//   两条与 640 顶层静态挂点逐字同构：
//   ① 字面量自身 128 域（478 链·IntegerLiteral 形态·含 parser 323 负号折叠
//      的带符号 raw）：提升链/128 域判定收口 types::integerLiteralRuntimeType
//      单点（910：原 textExceedsInt64/i128TextInRange 十进制文本判据对 hex 面
//      误判——`0x8000000000000000` 18 字符不判超/int 文本比较失真）。超域落
//      「整数字面量超出…范围」诊断（超界落发射层零占位=「诊断即错误」闭环）。
//   ② 直存槽值域（640·intLiteralFitsType）：字面量数学值须在槽类型域内——
//      原 640 三形态（顶层静态/函数内静态/类静态字段）不含裸全局，实测分叉：
//      十进制超域（整32 x=99999999999999）/承载内十六进制超域（0x1FFFFFFFF）/
//      正128 负字面量——宿主放行（20 位超 u64 落 ml64 A2071·32 位截断错值）
//      vs v2 535 通道全拒。①已拒时跳过②（互斥防双报）。
void SemanticAnalyzer::checkGlobalVarInitLiteralDomain(
    const std::string& name, const SourceLocation& declLoc, const Expr* init,
    const std::string& slotType) {
    if (init == nullptr) return;
    bool selfRejected = false;
    if (init->getType() == NodeType::IntegerLiteral) {
        const auto* lit = static_cast<const IntegerLiteral*>(init);
        int st910 = 0;
        (void)types::integerLiteralRuntimeType(lit->raw, &st910);
        if (st910 != 0) {
            diagnostics_.report(
                DiagnosticLevel::Error, declLoc,
                "整数字面量超出" +
                    std::string(st910 == 2 ? "正128（2^128-1）"
                                           : "整128（2^127-1）") +
                    "范围");
            selfRejected = true;
        }
    }
    if (selfRejected) return;
    // 919-final：谓词 isIntLiteralExpr→cnExprHasIntLiteral919（含字面量的任意
    //   形态入判——子树扫描 cnSubtreeLiteralBeyond919 在 intLiteralFitsType
    //   内对含变量形态单点判域·四边界口径 2026-10-18 用户裁决）。
    if (cnExprHasIntLiteral919(init) && !intLiteralFitsType(init, slotType)) {
        diagnostics_.report(
            DiagnosticLevel::Error, init->location,
            "静态变量 '" + name + "' 的初始字面量超出 '" + slotType +
                "' 值域——静态槽加载期直存须为值域内字面量（001 §静态成员·535）；"
                "如确需位模式截断请显式转换（如 " + slotType + "(表达式)）");
    }
}

}  // namespace cn_compiler
