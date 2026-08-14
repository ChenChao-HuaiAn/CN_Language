// CN语言编译器命令行入口（Task 0.1 CLI框架 + Task 1.9 build全链路 + Task 1.10 全命令集成）
// 命名规范：标识符统一使用英文（GCC 7 不支持中文标识符，中文仅用于注释与输出文本）
//
// Task 1.10 里程碑：全部 7 个命令可用
//   build <源.cn>    词法->语法->语义->IR->X64汇编 -> ml64 -> cl运行时 -> link -> exe
//   compile <源.cn>  仅编译生成汇编文件(.asm)
//   run <源.cn>      编译并运行（透传exe输出与退出码）
//   check <源.cn>    仅检查语法和类型
//   ir <源.cn>       输出IR（调试用）
//   ast <源.cn>      输出AST（调试用）
//   token <源.cn>    输出Token流（调试用）
// 公共流水线（词法->语法->语义->IR->汇编）抽取到 cn_compiler/driver 模块
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/parser.hpp"

// 命令行选项结构：保存解析结果
struct CliOptions {
    std::string target = "win-x64";  // 目标平台
    int optLevel = 2;                // 优化级别
    std::string output;              // 输出文件路径
    bool verbose = false;            // 详细输出
    // 阶段C（Task 4.3/4.4）：寄存器分配与调试信息
    bool useRegAlloc = true;         // 是否启用寄存器分配（-O2 起联动；--no-regalloc 显式关闭）
    bool debugInfo = false;          // 是否嵌入源码位置注释（--debug）
    // Task 6.6 条件编译：命令行注入宏（-D 宏名，可多次；#如果定义 判定用）
    std::unordered_set<std::string> macros;
};

// 打印版本信息
void printVersion() {
    std::cout << "CN语言编译器 cn 0.1.0\n";
    std::cout << "目标平台: win-x64 (初期), linux-arm64 (初期)\n";
}

// 打印帮助信息
void printHelp() {
    std::cout << "用法: cn <命令> [选项] <文件>\n";
    std::cout << "\n命令:\n";
    std::cout << "  build <文件.cn>        编译并生成可执行文件\n";
    std::cout << "  compile <文件.cn>      仅编译生成汇编文件(.asm / .s)\n";
    std::cout << "  run <文件.cn>          编译并运行\n";
    std::cout << "  check <文件.cn>        仅检查语法和类型，不生成代码\n";
    std::cout << "  ir <文件.cn>           输出IR（调试用）\n";
    std::cout << "  ast <文件.cn>          输出AST（调试用）\n";
    std::cout << "  token <文件.cn>        输出Token流（调试用）\n";
    std::cout << "\n选项:\n";
    std::cout << "  --target <平台>        目标平台 (win-x64 | linux-arm64)\n";
    std::cout << "  -O0/-O1/-O2/-O3       优化级别（规格书9.1+完善C；-O1=折叠+DCE+代数简化+复写传播，\n";
    std::cout << "                         -O2 增加 CSE+跨块DCE，-O3 增加全局值传播）\n";
    std::cout << "  --opt <级别>           优化级别 (0 | 1 | 2 | 3)（兼容写法，等价 -O<级别>）\n";
    std::cout << "  --no-regalloc          关闭寄存器分配（阶段C：-O2 起默认启用，保持全栈帧）\n";
    std::cout << "  --debug                汇编中嵌入源码位置注释（阶段C 调试信息）\n";
    std::cout << "  --output <路径>        输出文件路径\n";
    std::cout << "  --verbose              详细输出\n";
    std::cout << "  --version, -v          显示版本信息\n";
    std::cout << "  --help, -h             显示帮助信息\n";
}

