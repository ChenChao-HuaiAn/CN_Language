// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

namespace {

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

} // namespace

// ==================== 符号表管理 ====================

// 进入新作用域（压栈一个空作用域）
void SemanticAnalyzer::pushScope() {
    scopes_.emplace_back();
}

// 退出当前作用域（弹栈，作用域栈始终至少保留全局层）
void SemanticAnalyzer::popScope() {
    if (scopes_.size() > 1) scopes_.pop_back();
}

// 注册结构体/枚举类型名（顶层类型表，供变量声明/字段访问使用，Task 2.7）
// 重复注册（同名结构体/枚举）报错
void SemanticAnalyzer::declareTypeName(const std::string& name, const SourceLocation& loc) {
    if (typeNames_.find(name) != typeNames_.end()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "重复声明类型 '" + name + "'");
        return;
    }
    typeNames_.insert(name);
}

// 是否结构体/联合体类型名（Task 2.7）
// 阶段3（Task 3.5）：结果<T,E>/可选<T> 降级为合成结构体（结果$T$E / 可选$T），
//   模板类型名映射到合成名后同样视为结构体（供 IR 层 findStruct/typeSizeOf 使用）
bool SemanticAnalyzer::isStructType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    for (const auto& s : program_->structs) {
        if (s->name == type) return true;
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

// 是否枚举类型名（Task 2.7）
bool SemanticAnalyzer::isEnumType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    for (const auto& e : program_->enums) {
        if (e->name == type) return true;
    }
    return false;
}

// 查找结构体/联合体定义（未找到返回nullptr）
// 阶段3（Task 3.5）：结果<T,E>/可选<T> 模板类型名映射到合成结构体名
const StructDecl* SemanticAnalyzer::findStruct(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    for (const auto& s : program_->structs) {
        if (s->name == name) return s.get();
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

// 查找枚举定义（未找到返回nullptr）
const EnumDecl* SemanticAnalyzer::findEnum(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    for (const auto& e : program_->enums) {
        if (e->name == name) return e.get();
    }
    return nullptr;
}

// 查找枚举成员值（未找到返回false，Task 2.7）
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

// 查询函数返回类型（未注册返回空串；供IR层推导调用结果类型）
// 集成验证发现：用户函数返回浮64 时 IR 调用结果类型误标 i32，导致
// codegen 用 eax 读 xmm0 返回值（除零崩溃）——Task 2.7 修复
// Task 2.10：key 可为"名#参数串"（重载签名）或纯函数名（内置函数/单版本查询）
std::string SemanticAnalyzer::funcReturnTypeOf(const std::string& funcName) const {
    auto it = functions_.find(funcName);
    if (it == functions_.end()) return "";
    return it->second.returnType;
}

// 查询函数参数类型列表（未注册返回空向量；供IR层推导 i128 实参是否需截断，
// 集成验证修复：i128 实参传给 i128 参数时不得截断为 i64）
std::vector<std::string> SemanticAnalyzer::funcParamTypesOf(const std::string& funcName) const {
    auto it = functions_.find(funcName);
    if (it == functions_.end()) return {};
    return it->second.paramTypes;
}

// 签名 key：名 + "#" + 参数类型串（逗号分隔）。重载决议与 mangling 共用，
// 保证"同名不同签名"各自唯一；仅返回类型不同不会改变 key（不构成重载）。
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

// 是否存在该函数名的任意签名
bool SemanticAnalyzer::hasFunctionName(const std::string& name) const {
    if (functions_.find(name) != functions_.end()) return true;  // 内置纯名 key
    for (const auto& kv : functions_) {
        // 用户函数 key 形如 名#参数串；纯名内置 key 无 '#'
        const std::size_t hashPos = kv.first.find('#');
        const std::string base = (hashPos == std::string::npos) ? kv.first
                                                                : kv.first.substr(0, hashPos);
        if (base == name) return true;
    }
    return false;
}

// 返回该函数名的第一个签名 key（函数名作值/取地址用）
// 修复（Linux 移植）：原实现遍历 unordered_map（functions_），返回顺序由哈希表
//   布局决定——MSVC 与 GCC 的哈希顺序不同，导致"函数名作值"选中的重载签名不稳定
//   （OverloadTest.FuncNameAsValue 在 GCC 下选到 加#浮64,浮64，类型不匹配失败）。
//   改为确定性选择：取同名签名 key 字典序最小者（与"注册顺序"语义一致，
//   跨编译器稳定，不依赖哈希顺序）。
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

// 实参类型到参数类型的转换等级（重载决议用）：
//   0 = 精确匹配（canonical 后相等，含枚举同名）
//   1 = 宽化（整型同符号向宽 / 浮点向宽 / 枚举->整型）
//   2 = 隐式转换（canConvertType 其余合法场景：整->浮、字符->整等）
//  -1 = 不可转换
int SemanticAnalyzer::conversionLevel(const std::string& argTypeRaw,
                                      const std::string& paramTypeRaw) {
    const std::string arg = canonicalType(argTypeRaw);
    const std::string param = canonicalType(paramTypeRaw);
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

// 重载决议：实参类型列表 -> 最佳匹配签名（返回签名 key）。
// 匹配规则（规格书04-一B）：
//   1. 参数个数：实参个数在 [最少必填, 参数总数] 区间（默认参数补全参与）
//   2. 逐参数计算转换等级；按"总等级"比较（精确优先于宽化优先于隐式转换）
//   3. 唯一最佳 -> 返回其签名 key；多个同样优 -> 歧义错误（返回空串）
std::string SemanticAnalyzer::resolveOverload(const std::string& name,
                                              const std::vector<std::string>& argTypes,
                                              const SourceLocation& loc) {
    std::string bestKey;
    int bestTotal = INT32_MAX;
    bool ambiguous = false;
    std::string ambiguousDetail;
    for (const auto& kv : functions_) {
        const std::size_t hashPos = kv.first.find('#');
        const std::string base = (hashPos == std::string::npos) ? kv.first
                                                                : kv.first.substr(0, hashPos);
        if (base != name) continue;  // 仅同名的签名参与决议
        const FunctionInfo& info = kv.second;
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

// 扩展隐式转换（Task 2.7 + 阶段3）：枚举↔整数；枚举间须同名；结构体须同名；
// 结果<T,E>/可选<T> 模板类型须同模板结构（Task 3.5）
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
    return types::canConvert(from, to);
}

// 计算基本类型/指针/数组的大小（Task 2.7）
// 基本类型见 type_system.hpp 的 typeSize；指针恒为8；数组 = 元素大小*长度；
// 枚举 = 4（整32）；结构体 = 布局后总大小
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

// 计算类型的对齐（Task 2.7，C风格：按字段类型对齐，结构体对齐 = 最大成员对齐）
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

// 查找结构体字段偏移（-1表示无此字段；联合体字段偏移恒为0）
// 阶段3（Task 3.5）：结果/可选 合成结构体成员名映射——源码访问 .正常/.值/.错误/
//   .有值，合成结构体字段为 是否正常/值/错误值/是否某些，此处统一映射查偏移。
//   注意：结果<T,E> 的 .值/.错误 位于内层联合体（错误值联合，偏移=联合体字段偏移）；
//   可选<T> 的 .值 是外层字段（值，偏移=字段偏移）。
int SemanticAnalyzer::fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const {
    for (const auto& f : decl->fields) {
        if (f.name == fieldName) {
            // Task 6.1（ensureLoweredType 生成的合成结构体）：错误值联合
            //   （.值/.错误 所在）布局可能因联合体 align 时机返回错误偏移
            //   （1 而非 8）——合成结构体首字段 布尔(1) + 8 字节对齐联合体，
            //   统一按 8 字节偏移（与 lowerResultOptionalTypes 标准布局一致）。
            if (fieldName == "错误值联合" && !decl->isUnion && f.offset < 8) return 8;
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
                // Task 6.1（ensureLoweredType 生成的合成结构体）：错误值联合
                //   （.值/.错误 所在）布局可能因联合体 align 时机返回错误偏移
                //   （1 而非 8）——合成结构体首字段 布尔(1) + 8 字节对齐联合体，
                //   统一按 8 字节偏移（与 lowerResultOptionalTypes 标准布局一致）。
                if (mapped == "错误值联合" && !decl->isUnion) return 8;
                return decl->isUnion ? 0 : f.offset;
            }
        }
        // Task 6.1 防御：结果 合成结构体映射到 错误值联合 但字段未找到（布局错），
        //   恒返回 8（标准偏移）。
        if (mapped == "错误值联合" && !decl->isUnion) return 8;
    }
    return -1;
}

// 计算结构体/联合体布局（C风格对齐规则，Task 2.7）
// 结构体：字段按类型对齐放置，总大小对齐到最大成员对齐；
// 联合体：所有字段从偏移0开始，大小 = 最大字段大小（按最大对齐）。
// 递归处理嵌套结构体/数组字段；循环引用（直接/间接包含自身）检测报错
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

// 枚举成员值求值：未赋值自动递增（首个默认0），显式赋值可为负数（Task 2.7）
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

// 声明变量：同作用域重复声明返回false并报告错误（类型统一存规范化形式）
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

// 查找变量类型：从内到外遍历作用域栈
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

// ==================== 类型工具（Task 2.3 已抽取到 type_system 子模块） ====================
// 说明：isNumeric/isInteger/isFloat/canConvert/commonNumericType 已在 semantic.hpp
//       内联委托 types:: 命名空间（semantic/type_system.hpp），此处不再重复定义。
//       canonicalType 在文件顶部转发 types::canonical。

// ==================== 函数指针类型工具（Task 2.2） ====================

// 判断类型字符串是否为函数指针类型（函数指针<返回>(参数,...)）
bool SemanticAnalyzer::isFuncPtrType(const std::string& type) {
    return isFuncPtrTypeStr(type);
}

// 从函数指针类型字符串提取返回类型（"函数指针<整32>(整32,整32)" -> "整32"）
std::string SemanticAnalyzer::funcPtrReturnOf(const std::string& type) {
    return funcPtrReturn(type);
}

// 从函数指针类型字符串提取参数类型列表
std::vector<std::string> SemanticAnalyzer::funcPtrParamsOf(const std::string& type) {
    return funcPtrParams(type);
}

// 函数指针类型兼容性检查：返回类型可隐式转换（逆变要求从返回精确匹配，保守：完全相等）
// 参数类型逐个可隐式转换（参数支持子类型/宽化，保守：完全相等即可）
// 保守策略：函数指针仅允许"完全同签名"赋值（避免运行期ABI不匹配）。
//   返回类型要求完全相等；参数类型要求逐个可隐式转换（参数类型比返回宽松）。
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

// 数值运算结果类型：已抽取到 type_system 子模块（types::commonNumericType），
// semantic.hpp 内联委托，此处不再重复定义。

// 是否复合赋值运算符
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

// ==================== 检查辅助 ====================

// 检查条件表达式类型：必须为布尔类型
void SemanticAnalyzer::checkCondition(const std::string& type, const SourceLocation& loc,
                                      const std::string& ctx) {
    if (type != "布尔") {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            ctx + "条件必须为布尔类型，实际为 '" + type + "'");
    }
}

// 注册CN语言内置函数符号（阶段一：IO函数 + Task 2.5 字符串API，对应运行时 cnrt 的 extern "C" 导出）
// 打印（字符串/变参）-> 空类型；打印行（字符串/变参）-> 空类型
// Task 2.5 新增：
//   - 打印行 支持多参数（字符串/整数/浮点混合，visitCallExpr 特判，签名仅登记单参版本）
//   - 字符串API：字符串长度/字符串比较/字符串连接/字符串复制/字符串查找
// Task 2.9 语义调整（用户裁决，lessons.md 权重10.4）：
//   - 打印 = println（自动换行）、打印行 = print（不换行）——直觉命名
//   - 格式化（格式字符串, 参数...）-> 字符串（sprintf 风格，调用方释放）
// 方案C（2026-08-14）✅ 已修复：删除遗留的 打印行整数/打印行浮点 注册——
//   旧语义（"行"=换行）与 打印行（"行"=逐行连续输出，不换行）恰好相反，造成二义性。
//   统一用 打印（变参，自动换行）完全等价表达：打印行整数(42) == 打印(42)（输出 42\n）。
//   "行" 语义统一为：不追加换行（打印行）；打印/打印行 均为变参函数。
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
    // 中文名带 "数学." 前缀（形如 模块.函数 限定名），作为**内置函数唯一 key**：
    //   - 与 CN 层模块 stdlib/数学.cn 的公开函数（纯名 平方根 等）不冲突——
    //     模块函数注册为纯名（公开符号合并），内置函数注册为带点限定名
    //   - 调用方式 数学.平方根(值)：visitCallExpr 模块限定重写时，对已注册的
    //     数学.* 内置名特判：不重写为纯名，保留限定名走内置函数路径
    // 运行时符号：数学.平方根 -> __cn_sqrt、数学.幂 -> __cn_pow、
    //   数学.正弦 -> __cn_sin、数学.余弦 -> __cn_cos、数学.正切 -> __cn_tan、
    //   数学.绝对值 -> __cn_fabs、数学.向上取整 -> __cn_ceil、
    //   数学.向下取整 -> __cn_floor（IR 层按函数名映射）
    // 参数/返回均为 浮64（double）；P1 的对数/反三角/随机数留待后续
    const auto regMathFn = [this](const std::string& name, const std::string& retType,
                                  const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regMathFn("数学.平方根", "浮64", {"浮64"});
    regMathFn("数学.幂", "浮64", {"浮64", "浮64"});
    regMathFn("数学.正弦", "浮64", {"浮64"});
    regMathFn("数学.余弦", "浮64", {"浮64"});
    regMathFn("数学.正切", "浮64", {"浮64"});
    regMathFn("数学.绝对值", "浮64", {"浮64"});
    regMathFn("数学.向上取整", "浮64", {"浮64"});
    regMathFn("数学.向下取整", "浮64", {"浮64"});

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

// 第一趟：注册函数符号（支持前向调用与重名检测，类型统一存规范化形式）
// Task 2.2 增强：
//   - 函数指针参数：参数类型用 funcPtr.toString() 规范化表示
//   - 签名一致性：原型声明与定义签名（返回类型+参数类型）必须一致
// Task 2.10 重载：函数符号表 key 改为"名#参数类型串"（signatureKey），
//   同名不同参数类型/个数可共存；仅返回类型不同不构成重载（重复定义报错）。
//   默认参数记录尾部 defaultCount（调用补全用）；默认值表达式留在 AST（IR 层展开）。
void SemanticAnalyzer::registerFunction(FunctionDecl* node) {
    // 阶段3（Task 3.5）：内置构造器 正常/错误/某些 用户不可重定义
    if (node->name == "正常" || node->name == "错误" || node->name == "某些") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "内置构造器 '" + node->name + "' 不可重定义");
        return;
    }
    FunctionInfo info;
    info.returnType = node->returnType.empty() ? "空类型" : canonicalType(node->returnType);
    info.hasBody = (node->body != nullptr);
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
            info.paramTypes.push_back(canonicalType(param->typeName));
        }
    }
    // 生成签名 key（名 + "#" + 参数类型串，mangling 与决议共用）
    node->sigKey = signatureKey(node->name, info.paramTypes);
    auto it = functions_.find(node->sigKey);
    if (it != functions_.end()) {
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
                                "重复定义函数 '" + node->name +
                                "'（参数类型相同；仅返回类型不同不构成重载）");
        }
        return;
    }
    // Task 2.10 重载兼容：签名 key 未命中但同名已有其他签名——
    //   合法重载（加(整32,整32) 与 加(浮64,浮64)）；
    //   但"原型声明 + 不同签名定义"是错误（原型已锁定签名，定义须一致）。
    //   规则：同名存在 原型（无体）且 当前是定义（有体）→ 签名必须与原型一致。
    for (const auto& kv : functions_) {
        const std::size_t hashPos = kv.first.find('#');
        const std::string base = (hashPos == std::string::npos) ? kv.first
                                                                : kv.first.substr(0, hashPos);
        if (base == node->name && !kv.second.hasBody && info.hasBody) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数 '" + node->name +
                                "' 原型声明与定义签名不一致（重载须参数类型不同）");
            return;
        }
    }
    functions_[node->sigKey] = info;
}

