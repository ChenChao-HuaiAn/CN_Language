// IR生成器函数支持单元测试（Task 2.2）
// 覆盖：函数指针间接调用（CallIndirect/FuncAddr）、原型声明不生成IR函数、递归
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
    Lexer lexer(source, "IR函数测试.cn", result.diagnostics);
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

// 查找函数索引（按名）
int findFunction(const IRModule& module, const std::string& name) {
    for (std::size_t i = 0; i < module.functions.size(); ++i) {
        if (module.functions[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

} // namespace

// ==================== 函数指针间接调用 ====================

// 函数指针：赋值生成 FuncAddr，调用生成 CallIndirect（operand[0]=指针寄存器）
TEST(IRFunctionTest, FuncPtrCallIndirect) {
    auto r = buildIR(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b; }
函数 主() -> 整32 {
    整32(*回调)(整32, 整32);
    回调 = 加;
    返回 回调(10, 20);
}
)CN");
    EXPECT_FALSE(r.diagnostics.hasErrors());
    int mainIdx = findFunction(r.module, "主");
    ASSERT_GE(mainIdx, 0);
    // 赋值：函数地址（FuncAddr 加载 加 的地址）
    EXPECT_EQ(countOpcode(r.module, mainIdx, Opcode::FuncAddr), 1);
    // 调用：间接调用（CallIndirect）
    EXPECT_EQ(countOpcode(r.module, mainIdx, Opcode::CallIndirect), 1);
    // 直接调用（Call）应为0（函数指针调用不走Call）
    EXPECT_EQ(countOpcode(r.module, mainIdx, Opcode::Call), 0);
}

// 函数指针作为参数传递：被调函数内间接调用
TEST(IRFunctionTest, FuncPtrAsParamIndirect) {
    auto r = buildIR(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b; }
函数 执行(整32(*func)(整32, 整32), 整32 x, 整32 y) -> 整32 {
    返回 func(x, y);
};
函数 主() -> 整32 {
    整32(*回调)(整32, 整32);
    回调 = 加;
    返回 执行(回调, 10, 20);
}
)CN");
    EXPECT_FALSE(r.diagnostics.hasErrors());
    int execIdx = findFunction(r.module, "执行");
    ASSERT_GE(execIdx, 0);
    // 执行 函数内：间接调用 func 指针
    EXPECT_EQ(countOpcode(r.module, execIdx, Opcode::CallIndirect), 1);
    // 函数指针参数类型为 ptr
    ASSERT_EQ(r.module.functions[execIdx].params.size(), 3u);
    EXPECT_EQ(r.module.functions[execIdx].params[0].second, "ptr");
    // 主 内：直接调用 执行（传指针实参）
    int mainIdx = findFunction(r.module, "主");
    ASSERT_GE(mainIdx, 0);
    EXPECT_EQ(countOpcode(r.module, mainIdx, Opcode::Call), 1);
    EXPECT_EQ(countOpcode(r.module, mainIdx, Opcode::FuncAddr), 1);
}

// ==================== 函数原型声明 ====================

// 原型声明不生成IR函数（无函数体），仅定义生成
TEST(IRFunctionTest, PrototypeNoIRFunction) {
    auto r = buildIR(R"CN(
函数 计算(整32 n) -> 整32
函数 计算(整32 n) -> 整32 { 返回 n * 2; }
)CN");
    EXPECT_FALSE(r.diagnostics.hasErrors());
    // 计算 只应出现一次（定义），原型不生成
    EXPECT_EQ(findFunction(r.module, "计算"), 0);
    // 模块中函数数量：只有 计算 一个（原型被跳过）
    EXPECT_EQ(r.module.functions.size(), 1u);
}

// 纯原型声明（无定义）：IR层不生成任何函数
TEST(IRFunctionTest, PrototypeOnlyNoFunction) {
    auto r = buildIR(R"CN(
函数 外部函数(整32 n) -> 整32
)CN");
    EXPECT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(r.module.functions.size(), 0u);
}

// ==================== 递归调用 ====================

// 递归：函数体内直接调用自身（Call 指令，extra=自身名）
TEST(IRFunctionTest, RecursionDirectCall) {
    auto r = buildIR(R"CN(
函数 阶乘(整32 n) -> 整32 {
    如果 (n <= 1) { 返回 1; }
    返回 n * 阶乘(n - 1);
}
)CN");
    EXPECT_FALSE(r.diagnostics.hasErrors());
    int factIdx = findFunction(r.module, "阶乘");
    ASSERT_GE(factIdx, 0);
    // 阶乘 体内应有对自身的 Call
    EXPECT_GT(countOpcode(r.module, factIdx, Opcode::Call), 0);
    // 查找 Call 指令，确认 extra 为 阶乘（自身递归）
    bool foundSelfCall = false;
    for (auto& block : r.module.functions[factIdx].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == "阶乘") {
                foundSelfCall = true;
            }
        }
    }
    EXPECT_TRUE(foundSelfCall);
}

// 前向引用：调用定义在后的函数（Call 指令引用后定义函数）
TEST(IRFunctionTest, ForwardReferenceCall) {
    auto r = buildIR(R"CN(
函数 计算(整32 n) -> 整32
函数 使用() -> 整32 {
    返回 计算(10);
}
函数 计算(整32 n) -> 整32 { 返回 n * 2; }
)CN");
    EXPECT_FALSE(r.diagnostics.hasErrors());
    int useIdx = findFunction(r.module, "使用");
    ASSERT_GE(useIdx, 0);
    // 使用 内 Call 计算（前向引用）
    bool foundCall = false;
    for (auto& block : r.module.functions[useIdx].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == "计算") {
                foundCall = true;
            }
        }
    }
    EXPECT_TRUE(foundCall);
    // 计算 定义生成（原型不生成）
    int calcIdx = findFunction(r.module, "计算");
    ASSERT_GE(calcIdx, 0);
    EXPECT_EQ(r.module.functions.size(), 2u);  // 使用 + 计算
}
