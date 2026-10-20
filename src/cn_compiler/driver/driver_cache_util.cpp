// 驱动层缓存公共原语实现（任务 1156·自 driver_ast_cache.cpp 提炼）
// 行为与 1142 AST 缓存原实现逐字节一致（哈希算法/目录探测/原子写语义零变化），
// 仅把「AST 缓存专用」提升为「驱动层缓存共用」；失败面全部显式返回，禁静默默认值。
#include "cn_compiler/driver/driver_cache_util.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

#ifdef _WIN32
#include <direct.h>
#include <process.h>  // _getpid
#else
#include <sys/stat.h>
#include <unistd.h>  // getpid
#endif

namespace cn_compiler {
namespace driver {
namespace {

constexpr std::uint64_t kFnvPrime = 1099511628211ULL;             // FNV-1a 质数
constexpr std::uint64_t kBasisA = 14695981039346656037ULL;        // FNV-1a 标准
constexpr std::uint64_t kBasisB = 14695981039346656037ULL ^ 0x9E3779B97F4A7C15ULL;
constexpr std::uint64_t kMixA = 0xBF58476D1CE4E5B9ULL;            // 长度掺入乘子
constexpr std::uint64_t kMixB = 0x94D049BB133111EBULL;

// 单字节双通道推进（流式哈希用；与 cacheHash128 的逐字节循环同构）
inline void feedByte(std::uint64_t& a, std::uint64_t& b, char ch) {
    const std::uint8_t v = static_cast<std::uint8_t>(ch);
    a = (a ^ v) * kFnvPrime;
    b = (b ^ v) * kFnvPrime;
}

}  // namespace

std::uint64_t cacheFnv1a64(const char* data, std::size_t n, std::uint64_t basis) {
    std::uint64_t h = basis;
    for (std::size_t i = 0; i < n; ++i) {
        h ^= static_cast<std::uint8_t>(data[i]);
        h *= kFnvPrime;
    }
    return h;
}

CacheKey128 cacheHash128(const std::string& data) {
    CacheKey128 k;
    k.a = cacheFnv1a64(data.data(), data.size(), kBasisA) ^
          (static_cast<std::uint64_t>(data.size()) * kMixA);
    k.b = cacheFnv1a64(data.data(), data.size(), kBasisB) ^
          (static_cast<std::uint64_t>(data.size()) * kMixB);
    return k;
}

bool cacheHashStream(std::FILE* fp, CacheKey128& out) {
    // 静默豁免运行时API: fp 判空守卫（调用方契约·同为显式 false 返回非默认值）
    if (fp == nullptr) return false;
    std::uint64_t a = kBasisA;
    std::uint64_t b = kBasisB;
    std::uint64_t total = 0;
    char buf[65536];
    while (true) {
        const std::size_t got = std::fread(buf, 1, sizeof(buf), fp);
        for (std::size_t i = 0; i < got; ++i) feedByte(a, b, buf[i]);
        total += static_cast<std::uint64_t>(got);
        if (got < sizeof(buf)) break;
    }
    if (std::ferror(fp) != 0) return false;
    out.a = a ^ (total * kMixA);
    out.b = b ^ (total * kMixB);
    return true;
}

bool cacheHashFile(const std::string& path, CacheKey128& out) {
    std::FILE* fp = std::fopen(path.c_str(), "rb");
    // 静默豁免运行时API: fopen 失败=系统调用契约（文件缺失/不可读=调用方按不可得处置）
    if (fp == nullptr) return false;
    const bool ok = cacheHashStream(fp, out);
    std::fclose(fp);
    return ok;
}

std::string cacheHex64(std::uint64_t v) {
    char buf[24] = {0};
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(v));
    return std::string(buf);
}

std::string cacheKeyHex(const CacheKey128& key) {
    return cacheHex64(key.a) + cacheHex64(key.b);
}

bool cacheDirExists(const std::string& path) {
#ifdef _WIN32
    struct _stat st;
    return _stat(path.c_str(), &st) == 0 && (st.st_mode & _S_IFDIR) != 0;
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFDIR) != 0;
#endif
}

