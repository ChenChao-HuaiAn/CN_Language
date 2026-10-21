// 1142 模块缓存读环境开关：MSVC /WX 下 getenv 触发 C4996（std::getenv 同报）——
// 文件级豁免必须先于任何 CRT 头（仅本文件·MSVC 惯例·POSIX 平台无此告警）。
#define _CRT_SECURE_NO_WARNINGS
// AST 模块缓存实现（任务 1142·模块级预编译缓存第一台阶）
// 1185 构建修复：getenv 触发 C4996（/WX 下 warning 当错误·本机 MSVC 实测
//   develop 顶存量构建红）——补 _CRT_SECURE_NO_WARNINGS（semantic_expr_binary.cpp
//   等文件同款先例），零语义变化。
#define _CRT_SECURE_NO_WARNINGS
// 文件格式（小端）：
//   [8]  魔数 "CNASTC01"
//   [8]  内容哈希通道 a        [8] 内容哈希通道 b
//   [8]  环境哈希通道 a        [8] 环境哈希通道 b（模块名+宏集合）
//   [8]  payload FNV-1a 校验和（防位翻转/截断损坏）
//   [4]  payload 长度
//   [N]  payload（ast_serial::serializeProgram 产物，内含格式版本）
// 条目文件名（1271 起）：<路径哈希32hex>-<环境哈希32hex>.cnast——环境摘要
//   （编译器自身身份 + 模块名 + 排序宏集合）进入条目身份：同一路径不同 -D 宏
//   集合各有独立条目，交替构建各自稳定命中（1142 原始实现仅用路径哈希命名、
//   环境哈希只存文件头，同路径不同宏集合共写一条互相覆盖→交替宏每次失效重建：
//   1271 根治）。编译器身份（可执行文件内容哈希）机械覆盖「解析行为变化」——
//   禁止只靠人工递增 CACHE_FORMAT_VERSION（布局没变而词法/语法/预处理行为变了
//   时旧 AST 会被误命中）。
// 命中判定：文件名身份（路径+环境）→ 魔数 → 内容/环境哈希逐字段复核（防复制/
//   命名碰撞；内容更新=陈旧失效）→ 长度一致 → 校验和 → 反序列化成功。
//   任一不符=安全回退完整解析；损坏/版本不符显形。
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

#include "cn_compiler/driver/driver_cache_util.hpp"
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

// 缓存根目录：<项目根>/target/cache/ast（CN_AST_CACHE_DIR 覆盖；
//   项目根不可推导=缓存禁用）。探测/记忆化统一住 driver_cache_util。
const std::string& cacheDirRef(const DriverOptions& options) {
    return cacheLeafDir(options, "ast", "CN_AST_CACHE_DIR");
}

// ==================== 环境键（编译器身份+模块名+宏集合） ====================

// 身份不可得：显式告警一次并禁用 AST 缓存（不以「无身份键」静默继续——
//   miss 不可静默纪律；语义与 1156 程序缓存身份不可得处置同构）
void warnAstIdentityUnavailableOnce() {
    static bool warned = false;
    if (warned) return;
    warned = true;
    std::fprintf(stderr,
                 "提示: [AST 缓存] 无法读取编译器自身身份，AST 缓存禁用（直连解析）\n");
}

// 环境键：编译器自身身份 + 模块名 + 排序后的宏集合（-D 宏影响 #如果定义
//   条件编译结果）。字段以 '\0' 结尾编码（cacheAppendField·与 1156 程序缓存
//   同实现）：模块名/宏名不含 NUL，module="m\x1fx",macros={} 与
//   module="m",macros={"x"} 不再同键（0x1F 分隔的确定性键歧义：1271 审查 P2）。
//   排序保证同一宏集合与 -D 传入顺序 / std::unordered_set 遍历序无关——
//   环境摘要跨进程确定、稳定（不含时间/临时路径等进程瞬时量）。
CacheKey128 hashEnv(const CompilerIdentity& identity, const std::string& moduleName,
                    const std::unordered_set<std::string>& macros) {
    std::vector<std::string> ms(macros.begin(), macros.end());
    std::sort(ms.begin(), ms.end());
    std::string canonical;
    cacheAppendField(canonical, cacheKeyHex(identity.key));  // 编译器语言行为身份
    cacheAppendField(canonical, moduleName);
    for (const auto& m : ms) cacheAppendField(canonical, m);
    return cacheHash128(canonical);
}

// 缓存文件路径：<dir>/<路径哈希32hex>-<环境哈希32hex>.cnast
//   路径哈希=filePath 的 128 位内容键；环境哈希=hashEnv（模块名+排序宏集合）。
//   环境摘要进入文件名身份=同路径不同宏集合各有条目，互不覆盖（1271）；
//   文件头中的内容/环境键仍全量复核（损坏/复制条目显形回退）。
std::string cacheFilePath(const std::string& dir, const std::string& filePath,
                          const CacheKey128& envKey) {
    return dir + "/" + cacheKeyHex(cacheHash128(filePath)) + "-" + cacheKeyHex(envKey) +
           ".cnast";
}

