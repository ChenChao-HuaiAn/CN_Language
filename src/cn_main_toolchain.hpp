// CN语言编译器命令行工具链层接口（349 重构E 自 cn_main.cpp 纯机械搬移·零逻辑变化）
// 职责：MSVC vcvars 探测 + 工具链调用（汇编/运行时编译/链接）+ 目标平台辅助
//   （win-x64 / linux-arm64 / linux-x86_64）+ buildExe 全链路；
//   CLI 解析/路径工具/子命令实现/main 留 cn_main.cpp。
// 双侧共享：下方「cn_main.cpp 提供」一组由 cn_main.cpp 实现（原文件级 static
//   去 static 提为可链接·签名/语义零变化），工具链与 CLI 两侧经本头链接。
#ifndef CN_MAIN_TOOLCHAIN_HPP
#define CN_MAIN_TOOLCHAIN_HPP

#include <string>
#include <unordered_set>
#include <vector>

namespace cn_compiler {
namespace driver {
struct DriverOptions;   // 前置声明（toDriverOptions 按引用传递·免拖 driver.hpp）
}
}

// 命令行选项结构：保存解析结果
struct CliOptions {
    std::string target = "win-x64";  // 目标平台
    int optLevel = 2;                // 优化级别
    std::string output;              // 输出文件路径
    bool verbose = false;            // 详细输出
    // 阶段C（Task 4.3/4.4）：寄存器分配与调试信息
    bool useRegAlloc = true;         // 是否启用寄存器分配（-O2 起联动；--no-regalloc 显式关闭）
    bool debugInfo = false;          // 是否嵌入源码位置注释（--debug）
    bool releaseMode = false;        // 239-a：发布构建（--发布/--release）——内建常量 调试模式=假
    bool verifyIr = false;           // 是否验证 IR 结构不变量（--验证-ir，B-4）
    bool jsonDiagnostics = false;    // F2-35（556-a）：--json 诊断 JSON 机器可读输出（check 命令）
    bool useCfi = false;             // P3/D4：接口间接调用 CFI 校验（--cfi，默认关保性能）
    // Task 6.6 条件编译：命令行注入宏（-D 宏名，可多次；#如果定义 判定用）
    std::unordered_set<std::string> macros;
    // 模块系统 v2.0 第 5 层（规格书09）：货舱.toml 依赖管理
    // --货舱 <路径> 显式指定；为空时按入口文件同目录自动发现 货舱.toml
    std::string cargoToml;
    // 编译器内置 stdlib 目录（--stdlib <路径> 显式覆盖；默认相对可执行文件探测）
    std::string stdlibDir;
    // C-3（FFI）：附加链接库（--链接 <库>，可多次；ASCII 别名 --link-lib）
    std::vector<std::string> extraLibs;
};

// ---- cn_main_toolchain.cpp 提供（buildExe 被子命令 runBuild/runRun 调用·签名不变）----
int buildExe(const CliOptions& options, const std::string& file,
             std::string& exePath, std::string& error);
bool isWinX64(const std::string& target);
bool isGnuToolchain(const std::string& target);
std::string asmSuffix(const std::string& target);

// ---- cn_main.cpp 提供（原文件级 static 去 static·签名/语义零变化）----
bool readSourceFile(const std::string& path, std::string& content, std::string& error);
std::string pathStem(const std::string& path);
int systemExitCode(int status);
bool ensureDirExists(const std::string& dir);
std::string ansiToUtf8(const std::string& ansi);
bool toDriverOptions(const CliOptions& options, const std::string& file,
                     cn_compiler::driver::DriverOptions& dopts, std::string& error);

#endif // CN_MAIN_TOOLCHAIN_HPP
