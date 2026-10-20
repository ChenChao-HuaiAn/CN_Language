// 程序级预编译缓存实现（任务 1156·方案 A：最终汇编产物缓存）
// 文件格式（小端）：
//   [8]  魔数 "CNPGMC01"
//   [8]  缓存键通道 a        [8] 缓存键通道 b
//   [8]  保留（0）           [8] 保留（0）——与 AST 缓存头布局同构，便于共用原语
//   [8]  payload FNV-1a 校验和
//   [4]  payload 长度
//   payload:
//     [4] 汇编长度  [4] human 警告长度  [4] JSON 警告长度  [4] 保留（0）
//     [N] 汇编文本（UTF-8）  [N] warningsHuman  [N] warningsJson
// 命中判定：魔数 → 键比对（完整输入变化=陈旧失效）→ 长度一致 → 校验和 → 载荷结构。
//   任一不符=显形（损坏分支）或普通未命中，一律回退完整编译（唯一正确路径）。
// MSVC /W4 /WX：getenv 等 CRT 传统函数视为 C4996（项目既有文件同款处置）
#define _CRT_SECURE_NO_WARNINGS
#include "cn_compiler/driver/driver_program_cache.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>  // GetModuleFileNameW（编译器自身身份）
#else
#include <unistd.h>  // readlink（/proc/self/exe）
#endif

