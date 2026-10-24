// 1499（2026-10-23·P1 安全）并行 codegen 语义只读冻结单元测试。
// 背景：1145 多 worker 共享同一 SemanticAnalyzer；findClass 为 const 却惰性写
//   mutable classCache（裸名解析缓存）——并行首次查询=共享 unordered_map 并发
//   find/insert（数据竞争 UB；TSan 实证见任务记录）。本测试锁定修复契约：
//   ① spawn 前 beginReadonlyCodegenLookup 预热全部类符号名（含未引用类）；
//   ② 冻结期查询只读——未预热查询不写缓存、登记 miss，end 返回首个 miss 名
//      （主线程据此报内部错误中止，miss 不可静默）；
//   ③ 冻结期并发查询零写（第二次 begin 的 prewarmed 计数不增长）；
//   ④ end 后写路径恢复（游离查询重新入缓存，再次 begin 预热计数增长）；
//   ⑤ begin 重入=显式失败（begin/end 必须配对）；generateFunctionBodies 将该
//      失败映射为决策行 reason=readonly-error（门禁观测口径）；
//   ⑥ 模块限定名/模板实参含 "::" 的类名：限定名走 classByFullKey 不经缓存，
//      冻结期 miss 不登记（白名单外合法查询）；泛型实例名预热不产生垃圾键。
// GCC 7 不支持中文标识符——测试名用英文，注释/源码文本可为中文。
#define _CRT_SECURE_NO_WARNINGS  // getenv（MSVC C4996；须先于任何 CRT 头）
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "cn_compiler/codegen/parallel_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/module/module.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;

namespace {

// 环境变量作用域守卫（与 test_parallel_codegen_failure 同构：析构恢复原值）
class ScopedEnv {
public:
    ScopedEnv(const char* name, const char* value) : name_(name) {
        const char* old = std::getenv(name);
        if (old != nullptr) {
            hadValue_ = true;
            oldValue_ = old;
        }
        Apply(value);
    }
    ~ScopedEnv() { Apply(hadValue_ ? oldValue_.c_str() : nullptr); }
    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

private:
    void Apply(const char* value) {
#ifdef _WIN32
        _putenv_s(name_, value == nullptr ? "" : value);
#else
        if (value == nullptr) {
            unsetenv(name_);
        } else {
            setenv(name_, value, 1);
        }
#endif
    }

    const char* name_;
    bool hadValue_ = false;
    std::string oldValue_;
};

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

// 多模块夹具：单元 甲（类/结构体）+ 单元 主（泛型类以 甲::记录 为实参实例化）
class MergedSemanticFixture {
public:
    MergedSemanticFixture() {
        unitA_ = MakeUnit(
            "公开:\n"
            "类 冷甲 { 公开: 整32 值; }\n"
            "结构体 记录 { 整32 值; }\n",
            "甲.cn", "甲", false);
        // 两模块同名结构体 记录 → 主 内 盒<记录> 的实参经 resolveTypeName 改写为
        //   限定键 主::记录，实例名 盒$主::记录 携带 "::"（审查 P2-7 目标形态；
        //   parser 不接受声明位模板实参直接写 "::"，故用同名冲突触发语义改写）。
        unitMain_ = MakeUnit(
            "结构体 记录 { 整32 值; }\n"
            "泛型 <类型 T>\n"
            "类 盒 {\n"
            "公开:\n"
            "    T 值;\n"
            "}\n"
            "函数 主() -> 整32 {\n"
            "    盒<记录> 物 = 盒<记录>();\n"
            "    返回 物.值.值;\n"
            "}\n",
            "主.cn", "主", true);
        std::vector<cn_compiler::module::ModuleUnit*> ordered = {
            unitA_.get(), unitMain_.get()};
        program_ = std::make_unique<Program>();
        ok_ = cn_compiler::module::mergeModules(ordered, program_.get(), diagnostics_);
        analyzer_ = std::make_unique<SemanticAnalyzer>(diagnostics_);
        ok_ = ok_ && analyzer_->analyze(program_.get());
    }

    bool ok() const { return ok_; }
    SemanticAnalyzer& analyzer() { return *analyzer_; }
    bool hasClassKey(const std::string& key) const {
        return analyzer_ != nullptr && analyzer_->classes().count(key) != 0;
    }
    std::string messages() const {
        std::string text;
        for (const auto& d : diagnostics_.getAll()) text += d.message + "\n";
        return text;
    }

private:
    std::unique_ptr<cn_compiler::module::ModuleUnit> MakeUnit(
        const std::string& source, const std::string& fileName,
        const std::string& moduleName, bool entry) {
        auto unit = std::make_unique<cn_compiler::module::ModuleUnit>();
        unit->filePath = fileName;
        unit->moduleName = moduleName;
        unit->isEntryUnit = entry;
        cn_compiler::module::parseSourceText(source, fileName, moduleName,
                                             unit->ast, unit->imports, diagnostics_);
        return unit;
    }

