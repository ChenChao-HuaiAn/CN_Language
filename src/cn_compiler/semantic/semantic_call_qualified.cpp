// CN 语义分析器——模块限定调用族（D1 行数整改 173-a：自 semantic_call.cpp 按族拆出）
//   族 = 模块限定调用（模块.函数(实参) / 包::模块::符号(...)）的信息收集与重写
//   （collectQualifiedCallInfo + rewriteQualifiedCall + QualifiedCallInfo，
//   170-a struct 化提取的族A 两方法）；纯重构零行为变更（成员函数实现搬迁
//   ——声明仍在 semantic.hpp）。文件级整改背景：semantic_call.cpp 1127 行
//   超 ≤1000 行门禁（2026-09-15 家机历史成果审查专项立案）。
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <utility>

#include <memory>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// ===== 族A：模块限定调用 模块.函数(实参) / 包::模块::符号(...)（原 visitCallExpr 116~268 段，170-a struct 化提取）=====
// 分派依据：callee 若是函数名（在函数符号表中）→ 直接调用；
//           否则检查其类型，若是函数指针变量 → 间接调用；
//           阶段3：成员方法调用（对象.方法(...)）、内置构造器（正常/错误/某些）、
//           泛型实例化（类型名<实参>(...)）
// ---- 收集面：展平路径 + 值拷贝名称 + 算齐判定布尔（原 120~211 段）----
// 语法：导入 数学.平方根 后调用 数学.平方根(16.0)——parseCallOrMember 将其
//   解析为 CallExpr(MemberExpr(标识符"数学", "平方根"))。
// 识别：object 为标识符且名字在 importedModules_（已导入模块名）中，
//   且该名字不是类型名（结构体/枚举/类）→ 重写 callee 为直接函数名，
//   复用"直接函数名调用"路径（重载决议/参数检查/IR 符号生成均无需改动）。
// Task 6.3 数学内置函数特判：内置函数注册为带点限定名（数学.平方根）。
//   优先级：用户模块函数优先——若模块 数学 已导入且公开符号合并后存在纯名
//   平方根（用户自定义 数学.cn 的公开函数），走"普通模块函数"路径（重写为纯名）；
//   否则若限定名是已注册内置函数（数学.平方根 全局注册，无需导入，
//   如 stdlib/数学.cn 模块体内直接写 数学.平方根(值)），保留限定名走内置路径。
//   即：内置限定名仅在"无同名用户模块公开函数"时生效，二者不冲突。
// 悬垂纪律：重写会整体替换 callee（销毁旧 MemberExpr 致 mem 悬垂），因此
//   收集面只读 mem 并把全部所需名称值拷贝进 info（重写面不再触碰表达式树）。
bool SemanticAnalyzer::collectQualifiedCallInfo(const MemberExpr& mem,
                                                QualifiedCallInfo& info) {
    // v2.1（2026-09-03）：-> 语法已废除（成员访问统一 .），限定路径调用
    //   不再有 isArrow 排除分支——原守卫删除，路径重写逻辑无条件进入。
    // 第 4 层（v2.0 决策1）：多段路径 包::模块::符号 解析——parser 将
    //   数学::平方根 折叠为 MemberExpr(标识符"数学", "平方根")；多段
    //   包::模块::符号 折叠为嵌套 MemberExpr(MemberExpr(标识符"包","模块"),"符号")。
    //   此处把嵌套 MemberExpr 展平为完整路径字符串（包::模块::符号）判定：
    //   取最深 object 为模块名（首段），memberName 链拼接为完整符号路径。
    const Expr* objPtr = mem.object.get();
    while (objPtr->getType() == NodeType::MemberExpr) {
        const auto* nested = static_cast<const MemberExpr*>(objPtr);
        info.pathPrefix = nested->memberName + "::" + info.pathPrefix;
        objPtr = nested->object.get();
    }
    if (objPtr->getType() != NodeType::IdentifierExpr) return false;
    // 值拷贝：重写会销毁旧 MemberExpr（mem->object 悬垂），须先取名字
    info.moduleName = static_cast<const IdentifierExpr*>(objPtr)->name;
    // A-5（整路径重命名绑定模块级别名）：别名::符号 重映射为完整路径
    //   （导入 甲::乙 作为 丙 -> 丙::连接() 解析 甲::乙::连接()）
    const std::string moduleNameRaw = info.moduleName;
    auto maIt = moduleAliases_.find(moduleNameRaw);
    if (maIt != moduleAliases_.end()) {
        info.moduleName = maIt->second;
    }
    // 完整限定名：模块名::[中间路径::]函数名（数学::平方根 / 包::模块::符号）
    info.qualified = info.moduleName + "::" + info.pathPrefix + mem.memberName;
    // isTypeName：首段是类型名（结构体/枚举/类）→ 对象构造/枚举访问面，非模块调用
    info.isTypeName = isStructType(info.moduleName) || isEnumType(info.moduleName) ||
                      findClass(info.moduleName) != nullptr ||
                      findInterface(info.moduleName) != nullptr;
    // 先取函数名到 info（重写面将销毁旧 MemberExpr，mem 悬垂，不可再取！）
    // Task 6.1：模块限定泛型调用 核心.交换<整32>(...)——memberName
    //   已含泛型实参（交换<整32>），限定名携带 <...> 供下方单态化识别。
    info.funcName = mem.memberName;
    // 第 4 层：限定名统一为 :: 分隔（v2.0 内置 key 化）；旧点号路径
    //   兼容（第 6 层迁移前，25_math 等 E2E 仍用 数学.平方根）。
    // 兼容旧点号限定名（内置函数注册曾用 数学.平方根；v2.0 已改为 ::）
    info.qualifiedDot = info.moduleName + "." + mem.memberName;
    // Task 6.1：泛型函数名剥离 <实参> 查纯名（交换<整32> -> 交换），
    //   泛型函数以纯名注册（registerGenerics），用户公开函数判定用纯名
    const std::size_t funcGenLt = info.funcName.find('<');
    const std::string funcBaseName =
        (funcGenLt == std::string::npos) ? info.funcName : info.funcName.substr(0, funcGenLt);
    // 已导入模块的公开函数优先（用户模块 数学.cn 的公开符号合并为纯名）。
    // Task 6.1：泛型函数注册在 generics_（非 functions_），hasFunctionName
    //   查不到——补充 findGeneric 判定（泛型函数以纯名注册，可跨模块实例化）。
    const bool moduleImported =
        importedModules_.count(info.moduleName) > 0 ||
        importedModules_.count(moduleNameRaw) > 0;
    // 第 4 层（P1-2 目录层级）：子模块路径——net::transport::send() 中
    //   send 属于子模块 net::transport（模块名含 ::）。父模块 net 已导入
    //   （路径导入 wildcard），子模块公开符号经 merge 合并（moduleName=
    //   "net::transport"）。模块符号名 = 完整路径前缀（net::transport）。
    info.subModule = info.moduleName;
    if (!info.pathPrefix.empty()) {
        const std::string mid = info.pathPrefix.substr(0, info.pathPrefix.size() - 2);
        info.subModule = info.moduleName + "::" + mid;
    }
    // plans/018 呈报一B（2026-09-07 用户终裁）：P1-1 废止——限定调用
    //   按语音「模块已加载」放行（Rust 习惯：路径项 m::f() 恒可用，
    //   导入 use 只影响不带前缀的名字）。加载判定 = 显式导入过（任意
    //   形式导入均登记首段模块名）或模块在已加载集合 knownModules_
    //   （合并声明 moduleName 全集 + driver 注入的 crate/包名）。
    //   「先导入才能限定调用」旧规废止，E0255 具名绑定冲突检查补位。
    info.moduleLoaded =
        moduleImported || knownModules_.count(info.moduleName) > 0 ||
        knownModules_.count(info.subModule) > 0;
    // 用户模块公开函数（全局符号表存在该函数名，含泛型；限定调用的
    //   模块归属过滤由 resolveOverload 按 moduleFilter 完成——符号不属
    //   该模块时决议失败报「未找到匹配的函数」，归属校验单一归属）
    info.userFuncExists =
        hasFunctionName(funcBaseName) || findGeneric(funcBaseName) != nullptr;
    // prelude 例外（第 4 层）：内置限定名（数学::平方根 等 24 个）无需
    //   显式导入即可用（核心 包 prelude）；用户模块公开函数优先。
    info.builtinQualified = hasFunctionName(info.qualified) ||
                            hasFunctionName(info.qualifiedDot);
    info.qualifiedClass = isClassType(info.qualified);
    return true;
}

