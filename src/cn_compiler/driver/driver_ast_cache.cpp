// 1142 模块缓存读环境开关：MSVC /WX 下 getenv 触发 C4996（std::getenv 同报）——
// 文件级豁免必须先于任何 CRT 头（仅本文件·MSVC 惯例·POSIX 平台无此告警）。
#define _CRT_SECURE_NO_WARNINGS
// AST 模块缓存实现（任务 1142·模块级预编译缓存第一台阶）
// 文件格式（小端）：
//   [8]  魔数 "CNASTC01"
//   [8]  内容哈希通道 a        [8] 内容哈希通道 b
//   [8]  环境哈希通道 a        [8] 环境哈希通道 b（模块名+宏集合）
//   [8]  payload FNV-1a 校验和（防位翻转/截断损坏）
//   [4]  payload 长度
//   [N]  payload（ast_serial::serializeProgram 产物，内含格式版本）
// 命中判定：魔数 → 内容/环境哈希比对（内容更新=陈旧失效）→ 长度一致 →
//   校验和 → 反序列化成功。任一不符=安全回退完整解析；损坏/版本不符显形。
// 本文件 getenv/fopen 为缓存统计与读写专用（MSVC C4996 安全警告压制·
//   项目既有惯例同 semantic_call.cpp 等——缓存面不处理用户输入，无安全面）
#define _CRT_SECURE_NO_WARNINGS
#include "cn_compiler/driver/driver_ast_cache.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#include <process.h>  // _getpid
#else
#include <sys/stat.h>
#include <unistd.h>  // getpid
#endif

#include "cn_compiler/model/ast_printer.hpp"
#include "cn_compiler/model/ast_serialize.hpp"
#include "cn_compiler/module/module.hpp"

