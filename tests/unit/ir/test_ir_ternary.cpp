// 三元表达式 IR 生成单元测试（Task 2.9）
// 覆盖：惰性求值 CFG（条件跳转 + 真/假分支块 + 汇合块）、临时槽 Store/Load、
//       字符串+数值拼接展开（__cn_str_from_* + __cn_str_concat）、格式化展开（__cn_format）
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
    Lexer lexer(source, "IR三元测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(result.diagnostics);
    result.module = generator.generate(program.get());
    return result;
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

// 三元 IR：条件跳转（惰性）——应有 1 个条件跳转终止块 + 2 个无条件跳转 + 临时槽 Store×2 + Load×1
TEST(IRTernaryTest, LazyCfgStructure) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 t = x > 0 ? 100 : 200;
    返回 0;
}
)CN");
    ASSERT_FALSE(r.module.functions.empty());
    // 条件跳转：1（条件分支）
    EXPECT_EQ(countTerminated(r.module, 0, "条件跳转"), 1);
    // 无条件跳转：2（真块 -> 汇合、假块 -> 汇合）
    EXPECT_EQ(countTerminated(r.module, 0, "跳转"), 2);
    // 临时槽 Store（真/假各1）+ 汇合 Load（1）
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Store), 2);
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Load), 1);
    // 基本块数：入口 + 真 + 假 + 汇合 = 4（含函数入口 bb0）
    EXPECT_GE(r.module.functions[0].blocks.size(), 4u);
}

// 三元嵌套（右结合）：外层三元 + 内层三元均生成条件跳转
TEST(IRTernaryTest, NestedTernaryCfg) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 t = 假 ? "一" : 假 ? "二" : "三";
    打印(t);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.module.functions.empty());
    // 两个三元 -> 2 个条件跳转
    EXPECT_EQ(countTerminated(r.module, 0, "条件跳转"), 2);
}

// 字符串 + 数值拼接：展开 __cn_str_from_int + __cn_str_concat
TEST(IRTernaryTest, StringPlusNumberExpand) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印("值" + 42);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.module.functions.empty());
    // __cn_str_from_int 调用 1 次 + __cn_str_concat 1 次
    int fromInt = 0, concat = 0;
    for (auto& block : r.module.functions[0].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call) {
                if (inst.extra == "__cn_str_from_int") fromInt++;
                if (inst.extra == "__cn_str_concat") concat++;
            }
        }
    }
    EXPECT_EQ(fromInt, 1);
    EXPECT_EQ(concat, 1);
}

// 字符串 + 布尔拼接：展开 __cn_str_from_bool
TEST(IRTernaryTest, StringPlusBoolExpand) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印("布尔=" + 真);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.module.functions.empty());
    int fromBool = 0, concat = 0;
    for (auto& block : r.module.functions[0].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call) {
                if (inst.extra == "__cn_str_from_bool") fromBool++;
                if (inst.extra == "__cn_str_concat") concat++;
            }
        }
    }
    EXPECT_EQ(fromBool, 1);
    EXPECT_EQ(concat, 1);
}

// 格式化展开：__cn_format 调用（保留原始格式字符串 + 参数）
TEST(IRTernaryTest, FormatExpand) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 s = 格式化("值%d 浮点%f", 42, 3.14);
    字符串释放(s);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.module.functions.empty());
    int format = 0;
    for (auto& block : r.module.functions[0].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == "__cn_format") format++;
        }
    }
    EXPECT_EQ(format, 1);
}
