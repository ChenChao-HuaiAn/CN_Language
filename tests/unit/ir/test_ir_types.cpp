// IR 类型系统完善单元测试（Task 2.3）
// 覆盖：字面量后缀 -> IR类型映射（i64/i128/u32/u64/u128/f32）、
//       Cast 转换指令（隐式转换链）、位运算/移位 Opcode（BitAnd/BitOr/BitXor/Shl/Shr）、
//       i128 基本运算 IR、8/16位类型映射
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
    Lexer lexer(source, "IR类型测试.cn", result.diagnostics);
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

// ==================== 字面量后缀 -> IR类型 ====================

// 无后缀整数字面量 -> i32
TEST(IRTypeTest, IntLiteralDefaultI32) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 x = 42;
    返回 x;
}
)CN");
    ASSERT_NE(findFirst(r.module, Opcode::ConstInt), nullptr);
    EXPECT_EQ(findFirst(r.module, Opcode::ConstInt)->type, "i32");
}

// L 后缀 -> i64
TEST(IRTypeTest, IntLiteralSuffixL) {
    auto r = generateIR(R"CN(
函数 主() -> 整64 {
    整64 x = 42L;
    返回 x;
}
)CN");
    ASSERT_NE(findFirst(r.module, Opcode::ConstInt), nullptr);
    EXPECT_EQ(findFirst(r.module, Opcode::ConstInt)->type, "i64");
}

// LL 后缀 -> i128
TEST(IRTypeTest, IntLiteralSuffixLL) {
    auto r = generateIR(R"CN(
函数 主() -> 整128 {
    整128 x = 42LL;
    返回 x;
}
)CN");
    ASSERT_NE(findFirst(r.module, Opcode::ConstInt), nullptr);
    EXPECT_EQ(findFirst(r.module, Opcode::ConstInt)->type, "i128");
}

// U 后缀 -> u32；UL -> u64；ULL -> u128
TEST(IRTypeTest, IntLiteralUnsignedSuffix) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    正32 a = 1U;
    正64 b = 2UL;
    正128 c = 3ULL;
    返回 0;
}
)CN");
    EXPECT_EQ(countOp(r.module, Opcode::ConstInt), 4);
    // 类型检查：查找所有 ConstInt，确认后缀映射正确
    bool hasU32 = false, hasU64 = false, hasU128 = false;
    for (const auto& func : r.module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::ConstInt) {
                    if (inst.type == "u32") hasU32 = true;
                    if (inst.type == "u64") hasU64 = true;
                    if (inst.type == "u128") hasU128 = true;
                }
            }
        }
    }
    EXPECT_TRUE(hasU32);
    EXPECT_TRUE(hasU64);
    EXPECT_TRUE(hasU128);
}

// f 后缀浮点字面量 -> f32；无后缀 -> f64
TEST(IRTypeTest, FloatLiteralSuffixF) {
    auto r = generateIR(R"CN(
函数 主() -> 浮32 {
    浮32 a = 1.5f;
    浮64 b = 2.5;
    返回 a;
}
)CN");
    bool hasF32 = false, hasF64 = false;
    for (const auto& func : r.module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::ConstFloat) {
                    if (inst.type == "f32") hasF32 = true;
                    if (inst.type == "f64") hasF64 = true;
                }
            }
        }
    }
    EXPECT_TRUE(hasF32);
    EXPECT_TRUE(hasF64);
}

// ==================== 隐式转换 Cast ====================

// 整32 -> 整64 赋值生成 Cast
TEST(IRTypeTest, CastIntWidenAssign) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 a = 100;
    整64 b = a;
    返回 0;
}
)CN");
    const auto* cast = findFirst(r.module, Opcode::Cast);
    ASSERT_NE(cast, nullptr);
    EXPECT_EQ(cast->type, "i64");          // 目标类型 i64
    EXPECT_EQ(cast->operands[0].type, "i32");  // 源类型 i32
}

// 整 -> 浮 转换生成 Cast
TEST(IRTypeTest, CastIntToFloat) {
    auto r = generateIR(R"CN(
函数 主() -> 浮64 {
    整32 a = 100;
    浮64 b = a;
    返回 b;
}
)CN");
    const auto* cast = findFirst(r.module, Opcode::Cast);
    ASSERT_NE(cast, nullptr);
    EXPECT_EQ(cast->type, "f64");
}

