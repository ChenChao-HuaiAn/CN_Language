// CN语言编译器命令行工具链实现（349 重构E 自 cn_main.cpp 纯机械搬移·零逻辑变化）
// 职责：MSVC vcvars 探测（vswhere）+ 工具链命令执行（Windows 经 vcvars 环境、
//   Linux 直接 as/g++）+ 目标平台辅助（win-x64 / linux-arm64 / linux-x86_64）+
//   汇编/运行时编译/链接 + buildExe 全链路（build 与 run 命令共用）。
//   readSourceFile/pathStem/systemExitCode/ensureDirExists/ansiToUtf8/toDriverOptions
//   仍由 cn_main.cpp 提供（声明见 cn_main_toolchain.hpp）。
// 969（2026-10-17）：运行时清单单源化（runtimeSrcs 编译/链接共用）+ 整批指纹缓存
//   （清单/源/工具任一变化=整批重编，陈旧 .o 假绿根治·945 移池断链实录）。
// 522（2026-10-13）：默认产物根=target/<目标平台>/（archBucketDir）——运行时 obj、
//   中间 asm/obj、可执行按目标架构分桶，根治 arm64 交叉轮与 x64 构建在同 target/ 根
//   互踩（399 残留面·cn-arm64-3 实弹「file in wrong format」连坐红）。存量（旧版
//   target/ 根 *.o/*.obj）新版不再读取=天然免疫；一键清理：rm -f target/*.o target/*.obj
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

// 522：产物架构桶目录名——产物统一落 target/<桶>/（运行时 obj、中间 asm/obj、可执行），
//   桶名 = 目标平台标识本身（win-x64 / linux-arm64 / linux-x86_64）＝单一架构标识来源。
//   根治互踩（399 残留面·10-13 cn-arm64-3 实弹）：旧版运行时 obj 落 target/ 根且缓存
//   判据只有 mtime 不含架构——arm64 交叉轮产出的 aarch64 .o 被后续 x64 构建复用，
//   链接报「file in wrong format」→ 整链连坐红。分桶后双架构互不可见。
//   Python 侧消费者按「桶名 = --target 值」同规则拼路径（零映射表·见各调用点注释）。
//   未知值原样返回=防御兜底（CLI 解析层已拒非法平台，不会到达此处）。
std::string archBucketDir(const std::string& target) {
    if (isWinX64(target)) return "win-x64";
    if (isLinuxArm64(target)) return "linux-arm64";
    if (isLinuxX64(target)) return "linux-x86_64";
    return target;
}

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

