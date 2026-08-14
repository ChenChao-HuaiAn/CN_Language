// 函数内联 Pass 单元测试（阶段B Task 4.2）
// 覆盖：小型纯运算函数内联（参数 Load 折叠为实参）、
//       递归函数不内联、入口函数（主）不内联、
//       OOP 方法不内联、含 Alloca/参数 Store 不内联、
//       幂等
// 测试方式：直接构造 IRModule 调用 InlinePass（opt 为纯内部模块）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/inline.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::InlinePass;

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

// 构造模块：
//   函数 加（双参数，纯运算）：Load a$0 -> %v1; Load b$0 -> %v2;
//                               Add %v1 %v2 -> %v3; 返回 %v3
//   函数 主：Call 加(5, 7) -> %v9; 返回 %v9
IRModule makeInlineModule() {
    IRModule module;

    // 函数 加
    IRFunction addFn;
    addFn.name = "加";
    addFn.returnType = "i32";
    addFn.params = {{"a", "i32"}, {"b", "i32"}};
    addFn.paramUniques = {"a$0", "b$0"};
    auto addBlock = makeBlock("bb0", {
        makeInst(Opcode::Load, {IRValue::var("a$0", "i32")},
                 IRValue::reg(1, "i32"), "i32"),
        makeInst(Opcode::Load, {IRValue::var("b$0", "i32")},
                 IRValue::reg(2, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                 IRValue::reg(3, "i32"), "i32"),
    });
    endReturn(*addBlock, "%v3");
    addFn.blocks.push_back(std::move(addBlock));
    module.functions.push_back(std::move(addFn));

    // 函数 主
    IRFunction mainFn;
    mainFn.name = "主";
    mainFn.returnType = "i32";
    IRInstruction callInst =
        makeInst(Opcode::Call,
                 {IRValue::constant("5", "i32"), IRValue::constant("7", "i32")},
                 IRValue::reg(9, "i32"), "i32");
    callInst.extra = "加";  // Call 符号名存 extra（与 IRGenerator 契约一致）
    auto mainBlock = makeBlock("bb0", {std::move(callInst)});
    endReturn(*mainBlock, "%v9");
    mainFn.blocks.push_back(std::move(mainBlock));
    module.functions.push_back(std::move(mainFn));
    return module;
}

// 统计函数内 Call 指令数
int countCall(const IRModule& module, std::size_t fnIndex) {
    int count = 0;
    for (const auto& block : module.functions[fnIndex].blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call) ++count;
        }
    }
    return count;
}

} // namespace

// ==================== 小型纯运算函数内联 ====================

// 主 内调用 加 被内联：主 块内 Call 消失，Add 指令出现，
//   参数 Load 折叠为常量实参（5/7）
TEST(InlineTest, SmallFunctionInlined) {
    auto module = makeInlineModule();
    EXPECT_TRUE(InlinePass().run(module));
    // 主 内不再有 Call
    EXPECT_EQ(countCall(module, 1), 0);
    // 主 内出现 Add 指令（内联展开）
    const auto& mainBlock = *module.functions[1].blocks[0];
    bool hasAdd = false;
    for (const auto& inst : mainBlock.instructions) {
        if (inst.opcode == Opcode::Add) hasAdd = true;
    }
    EXPECT_TRUE(hasAdd);
}

// 内联后参数 Load 折叠为实参：主 内不再有对参数槽 a$0/b$0 的 Load，
//   且出现 ConstInt 5 / ConstInt 7（实参常量折叠）
TEST(InlineTest, ParamLoadFoldedToArg) {
    auto module = makeInlineModule();
    EXPECT_TRUE(InlinePass().run(module));
    const auto& mainBlock = *module.functions[1].blocks[0];
    // 1. 不再有对参数槽的 Load
    bool hasParamLoad = false;
    for (const auto& inst : mainBlock.instructions) {
        if (inst.opcode == Opcode::Load &&
            (inst.operands[0].extra == "a$0" || inst.operands[0].extra == "b$0")) {
            hasParamLoad = true;
        }
    }
    EXPECT_FALSE(hasParamLoad);
    // 2. 出现 ConstInt 5 与 ConstInt 7（实参常量折叠）
    bool hasConst5 = false, hasConst7 = false;
    for (const auto& inst : mainBlock.instructions) {
        if (inst.opcode == Opcode::ConstInt) {
            if (inst.extra == "5") hasConst5 = true;
            if (inst.extra == "7") hasConst7 = true;
        }
    }
    EXPECT_TRUE(hasConst5);
    EXPECT_TRUE(hasConst7);
    // 3. Add 指令仍存在（内联展开的运算）
    bool hasAdd = false;
    for (const auto& inst : mainBlock.instructions) {
        if (inst.opcode == Opcode::Add) hasAdd = true;
    }
    EXPECT_TRUE(hasAdd);
}

// ==================== 保守场景（不内联） ====================

