// CN语言编译器驱动层路径工具族（单一职责：源文件路径的提取/探测/归一）
// 职责：
//   1. pathStem/pathDir/tryReadSource：模块加载链既有路径原语（自 driver_module
//      内部工具迁出——该文件超 900 行警戒线先拆再加，工具族落独立文件）
//   2. normalizeEntryPath：入口源文件路径归一——相对路径拼当前工作目录转绝对，
//      分隔符统一正斜杠，逐段词法折叠（./ 与 ../ 与重复分隔符）。模块归属判定
//      （相对入口目录剥离/外部 bare 判据）由此与命令行路径形态解耦：入口写成
//      「主.cn」（无目录前缀）时 pathDir 为空串，依赖候选链加载的带目录文件
//      「入口目录前缀剥离」恒不命中 → 外部模块 bare 命名（pathStem 砍掉包目录
//      前缀）→ 包成员 pkgRoot 失效 → 自包导入边漏豁免 → 拓扑排序假环
//      （IR -> IR生成 -> IR，同一程序树根调用正常、子目录调用报环实测）。
#ifndef CN_COMPILER_DRIVER_PATH_UTIL_HPP
#define CN_COMPILER_DRIVER_PATH_UTIL_HPP

#include <string>

namespace cn_compiler {
namespace driver {

// 提取文件主干名（去除目录与扩展名）：tests/主.cn -> 主
std::string pathStem(const std::string& path);

// 提取文件所在目录（含末尾分隔符）；无目录返回空串
std::string pathDir(const std::string& path);

// 安静尝试读取文件（不写 error）：探测候选路径是否存在
bool tryReadSource(const std::string& path, std::string& out);

// 入口源文件路径归一：相对路径拼当前工作目录转绝对 + 分隔符统一正斜杠 +
//   逐段词法折叠（"" 重复分隔与 "." 当前段丢弃、".." 上溯一段——已拼 cwd 前
//   缀后全程绝对，栈底 ".." 越根丢弃）。
// 规则显式不猜（禁兜底）：
//   ① 绝对判定穷举 = 首字符 '/'（POSIX 绝对）、"//"（UNC）、或「盘符字母+:」
//      （win）；命中不拼 cwd，未命中必为相对（拼 cwd 前缀）；
//   ② 词法折叠不触文件系统（不解析符号链接；std::filesystem 弃用——MSVC
//      u8path 中文路径挂起，module.cpp 同款约束）；纯字符串字节操作
//      （UTF-8 多字节不含 '/' '\\' 字节，逐字节扫描安全）；
//   ③ cwd 获取失败（getcwd 不可用/环境损坏）= 返回 false 由调用方显式报错
//      中止——禁静默回退原形态继续（静默即假环回归通道）。
// 返回 true 时 out = 归一绝对路径；返回 false = cwd 获取失败（out 保持入参）。
[[nodiscard]] bool normalizeEntryPath(const std::string& path, std::string& out);

}  // namespace driver
}  // namespace cn_compiler

#endif  // CN_COMPILER_DRIVER_PATH_UTIL_HPP
