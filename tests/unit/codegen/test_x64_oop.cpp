// 阶段3 代码生成 OOP 单元测试（Task 3.1/3.2/3.9）
// 覆盖：类布局偏移（classFieldOffset/classTotalSize）、虚表 .rdata 生成、
//       虚调用间接跳转（VirtualCall 展开）、NewObject/DeleteObject/VtableAddr 展开、
//       静态字段 .data 分配、类方法符号生成（this 参数位契约）
// 测试方式：全链路 Lexer + Parser + SemanticAnalyzer + IRGenerator + X64CodeGenerator
//   （codegen 绑定 semantic 指针：虚表槽位/类布局查询走真实语义分析产物）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;
using cn_compiler::X64CodeGenerator;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::Opcode;

namespace {

// 辅助：全链路生成汇编文本（语义分析成功才继续）
struct AsmResult {
    bool ok = false;
    std::string asmText;
    std::string messages;
};

AsmResult generateAsm(const std::string& source) {
    AsmResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "OOP codegen测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer semantic(diagnostics);
    if (!semantic.analyze(program.get())) {
        result.messages = diagnostics.format();
        return result;
    }
    IRGenerator irGen(diagnostics, &semantic);
    IRModule module = irGen.generate(program.get());
    if (diagnostics.hasErrors()) {
        result.messages = diagnostics.format();
        return result;
    }
    X64CodeGenerator codegen(diagnostics, &semantic);
    result.asmText = codegen.generateAssembly(module);
    result.ok = !diagnostics.hasErrors();
    result.messages = diagnostics.format();
    return result;
}

// 辅助：判断汇编文本是否包含子串
bool asmContains(const std::string& asmText, const std::string& needle) {
    return asmText.find(needle) != std::string::npos;
}

} // namespace

// ==================== 类布局偏移（Task 3.1，经语义层查询） ====================

