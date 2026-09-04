// 驱动模块多文件编译流水线单测（Task 6.11 补测：模块导入语法边界）
// 覆盖：导入不存在的模块文件 -> 编译失败（driver 加载依赖模块失败路径）
//       导入存在的模块文件 -> 编译成功（driver 正向链路）
// 说明：runModulePipeline 从入口文件递归加载 导入 依赖（同目录 模块名.cn），
//       依赖文件缺失时 readSourceFile 失败 -> "无法打开源文件" -> 返回非0。
//       E2E run_e2e.py 不支持"预期编译失败"用例（编译失败即 FAIL），故本
//       错误路径由单测覆盖（真实临时文件、无 Mock、无 skip）。
// 注：临时文件名用 ASCII（Windows ofstream 窄字符按 ANSI 写文件名，而
//       readSourceFile 用 UTF-8 宽路径读——中文文件名编码不匹配会误失败；
//       中文模块名路径兼容已由 E2E 27_module 覆盖）。
#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <direct.h>   // _mkdir / _rmdir（目录层级测试）
#include <windows.h>  // MultiByteToWideChar（UTF-8 中文文件名写入）
#else
#include <sys/stat.h> // mkdir
#include <unistd.h>   // rmdir
#endif
#include <vector>

#include "cn_compiler/driver/driver.hpp"

using cn_compiler::driver::DriverOptions;
using cn_compiler::driver::PipelineOutput;
using cn_compiler::driver::runModulePipeline;

namespace {

// 获取系统临时目录（Windows 用 _dupenv_s 避免 C4996 警告；非 Windows 用 /tmp）
std::string tempDir() {
#ifdef _WIN32
    char* buf = nullptr;
    if (_dupenv_s(&buf, nullptr, "TEMP") == 0 && buf != nullptr) {
        std::string dir = buf;
        free(buf);
        return dir.empty() ? "." : dir;
    }
    return ".";
#else
    return "/tmp";
#endif
}

// 在系统临时目录创建真实 .cn 文件（返回完整路径；失败返回空串）
std::string writeTempFile(const std::string& name, const std::string& content) {
    const std::string path = tempDir() + "/" + name;
    std::ofstream out(path, std::ios::binary);
    if (!out) return "";
    out << content;
    out.close();
    return path;
}

} // namespace

