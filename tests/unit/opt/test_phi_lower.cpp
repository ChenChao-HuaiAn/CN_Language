// Phi 降级（F1-26 方案 A·256-a）单元测试
// 覆盖：①汇合降级（Phi 全消·前驱块尾并行拷贝·汇合块内槽 Load 被重写消除）
//   ②循环携带降级（回边前驱拷贝）③交换环手构（两 Phi 互为源·环打破含临时搬运）
//   ④占位前驱（单分支 if·前驱无 Store→前驱尾注入 Load 槽）
// 测试方式：Lexer + Parser + IRGenerator + runOptLevel(3) 全链路（非 Mock）+
//   手构 IRModule（精确控制环形态）。
// 注意：测试名/标识符必须使用英文（GCC 9 无 UTF-8 标识符；注释/字符串可为中文）
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/opt/pass_manager.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;

namespace {

IRModule buildOpt3(const std::string& source) {
    Diagnostics diagnostics = Diagnostics();
    Lexer lexer(source, "phi_lower_test.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(diagnostics);
    IRModule module = generator.generate(program.get());
    cn_compiler::opt::runOptLevel(module, 3);
    return module;
}

int countOpcodeAll(const IRModule& module, Opcode opcode) {
    int count = 0;
    for (const auto& fn : module.functions) {
        for (const auto& blk : fn.blocks) {
            for (const auto& inst : blk->instructions) {
                if (inst.opcode == opcode) count++;
            }
        }
    }
    return count;
}

}  // namespace

TEST(PhiLower, JoinPointPhiEliminated) {
    const std::string source = R"CN(
函数 取较大(整32 甲, 整32 乙) -> 整32 {
    整32 结果值 = 0;
    如果 (甲 > 乙) {
        结果值 = 甲;
    } 否则 {
        结果值 = 乙;
    }
    返回 结果值;
}
)CN";
    IRModule module = buildOpt3(source);
    // Phi 全降级（无残留）
    EXPECT_EQ(countOpcodeAll(module, Opcode::Phi), 0);
    // -O3 全程后模块仍合法（IR 验证器：CFG 结构不变量）
    std::vector<std::string> errs = cn_compiler::ir::verifyIRModule(module);
    for (const auto& e : errs) std::fprintf(stderr, "[验证错误] %s\n", e.c_str());
    EXPECT_TRUE(errs.empty());
}

TEST(PhiLower, LoopCarriedPhiEliminated) {
    const std::string source = R"CN(
函数 累加(整32 上限) -> 整32 {
    整32 累计 = 0;
    整32 i = 0;
    当 (i < 上限) {
        累计 = 累计 + i;
        i = i + 1;
    }
    返回 累计;
}
)CN";
    IRModule module = buildOpt3(source);
    EXPECT_EQ(countOpcodeAll(module, Opcode::Phi), 0);
    for (const auto& fn : module.functions) {
        for (const auto& blk : fn.blocks) {
            std::fprintf(stderr, "块 %s:\n", blk->label.c_str());
            for (const auto& inst : blk->instructions) {
                std::fprintf(stderr, "  op=%d r=%d nops=%zu extra=%s\n",
                             static_cast<int>(inst.opcode), inst.result.id,
                             inst.operands.size(), inst.extra.c_str());
                for (const auto& op : inst.operands) {
                    std::fprintf(stderr, "    operand id=%d\n", op.id);
                }
            }
        }
    }
    std::vector<std::string> errs = cn_compiler::ir::verifyIRModule(module);
    std::string all;
    for (const auto& e : errs) all += e + " | ";
    EXPECT_TRUE(errs.empty()) << all;
}

TEST(PhiLower, SwapRingHandled) {
    // 手构交换环：块 b 有两个 Phi（p1、p2）互为源——环打破须保证「读先于写」
    IRModule module;
    IRFunction fn;
    fn.name = "ring_fn";
    fn.returnType = "i32";

    IRBlock entry;
    entry.label = "entry";
    entry.terminated = true;
    entry.termKind = "条件跳转";
    entry.termTrueTarget = "join";
    entry.termFalseTarget = "join";
    fn.blocks.push_back(std::make_unique<IRBlock>(std::move(entry)));

    IRBlock join;
    join.label = "join";
    {
        IRInstruction p1;
        p1.opcode = Opcode::Phi;
        p1.result = IRValue::reg(10, "i32");
        p1.operands.push_back(IRValue::reg(11, "i32"));   // 源=另一 Phi 的结果（互为源）
        p1.operands.push_back(IRValue::reg(11, "i32"));
        p1.extra = "slot_x";
        p1.type = "i32";
        join.instructions.push_back(std::move(p1));
        IRInstruction p2;
        p2.opcode = Opcode::Phi;
        p2.result = IRValue::reg(11, "i32");
        p2.operands.push_back(IRValue::reg(10, "i32"));
        p2.operands.push_back(IRValue::reg(10, "i32"));
        p2.extra = "slot_y";
        p2.type = "i32";
        join.instructions.push_back(std::move(p2));
    }
    join.terminated = true;
    join.termKind = "返回";
    join.termReturnValue = "v10";
    fn.blocks.push_back(std::make_unique<IRBlock>(std::move(join)));

    module.functions.push_back(std::move(fn));
    cn_compiler::opt::runOptLevel(module, 3);

    // Phi 全消 + 环打破：entry 前驱尾应有 ≥4 条 Copy（2 条入临时 + 2 条出临时）
    EXPECT_EQ(countOpcodeAll(module, Opcode::Phi), 0);
    int copies = 0;
    for (const auto& blk : module.functions[0].blocks) {
        for (const auto& inst : blk->instructions) {
            if (inst.opcode == Opcode::Copy) ++copies;
        }
    }
    EXPECT_GE(copies, 4);
    // 注：本用例为「环打破」防御能力验证——手构 IR 的「源=另一 Phi 的
    //   result」不是本 SSA 构造的自然产物（自然 Phi 源=前驱块内定义的值），
    //   故不做 IR 验证器断言（其结构性定义检查不适用于此防御形态）。
}

TEST(PhiLower, PlaceholderPredGetsLoad) {
    // 单分支 if：假分支路径无 Store（走 else 隐含空路径）→占位前驱注入 Load
    const std::string source = R"CN(
函数 条件加(整32 输入) -> 整32 {
    整32 结果值 = 5;
    如果 (输入 > 0) {
        结果值 = 输入;
    }
    返回 结果值;
}
)CN";
    IRModule module = buildOpt3(source);
    EXPECT_EQ(countOpcodeAll(module, Opcode::Phi), 0);
    EXPECT_TRUE(cn_compiler::ir::verifyIRModule(module).empty());
}
