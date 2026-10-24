// 1499（2026-10-23·P1 安全）：并行 codegen 语义只读冻结。
// 背景：1145 函数级并行发射后多个 worker 共享同一 SemanticAnalyzer 视图；
//   findClass 为 const 却在裸名未命中时写 mutable classCache——多线程对同一
//   unordered_map 并发 find/insert = 数据竞争（UB）。IR 阶段通常已预热使
//   现有语料未爆，但「通常」不是证明；本文件把只读契约机械化：
//   ① spawn 前主线程预热全部类符号名（ClassInfo.name——IR/codegen 查询面）
//      到 classCache；
//   ② 封存 readonlySealed（atomic acquire/release）——worker 侧 findClass 零写；
//   ③ join 后主线程取回未预热 miss（若有）=内部错误中止（miss 不可静默）。
// 设计：安全第一、性能第二——正常路径零锁（仅一次 acquire 读）；互斥只在异常
//   路径（未预热查询）使用。函数 <=100 行；中文注释；英文 API 命名。
#include <cstddef>
#include <mutex>
#include <string>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// 类查询缓存兜底（findClass 尾部单点）：封存期只读+miss 登记，非封存期写缓存。
//   把写面/只读分流集中于此，class_resolver.cpp 净增回落到警戒线下（1499 审查 P2-6）。
cn::Optional<const ClassInfo*> SemanticAnalyzer::cacheOrRecordClassLookup(
    const TypeLookupIndex& idxConst, const std::string& name,
    const ClassInfo* result) const {
    TypeLookupIndex& idx = const_cast<TypeLookupIndex&>(idxConst);
    // 只读冻结（并行 codegen worker）——写缓存=共享 unordered_map 并发 find/insert
    //   （数据竞争 UB·TSan 实证）；封存期只读索引映射照常解析，未命中仅登记
    //   （主线程 join 后统一报内部错误），绝不静默写。
    if (idx.readonlySealed.load(std::memory_order_acquire)) {
        idx.recordReadonlyMiss(name);
        if (result != nullptr) return cn::Optional<const ClassInfo*>(result);
        return std::nullopt;
    }
    idx.classCache[name] = TypeLookupIndex::ClassLookupCacheEntry{currentModuleName_, result};
    if (result != nullptr) return cn::Optional<const ClassInfo*>(result);
    return std::nullopt;
}

// 类型索引可复用判定（ensureTypeLookupIndex 头部单点）：
//   - 程序/三表尺寸全部一致=索引与表对齐，直接复用；
//   - 封存期尺寸变化=只读契约破坏（并行期类表不应再变）：登记 miss、复用旧只读
//     索引（绝不重建/写——并发写=数据竞争），主线程 join 后统一中止编译。
bool SemanticAnalyzer::typeLookupIndexReusable(const Program* prog,
                                               std::size_t structsSize,
                                               std::size_t enumsSize,
                                               std::size_t classesSize) const {
    const TypeLookupIndex& idx = typeLookupIndex_;
    if (idx.builtProgram == prog && idx.structsSize == structsSize &&
        idx.enumsSize == enumsSize && idx.classesSize == classesSize) {
        return true;
    }
    if (idx.readonlySealed.load(std::memory_order_acquire)) {
        // 封存期尺寸变化=只读契约破坏：登记 miss（主线程 join 后中止），复用旧索引
        idx.recordReadonlyMiss("<类表在只读冻结期发生变化>");
        return true;
    }
    return false;  // 未封存：走正常重建路径
}

// 并行 codegen 只读冻结入口（主线程调用，先于 worker 线程创建）。
//   预热键=ClassInfo.name（IR/codegen 实际查询的类符号名：无冲突=裸名，跨模块
//   同名冲突=限定全键）。按符号名预热天然与 findClass 的限定名判定对齐：
//   - 含 "::" 的符号名走 classByFullKey 直查（不经 classCache），预热调用不产生
//     垃圾键（审查 P2-7：旧实现按 class 键 rfind("::") 提取基名，模板实参含
//     模块限定名如 容器$映射<甲::乙,整32> 会造出 "乙,整32>" 垃圾键）；
//   - 裸名（含泛型实例名 向量$整32）走缓存，正是 worker 首查面。
//   白名单：worker 查询面=仅已注册类符号名；合法负探测（查不存在的类）不在
//   白名单——封存期会判内部错误中止（如需支持须显式设计只读负缓存通道）。
//   prewarmedCount=封存后 classCache.size()（含语义/IR 阶段遗留条目与负缓存，
//   不是「本轮新增预热数」；门禁只断言 >0=预热通道真实执行）。
bool SemanticAnalyzer::beginReadonlyCodegenLookup(std::size_t& prewarmedCount,
                                                  std::string& errorOut) {
    TypeLookupIndex& idx = const_cast<TypeLookupIndex&>(ensureTypeLookupIndex());
    if (idx.readonlySealed.load(std::memory_order_acquire)) {
        // 重入=内部契约破坏（begin 未配对 end）：显式拒绝，绝不按成功继续
        errorOut = "内部错误: 并行 codegen 语义只读冻结重复进入（begin/end 未配对）";
        return false;
    }
    // 显式清空上次 miss 状态（异常/中断后重入时不得带入陈旧记录）
    {
        std::lock_guard<std::mutex> guard(idx.readonlyMissMutex);
        idx.readonlyMissCount = 0;
        idx.firstReadonlyMissName.clear();
    }
    // 预热：单线程主线程写 classCache（未封存态=普通写路径，无并发）
    idx.classCache.reserve(idx.classCache.size() + classes_.size());
    for (const auto& kv : classes_) {
        const std::string& symbol =
            kv.second.name.empty() ? kv.first : kv.second.name;
        static_cast<void>(findClass(symbol));  // 结果（含负缓存）由当前模块上下文决定
    }
    idx.readonlySealed.store(true, std::memory_order_release);
    prewarmedCount = idx.classCache.size();
    return true;
}

// 解除只读冻结（主线程 join 后调用）。返回有值=冻结期首个未预热查询名
//   （调用方必须报内部错误并中止编译）；无值=全部命中预热缓存。
//   计数/首名在互斥下读取并清零（此后缓存写路径恢复，供串行回退等场景）。
cn::Optional<std::string> SemanticAnalyzer::endReadonlyCodegenLookup() {
    TypeLookupIndex& idx = typeLookupIndex_;
    idx.readonlySealed.store(false, std::memory_order_release);
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
