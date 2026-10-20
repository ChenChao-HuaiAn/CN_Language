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
#include <cassert>
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
#include <shellapi.h>  // CommandLineToArgvW（1026：宽命令行取参·shell32）
#include <crtdbg.h>  // _CALL_REPORTFAULT（#513 abort 弹窗收敛）
#else
#include <sys/wait.h>  // WIFEXITED/WEXITSTATUS（system 返回值解包，249-a）
#include <unistd.h>  // readlink（stdlib 目录探测）
#endif

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/driver/cargo_parser.hpp"
#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/parser.hpp"

// 349 重构E：MSVC vcvars 探测+工具链调用+buildExe 抽取到 cn_main_toolchain.hpp/cpp
//   （纯机械搬移·零逻辑变化）；CliOptions 移入 cn_main_toolchain.hpp；
//   readSourceFile/pathStem/systemExitCode/ensureDirExists/toDriverOptions
//   由文件级 static 提为可链接（签名/语义零变化·声明住 cn_main_toolchain.hpp）。
#include "cn_main_toolchain.hpp"

// 打印版本信息
void printVersion() {
    std::cout << "CN语言编译器 cn 0.1.0\n";
    std::cout << "目标平台: win-x64 (初期), linux-arm64 (初期), linux-x86_64 (plans/016)\n";
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
    std::cout << "  --target <平台>        目标平台 (win-x64 | linux-arm64 | linux-x86_64)\n";
    std::cout << "  -O0/-O1/-O2/-O3       优化级别（规格书9.1+完善C；-O1=折叠+DCE+代数简化+复写传播，\n";
    std::cout << "                         -O2 增加 CSE+跨块DCE，-O3 增加全局值传播）\n";
    std::cout << "  --opt <级别>           优化级别 (0 | 1 | 2 | 3)（兼容写法，等价 -O<级别>）\n";
    std::cout << "  --no-regalloc          关闭寄存器分配（阶段C：-O2 起默认启用，保持全栈帧）\n";
    std::cout << "  --debug                汇编中嵌入源码位置注释（阶段C 调试信息）\n";
    std::cout << "  --发布 | --release     发布构建（内建常量 调试模式=假；规格书 3.8）\n";

    std::cout << "  --output <路径>        输出文件路径\n";
    std::cout << "  --verbose              详细输出\n";
    // 模块系统 v2.0 第 5 层（规格书09）：货舱.toml 依赖管理
    std::cout << "  --货舱 <路径>           指定 货舱.toml 路径（默认按入口文件同目录自动发现）\n";
    std::cout << "  --stdlib <路径>        指定编译器内置 stdlib 目录（默认相对可执行文件探测）\n";
    std::cout << "  --链接 <库>             附加链接库（C-3 FFI；可多次；ASCII 别名 --link-lib）\n";
    std::cout << "  --version, -v          显示版本信息\n";
    std::cout << "  --help, -h             显示帮助信息\n";
}

#ifdef _WIN32
// UTF-8 -> UTF-16 宽路径（Windows 文件 API 通道）。返回 false=UTF-8 非法/转换失败。
static bool utf8ToWidePath(const std::string& path, std::vector<wchar_t>& wide) {
    wide.clear();
    // MB_ERR_INVALID_CHARS：非法 UTF-8 显式失败（不静默替换 U+FFFD）
    const int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                            path.c_str(), -1, nullptr, 0);
    if (wideLen == 0) return false;
    wide.resize(static_cast<std::size_t>(wideLen));
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, wide.data(), wideLen);
    return true;
}

