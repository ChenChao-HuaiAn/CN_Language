// CN 语义层共享纯函数助手（D1 行数整改 115-a·346 重构D 挪入 model 中立层——无状态 inline 纯函数·零分析器状态依赖·semantic/ir/model 三方共用）
//   背景：原 11 个 helper 在 semantic.cpp / semantic_call.cpp / semantic_decl.cpp /
//   semantic_expr.cpp / semantic_stmt.cpp 各复制一份（55 处定义、~600 行重复）——
//   本头文件收敛为单一定义（Rust 对照：rustc 的 crate 内共享工具模块）。
//   各文件去掉匿名 ns 副本后 include 本头，调用点零改动（同名可见）。
//   inline：头文件多 TU 安全（ODR 齐一）；原匿名 ns 内部链接语义由 inline 等价承接。
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ===== P3-22 常量求值助手（#286 起 semantic_constlen.cpp 共用·自 semantic.cpp
//   匿名 ns 提升 inline——多 TU 安全 ODR 齐一，D1 收敛头同纪律）=====
// 剥离数值字面量后缀（U/LL 等）
[[maybe_unused]] inline std::string cnStripLiteralSuffix(const std::string& s) {
    std::string r = s;
    while (!r.empty() && (r.back() == 'U' || r.back() == 'u' ||
                          r.back() == 'L' || r.back() == 'l')) r.pop_back();
    return r;
}

[[maybe_unused]] inline bool cnParseInt(const std::string& t, long long& v) {
    const std::string s = cnStripLiteralSuffix(t);
    if (s.empty()) return false;
    try {
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            v = std::stoll(s.substr(2), nullptr, 16);
        else
            v = std::stoll(s, nullptr, 10);
        return true;
    } catch (...) { return false; }
}

[[maybe_unused]] inline bool cnIsFloatText(const std::string& t) {
    const std::string s = cnStripLiteralSuffix(t);
    return s.find('.') != std::string::npos || s.find('e') != std::string::npos ||
           s.find('E') != std::string::npos;
}