// ---- 重写面：按收集信息执行 callee 重写/未加载诊断（原 214~266 段）----
//   只消费 info，不触碰原 MemberExpr（重写整体替换 callee）。
void SemanticAnalyzer::rewriteQualifiedCall(CallExpr* node,
                                            const QualifiedCallInfo& info) {
    // 内置限定名（prelude）独立于模块加载判定——核心包内置函数恒可用
    if (!info.isTypeName &&
        (info.builtinQualified ||
         (info.moduleLoaded && (info.userFuncExists || info.qualifiedClass)))) {
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
        if (info.userFuncExists &&
            !(info.builtinQualified && currentModuleName_ == info.moduleName)) {
            node->callee = std::make_unique<IdentifierExpr>(info.funcName);
            // 模块过滤（resolveOverload 用）：子模块限定调用按子模块名过滤
            node->moduleFilter =
                (!info.pathPrefix.empty()) ? info.subModule : info.moduleName;
        } else if (hasFunctionName(info.qualified)) {
            // 内置函数（数学::平方根 等）：保留 :: 限定名作标识符
            node->callee = std::make_unique<IdentifierExpr>(info.qualified);
        } else if (hasFunctionName(info.qualifiedDot)) {
            // 兼容旧点号内置名（数学.平方根，v1.0）：保留点号限定名
            node->callee = std::make_unique<IdentifierExpr>(info.qualifiedDot);
        } else if (info.qualifiedClass) {
            // P2-16：模块限定类构造调用（模块::类(...)）——
            //   保留 :: 限定名走类构造路径（findClass 模块感知解析）
            node->callee = std::make_unique<IdentifierExpr>(info.qualified);
            node->moduleFilter = info.moduleName;
        } else {
            // 模块已加载但既非已知函数/内置限定名/类：公开符号不存在
            //   → 报错，避免走"函数指针间接调用"静默路径
            node->callee = std::make_unique<IdentifierExpr>(info.funcName);
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "模块 '" + info.moduleName + "' 没有公开符号 '" +
                                    info.funcName + "'");
        }
    } else if (!info.isTypeName && !info.moduleLoaded && !info.builtinQualified) {
        // 对象方法调用排除：object 是局部变量/参数（动物.描述()）时，
        //   moduleName 是变量名而非模块名——跳过，走下方成员方法
        //   调用路径（防误判：变量名不在类型名/模块名中）。
        std::string objType;
        const bool objIsVar = lookupVar(info.moduleName, objType);
        if (!objIsVar) {
            // plans/018 呈报一B：P1-1 旧文「未导入」废止——模块从未被
            //   加载（未导入且合并声明/依赖包均无此模块）才报错
            node->callee = std::make_unique<IdentifierExpr>(info.funcName);
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "未声明的标识符 '" + info.qualified +
                                    "'（模块 '" + info.moduleName +
                                    "' 未加载；请检查模块是否存在或在货舱.toml 声明依赖）");
        }
    }
}

}  // namespace cn_compiler