// UTF-16 -> UTF-8（宽命令行取参通道）。返回 false=Win32 转换失败（调用方显式
//   报错退出，禁静默空串——miss 不可静默纪律）。
static bool wideToUtf8(const wchar_t* wide, std::string& utf8) {
    // 调用契约：wide 非空（CommandLineToArgvW 返回数组元素保证）——不可达断言
    assert(wide != nullptr);
    utf8.clear();
    const int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1,
                                        nullptr, 0, nullptr, nullptr);
    if (len == 0) return false;
    std::vector<char> buf(static_cast<std::size_t>(len));
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1,
                            buf.data(), len, nullptr, nullptr) == 0) {
        return false;
    }
    utf8.assign(buf.data(), static_cast<std::size_t>(len - 1));
    return true;
}
#endif

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
            if (options.target != "win-x64" && options.target != "linux-arm64" &&
                options.target != "linux-x86_64")
                return "无效目标平台: " + options.target +
                       "（应为 win-x64、linux-arm64 或 linux-x86_64）";
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
        } else if (current == "--发布" || current == "--release") {
            // 239-a（规格书 3.8）：发布构建旗标——内建编译期常量 调试模式 取 假
            //   （ASCII 别名同 --验证-ir 先例：Windows argv GBK 乱码兜底）
            options.releaseMode = true;
        } else if (current == "--json") {
            // F2-35（556-a）：诊断 JSON 机器可读输出（check 命令·LSP 后端铺路）
            options.jsonDiagnostics = true;
        } else if (current == "--验证-ir" || current == "--verify-ir") {
            // B-4（2026-08，规格书9.3）：优化前后验证 IR 结构不变量
            // （--verify-ir 为 ASCII 别名：Windows argv 为 GBK 编码，
            //   命令行传中文选项在部分 shell 会乱码）
            options.verifyIr = true;
        } else if (current == "--cfi") {
            // P3/D4（2026-08）：接口间接调用 CFI（控制流完整性）校验，默认关保性能
            options.useCfi = true;
        } else if (current == "--output") {
            if (index + 1 >= args.size()) return "选项 --output 缺少参数";
            options.output = args[++index];
        } else if (current == "-D") {
            // Task 6.6 条件编译：注入宏定义（-D 宏名，可多次指定）
            if (index + 1 >= args.size()) return "选项 -D 缺少宏名参数";
            // 1026（1020 采样轮 H5·审查 P1 根治）：args 由宽命令行
            //   （GetCommandLineW+CommandLineToArgvW）转 UTF-8，宏名与源文件
            //   #如果定义 文本同码空间直接比对——不再依赖「字节串是否合法 UTF-8」
            //   判别式（CRT 窄 argv 恒=ACP 字节，该判别式不成立）。宏展开语义不变。
            options.macros.insert(args[++index]);
        } else if (current == "--货舱") {
            // 模块系统 v2.0 第 5 层：显式指定 货舱.toml 路径（覆盖自动发现）
            if (index + 1 >= args.size()) return "选项 --货舱 缺少路径参数";
            options.cargoToml = args[++index];
        } else if (current == "--stdlib") {
            // 显式指定编译器内置 stdlib 目录（覆盖相对可执行文件探测）
            if (index + 1 >= args.size()) return "选项 --stdlib 缺少路径参数";
            options.stdlibDir = args[++index];
        } else if (current == "--链接" || current == "--link-lib") {
            // C-3（FFI）：附加链接库（可多次；--link-lib 为 ASCII 别名）
            if (index + 1 >= args.size()) return "选项 --链接 缺少库名参数";
            options.extraLibs.push_back(args[++index]);
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
// 路径编码（1026 宽命令行通道）：CLI 路径参数统一 UTF-8——Windows 下先
//   UTF-8 -> UTF-16 宽路径（_wfopen·中文/Ext-B 均可达）；UTF-8 转换失败
//   （非 UTF-8 历史调用面）才回退窄字符（GBK/ASCII）。Linux 直接 UTF-8。
bool readSourceFile(const std::string& path, std::string& content, std::string& error) {
    // 拒绝目录等非常规文件路径（Linux 下 ifstream 打开目录会"成功"且读出空内容——
    // 曾致目录 .cn 路径静默放行"检查通过"；249-a CLI 契约面发现并根治）
#ifdef _WIN32
    {
        // UTF-8 -> 宽路径判定（MB_ERR_INVALID_CHARS：非法 UTF-8 才回退 ANSI）
        DWORD attrs = INVALID_FILE_ATTRIBUTES;
        std::vector<wchar_t> widePath;
        const bool wideOk = utf8ToWidePath(path, widePath);
        if (wideOk) {
            attrs = GetFileAttributesW(widePath.data());
        }
        if (wideOk == false) {
            attrs = GetFileAttributesA(path.c_str());
        }
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            error = "源文件路径是目录而非文件: " + path;
            return false;
        }
    }
#else
    {
        struct stat st;
        if (::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            error = "源文件路径是目录而非文件: " + path;
            return false;
        }
    }
#endif
#ifdef _WIN32
    auto readNarrow = [&](const std::string& p, std::string& out) -> bool {
        FILE* fp = nullptr;
        if (fopen_s(&fp, p.c_str(), "rb") != 0 || fp == nullptr) return false;
        std::fseek(fp, 0, SEEK_END);
        const long size = std::ftell(fp);
        std::fseek(fp, 0, SEEK_SET);
        out.clear();
        if (size > 0) {
            out.resize(static_cast<std::size_t>(size));
            out.resize(std::fread(&out[0], 1, static_cast<std::size_t>(size), fp));
        }
        std::fclose(fp);
        return true;
    };
    // UTF-8 宽路径读取（1026 主通道）：converted=路径是否已按 UTF-8 转宽成功
    auto readWideUtf8 = [&](std::string& out, bool& converted) -> bool {
        std::vector<wchar_t> widePath;
        converted = utf8ToWidePath(path, widePath);
        if (converted) {
            FILE* fp = nullptr;
            if (_wfopen_s(&fp, widePath.data(), L"rb") != 0) return false;
            // _wfopen_s 成功契约：fp 必非空（失败返回非 0）
            out.clear();
            std::fseek(fp, 0, SEEK_END);
            const long size = std::ftell(fp);
            std::fseek(fp, 0, SEEK_SET);
            if (size > 0) {
                out.resize(static_cast<std::size_t>(size));
                out.resize(std::fread(&out[0], 1, static_cast<std::size_t>(size), fp));
            }
            std::fclose(fp);
            return true;
        }
        return false;
    };
    bool converted = false;
    bool readOk = readWideUtf8(content, converted);
    if (readOk == false && converted == false) {
        // 仅 UTF-8 转换失败（非 UTF-8 历史调用面）才回退 ANSI 窄路径；
        //   UTF-8 已转换成功但打开失败=确定失败（禁回退窄字符：那是另一个文件名）
        readOk = readNarrow(path, content);
    }
    if (readOk == false) {
        error = "无法打开源文件: " + path;
        return false;
    }
#else
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "无法打开源文件: " + path;
        return false;
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    content = buf.str();
#endif
    // 去除UTF-8 BOM（EF BB BF）
    if (content.size() >= 3 && content.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        content = content.substr(3);
    }
    return true;
}