// 函数体是否保证有返回：最后一条语句为返回语句或无限循环
bool SemanticAnalyzer::bodyGuaranteesReturn(BlockStmt* body) const {
    if (body == nullptr || body->statements.empty()) return false;
    Stmt* last = body->statements.back().get();
    if (last->getType() == NodeType::ReturnStmt) return true;
    // 无限循环：循环 ( ; ; ) { } 或 循环 { }（无条件表达式）
    if (last->getType() == NodeType::ForStmt) {
        ForStmt* forStmt = static_cast<ForStmt*>(last);
        if (forStmt->condition == nullptr) return true;
    }
    return false;
}

// ==================== 声明节点 ====================

// 程序入口：多趟处理（Task 2.7 增加结构体/枚举注册与布局计算；阶段3 类/接口/泛型/错误降级）
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
    for (auto& imp : node->imports) {
        visitImportDecl(imp.get());
    }
    // 第一趟a：注册全部结构体/联合体/枚举类型名（支持前向引用：字段可引用后定义的类型）
    for (auto& s : node->structs) {
        declareTypeName(s->name, s->location);
    }
    for (auto& e : node->enums) {
        declareTypeName(e->name, e->location);
    }
    // 第一趟b：计算全部结构体/联合体布局（递归，循环引用检测）
    for (auto& s : node->structs) {
        computeLayout(s.get());
    }
    // 第一趟c：枚举成员值求值（自动递增/显式赋值/负数）
    for (auto& e : node->enums) {
        computeEnumValues(e.get());
    }
    // 第一趟d（阶段3）：注册类/接口符号（类名 + 成员解析 + 虚表 + 接口验证 + 布局）
    registerClassAndInterfaces(node);
    // 第一趟e（阶段3）：注册泛型声明（泛型类/函数模板）
    registerGenerics(node);
    // 第一趟f（阶段3）：结果/可选类型降级（生成合成结构体并布局）
    lowerResultOptionalTypes(node);
    // 第一趟g：注册全部函数符号（含前向调用）
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            registerFunction(decl.get());
        }
    }
    // 第二趟a（阶段3）：检查类方法体（自身/父类/访问控制/常量 上下文）
    for (auto& kv : classes_) {
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
            checkClassMethods(const_cast<ClassInfo&>(kv.second));
        }
    }
    popScope();
}

// 结构体/联合体声明：注册类型名（字段布局在 visitProgram 中统一计算）
void SemanticAnalyzer::visitStructDecl(StructDecl* node) {
    // 由 visitProgram 驱动注册/布局；单独访问时仅注册类型名（防御性）
    declareTypeName(node->name, node->location);
}

// 枚举声明：成员值求值（自动递增/显式赋值/负数）
void SemanticAnalyzer::visitEnumDecl(EnumDecl* node) {
    computeEnumValues(node);
}

// 主入口：分析程序AST，返回是否成功
bool SemanticAnalyzer::analyze(Program* program) {
    if (program == nullptr) return false;
    visitProgram(program);
    return !diagnostics_.hasErrors();
}

// 第二趟：检查函数体（参数入作用域 + 语句检查 + 返回类型检查）
void SemanticAnalyzer::checkFunctionBody(FunctionDecl* node) {
    // 函数符号必须已注册（原型声明无函数体）。Task 2.10：按签名 key 查询
    auto it = functions_.find(node->sigKey);
    if (it == functions_.end()) return;
    if (node->body == nullptr) return;  // 函数原型声明：无需检查体

    currentReturnType_ = it->second.returnType;
    // 阶段3（Task 3.9）：记录当前上下文函数名（友元函数访问检查用）
    currentFunctionName_ = node->name;
    pushScope();  // 参数作用域
    for (auto& param : node->params) {
        // 函数指针参数：类型为 funcPtr 规范化字符串；普通参数用 typeName
        std::string paramType = param->funcPtr.isFunctionPtr()
                                    ? param->funcPtr.toString()
                                    : param->typeName;
        if (!declareVar(param->name, paramType, param->location)) {
            // 重复声明参数
        }
    }
    // 检查函数体
    for (auto& stmt : node->body->statements) {
        checkStmt(stmt.get());
    }
    // 缺少返回语句检查：有返回类型且函数体不保证返回
    if (currentReturnType_ != "空类型" && !bodyGuaranteesReturn(node->body.get())) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "函数 '" + node->name + "' 缺少返回语句，返回类型为 '" +
                            currentReturnType_ + "'");
    }
    currentReturnType_.clear();
    currentFunctionName_.clear();  // 阶段3：退出函数上下文
    popScope();
}

void SemanticAnalyzer::visitFunctionDecl(FunctionDecl* node) {
    // 由 visitProgram 两趟驱动；单独访问时仅注册（防御性）
    registerFunction(node);
}

void SemanticAnalyzer::visitParamDecl(ParamDecl* node) {
    // 参数由 checkFunctionBody 统一入符号表，这里不做处理
    (void)node;
}

