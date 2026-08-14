// 线性扫描寄存器分配器单元测试（阶段C Task 4.3）
// 覆盖：
//   1. 活跃区间计算：def-use 链 + 跨块活跃传播（简单顺序/分支/循环场景）
//   2. 分配无冲突：同一物理寄存器不被两个活跃区间同时占用（重叠区间不同寄存器）
//   3. 溢出正确：物理寄存器不足时溢出到栈槽（spillSlot >= 0）
//   4. 被调用者保存寄存器：分配结果只使用被调用者保存寄存器（rbx/r12~r15 / x19~x28）
//   5. 类型过滤：仅 i64/u64/ptr 参与分配，i32/浮点/i128 保持栈槽
//   6. 保留寄存器：x64 隐藏返回指针场景保留 r12
// 构造方式：直接手工构造 ir::IRFunction（不经过前端链路，聚焦分配器本身）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>

#include "cn_compiler/codegen/reg_alloc.hpp"
#include "cn_compiler/ir/ir.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::regalloc::LinearScanAllocator;
using cn_compiler::regalloc::LiveInterval;
using cn_compiler::regalloc::RegAssignmentMap;
using cn_compiler::regalloc::TargetArch;

namespace {

// 辅助：构造单块函数——连续 ConstInt/Add 链
//   %v0 = ConstInt 1（i64）
//   %v1 = ConstInt 2（i64）
//   %v2 = Add %v0, %v1（i64）
//   %v3 = Add %v2, %v0（i64）
//   返回 %v3
IRFunction buildSimpleFunc() {
    IRFunction func;
    func.name = "simple";
    func.returnType = "i64";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    auto addConst = [&block](int id, const std::string& val) {
        IRInstruction c;
        c.opcode = Opcode::ConstInt;
        c.result = IRValue::reg(id, "i64");
        c.type = "i64";
        c.extra = val;
        block->instructions.push_back(c);
    };
    auto addAdd = [&block](int id, int op1, int op2) {
        IRInstruction a;
        a.opcode = Opcode::Add;
        a.result = IRValue::reg(id, "i64");
        a.type = "i64";
        a.operands = {IRValue::reg(op1, "i64"), IRValue::reg(op2, "i64")};
        block->instructions.push_back(a);
    };
    addConst(0, "1");
    addConst(1, "2");
    addAdd(2, 0, 1);
    addAdd(3, 2, 0);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v3";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 4;
    return func;
}

// 辅助：构造跨块函数（条件跳转，验证跨块活跃传播）
//   块0: %v0 = ConstInt 1; %v1 = ConstInt 2; 条件跳转（%v0 活跃跨块）
//   块1: %v2 = Add %v0, %v1; 返回 %v2
IRFunction buildBranchFunc() {
    IRFunction func;
    func.name = "branch";
    func.returnType = "i64";
    func.nextRegId = 3;

    auto block0 = std::make_unique<IRBlock>();
    block0->label = "块0";
    IRInstruction c0;
    c0.opcode = Opcode::ConstInt;
    c0.result = IRValue::reg(0, "i64");
    c0.type = "i64";
    c0.extra = "1";
    block0->instructions.push_back(c0);
    IRInstruction c1;
    c1.opcode = Opcode::ConstInt;
    c1.result = IRValue::reg(1, "i64");
    c1.type = "i64";
    c1.extra = "2";
    block0->instructions.push_back(c1);
    // 条件跳转：条件 = %v0（i1 不参与分配，但 %v0 用作 %v0 需要活跃跨块）
    block0->terminated = true;
    block0->termKind = "条件跳转";
    block0->termTrueTarget = "块1";
    block0->termFalseTarget = "块1";

    auto block1 = std::make_unique<IRBlock>();
    block1->label = "块1";
    IRInstruction a;
    a.opcode = Opcode::Add;
    a.result = IRValue::reg(2, "i64");
    a.type = "i64";
    a.operands = {IRValue::reg(0, "i64"), IRValue::reg(1, "i64")};
    block1->instructions.push_back(a);
    block1->terminated = true;
    block1->termKind = "返回";
    block1->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block0));
    func.blocks.push_back(std::move(block1));
    return func;
}

} // namespace

// ---- 1. 活跃区间计算 ----

