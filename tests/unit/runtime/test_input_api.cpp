// 输入运行时 API 单元测试（Task 6.2，规格书10.6 输入 API）
// 覆盖：__cn_read_line（含空格行、空行、EOF）/
//       __cn_read_int（合法/非法/EOF）/ __cn_read_float /
//       __cn_print_err（重定向 stderr 捕获）
// 测试技术：临时文件重定向 stdin（freopen_s）——每用例独立临时文件，
//   afterEach 恢复 stdin 并清理。stderr 重定向用 freopen_s 到临时文件。
// 注意：测试名英文（GCC 7 不支持中文标识符）；中文仅注释
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <io.h>  // _dup/_dup2/_fileno（stderr 重定向捕获）
#else
#include <unistd.h>  // dup/dup2/close/fileno（POSIX）
#endif

// 安全的文件打开/重定向（MSVC /W4 /WX 下 fopen/freopen 触发 C4996 警告即错误；
//   fopen_s/freopen_s 为 C11 安全版本，MSVC 无警告）
#ifdef _WIN32
#define TEST_FOPEN(path, mode, f) (fopen_s(&(f), (path), (mode)) == 0)
#define TEST_FREOPEN(path, mode, stream, f) \
    (freopen_s(&(f), (path), (mode), (stream)) == 0)
#else
#define TEST_FOPEN(path, mode, f) ((f) = std::fopen((path), (mode))) != nullptr
#define TEST_FREOPEN(path, mode, stream, f) \
    (((f) = std::freopen((path), (mode), (stream))) != nullptr \
     ? true : (std::fclose(f), false))
#endif

#include "runtime/runtime.hpp"

namespace {

// 临时文件路径（每用例独立，避免并行冲突）
std::string tempInputPath() {
    static int counter = 0;
    return std::string("test_input_api_") + std::to_string(counter++) + ".txt";
}

// 设置 stdin 内容（写入临时文件并 freopen 为 stdin）
void setStdinText(const std::string& text, const std::string& path) {
    FILE* out = nullptr;
    ASSERT_TRUE(TEST_FOPEN(path.c_str(), "wb", out));
    std::fwrite(text.data(), 1, text.size(), out);
    std::fclose(out);
    FILE* fp = nullptr;
    ASSERT_TRUE(TEST_FREOPEN(path.c_str(), "rb", stdin, fp));
}

// 恢复 stdin 为控制台（Windows 下 NUL，POSIX 下 /dev/null）并删除临时文件
void restoreStdin(const std::string& path) {
    std::remove(path.c_str());
    FILE* fp = nullptr;
#ifdef _WIN32
    TEST_FREOPEN("NUL", "r", stdin, fp);
#else
    TEST_FREOPEN("/dev/null", "r", stdin, fp);
#endif
}

} // namespace

// 读取行：普通行（不含换行）
TEST(InputApiTest, ReadLineBasic) {
    const std::string path = tempInputPath();
    setStdinText("hello world\n", path);
    char* line = __cn_read_line();
    restoreStdin(path);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "hello world");
    __cn_str_free(line);
}

// 读取行：含前导/尾部空格的行（保留空格，剥换行）
TEST(InputApiTest, ReadLineSpaces) {
    const std::string path = tempInputPath();
    setStdinText("  spaced line  \n", path);
    char* line = __cn_read_line();
    restoreStdin(path);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "  spaced line  ");
    __cn_str_free(line);
}

// 读取行：空行（只有换行，返回空字符串）
TEST(InputApiTest, ReadLineEmpty) {
    const std::string path = tempInputPath();
    setStdinText("\n", path);
    char* line = __cn_read_line();
    restoreStdin(path);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "");
    __cn_str_free(line);
}

// 读取行：EOF（无任何数据，返回 nullptr）
TEST(InputApiTest, ReadLineEof) {
    const std::string path = tempInputPath();
    setStdinText("", path);
    char* line = __cn_read_line();
    restoreStdin(path);
    EXPECT_EQ(line, nullptr);
}

