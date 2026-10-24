// 并行 codegen 工作线程失败传播单元测试（任务 1220·§1.1a② 确定性失败）
// 覆盖：
//   1. 任务内 std::exception：捕获后返回 kFailure（不 terminate），失败序号/函数名/
//      消息确定（最小失败序号），reportBodyGenFailure 诊断含函数名与序号
//   2. 任务内未知异常（非 std::exception）：catch (...) 通道同样收口
//   3. 空函数文本：发射器零产出=内部失败，不得静默拼接
//   4. 正常路径：并行成功、按原函数序输出（注入关闭零行为）
//   5. 线程创建失败：确定性降级（成功+产出正确）、警告写入 Diagnostics（计数 1）、
//      决策行 workers=1 观测
//   6. 注入配置非法/序号越界/2^64 溢出：显式内部失败（注入失效不得静默假绿）
//   7. 未知 BodyGenStatus：applyBodyGenStatus 报内部错误且不拼接部分汇编（P2-3）
// 说明：本文件用合成 IRModule + 平凡生成器聚焦线程池契约；真实编译链路的
//   rc/诊断/产物清理由 scripts/check_parallel_failure.py 覆盖（不 mock 编译器）。
//   GCC 7 不支持中文标识符——测试名/变量名用英文，注释与数据文本可为中文。
#define _CRT_SECURE_NO_WARNINGS  // getenv（MSVC C4996；须先于任何 CRT 头）
#include <gtest/gtest.h>

#include <cstdlib>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/codegen/parallel_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::parallel_codegen::BodyGenFailure;
using cn_compiler::parallel_codegen::BodyGenStatus;
using cn_compiler::parallel_codegen::FunctionOutput;
using cn_compiler::parallel_codegen::IGenerator;
namespace ir = cn_compiler::ir;

namespace {

// 环境变量作用域守卫：析构恢复原值（未设则删除），避免测试间污染/注入泄漏
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
        // Windows CRT：置空串=删除变量（std::getenv 读取同一 CRT 环境）
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

ir::IRModule BuildModule(std::size_t count) {
    ir::IRModule module;
    for (std::size_t index = 0; index < count; ++index) {
        ir::IRFunction function;
        function.name = "功能" + std::to_string(index);
        function.returnType = "i32";
        module.functions.push_back(std::move(function));
    }
    return module;
}

// 平凡生成器：按函数名制造指定故障；其余函数产非空文本（并行路径可正常完成）
class TestGenerator : public IGenerator {
public:
    TestGenerator(std::string throwName, std::string emptyName, bool unknownThrow)
        : throwName_(std::move(throwName)),
          emptyName_(std::move(emptyName)),
          unknownThrow_(unknownThrow) {}

    void generateFunction(const ir::IRFunction& function, FunctionOutput& out) override {
        if (!throwName_.empty() && function.name == throwName_) {
            if (unknownThrow_) throw 1220;  // 非 std::exception（catch(...) 通道）
            throw std::runtime_error("单测注入异常");
        }
        if (!emptyName_.empty() && function.name == emptyName_) {
            out.text.clear();  // 发射器零产出
            out.labelCount = 0;
            return;
        }
        out.text = "函数体:" + function.name + "\n";
        out.labelCount = 1;
    }
    bool constantPoolMiss() const override { return false; }

private:
    std::string throwName_;
    std::string emptyName_;
    bool unknownThrow_;
};

// 一次调用所需的测试夹具（注入变量统一清空/按需设置，线程数固定 4）
struct Harness {
    ir::IRModule module;
    Diagnostics diagnostics;
    std::vector<std::string> texts;
    BodyGenFailure failure;

    explicit Harness(std::size_t functionCount) : module(BuildModule(functionCount)) {}
};

using MakeFn = std::function<std::unique_ptr<IGenerator>(Diagnostics&)>;

BodyGenStatus GenerateBodies(Harness& harness, const MakeFn& make) {
    return cn_compiler::parallel_codegen::generateFunctionBodies(
        harness.module, {}, make, /*semantic=*/nullptr, harness.diagnostics,
        harness.texts, harness.failure);
}

MakeFn Touching(std::string throwName, std::string emptyName, bool unknownThrow) {
    return [throwName, emptyName, unknownThrow](Diagnostics&) {
        return std::make_unique<TestGenerator>(throwName, emptyName, unknownThrow);
    };
}

}  // namespace

// ==================== 失败传播 ====================