namespace cn_compiler {
namespace driver {
namespace {

// ==================== 统计计数（进程粒度） ====================

struct AstCacheStats {
    long long hit = 0;           // 命中（反序列化成功复用）
    long long missNew = 0;       // 无缓存文件（首次构建/已清理）
    long long missStale = 0;     // 有缓存但键不符（内容/模块名/宏变化后重建）
    long long corrupt = 0;       // 键相符但损坏/版本不符（显形回退）
    long long storeFail = 0;     // 写缓存失败（磁盘/权限；编译不受影响）
    long long serializeFail = 0; // 序列化内部缺口（编程错误，显形）
    long long written = 0;       // 实际写入条目数
};

AstCacheStats g_stats;

bool statsEnabled() {
    const char* env = std::getenv("CN_AST_CACHE_STATS");
    return env != nullptr && env[0] != '\0';
}

// ==================== 环境开关 ====================

// 缓存总开关（CN_NO_AST_CACHE=1 关闭；对拍验证用逃生门）
bool cacheDisabled() {
    const char* env = std::getenv("CN_NO_AST_CACHE");
    return env != nullptr && env[0] != '\0' && std::strcmp(env, "0") != 0;
}

// 验证模式（CN_AST_CACHE_VERIFY=1）：命中后同时重新解析并比对 AST 打印
bool verifyEnabled() {
    const char* env = std::getenv("CN_AST_CACHE_VERIFY");
    return env != nullptr && env[0] != '\0';
}

// ==================== 缓存目录探测 ====================

// 缓存根目录（进程内探测一次）：
//   1. CN_AST_CACHE_DIR 显式覆盖（CI 指向临时目录）
//   2. <项目根>/target/cache/ast——项目根 = options.stdlibDir 的父目录
//      （cn_main 的 detectStdlibDir：可执行文件上溯含 stdlib/ 的目录）
//   3. 无法推导（stdlibDir 为空）：返回空串=缓存禁用（直连解析，非静默错用）
const std::string& cacheDirRef(const DriverOptions& options) {
    static bool probed = false;
    static std::string dir;
    if (probed) return dir;
    probed = true;
    const char* env = std::getenv("CN_AST_CACHE_DIR");
    if (env != nullptr && env[0] != '\0') {
        dir = env;
        return dir;
    }
    if (options.stdlibDir.empty()) return dir;
    const std::size_t cut = options.stdlibDir.find_last_of("/\\");
    if (cut == std::string::npos || cut == 0) return dir;
    dir = options.stdlibDir.substr(0, cut) + "/target/cache/ast";
    return dir;
}

bool dirExists(const std::string& path) {
#ifdef _WIN32
    struct _stat st;
    return _stat(path.c_str(), &st) == 0 && (st.st_mode & _S_IFDIR) != 0;
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFDIR) != 0;
#endif
}

// 递归创建目录（mkdir -p）：与 cn_main ensureDirExists 同模式——层级不同
// （driver 不可依赖 main 层）无法跨层复用，重复实现已标记（事不过三第 2 处）。
bool ensureDirRecursive(const std::string& path) {
    // 静默豁免闭集穷举: 空路径守卫（无目录可建·返回 false 由调用方计 storeFail）
    if (path.empty()) return false;
    if (dirExists(path)) return true;
    std::string cur;
    for (std::size_t i = 0; i < path.size(); ++i) {
        cur += path[i];
        const bool isSep = (path[i] == '/' || path[i] == '\\');
        if (!isSep) continue;
        // 静默豁免闭集穷举: 根 "/" 段跳过（逐级 mkdir 的段守卫·非业务数值区间）
        if (cur.size() <= 1) continue;  // 根 "/"
        if (dirExists(cur)) continue;
#ifdef _WIN32
        _mkdir(cur.c_str());
#else
        ::mkdir(cur.c_str(), 0755);
#endif
    }
    if (!dirExists(path)) {
#ifdef _WIN32
        _mkdir(path.c_str());
#else
        ::mkdir(path.c_str(), 0755);
#endif
    }
    return dirExists(path);
}

// ==================== 哈希（双通道 FNV-1a 64 → 128 位键） ====================

std::uint64_t fnv1a64(const char* data, std::size_t n, std::uint64_t basis) {
    std::uint64_t h = basis;
    for (std::size_t i = 0; i < n; ++i) {
        h ^= static_cast<std::uint8_t>(data[i]);
        h *= 1099511628211ULL;  // FNV 质数
    }
    return h;
}

struct Key128 {
    std::uint64_t a = 0;
    std::uint64_t b = 0;
};

// 128 位内容键：两条独立 offset basis 通道 + 长度掺入（防尾部零字节截断歧义）。
//   非对抗场景碰撞概率 ≤2^-128，配合 payload 校验和与反序列化校验三层防护。
Key128 hash128(const std::string& data) {
    constexpr std::uint64_t kBasisA = 14695981039346656037ULL;  // FNV-1a 标准
    constexpr std::uint64_t kBasisB = 14695981039346656037ULL ^ 0x9E3779B97F4A7C15ULL;
    Key128 k;
    k.a = fnv1a64(data.data(), data.size(), kBasisA) ^
          (static_cast<std::uint64_t>(data.size()) * 0xBF58476D1CE4E5B9ULL);
    k.b = fnv1a64(data.data(), data.size(), kBasisB) ^
          (static_cast<std::uint64_t>(data.size()) * 0x94D049BB133111EBULL);
    return k;
}

// 环境键：模块名 + 排序后的宏集合（-D 宏影响 #如果定义 条件编译结果）
Key128 hashEnv(const std::string& moduleName,
               const std::unordered_set<std::string>& macros) {
    std::vector<std::string> ms(macros.begin(), macros.end());
    std::sort(ms.begin(), ms.end());
    std::string joined = moduleName;
    joined.push_back('\x1f');
    for (const auto& m : ms) {
        joined += m;
        joined.push_back('\x1f');
    }
    return hash128(joined);
}

std::string hex64(std::uint64_t v) {
    char buf[24] = {0};
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(v));
    return std::string(buf);
}

// 缓存文件路径：<dir>/<路径哈希32hex>.cnast（路径哈希=内容键函数对 filePath 复用）
std::string cacheFilePath(const std::string& dir, const std::string& filePath) {
    const Key128 pathKey = hash128(filePath);
    return dir + "/" + hex64(pathKey.a) + hex64(pathKey.b) + ".cnast";
}

// ==================== 文件头读写 ====================

constexpr char kMagic[8] = {'C', 'N', 'A', 'S', 'T', 'C', '0', '1'};
// 头总长 = 魔数 8 + 5×u64（内容哈希 2 + 环境哈希 2 + 载荷校验和 1）+ 载荷长 u32
constexpr std::size_t kHeaderBytes = 8 + 8 * 5 + 4;  // 52
constexpr std::size_t kMaxFileBytes = (1u << 30) + 64;  // payload 上限+头

void appendU32LE(std::vector<char>& buf, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) buf.push_back(static_cast<char>((v >> (8 * i)) & 0xFFu));
}

