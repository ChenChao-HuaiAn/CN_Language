// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <cstdio>
#include <string>
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

#pragma GCC diagnostic pop

} // namespace

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
    // A-2（crate 分桶）：变量类型按所属模块解析并改写（顶层声明用节点模块，
    //   局部变量用当前函数模块上下文；IR 层按改写后的类型查询语义表）
    if (!varType.empty() && !node->funcPtr.isFunctionPtr()) {
        const std::string mod = node->moduleName.empty() ? currentModuleName_
                                                         : node->moduleName;
        varType = resolveTypeName(varType, mod, node->location);
        if (!node->typeName.empty()) node->typeName = varType;
    }
    // H7 补完（2026-08-25）：类类型栈变量无初始化器裸声明（类名 变量）须可默认构造——
    //   类声明了构造但无 0 参构造（仅有带参构造）时无法默认构造，编译报错
    //   （与 C++ 语义一致；无构造类允许默认构造仅分配，与 类名() 语义一致）。
    //   IR 层裸声明按"无参构造调用/仅分配"生成 NewObject，无默认构造时静默
    //   分配未初始化对象是安全隐患，须在语义层拦截（H7 根治补完）。
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        !varType.empty() && isClassType(varType)) {
        const ClassInfo* ci = findClass(varType);
        if (ci != nullptr) {
            bool hasAnyCtor = false;
            bool hasDefaultCtor = false;
            for (const auto& mk : ci->methods) {
                if (mk.second.isConstructor && mk.second.hasBody &&
                    mk.second.ownerClass == varType) {
                    hasAnyCtor = true;
                    if (mk.second.paramTypes.empty()) {
                        hasDefaultCtor = true;
                        break;
                    }
                }
            }
            if (hasAnyCtor && !hasDefaultCtor) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "类类型变量 '" + node->name + "' 无默认构造（类 '" +
                                    varType + "' 仅声明带参构造），裸声明须可默认构造");
            }
        }
    }
    // P3-18（引用参数 A-1 扩展）：引用变量声明（变量 整32& r = x）——
    //   槽存被引用左值地址，读/写经 byRef 解引用。绑定目标须为左值：
    //   标识符 / 下标 / 解引用 / 成员 / 引用返回调用（P3-18 补完）。
    //   CallExpr 须为"引用返回调用"（下方 checkExpr 后按 lastExprIsRefReturn_ 校验）。
    bool refVarInitIsCall = false;
    if (!varType.empty() && types::isReference(varType)) {
        if (node->initializer == nullptr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "引用变量声明须绑定左值且须初始化（变量 整32& r = x）");
            return;
        }
        const NodeType it = node->initializer->getType();
        if (it != NodeType::IdentifierExpr && it != NodeType::IndexExpr &&
            it != NodeType::UnaryExpr && it != NodeType::MemberExpr &&
            it != NodeType::CallExpr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "引用变量声明须绑定左值且须初始化（变量 整32& r = x）");
            return;
        }
        refVarInitIsCall = (it == NodeType::CallExpr);
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
    // P3-18 补完：引用变量绑定"引用返回调用"须确认调用确为引用返回（否则报错）
    if (refVarInitIsCall && !lastExprIsRefReturn_) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "引用变量初始化须绑定左值（引用返回调用 或 标识符/下标/解引用/成员）");
        return;
    }
    if (varType.empty()) {
        // 无类型标注且无初始值：类型未知
        varType = "未知";
    }
    // 方案A 强制规则（2026-08-25）：类对象初始化拷贝（类名 乙 = 甲，甲 为类变量）——
    //   有析构类须有拷贝构造（函数 类名(类名& 其他) 深拷贝），否则浅拷贝裸指针
    //   字段析构双释放 0xC0000374（checkCopyRequiresCtor 仅在确认发生拷贝时拦截）。
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::IdentifierExpr &&
        !varType.empty() && varType != "未知" && isClassType(varType)) {
        std::string initVarType;
        if (lookupVar(static_cast<IdentifierExpr*>(node->initializer.get())->name,
                      initVarType) &&
            isClassType(types::canonical(initVarType))) {
            checkCopyRequiresCtor(varType, node->location);
        }
    }
    declareVar(node->name, varType, node->location);
}
void SemanticAnalyzer::visitImportDecl(ImportDecl* node) {
    if (node == nullptr || node->segments.empty()) return;
    // ---- 模块声明（模块 X）：仅建立模块树引用，不引入符号 ----
    if (node->isModuleDecl) return;
    // ---- 路径导入 / 重命名导入 / 通配符导入 ----
    // 取路径首段为模块名（跨包路径 包名::模块::符号 首段 = 包名；当前实现
    //   以首段为 crate 边界，多段路径按模块名 = 完整路径前缀解析）
    std::string moduleName = node->segments[0];
    UseImportInfo& use = useImports_[moduleName];
    if (node->wildcard) {
        // 导入 模块::*：通配符导入（模块全部公开符号）
        use.wildcard = true;
        importedModules_.insert(moduleName);
        return;
    }
    if (!node->names.empty()) {
        // 导入 路径::{项1 [作为 别名], ...}：花括号导入
        // A-5（花括号项别名跨模块同名）：记录每个导入项的来源模块**完整路径**
        //   （工具库::格式化）——纯名调用重写回原符号名后按完整路径过滤；
        //   首段（工具库）过滤在跨 crate 场景会漏掉 格式化 模块条目
        std::string braceFullPath;
        for (std::size_t si = 0; si < node->segments.size(); ++si) {
            if (si > 0) braceFullPath += "::";
            braceFullPath += node->segments[si];
        }
        for (const auto& item : node->names) {
            if (item.name.empty()) continue;
            use.symbols.insert(item.name);
            if (!item.alias.empty()) {
                use.aliases[item.alias] = item.name;
                itemAliasModules_[item.alias] = braceFullPath;
            } else {
                use.aliases[item.name] = item.name;
                itemAliasModules_[item.name] = braceFullPath;
            }
        }
        importedModules_.insert(moduleName);
        return;
    }
    // 路径导入 / 重命名导入：导入整个模块（限定访问 模块::符号）。
    // 第 4 层：路径导入语义 = 该模块任意公开符号可限定访问（与通配符导入
    //   对 useHasSymbol 判定等价）——设 wildcard=true，使 visitCallExpr
    //   的 useHasSymbol 校验通过（模块公开符号表 modulePublicSymbols_ 实际
    //   决定符号存在性，wildcard 仅放宽"符号已导入"校验）。
    use.wildcard = true;
    if (!node->alias.empty()) {
        // 重命名导入：导入 模块 作为 别名——别名绑定到模块级符号
        //   （调用 别名::符号 时按原模块解析；本层别名表记录）
        use.aliases[node->alias] = moduleName;
        // A-5（整路径重命名绑定模块级别名）：别名绑定完整路径（含子模块/包
        //   路径 甲::乙），并登记别名本身可导入（别名::符号 限定调用路径解析）
        std::string fullPath;
        for (std::size_t si = 0; si < node->segments.size(); ++si) {
            if (si > 0) fullPath += "::";
            fullPath += node->segments[si];
        }
        moduleAliases_[node->alias] = fullPath;
        importedModules_.insert(node->alias);
        useImports_[node->alias].wildcard = true;
    }
    importedModules_.insert(moduleName);
}
void SemanticAnalyzer::visitClassDecl(ClassDecl* node) {
    (void)node;
}
void SemanticAnalyzer::visitClassMember(ClassMember* node) {
    (void)node;
}
void SemanticAnalyzer::visitInterfaceDecl(InterfaceDecl* node) {
    (void)node;
}
void SemanticAnalyzer::visitGenericDecl(GenericDecl* node) {
    (void)node;
}
void SemanticAnalyzer::checkFunctionBody(FunctionDecl* node) {
    // 函数符号必须已注册（原型声明无函数体）。Task 2.10：按签名 key 查询
    auto it = functions_.find(node->sigKey);
    if (it == functions_.end()) return;
    if (node->body == nullptr) return;  // 函数原型声明：无需检查体

    currentReturnType_ = it->second.returnType;
    // P3-18 补完（2026-08）：当前函数是否引用返回（visitReturnStmt 校验用）
    currentIsRefReturn_ = it->second.isRefReturn;
    // 收集当前函数引用参数名：引用返回允许"返回引用参数"（指向调用方存储，
    // 悬垂只发生在返回本函数局部/按值参数时）
    currentRefParams_.clear();
    for (auto& param : node->params) {
        if (!param->funcPtr.isFunctionPtr() && types::isReference(param->typeName)) {
            currentRefParams_.insert(param->name);
        }
    }
    // 当前函数作用域起始索引：scopes_ 中索引 >= 该值的绑定属函数局部
    // （引用返回局部检查：返回本函数局部变量/按值参数地址 -> 悬垂引用报错）
    funcScopeStart_ = static_cast<int>(scopes_.size());
    // 阶段3（Task 3.9）：记录当前上下文函数名（友元函数访问检查用）
    currentFunctionName_ = node->name;
    // A-2（crate 分桶）：记录当前分析上下文模块名——类型/常量/静态引用按此解析
    //   （多模块同名符号各自命中本模块的定义）
    const std::string savedModule = currentModuleName_;
    currentModuleName_ = node->moduleName;
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
    currentIsRefReturn_ = false;
    currentRefParams_.clear();
    funcScopeStart_ = -1;
    currentFunctionName_.clear();  // 阶段3：退出函数上下文
    currentModuleName_ = savedModule;  // A-2：恢复外层模块上下文
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
bool SemanticAnalyzer::refReturnLvalueBase(const Expr* e, std::string& baseName) {
    if (e == nullptr) return false;
    switch (e->getType()) {
        case NodeType::IdentifierExpr:
            baseName = static_cast<const IdentifierExpr*>(e)->name;
            return true;
        case NodeType::IndexExpr:
            // 数组元素：递归解到数组/指针变量名
            return refReturnLvalueBase(
                static_cast<const IndexExpr*>(e)->object.get(), baseName);
        case NodeType::MemberExpr: {
            // 成员字段：解到对象名（对象生命周期决定字段引用是否悬垂）；
            // -> 成员 = 指针所指（*p 同类），可作返回（指向堆/调用方存储）
            const MemberExpr* m = static_cast<const MemberExpr*>(e);
            if (m->isArrow) return true;
            return refReturnLvalueBase(m->object.get(), baseName);
        }
        case NodeType::UnaryExpr: {
            // 解引用 *p：返回指针所指地址（可堆/全局/调用方存储）
            const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
            return u->op == Operator::Deref;
        }
        case NodeType::CallExpr:
            // 引用返回调用链（如 返回 传回(x)）：递归解到传回 的返回 lvalue/参数
            // （此处无需解剖——调用方存储可作返回；展开可简化为放行）
            return true;
        default:
            return false;
    }
}
bool SemanticAnalyzer::isRefParamForCurrentFn(const std::string& name) const {
    return currentRefParams_.count(name) > 0;
}
bool SemanticAnalyzer::isCurrentFnLocal(const std::string& name) const {
    if (funcScopeStart_ < 0) return false;
    int idx = 0;
    for (auto it = scopes_.begin(); it != scopes_.end(); ++it, ++idx) {
        if (it->find(name) != it->end()) return idx >= funcScopeStart_;
    }
    return false;  // 未在作用域找到：全局/静态/未知 → 放行（不拦）
}
bool SemanticAnalyzer::argIsBoundMethodValue(const Expr* e) {
    // P3-23 补完（D2）：对象.实例方法 作值为"绑定 this 的闭包"（fnptr 无法携带
    //   this）；用作裸 fnptr 实参属静默错行为，编译报错提示先赋给变量。
    return e != nullptr && e->getType() == NodeType::MemberExpr &&
           static_cast<const MemberExpr*>(e)->isMethodValue;
}
} // namespace cn_compiler
