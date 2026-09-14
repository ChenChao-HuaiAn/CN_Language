// 自举前置 4 项语言缺口单元测试（plans/004，2026-08）
// 覆盖：
//   A-1 字符串[i] 逐字节访问（语义：下标结果类型 字符）
//   A-2 选择 全分支返回识别（bodyGuaranteesReturn：选择+默认 全返回、如果/否则
//       全返回、缺少默认分支仍报错）
//   A-3a 泛型实例化类型作函数参数（parseParamDecl 模板判定 + registerFunction
//        实例符号名统一：向量<字符串> 词表 -> 向量$字符串）
//   A-3b 泛型类实例作返回类型（canConvertType 模板/实例名统一：
//        返回 向量<字符串> 与 向量$字符串 视为同一类型）
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路（真实 AST）
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
    Lexer lexer(source, "自举前置缺口测试.cn", diagnostics);
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

// ==================== A-1：字符串[i] 逐字节访问 ====================

// 字符串下标：结果类型 字符，可赋给 字符 变量（O(1) 字节视图）
TEST(BootstrapGapTest, StringIndexByteAccess) {
    auto r = analyzeSource(R"CN(
不安全 函数 主() -> 整32 {
    字符串 s = "ABC";
    字符 c = s[0];
    打印行(整32(c));
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 字符串下标 + 整64 索引混用（词法器逐字节扫描模式）
TEST(BootstrapGapTest, StringIndexI64Index) {
    auto r = analyzeSource(R"CN(
不安全 函数 主() -> 整32 {
    字符串 s = "你好";
    整64 i = 0;
    整64 总和 = 0;
    当 (i < 字符串长度(s)) {
        总和 += 整64(s[i]);
        i++;
    }
    打印行(总和);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 结构体 字符串 字段下标（成员表达式路径）
TEST(BootstrapGapTest, StringFieldIndex) {
    auto r = analyzeSource(R"CN(
结构体 记录 {
    字符串 标题;
}
不安全 函数 主() -> 整32 {
    记录 名片 = 记录 { 标题 = "你好" };
    字符 c = 名片.标题[0];
    打印行(整32(c));
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== A-2：选择 全分支返回识别 ====================

// 选择 全分支返回（全部 情况 + 默认 均返回）：不报"缺少返回语句"
TEST(BootstrapGapTest, SwitchAllBranchReturn) {
    auto r = analyzeSource(R"CN(
枚举 类型名 { 数字, 文本, 布尔值 }
不安全 函数 类型转名称(类型名 t) -> 字符串 {
    选择 (t) {
        情况 数字:
            返回 "数字";
        情况 文本:
            返回 "文本";
        情况 布尔值:
            返回 "布尔值";
        默认:
            返回 "未知";
    }
}
不安全 函数 主() -> 整32 {
    打印行(类型转名称(类型名.数字));
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 字符串选择 + 多值分组（情况 "继续", "暂停":）全分支返回
TEST(BootstrapGapTest, SwitchStringMultiValueReturn) {
    auto r = analyzeSource(R"CN(
不安全 函数 命令处理(字符串 命令) -> 整64 {
    选择 (命令) {
        情况 "开始":
            返回 1;
        情况 "继续", "暂停":
            返回 3;
        默认:
            返回 0;
    }
}
不安全 函数 主() -> 整32 {
    打印行(命令处理("开始"));
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 缺少 默认 分支：选择 不保证全路径返回 -> 仍报"缺少返回语句"
TEST(BootstrapGapTest, SwitchMissingDefaultStillErrors) {
    auto r = analyzeSource(R"CN(
枚举 类型名 { 数字, 文本 }
不安全 函数 类型转名称(类型名 t) -> 字符串 {
    选择 (t) {
        情况 数字:
            返回 "数字";
        情况 文本:
            返回 "文本";
    }
}
不安全 函数 主() -> 整32 {
    打印行(类型转名称(类型名.数字));
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("缺少返回语句"), std::string::npos) << r.messages;
}

// 如果/否则 双分支均返回：不报"缺少返回语句"
TEST(BootstrapGapTest, IfElseAllBranchReturn) {
    auto r = analyzeSource(R"CN(
不安全 函数 取绝对值(整64 x) -> 整64 {
    如果 (x >= 0) {
        返回 x;
    } 否则 {
        返回 -x;
    }
}
不安全 函数 主() -> 整32 {
    打印行(取绝对值(-5));
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== A-3a：泛型实例化类型作函数参数 ====================

// 向量<字符串> 参数：解析（parseParamDecl 模板判定）+ 注册（实例符号名统一）
//   + 方法调用（参数对象.大小() 成员访问）
TEST(BootstrapGapTest, GenericClassParam) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
类 向量 {
私有:
    T* 数据;
    整64 元素数量;
    整64 数组容量;
公开:
    不安全 函数 向量() {
        数据 = 无;
        元素数量 = 0;
        数组容量 = 0;
    }
    常量 函数 大小() -> 整64 {
        返回 元素数量;
    }
    不安全 函数 追加(T 值) -> 结果<空类型, 整32> {
        数据[元素数量] = 值;
        元素数量++;
        返回 正常();
    }
}
不安全 函数 词频统计(向量<字符串> 词表) -> 整64 {
    返回 词表.大小();
}
不安全 函数 主() -> 整32 {
    向量<字符串> 表 = 向量<字符串>();
    结果<空类型, 整32> 加 = 表.追加("你好");
    如果 (加.正常) {
        打印行(词频统计(表));
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 多参数 + 不同类型实参（向量<整64> 第二实例）
TEST(BootstrapGapTest, GenericClassParamMultiple) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
类 向量 {
私有:
    T* 数据;
    整64 元素数量;
公开:
    不安全 函数 向量() {
        数据 = 无;
        元素数量 = 0;
    }
    不安全 函数 追加(T 值) -> 结果<空类型, 整32> {
        数据[元素数量] = 值;
        元素数量++;
        返回 正常();
    }
    常量 函数 大小() -> 整64 {
        返回 元素数量;
    }
}
不安全 函数 双表大小(向量<字符串> 甲, 向量<整64> 乙) -> 整64 {
    返回 甲.大小() + 乙.大小();
}
不安全 函数 主() -> 整32 {
    向量<字符串> 甲 = 向量<字符串>();
    向量<整64> 乙 = 向量<整64>();
    打印行(双表大小(甲, 乙));
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== A-3b：泛型类实例作返回类型 ====================

// 返回 向量<字符串>：canConvertType 模板/实例名统一
//   （返回 表 的值类型 向量$字符串 与返回类型 向量<字符串> 视为同一类型）
TEST(BootstrapGapTest, GenericClassReturn) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
类 向量 {
私有:
    T* 数据;
    整64 元素数量;
公开:
    不安全 函数 向量() {
        数据 = 无;
        元素数量 = 0;
    }
    不安全 函数 追加(T 值) -> 结果<空类型, 整32> {
        数据[元素数量] = 值;
        元素数量++;
        返回 正常();
    }
    常量 函数 大小() -> 整64 {
        返回 元素数量;
    }
}
不安全 函数 构建词表() -> 向量<字符串> {
    向量<字符串> 表 = 向量<字符串>();
    结果<空类型, 整32> 加 = 表.追加("你好");
    如果 (!加.正常) {
        返回 表;
    }
    返回 表;
}
不安全 函数 主() -> 整32 {
    向量<字符串> 表 = 构建词表();
    打印行(表.大小());
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 泛型函数（泛型 <类型 T> 函数）实例返回容器：subst 后类型名统一
TEST(BootstrapGapTest, GenericFuncReturnContainer) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
类 向量 {
私有:
    T* 数据;
    整64 元素数量;
公开:
    不安全 函数 向量() {
        数据 = 无;
        元素数量 = 0;
    }
    不安全 函数 追加(T 值) -> 结果<空类型, 整32> {
        数据[元素数量] = 值;
        元素数量++;
        返回 正常();
    }
    常量 函数 大小() -> 整64 {
        返回 元素数量;
    }
}
泛型 <类型 T>
不安全 函数 新建向量(T 首元素) -> 向量<T> {
    向量<T> 表 = 向量<T>();
    结果<空类型, 整32> 加 = 表.追加(首元素);
    如果 (!加.正常) {
        返回 表;
    }
    返回 表;
}
不安全 函数 主() -> 整32 {
    向量<字符串> 表 = 新建向量<字符串>("你好");
    打印行(表.大小());
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}
