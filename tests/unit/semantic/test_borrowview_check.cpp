// plans/019 阶段3 扩展（A21 借出视图生命周期，2026-09-12 第七十七轮）单元测试
// 覆盖：负形态（同作用域失效主形态 / 结果读取形态 / 设置覆盖 / 赋值位绑定 /
//       容器先亡跨作用域逃逸 / **成员链接收者** / **调用点同源互斥**）精确消息
//       断言 + 正形态零误伤（立即消费 / 显式复制 / 顺序正确消费后修改 / 非字符串
//       元素容器 / 修改点后不使用 / 循环内每轮重新借出 / 扩容类修改点不在面内 /
//       借出视图单独作实参）。
// 权威锚定：Rust 借用检查器（E0502 同作用域可变借用冲突 / E0597 借用比 owner
//   长寿）——CN 无借用类型，以「方法名清单 + 来源登记 + NLL 顺序近似活跃区间」
//   达成同一拦截（消息文本以 cn check 实测输出为准，2026-09-12 深度机锚定）。
// 测试方式：Lexer + Parser 真实 AST -> SemanticAnalyzer 全链路（防虚假验收）；
//   容器以内联最小泛型类定义（实例化名 向量$字符串/映射$整64$字符串 与 stdlib
//   同构，无需模块导入——检查面判定按实例化名，不依赖 stdlib 源码）。
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

struct BorrowResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

BorrowResult analyzeSource(const std::string& source) {
    BorrowResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "借出视图检查测试.cn", diagnostics);
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

// 最小向量定义（实例化名 向量$字符串——与 stdlib 同名同构：借出方法 元素/读取，
//   失效方法 删除/设置，扩容类 追加/预留 不在失效面）
const char* k向量定义 = R"CN(泛型 <类型 T>
类 向量 {
私有:
    T* 数据;
公开:
    不安全 函数 向量() { 数据 = 无; }
    不安全 函数 元素(整64 位置) -> T { 返回 数据[位置]; }
    不安全 函数 读取(整64 位置) -> 结果<T, 整32> { 返回 正常(数据[位置]); }
    不安全 函数 追加(T 值) -> 结果<空类型, 整32> { 返回 正常(); }
    不安全 函数 设置(整64 位置, T 值) -> 结果<空类型, 整32> { 返回 正常(); }
    不安全 函数 删除(整64 位置) -> 结果<空类型, 整32> { 返回 正常(); }
    不安全 函数 清空() { }
}
)CN";

// 最小映射定义（实例化名 映射$整64$字符串——值侧借出方法 获取，失效方法 删除）
const char* k映射定义 = R"CN(泛型 <类型 K, 类型 V>
类 映射 {
私有:
    V* 值表;
公开:
    不安全 函数 映射() { 值表 = 无; }
    不安全 函数 获取(K 键) -> 结果<V, 整32> { 返回 正常(值表[0]); }
    不安全 函数 删除(K 键) -> 布尔 { 返回 真; }
}
)CN";

}  // namespace

// ==================== 负形态：同作用域失效（E0502 对标） ====================

// 主形态：向量元素借出 × 删除 × 后用
TEST(BorrowViewCheckTest, RejectElemAfterRemove) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 坏() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 整32(字符串长度(借出));
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("借出视图 '借出'"), std::string::npos);
    EXPECT_NE(r.messages.find("容器失效点"), std::string::npos);
    EXPECT_NE(r.messages.find("删除"), std::string::npos);
}

// 结果形态：向量读取 × 设置覆盖 × 后用（结果.值 是借出句柄）
TEST(BorrowViewCheckTest, RejectReadResultAfterOverwrite) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 坏() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    结果<字符串, 整32> 读 = 表.读取(0);
    结果<空类型, 整32> 设 = 表.设置(0, 字符串复制("新值"));
    如果 (读.正常) {
        返回 整32(字符串长度(读.值));
    }
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("借出视图 '读'"), std::string::npos);
    EXPECT_NE(r.messages.find("设置"), std::string::npos);
}

// 映射值侧借出 × 删除 × 后用
TEST(BorrowViewCheckTest, RejectMapGetAfterRemove) {
    auto r = analyzeSource(std::string(k映射定义) + R"CN(不安全 函数 坏() -> 整32 {
    映射<整64, 字符串> 表 = 映射<整64, 字符串>();
    结果<字符串, 整32> 取 = 表.获取(1);
    布尔 删 = 表.删除(1);
    如果 (取.正常) {
        返回 整32(字符串长度(取.值));
    }
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("借出视图 '取'"), std::string::npos);
}

// 赋值位绑定形态：s 先声明、后借出（活跃区间从赋值点起算）
TEST(BorrowViewCheckTest, RejectAssignFormBinding) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 坏() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = "";
    借出 = 表.元素(0);
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 整32(字符串长度(借出));
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("借出视图 '借出'"), std::string::npos);
}

// ==================== 负形态：容器先亡（E0597 对标） ====================

