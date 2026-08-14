// 尾调用优化（TCO）Pass 单元测试（阶段B Task 4.2）
// 覆盖：尾递归调用转为"参数槽更新 + 跳回入口"、
//       非自调用（尾调用其他函数）不优化、
//       入口块对参数槽初始 Store 时不优化、
//       实参为嵌套调用结果时不优化、幂等
// 测试方式：直接构造 IRModule 调用 TailCallPass（opt 为纯内部模块）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/tail_call.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::TailCallPass;

namespace {

IRInstruction makeInst(Opcode op, const std::vector<IRValue>& ops,
                       const IRValue& result, const std::string& type) {
    IRInstruction inst;
    inst.opcode = op;
    inst.operands = ops;
    inst.result = result;
    inst.type = type;
    return inst;
}

std::unique_ptr<IRBlock> makeBlock(const std::string& label,
                                   std::vector<IRInstruction> insts) {
    auto block = std::make_unique<IRBlock>();
    block->label = label;
    block->instructions = std::move(insts);
    return block;
}

void endReturn(IRBlock& block, const std::string& value) {
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = value;
}

void endJump(IRBlock& block, const std::string& target) {
    block.terminated = true;
    block.termKind = "跳转";
    block.termTarget = target;
}

// 构造尾递归函数：
//   函数 递减（单参数 n）：入口 bb0（无参数 Store）；
//                           尾块 bb1: Load n$0 -> %v1; Sub %v1 1 -> %v2;
//                                    Call 递减(%v2) -> %v3; 返回 %v3
//   入口块跳转到尾块（简化：直接尾块返回）
IRModule makeTailRecModule() {
    IRModule module;
    IRFunction fn;
    fn.name = "递减";
    fn.returnType = "i32";
    fn.params = {{"n", "i32"}};
    fn.paramUniques = {"n$0"};
    auto b0 = makeBlock("bb0", {});
    endJump(*b0, "bb1");
    IRInstruction recCall =
        makeInst(Opcode::Call, {IRValue::reg(2, "i32")},
                 IRValue::reg(3, "i32"), "i32");
    recCall.extra = "递减";  // 尾递归自调用（符号名存 extra，与 IRGenerator 契约一致）
    auto b1 = makeBlock("bb1", {
        makeInst(Opcode::Load, {IRValue::var("n$0", "i32")},
                 IRValue::reg(1, "i32"), "i32"),
        makeInst(Opcode::Sub, {IRValue::reg(1, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(2, "i32"), "i32"),
        std::move(recCall),
    });
    endReturn(*b1, "%v3");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));
    return module;
}

// 检查函数某块终止是否为跳转
bool isJump(const IRModule& module, std::size_t blockIndex,
            const std::string& target) {
    const auto& block = *module.functions[0].blocks[blockIndex];
    return block.terminated && block.termKind == "跳转" &&
           block.termTarget == target;
}

// 统计函数内 Call 指令数
int countCall(const IRModule& module) {
    int count = 0;
    for (const auto& block : module.functions[0].blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call) ++count;
        }
    }
    return count;
}

} // namespace

// ==================== 尾递归 -> 跳回入口 ====================

// 尾递归调用被消除：Call 删除，尾块终止变为跳转 bb0，参数槽 Store 出现
TEST(TailCallTest, TailRecursionConvertedToJump) {
    auto module = makeTailRecModule();
    EXPECT_TRUE(TailCallPass().run(module));
    // 函数内不再有 Call（自调用被消除）
    EXPECT_EQ(countCall(module), 0);
    // 尾块终止为跳转回入口 bb0
    EXPECT_TRUE(isJump(module, 1, "bb0"));
    // 尾块内出现参数槽 Store（n$0 <- %v2）
    const auto& b1 = *module.functions[0].blocks[1];
    bool hasParamStore = false;
    for (const auto& inst : b1.instructions) {
        if (inst.opcode == Opcode::Store && inst.extra == "n$0") hasParamStore = true;
    }
    EXPECT_TRUE(hasParamStore);
}

// 幂等：改写后无 Call 可匹配，第二次运行无修改
TEST(TailCallTest, Idempotent) {
    auto module = makeTailRecModule();
    TailCallPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_FALSE(pass.run(module));
}

// ==================== 保守场景（不优化） ====================

