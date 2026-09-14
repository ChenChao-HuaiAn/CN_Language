// plans/019 阶段1（2026-09-10）：显式转移 转移() + 已转移使用检查 单元测试
// 覆盖：负形态九类精确消息断言（读值/赋值目标/传参/返回/再转移/取地址/标量
//       复制语义/非变量目标/拥有类型表达式位受限）+ 正形态四类零误伤
//       （指针转移链/类声明位转移/遮蔽新声明/字符串转移）。
// 权威锚定：Rust E0382（use of moved value，编译期硬错误）；
//           消息文本以 cn check 实测输出为准（2026-09-10 深度机 linux-x86_64 锚定）。
// 测试方式：Lexer + Parser 真实 AST -> SemanticAnalyzer 全链路（防虚假验收）。
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

struct MoveResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

MoveResult analyzeSource(const std::string& source) {
    MoveResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "转移检查测试.cn", diagnostics);
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

}  // namespace

// ==================== 负形态：精确消息断言 ====================

// 转移后读值（E0382 主形态）
TEST(MoveCheckTest, RejectReadAfterTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 {
    整64 基 = 5;
    整64* q = &基;
    整64* r = 转移(q);
    整64 v = *r + *q;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("已转移，不能继续使用"), std::string::npos);
}

// 转移后赋值目标
TEST(MoveCheckTest, RejectAssignTargetAfterTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 {
    整64 基 = 5;
    整64* q = &基;
    整64* r = 转移(q);
    q = r;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("已转移，不能继续使用"), std::string::npos);
}

// 转移后传参
TEST(MoveCheckTest, RejectArgAfterTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 消(整64* p) -> 空类型 { }
不安全 函数 坏() -> 整32 {
    整64 基 = 5;
    整64* q = &基;
    整64* r = 转移(q);
    消(q);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("已转移，不能继续使用"), std::string::npos);
}

// 转移后返回
TEST(MoveCheckTest, RejectReturnAfterTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整64* {
    整64 基 = 5;
    整64* q = &基;
    整64* r = 转移(q);
    返回 q;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("已转移，不能继续使用"), std::string::npos);
}

// 再转移（转移已转移变量）
TEST(MoveCheckTest, RejectDoubleTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 {
    整64 基 = 5;
    整64* q = &基;
    整64* r = 转移(q);
    整64* s = 转移(q);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("已转移，不能继续使用"), std::string::npos);
}

// 转移后取地址
TEST(MoveCheckTest, RejectAddressOfAfterTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 {
    整64 基 = 5;
    整64* q = &基;
    整64* r = 转移(q);
    整64** d = &q;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("已转移，不能继续使用"), std::string::npos);
}

// 标量误转移（复制语义）
TEST(MoveCheckTest, RejectScalarTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 {
    整32 a = 5;
    整32 b = 转移(a);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("具有复制语义，无需转移"), std::string::npos);
}

// 转移目标非变量（解引用形态）
TEST(MoveCheckTest, RejectNonVariableTransferTarget) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 {
    整64 基 = 5;
    整64* q = &基;
    整64* r = 转移(*q);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("转移目标须为变量"), std::string::npos);
}

// 拥有资源类型（类）表达式位转移受限（仅声明初始化位）
TEST(MoveCheckTest, RejectClassTransferAtExprPosition) {
    auto r = analyzeSource(R"CN(类 甲 { 公开: 整64 值; }
不安全 函数 消(甲 a) -> 空类型 { }
不安全 函数 坏() -> 整32 {
    甲 a;
    消(转移(a));
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("仅支持声明初始化位"), std::string::npos);
}

// ==================== 正形态：零误伤 ====================

// 指针转移链 + 解引用读（新变量接管）
TEST(MoveCheckTest, AcceptPointerTransferChain) {
    auto r = analyzeSource(R"CN(不安全 函数 好() -> 整32 {
    整64 基 = 55;
    整64* q = &基;
    整64* r = 转移(q);
    整64 v = *r;
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 类声明位转移 + 新变量成员访问（遮蔽语义另测）
TEST(MoveCheckTest, AcceptClassTransferAtDeclPosition) {
    auto r = analyzeSource(R"CN(类 甲 { 公开: 整64 值; }
不安全 函数 好() -> 整32 {
    甲 a;
    甲 b = 转移(a);
    b.值 = 7;
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 遮蔽：外层已转移，内层同名新声明不受影响
TEST(MoveCheckTest, AcceptShadowingNewDeclaration) {
    auto r = analyzeSource(R"CN(不安全 函数 好() -> 整32 {
    整64 基 = 5;
    {
        整64* q = &基;
        整64* r = 转移(q);
    }
    {
        整64* q = &基;
        整64 v = *q;
    }
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 字符串转移 + 新变量使用
TEST(MoveCheckTest, AcceptStringTransfer) {
    auto r = analyzeSource(R"CN(不安全 函数 好() -> 整32 {
    字符串 甲 = "左";
    字符串 乙 = 转移(甲);
    打印(乙);
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}
