// 480：函数内静态局部变量写安全区判定实现（设计契约见 semantic_staticguard.hpp）。
#include "cn_compiler/semantic/semantic_staticguard.hpp"

#include <unordered_set>

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

} // namespace staticguard
} // namespace cn_compiler
