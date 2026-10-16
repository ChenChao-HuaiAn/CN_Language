// CN 运行时小对象池内部接口（mem_pool.cpp 实现）——074 波3 立（2026-09-29·[基准=019]）；
//   945（2026-10-16）无锁并发分配数据竞争根治后定型为「线程局部快路径 + 全局锁慢路径」。
// 仅 runtime 内部使用（mem_api.cpp 调用）；不当做 CN 符号（无 extern "C"）。
// 设计口径（945·对标 Rust/jemalloc/tcmalloc 的线程局部 arena）：
//   · 快路径全线程局部（TLS 自由链 + TLS 切分游标·无锁无原子）= 单线程零税；
//   · 慢路径全局限阱（std::mutex）：跨线程释放/新块创建/线程退出回收/reset/观测；
//   · 指针溯源=块目录二分，不读对象头（外部/悬垂指针绝不触碰内存）。
#pragma once

#include <cstddef>

namespace cnrt {

// 池分配：≤4KB 定档池返回槽指针；>4KB 或池不可用返回 nullptr（调用方回退 malloc）
void* poolAlloc(std::size_t size);

// 池释放分流结果（945：与旧 poolFindClass+poolFree 双返回值语义逐字对齐——
//   旧实现「重复释放拦截」**不扣活动分配数**（拦截=对已归还槽的错误释放，
//   再扣会让计数偏低）；宿主/v2 双侧口径一致=零分叉，69 用例 v2 侧重复
//   释放形态之计数行为由此保持）。
enum class PoolFreeResult {
    NotPool,        // 非池指针/已 reset 的池外指针：调用方继续自己的释放路径
    Freed,          // 池内槽已归还：调用方 --活动分配数
    RejectedDouble, // 池内槽重复释放被拦截（未归还）：调用方不得再 free，也不扣计数
};
PoolFreeResult poolTryFree(void* ptr);

// 池指针档位查询（cn_realloc 容量内复用判定用）：命中返回档位下标；否则 -1
int poolClassOfPtr(void* ptr);

// 档位容量（字节）：cls ∈ [0, 档位数)；越界返回 0（哨兵·调用方分流）
std::size_t poolClassCapacity(int cls);

// 池重置（内存::释放全部）：全部块归还、链/目录清空；返回重置前池内活动对象数
//   （供 g_cn_alloc_live 扣减——945 用位图扫描统计替代旧全局 live 计数）。
//   语义前提：调用时无并发池操作（组件间清场·用户责任）。
long long poolReset();

// 观测报告（atexit·CN_RT_MEM_STATS 门控）：池规模/活动对象数/重复释放拦截计数
void poolReportLines();

} // namespace cnrt
