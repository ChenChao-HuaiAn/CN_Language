// AST arena 实现（任务 1482）：分段 bump 分配 + AstNode 类专属 new/delete。
// 设计与生存期论证见 ast_arena.hpp 文件头注释。
#include "cn_compiler/model/ast_arena.hpp"

#include <cstdio>
#include <new>

#include "cn_compiler/common/env_query.hpp"
#include "cn_compiler/model/ast_base.hpp"

namespace cn_compiler {
namespace ast_arena {

namespace {

// 环境开关：CN_AST_ARENA_STATS=1 时 Arena 析构输出累计统计（节点数/字节/段数
// ——节点数即原 make_unique 堆分配次数的等价观测）。默认关闭零输出。
// 读取走 common/env_query 归纳单点（1671：跨平台环境变量读取不散写）。
bool statsEnabled() {
    std::string value;
    return cn::env_query::readValue("CN_AST_ARENA_STATS", value) && value == "1";
}

}  // namespace

Arena::~Arena() {
    if (statsEnabled()) {
        // stderr 通道：与诊断一致，stdout 产物不受污染
        std::fprintf(stderr, "[ast-arena] 节点分配 %zu 次 / %zu 字节 / %zu 段\n",
                     allocCount(), allocBytes(), segmentCount());
    }
    Segment* seg = head_;
    while (seg != nullptr) {
        Segment* next = seg->next;
        // 段本体与数据区一次分配（newSegment 内含头部空间），一处归还
        ::operator delete(static_cast<void*>(seg));
        seg = next;
    }
    head_ = nullptr;
    current_ = nullptr;
}

Arena::Segment* Arena::newSegment(std::size_t capacity) {
    // 段头 + 数据区一次分配；段头对齐即最大基础对齐，数据区起点另行对齐
    void* raw = ::operator new(sizeof(Segment) + capacity);
    Segment* seg = static_cast<Segment*>(raw);
    seg->next = head_;
    seg->capacity = capacity;
    seg->used = 0;
    head_ = seg;
    current_ = seg;
    segmentCount_.fetch_add(1, std::memory_order_relaxed);
    return seg;
}

void* Arena::allocate(std::size_t size, std::size_t align) {
    // 对齐规整：align 为 2 的幂（new 表达式保证）；1 归一为合法下界
    const std::size_t alignEff = align < 1 ? 1 : align;

    // 循环形式（禁递归）：当前段可容纳则 bump；不足则扩段后重试一次——
    // 扩段容量含最大对齐 padding（size + alignEff - 1），新段必容纳。
    for (;;) {
        Segment* seg = current_;
        if (seg != nullptr) {
            // bump 指针按对齐上取：数据区起点=段头之后
            const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(seg) +
                                        sizeof(Segment) + seg->used;
            const std::uintptr_t aligned =
                (base + (alignEff - 1)) & ~(static_cast<std::uintptr_t>(alignEff - 1));
            const std::size_t need = static_cast<std::size_t>(aligned - base) + size;
            if (need <= seg->capacity - seg->used) {
                seg->used += need;
                allocCount_.fetch_add(1, std::memory_order_relaxed);
                allocBytes_.fetch_add(size, std::memory_order_relaxed);
                return reinterpret_cast<void*>(aligned);
            }
        }
        // 当前段不足：超大请求独立段（容量含对齐 padding，杜绝边界死循环），
        // 常规请求开新标准段
        const std::size_t cap =
            size + alignEff - 1 > kSegmentCapacity ? size + alignEff - 1 : kSegmentCapacity;
        newSegment(cap);
    }
}

Arena& globalArena() {
    static Arena instance;
    return instance;
}

}  // namespace ast_arena

// ==================== AstNode 类专属 new/delete（1482） ====================
// 声明见 ast_base.hpp；语义：
//   new  -> arena bump 分配（含对齐版本，覆盖超基础对齐的派生类编译期要求）；
//   delete -> no-op：节点本体不归还，随进程 arena 统一回收；unique_ptr 析构链
//   中成员资源照常释放（见 ast_arena.hpp 生存期论证）。

void* AstNode::operator new(std::size_t size) {
    // 标准 new 表达式承诺 __STDCPP_DEFAULT_NEW_ALIGNMENT__ 对齐（MSVC x64 为
    // 16——不可用 alignof(std::max_align_t) 代替，其值为 8，会对 16 对齐成员
    // 欠对齐）
    return ast_arena::globalArena().allocate(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

void* AstNode::operator new(std::size_t size, std::align_val_t align) {
    return ast_arena::globalArena().allocate(size, static_cast<std::size_t>(align));
}

void AstNode::operator delete(void* ptr) noexcept {
    (void)ptr;  // arena 语义：不逐节点归还
}

void AstNode::operator delete(void* ptr, std::size_t) noexcept {
    (void)ptr;  // arena 语义：不逐节点归还
}

void AstNode::operator delete(void* ptr, std::align_val_t) noexcept {
    (void)ptr;  // arena 语义：不逐节点归还
}

void AstNode::operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    (void)ptr;  // arena 语义：不逐节点归还
}

}  // namespace cn_compiler