void appendU64LE(std::vector<char>& buf, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) buf.push_back(static_cast<char>((v >> (8 * i)) & 0xFFu));
}

std::uint32_t readU32LE(const char* p) {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(p[i])) << (8 * i);
    return v;
}

std::uint64_t readU64LE(const char* p) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(p[i])) << (8 * i);
    return v;
}

bool readWholeFile(const std::string& path, std::vector<char>& out) {
    FILE* fp = std::fopen(path.c_str(), "rb");
    // 静默豁免运行时API: fopen 失败=系统调用契约（文件缺失/不可读=缓存未命中正常态）
    if (fp == nullptr) return false;
    std::fseek(fp, 0, SEEK_END);
    const long size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    // 静默豁免闭集穷举: 文件尺寸合法性守卫（非正=空文件/超限=损坏·false 由调用方按未命中处置）
    if (size <= 0 || static_cast<std::size_t>(size) > kMaxFileBytes) {
        std::fclose(fp);
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    const std::size_t got = std::fread(out.data(), 1, out.size(), fp);
    std::fclose(fp);
    return got == out.size();
}

// 损坏显形（键相符但数据坏/版本不符——异常事件，直接 stderr 冒泡）
void warnCorrupt(const std::string& why, const std::string& file) {
    g_stats.corrupt += 1;
    std::fprintf(stderr, "警告: [AST 缓存] 缓存条目无效（%s），回退完整解析: %s\n",
                 why.c_str(), file.c_str());
}

// ==================== 命中读取 ====================

// 尝试从缓存命中：返回 true=ast 已填充；false=未命中（原因已计数）。
//   不命中分支：missNew（文件缺失）/missStale（键不符·陈旧）/corrupt（损坏）。
//   imports 重建由调用方按同一规则统一执行（collectImportDeps）。
bool tryLoadCache(const std::string& cacheFile, const Key128& kContent,
                  const Key128& kEnv, std::unique_ptr<Program>& ast) {
    std::vector<char> buf;
    // 静默豁免业务值: 读缓存失败=未命中信号（missNew 计数·回退完整解析=唯一正确路径）
    if (!readWholeFile(cacheFile, buf)) {
        g_stats.missNew += 1;  // 文件缺失/不可读=首次构建常态（非损坏）
        return false;
    }
    if (buf.size() < kHeaderBytes) {
        warnCorrupt("文件长度不足头部", cacheFile);
        return false;
    }
    if (std::memcmp(buf.data(), kMagic, sizeof(kMagic)) != 0) {
        warnCorrupt("魔数不符", cacheFile);
        return false;
    }
    const char* p = buf.data() + sizeof(kMagic);
    const Key128 storedContent{readU64LE(p), readU64LE(p + 8)};
    const Key128 storedEnv{readU64LE(p + 16), readU64LE(p + 24)};
    const std::uint64_t storedFnv = readU64LE(p + 32);
    const std::uint32_t payloadLen = readU32LE(p + 40);
    if (storedContent.a != kContent.a || storedContent.b != kContent.b ||
        storedEnv.a != kEnv.a || storedEnv.b != kEnv.b) {
        g_stats.missStale += 1;  // 键不符=内容/模块名/宏已变化：正常重建
        return false;
    }
    if (static_cast<std::size_t>(payloadLen) != buf.size() - kHeaderBytes) {
        warnCorrupt("载荷长度不符", cacheFile);
        return false;
    }
    const char* payload = buf.data() + kHeaderBytes;
    if (fnv1a64(payload, payloadLen, 14695981039346656037ULL) != storedFnv) {
        warnCorrupt("载荷校验和不符", cacheFile);
        return false;
    }
    std::string derr;
    ast = ast_serial::deserializeProgram(payload, payloadLen, derr);
    // 静默豁免上游已诊断: 反序列化失败已 warnCorrupt 显形（stderr）+回退完整解析
    if (ast == nullptr) {
        warnCorrupt(derr.empty() ? "反序列化失败" : derr, cacheFile);
        return false;
    }
    g_stats.hit += 1;
    return true;
}

// 验证模式：反序列化 AST 与「重新解析」AST 的打印文本比对——抓序列化漏字段/
//   反序列化漂移（仅在 CN_AST_CACHE_VERIFY=1 时启用）。比对不符=显形+回退解析。
bool verifyAgainstParse(Program& cachedAst, const std::string& source,
                        const std::string& filePath, const std::string& moduleName,
                        const std::unordered_set<std::string>& macros) {
    Diagnostics vdiags;
    std::unique_ptr<Program> fresh;
    std::vector<std::string> vimports;
    if (!module::parseSourceText(source, filePath, moduleName, fresh, vimports, vdiags, macros)) {
        std::fprintf(stderr,
                     "错误: [AST 缓存验证] 命中条目可反序列化但重新解析失败（缓存可疑）: %s\n",
                     filePath.c_str());
        g_stats.corrupt += 1;
        return false;
    }
    std::ostringstream cachedText;
    std::ostringstream freshText;
    AstPrinter pc(cachedText);
    pc.print(&cachedAst);
    AstPrinter pf(freshText);
    pf.print(fresh.get());
    if (cachedText.str() != freshText.str()) {
        std::fprintf(stderr,
                     "错误: [AST 缓存验证] 反序列化 AST 与重新解析不一致（回退解析结果）: %s\n",
                     filePath.c_str());
        g_stats.corrupt += 1;
        return false;
    }
    return true;
}

// ==================== 写入 ====================

// 序列化并原子写入缓存条目（tmp + rename；失败不阻断编译，verbose 提示）。
void storeCache(const std::string& cacheFile, const Key128& kContent, const Key128& kEnv,
                const Program& ast, bool verbose) {
    std::vector<char> payload;
    std::string serr;
    if (!ast_serial::serializeProgram(&ast, payload, serr)) {
        g_stats.serializeFail += 1;
        std::fprintf(stderr, "警告: [AST 缓存] %s（该模块未缓存）\n", serr.c_str());
        return;
    }
    std::vector<char> out;
    out.reserve(kHeaderBytes + payload.size());
    out.insert(out.end(), kMagic, kMagic + sizeof(kMagic));
    appendU64LE(out, kContent.a);
    appendU64LE(out, kContent.b);
    appendU64LE(out, kEnv.a);
    appendU64LE(out, kEnv.b);
    appendU64LE(out, fnv1a64(payload.data(), payload.size(), 14695981039346656037ULL));
    appendU32LE(out, static_cast<std::uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());

#ifdef _WIN32
    const std::string tmp = cacheFile + ".tmp" + std::to_string(_getpid());
#else
    const std::string tmp = cacheFile + ".tmp" + std::to_string(getpid());
#endif
    FILE* fp = std::fopen(tmp.c_str(), "wb");
    if (fp == nullptr) {
        g_stats.storeFail += 1;
        if (verbose) std::cerr << "提示: [AST 缓存] 无法写缓存文件: " << tmp << "\n";
        return;
    }
    const std::size_t wrote = std::fwrite(out.data(), 1, out.size(), fp);
    std::fclose(fp);
    if (wrote != out.size()) {
        std::remove(tmp.c_str());
        g_stats.storeFail += 1;
        if (verbose) std::cerr << "提示: [AST 缓存] 缓存写入不完整: " << tmp << "\n";
        return;
    }
#ifdef _WIN32
    // Windows rename 不覆盖已存在目标：先删旧条目（缓存可重建，窗口期 miss 无害）
    std::remove(cacheFile.c_str());
#endif
    if (std::rename(tmp.c_str(), cacheFile.c_str()) != 0) {
        std::remove(tmp.c_str());
        g_stats.storeFail += 1;
        if (verbose) std::cerr << "提示: [AST 缓存] 缓存改名失败: " << tmp << "\n";
        return;
    }
    g_stats.written += 1;
}

}  // namespace

