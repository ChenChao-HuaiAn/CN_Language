// CN语言编译器命令行工具链实现（349 重构E 自 cn_main.cpp 纯机械搬移·零逻辑变化）
// 职责：MSVC vcvars 探测（vswhere）+ 工具链命令执行（Windows 经 vcvars 环境、
//   Linux 直接 as/g++）+ 目标平台辅助（win-x64 / linux-arm64 / linux-x86_64）+
//   汇编/运行时编译/链接 + buildExe 全链路（build 与 run 命令共用）。
//   readSourceFile/pathStem/systemExitCode/ensureDirExists/ansiToUtf8/toDriverOptions
//   仍由 cn_main.cpp 提供（声明见 cn_main_toolchain.hpp）。
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>  // WIFEXITED/WEXITSTATUS（system 返回值解包，249-a）
#include <unistd.h>  // readlink（stdlib 目录探测）
#endif

#include "cn_main_toolchain.hpp"
#include "cn_compiler/driver/driver.hpp"

// ==================== MSVC 工具链（build/run 命令共用） ====================

// file 是否不早于 ref（file 存在且修改时间 >= ref，用于运行时编译缓存判断）
static bool isNewerThan(const std::string& file, const std::string& ref) {
    struct stat stF, stR;
    if (stat(file.c_str(), &stF) != 0) return false;  // file 不存在
    if (stat(ref.c_str(), &stR) != 0) return true;
    return stF.st_mtime >= stR.st_mtime;
}

// 执行命令并捕获stdout（用于 vswhere 探测；stderr 透传）
static std::string runCapture(const std::string& cmdLine) {
    std::string result;
#ifdef _WIN32
    FILE* pipe = _popen(cmdLine.c_str(), "r");
    if (!pipe) return result;
    char buf[512];
    while (std::fgets(buf, sizeof(buf), pipe)) result += buf;
    _pclose(pipe);
#else
    (void)cmdLine;  // Linux 下未使用（vswhere 探测仅 Windows）
#endif
    return result;
}

