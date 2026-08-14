// 结构体按值传参/返回 + 整体赋值 IR 生成单元测试（Task 完善A：规格书7.4）
// 覆盖：
//   1. 结构体变量声明生成 AddrOf（返回地址而非 Load）
//   2. 结构体按值参数标记（structParamIndexes）
//   3. 结构体返回值标记（structReturn + structReturnSize）
//   4. 结构体整体赋值生成 CopyStruct（含拷贝字节数）
//   5. 结构体返回函数调用：分配返回缓冲区 + 隐藏参数
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
    Lexer lexer(source, "结构体值IR测试.cn", diagnostics);
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

// 查找函数（按名）
const cn_compiler::ir::IRFunction* findFunction(const IRModule& module,
                                                const std::string& name) {
    for (const auto& fn : module.functions) {
        if (fn.name == name) return &fn;
    }
    return nullptr;
}

// 函数中是否出现指定操作码（部分用例未使用，GCC -Wunused-function 兼容）
[[maybe_unused]] bool functionHasOpcode(const cn_compiler::ir::IRFunction& fn, Opcode op) {
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == op) return true;
        }
    }
    return false;
}

} // namespace

// ==================== 结构体按值参数标记 ====================

// 结构体参数应被标记（structParamIndexes 含索引0）
TEST(IRStructValueTest, StructParamMarked) {
    const std::string src = R"(
结构体 学生 { 整32 学号; 整32 成绩; 整32 班级 }
函数 加分(学生 入) -> 学生 {
    学生 出 = 入
    返回 出
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "加分");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->structParamIndexes.count(0), 1u);
}

// ==================== 结构体返回值标记 ====================

// 返回结构体的函数：structReturn=true 且 structReturnSize=12
TEST(IRStructValueTest, StructReturnMarked) {
    const std::string src = R"(
结构体 学生 { 整32 学号; 整32 成绩; 整32 班级 }
函数 加分(学生 入) -> 学生 {
    学生 出 = 入
    返回 出
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "加分");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->structReturn);
    EXPECT_EQ(fn->structReturnSize, 12);  // 3×整32
}

// 非结构体返回：structReturn=false
TEST(IRStructValueTest, NonStructReturnNotMarked) {
    const std::string src = R"(
函数 加(整32 a) -> 整32 { 返回 a + 1 }
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "加");
    ASSERT_NE(fn, nullptr);
    EXPECT_FALSE(fn->structReturn);
}

// ==================== 结构体整体赋值（CopyStruct） ====================

// 结构体变量声明时整体赋值：生成 CopyStruct 且字节数=结构体大小
TEST(IRStructValueTest, CopyStructAssignment) {
    const std::string src = R"(
结构体 点对 { 整32 x; 整32 y }
函数 主() -> 整32 {
    点对 a = 点对{ x = 1, y = 2 }
    点对 b = a
    返回 0
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "主");
    ASSERT_NE(fn, nullptr);
    // 找出 CopyStruct 指令并验证字节数 = 8
    bool found = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::CopyStruct) {
                found = true;
                EXPECT_EQ(inst.extra, "8");  // 点对 = 2×整32 = 8 字节
            }
        }
    }
    EXPECT_TRUE(found) << "缺少 CopyStruct 指令";
}

// 含数组字段结构体整体赋值：CopyStruct 字节数=结构体总大小（含数组）
TEST(IRStructValueTest, CopyStructWithArrayField) {
    const std::string src = R"(
结构体 班级 { 整32 编号; 整32[3] 分数 }
函数 主() -> 整32 {
    班级 一班 = 班级{ 编号 = 1, 分数 = { 80, 90, 70 } }
    班级 二班 = 一班
    返回 0
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "主");
    ASSERT_NE(fn, nullptr);
    bool found = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::CopyStruct) {
                found = true;
                EXPECT_EQ(inst.extra, "16");  // 编号4 + 分数12 = 16 字节
            }
        }
    }
    EXPECT_TRUE(found) << "缺少 CopyStruct 指令";
}

// ==================== 结构体返回函数调用（隐藏返回缓冲区） ====================

// 调用返回结构体的函数：生成返回缓冲区分配（Alloca __retbuf）与 Call
TEST(IRStructValueTest, StructReturnCallAllocatesBuffer) {
    const std::string src = R"(
结构体 学生 { 整32 学号; 整32 成绩; 整32 班级 }
函数 加分(学生 入) -> 学生 {
    学生 出 = 入
    返回 出
}
函数 主() -> 整32 {
    学生 张三 = 学生{ 学号 = 1, 成绩 = 80, 班级 = 3 }
    学生 张三加 = 加分(张三)
    返回 张三加.成绩
}
)";
    IrResult r = generateIr(src);
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "主");
    ASSERT_NE(fn, nullptr);
    // 返回缓冲区 Alloca（__retbuf）应存在
    bool hasRetBuf = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Alloca && inst.extra.find("__retbuf") == 0) {
                hasRetBuf = true;
            }
        }
    }
    EXPECT_TRUE(hasRetBuf) << "缺少返回缓冲区 Alloca";
}
