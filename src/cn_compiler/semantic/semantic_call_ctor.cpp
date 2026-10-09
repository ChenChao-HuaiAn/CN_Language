// 语义调用检查·族B（357 重构F2 自 semantic_call.cpp 纯机械搬移·函数体逐字保留）
// 职责：构造函数调用 类名(实参)——泛型构造名解析+构造调用检查
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

// ===== 族B：构造函数调用 类名(实参)（原 visitCallExpr 295~405 段）=====
// 族B1：泛型类构造单态化（原 298~341 段）——名<实参> → 实例化类符号（盒子$整32）。
void SemanticAnalyzer::resolveGenericCtorName(CallExpr* node, std::string& className) {
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
}

// 族B2：类构造调用检查（原 342~404 段）——构造函数匹配（精确优先）+ 参数检查 +
//   借用纪律/引用实参包装 + resolvedSignature 记录。true = 已处理。
bool SemanticAnalyzer::checkCtorCall(CallExpr* node, const std::string& className) {
        const ClassInfo* ctorCls = findClass(className);
        if (ctorCls != nullptr) {
            // 查找构造函数（函数名 == 类名）。Debug 子任务修复（构造函数重载）：
            //   methods 表构造条目 key=sigKey（名#参数串），遍历按 isConstructor +
            //   ownerClass（排除父类构造，阶段A-3）+ 实参个数 + 类型可转换 匹配最优。
            std::vector<std::string> argTypes =
                collectCallArgTypes(node->arguments, node->location);
            const ClassMemberInfo* ctor = nullptr;
            const ClassMemberInfo* ctorExact = nullptr;
            for (const auto& mk : ctorCls->methods) {
                const ClassMemberInfo& mi = mk.second;
                if (!mi.isConstructor || mi.ownerClass != className) continue;
                // D23 根治（248-a）：构造匹配按「实参个数 + 可补全」（与普通函数
                //   决议 defaultCount 同构）——原实现严格等个数，带默认参数的构造
                //   少参调用不匹配而静默落「无构造=默认构造」分支（初始化列表不跑，
                //   探针 乙():甲(n=9) 值=0）。
                const int ctorRequired =
                    static_cast<int>(mi.paramTypes.size()) - mi.defaultCount;
                if (static_cast<int>(argTypes.size()) < ctorRequired ||
                    argTypes.size() > mi.paramTypes.size()) continue;
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
                const int selRequired =
                    static_cast<int>(ctor->paramTypes.size()) - ctor->defaultCount;
                if (static_cast<int>(argTypes.size()) < selRequired ||
                    argTypes.size() > ctor->paramTypes.size()) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "构造函数 '" + className + "' 期望 " +
                                            std::to_string(ctor->paramTypes.size()) +
                                            " 个实参，实际提供 " +
                                            std::to_string(argTypes.size()) + " 个");
                } else {
                    for (std::size_t i = 0; i < argTypes.size(); ++i) {
                        if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], ctor->paramTypes[i]) &&
                            !canConvertArgNarrow(node->arguments[i].get(), argTypes[i], ctor->paramTypes[i])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "构造函数 '" + className + "' 第 " + std::to_string(i + 1) +
                                    " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    ctor->paramTypes[i] + "'");
                        }
                    }
                }
                // plans/019 阶段3b：构造调用面借用纪律（与普通函数面同构）
                checkConstRefBorrowDiscipline(node, ctor->paramTypes,
                                              ctor->constParams);
                // A-1（引用参数）：构造形参为引用时实参自动取地址
                wrapRefArgs(node, ctor->paramTypes);
                // 记录选中的构造 sigKey（IR 层按此生成构造体 Call 符号）
                node->resolvedSignature = className + "$" + ctor->sigKey;
                lastType_ = className;  // 构造返回对象
                return true;
            }
            // 121（928·v10 后续）：无匹配构造≠默认构造——类有自有构造但实参
            //   个数/类型无一可匹配，或类无构造却传了实参，均为编译期硬错误
            //   （原两形态与「类无构造」分支合并静默放行：NewObject 零初始化+
            //   实参整丢=字段垃圾值，t_N2b 探针 点("abc") x=4293200 实锤；
            //   对照普通函数调用面「无匹配重载」先例/g++ no matching function）。
            {
                const ClassMemberInfo* diag = nullptr;   // 诊断基准（同数优先）
                bool hasOwnCtor = false;
                for (const auto& mk : ctorCls->methods) {
                    const ClassMemberInfo& mi = mk.second;
                    if (!mi.isConstructor || mi.ownerClass != className) continue;
                    hasOwnCtor = true;
                    if (diag == nullptr ||
                        mi.paramTypes.size() == argTypes.size()) {
                        diag = &mk.second;
                        if (mi.paramTypes.size() == argTypes.size()) break;
                    }
                }
                if (hasOwnCtor || !node->arguments.empty()) {
                    if (hasOwnCtor && diag != nullptr) {
                        // 复用选中态同款诊断（个数/类型）——文案与上方选中分支一致
                        const int req =
                            static_cast<int>(diag->paramTypes.size()) - diag->defaultCount;
                        if (static_cast<int>(argTypes.size()) < req ||
                            argTypes.size() > diag->paramTypes.size()) {
                            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                                "构造函数 '" + className + "' 期望 " +
                                                    std::to_string(diag->paramTypes.size()) +
                                                    " 个实参，实际提供 " +
                                                    std::to_string(argTypes.size()) + " 个");
                        } else {
                            for (std::size_t i = 0; i < argTypes.size(); ++i) {
                                if (!canConvertWithLiteral(node->arguments[i].get(),
                                                           argTypes[i], diag->paramTypes[i]) &&
                                    !canConvertArgNarrow(node->arguments[i].get(),
                                                           argTypes[i], diag->paramTypes[i])) {
                                    diagnostics_.report(
                                        DiagnosticLevel::Error,
                                        node->arguments[i]->location,
                                        "构造函数 '" + className + "' 第 " +
                                            std::to_string(i + 1) + " 个实参无法将 '" +
                                            argTypes[i] + "' 隐式转换为 '" +
                                            diag->paramTypes[i] + "'");
                                }
                            }
                        }
                    } else {
                        diagnostics_.report(DiagnosticLevel::Error, node->location,
                                            "类 '" + className + "' 无构造函数，不接受实参（提供 " +
                                                std::to_string(argTypes.size()) + " 个）");
                    }
                }
            }
            // 类真无构造+零实参：默认构造放行（117 语义：零初始化+基类链注入）
            lastType_ = className;
            return true;
        }
    return false;
}

}