// 去除首尾空白字符
static std::string trim(const std::string& text) {
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

// 读取环境变量（MSVC安全版封装：_dupenv_s），未设置返回空串
static std::string getEnvVar(const std::string& name) {
#ifdef _WIN32
    char* value = nullptr;
    size_t len = 0;
    if (_dupenv_s(&value, &len, name.c_str()) != 0 || value == nullptr) return "";
    std::string result(value);
    std::free(value);
    return result;
#else
    (void)name;  // Linux 分支：仅作 getenv 参数，防御性避免 -Wunused-parameter
    const char* v = std::getenv(name.c_str());
    return v ? std::string(v) : "";
#endif
}

// 查找 vcvars64.bat 路径：优先检测是否已在VS开发者环境（PATH已含ml64/link/cl），
// 否则用 vswhere 定位 Visual Studio 安装路径并拼接 vcvars64.bat
// 返回空串表示已在开发者环境中；error 非空表示定位失败
static std::string findVcvarsBat(std::string& error) {
    // 已处于 VS 开发者命令提示符：VSCMD_ARG_TGT_ARCH 环境变量存在
    if (!getEnvVar("VSCMD_ARG_TGT_ARCH").empty()) return "";
    // vswhere 探测安装路径（要求已安装VC工具集组件）
    const std::string vswhere =
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
    std::string installPath = trim(runCapture(
        "\"" + vswhere + "\" -latest -products * "
        "-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 "
        "-property installationPath"));
    if (installPath.empty()) {
        error = "未找到 Visual Studio 2022（vswhere 无输出），build 命令需要 MSVC 工具链";
        return "";
    }
    std::string vcvars = installPath + "\\VC\\Auxiliary\\Build\\vcvars64.bat";
    struct stat st;
    if (stat(vcvars.c_str(), &st) != 0) {
        error = "未找到 vcvars64.bat: " + vcvars;
        return "";
    }
    return vcvars;
}

// 执行工具链命令（平台无关：Windows 经 vcvars 环境执行 MSVC 命令；Linux 直接执行 as/g++）
// vcvarsBat 为空表示已在开发者环境，直接执行；否则 call vcvars64.bat 后执行
static int runToolchainCommand(const std::string& vcvarsBat, const std::string& cmdLine,
                               bool verbose) {
#ifdef _WIN32
    if (verbose) std::cout << "执行: " << cmdLine << "\n";
    std::string full;
    if (vcvarsBat.empty()) {
        full = cmdLine;
    } else {
        full = "call \"" + vcvarsBat + "\" >nul 2>&1 && " + cmdLine;
    }
    return systemExitCode(std::system(full.c_str()));
#else
    (void)vcvarsBat;  // Linux 下无 vcvars 环境，直接执行
    if (verbose) std::cout << "执行: " << cmdLine << "\n";
    return systemExitCode(std::system(cmdLine.c_str()));
#endif
}

// 目标平台辅助：是否为 win-x64（Windows 工具链）或 linux-arm64/linux-x86_64（as/g++ 工具链）
// linux-x86_64 与 linux-arm64 共用同一套 as/g++ 命令形态（本机原生工具链）
bool isWinX64(const std::string& target) { return target == "win-x64"; }
static bool isLinuxArm64(const std::string& target) { return target == "linux-arm64"; }
static bool isLinuxX64(const std::string& target) { return target == "linux-x86_64"; }
// 是否为 as/g++ 工具链平台（linux-arm64 + linux-x86_64，汇编/运行时编译/链接命令共用）
bool isGnuToolchain(const std::string& target) {
    return isLinuxArm64(target) || isLinuxX64(target);
}

// Linux 工具链探测：环境变量 CN_AS / CN_CXX 优先，其次便携工具链（~/gcc7），最后 PATH
//   本机为无系统 g++ 的 ARM64 环境，便携工具链位于 /home/user/gcc7/usr/bin/g++
static std::string linuxAsTool() {
    const std::string env = getEnvVar("CN_AS");
    if (!env.empty()) return env;
    struct stat st;
    if (stat("/home/user/gcc7/usr/bin/as", &st) == 0) return "/home/user/gcc7/usr/bin/as";
    return "as";
}
static std::string linuxCxxTool() {
    const std::string env = getEnvVar("CN_CXX");
    if (!env.empty()) return env;
    struct stat st;
    if (stat("/home/user/gcc7/usr/bin/g++", &st) == 0) return "/home/user/gcc7/usr/bin/g++";
    return "g++";
}
// 可执行文件后缀：Windows .exe；Linux 无后缀
static std::string exeSuffix(const std::string& target) { return isWinX64(target) ? ".exe" : ""; }
// 汇编文件后缀：Windows .asm（MASM）；Linux .s（GAS）
std::string asmSuffix(const std::string& target) { return isWinX64(target) ? ".asm" : ".s"; }
// 目标文件后缀：Windows .obj；Linux .o
static std::string objSuffix(const std::string& target) { return isWinX64(target) ? ".obj" : ".o"; }

// 汇编：win-x64 -> ml64 /c /Fo<obj> <asm>；linux-arm64 -> as -o <obj> <asm>
static bool assembleAsm(const std::string& target, const std::string& vcvarsBat,
                        const std::string& asmPath, const std::string& objPath,
                        bool verbose, std::string& error) {
    std::string cmdLine;
    if (isWinX64(target)) {
        cmdLine = "ml64 /nologo /c /Fo\"" + objPath + "\" \"" + asmPath + "\"";
    } else {
        // GAS（GNU as）：直接汇编 .s -> .o（本机 ARM64 用系统 as / 便携工具链）
        cmdLine = linuxAsTool() + " -o \"" + objPath + "\" \"" + asmPath + "\"";
    }
    int rc = runToolchainCommand(vcvarsBat, cmdLine, verbose);
    if (rc != 0) {
        error = "汇编失败（" + std::string(isWinX64(target) ? "ml64" : "as")
                + " 退出码 " + std::to_string(rc) + "）";
        return false;
    }
    return true;
}

// 编译运行时源文件 -> 目标文件（带缓存：obj 新于 cpp 则跳过）
//   win-x64：cl /c /std:c++17 /utf-8；linux-arm64：g++ -c -std=c++17 -fno-exceptions -fno-rtti
//   Linux 需 -DCNRT_LINUX_MAIN 使 runtime.cpp 的 main 生效（与 CMake cn 目标定义一致）
static bool compileRuntime(const std::string& target, const std::string& vcvarsBat,
                           const std::string& objDir, bool verbose, std::string& error) {
    static const char* runtimeSrcs[] = {
        "src/runtime/io_api.cpp",
        "src/runtime/mem_api.cpp",   // 349 重构E：自 io_api.cpp 拆出（内存管理+小对象池）
        "src/runtime/arena.cpp",     // 349 重构E：自 io_api.cpp 拆出（进程竞技场）
        "src/runtime/crash_handler.cpp",  // 349 重构E：自 io_api.cpp 拆出（崩溃处理器）
        "src/runtime/intern_api.cpp",  // 2026-08-25 自举重建 P1：字符串驻留（Symbol ID）
        "src/runtime/runtime.cpp",
        "src/runtime/string_api.cpp",
        "src/runtime/thread_api.cpp",  // 996（027 波 1·线程库 __cn_thread_*/__cn_mutex_*）
        "src/runtime/i128_api.cpp",
        "src/runtime/math_api.cpp",  // Task 6.3 数学库（__cn_sqrt 等）
        "src/runtime/input_api.cpp", // Task 6.2 输入 API（__cn_read_* / __cn_print_err）
        "src/runtime/file_api.cpp",  // Task 6.2 文件 API（__cn_file_*）
        "src/runtime/time_api.cpp",  // Task 6.5 时间 API（__cn_time/__cn_clock_ms/__cn_time_format）
        "src/runtime/system_api.cpp",// Task 6.5 系统 API（__cn_argc/__cn_argv）
    };
    const std::string sep = isWinX64(target) ? "\\" : "/";
    for (const char* src : runtimeSrcs) {
        std::string stem = pathStem(src);
        std::string obj = objDir + sep + stem + objSuffix(target);
        // 缓存：obj 已存在且不早于 cpp 时跳过（避免重复编译）
        if (isNewerThan(obj, src)) continue;
        std::string cmdLine;
        if (isWinX64(target)) {
            cmdLine = "cl /nologo /c /std:c++17 /utf-8 /I\"src\" "
                      "/Fo\"" + obj + "\" \"" + src + "\"";
        } else {
            cmdLine = linuxCxxTool() + " -c -std=c++17 -fno-exceptions -fno-rtti "
                      "-DCNRT_LINUX_MAIN -I\"src\" -o \"" + obj + "\" \"" + src + "\"";
        }
        int rc = runToolchainCommand(vcvarsBat, cmdLine, verbose);
        if (rc != 0) {
            error = "编译运行时失败（" + std::string(isWinX64(target) ? "cl" : "g++")
                    + " 退出码 " + std::to_string(rc) + "，源文件 " + src + "）";
            return false;
        }
    }
    return true;
}

// 链接：win-x64 -> link /ENTRY:WinMainCRTStartup；linux-arm64 -> g++ -no-pie（尾内联 -lpthread：thread_api 996 入链配套·glibc<2.34 缺标志必炸·>=2.34 并入 libc 静默通过=平台掩盖·336 实录）
// win-x64 说明：
//   1. WinMainCRTStartup（而非 WinMain）：CRT 初始化 stdout/堆后调用用户 WinMain，
//      否则 printLine（puts）输出为空（stdout 未初始化）
//   2. 自定义入口时 link 不自动注入 CRT 默认库，需显式 /DEFAULTLIB 指定：
//      libcmt（静态CRT）+ libucrt（静态UCRT，提供 memcpy/memset）+ kernel32（Win32 API）
// linux-arm64 说明：
//   - 后端 GAS 用 adrp+add（PC 相对寻址）生成代码；-no-pie 避免 PIE 下数据符号
//     需经 GOT 间接访问（R_AARCH64_ADR_PREL_PG_HI21 无法直接引用 PIE 数据符号）
//   - 主函数符号 cn_main / 内置函数符号由运行时 .o 提供（extern "C"）
static bool linkExe(const std::string& target, const std::string& vcvarsBat,
                    const std::string& userObj, const std::string& runtimeObjDir,
                    const std::string& exePath, bool verbose,
                    const std::vector<std::string>& extraLibs, std::string& error) {
    std::string cmdLine;
    if (isWinX64(target)) {
        cmdLine =
            "link /nologo /ENTRY:WinMainCRTStartup /SUBSYSTEM:CONSOLE "
            "/STACK:8388608 "  // 2026-08（自举 Task 7.6）：栈 8MB——组件链递归
                              // （解析 469 行组件源码）大帧（regId 全局累计 35KB/函数）
                              // 叠加溢出 0xC00000FD；编译器级栈 8MB 为常见配置
            "/DEFAULTLIB:libcmt.lib /DEFAULTLIB:libucrt.lib /DEFAULTLIB:kernel32.lib "
            "/DEFAULTLIB:shell32.lib "  // Task 6.5 系统库：CommandLineToArgvW（Unicode 命令行解析）
            "/OUT:\"" + exePath + "\" \"" + userObj + "\" \"" +
            runtimeObjDir + "\\io_api.obj\" \"" + runtimeObjDir + "\\mem_api.obj\" \"" +  // 349 重构E：io_api 拆分三件同链
            runtimeObjDir + "\\arena.obj\" \"" + runtimeObjDir + "\\crash_handler.obj\" \"" +
            runtimeObjDir + "\\intern_api.obj\" \"" +
            runtimeObjDir + "\\runtime.obj\" \"" + runtimeObjDir + "\\thread_api.obj\" \"" + runtimeObjDir + "\\string_api.obj\" \"" + runtimeObjDir + "\\i128_api.obj\" \"" +
            runtimeObjDir + "\\math_api.obj\" \"" + runtimeObjDir + "\\input_api.obj\" \"" +
            runtimeObjDir + "\\file_api.obj\" \"" + runtimeObjDir + "\\time_api.obj\" \"" +
            runtimeObjDir + "\\system_api.obj\"";
        // C-3（FFI）：附加链接库（--链接 库名，可多次）
        for (const auto& lib : extraLibs) {
            cmdLine += " \"" + lib + "\"";
        }
    } else {
        cmdLine =
            linuxCxxTool() + " -no-pie -o \"" + exePath + "\" \"" + userObj + "\" \"" +
            runtimeObjDir + "/io_api.o\" \"" + runtimeObjDir + "/mem_api.o\" \"" +  // 349 重构E：io_api 拆分三件同链
            runtimeObjDir + "/arena.o\" \"" + runtimeObjDir + "/crash_handler.o\" \"" +
            runtimeObjDir + "/intern_api.o\" \"" +
            runtimeObjDir + "/runtime.o\" \"" + runtimeObjDir + "/thread_api.o\" \"" + runtimeObjDir + "/string_api.o\" \"" + runtimeObjDir + "/i128_api.o\" \"" +
            runtimeObjDir + "/math_api.o\" \"" + runtimeObjDir + "/input_api.o\" \"" +
            runtimeObjDir + "/file_api.o\" \"" + runtimeObjDir + "/time_api.o\" \"" +
            runtimeObjDir + "/system_api.o\" -lpthread";
        for (const auto& lib : extraLibs) {
            cmdLine += " " + lib;
        }
    }
    int rc = runToolchainCommand(vcvarsBat, cmdLine, verbose);
    if (rc != 0) {
        error = "链接失败（" + std::string(isWinX64(target) ? "link" : "g++")
                + " 退出码 " + std::to_string(rc) + "）";
        return false;
    }
    return true;
}

// 编译并生成可执行文件（build 与 run 命令共用）
// 参数: options 选项、file 源文件、exePath 输出exe路径（引用）、error 错误消息
// 返回: 0 成功；非0 失败（错误消息已写入 error 或已打印诊断）
int buildExe(const CliOptions& options, const std::string& file,
                    std::string& exePath, std::string& error) {
    // 支持平台：win-x64（MASM + MSVC）/ linux-arm64 / linux-x86_64（GAS + as/g++）
    if (!isWinX64(options.target) && !isGnuToolchain(options.target)) {
        error = "该命令不支持目标平台 " + options.target +
                "（应为 win-x64、linux-arm64 或 linux-x86_64）";
        return 1;
    }

    // 1. 读取源文件（UTF-8）
    std::string source;
    if (!readSourceFile(file, source, error)) return 1;

    // 2. 编译器流水线：词法 -> 语法 -> 语义 -> IR -> 汇编文本
    //    Task 3.6 模块系统：统一走多文件流水线（单文件无导入时行为与 runPipeline 等价；
    //    含导入时自动加载依赖模块）。source 已读取但多文件流水线按文件路径重新加载
    //    （含依赖模块），保持一致的文件解析语义。
    //    模块系统 v2.0 第 5 层：toDriverOptions 负责加载 货舱.toml（自动发现/--货舱）。
    cn_compiler::driver::DriverOptions dopts;
    if (!toDriverOptions(options, file, dopts, error)) {
        if (error.empty()) error = "加载 货舱.toml 配置失败";
        return 1;
    }
    cn_compiler::driver::PipelineOutput output;
    // 242-a（D13）：build 强制入口 主 函数（编译期诊断）
    dopts.requireEntryMain = true;
    // 入口文件转 UTF-8 传给 driver（依赖查找/模块名判定统一 UTF-8；
    //   工具链路径保持 GBK file——ml64/link 按 ANSI 解释中文名）
    if (cn_compiler::driver::runModulePipeline(ansiToUtf8(file), dopts, output) != 0) {
        return 1;
    }

    // 3. 确定输出路径并确保输出目录存在
    //    中间文件（.asm/.s + .obj/.o）放在 exe 同目录，避免不同目录同名源文件互相覆盖；
    //    --output 指定深层不存在目录时自动逐级创建（如 target/deep/a/b/c/out）
    const std::string exeSuf = exeSuffix(options.target);
    std::string stem = pathStem(file);
    exePath = options.output.empty() ? "target/" + stem + exeSuf : options.output;
    // 提取 exe 所在目录（含末尾分隔符）；无目录（纯文件名）则用 "target/"
    std::string exeDir = "target/";
    std::string exeName = stem + exeSuf;
    {
        size_t slash = exePath.find_last_of("/\\");
        if (slash != std::string::npos) {
            exeDir = exePath.substr(0, slash + 1);
            exeName = exePath.substr(slash + 1);
            if (exeName.empty()) exeName = stem + exeSuf;
        }
    }
    // 去掉 exe 扩展名作为中间文件主干名（避免 out.exe -> out.asm / out -> out.s）
    std::string midStem = exeName;
    size_t dot = midStem.find_last_of('.');
    if (dot != std::string::npos) midStem = midStem.substr(0, dot);
    if (midStem.empty()) midStem = stem;
    // 统一中间文件路径分隔符（ml64/link 用反斜杠；as/g++ 用正斜杠）
    // 999 案乙：仅真 win 宿主才替换分隔符——linux 交叉生成 win asm 时写盘走
    //   POSIX 文件系统，"target\" 会让 mkdir -p 的双引号命令行引号不闭合，
    //   写盘目录创建直接失败（交叉出 asm 文本这一门禁路径全断）。
#ifdef _WIN32
    if (isWinX64(options.target)) {
        for (char& ch : exeDir) if (ch == '/') ch = '\\';
    }
#endif
    if (!ensureDirExists(exeDir)) {
        error = "无法创建输出目录 " + exeDir;
        return 1;
    }
    std::string asmPath = exeDir + midStem + asmSuffix(options.target);
    std::string objPath = exeDir + midStem + objSuffix(options.target);

    // 4. 写入汇编文件（UTF-8 无 BOM：ml64 对 BOM 报 A2044 无效字符；as 同样不接受 BOM）
    {
        std::ofstream out(asmPath, std::ios::binary);
        if (!out) {
            error = "无法写入汇编文件 " + asmPath;
            return 1;
        }
        out << output.asmText;
    }

    // 5. 定位 MSVC 工具链（仅 win-x64；linux-arm64 用系统 as/g++，无需 vcvars）
    std::string vcvarsBat;
    if (isWinX64(options.target)) {
        vcvarsBat = findVcvarsBat(error);
        if (!error.empty()) return 1;
    }

    // 6. 汇编用户代码 -> .obj/.o
    if (!assembleAsm(options.target, vcvarsBat, asmPath, objPath,
                     options.verbose, error)) return 1;

    // 7. 编译运行时库 -> .obj/.o（缓存：target/io_api.obj + target/runtime.obj）
    if (!compileRuntime(options.target, vcvarsBat, "target", options.verbose, error)) return 1;

    // 8. 链接 -> 可执行文件（win-x64 .exe / linux-arm64 无后缀）
    if (!linkExe(options.target, vcvarsBat, objPath, "target", exePath,
                 options.verbose, options.extraLibs, error)) return 1;
    return 0;
}
