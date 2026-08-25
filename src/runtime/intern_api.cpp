
#include "runtime/runtime.hpp"

// ==================== 字符串驻留表（自举重建 P1，2026-08-25） ====================
// 对标 rustc Symbol / LLVM StringPool：
//   符号名/类型名/标识符 经驻留 -> 唯一整型 ID（Symbol）。相同内容返回同一 ID，
//   字符串内容只存一份（字节池）。Token/AST/IR/符号表 全部用 ID 引用——
//   比较/哈希 O(1)，消除百万级重复字符串分配（字符串行架构的病根）。
// 生命周期：编译器进程内常驻（随进程退出回收），不参与 内存::释放全部()（reset），
//   避免 reset 破坏符号表引用。

#include <string>
#include <unordered_map>
#include <vector>

namespace {
// 内容 -> ID（经典哈希 + 游动指针定位字节池）
struct InternState {
    std::unordered_map<std::string, int> map;
    std::vector<std::string> pool;  // pool[id] = 文本（ID 从 1 开始，0 保留空串）
};
InternState& internState() {
    static InternState s;
    if (s.pool.empty()) s.pool.push_back(std::string());  // ID 0 = 空串
    return s;
}
}  // namespace

// 驻留：返回字符串的稳定 Symbol ID（相同内容同 ID；内容量自有）
extern "C" long long __cn_intern(const char* text) {
    if (text == nullptr) text = "";
    auto& s = internState();
    const std::string key(text);
    const auto it = s.map.find(key);
    if (it != s.map.end()) {
        return static_cast<long long>(it->second);
    }
    const int id = static_cast<int>(s.pool.size());
    s.pool.push_back(key);           // 拷贝存档（内容只存一份）
    s.map.emplace(key, id);
    return static_cast<long long>(id);
}

// 反查：由 ID 取回文本（调试/错误消息用；越界/0 返回空串）
extern "C" const char* __cn_intern_text(long long id) {
    auto& s = internState();
    if (id <= 0 || static_cast<std::size_t>(id) >= s.pool.size()) return "";
    return s.pool[static_cast<std::size_t>(id)].c_str();
}

// 驻留表大小（诊断用）
extern "C" long long __cn_intern_count() {
    return static_cast<long long>(internState().pool.size() - 1);
}
