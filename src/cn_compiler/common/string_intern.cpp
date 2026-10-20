// 全局字符串驻留表实现（Task #1154 阶段A）
// 结构：std::deque<std::string> 存正文（尾部追加，既有元素引用稳定）；
//       std::unordered_map<std::string_view, InternId> 键视图指向 deque 内文本。
// 为什么用 deque 而非 vector：vector 扩容会搬移元素，ID→文本的稳定引用承诺
//   即被破坏；deque 尾部插入不使既有元素引用失效。
// 为什么表对象用进程级不析构单例：驻留表可能被静态对象/并行 codegen 线程在
//   程序退出阶段访问，函数内 static 的析构会引入静态销毁顺序风险；进程退出由
//   操作系统回收（有意不 delete，注释明示）。
#include "cn_compiler/common/string_intern.hpp"

#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace cn_compiler {
namespace intern {

namespace {

// 驻留表：正文 + 内容索引 + 互斥量一体（同一把锁保护两者的一致性）
struct InternTable {
    std::deque<std::string> texts;
    std::unordered_map<std::string_view, InternId> index;
    std::mutex mutex;

    InternTable() {
        // ID 0 = 空串（固定）：先入正文再建视图，视图指向 deque 内文本
        texts.emplace_back();
        index.emplace(std::string_view(texts.back()), kEmptyId);
    }
};

InternTable& table() {
    // 进程级单例（有意不析构：见文件头注释）
    static InternTable* instance = new InternTable();
    return *instance;
}

} // namespace

InternId intern(std::string_view text) {
    if (text.empty()) return kEmptyId;  // 空串固定 0，免加锁
    InternTable& t = table();
    std::lock_guard<std::mutex> lock(t.mutex);
    const auto it = t.index.find(text);
    if (it != t.index.end()) return it->second;
    // 新内容：追加正文（deque 尾部插入，既有引用稳定）再以视图登记
    const InternId id = static_cast<InternId>(t.texts.size());
    t.texts.emplace_back(text);
    t.index.emplace(std::string_view(t.texts.back()), id);
    return id;
}

const std::string& internRef(std::string_view text) {
    InternTable& t = table();
    std::lock_guard<std::mutex> lock(t.mutex);
    if (!text.empty()) {
        const auto it = t.index.find(text);
        if (it != t.index.end()) return t.texts[it->second];
    }
    // 空串恒命中 ID 0；非空新内容追加（deque 尾部插入，既有引用稳定）
    if (text.empty()) return t.texts[kEmptyId];
    const InternId id = static_cast<InternId>(t.texts.size());
    t.texts.emplace_back(text);
    t.index.emplace(std::string_view(t.texts.back()), id);
    return t.texts.back();
}

InternId find(std::string_view text) {
    if (text.empty()) return kEmptyId;  // 空串恒命中固定 ID
    InternTable& t = table();
    std::lock_guard<std::mutex> lock(t.mutex);
    const auto it = t.index.find(text);
    return it == t.index.end() ? kInvalidId : it->second;
}

const std::string& text(InternId id) {
    InternTable& t = table();
    std::lock_guard<std::mutex> lock(t.mutex);
    // 契约破坏（非法 ID）=编译器内部错误：显式中止，禁静默返回空串冒充成功。
    //   id 只在 intern 返回后使用，越界说明消费方把 kInvalidId/野值当 ID 用。
    if (id >= t.texts.size()) {
        std::fprintf(stderr,
                     "[string_intern] 内部错误: 非法驻留 ID %u（已登记 %zu 条）\n",
                     static_cast<unsigned>(id), t.texts.size());
        std::abort();
    }
    return t.texts[id];
}

std::size_t size() {
    InternTable& t = table();
    std::lock_guard<std::mutex> lock(t.mutex);
    return t.texts.size();
}

} // namespace intern
} // namespace cn_compiler