// 导入不存在的模块文件：入口文件 导入 missing_mod 但同目录无 missing_mod.cn
// -> loadModuleTree 递归加载依赖失败 -> 编译失败（返回非0，符合预期）
TEST(ModuleDriverTest, ImportMissingModuleFileFails) {
    const std::string entryPath =
        writeTempFile("main_missing_dep_entry.cn", "导入 missing_mod;\n函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_FALSE(entryPath.empty()) << "无法创建临时入口文件";

    DriverOptions options;
    PipelineOutput output;
    const int rc = runModulePipeline(entryPath, options, output);
    // 依赖模块 missing_mod.cn 缺失 -> 编译失败（返回非0）
    EXPECT_NE(rc, 0) << "导入不存在的模块文件应编译失败";

    std::remove(entryPath.c_str());
}

// 导入存在的模块文件：入口 + 依赖同目录 -> 编译成功（driver 正向链路）
// 模块名 = 文件名主干：依赖文件 dep_exist_mod.cn -> 导入 dep_exist_mod
TEST(ModuleDriverTest, ImportExistingModuleFileSucceeds) {
    const std::string depPath = writeTempFile("dep_exist_mod.cn",
                                              "公开:\n函数 双倍(整32 n) -> 整32 { 返回 n * 2; }\n");
    const std::string entryPath =
        writeTempFile("main_exist_dep_entry.cn",
                      "导入 dep_exist_mod;\n函数 主() -> 整32 {\n    变量 数值 = 双倍(21);\n    返回 0;\n}\n");
    ASSERT_FALSE(depPath.empty()) << "无法创建临时依赖文件";
    ASSERT_FALSE(entryPath.empty()) << "无法创建临时入口文件";

    DriverOptions options;
    PipelineOutput output;
    const int rc = runModulePipeline(entryPath, options, output);
    EXPECT_EQ(rc, 0) << "导入存在的模块文件应编译成功";

    std::remove(depPath.c_str());
    std::remove(entryPath.c_str());
}

// 目录层级（第 4 层，v2.0 决策6，P1-2）：net/transport.cn 子目录模块树
// net.cn 内 模块 transport -> 加载 net/transport.cn；主 导入 net 使用其公开符号
// 注：目录/子模块文件用 ASCII（net/transport.cn——模块路径 net::transport）；
//   入口文件 主.cn 用 UTF-8 宽字符写入（readSourceFile UTF-8 宽路径读，避免
//   Windows ofstream 窄字符写中文文件名失败——MultiByteToWideChar 兼容路径）
TEST(ModuleDriverTest, ModuleTreeSubdirectory) {
    // 创建子目录 net/
    const std::string rootDir = tempDir() + "/tree_test_net";
    const std::string netDir = rootDir + "/net";
#ifdef _WIN32
    _mkdir(rootDir.c_str());
    _mkdir(netDir.c_str());
#else
    mkdir(rootDir.c_str(), 0755);
    mkdir(netDir.c_str(), 0755);
#endif
    // net.cn：模块 transport 声明（子模块树）+ 公开函数（ASCII 文件名，
    //   ofstream 窄字符写 OK；内容 UTF-8 字节直写）
    const std::string netPath = rootDir + "/net.cn";
    std::ofstream netOut(netPath, std::ios::binary);
    netOut <<
        "公开:\n"
        "模块 transport;\n"
        "函数 entry() -> 整32 { 返回 1; }\n";
    netOut.close();
    // net/transport.cn：公开函数（子模块符号）
    const std::string subPath = netDir + "/transport.cn";
    std::ofstream subOut(subPath, std::ios::binary);
    subOut <<
        "公开:\n"
        "函数 send() -> 整32 { 返回 42; }\n";
    subOut.close();
    // 主.cn：导入 net（模块树），调用 net::transport::send（多段限定）
    const std::string entryPath = rootDir + "/主.cn";
#ifdef _WIN32
    // 中文文件名用 UTF-8 宽路径打开 + UTF-8 字节写入（readSourceFile 按
    //   UTF-8 宽路径读取兼容；文件内容保持 UTF-8 字节，不转 UTF-16）
    {
        const std::string content =
            "导入 net;\n"
            "函数 主() -> 整32 {\n"
            "    变量 数值1 = net::transport::send();\n"
            "    变量 数值2 = net::entry();\n"
            "    返回 0;\n"
            "}\n";
        const int pathLen = MultiByteToWideChar(CP_UTF8, 0, entryPath.c_str(), -1, nullptr, 0);
        std::vector<wchar_t> widePath(static_cast<std::size_t>(pathLen));
        MultiByteToWideChar(CP_UTF8, 0, entryPath.c_str(), -1, widePath.data(), pathLen);
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, widePath.data(), L"wb") == 0 && fp != nullptr) {
            std::fwrite(content.c_str(), 1, content.size(), fp);  // UTF-8 字节直写
            std::fclose(fp);
        }
    }
#else
    std::ofstream(entryPath) <<
        "导入 net;\n"
        "函数 主() -> 整32 {\n"
        "    变量 数值1 = net::transport::send();\n"
        "    变量 数值2 = net::entry();\n"
        "    返回 0;\n"
        "}\n";
#endif

    DriverOptions options;
    PipelineOutput output;
    const int rc = runModulePipeline(entryPath, options, output);
    // 诊断：失败时打印目录内容（文件是否写入，含子目录递归）
    if (rc != 0) {
        std::cerr << "诊断: tree_test_net 目录内容（递归）:\n";
#ifdef _WIN32
        std::string cmd = "dir /s \"" + rootDir + "\"";
        std::system(cmd.c_str());
#endif
    }
    EXPECT_EQ(rc, 0) << "目录层级模块树应编译成功";
    // 失败时保留文件供排查（正常时清理）
    if (rc != 0) return;

    std::remove(entryPath.c_str());
    std::remove(subPath.c_str());
    std::remove(netPath.c_str());
#ifdef _WIN32
    _rmdir(netDir.c_str());
    _rmdir(rootDir.c_str());
#else
    rmdir(netDir.c_str());
    rmdir(rootDir.c_str());
#endif
}