// ==================== 对外入口 ====================

bool parseModuleWithCache(const std::string& source, const std::string& filePath,
                          const std::string& moduleName,
                          const std::unordered_set<std::string>& macros,
                          const DriverOptions& options,
                          std::unique_ptr<Program>& ast,
                          std::vector<std::string>& imports,
                          Diagnostics& diags) {
    const std::string& dir = cacheDirRef(options);
    if (dir.empty() || cacheDisabled()) {
        // 缓存禁用/不可用：直连解析（与 parseSourceText 同语义）
        return module::parseSourceText(source, filePath, moduleName, ast, imports, diags,
                                       macros);
    }
    const Key128 kContent = hash128(source);
    const Key128 kEnv = hashEnv(moduleName, macros);
    const std::string cacheFile = cacheFilePath(dir, filePath);
    if (tryLoadCache(cacheFile, kContent, kEnv, ast)) {
        // 命中路径：按同一规则重建依赖表（collectImportDeps 与解析路径共用）
        imports.clear();
        module::collectImportDeps(*ast, moduleName, imports);
        if (verifyEnabled() && !verifyAgainstParse(*ast, source, filePath, moduleName, macros)) {
            ast = nullptr;
            imports.clear();
            return module::parseSourceText(source, filePath, moduleName, ast, imports, diags,
                                           macros);
        }
        return true;
    }
    // 未命中/失效/损坏：完整解析（唯一正确路径；解析失败不缓存——诊断每次重建）
    // 静默豁免上游已诊断: 解析失败 diags 已记录·error 由上层原样报出
    if (!module::parseSourceText(source, filePath, moduleName, ast, imports, diags, macros)) {
        return false;
    }
    if (!ensureDirRecursive(dir)) {
        g_stats.storeFail += 1;
        if (options.verbose) {
            std::cerr << "提示: [AST 缓存] 无法创建缓存目录: " << dir << "\n";
        }
        return true;
    }
    storeCache(cacheFile, kContent, kEnv, *ast, options.verbose);
    return true;
}

void reportAstCacheStats(bool verbose) {
    if (!verbose && !statsEnabled()) return;
    std::fprintf(stderr,
                 "[AST 缓存] 命中=%lld 首建=%lld 失效重建=%lld 损坏回退=%lld "
                 "写入=%lld 写失败=%lld 序列化失败=%lld\n",
                 g_stats.hit, g_stats.missNew, g_stats.missStale, g_stats.corrupt,
                 g_stats.written, g_stats.storeFail, g_stats.serializeFail);
}

}  // namespace driver
}  // namespace cn_compiler
