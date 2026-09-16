// AST 打印器单元测试（253-a·D25 低覆盖语料补强，2026-09-16）
// 覆盖 AstPrinter 各 visit 分支：解析一段语法面丰富的源码（函数/变量/二元/三元/
//   如果/否则/当/选择/情况/默认/结构体/枚举/类/继承/构造/方法/返回）后打印到
//   内存流（std::ostringstream 注入），断言各节点标记出现。
// 测试方式：Lexer + Parser + AstPrinter 全链路（非 Mock）
// 注意：测试名/标识符必须使用英文（GCC 9 无 UTF-8 标识符；注释/断言目标文本可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast_printer.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::AstPrinter;
using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;

namespace {

// 辅助：解析源码并打印到字符串（返回打印文本；解析失败返回空并置 ok=false）
struct PrintResult {
    std::string text;
    bool ok = false;
};

PrintResult parseAndPrint(const std::string& source) {
    PrintResult r;
    Diagnostics diagnostics = Diagnostics();
    Lexer lexer(source, "ast_printer_test.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    if (diagnostics.hasErrors() || program == nullptr) return r;
    std::ostringstream out;
    AstPrinter printer(out);
    printer.print(program.get());
    r.text = out.str();
    r.ok = true;
    return r;
}

// 辅助：断言文本包含全部片段（第一个缺失的即报出）
::testing::AssertionResult textContains(const std::string& text,
                                        std::initializer_list<const char*> parts) {
    for (const char* part : parts) {
        if (text.find(part) == std::string::npos)
            return ::testing::AssertionFailure()
                   << "输出缺少片段: " << part << "\n--- 实际输出 ---\n" << text;
    }
    return ::testing::AssertionSuccess();
}

}  // namespace

TEST(AstPrinter, StatementsFullyPrinted) {
    const std::string source = R"CN(
函数 主() -> 整32 {
    整32 合计 = 0;
    如果 (合计 > 0) {
        合计 = 合计 - 1;
    } 否则 {
        合计 = 合计 + 1;
    }
    当 (合计 < 10) {
        合计 = 合计 + 2;
    }
    选择 (合计) {
    情况 0:
        合计 = 1;
    默认:
        合计 = 2;
    }
    返回 合计;
}
)CN";
    PrintResult r = parseAndPrint(source);
    ASSERT_TRUE(r.ok);
    EXPECT_TRUE(textContains(r.text, {"程序", "函数", "如果", "当", "选择", "返回"}));
}

TEST(AstPrinter, StructEnumExprPrinted) {
    const std::string source = R"CN(
结构体 坐标 {
    整32 x;
    整32 y;
}

枚举 颜色 {
    红,
    绿
}

函数 取值(整32 输入) -> 整32 {
    坐标 点 = 坐标{ x = 1, y = 2 };
    整32 判 = 输入 > 0 ? 点.x : 点.y;
    返回 判 * (输入 + 1);
}
)CN";
    PrintResult r = parseAndPrint(source);
    ASSERT_TRUE(r.ok);
    EXPECT_TRUE(textContains(r.text, {"结构体声明", "枚举声明", "字段", "成员"}));
}

TEST(AstPrinter, ClassInheritancePrinted) {
    const std::string source = R"CN(
类 动物 {
公开:
    整32 年龄;

    函数 动物() {
        年龄 = 0;
    }

    函数 叫() -> 整32 {
        返回 年龄;
    }
}

类 猫 : 动物 {
公开:
    函数 叫() -> 整32 {
        返回 年龄 + 1;
    }
}

函数 主() -> 整32 {
    猫 咪 = 猫();
    返回 咪.叫();
}
)CN";
    PrintResult r = parseAndPrint(source);
    ASSERT_TRUE(r.ok);
    EXPECT_TRUE(textContains(r.text, {"类", "年龄"}));
}
