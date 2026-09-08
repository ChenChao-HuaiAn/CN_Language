// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {


namespace {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

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

#pragma GCC diagnostic pop

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

// ==================== P3-22：顶层常量表达式求值（编译期常量折叠） ====================

// 剥离数值字面量后缀（U/LL 等）
std::string cnStripLiteralSuffix(const std::string& s) {
    std::string r = s;
    while (!r.empty() && (r.back() == 'U' || r.back() == 'u' ||
                          r.back() == 'L' || r.back() == 'l')) r.pop_back();
    return r;
}

bool cnParseInt(const std::string& t, long long& v) {
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

bool cnIsFloatText(const std::string& t) {
    const std::string s = cnStripLiteralSuffix(t);
    return s.find('.') != std::string::npos || s.find('e') != std::string::npos ||
           s.find('E') != std::string::npos;
}

std::string cnFormatDouble(double d) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%g", d);
    return buf;
}

// 折叠二元常量：整数/浮点算术 + 字符串字面量拼接
std::string cnFoldConstBinary(Operator op, const std::string& l, const std::string& r) {
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
        case Operator::Divide: if (b == 0) return ""; c = a / b; break;
        case Operator::Modulo: if (b == 0) return ""; c = a % b; break;
        default: return "";
    }
    return std::to_string(c);
}

// 递归求值顶层常量表达式；成功返回 true 并输出值文本（供引用处重写）
bool cnEvalConstExpr(const std::unordered_map<std::string, std::string>& vals,
                     const std::string& curMod, Expr* e, std::string& out) {
    if (e == nullptr) return false;
    switch (e->getType()) {
        case NodeType::IntegerLiteral: out = static_cast<IntegerLiteral*>(e)->raw; return true;
        case NodeType::FloatLiteral: out = static_cast<FloatLiteral*>(e)->raw; return true;
        case NodeType::StringLiteral: out = static_cast<StringLiteral*>(e)->raw; return true;
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

} // namespace

void SemanticAnalyzer::pushScope() {
    scopes_.emplace_back();
    scopeConsts_.emplace_back();  // 缺陷②配套：与 scopes_ 平行维护
}
void SemanticAnalyzer::popScope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
        if (scopeConsts_.size() > 1) scopeConsts_.pop_back();  // 与 scopes_ 同步
    }
}
void SemanticAnalyzer::declareTypeName(const std::string& name, const std::string& module,
                                       const SourceLocation& loc) {
    auto& mods = typeModules_[name];
    if (mods.count(module) > 0) {
        // 同模块重复声明：报错（模块参数为空时按全局重复处理，兼容单模块）
        if (module.empty() && mods.size() > 0) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "重复声明类型 '" + name + "'");
        } else if (!module.empty()) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "重复声明类型 '" + name + "'（模块 '" + module + "' 内）");
        }
        return;
    }
    mods.insert(module);
    typeNames_.insert(name);
}
void SemanticAnalyzer::splitQualifiedType(const std::string& type, std::string& module,
                                          std::string& base) {
    const std::size_t pos = type.find("::");
    if (pos != std::string::npos) {
        module = type.substr(0, pos);
        base = type.substr(pos + 2);
    } else {
        module.clear();
        base = type;
    }
}
const StructDecl* SemanticAnalyzer::findStructInModule(const std::string& module,
                                                       const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    for (const auto& s : program_->structs) {
        if (s->name != name) continue;
        if (module.empty() || s->moduleName == module) return s.get();
    }
    return nullptr;
}
const EnumDecl* SemanticAnalyzer::findEnumInModule(const std::string& module,
                                                   const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    for (const auto& e : program_->enums) {
        if (e->name != name) continue;
        if (module.empty() || e->moduleName == module) return e.get();
    }
    return nullptr;
}

namespace {
// 2026-08-25 H3（嵌套泛型）：找模板实参列表中的顶层逗号（平衡尖括号）——
//   嵌套泛型 向量<映射<整64, 整64>> 内层 '<' 中的 ',' 不是外层分隔。
//   返回逗号位置；无顶层逗号返回 std::string::npos。
std::size_t findTopLevelComma(const std::string& s, std::size_t from) {
    int depth = 0;
    for (std::size_t i = from; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '<') depth++;
        else if (c == '>') { if (depth > 0) depth--; }
        else if (c == ',' && depth == 0) return i;
    }
    return std::string::npos;
}
}  // namespace

