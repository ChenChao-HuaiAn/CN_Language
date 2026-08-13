// 格式化/布尔转字符串运行时API单元测试（Task 2.9）
// 覆盖：__cn_str_from_bool（真/假）、__cn_format 占位符
//      （%d/%u/%f/%s/%c/%x/%X/%o/%p 混合、中文、空串、多参数）
// 测试方式：直接调用运行时 extern "C" 函数（真实实现，非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <cstring>

#include "runtime/runtime.hpp"

namespace {

// 便捷断言：比较字符串相等（UTF-8 字节）
void expectStrEq(const char* actual, const char* expected) {
    ASSERT_NE(actual, nullptr);
    EXPECT_STREQ(actual, expected);
}

} // namespace

// 布尔转字符串：真 -> "真"、假 -> "假"
TEST(FormatApiTest, StrFromBool) {
    char* t = __cn_str_from_bool(1);
    expectStrEq(t, "真");
    std::free(t);
    char* f = __cn_str_from_bool(0);
    expectStrEq(f, "假");
    std::free(f);
    char* t2 = __cn_str_from_bool(42);  // 非零均视为真
    expectStrEq(t2, "真");
    std::free(t2);
}

// 格式化：整数 %d
TEST(FormatApiTest, FormatInt) {
    char* s = __cn_format("值%d", 42);
    expectStrEq(s, "值42");
    std::free(s);
}

// 格式化：浮点 %f（默认6位小数）
TEST(FormatApiTest, FormatFloat) {
    char* s = __cn_format("浮点%f", 3.14);
    expectStrEq(s, "浮点3.140000");
    std::free(s);
}

// 格式化：字符串 %s（中文）
TEST(FormatApiTest, FormatString) {
    char* s = __cn_format("%s 世界", "你好");
    expectStrEq(s, "你好 世界");
    std::free(s);
}

// 格式化：字符 %c / 十六 %x / 大写 %X / 八 %o
TEST(FormatApiTest, FormatCharHexOctal) {
    char* s = __cn_format("字符%c 十六%x 大写%X 八%o", 'A', 255, 255, 8);
    expectStrEq(s, "字符A 十六ff 大写FF 八10");
    std::free(s);
}

// 格式化：无符号 %u
TEST(FormatApiTest, FormatUnsigned) {
    char* s = __cn_format("无符号%u", 4000000000U);
    expectStrEq(s, "无符号4000000000");
    std::free(s);
}

// 格式化：指针 %p（输出为十六进制地址，格式随平台（MSVC 无 0x 前缀），断言"指针"前缀）
TEST(FormatApiTest, FormatPointer) {
    int local = 0;
    char* s = __cn_format("指针%p", static_cast<void*>(&local));
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(std::strncmp(s, "指针", std::strlen("指针")), 0);
    EXPECT_GT(std::strlen(s), std::strlen("指针"));  // 地址非空
    std::free(s);
}

// 格式化：多参数混合（4 参数）
TEST(FormatApiTest, FormatMixedArgs) {
    char* s = __cn_format("%d + %f + %s + %c", 1, 2.5, "三", 'X');
    expectStrEq(s, "1 + 2.500000 + 三 + X");
    std::free(s);
}

// 格式化：空格式串
TEST(FormatApiTest, FormatEmpty) {
    char* s = __cn_format("");
    expectStrEq(s, "");
    std::free(s);
}

// 格式化：仅文字无占位符
TEST(FormatApiTest, FormatPlainText) {
    char* s = __cn_format("纯文本输出");
    expectStrEq(s, "纯文本输出");
    std::free(s);
}

// 格式化：与 E2E 15 完全相同的调用（验证 MSVC 变参 %d+%f 混合读取）
TEST(FormatApiTest, FormatDAndFExact) {
    char* s = __cn_format("值%d 浮点%f", 42, 3.14);
    expectStrEq(s, "值42 浮点3.140000");
    std::free(s);
}