TEST(RegAllocTest, LiveIntervalsSimple) {
    const IRFunction func = buildSimpleFunc();
    const std::vector<LiveInterval> intervals =
        LinearScanAllocator::computeLiveIntervals(func);
    // %v0 def@0 use@2,3 -> start=0 end=3
    // %v1 def@1 use@2 -> start=1 end=2
    // %v2 def@2 use@3 -> start=2 end=3
    // %v3 def@3 use@3(返回) -> start=3 end=3
    ASSERT_EQ(intervals.size(), 4u);
    // 按 start 升序：v0(0), v1(1), v2(2), v3(3)
    EXPECT_EQ(intervals[0].regId, 0);
    EXPECT_EQ(intervals[0].start, 0);
    EXPECT_EQ(intervals[0].end, 3);
    EXPECT_EQ(intervals[1].regId, 1);
    EXPECT_EQ(intervals[1].start, 1);
    EXPECT_EQ(intervals[1].end, 2);
    EXPECT_EQ(intervals[2].regId, 2);
    EXPECT_EQ(intervals[2].start, 2);
    EXPECT_EQ(intervals[2].end, 3);
    EXPECT_EQ(intervals[3].regId, 3);
    EXPECT_EQ(intervals[3].start, 3);
}

TEST(RegAllocTest, LiveIntervalsCrossBlock) {
    const IRFunction func = buildBranchFunc();
    const std::vector<LiveInterval> intervals =
        LinearScanAllocator::computeLiveIntervals(func);
    // %v0 def@块0:0 use@块1:0 -> 跨块活跃：end 必须覆盖到块1（use 点）
    //   块0 指令序数 0,1；块1 起始序数 2；use 在序数 2
    bool foundV0 = false;
    for (const auto& li : intervals) {
        if (li.regId == 0) {
            foundV0 = true;
            EXPECT_EQ(li.start, 0);
            // 跨块：end 必须 >= 块1 的 use 点（序数 2）
            EXPECT_GE(li.end, 2);
        }
    }
    EXPECT_TRUE(foundV0);
}

// ---- 2. 分配无冲突 ----

TEST(RegAllocTest, NoOverlapConflict) {
    const IRFunction func = buildSimpleFunc();
    LinearScanAllocator allocator(TargetArch::X64);
    const RegAssignmentMap map = allocator.allocate(func);
    // 4 个区间，x64 可用 5 个被调用者保存寄存器（rbx/r12/r13/r14/r15），全部不溢出
    for (int id = 0; id < 4; ++id) {
        auto it = map.find(id);
        ASSERT_NE(it, map.end()) << "regId " << id << " 应有分配结果";
        EXPECT_FALSE(it->second.assignedReg.empty()) << "regId " << id << " 应分配到寄存器";
    }
    // 重叠区间（v0 与 v1、v2 重叠）不得分配到同一物理寄存器
    const std::string reg0 = map.at(0).assignedReg;
    const std::string reg1 = map.at(1).assignedReg;
    const std::string reg2 = map.at(2).assignedReg;
    EXPECT_NE(reg0, reg1);
    EXPECT_NE(reg0, reg2);
}

// ---- 3. 溢出正确 ----

TEST(RegAllocTest, SpillWhenRegsExhausted) {
    // 构造大量连续活跃寄存器（8 个 i64，同时活跃），x64 只有 5 个可用寄存器
    IRFunction func;
    func.name = "spill";
    func.returnType = "i64";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    // 8 个 ConstInt（各活跃到返回），v0..v7 全部同时活跃
    for (int i = 0; i < 8; ++i) {
        IRInstruction c;
        c.opcode = Opcode::ConstInt;
        c.result = IRValue::reg(i, "i64");
        c.type = "i64";
        c.extra = std::to_string(i);
        block->instructions.push_back(c);
    }
    // 求和链使全部寄存器活跃到最后一条指令
    for (int i = 8; i < 16; ++i) {
        IRInstruction a;
        a.opcode = Opcode::Add;
        a.result = IRValue::reg(i, "i64");
        a.type = "i64";
        a.operands = {IRValue::reg(i - 8, "i64"), IRValue::reg(i - 1, "i64")};
        block->instructions.push_back(a);
    }
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v15";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 16;

    LinearScanAllocator allocator(TargetArch::X64);
    const RegAssignmentMap map = allocator.allocate(func);
    // 至少有一个寄存器溢出到栈槽（5 个寄存器不够 8 个同时活跃区间）
    int spillCount = 0;
    for (const auto& kv : map) {
        if (kv.second.spillSlot >= 0) ++spillCount;
    }
    EXPECT_GT(spillCount, 0) << "寄存器不足时应发生溢出";
    // 溢出槽索引非负且不冲突（递增分配）
    std::unordered_map<int, int> seenSlots;
    for (const auto& kv : map) {
        if (kv.second.spillSlot >= 0) {
            EXPECT_EQ(seenSlots.count(kv.second.spillSlot), 0u)
                << "溢出槽 " << kv.second.spillSlot << " 被重复分配";
            seenSlots[kv.second.spillSlot] = kv.first;
        }
    }
}