TEST(ParallelCodegenFailureTest, StdExceptionBecomesDeterministicFailure) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", nullptr);
    Harness harness(12);
    const std::string target = harness.module.functions[5].name;

    const BodyGenStatus status = GenerateBodies(harness, Touching(target, "", false));

    EXPECT_EQ(status, BodyGenStatus::kFailure);
    EXPECT_EQ(harness.failure.functionIndex, 5u);
    EXPECT_EQ(harness.failure.functionName, target);
    EXPECT_NE(harness.failure.message.find("单测注入异常"), std::string::npos);
    EXPECT_TRUE(harness.texts.empty()) << "失败路径不得填充部分产物";

    cn_compiler::parallel_codegen::reportBodyGenFailure(harness.diagnostics,
                                                        harness.failure);
    EXPECT_TRUE(harness.diagnostics.hasErrors());
    ASSERT_EQ(harness.diagnostics.getAll().size(), 1u);
    const std::string& message = harness.diagnostics.getAll()[0].message;
    EXPECT_NE(message.find("内部错误"), std::string::npos);
    EXPECT_NE(message.find("#5"), std::string::npos);
    EXPECT_NE(message.find(target), std::string::npos);
    EXPECT_NE(message.find("不产出部分汇编"), std::string::npos);
}

TEST(ParallelCodegenFailureTest, UnknownExceptionIsCaught) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", nullptr);
    Harness harness(12);
    const std::string target = harness.module.functions[3].name;

    const BodyGenStatus status = GenerateBodies(harness, Touching(target, "", true));

    EXPECT_EQ(status, BodyGenStatus::kFailure);
    EXPECT_EQ(harness.failure.functionIndex, 3u);
    EXPECT_EQ(harness.failure.functionName, target);
    EXPECT_NE(harness.failure.message.find("未知异常"), std::string::npos);
}

TEST(ParallelCodegenFailureTest, EmptyFunctionTextIsFailure) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", nullptr);
    Harness harness(12);
    const std::string target = harness.module.functions[2].name;

    const BodyGenStatus status = GenerateBodies(harness, Touching("", target, false));

    EXPECT_EQ(status, BodyGenStatus::kFailure);
    EXPECT_EQ(harness.failure.functionIndex, 2u);
    EXPECT_EQ(harness.failure.functionName, target);
    EXPECT_NE(harness.failure.message.find("函数文本为空"), std::string::npos);
}

TEST(ParallelCodegenFailureTest, FirstFailureIndexIsDeterministicMinimum) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", nullptr);
    Harness harness(12);
    // 多个函数同时失败：首错=最小函数序号（与线程调度顺序无关）
    const std::string first = harness.module.functions[4].name;
    const std::string second = harness.module.functions[7].name;
    auto combined = [first, second](Diagnostics&) -> std::unique_ptr<IGenerator> {
        struct MultiGenerator : public IGenerator {
            std::string a;
            std::string b;
            MultiGenerator(std::string first, std::string second)
                : a(std::move(first)), b(std::move(second)) {}
            void generateFunction(const ir::IRFunction& function,
                                  FunctionOutput& out) override {
                if (function.name == a || function.name == b) {
                    throw std::runtime_error("多任务失败");
                }
                out.text = "函数体\n";
                out.labelCount = 1;
            }
            bool constantPoolMiss() const override { return false; }
        };
        return std::make_unique<MultiGenerator>(first, second);
    };
    // 多轮复跑：线程抢号顺序每轮不同，最小失败序号必须恒定（判别力增强·P3）
    for (int round = 0; round < 20; ++round) {
        Harness roundHarness(12);
        SCOPED_TRACE(::testing::Message() << "round=" << round);
        const BodyGenStatus status = GenerateBodies(roundHarness, combined);
        EXPECT_EQ(status, BodyGenStatus::kFailure);
        EXPECT_EQ(roundHarness.failure.functionIndex, 4u);
        EXPECT_EQ(roundHarness.failure.functionName, first);
        EXPECT_NE(roundHarness.failure.message.find("多任务失败"), std::string::npos);
    }
}

// ==================== 正常路径零回归 ====================

TEST(ParallelCodegenFailureTest, NormalParallelPathKeepsFunctionOrder) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", nullptr);
    Harness harness(12);

    const BodyGenStatus status = GenerateBodies(harness, Touching("", "", false));

    EXPECT_EQ(status, BodyGenStatus::kSuccess);
    ASSERT_EQ(harness.texts.size(), 12u);
    for (std::size_t index = 0; index < harness.texts.size(); ++index) {
        EXPECT_EQ(harness.texts[index],
                  "函数体:" + harness.module.functions[index].name + "\n")
            << "函数序漂移（index=" << index << "）";
    }
    EXPECT_FALSE(harness.diagnostics.hasErrors());
}