// 解析选项与文件参数
// 参数: 待解析参数列表、当前下标（引用，前进）、选项结构、输出文件路径
// 返回: 错误消息（空字符串表示成功）
std::string parseOptions(const std::vector<std::string>& args, size_t& index,
                         CliOptions& options, std::string& file) {
    while (index < args.size()) {
        const std::string& current = args[index];
        if (current == "--target") {
            if (index + 1 >= args.size()) return "选项 --target 缺少参数";
            options.target = args[++index];
            if (options.target != "win-x64" && options.target != "linux-arm64")
                return "无效目标平台: " + options.target + "（应为 win-x64 或 linux-arm64）";
        } else if (current == "-O0" || current == "-O1" ||
                   current == "-O2" || current == "-O3") {
            // 优化级别（规格书9.1 + 完善C）：-O0 无优化；
            // -O1 常量折叠+DCE+代数简化+复写传播；
            // -O2 增加 CSE+跨块DCE；-O3 增加全局值传播
            const int level = current[2] - '0';
            options.optLevel = level;
        } else if (current == "--opt") {
            if (index + 1 >= args.size()) return "选项 --opt 缺少参数";
            const std::string value = args[++index];
            if (value != "0" && value != "1" && value != "2" && value != "3")
                return "无效优化级别: " + value + "（应为 0、1、2 或 3）";
            options.optLevel = std::stoi(value);
        } else if (current == "--no-regalloc") {
            // 阶段C（Task 4.3）：显式关闭寄存器分配（保持全栈帧行为）
            options.useRegAlloc = false;
        } else if (current == "--debug") {
            // 阶段C（Task 4.4）：汇编中嵌入源码位置注释
            options.debugInfo = true;
        } else if (current == "--output") {
            if (index + 1 >= args.size()) return "选项 --output 缺少参数";
            options.output = args[++index];
        } else if (current == "-D") {
            // Task 6.6 条件编译：注入宏定义（-D 宏名，可多次指定）
            if (index + 1 >= args.size()) return "选项 -D 缺少宏名参数";
            options.macros.insert(args[++index]);
        } else if (current == "--verbose") {
            options.verbose = true;
        } else if (current.rfind("--", 0) == 0) {
            return "未知选项: " + current;
        } else {
            // 位置参数：源文件
            if (!file.empty()) return "多余的源文件参数: " + current;
            file = current;
        }
        index++;
    }
    return "";
}

// ==================== 源文件读取与路径工具 ====================

// 读取UTF-8源文件（自动去除UTF-8 BOM，兼容记事本保存的源码）
static bool readSourceFile(const std::string& path, std::string& content, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "无法打开源文件: " + path;
        return false;
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    content = buf.str();
    // 去除UTF-8 BOM（EF BB BF）
    if (content.size() >= 3 && content.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        content = content.substr(3);
    }
    return true;
}

// 提取文件主干名（去除目录与扩展名）：tests/e2e/01_hello/hello.cn -> hello
static std::string pathStem(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    return (dot == std::string::npos) ? base : base.substr(0, dot);
}

// 确保输出目录存在（单层，已存在时返回失败但无害）
static void ensureTargetDir() {
#ifdef _WIN32
    CreateDirectoryA("target", nullptr);  // 已存在时返回失败但无害
#else
    std::system("mkdir -p target 2>/dev/null");
#endif
}

// 确保目录存在（支持多层路径，逐级创建；供 --output 指定深层目录时使用）
// 例如 target/deep/a/b/c -> 逐级创建 target、target/deep、...、target/deep/a/b/c
static bool ensureDirExists(const std::string& dir) {
    if (dir.empty()) return true;
#ifdef _WIN32
    // 逐级创建：按 '\\' 或 '/' 分割逐级 CreateDirectoryA（已存在时忽略失败）
    std::string current;
    for (std::size_t i = 0; i <= dir.size(); ++i) {
        if (i == dir.size() || dir[i] == '\\' || dir[i] == '/') {
            if (!current.empty()) {
                CreateDirectoryA(current.c_str(), nullptr);
            }
            if (i < dir.size()) {
                current += dir[i];
            }
        } else {
            current += dir[i];
        }
    }
    return true;
#else
    std::string cmd = "mkdir -p \"" + dir + "\" 2>/dev/null";
    return std::system(cmd.c_str()) == 0;
#endif
}