// 比较运算符（== != < > <= >=）
[[maybe_unused]] inline bool isComparisonOp(Operator op) {
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
[[maybe_unused]] inline bool isLogicalOp(Operator op) {
    switch (op) {
        case Operator::AndAnd: case Operator::OrOr: case Operator::Bang:
            return true;
        default:
            return false;
    }
}

// 位运算符（& | ^ ~ << >>）
[[maybe_unused]] inline bool isBitwiseOp(Operator op) {
    switch (op) {
        case Operator::Amp: case Operator::Pipe: case Operator::Caret:
        case Operator::Tilde: case Operator::LessLess: case Operator::GreaterGreater:
            return true;
        default:
            return false;
    }
}

// 算术运算符（+ - * / %）
[[maybe_unused]] inline bool isArithmeticOp(Operator op) {
    return op == Operator::Add || op == Operator::Subtract ||
           op == Operator::Multiply || op == Operator::Divide ||
           op == Operator::Modulo;
}

// 指针类型辅助（Task 2.4）：是否指针类型 / 是否数组类型
[[maybe_unused]] inline bool isPointerType(const std::string& type) {
    return types::isPointer(type);
}
[[maybe_unused]] inline bool isArrayType(const std::string& type) {
    return types::isArray(type);
}
// 字符串语义类型（字符串/字符*）：+ 参与按字符串拼接分派（Task 2.5/2.9）。
//   字符* 在 IR 层同为 ptr、类型文本以 '*' 结尾，但语义是字符串视图——指针类
//   检查（安全区边界·指针算术等）须据此排除（plans/022 波 1：宿主 check v2 树
//   75 处 字符* 拼接被按「指针算术」误报的根治；拼接分派与指针检查共用本判定，
//   保证永不漂移）。
[[maybe_unused]] inline bool isStringSemanticType(const std::string& type) {
    return type == "字符串" || type == "字符*";
}
// 计算数组总字节大小（元素大小 × 长度）
// GCC -Wunused-function 下标记 maybe_unused（MSVC 不报，GCC 严格）
[[maybe_unused]] inline int arrayTotalSize(const std::string& type) {
    const int len = types::arrayLenOf(type);
    const int elemSize = types::typeSize(types::arrayElemOf(type));
    if (len <= 0 || elemSize <= 0) return 0;
    return len * elemSize;
}

// 类型别名规范化：整数 -> 整32、小数 -> 浮64（规格书02-类型系统：默认类型别名）
// Task 2.3：转发到 type_system 子模块（types::canonical），语义与IR共用同一实现
inline std::string canonicalType(const std::string& type) {
    return types::canonical(type);
}

// ==================== 函数指针类型工具（Task 2.2） ====================

// 判断类型字符串是否为函数指针类型（函数指针<返回>(参数,...)）
[[maybe_unused]] inline bool isFuncPtrTypeStr(const std::string& type) {
    return type.rfind("函数指针<", 0) == 0;
}

// 从函数指针类型字符串提取返回类型（"函数指针<整32>(整32,整32)" -> "整32"）
[[maybe_unused]] inline std::string funcPtrReturn(const std::string& type) {
    std::size_t lt = type.find('<');
    std::size_t gt = type.find('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    return type.substr(lt + 1, gt - lt - 1);
}

// 从函数指针类型字符串提取参数类型列表
// "函数指针<整32>(整32,整32)" -> ["整32","整32"]
// 337-a（T53 家系）：实现下沉 types::funcPtrParamsOf（唯一实现）——本函数保留
//   为语义层内部名（调用点零改动），与 IR 层间接调用点共用同一份解析逻辑。
[[maybe_unused]] inline std::vector<std::string> funcPtrParams(const std::string& type) {
    return types::funcPtrParamsOf(type);
}

// ===== P3-22 常量求值助手族（#286 起 semantic_constlen.cpp 共用·自 semantic.cpp
//   匿名 ns 整段提升 inline——多 TU 安全 ODR 齐一，D1 收敛头同纪律）=====
// 剥离数值字面量后缀（U/LL 等）
[[maybe_unused]] inline std::string cnFormatDouble(double d) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%g", d);
    return buf;
}

// 折叠二元常量：整数/浮点算术 + 字符串字面量拼接
[[maybe_unused]] inline std::string cnFoldConstBinary(Operator op, const std::string& l, const std::string& r) {
    if (cnIsFloatText(l) || cnIsFloatText(r)) {
        const double a = std::atof(cnStripLiteralSuffix(l).c_str());
        const double b = std::atof(cnStripLiteralSuffix(r).c_str());
        double c = 0;
        switch (op) {
            case Operator::Add: c = a + b; break;
            case Operator::Subtract: c = a - b; break;
            case Operator::Multiply: c = a * b; break;
            case Operator::Divide: if (b == 0) return ""; c = a / b; break;
            case Operator::Modulo: return "";
            default: return "";
        }
        return cnFormatDouble(c);
    }
    long long a, b;
    if (!cnParseInt(l, a) || !cnParseInt(r, b)) {
        // 字符串字面量拼接（"a" + "b" -> "ab"）
        if (!l.empty() && !r.empty() && l.front() == '"' && r.front() == '"') {
            return l.substr(0, l.size() - 1) + r.substr(1);
        }
        return "";
    }
    long long c = 0;
    switch (op) {
        case Operator::Add: c = a + b; break;
        case Operator::Subtract: c = a - b; break;
        case Operator::Multiply: c = a * b; break;
        case Operator::Divide:
            if (b == 0) return "";
            // 1016（审查 P1-2·存量崩溃）：INT64_MIN / -1 在 x86 idiv 溢出
            //   （0xC0000409 无诊断脏崩）——折叠层按不可折叠失败返回，交调用方
            //   诊断通道（与 cnFoldConstIntExpr505 同口径）。
            if (a == INT64_MIN && b == -1) return "";  // 静默豁免业务值: 折叠失败哨兵（调用方诊断通道）
            c = a / b;
            break;
        case Operator::Modulo:
            if (b == 0) return "";
            // 1016：INT64_MIN % -1 同族（数学值 0，但 idiv 指令先溢出）
            if (a == INT64_MIN && b == -1) return "";  // 静默豁免业务值: 折叠失败哨兵（调用方诊断通道）
            c = a % b;
            break;
        default: return "";
    }
    return std::to_string(c);
}

// 递归求值顶层常量表达式；成功返回 true 并输出值文本（供引用处重写）
[[maybe_unused]] inline bool cnEvalConstExpr(const std::unordered_map<std::string, std::string>& vals,
                     const std::string& curMod, Expr* e, std::string& out) {
    if (e == nullptr) return false;
    switch (e->getType()) {
        case NodeType::IntegerLiteral: {
            // 790（569 采样轮 H1）：整数字面量数值化——原 raw 原文透传经 569
            //   constInitText 直存 .data，0x/0b/0o/L 后缀形态触发汇编器拒绝
            //   （win MASM A2206/A2048·linux GAS .quad ...L 同病）。数值化=
            //   十进制规范文本（与常量算术折叠 cnFoldConstBinary 输出同形态·
            //   三后端通存）；超 u64 域文本（128 位字面量）原文保真——128 位
            //   直存通道 parseInt128InitText 原生支持四形态解析。
            const std::string raw = static_cast<IntegerLiteral*>(e)->raw;
            if (!types::foldIntLiteralText(raw, out)) out = raw;
            return true;
        }
        case NodeType::FloatLiteral: out = static_cast<FloatLiteral*>(e)->raw; return true;
        case NodeType::StringLiteral: out = static_cast<StringLiteral*>(e)->raw; return true;
        case NodeType::BoolLiteral:
            // 241-a（D14 根治）：布尔字面量常量表达式——值文本与 IR 布尔常量同口径（真/假）
            out = static_cast<BoolLiteral*>(e)->raw;
            return true;
        case NodeType::IdentifierExpr: {
            const std::string n = static_cast<IdentifierExpr*>(e)->name;
            auto it = vals.find(n);
            if (it != vals.end()) { out = it->second; return true; }
            if (!curMod.empty()) {
                auto itq = vals.find(curMod + "$" + n);
                if (itq != vals.end()) { out = itq->second; return true; }
            }
            return false;
        }
        case NodeType::UnaryExpr: {
            UnaryExpr* u = static_cast<UnaryExpr*>(e);
            if (u->postfix) return false;
            if (u->op == Operator::Subtract) {
                std::string v;
                if (!cnEvalConstExpr(vals, curMod, u->operand.get(), v)) return false;
                if (cnIsFloatText(v)) {
                    out = cnFormatDouble(-std::atof(cnStripLiteralSuffix(v).c_str()));
                } else {
                    long long iv;
                    if (!cnParseInt(v, iv)) return false;
                    // 1016（审查 P1-2 同族）：-INT64_MIN 超出 int64 域（C++
                    //   有符号溢出 UB）——按不可折叠失败返回交诊断通道。
                    if (iv == INT64_MIN) return false;
                    out = std::to_string(-iv);
                }
                return true;
            }
            if (u->op == Operator::Add)
                return cnEvalConstExpr(vals, curMod, u->operand.get(), out);
            return false;
        }
        case NodeType::BinaryExpr: {
            BinaryExpr* b = static_cast<BinaryExpr*>(e);
            std::string l, r;
            if (!cnEvalConstExpr(vals, curMod, b->left.get(), l)) return false;
            if (!cnEvalConstExpr(vals, curMod, b->right.get(), r)) return false;
            out = cnFoldConstBinary(b->op, l, r);
            return !out.empty();
        }
        default:
            return false;
    }
}


// ===== 静态类型判据/工具族（346 重构D 自 SemanticAnalyzer 静态方法转自由函数——ir/codegen/semantic 三方共用·原 78+ 处 SemanticAnalyzer:: 前缀调用点全树去前缀）=====
[[maybe_unused]] inline bool isResultType(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (!suffix.empty()) return false;
    if (core.rfind("结果<", 0) != 0) return false;
    if (core.find('>') == std::string::npos) return false;
    // 067-002 治本（嵌套合成体同族）：顶层逗号判据须平衡扫描（<> 深度）——
    //   原 find(',') 见任意逗号即真，「结果<映射<整64,整64>>」（仅内层逗号）
    //   误判为结果类型。合法结果恒含顶层逗号，本判据等价收紧。
    int depth = 0;
    for (char ch : core) {
        if (ch == '<') { ++depth; }
        else if (ch == '>') { --depth; if (depth < 0) break; }
        else if (ch == ',' && depth == 1) { return true; }
    }
    return false;
}

[[maybe_unused]] inline std::vector<std::string> resultTypeArgs(const std::string& type) {
    std::vector<std::string> result;
    if (!isResultType(type)) return result;
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return result;
    const std::string inner = type.substr(lt + 1, gt - lt - 1);
    // 2026-08-30 根治：嵌套泛型实参（结果<映射<整64, 整64>, 整32>）的逗号
    //   须平衡扫描——原 find(',') 在 映射<整64, 整64> 内部逗号处误切，
    //   t 截断成 映射<整64（成员访问报「映射<整64 不是类类型」）。
    std::size_t comma = std::string::npos;
    {
        int depth = 0;
        for (std::size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '<') depth++;
            else if (inner[i] == '>') depth--;
            else if (inner[i] == ',' && depth == 0) { comma = i; break; }
        }
    }
    if (comma == std::string::npos) return result;
    const std::string t = inner.substr(0, comma);
    const std::string e = inner.substr(comma + 1);
    // 去除首尾空白
    auto trim = [](const std::string& s) -> std::string {
        std::size_t b = s.find_first_not_of(" \t");
        if (b == std::string::npos) return "";
        std::size_t en = s.find_last_not_of(" \t");
        return s.substr(b, en - b + 1);
    };
    result.push_back(trim(t));
    result.push_back(trim(e));
    return result;
}

