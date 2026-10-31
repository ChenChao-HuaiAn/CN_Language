// 语义·借用/返回辅助族（#1769 轮纯机械搬移 自 semantic_decl.cpp：函数体逐字
//   保留·零行为变更。先拆再加纪律——该文件改动前 914 行超 900 警戒线；本文件
//   承接四个借用/返回辅助函数为多维初始化列表新逻辑腾位）。
//   职责：引用返回 lvalue 基名解剖（refReturnLvalueBase）/引用形参判定/
//   当前函数局部判定/绑定方法值实参判定/类对象返回拥有形态收集（662）。
#include <string>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

bool SemanticAnalyzer::refReturnLvalueBase(const Expr* e, std::string& baseName) {
    if (e == nullptr) return false;  // 静默豁免业务值: 空表达式=lvalue 解剖的合法否定回答（否定≠错误·调用方分流）
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
            // v2.1：经指针访问（p.字段 ≡ (*p).字段，语义层 isDerefAccess 标记
            //   ——返回检查前 checkExpr 已按对象类型置位）= 指针所指（*p 同类），
            //   可作返回（指向堆/调用方存储）；值对象递归解到对象名。
            const MemberExpr* m = static_cast<const MemberExpr*>(e);
            if (m->isDerefAccess) return true;
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
    // 静默豁免业务值: 顶层（无函数语境哨兵 -1）名字=非当前函数局部为合法否定回答
    if (funcScopeStart_ == -1) return false;  // 哨兵等值（域={-1}∪索引）
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

// 943（2026-10-17·用户质询收口）位置守卫与统一诊断文案已搬移——031（#1075）
//   起实现归位 semantic_method_value_943.cpp（本文件触 1000 行硬线「先拆再加」），
//   并在该文件扩充 argDepth（实参位开放）与 fnPtrConvertDiag（方法值→fnptr
//   转换拒绝专用文案）。声明见 semantic_method_value_943.hpp。

// 662（方法链消费位·2026-10-16）：类对象返回「拥有形态」收集——返回类型为类
//   对象（非指针/引用）时判定返回值表达式形态（函数级两标记，收尾落
//   FunctionInfo::returnsOwnedClass662）：
//     拥有形态：本地标识符（返回局部=移交·IR 返回块移出豁免语义）/ 类构造调用
//       （新建）/ 转移(...)（显式交接·成员源成员槽清零）——不置 borrow；
//     借用形态（置 borrow=函数级不标记·保守放弃析构）：成员/下标/解引用借出、
//       参数标识符、全局/静态标识符、其余调用（跨函数透传所有权不可静态判定）
//       及三元等其余形态。
//   零语义变更：仅收集标记不报错——借用返回行为=现状（调用方不析构·保持既有
//   边界）；拥有返回在 IR 调用点补析构（方法接收链/字段链/语句位丢弃）。
// 命名豁免规范编号: 662 立法名全族存量（声明在 semantic.hpp·多消费点不在本轮战场）
void SemanticAnalyzer::checkReturnOwnedClassCollect(ReturnStmt* node) {
    if (node->value == nullptr) return;
    if (!isClassType(types::canonical(currentReturnType_))) return;
    if (!genericTypeParams_.empty()) return;   // 泛型单态化体内保守豁免（不标记）
    curFnClassRetSeen_ = true;
    const NodeType kind = node->value->getType();
    if (kind == NodeType::IndexExpr || kind == NodeType::MemberExpr) {
        curFnClassRetBorrow_ = true;
        return;
    }
    if (kind == NodeType::UnaryExpr &&
        static_cast<const UnaryExpr*>(node->value.get())->op == Operator::Deref) {
        curFnClassRetBorrow_ = true;
        return;
    }
    if (kind == NodeType::IdentifierExpr) {
        const std::string& rn =
            static_cast<const IdentifierExpr*>(node->value.get())->name;
        if (currentFnParamNames_.count(rn) != 0) {   // 参数=借用（集合存在性谓词·等值化）
            curFnClassRetBorrow_ = true;
            return;
        }
        if (!isCurrentFnLocal(rn)) {   // 全局/静态/类字段（裸名不在局部作用域）=借用
            curFnClassRetBorrow_ = true;
            return;
        }
        return;   // 本地标识符=移出（拥有）
    }
    if (kind == NodeType::CallExpr) {
        const CallExpr* ce = static_cast<const CallExpr*>(node->value.get());
        if (ce->callee != nullptr &&
            ce->callee->getType() == NodeType::IdentifierExpr) {
            const std::string calleeName =
                static_cast<const IdentifierExpr*>(ce->callee.get())->name;
            if (isClassType(calleeName)) return;   // 类构造调用=新建（拥有）
        }
        // 其余调用=透传（跨函数所有权不可静态判定·保守不标记）；转移(源) 已在
        //   AST 改写为源标识符（走上方标识符分支），此处不再特判（双侧零分叉）。
        curFnClassRetBorrow_ = true;
        return;
    }
    curFnClassRetBorrow_ = true;   // 三元等其余形态保守不标记
}

} // namespace cn_compiler