// 将 CliOptions 转换为 driver 选项
static cn_compiler::driver::DriverOptions toDriverOptions(const CliOptions& options) {
    cn_compiler::driver::DriverOptions dopts;
    dopts.target = options.target;
    dopts.optLevel = options.optLevel;
    dopts.output = options.output;
    dopts.verbose = options.verbose;
    // Task 6.6 条件编译：命令行注入宏集合透传
    dopts.macros = options.macros;
    // 阶段C（Task 4.3）：寄存器分配联动——-O2 及以上默认启用，
    //   --no-regalloc 显式关闭（options.useRegAlloc=false 覆盖）
    dopts.useRegAlloc = (options.optLevel >= 2) && options.useRegAlloc;
    // 阶段C（Task 4.4）：调试信息
    dopts.debugInfo = options.debugInfo;
    return dopts;
}

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
    return std::system(full.c_str());
#else
    (void)vcvarsBat;  // Linux 下无 vcvars 环境，直接执行
    if (verbose) std::cout << "执行: " << cmdLine << "\n";
    return std::system(cmdLine.c_str());
#endif
}

// 目标平台辅助：是否为 win-x64（Windows 工具链）或 linux-arm64（as/g++ 工具链）
static bool isWinX64(const std::string& target) { return target == "win-x64"; }
static bool isLinuxArm64(const std::string& target) { return target == "linux-arm64"; }

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
static std::string asmSuffix(const std::string& target) { return isWinX64(target) ? ".asm" : ".s"; }
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
        "src/runtime/runtime.cpp",
        "src/runtime/string_api.cpp",
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

