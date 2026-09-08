
#include "runtime/runtime.hpp"

// ==================== 字符串驻留表（自举重建 P1，2026-08-25） ====================
// 对标 rustc Symbol / LLVM StringPool：
//   符号名/类型名/标识符 经驻留 -> 唯一整型 ID（Symbol）。相同内容返回同一 ID，
//   字符串内容只存一份（字节池）。Token/AST/IR/符号表 全部用 ID 引用——
//   比较/哈希 O(1)，消除百万级重复字符串分配（字符串行架构的病根）。
// 生命周期：编译器进程内常驻（随进程退出回收），不参与 内存::释放全部()（reset），
//   避免 reset 破坏符号表引用。

#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
// 内容 -> ID（经典哈希 + 游动指针定位字节池）
struct InternState {
    std::unordered_map<std::string, int> map;
    // 宿主缺陷根治（2026-09-01，用户令缺陷零容忍·体内二分定位）：pool 原为
    //   vector<string>——扩容搬移全部 string，已发放的 c_str() 指针悬垂
    //   （__cn_intern_text 返回的指针被编译器全链长期持有：v2 组件 驻留文本
    //   取回后跨多轮调用使用，任何后续 驻留 触发扩容即读到搬家后旧内存——
    //   表现为「字符串参数跨调用被随机清空/混入无关文本」的 UB，破坏位置
    //   随扩容时机漂移）。deque 的 push_back 不使已有元素指针失效（标准保证）
    //   ——对标 rustc StringPool 的指针稳定性要求。
    std::deque<std::string> pool;  // pool[id] = 文本（ID 从 1 开始，0 保留空串）
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
    // 驻留轨迹（灰色点取证专用，2026-09-08）：CN_INTERN_TRACE=1 时逐条打印
    //   新驻留（重复驻留不打印——池不变）。仅新驻留分支执行 getenv 判定
    //   （static 缓存零重复开销），默认关闭=零输出零开销。取证用法：
    //   v2p 与 cn_self 各跑同一入口，diff 两条轨迹——首个分叉条目即
    //   「一侧多驻留」的字符串内容与时机（组件对拍编号漂移根因定位）。
    {
        static int traceOn = -1;     // -1=未判定；1=开；0=关
        if (traceOn < 0) {
            traceOn = getenv("CN_INTERN_TRACE") != nullptr ? 1 : 0;
        }
        if (traceOn == 1) {
            // ra=调用者返回地址（nm 排序表二分可反解 CN 函数符号名——两二进制
            //   各自解析后按函数名对比，定位「首次驻留」的漂移调用点）
            std::fprintf(stderr, "[intern] %d %s ra=%p\n", id, text,
                         __builtin_return_address(0));
        }
    }
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
