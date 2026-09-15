// 线性扫描寄存器分配器（阶段C Task 4.3，规格书8.3）
// 设计要点：
//   1. 输入：IRFunction（虚拟寄存器 %vN + 指令序列），输出：虚拟寄存器 -> 物理寄存器 或 栈槽
//   2. 活跃区间计算：函数级 def-use 链 + 跨块活跃分析（迭代数据流，可复用 opt::DomTree 的 CFG 结构）
//   3. 线性扫描：区间按起始点排序，维护活跃区间集合（按结束点最小堆），
//      区间开始分配空闲物理寄存器，不足时溢出到栈槽
//   4. 寄存器集合按目标平台配置：
//        x64：rbx/r12~r15 被调用者保存（分配器可用）；rax/rcx/rdx/r8~r11 调用者保存（不分配）
//        arm64：x19~x28 被调用者保存；x0~x18 调用者保存（不分配）
//       —— 仅分配被调用者保存寄存器，是因为两个后端发射器内部均使用调用者保存寄存器
//          （rax/rcx/rdx/r8~r11 / x0~x18 等）作临时寄存器，被调用者保存区空闲可安全使用；
//          同时分配器寄存器在函数序言保存、尾声恢复（不破坏调用者预期）
//   5. 保守策略（正确性最高优先）：仅对 64 位整型/指针类型（i64/u64/ptr）虚拟寄存器做
//      物理寄存器分配；i128 双槽、浮点、小位宽寄存器保持栈槽映射（避免与发射器宽度假设冲突）
//   6. 溢出策略：物理寄存器不足时，溢出虚拟寄存器到"溢出槽"（复用栈槽区，偏移由调用方决定）
//   7. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {
namespace regalloc {

// 目标平台类型（与 Backend::targetPlatform 对应）
enum class TargetArch {
    X64,      // Win x64（被调用者保存：rbx/r12~r15）
    Arm64,    // Linux ARM64（被调用者保存：x19~x28）
};

// 活跃区间（live interval）：虚拟寄存器从定义点到最后一次使用的范围
struct LiveInterval {
    int regId = -1;            // 虚拟寄存器编号
    int start = 0;             // 起始点（指令序数，函数级线性化）
    int end = 0;               // 结束点（最后一次使用，含）
    bool isFloat = false;      // 是否浮点（当前不参与寄存器分配）
    bool is128 = false;        // 是否 i128/u128 双槽（当前不参与寄存器分配）

    bool overlaps(const LiveInterval& other) const {
        return start <= other.end && other.start <= end;
    }
};

// 分配结果：虚拟寄存器 -> 物理寄存器 或 溢出槽
//  - assignedReg: 非空表示分配到物理寄存器（如 "rbx"/"x20"）
//  - spillSlot:   溢出槽索引（>=0 表示溢出；-1 表示按原有栈槽映射）
struct RegAssignment {
    std::string assignedReg;   // 物理寄存器名（空 = 未分配寄存器，走栈槽）
    int spillSlot = -1;        // 溢出槽索引（>=0 表示溢出；-1 表示按原有栈槽映射）
};

// 分配结果表：虚拟寄存器ID -> RegAssignment
using RegAssignmentMap = std::unordered_map<int, RegAssignment>;

// 线性扫描寄存器分配器
class LinearScanAllocator {
public:
    // 平台配置：目标架构 + 额外保留的物理寄存器（如 x64 r12 用于隐藏返回指针场景）
    explicit LinearScanAllocator(TargetArch arch,
                                 const std::vector<std::string>& reservedRegs = {});

    // 主入口：对函数做寄存器分配，返回虚拟寄存器 -> 分配结果
    //  - 仅对 64 位整型/指针虚拟寄存器分配物理寄存器；其余保持栈槽映射（assignedReg 空）
    RegAssignmentMap allocate(const ir::IRFunction& function);

    // 活跃区间列表（allocate 后可用，测试/调试）
    const std::vector<LiveInterval>& intervals() const { return intervals_; }

    // 可用物理寄存器名列表（allocate 后可用）
    const std::vector<std::string>& regs() const { return regs_; }

    // 静态：活跃区间计算（暴露供单测直接验证 def-use / 跨块活跃）
    //  - 返回按 start 升序排序的活跃区间列表
    static std::vector<LiveInterval> computeLiveIntervals(const ir::IRFunction& function);
    // ---- computeLiveIntervals 流水线族子方法（190-a 函数级拆分·原 179 行函数）----
    // 族①：块序线性化——块序 -> (起始序数, 块内指令数)
    static std::vector<std::pair<int, int>> computeBlockRanges(const ir::IRFunction& function);
    // 族②：块级 def/use 集合（跨块活跃传播用）
    static void computeBlockDefUse(const ir::IRFunction& function,
                                   const std::vector<std::pair<int, int>>& blockRanges,
                                   std::vector<std::unordered_set<int>>& blockDef,
                                   std::vector<std::unordered_set<int>>& blockUse);
    // 族③：跨块活跃传播（迭代数据流）——返回 in 集合
    static std::vector<std::unordered_set<int>> computeLivenessIn(
        const ir::IRFunction& function,
        const std::vector<std::unordered_set<int>>& blockDef,
        const std::vector<std::unordered_set<int>>& blockUse);
    // 族④：活跃区间生成（def/use 精确序数；Alloca 结果排除）
    static std::unordered_map<int, LiveInterval> buildLiveIntervalMap(
        const ir::IRFunction& function,
        const std::vector<std::pair<int, int>>& blockRanges,
        const std::vector<std::unordered_set<int>>& in,
        const std::vector<std::unordered_set<int>>& blockDef);

    // 静态：判断 IR 类型是否可参与寄存器分配（i64/u64/ptr；非浮点、非 i128）
    static bool isAllocableType(const std::string& type);

private:
    // 线性扫描主循环（区间按 start 升序，活跃集合按 end 升序）
    void scan(const std::vector<LiveInterval>& intervals, RegAssignmentMap& out);

    TargetArch arch_;                     // 目标架构
    std::vector<std::string> regs_;       // 可用物理寄存器（被调用者保存，去保留）
    std::vector<LiveInterval> intervals_; // 活跃区间（排序后）
};

} // namespace regalloc
} // namespace cn_compiler
