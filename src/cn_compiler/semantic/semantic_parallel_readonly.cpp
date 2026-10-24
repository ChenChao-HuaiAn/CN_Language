// 1501（2026-10-23·P1 安全）：并行 codegen 语义只读冻结。
// 背景：1145 函数级并行发射后多个 worker 共享同一 SemanticAnalyzer 视图；
//   findClass 为 const 却在裸名未命中时写 mutable classCache——多线程对同一
//   unordered_map 并发 find/insert = 数据竞争（UB）。IR 阶段通常已预热使
//   现有语料未爆，但「通常」不是证明；本文件把只读契约机械化：
//   ① spawn 前主线程预热全部类基名（codegen 的裸名查询面）到 classCache；
//   ② 封存 readonlySealed——worker 侧 findClass 零写（见 class_resolver.cpp）；
//   ③ join 后主线程取回未预热 miss（若有）=内部错误中止（miss 不可静默）。
// 设计：安全第一、性能第二——正常路径零锁（仅一次 bool 读）；互斥只在异常
//   路径（未预热查询）使用。函数 <=100 行；中文注释；英文 API 命名。
#include <cstddef>
#include <mutex>
#include <string>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// 并行 codegen 只读冻结入口（主线程调用，先于 worker 线程创建）。
//   预热键=全部类键的最后一段（codegen 裸名查询必然落在类基名上；限定名
//   "模块::类" 走 classByFullKey 不经 classCache，无需预热）。以「当前模块
//   上下文」（codegen 期该值恒定）解析，与 worker 查询的缓存命中条件一致。
//   prewarmedCount 回填缓存条目数（观测/门禁用，非决策依据）。
bool SemanticAnalyzer::beginReadonlyCodegenLookup(std::size_t& prewarmedCount,
                                                  std::string& errorOut) {
    TypeLookupIndex& idx = const_cast<TypeLookupIndex&>(ensureTypeLookupIndex());
    if (idx.readonlySealed) {
        // 重入=内部契约破坏（begin 未配对 end）：显式拒绝，绝不按成功继续
        errorOut = "内部错误: 并行 codegen 语义只读冻结重复进入（begin/end 未配对）";
        return false;
    }
    // 预热：单线程主线程写 classCache（未封存态=普通写路径，无并发）
    idx.classCache.reserve(idx.classCache.size() + classes_.size());
    for (const auto& kv : classes_) {
        const std::size_t sep = kv.first.rfind("::");
        const std::string base =
            (sep == std::string::npos) ? kv.first : kv.first.substr(sep + 2);
        static_cast<void>(findClass(base));  // 结果（含负缓存）由当前模块上下文决定
    }
    idx.readonlySealed = true;  // 线程创建前的写：worker 侧经 happens-before 可见
    prewarmedCount = idx.classCache.size();
    return true;
}

// 解除只读冻结（主线程 join 后调用）。返回有值=冻结期首个未预热查询名
//   （调用方必须报内部错误并中止编译）；无值=全部命中预热缓存。
//   计数/首名在互斥下读取并清零（此后缓存写路径恢复，供串行回退等场景）。
cn::Optional<std::string> SemanticAnalyzer::endReadonlyCodegenLookup() {
    TypeLookupIndex& idx = typeLookupIndex_;
    idx.readonlySealed = false;  // join 后的写：worker 已全部退出
    std::lock_guard<std::mutex> guard(idx.readonlyMissMutex);
    const std::size_t misses = idx.readonlyMissCount;
    std::string first = idx.firstReadonlyMissName;
    idx.readonlyMissCount = 0;
    idx.firstReadonlyMissName.clear();
    if (misses == 0) return std::nullopt;
    if (first.empty()) first = "(未记录名字)";
    return cn::Optional<std::string>(std::move(first));
}

}  // namespace cn_compiler