// 类布局：虚表指针 8 字节在首地址，字段偏移已含占位
TEST(X64OopTest, ClassLayoutOffsets) {
    auto r = generateAsm(R"CN(
类 动物 {
公开:
    整32 年龄;
    整64 体重;
    虚拟 函数 叫声() -> 字符串 {
        返回 "...";
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    // 虚表存在：.const 只读段含虚表符号（hasVtable 类；E2E 21 修复：
    //   ml64 只读段指令为 .const，.rdata 是 PE 链接产物节名非 MASM 源指令）
    EXPECT_TRUE(asmContains(r.asmText, ".const"));
    EXPECT_TRUE(asmContains(r.asmText, "?vtable_?"));
}

// 虚表槽位：父类虚函数 + 子类重写覆盖槽位
TEST(X64OopTest, VtableSlotOverride) {
    auto r = generateAsm(R"CN(
类 动物 {
公开:
    虚拟 函数 叫声() -> 字符串 {
        返回 "...";
    }
    虚拟 函数 年龄() -> 整32 {
        返回 0;
    }
}
类 狗 : 动物 {
公开:
    重写 函数 叫声() -> 字符串 {
        返回 "汪汪";
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    // 两个虚表数组生成（动物/狗），狗的重写覆盖槽位
    EXPECT_TRUE(asmContains(r.asmText, "?vtable_?"));
    // 槽位数 == vtableOrder 大小（叫声+年龄 = 2 个槽位，数组含 2 个 dq 项）
    EXPECT_TRUE(asmContains(r.asmText, "2 个槽位"));
}

// 类方法符号：类名$方法名#参数串（UTF-8 修饰），供虚表 dq 引用
TEST(X64OopTest, ClassMethodSymbolInVtable) {
    auto r = generateAsm(R"CN(
类 形状 {
公开:
    虚拟 函数 面积() -> 浮64 {
        返回 0.0;
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    // 虚表数组引用 面积 方法符号（? + UTF-8 十六进制 + @@Y）
    EXPECT_TRUE(asmContains(r.asmText, " dq ?"));
}

// ==================== 静态字段 .data 分配（Task 3.9） ====================

// 静态字段：.data 段分配（全类共享），符号 ?static_类名_字段名
TEST(X64OopTest, StaticFieldDataAlloc) {
    auto r = generateAsm(R"CN(
类 计数器 {
公开:
    静态 整32 总数 = 0;
    函数 计数器() {
        总数++;
    }
    静态 函数 获取总数() -> 整32 {
        返回 总数;
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    // .data 段含静态字段符号（?static_? 前缀 + 类名$字段名 UTF-8 修饰）
    EXPECT_TRUE(asmContains(r.asmText, ".data"));
    EXPECT_TRUE(asmContains(r.asmText, "?static_?"));
}

// ==================== 结果/可选降级结构体 codegen（Task 3.5） ====================

// 结果<T,E> 合成结构体：普通结构体字段访问路径（FieldAddr/StorePtr）可生成
TEST(X64OopTest, ResultOptionalStructCodegen) {
    auto r = generateAsm(R"CN(
函数 除(整32 a, 整32 b) -> 结果<整32, 整32> {
    如果 (b == 0) {
        返回 错误(-1);
    }
    返回 正常(a / b);
}
函数 主() -> 整32 {
    结果<整32, 整32> r = 除(10, 2);
    如果 (r.正常) {
        打印(r.值);
    } 否则 {
        打印(r.错误);
    }
    返回 0;
}
)CN");
    // 结果/可选降级结构体由 IR 层生成 FieldAddr/CopyStruct；
    //   codegen 按普通结构体生成（本子任务保证不崩溃、含结构体返回路径）
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_TRUE(asmContains(r.asmText, "PROC"));
}

// ==================== 无类模块回归（语义指针可空） ====================

// 未绑定 semantic 的 codegen（既有测试路径）：普通模块仍可生成
TEST(X64OopTest, NoSemanticRegression) {
    // 直接手工构造最小 IR 模块（不经过语义层，验证兼容旧路径）
    cn_compiler::ir::IRModule module;
    cn_compiler::ir::IRFunction func;
    func.name = "main";
    func.returnType = "i32";
    auto block = std::make_unique<cn_compiler::ir::IRBlock>();
    block->label = "bb0";
    cn_compiler::ir::IRInstruction c0;
    c0.opcode = Opcode::ConstInt;
    c0.result = cn_compiler::ir::IRValue::reg(0, "i32");
    c0.type = "i32";
    c0.extra = "7";
    block->instructions.push_back(c0);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    Diagnostics diagnostics;
    X64CodeGenerator codegen(diagnostics);  // 无 semantic（默认构造）
    std::string asmText = codegen.generateAssembly(module);
    EXPECT_TRUE(asmContains(asmText, "PROC"));
    EXPECT_TRUE(asmContains(asmText, "ret"));
}

// ==================== #1212 审查 P1-2：虚调用 128 形参 ABI ====================

// 多实现 + 自身. 调用强制 VirtualCall（call rax，防去虚拟化）：整128 实参
//   必须按 ABI 传双槽地址（禁单 64 位）——结果类型 i128 => vcBigRet 布局
//   （rcx=retbuf、rdx=this、r8/r9=实参双槽地址）。
TEST(X64OopTest, VirtualCallI128ArgDualSlotAddress) {
    const char* src = R"(
类 基算 {
公开:
    函数 基算() {}
    虚拟 函数 加一(整128 甲, 整128 乙) -> 整128 { 返回 0LL; }
    函数 经加(整128 甲, 整128 乙) -> 整128 { 返回 自身.加一(甲, 乙); }
}
类 子算 : 基算 {
公开:
    函数 子算() {}
    重写 函数 加一(整128 甲, 整128 乙) -> 整128 { 返回 甲 + 乙 + 1LL; }
}
函数 主() -> 整32 {
    子算 对象 = 子算();
    整128 甲 = 10000000000000000000LL;
    打印(对象.经加(甲, 3LL));
    返回 0;
}
)";
    AsmResult r = generateAsm(src);
    ASSERT_TRUE(r.ok) << r.messages;
    EXPECT_NE(r.asmText.find("call rax"), std::string::npos)
        << "自身. 调用须展开为 VirtualCall（call rax）";
    EXPECT_NE(r.asmText.find("lea r8, [rbp-"), std::string::npos)
        << "整128 实参须传双槽地址（双槽低半 lea），禁单 64 位";
    EXPECT_NE(r.asmText.find("lea r9, [rbp-"), std::string::npos)
        << "第二实参同款双槽地址";
}

// ==================== #1212 复审 P1-1/P1-2：虚调用栈参数 ====================

// 5 实参虚调用：未分配 vreg 的栈参数位禁止 mem-to-mem（mov [rsp+N],[rbp-M]
//   = A2070）；必须经 r10 中转。寄存器位保留直接 mov。
TEST(X64OopTest, VirtualCallFiveArgsNoMemToMem) {
    const char* src = R"(
类 基五 {
公开:
    函数 基五() {}
    虚拟 函数 五一(整64 甲, 整64 乙, 整64 丙, 整64 丁, 整64 戊) -> 整64 { 返回 0; }
    函数 经五(整64 甲, 整64 乙, 整64 丙, 整64 丁, 整64 戊) -> 整64 {
        返回 自身.五一(甲, 乙, 丙, 丁, 戊);
    }
}
类 子五 : 基五 {
公开:
    函数 子五() {}
    重写 函数 五一(整64 甲, 整64 乙, 整64 丙, 整64 丁, 整64 戊) -> 整64 {
        返回 甲 + 乙 + 丙 + 丁 + 戊;
    }
}
函数 主() -> 整32 {
    子五 对象 = 子五();
    整64 甲 = 11;
    整64 乙 = 22;
    打印(对象.经五(甲, 乙, 33, 44, 55));
    返回 0;
}
)";
    AsmResult r = generateAsm(src);
    ASSERT_TRUE(r.ok) << r.messages;
    EXPECT_NE(r.asmText.find("call rax"), std::string::npos);
    EXPECT_EQ(r.asmText.find("mov [rsp+32], [rbp"), std::string::npos)
        << "栈参数位禁止 mem-to-mem（A2070）";
    EXPECT_EQ(r.asmText.find("mov [rsp+40], [rbp"), std::string::npos)
        << "栈参数位禁止 mem-to-mem（A2070）";
    EXPECT_NE(r.asmText.find("mov [rsp+32], r10"), std::string::npos)
        << "栈参数位须经 r10 中转";
}

// 5 实参 + 128 形参虚调用（vcBigRet）：retbuf 占参数位 0、this 占 1，实参从
//   2 起——totalParams 须计入 retbuf 位（原 argCount+1 少一，末栈位越界 8B）。
//   同时断言 128 实参传双槽地址、全部 [rsp+N] 落在 sub rsp 分配区内。
TEST(X64OopTest, VirtualCallI128StackArgsInBounds) {
    const char* src = R"(
类 基五 {
公开:
    函数 基五() {}
    虚拟 函数 五一(整128 甲, 整64 乙, 整128 丙, 整64 丁, 整64 戊) -> 整128 { 返回 0LL; }
    函数 经五(整128 甲, 整64 乙, 整128 丙, 整64 丁, 整64 戊) -> 整128 {
        返回 自身.五一(甲, 乙, 丙, 丁, 戊);
    }
}
类 子五 : 基五 {
公开:
    函数 子五() {}
    重写 函数 五一(整128 甲, 整64 乙, 整128 丙, 整64 丁, 整64 戊) -> 整128 {
        返回 甲 + 丙 + 乙 + 丁 + 戊;
    }
}
函数 主() -> 整32 {
    子五 对象 = 子五();
    整64 乙 = 22;
    整128 甲 = 10000000000000000000LL;
    打印(对象.经五(甲, 乙, 10000000000000000003LL, 44, 55));
    返回 0;
}
)";
    AsmResult r = generateAsm(src);
    ASSERT_TRUE(r.ok) << r.messages;
    EXPECT_NE(r.asmText.find("call rax"), std::string::npos);
    // 128 实参（甲/丙）传双槽地址（lea 低半槽 -> 参数寄存器/栈位中转）
    EXPECT_NE(r.asmText.find("lea r8, [rbp-"), std::string::npos)
        << "第一 128 实参须传双槽地址";
    EXPECT_EQ(r.asmText.find("mov [rsp+32], [rbp"), std::string::npos);
    EXPECT_EQ(r.asmText.find("mov [rsp+40], [rbp"), std::string::npos);
    EXPECT_EQ(r.asmText.find("mov [rsp+48], [rbp"), std::string::npos);
    // 分配区核对：找虚调用段最后一个 sub rsp, N，断言所有 [rsp+K] K < N
    const std::size_t callPos = r.asmText.rfind("call rax");
    ASSERT_NE(callPos, std::string::npos);
    const std::size_t subPos = r.asmText.rfind("sub rsp, ", callPos);
    ASSERT_NE(subPos, std::string::npos);
    const std::size_t subEnd = r.asmText.find_first_of("\r\n", subPos);
    const int subBytes = std::stoi(
        r.asmText.substr(subPos + 9, subEnd - subPos - 9));
    std::size_t scan = callPos;
    while (true) {
        const std::size_t p = r.asmText.rfind("[rsp+", scan);
        if (p == std::string::npos || p < subPos) break;
        const std::size_t e = r.asmText.find(']', p);
        if (e == std::string::npos) break;
        const int off = std::stoi(r.asmText.substr(p + 5, e - p - 5));
        EXPECT_LT(off, subBytes) << "[rsp+" << off << "] 越出 sub rsp, "
                                 << subBytes;
        scan = p - 1;
    }
}
