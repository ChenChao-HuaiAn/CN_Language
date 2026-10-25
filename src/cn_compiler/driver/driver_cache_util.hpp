// 驱动层缓存公共原语（任务 1156：自 driver_ast_cache.cpp 提炼·AST 缓存与程序
//   级缓存共用，纪律二「重复归纳」第 2 处即提炼）。
// 职责：
//   1. 128 位内容键（双通道 FNV-1a 64 位 + 长度掺入）与十六进制编码
//   2. 小端读写/整文件读取（含尺寸上限）/流式文件哈希（编译器自身身份）
//   3. 缓存叶目录探测（<项目根>/target/cache/<leaf>，环境变量覆盖）
//   4. tmp+rename 原子写（失败原因经 error 返回，调用方计数/verbose 显形）
// 设计纪律：
//   - 哈希算法与 1142 AST 缓存逐字节一致（改动=全部 AST 缓存失效，非必要不动）；
//   - 所有失败返回 bool/错误串，绝不以默认值继续（miss 不可静默）；
//   - 单文件 <=1000 行、单函数 <=100 行。
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "cn_compiler/driver/driver.hpp"

namespace cn_compiler {
namespace driver {

// 128 位缓存键（两条独立 FNV-1a 通道 + 长度掺入）
struct CacheKey128 {
    std::uint64_t a = 0;
    std::uint64_t b = 0;
};

// 内存内容 128 位键（非对抗场景碰撞概率 <=2^-128）
CacheKey128 cacheHash128(const std::string& data);

// FNV-1a 64（basis 由调用方给出；缓存文件校验和/流式哈希共用）
std::uint64_t cacheFnv1a64(const char* data, std::size_t n, std::uint64_t basis);

// 流式文件哈希（64KB 分块·不整读入内存）：返回 false=打开/读取失败
bool cacheHashStream(std::FILE* fp, CacheKey128& out);
bool cacheHashFile(const std::string& path, CacheKey128& out);

// 16 位小写十六进制（u64 -> 16 字符；键拼文件名为 32 字符）
std::string cacheHex64(std::uint64_t v);
std::string cacheKeyHex(const CacheKey128& key);

// 目录探测/递归创建（mkdir -p）
bool cacheDirExists(const std::string& path);
bool cacheEnsureDir(const std::string& path);

// 缓存条目路径是否存在（任意文件类型·stat 判据）：用于区分「不存在=正常未命中」
//   与「存在但不可读/为空/超限=损坏条目须显形」（1376/1431·空文件曾静默按未命中
//   处理——与「损坏必须显形回退」契约不符）。
bool cacheEntryPathExists(const std::string& path);

// 整文件读取：文件缺失/不可读/尺寸非正/超过 maxBytes 均返回 false（调用方按未命中处置）
bool cacheReadWholeFile(const std::string& path, std::vector<char>& out,
                        std::size_t maxBytes);

// 小端追加/读取（缓存文件头与载荷长度字段）
void cacheAppendU32LE(std::vector<char>& buf, std::uint32_t v);
void cacheAppendU64LE(std::vector<char>& buf, std::uint64_t v);
std::uint32_t cacheReadU32LE(const char* p);
std::uint64_t cacheReadU64LE(const char* p);

// 缓存文件头长度：魔数 8 + 5×u64（键两通道 + 环境键两通道 + 载荷校验和）+ 载荷长 u32
constexpr std::size_t kCacheHeaderBytes = 8 + 8 * 5 + 4;  // 52

// ==================== 编译器自身身份（1271 自 1156 提炼·两缓存共用） ====================

// 编译器身份 = 当前可执行文件内容哈希：编译器源码/构建配置任何变化=身份变，
//   机械保证「新编译器绝不命中旧产物」（无需人工维护格式版本号手动递增）。
struct CompilerIdentity {
    bool valid = false;
    CacheKey128 key;
};

// 探测并进程内记忆化编译器身份（每进程仅一次：首次调用读取可执行文件并做
//   128 位流式哈希；1156 程序缓存与 1142 AST 缓存共用同一实现）。
const CompilerIdentity& compilerIdentity();

// ==================== 键字段规范编码（1271 自 1156 appendField 提炼） ====================

// 字段以 '\0' 结尾拼入规范串（文件名/模块名/宏名/十六进制键均不含 NUL——
//   命令行 argv 无法承载 NUL，长度歧义为零；禁止以 0x1F 等可出现在字段内容中
//   的分隔符拼键：module="m\x1fx",macros={} 与 module="m",macros={"x"} 会同串）。
void cacheAppendField(std::string& out, const std::string& value);

// tmp+rename 原子写（同目录多进程/同进程多线程并发写互不撕裂；tmp 名含 pid+
//   进程内原子序号；POSIX rename 原子替换，Windows 先删旧条目再改名）。
//   失败时 error 写入具体阶段（调用方显形）。
bool cacheWriteFileAtomic(const std::string& path, const std::vector<char>& data,
                          std::string& error);

// 缓存叶目录（进程内按 leaf 记忆化·线程安全）：
//   1. envOverride（如 CN_AST_CACHE_DIR / CN_PROGRAM_CACHE_DIR）显式覆盖
//   2. <项目根>/target/cache/<leaf>——项目根 = options.stdlibDir 的父目录
//   3. 无法推导（stdlibDir 为空）：返回空串=该缓存禁用（直连，非静默错用）
const std::string& cacheLeafDir(const DriverOptions& options, const char* leaf,
                                const char* envOverride);

// 750（复审4 P2-5）：CN_NO_STR_ACCUM 环境开关判定（精确 =="1" 生效；空串/0/其它
//   不生效）。driver 侧唯一事实源：programCacheActive 据此禁用缓存（开关改变产物
//   且未入缓存键）+ toDriverOptions 据此下发 DriverOptions.disableStrAccum 至
//   IRGenerator（消除双 getenv 读取）。
bool strAccumDisabledByEnv();

}  // namespace driver
}  // namespace cn_compiler