// 变量声明：类型检查 + 类型推断 + 入符号表
void SemanticAnalyzer::visitVarDecl(VarDecl* node) {
    std::string varType = node->typeName;
    // Task 2.2：函数指针变量声明（整32(*回调)(整32, 整32) 无typeName，funcPtr非空）
    if (node->funcPtr.isFunctionPtr()) {
        varType = node->funcPtr.toString();
    }
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化类型替换提前——
    //   名<实参> -> 实例化类名（容器<整32> -> 容器$整32）。必须在初始值检查
    //   （checkExpr 触发实例化构造，返回 容器$整32）之前替换 varType，
    //   否则类型匹配（容器<整32> vs 容器$整32）失败。
    varType = resolveGenericTypeName(varType, node->location);
    if (varType.empty() && node->initializer != nullptr) {
        // 类型推断：无显式类型时从初始值推断
        varType = checkExpr(node->initializer.get());
    } else if (node->initializer != nullptr && !varType.empty()) {
        // 数组初始化列表（Task 2.4）：整32[5] 数据 = { 1, 2, 3 }
        if (node->initializer->getType() == NodeType::InitListExpr &&
            types::isArray(varType)) {
            InitListExpr* initList = static_cast<InitListExpr*>(node->initializer.get());
            const std::string elemType = types::arrayElemOf(varType);
            const int arrayLen = types::arrayLenOf(varType);
            // 每个元素须可隐式转换为数组元素类型
            for (auto& elem : initList->elements) {
                std::string elemInitType = checkExpr(elem.get());
                if (!canConvert(elemInitType, elemType)) {
                    diagnostics_.report(DiagnosticLevel::Error, elem->location,
                                        "数组元素无法将 '" + elemInitType +
                                        "' 隐式转换为 '" + elemType + "'");
                }
            }
            // 初始化元素个数不得超过数组长度（部分初始化允许，剩余补零）
            if (arrayLen > 0 && static_cast<int>(initList->elements.size()) > arrayLen) {
                diagnostics_.report(DiagnosticLevel::Error, node->initializer->location,
                                    "数组初始化元素个数 " +
                                    std::to_string(initList->elements.size()) +
                                    " 超过数组长度 " + std::to_string(arrayLen));
            }
        } else if (node->initializer->getType() == NodeType::InitListExpr &&
                   !types::isArray(varType)) {
            // 初始化列表用于非数组类型：报错
            checkExpr(node->initializer.get());
        } else if (node->initializer->getType() == NodeType::StructInitExpr) {
            // 结构体/联合体初始化列表：类型须匹配（Task 2.7）
            std::string initType = checkExpr(node->initializer.get());
            if (varType != "未知" && !canConvertType(initType, varType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->initializer->location,
                                    "无法将 '" + initType + "' 隐式转换为 '" + varType + "'");
            }
        } else {
            // 显式类型：检查初始值可隐式转换
            std::string initType = checkExpr(node->initializer.get());
            if (!canConvertType(initType, varType)) {
                // Task 2.3：字面量常量窄化（整8 a = 10：10 默认整32，但值是编译期
                // 常量且适配目标位宽）——无后缀整数字面量允许窄化到目标整数类型；
                // 非字面量（变量/表达式）仍按严格隐式转换规则拒绝窄化
                const bool isIntLiteral =
                    (node->initializer->getType() == NodeType::IntegerLiteral) &&
                    types::literalTypeOf(
                        static_cast<IntegerLiteral*>(node->initializer.get())->raw, false) == "整32" &&
                    types::isInteger(varType);
                if (!isIntLiteral) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "无法将 '" + initType + "' 隐式转换为 '" + varType + "'");
                }
            }
        }
    }
    if (varType.empty()) {
        // 无类型标注且无初始值：类型未知
        varType = "未知";
    }
    declareVar(node->name, varType, node->location);
}

// ==================== 语句节点 ====================

// 代码块：进入子作用域检查
void SemanticAnalyzer::visitBlockStmt(BlockStmt* node) {
    pushScope();
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
    popScope();
}

void SemanticAnalyzer::visitExprStmt(ExprStmt* node) {
    const std::string exprType = checkExpr(node->expr.get());
    // 规则1（Task 3.5）：结果<T,E> 返回值被丢弃未检查 -> 错误
    // 仅函数调用表达式触发（普通表达式如 结果.正常 读取本身即检查）
    if (node->expr->getType() == NodeType::CallExpr) {
        checkResultDiscard(exprType, node->expr->location);
    }
}

// 如果语句：条件必须为布尔；分支各自进入子作用域
// 阶段3（Task 3.5）：条件为 结果.正常/可选.有值 时走错误码传播跟踪
//   （真分支可访问 .值；否则分支可访问 .错误；规则2 检查未处理错误分支）
void SemanticAnalyzer::visitIfStmt(IfStmt* node) {
    // 结果/可选 检查跟踪（条件为 结果.正常 / 可选.有值 时专用处理）
    if (node->condition->getType() == NodeType::MemberExpr) {
        MemberExpr* cond = static_cast<MemberExpr*>(node->condition.get());
        const std::string condObjType = checkExpr(cond->object.get());
        const bool isResultCheck =
            isResultType(condObjType) && cond->memberName == "正常";
        const bool isOptionalCheck =
            isOptionalType(condObjType) && cond->memberName == "有值";
        if (isResultCheck || isOptionalCheck) {
            trackIfCheck(node);
            return;
        }
    }
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'如果'");
    if (node->thenBranch != nullptr) checkBlock(node->thenBranch.get());
    if (node->elseBranch != nullptr) checkStmt(node->elseBranch.get());
}

// 当循环：条件必须为布尔；循环体进入子作用域且允许中断/继续
void SemanticAnalyzer::visitWhileStmt(WhileStmt* node) {
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'当'");
    loopDepth_++;
    if (node->body != nullptr) checkBlock(node->body.get());
    loopDepth_--;
}

// 循环语句：初始化/条件/更新检查；循环体进入子作用域且允许中断/继续
void SemanticAnalyzer::visitForStmt(ForStmt* node) {
    if (node->init != nullptr) checkStmt(node->init.get());
    if (node->condition != nullptr) {
        checkCondition(checkExpr(node->condition.get()), node->condition->location, "'循环'");
    }
    if (node->update != nullptr) checkExpr(node->update.get());
    loopDepth_++;
    if (node->body != nullptr) checkBlock(node->body.get());
    loopDepth_--;
}

// 返回语句：检查返回值类型与函数返回类型匹配
void SemanticAnalyzer::visitReturnStmt(ReturnStmt* node) {
    // lambda 返回类型推导模式（Task 2.10）：currentReturnType_ 为空，
    // 不报"只能出现在函数体内"，而是收集返回表达式类型作为推导候选
    if (currentReturnType_.empty()) {
        if (lambdaInferMode_) {
            if (node->value != nullptr) {
                lambdaReturnCandidate_.push_back(checkExpr(node->value.get()));
            } else {
                lambdaReturnCandidate_.push_back("空类型");
            }
            return;
        }
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'返回'语句只能出现在函数体内");
        return;
    }
    if (node->value == nullptr) {
        // 无返回值：要求函数返回类型为空类型
        if (currentReturnType_ != "空类型") {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数返回类型为 '" + currentReturnType_ +
                                "'，但返回语句未携带值");
        }
        return;
    }
    std::string valueType = checkExpr(node->value.get());
    if (currentReturnType_ == "空类型") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "空类型函数不允许返回值");
    } else if (!canConvertType(valueType, currentReturnType_)) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + valueType + "' 隐式转换为返回类型 '" +
                            currentReturnType_ + "'");
    }
}

// 中断语句：在循环内或选择语句内合法（跳出循环/跳出选择）
void SemanticAnalyzer::visitBreakStmt(BreakStmt* node) {
    if (loopDepth_ == 0 && switchDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'中断'语句只能出现在循环体内或选择语句内");
    }
}

// 继续语句：只能在循环内使用（选择语句内继续不合法）
void SemanticAnalyzer::visitContinueStmt(ContinueStmt* node) {
    if (loopDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'继续'语句只能出现在循环体内");
    }
}

// 选择语句：条件表达式必须为整型/字符；情况值去重；分支体进入子作用域检查
// switchDepth_ 允许 '中断' 跳出选择（与循环的中断语义一致）
void SemanticAnalyzer::visitSwitchStmt(SwitchStmt* node) {
    std::string condType = checkExpr(node->condition.get());
    // Task 2.7：枚举条件允许（枚举本质为整32，case 值为枚举成员整数值）
    if (!isInteger(condType) && condType != "字符" && !isEnumType(condType)) {
        diagnostics_.report(DiagnosticLevel::Error, node->condition->location,
                            "选择语句的表达式必须是整数或字符类型，实际为 '" +
                            condType + "'");
    }
    // 情况值去重检测（编译期常量，语义层用 set 去重）
    std::unordered_set<std::int64_t> seenValues;
    switchDepth_++;
    for (auto& caseNode : node->cases) {
        // 枚举引用情况值求值：rawValue 形如 "颜色.红"（Task 2.7）
        if (caseNode->rawValue.find('.') != std::string::npos) {
            const std::size_t dotPos = caseNode->rawValue.find('.');
            const std::string enumName = caseNode->rawValue.substr(0, dotPos);
            const std::string memberName = caseNode->rawValue.substr(dotPos + 1);
            std::int64_t enumVal = 0;
            if (enumValueOf(enumName, memberName, enumVal)) {
                caseNode->value = enumVal;
            } else {
                diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                    "情况标签枚举引用无效：'" + caseNode->rawValue + "'");
            }
        }
        // 重复检测：与已见情况值比较
        if (!seenValues.insert(caseNode->value).second) {
            diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                "选择语句中情况值 '" + caseNode->rawValue +
                                "' 重复");
        }
        checkStmt(caseNode.get());
    }
    if (node->defaultCase != nullptr) {
        checkStmt(node->defaultCase.get());
    }
    switchDepth_--;
}

// 情况标签：语句体检查（每个分支作为独立语句序列检查）
void SemanticAnalyzer::visitCaseLabel(CaseLabel* node) {
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
}

// 默认标签：语句体检查
void SemanticAnalyzer::visitDefaultLabel(DefaultLabel* node) {
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
}

// ==================== 表达式节点 ====================

// 整数字面量（Task 2.3：后缀决定推断类型，规格书4.3）
// 无后缀 -> 整32；L -> 整64；LL -> 整128；U -> 正32；UL -> 正64；ULL -> 正128
void SemanticAnalyzer::visitIntegerLiteral(IntegerLiteral* node) {
    lastType_ = types::literalTypeOf(node->raw, false);
    if (lastType_.empty()) lastType_ = "整32";  // 非法后缀防御性回退
    // Task 完善A：i128/正128 字面量越界检查（规格书4.3 字面量范围）——
    //   整128（有符号）上限 2^127-1、正128（无符号）上限 2^128-1。
    //   超限立即报错（IR 层同样防御性检查，语义层先拦截供诊断）。
    //   注意：无后缀超 int64 的字面量（如 2^127）IR 层会提升为整128，
    //   此处同样按整128 上限检查（2^127 超出 2^127-1 报错）。
    const std::string stripped = types::stripLiteralSuffix(node->raw);
    if (lastType_ == "整128" || lastType_ == "正128" ||
        (lastType_ == "整32" && types::textExceedsInt64(stripped))) {
        const bool isSigned = (lastType_ != "正128");
        const std::string limit = isSigned
                                      ? "170141183460469231731687303715884105727"  // 2^127-1
                                      : "340282366920938463463374607431768211455";  // 2^128-1
        if (stripped.size() > limit.size() ||
            (stripped.size() == limit.size() && stripped > limit)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "整数字面量超出" +
                                    std::string(isSigned ? "整128（2^127-1）"
                                                         : "正128（2^128-1）") +
                                    "范围");
        }
    }
}

// 浮点字面量（Task 2.3：f后缀 -> 浮32，无后缀 -> 浮64）
void SemanticAnalyzer::visitFloatLiteral(FloatLiteral* node) {
    lastType_ = types::literalTypeOf(node->raw, true);
    (void)node;
}

// 字符串字面量：类型为字符串
void SemanticAnalyzer::visitStringLiteral(StringLiteral* node) {
    lastType_ = "字符串";
    (void)node;
}

// 字符字面量：类型为字符
void SemanticAnalyzer::visitCharLiteral(CharLiteral* node) {
    lastType_ = "字符";
    (void)node;
}

// 布尔字面量：类型为布尔
void SemanticAnalyzer::visitBoolLiteral(BoolLiteral* node) {
    lastType_ = "布尔";
    (void)node;
}

