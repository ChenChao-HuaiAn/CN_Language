// 语义调用检查·调用点实参包装与调用名改写族（1855 拆分轮追加·行数门禁 ≤500 立法
//   2026-11-04 用户令·自 semantic_call.cpp 纯机械搬移·段体逐字保留零行为变更）
// 职责：①引用实参取地址包装（wrapRefArgs/wrapRefArg·A-1 引用参数按地址传递）；
//   ②use 导入绑定名重写（rewriteUseImportAlias·第 4 层 use 导入表）；
//   ③泛型函数调用单态化（registerGenericFuncInstance/rewriteGenericFuncCall·Task 6.1）
//   成员函数声明仍在 semantic.hpp 类体（semantic_call_decls.hpp 片段承载）。
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/switch_enum_gate.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
void SemanticAnalyzer::wrapRefArgs(CallExpr* node,
                                   const std::vector<std::string>& paramTypes) {
    const std::size_t n = std::min(node->arguments.size(), paramTypes.size());
    for (std::size_t i = 0; i < n; ++i) {
        wrapRefArg(node->arguments[i], paramTypes[i]);
    }
}

// 1675 根修：引用实参取地址包装的单实参核心（原 wrapRefArgs 循环体原样抽出）——
//   调用面=普通调用（wrapRefArgs）与运算符重载决议命中点（族⑦算术/族⑧比较，
//   见 semantic_expr_binary.cpp）：重载降级为成员方法调用后右操作数=实参，
//   引用形参（类&）须与普通调用同口径取地址，否则方法体内引用两级解引用约定
//   与调用方传值差一层（o2 探针 C0000005 实录：调用方传一级堆指针，被调方
//   [o] 取首字段值当对象地址再解引用→读地址 1 崩溃）。
void SemanticAnalyzer::wrapRefArg(std::unique_ptr<Expr>& arg,
                                  const std::string& paramType) {
    if (!types::isReference(paramType)) return;
    Expr* raw = arg.get();
    // 2026-08-25 H3 幂等：实参已是 &x（AddressOf）——共享 AST 重复检查时
    //   二次包装不再套（否则 前驱 -> &前驱 -> &(&前驱)=整64*）
    if (raw->getType() == NodeType::UnaryExpr &&
        static_cast<UnaryExpr*>(raw)->op == Operator::AddressOf) {
        return;
    }
    bool isLvalue = false;
    CN_SWITCH_ENUM_EXEMPT_BEGIN
    // 存量清偿挂#2074：漏 case 靠 default 挡存量（枚举穷举基线·逐处显式补 case 后删包裹）
    switch (raw->getType()) {
        case NodeType::IdentifierExpr: {
            std::string vt;
            isLvalue = lookupVar(static_cast<IdentifierExpr*>(raw)->name, vt);
            // 188-a（D6 B11 变量常量传播）：引用实参=别名逃逸（被调方可能改写
            //   该变量）→ 恒空判定失格（保守，防假阳性）
            if (isLvalue) {
                noteNullEscape(static_cast<IdentifierExpr*>(raw)->name);
                // 548（编译期除零·常量传播）：引用实参=别名逃逸同款失格
                noteZeroDisqual(static_cast<IdentifierExpr*>(raw)->name);
            }
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
            isLvalue = (static_cast<UnaryExpr*>(raw)->op == Operator::Deref ||
                        static_cast<UnaryExpr*>(raw)->op == Operator::AddressOf);
            break;
        default:
            isLvalue = false;
            break;
    }
    CN_SWITCH_ENUM_EXEMPT_END
    if (!isLvalue) {
        diagnostics_.report(DiagnosticLevel::Error, raw->location,
                            "引用参数要求左值实参（不能对常量/字面量/临时值取地址）");
        return;
    }
    const SourceLocation loc = raw->location;
    arg = std::make_unique<UnaryExpr>(Operator::AddressOf, std::move(arg));
    arg->location = loc;
    // 588-a（005 甲案）：包装节点=内部引用机制形态（IR 层维持 AddrOf 槽——
    //   A-1 契约"引用形参槽存被引用左值地址"，探针实证写回/读/方法/换绑
    //   全链依赖此形态），与用户显式 &（类类型→Load 槽产对象地址）区分。
    static_cast<UnaryExpr*>(arg.get())->refWrapAddr = true;
}

// 子族A：第 4 层 use 导入表——花括号/路径导入绑定名重写（原 visitCallExpr 273~311 段）：
//   calleeName 为绑定名时映射回原符号名，并携带来源模块（moduleFilter）。
void SemanticAnalyzer::rewriteUseImportAlias(CallExpr* node, std::string& calleeName) {
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
                    //   未命中再落到重写后的符号名（自映射绑定 条目以符号名为键）。
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
}

// 317-a：泛型函数实例化记录登记（去重）——显式 <> 调用段与推断段共用。
void SemanticAnalyzer::registerGenericFuncInstance(
    const std::string& instName, const GenericInfo* ginfo,
    const std::vector<std::string>& args) {
    if (ginfo == nullptr || ginfo->ast->innerFunc == nullptr) return;
    for (const auto& gi : genericFuncInstances_) {
        if (gi.instanceName == instName) return;   // 已登记（去重）
    }
    GenericFuncInstance gfi;
    gfi.instanceName = instName;
    gfi.gen = ginfo->ast;
    gfi.args = args;
    genericFuncInstances_.push_back(std::move(gfi));
}

// 子族B：Task 6.1 泛型函数调用单态化（原 visitCallExpr 312~383 段）：识别 名<类型>(实参)，
//   实例化（instantiateGeneric 注册 名$实参）并重写 callee；登记 genericFuncInstances_。
void SemanticAnalyzer::rewriteGenericFuncCall(CallExpr* node, std::string& calleeName) {
        // ---- 317-a（T19②）+ 303（混合形态放宽）：推断式泛型函数调用 ----
        // 场景①（317-a 原）：泛型函数体内嵌套泛型调用 两倍(T 值){ 返回 恒等(值); }
        // 场景②（303·293 呈报#5）：用户代码 线性查找(值, 4, 30)——参数表为
        //   混合形态（T* 数组 + 整64 固定参 + T），原「纯 T 参」保守判据不推断
        //   →裸名「未声明的标识符」（新手高频卡点）。推导逻辑见
        //   inferGenericArgs（generics.cpp·303 逐参数对齐：类型参数位从实参
        //   推导·固定类型位可转换即可·函数指针位不参与；全部类型参数被约束且
        //   一致才单态化，否则放弃——显式 <> 仍可用）。
        if (calleeName.find('<') == std::string::npos && !node->arguments.empty()) {
            // 静默豁免业务值: findGeneric miss=nullptr 哨兵+下一行显式判空分流（882 处置②·未命中即让位既有调用路径）
            const GenericInfo* gi = findGeneric(calleeName).value_or(nullptr);
            if (gi != nullptr && gi->ast->innerFunc != nullptr) {
                const FunctionDecl* src = gi->ast->innerFunc.get();
                std::vector<std::string> infArgs;
                if (src->params.size() == node->arguments.size() &&
                    inferGenericArgs(gi, src, node, infArgs)) {
                    const cn::Optional<std::string> instName =
                        instantiateGeneric(calleeName, infArgs, node->location);
                    if (instName.has_value()) {
                        registerGenericFuncInstance(instName.value(), gi, infArgs);
                        node->callee = std::make_unique<IdentifierExpr>(instName.value());
                        calleeName = instName.value();
                    }
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
            if (findGeneric(head).has_value() &&
                findGeneric(head).value()->ast->innerFunc != nullptr) {
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
                const cn::Optional<std::string> instName =
                    instantiateGeneric(head, args, node->location);
                if (instName.has_value()) {
                    // 重写 callee 为实例化函数名（名$实参），直接函数调用路径命中
                    node->callee = std::make_unique<IdentifierExpr>(instName.value());
                    calleeName = instName.value();
                    // 登记泛型函数实例化记录（供 IR 层生成函数体）：
                    //   记录 实例化名 + 原泛型声明 + 类型实参（替换类型参数用）
                    // 静默豁免业务值: 同款 nullptr 哨兵+判空分流（miss=泛型声明已消失的滞后形态·守卫假即跳过登记）
                    const GenericInfo* ginfo = findGeneric(head).value_or(nullptr);
                    if (ginfo != nullptr && ginfo->ast->innerFunc != nullptr) {
                        registerGenericFuncInstance(instName.value(), ginfo, args);
                    }
                }
            }
        }
}

} // namespace cn_compiler