// 提取文件主干名（去除目录与扩展名）：tests/e2e/01_hello/hello.cn -> hello
std::string pathStem(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    return (dot == std::string::npos) ? base : base.substr(0, dot);
}

// 统一 std::system 返回值语义（249-a/B4 根治）：
//   MSVC CRT：返回值即子进程退出码，直接透传；
//   POSIX：返回 waitstatus（退出码 N 编码为 N<<8，直接当退出码用会恒为 0）——解包为真实退出码，
//   信号终止按 shell 惯例 128+信号号（与 Rust Command::status().code() 语义对齐）。
// 全文件 std::system 消费点必须经本函数归一（check_cli_contract.py 门禁锚定）。
int systemExitCode(int status) {
#ifdef _WIN32
    return status;
#else
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return status;
#endif
}

// 确保目录存在（支持多层路径，逐级创建；供 --output 指定深层目录时使用）
// 例如 target/deep/a/b/c -> 逐级创建 target、target/deep、...、target/deep/a/b/c
bool ensureDirExists(const std::string& dir) {
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
    return systemExitCode(std::system(cmd.c_str())) == 0;
#endif
}

// 提取文件所在目录（含末尾分隔符）；无目录返回空串
static std::string pathDirPart(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
}

// 探测编译器内置 stdlib 目录：
//   1. --stdlib 显式指定 -> 使用该路径
//   2. 默认：相对可执行文件所在目录的 stdlib/（编译器发行布局 target/stdlib 或 exe 同级）
// 返回空串表示未探测到（依赖查找跳过内置 stdlib 兜底）
static std::string detectStdlibDir(const CliOptions& options) {
    if (!options.stdlibDir.empty()) {
        return options.stdlibDir;
    }
    // 可执行文件路径：GetModuleFileNameA（Windows）/ /proc/self/exe（Linux）
    std::string exeDir;
#ifdef _WIN32
    char buf[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    exeDir = pathDirPart(buf);
#else
    char buf[4096] = {0};
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        exeDir = pathDirPart(buf);
    }
#endif
    if (exeDir.empty()) return "";
    // 从可执行文件目录逐级向上回溯（最多 3 层），找含 stdlib/ 子目录的目录：
    //   发行布局 target/Debug/cn.exe -> 项目根/stdlib（上溯 2 层）
    //   本地布局 target/cn.exe       -> 项目根/stdlib（上溯 1 层）
    //   便携布局 <bin>/cn            -> <bin>/stdlib（上溯 0 层）
    struct stat st;
    std::string dir = exeDir;
    for (int level = 0; level <= 3; ++level) {
        const std::string cand = dir + "stdlib";
        if (stat(cand.c_str(), &st) == 0 && (st.st_mode & S_IFDIR)) return cand;
        if (level == 3) break;
        // 上溯一层：去掉末尾目录（保留末尾分隔符）
        const std::size_t sep = dir.find_last_of("/\\", dir.size() - 2);
        if (sep == std::string::npos) break;
        dir = dir.substr(0, sep + 1);
    }
    return "";
}