// 非自调用（尾调用其他函数）不优化
TEST(TailCallTest, NonSelfCallNotOptimized) {
    IRModule module;
    IRFunction fn;
    fn.name = "甲";
    fn.returnType = "i32";
    fn.params = {{"n", "i32"}};
    fn.paramUniques = {"n$0"};
    auto b0 = makeBlock("bb0", {});
    endJump(*b0, "bb1");
    IRInstruction otherCall =
        makeInst(Opcode::Call, {IRValue::reg(1, "i32")},
                 IRValue::reg(2, "i32"), "i32");
    otherCall.extra = "乙";  // 尾调用其他函数（非自调用）
    auto b1 = makeBlock("bb1", {
        makeInst(Opcode::ConstInt, {}, IRValue::reg(1, "i32"), "i32"),
        std::move(otherCall),
    });
    endReturn(*b1, "%v2");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));
    EXPECT_FALSE(TailCallPass().run(module));
    EXPECT_EQ(countCall(module), 1);
}

// 入口块对参数槽初始 Store：不优化（跳回会覆盖新参数）
TEST(TailCallTest, EntryStoresParamNotOptimized) {
    IRModule module;
    IRFunction fn;
    fn.name = "递减";
    fn.returnType = "i32";
    fn.params = {{"n", "i32"}};
    fn.paramUniques = {"n$0"};
    // 入口块对参数槽初始 Store（覆盖参数默认值）
    IRInstruction initStore =
        makeInst(Opcode::Store, {IRValue::constant("0", "i32")}, IRValue(), "i32");
    initStore.extra = "n$0";  // Store 槽名存 extra（与 IRGenerator 契约一致）
    auto b0 = makeBlock("bb0", {std::move(initStore)});
    endJump(*b0, "bb1");
    IRInstruction recCall2 =
        makeInst(Opcode::Call, {IRValue::reg(1, "i32")},
                 IRValue::reg(2, "i32"), "i32");
    recCall2.extra = "递减";
    auto b1 = makeBlock("bb1", {
        makeInst(Opcode::ConstInt, {}, IRValue::reg(1, "i32"), "i32"),
        std::move(recCall2),
    });
    endReturn(*b1, "%v2");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));
    EXPECT_FALSE(TailCallPass().run(module));
    EXPECT_EQ(countCall(module), 1);
}

// 实参为嵌套调用结果：不优化（删除 Call 会破坏嵌套调用语义）
TEST(TailCallTest, NestedCallArgNotOptimized) {
    IRModule module;
    IRFunction fn;
    fn.name = "递减";
    fn.returnType = "i32";
    fn.params = {{"n", "i32"}};
    fn.paramUniques = {"n$0"};
    auto b0 = makeBlock("bb0", {});
    endJump(*b0, "bb1");
    // 尾块：Call 辅助(%v1) -> %v2（嵌套调用）；Call 递减(%v2) -> %v3；返回 %v3
    IRInstruction auxCall =
        makeInst(Opcode::Call, {IRValue::reg(1, "i32")},
                 IRValue::reg(2, "i32"), "i32");
    auxCall.extra = "辅助";
    IRInstruction recCall3 =
        makeInst(Opcode::Call, {IRValue::reg(2, "i32")},
                 IRValue::reg(3, "i32"), "i32");
    recCall3.extra = "递减";
    auto b1 = makeBlock("bb1", {
        makeInst(Opcode::ConstInt, {}, IRValue::reg(1, "i32"), "i32"),
        std::move(auxCall),
        std::move(recCall3),
    });
    endReturn(*b1, "%v3");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));
    // 实参 %v2 由块内 Call 定义 -> 保守跳过
    EXPECT_FALSE(TailCallPass().run(module));
    EXPECT_EQ(countCall(module), 2);
}

// 实参数量与参数数量不匹配：不优化
TEST(TailCallTest, ArgCountMismatchNotOptimized) {
    IRModule module;
    IRFunction fn;
    fn.name = "双参";
    fn.returnType = "i32";
    fn.params = {{"a", "i32"}, {"b", "i32"}};
    fn.paramUniques = {"a$0", "b$0"};
    auto b0 = makeBlock("bb0", {});
    endJump(*b0, "bb1");
    IRInstruction mismatchCall =
        makeInst(Opcode::Call, {IRValue::reg(1, "i32")},
                 IRValue::reg(2, "i32"), "i32");
    mismatchCall.extra = "双参";  // 实参数量不匹配（应 2 个实参）
    auto b1 = makeBlock("bb1", {
        makeInst(Opcode::ConstInt, {}, IRValue::reg(1, "i32"), "i32"),
        std::move(mismatchCall),
    });
    endReturn(*b1, "%v2");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));
    EXPECT_FALSE(TailCallPass().run(module));
    EXPECT_EQ(countCall(module), 1);
}