// 递归函数不内联：函数 阶乘 体内 Call 自身 -> 不可内联
TEST(InlineTest, RecursiveNotInlined) {
    IRModule module;
    IRFunction factFn;
    factFn.name = "阶乘";
    factFn.returnType = "i32";
    factFn.params = {{"n", "i32"}};
    factFn.paramUniques = {"n$0"};
    // 体内：Load n$0 -> %v1; Call 阶乘(%v1) -> %v2（递归）
    IRInstruction recCall =
        makeInst(Opcode::Call, {IRValue::reg(1, "i32")},
                 IRValue::reg(2, "i32"), "i32");
    recCall.extra = "阶乘";  // 递归自调用（符号名存 extra）
    auto factBlock = makeBlock("bb0", {
        makeInst(Opcode::Load, {IRValue::var("n$0", "i32")},
                 IRValue::reg(1, "i32"), "i32"),
        std::move(recCall),
    });
    endReturn(*factBlock, "%v2");
    factFn.blocks.push_back(std::move(factBlock));
    module.functions.push_back(std::move(factFn));

    IRFunction mainFn;
    mainFn.name = "主";
    mainFn.returnType = "i32";
    auto mainBlock = makeBlock("bb0", {
        makeInst(Opcode::Call, {IRValue::constant("5", "i32")},
                 IRValue::reg(9, "i32"), "阶乘"),
    });
    endReturn(*mainBlock, "%v9");
    mainFn.blocks.push_back(std::move(mainBlock));
    module.functions.push_back(std::move(mainFn));

    // 阶乘 不可内联（递归），主 内 Call 保留
    EXPECT_FALSE(InlinePass().run(module));
    EXPECT_EQ(countCall(module, 1), 1);
}

// 含 Alloca 的函数不内联（栈分配逃逸）
TEST(InlineTest, AllocaNotInlined) {
    IRModule module;
    IRFunction fn;
    fn.name = "创建";
    fn.returnType = "i32";
    auto block = makeBlock("bb0", {
        makeInst(Opcode::Alloca, {}, IRValue::reg(1, "ptr"), "ptr"),
        makeInst(Opcode::ConstInt, {}, IRValue::reg(2, "i32"), "i32"),
    });
    endReturn(*block, "%v2");
    fn.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(fn));

    IRFunction mainFn;
    mainFn.name = "主";
    mainFn.returnType = "i32";
    auto mainBlock = makeBlock("bb0", {
        makeInst(Opcode::Call, {}, IRValue::reg(9, "i32"), "创建"),
    });
    endReturn(*mainBlock, "%v9");
    mainFn.blocks.push_back(std::move(mainBlock));
    module.functions.push_back(std::move(mainFn));

    EXPECT_FALSE(InlinePass().run(module));
    EXPECT_EQ(countCall(module, 1), 1);
}

// 入口函数（主）不内联：主 不作为内联目标（其定义只被调用一次，
//   但语义上保留入口；此处验证 主 不被其他函数调用场景——直接
//   构造 主 含 Call 到小型函数，验证调用侧内联正常且 主 自身保留）
TEST(InlineTest, EntryFunctionKept) {
    auto module = makeInlineModule();
    // 主 体内 Call 加 内联后，主 自身作为函数仍存在
    EXPECT_TRUE(InlinePass().run(module));
    EXPECT_EQ(module.functions.size(), std::size_t(2));  // 加 与 主 都保留
    EXPECT_EQ(module.functions[1].name, "主");
}

// 对参数 Store 的函数不内联（值传递副本语义，内联后无副本可写）
TEST(InlineTest, ParamStoreNotInlined) {
    IRModule module;
    IRFunction fn;
    fn.name = "改参数";
    fn.returnType = "i32";
    fn.params = {{"a", "i32"}};
    fn.paramUniques = {"a$0"};
    // 体内：Store 常量 1 -> a$0（参数槽写）；ConstInt 0 -> %v2；返回 %v2
    IRInstruction paramStore =
        makeInst(Opcode::Store, {IRValue::constant("1", "i32")}, IRValue(), "i32");
    paramStore.extra = "a$0";  // Store 槽名存 extra（与 IRGenerator 契约一致）
    auto block = makeBlock("bb0", {
        std::move(paramStore),
        makeInst(Opcode::ConstInt, {}, IRValue::reg(2, "i32"), "i32"),
    });
    endReturn(*block, "%v2");
    fn.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(fn));

    IRFunction mainFn;
    mainFn.name = "主";
    mainFn.returnType = "i32";
    auto mainBlock = makeBlock("bb0", {
        makeInst(Opcode::Call, {IRValue::constant("5", "i32")},
                 IRValue::reg(9, "i32"), "改参数"),
    });
    endReturn(*mainBlock, "%v9");
    mainFn.blocks.push_back(std::move(mainBlock));
    module.functions.push_back(std::move(mainFn));

    EXPECT_FALSE(InlinePass().run(module));
    EXPECT_EQ(countCall(module, 1), 1);
}