// 标识符表达式：从符号表查找变量或函数
// Task 2.2：函数名作值（不加括号）时类型为"函数指针<返回>(参数,...)"，用于赋值给函数指针变量
// 阶段3：类/接口/泛型类型名识别（类名.静态成员 引用、泛型实例化 类型名<实参>）
void SemanticAnalyzer::visitIdentifierExpr(IdentifierExpr* node) {
    std::string varType;
    if (lookupVar(node->name, varType)) {
        lastType_ = varType;
        return;
    }
    // 枚举/结构体/类/接口类型名作标识符（供 枚举名.成员、&结构体、类名.静态成员，Task 2.7/3.x）
    if (isEnumType(node->name) || isStructType(node->name) ||
        isClassType(node->name) || isInterfaceType(node->name)) {
        lastType_ = node->name;
        return;
    }
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化类型名 名<实参>（如 盒子<整32>）
    //   作标识符（构造调用 callee / 类型引用）——触发单态化，返回实例化类名。
    const std::size_t genLt = node->name.find('<');
    const std::size_t genGt = node->name.rfind('>');
    if (genLt != std::string::npos && genGt != std::string::npos &&
        genGt > genLt) {
        const std::string head = node->name.substr(0, genLt);
        if (findGeneric(head) != nullptr) {
            const std::string inner =
                node->name.substr(genLt + 1, genGt - genLt - 1);
            std::vector<std::string> args;
            std::size_t pos = 0;
            while (pos <= inner.size()) {
                const std::size_t comma = inner.find(',', pos);
                if (comma == std::string::npos) {
                    args.push_back(inner.substr(pos));
                    break;
                }
                args.push_back(inner.substr(pos, comma - pos));
                pos = comma + 1;
            }
            for (auto& a : args) {
                const std::size_t b = a.find_first_not_of(" \t");
                const std::size_t e = a.find_last_not_of(" \t");
                if (b != std::string::npos && e != std::string::npos) {
                    a = a.substr(b, e - b + 1);
                }
            }
            const std::string instName =
                instantiateGeneric(head, args, node->location);
            if (!instName.empty()) {
                lastType_ = instName;
                return;
            }
        }
    }
    // 函数名作为值（Task 2.10 重载）：构造函数指针类型。
    // 有多个签名时取第一个（确定性选择，见 funcFirstSigKey 修复——原实现
    //   遍历 unordered_map 依赖哈希顺序，GCC/MSVC 平台行为不一致）。
    // 注：重载函数作函数指针值语义未定义（C++ 需显式类型化），此处保守取首签名，
    //     并允许 回调 = 加 单版本场景（既有测试契约）。
    if (hasFunctionName(node->name)) {
        const std::string sig = funcFirstSigKey(node->name);
        if (!sig.empty()) {
            const auto it = functions_.find(sig);
            if (it != functions_.end()) {
                const FunctionInfo& info = it->second;
                std::string funcPtrType = "函数指针<" + info.returnType + ">(";
                for (std::size_t i = 0; i < info.paramTypes.size(); ++i) {
                    if (i > 0) funcPtrType += ",";
                    funcPtrType += info.paramTypes[i];
                }
                funcPtrType += ")";
                lastType_ = funcPtrType;
                return;
            }
        }
    }
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "未声明的标识符 '" + node->name + "'");
    lastType_ = "未知";
}

// 二元运算：操作数类型兼容检查 + 结果类型推导
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

    if (isComparisonOp(node->op)) {
        // 比较运算：要求可互相转换的同类操作数，结果为布尔
        if (leftType == "未知" || rightType == "未知") {
            lastType_ = "布尔";
            return;
        }
        // 指针比较（Task 2.4）：两指针（或指针与空指针）按地址比较；
        // 指针与整型禁止隐式比较（规格书3.7：指针与整数禁止隐式转换）
        const bool leftPtr = isPointerType(leftType);
        const bool rightPtr = isPointerType(rightType);
        if ((leftPtr || rightPtr) && !(leftPtr && rightPtr)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "指针只能与指针或空指针比较，实际为 '" + leftType +
                                "' 与 '" + rightType + "'");
            lastType_ = "布尔";
            return;
        }
        if (!canConvert(leftType, rightType) && !canConvert(rightType, leftType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "比较运算操作数类型不兼容：'" + leftType + "' 与 '" +
                                rightType + "'");
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
        //       字符串类型（字符串/字符*）的 + 视为连接（字符* 也承载字符串语义）
        const bool leftStr = (leftType == "字符串" || leftType == "字符*");
        const bool rightStr = (rightType == "字符串" || rightType == "字符*");
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

// 一元运算：! 要求布尔，- ~ 要求数值/整数，++ -- 要求左值数值
void SemanticAnalyzer::visitUnaryExpr(UnaryExpr* node) {
    std::string operandType = checkExpr(node->operand.get());
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
            } else {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'*'解引用要求指针操作数，实际为 '" + operandType + "'");
                lastType_ = "未知";
            }
            break;
        case Operator::Increment:
        case Operator::Decrement:
            // 自增/自减：数值 或 指针（Task 2.4 指针 ++/-- 按元素大小步进）
            if (!isNumeric(operandType) && !isPointerType(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "自增/自减要求数值或指针操作数，实际为 '" +
                                    operandType + "'");
            }
            lastType_ = operandType;
            break;
        default:
            lastType_ = operandType;
            break;
    }
}

// 三元条件表达式（Task 2.9，规格书4.5）：条件 ? 真值 : 假值
// 规则：① 条件必须为布尔；② 真/假分支类型统一——数值用 commonNumericType 宽化合并，
//      字符串/指针/结构体 等非数值类型须两分支完全一致（禁止 字符串 vs 整32 分支混用，
//      如需混用请在真/假分支内用字符串拼接/格式化显式转字符串）。
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

// 赋值表达式：左值须可写（标识符/下标/解引用），类型兼容检查
// 阶段3（Task 3.9）：常量成员函数体内禁止修改成员（赋值目标为 自身.字段 时报错）
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
    // 检查左值（标识符/下标访问/解引用为可写左值；Task 2.4 扩展下标与解引用）
    std::string targetType = "未知";
    if (node->target->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
        std::string varType;
        if (lookupVar(ident->name, varType)) {
            targetType = varType;
        } else {
            diagnostics_.report(DiagnosticLevel::Error, ident->location,
                                "赋值目标未声明：'" + ident->name + "'");
        }
    } else if (node->target->getType() == NodeType::IndexExpr ||
               node->target->getType() == NodeType::UnaryExpr) {
        // 下标访问（数组[i]）/解引用（*p）均为可写左值
        targetType = checkExpr(node->target.get());
    } else {
        // 其他左值形式（成员访问等）：后续Task实现
        targetType = checkExpr(node->target.get());
    }

    // 检查右值
    std::string valueType = checkExpr(node->value.get());

    // 复合赋值：+= -= 等要求数值
    if (isCompoundAssign(node->op)) {
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

    // 简单赋值 =：要求右值可隐式转换为左值类型
    if (targetType != "未知" && valueType != "未知" &&
        !canConvertType(valueType, targetType)) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + valueType + "' 隐式转换为 '" + targetType + "'");
    }
    lastType_ = targetType == "未知" ? valueType : targetType;
}