namespace cn_compiler {
namespace driver {
namespace {

// ==================== 统计计数（进程粒度） ====================

struct ProgramCacheStats {
    long long hit = 0;            // 命中（产物直接复用）
    long long missNew = 0;        // 无缓存条目（首次构建/已清理）
    long long missStale = 0;      // 条目存在但键不符（输入/选项/编译器变化）
    long long corrupt = 0;        // 键相符但损坏/结构不符（显形回退）
    long long storeFail = 0;      // 写缓存失败（磁盘/权限；编译不受影响）
    long long written = 0;        // 实际写入条目数
    long long verifyMismatch = 0; // 验证通道：命中产物与重编译产物不一致
};

ProgramCacheStats g_stats;

bool statsEnabled() {
    const char* env = std::getenv("CN_PROGRAM_CACHE_STATS");
    return env != nullptr && env[0] != '\0';
}

// ==================== 环境开关与配置 ====================

// 缓存总开关（CN_NO_PROGRAM_CACHE=1 关闭；默认开——键含编译器自身哈希+全输入摘要）
bool cacheDisabledByEnv() {
    const char* env = std::getenv("CN_NO_PROGRAM_CACHE");
    return env != nullptr && env[0] != '\0' && std::strcmp(env, "0") != 0;
}

// ==================== 编译器自身身份（可执行文件内容哈希） ====================

struct CompilerIdentity {
    bool valid = false;
    CacheKey128 key;
};

// 读取当前可执行文件并做 128 位流式哈希：编译器源码/构建配置任何变化=键变，
//   机械保证「新编译器绝不命中旧产物」（无需人工维护版本号）。
CompilerIdentity probeCompilerIdentity() {
    CompilerIdentity id;
#ifdef _WIN32
    wchar_t buf[32768];
    const DWORD n = GetModuleFileNameW(nullptr, buf, 32768);
    // 路径长度双边界闭区间（1..32767）：0=GetModuleFileNameW 失败，32768=可能截断
    if (n < 1 || n > 32767) return id;
    std::FILE* fp = nullptr;
    if (_wfopen_s(&fp, buf, L"rb") != 0 || fp == nullptr) return id;
    id.valid = cacheHashStream(fp, id.key);
    std::fclose(fp);
#else
    char buf[4096];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    // 路径长度双边界闭区间（1..4095）：0=readlink 失败，4095=可能截断
    const bool lengthOk = (n >= 1 && n <= static_cast<ssize_t>(sizeof(buf) - 2));
    // 静默豁免运行时API: readlink 失败/截断=系统调用契约（身份不可得→缓存显式禁用）
    if (!lengthOk) return id;
    buf[n] = '\0';
    id.valid = cacheHashFile(std::string(buf, static_cast<std::size_t>(n)), id.key);
#endif
    return id;
}

const CompilerIdentity& compilerIdentity() {
    static const CompilerIdentity id = probeCompilerIdentity();
    return id;
}

// 身份不可得：显式告警一次并禁用缓存（不以其他版本号冒充身份）
void warnIdentityUnavailableOnce() {
    static bool warned = false;
    if (warned) return;
    warned = true;
    std::fprintf(stderr,
                 "提示: [程序缓存] 无法读取编译器自身身份，程序级缓存禁用（直连编译）\n");
}

// ==================== 键序列化 ====================

// 字段以 '\0' 结尾拼入规范串（文件名/模块名不含 NUL，长度歧义为零）
void appendField(std::string& out, const std::string& value) {
    out += value;
    out.push_back('\0');
}

void appendBool(std::string& out, bool value) { appendField(out, value ? "1" : "0"); }

void appendOptions(std::string& out, const DriverOptions& options) {
    appendField(out, options.target);
    appendField(out, std::to_string(options.optLevel));
    appendBool(out, options.useRegAlloc);
    appendBool(out, options.debugInfo);
    appendBool(out, options.releaseMode);
    appendBool(out, options.useCfi);
    appendBool(out, options.requireEntryMain);
    std::vector<std::string> macros(options.macros.begin(), options.macros.end());
    std::sort(macros.begin(), macros.end());
    appendField(out, std::to_string(macros.size()));
    for (const std::string& macro : macros) appendField(out, macro);
}

void appendUnits(std::string& out, const std::vector<ProgramUnitKey>& units) {
    appendField(out, std::to_string(units.size()));
    for (const ProgramUnitKey& unit : units) {
        appendField(out, unit.moduleName);
        appendField(out, unit.filePath);
        appendField(out, unit.sourceDigest);
        appendField(out, unit.moduleDir);
        appendField(out, unit.pkgRoot);
        appendBool(out, unit.isEntryUnit);
        appendBool(out, unit.cargoPkgRoot);
    }
}

// ==================== 文件格式常量 ====================

constexpr char kMagic[8] = {'C', 'N', 'P', 'G', 'M', 'C', '0', '1'};
// 键格式版本：键组成项变化时必须递增（载荷格式变化同时递增魔数尾号更好——
//   魔数不符=损坏分支显形，两者叠加保证旧条目绝不静默沿用）
constexpr const char* kKeyFormatVersion = "CNPG-KEY-1";
constexpr std::size_t kPayloadHeaderBytes = 16;  // 三个长度 + 保留
constexpr std::size_t kMaxFileBytes = (2ULL << 30) + 64;  // 汇编产物可达数十 MB

// 条目文件名 = 入口路径 + 编译选项签名（含宏集合）的 128 位键：同一入口的同一
//   选项组合原地覆盖旧产物版本；完整输入键存文件头，内容变化=同文件键不符→陈旧
//   重建。宏组合并入签名——CI 多宏矩阵构建互不覆盖（避免交替跑时反复重建）。
std::string cacheFilePath(const std::string& dir, const std::string& entryFilePath,
                          const DriverOptions& options) {
    std::string subject;
    subject += entryFilePath;
    subject.push_back('\0');
    subject += options.target;
    subject.push_back('\0');
    subject += std::to_string(options.optLevel);
    subject.push_back('\0');
    subject += options.useRegAlloc ? "1" : "0";
    subject.push_back('\0');
    subject += options.debugInfo ? "1" : "0";
    subject.push_back('\0');
    subject += options.useCfi ? "1" : "0";
    subject.push_back('\0');
    subject += options.releaseMode ? "1" : "0";
    subject.push_back('\0');
    subject += options.requireEntryMain ? "1" : "0";
    std::vector<std::string> macros(options.macros.begin(), options.macros.end());
    std::sort(macros.begin(), macros.end());
    for (const std::string& macro : macros) {
        subject.push_back('\0');
        subject += macro;
    }
    return dir + "/" + cacheKeyHex(cacheHash128(subject)) + ".cnpg";
}

void warnCorrupt(const std::string& why, const std::string& file) {
    g_stats.corrupt += 1;
    std::fprintf(stderr, "警告: [程序缓存] 缓存条目无效（%s），回退完整编译: %s\n",
                 why.c_str(), file.c_str());
}

// 从载荷解析三条文本（结构不符返回 false + 原因）
bool parsePayload(const char* payload, std::size_t payloadLen, ProgramCacheEntry& entry,
                  std::string& why) {
    if (payloadLen < kPayloadHeaderBytes) {
        why = "载荷长度不足";
        return false;
    }
    const std::uint32_t asmLen = cacheReadU32LE(payload);
    const std::uint32_t humanLen = cacheReadU32LE(payload + 4);
    const std::uint32_t jsonLen = cacheReadU32LE(payload + 8);
    const std::size_t need = kPayloadHeaderBytes + static_cast<std::size_t>(asmLen) +
                             static_cast<std::size_t>(humanLen) +
                             static_cast<std::size_t>(jsonLen);
    if (need != payloadLen) {
        why = "载荷分片长度不符";
        return false;
    }
    const char* body = payload + kPayloadHeaderBytes;
    entry.asmText.assign(body, asmLen);
    entry.warningsHuman.assign(body + asmLen, humanLen);
    entry.warningsJson.assign(body + asmLen + humanLen, jsonLen);
    entry.valid = true;
    return true;
}

}  // namespace

// ==================== 对外入口 ====================

bool programCacheActive(const DriverOptions& options) {
    // 调用方须显式声明消费产物（check/ir 等命令不启用——命中会跳过其需要的阶段）
    // 静默豁免业务值: 功能开关不启用（调用方按命令语义显式声明·非查表默认值）
    if (!options.useProgramCache) return false;
    if (cacheDisabledByEnv()) return false;
    // --验证-ir 的验证副作用不可省：显式旁路缓存（保留旧行为）
    if (options.verifyIr) return false;
    // 静默豁免闭集穷举: 项目根不可推导=缓存目录为空（缓存禁用·直连编译）
    if (cacheLeafDir(options, "program", "CN_PROGRAM_CACHE_DIR").empty()) return false;
    // 静默豁免上游已诊断: 身份不可得已 warnIdentityUnavailableOnce 显形（缓存禁用）
    if (!compilerIdentity().valid) {
        warnIdentityUnavailableOnce();
        return false;
    }
    return true;
}

bool computeProgramCacheKey(const std::vector<ProgramUnitKey>& units,
                            const DriverOptions& options,
                            const std::vector<std::string>& cargoDepNames,
                            CacheKey128& key) {
    const CompilerIdentity& identity = compilerIdentity();
    // 静默豁免上游已诊断: 身份不可得已由 programCacheActive 显形告警（缓存禁用）
    if (!identity.valid) return false;
    std::string canonical = kKeyFormatVersion;
    canonical.push_back('\0');
    appendField(canonical, cacheKeyHex(identity.key));
    appendOptions(canonical, options);
    appendField(canonical, std::to_string(cargoDepNames.size()));
    for (const std::string& dep : cargoDepNames) appendField(canonical, dep);
    appendUnits(canonical, units);
    key = cacheHash128(canonical);
    return true;
}

bool loadProgramCache(const std::string& dir, const std::string& entryFilePath,
                      const DriverOptions& options, const CacheKey128& key,
                      ProgramCacheEntry& entry) {
    entry = ProgramCacheEntry();
    const std::string cacheFile = cacheFilePath(dir, entryFilePath, options);
    std::vector<char> buf;
    // 静默豁免业务值: 读缓存失败=未命中信号（missNew 计数·回退完整编译=唯一正确路径）
    if (!cacheReadWholeFile(cacheFile, buf, kMaxFileBytes)) {
        g_stats.missNew += 1;
        return false;
    }
    if (buf.size() < kCacheHeaderBytes) {
        warnCorrupt("文件长度不足头部", cacheFile);
        return false;
    }
    if (std::memcmp(buf.data(), kMagic, sizeof(kMagic)) != 0) {
        warnCorrupt("魔数不符（格式版本变化）", cacheFile);
        return false;
    }
    const char* p = buf.data() + sizeof(kMagic);
    const CacheKey128 storedKey{cacheReadU64LE(p), cacheReadU64LE(p + 8)};
    const std::uint64_t storedFnv = cacheReadU64LE(p + 32);
    const std::uint32_t payloadLen = cacheReadU32LE(p + 40);
    if (storedKey.a != key.a || storedKey.b != key.b) {
        g_stats.missStale += 1;  // 键不符=输入/选项/编译器已变化：正常重建
        return false;
    }
    if (static_cast<std::size_t>(payloadLen) != buf.size() - kCacheHeaderBytes) {
        warnCorrupt("载荷长度不符", cacheFile);
        return false;
    }
    const char* payload = buf.data() + kCacheHeaderBytes;
    if (cacheFnv1a64(payload, payloadLen, 14695981039346656037ULL) != storedFnv) {
        warnCorrupt("载荷校验和不符", cacheFile);
        return false;
    }
    std::string why;
    // 静默豁免上游已诊断: 载荷结构不符由 warnCorrupt 显形（stderr）+回退重编译
    if (!parsePayload(payload, payloadLen, entry, why)) {
        warnCorrupt(why, cacheFile);
        return false;
    }
    g_stats.hit += 1;
    return true;
}

void storeProgramCache(const std::string& dir, const std::string& entryFilePath,
                       const DriverOptions& options, const CacheKey128& key,
                       const ProgramCacheEntry& entry, bool verbose) {
    // 载荷分片长度上限守卫：超过 u32 可表示范围=格式无法承载（显式拒绝写入，
    //   绝不截断长度字段——截断=损坏条目静默扩散）
    constexpr std::size_t kMaxShard = 0xFFFFFFFFULL;
    if (entry.asmText.size() > kMaxShard || entry.warningsHuman.size() > kMaxShard ||
        entry.warningsJson.size() > kMaxShard) {
        g_stats.storeFail += 1;
        std::fprintf(stderr, "警告: [程序缓存] 产物尺寸超出缓存格式上限（未缓存）\n");
        return;
    }
    std::vector<char> out;
    out.reserve(kCacheHeaderBytes + kPayloadHeaderBytes + entry.asmText.size() +
                entry.warningsHuman.size() + entry.warningsJson.size());
    out.insert(out.end(), kMagic, kMagic + sizeof(kMagic));
    cacheAppendU64LE(out, key.a);
    cacheAppendU64LE(out, key.b);
    cacheAppendU64LE(out, 0);  // 保留（与 AST 缓存头同布局）
    cacheAppendU64LE(out, 0);  // 保留
    // payload（先占位校验和与长度，填完载荷后回写）
    const std::size_t fnvPos = out.size();
    cacheAppendU64LE(out, 0);
    const std::size_t lenPos = out.size();
    cacheAppendU32LE(out, 0);
    const std::size_t payloadStart = out.size();
    cacheAppendU32LE(out, static_cast<std::uint32_t>(entry.asmText.size()));
    cacheAppendU32LE(out, static_cast<std::uint32_t>(entry.warningsHuman.size()));
    cacheAppendU32LE(out, static_cast<std::uint32_t>(entry.warningsJson.size()));
    cacheAppendU32LE(out, 0);  // 保留
    out.insert(out.end(), entry.asmText.begin(), entry.asmText.end());
    out.insert(out.end(), entry.warningsHuman.begin(), entry.warningsHuman.end());
    out.insert(out.end(), entry.warningsJson.begin(), entry.warningsJson.end());
    const std::size_t payloadLen = out.size() - payloadStart;
    const std::uint64_t checksum =
        cacheFnv1a64(out.data() + payloadStart, payloadLen, 14695981039346656037ULL);
    for (int i = 0; i < 8; ++i) {
        out[fnvPos + static_cast<std::size_t>(i)] =
            static_cast<char>((checksum >> (8 * i)) & 0xFFu);
    }
    for (int i = 0; i < 4; ++i) {
        out[lenPos + static_cast<std::size_t>(i)] =
            static_cast<char>((static_cast<std::uint64_t>(payloadLen) >> (8 * i)) & 0xFFu);
    }
    if (!cacheEnsureDir(dir)) {
        g_stats.storeFail += 1;
        if (verbose) std::cerr << "提示: [程序缓存] 无法创建缓存目录: " << dir << "\n";
        return;
    }
    std::string werr;
    if (!cacheWriteFileAtomic(cacheFilePath(dir, entryFilePath, options), out, werr)) {
        g_stats.storeFail += 1;
        if (verbose) std::cerr << "提示: [程序缓存] " << werr << "\n";
        return;
    }
    g_stats.written += 1;
}

bool programCacheVerifyEnabled() {
    const char* env = std::getenv("CN_PROGRAM_CACHE_VERIFY");
    return env != nullptr && env[0] != '\0';
}

void noteProgramCacheVerifyResult(bool matched) {
    if (!matched) g_stats.verifyMismatch += 1;
}

void reportProgramCacheStats(bool verbose) {
    if (!verbose && !statsEnabled()) return;
    std::fprintf(stderr,
                 "[程序缓存] 命中=%lld 首建=%lld 失效重建=%lld 损坏回退=%lld "
                 "写入=%lld 写失败=%lld 验证不符=%lld\n",
                 g_stats.hit, g_stats.missNew, g_stats.missStale, g_stats.corrupt,
                 g_stats.written, g_stats.storeFail, g_stats.verifyMismatch);
}

}  // namespace driver
}  // namespace cn_compiler
