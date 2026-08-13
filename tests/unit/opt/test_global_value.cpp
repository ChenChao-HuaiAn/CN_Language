// 全局值传播 Pass 单元测试（Task 完善C 优化器增强）
// 覆盖：常量 Store->Load 转发（跨块线性序）、槽被非常量重写失效、
//       多槽变量跳过、浮点/i128 不传播、类型匹配
// 测试名英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/global_value.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::GlobalValuePass;

namespace {

// 构造含给定块列表的模块
IRModule makeModule(std::vector<std::unique_ptr<IRBlock>> blocks) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.blocks = std::move(blocks);
    module.functions.push_back(std::move(fn));
    return module;
}

// 新建块（label + 指令列表）
std::unique_ptr<IRBlock> makeBlock(const std::string& label,
                                   std::vector<IRInstruction> insts) {
    auto block = std::make_unique<IRBlock>();
    block->label = label;
    block->instructions = std::move(insts);
    return block;
}

// 便捷构造：指令
IRInstruction makeInst(Opcode op, const std::vector<IRValue>& ops,
                       const IRValue& result, const std::string& type) {
    IRInstruction inst;
    inst.opcode = op;
    inst.operands = ops;
    inst.result = result;
    inst.type = type;
    return inst;
}

// Store 常量到槽
IRInstruction storeConst(const std::string& slot, const std::string& text,
                         const std::string& type) {
    IRInstruction inst = makeInst(Opcode::Store,
                                  {IRValue::constant(text, type)}, IRValue(), type);
    inst.extra = slot;
    return inst;
}

// Load 槽到结果寄存器
IRInstruction loadSlot(const std::string& slot, const std::string& type, int resultId) {
    return makeInst(Opcode::Load, {IRValue::var(slot, type)},
                    IRValue::reg(resultId, type), type);
}

// 检查指令操作数是否已替换为常量
bool operandIsConst(const IRModule& module, std::size_t blockIndex,
                    std::size_t instIndex, std::size_t opIndex,
                    const std::string& text) {
    const auto& inst = module.functions[0].blocks[blockIndex]->instructions[instIndex];
    if (opIndex >= inst.operands.size()) return false;
    return inst.operands[opIndex].isConstant &&
           inst.operands[opIndex].extra == text;
}

} // namespace

// ==================== 常量 Store->Load 跨块转发 ====================

TEST(GlobalValueTest, CrossBlockConstForward) {
    // 块0: Store 100 -> x
    // 块1: Load x -> %v5; Add %v5, 1 -> %v6
    // 线性序（块0 在块1 前）：Load 命中常量 100 -> %v5 替换为常量
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {storeConst("x$0", "100", "i32")}));
    blocks.push_back(makeBlock("块1", {
        loadSlot("x$0", "i32", 5),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(6, "i32"), "i32"),
    }));
    auto module = makeModule(std::move(blocks));
    EXPECT_TRUE(GlobalValuePass().run(module));
    EXPECT_TRUE(operandIsConst(module, 1, 1, 0, "100"));
}

TEST(GlobalValueTest, SameBlockConstForward) {
    // 块0: Store 7 -> x; Load x -> %v5; Add %v5, 1
    auto module = makeModule(std::vector<std::unique_ptr<IRBlock>>());
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {
        storeConst("x$0", "7", "i32"),
        loadSlot("x$0", "i32", 5),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(6, "i32"), "i32"),
    }));
    module = makeModule(std::move(blocks));
    EXPECT_TRUE(GlobalValuePass().run(module));
    EXPECT_TRUE(operandIsConst(module, 0, 2, 0, "7"));
}

TEST(GlobalValueTest, RewrittenSlotNotForwarded) {
    // Store 100 -> x; Store %v1 -> x; Load x
    // x 被非常量重写（Store 寄存器值）-> 槽值不确定 -> 不转发
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {
        storeConst("x$0", "100", "i32"),
        makeInst(Opcode::Store, {IRValue::reg(1, "i32")}, IRValue(), "i32"),
        loadSlot("x$0", "i32", 5),
    }));
    blocks[0]->instructions[1].extra = "x$0";
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(GlobalValuePass().run(module));
}

TEST(GlobalValueTest, CallInvalidatesAll) {
    // Store 100 -> x; Call; Load x（Call 可能改 x -> 不转发）
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {
        storeConst("x$0", "100", "i32"),
        makeInst(Opcode::Call, {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")},
                 IRValue::reg(20, "i32"), "i32"),
        loadSlot("x$0", "i32", 5),
    }));
    blocks[0]->instructions[1].extra = "函数";
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(GlobalValuePass().run(module));
}

// ==================== 多槽变量跳过 ====================

TEST(GlobalValueTest, MultiSlotSkipped) {
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {
        storeConst("x$0", "100", "i32"),
        loadSlot("x$0", "i32", 5),
    }));
    auto module = makeModule(std::move(blocks));
    module.functions[0].varSlots["x$0"] = 5;  // 数组 5 槽
    EXPECT_FALSE(GlobalValuePass().run(module));
}

// ==================== 浮点/i128 不传播 ====================

TEST(GlobalValueTest, FloatConstNotForwarded) {
    // 浮点常量存储 -> 不传播（MASM 无浮点立即数）
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {
        storeConst("x$0", "3.5", "f64"),
        loadSlot("x$0", "f64", 5),
    }));
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(GlobalValuePass().run(module));
}

TEST(GlobalValueTest, I128ConstNotForwarded) {
    // i128 常量存储 -> 不传播（双槽，无单一立即数）
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {
        storeConst("x$0", "100", "i128"),
        loadSlot("x$0", "i128", 5),
    }));
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(GlobalValuePass().run(module));
}

// ==================== 类型匹配 ====================

TEST(GlobalValueTest, TypeMismatchNotForwarded) {
    // Store i32 100 -> x; Load x 结果 i64（不匹配 -> 不转发）
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {
        storeConst("x$0", "100", "i32"),
        loadSlot("x$0", "i64", 5),
    }));
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(GlobalValuePass().run(module));
}