// 块内容器 → 借出赋给块外变量 → 块外使用
TEST(BorrowViewCheckTest, RejectContainerDiesBeforeUse) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 坏() -> 整32 {
    字符串 借出 = "";
    如果 (1 == 1) {
        向量<字符串> 表 = 向量<字符串>();
        借出 = 表.元素(0);
    }
    返回 整32(字符串长度(借出));
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("容器作用域结束后"), std::string::npos);
}

// ==================== 正形态：零误伤 ====================

// 立即消费（借出不绑定）——修改点在后也不报
TEST(BorrowViewCheckTest, AllowImmediateConsume) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    如果 (字符串比较(表.元素(0), "甲")) { 返回 1; }
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 显式 字符串复制 取拥有副本后容器修改（副本独立于容器）
TEST(BorrowViewCheckTest, AllowExplicitCopy) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 副本 = 字符串复制(表.元素(0));
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 整32(字符串长度(副本));
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 借出 → 消费 → 容器修改（NLL 活跃区间已结束）
TEST(BorrowViewCheckTest, AllowUseThenRemove) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    整32 长度 = 整32(字符串长度(借出));
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 长度;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 借出后容器修改但不再使用借出
TEST(BorrowViewCheckTest, AllowRemoveWithoutLaterUse) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 非字符串元素容器：元素()=值语义深拷贝，非借用面
TEST(BorrowViewCheckTest, AllowNumericElemContainer) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<整64> 表 = 向量<整64>();
    整64 借出值 = 表.元素(0);
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 整32(借出值);
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 扩容类修改点（追加）不在失效面：字符串句柄指向独立堆块，槽位搬移不失效
TEST(BorrowViewCheckTest, AllowAppendAfterBorrow) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    结果<空类型, 整32> 增 = 表.追加(字符串复制("填充"));
    返回 整32(字符串长度(借出));
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 循环内每轮重新借出（每轮消费后再修改——下一轮重建借出，NLL 正确）
TEST(BorrowViewCheckTest, AllowRebindEachIteration) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    整64 i = 0;
    当 (i < 3) {
        字符串 行 = 表.元素(i);
        整32 长度 = 整32(字符串长度(行));
        结果<空类型, 整32> 删 = 表.删除(0);
        i++;
    }
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 非字符串元素容器的 清空 不入面（向量 清空=仅计数归零不释放）；
//   且字符串容器的 追加（扩容）不入面——防「过度拦截」回归
TEST(BorrowViewCheckTest, AllowVectorClearOnStringElem) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    表.清空();
    返回 整32(字符串长度(借出));
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// ==================== 77-a 扩展：成员链接收者 / 调用点同源互斥 ====================

// 最小类定义（字段持字符串元素容器——成员链接收者形态；类定义末尾无分号=CN 语法）
const char* k货架类定义 = R"CN(类 货架 {
公开:
    向量<字符串> 表;
    不安全 函数 货架() {
        表 = 向量<字符串>();
    }
}
)CN";

// 负形态：成员链接收者（架.表.元素(0) × 架.表.删除(0)）——容器引用键=架.表
TEST(BorrowViewCheckTest, RejectMemberChainBorrow) {
    auto r = analyzeSource(std::string(k向量定义) + std::string(k货架类定义) +
        R"CN(不安全 函数 坏() -> 整32 {
    货架 架 = 货架();
    字符串 借出 = 架.表.元素(0);
    结果<空类型, 整32> 删 = 架.表.删除(0);
    返回 整32(字符串长度(借出));
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("架.表"), std::string::npos);
    EXPECT_NE(r.messages.find("容器失效点"), std::string::npos);
}

// 负形态：调用点同源互斥（容器与其借出视图同时作实参传入）
TEST(BorrowViewCheckTest, RejectContainerAndViewAsArgs) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(
不安全 函数 帮忙(向量<字符串> 表, 字符串 视图) -> 整32 {
    结果<空类型, 整32> 删 = 表.删除(0);
    如果 (!删.正常) { 返回 0; }
    返回 整32(字符串长度(视图));
}
不安全 函数 坏() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    返回 帮忙(表, 借出);
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("同时作为实参传入"), std::string::npos);
}

// 正形态：借出视图单独作实参（容器不在同调用中——不误报）
TEST(BorrowViewCheckTest, AllowViewArgAlone) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(
不安全 函数 量(字符串 值) -> 整32 {
    返回 整32(字符串长度(值));
}
不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    整32 长度 = 量(借出);
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 长度;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 正形态：容器与无关字符串同调用（非同源——不误报）
TEST(BorrowViewCheckTest, AllowContainerWithUnrelatedArg) {
    auto r = analyzeSource(std::string(k向量定义) + R"CN(
不安全 函数 两长(字符串 甲, 字符串 乙) -> 整32 {
    返回 整32(字符串长度(甲) + 字符串长度(乙));
}
不安全 函数 好() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    字符串 借出 = 表.元素(0);
    字符串 无关 = 字符串复制("无关内容甲乙丙丁戊己庚辛壬癸");
    整32 长度 = 两长(借出, 无关);
    结果<空类型, 整32> 删 = 表.删除(0);
    返回 长度;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}
