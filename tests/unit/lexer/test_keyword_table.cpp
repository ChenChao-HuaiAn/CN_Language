// #1215 阶段A 机械锁：46 个关键字 -> TokenType 映射逐项核验（独立 oracle）。
// 背景：lexer.cpp 的 static_assert 只校验表序/首码点分组——若某词映射写错
//   （如 整8 -> Kw_Int16 且 整16 -> Kw_Int8 互换），顺序与分组检查都能通过，
//   词法则把关键字识别成错误 TokenType，语义层出现难查的静默偏差。
// 判据：用真实 Lexer 全链路（tokenize）逐个关键字断言类型与文本；本文件
//   内嵌独立的 46 对期望表（与实现表互为 oracle，实现改错即红）。
// 说明：GCC 7 不支持中文标识符，测试名用英文；注释/字符串可中文。
#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/lexer/token.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::TokenType;

namespace {

struct KeywordCase {
    const char* text;
    TokenType type;
};

// 46 个保留字及其唯一正确 TokenType（plans/024 §7：162-a 收口后的保留字表）
constexpr KeywordCase kKeywordCases[] = {
        {"不安全", TokenType::Kw_Unsafe},
        {"中断", TokenType::Kw_Break},
        {"假", TokenType::Kw_False},
        {"函数", TokenType::Kw_Function},
        {"友元", TokenType::Kw_Friend},
        {"变量", TokenType::Kw_Var},
        {"可选", TokenType::Kw_Optional},
        {"否则", TokenType::Kw_Else},
        {"如果", TokenType::Kw_If},
        {"字符", TokenType::Kw_Char},
        {"字符串", TokenType::Kw_String},
        {"导入", TokenType::Kw_Import},
        {"小数", TokenType::Kw_Double},
        {"布尔", TokenType::Kw_Bool},
        {"当", TokenType::Kw_While},
        {"循环", TokenType::Kw_For},
        {"情况", TokenType::Kw_Case},
        {"接口", TokenType::Kw_Interface},
        {"整128", TokenType::Kw_Int128},
        {"整16", TokenType::Kw_Int16},
        {"整32", TokenType::Kw_Int32},
        {"整64", TokenType::Kw_Int64},
        {"整8", TokenType::Kw_Int8},
        {"整数", TokenType::Kw_Int},
        {"无", TokenType::Kw_None},
        {"枚举", TokenType::Kw_Enum},
        {"正128", TokenType::Kw_UInt128},
        {"正16", TokenType::Kw_UInt16},
        {"正32", TokenType::Kw_UInt32},
        {"正64", TokenType::Kw_UInt64},
        {"正8", TokenType::Kw_UInt8},
        {"泛型", TokenType::Kw_Generic},
        {"浮32", TokenType::Kw_Float32},
        {"浮64", TokenType::Kw_Float64},
        {"父类", TokenType::Kw_Super},
        {"真", TokenType::Kw_True},
        {"空类型", TokenType::Kw_Void},
        {"类", TokenType::Kw_Class},
        {"结构体", TokenType::Kw_Struct},
        {"结果", TokenType::Kw_Result},
        {"继续", TokenType::Kw_Continue},
        {"联合体", TokenType::Kw_Union},
        {"自身", TokenType::Kw_Self},
        {"返回", TokenType::Kw_Return},
        {"选择", TokenType::Kw_Switch},
        {"默认", TokenType::Kw_Default},
};

}  // namespace

// 每个关键字的 Token 类型逐项对照（实现表与 oracle 表独立维护）。
TEST(KeywordTableTest, AllVerbTableMappings) {
    for (const auto& c : kKeywordCases) {
        Diagnostics diagnostics;
        Lexer lexer(std::string(c.text), "关键字映射测试.cn", diagnostics);
        const std::vector<cn_compiler::Token> tokens = lexer.tokenize();
        ASSERT_FALSE(diagnostics.hasErrors()) << "关键字=[" << c.text << "] 词法报错";
        ASSERT_GE(tokens.size(), 2u) << "关键字=[" << c.text << "]";
        EXPECT_EQ(tokens[0].getType(), c.type) << "关键字=[" << c.text << "]";
        EXPECT_EQ(tokens[0].getValue(), c.text) << "关键字=[" << c.text << "]";
        EXPECT_EQ(tokens[1].getType(), TokenType::EndOfFile) << "关键字=[" << c.text << "]";
    }
}

// 表规模与唯一性（词不重复、TokenType 不重复——重复即两词映射到同一类型）。
TEST(KeywordTableTest, TableCountAndUniqueness) {
    std::set<std::string> texts;
    std::set<int> types;
    for (const auto& c : kKeywordCases) {
        EXPECT_TRUE(texts.insert(c.text).second) << "重复词=[" << c.text << "]";
        EXPECT_TRUE(types.insert(static_cast<int>(c.type)).second)
            << "重复 TokenType：词=[" << c.text << "]";
    }
    EXPECT_EQ(texts.size(), 46u);
    EXPECT_EQ(types.size(), 46u);
}
