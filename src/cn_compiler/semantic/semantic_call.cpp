// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {


namespace {

// 比较运算符（== != < > <= >=）
[[maybe_unused]] bool isComparisonOp(Operator op) {
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
[[maybe_unused]] bool isLogicalOp(Operator op) {
    switch (op) {
        case Operator::AndAnd: case Operator::OrOr: case Operator::Bang:
            return true;
        default:
            return false;
    }
}

// 位运算符（& | ^ ~ << >>）
[[maybe_unused]] bool isBitwiseOp(Operator op) {
    switch (op) {
        case Operator::Amp: case Operator::Pipe: case Operator::Caret:
        case Operator::Tilde: case Operator::LessLess: case Operator::GreaterGreater:
            return true;
        default:
            return false;
    }
}

// 算术运算符（+ - * / %）
[[maybe_unused]] bool isArithmeticOp(Operator op) {
    return op == Operator::Add || op == Operator::Subtract ||
           op == Operator::Multiply || op == Operator::Divide ||
           op == Operator::Modulo;
}

// 指针类型辅助（Task 2.4）：是否指针类型 / 是否数组类型
[[maybe_unused]] bool isPointerType(const std::string& type) {
    return types::isPointer(type);
}
[[maybe_unused]] bool isArrayType(const std::string& type) {
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
[[maybe_unused]] bool isFuncPtrTypeStr(const std::string& type) {
    return type.rfind("函数指针<", 0) == 0;
}

// 从函数指针类型字符串提取返回类型（"函数指针<整32>(整32,整32)" -> "整32"）
[[maybe_unused]] std::string funcPtrReturn(const std::string& type) {
    std::size_t lt = type.find('<');
    std::size_t gt = type.find('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    return type.substr(lt + 1, gt - lt - 1);
}

// 从函数指针类型字符串提取参数类型列表
// "函数指针<整32>(整32,整32)" -> ["整32","整32"]
[[maybe_unused]] std::vector<std::string> funcPtrParams(const std::string& type) {
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

void SemanticAnalyzer::wrapRefArgs(CallExpr* node,
                                   const std::vector<std::string>& paramTypes) {
    const std::size_t n = std::min(node->arguments.size(), paramTypes.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (!types::isReference(paramTypes[i])) continue;
        Expr* arg = node->arguments[i].get();
        // 2026-08-25 H3 幂等：实参已是 &x（AddressOf）——共享 AST 重复检查时
        //   二次 wrapRefArgs 不再包装（否则 前驱 -> &前驱 -> &(&前驱)=整64*）
        if (arg->getType() == NodeType::UnaryExpr &&
            static_cast<UnaryExpr*>(arg)->op == Operator::AddressOf) {
            continue;
        }
        bool isLvalue = false;
        switch (arg->getType()) {
            case NodeType::IdentifierExpr: {
                std::string vt;
                isLvalue = lookupVar(static_cast<IdentifierExpr*>(arg)->name, vt);
                break;
            }
            case NodeType::IndexExpr:
            case NodeType::MemberExpr:
            case NodeType::SelfExpr:
                isLvalue = true;
                break;
            case NodeType::UnaryExpr:
                // 2026-08-25 H3：Deref 与 AddressOf（已 wrap 过 &前驱）都是左值——
                //   嵌套实例化共享 AST 二次检查时 &前驱 不应再报"非左值"
                isLvalue = (static_cast<UnaryExpr*>(arg)->op == Operator::Deref ||
                            static_cast<UnaryExpr*>(arg)->op == Operator::AddressOf);
                break;
            default:
                isLvalue = false;
                break;
        }
        if (!isLvalue) {
            diagnostics_.report(DiagnosticLevel::Error, arg->location,
                                "引用参数要求左值实参（不能对常量/字面量/临时值取地址）");
            continue;
        }
        const SourceLocation loc = arg->location;
        node->arguments[i] = std::make_unique<UnaryExpr>(
            Operator::AddressOf, std::move(node->arguments[i]));
        node->arguments[i]->location = loc;
    }
}
void SemanticAnalyzer::visitCallExpr(CallExpr* node) {
    // P3-18 补完：本轮默认非引用返回；决议到引用返回函数时置 true
    node->isRefReturnCall = false;
    lastExprIsRefReturn_ = false;
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
        // v2.1（2026-09-03）：-> 语法已废除（成员访问统一 .），限定路径调用
        //   不再有 isArrow 排除分支——原守卫删除，路径重写逻辑无条件进入。
        // 第 4 层（v2.0 决策1）：多段路径 包::模块::符号 解析——parser 将
        //   数学::平方根 折叠为 MemberExpr(标识符"数学", "平方根")；多段
        //   包::模块::符号 折叠为嵌套 MemberExpr(MemberExpr(标识符"包","模块"),"符号")。
        //   此处把嵌套 MemberExpr 展平为完整路径字符串（包::模块::符号）判定：
        //   取最深 object 为模块名（首段），memberName 链拼接为完整符号路径。
        std::string pathPrefix;      // 嵌套路径前缀（如 包::模块 或 数学）
        Expr* objPtr = mem->object.get();
        MemberExpr* nested = nullptr;
        while (objPtr->getType() == NodeType::MemberExpr) {
            nested = static_cast<MemberExpr*>(objPtr);
            pathPrefix = nested->memberName + "::" + pathPrefix;
            objPtr = nested->object.get();
        }
        if (objPtr->getType() == NodeType::IdentifierExpr) {
            // 值拷贝：重写会销毁旧 MemberExpr（mem->object 悬垂），须先取名字
            std::string moduleName =
                static_cast<IdentifierExpr*>(objPtr)->name;
            // A-5（整路径重命名绑定模块级别名）：别名::符号 重映射为完整路径
            //   （导入 甲::乙 作为 丙 -> 丙::连接() 解析 甲::乙::连接()）
            const std::string moduleNameRaw = moduleName;
            auto maIt = moduleAliases_.find(moduleNameRaw);
            if (maIt != moduleAliases_.end()) {
                moduleName = maIt->second;
            }
            // 完整限定名：模块名::[中间路径::]函数名（数学::平方根 / 包::模块::符号）
            const std::string fullPath = moduleName + "::" + pathPrefix + mem->memberName;
            const bool isTypeName = isStructType(moduleName) || isEnumType(moduleName) ||
                                    findClass(moduleName) != nullptr ||
                                    findInterface(moduleName) != nullptr;
            // 先取函数名到局部变量（下方重写会销毁旧 MemberExpr，mem 悬垂！）
            // Task 6.1：模块限定泛型调用 核心.交换<整32>(...)——memberName
            //   已含泛型实参（交换<整32>），限定名携带 <...> 供下方单态化识别。
            const std::string funcName = mem->memberName;
            // 第 4 层：限定名统一为 :: 分隔（v2.0 内置 key 化）；旧点号路径
            //   兼容（第 6 层迁移前，25_math 等 E2E 仍用 数学.平方根）。
            const std::string qualified = fullPath;
            // 兼容旧点号限定名（内置函数注册曾用 数学.平方根；v2.0 已改为 ::）
            const std::string qualifiedDot = moduleName + "." + mem->memberName;
            // Task 6.1：泛型函数名剥离 <实参> 查纯名（交换<整32> -> 交换），
            //   泛型函数以纯名注册（registerGenerics），用户公开函数判定用纯名
            const std::size_t funcGenLt = funcName.find('<');
            const std::string funcBaseName =
                (funcGenLt == std::string::npos) ? funcName : funcName.substr(0, funcGenLt);
            // 已导入模块的公开函数优先（用户模块 数学.cn 的公开符号合并为纯名）。
            // Task 6.1：泛型函数注册在 generics_（非 functions_），hasFunctionName
            //   查不到——补充 findGeneric 判定（泛型函数以纯名注册，可跨模块实例化）。
            const bool moduleImported =
                importedModules_.count(moduleName) > 0 ||
                importedModules_.count(moduleNameRaw) > 0;
            // 第 4 层（P1-2 目录层级）：子模块路径——net::transport::send() 中
            //   send 属于子模块 net::transport（模块名含 ::）。父模块 net 已导入
            //   （路径导入 wildcard），子模块公开符号经 merge 合并（moduleName=
            //   "net::transport"）。模块符号名 = 完整路径前缀（net::transport）。
            std::string subModule = moduleName;
            if (!pathPrefix.empty()) {
                const std::string mid = pathPrefix.substr(0, pathPrefix.size() - 2);
                subModule = moduleName + "::" + mid;
            }
            // plans/018 呈报一B（2026-09-07 用户终裁）：P1-1 废止——限定调用
            //   按语音「模块已加载」放行（Rust 习惯：路径项 m::f() 恒可用，
            //   导入 use 只影响不带前缀的名字）。加载判定 = 显式导入过（任意
            //   形式导入均登记首段模块名）或模块在已加载集合 knownModules_
            //   （合并声明 moduleName 全集 + driver 注入的 crate/包名）。
            //   「先导入才能限定调用」旧规废止，E0255 具名绑定冲突检查补位。
            const bool moduleLoaded =
                moduleImported || knownModules_.count(moduleName) > 0 ||
                knownModules_.count(subModule) > 0;
            // 用户模块公开函数（全局符号表存在该函数名，含泛型；限定调用的
            //   模块归属过滤由 resolveOverload 按 moduleFilter 完成——符号不属
            //   该模块时决议失败报「未找到匹配的函数」，归属校验单一归属）
            const bool userFuncExists =
                hasFunctionName(funcBaseName) || findGeneric(funcBaseName) != nullptr;
            // prelude 例外（第 4 层）：内置限定名（数学::平方根 等 24 个）无需
            //   显式导入即可用（核心 包 prelude）；用户模块公开函数优先。
            const bool builtinQualified = hasFunctionName(qualified) ||
                                          hasFunctionName(qualifiedDot);
            const bool qualifiedClass = isClassType(qualified);
            // 内置限定名（prelude）独立于模块加载判定——核心包内置函数恒可用
            if (!isTypeName &&
                (builtinQualified ||
                 (moduleLoaded && (userFuncExists || qualifiedClass)))) {
                // 用户模块公开函数：重写为直接函数名（成员方法调用分支不再命中 MemberExpr）。
                //   泛型函数保留 名<实参> 完整形态（下方 visitCallExpr 泛型单态化识别）；
                //   普通函数重写为纯名（含 数学.平方根 内置限定名的既有路径）。
                // 第 4 层（crate 隔离）：限定调用带模块上下文——重写后的纯名在
                //   resolveOverload 时按模块过滤（跨模块同名函数不歧义）。
                // 2026-08（自举 Task 7.6 修复）：仅当"当前编译模块 == 目标模块"
                //   （stdlib 包装自引用，如 文件.cn 内部 文件::读取文件行）且内置
                //   限定名存在时，内置优先——否则解析到自身包装（返回 结果 与内置
                //   字符串 类型冲突）。用户模块限定调用（27_module 数学::平方根）
                //   仍用户公开函数优先（内置 数学::平方根 存在但不应遮蔽）。
                if (userFuncExists &&
                    !(builtinQualified && currentModuleName_ == moduleName)) {
                    node->callee = std::make_unique<IdentifierExpr>(funcName);
                    // 模块过滤（resolveOverload 用）：子模块限定调用按子模块名过滤
                    node->moduleFilter = (!pathPrefix.empty()) ? subModule : moduleName;
                } else if (hasFunctionName(qualified)) {
                    // 内置函数（数学::平方根 等）：保留 :: 限定名作标识符
                    node->callee = std::make_unique<IdentifierExpr>(qualified);
                } else if (hasFunctionName(qualifiedDot)) {
                    // 兼容旧点号内置名（数学.平方根，v1.0）：保留点号限定名
                    node->callee = std::make_unique<IdentifierExpr>(qualifiedDot);
                } else if (qualifiedClass) {
                    // P2-16：模块限定类构造调用（模块::类(...)）——
                    //   保留 :: 限定名走类构造路径（findClass 模块感知解析）
                    node->callee = std::make_unique<IdentifierExpr>(qualified);
                    node->moduleFilter = moduleName;
                } else {
                    // 模块已加载但既非已知函数/内置限定名/类：公开符号不存在
                    //   → 报错，避免走"函数指针间接调用"静默路径
                    node->callee = std::make_unique<IdentifierExpr>(funcName);
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "模块 '" + moduleName + "' 没有公开符号 '" +
                                            funcName + "'");
                }
            } else if (!isTypeName && !moduleLoaded && !builtinQualified) {
                // 对象方法调用排除：object 是局部变量/参数（动物.描述()）时，
                //   moduleName 是变量名而非模块名——跳过，走下方成员方法
                //   调用路径（防误判：变量名不在类型名/模块名中）。
                std::string objType;
                const bool objIsVar = lookupVar(moduleName, objType);
                if (!objIsVar) {
                    // plans/018 呈报一B：P1-1 旧文「未导入」废止——模块从未被
                    //   加载（未导入且合并声明/依赖包均无此模块）才报错
                    node->callee = std::make_unique<IdentifierExpr>(funcName);
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "未声明的标识符 '" + qualified +
                                            "'（模块 '" + moduleName +
                                            "' 未加载；请检查模块是否存在或在货舱.toml 声明依赖）");
                }
            }
    }
    }  // 模块限定调用重写块结束（Task 3.6 / 第 4 层 P1-1）
    bool isDirect = false;
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        // 第 4 层（use 导入表）：花括号/路径导入绑定名重写——导入 数学::{正弦 作为 正}
        //   或（呈报一B）导入 数学::正弦 后调用 绑定名() 时，calleeName 是绑定名；
        //   查 useImports_ 各模块别名表映射回原符号名（正弦）。须在 isDirect 判定
        //   之前（绑定名未注册为函数名，hasFunctionName("正") 失败会导致
        //   isDirect=false 走间接调用路径报错）。
        //   限定调用重写（上方 MemberExpr 块）已设置 moduleFilter 时跳过——
        //   呈报一B 后路径导入也登记绑定名（模块X::双倍 绑定 双倍），限定调用
        //   模块Y::双倍 重写为纯名后会被本块按 绑定名 双倍（来源 模块X）二次
        //   重写覆盖过滤器 → 解析到错误模块（44_crate_isolate 实测）。限定
        //   调用的过滤器优先（调用点已显式指定模块归属）。
        if (calleeName.find('<') == std::string::npos &&
            node->moduleFilter.empty()) {
            for (const auto& ui : useImports_) {
                const auto& aliases = ui.second.aliases;
                const auto aliasIt = aliases.find(calleeName);
                if (aliasIt != aliases.end() && aliasIt->second != ui.first) {
                    // A-5（花括号项别名跨模块同名歧义根治）：重写回原符号名时
                    //   携带来源模块（moduleFilter）——resolveOverload 按模块过滤，
                    //   跨模块同名（模块X$双倍 与 模块Y$双倍）纯名别名调用不再歧义。
                    //   来源模块查 itemAliasModules_：按**原绑定名**查（别名 与
                    //   原符号名 不同名时，重写后查原符号名会命中同名的其他绑定
                    //   条目——91_别名跨模块同名 实测 Y双倍 误取 双倍 的来源模块），
                    //   未命中再回退重写后的符号名（自映射绑定 条目以符号名为键）。
                    const std::string bindingName = calleeName;
                    calleeName = aliasIt->second;  // 绑定名 -> 原符号名
                    static_cast<IdentifierExpr*>(node->callee.get())->name = calleeName;
                    auto iamIt = itemAliasModules_.find(bindingName);
                    if (iamIt == itemAliasModules_.end()) {
                        iamIt = itemAliasModules_.find(calleeName);
                    }
                    if (iamIt != itemAliasModules_.end()) {
                        node->moduleFilter = iamIt->second;
                    } else {
                        node->moduleFilter = ui.first;
                    }
                    break;
                }
            }
        }
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
                int angleDepth = 0;
                std::size_t segStart = 0;
                while (pos <= inner.size()) {
                    if (pos == inner.size() ||
                        (inner[pos] == ',' && angleDepth == 0)) {
                        args.push_back(inner.substr(segStart, pos - segStart));
                        segStart = pos + 1;
                        if (pos == inner.size()) break;
                    } else if (inner[pos] == '<') {
                        angleDepth++;
                    } else if (inner[pos] == '>') {
                        angleDepth--;
                    }
                    pos++;
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
                    if (a.find('<') != std::string::npos) {
                        a = resolveGenericTypeName(a, node->location);
                    }
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
                int angleDepth = 0;
                std::size_t segStart = 0;
                while (pos <= inner.size()) {
                    if (pos == inner.size() ||
                        (inner[pos] == ',' && angleDepth == 0)) {
                        args.push_back(inner.substr(segStart, pos - segStart));
                        segStart = pos + 1;
                        if (pos == inner.size()) break;
                    } else if (inner[pos] == '<') {
                        angleDepth++;
                    } else if (inner[pos] == '>') {
                        angleDepth--;
                    }
                    pos++;
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
                    if (a.find('<') != std::string::npos) {
                        a = resolveGenericTypeName(a, node->location);
                    }
                }
                const std::string instName =
                    instantiateGeneric(head, args, node->location);
                if (!instName.empty()) className = instName;
            }
        }
        const ClassInfo* ctorCls = findClass(className);
        if (ctorCls != nullptr) {
            // 查找构造函数（函数名 == 类名）。Debug 子任务修复（构造函数重载）：
            //   methods 表构造条目 key=sigKey（名#参数串），遍历按 isConstructor +
            //   ownerClass（排除父类构造，阶段A-3）+ 实参个数 + 类型可转换 匹配最优。
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
            }
            const ClassMemberInfo* ctor = nullptr;
            const ClassMemberInfo* ctorExact = nullptr;
            for (const auto& mk : ctorCls->methods) {
                const ClassMemberInfo& mi = mk.second;
                if (!mi.isConstructor || mi.ownerClass != className) continue;
                if (mi.paramTypes.size() != argTypes.size()) continue;
                bool ok = true;
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    if (conversionLevel(argTypes[i], mi.paramTypes[i],
                                        isIntLiteralExpr(node->arguments[i].get())) < 0) { ok = false; break; }
                }
                if (!ok) continue;
                ctor = &mi;
                // 精确类型匹配（全部 0 级转换）优先
                bool exact = true;
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    if (conversionLevel(argTypes[i], mi.paramTypes[i],
                                        isIntLiteralExpr(node->arguments[i].get())) != 0) { exact = false; break; }
                }
                if (exact) { ctorExact = &mi; break; }
            }
            if (ctorExact != nullptr) ctor = ctorExact;
            if (ctor != nullptr) {
                if (argTypes.size() != ctor->paramTypes.size()) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "构造函数 '" + className + "' 期望 " +
                                            std::to_string(ctor->paramTypes.size()) +
                                            " 个实参，实际提供 " +
                                            std::to_string(argTypes.size()) + " 个");
                } else {
                    for (std::size_t i = 0; i < argTypes.size(); ++i) {
                        if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], ctor->paramTypes[i])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "构造函数 '" + className + "' 第 " + std::to_string(i + 1) +
                                    " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    ctor->paramTypes[i] + "'");
                        }
                    }
                }
                // A-1（引用参数）：构造形参为引用时实参自动取地址
                wrapRefArgs(node, ctor->paramTypes);
                // 记录选中的构造 sigKey（IR 层按此生成构造体 Call 符号）
                node->resolvedSignature = className + "$" + ctor->sigKey;
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
        // P3-19：接口对象方法调用（图形.方法(实参)）——接口方法经全局槽位运行时分派
        {
            const std::string ifaceName = canonicalType(
                types::isPointer(objType) ? types::pointeeOf(objType) : objType);
            const InterfaceInfo* iface = findInterface(ifaceName);
            if (iface != nullptr) {
                const auto imit = iface->methods.find(methodName);
                if (imit == iface->methods.end()) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "接口 '" + ifaceName + "' 没有成员 '" +
                                            methodName + "'");
                    lastType_ = "未知";
                    return;
                }
                std::vector<std::string> argTypes;
                for (auto& arg : node->arguments) {
                    argTypes.push_back(checkExpr(arg.get()));
                    // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
                    if (argIsBoundMethodValue(arg.get())) {
                        diagnostics_.report(
                            DiagnosticLevel::Error, arg->location,
                            "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：自动 cb = 对象.方法）");
                    }
                }
                if (argTypes.size() != imit->second.paramTypes.size()) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "接口方法 '" + methodName + "' 期望 " +
                            std::to_string(imit->second.paramTypes.size()) +
                            " 个实参，实际提供 " + std::to_string(argTypes.size()) + " 个");
                } else {
                    for (std::size_t i = 0; i < argTypes.size(); ++i) {
                        if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], imit->second.paramTypes[i])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "接口方法 '" + methodName + "' 第 " +
                                    std::to_string(i + 1) + " 个实参无法将 '" +
                                    argTypes[i] + "' 隐式转换为 '" +
                                    imit->second.paramTypes[i] + "'");
                        }
                    }
                }
                lastType_ = imit->second.type;
                lastExprIsRefReturn_ = false;  // 接口方法引用返回暂不支持（方法返回类型 canonical 剥 &）
                return;
            }
        }
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
        // v2.1（成员访问统一 .）：对象为类指针（账户* 账.方法()）自动解引用
        //   一级（≡ (*账).方法()）——类型驱动剥指针，不依赖语义遍历顺序。
        // 簇⑥根治（2026-09-04，与 visitMemberExpr 同款）：泛型实例名可含实参
        //   星号（盒子$整64*——合成名保留尾 *），尾 * 非对象指针语义——原名
        //   查类命中即用原名；真指针（盒子$整64**）不命中类表自然落入剥分支。
        const std::string clsName =
            (findClass(objTypeForClass) != nullptr)
                ? canonicalType(objTypeForClass)
                : (types::isPointer(objTypeForClass)
                       ? canonicalType(types::pointeeOf(objTypeForClass))
                       : canonicalType(objTypeForClass));
        const ClassMemberInfo* method = lookupClassMember(clsName, methodName, ownerClass);
        if (method != nullptr && !method->isStatic) {
            // 实例方法调用：校验参数个数与类型
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
                // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
                if (argIsBoundMethodValue(arg.get())) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, arg->location,
                        "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：自动 cb = 对象.方法）");
                }
            }
            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            } else {
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    // 2026-08-25 H3：形参是引用（整64&）且实参已是 &x（AddressOf）——
                    //   共享 AST（实例化类同一 mi.ast）二次检查时已 wrap，引用已满足，
                    //   跳过转换比较（否则 &前驱=整64* 误报"无法转 整64&"）
                    bool refAlready =
                        types::isReference(method->paramTypes[i]) &&
                        node->arguments[i]->getType() == NodeType::UnaryExpr &&
                        static_cast<UnaryExpr*>(node->arguments[i].get())->op ==
                            Operator::AddressOf;
                    if (!refAlready &&
                        !canConvertWithLiteral(node->arguments[i].get(), argTypes[i], method->paramTypes[i])) {
                        diagnostics_.report(
                            DiagnosticLevel::Error, node->arguments[i]->location,
                            "方法 '" + methodName + "' 第 " + std::to_string(i + 1) +
                                " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                method->paramTypes[i] + "'");
                    }
                }
            }
            // A-1（引用参数）：实例方法引用形参的实参自动取地址
            wrapRefArgs(node, method->paramTypes);
            // 访问控制检查（Task 3.4）
            const std::string contextClass = contextClassStack_.empty()
                                                 ? ""
                                                 : contextClassStack_.back();
            checkAccess(*findClass(ownerClass), *method, contextClass, node->location,
                        "方法");
            lastType_ = method->type;
            lastExprIsRefReturn_ = false;  // 方法引用返回暂不支持（类型 canonical 剥 &）
            return;
        }
        if (method != nullptr && method->isStatic) {
            // 静态方法调用（类名.静态方法(...)）
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
                // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
                if (argIsBoundMethodValue(arg.get())) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, arg->location,
                        "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：自动 cb = 对象.方法）");
                }
            }
            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "静态方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            }
            // A-1（引用参数）：静态方法引用形参的实参自动取地址
            wrapRefArgs(node, method->paramTypes);
            lastType_ = method->type;
            lastExprIsRefReturn_ = false;  // 方法引用返回暂不支持（类型 canonical 剥 &）
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
            // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
            if (argIsBoundMethodValue(arg.get())) {
                diagnostics_.report(
                    DiagnosticLevel::Error, arg->location,
                    "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：自动 cb = 对象.方法）");
            }
        }
        // 第 4 层（crate 隔离）：限定调用按模块过滤（数学::双倍 只解析数学.cn 的）
        // 55-c 方案A：实参字面量标志供决议豁免（`读值(100)` 传 正32 形参保留）
        std::vector<bool> argLitFlags;
        argLitFlags.reserve(node->arguments.size());
        for (const auto& a : node->arguments) {
            argLitFlags.push_back(isIntLiteralExpr(a.get()));
        }
        std::string sigKey = resolveOverload(calleeName, argTypes, node->location,
                                             node->moduleFilter, argLitFlags);
        if (sigKey.empty()) {
            // 决议失败（参数个数/类型不匹配或歧义）：恢复兼容——若纯名存在（内置
            // 单版本函数），按旧逻辑检查，避免错误级联导致 IR 层找不到符号
            auto fallback = functions_.find(calleeName);
            if (fallback != functions_.end()) {
                lastType_ = fallback->second.returnType;
                // P3-18 补完：引用返回函数调用結果可作左值
                if (fallback->second.isRefReturn) {
                    node->isRefReturnCall = true;
                    lastExprIsRefReturn_ = true;
                }
            } else {
                lastType_ = "未知";
            }
            return;
        }
        // plans/018 呈报二 A′（2026-09-07 用户裁决）：resolvedSignature = 注册键。
        //   注册键在 registerFunction 已按函数链接键公式生成（模块条目自带
        //   模块$ 前缀、主/单文件恒裸键），与定义侧 mangledName 同源——
        //   旧「决议后按 it->second.moduleName 补拼前缀」的补丁块删除
        //   （其仅为旧裸键注册方案的对齐补丁，公式化后结构上不存在劈叉）。
        node->resolvedSignature = sigKey;
        auto it = functions_.find(sigKey);
        const FunctionInfo& info = it->second;
        // A-1（引用参数）：引用形参的实参自动取地址（重写为 &左值）——
        //   须在 IR 层实参求值之前（IR genExpr 对 AddressOf 生成 lvalueAddress）
        wrapRefArgs(node, info.paramTypes);
        // 参数类型检查（决议已保证可转换；此处再逐个报告具体错误位置）。
        //   55-c 方案A：字面量实参豁免与决议豁免（conversionLevel）同步——
        //   决议按宽化级放行的字面量形态此处不再误报
        for (std::size_t i = 0; i < node->arguments.size(); i++) {
            const std::string& paramType = info.paramTypes[i];
            if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], paramType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                    "函数 '" + calleeName + "' 第 " + std::to_string(i + 1) +
                                    " 个参数无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    paramType + "'");
            }
        }
        lastType_ = info.returnType;
        // P3-18 补完：引用返回函数调用結果可作左值（整32& r = 获取() / 获取()=值 / &获取()）
        if (info.isRefReturn) {
            node->isRefReturnCall = true;
            lastExprIsRefReturn_ = true;
        }
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
            if (!canConvertWithLiteral(node->arguments[i].get(), argType, paramType)) {
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
} // namespace cn_compiler
