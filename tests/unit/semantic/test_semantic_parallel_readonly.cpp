// 1501（2026-10-23·P1 安全）并行 codegen 语义只读冻结单元测试。
// 背景：1145 多 worker 共享同一 SemanticAnalyzer；findClass 为 const 却惰性写
//   mutable classCache（裸名解析缓存）——并行首次查询=共享 unordered_map 并发
//   find/insert（数据竞争 UB；TSan 实证见任务记录）。本测试锁定修复契约：
//   ① spawn 前 beginReadonlyCodegenLookup 预热全部类基名（含未引用类）；
//   ② 冻结期查询只读——未预热查询不写缓存、登记 miss，end 返回首个 miss 名
//      （主线程据此报内部错误中止，miss 不可静默）；
//   ③ 冻结期并发查询无写（本文件在 TSan 构建下复跑=零 WARNING 证据）；
//   ④ end 后写路径恢复（游离查询重新入缓存，再次 begin 预热计数增长）；
//   ⑤ begin 重入=显式失败（begin/end 必须配对）。
// GCC 7 不支持中文标识符——测试名用英文，注释/源码文本可为中文。
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;

namespace {

// 语义夹具：Diagnostics 先于分析器声明（分析器持引用，析构序安全）
class SemanticFixture {
public:
    explicit SemanticFixture(const std::string& source) {
        Lexer lexer(source, "只读冻结1499.cn", diagnostics_);
        std::vector<cn_compiler::Token> tokens = lexer.tokenize();
        Parser parser(diagnostics_);
        program_ = parser.parse(tokens);
        analyzer_ = std::make_unique<SemanticAnalyzer>(diagnostics_);
        ok_ = analyzer_->analyze(program_.get());
    }

    bool ok() const { return ok_; }
    SemanticAnalyzer& analyzer() { return *analyzer_; }
    // 全部诊断拼接（失败断言时一并输出，便于定位）
    std::string messages() const {
        std::string text;
        for (const auto& d : diagnostics_.getAll()) text += d.message + "\n";
        return text;
    }

private:
    Diagnostics diagnostics_;
    std::unique_ptr<Program> program_;
    std::unique_ptr<SemanticAnalyzer> analyzer_;
    bool ok_ = false;
};

// 9 个类：冷零..冷七（源码未引用=缓存冷）+ 主（保证入口函数存在）
const char* kSource =
    "类 冷零 { 公开: 整32 值; }\n"
    "类 冷一 { 公开: 整32 值; }\n"
    "类 冷二 { 公开: 整32 值; }\n"
    "类 冷三 { 公开: 整32 值; }\n"
    "类 冷四 { 公开: 整32 值; }\n"
    "类 冷五 { 公开: 整32 值; }\n"
    "类 冷六 { 公开: 整32 值; }\n"
    "类 冷七 { 公开: 整32 值; }\n"
    "函数 主() -> 整32 { 返回 0; }\n";

const char* kClassNames[] = {"冷零", "冷一", "冷二", "冷三",
                             "冷四", "冷五", "冷六", "冷七"};

// 预热覆盖全部类基名：begin 成功后冷类查询全部命中（end 无 miss）
TEST(ParallelSemanticReadonlyTest, PrewarmCoversAllClasses) {
    SemanticFixture fixture(kSource);
    ASSERT_TRUE(fixture.ok()) << fixture.messages();
    std::size_t prewarmed = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(prewarmed, error))
        << error;
    EXPECT_GE(prewarmed, 8u) << "预热条目数应覆盖全部类基名";
    for (const char* name : kClassNames) {
        EXPECT_TRUE(fixture.analyzer().findClass(name).has_value())
            << "冻结期已预热类查询应命中: " << name;
    }
    EXPECT_FALSE(fixture.analyzer().endReadonlyCodegenLookup().has_value())
        << "全部命中预热缓存时不得登记 miss";
}

// 未预热查询=登记 miss（不写缓存），end 返回首个 miss 名（内部错误显形）
TEST(ParallelSemanticReadonlyTest, UnwarmedLookupIsExplicitFailure) {
    SemanticFixture fixture(kSource);
    ASSERT_TRUE(fixture.ok()) << fixture.messages();
    std::size_t prewarmed = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(prewarmed, error))
        << error;
    const auto resolved = fixture.analyzer().findClass("未预热类甲");
    EXPECT_FALSE(resolved.has_value()) << "不存在的类不得凭空命中";
    const auto miss = fixture.analyzer().endReadonlyCodegenLookup();
    ASSERT_TRUE(miss.has_value()) << "冻结期未预热查询必须登记（miss 不可静默）";
    EXPECT_EQ(miss.value(), "未预热类甲");
}

// 冻结期并发查询（含未预热名）——不得写共享缓存；miss 仍被确定性登记。
//   本用例在 TSan 构建下复跑=零 WARNING（修复前同一形态见 1501 取证夹具）。
TEST(ParallelSemanticReadonlyTest, ConcurrentLookupHasNoCacheWrite) {
    SemanticFixture fixture(kSource);
    ASSERT_TRUE(fixture.ok()) << fixture.messages();
    std::size_t prewarmed = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(prewarmed, error))
        << error;
    constexpr int kThreads = 8;
    std::vector<std::thread> workers;
    for (int worker = 0; worker < kThreads; ++worker) {
        workers.emplace_back([&fixture, worker]() {
            for (int round = 0; round < 100; ++round) {
                const int index = (round + worker) % 8;
                static_cast<void>(fixture.analyzer().findClass(kClassNames[index]));
                static_cast<void>(fixture.analyzer().findClass("未预热类乙"));
            }
        });
    }
    for (std::thread& thread : workers) thread.join();
    const auto miss = fixture.analyzer().endReadonlyCodegenLookup();
    ASSERT_TRUE(miss.has_value()) << "并发未预热查询必须登记 miss";
    EXPECT_EQ(miss.value(), "未预热类乙");
}

// end 后写路径恢复：游离查询重新入缓存（再次 begin 预热计数增长）
TEST(ParallelSemanticReadonlyTest, WritePathResumesAfterUnseal) {
    SemanticFixture fixture(kSource);
    ASSERT_TRUE(fixture.ok()) << fixture.messages();
    std::size_t first = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(first, error)) << error;
    EXPECT_FALSE(fixture.analyzer().endReadonlyCodegenLookup().has_value());
    // 解封后查询（正常写路径）——不登记 miss
    EXPECT_FALSE(fixture.analyzer().findClass("未预热类丙").has_value());
    std::size_t second = 0;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(second, error)) << error;
    EXPECT_GT(second, first) << "解封后查询应重新写入缓存（再次预热计数增长）";
    EXPECT_FALSE(fixture.analyzer().endReadonlyCodegenLookup().has_value());
}

// begin 重入=显式失败（begin/end 必须配对；不得静默按成功继续）
TEST(ParallelSemanticReadonlyTest, ReentrantBeginIsRejected) {
    SemanticFixture fixture(kSource);
    ASSERT_TRUE(fixture.ok()) << fixture.messages();
    std::size_t prewarmed = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(prewarmed, error))
        << error;
    std::size_t again = 0;
    std::string secondError;
    EXPECT_FALSE(fixture.analyzer().beginReadonlyCodegenLookup(again, secondError));
    EXPECT_NE(secondError.find("重复进入"), std::string::npos) << secondError;
    EXPECT_FALSE(fixture.analyzer().endReadonlyCodegenLookup().has_value());
}

}  // namespace