// ==================== 运行时源清单（编译与链接的单一权威） ====================
// compileRuntime 按本清单编 .o，linkExe 按本清单拼链接命令——单源双用，
//   结构上杜绝「编译清单与链接清单漂移」类事故（945 实录：mem_pool.cpp 自
//   mem_api.cpp 拆出后，任一清单漏它即断链）。新增运行时源文件=只改本清单一处。
static const char* runtimeSrcs[] = {
    "src/runtime/io_api.cpp",
    "src/runtime/mem_api.cpp",   // 349 重构E：自 io_api.cpp 拆出（内存管理）
    "src/runtime/mem_pool.cpp",  // 945：自 mem_api.cpp 拆出（小对象池·并发安全）
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

// 运行时整批构建指纹（对齐 run_e2e.py::计算运行时构建指纹 68-a 方案A）：
//   清单全列表（含顺序）+ 各源文件 size/mtime + 目标平台 + 编译工具与标志特征。
//   任一变化 → 指纹不符 → compileRuntime 整批原子重编（不留半新半旧混链）；
//   指纹文件缺失/内容不符同判不符（显式 miss=整批重编，非静默默认值——648 纪律）。
static std::string runtimeBuildFingerprint(const std::string& target) {
    std::string fp;
    for (const char* src : runtimeSrcs) {
        struct stat st;
        if (stat(src, &st) != 0) {
            // 清单源文件缺失：以显式标记入指纹（编译阶段必然报错暴露）
            fp += std::string(src) + ":MISSING\n";
            continue;
        }
        fp += std::string(src) + ":" + std::to_string(static_cast<long long>(st.st_size))
              + ":" + std::to_string(static_cast<long long>(st.st_mtime)) + "\n";
    }
    fp += "target:" + target + "\n";
    fp += isWinX64(target)
              ? "cc:cl /nologo /c /std:c++17 /utf-8 /O2\n"
              : "cc:" + linuxCxxTool() + " -c -std=c++17 -fno-exceptions -fno-rtti -O2 "
                "-DCNRT_LINUX_MAIN\n";
    return fp;
}

// 编译运行时源文件 -> 目标文件（整批指纹缓存：指纹相符且各 .o 齐全=整批跳过；
//   任一 input 变化=整批原子重编，根治陈旧 .o 假绿）
//   win-x64：cl /c /std:c++17 /utf-8 /O2；linux-arm64/linux-x86_64：g++ -c -std=c++17
//   -fno-exceptions -fno-rtti -O2；Linux 需 -DCNRT_LINUX_MAIN 使 runtime.cpp 的 main
//   生效（与 CMake cn 目标定义一致）
//   982（2026-10-17·用户令性能第一）：加 -O2——944 性能对拍发现 runtime 全族以
//   -O0 链编入用户产物（句柄代际检查热路径 -O2 下可降至 ~12 指令），此为全
//   runtime 族（字符串/容器/内存/时间等）用户程序性能普涨的单一杠杆。两侧平台
//   同步加（win cl /O2、linux g++ -O2），并同步 runtimeBuildFingerprint 的 cc 串
//   （命令行变化=产物语义变化 → 旧 -O0 .o 缓存键不符自动整批重编·不留混链）。
//   O0=O3 一致性原则核：-O2 不改变行为——runtime 全 15 文件静态排查（严格
//   别名/符号回绕/未初始化/volatile/移位面·回绕面全用无符号实现）+ 双编译器
//   警告面（g++ -O2 -Wall -Wextra 与 cl /O2 /W4 均 0 警告）+ -O2 UBSan/ASan
//   实弹（微基准全段+重点用例·零报告·输出逐字一致）。若 -O2 暴露 UB 则修
//   runtime 源码，不降优化级（现无此面）。
static bool compileRuntime(const std::string& target, const std::string& vcvarsBat,
                           const std::string& objDir, bool verbose, std::string& error) {
    const std::string sep = isWinX64(target) ? "\\" : "/";
    // 缓存键与产物同桶（target/<目标平台>/·522 架构分桶天然隔离）
    const std::string keyPath = objDir + sep + "runtime_build_key.txt";
    const std::string fingerprint = runtimeBuildFingerprint(target);
    bool allPresent = true;
    for (const char* src : runtimeSrcs) {
        struct stat st;
        if (stat((objDir + sep + pathStem(src) + objSuffix(target)).c_str(), &st) != 0) {
            allPresent = false;  // 清单任一件缺失=缓存 miss
        }
    }
    std::string key;
    {
        std::ifstream in(keyPath, std::ios::binary);
        std::ostringstream ss;
        ss << in.rdbuf();
        key = ss.str();
    }
    if (allPresent && key == fingerprint) {
        if (verbose) std::cout << "运行时 .o 缓存命中（清单与源未变），复用 " << objDir << "\n";
        return true;
    }
    for (const char* src : runtimeSrcs) {
        std::string obj = objDir + sep + pathStem(src) + objSuffix(target);
        std::string cmdLine;
        if (isWinX64(target)) {
            cmdLine = "cl /nologo /c /std:c++17 /utf-8 /O2 /I\"src\" "
                      "/Fo\"" + obj + "\" \"" + src + "\"";
        // 静默豁免闭集穷举: win/linux 平台二分（compileRuntime 运行时编译命令分派·isWinX64 闭集且平台集闭合·另一平台命令行构建属正当分派非业务兜底·登记 miss_registry）
        } else {
            cmdLine = linuxCxxTool() + " -c -std=c++17 -fno-exceptions -fno-rtti -O2 "
                      "-DCNRT_LINUX_MAIN -I\"src\" -o \"" + obj + "\" \"" + src + "\"";
        }
        int rc = runToolchainCommand(vcvarsBat, cmdLine, verbose);
        if (rc != 0) {
            error = "编译运行时失败（" + std::string(isWinX64(target) ? "cl" : "g++")
                    + " 退出码 " + std::to_string(rc) + "，源文件 " + src + "）";
            return false;
        }
    }
    // 整批成功后写缓存键（下次命中=全部跳过）
    std::ofstream out(keyPath, std::ios::binary);
    out << fingerprint;
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
            "/OUT:\"" + exePath + "\" \"" + userObj + "\"";
    // 静默豁免闭集穷举: win/linux 平台二分（isWinX64 闭集·另一平台命令行构建属正当分派·登记 miss_registry）
    } else {
        cmdLine = linuxCxxTool() + " -no-pie -o \"" + exePath + "\" \"" + userObj + "\"";
    }
    // 运行时 .o 逐件拼自 runtimeSrcs 单一清单（编译与链接同源——945 教训：原
    //   编译/链接两处硬编码列表漂移即断链；win 后缀 .obj / linux .o 由 objSuffix 统一）
    for (const char* src : runtimeSrcs) {
        cmdLine += " \"" + runtimeObjDir + (isWinX64(target) ? "\\" : "/") +
                   pathStem(src) + objSuffix(target) + "\"";
    }
    if (isWinX64(target)) {
        // C-3（FFI）：附加链接库（--链接 库名，可多次）
        for (const auto& lib : extraLibs) {
            cmdLine += " \"" + lib + "\"";
        }
    // 静默豁免闭集穷举: win/linux 平台二分（isWinX64 闭集·另一平台命令行构建属正当分派·登记 miss_registry）
    } else {
        cmdLine += " -lpthread";
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
    //    522：产物按目标架构分桶——默认产物根 = target/<目标平台>/（archBucketDir），
    //    运行时 obj + 中间 asm/obj + 可执行全落本架构桶内，双架构构建互不可见。
    //    中间文件（.asm/.s + .obj/.o）放在 exe 同目录，避免不同目录同名源文件互相覆盖；
    //    --output 指定深层不存在目录时自动逐级创建（如 target/deep/a/b/c/out）
    const std::string exeSuf = exeSuffix(options.target);
    const std::string bucketRoot = "target/" + archBucketDir(options.target);
    std::string stem = pathStem(file);
    exePath = options.output.empty() ? bucketRoot + "/" + stem + exeSuf : options.output;
    // 提取 exe 所在目录（含末尾分隔符）；无目录（纯文件名）则用本架构桶目录
    std::string exeDir = bucketRoot + "/";
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

    // 7. 编译运行时库 -> .obj/.o（整批指纹缓存：清单/源/工具任一变化=整批重编，
    //    指纹相符且各 .o 齐全=整批跳过·新文件缺失即 miss 必编；产物=target/<桶>/）
    //    运行时桶目录独立于 exeDir：--output 指定别处时仍复用本架构共享运行时缓存；
    //    win 宿主统一反斜杠（与 exeDir 同款·对齐 999 案乙）
    std::string runtimeObjDir = bucketRoot;
#ifdef _WIN32
    if (isWinX64(options.target)) {
        for (char& ch : runtimeObjDir) if (ch == '/') ch = '\\';
    }
#endif
    if (!ensureDirExists(runtimeObjDir)) {
        error = "无法创建运行时产物目录 " + runtimeObjDir;
        return 1;
    }
    if (!compileRuntime(options.target, vcvarsBat, runtimeObjDir, options.verbose, error)) return 1;

    // 8. 链接 -> 可执行文件（win-x64 .exe / linux-arm64 无后缀）
    if (!linkExe(options.target, vcvarsBat, objPath, runtimeObjDir, exePath,
                 options.verbose, options.extraLibs, error)) return 1;
    return 0;
}
