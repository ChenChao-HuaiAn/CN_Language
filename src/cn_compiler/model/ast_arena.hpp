// AST 节点 arena（任务 1482）：解析/语义/驱动/反序列化各期构造的 AstNode
// 全部经 AstNode::operator new 落入本 arena（分段 bump 分配），节点自身内存
// 不逐个归还、随进程统一回收——原每轮约 30 万次 make_unique 堆分配收拢为
// 按段批量分配（分配次数降约 3 个数量级），且节点内存紧凑相邻、遍历局部性
// 显著改善。
//
// 设计要点（1482 方案 C·三选型中最小侵入面）：
//   A. arena+裸指针：须改全部 unique_ptr 字段与签名（>100 文件），否决；
//   B. unique_ptr+假删除器：unique_ptr<T,D> 与 unique_ptr<T> 类型不兼容，
//      改造面同量级，否决；
//   C. 基类 operator new/delete 重载（本方案）：new 表达式自动走类专属
//      分配函数，make_unique 全部调用点零改动，改造面=ast_base.hpp 声明+
//      ast_arena.cpp 实现。
//
// 生存期与所有权语义不变（1482 核实结论）：
//   - AST 由 unique_ptr<Program> 独占持有（PipelineOutput/局部），活到编译
//     结束；全库无裸 delete、无 shared_ptr 持有 AST 节点；
//   - unique_ptr 销毁时虚析构链照常执行——字符串/vector 等堆成员资源正常
//     释放，仅节点本体内存滞留 arena（与「AST 活到编译结束」的生存期一致，
//     不构成泄漏：段链由下方静态 Arena 对象持有，内存检查工具视为仍可达）；
//   - 解析失败异常路径：析构照常执行，无泄漏面变化。
//
// 线程安全论证（1482 核实）：AstNode 构造仅发生在 parser/semantic/driver/
// AST 缓存反序列化——均为主线程顺序执行；codegen 并行线程池只读 AST 不构造
// 节点（1482 grep 核实）。故 arena 不加锁；若未来出现多线程构造 AstNode，
// 须先改 per-thread 分段（届时本注释即过期证据）。
//
// 失败语义：内存耗尽显式抛 std::bad_alloc（标准 new 表达式语义），禁静默
// 返回空继续编译（1482 纪律红线）。
#pragma once

#include <atomic>
#include <cstddef>
#include <new>

namespace cn_compiler {
namespace ast_arena {

// 分段 bump 分配器：每段头部 Segment 结构 + 数据区；段链单向串接，
// 由 Arena 静态对象析构时统一释放（进程退出/单测进程结束）。
class Arena {
public:
    Arena() = default;
    ~Arena();  // 释放全部段；统计开关开启时输出累计数据

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    // 按 align 对齐分配 size 字节；不足时扩段（大请求独立段）；
    // 系统内存耗尽时抛 std::bad_alloc（显式失败，无静默路径）。
    void* allocate(std::size_t size, std::size_t align);

    // 累计统计（诊断显形通道；atomics 宽松序——仅观测用）
    std::size_t allocCount() const { return allocCount_.load(std::memory_order_relaxed); }
    std::size_t allocBytes() const { return allocBytes_.load(std::memory_order_relaxed); }
    std::size_t segmentCount() const { return segmentCount_.load(std::memory_order_relaxed); }

private:
    struct Segment {
        Segment* next;       // 下一段（后进链头）
        std::size_t capacity;  // 数据区容量（字节，不含 Segment 头）
        std::size_t used;      // 已用量（字节）
        // 数据区紧跟本结构之后（ flexible 起点由 allocate 计算，不存字段）
    };

    // 常规段数据区容量：约容纳数千典型节点（节点本体普遍 <256 字节）
    static constexpr std::size_t kSegmentCapacity = 256 * 1024;

    // 新建一段并链入段链；capacity 为数据区字节数。返回段指针。
    Segment* newSegment(std::size_t capacity);

    Segment* head_ = nullptr;      // 段链头（最近一段）
    Segment* current_ = nullptr;   // 当前 bump 用的段
    std::atomic<std::size_t> allocCount_{0};
    std::atomic<std::size_t> allocBytes_{0};
    std::atomic<std::size_t> segmentCount_{0};
};

// 进程级单例（Meyer's singleton）：AST 节点生存期=编译会话，进程退出统一回收。
// 单测进程多轮小编译的累计驻留=各轮 AST 本体总和（量级几 MB～几十 MB，可接受，
// 1482 提交信息有量化）。
Arena& globalArena();

}  // namespace ast_arena
}  // namespace cn_compiler
