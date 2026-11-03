// CN语言编译器驱动层路径工具族实现（接口说明见 driver_path_util.hpp）
#include "cn_compiler/driver/driver_path_util.hpp"

#include <cstdio>
#include <string>

#ifdef _WIN32
#include <direct.h>  // _getcwd（win）
#else
#include <unistd.h>  // getcwd（POSIX）
#endif

#include "cn_compiler/module/module.hpp"

namespace cn_compiler {
namespace driver {

std::string pathStem(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    std::size_t dot = base.find_last_of('.');
    return (dot == std::string::npos) ? base : base.substr(0, dot);
}

std::string pathDir(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
}

bool tryReadSource(const std::string& path, std::string& out) {
    std::string err;
    return module::readSourceFile(path, out, err);
}

namespace {

// 判定路径是否绝对形态（穷举显式：POSIX '/'、UNC '//'、win 盘符 'X:'）；
//   空串与未命中=相对形态（调用方拼 cwd 前缀——相对/绝对的完备二分，非 miss）
bool isAbsolutePath(const std::string& path) {
    const bool posixAbs = !path.empty() && path[0] == '/';
    // 盘符形态：冒号恒在位置 1（find 等值判定隐含长度守卫——位置 0 的冒号是
    //   相对形态，位置 2 及以后与本判定无关）
    const bool winDrive =
        path.find(':') == 1 &&
        ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'));
    return posixAbs || winDrive;
}

// 获取当前工作目录（成功返回 true；失败返回 false 由调用方显式报错中止）
bool currentWorkingDir(std::string& out) {
#ifdef _WIN32
    char buf[4096];
    // 静默豁免运行时API: _getcwd 失败=环境损坏（cwd 不可得）——返回 false 由
    //   normalizeEntryPath 上抛、runModulePipeline 报错中止（非取默认值继续）
    if (_getcwd(buf, sizeof(buf)) == nullptr) return false;
#else
    char buf[4096];
    // 静默豁免运行时API: getcwd 失败=环境损坏（cwd 不可得）——返回 false 由
    //   normalizeEntryPath 上抛、runModulePipeline 报错中止（非取默认值继续）
    if (getcwd(buf, sizeof(buf)) == nullptr) return false;
#endif
    out = buf;
    return true;
}

}  // namespace

bool normalizeEntryPath(const std::string& path, std::string& out) {
    // 1. 相对 → 绝对（拼 cwd 前缀；失败显式上抛，禁静默回退）
    std::string abs = path;
    if (!isAbsolutePath(path)) {
        std::string cwd;
        // 静默豁免运行时API: cwd 获取失败上抛（false→runModulePipeline 报错
        //   中止·显式终态，非取默认值继续）
        if (!currentWorkingDir(cwd)) return false;
        abs = cwd + "/" + path;
    }
    // 2. 分隔符统一正斜杠（win 反斜杠形态折叠为项目内部单一口径）
    for (char& ch : abs) {
        if (ch == '\\') ch = '/';
    }
    // 3. 前缀落盘（显式两类）：win 盘符 "X:"（冒号位置 1·find 等值判定）或
    //    POSIX 根 "/"（"//" 折叠为单根——项目内部路径单一口径）；其余为路径主体段。
    std::string result;
    std::size_t prefixLen = 0;
    std::size_t bodyStart = 0;
    if (abs.find(':') == 1) {
        result = abs.substr(0, 2);  // 盘符段原样落盘
        prefixLen = 2;
        bodyStart = 2;
    } else if (!abs.empty() && abs[0] == '/') {
        result = "/";
        prefixLen = 1;
        bodyStart = 1;
    }
    // 4. 主体逐段词法折叠（三形态显式枚举·无 else 兜底面）：""（重复分隔）与
    //    "." 当前段丢弃；".." 弹上一段（栈底=越过根/盘符，丢弃）；其余合法段
    //    追加。纯字符串操作不触文件系统。
    std::size_t segStart = bodyStart;
    while (segStart < abs.size() || segStart == bodyStart) {
        std::size_t segEnd = abs.find('/', segStart);
        if (segEnd == std::string::npos) segEnd = abs.size();
        const std::string seg = abs.substr(segStart, segEnd - segStart);
        if (seg.empty() || seg == ".") {
            // 重复分隔/当前段：丢弃
        }
        if (seg == "..") {
            const std::size_t prev = result.find_last_of('/');
            if (prev != std::string::npos) {
                // 截到上一分隔符（不越过前缀——根/盘符恒保留）
                result = result.substr(0, prev > prefixLen ? prev : prefixLen);
            }
            // 栈底（result=前缀）：越根丢弃，保持前缀
        }
        if (!seg.empty() && seg != "." && seg != "..") {
            // 合法路径段追加（显式枚举：非空且非"."/非".."的段）
            if (result.back() != '/') result += '/';
            result += seg;
        }
        if (segEnd >= abs.size()) break;
        segStart = segEnd + 1;
    }
    out = result;
    return true;
}

}  // namespace driver
}  // namespace cn_compiler
