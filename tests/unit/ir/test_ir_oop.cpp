// 阶段3 OOP IR 指令扩展单元测试（Task 3.1/3.2）
// 覆盖：新增 IR 指令操作码（NewObject/DeleteObject/VirtualCall/VtableAddr）、
//       opcodeToString 中文名映射、IR 模块结构（函数列表/基本块）在
//       类/虚函数场景下可正常生成（语义层通过后 IR 生成不崩溃）
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
    Lexer lexer(source, "OOP IR测试.cn", diagnostics);
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

} // namespace

// ==================== IR 指令操作码扩展（Task 3.1/3.2） ====================

// 新增操作码：NewObject/DeleteObject/VirtualCall/VtableAddr 存在且可转中文名
TEST(IrOopTest, NewOpcodeToString) {
    // 操作码 -> 中文名映射（与 ir.cpp opcodeToString 一致）
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::NewObject), "新建对象");
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::DeleteObject), "删除对象");
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::VirtualCall), "虚调用");
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::VtableAddr), "虚表地址");
}

// 既有操作码中文名不受影响（回归）
TEST(IrOopTest, ExistingOpcodeToString) {
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::Call), "调用");
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::CallIndirect), "间接调用");
    EXPECT_STREQ(cn_compiler::ir::opcodeToString(Opcode::LoadPtr), "指针加载");
}

// ==================== 类/虚函数场景 IR 生成（Task 3.1/3.2） ====================

// 类 + 虚函数 + 重写：IR 生成成功（语义通过，IR 不崩溃）
// 注：本子任务只保证 IR 层可生成模块；新指令的 codegen 展开由后续子任务完成。
TEST(IrOopTest, ClassVirtualIrGenerate) {
    auto r = generateIr(R"CN(
类 动物 {
公开:
    虚拟 函数 叫声() -> 字符串 {
        返回 "..."
    }
}
类 狗 : 动物 {
公开:
    重写 函数 叫声() -> 字符串 {
        返回 "汪汪"
    }
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    // 生成的函数列表至少含 主（类方法由后续 codegen 子任务展开）
    bool hasMain = false;
    for (const auto& fn : r.module.functions) {
        if (fn.name == "主") hasMain = true;
    }
    EXPECT_TRUE(hasMain);
}

// 接口 + 实现类：IR 生成成功
TEST(IrOopTest, InterfaceIrGenerate) {
    auto r = generateIr(R"CN(
接口 可绘制 {
    虚拟 函数 绘制() -> 空类型
}
类 圆形 : 可绘制 {
公开:
    重写 函数 绘制() -> 空类型 {
    }
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 结果<T,E> 类型：IR 生成成功（降级为结构体操作，不崩溃）
TEST(IrOopTest, ResultTypeIrGenerate) {
    auto r = generateIr(R"CN(
函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    返回 正常(a / b)
}
函数 主() -> 整32 {
    变量 除法结果 = 除法(10, 2)
    如果 除法结果.正常 {
        打印行(除法结果.值)
    } 否则 {
        打印行(除法结果.错误)
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 可选<T> 类型：IR 生成成功
TEST(IrOopTest, OptionalTypeIrGenerate) {
    auto r = generateIr(R"CN(
函数 查找(整32 键) -> 可选<整32> {
    返回 某些(键)
}
函数 主() -> 整32 {
    变量 查找结果 = 查找(42)
    如果 查找结果.有值 {
        打印行(查找结果.值)
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 泛型类实例化：IR 生成成功（单态化类名注册后 IR 不崩溃）
TEST(IrOopTest, GenericIrGenerate) {
    auto r = generateIr(R"CN(
泛型 <类型 T>
类 盒子 {
公开:
    T 内容
}
函数 主() -> 整32 {
    盒子<整32> 整数盒子
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}