[[maybe_unused]] inline bool isOptionalType(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (!suffix.empty()) return false;
    if (core.rfind("可选<", 0) != 0) return false;
    if (core.find('>') == std::string::npos) return false;
    // 067-002 治本（嵌套合成体同族·m1 最小复现）：原 find(',')==npos 见任意
    //   逗号即假——内层实参「结果<整32,整32>」的逗号被误判为「可选有两参」，
    //   「可选<结果<整32,整32>>」不被识别（成员访问报「不是结构体/联合体/
    //   类类型」）。可选恒单参：顶层（深度1）不得有逗号。
    int depth = 0;
    for (char ch : core) {
        if (ch == '<') { ++depth; }
        else if (ch == '>') { --depth; if (depth < 0) break; }
        else if (ch == ',' && depth == 1) { return false; }
    }
    return true;
}

[[maybe_unused]] inline std::string optionalTypeArg(const std::string& type) {
    if (!isOptionalType(type)) return "";
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    std::string t = type.substr(lt + 1, gt - lt - 1);
    std::size_t b = t.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    std::size_t en = t.find_last_not_of(" \t");
    return t.substr(b, en - b + 1);
}

[[maybe_unused]] inline std::string optionalStructName(const std::string& t) {
    return "可选$" + types::canonical(t);
}

