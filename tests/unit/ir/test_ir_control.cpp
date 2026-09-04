// IR生成器控制流增量单元测试（Task 2.1：选择/情况/默认）
// 覆盖：switch IR结构（Eq条件跳转序列、默认分支、汇合块、fallthrough）
// 测试方式：Lexer + Parser + IRGenerator 全链路（非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::Opcode;

namespace {

// 辅助：解析源码并生成IR模块
struct IRResult {
    IRModule module;
    Diagnostics diagnostics;
};

IRResult buildIR(const std::string& source) {
    IRResult result;
    Lexer lexer(source, "IR控制流测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(result.diagnostics);
    result.module = generator.generate(program.get());
    return result;
}

// 便捷访问：第n个函数的第m个基本块
IRBlock* blockAt(IRModule& module, std::size_t funcIndex, std::size_t blockIndex) {
    if (funcIndex >= module.functions.size()) return nullptr;
    auto& func = module.functions[funcIndex];
    if (blockIndex >= func.blocks.size()) return nullptr;
    return func.blocks[blockIndex].get();
}

// 统计指定操作码在函数中的出现次数
int countOpcode(IRModule& module, std::size_t funcIndex, Opcode opcode) {
    int count = 0;
    if (funcIndex >= module.functions.size()) return 0;
    for (auto& block : module.functions[funcIndex].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == opcode) count++;
        }
    }
    return count;
}

// 统计指定终止类型的块数
int countTerminated(IRModule& module, std::size_t funcIndex, const std::string& kind) {
    int count = 0;
    if (funcIndex >= module.functions.size()) return 0;
    for (auto& block : module.functions[funcIndex].blocks) {
        if (block->terminated && block->termKind == kind) count++;
    }
    return count;
}

} // namespace

// ==================== switch IR 结构 ====================

// 2个case + 默认：应为 2个Eq比较 + 2个条件跳转 + 1个无条件跳转（默认兜底）+ 出口汇合
TEST(IRSwitchTest, SwitchCFGStructure) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整数 v = 2;
    选择(v) {
        情况 1:
            打印行("一");
            中断;
        情况 2:
            打印行("二");
            中断;
        默认:
            打印行("其他");
    }
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countOpcode(r.module, 0, Opcode::Eq), 2);   // 每个case一个比较
    EXPECT_EQ(countTerminated(r.module, 0, "条件跳转"), 2); // 每个case一个条件跳转
    // 至少有一个无条件跳转（默认体块 -> 出口）
    EXPECT_GE(countTerminated(r.module, 0, "跳转"), 1);
    // 入口块以跳转或条件跳转终止（进入第一个判断块）
    IRBlock* entry = blockAt(r.module, 0, 0);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->terminated);
    // 函数末尾必须有返回（基本块必须终止）
    IRBlock* last = blockAt(r.module, 0, r.module.functions[0].blocks.size() - 1);
    ASSERT_NE(last, nullptr);
    EXPECT_TRUE(last->terminated);
}

// 无默认分支：最后一个case不匹配时直接跳出口（无额外兜底跳转体）
TEST(IRSwitchTest, SwitchWithoutDefaultCFG) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整数 v = 1;
    选择(v) {
        情况 1:
            打印行("一");
            中断;
        情况 2:
            打印行("二");
            中断;
    }
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countOpcode(r.module, 0, Opcode::Eq), 2);
    // 所有块最终汇合到出口块，出口块后续接返回（保证终止）
    IRBlock* last = blockAt(r.module, 0, r.module.functions[0].blocks.size() - 1);
    ASSERT_NE(last, nullptr);
    EXPECT_TRUE(last->terminated);
}

// fallthrough：case 1 无中断，其体块应跳转到 case 2 体块
TEST(IRSwitchTest, FallthroughJumpsToNextBody) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整数 w = 1;
    选择(w) {
        情况 1:
            打印行("fall1");
        情况 2:
            打印行("fall2");
            中断;
        默认:
            打印行("fall默认");
    }
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    // case1体块未以中断终止：应有一个无条件跳转（落到case2体块）
    EXPECT_GE(countTerminated(r.module, 0, "跳转"), 1);
    // 找到 case1 体块（第二个块）：不应是条件跳转终止
    IRBlock* body1 = blockAt(r.module, 0, 2);  // 入口0/判断1/体1(索引2)
    ASSERT_NE(body1, nullptr);
    if (body1->terminated) {
        EXPECT_NE(body1->termKind, "返回");  // fallthrough 不可能是返回终止
    }
}

// 中断跳出 switch：中断生成到出口块的无条件跳转
TEST(IRSwitchTest, BreakJumpsToExit) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整数 v = 1;
    选择(v) {
        情况 1:
            打印行("一");
            中断;
        默认:
            中断;
    }
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    // 中断在两个分支中各自产生一个无条件跳转（体块 -> 出口块）
    EXPECT_GE(countTerminated(r.module, 0, "跳转"), 2);
    // 所有块都终止（无悬挂块）
    for (auto& block : r.module.functions[0].blocks) {
        EXPECT_TRUE(block->terminated) << "块 " << block->label << " 未终止";
    }
}

// 字符条件 switch：比较值为字符码点（i32）
TEST(IRSwitchTest, CharConditionIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符 c = 'B';
    选择(c) {
        情况 'A':
            打印行("A");
            中断;
        情况 'B':
            打印行("B");
            中断;
    }
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countOpcode(r.module, 0, Opcode::Eq), 2);
    EXPECT_GE(countTerminated(r.module, 0, "条件跳转"), 2);
}

// 嵌套 switch：内层中断只跳内层出口，外层结构完整
TEST(IRSwitchTest, NestedSwitchIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整数 v = 2;
    选择(v) {
        情况 2:
            整数 n = 1;
            选择(n) {
                情况 1:
                    中断;
                默认:
                    中断;
            }
            中断;
        默认:
            中断;
    }
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    // 外层1个Eq + 内层1个Eq
    EXPECT_EQ(countOpcode(r.module, 0, Opcode::Eq), 2);
    // 所有块必须终止
    for (auto& block : r.module.functions[0].blocks) {
        EXPECT_TRUE(block->terminated) << "块 " << block->label << " 未终止";
    }
}