// 链接：win-x64 -> link /ENTRY:WinMainCRTStartup；linux-arm64 -> g++ -no-pie
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
                    const std::string& exePath, bool verbose, std::string& error) {
    std::string cmdLine;
    if (isWinX64(target)) {
        cmdLine =
            "link /nologo /ENTRY:WinMainCRTStartup /SUBSYSTEM:CONSOLE "
            "/DEFAULTLIB:libcmt.lib /DEFAULTLIB:libucrt.lib /DEFAULTLIB:kernel32.lib "
            "/DEFAULTLIB:shell32.lib "  // Task 6.5 系统库：CommandLineToArgvW（Unicode 命令行解析）
            "/OUT:\"" + exePath + "\" \"" + userObj + "\" \"" +
            runtimeObjDir + "\\io_api.obj\" \"" + runtimeObjDir + "\\runtime.obj\" \"" +
            runtimeObjDir + "\\string_api.obj\" \"" + runtimeObjDir + "\\i128_api.obj\" \"" +
            runtimeObjDir + "\\math_api.obj\" \"" + runtimeObjDir + "\\input_api.obj\" \"" +
            runtimeObjDir + "\\file_api.obj\" \"" + runtimeObjDir + "\\time_api.obj\" \"" +
            runtimeObjDir + "\\system_api.obj\"";
    } else {
        cmdLine =
            linuxCxxTool() + " -no-pie -o \"" + exePath + "\" \"" + userObj + "\" \"" +
            runtimeObjDir + "/io_api.o\" \"" + runtimeObjDir + "/runtime.o\" \"" +
            runtimeObjDir + "/string_api.o\" \"" + runtimeObjDir + "/i128_api.o\" \"" +
            runtimeObjDir + "/math_api.o\" \"" + runtimeObjDir + "/input_api.o\" \"" +
            runtimeObjDir + "/file_api.o\" \"" + runtimeObjDir + "/time_api.o\" \"" +
            runtimeObjDir + "/system_api.o\"";
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
static int buildExe(const CliOptions& options, const std::string& file,
                    std::string& exePath, std::string& error) {
    // 支持平台：win-x64（MASM + MSVC）/ linux-arm64（GAS + as/g++）
    if (!isWinX64(options.target) && !isLinuxArm64(options.target)) {
        error = "该命令不支持目标平台 " + options.target + "（应为 win-x64 或 linux-arm64）";
        return 1;
    }

    // 1. 读取源文件（UTF-8）
    std::string source;
    if (!readSourceFile(file, source, error)) return 1;

    // 2. 编译器流水线：词法 -> 语法 -> 语义 -> IR -> 汇编文本
    //    Task 3.6 模块系统：统一走多文件流水线（单文件无导入时行为与 runPipeline 等价；
    //    含导入时自动加载依赖模块）。source 已读取但多文件流水线按文件路径重新加载
    //    （含依赖模块），保持一致的文件解析语义。
    cn_compiler::driver::PipelineOutput output;
    if (cn_compiler::driver::runModulePipeline(file, toDriverOptions(options), output) != 0) {
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
    if (isWinX64(options.target)) {
        for (char& ch : exeDir) if (ch == '/') ch = '\\';
    }
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
                 options.verbose, error)) return 1;
    return 0;
}

// ==================== 各命令实现 ====================

// build 命令：编译并生成可执行文件
static int runBuild(const CliOptions& options, const std::string& file) {
    std::string exePath;
    std::string error;
    if (buildExe(options, file, exePath, error) != 0) {
        if (!error.empty()) std::cerr << "错误: " << error << "\n";
        return 1;
    }
    std::cout << "构建成功: " << exePath << "\n";
    return 0;
}

// compile 命令：仅编译生成汇编文件（win-x64 .asm / linux-arm64 .s）
static int runCompile(const CliOptions& options, const std::string& file) {
    // 支持平台：win-x64 / linux-arm64
    if (!isWinX64(options.target) && !isLinuxArm64(options.target)) {
        std::cerr << "错误: compile 命令不支持目标平台 " << options.target
                  << "（应为 win-x64 或 linux-arm64）\n";
        return 1;
    }
    std::string source;
    std::string error;
    if (!readSourceFile(file, source, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::driver::PipelineOutput output;
    // Task 3.6：compile 命令走多文件流水线（自动加载导入依赖）
    if (cn_compiler::driver::runModulePipeline(file, toDriverOptions(options), output) != 0) {
        return 1;
    }
    // 输出汇编文件（--output 指定或默认 target/<stem>.<asm|s>）
    std::string stem = pathStem(file);
    std::string asmPath = options.output.empty()
        ? "target/" + stem + asmSuffix(options.target) : options.output;
    // --output 指定深层目录时自动创建
    {
        size_t slash = asmPath.find_last_of("/\\");
        if (slash != std::string::npos) {
            std::string dir = asmPath.substr(0, slash + 1);
            ensureDirExists(dir);
        }
    }
    ensureTargetDir();
    {
        std::ofstream out(asmPath, std::ios::binary);
        if (!out) {
            std::cerr << "错误: 无法写入汇编文件 " << asmPath << "\n";
            return 1;
        }
        out << output.asmText;
    }
    std::cout << "编译成功: " << asmPath << "\n";
    return 0;
}

// run 命令：编译并运行（透传exe输出与退出码）
static int runRun(const CliOptions& options, const std::string& file) {
    std::string exePath;
    std::string error;
    if (buildExe(options, file, exePath, error) != 0) {
        if (!error.empty()) std::cerr << "错误: " << error << "\n";
        return 1;
    }
    if (options.verbose) std::cout << "运行: " << exePath << "\n";
    std::string exe = exePath;
    if (isWinX64(options.target)) {
        // Windows cmd 会把正斜杠当作选项分隔符（'target' is not recognized），统一转反斜杠
        for (char& ch : exe) {
            if (ch == '/') ch = '\\';
        }
    }
    // 路径含空格时加引号，否则直接执行并透传退出码（Linux 直接执行无后缀可执行文件）
    const std::string cmd = (exe.find(' ') != std::string::npos) ? ("\"" + exe + "\"") : exe;
    return std::system(cmd.c_str());
}

// check 命令：仅检查语法和类型，不生成代码
// Task 3.6：走多文件流水线到语义阶段（自动加载导入依赖；不生成代码）
static int runCheckCommand(const CliOptions& options, const std::string& file) {
    std::string source;
    std::string error;
    if (!readSourceFile(file, source, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::driver::PipelineOutput output;
    const int rc = cn_compiler::driver::runModulePipeline(file, toDriverOptions(options), output);
    if (rc == 0) {
        std::cout << "检查通过: " << file << "\n";
    }
    return rc;
}

// ir 命令：输出IR（调试用）
// Task 3.6：走多文件流水线（含导入依赖模块的 IR 一并生成）
static int runIr(const CliOptions& options, const std::string& file) {
    std::string source;
    std::string error;
    if (!readSourceFile(file, source, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::driver::PipelineOutput output;
    if (cn_compiler::driver::runModulePipeline(file, toDriverOptions(options), output) != 0) {
        return 1;
    }
    cn_compiler::driver::printIr(output.module);
    return 0;
}

// ast 命令：输出AST（调试用，仅词法+语法）
static int runAst(const CliOptions& options, const std::string& file) {
    (void)options;  // ast 命令不使用目标平台/优化选项
    std::string source;
    std::string error;
    if (!readSourceFile(file, source, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::Diagnostics diagnostics;
    cn_compiler::Lexer lexer(source, file, diagnostics);
    std::vector<cn_compiler::Token> tokens = lexer.tokenize();
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }
    cn_compiler::Parser parser(diagnostics);
    std::unique_ptr<cn_compiler::Program> program = parser.parse(tokens);
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }
    cn_compiler::driver::printAst(program.get());
    return 0;
}

// token 命令：输出Token流（调试用，仅词法）
static int runToken(const CliOptions& options, const std::string& file) {
    (void)options;  // token 命令不使用目标平台/优化选项
    std::string source;
    std::string error;
    if (!readSourceFile(file, source, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::Diagnostics diagnostics;
    cn_compiler::Lexer lexer(source, file, diagnostics);
    std::vector<cn_compiler::Token> tokens = lexer.tokenize();
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }
    cn_compiler::driver::printTokens(tokens);
    return 0;
}

// 程序入口
int main(int argc, char** argv) {
#ifdef _WIN32
    // 控制台 UTF-8 输出（避免中文在GBK代码页下乱码）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::vector<std::string> args(argv + 1, argv + argc);

    // 无参数：打印帮助
    if (args.empty()) {
        printHelp();
        return 0;
    }

    // 第一个参数为命令或帮助/版本选项
    const std::string command = args[0];
    if (command == "--version" || command == "-v") { printVersion(); return 0; }
    if (command == "--help" || command == "-h") { printHelp(); return 0; }

    // 校验是否为已知命令
    static const std::vector<std::string> knownCommands = {
        "build", "compile", "run", "check", "ir", "ast", "token"
    };
    bool isKnownCommand = false;
    for (const auto& c : knownCommands) {
        if (c == command) { isKnownCommand = true; break; }
    }
    if (!isKnownCommand) {
        std::cerr << "错误: 未知命令 '" << command << "'\n";
        printHelp();
        return 1;
    }

    // 解析选项与文件参数
    CliOptions options;
    std::string file;
    size_t index = 1;
    const std::string error = parseOptions(args, index, options, file);
    if (!error.empty()) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }

    // 编译类命令必须提供源文件
    if (file.empty()) {
        std::cerr << "错误: 命令 '" << command << "' 缺少源文件参数\n";
        printHelp();
        return 1;
    }

    // 详细输出模式：打印解析结果
    if (options.verbose) {
        std::cout << "命令: " << command << "\n";
        std::cout << "文件: " << file << "\n";
        std::cout << "目标平台: " << options.target << "\n";
        std::cout << "优化级别: " << options.optLevel << "\n";
        if (!options.output.empty()) std::cout << "输出: " << options.output << "\n";
    }

    // Task 1.10：全部7个命令分发
    if (command == "build") return runBuild(options, file);
    if (command == "compile") return runCompile(options, file);
    if (command == "run") return runRun(options, file);
    if (command == "check") return runCheckCommand(options, file);
    if (command == "ir") return runIr(options, file);
    if (command == "ast") return runAst(options, file);
    if (command == "token") return runToken(options, file);
    std::cerr << "错误: 命令 '" << command << "' 尚未实现\n";
    return 1;
}
