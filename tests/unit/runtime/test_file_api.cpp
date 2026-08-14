// 文件运行时 API 单元测试（Task 6.2，规格书阶段五「文件系统」）
// 覆盖：打开（写/读/追加）/ 写入 / 读取 / 按行读取 / 文件大小 / 关闭 /
//       文件存在 / 中文文件名路径（MultiByteToWideChar + _wfopen_s）
// 测试技术：临时文件（英文与中文名）创建/清理；各用例独立文件避免冲突。
// 注意：测试名英文（GCC 7 不支持中文标识符）；中文仅注释与测试内容
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// 安全的文件打开（MSVC /W4 /WX 下 fopen 触发 C4996 警告即错误）
#ifdef _WIN32
#define TEST_FOPEN(path, mode, f) (fopen_s(&(f), (path), (mode)) == 0)
#else
#define TEST_FOPEN(path, mode, f) ((f) = std::fopen((path), (mode))) != nullptr
#endif

#include "runtime/runtime.hpp"

namespace {

// 临时文件路径（每用例独立，避免并行冲突；含中英文两种）
std::string tempFile(const char* tag) {
    static int counter = 0;
    return std::string("test_file_api_") + tag + std::to_string(counter++) + ".tmp";
}

// 删除临时文件（忽略不存在）
void cleanup(const std::string& path) {
    std::remove(path.c_str());
}

} // namespace

// 打开写 -> 写入 -> 关闭 -> 存在（英文路径）
TEST(FileApiTest, WriteReadBasic) {
    const std::string path = tempFile("basic");
    cleanup(path);

    // 打开写（模式2）
    void* handle = __cn_file_open(path.c_str(), 2);
    ASSERT_NE(handle, nullptr);
    const long long written = __cn_file_write(handle, "hello file\nsecond\n");
    EXPECT_EQ(written, 18LL);  // "hello file\n"(11) + "second\n"(7) = 18
    __cn_file_close(handle);

    // 文件存在
    EXPECT_EQ(__cn_file_exists(path.c_str()), 1);

    // 打开读（模式1）按行读取
    handle = __cn_file_open(path.c_str(), 1);
    ASSERT_NE(handle, nullptr);
    char* line = __cn_file_read_line(handle);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "hello file\n");
    __cn_str_free(line);
    line = __cn_file_read_line(handle);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "second\n");
    __cn_str_free(line);
    // EOF
    line = __cn_file_read_line(handle);
    EXPECT_EQ(line, nullptr);
    __cn_file_close(handle);

    cleanup(path);
    EXPECT_EQ(__cn_file_exists(path.c_str()), 0);
}

// 文件大小：UTF-8 中文内容字节数
TEST(FileApiTest, FileSizeUtf8) {
    const std::string path = tempFile("size");
    cleanup(path);
    void* handle = __cn_file_open(path.c_str(), 2);
    ASSERT_NE(handle, nullptr);
    // "你好世界" = 4 字 × 3 字节 = 12；+ \n = 13
    const long long written = __cn_file_write(handle, "\xE4\xBD\xA0\xE5\xA5\xBD\xE4\xB8\x96\xE7\x95\x8C\n");
    EXPECT_EQ(written, 13LL);
    __cn_file_close(handle);

    handle = __cn_file_open(path.c_str(), 1);
    ASSERT_NE(handle, nullptr);
    EXPECT_EQ(__cn_file_size(handle), 13LL);
    __cn_file_close(handle);
    cleanup(path);
}

// 读取原始字节（fread 语义）
TEST(FileApiTest, ReadRawBytes) {
    const std::string path = tempFile("raw");
    cleanup(path);
    void* handle = __cn_file_open(path.c_str(), 2);
    ASSERT_NE(handle, nullptr);
    __cn_file_write(handle, "abcd");
    __cn_file_close(handle);

    handle = __cn_file_open(path.c_str(), 1);
    ASSERT_NE(handle, nullptr);
    char buf[16] = {0};
    const long long got = __cn_file_read(handle, buf, 4);
    EXPECT_EQ(got, 4LL);
    EXPECT_STREQ(buf, "abcd");
    // 越界读取：返回 0
    char extra[16] = {0};
    EXPECT_EQ(__cn_file_read(handle, extra, 4), 0LL);
    __cn_file_close(handle);
    cleanup(path);
}

// 追加模式（模式3）：不截断，追加到末尾
TEST(FileApiTest, AppendMode) {
    const std::string path = tempFile("append");
    cleanup(path);
    void* handle = __cn_file_open(path.c_str(), 2);
    ASSERT_NE(handle, nullptr);
    __cn_file_write(handle, "first\n");
    __cn_file_close(handle);

    handle = __cn_file_open(path.c_str(), 3);
    ASSERT_NE(handle, nullptr);
    __cn_file_write(handle, "second\n");
    __cn_file_close(handle);

    handle = __cn_file_open(path.c_str(), 1);
    ASSERT_NE(handle, nullptr);
    EXPECT_EQ(__cn_file_size(handle), 13LL);  // 6 + 7
    char* line = __cn_file_read_line(handle);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "first\n");
    __cn_str_free(line);
    line = __cn_file_read_line(handle);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "second\n");
    __cn_str_free(line);
    __cn_file_close(handle);
    cleanup(path);
}

// 中文文件名路径（UTF-8 -> MultiByteToWideChar + _wfopen_s）
TEST(FileApiTest, ChineseFilename) {
    const std::string path = std::string("测试文件_") + tempFile("zh");
    cleanup(path);
    void* handle = __cn_file_open(path.c_str(), 2);
    ASSERT_NE(handle, nullptr);
    const long long written = __cn_file_write(handle, "中文路径测试\n");
    EXPECT_EQ(written, 19LL);  // "中文路径测试"=6字×3=18字节 + \n = 19
    __cn_file_close(handle);

    EXPECT_EQ(__cn_file_exists(path.c_str()), 1);
    handle = __cn_file_open(path.c_str(), 1);
    ASSERT_NE(handle, nullptr);
    char* line = __cn_file_read_line(handle);
    ASSERT_NE(line, nullptr);
    EXPECT_STREQ(line, "中文路径测试\n");
    __cn_str_free(line);
    __cn_file_close(handle);
    cleanup(path);
    EXPECT_EQ(__cn_file_exists(path.c_str()), 0);
}

// 打开失败：不存在的文件（读模式）返回 nullptr
TEST(FileApiTest, OpenMissing) {
    void* handle = __cn_file_open("nonexistent_file_xyz.tmp", 1);
    EXPECT_EQ(handle, nullptr);
}

// 非法模式：模式 0/4 返回 nullptr
TEST(FileApiTest, OpenInvalidMode) {
    void* handle = __cn_file_open("dummy.tmp", 0);
    EXPECT_EQ(handle, nullptr);
    handle = __cn_file_open("dummy.tmp", 4);
    EXPECT_EQ(handle, nullptr);
}