[[maybe_unused]] inline std::string resultStructName(const std::string& t, const std::string& e) {
    return "结果$" + types::canonical(t) + "$" + types::canonical(e);
}

[[maybe_unused]] inline std::string canonicalizeSyntheticArgText(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (isResultType(core)) {
        const std::vector<std::string> args = resultTypeArgs(core);
        if (args.size() == 2) {
            return resultStructName(types::canonical(args[0]),
                                    types::canonical(args[1])) + suffix;
        }
    } else if (isOptionalType(core)) {
        const std::string arg = optionalTypeArg(core);
        if (!arg.empty()) {
            return optionalStructName(types::canonical(arg)) + suffix;
        }
    }
    return type;
}

[[maybe_unused]] inline bool isTransferCall(const CallExpr* node) {
    return node != nullptr &&
           node->callee->getType() == NodeType::IdentifierExpr &&
           static_cast<const IdentifierExpr*>(node->callee.get())->name == "转移" &&
           node->arguments.size() == 1;
}

// 923（第三处出现前收口）：复制 内置调用判定——callee=标识符「复制」+ 单实参
//   + 语义层已推导 resolvedType（与 ir_call.cpp 生成特判同口径）。原为
//   ir_expr_member.cpp 文件级 static（234-a 首处·structWholeAssignSrcFirst/
//   emitStructWholeAssign 两处引用）；923 元素面接管通道需第三处判定 → 提炼
//   为公共单点，原 static 删除改调本函数（判据逐字搬运·零行为变更）。
[[maybe_unused]] inline bool isCopyBuiltinCall(const Expr* node) {
    if (node == nullptr || node->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(node);
    return call->callee->getType() == NodeType::IdentifierExpr &&
           static_cast<const IdentifierExpr*>(call->callee.get())->name == "复制" &&
           call->arguments.size() == 1 &&
           !call->resolvedType.empty();
}

[[maybe_unused]] inline bool isOwnedStringBuiltin(const std::string& name) {
    // 与 IR 层字符串拥有判定白名单同一集合（原 ir_expr_assign_ident.cpp
    // identifierStringOwnAssign / ir_stmt_decl.cpp 拥有型初始化两处硬编码——
    // 094 收口为单点，IR 层两处改调本方法，判据漂移根除）。
    return name == "字符串复制" || name == "字符串连接" ||
           name == "字符串拼接" || name == "字符串子串" ||
           name == "字符串大写" || name == "字符串小写" ||
           name == "字符串修剪" || name == "字符串反转";
}

[[maybe_unused]] inline bool isBorrowViewMethod(const std::string& m) {
    return m == "元素" || m == "读取" || m == "栈顶" || m == "队首" ||
           m == "头部元素" || m == "读取头部" || m == "读取尾部" || m == "获取";
}

[[maybe_unused]] inline std::string functionLinkKey(const std::string& moduleName,
                                              const std::string& funcName,
                                              const std::string& sigKey) {
    if (moduleName.empty() || moduleName == "主" || funcName == "主" ||
        moduleName.rfind("__cn_", 0) == 0) {
        return sigKey;
    }
    return moduleName + "$" + sigKey;
}


// ===== IR 层文本工具声明（357 自 ir.hpp 下沉——纯字符串/AST 工具·functionLinkKey 同族）=====
[[maybe_unused]] inline std::string funcPtrAwareSrcType(const FuncPtrTypeInfo& funcPtr,
                                                        const std::string& typeName) {
    return funcPtr.isFunctionPtr() ? funcPtr.toSymbolType() : typeName;
}
[[maybe_unused]] inline std::string paramSrcTypeOf(const ParamDecl* param) {
    if (param == nullptr) return "";
    return funcPtrAwareSrcType(param->funcPtr, param->typeName);
}
std::string methodSymbolKey(const std::string& className, const std::string& sigKey);
int charLiteralCodePoint(const std::string& raw);
// 1137（P0-2 下沉配套）：转义序列/unicode 花括号转义解码（charLiteralCodePoint
//   与字符串解码共用·实现驻 model/char_literal.cpp）
std::string decodeEscapes(const std::string& body);

// ===== 728b：模块限定类键（自 class_resolver.cpp static cclassKey 提升 inline——
//   预降级期类名注册（第一趟a）与 findClass/类成员解析（第一趟d）共用单一键规则；
//   模块为空=裸名（单文件；模块私有类不合并进 Program，合并后类 moduleAccess
//   恒 Public 口径见 registerClassAndInterfaces 注释）=====
[[maybe_unused]] inline std::string classKey(const std::string& module, const std::string& name) {
    return module.empty() ? name : module + "::" + name;
}

// ===== 表达式/语句深度克隆（86 提升公开·非 inline·定义 semantic_stmt.cpp）
//   =====：降级树/泛型实例化/复合赋值糖展开（字符串拼接面 甲 += X ≡ 甲 = 甲 + X
//   的左值副本）共用单一克隆器——防跨翻译单元双份实现漂移。
std::unique_ptr<Expr> cloneExpr(Expr* node);
std::unique_ptr<Stmt> cloneStmt(Stmt* node);

// ===== 548：编译期整数常量折叠器（自 semantic_expr_binary.cpp 匿名 ns 提升
//   中立层共享——505/503/548 三消费方〔溢出预演/移位量/除零定性〕跨文件单份）=====
// 505（G1·501 采样轮）：小型常量折叠求值——编译期预演面专用纯函数（零副作用）。
//   背景：checkConstIntExprSafety/checkShiftConstWidth 旧取值通道只认直接子节点
//   字面量（litValue 两处手写提取），嵌套折叠形态 ((30000+30000)*30000000)、
//   甲<<(700+66)、折叠零除数 10/(5-5) 双侧一致漏编译期检查（运行期错误码 8 兜底）
//   ——预演发生在折叠之前。修法=子树先递归折叠出常量闭包再按域预演（检查提前+
//   精确·不改变运行期兜底语义：无法折叠的形态〔含变量/中间溢出/除零〕返回 假=
//   保持现状走运行期错误码；错误形态由该子表达式自身节点的预演报告——
//   visitBinaryExpr 逐节点递归遍历，无遗漏面）。
//   折叠域：int64 安全闭包——加减乘用溢出条件检测（hi-vR 技巧），除零/INT64_MIN÷-1/
//   移位量越 [0,63] 域/负值左移 返回 假（不可折叠——这些形态各自的节点级检查
//   〔编译期除零/INT_MIN÷-1 溢出/移位量超宽〕按现状口径覆盖，折叠不与之竞争）。
//   503 rebase 归纳：编译期除零定性（除/余右操作数折叠值==0）同样复用本通道。
//   548：折叠通道扩常量名解析——resolveConstName 非空时 IdentifierExpr 经其
//   查顶层常量值折叠为整值（001 §1.1a①「常量传播面随常量折叠后续波」·语义侧
//   SemanticAnalyzer::resolveConstNameValue 提供含 584-a 遮蔽守卫的实现）；空
//   （默认）保持旧口径=不可折叠（溢出预演/移位量通道不传——548 扩面仅限除零
//   定性通道，防溢出/移位面超范围扩面）。
[[maybe_unused]] inline bool cnFoldConstIntExpr505(
    const Expr* e, std::int64_t& out, int depth = 0,
    const std::function<bool(const std::string&, std::int64_t&)>* resolveConstName = nullptr) {
    if (e == nullptr || depth > 64) return false;  // 深度守卫（防病态深嵌套）
    if (e->getType() == NodeType::IntegerLiteral) {
        out = static_cast<const IntegerLiteral*>(e)->value;
        return true;
    }
    if (e->getType() == NodeType::IdentifierExpr) {
        // 548：常量名折叠（仅除零定性通道开启·解析器内含遮蔽守卫与值文本解析）
        if (resolveConstName != nullptr &&
            (*resolveConstName)(static_cast<const IdentifierExpr*>(e)->name, out)) {
            return true;
        }
        return false;
    }
    if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->op == Operator::Subtract && !u->postfix) {
            std::int64_t v = 0;
            if (!cnFoldConstIntExpr505(u->operand.get(), v, depth + 1, resolveConstName))
                return false;
            // 取负按两补码回绕位模式（INT64_MIN 取负=自身·与旧 litValue 同口径）
            out = static_cast<std::int64_t>(~static_cast<std::uint64_t>(v) + 1);
            return true;
        }
        return false;
    }
    if (e->getType() != NodeType::BinaryExpr) return false;
    const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
    std::int64_t l = 0, r = 0;
    if (!cnFoldConstIntExpr505(b->left.get(), l, depth + 1, resolveConstName)) return false;
    if (!cnFoldConstIntExpr505(b->right.get(), r, depth + 1, resolveConstName)) return false;
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

