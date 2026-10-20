// 程序级预编译缓存（任务 1156·#032 增量第二台阶·方案 A=最终汇编产物缓存）
// 职责：对「完整编译输入」做产物级缓存——输入内容全哈希命中后直接复用汇编文本，
//   跳过语义分析/IR/优化/代码生成（加载段仍走 AST 缓存），as/link 由调用方执行。
// 缓存键完备性（任一遗漏=静默错产物，故逐项入键）：
//   1. 编译器自身身份（可执行文件内容哈希）：源码/构建配置任何变化=键变
//   2. 缓存键格式版本（键组成本身变化须递增）
//   3. 全模块闭包（拓扑序）：模块名 + 文件路径 + 源内容 128 位摘要 + moduleDir +
//      pkgRoot + isEntryUnit + cargoPkgRoot（源内容含 stdlib/导入闭包全部文件）
//   4. 编译选项：target/optLevel/useRegAlloc/debugInfo/releaseMode/useCfi/
//      requireEntryMain + 排序后的宏集合（-D 与货舱 [特性] 已并入）
//   5. 货舱 [依赖] 包名清单（并入 program->loadedModules 影响语义判定）
// 显式旁路：--验证-ir（保留验证副作用）；CN_NO_PROGRAM_CACHE=1 逃生门。
// 存放：<项目根>/target/cache/program/<键32hex>.cnpg（gitignore 面）；
//   CN_PROGRAM_CACHE_DIR 覆盖；tmp+rename 原子写；损坏/版本不符显形回退重编译。
// 验证通道：CN_PROGRAM_CACHE_VERIFY=1——命中后仍完整重编译并逐字节比对产物。
// 单文件 <=1000 行、单函数 <=100 行。
#pragma once
#include <string>
#include <vector>

#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/driver/driver_cache_util.hpp"

namespace cn_compiler {
namespace driver {

// 参与程序缓存键的模块单元摘要（调用方按拓扑序填充）
struct ProgramUnitKey {
    std::string moduleName;     // 模块名（符号面身份）
    std::string filePath;       // 源文件路径（诊断位置/调试注释位置）
    std::string sourceDigest;   // 源内容 128 位摘要（32 hex·loadModuleTree 计算）
    std::string moduleDir;      // 模块树目录前缀（加载形态）
    std::string pkgRoot;        // 目录包包根名
    bool isEntryUnit = false;   // 命令行入口标记（合并可见性判定）
    bool cargoPkgRoot = false;  // 货舱依赖包根标记（成员命名形态）
};

// 缓存条目（汇编文本 + 该次编译的诊断警告文本，命中时原样重放）
struct ProgramCacheEntry {
    std::string asmText;
    std::string warningsHuman;  // diagnostics.format()（stderr human 通道）
    std::string warningsJson;   // diagnostics.formatJson()（--json 通道）
    bool valid = false;
};

// 程序缓存是否可用：调用方已声明需要产物（options.useProgramCache）、未被
//   CN_NO_PROGRAM_CACHE 关闭、缓存目录可推导、编译器自身身份可读、未开 --验证-ir。
bool programCacheActive(const DriverOptions& options);

// 计算程序级缓存键；返回 false=编译器自身身份不可得（缓存禁用·显式告警一次）
bool computeProgramCacheKey(const std::vector<ProgramUnitKey>& units,
                            const DriverOptions& options,
                            const std::vector<std::string>& cargoDepNames,
                            CacheKey128& key);

// 命中读取：魔数/键/长度/校验和全部通过才返回 true（valid=true）；
//   文件缺失=普通未命中（计数），键不符=陈旧（内容/编译器已变·正常重建），
//   损坏=stderr 显形 + 未命中（回退重编译）。
//   dir=缓存目录；entryFilePath=入口文件（与选项签名共同决定条目文件名——同一
//   入口的旧版产物被新版覆盖，磁盘占用按入口而非按内容版本累积）。
bool loadProgramCache(const std::string& dir, const std::string& entryFilePath,
                      const DriverOptions& options, const CacheKey128& key,
                      ProgramCacheEntry& entry);

// 成功编译后原子写入（写失败不阻断编译，verbose/统计可见）
void storeProgramCache(const std::string& dir, const std::string& entryFilePath,
                       const DriverOptions& options, const CacheKey128& key,
                       const ProgramCacheEntry& entry, bool verbose);

// CN_PROGRAM_CACHE_VERIFY=1：命中后仍完整重编译并逐字节比对（验证通道）
bool programCacheVerifyEnabled();

// 验证通道结果登记（matched=false 计入「验证不符」统计，由调用方显形报错）
void noteProgramCacheVerifyResult(bool matched);

// 统计汇总（CN_PROGRAM_CACHE_STATS=1 或 verbose 时输出）
void reportProgramCacheStats(bool verbose);

}  // namespace driver
}  // namespace cn_compiler
