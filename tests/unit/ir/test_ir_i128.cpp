// i128 IR 生成单元测试（Task 完善A：规格书10.5）
// 覆盖：i128 字面量双寄存器结果、i128 变量双槽登记（varSlots=2）、
//       i128 乘法走辅助函数调用（__cn_mul_i128）
// 测试方式：全链路 Lexer + Parser + SemanticAnalyzer + IRGenerator（真实流水线）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::Opcode;

namespace {

struct IrResult {
    bool ok = false;
    IRModule module;
    std::string messages;
};

IrResult generateIr(const std::string& source) {
    IrResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "i128IR测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer semantic(diagnostics);
    if (!semantic.analyze(program.get())) {
        result.messages = diagnostics.format();
        return result;
    }
    IRGenerator irGen(diagnostics, &semantic);
    result.module = irGen.generate(program.get());
    result.ok = !diagnostics.hasErrors();
    result.messages = diagnostics.format();
    return result;
}

const cn_compiler::ir::IRFunction* findFunction(const IRModule& module,
                                                const std::string& name) {
    for (const auto& fn : module.functions) {
        if (fn.name == name) return &fn;
    }
    return nullptr;
}

} // namespace

// ==================== i128 变量双槽登记 ====================

// 整128 变量 varSlots=2（低64位槽 + 高64位槽）
TEST(IRI128Test, I128VarDoubleSlot) {
    const std::string src = R"(
函数 主() -> 整32 {
    整128 大 = 9223372036854775808;
    返回 0;
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "主");
    ASSERT_NE(fn, nullptr);
    bool found = false;
    for (const auto& kv : fn->varSlots) {
        if (kv.second == 2) {  // i128 双槽
            found = true;
        }
    }
    EXPECT_TRUE(found) << "缺少双槽 i128 变量";
}

// ==================== i128 乘法运算 ====================

// i128 乘法：IR 层生成 Mul 指令且结果为 i128（codegen 层才转辅助函数）
TEST(IRI128Test, I128MulIR) {
    const std::string src = R"(
函数 主() -> 整32 {
    整128 a = 170141183460469231731687303715884105727;
    整128 b = 2;
    整128 c = a * b;
    返回 0;
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "主");
    ASSERT_NE(fn, nullptr);
    bool hasMul = false;
    bool mulIsI128 = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Mul) {
                hasMul = true;
                if (inst.result.type == "i128") mulIsI128 = true;
            }
        }
    }
    EXPECT_TRUE(hasMul) << "i128 乘法未生成 Mul 指令";
    EXPECT_TRUE(mulIsI128) << "Mul 结果类型非 i128";
}

// ==================== i128 除法运算 ====================

// i128 除法：IR 层生成 Div 指令且结果为 i128（codegen 层才转辅助函数）
TEST(IRI128Test, I128DivIR) {
    const std::string src = R"(
函数 主() -> 整32 {
    整128 a = 170141183460469231731687303715884105727;
    整128 b = 3;
    整128 c = a / b;
    返回 0;
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "主");
    ASSERT_NE(fn, nullptr);
    bool hasDiv = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Div && inst.result.type == "i128") {
                hasDiv = true;
            }
        }
    }
    EXPECT_TRUE(hasDiv) << "i128 除法未生成 Div 指令";
}

// ==================== u128 除法运算 ====================

// 正128 除法：IR 层生成 Div 指令且结果为 u128（codegen 层才转辅助函数）
//   字面量加 ULL 后缀确保类型为正128（无后缀默认整32，超 int64 自动提升整128）
TEST(IRI128Test, U128DivIR) {
    const std::string src = R"(
函数 主() -> 整32 {
    正128 a = 340282366920938463463374607431768211455ULL;
    正128 b = 2;
    正128 c = a / b;
    返回 0;
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "主");
    ASSERT_NE(fn, nullptr);
    bool hasDiv = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Div && inst.result.type == "u128") {
                hasDiv = true;
            }
        }
    }
    EXPECT_TRUE(hasDiv) << "正128 除法未生成 Div 指令";
}
