// IR生成器单元测试（Task 1.6）
// 覆盖：函数IR生成、表达式IR生成（算术/比较/逻辑）、变量声明IR生成、
//       控制流IR生成（如果/循环）、函数调用IR生成、字符串常量收集、完整程序IR生成
// 测试方式：通过 Lexer + Parser 得到真实AST，交给 IRGenerator 生成IR（全链路）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

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
    Lexer lexer(source, "IR测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(result.diagnostics);
    result.module = generator.generate(program.get());
    return result;
}

// 便捷访问：第n个函数的第m个基本块
cn_compiler::ir::IRBlock* blockAt(IRModule& module, std::size_t funcIndex,
                                  std::size_t blockIndex) {
    if (funcIndex >= module.functions.size()) return nullptr;
    auto& func = module.functions[funcIndex];
    if (blockIndex >= func.blocks.size()) return nullptr;
    return func.blocks[blockIndex].get();
}

// 便捷访问：第n个函数的第m个块的指令序列
std::vector<cn_compiler::ir::IRInstruction>& insts(IRModule& module, std::size_t funcIndex,
                                                   std::size_t blockIndex) {
    return blockAt(module, funcIndex, blockIndex)->instructions;
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

} // namespace

// ==================== 函数IR生成 ====================

// 函数定义：名称/返回类型/参数列表
TEST(IRTest, FunctionIR) {
    auto r = buildIR(R"CN(
函数 加(整32 a, 整32 b) -> 整32 {
    返回 a + b
}
)CN");
    ASSERT_EQ(r.module.functions.size(), 1u);
    EXPECT_EQ(r.module.functions[0].name, "加");
    EXPECT_EQ(r.module.functions[0].returnType, "i32");
    ASSERT_EQ(r.module.functions[0].params.size(), 2u);
    EXPECT_EQ(r.module.functions[0].params[0].first, "a");
    EXPECT_EQ(r.module.functions[0].params[0].second, "i32");
    EXPECT_EQ(r.module.functions[0].params[1].first, "b");
    EXPECT_EQ(r.module.functions[0].params[1].second, "i32");
    // 至少一个基本块，返回终止
    ASSERT_GE(r.module.functions[0].blocks.size(), 1u);
    auto& lastBlock = *r.module.functions[0].blocks.back();
    EXPECT_TRUE(lastBlock.terminated);
    EXPECT_EQ(lastBlock.termKind, "返回");
}

// 空类型函数默认返回
TEST(IRTest, VoidFunctionDefaultReturn) {
    auto r = buildIR(R"CN(
函数 打印() {
    返回
}
)CN");
    ASSERT_EQ(r.module.functions.size(), 1u);
    EXPECT_EQ(r.module.functions[0].returnType, "void");
    ASSERT_GE(r.module.functions[0].blocks.size(), 1u);
    auto& lastBlock = *r.module.functions[0].blocks.back();
    EXPECT_TRUE(lastBlock.terminated);
    EXPECT_EQ(lastBlock.termKind, "返回");
}

// ==================== 表达式IR生成 ====================

// 整数字面量 -> ConstInt
TEST(IRTest, IntegerLiteralIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    返回 42
}
)CN");
    auto& inst = insts(r.module, 0, 0);
    ASSERT_GE(inst.size(), 1u);
    EXPECT_EQ(inst[0].opcode, Opcode::ConstInt);
    EXPECT_EQ(inst[0].extra, "42");
    EXPECT_EQ(inst[0].type, "i32");
}

// 浮点字面量 -> ConstFloat
TEST(IRTest, FloatLiteralIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    返回 3.14
}
)CN");
    auto& inst = insts(r.module, 0, 0);
    ASSERT_GE(inst.size(), 1u);
    EXPECT_EQ(inst[0].opcode, Opcode::ConstFloat);
    EXPECT_EQ(inst[0].type, "f64");
}

// 算术运算 -> Add/Sub/Mul/Div/Mod
TEST(IRTest, ArithmeticIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 x = 1 + 2 * 3
    返回 x
}
)CN");
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Add), 1);
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Mul), 1);
}

// 比较运算 -> Eq/Lt，结果为i1
TEST(IRTest, ComparisonIR) {
    auto r = buildIR(R"CN(
函数 主() -> 布尔 {
    返回 1 < 2
}
)CN");
    auto& inst = insts(r.module, 0, 0);
    bool foundLt = false;
    for (auto& i : inst) {
        if (i.opcode == Opcode::Lt) {
            foundLt = true;
            EXPECT_EQ(i.type, "i1");
        }
    }
    EXPECT_TRUE(foundLt);
}

// 逻辑运算 -> And/Or/Not
TEST(IRTest, LogicalIR) {
    auto r = buildIR(R"CN(
函数 主() -> 布尔 {
    返回 真 && 假 || !真
}
)CN");
    EXPECT_GE(countOpcode(r.module, 0, Opcode::And), 1);
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Or), 1);
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Not), 1);
}

// ==================== 变量声明IR生成 ====================

// 变量声明 -> Alloca + Store
TEST(IRTest, VarDeclIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 x = 10
    返回 x
}
)CN");
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Alloca), 1);
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Store), 1);
    // 变量引用生成 Load
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Load), 1);
}