// ===== 919：常量整数表达式文本级求值（超承载字面量求值单点·大数安全） =====
// 640 侦察副产品（超承载十进制字面量参与表达式求值双侧静默分叉根治）：
//   表达式语境常量值求值——文本级大数安全（types::decText*919 十进制串
//   四则），区别于值级折叠通道（cnFoldConstIntExpr505 信任 IntegerLiteral::
//   value——parser parseIntValue 对超 int64 承载文本 std::stoll 溢出静默回
//   0 的毒化值；1034 教训 #232 毒化值多消费点同源）。
//   语义边界（与 001 §506甲/558/638/640 立法链口径一致）：
//     ① 返回 true + out 非空 = 折叠成功（out=规范十进制文本·可带 '-'）；
//     ② 返回 true + out 空串 = 命中超 u128 域哨兵（沿 910
//        normalizeIntLiteralToDecimal 空串口径——中间值或最终值任一超
//        2^128-1 即哨兵）；消费方判「超一切整型域」；
//     ③ 返回 false = 非常量形态（变量/调用/除零/移位/位运算/比较等）——消费
//        方维持既有链（编译期除零/溢出预演/移位量域各专诊断不动）。
//   节点覆盖面：整数字面量（raw 全文归一·含负号折叠 raw）+ 一元 ± 负号链
//   + 二元 + - * / %。消费点=语义层 intLiteralFitsType 表达式扩展
//   （semantic_literal_domain.cpp）——「表达式常量数学值超目标类型域 →
//   编译期拒」判据底座。
[[maybe_unused]] inline bool cnEvalConstIntExprText919(
    const Expr* e, std::string& out, int depth = 0) {
    if (e == nullptr || depth > 64) return false;   // 静默豁免业务值: 919 轮求值器入口判定（空节点/深度超限=非常量形态缺省判假·谓词语义非静默默认；深度守卫防病态深嵌套同步在位）
    if (e->getType() == NodeType::IntegerLiteral) {
        out = types::normalizeIntLiteralToDecimal(
            static_cast<const IntegerLiteral*>(e)->raw);
        return true;   // 空串=超 u128 域哨兵（词法已拒非法文本·上游已诊断）
    }
    if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->postfix) return false;
        if (u->op != Operator::Subtract && u->op != Operator::Add) return false;
        std::string v;
        if (!cnEvalConstIntExprText919(u->operand.get(), v, depth + 1)) return false;  // 静默豁免业务值: 919 轮求值器非常量形态传播（子节点不可折叠=整体非常量·谓词语义）
        if (v.empty()) {  // 静默豁免业务值: 919 轮求值器超 u128 域哨兵传播（空串=显式哨兵·调用方按超域拒）
            out.clear();
            return true;
        }
        out = (u->op == Operator::Subtract) ? types::decTextNegate919(v) : v;
        return true;
    }
    if (e->getType() != NodeType::BinaryExpr) return false;
    const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
    std::string l, r;
    if (!cnEvalConstIntExprText919(b->left.get(), l, depth + 1)) return false;  // 静默豁免业务值: 919 轮求值器非常量形态传播（子节点不可折叠=整体非常量·谓词语义）
    if (!cnEvalConstIntExprText919(b->right.get(), r, depth + 1)) return false;  // 静默豁免业务值: 919 轮求值器非常量形态传播（子节点不可折叠=整体非常量·谓词语义）
    if (l.empty() || r.empty()) {  // 静默豁免业务值: 919 轮求值器超 u128 域哨兵传播（空串=显式哨兵·调用方按超域拒）
        out.clear();
        return true;
    }
    switch (b->op) {
        case Operator::Add: out = types::decTextAdd919(l, r); return true;
        case Operator::Subtract: out = types::decTextSub919(l, r); return true;
        case Operator::Multiply: out = types::decTextMul919(l, r); return true;
        case Operator::Divide:
            // 除零=不可折叠交既有编译期除零诊断链（503·此处 return false）
            if (r == "0") return false;
            out = types::decTextDiv919(l, r);
            return true;
        case Operator::Modulo:
            if (r == "0") return false;
            out = types::decTextMod919(l, r);
            return true;
        default:
            // 移位/位运算/比较/逻辑不进本族（547 巨移位量/474 位宽域各现有通道）
            return false;
    }
}

