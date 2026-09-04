// IR 指针/数组生成单元测试（Task 2.4）
// 覆盖：AddrOf（取地址）、LoadPtr/StorePtr（指针访存）、指针算术（Add/Sub + ptr）、
//       数组下标（AddrOf + Mul + Add + LoadPtr）、越界检查插桩（Call __cn_runtime_error）、
//       数组多槽登记（varSlots）、空指针字面量（ConstInt 0）
// 测试方式：通过 Lexer + Parser 得到真实AST，再交给 IRGenerator 生成 IR（全链路）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::ir::Opcode;

namespace {

// 辅助：解析+IR生成，返回 IR 模块与诊断
struct IRResult {
    cn_compiler::ir::IRModule module;
    Diagnostics diagnostics;
};

IRResult generateIR(const std::string& source) {
    IRResult result;
    Lexer lexer(source, "IR指针测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(result.diagnostics);
    result.module = generator.generate(program.get());
    return result;
}

// 在模块中查找第一条匹配 opcode 的指令
const cn_compiler::ir::IRInstruction* findFirst(const cn_compiler::ir::IRModule& module,
                                                Opcode opcode) {
    for (const auto& func : module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == opcode) return &inst;
            }
        }
    }
    return nullptr;
}

// 统计模块中某 opcode 的指令数量
int countOp(const cn_compiler::ir::IRModule& module, Opcode opcode) {
    int n = 0;
    for (const auto& func : module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == opcode) n++;
            }
        }
    }
    return n;
}

} // namespace

// ==================== 取地址/解引用 ====================

// &x 生成 AddrOf 指令（结果类型 ptr）
TEST(IRPointerTest, AddressOfEmit) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 x = 42;
    整32* p = &x;
    返回 0;
}
)CN");
    const auto* addr = findFirst(r.module, Opcode::AddrOf);
    ASSERT_NE(addr, nullptr);
    EXPECT_EQ(addr->result.type, "ptr");
}

// *p 生成 LoadPtr 指令
TEST(IRPointerTest, DerefLoadPtr) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 x = 42;
    整32* p = &x;
    整32 y = *p;
    返回 0;
}
)CN");
    ASSERT_NE(findFirst(r.module, Opcode::LoadPtr), nullptr);
}

// *p = 值 生成 StorePtr 指令
TEST(IRPointerTest, DerefStorePtr) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 x = 42;
    整32* p = &x;
    *p = 99;
    返回 0;
}
)CN");
    ASSERT_NE(findFirst(r.module, Opcode::StorePtr), nullptr);
}

// ==================== 指针算术 ====================

// p + 1：Add(ptr, Mul(i64 1, 8)) -> ptr
TEST(IRPointerTest, PointerAdd) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 };
    整32* p = &数据[0];
    整32* q = p + 1;
    返回 0;
}
)CN");
    // 指针加法：Mul（i64, 8）随后 Add（ptr）
    const auto* mul = findFirst(r.module, Opcode::Mul);
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->result.type, "i64");
    EXPECT_EQ(mul->operands[1].extra, "8");  // 步进8字节（栈槽宽）
}

// p - 1：Sub(ptr, Mul(i64 1, 8))
TEST(IRPointerTest, PointerSub) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 };
    整32* p = &数据[0];
    整32* q = p - 1;
    返回 0;
}
)CN");
    // 指针减法：Sub 结果类型 ptr
    const cn_compiler::ir::IRInstruction* sub = nullptr;
    for (const auto& func : r.module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::Sub && inst.result.type == "ptr") sub = &inst;
            }
        }
    }
    ASSERT_NE(sub, nullptr);
}

// ==================== 数组下标 ====================

// 数据[i]：AddrOf + Mul + Add + LoadPtr
TEST(IRPointerTest, ArrayIndexEmit) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32[5] 数据 = { 1, 2, 3, 4, 5 };
    整32 i = 2;
    整32 v = 数据[i];
    返回 0;
}
)CN");
    ASSERT_NE(findFirst(r.module, Opcode::AddrOf), nullptr);
    ASSERT_NE(findFirst(r.module, Opcode::LoadPtr), nullptr);
    // 越界检查：Call __cn_runtime_error 出现
    const auto* call = findFirst(r.module, Opcode::Call);
    // Call 可能被 __cn_runtime_error 使用（越界检查插桩）
    (void)call;
}

// 越界检查插桩：index<0 || index>=len 的比较与错误调用
TEST(IRPointerTest, BoundsCheckEmit) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32[5] 数据 = { 1, 2, 3, 4, 5 };
    整32 i = 2;
    整32 v = 数据[i];
    返回 0;
}
)CN");
    // 越界检查生成：Lt（i<0）、Ge（i>=5）、Or 合并
    const auto* lt = findFirst(r.module, Opcode::Lt);
    ASSERT_NE(lt, nullptr);
    const auto* ge = findFirst(r.module, Opcode::Ge);
    ASSERT_NE(ge, nullptr);
    // __cn_runtime_error 调用：扫描 Call 指令 extra
    bool foundErrCall = false;
    for (const auto& func : r.module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::Call && inst.extra == "__cn_runtime_error") {
                    foundErrCall = true;
                }
            }
        }
    }
    EXPECT_TRUE(foundErrCall);
}

// 数组初始化列表：多元素 StorePtr（每元素一个）
TEST(IRPointerTest, InitListStore) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 };
    返回 0;
}
)CN");
    // 3个元素 -> 至少3条 StorePtr
    EXPECT_GE(countOp(r.module, Opcode::StorePtr), 3);
}

// 数组多槽登记：varSlots 记录数组长度
TEST(IRPointerTest, ArrayVarSlots) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32[5] 数据 = { 1, 2, 3, 4, 5 };
    返回 0;
}
)CN");
    ASSERT_FALSE(r.module.functions.empty());
    // varSlots 应包含数组变量（槽数5）
    bool found = false;
    for (const auto& kv : r.module.functions[0].varSlots) {
        if (kv.second == 5) found = true;
    }
    EXPECT_TRUE(found);
}

// ==================== 空指针字面量 ====================

// 无 -> ConstInt 0（ptr）
TEST(IRPointerTest, NullLiteralConst) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32* p = 无;
    返回 0;
}
)CN");
    // p = 无：ConstInt(0, ptr) + Store（ptr槽）
    const auto* c = findFirst(r.module, Opcode::ConstInt);
    ASSERT_NE(c, nullptr);
    // 空指针常量 0 且类型 ptr 或为初始化 Store 的源
    EXPECT_EQ(c->extra, "0");
}

// ==================== 数组名退化 ====================

// 数组名赋给指针：AddrOf（数组基址）-> ptr
TEST(IRPointerTest, ArrayDecayAddr) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 };
    整32* p = 数据;
    返回 0;
}
)CN");
    // 数组名退化生成 AddrOf
    ASSERT_NE(findFirst(r.module, Opcode::AddrOf), nullptr);
}
