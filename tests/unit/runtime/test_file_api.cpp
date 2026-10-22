// 文件运行时 API 单元测试（Task 6.2，规格书阶段五「文件系统」）
// 覆盖：打开（写/读/追加）/ 写入 / 读取 / 按行读取 / 文件大小 / 关闭 /
//       文件存在 / 中文文件名路径（MultiByteToWideChar + _wfopen_s）
// 测试技术：临时文件（英文与中文名）创建/清理；各用例独立文件避免冲突。
//   临时文件放系统临时目录专属子目录（2026-09-16 根治：旧实现用相对路径
//   落在进程 cwd=项目根，跑单测即残留 test_file_api_*.tmp 于工作区）。
// 注意：测试名英文（GCC 7 不支持中文标识符）；中文仅注释与测试内容
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

// 安全的文件打开（MSVC /W4 /WX 下 fopen 触发 C4996 警告即错误）
#ifdef _WIN32
#define TEST_FOPEN(path, mode, f) (fopen_s(&(f), (path), (mode)) == 0)
#else
#define TEST_FOPEN(path, mode, f) ((f) = std::fopen((path), (mode))) != nullptr
#endif

#include "runtime/runtime.hpp"

namespace {

// 临时文件路径（每用例独立，避免并行冲突；含中英文两种）：系统临时目录
//   专属子目录，任何 cwd / 运行入口都不污染项目目录（跨平台 std::filesystem）
std::string tempDir() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "cn_test_file_api";
    std::error_code ec;
    fs::create_directories(dir, ec);
    EXPECT_FALSE(ec) << "创建临时目录失败: " << ec.message();
    // #1215 审查 P1-2：u8string()（UTF-8）而非 string()（ANSI/GBK）——中文
    //   TEMP/用户名下 string() 会按 GBK 转码，路径已非合法 UTF-8，运行时宽路径
    //   转换失败/换 U+FFFD，cleanup 删错名留残留（娴嬭瘯鏂囦欢_zh0.tmp 实录）。
    return dir.u8string();
}

std::string tempFile(const char* tag) {
    static int counter = 0;
    return tempDir() + "/test_file_api_" + tag + std::to_string(counter++) + ".tmp";
}

#ifdef _WIN32
// 环境变量临时覆盖守卫（析构恢复；ASSERT 提前返回也覆盖）——中文 TEMP 回归用。
//   必须走 Win32 宽环境 API：_putenv_s 只更新 CRT 窄环境，GetTempPathW 读宽环境
//   时会按 GBK 再转换（中文变 '?' → temp_directory_path 抛 not a directory）。
struct EnvGuard {
    std::wstring name;
    std::wstring oldValue;
    bool hadValue = false;
    explicit EnvGuard(const wchar_t* n) : name(n) {
        const DWORD len = GetEnvironmentVariableW(n, nullptr, 0);
        if (len > 0) {
            oldValue.resize(static_cast<std::size_t>(len));
            const DWORD got = GetEnvironmentVariableW(n, &oldValue[0], len);
            if (got > 0) {
                oldValue.resize(static_cast<std::size_t>(got));
                hadValue = true;
            } else {
                oldValue.clear();
            }
        }
    }
    ~EnvGuard() {
        SetEnvironmentVariableW(name.c_str(), hadValue ? oldValue.c_str() : nullptr);
    }
    EnvGuard(const EnvGuard&) = delete;
    EnvGuard& operator=(const EnvGuard&) = delete;
};
#endif

// 删除临时文件（忽略不存在；删除失败必须暴露——句柄未关/路径错等都属异常）
//   #1215 报备修改（仅测试辅助层，断言零改动）：Windows 下 std::remove 按 ANSI
//   （GBK 936）解释路径，而运行时自 #1215 起 UTF-8 宽路径优先——中文文件名用
//   窄删除会删错名/漏删（ChineseFilename 残留→exists 断言失败）。清理走与生产
//   同一 UTF-8→UTF-16 口径。
//   审查 P1-2：宽转换必须 MB_ERR_INVALID_CHARS——原无该标志时非法 UTF-8 被替换
//   为 U+FFFD，_wremove 命中错误路径返回 ENOENT 被当成功（残留乱码名文件）。
//   宽转换失败（路径非 UTF-8）回退窄 std::remove；宽删除非 ENOENT 失败也双尝试。
void cleanup(const std::string& path) {
#ifdef _WIN32
    const int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(),
                                            -1, nullptr, 0);
    if (wideLen > 0) {
        std::wstring widePath(static_cast<std::size_t>(wideLen), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1,
                            &widePath[0], wideLen);
        errno = 0;
        if (_wremove(widePath.c_str()) == 0 || errno == ENOENT) return;
        // 宽删除非 ENOENT 失败：落窄路径双尝试（保留最终窄 errno 口径）
    }
#endif
    errno = 0;
    EXPECT_TRUE(std::remove(path.c_str()) == 0 || errno == ENOENT)
        << "临时文件删除失败（errno=" << errno << "）: " << path;
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
    static int zhCounter = 0;  // 进程内唯一即可（gtest 用例串行执行）
    const std::string path = tempDir() + "/测试文件_zh" + std::to_string(zhCounter++) + ".tmp";
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