bool cacheEnsureDir(const std::string& path) {
    // 静默豁免闭集穷举: 空路径守卫（无目录可建·返回 false 由调用方计 storeFail）
    if (path.empty()) return false;
    if (cacheDirExists(path)) return true;
    std::string cur;
    for (std::size_t i = 0; i < path.size(); ++i) {
        cur += path[i];
        const bool isSep = (path[i] == '/' || path[i] == '\\');
        if (!isSep) continue;
        // 静默豁免闭集穷举: 根 "/" 段跳过（逐级 mkdir 的段守卫·非业务数值区间）
        if (cur.size() <= 1) continue;  // 根 "/"
        if (cacheDirExists(cur)) continue;
#ifdef _WIN32
        _mkdir(cur.c_str());
#else
        ::mkdir(cur.c_str(), 0755);
#endif
    }
    if (!cacheDirExists(path)) {
#ifdef _WIN32
        _mkdir(path.c_str());
#else
        ::mkdir(path.c_str(), 0755);
#endif
    }
    return cacheDirExists(path);
}

bool cacheReadWholeFile(const std::string& path, std::vector<char>& out,
                        std::size_t maxBytes) {
    std::FILE* fp = std::fopen(path.c_str(), "rb");
    // 静默豁免运行时API: fopen 失败=系统调用契约（文件缺失/不可读=缓存未命中正常态）
    if (fp == nullptr) return false;
    std::fseek(fp, 0, SEEK_END);
    const long size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    // 静默豁免闭集穷举: 文件尺寸合法性守卫（非正=空文件/超限=损坏·false 由调用方按未命中处置）
    if (size <= 0 || static_cast<std::size_t>(size) > maxBytes) {
        std::fclose(fp);
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    const std::size_t got = std::fread(out.data(), 1, out.size(), fp);
    std::fclose(fp);
    return got == out.size();
}

void cacheAppendU32LE(std::vector<char>& buf, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) buf.push_back(static_cast<char>((v >> (8 * i)) & 0xFFu));
}

void cacheAppendU64LE(std::vector<char>& buf, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) buf.push_back(static_cast<char>((v >> (8 * i)) & 0xFFu));
}

std::uint32_t cacheReadU32LE(const char* p) {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(p[i])) << (8 * i);
    }
    return v;
}

std::uint64_t cacheReadU64LE(const char* p) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(p[i])) << (8 * i);
    }
    return v;
}

bool cacheWriteFileAtomic(const std::string& path, const std::vector<char>& data,
                          std::string& error) {
#ifdef _WIN32
    const std::string tmp = path + ".tmp" + std::to_string(_getpid());
#else
    const std::string tmp = path + ".tmp" + std::to_string(getpid());
#endif
    std::FILE* fp = std::fopen(tmp.c_str(), "wb");
    // 静默豁免运行时API: fopen 失败=系统调用契约（error 返回具体阶段·调用方显形计数）
    if (fp == nullptr) {
        error = "无法写缓存文件: " + tmp;
        return false;
    }
    const std::size_t wrote = std::fwrite(data.data(), 1, data.size(), fp);
    std::fclose(fp);
    if (wrote != data.size()) {
        std::remove(tmp.c_str());
        error = "缓存写入不完整: " + tmp;
        return false;
    }
#ifdef _WIN32
    // Windows rename 不覆盖已存在目标：先删旧条目（缓存可重建，窗口期 miss 无害）
    std::remove(path.c_str());
#endif
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::remove(tmp.c_str());
        error = "缓存改名失败: " + tmp;
        return false;
    }
    return true;
}

const std::string& cacheLeafDir(const DriverOptions& options, const char* leaf,
                                const char* envOverride) {
    // 进程内按叶目录记忆化（options 在单进程内恒定；环境变量不再变化）
    static std::map<std::string, std::string> probe;
    const auto it = probe.find(leaf);
    if (it != probe.end()) return it->second;
    std::string dir;
    const char* env = std::getenv(envOverride);
    if (env != nullptr && env[0] != '\0') {
        dir = env;
    } else if (!options.stdlibDir.empty()) {
        const std::size_t cut = options.stdlibDir.find_last_of("/\\");
        // 静默豁免闭集穷举: 路径无分隔符/根分隔符=项目根不可推导（缓存禁用·非默认值继续）
        if (cut != std::string::npos && cut != 0) {
            dir = options.stdlibDir.substr(0, cut) + "/target/cache/" + leaf;
        }
    }
    return probe.emplace(leaf, dir).first->second;
}

}  // namespace driver
}  // namespace cn_compiler
