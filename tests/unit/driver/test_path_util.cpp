// 驱动层路径工具族单测（入口归一 + cwd 形态假环/真环集成）
// 覆盖：
//   1. normalizeEntryPath 词法折叠全形态（相对转绝对/分隔符统一/./ ../ 折叠/
//      根与盘符保留/重复分隔折叠）——路径归一规则显式性的回归锁
//   2. cwd 形态集成（进程内 chdir）：入口无目录前缀（cwd=源目录·cn build 主.cn
//      常态）+ 目录包自导入/同包成员互引——原缺陷=入口路径形态耦合 bare 命名
//      （外部模块判据 relPart==filePath 在 entryDir 空串时恒真）→ 包成员归属
//      （pkgRoot）丢失 → 自包导入豁免失守 → 拓扑排序假环；真环负例锁死环检测
//      不宽容（检测失败不检测类兜底禁入）。
// 注：临时目录/文件名 ASCII（Windows ofstream 窄字符写中文文件名失败——
//   test_module_driver 同款约束；仅 包.cn 为协议固定中文名，POSIX 直写/
//   Windows _wfopen_s UTF-8 宽路径写入）；chdir 测试尾必恢复进程工作目录。
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <direct.h>   // _mkdir / _rmdir / _chdir
#include <windows.h>  // MultiByteToWideChar（UTF-8 中文文件名写入）
#include <process.h>  // _getpid
#define TEST_GETPID() _getpid()
#define TEST_CHDIR(p) _chdir(p)
#define TEST_MKDIR(p) _mkdir(p)
#define TEST_RMDIR(p) _rmdir(p)
#else
#include <sys/stat.h>  // mkdir
#include <unistd.h>    // chdir / rmdir / getpid
#define TEST_GETPID() getpid()
#define TEST_CHDIR(p) chdir(p)
#define TEST_MKDIR(p) mkdir((p), 0755)
#define TEST_RMDIR(p) rmdir(p)
#endif

#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/driver/driver_path_util.hpp"

using cn_compiler::driver::DriverOptions;
using cn_compiler::driver::PipelineOutput;
using cn_compiler::driver::normalizeEntryPath;
using cn_compiler::driver::runModulePipeline;

namespace {

// 当前工作目录（单测断言用；失败返回空串）
// 分隔符统一正斜杠（win 平台必守）：normalizeEntryPath 把反斜杠折叠为项目
//   内部单一口径（_getcwd 返回反斜杠形态），断言期望值必须同口径构造——
//   否则 win 上 RelativeBecomesAbsolute 期望（裸 _getcwd 串）与实际输出
//   （归一正斜杠）永不相等（云端预检片3 win-x64 单测红实录·POSIX 不可见）。
std::string testCwd() {
    char buf[4096];
    if (TEST_CHDIR(".") != 0) return "";
#ifdef _WIN32
    if (_getcwd(buf, sizeof(buf)) == nullptr) return "";
#else
    if (getcwd(buf, sizeof(buf)) == nullptr) return "";
#endif
    std::string cwd = buf;
    for (char& ch : cwd) {
        if (ch == '\\') ch = '/';
    }
    return cwd;
}

// 本测试专用临时根目录（唯一名·多 worktree 并行 gate_quick 不互踩）
std::string makeTempRoot(const char* tag) {
    std::string dir = testCwd().empty() ? std::string(".") : testCwd();
    // 用系统临时目录（与 test_module_driver 同款面）
#ifdef _WIN32
    char* buf = nullptr;
    if (_dupenv_s(&buf, nullptr, "TEMP") == 0 && buf != nullptr) {
        dir = buf;
        free(buf);
        for (char& ch : dir) {
            if (ch == '\\') ch = '/';  // 与 testCwd 同口径（正斜杠单一形态）
        }
    }
#else
    dir = "/tmp";
#endif
    dir += std::string("/cn_path_util_") + tag + "_" +
           std::to_string(static_cast<long long>(TEST_GETPID()));
    TEST_MKDIR(dir.c_str());
    return dir;
}

// 删除普通文件（清理面）：std::remove 对文件有效；目录须走 TEST_RMDIR——
//   旧版清理误用 _rmdir/rmdir 删文件（POSIX ENOTDIR/win 报错）恒失败，
//   临时文件全量残留（/tmp 与 TEMP 垃圾累积）——本辅助修正分工。
void removeFile(const std::string& path) { std::remove(path.c_str()); }

// 写 UTF-8 内容文件（包.cn 中文文件名走 UTF-8 宽路径；ASCII 名走窄路径直写）
bool writeUtf8File(const std::string& path, const std::string& content) {
#ifdef _WIN32
    const int pathLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::string wideBuf(static_cast<std::size_t>(pathLen) * sizeof(wchar_t), '\0');
    wchar_t* widePath = reinterpret_cast<wchar_t*>(&wideBuf[0]);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, widePath, pathLen);
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, widePath, L"wb") != 0 || fp == nullptr) return false;
    std::fwrite(content.c_str(), 1, content.size(), fp);  // UTF-8 字节直写
    std::fclose(fp);
    return true;
#else
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << content;
    return true;
#endif
}

// chdir 作用域守卫（构造切目标目录·析构恢复原目录——进程级状态必复原）
class ChdirGuard {
public:
    explicit ChdirGuard(const std::string& target) {
        saved_ = testCwd();
        changed_ = (TEST_CHDIR(target.c_str()) == 0);
    }
    ~ChdirGuard() {
        if (!saved_.empty()) TEST_CHDIR(saved_.c_str());
    }
    bool changed() const { return changed_; }

private:
    std::string saved_;
    bool changed_ = false;
};

}  // namespace

// ---- normalizeEntryPath 词法折叠形态断言 ----