// 加载 货舱.toml 配置到 driver 选项：
//   --货舱 显式指定 -> 加载指定路径（缺失/解析失败报错）
//   否则 -> 入口文件同目录自动发现 货舱.toml（对标 Cargo 自动发现；缺失不报错）
// 返回 false 表示货舱.toml 存在但加载失败（错误已写入 error）
static bool applyCargoConfig(const CliOptions& options, const std::string& file,
                             cn_compiler::driver::DriverOptions& dopts, std::string& error) {
    // 确定 货舱.toml 路径：显式 --货舱 优先；为空=入口同目录自动发现
    std::string tomlPath = options.cargoToml;
    if (tomlPath.empty()) {
        // 入口目录已是 UTF-8（宽命令行取参通道）——直接拼接 UTF-8 文件名
        //   货舱.toml，得到纯 UTF-8 路径（readTomlFile 宽路径分支可打开）
        tomlPath = pathDirPart(file) + "货舱.toml";
    }
    // 读取并解析
    cn_compiler::driver::CargoConfig config;
    std::string parseErr;
    if (!cn_compiler::driver::loadCargoConfig(tomlPath, config, parseErr)) {
        if (!parseErr.empty()) {
            error = "解析 货舱.toml 失败: " + parseErr;
            return false;
        }
        // 文件不存在：非错误（自动发现允许无配置；显式 --货舱 缺失时提示）
        if (!options.cargoToml.empty()) {
            error = "无法打开 货舱.toml: " + tomlPath;
            return false;
        }
        return true;  // 未发现配置：继续，依赖查找只走入口同目录 + stdlib 兜底
    }
    dopts.hasCargoConfig = true;
    dopts.cargoConfig = config;
    dopts.cargoDir = pathDirPart(tomlPath);
    // 239-a（规格书 4.7 特性声明制）：货舱 [特性] 启用名单并入注入宏集合——
    //   每个启用特性即一个条件编译旗标（#如果定义(名) 命中），
    //   与命令行 -D 同通道；未启用特性不在集合（裁剪为假分支）。
    for (const std::string& feature : config.features) {
        dopts.macros.insert(feature);
    }
    return true;
}