// 读取整数：合法输入
TEST(InputApiTest, ReadIntValid) {
    const std::string path = tempInputPath();
    setStdinText("42\n", path);
    int ok = 0;
    const long long value = __cn_read_int(&ok);
    restoreStdin(path);
    EXPECT_EQ(ok, 1);
    EXPECT_EQ(value, 42LL);
}

// 读取整数：负数与超大值
TEST(InputApiTest, ReadIntSignedLarge) {
    const std::string path = tempInputPath();
    setStdinText("-123\n9007199254740993\n", path);  // 2^53+1（超 2^53 仍精确 64 位）
    int ok = 0;
    EXPECT_EQ(__cn_read_int(&ok), -123LL);
    EXPECT_EQ(ok, 1);
    EXPECT_EQ(__cn_read_int(&ok), 9007199254740993LL);
    EXPECT_EQ(ok, 1);
    restoreStdin(path);
}

// 读取整数：非法输入（非数字，ok=0 值=0）
TEST(InputApiTest, ReadIntInvalid) {
    const std::string path = tempInputPath();
    setStdinText("abc\n", path);
    int ok = 1;
    const long long value = __cn_read_int(&ok);
    restoreStdin(path);
    EXPECT_EQ(ok, 0);
    EXPECT_EQ(value, 0LL);
}

// 读取整数：EOF（ok=0 值=0）
TEST(InputApiTest, ReadIntEof) {
    const std::string path = tempInputPath();
    setStdinText("", path);
    int ok = 1;
    const long long value = __cn_read_int(&ok);
    restoreStdin(path);
    EXPECT_EQ(ok, 0);
    EXPECT_EQ(value, 0LL);
}

// 读取浮点：合法输入（含小数与科学计数）
TEST(InputApiTest, ReadFloatValid) {
    const std::string path = tempInputPath();
    setStdinText("3.14\n1e3\n", path);
    int ok = 0;
    const double v1 = __cn_read_float(&ok);
    EXPECT_EQ(ok, 1);
    EXPECT_DOUBLE_EQ(v1, 3.14);
    const double v2 = __cn_read_float(&ok);
    EXPECT_EQ(ok, 1);
    EXPECT_DOUBLE_EQ(v2, 1000.0);
    restoreStdin(path);
}

// 读取浮点：非法输入与 EOF
TEST(InputApiTest, ReadFloatInvalidEof) {
    const std::string path = tempInputPath();
    setStdinText("xyz\n", path);
    int ok = 1;
    const double v1 = __cn_read_float(&ok);
    EXPECT_EQ(ok, 0);
    EXPECT_DOUBLE_EQ(v1, 0.0);
    const double v2 = __cn_read_float(&ok);  // EOF
    EXPECT_EQ(ok, 0);
    EXPECT_DOUBLE_EQ(v2, 0.0);
    restoreStdin(path);
}

// 打印到错误：重定向 stderr 捕获（_dup/_dup2 文件描述符重定向）
// 说明：MSVC 下 stderr 是 __acrt_iob_func(2) 宏（右值），freopen_s 无法重定向；
//   用 _dup2 将 stderr 的文件描述符指向临时文件（POSIX 标准机制，MSVC 兼容）。
TEST(InputApiTest, PrintErr) {
    const std::string path = tempInputPath();
    FILE* newErr = nullptr;
    ASSERT_TRUE(TEST_FOPEN(path.c_str(), "wb", newErr));
    // 保存原始 stderr 文件描述符，重定向 stderr 到临时文件
    const int savedStderr = dup(fileno(stderr));
    ASSERT_GE(savedStderr, 0);
    ASSERT_EQ(dup2(fileno(newErr), fileno(stderr)), 0);
    std::fclose(newErr);

    __cn_print_err("错误消息测试\n");
    std::fflush(stderr);

    // 恢复 stderr
    ASSERT_EQ(dup2(savedStderr, fileno(stderr)), 0);
    close(savedStderr);

    // 读取临时文件内容
    FILE* in = nullptr;
    ASSERT_TRUE(TEST_FOPEN(path.c_str(), "rb", in));
    std::string captured;
    char buf[512];
    std::size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), in)) > 0) {
        captured.append(buf, n);
    }
    std::fclose(in);
    std::remove(path.c_str());
    EXPECT_EQ(captured, "错误消息测试\n");
}