// ---- 4. 被调用者保存寄存器 ----

TEST(RegAllocTest, OnlyCalleeSavedRegs) {
    const IRFunction func = buildSimpleFunc();
    LinearScanAllocator allocator(TargetArch::X64);
    const RegAssignmentMap map = allocator.allocate(func);
    static const char* calleeSaved[] = {"rbx", "r12", "r13", "r14", "r15"};
    for (const auto& kv : map) {
        if (!kv.second.assignedReg.empty()) {
            bool found = false;
            for (const char* r : calleeSaved) {
                if (kv.second.assignedReg == r) { found = true; break; }
            }
            EXPECT_TRUE(found) << "分配器不得使用非被调用者保存寄存器: "
                               << kv.second.assignedReg;
        }
    }
}

TEST(RegAllocTest, Arm64RegSet) {
    const IRFunction func = buildSimpleFunc();
    LinearScanAllocator allocator(TargetArch::Arm64);
    const RegAssignmentMap map = allocator.allocate(func);
    // arm64 被调用者保存：x19~x28
    static const char* calleeSaved[] = {"x19", "x20", "x21", "x22", "x23",
                                        "x24", "x25", "x26", "x27", "x28"};
    for (const auto& kv : map) {
        if (!kv.second.assignedReg.empty()) {
            bool found = false;
            for (const char* r : calleeSaved) {
                if (kv.second.assignedReg == r) { found = true; break; }
            }
            EXPECT_TRUE(found) << "arm64 分配器不得使用非被调用者保存寄存器: "
                               << kv.second.assignedReg;
        }
    }
}

// ---- 5. 类型过滤 ----

TEST(RegAllocTest, AllocableTypeFilter) {
    // i64/u64/ptr 可分配；i32/浮点/i128/i1 不可分配
    EXPECT_TRUE(LinearScanAllocator::isAllocableType("i64"));
    EXPECT_TRUE(LinearScanAllocator::isAllocableType("u64"));
    EXPECT_TRUE(LinearScanAllocator::isAllocableType("ptr"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("i32"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("u32"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("i8"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("f64"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("f32"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("i128"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("u128"));
    EXPECT_FALSE(LinearScanAllocator::isAllocableType("i1"));
}

TEST(RegAllocTest, NarrowTypesStayStack) {
    // i32 寄存器不参与分配（保持栈槽）
    IRFunction func;
    func.name = "narrow";
    func.returnType = "i32";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction c;
    c.opcode = Opcode::ConstInt;
    c.result = IRValue::reg(0, "i32");
    c.type = "i32";
    c.extra = "10";
    block->instructions.push_back(c);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;

    LinearScanAllocator allocator(TargetArch::X64);
    const RegAssignmentMap map = allocator.allocate(func);
    // i32 寄存器不参与分配 -> 映射为空或 assignedReg 为空
    auto it = map.find(0);
    if (it != map.end()) {
        EXPECT_TRUE(it->second.assignedReg.empty());
        EXPECT_EQ(it->second.spillSlot, -1);
    }
}

// ---- 6. 保留寄存器 ----

TEST(RegAllocTest, ReservedR12) {
    const IRFunction func = buildSimpleFunc();
    // 隐藏返回指针场景：保留 r12
    LinearScanAllocator allocator(TargetArch::X64, {"r12"});
    const RegAssignmentMap map = allocator.allocate(func);
    for (const auto& kv : map) {
        EXPECT_NE(kv.second.assignedReg, "r12")
            << "保留寄存器 r12 不得被分配";
    }
}