// 函数调用：检查被调者与实参数量/类型
// Task 2.2：支持两种调用——直接函数名调用、函数指针变量间接调用
// Task 2.10：直接函数名调用改为重载决议（按实参个数+类型匹配签名，
//   支持默认参数补全；仅返回类型不同不构成重载）；决议结果写回
//   node->resolvedSignature（IR 层按此生成 mangled 符号）
void SemanticAnalyzer::visitCallExpr(CallExpr* node) {
    // 分派依据：callee 若是函数名（在函数符号表中）→ 直接调用；
    //           否则检查其类型，若是函数指针变量 → 间接调用；
    //           阶段3：成员方法调用（对象.方法(...)）、内置构造器（正常/错误/某些）、
    //           泛型实例化（类型名<实参>(...)）
    // ---- 阶段3（Task 3.6）：模块限定调用 模块.函数(实参) ----
    // 语法：导入 数学.平方根 后调用 数学.平方根(16.0)——parseCallOrMember 将其
    //   解析为 CallExpr(MemberExpr(标识符"数学", "平方根"))。
    // 识别：object 为标识符且名字在 importedModules_（已导入模块名）中，
    //   且该名字不是类型名（结构体/枚举/类）→ 重写 callee 为直接函数名，
    //   复用下方"直接函数名调用"路径（重载决议/参数检查/IR 符号生成均无需改动）。
    // Task 6.3 数学内置函数特判：内置函数注册为带点限定名（数学.平方根）。
    //   优先级：用户模块函数优先——若模块 数学 已导入且公开符号合并后存在纯名
    //   平方根（用户自定义 数学.cn 的公开函数），走"普通模块函数"路径（重写为纯名）；
    //   否则若限定名是已注册内置函数（数学.平方根 全局注册，无需导入，
    //   如 stdlib/数学.cn 模块体内直接写 数学.平方根(值)），保留限定名走内置路径。
    //   即：内置限定名仅在"无同名用户模块公开函数"时生效，二者不冲突。
    if (node->callee->getType() == NodeType::MemberExpr) {
        MemberExpr* mem = static_cast<MemberExpr*>(node->callee.get());
        if (!mem->isArrow) {
        if (mem->object->getType() == NodeType::IdentifierExpr) {
            // 值拷贝：重写会销毁旧 MemberExpr（mem->object 悬垂），须先取名字
            const std::string moduleName =
                static_cast<IdentifierExpr*>(mem->object.get())->name;
            const bool isTypeName = isStructType(moduleName) || isEnumType(moduleName) ||
                                    findClass(moduleName) != nullptr ||
                                    findInterface(moduleName) != nullptr;
            // 先取函数名到局部变量（下方重写会销毁旧 MemberExpr，mem 悬垂！）
            // Task 6.1：模块限定泛型调用 核心.交换<整32>(...)——memberName
            //   已含泛型实参（交换<整32>），限定名携带 <...> 供下方单态化识别。
            const std::string funcName = mem->memberName;
            const std::string qualified = moduleName + "." + funcName;
            // Task 6.1：泛型函数名剥离 <实参> 查纯名（交换<整32> -> 交换），
            //   泛型函数以纯名注册（registerGenerics），用户公开函数判定用纯名
            const std::size_t funcGenLt = funcName.find('<');
            const std::string funcBaseName =
                (funcGenLt == std::string::npos) ? funcName : funcName.substr(0, funcGenLt);
            // 已导入模块的公开函数优先（用户模块 数学.cn 的公开符号合并为纯名）。
            // Task 6.1：泛型函数注册在 generics_（非 functions_），hasFunctionName
            //   查不到——补充 findGeneric 判定（泛型函数以纯名注册，可跨模块实例化）。
            const bool moduleImported = importedModules_.count(moduleName) > 0;
            const bool userFuncExists =
                moduleImported &&
                (hasFunctionName(funcBaseName) || findGeneric(funcBaseName) != nullptr);
            if (!isTypeName &&
                (userFuncExists || moduleImported || hasFunctionName(qualified))) {
                // 用户模块公开函数：重写为直接函数名（成员方法调用分支不再命中 MemberExpr）。
                //   泛型函数保留 名<实参> 完整形态（下方 visitCallExpr 泛型单态化识别）；
                //   普通函数重写为纯名（含 数学.平方根 内置限定名的既有路径）。
                if (userFuncExists) {
                    node->callee = std::make_unique<IdentifierExpr>(funcName);
                } else if (hasFunctionName(qualified)) {
                    // 数学库内置函数（数学.平方根 等）：保留限定名作标识符
                    node->callee = std::make_unique<IdentifierExpr>(qualified);
                } else {
                    // 已导入模块但符号不存在 → 报错，避免走"函数指针间接调用"静默路径
                    node->callee = std::make_unique<IdentifierExpr>(funcName);
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "模块 '" + moduleName + "' 没有公开符号 '" +
                                            funcName + "'");
                }
            }
        }
    }
    }  // 模块限定调用重写块结束（Task 3.6）
    bool isDirect = false;
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        // ---- Task 6.1（泛型函数调用打通）：函数名<类型>(实参) 泛型实例化调用 ----
        // 语法：最小<整32>(3, 7)——parser 把 callee 生成 IdentifierExpr("最小<整32>")。
        // 26_generics 遗留限制「泛型函数调用单态化注册未接入」：语义层此前只对
        //   泛型类构造（名<实参>(...)）触发单态化，泛型函数调用落入"非函数类型"错误。
        // 本子任务打通：识别 名<类型> 形态，若 名 是已注册泛型函数 -> 触发单态化
        //   （instantiateGeneric 注册 名$实参 函数符号），重写 callee 为实例化名，
        //   复用下方"直接函数名调用"路径（重载决议/参数检查/IR 符号生成均无需改动）。
        const std::size_t genLt = calleeName.find('<');
        const std::size_t genGt = calleeName.rfind('>');
        if (genLt != std::string::npos && genGt != std::string::npos &&
            genGt > genLt) {
            const std::string head = calleeName.substr(0, genLt);
            if (findGeneric(head) != nullptr &&
                findGeneric(head)->ast->innerFunc != nullptr) {
                const std::string inner =
                    calleeName.substr(genLt + 1, genGt - genLt - 1);
                std::vector<std::string> args;
                std::size_t pos = 0;
                while (pos <= inner.size()) {
                    const std::size_t comma = inner.find(',', pos);
                    if (comma == std::string::npos) {
                        args.push_back(inner.substr(pos));
                        break;
                    }
                    args.push_back(inner.substr(pos, comma - pos));
                    pos = comma + 1;
                }
                for (auto& a : args) {
                    const std::size_t b = a.find_first_not_of(" \t");
                    const std::size_t e = a.find_last_not_of(" \t");
                    if (b != std::string::npos && e != std::string::npos) {
                        a = a.substr(b, e - b + 1);
                    }
                    // Task 6.1（嵌套泛型 链表$整32 内 节点<T>() 构造）：类型实参
                    //   T 替换为当前泛型上下文实参（整32）——否则 节点$T 实例化失败。
                    auto pit = genericTypeParams_.find(a);
                    if (pit != genericTypeParams_.end()) a = pit->second;
                }
                const std::string instName =
                    instantiateGeneric(head, args, node->location);
                if (!instName.empty()) {
                    // 重写 callee 为实例化函数名（名$实参），直接函数调用路径命中
                    node->callee = std::make_unique<IdentifierExpr>(instName);
                    calleeName = instName;
                    // 登记泛型函数实例化记录（供 IR 层生成函数体）：
                    //   记录 实例化名 + 原泛型声明 + 类型实参（替换类型参数用）
                    const GenericInfo* ginfo = findGeneric(head);
                    if (ginfo != nullptr && ginfo->ast->innerFunc != nullptr) {
                        bool exists = false;
                        for (const auto& gi : genericFuncInstances_) {
                            if (gi.instanceName == instName) { exists = true; break; }
                        }
                        if (!exists) {
                            GenericFuncInstance gfi;
                            gfi.instanceName = instName;
                            gfi.gen = ginfo->ast;
                            gfi.args = args;
                            genericFuncInstances_.push_back(std::move(gfi));
                        }
                    }
                }
            }
        }
        if (hasFunctionName(calleeName)) isDirect = true;
    }

    // ---- 阶段3：内置构造器 正常(值)/错误(值)/某些(值)（Task 3.5）----
    // 这些函数已注册在 functions_（纯名 key），但返回类型含占位符"自动"；
    // 此处按"参数类型 + 返回上下文"推导实际 结果<T,E>/可选<T> 类型。
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        const std::string builtinName =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        auto bit = functions_.find(builtinName);
        if (bit != functions_.end() &&
            (builtinName == "正常" || builtinName == "错误" || builtinName == "某些")) {
            // 参数类型检查：正常/错误/某些 期望 1 个实参；例外——正常() 无参数
            //   用于 结果<空类型,E>（空类型正常值，容器库 追加/删除 等返回
            //   结果<空类型,整32> 的 返回 正常()，Task 6.1）。
            if (node->arguments.size() != 1 &&
                !(builtinName == "正常" && node->arguments.empty())) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "内置构造器 '" + builtinName + "' 期望 1 个实参");
            }
            std::string argType = "未知";
            for (auto& arg : node->arguments) {
                argType = checkExpr(arg.get());
            }
            // 正常() 无参数：正常值类型 = 返回上下文 T（结果<空类型,E> -> 空类型）
            if (builtinName == "正常" && node->arguments.empty()) {
                argType = "空类型";
            }
            // 构造器返回类型推导：
            //   正常(v) -> 结果<typeof(v), E>（E 由返回上下文/默认整32 决定）
            //   错误(v) -> 结果<T, typeof(v)>（T 由返回上下文/默认整32 决定）
            //   某些(v) -> 可选<typeof(v)>
            // 返回上下文推断（Task 3.5 E2E 24 修复）：构造器用于 返回 语句时，
            //   从当前函数返回类型 结果<T,E> 取缺失的 T/E（如 打开配置 返回
            //   结果<字符串,整32>，`返回 错误(5)` 的 T 推断为 字符串）。
            std::string ctxT = "";
            std::string ctxE = "";
            if (!currentReturnType_.empty() && isResultType(currentReturnType_)) {
                const std::vector<std::string> args = resultTypeArgs(currentReturnType_);
                if (args.size() == 2) {
                    ctxT = args[0];
                    ctxE = args[1];
                }
            } else if (!currentReturnType_.empty() && isOptionalType(currentReturnType_)) {
                ctxT = optionalTypeArg(currentReturnType_);
            }
            if (builtinName == "正常") {
                const std::string e = (ctxE.empty() ? "整32" : ctxE);
                lastType_ = "结果<" + (argType == "未知" ? "整32" : argType) + "," + e + ">";
            } else if (builtinName == "错误") {
                const std::string t = (ctxT.empty() ? "整32" : ctxT);
                lastType_ = "结果<" + t + "," + (argType == "未知" ? "整32" : argType) + ">";
            } else {
                lastType_ = "可选<" + (argType == "未知" ? "整32" : argType) + ">";
            }
            // 写回推导类型（Task 3.5 E2E 24 修复）：IR 层按 resolvedType 降级为
            //   合成结构体构造（分配槽 + 写 是否正常/是否某些 + 值/错误值）
            node->resolvedType = lastType_;
            return;
        }
    }
    // ---- 阶段3：构造函数调用 类名(实参)（Task 3.1，规格书06-三）----
    // 语法：点 p = 点(1, 2)——callee 为类类型名时视为构造调用。
    // 构造返回对象（结果类型 = 类名）；校验参数个数与类型（查构造方法）。
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化构造 盒子<整32>(42)——callee
    //   为 名<实参>（IdentifierExpr 名字含 <），先触发单态化（instantiateGeneric）
    //   生成实例化类符号（盒子$整32），再按普通类构造处理。
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        std::string className =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        const std::size_t genLt = className.find('<');
        const std::size_t genGt = className.rfind('>');
        if (genLt != std::string::npos && genGt != std::string::npos &&
            genGt > genLt) {
            const std::string head = className.substr(0, genLt);
            if (findGeneric(head) != nullptr) {
                const std::string inner =
                    className.substr(genLt + 1, genGt - genLt - 1);
                std::vector<std::string> args;
                std::size_t pos = 0;
                while (pos <= inner.size()) {
                    const std::size_t comma = inner.find(',', pos);
                    if (comma == std::string::npos) {
                        args.push_back(inner.substr(pos));
                        break;
                    }
                    args.push_back(inner.substr(pos, comma - pos));
                    pos = comma + 1;
                }
                for (auto& a : args) {
                    const std::size_t b = a.find_first_not_of(" \t");
                    const std::size_t e = a.find_last_not_of(" \t");
                    if (b != std::string::npos && e != std::string::npos) {
                        a = a.substr(b, e - b + 1);
                    }
                    // Task 6.1（嵌套泛型 链表$整32 内 节点<T>() 构造）：类型实参
                    //   T 替换为当前泛型上下文实参（整32）——否则 节点$T 实例化失败。
                    auto pit = genericTypeParams_.find(a);
                    if (pit != genericTypeParams_.end()) a = pit->second;
                }
                const std::string instName =
                    instantiateGeneric(head, args, node->location);
                if (!instName.empty()) className = instName;
            }
        }
        const ClassInfo* ctorCls = findClass(className);
        if (ctorCls != nullptr) {
            // 查找构造函数（函数名 == 类名）
            const ClassMemberInfo* ctor = nullptr;
            auto mit = ctorCls->methods.find(className);
            if (mit != ctorCls->methods.end() && mit->second.isConstructor) {
                ctor = &mit->second;
            }
            if (ctor != nullptr) {
                std::vector<std::string> argTypes;
                for (auto& arg : node->arguments) {
                    argTypes.push_back(checkExpr(arg.get()));
                }
                if (argTypes.size() != ctor->paramTypes.size()) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "构造函数 '" + className + "' 期望 " +
                                            std::to_string(ctor->paramTypes.size()) +
                                            " 个实参，实际提供 " +
                                            std::to_string(argTypes.size()) + " 个");
                } else {
                    for (std::size_t i = 0; i < argTypes.size(); ++i) {
                        if (!canConvertType(argTypes[i], ctor->paramTypes[i])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "构造函数 '" + className + "' 第 " + std::to_string(i + 1) +
                                    " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    ctor->paramTypes[i] + "'");
                        }
                    }
                }
                lastType_ = className;  // 构造返回对象
                return;
            }
            // 无构造函数：允许默认构造（返回类类型）
            lastType_ = className;
            return;
        }
    }
    // ---- 阶段3：成员方法调用 对象.方法(实参) / 类名.静态方法(实参)（Task 3.1/3.9）----
    if (node->callee->getType() == NodeType::MemberExpr) {
        MemberExpr* mem = static_cast<MemberExpr*>(node->callee.get());
        const std::string objType = checkExpr(mem->object.get());
        const std::string methodName = mem->memberName;
        std::string ownerClass;
        // 对象为类实例 或 类名.静态方法
        // 集成修复（自身/父类）：自身 类型为 类名*（this 指针），父类 类型为 父类名*，
        //   方法调用须剥指针取类类型（与 visitMemberExpr 的自身.成员 处理一致）；
        //   -> 访问 自身->方法() 同样剥指针。
        std::string objTypeForClass = objType;
        if (mem->object->getType() == NodeType::SelfExpr ||
            mem->object->getType() == NodeType::SuperExpr) {
            if (types::isPointer(objTypeForClass)) {
                objTypeForClass = types::pointeeOf(objTypeForClass);
            }
        }
        const std::string clsName = mem->isArrow
                                        ? canonicalType(types::pointeeOf(objTypeForClass))
                                        : canonicalType(objTypeForClass);
        const ClassMemberInfo* method = lookupClassMember(clsName, methodName, ownerClass);
        if (method != nullptr && !method->isStatic) {
            // 实例方法调用：校验参数个数与类型
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
            }
            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            } else {
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    if (!canConvertType(argTypes[i], method->paramTypes[i])) {
                        diagnostics_.report(
                            DiagnosticLevel::Error, node->arguments[i]->location,
                            "方法 '" + methodName + "' 第 " + std::to_string(i + 1) +
                                " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                method->paramTypes[i] + "'");
                    }
                }
            }
            // 访问控制检查（Task 3.4）
            const std::string contextClass = contextClassStack_.empty()
                                                 ? ""
                                                 : contextClassStack_.back();
            checkAccess(*findClass(ownerClass), *method, contextClass, node->location,
                        "方法");
            lastType_ = method->type;
            return;
        }
        if (method != nullptr && method->isStatic) {
            // 静态方法调用（类名.静态方法(...)）
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
            }
            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "静态方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            }
            lastType_ = method->type;
            return;
        }
        // 非类成员：继续走通用路径（结构体字段函数指针等）
    }

    // ---- 直接函数名调用：函数名(实参) ----
    if (isDirect) {
        // 变参内置函数（打印/打印行/格式化 Task 2.5/2.9）：纯名 key 直接查，
        // 参数个数不限，逐个检查类型（字符串/字符*/整型/浮点/布尔/字符/枚举均允许）
        // 方案C审查（2026-08-14）：okNum 补 字符——旧 打印行整数('A')（字符→整64
        //   隐式转换）替换为 打印('A') 后，IR 层 字符(i32) Cast i64 走 __cn_print_int
        //   输出ASCII码（65），行为等价；缺此检查会误拒 打印(字符变量)/打印('A')
        auto builtinIt = functions_.find(calleeName);
        if (builtinIt != functions_.end() && builtinIt->second.variadic) {
            const FunctionInfo& info = builtinIt->second;
            for (auto& arg : node->arguments) {
                std::string argType = checkExpr(arg.get());
                const bool okStr = (argType == "字符串" || argType == "字符*" ||
                                    argType == "字符串*");
                const bool okNum = (isNumeric(argType) || argType == "布尔" ||
                                    argType == "字符" || isEnumType(argType));
                if (!okStr && !okNum) {
                    diagnostics_.report(DiagnosticLevel::Error, arg->location,
                                        "打印行 参数类型不支持：'" + argType + "'");
                }
            }
            lastType_ = info.returnType;
            return;
        }
        // 非变参直接调用：重载决议（先检查实参类型）
        std::vector<std::string> argTypes;
        argTypes.reserve(node->arguments.size());
        for (auto& arg : node->arguments) {
            argTypes.push_back(checkExpr(arg.get()));
        }
        std::string sigKey = resolveOverload(calleeName, argTypes, node->location);
        if (sigKey.empty()) {
            // 决议失败（参数个数/类型不匹配或歧义）：恢复兼容——若纯名存在（内置
            // 单版本函数），按旧逻辑检查，避免错误级联导致 IR 层找不到符号
            auto fallback = functions_.find(calleeName);
            if (fallback != functions_.end()) {
                lastType_ = fallback->second.returnType;
            } else {
                lastType_ = "未知";
            }
            return;
        }
        node->resolvedSignature = sigKey;
        auto it = functions_.find(sigKey);
        const FunctionInfo& info = it->second;
        // 参数类型检查（决议已保证可转换；此处再逐个报告具体错误位置）
        for (std::size_t i = 0; i < node->arguments.size(); i++) {
            const std::string& paramType = info.paramTypes[i];
            if (!canConvertType(argTypes[i], paramType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                    "函数 '" + calleeName + "' 第 " + std::to_string(i + 1) +
                                    " 个参数无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    paramType + "'");
            }
        }
        lastType_ = info.returnType;
        return;
    }

    // ---- 函数指针间接调用：回调(10, 20) ----
    std::string calleeType = checkExpr(node->callee.get());
    if (calleeType == "未知") {
        for (auto& arg : node->arguments) checkExpr(arg.get());
        lastType_ = "未知";
        return;
    }
    if (isFuncPtrType(calleeType)) {
        std::string retType = funcPtrReturnOf(calleeType);
        std::vector<std::string> paramTypes = funcPtrParamsOf(calleeType);
        // 参数数量检查
        if (node->arguments.size() != paramTypes.size()) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数指针调用期望 " + std::to_string(paramTypes.size()) +
                                " 个参数，实际提供 " +
                                std::to_string(node->arguments.size()) + " 个");
            lastType_ = retType;
            return;
        }
        // 参数类型检查
        for (std::size_t i = 0; i < node->arguments.size(); i++) {
            std::string argType = checkExpr(node->arguments[i].get());
            const std::string& paramType = paramTypes[i];
            if (!canConvertType(argType, paramType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                    "函数指针第 " + std::to_string(i + 1) +
                                    " 个参数无法将 '" + argType + "' 隐式转换为 '" +
                                    paramType + "'");
            }
        }
        lastType_ = retType;
        return;
    }

    // 其他被调者（成员函数等）：后续Task实现，跳过
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "无法调用非函数类型 '" + calleeType + "'");
    lastType_ = "未知";
}