// 浮32 -> 浮64 转换生成 Cast
TEST(IRTypeTest, CastFloat32To64) {
    auto r = generateIR(R"CN(
函数 主() -> 浮64 {
    浮32 a = 1.5f;
    浮64 b = a;
    返回 b;
}
)CN");
    const auto* cast = findFirst(r.module, Opcode::Cast);
    ASSERT_NE(cast, nullptr);
    EXPECT_EQ(cast->type, "f64");
    EXPECT_EQ(cast->operands[0].type, "f32");
}

// ==================== 位运算与移位 IR ====================

// 位与/位或/位异或 -> BitAnd/BitOr/BitXor
TEST(IRTypeTest, BitwiseOpsIR) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 a = 0b1100;
    整32 b = 0b1010;
    整32 c = a & b;
    整32 d = a | b;
    整32 e = a ^ b;
    返回 c;
}
)CN");
    EXPECT_GE(countOp(r.module, Opcode::BitAnd), 1);
    EXPECT_GE(countOp(r.module, Opcode::BitOr), 1);
    EXPECT_GE(countOp(r.module, Opcode::BitXor), 1);
}

// 移位 -> Shl/Shr
TEST(IRTypeTest, ShiftOpsIR) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 a = 12;
    整32 b = a << 2;
    整32 c = a >> 1;
    返回 b;
}
)CN");
    EXPECT_GE(countOp(r.module, Opcode::Shl), 1);
    EXPECT_GE(countOp(r.module, Opcode::Shr), 1);
}

// 按位非 ~x -> x ^ -1（BitXor）
TEST(IRTypeTest, BitwiseNotIR) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整32 a = 12;
    整32 b = ~a;
    返回 b;
}
)CN");
    EXPECT_GE(countOp(r.module, Opcode::BitXor), 1);
}

// 8/16位整数类型映射（i8/i16/u8/u16）
TEST(IRTypeTest, SmallIntTypeMapping) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整8 a = 10;
    整16 b = 100;
    正8 c = 200;
    正16 d = 300;
    返回 0;
}
)CN");
    bool hasI8 = false, hasI16 = false, hasU8 = false, hasU16 = false;
    for (const auto& func : r.module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::Alloca) {
                    if (inst.type == "i8") hasI8 = true;
                    if (inst.type == "i16") hasI16 = true;
                    if (inst.type == "u8") hasU8 = true;
                    if (inst.type == "u16") hasU16 = true;
                }
            }
        }
    }
    EXPECT_TRUE(hasI8);
    EXPECT_TRUE(hasI16);
    EXPECT_TRUE(hasU8);
    EXPECT_TRUE(hasU16);
}

// ==================== i128 运算 IR ====================

// i128 加/减（Add/Sub 操作数类型为 i128）
TEST(IRTypeTest, Int128AddSubIR) {
    auto r = generateIR(R"CN(
函数 主() -> 整128 {
    整128 a = 1000000LL;
    整128 b = 2000000LL;
    整128 c = a + b;
    整128 d = b - a;
    返回 c;
}
)CN");
    // 查找 Add 指令（类型 i128）
    bool hasAdd128 = false, hasSub128 = false;
    for (const auto& func : r.module.functions) {
        for (const auto& block : func.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::Add && inst.type == "i128") hasAdd128 = true;
                if (inst.opcode == Opcode::Sub && inst.type == "i128") hasSub128 = true;
            }
        }
    }
    EXPECT_TRUE(hasAdd128);
    EXPECT_TRUE(hasSub128);
}

// 运算结果类型映射（mapType 全类型覆盖）
TEST(IRTypeTest, MapTypeCoverage) {
    auto r = generateIR(R"CN(
函数 主() -> 整32 {
    整8 a8 = 1;
    整16 a16 = 1;
    整32 a32 = 1;
    整64 a64 = 1L;
    整128 a128 = 1LL;
    正8 b8 = 1;
    正16 b16 = 1;
    正32 b32 = 1U;
    正64 b64 = 1UL;
    正128 b128 = 1ULL;
    浮32 c32 = 1.5f;
    浮64 c64 = 1.5;
    返回 0;
}
)CN");
    EXPECT_EQ(r.diagnostics.getErrorCount(), 0);
}
