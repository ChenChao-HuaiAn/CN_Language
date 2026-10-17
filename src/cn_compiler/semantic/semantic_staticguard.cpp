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

// 916（791 派生·P1 安全面）：容器写方法名白名单实现（契约见头文件）。
//   清单=stdlib/容器.cn+映射集合.cn 写方法逐一核对（归属行注）：
//     向量<T>：追加/插入/删除/设置/清空/预留/析构元素/析构被移除/移动元素
//     链表<T>：追加/头部追加/删除头部/删除尾部/清空/取空闲槽/归还槽
//     栈<T>：压入/弹出          队列<T>：入队/出队/清空/取空闲槽/归还槽
//     映射/字符串映射：设置/删除/清空/释放内部数组/析构键值/析构值
//     集合<T>：添加/删除/清空/析构被移除/移动元素/数组前移
bool isContainerWriteMethod(const std::string& name) {
    static const std::unordered_set<std::string> kContainerWriteMethods = {
        "追加", "插入", "删除", "设置", "清空", "预留",
        "析构元素", "析构被移除", "移动元素",
        "头部追加", "删除头部", "删除尾部", "取空闲槽", "归还槽",
        "压入", "弹出", "入队", "出队",
        "释放内部数组", "析构键值", "析构值",
        "添加", "数组前移",
    };
    return kContainerWriteMethods.count(name) > 0;
}

// 916：方法调用形态判定辅助实现（契约见头文件）——判定链=写白名单 ×
//   接收者链根名解析（writeTargetRootName 复用·791 同源）；根非标识符
//   （调用/解引用/字面量等根）返回空串=不判定（合法不判定面声明同 791：
//   集合键=标识符名不可能命中非标识符根，写责任留既有条款），非静默
//   默认。
std::string containerWriteCallReceiverRoot(const Expr* receiver,
                                           const std::string& methodName) {
    if (!isContainerWriteMethod(methodName)) return "";
    return writeTargetRootName(receiver);
}

} // namespace staticguard
} // namespace cn_compiler