// 成员访问：结构体/联合体字段访问（. 与 ->，Task 2.7）+ 类成员（Task 3.1）+ 结果/可选（Task 3.5）
// .  ：对象须为结构体/联合体/类值，memberName 为其字段/方法；
// -> ：对象须为指向结构体/联合体/类的指针，解引用后访问成员
void SemanticAnalyzer::visitMemberExpr(MemberExpr* node) {
    const std::string memberName = node->memberName;
    const std::string objectVar = objectVarName(node->object.get());
    std::string objectType = checkExpr(node->object.get());
    // 结果/可选成员检查（Task 3.5 规则2/3）：.正常/.有值/.值/.错误
    if (isResultType(objectType) || isOptionalType(objectType)) {
        // 结果<T,E> / 可选<T> 经降级为合成结构体，其成员 .正常/.有值/.值/.错误
        // 在此处做强制检查规则分析；成员类型按降级结构体字段推导。
        checkResultMember(objectType, memberName, node->location, objectVar);
        // 推导成员类型：结果.正常 -> 布尔；可选.有值 -> 布尔；结果.值 -> T；可选.值 -> T；
        // 结果.错误 -> E
        if (isResultType(objectType)) {
            const std::vector<std::string> args = resultTypeArgs(objectType);
            if (memberName == "正常") {
                lastType_ = "布尔";
                return;
            }
            if (memberName == "值" && args.size() == 2) {
                lastType_ = canonicalType(args[0]);
                return;
            }
            if (memberName == "错误" && args.size() == 2) {
                lastType_ = canonicalType(args[1]);
                return;
            }
        }
        if (isOptionalType(objectType)) {
            if (memberName == "有值") {
                lastType_ = "布尔";
                return;
            }
            if (memberName == "值") {
                lastType_ = canonicalType(optionalTypeArg(objectType));
                return;
            }
        }
        // 其他成员：走降级结构体字段查找（防御）
        const StructDecl* lowered = findStruct(canonicalType(objectType));
        if (lowered != nullptr) {
            for (const auto& f : lowered->fields) {
                if (f.name == memberName) {
                    lastType_ = canonicalType(f.type);
                    return;
                }
            }
        }
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结果/可选 类型没有成员 '" + memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 枚举值引用：枚举名.成员（如 颜色.红，Task 2.7）
    // object 为标识符且其类型是枚举类型名 → 求值为枚举成员整数值
    if (!node->isArrow && node->object->getType() == NodeType::IdentifierExpr) {
        const std::string enumName = objectType;
        std::int64_t enumValue = 0;
        if (isEnumType(enumName) && enumValueOf(enumName, memberName, enumValue)) {
            lastType_ = enumName;  // 枚举值类型为枚举类型名（可与整型互转）
            return;
        }
        if (isEnumType(enumName)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "枚举 '" + enumName + "' 没有成员 '" + memberName + "'");
            lastType_ = "未知";
            return;
        }
    }
    std::string structType;  // 承载字段的结构体类型名（.为对象类型，->为指针所指）
    // 自身（this）指针：自身.成员 应剥指针取类类型（Task 3.1，规格书06-七）
    if (node->object->getType() == NodeType::SelfExpr) {
        structType = canonicalType(types::isPointer(objectType)
                                       ? types::pointeeOf(objectType)
                                       : objectType);
    } else if (node->isArrow) {
        // -> 访问：object 须为指针
        if (types::isPointer(objectType)) {
            structType = canonicalType(types::pointeeOf(objectType));
        } else {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "-> 成员访问要求左侧为指针，实际为 '" + objectType + "'");
            lastType_ = "未知";
            return;
        }
    } else {
        structType = canonicalType(objectType);
    }
    // 类成员访问（Task 3.1）：对象为类类型 或 类名.静态成员（标识符且是类类型名）
    const ClassInfo* cls = findClass(structType);
    if (cls != nullptr) {
        std::string ownerClass;
        const ClassMemberInfo* member = lookupClassMember(structType, memberName, ownerClass);
        if (member == nullptr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "类 '" + structType + "' 没有成员 '" + memberName + "'");
            lastType_ = "未知";
            return;
        }
        // 静态成员访问检查（Task 3.9）：类名.静态成员 允许；实例.静态成员 也允许；
        //   非静态成员经 类名. 访问 -> 错误（无实例）
        // 判断"类名.成员"：标识符本身是已注册类名（非类类型变量！变量 a 类型为
        //   账户 时 a.余额 是实例访问，不应误判为 类名.静态访问）
        bool objectIsTypeName = false;
        if (node->object->getType() == NodeType::IdentifierExpr) {
            const std::string& objName =
                static_cast<IdentifierExpr*>(node->object.get())->name;
            objectIsTypeName = isClassType(objName);
        }
        if (objectIsTypeName && !member->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "静态访问 '类名." + memberName +
                                    "' 要求成员为静态（非静态成员须经实例访问）");
            lastType_ = "未知";
            return;
        }
        // 静态成员引用：直接给类型（供 IR 层取静态字段/静态方法地址）
        if (member->isStatic) {
            lastType_ = member->type;
            return;
        }
        // 访问控制检查（Task 3.4）：非类上下文访问 私有/保护 成员 -> 错误
        const std::string contextClass = contextClassStack_.empty()
                                             ? ""
                                             : contextClassStack_.back();
        // 实例成员访问控制（自身.私有字段 在子类访问父类私有 -> 报错）
        const ClassInfo* ownerInfo = findClass(ownerClass);
        if (ownerInfo != nullptr) {
            checkAccess(*ownerInfo, *member, contextClass, node->location,
                        member->isConstructor || member->isDestructor ? "方法" : "成员");
        }
        // 方法引用：类型为 方法签名（供 对象.方法() 调用检查；此处给返回类型）
        if (!member->paramTypes.empty() || member->isConstructor ||
            member->isDestructor) {
            // 方法作值（函数指针类型）
            std::string fp = "函数指针<" + member->type + ">(";
            for (std::size_t i = 0; i < member->paramTypes.size(); ++i) {
                if (i > 0) fp += ",";
                fp += member->paramTypes[i];
            }
            fp += ")";
            lastType_ = fp;
            return;
        }
        lastType_ = member->type;
        return;
    }
    // 结构体/联合体字段访问（Task 2.7）
    const StructDecl* decl = findStruct(structType);
    if (decl == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类型 '" + structType + "' 不是结构体/联合体/类类型，无法访问成员 '" +
                            memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 字段存在性检查
    int offset = fieldOffsetOf(decl, memberName);
    if (offset < 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结构体 '" + decl->name + "' 没有成员 '" + memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 字段类型（从声明中查找）
    for (const auto& f : decl->fields) {
        if (f.name == memberName) {
            lastType_ = canonicalType(f.type);
            return;
        }
    }
    lastType_ = "未知";
}

// 空指针字面量：无（Task 2.4/3.5）
// 双义（规格书07-三）：指针上下文为空指针常量（空类型*）；
//   可选<T> 上下文为空可选值（无）——由赋值/返回的目标类型在 canConvertType
//   中处理（空类型* 可转换为 可选<T>：视为空可选构造）。
// 本节点类型推断保持 空类型*（与既有指针语义一致），可选赋值由 canConvertType 放行。
void SemanticAnalyzer::visitNullLiteral(NullLiteral* node) {
    (void)node;
    lastType_ = "空类型*";
}

// 下标访问：对象须为数组或指针，下标须为整型；结果类型为元素类型
// 数组越界检查（错误码2）在IR生成阶段插桩（运行期检查）
void SemanticAnalyzer::visitIndexExpr(IndexExpr* node) {
    std::string objectType = checkExpr(node->object.get());
    std::string indexType = checkExpr(node->index.get());
    if (objectType == "未知") {
        lastType_ = "未知";
        return;
    }
    // 数组退化：数组名作下标对象（数据[i]）按元素类型处理
    if (isArrayType(objectType)) {
        // 数组对象：元素类型即结果
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "数组下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = types::arrayElemOf(objectType);
        return;
    }
    if (isPointerType(objectType)) {
        // 指针对象（p[i] 等价 *(p+i)）：结果类型为所指元素类型
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "数组下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = types::pointeeOf(objectType);
        return;
    }
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "下标访问要求数组或指针对象，实际为 '" + objectType + "'");
    lastType_ = "未知";
}

