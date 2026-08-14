// 调试信息单元测试（阶段C Task 4.4）
// 覆盖：
//   1. 源码位置注释生成（MASM 分号风格 / GAS // 风格）
//   2. 行号映射表登记（SourceMapping 正确性）
//   3. 去重：同一源码位置只注释一次
//   4. 无效位置过滤（空文件名/行号 0）
//   5. DWARF 扩展位预留接口存在性
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文
#include <gtest/gtest.h>
#include <string>

#include "cn_compiler/codegen/debug_info.hpp"
#include "cn_compiler/common/source_location.hpp"

using cn_compiler::SourceLocation;
using cn_compiler::debuginfo::AsmCommentStyle;
using cn_compiler::debuginfo::DebugInfoCollector;

namespace {

// 辅助：构造带位置的指令（测试用）
SourceLocation makeLoc(int line) {
    return SourceLocation("main.cn", line, 1);
}

} // namespace

// ---- 1. 注释格式 ----

TEST(DebugInfoTest, MasmCommentStyle) {
    DebugInfoCollector collector;
    collector.setCommentStyle(AsmCommentStyle::MasmSemicolon);
    const std::string c = collector.commentFor(makeLoc(12), 1);
    EXPECT_EQ(c, "; src: main.cn:12");
}

TEST(DebugInfoTest, GasCommentStyle) {
    DebugInfoCollector collector;
    collector.setCommentStyle(AsmCommentStyle::GasSlash);
    const std::string c = collector.commentFor(makeLoc(12), 1);
    EXPECT_EQ(c, "// src: main.cn:12");
}

// ---- 2. 行号映射表 ----

TEST(DebugInfoTest, MappingTable) {
    DebugInfoCollector collector;
    collector.commentFor(makeLoc(12), 5);
    collector.commentFor(makeLoc(20), 10);
    const auto& mappings = collector.mappings();
    ASSERT_EQ(mappings.size(), 2u);
    EXPECT_EQ(mappings[0].fileName, "main.cn");
    EXPECT_EQ(mappings[0].line, 12);
    EXPECT_EQ(mappings[0].asmLine, 5);
    EXPECT_TRUE(mappings[0].isStart);
    EXPECT_EQ(mappings[1].line, 20);
    EXPECT_EQ(mappings[1].asmLine, 10);
}

// ---- 3. 去重 ----

TEST(DebugInfoTest, DedupSameLoc) {
    DebugInfoCollector collector;
    const std::string first = collector.commentFor(makeLoc(12), 1);
    EXPECT_FALSE(first.empty());
    // 相同位置再次调用 -> 空串（去重）
    const std::string second = collector.commentFor(makeLoc(12), 2);
    EXPECT_TRUE(second.empty());
    // 不同行 -> 正常生成
    const std::string third = collector.commentFor(makeLoc(13), 3);
    EXPECT_EQ(third, "; src: main.cn:13");
    // 映射表只登记 2 条（去重后）
    EXPECT_EQ(collector.mappings().size(), 2u);
}

// ---- 4. 无效位置过滤 ----

TEST(DebugInfoTest, InvalidLocFiltered) {
    DebugInfoCollector collector;
    // 空文件名
    EXPECT_FALSE(DebugInfoCollector::isValidLoc(SourceLocation("", 1, 1)));
    // 行号 0
    EXPECT_FALSE(DebugInfoCollector::isValidLoc(SourceLocation("main.cn", 0, 1)));
    // 有效
    EXPECT_TRUE(DebugInfoCollector::isValidLoc(makeLoc(1)));
    // commentFor 返回空串
    EXPECT_TRUE(collector.commentFor(SourceLocation("", 1, 1), 1).empty());
    EXPECT_TRUE(collector.commentFor(SourceLocation("main.cn", 0, 1), 1).empty());
    EXPECT_TRUE(collector.mappings().empty());
}

// ---- 5. DWARF 扩展位 ----

TEST(DebugInfoTest, DwarfExtensionReserved) {
    DebugInfoCollector collector;
    collector.commentFor(makeLoc(1), 1);
    // DWARF 行号程序导出接口存在（本阶段返回空串，扩展位预留）
    EXPECT_TRUE(collector.dwarfLineProgram().empty());
    // DwarfSection 枚举存在（编译期检查）
    DebugInfoCollector::DwarfSection sec = DebugInfoCollector::DwarfSection::Line;
    (void)sec;
}
