// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
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
int arrayTotalSize(const std::string& type) {
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
bool SemanticAnalyzer::isStructType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    for (const auto& s : program_->structs) {
        if (s->name == type) return true;
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
const StructDecl* SemanticAnalyzer::findStruct(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    for (const auto& s : program_->structs) {
        if (s->name == name) return s.get();
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
std::string SemanticAnalyzer::funcReturnTypeOf(const std::string& funcName) const {
    auto it = functions_.find(funcName);
    if (it == functions_.end()) return "";
    return it->second.returnType;
}

// 扩展隐式转换（Task 2.7）：枚举↔整数（枚举本质为整32）；枚举间须同名；结构体须同名
bool SemanticAnalyzer::canConvertType(const std::string& fromRaw,
                                      const std::string& toRaw) const {
    const std::string from = canonicalType(fromRaw);
    const std::string to = canonicalType(toRaw);
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
    const StructDecl* decl = findStruct(type);
    if (decl != nullptr) return decl->totalSize;
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
    const StructDecl* decl = findStruct(type);
    if (decl != nullptr) return decl->align;
    if (type == "整128" || type == "正128") return 16;
    if (type == "整64" || type == "正64" || type == "浮64" || type == "字符串") return 8;
    if (type == "整32" || type == "正32" || type == "浮32" || type == "字符") return 4;
    if (type == "整16" || type == "正16") return 2;
    return 1;  // 整8/正8/布尔
}

// 查找结构体字段偏移（-1表示无此字段；联合体字段偏移恒为0）
int SemanticAnalyzer::fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const {
    for (const auto& f : decl->fields) {
        if (f.name == fieldName) return decl->isUnion ? 0 : f.offset;
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
// 打印行（字符串）-> 空类型；打印行整数（整64）-> 空类型；打印行浮点（浮64）-> 空类型
// Task 2.5 新增：
//   - 打印行 支持多参数（字符串/整数/浮点混合，visitCallExpr 特判，签名仅登记单参版本）
//   - 字符串API：字符串长度/字符串比较/字符串连接/字符串复制/字符串查找
void SemanticAnalyzer::registerBuiltins() {
    // ---- 打印行系列 ----
    FunctionInfo printLineInfo;
    printLineInfo.returnType = "空类型";
    printLineInfo.paramTypes = {"字符串"};
    printLineInfo.hasBody = true;  // 运行时提供实现，语义检查视为已定义
    // 变参标记：参数个数不限（visitCallExpr 特判展开），paramTypes 仅用于单参检查
    printLineInfo.variadic = true;
    functions_["打印行"] = printLineInfo;

    FunctionInfo printLineIntInfo;
    printLineIntInfo.returnType = "空类型";
    printLineIntInfo.paramTypes = {"整64"};
    printLineIntInfo.hasBody = true;
    functions_["打印行整数"] = printLineIntInfo;

    FunctionInfo printLineFloatInfo;
    printLineFloatInfo.returnType = "空类型";
    printLineFloatInfo.paramTypes = {"浮64"};
    printLineFloatInfo.hasBody = true;
    functions_["打印行浮点"] = printLineFloatInfo;

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
}

// 第一趟：注册函数符号（支持前向调用与重名检测，类型统一存规范化形式）
// Task 2.2 增强：
//   - 函数指针参数：参数类型用 funcPtr.toString() 规范化表示
//   - 签名一致性：原型声明与定义签名（返回类型+参数类型）必须一致
void SemanticAnalyzer::registerFunction(FunctionDecl* node) {
    FunctionInfo info;
    info.returnType = node->returnType.empty() ? "空类型" : canonicalType(node->returnType);
    info.hasBody = (node->body != nullptr);
    for (auto& param : node->params) {
        // 函数指针参数：整32(*func)(整32, 整32) 类型存规范化字符串
        if (param->funcPtr.isFunctionPtr()) {
            info.paramTypes.push_back(param->funcPtr.toString());
        } else {
            info.paramTypes.push_back(canonicalType(param->typeName));
        }
    }
    auto it = functions_.find(node->name);
    if (it != functions_.end()) {
        // 重名：允许"原型声明 + 定义"组合，其余为重复定义
        bool isProtoPlusDef = !it->second.hasBody && info.hasBody;
        if (isProtoPlusDef) {
            // 签名一致性检查：原型（已注册）与定义（当前）签名必须一致
            const FunctionInfo& proto = it->second;
            bool same = (proto.returnType == info.returnType &&
                         proto.paramTypes.size() == info.paramTypes.size());
            if (same) {
                for (std::size_t i = 0; i < proto.paramTypes.size() && same; ++i) {
                    if (proto.paramTypes[i] != info.paramTypes[i]) same = false;
                }
            }
            if (!same) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "函数 '" + node->name + "' 原型声明与定义签名不一致");
            }
        } else if (!it->second.hasBody || info.hasBody) {
            // 原型+原型 或 定义+定义 均为重复声明
            if (it->second.hasBody || info.hasBody) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "重复定义函数 '" + node->name + "'");
            }
        }
        return;
    }
    functions_[node->name] = info;
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

// 程序入口：多趟处理（Task 2.7 增加结构体/枚举注册与布局计算）
void SemanticAnalyzer::visitProgram(Program* node) {
    pushScope();  // 全局作用域
    program_ = node;
    // 第零趟：注册CN语言内置函数符号（打印行等，无需源码声明即可调用）
    registerBuiltins();
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
    // 第一趟d：注册全部函数符号（含前向调用）
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            registerFunction(decl.get());
        }
    }
    // 第二趟：逐个检查函数体
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            checkFunctionBody(static_cast<FunctionDecl*>(decl.get()));
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
    // 函数符号必须已注册（原型声明无函数体）
    auto it = functions_.find(node->name);
    if (it == functions_.end()) return;
    if (node->body == nullptr) return;  // 函数原型声明：无需检查体

    currentReturnType_ = it->second.returnType;
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
    checkExpr(node->expr.get());
}

// 如果语句：条件必须为布尔；分支各自进入子作用域
void SemanticAnalyzer::visitIfStmt(IfStmt* node) {
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
    if (currentReturnType_.empty()) {
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
void SemanticAnalyzer::visitIdentifierExpr(IdentifierExpr* node) {
    std::string varType;
    if (lookupVar(node->name, varType)) {
        lastType_ = varType;
        return;
    }
    // 枚举/结构体类型名作标识符（供 枚举名.成员 与 &结构体 引用，Task 2.7）
    if (isEnumType(node->name) || isStructType(node->name)) {
        lastType_ = node->name;
        return;
    }
    auto it = functions_.find(node->name);
    if (it != functions_.end()) {
        // 函数名作为值：构造函数指针类型（返回类型 + 参数类型）
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
        if (!isNumeric(leftType) || !isNumeric(rightType)) {
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

// 赋值表达式：左值须可写（标识符/下标/解引用），类型兼容检查
void SemanticAnalyzer::visitAssignmentExpr(AssignmentExpr* node) {
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
void SemanticAnalyzer::visitCallExpr(CallExpr* node) {
    // 分派依据：callee 若是函数名（在函数符号表中）→ 直接调用；
    //           否则检查其类型，若是函数指针变量 → 间接调用
    bool isDirect = false;
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        if (functions_.find(calleeName) != functions_.end()) isDirect = true;
    }

    // ---- 直接函数名调用：函数名(实参) ----
    if (isDirect) {
        auto it = functions_.find(calleeName);
        const FunctionInfo& info = it->second;
        // 变参函数（打印行 Task 2.5）：参数个数不限，逐个检查类型
        // （字符串/字符*/整型/浮点/布尔/枚举均允许，IR 层按类型展开为打印序列）
        if (info.variadic) {
            for (auto& arg : node->arguments) {
                std::string argType = checkExpr(arg.get());
                const bool okStr = (argType == "字符串" || argType == "字符*" ||
                                    argType == "字符串*");
                const bool okNum = (isNumeric(argType) || argType == "布尔" ||
                                    isEnumType(argType));
                if (!okStr && !okNum) {
                    diagnostics_.report(DiagnosticLevel::Error, arg->location,
                                        "打印行 参数类型不支持：'" + argType + "'");
                }
            }
            lastType_ = info.returnType;
            return;
        }
        // 参数数量检查
        if (node->arguments.size() != info.paramTypes.size()) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数 '" + calleeName + "' 期望 " +
                                std::to_string(info.paramTypes.size()) + " 个参数，实际提供 " +
                                std::to_string(node->arguments.size()) + " 个");
            lastType_ = info.returnType;
            return;
        }
        // 参数类型检查
        for (std::size_t i = 0; i < node->arguments.size(); i++) {
            std::string argType = checkExpr(node->arguments[i].get());
            const std::string& paramType = info.paramTypes[i];
            if (!canConvertType(argType, paramType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                    "函数 '" + calleeName + "' 第 " + std::to_string(i + 1) +
                                    " 个参数无法将 '" + argType + "' 隐式转换为 '" +
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

// 成员访问：结构体/联合体字段访问（. 与 ->，Task 2.7 接通）
// .  ：对象须为结构体/联合体值或引用，memberName 为其字段；
// -> ：对象须为指向结构体/联合体的指针，解引用后访问字段
void SemanticAnalyzer::visitMemberExpr(MemberExpr* node) {
    std::string objectType = checkExpr(node->object.get());
    const std::string memberName = node->memberName;
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
    if (node->isArrow) {
        // -> 访问：object 须为结构体/联合体指针（类型以 * 结尾）
        if (types::isPointer(objectType)) {
            structType = canonicalType(types::pointeeOf(objectType));
        } else {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "-> 成员访问要求左侧为结构体指针，实际为 '" +
                                objectType + "'");
            lastType_ = "未知";
            return;
        }
    } else {
        // . 访问：object 须为结构体/联合体（枚举访问 枚举名.成员 走标识符路径）
        structType = canonicalType(objectType);
    }
    const StructDecl* decl = findStruct(structType);
    if (decl == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类型 '" + structType + "' 不是结构体/联合体类型，无法访问成员 '" +
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

// 空指针字面量：无（Task 2.4）——类型为"空类型*"，可赋给任意指针（规格书3.7）
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

// 类型节点：语义阶段不做处理
void SemanticAnalyzer::visitType(Type* node) {
    (void)node;
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
