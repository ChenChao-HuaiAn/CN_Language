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
//   5. 类型面（1281 全架构统一）：i64/u64/ptr + i32/u32 + i8/u8/i16/u16 可分配；
//      i128 双槽、浮点保持栈槽映射（1256 的 win-x64/arm64 架构门控随 1159
//      三后端直写审计收口退场）
//      1267 续（仅 LinuxX64）：i1 布尔纳入可分配面——布尔值在寄存器内按 32 位
//      规范（低 32 位 0/1、高 32 位清零，与比较/逻辑/常量发射同款），是
//      mem2reg 后 φ 降级搬运（v2 全树 11,125 条中 6,783 条为 i1）与条件值的
//      主要承载类型；不分配时每条 i1 搬运=槽读+槽写两次内存访问。win-x64/
//      arm64 的 i1 落位面未审计=后续任务，allocate() 显式按架构剔除 i1 区间
//      （其产物逐字节不变）。
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
    X64,        // Win x64（被调用者保存：rbx/r12~r15）
    Arm64,      // Linux ARM64（被调用者保存：x19~x28）
    LinuxX64,   // Linux x86_64（被调用者保存：rbx/r12~r15；1256 起 32/窄整型接入）
};

// 寄存器类（#1212）：整数/浮点各自独立寄存器堆与生命周期，互不挤占。
//   浮点类 = SSE xmm（win-x64）/ V 寄存器（arm64）/ SSE（linux-x64）。
enum class RegClass {
    Int,     // 通用寄存器（GPR）
    Float,   // 浮点/向量寄存器堆（f32/f64）
};

// 活跃区间（live interval）：虚拟寄存器从定义点到最后一次使用的范围
struct LiveInterval {
    int regId = -1;            // 虚拟寄存器编号
    int start = 0;             // 起始点（指令序数，函数级线性化）
    int end = 0;               // 结束点（最后一次使用，含）
    bool isFloat = false;      // 是否浮点（#1212：进入独立浮点寄存器堆）
    bool is128 = false;        // 是否 i128/u128 双槽（#1212 步骤 b：双 GPR 成对分配）
    std::string type;          // 值类型（可分配面判定：1281 全架构统一）

    bool overlaps(const LiveInterval& other) const {
        return start <= other.end && other.start <= end;
    }
};

// 分配结果：虚拟寄存器 -> 物理寄存器 或 溢出槽
//  - assignedReg: 非空表示分配到物理寄存器（如 "rbx"/"x20"/"xmm6"/"v8"）
//  - assignedReg2: 128 位双半的第二半（高半寄存器；仅 i128/u128 成对分配时非空）
//  - spillSlot:   溢出槽索引（>=0 表示溢出；-1 表示按原有栈槽映射）
struct RegAssignment {
    std::string assignedReg;   // 物理寄存器名（空 = 未分配寄存器，走栈槽）
    std::string assignedReg2;  // 128 位第二半寄存器（#1212 步骤 b；空 = 无）
    int spillSlot = -1;        // 溢出槽索引（>=0 表示溢出；-1 表示按原有栈槽映射）
    RegClass regClass = RegClass::Int;  // 寄存器类（发射端选择指令族）
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
    //  - 仅对可分配类型分配物理寄存器；其余保持栈槽映射（assignedReg 空）
    //  - maxRegId：调用方已知的函数最大虚拟寄存器ID（同 ir::maxVirtualRegId 口径）；
    //    -1=内部实测（1256：免后端路径二次全函数扫描）
    RegAssignmentMap allocate(const ir::IRFunction& function, int maxRegId = -1);

    // 活跃区间列表（allocate 后可用，测试/调试）
    const std::vector<LiveInterval>& intervals() const { return intervals_; }

    // 可用物理寄存器名列表（allocate 后可用）
    const std::vector<std::string>& regs() const { return regs_; }

    // 静态：活跃区间计算（暴露供单测直接验证 def-use / 跨块活跃）
    //  - 返回按 start 升序排序的活跃区间列表（全可分配类型面）
    static std::vector<LiveInterval> computeLiveIntervals(const ir::IRFunction& function);
    // 活跃性参与面：allowNarrow=false 时活跃集只含 i64/u64/ptr（单元测试调试用）
    static std::vector<LiveInterval> computeLiveIntervalsInternal(
        const ir::IRFunction& function, bool allowNarrow, int maxRegId);
    // ---- computeLiveIntervals 流水线族子方法（190-a 函数级拆分·原 179 行函数）----
    // 族①：块序线性化——块序 -> (起始序数, 块内指令数)
    static std::vector<std::pair<int, int>> computeBlockRanges(const ir::IRFunction& function);
    // 族②：块级 def/use 集合（跨块活跃传播用）
    static void computeBlockDefUse(const ir::IRFunction& function,
                                   const std::vector<std::pair<int, int>>& blockRanges,
                                   std::vector<std::unordered_set<int>>& blockDef,
                                   std::vector<std::unordered_set<int>>& blockUse,
                                   bool allowNarrow, int maxRegId);
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
        const std::vector<std::unordered_set<int>>& blockDef, bool allowNarrow,
        int maxRegId);

    // 静态：判断 IR 类型是否可参与寄存器分配（i64/u64/ptr/i32/u32/i1/i8/u8/
    //   i16/u16 + f32/f64）。#1212：浮点进入独立 SSE 寄存器堆（xmm），
    //   整数面不变；i128 双半成对分配见步骤 b。
    static bool isAllocableType(const std::string& type);

    // #1212：浮点可分配类型（f32/f64）——独立寄存器类判据
    static bool isAllocableFloatType(const std::string& type);

private:
    // 线性扫描主循环（区间按 start 升序，活跃集合按 end 升序）
    void scan(const std::vector<LiveInterval>& intervals, RegAssignmentMap& out);
    // 单一寄存器堆扫描（#1212：整数/浮点各自独立调用，资源池与溢出通道隔离）
    void scanPool(const std::vector<LiveInterval>& intervals,
                  const std::vector<std::string>& pool, RegClass regClass,
                  RegAssignmentMap& out);

    TargetArch arch_;                     // 目标架构
    std::vector<std::string> regs_;       // 可用整数物理寄存器（被调用者保存，去保留）
    std::vector<std::string> fpRegs_;     // 可用浮点物理寄存器（#1212，独立寄存器堆）
    std::vector<LiveInterval> intervals_; // 活跃区间（排序后）
};

} // namespace regalloc
} // namespace cn_compiler