std::string SemanticAnalyzer::resolveTypeName(const std::string& type,
                                              const std::string& module,
                                              const SourceLocation& loc) {
    if (type.empty()) return type;
    // 已是限定键（甲::记录）：直接返回（引用方已解析）
    if (type.find("::") != std::string::npos &&
        type.find('<') == std::string::npos &&
        type.find('[') == std::string::npos &&
        type.back() != '*' && type.back() != '&') {
        return type;
    }
    // 模板类型（结果<...>/可选<...>/泛型类<...>）：递归改写内参
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
        const std::string head = type.substr(0, lt);
        const std::string inner = type.substr(lt + 1, gt - lt - 1);
        std::string newInner;
        std::size_t pos = 0;
        while (pos <= inner.size()) {
            const std::size_t comma = findTopLevelComma(inner, pos);  // H3：平衡尖括号
            const std::string part = (comma == std::string::npos)
                                         ? inner.substr(pos)
                                         : inner.substr(pos, comma - pos);
            std::size_t b = part.find_first_not_of(" \t");
            std::size_t e = part.find_last_not_of(" \t");
            const std::string trimmed = (b != std::string::npos && e != std::string::npos)
                                            ? part.substr(b, e - b + 1) : part;
            if (!newInner.empty()) newInner += ",";
            newInner += resolveTypeName(trimmed, module, loc);
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        // 保留 <...> 之后的后缀（函数指针参数列表 (T,T) 等）
        std::string suffix = (gt + 1 < type.size()) ? type.substr(gt + 1) : "";
        return head + "<" + newInner + ">" + suffix;
    }
    // 指针/引用/数组后缀：递归改写基础类型
    if (!type.empty() && (type.back() == '*' || type.back() == '&')) {
        return resolveTypeName(type.substr(0, type.size() - 1), module, loc) +
               type.substr(type.size() - 1);
    }
    const std::size_t lb = type.rfind('[');
    if (lb != std::string::npos && type.back() == ']') {
        const std::string len = type.substr(lb + 1, type.size() - lb - 2);
        if (!len.empty() && len.find_first_not_of("0123456789") == std::string::npos) {
            return resolveTypeName(type.substr(0, lb), module, loc) +
                   type.substr(lb);
        }
    }
    // 裸名：仅当是多模块同名的结构体/枚举时需限定（其余类型原样返回）
    const auto tmodIt = typeModules_.find(type);
    if (tmodIt == typeModules_.end() || tmodIt->second.size() <= 1) {
        return type;
    }
    // 多模块同名：当前模块有定义 -> 限定键；否则报歧义错误
    if (!module.empty() && tmodIt->second.count(module) > 0) {
        return module + "::" + type;
    }
    std::string modList;
    for (const auto& m : tmodIt->second) {
        if (!modList.empty()) modList += "/";
        modList += m.empty() ? "(全局)" : m;
    }
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "类型 '" + type + "' 在多个模块中定义（" + modList +
                            "），请使用 模块名::" + type + " 限定");
    return type;
}
bool SemanticAnalyzer::isStructType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    // A-2：限定键（甲::记录）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(type, qmod, qbase);
    if (!qmod.empty()) {
        for (const auto& s : program_->structs) {
            if (s->name == qbase && s->moduleName == qmod) return true;
        }
    } else {
        for (const auto& s : program_->structs) {
            if (s->name == type) return true;
        }
    }
    if (isResultType(type)) {
        const std::vector<std::string> args = resultTypeArgs(type);
        if (args.size() == 2) {
            const std::string sname =
                resultStructName(types::canonical(args[0]), types::canonical(args[1]));
            for (const auto& s : program_->structs) {
                if (s->name == sname) return true;
            }
        }
    } else if (isOptionalType(type)) {
        const std::string t = types::canonical(optionalTypeArg(type));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            for (const auto& s : program_->structs) {
                if (s->name == sname) return true;
            }
        }
    }
    return false;
}
bool SemanticAnalyzer::isEnumType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    // A-2：限定键（甲::颜色）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(type, qmod, qbase);
    for (const auto& e : program_->enums) {
        if (e->name != qbase) continue;
        if (qmod.empty() || e->moduleName == qmod) return true;
    }
    return false;
}
const StructDecl* SemanticAnalyzer::findStruct(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    // A-2：限定键（甲::记录）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(name, qmod, qbase);
    for (const auto& s : program_->structs) {
        if (s->name != qbase) continue;
        if (qmod.empty() || s->moduleName == qmod) return s.get();
    }
    if (isResultType(name)) {
        const std::vector<std::string> args = resultTypeArgs(name);
        if (args.size() == 2) {
            const std::string sname =
                resultStructName(types::canonical(args[0]), types::canonical(args[1]));
            for (const auto& s : program_->structs) {
                if (s->name == sname) return s.get();
            }
        }
    } else if (isOptionalType(name)) {
        const std::string t = types::canonical(optionalTypeArg(name));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            for (const auto& s : program_->structs) {
                if (s->name == sname) return s.get();
            }
        }
    }
    return nullptr;
}
const EnumDecl* SemanticAnalyzer::findEnum(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    // A-2：限定键（甲::颜色）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(name, qmod, qbase);
    for (const auto& e : program_->enums) {
        if (e->name != qbase) continue;
        if (qmod.empty() || e->moduleName == qmod) return e.get();
    }
    return nullptr;
}
bool SemanticAnalyzer::enumValueOf(const std::string& enumName, const std::string& memberName,
                                   std::int64_t& outValue) const {
    const EnumDecl* decl = findEnum(enumName);
    if (decl == nullptr) return false;
    for (const auto& m : decl->members) {
        if (m.name == memberName) {
            outValue = m.value;
            return true;
        }
    }
    return false;
}
std::string SemanticAnalyzer::funcReturnTypeOf(const std::string& funcName) const {
    // 第 4 层（P2-6）：支持 模块名$签名key（resolvedSignature 带 crate 前缀）
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        const std::size_t dollar = funcName.find('$');
        if (dollar != std::string::npos) it = functions_.find(funcName.substr(dollar + 1));
    }
    if (it == functions_.end()) {
        // 泛型类方法（自举重建 ABI 修复 2026-08-25）：调用符号形如
        //   向量$Token$元素#整64（实例化类名$方法名#参数签名）。泛型类方法实例化
        //   注册在 classes_（非 functions_），此处回退解析：
        //   取最后一个'$'之后为方法名（元素#整64 -> 元素），剩余（向量$Token）为
        //   实例化类名，查 classes_ 方法表得返回类型（Token）——使调用方正确走
        //   结构体返回缓冲（隐藏返回指针），否则 16 字节结构体按寄存器返回字段错乱。
        const std::size_t lastDollar = funcName.rfind('$');
        if (lastDollar != std::string::npos) {
            const std::string classPart = funcName.substr(0, lastDollar);
            std::string methodPart = funcName.substr(lastDollar + 1);
            const std::size_t hash = methodPart.find('#');
            if (hash != std::string::npos) methodPart = methodPart.substr(0, hash);
            const ClassInfo* ci = findClass(classPart);
            if (ci != nullptr) {
                const auto mit = ci->methods.find(methodPart);
                if (mit != ci->methods.end()) return mit->second.type;
                for (const auto& mk : ci->methods) {
                    if (mk.first == methodPart || mk.first.rfind(methodPart, 0) == 0) {
                        return mk.second.type;
                    }
                }
            }
        }
        return "";
    }
    return it->second.returnType;
}
bool SemanticAnalyzer::funcReturnsRef(const std::string& funcName) const {
    // P3-18 补完：函数返回类型是否为引用（T&）——调用点把结果当"被引用左值的地址"
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        const std::size_t dollar = funcName.find('$');
        if (dollar != std::string::npos) it = functions_.find(funcName.substr(dollar + 1));
    }
    if (it == functions_.end()) return false;
    return it->second.isRefReturn;
}
std::vector<std::string> SemanticAnalyzer::funcParamTypesOf(const std::string& funcName) const {
    // 第 4 层（P2-6）：支持 模块名$签名key（resolvedSignature 带 crate 前缀）
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        const std::size_t dollar = funcName.find('$');
        if (dollar != std::string::npos) it = functions_.find(funcName.substr(dollar + 1));
    }
    if (it == functions_.end()) return {};
    return it->second.paramTypes;
}
std::string SemanticAnalyzer::signatureKey(const std::string& name,
                                           const std::vector<std::string>& paramTypes) {
    std::string key = name;
    if (!paramTypes.empty()) {
        key += "#";
        for (std::size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) key += ",";
            key += paramTypes[i];
        }
    }
    return key;
}
// plans/018 呈报二 A′：函数链接键公式实现（唯一归属，注释见 semantic.hpp 声明处）。
//   例外三形态与 codegen symbolName 映射对齐：
//   ① moduleName 空（单文件编译，mergeModules singleModule 不写 crate 名）；
//   ② moduleName=="主"（入口 crate 根文件——主->cn_main 映射基于纯名）；
//   ③ funcName=="主"（入口函数本身，codegen 映射 cn_main）；
//   ④ moduleName 以 __cn_ 开头（内置运行时符号直通）。
std::string SemanticAnalyzer::functionLinkKey(const std::string& moduleName,
                                              const std::string& funcName,
                                              const std::string& sigKey) {
    if (moduleName.empty() || moduleName == "主" || funcName == "主" ||
        moduleName.rfind("__cn_", 0) == 0) {
        return sigKey;
    }
    return moduleName + "$" + sigKey;
}
bool SemanticAnalyzer::hasFunctionName(const std::string& name) const {
    if (functions_.find(name) != functions_.end()) return true;  // 内置纯名 key
    for (const auto& kv : functions_) {
        // 用户函数 key 形如 名#参数串；纯名内置 key 无 '#'
        const std::size_t hashPos = kv.first.find('#');
        const std::size_t dollarPos = kv.first.find('$');
        // 第 8 层（52_library 实测缺陷）：无参跨模块条目（模块名$函数名，
        //   如 格式化$版本）剥离 '$' 前缀后 base 才匹配纯名；含 '#' 的
        //   跨模块条目（模块名$名#参数）同样剥离。泛型实例名（排序$整32）
        //   不剥离（base 保持 排序$整32 匹配泛型调用）——name 含 '$' 时跳过。
        const bool crossModuleNoHash =
            (hashPos == std::string::npos && dollarPos != std::string::npos &&
             name.find('$') == std::string::npos);
        const bool crossModuleWithHash =
            (hashPos != std::string::npos && dollarPos != std::string::npos &&
             dollarPos < hashPos);
        if (crossModuleNoHash || crossModuleWithHash) {
            const std::size_t hashP2 = kv.first.find('#');
            const std::string base2 = (hashP2 == std::string::npos)
                                          ? kv.first.substr(dollarPos + 1)
                                          : kv.first.substr(dollarPos + 1, hashP2 - dollarPos - 1);
            if (base2 == name) return true;
        } else {
            const std::string base = (hashPos == std::string::npos)
                                         ? kv.first
                                         : kv.first.substr(0, hashPos);
            if (base == name) return true;
        }
    }
    return false;
}
std::string SemanticAnalyzer::funcFirstSigKey(const std::string& name) const {
    std::string best;
    for (const auto& kv : functions_) {
        const std::size_t hashPos = kv.first.find('#');
        const std::string base = (hashPos == std::string::npos) ? kv.first
                                                                : kv.first.substr(0, hashPos);
        if (base != name) continue;
        if (best.empty() || kv.first < best) best = kv.first;
    }
    return best;
}
int SemanticAnalyzer::conversionLevel(const std::string& argTypeRaw,
                                      const std::string& paramTypeRaw) {
    // A-1（引用参数）：按值/按引用双方均剥 & 后比较——引用参数绑定左值实参，
    //   实参与参数的数据形状一致（都是被引用类型的值）；引用 vs 按值 的区分
    //   由签名 key（& 保留在 paramTypes）完成，决议只看形状
    const std::string arg = types::stripRef(canonicalType(argTypeRaw));
    const std::string param = types::stripRef(canonicalType(paramTypeRaw));
    if (arg == param) return 0;
    if (!canConvertType(arg, param)) return -1;
    // 枚举 -> 整数：按宽化处理（枚举本质为整32，值域不损失）
    if (isEnumType(arg) && !isEnumType(param) && types::isInteger(param)) return 1;
    // 整数族内部：同符号宽化（整8->整16->整32->整64、正8->正16->...）
    if (types::isInteger(arg) && types::isInteger(param)) {
        const bool argUnsigned = types::isUnsigned(arg);
        const bool paramUnsigned = types::isUnsigned(param);
        if (argUnsigned == paramUnsigned) {
            if (types::intRank(arg) <= types::intRank(param)) return 1;  // 同符号向宽
        }
        // 符号混合：一律按隐式转换（保守，避免有符号->无符号宽化的值域陷阱）
        return 2;
    }
    // 浮点族内部：浮32 -> 浮64 宽化
    if (types::isFloat(arg) && types::isFloat(param)) {
        if (arg == "浮32" && param == "浮64") return 1;
        return 2;  // 浮64 -> 浮32 窄化：隐式转换（有损）
    }
    // 整数 -> 浮点 / 字符 -> 整数 等：隐式转换
    return 2;
}
std::string SemanticAnalyzer::resolveOverload(const std::string& name,
                                              const std::vector<std::string>& argTypes,
                                              const SourceLocation& loc,
                                              const std::string& moduleFilter) {
    std::string bestKey;
    int bestTotal = INT32_MAX;
    bool ambiguous = false;
    std::string ambiguousDetail;
    // A-5（crate 隔离纯名调用）：前置扫描——当前模块是否定义过该函数名。
    //   若定义过，其他模块的同名条目不参与纯名决议（作用域遮蔽导入语义：
    //   跨模块同名函数纯名调用不再一律报歧义，当前模块版本优先）；
    //   当前模块无定义时才回退导入条目（唯一导入/歧义判定照旧）。
    bool currentHasName = false;
    if (moduleFilter.empty() && !currentModuleName_.empty()) {
        for (const auto& kv : functions_) {
            std::string k2 = kv.first;
            const std::size_t h2 = k2.find('#');
            const std::size_t d2 = k2.find('$');
            std::string km2;
            if (d2 != std::string::npos && h2 != std::string::npos && d2 < h2) {
                km2 = k2.substr(0, d2);
                k2 = k2.substr(d2 + 1);
            } else if (h2 == std::string::npos && d2 != std::string::npos &&
                       name.find('$') == std::string::npos) {
                km2 = k2.substr(0, d2);
                k2 = k2.substr(d2 + 1);
            }
            const std::size_t hp = k2.find('#');
            const std::string b2 = (hp == std::string::npos) ? k2 : k2.substr(0, hp);
            if (b2 != name) continue;
            const std::string em2 = km2.empty() ? kv.second.moduleName : km2;
            if (em2 == currentModuleName_) {
                currentHasName = true;
                break;
            }
        }
    }
    for (const auto& kv : functions_) {
        // 第 4 层：key 形态兼容——普通签名（名#参数）与跨模块条目
        //   （模块名$名#参数）。模块条目 key 含 '$' 前缀（模块名$），
        //   base 提取须剥离 模块名$ 前缀。
        // 注意：泛型实例名（排序$整32）与重载签名（名#参数）不含模块前缀——
        //   仅当 '#' 存在且 '$' 位于 '#' 之前（模块名$名#参数）才剥离；
        //   排序$整32 无 '#' -> 不剥离（base 保持 排序$整32 匹配泛型调用）。
        std::string key = kv.first;
        std::string keyModule;  // key 携带的模块名（跨模块条目）
        const std::size_t hashFirst = key.find('#');
        const std::size_t dollarPos = key.find('$');
        if (dollarPos != std::string::npos && hashFirst != std::string::npos &&
            dollarPos < hashFirst) {
            keyModule = key.substr(0, dollarPos);
            key = key.substr(dollarPos + 1);
        } else if (hashFirst == std::string::npos && dollarPos != std::string::npos &&
                   name.find('$') == std::string::npos) {
            // 第 8 层（52_library 实测缺陷）：无参函数（sigKey 无 '#'）跨模块
            //   注册 key = 模块名$函数名（如 格式化$版本）。调用 name 不含 '$'
            //   （普通函数调用，区别于泛型实例名 排序$整32）——剥离 '$' 前缀。
            keyModule = key.substr(0, dollarPos);
            key = key.substr(dollarPos + 1);
        }
        const std::size_t hashPos = key.find('#');
        const std::string base = (hashPos == std::string::npos) ? key
                                                                : key.substr(0, hashPos);
        if (base != name) continue;  // 仅同名的签名参与决议
        const FunctionInfo& info = kv.second;
        // 第 4 层（crate 隔离）：限定调用按模块过滤——跨模块同名函数各自独立，
        //   仅匹配调用模块的签名（数学::双倍 只解析 数学.cn 的双倍）。
        // A-5（2026-08）：纯名调用（moduleFilter 空）当前模块条目优先——
        //   crate 隔离同名函数纯名调用不再一律歧义（作用域遮蔽导入语义，
        //   与 52_library 的 主::版本() 自限定等价）。
        const std::string entryModule = keyModule.empty() ? info.moduleName : keyModule;
        if (!moduleFilter.empty()) {
            // 模块过滤：普通条目按 info.moduleName，跨模块条目按 key 前缀模块
            //   呈报一B 补充（2026-09-07）：moduleName 空 = 单文件管线
            //   （runPipeline 未合并 / mergeModules singleModule）——全部声明
            //   同属唯一模块，限定调用过滤器（自导入 主::版本 等）恒命中本
            //   模块；多文件管线声明恒带模块名，不受影响。
            if (entryModule == moduleFilter || entryModule.empty()) {
                // 精确匹配：同包限定调用（网络::传输控制::发送 -> 网络::传输控制）
            } else {
                // A-5（父模块名限定调用子模块函数）：entryModule（网络::传输控制）
                //   以 moduleFilter + "::"（网络::）为前缀即视为同一模块——
                //   子模块属于父模块命名空间（网络::连接() 解析到 网络::传输控制::连接）
                const std::string filterPrefix = moduleFilter + "::";
                const bool isSubModule =
                    entryModule.compare(0, filterPrefix.size(), filterPrefix) == 0;
                if (!isSubModule) {
                    // 第 8 层（52_library 实测缺陷）：跨 crate 限定调用
                    //   （工具库::格式化::版本）——外部依赖模块注册 moduleName =
                    //   文件主干（格式化），而调用路径 subModule = 工具库::格式化。
                    //   最后段匹配：moduleFilter 末段（:: 之后）== entryModule 即视为
                    //   同一模块（跨 crate 限定调用解析到依赖包内同名模块）。
                    //   挂账1 包前缀化扩展（2026-09-08）：目录包/父挂子成员模块名
                    //   带包前缀（语义::内置）——条目名**末段**与 filterLast 对齐即
                    //   命中（v1 自举组件 语义::语义::内置::行类型 实测：条目
                    //   语义::内置 末段 内置 == filterLast 内置）。与跨 crate 末段
                    //   按名匹配同族：精确/前缀匹配优先，末段为松匹配兜底。
                    const std::size_t lastColon = moduleFilter.rfind("::");
                    const std::string filterLast = (lastColon == std::string::npos)
                                                        ? moduleFilter
                                                        : moduleFilter.substr(lastColon + 2);
                    const std::size_t entryLastColon = entryModule.rfind("::");
                    const std::string entryLast =
                        (entryLastColon == std::string::npos)
                            ? entryModule
                            : entryModule.substr(entryLastColon + 2);
                    if (entryModule != filterLast && entryLast != filterLast) continue;
                }
            }
        } else if (currentHasName && !entryModule.empty() &&
                   entryModule != currentModuleName_) {
            // A-5（crate 隔离纯名调用）：当前模块有同名函数时，其他模块条目
            //   不参与决议（作用域遮蔽导入；52_library 的 主::版本() 自限定
            //   语义等价，纯名 版本() 现在直接命中当前模块版本）
            continue;
        }
        // 参数个数匹配：实参个数 + 可补全的默认参数数 >= 参数总数
        const int required = static_cast<int>(info.paramTypes.size()) - info.defaultCount;
        const int given = static_cast<int>(argTypes.size());
        if (given < required || given > static_cast<int>(info.paramTypes.size())) {
            continue;  // 参数个数不匹配
        }
        // 逐参数转换等级（只检查实际提供的参数；缺省部分由默认值补全）
        int total = 0;
        bool ok = true;
        for (int i = 0; i < given; ++i) {
            const int level = conversionLevel(argTypes[i], info.paramTypes[i]);
            if (level < 0) { ok = false; break; }
            total += level;
        }
        if (!ok) continue;
        if (bestKey.empty() || total < bestTotal) {
            bestKey = kv.first;
            bestTotal = total;
            ambiguous = false;
        } else if (total == bestTotal && kv.first != bestKey) {
            ambiguous = true;  // 两个签名同样优
            ambiguousDetail = kv.first + " 与 " + bestKey;
        }
    }
    if (ambiguous && !bestKey.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "函数 '" + name + "' 调用存在歧义（" + ambiguousDetail +
                            " 均可匹配），请使用显式类型转换消除歧义");
        return "";
    }
    if (bestKey.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "未找到匹配的函数 '" + name + "'（参数个数或类型不匹配）");
        return "";
    }
    return bestKey;
}
bool SemanticAnalyzer::canConvertType(const std::string& fromRaw,
                                      const std::string& toRaw) const {
    const std::string from = canonicalType(fromRaw);
    const std::string to = canonicalType(toRaw);
    // 模板类型兼容（Task 3.5）：结果<T,E> 与 结果<T,E> 须完全一致；
    //   结果<A,B> 与 结果<C,D>（A可转C 且 B可转D）允许（错误码类型宽化）
    const bool fromResult = isResultType(from);
    const bool toResult = isResultType(to);
    if (fromResult && toResult) {
        const std::vector<std::string> fa = resultTypeArgs(from);
        const std::vector<std::string> ta = resultTypeArgs(to);
        if (fa.size() == 2 && ta.size() == 2) {
            return canConvertType(fa[0], ta[0]) && canConvertType(fa[1], ta[1]);
        }
        return false;
    }
    const bool fromOpt = isOptionalType(from);
    const bool toOpt = isOptionalType(to);
    if (fromOpt && toOpt) {
        const std::string fa = optionalTypeArg(from);
        const std::string ta = optionalTypeArg(to);
        return !fa.empty() && !ta.empty() && canConvertType(fa, ta);
    }
    // 空指针常量 -> 可选<T>：视为空可选值（无）（Task 3.5，规格书07-三）
    if (from == "空类型*" && toOpt) return true;
    if (fromResult || toResult || fromOpt || toOpt) return false;  // 模板与非模板不可转
    const bool fromEnum = isEnumType(from);
    const bool toEnum = isEnumType(to);
    // 枚举 ↔ 整数：双向允许（枚举值可赋给整型变量；整数可赋给枚举变量）
    if (fromEnum && !toEnum) return types::isInteger(to);
    if (!fromEnum && toEnum) return types::isInteger(from);
    // 枚举 → 枚举：须同一枚举类型
    if (fromEnum && toEnum) return from == to;
    // 结构体 → 结构体：须同一类型（值类型拷贝）
    const bool fromStruct = isStructType(from);
    const bool toStruct = isStructType(to);
    if (fromStruct || toStruct) return from == to;
    // 自举前置 A-3b（plans/004）：自定义泛型类实例 类型名统一——源码模板形式
    //   （向量<字符串>）与实例化符号名（向量$字符串，instantiateGeneric 生成）
    //   视为同一类型。此前 返回 表（表=向量$字符串）与返回类型 向量<字符串>
    //   比较失败 -> "无法将 '向量$字符串' 隐式转换为 '向量<字符串>'"
    //   （返回容器/自定义泛型不可用，自举源码大面积受阻）。
    const std::string fromInst = genericClassInstanceName(from);
    const std::string toInst = genericClassInstanceName(to);
    if (fromInst != from || toInst != to) {
        return fromInst == toInst;
    }
    // P3-19：类 → 接口 隐式转换（值或指针；接口类型可持有任何实现类对象）
    {
        std::string toIface = to;
        if (types::isPointer(to) && isInterfaceType(types::pointeeOf(to))) {
            toIface = types::pointeeOf(to);
        }
        if (isInterfaceType(toIface)) {
            std::string fromBase = types::isPointer(from) ? types::pointeeOf(from) : from;
            if (isClassType(fromBase) && classImplementsInterface(fromBase, toIface)) {
                return true;
            }
        }
    }
    return types::canConvert(from, to);
}
std::string SemanticAnalyzer::genericClassInstanceName(const std::string& typeRaw) const {
    const std::string type = canonicalType(typeRaw);
    const std::size_t lt = type.find('<');
    if (lt == std::string::npos || type.empty() || type.back() != '>') return type;
    const std::string head = type.substr(0, lt);
    // 仅自定义泛型类（向量/映射/链表/栈/队列 等）参与统一；结果/可选 是
    //   合成模板（另有降级路径），函数指针/其他 名<...> 形态不在此列
    if (findGeneric(head) == nullptr || findGeneric(head)->ast->innerClass == nullptr) {
        return type;
    }
    std::string instance = head;
    std::string inner = type.substr(lt + 1, type.size() - lt - 2);
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        const std::size_t comma = inner.find(',', pos);
        const std::string part = (comma == std::string::npos)
                                     ? inner.substr(pos)
                                     : inner.substr(pos, comma - pos);
        std::size_t b = part.find_first_not_of(" \t");
        std::size_t e = part.find_last_not_of(" \t");
        instance += "$" + (b == std::string::npos ? "" : part.substr(b, e - b + 1));
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return instance;
}
int SemanticAnalyzer::typeSizeOf(const std::string& typeRaw) const {
    const std::string type = canonicalType(typeRaw);
    if (isEnumType(type)) return 4;  // 枚举按整32存储
    if (types::isPointer(type)) return 8;
    if (types::isArray(type)) {
        return typeSizeOf(types::arrayElemOf(type)) * types::arrayLenOf(type);
    }
    // 阶段3（Task 3.5）：结果<T,E>/可选<T> 已降级为合成结构体，按结构体布局
    if (isResultType(type) || isOptionalType(type)) {
        const StructDecl* lowered = findStruct(type);
        if (lowered != nullptr) {
            // Task 6.1（ensureLoweredType 布局时机缺陷）：合成结构体 totalSize 可能
            //   因联合体布局未及计算而错（1 字节）——结果 最小 12 字节（布尔1+对齐+
            //   联合体8+填充）、可选 最小 9 字节；totalSize 异常小时按标准布局防御。
            if (isResultType(type) && lowered->totalSize < 12) return 12;
            if (isOptionalType(type) && lowered->totalSize < 9) return lowered->totalSize + 8;
            return lowered->totalSize;
        }
        // 未降级（防御）：结果 = 布尔+联合体（8+8=16）；可选 = 布尔+值
        if (isResultType(type)) return 16;
        return typeSizeOf(optionalTypeArg(type)) + 1;
    }
    const StructDecl* decl = findStruct(type);
    if (decl != nullptr) return decl->totalSize;
    // 类类型（Task 3.1）：实例大小（含虚表指针）
    const ClassInfo* cls = findClass(type);
    if (cls != nullptr) return cls->totalSize;
    const int baseSize = types::typeSize(type);
    if (baseSize > 0) return baseSize;
    // 字符串 = 指针（8字节）；未知类型防御性返回8
    if (type == "字符串") return 8;
    return 8;
}
int SemanticAnalyzer::typeAlignOf(const std::string& typeRaw) const {
    const std::string type = canonicalType(typeRaw);
    if (isEnumType(type)) return 4;
    if (types::isPointer(type)) return 8;
    if (types::isArray(type)) return typeAlignOf(types::arrayElemOf(type));
    // 阶段3（Task 3.5/3.1）：结果/可选 合成结构体、类类型
    if (isResultType(type) || isOptionalType(type)) {
        const StructDecl* lowered = findStruct(type);
        if (lowered != nullptr) return lowered->align;
        return 8;  // 防御：未降级按8对齐
    }
    const ClassInfo* cls = findClass(type);
    if (cls != nullptr) return cls->align;
    const StructDecl* decl = findStruct(type);
    if (decl != nullptr) return decl->align;
    if (type == "整128" || type == "正128") return 16;
    if (type == "整64" || type == "正64" || type == "浮64" || type == "字符串") return 8;
    if (type == "整32" || type == "正32" || type == "浮32" || type == "字符") return 4;
    if (type == "整16" || type == "正16") return 2;
    return 1;  // 整8/正8/布尔
}
int SemanticAnalyzer::fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const {
    for (const auto& f : decl->fields) {
        if (f.name == fieldName) {
            return decl->isUnion ? 0 : f.offset;
        }
    }
    // 合成结构体（结果$T$E / 可选$T）：映射源码成员名到合成字段名
    const std::string& declName = decl->name;
    std::string mapped = fieldName;
    if (declName.rfind("结果$", 0) == 0) {
        if (fieldName == "正常") mapped = "是否正常";
        else if (fieldName == "值" || fieldName == "错误") mapped = "错误值联合";
    } else if (declName.rfind("可选$", 0) == 0) {
        if (fieldName == "有值") mapped = "是否某些";
        else if (fieldName == "值") mapped = "值";
    }
    if (mapped != fieldName) {
        for (const auto& f : decl->fields) {
            if (f.name == mapped) {
                // 宿主缺陷根治（2026-08-25）：联合体偏移按真实布局（computeLayout
                //   ——整64/结构体 联合体偏移 8、整32 偏移 4），不再强制 8——原强制
                //   8 使 结果<整32,整32>（联合体真实偏移 4）写/读偏移错位（坏.错误 读 0）。
                return decl->isUnion ? 0 : f.offset;
            }
        }
    }
    return -1;
}
void SemanticAnalyzer::computeLayout(StructDecl* decl) {
    if (decl->layoutComputed) return;
    // 循环检测：布局计算中递归调用；用 layoutComputed 提前标记防无限递归
    decl->layoutComputed = true;
    decl->totalSize = 0;
    decl->align = 1;
    int maxAlign = 1;
    int maxFieldSize = 0;
    for (auto& field : decl->fields) {
        const int fieldAlign = typeAlignOf(field.type);
        const int fieldSize = typeSizeOf(field.type);
        if (fieldAlign > maxAlign) maxAlign = fieldAlign;
        if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
        if (decl->isUnion) {
            // 联合体：所有字段从偏移0开始
            field.offset = 0;
        } else {
            // 结构体：字段对齐放置
            field.offset = (decl->totalSize + fieldAlign - 1) / fieldAlign * fieldAlign;
            decl->totalSize = field.offset + fieldSize;
        }
    }
    if (decl->isUnion) {
        decl->totalSize = maxFieldSize;
    }
    decl->align = maxAlign;
    // 总大小对齐到最大成员对齐（C语义：sizeof(struct) 是最大对齐的倍数）
    decl->totalSize = (decl->totalSize + maxAlign - 1) / maxAlign * maxAlign;
}
void SemanticAnalyzer::computeEnumValues(EnumDecl* decl) {
    std::int64_t nextValue = 0;
    for (auto& member : decl->members) {
        if (member.explicitValue) {
            nextValue = member.value + 1;  // 显式赋值后下一个成员自动递增
        } else {
            member.value = nextValue;
            nextValue++;
        }
    }
}
bool SemanticAnalyzer::declareVar(const std::string& name, const std::string& type,
                                  const SourceLocation& loc) {
    if (scopes_.empty()) pushScope();
    auto& current = scopes_.back();
    if (current.find(name) != current.end()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "重复声明变量 '" + name + "'");
        return false;
    }
    current[name] = canonicalType(type);
    return true;
}
bool SemanticAnalyzer::lookupVar(const std::string& name, std::string& type) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            type = found->second;
            return true;
        }
    }
    return false;
}
// 缺陷②配套（2026-09-03）：名字解析处是否为常量——按 lookupVar 同序从内到外，
//   命中作用域后判 scopeConsts_（局部 常量）或（全局作用域）globalConstValues_
//   裸名命中（顶层常量经 declareVar 入 scopes_[0]；多模块同名常量裸名亦命中）。
bool SemanticAnalyzer::isConstVarName(const std::string& name) const {
    for (std::size_t i = scopes_.size(); i-- > 0; ) {
        if (scopes_[i].find(name) != scopes_[i].end()) {
            if (i < scopeConsts_.size() && scopeConsts_[i].count(name) > 0) return true;
            if (i == 0 && globalConstValues_.count(name) > 0) return true;
            return false;  // 解析处为变量（含内层变量遮蔽外层常量的情形）
        }
    }
    return false;
}
bool SemanticAnalyzer::isFuncPtrType(const std::string& type) {
    return isFuncPtrTypeStr(type);
}
std::string SemanticAnalyzer::funcPtrReturnOf(const std::string& type) {
    return funcPtrReturn(type);
}
std::vector<std::string> SemanticAnalyzer::funcPtrParamsOf(const std::string& type) {
    return funcPtrParams(type);
}
bool SemanticAnalyzer::funcPtrCompatible(const std::string& fromRaw, const std::string& toRaw) {
    // 两个都是函数指针才比较；一个函数指针一个普通类型不允许
    if (!isFuncPtrType(fromRaw) && !isFuncPtrType(toRaw)) return false;
    if (isFuncPtrType(fromRaw) && !isFuncPtrType(toRaw)) return false;
    if (!isFuncPtrType(fromRaw) && isFuncPtrType(toRaw)) return false;
    const std::string fromRet = canonicalType(funcPtrReturn(fromRaw));
    const std::string toRet = canonicalType(funcPtrReturn(toRaw));
    if (fromRet != toRet) return false;  // 返回类型必须完全相等
    std::vector<std::string> fromParams = funcPtrParams(fromRaw);
    std::vector<std::string> toParams = funcPtrParams(toRaw);
    if (fromParams.size() != toParams.size()) return false;
    for (std::size_t i = 0; i < fromParams.size(); ++i) {
        // 参数类型要求完全相等（保守；宽化在后续Task的调用约定对齐后放开）
        if (canonicalType(fromParams[i]) != canonicalType(toParams[i])) return false;
    }
    return true;
}
bool SemanticAnalyzer::isCompoundAssign(Operator op) {
    switch (op) {
        case Operator::PlusAssign: case Operator::MinusAssign:
        case Operator::StarAssign: case Operator::SlashAssign:
        case Operator::PercentAssign:
        case Operator::AmpAssign: case Operator::PipeAssign:
        case Operator::CaretAssign: case Operator::LessLessAssign:
        case Operator::GreaterGreaterAssign:
            return true;
        default:
            return false;
    }
}
void SemanticAnalyzer::checkCondition(const std::string& type, const SourceLocation& loc,
                                      const std::string& ctx) {
    if (type != "布尔") {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            ctx + "条件必须为布尔类型，实际为 '" + type + "'");
    }
}
void SemanticAnalyzer::registerBuiltins() {
    // ---- 打印 系列（Task 2.9 语义调整）----
    // 打印：println 语义（自动换行，运行时 printLine）；打印行：print 语义（不换行，运行时 printNoLine）
    // 变参：参数个数不限（IR 层逐参数展开为 __cn_print_* 序列，打印 末尾加换行）
    const auto regPrintFn = [this](const std::string& name) {
        FunctionInfo info;
        info.returnType = "空类型";
        info.paramTypes = {"字符串"};
        info.hasBody = true;
        info.variadic = true;  // 参数个数不限（visitCallExpr 特判展开）
        functions_[name] = info;
    };
    regPrintFn("打印");
    regPrintFn("打印行");

    // ---- 格式化（Task 2.9，规格书10.6）：格式化(格式字符串, 参数...) -> 字符串 ----
    // 变参：参数个数不限（IR 层按占位符展开为 __cn_format 调用）
    FunctionInfo formatInfo;
    formatInfo.returnType = "字符串";
    formatInfo.paramTypes = {"字符串"};
    formatInfo.hasBody = true;
    formatInfo.variadic = true;
    functions_["格式化"] = formatInfo;

    // ---- 字符串API（Task 2.5，规格书10.1 字符串操作：长度/比较/连接/复制/查找） ----
    // 运行时符号：字符串长度 -> __cn_str_len、字符串比较 -> __cn_str_eq、
    //           字符串连接 -> __cn_str_concat、字符串复制 -> __cn_str_copy、
    //           字符串查找 -> __cn_str_find（IR 层按函数名映射）
    FunctionInfo strLenInfo;
    strLenInfo.returnType = "整64";
    strLenInfo.paramTypes = {"字符串"};
    strLenInfo.hasBody = true;
    functions_["字符串长度"] = strLenInfo;

    FunctionInfo strEqInfo;
    strEqInfo.returnType = "布尔";
    strEqInfo.paramTypes = {"字符串", "字符串"};
    strEqInfo.hasBody = true;
    functions_["字符串比较"] = strEqInfo;

    FunctionInfo strConcatInfo;
    strConcatInfo.returnType = "字符串";
    strConcatInfo.paramTypes = {"字符串", "字符串"};
    strConcatInfo.hasBody = true;
    functions_["字符串连接"] = strConcatInfo;

    FunctionInfo strCopyInfo;
    strCopyInfo.returnType = "字符串";
    strCopyInfo.paramTypes = {"字符串"};
    strCopyInfo.hasBody = true;
    functions_["字符串复制"] = strCopyInfo;

    FunctionInfo strFindInfo;
    strFindInfo.returnType = "整64";
    strFindInfo.paramTypes = {"字符串", "字符串"};
    strFindInfo.hasBody = true;
    functions_["字符串查找"] = strFindInfo;

    // ---- 字符串驻留（自举重建 P1，2026-08-25；对标 rustc Symbol / LLVM StringPool） ----
    // 符号名/类型名/标识符 经 驻留 得唯一 整64 ID（相同内容同 ID，内容只存一份）。
    // Token/AST/IR/符号表 用 ID 引用——比较/哈希 O(1)，消除百万级重复字符串分配。
    // 运行时符号（IR 层映射）：驻留 -> __cn_intern、驻留文本 -> __cn_intern_text、
    //   驻留计数 -> __cn_intern_count。
    FunctionInfo internInfo;
    internInfo.returnType = "整64";
    internInfo.paramTypes = {"字符串"};
    internInfo.hasBody = true;
    functions_["驻留"] = internInfo;

    FunctionInfo internTextInfo;
    internTextInfo.returnType = "字符串";
    internTextInfo.paramTypes = {"整64"};
    internTextInfo.hasBody = true;
    functions_["驻留文本"] = internTextInfo;

    FunctionInfo internCountInfo;
    internCountInfo.returnType = "整64";
    internCountInfo.paramTypes = {};
    internCountInfo.hasBody = true;
    functions_["驻留计数"] = internCountInfo;

    // ---- 补充字符串API（Task 2.8，规格书10.1 标注"常见字符串库补充"） ----
    // 运行时符号：字符串子串 -> __cn_str_sub、字符串字典序 -> __cn_str_cmp、
    //           字符串大写 -> __cn_str_upper、字符串小写 -> __cn_str_lower、
    //           字符串前缀 -> __cn_str_starts_with、字符串后缀 -> __cn_str_ends_with、
    //           字符串包含 -> __cn_str_contains、字符串修剪 -> __cn_str_trim、
    //           字符串反转 -> __cn_str_reverse、整数转字符串 -> __cn_str_from_int、
    //           浮点转字符串 -> __cn_str_from_float、字符转字符串 -> __cn_str_from_char、
    //           布尔转字符串 -> __cn_str_from_bool、正数转字符串 -> __cn_str_from_uint、
    //           字符串释放 -> __cn_str_free（IR 层按函数名映射）
    // 内存语义：返回动态内存（子串/大写/小写/修剪/反转/整数转/浮点转/字符转/正数转），
    //           调用方负责用 字符串释放 释放；连接/复制沿用 Task 2.5 语义。
    // 注：字符串比较运算符（==/!=/</> 等）规格书未定义字符串变体（运算符表仅整型
    //     与浮点变体），不实现；等价能力由 字符串比较（相等）与 字符串字典序 提供。
    const auto regStrFn = [this](const std::string& name, const std::string& retType,
                                 const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regStrFn("字符串子串", "字符串", {"字符串", "整64", "整64"});
    regStrFn("字符串字典序", "整64", {"字符串", "字符串"});
    regStrFn("字符串大写", "字符串", {"字符串"});
    regStrFn("字符串小写", "字符串", {"字符串"});
    regStrFn("字符串前缀", "布尔", {"字符串", "字符串"});
    regStrFn("字符串后缀", "布尔", {"字符串", "字符串"});
    regStrFn("字符串包含", "布尔", {"字符串", "字符串"});
    regStrFn("字符串修剪", "字符串", {"字符串"});
    regStrFn("字符串反转", "字符串", {"字符串"});
    regStrFn("整数转字符串", "字符串", {"整64"});
    regStrFn("浮点转字符串", "字符串", {"浮64"});
    regStrFn("字符转字符串", "字符串", {"字符"});
    regStrFn("布尔转字符串", "字符串", {"布尔"});   // Task 2.9：布尔转"真"/"假"（拼接上下文）
    regStrFn("正数转字符串", "字符串", {"正64"});
    regStrFn("字符串释放", "空类型", {"字符串"});

    // ---- 数学库（Task 6.3，规格书10.5 数学库；对应运行时 math_api.cpp）----
    // 第 4 层（v2.0 决策6）：内置 key `::` 化——"数学." -> "数学::"（路径分隔
    //   由 v1.0 的 . 改为 ::）。限定名作为**核心 包 prelude 成员**（规格书08-六）：
    //   - 与 CN 层模块 stdlib/数学.cn 的公开函数（纯名 平方根 等）不冲突——
    //     模块函数注册为纯名（公开符号合并），内置函数注册为 :: 限定名
    //   - 调用方式 数学::平方根(值)（v2.0）或旧 数学.平方根(值)（第 6 层迁移前
    //     兼容）：visitCallExpr 模块限定重写时，对已注册的 数学::* 内置名特判：
    //     不重写为纯名，保留限定名走内置函数路径（prelude 无需显式导入）
    // 运行时符号：数学::平方根 -> __cn_sqrt、数学::幂 -> __cn_pow、
    //   数学::正弦 -> __cn_sin、数学::余弦 -> __cn_cos、数学::正切 -> __cn_tan、
    //   数学::绝对值 -> __cn_fabs、数学::向上取整 -> __cn_ceil、
    //   数学::向下取整 -> __cn_floor（IR 层按函数名映射）
    // 参数/返回均为 浮64（double）；P1 的对数/反三角/随机数留待后续
    const auto regMathFn = [this](const std::string& name, const std::string& retType,
                                  const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regMathFn("数学::平方根", "浮64", {"浮64"});
    regMathFn("数学::幂", "浮64", {"浮64", "浮64"});
    regMathFn("数学::正弦", "浮64", {"浮64"});
    regMathFn("数学::余弦", "浮64", {"浮64"});
    regMathFn("数学::正切", "浮64", {"浮64"});
    regMathFn("数学::绝对值", "浮64", {"浮64"});
    regMathFn("数学::向上取整", "浮64", {"浮64"});
    regMathFn("数学::向下取整", "浮64", {"浮64"});

    // ---- IO 输入（Task 6.2，规格书10.6 输入 API；对应运行时 input_api.cpp）----
    // 中文名带 "IO." 前缀（形如 模块.函数 限定名），与 stdlib/IO.cn 模块公开函数
    // （纯名 读取行 等）不冲突——模块函数走"公开符号合并"，内置走"限定名直调"。
    // 运行时符号：IO.读取行 -> __cn_read_line、IO.读取整数 -> __cn_read_int、
    //   IO.读取浮点 -> __cn_read_float、IO.打印到错误 -> __cn_print_err（IR 层映射）。
    // 设计说明：
    //   - 读取行 返回 字符串*（动态分配，EOF/失败返回 无/空指针）——CN 层 stdlib/IO.cn
    //     用 结果<字符串,整32> 包装（nullptr 判定 → 错误码.文件 5 / EOF 特殊语义）
    //   - 读取整数/读取浮点 为 C 风格成功标志：参数 (整32* 成功标志)，返回整64/浮64——
    //     CN 层 stdlib/IO.cn 用 &成功 传参，失败返回 错误(错误码.参数)（非 EOF 语义）
    //   - 打印到错误：单字符串参数，不换行（fprintf stderr，与 打印行 不换行语义一致）
    const auto regIoFn = [this](const std::string& name, const std::string& retType,
                                const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regIoFn("IO::读取行", "字符串", {});
    regIoFn("IO::读取整数", "整64", {"整32*"});
    regIoFn("IO::读取浮点", "浮64", {"整32*"});
    regIoFn("IO::打印到错误", "空类型", {"字符串"});

    // ---- 文件 API（Task 6.2，规格书阶段五「文件系统」；对应运行时 file_api.cpp）----
    // 中文名带 "文件." 前缀，与 stdlib/文件.cn 模块公开函数不冲突。
    // 运行时符号：文件.打开文件 -> __cn_file_open、文件.读取文件 -> __cn_file_read、
    //   文件.写入文件 -> __cn_file_write、文件.读取文件行 -> __cn_file_read_line、
    //   文件.文件大小 -> __cn_file_size、文件.关闭文件 -> __cn_file_close、
    //   文件.文件存在 -> __cn_file_exists（IR 层映射）。
    // 设计说明：
    //   - 句柄类型：空类型*（void*，C 层 FILE* 转换）
    //   - 打开文件：模式 整32（1=读/2=写/3=追加），返回 空类型*（失败 nullptr）
    //   - 读取文件/写入文件/文件大小：返回整64（实际字节数/大小，失败 -1）
    //   - 读取文件行：返回 字符串*（动态分配，EOF 返回 nullptr）——CN 层包装 结果<字符串,整32>
    //   - 文件存在：返回 布尔
    const auto regFileFn = [this](const std::string& name, const std::string& retType,
                                  const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regFileFn("文件::打开文件", "空类型*", {"字符串", "整32"});
    regFileFn("文件::读取文件", "整64", {"空类型*", "字符*", "整64"});
    regFileFn("文件::写入文件", "整64", {"空类型*", "字符串"});
    regFileFn("文件::读取文件行", "字符串", {"空类型*"});
    regFileFn("文件::文件大小", "整64", {"空类型*"});
    regFileFn("文件::关闭文件", "空类型", {"空类型*"});
    regFileFn("文件::文件存在", "布尔", {"字符串"});

    // ---- 字符串扩展库（Task 6.5，对标 C++ string 解析；对应运行时 string_api.cpp）----
    // 中文名带 "解析." 前缀（形如 模块.函数 限定名），与 stdlib/字符串扩展.cn
    // 模块公开函数（纯名 字符串转整数 等）不冲突——内置走"限定名直调"（IO 库同模式）。
    // 注意：不能用 "字符串." 前缀——字符串 是类型关键字（Kw_String），词法器将
    //   "字符串.转整数" 拆为 关键字+标识符 报"预期表达式"（lessons 变量名前缀同类）。
    // 运行时符号：解析.转整数 -> __cn_str_to_int、解析.转浮点 -> __cn_str_to_double、
    //   解析.转布尔 -> __cn_str_to_bool（IR 层映射）。
    // 设计说明（Task 6.2 IO 库同模式）：
    //   - 三个解析函数均带 整32* 成功标志输出参数（1=成功，0=非法输入/空串/范围错误）
    //   - 返回整64/浮64/整64（布尔 0/1）；失败返回 0/0.0——CN 层 stdlib/字符串扩展.cn
    //     用 &成功 传参，失败返回 错误(6)
    //   - 与现有 整数转字符串/浮点转字符串/布尔转字符串 命名对称（字符串转 前缀）
    const auto regStrExtFn = [this](const std::string& name, const std::string& retType,
                                    const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regStrExtFn("解析::转整数", "整64", {"字符串", "整32*"});
    regStrExtFn("解析::转浮点", "浮64", {"字符串", "整32*"});
    regStrExtFn("解析::转布尔", "整64", {"字符串", "整32*"});

    // ---- 时间库（Task 6.5，规格书10.4 时间；对应运行时 time_api.cpp）----
    // 中文名带 "时间." 前缀，与 stdlib/时间.cn 模块公开函数不冲突（数学库同模式）。
    // 运行时符号：时间.当前时间戳 -> __cn_time、时间.单调时钟毫秒 -> __cn_clock_ms、
    //   时间.格式化时间 -> __cn_time_format（IR 层映射）。
    // 设计说明：
    //   - 当前时间戳/单调时钟毫秒：零参数，返回整64（秒/毫秒）
    //   - 格式化时间：参数 (整64 时间戳, 字符串 格式)，返回 字符串*（动态分配，
    //     失败 nullptr）——CN 层 stdlib/时间.cn 包装 结果<字符串,整32>（nullptr→错误码.参数）
    const auto regTimeFn = [this](const std::string& name, const std::string& retType,
                                  const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regTimeFn("时间::当前时间戳", "整64", {});
    regTimeFn("时间::单调时钟毫秒", "整64", {});
    regTimeFn("时间::格式化时间", "字符串", {"整64", "字符串"});

    // ---- 内存库（自举前置 C-1/C-3，2026-08；对应运行时 io_api.cpp）----
    // 中文名带 "内存." 前缀（规格书10.2 内存管理扩展）。
    // 运行时符号（IR 层映射）：
    //   内存::活动分配数     -> __cn_alloc_live  （未释放块数，泄漏检测基线）
    //   内存::总分配次数     -> __cn_alloc_total（累计分配次数）
    //   内存::竞技场分配     -> __cn_arena_alloc（一次性进程 arena，bump 分配）
    //   内存::竞技场重置     -> __cn_arena_reset（释放全部块）
    //   内存::竞技场活动字节 -> __cn_arena_bytes（已分配总字节）
    // 设计说明：
    //   - 活动分配数/总分配次数：零参数，返回整64（原子计数）
    //   - 竞技场分配：参数 (整64 大小)，返回 空类型*（失败 nullptr）
    //   - 竞技场重置/竞技场活动字节：零参数，返回 空类型/整64
    const auto regMemLibFn = [this](const std::string& name, const std::string& retType,
                                    const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regMemLibFn("内存::活动分配数", "整64", {});
    regMemLibFn("内存::总分配次数", "整64", {});
    regMemLibFn("内存::竞技场分配", "空类型*", {"整64"});
    regMemLibFn("内存::竞技场重置", "空类型", {});
    regMemLibFn("内存::竞技场活动字节", "整64", {});
    regMemLibFn("内存::释放全部", "空类型", {});

    // ---- 系统库（Task 6.5，规格书10.4 命令行参数；对应运行时 system_api.cpp）----
    // 中文名带 "系统." 前缀，与 stdlib/系统.cn 模块公开函数不冲突（数学库同模式）。
    // 运行时符号：系统.参数个数 -> __cn_argc、系统.参数 -> __cn_argv（IR 层映射）。
    // 设计说明：
    //   - 参数个数：零参数，返回整64（argc，含可执行文件名本身）
    //   - 参数：参数 (整64 索引)，返回 字符串*（CRT 持有，越界 nullptr）——CN 层
    //     stdlib/系统.cn 包装 结果<字符串,整32>（nullptr→错误码.参数）
    const auto regSysFn = [this](const std::string& name, const std::string& retType,
                                 const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regSysFn("系统::参数个数", "整64", {});
    regSysFn("系统::参数", "字符串", {"整64"});

    // ---- 内存管理API（Task 6.1 核心库/容器库，规格书10.2 内存管理）----
    // 运行时符号：分配 -> cn_alloc、释放 -> cn_free、重新分配 -> cn_realloc、
    //   复制内存 -> cn_memcpy、置零内存 -> cn_memset（codegen symbolName 已有映射）
    // 参数/返回：分配/重新分配 返回 空类型*（void*）；复制内存 三个指针参数；
    //   置零内存 指针 + 整64 大小；释放 单指针（空类型返回）。
    // 说明：此前 codegen 层已为这 5 个函数保留符号映射，但语义层未注册，
    //   导致 CN 源码无法调用——本子任务补齐注册（容器库 向量/链表 依赖）。
    const auto regMemFn = [this](const std::string& name, const std::string& retType,
                                 const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regMemFn("分配", "空类型*", {"整64"});
    regMemFn("释放", "空类型", {"空类型*"});
    regMemFn("重新分配", "空类型*", {"空类型*", "整64"});
    regMemFn("复制内存", "空类型", {"空类型*", "空类型*", "整64"});
    regMemFn("置零内存", "空类型", {"空类型*", "整64"});

    // ---- 运行时错误（Task 6.1 核心库 断言 依赖，规格书10.2 错误处理）----
    // 中文内置函数 运行时错误(整64 错误码) -> 空类型：终止进程并打印错误消息。
    // 运行时符号 __cn_runtime_error（IR 层越界检查已用，codegen 有符号映射），
    // 此处补语义层注册使 CN 源码可调用（stdlib/核心.cn 的 断言 函数用）。
    // 注意：错误码 4~8 对应运行时错误表（1=除零/2=越界/3=空指针/4=内存/
    //   5=文件/6=无效参数/7=未初始化/8=溢出），断言失败用 6（无效参数）。
    FunctionInfo rtErrInfo;
    rtErrInfo.returnType = "空类型";
    rtErrInfo.paramTypes = {"整64"};
    rtErrInfo.hasBody = true;
    functions_["运行时错误"] = rtErrInfo;
}
void SemanticAnalyzer::registerFunction(FunctionDecl* node) {
    // 阶段3（Task 3.5）：内置构造器 正常/错误/某些 用户不可重定义
    if (node->name == "正常" || node->name == "错误" || node->name == "某些") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "内置构造器 '" + node->name + "' 不可重定义");
        return;
    }
    FunctionInfo info;
    // A-2（crate 分桶）：返回类型按所属模块解析——多模块同名类型改写为限定键
    //   （模块名::类型），IR 层按改写后的类型查询语义表（findStruct/typeSizeOf）
    if (!node->returnType.empty()) {
        node->returnType = resolveTypeName(node->returnType, node->moduleName,
                                           node->location);
    }
    info.returnType = node->returnType.empty() ? "空类型" : canonicalType(node->returnType);
    info.hasBody = (node->body != nullptr);
    // C-3（FFI）：外部 函数 声明——无函数体（C 符号由链接期解析）；
    //   有函数体属误用（外部=外部定义，禁止 CN 侧实现）
    info.isExtern = node->isExtern;
    if (node->isExtern && node->body != nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "外部 函数 声明不能有函数体（C 符号由外部库提供）");
    }
    // P3-18 补完（2026-08）：函数返回类型支持引用（T&）——返回被引用左值的地址。
    //   引用返回不参与重载签名（仅返回类型不同不构成重载）；isRefReturn 供 IR
    //   （返回类型映射 ptr）与调用方（get() = 值 写回 / 整32& r = get() 绑定 /
    //   &get() 取址）识别。返回的 lvalue 校验在 visitReturnStmt（禁返回局部变量地址）。
    if (!node->returnType.empty() && types::isReference(node->returnType)) {
        info.isRefReturn = true;
    }
    // 默认参数规则检查：从右向左连续声明（f(a=1, b) 非法——默认参数左侧出现无默认参数；
    //   f(a, b=1, c=2) 合法——最左侧参数可无默认）。
    // 正确判定：从左到右，一旦遇到无默认参数，其后所有参数都须无默认；
    //   即"第一个有默认参数的左侧参数都须有默认"（除最左侧参数）。
    // 简化实现：反向遍历，记录右侧是否已出现默认参数；若当前无默认且
    //   右侧已有默认 且 当前不是最左侧参数 -> 非法（左侧还有参数会继承默认？不——
    //   f(a=1, b) 中 a 有默认、b 无默认：反向看 b 无默认、a(右侧)有默认、b 不是最左 -> 合法？
    //   不对——f(a=1, b)：a 有默认，b 无默认，b 在 a 右侧（更右）应无默认，合法！
    //   f(a, b=1)：b 有默认，a 无默认，a 最左，合法！
    //   f(a=1, b, c=2)：c 有默认，b 无默认（右侧有 c 默认），b 非最左 -> 非法 ✓
    //   判定：反向遍历，遇到无默认参数时，若"已见默认"且"它左侧还有参数" -> 非法。
    // 默认参数规则：从右向左连续声明——反向扫描，一旦遇到无默认参数，
    //   其左侧（更左）不能再出现有默认参数（f(a=1, b) 非法：b 无默认在右，
    //   a 有默认在左被隔断；f(a, b=1) 合法：b 有默认最右、a 无默认最左）
    bool noDefaultSeen = false;
    for (auto it = node->params.rbegin(); it != node->params.rend(); ++it) {
        // A-1（引用参数）：引用参数不能有默认值（引用须绑定调用方左值，无法预置）
        if ((*it)->hasDefault && !(*it)->funcPtr.isFunctionPtr() &&
            types::isReference((*it)->typeName)) {
            diagnostics_.report(DiagnosticLevel::Error, (*it)->location,
                                "引用参数不能有默认值（引用须绑定调用方左值）");
        }
        if ((*it)->hasDefault) {
            ++info.defaultCount;
            info.hasDefault.push_back(true);
            if (noDefaultSeen) {
                diagnostics_.report(DiagnosticLevel::Error, (*it)->location,
                                    "默认参数必须从右向左连续声明（参数 '" + (*it)->name +
                                    "' 左侧不能出现无默认值参数）");
            }
        } else {
            info.hasDefault.push_back(false);
            noDefaultSeen = true;
        }
    }
    std::reverse(info.hasDefault.begin(), info.hasDefault.end());  // 恢复参数顺序
    for (auto& param : node->params) {
        // 函数指针参数：整32(*func)(整32, 整32) 类型存规范化字符串
        if (param->funcPtr.isFunctionPtr()) {
            info.paramTypes.push_back(param->funcPtr.toString());
        } else {
            // A-2（crate 分桶）：参数类型按所属模块解析并改写（IR 层按改写后
            //   的类型查询语义表）；A-1 引用保留 &（整32&/账户&）——签名 key/
            //   mangling 区分 按值/按引用
            param->typeName = resolveTypeName(param->typeName, node->moduleName,
                                              param->location);
            // 自举前置 A-3a（plans/004）：泛型实例化类型参数（向量<字符串> 词表）
            //   ——模板形式归一为实例符号名（向量$字符串）并触发单态化，与类方法
            //   参数（class_resolver resolveGenericTypeName 路径）一致。此前签名
            //   key/参数槽/成员访问全程用模板形式，与构造调用产生的实例名不匹配：
            //   "预期参数名，实际为 '<'"（parseParamDecl 已修）+"类型 '向量<字符串>'
            //   不是类类型，无法访问成员 '大小'"。结果/可选 等合成模板不受影响
            //   （findGeneric 未命中 -> 原样返回）。
            param->typeName = resolveGenericTypeName(param->typeName, param->location);
            info.paramTypes.push_back(types::canonicalParam(param->typeName));
        }
    }
    // 生成签名 key（名 + "#" + 参数类型串，mangling 与决议共用）
    node->sigKey = signatureKey(node->name, info.paramTypes);
    // ---- crate 模型（第 4 层，v2.0 决策4）：重复定义按模块分桶 ----
    // 跨模块同名同签名函数允许（crate 隔离：包A::工具 与 包B::工具 独立符号）；
    // 仅同模块内重名报错。moduleName 由 mergeModules 合并阶段写入 FunctionDecl。
    info.moduleName = node->moduleName;
    // plans/018 呈报二 A′（2026-09-07 用户裁决）：注册键 = 函数链接键公式键。
    //   注册侧与定义侧（ir_decl mangledName）同源单一公式——消灭旧「首注册占
    //   裸键、后注册得 模块$键」的顺序依赖（依赖先注册抢走裸键时，入口同名
    //   函数纯名调用解析到 主$键 而定义侧发射裸键 → 链接 undefined reference，
    //   base3 探针汇编实证）。Rust 对照：rustc 符号=f(def-id)，定义时即定，
    //   「同名抢裸键」结构上不存在。
    const std::string linkKey =
        functionLinkKey(node->moduleName, node->name, node->sigKey);
    auto it = functions_.find(linkKey);
    if (it != functions_.end()) {
        // 同签名重名（模块分桶判定）：原型+定义 组合须同模块才配对；
        //   公式键已内嵌模块名（除 主/空模块 裸键形态），跨模块命中同一键
        //   = 裸键形态碰撞（两模块同名同签名函数均归一为裸键，如依赖模块
        //   定义了与入口同名的 主 函数）——链接符号必然冲突，fail fast 报错。
        const bool sameModule = (it->second.moduleName == node->moduleName);
        if (!sameModule) {
            // 跨模块同名同签名且公式键归一为同一裸键：链接层面不可共存
            //   （两定义同符号），诊断拒绝（绝不静默覆盖丢函数）
            funcSigModules_[node->sigKey].insert(node->moduleName);
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "函数 '" + node->name + "' 链接符号冲突（模块 '" +
                    it->second.moduleName + "' 与 '" + node->moduleName +
                    "' 同名同签名且链接键归一为裸键，无法共存）");
            return;
        }
        // 同签名重名：允许"原型声明 + 定义"组合，其余为重复定义
        bool isProtoPlusDef = !it->second.hasBody && info.hasBody;
        if (isProtoPlusDef) {
            // 签名一致性检查：原型（已注册）与定义（当前）签名必须一致
            const FunctionInfo& proto = it->second;
            bool same = (proto.returnType == info.returnType &&
                         proto.defaultCount == info.defaultCount);
            if (!same) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "函数 '" + node->name + "' 原型声明与定义签名不一致");
            }
        } else if (it->second.hasBody || info.hasBody) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "模块 '" + node->moduleName + "' 内重复定义函数 '" + node->name +
                                "'（参数类型相同；仅返回类型不同不构成重载）");
        }
        return;
    }
    // Task 2.10 重载兼容：签名 key 未命中但同名已有其他签名——
    //   合法重载（加(整32,整32) 与 加(浮64,浮64)）；
    //   但"原型声明 + 不同签名定义"是错误（原型已锁定签名，定义须一致）。
    //   规则：同名存在 原型（无体）且 当前是定义（有体）→ 签名必须与原型一致
    //   （仅同模块内检查——跨模块原型不约束其他模块的定义）。
    //   A′：键含公式前缀（模块名$名#参数）须先剥前缀再取同名 base（与
    //   resolveOverload 的形态判定一致）。
    for (const auto& kv : functions_) {
        std::string k = kv.first;
        const std::size_t hashFirst = k.find('#');
        const std::size_t dollarPos = k.find('$');
        if (dollarPos != std::string::npos &&
            (hashFirst != std::string::npos
                 ? dollarPos < hashFirst
                 : node->name.find('$') == std::string::npos)) {
            k = k.substr(dollarPos + 1);
        }
        const std::size_t hashPos = k.find('#');
        const std::string base = (hashPos == std::string::npos) ? k
                                                                : k.substr(0, hashPos);
        if (base == node->name && kv.second.moduleName == node->moduleName &&
            !kv.second.hasBody && info.hasBody) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数 '" + node->name +
                                "' 原型声明与定义签名不一致（重载须参数类型不同）");
            return;
        }
    }
    funcSigModules_[node->sigKey].insert(node->moduleName);
    // A′：注册键 = 公式键（与定义侧 mangledName 同源，消顺序依赖）
    functions_[linkKey] = info;
}
// C-3（FFI）：查询签名 key 对应函数是否为 外部 函数（链接符号=纯名）
bool SemanticAnalyzer::isExternFunc(const std::string& sigKey) const {
    auto it = functions_.find(sigKey);
    if (it != functions_.end()) return it->second.isExtern;
    // 跨模块前缀键（模块名$sigKey，crate 隔离注册路径）：
    //   去掉模块前缀后按签名 key 比对（外部 声明均在入口文件，前缀=入口模块）
    const std::size_t dollar = sigKey.find('$');
    if (dollar != std::string::npos) {
        const std::string rest = sigKey.substr(dollar + 1);
        auto it2 = functions_.find(rest);
        if (it2 != functions_.end()) return it2->second.isExtern;
    }
    return false;
}
bool SemanticAnalyzer::bodyGuaranteesReturn(BlockStmt* body) const {
    if (body == nullptr || body->statements.empty()) return false;
    return stmtGuaranteesReturn(body->statements.back().get());
}
// 自举前置 A-2（plans/004）：语句是否必然以 返回 结束——用于函数体末语句
//   为 选择（switch）全分支返回 的识别（类型名() 枚举分发+返回 模式）。
//   规则：返回语句 ✓；无限循环（无条件）✓；选择 = 全部 情况 分支 + 默认
//   分支 均必然返回（无 fall-through 逃逸）✓；块语句按 bodyGuaranteesReturn。
bool SemanticAnalyzer::stmtGuaranteesReturn(Stmt* stmt) const {
    if (stmt == nullptr) return false;
    if (stmt->getType() == NodeType::ReturnStmt) return true;
    // 无限循环：循环 ( ; ; ) { } 或 循环 { }（无条件表达式）
    if (stmt->getType() == NodeType::ForStmt) {
        ForStmt* forStmt = static_cast<ForStmt*>(stmt);
        if (forStmt->condition == nullptr) return true;
    }
    // 选择（switch）全分支返回：每个 情况 分支（末语句必然返回）+ 默认 分支
    //   存在且必然返回——所有路径均以 返回 结束，函数缺省返回检查可放行。
    if (stmt->getType() == NodeType::SwitchStmt) {
        SwitchStmt* sw = static_cast<SwitchStmt*>(stmt);
        if (sw->cases.empty()) return false;
        for (auto& c : sw->cases) {
            // C-4 多值分组（情况 "继续", "暂停":）——前序标签体为空（C fallthrough
            //   分组语义），跳过；其语句归属最后标签，由该标签的返回检查覆盖
            if (c->statements.empty()) continue;
            if (!stmtGuaranteesReturn(c->statements.back().get())) return false;
        }
        if (sw->defaultCase == nullptr) return false;
        if (sw->defaultCase->statements.empty()) return false;
        return stmtGuaranteesReturn(sw->defaultCase->statements.back().get());
    }
    // 块语句（嵌套作用域）：按函数体同规则
    if (stmt->getType() == NodeType::BlockStmt) {
        return bodyGuaranteesReturn(static_cast<BlockStmt*>(stmt));
    }
    // 如果/否则 双分支必然返回（若 条件 时返回，否则 时返回——所有路径均返回）
    if (stmt->getType() == NodeType::IfStmt) {
        IfStmt* iff = static_cast<IfStmt*>(stmt);
        if (iff->elseBranch == nullptr) return false;  // 无否则分支：条件假时穿出
        if (!stmtGuaranteesReturn(iff->thenBranch.get())) return false;
        // 否则链：否则如果（IfStmt）或 块
        if (iff->elseBranch->getType() == NodeType::IfStmt) {
            return stmtGuaranteesReturn(iff->elseBranch.get());
        }
        return bodyGuaranteesReturn(static_cast<BlockStmt*>(iff->elseBranch.get()));
    }
    return false;
}
void SemanticAnalyzer::visitProgram(Program* node) {
    pushScope();  // 全局作用域
    program_ = node;
    // 第零趟：注册CN语言内置函数符号（打印行等，无需源码声明即可调用）
    //         阶段3：注册 正常/错误/某些 内置构造器（用户不可重定义）
    registerBuiltins();
    registerErrorBuiltins();
    // 第零趟b（阶段3，Task 3.6）：收集导入模块名（限定调用识别用）
    //   注：多文件编译时 driver 已合并被导入模块的公开声明到本 Program，
    //   导入声明（ImportDecl）仍保留在 AST 中供此处收集模块名。
    // 第 4 层（crate 分桶）：构建已加载模块集合 + 模块公开符号表（须在导入表
    //   构建之前——plans/018 呈报一B 路径导入「尾段是模块还是符号」消歧要查
    //   knownModules_）。
    //   knownModules_：全部声明 moduleName 全集 + driver 注入的加载模块清单
    //   （Program::loadedModules：依赖图模块名 + 货舱 [依赖] 包名）——P1-1
    //   废止后限定调用按「模块已加载」放行的判定数据源。
    //   modulePublicSymbols_：按模块收集公开符号，供可见性交集检查（模块私有
    //   类不导出）与花括号导入符号验证使用。
    for (const auto& f : node->declarations) {
        if (!f->moduleName.empty()) knownModules_.insert(f->moduleName);
        if (f->access == AccessSpecifier::Public && !f->moduleName.empty()) {
            modulePublicSymbols_[f->moduleName].insert(f->name);
        }
    }
    for (const auto& s : node->structs) {
        if (!s->moduleName.empty()) knownModules_.insert(s->moduleName);
        if (s->access == AccessSpecifier::Public && !s->moduleName.empty()) {
            modulePublicSymbols_[s->moduleName].insert(s->name);
        }
    }
    for (const auto& e : node->enums) {
        if (!e->moduleName.empty()) knownModules_.insert(e->moduleName);
        if (e->access == AccessSpecifier::Public && !e->moduleName.empty()) {
            modulePublicSymbols_[e->moduleName].insert(e->name);
        }
    }
    for (const auto& c : node->classes) {
        if (!c->moduleName.empty()) knownModules_.insert(c->moduleName);
        if (c->access == AccessSpecifier::Public && !c->moduleName.empty()) {
            modulePublicSymbols_[c->moduleName].insert(c->name);
            modulePublicClasses_[c->moduleName].insert(c->name);
        }
    }
    for (const auto& i : node->interfaces) {
        if (!i->moduleName.empty()) knownModules_.insert(i->moduleName);
        if (i->access == AccessSpecifier::Public && !i->moduleName.empty()) {
            modulePublicSymbols_[i->moduleName].insert(i->name);
        }
    }
    for (const auto& g : node->generics) {
        const bool genPublic = (g->innerClass != nullptr)
                                   ? (g->innerClass->access == AccessSpecifier::Public)
                                   : (g->innerFunc != nullptr &&
                                      g->innerFunc->access == AccessSpecifier::Public);
        if (!g->moduleName.empty()) knownModules_.insert(g->moduleName);
        if (!genPublic || g->moduleName.empty()) continue;
        const std::string gname = (g->innerClass != nullptr) ? g->innerClass->name
                                   : (g->innerFunc != nullptr) ? g->innerFunc->name : "";
        if (!gname.empty()) modulePublicSymbols_[g->moduleName].insert(gname);
    }
    for (const auto& m : node->loadedModules) {
        if (!m.empty()) knownModules_.insert(m);
    }
    // 第 4 层（P1-3）：visitImportDecl 构建 use 导入表（符号集合/别名/通配符）。
    for (auto& imp : node->imports) {
        visitImportDecl(imp.get());
    }
    // plans/018 P6b 工作流2（规格08-三 3.6 名称解析）：显式导入冲突检查
    //   （①×② 导入与本地定义同名 / ②×② 多次显式导入同名——纯 AST 扫描，
    //   不依赖函数注册趟；声明 moduleName == 导入 ownerModule 即本地定义）。
    //   【已启用（2026-09-07 呈报一B 用户终裁）】：「导入 m::符号」= 具名绑定
    //   ②（Rust 一致），与本文件本地定义同名 = 编译错误（E0255 对应）——
    //   旧「路径导入=模块级通配」实现随之废止（visitImportDecl 已改为真绑定）。
    checkImportLocalConflicts(node);
    // 第一趟a：注册全部结构体/联合体/枚举类型名（支持前向引用：字段可引用后定义的类型）
    // A-2（crate 分桶）：类型按所属模块注册（同模块重复报错，跨模块同名允许）
    for (auto& s : node->structs) {
        declareTypeName(s->name, s->moduleName, s->location);
    }
    for (auto& e : node->enums) {
        declareTypeName(e->name, e->moduleName, e->location);
    }
    // A-2：结构体/联合体字段类型引用解析（按所属模块解析多模块同名类型，
    //   改写字段类型为限定键；computeLayout/字段访问经 findStruct 精确命中）
    for (auto& s : node->structs) {
        for (auto& f : s->fields) {
            if (!f.type.empty()) {
                f.type = resolveTypeName(f.type, s->moduleName, s->location);
            }
        }
    }
    // 第一趟e（阶段3）：注册泛型声明（泛型类/函数模板）——须在类解析之前：
    //   A-4（2026-08）跨模块泛型类字段（馆藏 类字段 向量<整64>）在 resolveClass
    //   期间经 resolveGenericTypeName 触发实例化，需 findGeneric 已注册
    registerGenerics(node);
    // 缺陷2 根治（2026-09-02，趟序重排）：结构体容器字段（函数IR.指令 =
    //   向量<IR指令>，v2 自举组件核心用法）——原 第一趟b 在 registerGenerics
    //   之前算布局，容器类未实例化 typeSizeOf 防御兜底 8（字段 24 被算成 8，
    //   结构体总大小/槽区/零初始化全错位——p3 实证 表 字段只零 2 槽段错误）。
    //   修正：泛型注册后先对结构体字段类型实例化归一（向量<IR指令> ->
    //   向量$IR指令，触发实例化注册），再计算布局。
    //   ⚠ 迭代稳定性（ASAN 实证 heap-use-after-free）：resolveGenericTypeName ->
    //   instantiateGeneric -> ensureLoweredType 会向 node->structs 追加合成结构体
    //   （结果<T,E> 降级）触发 vector 重分配——不得持有 vector 槽引用；
    //   unique_ptr 重分配不移动 StructDecl 堆本体，缓存裸指针按索引重取安全。
    for (std::size_t si = 0; si < node->structs.size(); ++si) {
        StructDecl* s = node->structs[si].get();
        for (std::size_t fi = 0; fi < s->fields.size(); ++fi) {
            const std::string ftype = s->fields[fi].type;
            if (!ftype.empty() && ftype.find('<') != std::string::npos) {
                const std::string resolved =
                    resolveGenericTypeName(ftype, s->location);
                if (!resolved.empty() && resolved != ftype) {
                    s->fields[fi].type = resolved;
                }
            }
        }
    }
    // 第一趟b：计算全部结构体/联合体布局（递归，循环引用检测）。
    //   同上迭代稳定性：computeLayout -> typeSizeOf 可能触发降级追加，按索引取本体。
    for (std::size_t si = 0; si < node->structs.size(); ++si) {
        computeLayout(node->structs[si].get());
    }
    // 第一趟c：枚举成员值求值（自动递增/显式赋值/负数）
    for (auto& e : node->enums) {
        computeEnumValues(e.get());
    }
    // 第一趟d（阶段3）：注册类/接口符号（类名 + 成员解析 + 虚表 + 接口验证 + 布局）
    registerClassAndInterfaces(node);
    // 第一趟f（阶段3）：结果/可选类型降级（生成合成结构体并布局）
    lowerResultOptionalTypes(node);
    // 第一趟g：注册全部函数符号（含前向调用）
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            registerFunction(decl.get());
        }
    }
    // 第 4 层（v2.0 决策8/9，P1-4/P3-8）：注册顶层常量/静态（crate 级）。
    //   常量：编译期常量折叠——初始值为字面量（整/浮/字符串）时求值存入
    //   globalConstValues_（常量名 -> 值文本），visitIdentifierExpr 引用时替换。
    //   静态：登记符号名（globalStaticNames_），供 IR 层生成全局存储。
    //   常量值文本直接取字面量 raw（整数/浮点）或字符串内容（去引号）。
    for (auto& g : node->globals) {
        // 2026-08-30 根治（P3-8 补全）：无初始值的静态变量也应登记
        //   （静态 向量<整64> 全局表 零初始化 .data；原 continue 跳过导致
        //   静态容器变量整个漏登记——IR 无 gstatic、函数体引用报「模块未导入」）
        if (g->initializer == nullptr && !g->isStatic) continue;
        if (g->isConst) {
            Expr* init = g->initializer.get();
            std::string constText;
            // P3-22：编译期常量表达式求值（字面量 / 引用其他常量 / 整浮算术 / 字符串拼接）
            if (!cnEvalConstExpr(globalConstValues_, g->moduleName, init, constText)) {
                diagnostics_.report(DiagnosticLevel::Error, init->location,
                                    "顶层常量 '" + g->name +
                                        "' 初始值必须是字面量或常量表达式");
            }
            if (!constText.empty()) {
                // A-2（常量 crate 分桶）：多模块同名常量各自登记限定键（模块$名），
                //   引用经语义层按当前模块解析并重写节点名；裸名仅保留首定义
                //   （兼容单模块/唯一定义场景，IR 层裸名查询路径不变）
                constModules_[g->name].insert(g->moduleName);
                if (globalConstValues_.find(g->name) == globalConstValues_.end()) {
                    globalConstValues_[g->name] = constText;
                }
                globalConstValuesQualified_[g->moduleName + "$" + g->name] = constText;
                // 限定键（模块$名）同时登记到 IR 查询表：语义层把多模块同名
                //   常量引用重写为限定键后，IR 层 globalConstValue(限定键) 命中
                //   （IR 无模块上下文，靠重写后的名字直接查值文本）
                globalConstValues_[g->moduleName + "$" + g->name] = constText;
            }
            // 跨模块同名常量：仅首个模块 declareVar（全局作用域去重——
            //   多模块同名时按当前模块解析，见 visitIdentifierExpr 常量分支）
            if (constModules_[g->name].size() == 1) {
                declareVar(g->name, "自动", g->location);
            }
        } else if (g->isStatic) {
            // 第 9 层 Debug：顶层静态记录源码类型（IR 层生成 .data 全局存储），
            //   此前仅登记符号名导致函数体内引用落入 FuncAddr 分支（rbp0 汇编错误）
            // 2026-08-30 根治（P3-8 补全）：泛型归一——静态容器变量须走
            //   resolveGenericTypeName（容器<整64> -> 容器$整64），否则成员访问
            //   findClass("容器<整64>") 失败报「不是类类型」（局部变量 visitVarDecl
            //   已归一，全局静态此前漏接）。
            const std::string stType =
                g->typeName.empty() ? "自动"
                                    : resolveGenericTypeName(g->typeName, g->location);
            // A-2（静态 crate 分桶）：同常量——多模块同名静态登记限定键（模块$名）
            staticModules_[g->name].insert(g->moduleName);
            if (globalStatics_.find(g->name) == globalStatics_.end()) {
                globalStatics_[g->name] = stType;
            }
            globalStaticsQualified_[g->moduleName + "$" + g->name] = stType;
            // 限定键（模块$名）同时登记到 IR 查询表（isGlobalStatic/globalStaticType）
            globalStatics_[g->moduleName + "$" + g->name] = stType;
            if (staticModules_[g->name].size() == 1) {
                declareVar(g->name, stType, g->location);
            }
        }
    }
    // 第二趟a（阶段3）：检查类方法体（自身/父类/访问控制/常量 上下文）
    // 自举前置 A-3a（2026-08）：跳过实例化类（名含 $）——实例化可能已在第一趟g
    //   （registerFunction 泛型参数统一）发生，若此处检查、第二趟c 再检查同一
    //   共享 AST（mi.ast 指向原始模板方法），wrapRefArgs 会把 前驱 -> &前驱
    //   包装两次（第二次 arg 已是 整64* 地址 -> "引用参数要求左值实参" + 
    //   "无法将 '整64*' 隐式转换为 '整64&'" 连环误报，映射 方法实测）。
    //   实例化类统一由第二趟c 检查（每个类恰一次）。
    for (auto& kv : classes_) {
        if (kv.first.find('$') != std::string::npos) continue;
        if (checkedClasses_.count(kv.first) > 0) continue;  // H3：幂等
        checkedClasses_.insert(kv.first);
        checkClassMethods(const_cast<ClassInfo&>(kv.second));
    }
    // 第二趟b：逐个检查函数体
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            checkFunctionBody(static_cast<FunctionDecl*>(decl.get()));
        }
    }
    // 第二趟c（Task 6.1）：实例化泛型类方法体检查——泛型实例化（向量<整32>）
    //   发生在第二趟b（visitVarDecl 触发 instantiateGeneric），此时实例化类
    //   （向量$整32）刚加入 classes_，其方法体尚未经 checkClassMethods 检查
    //   （resolvedType 未推导，IR 层 handleResultCtor 对 正常()/错误() 降级失败
    //   -> 生成 Call 正常/错误 -> LNK2019）。补一遍：检查本趟新增的实例化类
    //   （名含 $ 的类，原泛型类名不含 $）。
    for (auto& kv : classes_) {
        if (kv.first.find('$') != std::string::npos) {
            if (checkedClasses_.count(kv.first) > 0) continue;  // H3：幂等
            checkedClasses_.insert(kv.first);
            checkClassMethods(const_cast<ClassInfo&>(kv.second));
        }
    }
    popScope();
}
void SemanticAnalyzer::visitStructDecl(StructDecl* node) {
    // 由 visitProgram 驱动注册/布局；单独访问时仅注册类型名（防御性）
    declareTypeName(node->name, node->moduleName, node->location);
}
void SemanticAnalyzer::visitEnumDecl(EnumDecl* node) {
    computeEnumValues(node);
}
bool SemanticAnalyzer::analyze(Program* program) {
    if (program == nullptr) return false;
    visitProgram(program);
    return !diagnostics_.hasErrors();
}
std::string SemanticAnalyzer::checkExpr(Expr* node) {
    if (node == nullptr) return "未知";
    node->accept(*this);
    return lastType_;
}
void SemanticAnalyzer::checkBlock(BlockStmt* node) {
    if (node == nullptr) return;
    visitBlockStmt(node);
}
void SemanticAnalyzer::checkStmt(Stmt* node) {
    if (node == nullptr) return;
    node->accept(*this);
}
} // namespace cn_compiler