// #1215 审查 P1-2：中文 TEMP 回归——tempDir 必须返回 UTF-8（u8string），
//   cleanup 的宽转换必须 MB_ERR_INVALID_CHARS；否则中文 TEMP/用户名下
//   string() 按 GBK 转码→非法 UTF-8→U+FFFD 替换→删错名残留乱码文件。
TEST(FileApiTest, ChineseTempDirCleanup) {
#ifdef _WIN32
    namespace fs = std::filesystem;
    const fs::path base = fs::temp_directory_path() /
                          fs::u8path("中文临时目录_1215");
    std::error_code ec;
    fs::create_directories(base, ec);
    ASSERT_FALSE(ec) << "创建中文临时目录失败: " << ec.message();
    EnvGuard tempGuard(L"TEMP");
    EnvGuard tmpGuard(L"TMP");
    ASSERT_TRUE(SetEnvironmentVariableW(L"TEMP", base.wstring().c_str()));
    ASSERT_TRUE(SetEnvironmentVariableW(L"TMP", base.wstring().c_str()));

    const std::string dir = tempDir();
    EXPECT_NE(dir.find("中文临时目录_1215"), std::string::npos) << dir;
    const std::string path = dir + "/中文清理回归.tmp";
    void* handle = __cn_file_open(path.c_str(), 2);
    ASSERT_NE(handle, nullptr);
    __cn_file_close(handle);
    EXPECT_EQ(__cn_file_exists(path.c_str()), 1);
    cleanup(path);
    EXPECT_EQ(__cn_file_exists(path.c_str()), 0);
    fs::remove_all(base, ec);
#else
    GTEST_SKIP() << "中文 TEMP 回归仅 Windows（ANSI/UTF-16 双编码面）";
#endif
}

// #1215 审查 P1-1：合法 UTF-8 宽路径失败**不得**回退窄路径——构造「仅窄
//   （GBK 解释）存在」的探测：fopen_s 以 UTF-8 字节建乱码名文件；宽路径
//   （正确中文名）不存在。修复前回退窄 fopen_s 会命中乱码文件（返回非空），
//   修复后必须直接 nullptr。仅 GBK 代码页可构造该分歧，其余平台跳过。
TEST(FileApiTest, ValidUtf8DoesNotFallbackNarrow) {
#ifdef _WIN32
    if (GetACP() != 936) GTEST_SKIP() << "仅 GBK 代码页（936）可构造宽/窄分歧";
    const std::string utf8Path = tempDir() + "/测试文件_zh0.tmp";
    FILE* raw = nullptr;
    ASSERT_EQ(fopen_s(&raw, utf8Path.c_str(), "wb"), 0);  // GBK 解释=乱码名文件
    ASSERT_NE(raw, nullptr);
    std::fclose(raw);
    EXPECT_EQ(__cn_file_open(utf8Path.c_str(), 1), nullptr);  // 宽失败不回退窄
    errno = 0;
    std::remove(utf8Path.c_str());  // 窄删除乱码名（与创建同口径）
#else
    GTEST_SKIP() << "宽/窄分歧仅 Windows GBK 代码页面";
#endif
}

// 打开失败：不存在的文件（读模式）返回 nullptr（路径收进临时目录，
//   不依赖「cwd 恰无同名文件」的巧合）
TEST(FileApiTest, OpenMissing) {
    void* handle = __cn_file_open((tempDir() + "/nonexistent_file_xyz.tmp").c_str(), 1);
    EXPECT_EQ(handle, nullptr);
}

// 非法模式：模式 0/4 返回 nullptr
TEST(FileApiTest, OpenInvalidMode) {
    void* handle = __cn_file_open((tempDir() + "/dummy.tmp").c_str(), 0);
    EXPECT_EQ(handle, nullptr);
    handle = __cn_file_open((tempDir() + "/dummy.tmp").c_str(), 4);
    EXPECT_EQ(handle, nullptr);
}

// 创建目录（#1138）：多层递归 + 幂等（已存在=成功）
TEST(FileApiTest, MkdirRecursive) {
    namespace fs = std::filesystem;
    static int mkdirCounter = 0;
    const fs::path dir = fs::path(tempDir()) / ("mkdir_nested_" + std::to_string(mkdirCounter++))
                         / "a" / "b" / "c";
    std::error_code ec;
    fs::remove_all(dir, ec);
    EXPECT_EQ(__cn_file_mkdir(dir.u8string().c_str()), 1);
    EXPECT_TRUE(fs::is_directory(dir));
    EXPECT_EQ(__cn_file_mkdir(dir.u8string().c_str()), 1);  // 幂等：已存在仍成功
    fs::remove_all(dir, ec);
}

// 创建目录失败面（#1138）：路径中段是已存在文件 → 0（显式失败，禁静默）
TEST(FileApiTest, MkdirBlockedByFile) {
    static int blockerCounter = 0;
    const std::string blocker = tempDir() + "/mkdir_blocker_"
                                + std::to_string(blockerCounter++) + ".tmp";
    FILE* f = nullptr;
    ASSERT_TRUE(TEST_FOPEN(blocker.c_str(), "wb", f));
    std::fclose(f);
    EXPECT_EQ(__cn_file_mkdir((blocker + "/child").c_str()), 0);
    cleanup(blocker);
}