// 初始化列表：仅作为数组声明的初始值（元素逐个检查在visitVarDecl中完成）。
// 作为表达式时报告错误（初始化列表不是值表达式）
void SemanticAnalyzer::visitInitListExpr(InitListExpr* node) {
    (void)node;
    // 各元素在 visitVarDecl 中结合数组元素类型逐个检查；
    // 此处作为独立表达式（非声明上下文）报告错误
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "初始化列表只能用于数组/聚合声明初始化");
    lastType_ = "未知";
}

// 结构体/联合体初始化：类型名{ 字段 = 值, ... }（Task 2.7）
// 检查：类型名须为已声明的结构体/联合体；字段名存在；字段值类型可隐式转换
void SemanticAnalyzer::visitStructInitExpr(StructInitExpr* node) {
    const std::string structType = canonicalType(node->typeName);
    const StructDecl* decl = findStruct(structType);
    if (decl == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结构体初始化引用了未声明的类型 '" + structType + "'");
        lastType_ = "未知";
        return;
    }
    // 字段存在性 + 类型检查
    std::unordered_set<std::string> seenFields;
    for (auto& fieldPair : node->fields) {
        const std::string& fieldName = fieldPair.first;
        // 字段存在性
        int offset = fieldOffsetOf(decl, fieldName);
        if (offset < 0) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "结构体 '" + decl->name + "' 没有字段 '" + fieldName + "'");
            lastType_ = "未知";
            continue;
        }
        // 重复字段
        if (!seenFields.insert(fieldName).second) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "结构体初始化字段 '" + fieldName + "' 重复");
        }
        // 字段类型
        std::string fieldType;
        for (const auto& f : decl->fields) {
            if (f.name == fieldName) {
                fieldType = canonicalType(f.type);
                break;
            }
        }
        // 数组字段初始化列表（Task 完善A）：字段值为 InitListExpr（如 分数 = { 80, 90, 70 }），
        //   字段类型须为数组，逐元素检查类型（元素类型与元素值类型可转换）
        if (fieldPair.second->getType() == NodeType::InitListExpr) {
            if (types::isArray(fieldType)) {
                InitListExpr* list = static_cast<InitListExpr*>(fieldPair.second.get());
                const std::string elemType = canonicalType(types::arrayElemOf(fieldType));
                for (auto& elem : list->elements) {
                    std::string elemValueType = checkExpr(elem.get());
                    if (!elemValueType.empty() && elemValueType != "未知" &&
                        !canConvertType(elemValueType, elemType)) {
                        diagnostics_.report(DiagnosticLevel::Error, elem->location,
                                            "结构体数组字段 '" + fieldName + "' 元素无法将 '" +
                                            elemValueType + "' 隐式转换为 '" + elemType + "'");
                    }
                }
            }
            continue;
        }
        // 值类型检查（嵌套结构体初始化递归检查：checkExpr 返回内层类型）
        std::string valueType = checkExpr(fieldPair.second.get());
        if (!fieldType.empty() && valueType != "未知" && !canConvertType(valueType, fieldType)) {
            diagnostics_.report(DiagnosticLevel::Error, fieldPair.second->location,
                                "结构体字段 '" + fieldName + "' 无法将 '" + valueType +
                                "' 隐式转换为 '" + fieldType + "'");
        }
    }
    lastType_ = structType;
}

// ==================== 阶段3：声明节点/表达式（Task 3.1~3.9） ====================
// 类/接口/泛型 声明由 visitProgram 统一驱动（registerClassAndInterfaces /
// registerGenerics / lowerResultOptionalTypes）；此处提供防御性空实现，
// 防止 AST 直接访问（AstVisitor 分发）时无方法可调。

// 类声明：由 registerClassAndInterfaces 处理（防御性空实现）
void SemanticAnalyzer::visitClassDecl(ClassDecl* node) {
    (void)node;
}

// 类成员：由 resolveClass/collectClassMembers 处理（防御性空实现）
void SemanticAnalyzer::visitClassMember(ClassMember* node) {
    (void)node;
}

// 接口声明：由 registerClassAndInterfaces 处理（防御性空实现）
void SemanticAnalyzer::visitInterfaceDecl(InterfaceDecl* node) {
    (void)node;
}

// 导入声明：模块系统（Task 3.6）
// 收集已导入模块名（importPath 首段）：导入 数学.平方根 / 从 数学 导入 正弦
// 供 visitCallExpr 识别"模块.函数"限定调用（重写为直接调用）。
// 模块加载/合并由 driver（runModulePipeline）在语义分析前完成；
// 此处仅记录模块名集合，不校验符号存在性（跨模块可见性在合并阶段已过滤私有）。
void SemanticAnalyzer::visitImportDecl(ImportDecl* node) {
    std::string moduleName = node->importPath;
    const std::size_t dot = moduleName.find('.');
    if (dot != std::string::npos) moduleName = moduleName.substr(0, dot);
    if (!moduleName.empty()) importedModules_.insert(moduleName);
}

// 泛型声明：由 registerGenerics/instantiateGeneric 处理（防御性空实现）
void SemanticAnalyzer::visitGenericDecl(GenericDecl* node) {
    (void)node;
}

// 自身表达式：自身（this 指针）——类型为 当前上下文类 指针
// （Task 3.1，规格书06-七；仅类方法体内合法）
void SemanticAnalyzer::visitSelfExpr(SelfExpr* node) {
    const ClassInfo* cls = currentContextClass();
    if (cls == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'自身' 只能出现在类方法体内");
        lastType_ = "未知";
        return;
    }
    lastType_ = cls->name + "*";
}

// 父类表达式：父类（SuperExpr）——解析为父类类型（供 父类.方法() 限定调用）
// （Task 3.1，规格书06-七；仅类方法体内合法）
void SemanticAnalyzer::visitSuperExpr(SuperExpr* node) {
    const ClassInfo* cls = currentContextClass();
    if (cls == nullptr || cls->baseName.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'父类' 只能出现在有父类的类方法体内");
        lastType_ = "未知";
        return;
    }
    lastType_ = cls->baseName;  // 父类类型（供 父类.方法() 查找父类成员）
}

// 类型节点：语义阶段不做处理
void SemanticAnalyzer::visitType(Type* node) {
    (void)node;
}