    Diagnostics diagnostics_;
    std::unique_ptr<cn_compiler::module::ModuleUnit> unitA_;
    std::unique_ptr<cn_compiler::module::ModuleUnit> unitMain_;
    std::unique_ptr<Program> program_;
    std::unique_ptr<SemanticAnalyzer> analyzer_;
    bool ok_ = false;
};

// 平凡生成器：只产出非空文本（聚焦语义冻结契约，不做真实发射）
class TrivialGenerator : public cn_compiler::parallel_codegen::IGenerator {
public:
    void generateFunction(const cn_compiler::ir::IRFunction& function,
                          cn_compiler::parallel_codegen::FunctionOutput& out) override {
        out.text = "函数:" + function.name + "\n";
        out.labelCount = 1;
    }
    bool constantPoolMiss() const override { return false; }
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

// 预热覆盖全部类符号名：begin 成功后冷类查询全部命中（end 无 miss）
TEST(ParallelSemanticReadonlyTest, PrewarmCoversAllClasses) {
    SemanticFixture fixture(kSource);
    ASSERT_TRUE(fixture.ok()) << fixture.messages();
    std::size_t prewarmed = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(prewarmed, error))
        << error;
    EXPECT_GE(prewarmed, 8u) << "预热条目数应覆盖全部类符号名";
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

// 冻结期并发查询（含未预热名）——不得写共享缓存（第二次 begin 计数不增长）；
//   miss 仍被确定性登记。本用例在 TSan 构建下复跑=零 WARNING。
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
    // 零写机械断言：封存期若有任何缓存写入，classCache.size() 增长，第二次
    //   begin 的 prewarmed 计数必随之增大（普通单测即可捕获，不依赖 TSan）。
    std::size_t second = 0;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(second, error))
        << error;
    EXPECT_EQ(second, prewarmed) << "只读冻结期发生缓存写入（只读契约破坏）";
    EXPECT_FALSE(fixture.analyzer().endReadonlyCodegenLookup().has_value());
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

// 重入失败的观测口径：generateFunctionBodies 映射为 kFailure + 决策行
//   reason=readonly-error readonly=off（门禁据此断言「封存失败绝不带竞态继续」）。
TEST(ParallelSemanticReadonlyTest, ReentrantSealReportsReadonlyError) {
    SemanticFixture fixture(kSource);
    ASSERT_TRUE(fixture.ok()) << fixture.messages();
    std::size_t prewarmed = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(prewarmed, error))
        << error;
    cn_compiler::ir::IRModule irModule;
    for (int index = 0; index < 12; ++index) {
        cn_compiler::ir::IRFunction function;
        function.name = "功能" + std::to_string(index);
        function.returnType = "i32";
        irModule.functions.push_back(std::move(function));
    }
    Diagnostics diagnostics;
    std::vector<std::string> texts;
    cn_compiler::parallel_codegen::BodyGenFailure failure;
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv stats("CN_CODEGEN_STATS", "1");
    testing::internal::CaptureStderr();
    const auto status = cn_compiler::parallel_codegen::generateFunctionBodies(
        irModule, {},
        [](Diagnostics&) -> std::unique_ptr<
            cn_compiler::parallel_codegen::IGenerator> {
            return std::make_unique<TrivialGenerator>();
        },
        &fixture.analyzer(), diagnostics, texts, failure);
    const std::string stderrText = testing::internal::GetCapturedStderr();
    EXPECT_EQ(status, cn_compiler::parallel_codegen::BodyGenStatus::kFailure);
    EXPECT_NE(failure.message.find("重复进入"), std::string::npos)
        << failure.message;
    EXPECT_NE(stderrText.find("reason=readonly-error"), std::string::npos)
        << stderrText;
    EXPECT_NE(stderrText.find("readonly=off"), std::string::npos) << stderrText;
    // 清理：手动 begin 仍封存（generate 未解封=begin 未成功）；无任务执行=无 miss
    EXPECT_FALSE(fixture.analyzer().endReadonlyCodegenLookup().has_value());
}

// 模块限定名/模板实参含 "::"：限定名走 classByFullKey（不经缓存、不登记 miss）；
//   泛型实例名（盒$甲::记录）按符号名预热，冻结期查询无 miss（无垃圾键）。
TEST(ParallelSemanticReadonlyTest, QualifiedNamesStayOutsideMissContract) {
    MergedSemanticFixture fixture;
    // 存量限制（非本轮引入）：lowerResultOptionalTypes 以空模块上下文解析局部/
    //   字段模板实参中的同名裸类型（主/甲 双定义 记录）会报歧义——analyze 返回
    //   false，但类表/泛型实例已完整构建。本用例只验证冻结契约，不把该存量诊断
    //   计入；实例 盒$主::记录 由 主 模块字段类型经 resolveTypeName 改写成限定键
    //   产生（正是 P2-7 要覆盖的「模板实参含模块限定名」形态）。
    ASSERT_TRUE(fixture.hasClassKey("盒$主::记录"))
        << "限定实参泛型实例未生成（P2-7 用例失效）\n" << fixture.messages();
    std::size_t prewarmed = 0;
    std::string error;
    ASSERT_TRUE(fixture.analyzer().beginReadonlyCodegenLookup(prewarmed, error))
        << error;
    // 模块内科的裸符号名（ClassInfo.name=冷甲）已被预热覆盖
    EXPECT_TRUE(fixture.analyzer().findClass("冷甲").has_value());
    // 限定全键直查（不经 classCache）
    EXPECT_TRUE(fixture.analyzer().findClass("甲::冷甲").has_value());
    // 泛型实例名含模板实参 "::"（盒$主::记录）：符号名直查（classByFullKey）
    EXPECT_TRUE(fixture.analyzer().findClass("盒$主::记录").has_value());
    // 限定名 miss（甲::记录 是结构体非类）=不计入 miss 契约（白名单外合法查询）
    EXPECT_FALSE(fixture.analyzer().findClass("甲::记录").has_value());
    EXPECT_FALSE(fixture.analyzer().endReadonlyCodegenLookup().has_value())
        << "限定名查询不得登记 miss（不经 classCache）";
}

}  // namespace
