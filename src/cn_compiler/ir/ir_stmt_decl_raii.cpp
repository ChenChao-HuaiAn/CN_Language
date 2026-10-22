// CN-IR生成器实现：变量声明 RAII 登记族——调用接收豁免判据
// 职责（929 自 ir_stmt_decl.cpp 拆出·先拆再加：原文件 1017 行超 1000 硬红线，
//   本函数族为 880/879 两轮新增的判据主体）：
//   callReceiverExempt880 —— registerOopRaii 的「初值=用户函数/方法调用接收」
//   返回来源三分判据（880 自由函数面 + 879 方法面扩展；声明见 ir_generator.hpp）。
// 搬移口径：原内联段逐字搬运零语义变更，唯一差异=入参 (node, ctx) 显式传递。
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_stmt_decl_ctx.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 980 红治理 v2（2026-10-16·880·第三轮判据收窄）：**初值=用户函数调用接收
//   → 按被调返回来源三分**——引用写穿立法（plans/001 §5.3·056 甲）下：
//   · 返回「形参/全局」=写穿别名（接收槽非拥有者）→ 豁免登记（980：
//     幼=喂(犬)·宠≡犬，两槽双删 C0000374）；
//   · 返回「局部/函数内新建」=句柄转移（接收槽=唯一拥有者）→ 正常登记
//     回收（836：甲=造货架() 块出口在册回收残留 0）；
//   · 引用返回=调用点专用深拷路径已物化副本→登记（878）；
//   · 内置「复制/转移」=克隆/显式交接产物→登记（284/719）；构造调用
//     （含泛型实例文本 名<...>(...)）→登记（770）；容器/合成体→登记。
//   AST 不可达/return 非标识符形态=保守豁免（宁漏勿崩·v2 普适口径）。
//   函数声明表 freeFuncDecls880_ 由 visitProgram 预收集（含后置定义）。
// 879 方法面扩展（980 红治理残余·探针 M11/M12 实录）：成员方法调用 甲.取(乙)
//   与自由函数同款三分——原判据只认 IdentifierExpr callee，方法调用回落
//   「接收槽一律登记」：被调返回形参（写穿别名）时接收槽与实参槽双删
//   C0000374。对象类型=语义层写回 semanticType（指针剥一层·v2.1 统一 . 模型）。
//   返回 true=接收槽豁免登记（写穿别名/保守面）；false=登记回收（拥有/转移）。
bool IRGenerator::callReceiverExempt880(VarDecl* node, const DeclGenCtx& ctx) {
    bool callTransferInit = false;
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::CallExpr) {
        auto* initCall = static_cast<CallExpr*>(node->initializer.get());
        // 880/879 共用判据（879 方法面扩展后两处消费·提炼于此）：
        //   体内首个 return 表达式搜索（块/如果 基本形态递归）。
        std::function<Expr*(Stmt*)> findReturnExpr880 = [&](Stmt* s) -> Expr* {
            if (s == nullptr) return nullptr;   // 空节点=非该形态（递归出口）
            switch (s->getType()) {
            case NodeType::ReturnStmt:
                return static_cast<ReturnStmt*>(s)->value.get();
            case NodeType::BlockStmt:
                for (auto& st : static_cast<BlockStmt*>(s)->statements) {
                    if (Expr* e = findReturnExpr880(st.get())) return e;
                }
                return nullptr;
            case NodeType::IfStmt: {
                auto* ifs = static_cast<IfStmt*>(s);
                if (Expr* e = findReturnExpr880(ifs->thenBranch.get())) return e;
                return findReturnExpr880(ifs->elseBranch.get());
            }
            default:
                return nullptr;
            }
        };
        // 返回来源三分（880 自由函数/879 方法面共用）：
        //   1=借用豁免（形参/全局标识符=写穿别名；非标识符 return=保守豁免）；
        //   0=登记（局部/新建标识符=句柄转移）。
        auto returnSourceExempt880 =
            [&](BlockStmt* body,
                const std::vector<std::unique_ptr<ParamDecl>>& params) -> bool {
            if (body == nullptr) return true;   // 函数体缺省=无 return 可查（缺省形态豁免登记·两调用点非空守卫）
            Expr* rv = findReturnExpr880(body);
            if (rv != nullptr && rv->getType() == NodeType::IdentifierExpr) {
                const std::string rn = static_cast<IdentifierExpr*>(rv)->name;
                for (auto& p : params) {
                    if (p != nullptr && p->name == rn) return true;   // 形参
                }
                if (semantic_ != nullptr) {
                    if (semantic_->isGlobalStatic(rn) ||
                        !semantic_->globalStaticType(rn).empty()) {
                        return true;   // 全局
                    }
                }
                return false;           // 局部/新建标识符=转移→登记
            }
            return true;                // 非标识符 return：保守豁免
        };
        std::string calleeName;
        bool isMemberCall880 = false;
        if (initCall->callee != nullptr &&
            initCall->callee->getType() == NodeType::IdentifierExpr) {
            calleeName = static_cast<IdentifierExpr*>(initCall->callee.get())->name;
        } else if (initCall->callee != nullptr &&
                   initCall->callee->getType() == NodeType::MemberExpr) {
            isMemberCall880 = true;
        }
        // 泛型实例文本归一：名<实参…> → 基名（770 字符串映射<整32> 构造识别）。
        auto peelTemplateSuffix = [](std::string s) {
            const std::size_t lt = s.find('<');
            return lt == std::string::npos ? s : s.substr(0, lt);
        };
        const std::string calleeBase = peelTemplateSuffix(calleeName);
        std::string baseType = ctx.srcType;
        const std::size_t dl = baseType.find('$');
        if (dl != std::string::npos) {
            baseType = baseType.substr(0, dl);
        }
        const std::string typeBase = peelTemplateSuffix(baseType);
        const bool builtinAggregate =
            typeBase == "向量" || typeBase == "链表" || typeBase == "栈" ||
            typeBase == "队列" || typeBase == "集合" || typeBase == "映射" ||
            typeBase == "字符串映射" || typeBase == "结果" || typeBase == "可选";
        const bool cloneOrMoveCall = calleeBase == "复制" || calleeBase == "转移";
        if (!calleeName.empty() && !cloneOrMoveCall && calleeBase != typeBase &&
            !builtinAggregate && semantic_ != nullptr && !ctx.srcType.empty() &&
            semantic_->isClassType(types::canonical(ctx.srcType))) {
            auto it = freeFuncDecls880_.find(calleeName);
            const bool astOk880 = (it != freeFuncDecls880_.end() &&
                                   it->second != nullptr &&
                                   it->second->body != nullptr);
            if (!astOk880) {
                callTransferInit = true;           // AST 不可达：保守豁免
            }
            if (astOk880) {
                FunctionDecl* fn = it->second;
                if (fn->returnType.find('&') == std::string::npos) {
                    callTransferInit = returnSourceExempt880(fn->body.get(),
                                                             fn->params);
                }
                // 引用返回：调用点已走专用深拷路径物化副本→登记（不置位）。
            }
        } else if (isMemberCall880) {
            auto* mem = static_cast<MemberExpr*>(initCall->callee.get());
            std::string objCanon = types::canonical(mem->object->semanticType);
            if (semantic_ != nullptr && !objCanon.empty() &&
                !semantic_->isClassType(objCanon) && types::isPointer(objCanon)) {
                objCanon = types::canonical(types::pointeeOf(objCanon));
            }
            // 879 回归根治（877-D2 实录）：**接收者=容器族排除**——容器方法
            //   （元素/获取等）返回用户类元素=副本语义（plans/001 §052 甲快照），
            //   接收槽=副本唯一拥有者须登记释放（宿主编译器 630「类类型接收不再
            //   豁免析构」既有模型）；本判据只辖**用户类方法**（v2 是返回类调用
            //   _544 的「是容器类(接文本)排除」同口径）。head 判据与
            //   ir_stmt_decl_init.cpp isContainerFamily 同款（先例：633 容器面）。
            const std::size_t dlObj = objCanon.find('$');
            const std::string objHead = (dlObj == std::string::npos)
                                            ? objCanon
                                            : objCanon.substr(0, dlObj);
            const bool objIsContainer880 =
                objHead == "向量" || objHead == "栈" || objHead == "链表" ||
                objHead == "队列" || objHead == "集合" || objHead == "映射";
            if (semantic_ != nullptr && !objCanon.empty() && !objIsContainer880 &&
                semantic_->isClassType(objCanon) && !ctx.srcType.empty() &&
                semantic_->isClassType(types::canonical(ctx.srcType))) {
                const bool cloneOrMoveCallM =
                    mem->memberName == "复制" || mem->memberName == "转移";
                if (!cloneOrMoveCallM) {
                    std::string owner;
                    // 1360 结果化：无值=非方法（has_value 分流）
                    const cn::Optional<const ClassMemberInfo*> mOpt =
                        findClassMethod(objCanon, mem->memberName, owner);
                    if (mOpt.has_value() && !mOpt.value()->isStatic) {
                        const ClassMemberInfo* m = mOpt.value();
                        if (!m->isRefReturn) {
                            BlockStmt* mBody880 = (m->ast != nullptr)
                                                      ? m->ast->body.get()
                                                      : nullptr;
                            if (mBody880 == nullptr) {
                                callTransferInit = true;   // AST 不可达：保守豁免
                            }
                            if (mBody880 != nullptr) {
                                callTransferInit = returnSourceExempt880(
                                    mBody880, m->ast->params);
                            }
                        }
                    }
                }
            }
        }
    }
    return callTransferInit;
}

}  // namespace cn_compiler