// 强制类型转换：类型名(表达式)（规格书04-一E，Task 2.10）
// 语义同 static_cast：宽化/窄化/浮整/指针↔整数 均显式触发（不检查隐式转换）。
// 检查规则：
//   1. 源类型/目标类型均须已知（未知源类型报错——IR 层无法生成转换）
//   2. 任意两个数值类型（整型族/浮点族）之间可转换
//   3. 指针 ↔ 整数：显式转换合法（整数 -> 指针 / 指针 -> 整数）
//   4. 指针 -> 指针：显式转换合法（位重解释）
// lambda 捕获分析（Task 2.10）：扫描函数体中的标识符引用，
//   收集不在参数表中的外层变量。递归遍历表达式/语句中的 IdentifierExpr；
//   显式捕获（[x]）不在此处理（visitLambdaExpr 单独校验）。
void SemanticAnalyzer::collectLambdaCaptures(
    LambdaExpr* node, const std::unordered_set<std::string>& paramNames) {
    std::vector<Expr*> exprs;
    std::vector<Stmt*> stmts;
    for (auto& stmt : node->body->statements) stmts.push_back(stmt.get());
    // 收集全部表达式（语句 + 嵌套表达式）
    std::vector<Expr*> allExprs;
    while (!stmts.empty()) {
        Stmt* s = stmts.back();
        stmts.pop_back();
        switch (s->getType()) {
            case NodeType::ExprStmt:
                exprs.push_back(static_cast<ExprStmt*>(s)->expr.get());
                break;
            case NodeType::VarDecl:
                if (static_cast<VarDecl*>(s)->initializer != nullptr)
                    exprs.push_back(static_cast<VarDecl*>(s)->initializer.get());
                break;
            case NodeType::ReturnStmt:
                if (static_cast<ReturnStmt*>(s)->value != nullptr)
                    exprs.push_back(static_cast<ReturnStmt*>(s)->value.get());
                break;
            case NodeType::IfStmt: {
                IfStmt* ifs = static_cast<IfStmt*>(s);
                exprs.push_back(ifs->condition.get());
                for (auto& sub : ifs->thenBranch->statements) stmts.push_back(sub.get());
                if (ifs->elseBranch) stmts.push_back(ifs->elseBranch.get());
                break;
            }
            case NodeType::WhileStmt: {
                WhileStmt* ws = static_cast<WhileStmt*>(s);
                exprs.push_back(ws->condition.get());
                for (auto& sub : ws->body->statements) stmts.push_back(sub.get());
                break;
            }
            case NodeType::ForStmt: {
                ForStmt* fs = static_cast<ForStmt*>(s);
                if (fs->condition) exprs.push_back(fs->condition.get());
                if (fs->update) exprs.push_back(fs->update.get());
                if (fs->init) stmts.push_back(fs->init.get());
                for (auto& sub : fs->body->statements) stmts.push_back(sub.get());
                break;
            }
            case NodeType::BlockStmt:
                for (auto& sub : static_cast<BlockStmt*>(s)->statements)
                    stmts.push_back(sub.get());
                break;
            default:
                break;
        }
    }
    // 递归展开表达式树，收集 IdentifierExpr
    while (!exprs.empty()) {
        Expr* e = exprs.back();
        exprs.pop_back();
        if (e == nullptr) continue;
        switch (e->getType()) {
            case NodeType::IdentifierExpr: {
                const std::string& name = static_cast<IdentifierExpr*>(e)->name;
                if (paramNames.count(name) == 0) {
                    std::string t;
                    // 必须是外层已声明变量（排除函数名/类型名；函数名捕获无意义）
                    if (lookupVar(name, t) && !t.empty() && t != "未知") {
                        // 去重加入
                        bool dup = false;
                        for (const auto& c : node->explicitCaptures)
                            if (c == name) { dup = true; break; }
                        if (!dup) node->explicitCaptures.push_back(name);
                    }
                }
                break;
            }
            case NodeType::BinaryExpr: {
                BinaryExpr* b = static_cast<BinaryExpr*>(e);
                exprs.push_back(b->left.get());
                exprs.push_back(b->right.get());
                break;
            }
            case NodeType::UnaryExpr:
                exprs.push_back(static_cast<UnaryExpr*>(e)->operand.get());
                break;
            case NodeType::AssignmentExpr: {
                AssignmentExpr* a = static_cast<AssignmentExpr*>(e);
                exprs.push_back(a->target.get());
                exprs.push_back(a->value.get());
                break;
            }
            case NodeType::CallExpr: {
                CallExpr* c = static_cast<CallExpr*>(e);
                exprs.push_back(c->callee.get());
                for (auto& arg : c->arguments) exprs.push_back(arg.get());
                break;
            }
            case NodeType::MemberExpr:
                exprs.push_back(static_cast<MemberExpr*>(e)->object.get());
                break;
            case NodeType::IndexExpr: {
                IndexExpr* ix = static_cast<IndexExpr*>(e);
                exprs.push_back(ix->object.get());
                exprs.push_back(ix->index.get());
                break;
            }
            case NodeType::TernaryExpr: {
                TernaryExpr* t = static_cast<TernaryExpr*>(e);
                exprs.push_back(t->condition.get());
                exprs.push_back(t->trueValue.get());
                exprs.push_back(t->falseValue.get());
                break;
            }
            case NodeType::CastExpr:
                exprs.push_back(static_cast<CastExpr*>(e)->operand.get());
                break;
            case NodeType::LambdaExpr: {
                // 缺陷修复（嵌套 lambda 捕获穿透）：内层 lambda 引用的外层变量
                //   必须合并到本层捕获集——否则内层 lambda 体生成时，其捕获变量
                //   穿透本层匿名函数边界，直接引用更外层函数的栈槽（跨函数槽
                //   越界，产生 [rbp0] 非法汇编，实测嵌套 lambda 汇编失败）。
                //   递归收集内层捕获集，再并入本层捕获（去重）。
                LambdaExpr* inner = static_cast<LambdaExpr*>(e);
                std::unordered_set<std::string> innerParams = paramNames;
                for (auto& p : inner->params) innerParams.insert(p->name);
                collectLambdaCaptures(inner, innerParams);
                for (const auto& c : inner->explicitCaptures) {
                    bool dup = false;
                    for (const auto& ec : node->explicitCaptures)
                        if (ec == c) { dup = true; break; }
                    if (!dup) node->explicitCaptures.push_back(c);
                }
                break;
            }
            default:
                break;
        }
    }
}

//   5. 其余组合（如 字符串 -> 整32、结构体 -> 整32）报错
void SemanticAnalyzer::visitCastExpr(CastExpr* node) {
    const std::string target = canonicalType(node->targetType);
    const std::string src = checkExpr(node->operand.get());
    if (src == "未知") {
        lastType_ = "未知";
        return;  // 源类型未知：操作数错误已由 checkExpr 报告，避免连锁误报
    }
    // 布尔（i1）本质为 0/1 整数，允许显式转整数（规格书04-一E 数值族内部转换；
    // emitCast 已有 i1->i32/u32/i64/u64 分支）。原实现漏判"布尔"导致
    // 整32(真)/整64(假) 被误拒。
    const bool srcNumeric = isNumeric(src) || isEnumType(src) ||
                            src == "字符" || src == "布尔";
    const bool dstNumeric = isNumeric(target) || target == "字符";
    const bool srcPtr = types::isPointer(src);
    const bool dstPtr = types::isPointer(target);
    const bool ok =
        (srcNumeric && dstNumeric) ||        // 数值族内部（整<->浮、整宽窄化、枚举/字符）
        (srcPtr && dstPtr) ||                // 指针 -> 指针（位重解释）
        (srcNumeric && dstPtr) ||            // 整数 -> 指针
        (srcPtr && dstNumeric);              // 指针 -> 整数
    if (!ok) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + src + "' 显式转换为 '" + target + "'");
        lastType_ = "未知";
        return;
    }
    lastType_ = target;
}

// lambda 表达式：检查捕获 + 返回类型推导（规格书04-一D，Task 2.10）
// 方案：lambda 降级为匿名函数 + 闭包捕获环境（IR 层生成）。本阶段只做：
//   1. 检查参数（复用函数体检查逻辑）
//   2. 检查函数体（捕获变量在闭包作用域中可见）
//   3. 推导返回类型（显式返回标注 或 函数体单一返回语句）
//   lastType_ 返回 lambda 类型描述符（函数指针<返回>(参数,...)），
//   供 `自动 加倍 = [...]...` 声明与 `加倍(...)` 调用检查使用。
void SemanticAnalyzer::visitLambdaExpr(LambdaExpr* node) {
    // Task 2.10 捕获分析：确定实际捕获变量集（回填 node->explicitCaptures）
    //   [] 不捕获；[=] 值捕获全部外层可见变量；[&] 引用捕获全部外层可见变量；
    //   [变量] 显式捕获（校验变量存在）。
    // 实现：扫描 lambda 函数体中的标识符引用，凡不在参数表中的外层变量即为捕获。
    //   （简化：扫描一次；嵌套 lambda 的捕获集合并到本层）
    if (node->captureKind == LambdaCaptureKind::None ||
        node->captureKind == LambdaCaptureKind::ByValue ||
        node->captureKind == LambdaCaptureKind::ByRef) {
        // [] 与 [=]/[&] 等价：扫描函数体引用，收集外层变量作捕获。
        //   （[] 不显式声明捕获，但体内引用外层变量时按隐式值捕获处理，
        //     与 Task 2.8 字符串指针共享语义一致；IR 层按此展开捕获实参）
        // [=]/[&]：收集函数体中引用的外层变量（标识符不在参数表、在 scopes_ 中）
        node->explicitCaptures.clear();
        std::unordered_set<std::string> paramNames;
        for (auto& p : node->params) paramNames.insert(p->name);
        // 在参数作用域压栈后扫描体（体检查时标识符已解析），
        // 这里预先收集：从 scopes_ 栈（不含 lambda 参数）查找可见变量
        // 简化实现：捕获分析依赖 visitIdentifierExpr 的变量解析——在 pushScope
        // 之前先把当前外层作用域全部变量视为候选，扫描体中出现的标识符。
        collectLambdaCaptures(node, paramNames);
    } else if (node->captureKind == LambdaCaptureKind::Explicit) {
        // [x, y]：校验捕获变量存在（外层作用域可查）
        for (const auto& cap : node->explicitCaptures) {
            std::string t;
            if (!lookupVar(cap, t)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "lambda 捕获的变量 '" + cap + "' 未声明");
            }
        }
    }
    // 推入 lambda 参数作用域（函数体内参数可见）
    pushScope();
    for (auto& param : node->params) {
        const std::string ptype = param->funcPtr.isFunctionPtr()
                                      ? param->funcPtr.toString()
                                      : canonicalType(param->typeName);
        if (!declareVar(param->name, ptype, param->location)) {
            diagnostics_.report(DiagnosticLevel::Error, param->location,
                                "lambda 参数 '" + param->name + "' 重复声明");
        }
    }
    // 检查函数体（返回语句由 checkFunctionBody 校验；lambda 的返回类型
    // 在 visitReturnStmt 里按 currentReturnType_ 校验——lambda 未标注时
    // 先置空、由 return 语句动态放宽，见 visitReturnStmt 特判）
    const std::string savedReturn = currentReturnType_;
    currentReturnType_ = node->returnType.empty() ? "" : canonicalType(node->returnType);
    if (node->body != nullptr) {
        // 显式返回标注：按普通函数检查（返回类型一致性由 visitReturnStmt 保证）
        if (!node->returnType.empty()) {
            checkBlock(node->body.get());
        } else {
            // 无返回标注：宽松检查——记录 return 表达式类型用于推导。
            // 直接复用 checkBlock 会因 currentReturnType_ 为空而漏检，
            // 故先按"任意类型"检查体，再单独推导返回类型。
            lambdaReturnCandidate_.clear();
            lambdaInferMode_ = true;
            for (auto& stmt : node->body->statements) {
                checkStmt(stmt.get());
            }
            lambdaInferMode_ = false;
            if (lambdaReturnCandidate_.size() == 1) {
                node->returnType = lambdaReturnCandidate_[0];
            } else if (lambdaReturnCandidate_.empty()) {
                node->returnType = "空类型";
            } else {
                // 多个返回类型不一致：取第一个（保守），避免类型检查二次报错
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "lambda 返回类型无法推导（多个返回语句类型不一致）");
                node->returnType = lambdaReturnCandidate_[0];
            }
        }
    }
    currentReturnType_ = savedReturn;
    popScope();
    // lambda 类型描述符：函数指针<返回>(参数类型,...)
    std::string params;
    for (std::size_t i = 0; i < node->params.size(); ++i) {
        if (i > 0) params += ", ";
        params += node->params[i]->funcPtr.isFunctionPtr()
                      ? node->params[i]->funcPtr.toString()
                      : canonicalType(node->params[i]->typeName);
    }
    lastType_ = "函数指针<" + (node->returnType.empty() ? "空类型" : node->returnType) +
                ">(" + params + ")";
}

// ==================== 分发辅助 ====================

// 检查表达式，返回推断类型（存到 lastType_）
std::string SemanticAnalyzer::checkExpr(Expr* node) {
    if (node == nullptr) return "未知";
    node->accept(*this);
    return lastType_;
}

// 检查代码块（含作用域进出）
void SemanticAnalyzer::checkBlock(BlockStmt* node) {
    if (node == nullptr) return;
    visitBlockStmt(node);
}

// 检查语句：分发到对应 visit 方法
void SemanticAnalyzer::checkStmt(Stmt* node) {
    if (node == nullptr) return;
    node->accept(*this);
}

} // namespace cn_compiler
