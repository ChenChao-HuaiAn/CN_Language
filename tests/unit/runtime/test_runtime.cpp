// 运行时库单元测试（Task 1.8）
// 覆盖：打印行（字符串/整数/浮点）输出、内存管理API（分配/释放/重分配/复制/置零）、
//       程序入口 entry 调用（模拟 cn_main 主函数）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <string>

#include "runtime/runtime.hpp"

// 模拟CN语言 主 函数（由编译器生成的汇编提供），返回固定值供 entry 验证
extern "C" int cn_main() {
    return 42;
}

namespace {

// 辅助：捕获 printLine 系列函数输出
std::string captureOutput(void (*func)()) {
    testing::internal::CaptureStdout();
    func();
    return testing::internal::GetCapturedStdout();
}

} // namespace

// ==================== IO API：打印行 ====================

// 打印行（字符串）：输出文本并换行
TEST(RuntimeTest, PrintLineString) {
    std::string out = captureOutput([] { printLine("hello"); });
    EXPECT_EQ(out, "hello\n");
}

// 打印行（字符串）：支持空字符串（仅换行）
TEST(RuntimeTest, PrintLineEmptyString) {
    std::string out = captureOutput([] { printLine(""); });
    EXPECT_EQ(out, "\n");
}

// 打印行（整数）：输出十进制整数并换行
TEST(RuntimeTest, PrintLineInt) {
    std::string out = captureOutput([] { printLineInt(42); });
    EXPECT_EQ(out, "42\n");
}

// 打印行（整数）：支持负数
TEST(RuntimeTest, PrintLineNegativeInt) {
    std::string out = captureOutput([] { printLineInt(-7); });
    EXPECT_EQ(out, "-7\n");
}

// 打印行（浮点数）：输出浮点数值（默认6位小数）并换行
TEST(RuntimeTest, PrintLineFloat) {
    std::string out = captureOutput([] { printLineFloat(3.14); });
    EXPECT_NE(out.find("3.14"), std::string::npos);
    EXPECT_EQ(out.back(), '\n');
}

// ==================== 内存管理API ====================

// 分配与释放：分配非空内存，写入并读回验证
TEST(RuntimeTest, AllocAndFree) {
    void* p = cn_alloc(64);
    ASSERT_NE(p, nullptr);
    std::memset(p, 0xAB, 16);
    EXPECT_EQ(static_cast<unsigned char*>(p)[0], 0xAB);
    EXPECT_EQ(static_cast<unsigned char*>(p)[15], 0xAB);
    cn_free(p);
}

// 分配失败：超大尺寸返回nullptr（不崩溃）
TEST(RuntimeTest, AllocFailureReturnsNull) {
    void* p = cn_alloc(static_cast<std::size_t>(-1) / 2);
    EXPECT_EQ(p, nullptr);
}

// 置零内存：将缓冲区全部清零
TEST(RuntimeTest, MemsetZero) {
    unsigned char buf[32];
    std::memset(buf, 0xFF, sizeof(buf));
    cn_memset(buf, sizeof(buf));
    for (std::size_t i = 0; i < sizeof(buf); ++i) {
        EXPECT_EQ(buf[i], 0x00);
    }
}

// 复制内存：源数据完整复制到目标
TEST(RuntimeTest, MemcpyCopiesBytes) {
    const char src[] = "CN语言运行时";
    char dst[32] = {0};
    cn_memcpy(dst, src, sizeof(src));
    EXPECT_EQ(std::memcmp(dst, src, sizeof(src)), 0);
}

// 重新分配：扩大缓冲区后数据保留且新区域可写
TEST(RuntimeTest, ReallocPreservesData) {
    void* p = cn_alloc(16);
    ASSERT_NE(p, nullptr);
    std::memset(p, 0x5A, 16);
    void* q = cn_realloc(p, 64);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(static_cast<unsigned char*>(q)[15], 0x5A);
    cn_free(q);
}

// ==================== 程序入口 ====================

// entry：转发到 cn_main（CN语言 主 函数），返回其返回值
TEST(RuntimeTest, EntryCallsMain) {
    EXPECT_EQ(entry(0, nullptr), 42);
}

// entry：argc/argv 参数不传递时不影响返回值（阶段一未使用命令行参数）
TEST(RuntimeTest, EntryIgnoresArgs) {
    char* argv[] = {const_cast<char*>("cn_program")};
    EXPECT_EQ(entry(1, argv), 42);
}
