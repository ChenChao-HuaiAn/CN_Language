// 480：函数内静态局部变量写安全区判定实现（设计契约见 semantic_staticguard.hpp）。
#include "cn_compiler/semantic/semantic_staticguard.hpp"

#include <unordered_set>

#include "cn_compiler/model/ast.hpp"  // 791：根对象链解析（IndexExpr/MemberExpr）

namespace cn_compiler {
namespace staticguard {
namespace {

// 当前函数内静态局部变量名集（声明位登记·函数入口清空·跨函数互不污染）。
// 扁平名集 + 遮蔽移除（865-a 双侧同构的保守边界）——名字遮蔽后不恢复，
// 与 v2 解析层「原名→原名」永久改写同语义（零分叉）。
std::unordered_set<std::string> g_staticLocals;

}  // namespace

void noteFunctionEntry() {
    g_staticLocals.clear();
}

void recordStaticLocal(const std::string& name) {
    g_staticLocals.insert(name);
}

void notePlainDecl(const std::string& name) {
    g_staticLocals.erase(name);
}

bool isStaticLocal(const std::string& name) {
    return g_staticLocals.count(name) > 0;
}

// 791（001 §747-8 执法扩面）：写目标根对象名解析实现（契约见头文件）。
std::string writeTargetRootName(const Expr* target) {
    const Expr* cur = target;
    // 链下降上限 256：AST 由解析器构造为无环树（同一节点不在自身 object 链
    //   上重复），上限语义上不可达——仅防构造期异常下的死循环（显式哨兵+
    //   消费点检查：达限即按「根非标识符」返回空串，不判定）。合法不判定面
    //   声明：根非标识符（解引用/调用/字面量等链根）不可能命中「全局静态名
    //   集合」（集合键=标识符名）——空串=明确否定结论，写责任留既有条款
    //   （解引用写/引用返回调用左值族），非静默默认。
    for (int depth = 0; cur != nullptr && depth < 256; ++depth) {
        switch (cur->getType()) {
            case NodeType::IndexExpr:
                cur = static_cast<const IndexExpr*>(cur)->object.get();
                break;
            case NodeType::MemberExpr:
                cur = static_cast<const MemberExpr*>(cur)->object.get();
                break;
            case NodeType::IdentifierExpr:
                return static_cast<const IdentifierExpr*>(cur)->name;
            default:
                return "";
        }
    }
    return "";
}

} // namespace staticguard
} // namespace cn_compiler