// 919-final（2026-10-18 用户裁决「字面量单点判域·Rust 口径」）：表达式子树
//   含整数字面量谓词——值域门槛适用面扩展（含变量形态也判「子树内字面量
//   数学值 vs 目标类型域」，判据主体=语义层 intLiteralFitsType 子树扫描
//   单点 cnSubtreeLiteralBeyond919）。覆盖=字面量/一元 ± 链/二元运算
//   （+ - * / % 及位/移位/比较等一切二元——子树内字面量不因运算种类豁免）；
//   不穿越显式转换（CastExpr=用户显式承接/截断意图·1106/1318 后既有语义面）
//   与调用/标识符/下标/成员访问（各自通道独立判域·非本扫描面）。刻意为
//   cnIsConstIntExpr919（全常量可折叠形态）的超集：调用点以本谓词替换即
//   完成「字面量∪常量表达式→含字面量的任意形态」适用范围扩展。
[[maybe_unused]] inline bool cnExprHasIntLiteral919(const Expr* e,
                                                    int depth = 0) {
    // 静默豁免业务值: 919-final 谓词入口守卫（空节点/深度超限=非扫描面
    // 判定·谓词语义非静默默认；深度上限防病态深嵌套栈溢出）
    if (e == nullptr || depth > 64) {
        return false;
    }
    switch (e->getType()) {
        case NodeType::IntegerLiteral:
            return true;
        case NodeType::UnaryExpr: {
            const auto* u = static_cast<const UnaryExpr*>(e);
            return cnExprHasIntLiteral919(u->operand.get(), depth + 1);
        }
        case NodeType::BinaryExpr: {
            const auto* b = static_cast<const BinaryExpr*>(e);
            return cnExprHasIntLiteral919(b->left.get(), depth + 1) ||
                   cnExprHasIntLiteral919(b->right.get(), depth + 1);
        }
        case NodeType::TernaryExpr: {
            const auto* t = static_cast<const TernaryExpr*>(e);
            return cnExprHasIntLiteral919(t->condition.get(), depth + 1) ||
                   cnExprHasIntLiteral919(t->trueValue.get(), depth + 1) ||
                   cnExprHasIntLiteral919(t->falseValue.get(), depth + 1);
        }
        case NodeType::AssignmentExpr: {
            const auto* a = static_cast<const AssignmentExpr*>(e);
            return cnExprHasIntLiteral919(a->value.get(), depth + 1);
        }
        default:
            // 静默豁免业务值: 919-final 谓词闭集穷举（显式转换/调用/标识符/
            // 下标/成员=非本扫描面·各自通道独立判域）
            return false;
    }
}