// ==================== 文件头 ====================

constexpr char kMagic[8] = {'C', 'N', 'A', 'S', 'T', 'C', '0', '1'};
// 头总长 = kCacheHeaderBytes（魔数 8 + 5×u64 + 载荷长 u32 = 52）
constexpr std::size_t kMaxFileBytes = (1u << 30) + 64;  // payload 上限+头

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
bool tryLoadCache(const std::string& cacheFile, const CacheKey128& kContent,
                  const CacheKey128& kEnv, std::unique_ptr<Program>& ast) {
    std::vector<char> buf;
    if (!cacheReadWholeFile(cacheFile, buf, kMaxFileBytes)) {
        g_stats.missNew += 1;  // 文件缺失/不可读=首次构建常态（非损坏）
        return false;
    }
    if (buf.size() < kCacheHeaderBytes) {
        warnCorrupt("文件长度不足头部", cacheFile);
        return false;
    }
    if (std::memcmp(buf.data(), kMagic, sizeof(kMagic)) != 0) {
        warnCorrupt("魔数不符", cacheFile);
        return false;
    }
    const char* p = buf.data() + sizeof(kMagic);
    const CacheKey128 storedContent{cacheReadU64LE(p), cacheReadU64LE(p + 8)};
    const CacheKey128 storedEnv{cacheReadU64LE(p + 16), cacheReadU64LE(p + 24)};
    const std::uint64_t storedFnv = cacheReadU64LE(p + 32);
    const std::uint32_t payloadLen = cacheReadU32LE(p + 40);
    if (storedContent.a != kContent.a || storedContent.b != kContent.b ||
        storedEnv.a != kEnv.a || storedEnv.b != kEnv.b) {
        g_stats.missStale += 1;  // 键不符=内容/模块名/宏已变化：正常重建
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
void storeCache(const std::string& cacheFile, const CacheKey128& kContent, const CacheKey128& kEnv,
                const Program& ast, bool verbose) {
    std::vector<char> payload;
    std::string serr;
    if (!ast_serial::serializeProgram(&ast, payload, serr)) {
        g_stats.serializeFail += 1;
        std::fprintf(stderr, "警告: [AST 缓存] %s（该模块未缓存）\n", serr.c_str());
        return;
    }
    std::vector<char> out;
    out.reserve(kCacheHeaderBytes + payload.size());
    out.insert(out.end(), kMagic, kMagic + sizeof(kMagic));
    cacheAppendU64LE(out, kContent.a);
    cacheAppendU64LE(out, kContent.b);
    cacheAppendU64LE(out, kEnv.a);
    cacheAppendU64LE(out, kEnv.b);
    cacheAppendU64LE(out, cacheFnv1a64(payload.data(), payload.size(), 14695981039346656037ULL));
    cacheAppendU32LE(out, static_cast<std::uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());

    std::string werr;
    if (!cacheWriteFileAtomic(cacheFile, out, werr)) {
        g_stats.storeFail += 1;
        if (verbose) std::cerr << "提示: [AST 缓存] " << werr << "\n";
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
    // 编译器自身身份（每进程一次记忆化探测）：任何编译器源码/构建变化=键变，
    //   旧 AST 条目不命中（防「改了解析行为但没递增格式版本」的假绿）。
    const CompilerIdentity& identity = compilerIdentity();
    // 静默豁免上游已诊断: 身份不可得已 warnAstIdentityUnavailableOnce 显形并禁用缓存
    if (!identity.valid) {
        warnAstIdentityUnavailableOnce();
        return module::parseSourceText(source, filePath, moduleName, ast, imports, diags,
                                       macros);
    }
    const CacheKey128 kContent = cacheHash128(source);
    const CacheKey128 kEnv = hashEnv(identity, moduleName, macros);
    const std::string cacheFile = cacheFilePath(dir, filePath, kEnv);
    if (tryLoadCache(cacheFile, kContent, kEnv, ast)) {
        // 命中路径：按同一规则重建依赖表（collectImportDeps 与解析路径共用）
        imports.clear();
        module::collectImportDeps(*ast, moduleName, imports);
        const bool cacheUsable =
            !verifyEnabled() ||
            verifyAgainstParse(*ast, source, filePath, moduleName, macros);
        if (cacheUsable) {
            return true;
        }
        // 验证不符=可反序列化但与重新解析不一致的损坏条目（verifyAgainstParse 已
        //   显形+计入损坏回退）：丢弃命中结果，落到下方完整解析并重建覆盖——
        //   1213 修复：原实现此处直接 return 重解析结果、不写回，坏条目残留导致
        //   下次未开验证的运行静默命中错误 AST（验证通道只报不修=半截机制）。
        ast = nullptr;
        imports.clear();
    }
    // 未命中/失效/损坏/验证不符：完整解析（唯一正确路径；解析失败不缓存——诊断每次重建）
    if (!module::parseSourceText(source, filePath, moduleName, ast, imports, diags, macros)) {
        return false;
    }
    if (!cacheEnsureDir(dir)) {
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