// 相对路径 → 绝对（cwd 前缀 + 原形态）
TEST(PathUtilTest, RelativeBecomesAbsolute) {
    const std::string cwd = testCwd();
    ASSERT_FALSE(cwd.empty());
    std::string out;
    ASSERT_TRUE(normalizeEntryPath("main.cn", out));
    EXPECT_EQ(out, cwd + "/main.cn");
}

// "./" 前缀与重复分隔折叠；".." 上溯一段
TEST(PathUtilTest, LexicalFoldSegments) {
    std::string out;
    ASSERT_TRUE(normalizeEntryPath("/a/b/../c/./d.cn", out));
    EXPECT_EQ(out, "/a/c/d.cn");
    ASSERT_TRUE(normalizeEntryPath("/x//y///z.cn", out));
    EXPECT_EQ(out, "/x/y/z.cn");
    ASSERT_TRUE(normalizeEntryPath("/a/../..", out));
    EXPECT_EQ(out, "/");  // 栈底越根丢弃（根保留）
}

// win 盘符形态：反斜杠统一 + 盘符段保留 + 折叠不越盘符
TEST(PathUtilTest, WindowsDrivePrefix) {
    std::string out;
    ASSERT_TRUE(normalizeEntryPath("C:\\x\\y.cn", out));
    EXPECT_EQ(out, "C:/x/y.cn");
    ASSERT_TRUE(normalizeEntryPath("C:/a/../b.cn", out));
    EXPECT_EQ(out, "C:/b.cn");
}

// 绝对路径直通（POSIX）
TEST(PathUtilTest, AbsolutePassThrough) {
    std::string out;
    ASSERT_TRUE(normalizeEntryPath("/opt/cn/主.cn", out));
    EXPECT_EQ(out, "/opt/cn/主.cn");
}

// ---- cwd 形态集成（chdir + runModulePipeline）----

// 目录包自导入 + 同包成员互引·入口无目录前缀（cwd=源目录）：
//   修复前=包成员归属丢失（bare 命名）→ 自包导入豁免失守 → 拓扑假环 rc!=0；
//   修复后=入口归一（cwd 解耦）→ 编译成功 rc=0。
TEST(PathUtilTest, CwdEntryPackageSelfImportNoFalseCycle) {
    const std::string root = makeTempRoot("pkg");
    ASSERT_FALSE(root.empty());
    const std::string pkgDir = root + "/pkgA";
    TEST_MKDIR(pkgDir.c_str());
    ASSERT_TRUE(writeUtf8File(pkgDir + "/包.cn",
                              "模块 membA;\n模块 membB;\n"));
    ASSERT_TRUE(writeUtf8File(pkgDir + "/membA.cn",
                              "导入 pkgA;\n公开:\n函数 membA值() -> 整32 { 返回 7; }\n"));
    ASSERT_TRUE(writeUtf8File(pkgDir + "/membB.cn",
                              "导入 pkgA;\n导入 membA;\n公开:\n"
                              "函数 membB值() -> 整32 { 返回 membA值() * 3; }\n"));
    ASSERT_TRUE(writeUtf8File(root + "/main.cn",
                              "导入 pkgA;\n函数 主() -> 整32 { 返回 membB值(); }\n"));

    ChdirGuard guard(root);
    ASSERT_TRUE(guard.changed()) << "chdir 失败（临时目录不可进入）";
    DriverOptions options;
    PipelineOutput output;
    // 入口=无目录前缀形态（cn build 主.cn 常态）——cwd 耦合缺陷的精确复现形态
    const int rc = runModulePipeline("main.cn", options, output);
    EXPECT_EQ(rc, 0) << "目录包自导入在子目录 cwd 下应编译成功（假环已根治）";

    removeFile(pkgDir + "/包.cn");
    removeFile(pkgDir + "/membA.cn");
    removeFile(pkgDir + "/membB.cn");
    TEST_RMDIR(pkgDir.c_str());
    removeFile(root + "/main.cn");
    TEST_RMDIR(root.c_str());
}

// 同包成员互引真环（cycA→cycB→cycA）：真环照拦（环检测不因入口 cwd 形态放宽）
TEST(PathUtilTest, CwdEntryPackageMemberCycleRejected) {
    const std::string root = makeTempRoot("cyc");
    ASSERT_FALSE(root.empty());
    const std::string pkgDir = root + "/pkgB";
    TEST_MKDIR(pkgDir.c_str());
    ASSERT_TRUE(writeUtf8File(pkgDir + "/包.cn", "模块 cycA;\n模块 cycB;\n"));
    ASSERT_TRUE(writeUtf8File(pkgDir + "/cycA.cn",
                              "导入 cycB;\n公开:\n函数 ca() -> 整32 { 返回 1; }\n"));
    ASSERT_TRUE(writeUtf8File(pkgDir + "/cycB.cn",
                              "导入 cycA;\n公开:\n函数 cb() -> 整32 { 返回 2; }\n"));
    ASSERT_TRUE(writeUtf8File(root + "/main2.cn",
                              "导入 pkgB;\n函数 主() -> 整32 { 返回 0; }\n"));

    ChdirGuard guard(root);
    ASSERT_TRUE(guard.changed()) << "chdir 失败（临时目录不可进入）";
    DriverOptions options;
    PipelineOutput output;
    const int rc = runModulePipeline("main2.cn", options, output);
    EXPECT_NE(rc, 0) << "同包成员互引真环必须拒绝（环检测真环照拦）";

    removeFile(pkgDir + "/包.cn");
    removeFile(pkgDir + "/cycA.cn");
    removeFile(pkgDir + "/cycB.cn");
    TEST_RMDIR(pkgDir.c_str());
    removeFile(root + "/main2.cn");
    TEST_RMDIR(root.c_str());
}