// 1073：128 域常量折叠放行——919 文本级大数求值 + 顶层常量引用（对偶
//   cnEvalConstIntExprText919：递归骨架逐支同构，仅增 IdentifierExpr 分支；
//   宿主常量注册回退链专用=需要 vals 上下文）。语义同 919 版：真=常量表达式
//   （out=规范十进制文本·空串=超 u128 域哨兵）；假=非常量（未登记常量引用/
//   非整数常量文本——字符串/浮点/布尔文本经整数文本校验落假，交既有链终态）。
[[maybe_unused]] inline bool cnEvalConstIntExprText919WithVals(
    const std::unordered_map<std::string, std::string>& vals,
    const std::string& curMod, const Expr* e, std::string& out, int depth = 0) {
    if (e == nullptr || depth > 64) return false;  // 静默豁免业务值: 919 轮求值器入口判定（空节点/深度超限=非常量形态缺省判假·谓词语义非静默默认）
    if (e->getType() == NodeType::IdentifierExpr) {
        const std::string n = static_cast<const IdentifierExpr*>(e)->name;
        auto it = vals.find(n);
        if (it == vals.end() && !curMod.empty()) it = vals.find(curMod + "$" + n);
        if (it == vals.end()) return false;  // 静默豁免业务值: 未登记/前向引用常量=非常量形态判假（声明序依赖·与旧链终态一致）
        const std::string& t = it->second;
        if (t.empty()) return false;  // 静默豁免业务值: 常量表空串值不可达（注册链空串=超域拒不入表）——防御判假
        std::size_t i = 0;
        if (t[0] == '-' || t[0] == '+') i = 1;
        if (i >= t.size()) return false;  // 静默豁免业务值: 仅有符号无数字的非法文本=非常量形态判假（词法已拒形态·防御）
        for (; i < t.size(); ++i) {
            if (t[i] < '0' || t[i] > '9') return false;  // 静默豁免业务值: 非整数常量文本（字符串/浮点/布尔=非常量形态判假·与旧链终态一致）
        }
        out = t;
        return true;
    }
    if (e->getType() == NodeType::IntegerLiteral) {
        out = types::normalizeIntLiteralToDecimal(
            static_cast<const IntegerLiteral*>(e)->raw);
        return true;  // 空串=超 u128 域哨兵（词法已拒非法文本·上游已诊断）
    }
    if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->postfix) return false;
        if (u->op != Operator::Subtract && u->op != Operator::Add) return false;
        std::string v;
        if (!cnEvalConstIntExprText919WithVals(vals, curMod, u->operand.get(), v, depth + 1)) return false;  // 静默豁免业务值: 919 轮求值器非常量形态传播（子节点不可折叠=整体非常量·谓词语义）
        if (v.empty()) {  // 静默豁免业务值: 919 轮求值器超 u128 域哨兵传播（空串=显式哨兵·调用方按超域拒）
            out.clear();
            return true;
        }
        out = (u->op == Operator::Subtract) ? types::decTextNegate919(v) : v;
        return true;
    }
    if (e->getType() != NodeType::BinaryExpr) return false;
    const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
    std::string l, r;
    if (!cnEvalConstIntExprText919WithVals(vals, curMod, b->left.get(), l, depth + 1)) return false;  // 静默豁免业务值: 919 轮求值器非常量形态传播（子节点不可折叠=整体非常量·谓词语义）
    if (!cnEvalConstIntExprText919WithVals(vals, curMod, b->right.get(), r, depth + 1)) return false;  // 静默豁免业务值: 919 轮求值器非常量形态传播（子节点不可折叠=整体非常量·谓词语义）
    if (l.empty() || r.empty()) {  // 静默豁免业务值: 919 轮求值器超 u128 域哨兵传播（空串=显式哨兵·调用方按超域拒）
        out.clear();
        return true;
    }
    switch (b->op) {
        case Operator::Add: out = types::decTextAdd919(l, r); return true;
        case Operator::Subtract: out = types::decTextSub919(l, r); return true;
        case Operator::Multiply: out = types::decTextMul919(l, r); return true;
        case Operator::Divide:
            // 除零=不可折叠交既有编译期除零诊断链（503·此处 return false）
            if (r == "0") return false;
            out = types::decTextDiv919(l, r);
            return true;
        case Operator::Modulo:
            if (r == "0") return false;
            out = types::decTextMod919(l, r);
            return true;
        default:
            // 移位/位运算/比较/逻辑不进本族（547 巨移位量/474 位宽域各现有通道）
            return false;
    }
}

} // namespace cn_compiler

