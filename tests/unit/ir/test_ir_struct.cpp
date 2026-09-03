// 结构体/枚举 IR 生成单元测试（Task 2.7）
// 覆盖：结构体变量声明生成 FieldAddr（字段偏移）、枚举引用生成 ConstInt、
//       结构体字段读取生成 FieldAddr + LoadPtr、字段写入生成 FieldAddr + StorePtr
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

// 辅助：全链路生成 IR 模块（语义分析成功才继续）
struct IrResult {
    bool ok = false;
    IRModule module;
    std::string messages;
};

IrResult generateIr(const std::string& source) {
    IrResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "结构体IR测试.cn", diagnostics);
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

// 查找函数中出现的操作码（从全部基本块扫描）
bool moduleHasOpcode(const IRModule& module, const std::string& funcName, Opcode op) {
    for (const auto& fn : module.functions) {
        if (fn.name != funcName) continue;
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == op) return true;
            }
        }
    }
    return false;
}

} // namespace

// 结构体字段读取：p.x 生成 FieldAddr（偏移4）+ LoadPtr
TEST(IrStructTest, FieldLoadGeneratesFieldAddr) {
    IrResult r = generateIr(
        "结构体 点 { 整32 x 整32 y }\n"
        "函数 主() -> 整32 {\n"
        "  点 p = 点{ x = 1, y = 2 }\n"
        "  整32 a = p.y\n"
        "  返回 a\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::FieldAddr));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::LoadPtr));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::StorePtr));
}

// 枚举引用：颜色.蓝 生成 ConstInt（值=2）
TEST(IrStructTest, EnumConstInt) {
    IrResult r = generateIr(
        "枚举 颜色 { 红, 绿, 蓝 }\n"
        "函数 主() -> 整32 {\n"
        "  整32 v = 颜色.蓝\n"
        "  返回 v\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    // 主函数中应有 ConstInt（枚举值以常量加载）
    bool foundConst = false;
    for (const auto& fn : r.module.functions) {
        if (fn.name != "主") continue;
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::ConstInt) foundConst = true;
            }
        }
    }
    EXPECT_TRUE(foundConst);
}

// 枚举负数：方向.上 生成 ConstInt（值=-1）
TEST(IrStructTest, EnumNegativeConst) {
    IrResult r = generateIr(
        "枚举 方向 { 上 = -1, 中, 下 }\n"
        "函数 主() -> 整32 {\n"
        "  整32 v = 方向.上\n"
        "  返回 v\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    bool foundMinusOne = false;
    for (const auto& fn : r.module.functions) {
        if (fn.name != "主") continue;
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::ConstInt && inst.extra == "-1") {
                    foundMinusOne = true;
                }
            }
        }
    }
    EXPECT_TRUE(foundMinusOne);
}

// 经指针成员访问（v2.1 统一 .，自动解引用一级）：ptr.y 生成 FieldAddr
//   （含空指针检查语义在 codegen）
TEST(IrStructTest, ArrowFieldAccess) {
    IrResult r = generateIr(
        "结构体 点 { 整32 x 整32 y }\n"
        "函数 主() -> 整32 {\n"
        "  点 p = 点{ x = 1, y = 2 }\n"
        "  点* ptr = &p\n"
        "  整32 a = ptr.y\n"
        "  返回 a\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::FieldAddr));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::LoadPtr));
}

// 结构体字段写：p.x = 10 生成 FieldAddr + StorePtr
TEST(IrStructTest, FieldStore) {
    IrResult r = generateIr(
        "结构体 点 { 整32 x 整32 y }\n"
        "函数 主() -> 整32 {\n"
        "  点 p = 点{ x = 1, y = 2 }\n"
        "  p.x = 10\n"
        "  返回 p.x\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::FieldAddr));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::StorePtr));
}

// 选择语句枚举 case：IR 层条件跳转链（值比较）
TEST(IrStructTest, EnumInSwitch) {
    IrResult r = generateIr(
        "枚举 颜色 { 红, 绿, 蓝 }\n"
        "函数 主() -> 整32 {\n"
        "  颜色 c = 颜色.绿\n"
        "  选择 (c) {\n"
        "    情况 颜色.红:\n"
        "      返回 1\n"
        "    情况 颜色.绿:\n"
        "      返回 2\n"
        "    默认:\n"
        "      返回 0\n"
        "  }\n"
        "  返回 0\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    // 选择语句展开为级联比较：应有 Eq 比较指令
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::Eq));
}

// ==================== 阶段2全面审查回归测试（2026-08-13） ====================

// BUG10：结构体数组字段（方形.顶点[0].x）应生成 字段地址 + 元素步进 + 越界检查，
//   原实现把数组字段当值 LoadPtr 读取（垃圾指针 -> 空指针错误/访问冲突崩溃）
TEST(IrStructTest, ArrayFieldAccessGeneratesBoundsCheck) {
    IrResult r = generateIr(
        "结构体 坐标 { 整32 x\n 整32 y }\n"
        "结构体 形状 { 坐标[4] 顶点 }\n"
        "函数 主() -> 整32 {\n"
        "  形状 方形\n"
        "  方形.顶点[1].x = 10\n"
        "  返回 方形.顶点[1].x\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    // 数组字段访问应含：FieldAddr（字段地址）+ Add（元素步进）+ __cn_runtime_error（越界检查）
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::FieldAddr));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::Add));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::Call));
}

// BUG10b：经指针访问 + 数组字段（形状指针 p.顶点[1].x，v2.1 统一 .）应正确展开
//   元素步进，原实现 objSrcType 推导失败（形状* 未剥指针）返回占位0
TEST(IrStructTest, ArrowArrayFieldAccessWorks) {
    IrResult r = generateIr(
        "结构体 坐标 { 整32 x\n 整32 y }\n"
        "结构体 形状 { 坐标[4] 顶点 }\n"
        "函数 主() -> 整32 {\n"
        "  形状 方形\n"
        "  方形.顶点[1].x = 10\n"
        "  形状* p = &方形\n"
        "  返回 p.顶点[1].x\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    // 经指针数组字段：FieldAddr + Add（步进）+ LoadPtr（读取）
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::FieldAddr));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::Add));
    EXPECT_TRUE(moduleHasOpcode(r.module, "主", Opcode::LoadPtr));
}

// BUG10c：数组字段越界应插桩运行时错误调用（错误码2）
TEST(IrStructTest, ArrayFieldOutOfBoundsEmitsError) {
    IrResult r = generateIr(
        "结构体 坐标 { 整32 x\n 整32 y }\n"
        "结构体 形状 { 坐标[4] 顶点 }\n"
        "函数 主() -> 整32 {\n"
        "  形状 方形\n"
        "  返回 方形.顶点[4].x\n"
        "}\n");
    ASSERT_TRUE(r.ok) << r.messages;
    // 越界检查：生成 Call __cn_runtime_error（错误码2）
    // 扫描 Call 指令的 extra 是否为运行时错误函数
    bool foundErrorCall = false;
    for (const auto& fn : r.module.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::Call && inst.extra == "__cn_runtime_error") {
                    foundErrorCall = true;
                }
            }
        }
    }
    EXPECT_TRUE(foundErrorCall) << "数组字段越界应生成 __cn_runtime_error 调用";
}