// 变量引用与赋值 -> Load + Store
TEST(IRTest, VarAssignIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 x = 1
    x = x + 1
    返回 x
}
)CN");
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Load), 1);
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Store), 2);
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Add), 1);
}

// ==================== 控制流IR生成 ====================

// 如果语句 -> 分支块 + 跳转
TEST(IRTest, IfStmtIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 x = 0
    如果 (x > 0) {
        x = 1
    } 否则 {
        x = 2
    }
    返回 x
}
)CN");
    // 应生成多个基本块（入口/真/假/汇合）
    ASSERT_GE(r.module.functions[0].blocks.size(), 4u);
    bool hasBranch = false;
    for (auto& block : r.module.functions[0].blocks) {
        if (block->termKind == "条件跳转") hasBranch = true;
        if (block->termKind == "跳转") {
            EXPECT_FALSE(block->termTarget.empty());
        }
    }
    EXPECT_TRUE(hasBranch);
}

// 当循环 -> 条件块/循环体/出口块
TEST(IRTest, WhileLoopIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 i = 0
    当 (i < 10) {
        i = i + 1
    }
    返回 i
}
)CN");
    ASSERT_GE(r.module.functions[0].blocks.size(), 3u);
    bool hasBackEdge = false;
    for (auto& block : r.module.functions[0].blocks) {
        if (block->termKind == "跳转") {
            hasBackEdge = true;
        }
    }
    EXPECT_TRUE(hasBackEdge);
}

// 循环语句（for风格） -> 多块结构
TEST(IRTest, ForLoopIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    变量 总和 = 0
    循环 (变量 i = 0; i < 10; i = i + 1) {
        如果 (i == 5) {
            中断
        }
        总和 = 总和 + i
    }
    返回 总和
}
)CN");
    ASSERT_GE(r.module.functions[0].blocks.size(), 5u);
    // 中断跳转存在（某个块跳转到出口）
    bool hasJump = false;
    for (auto& block : r.module.functions[0].blocks) {
        if (block->termKind == "跳转") hasJump = true;
    }
    EXPECT_TRUE(hasJump);
}

// 无限循环
TEST(IRTest, InfiniteLoopIR) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    循环 {
        中断
    }
    返回 0
}
)CN");
    ASSERT_GE(r.module.functions[0].blocks.size(), 2u);
}

// ==================== 函数调用IR生成 ====================

// 函数调用 -> Call指令（extra=函数名）
TEST(IRTest, CallIR) {
    auto r = buildIR(R"CN(
函数 加(整32 a, 整32 b) -> 整32 {
    返回 a + b
}
函数 主() -> 整32 {
    返回 加(1, 2)
}
)CN");
    ASSERT_EQ(r.module.functions.size(), 2u);
    bool hasCall = false;
    for (auto& block : r.module.functions[1].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call) {
                hasCall = true;
                EXPECT_EQ(inst.extra, "加");
                EXPECT_EQ(inst.operands.size(), 2u);  // 两个实参
            }
        }
    }
    EXPECT_TRUE(hasCall);
}

// ==================== 字符串常量收集 ====================

// 字符串字面量 -> ConstString + 常量池去重
TEST(IRTest, StringConstants) {
    auto r = buildIR(R"CN(
函数 主() -> 空类型 {
    变量 a = "你好"
    变量 b = "你好"
    变量 c = "世界"
    返回
}
)CN");
    ASSERT_EQ(r.module.stringConstants.size(), 2u);
    EXPECT_EQ(r.module.stringConstants[0], "你好");
    EXPECT_EQ(r.module.stringConstants[1], "世界");
    // ConstString 指令引用 @str0 / @str1
    int strInstCount = 0;
    for (auto& block : r.module.functions[0].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::ConstString) {
                strInstCount++;
                EXPECT_TRUE(inst.extra == "@str0" || inst.extra == "@str1");
            }
        }
    }
    EXPECT_EQ(strInstCount, 3);  // 三处引用（"你好"两次复用@str0）
}

// ==================== 完整程序IR生成 ====================

// 综合：多函数 + 控制流 + 调用 + 字符串
TEST(IRTest, FullProgramIR) {
    auto r = buildIR(R"CN(
函数 阶乘(整32 n) -> 整32 {
    如果 (n <= 1) {
        返回 1
    }
    返回 n * 阶乘(n - 1)
}
函数 主() -> 整32 {
    变量 答案 = 阶乘(5)
    变量 消息 = "完成"
    当 (答案 > 0) {
        答案 = 答案 - 1
    }
    返回 答案
}
)CN");
    ASSERT_EQ(r.module.functions.size(), 2u);
    // 阶乘：递归调用
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Call), 1);
    // 主函数：调用阶乘 + 字符串
    EXPECT_GE(countOpcode(r.module, 1, Opcode::Call), 1);
    EXPECT_GE(countOpcode(r.module, 1, Opcode::ConstString), 1);
    // 字符串常量池
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "完成");
    // 每个函数都有返回终止
    for (auto& func : r.module.functions) {
        ASSERT_FALSE(func.blocks.empty());
        EXPECT_TRUE(func.blocks.back()->terminated);
    }
}

// 操作码转字符串工具
TEST(IRTest, OpcodeToString) {
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::Add), "加");
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::Return), "返回");
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::Call), "调用");
}
