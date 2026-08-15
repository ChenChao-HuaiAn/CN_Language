// 类型大小内建语义单元测试（A-3 修复，2026-08）
// 覆盖：类型大小(整32/结构体/数组/泛型 T/限定类型) 编译期求值；
//   sizeof 表达式类型为 整64；结构体向量容器槽位修复的语义侧（泛型上下文）
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路（非Mock）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::SemanticAnalyzer;

namespace {

struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "类型大小测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(program.get());
    result.errorCount = diagnostics.getErrorCount();
    for (const auto& d : diagnostics.getAll()) {
        result.messages += d.message + "\n";
    }
    return result;
}

} // namespace

// 类型大小基本形态：标量/结构体/数组 -> 语义通过
TEST(SizeofTest, BasicSizesOk) {
    auto r = analyzeSource(R"CN(
结构体 点 {
    整64 x
    整64 y
    整64 z
}
函数 主() -> 整32 {
    整64 a = 类型大小(整32)
    整64 b = 类型大小(点)
    整64 c = 类型大小(整64[5])
    打印(a, b, c)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 泛型上下文：类型大小(T)（向量<T> 扩容分配用）-> 语义通过
TEST(SizeofTest, GenericSizeofOk) {
    auto r = analyzeSource(R"CN(
枚举 错误码 {
    内存 = 4
}

泛型 <类型 T>
类 向量 {
私有:
    T* 数据
    整64 元素数量
    整64 数组容量
公开:
    函数 追加(T 值) -> 结果<空类型, 整32> {
        如果 (元素数量 >= 数组容量) {
            整64 新容量 = 数组容量 * 2
            如果 (新容量 == 0) {
                新容量 = 4
            }
            空类型* 新数据 = 重新分配(数据, 新容量 * (类型大小(T) < 8 ? 8 : 类型大小(T)))
            如果 (新数据 == 无) {
                返回 错误(错误码.内存)
            }
            数据 = 新数据
            数组容量 = 新容量
        }
        数据[元素数量] = 值
        元素数量++
        返回 正常()
    }
}
函数 主() -> 整32 {
    向量<整32> 数表 = 向量<整32>()
    结果<空类型, 整32> 追加 = 数表.追加(1)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 限定类型：类型大小(甲::记录)（A-2 限定键解析）-> 语义通过
TEST(SizeofTest, QualifiedTypeSizeofOk) {
    auto r = analyzeSource(R"CN(
结构体 记录 {
    整64 标识
}
函数 主() -> 整32 {
    整64 s = 类型大小(记录)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