// ==================== 线程创建失败（确定性降级） ====================

TEST(ParallelCodegenFailureTest, ThreadCreationFailureDegradesWithWarning) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv stats("CN_CODEGEN_STATS", "1");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", "thread-create");
    Harness harness(12);

    testing::internal::CaptureStderr();
    const BodyGenStatus status = GenerateBodies(harness, Touching("", "", false));
    const std::string err = testing::internal::GetCapturedStderr();

    EXPECT_EQ(status, BodyGenStatus::kSuccess);
    ASSERT_EQ(harness.texts.size(), 12u);
    EXPECT_EQ(harness.texts[0], "函数体:" + harness.module.functions[0].name + "\n");
    // 降级警告进入 Diagnostics（P2-2：--json 可见、计数一致），决策行照旧观测 workers
    EXPECT_NE(err.find("workers=1"), std::string::npos);
    EXPECT_EQ(harness.diagnostics.getWarningCount(), 1);
    ASSERT_EQ(harness.diagnostics.getAll().size(), 1u);
    const std::string& warning = harness.diagnostics.getAll()[0].message;
    EXPECT_NE(warning.find("线程创建失败"), std::string::npos);
    EXPECT_NE(warning.find("worker 数=1/4"), std::string::npos);
    EXPECT_FALSE(harness.diagnostics.hasErrors());
}

// ==================== 注入配置失败必须显形 ====================

TEST(ParallelCodegenFailureTest, InvalidInjectionFailsLoudly) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", "bogus-mode");
    Harness harness(12);

    const BodyGenStatus status = GenerateBodies(harness, Touching("", "", false));

    EXPECT_EQ(status, BodyGenStatus::kFailure);
    EXPECT_NE(harness.failure.message.find("非法"), std::string::npos);
}

TEST(ParallelCodegenFailureTest, InjectionIndexOutOfRangeFailsLoudly) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", "throw-std:99");
    Harness harness(12);

    const BodyGenStatus status = GenerateBodies(harness, Touching("", "", false));

    EXPECT_EQ(status, BodyGenStatus::kFailure);
    EXPECT_NE(harness.failure.message.find("越界"), std::string::npos);
}

TEST(ParallelCodegenFailureTest, InjectionIndexOverflowFailsLoudly) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "4");
    // 2^64+1：必须显式非法序号（不得回绕成 1 号函数误命中·P2-1）
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT",
                     "throw-std:18446744073709551617");
    Harness harness(12);

    const BodyGenStatus status = GenerateBodies(harness, Touching("", "", false));

    EXPECT_EQ(status, BodyGenStatus::kFailure);
    EXPECT_NE(harness.failure.message.find("非法"), std::string::npos);
    EXPECT_EQ(harness.failure.functionName, "");
}

TEST(ParallelCodegenFailureTest, InjectionWithoutParallelPathFailsLoudly) {
    ScopedEnv threads("CN_CODEGEN_THREADS", "1");
    ScopedEnv inject("CN_CODEGEN_FAILURE_INJECT", "throw-std:5");
    Harness harness(12);

    const BodyGenStatus status = GenerateBodies(harness, Touching("", "", false));

    EXPECT_EQ(status, BodyGenStatus::kFailure);
    EXPECT_NE(harness.failure.message.find("未进入并行路径"), std::string::npos);
}

TEST(ParallelCodegenFailureTest, UnknownStatusIsInternalErrorNotSuccess) {
    Diagnostics diagnostics;
    BodyGenFailure failure;
    bool emitTexts = true;
    // 未来新增枚举值未接线：必须报内部错误且不得允许拼接部分汇编（P2-3）
    const bool handled = cn_compiler::parallel_codegen::applyBodyGenStatus(
        static_cast<BodyGenStatus>(99), diagnostics, failure, emitTexts);

    EXPECT_TRUE(handled);
    EXPECT_FALSE(emitTexts);
    EXPECT_TRUE(diagnostics.hasErrors());
    ASSERT_EQ(diagnostics.getAll().size(), 1u);
    EXPECT_NE(diagnostics.getAll()[0].message.find("未知状态"), std::string::npos);
}
