// AST 模块缓存（任务 1142·模块级预编译缓存第一台阶）
// 职责：驱动层按模块（源文件粒度）缓存「词法+语法」产物（AST 反序列化重建），
//   跨进程复用——命中跳过词法/语法分析；stdlib 等稳定模块集合收益最大。
// 设计要点（评审结论见任务 1142 报告）：
//   1. 粒度：每源文件一条缓存条目（内容哈希为键）——stdlib/用户代码统一规则，
//      任一文件内容变化只失效该文件条目（模块级增量语义）。
//   2. 失效键：源内容（双 FNV-1a 128 位）+ 编译器自身身份（可执行文件内容
//      哈希·1271 由 1156 身份探测提炼共用）+ 模块名 + 宏集合（-D 参与条件编译）
//      + 序列化载荷内格式版本（CACHE_FORMAT_VERSION，枚举/字段布局变更递增）
//      + 源长度（掺入哈希，防尾零歧义）。条目文件名身份 = 路径哈希 + 环境摘要
//      （编译器身份+模块名+排序宏集合，1271）——同一路径不同宏集合各有独立
//      条目、各自稳定命中，不互相覆盖；文件头仍存全量键做命中复核（损坏/复制
//      条目显形回退）。字段以 '\0' 结尾编码（cacheAppendField），无分隔符歧义。
//   3. 存放：<项目根>/target/cache/ast/（项目根由 stdlibDir 推导·gitignore 面）；
//      CN_AST_CACHE_DIR 覆盖；探测失败（stdlibDir 为空）=缓存禁用（直连解析）。
//   4. 并发：同目录多进程写=各自 tmp 文件 + rename 原子替换；读侧 POSIX rename
//      原子性保证读到完整旧版或新版；缓存文件损坏/版本不符=显形警告+回退
//      完整解析（禁静默沿用——缓存最危险的失效形态是「命中但过期」，故键
//      含内容哈希全量校验 + payload FNV 校验和 + 反序列化全程校验三层）。
//   5. 开关：CN_NO_AST_CACHE=1 禁用；CN_AST_CACHE_STATS=1 输出统计；
//      CN_AST_CACHE_VERIFY=1 命中后同时重新解析并打印比对（验证通道）。
// 单文件 <=1000 行、单函数 <=100 行约束。
#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/model/ast.hpp"

namespace cn_compiler {
namespace driver {

// 带 AST 缓存的模块解析：与 module::parseSourceText 同语义出口
//   （ast/imports/diags 输出一致；缓存不可用时直连解析）。
// 返回 false=解析失败（diags 已记录，与 parseSourceText 失败语义一致）。
bool parseModuleWithCache(const std::string& source, const std::string& filePath,
                          const std::string& moduleName,
                          const std::unordered_set<std::string>& macros,
                          const DriverOptions& options,
                          std::unique_ptr<Program>& ast,
                          std::vector<std::string>& imports,
                          Diagnostics& diags);

// 缓存统计汇总输出（CN_AST_CACHE_STATS=1 或 verbose 时；编译收尾每次调用）
void reportAstCacheStats(bool verbose);

}  // namespace driver
}  // namespace cn_compiler