// 将 CliOptions 转换为 driver 选项（含货舱.toml 加载；失败返回 false 并写 error）
bool toDriverOptions(const CliOptions& options, const std::string& file,
                            cn_compiler::driver::DriverOptions& dopts, std::string& error) {
    dopts.target = options.target;
    dopts.optLevel = options.optLevel;
    dopts.output = options.output;
    dopts.verbose = options.verbose;
    // Task 6.6 条件编译：命令行注入宏集合透传
    dopts.macros = options.macros;
    // 239-a：发布构建旗标透传（内建编译期常量 调试模式 取值）
    dopts.releaseMode = options.releaseMode;
    // 阶段C（Task 4.3）：寄存器分配联动——-O2 及以上默认启用，
    //   --no-regalloc 显式关闭（options.useRegAlloc=false 覆盖）
    dopts.useRegAlloc = (options.optLevel >= 2) && options.useRegAlloc;
    // 阶段C（Task 4.4）：调试信息
    dopts.debugInfo = options.debugInfo;
    // B-4（2026-08，规格书9.3）：IR 结构验证
    dopts.verifyIr = options.verifyIr;
    // P3/D4（2026-08）：接口间接调用 CFI 校验
    dopts.useCfi = options.useCfi;
    // F2-35（556-a）：诊断 JSON 机器可读输出透传
    dopts.jsonDiagnostics = options.jsonDiagnostics;
    // 模块系统 v2.0 第 5 层：货舱.toml + stdlib 目录
    dopts.stdlibDir = detectStdlibDir(options);
    if (!applyCargoConfig(options, file, dopts, error)) return false;
    return true;
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

// compile 命令：仅编译生成汇编文件（win-x64 .asm / linux .s）
static int runCompile(const CliOptions& options, const std::string& file) {
    // 支持平台：win-x64 / linux-arm64 / linux-x86_64
    if (!isWinX64(options.target) && !isGnuToolchain(options.target)) {
        std::cerr << "错误: compile 命令不支持目标平台 " << options.target
                  << "（应为 win-x64、linux-arm64 或 linux-x86_64）\n";
        return 1;
    }
    std::string source;
    std::string error;
    if (!readSourceFile(file, source, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::driver::DriverOptions dopts;
    if (!toDriverOptions(options, file, dopts, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::driver::PipelineOutput output;
    // Task 3.6：compile 命令走多文件流水线（自动加载导入依赖）；
    //   入口文件=UTF-8（宽命令行通道·依赖查找/模块名判定统一 UTF-8）
    // 242-a（D13）：compile 同 build 强制入口 主 函数
    dopts.requireEntryMain = true;
    // 1156：compile 只消费汇编产物 → 启用程序级预编译缓存
    dopts.useProgramCache = true;
    if (cn_compiler::driver::runModulePipeline(file, dopts, output) != 0) {
        return 1;
    }
    // 输出汇编文件（--output 指定或默认 target/<目标架构桶>/<stem>.<asm|s>）
    // 522：默认产物根同 build 命令=target/<目标平台>/（archBucketDir·单一规则）
    // 1026：写盘/工具链路径走 ANSI 视图（与 buildExe 同口径）
    std::string fileAnsi;
    std::string outputAnsi;
    if (!utf8ToAnsi(file, fileAnsi)) {
        std::cerr << "错误: 源文件路径包含当前 ANSI 代码页无法表示的字符: " << file << "\n";
        return 1;
    }
    if (!utf8ToAnsi(options.output, outputAnsi)) {
        std::cerr << "错误: 输出路径包含当前 ANSI 代码页无法表示的字符: "
                  << options.output << "\n";
        return 1;
    }
    std::string stem = pathStem(fileAnsi);
    std::string asmPath = outputAnsi.empty()
        ? "target/" + archBucketDir(options.target) + "/" + stem + asmSuffix(options.target)
        : outputAnsi;
    // 默认桶目录与 --output 深层目录统一由本分支逐级创建（522：旧 ensureTargetDir
    //   单层兜底退役——默认路径必含 '/'，本分支覆盖全部创建需求）
    {
        size_t slash = asmPath.find_last_of("/\\");
        if (slash != std::string::npos) {
            std::string dir = asmPath.substr(0, slash + 1);
            ensureDirExists(dir);
        }
    }
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
    // 路径含空格时加引号，否则直接执行（Linux 直接执行无后缀可执行文件）
    // 退出码透传：经 systemExitCode 归一（POSIX waitstatus 解包——249-a/B4）
    const std::string cmd = (exe.find(' ') != std::string::npos) ? ("\"" + exe + "\"") : exe;
    return systemExitCode(std::system(cmd.c_str()));
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
    cn_compiler::driver::DriverOptions dopts;
    if (!toDriverOptions(options, file, dopts, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::driver::PipelineOutput output;
    const int rc = cn_compiler::driver::runModulePipeline(file, dopts, output);
    // F2-35（556-a）：--json 模式诊断已由流水线走 stdout JSON 通道；rc==0 时
    //   输出零诊断空数组（纯 JSON 流·human「检查通过」行抑制）；rc 语义不变
    if (options.jsonDiagnostics) {
        if (rc == 0) {
            std::cout << "[]\n";
        }
        return rc;
    }
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
    cn_compiler::driver::DriverOptions dopts;
    if (!toDriverOptions(options, file, dopts, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }
    cn_compiler::driver::PipelineOutput output;
    if (cn_compiler::driver::runModulePipeline(file, dopts, output) != 0) {
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
    // 无人值守 CI 面收敛（#513）：Debug CRT 下 abort()/断言失败默认弹「Debug
    //   Error」模态对话框（_CrtDbgReport·_CALL_REPORTFAULT），无人值守 runner
    //   被整链卡死（wt3 实弹 10-13：坏构建产物全格崩→数十框堆叠整夜）。收敛为
    //   stderr 报错+非零退出码——clang/rustc 无人值守构建同款做法；对正常编译
    //   与交互式诊断零影响。
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    // 控制台 UTF-8 输出（避免中文在GBK代码页下乱码）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 命令行取参（1026·审查 P1 根治）：Windows CRT 窄 argv 恒=进程 ANSI 代码页
    //   （CP_ACP·简中=GBK）字节——「字节串是否合法 UTF-8」不是可用判别式（GBK
    //   双字节串偶合合法 UTF-8 约 14.7%，如「目录」「平台」；Ext-B 字符 ANSI
    //   无法承载）。故与 v2 系统::参数 同源：GetCommandLineW+CommandLineToArgvW
    //   取宽命令行，逐参数 UTF-16→UTF-8——文本语义（-D 宏名/中文旗标/源路径逻辑/
    //   诊断）全部统一 UTF-8。路径参数在工具链边界由 utf8ToAnsi 生成 ANSI 视图
    //   供 ml64/link（保留其 ANSI 路径约束·E2E 18/19 回归史）。
    std::vector<std::string> args;
#ifdef _WIN32
    (void)argc;
    (void)argv;
    {
        int wideArgc = 0;
        wchar_t** wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
        if (wideArgv == nullptr) {
            std::cerr << "错误: 无法解析命令行（CommandLineToArgvW 失败）\n";
            return 1;
        }
        args.reserve(wideArgc != 0 ? static_cast<std::size_t>(wideArgc - 1) : 0);
        for (int i = 1; i < wideArgc; ++i) {
            std::string arg;
            if (!wideToUtf8(wideArgv[i], arg)) {
                LocalFree(wideArgv);
                std::cerr << "错误: 命令行参数 UTF-16→UTF-8 转换失败\n";
                return 1;
            }
            args.push_back(std::move(arg));
        }
        LocalFree(wideArgv);
    }
    // 路径身份归一（2026-09-09 用户裁决，Rust std::path 分隔符等价同构）：win 下
    //   '\' 与 '/' 均为合法分隔符，argv 反斜杠形态与编译器内部构造的正斜杠路径
    //   文本失配=包上下文恢复失配/已加载去重失效双载歧义。入口单点归一 '\'->'/'，
    //   下游全部字符串路径身份自然一致（unix 下 '\' 是合法文件名字符，不动）。
    for (auto& a : args) {
        for (auto& ch : a) {
            if (ch == '\\') ch = '/';
        }
    }
#else
    args.assign(argv + 1, argv + argc);
#endif

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
        // 模块系统 v2.0 第 5 层：货舱.toml 与 stdlib 目录
        if (!options.cargoToml.empty()) std::cout << "货舱.toml: " << options.cargoToml << "\n";
        if (!options.stdlibDir.empty()) std::cout << "stdlib目录: " << options.stdlibDir << "\n";
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
