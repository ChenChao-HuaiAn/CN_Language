// 程序级缓存键输入面登记表（任务 1376·机械对账单一事实源）
// 职责：以结构化常量表枚举「程序缓存键 + 条目文件名签名」的完整输入面，供
//   scripts/check_program_cache_key_audit.py 与实现/文档逐项对账——新增任何影响
//   产物的编译输入必须在此登记，否则 L1 门禁红（防「新增输入忘记入键」）。
// 契约（门禁双向执法·两个方向都是红）：
//   1. 代码有而表无：DriverOptions / ProgramUnitKey / CargoDependency 新增字段、
//      键构造函数新增 options.* / unit.* 引用、驱动层新增 CN_* 环境开关——一律红；
//   2. 表有而代码无：登记项声明的键构造/签名引用在实现中消失、证据锚文本不存在
//      ——一律红（防登记表变成过期文档）。
// 角色枚举（role）：
//   键      = 进入内容键（computeProgramCacheKey 材料）
//   条目身份 = 进入条目文件名签名（cacheFilePath）
//   派生    = 由已入键输入唯一决定（派生式变更=编译器身份变·必须给证据锚）
//   间接    = 影响加载路径/依赖解析，其结果经源闭包路径与摘要入键（必须给证据锚）
//   非产物  = 不改变汇编产物（必须给证据锚）
//   旁路    = 触发缓存禁用以保护正确性（如 --验证-ir / CN_NO_STR_ACCUM）
//   观测    = 只影响日志/统计/验证通道，不改变产物与键
// 载体语法（carrier）：
//   options.<字段> / unit.<字段> / cargo.<字段> / const:<常量名> /
//   env:<环境变量名> / arg:<键构造函数参数名>
// 锚格式（anchor）：`<仓库相对路径>#<锚文本>`；"-" = 由角色规则机械核验（仅键/
//   条目身份/环境开关/旁路允许）。锚文本禁含双引号（表解析以双引号分列）。
// 纪律：登记项 id 禁改义（改名/语义变化=键组成变化·须同步递增 kKeyFormatVersion）。
#pragma once
#include <cstddef>

namespace cn_compiler {
namespace driver {

// 单条缓存输入登记
struct ProgramCacheInputReg {
    const char* id;        // 稳定标识（门禁与提交信息共用）
    const char* category;  // 输入面类别（格式版本/编译器身份/源闭包/编译选项/宏集合/
                           //   依赖/条目身份/派生面/非产物面/观测面/缓存开关）
    const char* role;      // 角色（见文件头枚举）
    const char* carrier;   // 载体（options.X / unit.X / cargo.X / const:X / env:X / arg:X）
    const char* anchor;    // 证据锚（文件#锚文本）或 "-"
    const char* note;      // 说明（禁含 ASCII 逗号与双引号）
};

// 登记表本体（顺序=键材料的大致拼装顺序·门禁不依赖顺序）
inline constexpr ProgramCacheInputReg kProgramCacheInputs[] = {
    // ---- 格式版本 ----
    {"key.format_version", "格式版本", "键", "const:kKeyFormatVersion", "-",
     "缓存键格式版本（键组成变化必须递增·键材料首字段）"},
    // ---- 编译器身份（可执行文件内容哈希=源码/构建配置的机械身份） ----
    {"key.compiler_identity", "编译器身份", "键", "const:compilerIdentity", "-",
     "可执行文件内容 128 位摘要（源码/构建配置任何变化=键变·无独立版本号）"},
    // ---- 源闭包（拓扑序全模块单元） ----
    {"unit.count", "源闭包", "键", "arg:units", "-", "拓扑序闭包单元数"},
    {"unit.module_name", "源闭包", "键", "unit.moduleName", "-",
     "模块名（符号面身份）"},
    {"unit.file_path", "源闭包", "键", "unit.filePath", "-",
     "源文件路径（诊断与调试注释位置·含 stdlib 与依赖解析形态）"},
    {"unit.source_digest", "源闭包", "键", "unit.sourceDigest", "-",
     "源内容 128 位摘要（内容键·mtime 不入键）"},
    {"unit.module_dir", "源闭包", "键", "unit.moduleDir", "-",
     "模块树目录前缀（加载形态）"},
    {"unit.pkg_root", "源闭包", "键", "unit.pkgRoot", "-", "目录包包根名"},
    {"unit.is_entry", "源闭包", "键", "unit.isEntryUnit", "-",
     "命令行入口标记（合并可见性判定）"},
    {"unit.cargo_pkg_root", "源闭包", "键", "unit.cargoPkgRoot", "-",
     "货舱依赖包根标记（成员命名形态）"},
    // ---- 编译选项 ----
    {"opt.target", "编译选项", "键", "options.target", "-", "目标平台"},
    {"opt.opt_level", "编译选项", "键", "options.optLevel", "-", "优化级别"},
    {"opt.use_regalloc", "编译选项", "键", "options.useRegAlloc", "-",
     "寄存器分配开关"},
    {"opt.debug_info", "编译选项", "键", "options.debugInfo", "-",
     "调试信息（源码位置注释）"},
    {"opt.release_mode", "编译选项", "键", "options.releaseMode", "-",
     "发布构建（内建常量 调试模式）"},
    {"opt.use_cfi", "编译选项", "键", "options.useCfi", "-",
     "CFI 接口间接调用校验"},
    {"opt.require_entry_main", "编译选项", "键", "options.requireEntryMain", "-",
     "入口 主 函数强制检查"},
    // ---- 宏集合（-D 与货舱 [特性] 已合并） ----
    {"macro.set", "宏集合", "键", "options.macros", "-",
     "排序宏集合（-D 与货舱 [特性] 合并后·排序保证与传入序无关）"},
    // ---- 依赖 ----
    {"dep.cargo_names", "依赖", "键", "arg:cargoDepNames", "-",
     "货舱 [依赖] 包名清单（影响 loadedModules 与依赖解析）"},
    {"cargo.dep_name", "依赖", "键", "cargo.name",
     "src/cn_compiler/driver/driver_module.cpp#cargoDepNames.push_back(dep.name)",
     "依赖声明名（逐项并入 cargoDepNames）"},
    {"cargo.dep_version", "依赖", "间接", "cargo.version",
     "src/cn_compiler/driver/driver_module.cpp#cargoDep.value()->version ==",
     "版本决定解析路径（内置/本地）；解析结果经闭包路径与摘要入键"},
    {"cargo.config", "依赖", "间接", "options.cargoConfig",
     "src/cn_compiler/driver/driver_module.cpp#options.cargoConfig.findDependency",
     "依赖与特性配置；依赖解析产物经闭包入键·特性经宏集合入键"},
    // ---- 条目身份（文件名签名·同一入口同一选项组合原地覆盖） ----
    {"entry.entry_path", "条目身份", "条目身份", "arg:entryFilePath", "-",
     "入口路径（同一入口的选项组合共用一个条目名）"},
    {"entry.target", "条目身份", "条目身份", "options.target", "-",
     "条目名签名：目标平台"},
    {"entry.opt_level", "条目身份", "条目身份", "options.optLevel", "-",
     "条目名签名：优化级别"},
    {"entry.use_regalloc", "条目身份", "条目身份", "options.useRegAlloc", "-",
     "条目名签名：寄存器分配"},
    {"entry.debug_info", "条目身份", "条目身份", "options.debugInfo", "-",
     "条目名签名：调试信息"},
    {"entry.use_cfi", "条目身份", "条目身份", "options.useCfi", "-",
     "条目名签名：CFI"},
    {"entry.release_mode", "条目身份", "条目身份", "options.releaseMode", "-",
     "条目名签名：发布构建"},
    {"entry.require_entry_main", "条目身份", "条目身份", "options.requireEntryMain",
     "-", "条目名签名：入口主函数检查"},
    {"entry.macros", "条目身份", "条目身份", "options.macros", "-",
     "条目名签名：排序宏集合"},
    // ---- 派生面（不直接入键·派生式变更随编译器身份失效） ----
    {"opt.use_mem2reg", "派生面", "派生", "options.useMem2Reg",
     "src/cn_main.cpp#dopts.useMem2Reg = dopts.useRegAlloc && options.target",
     "派生自 useRegAlloc 与 target（两者入键）·派生式变更=编译器身份变"},
    // ---- 非产物面（不改变汇编产物） ----
    {"io.output", "非产物面", "非产物", "options.output",
     "src/cn_main.cpp#dopts.output = options.output",
     "输出路径只决定落盘位置"},
    {"io.verbose", "观测面", "观测", "options.verbose",
     "src/cn_main.cpp#dopts.verbose = options.verbose", "详细日志开关"},
    {"io.json", "非产物面", "非产物", "options.jsonDiagnostics",
     "src/cn_compiler/driver/driver_module.cpp#cachedProgram.warningsJson",
     "只切换警告输出通道；human 与 JSON 两份警告均随条目存储"},
    {"cache.enable", "非产物面", "非产物", "options.useProgramCache",
     "src/cn_compiler/driver/driver_program_cache.cpp#if (!options.useProgramCache)",
     "缓存启用开关（按命令语义由调用方声明）"},
    {"loc.cache_dir", "非产物面", "非产物", "env:CN_PROGRAM_CACHE_DIR", "-",
     "缓存目录覆盖（位置分离·不入内容键）"},
    // ---- 间接面（影响加载路径·结果经闭包入键） ----
    {"closure.stdlib_dir", "源闭包", "间接", "options.stdlibDir",
     "src/cn_compiler/driver/driver_module.cpp#const std::string candBuiltin = options.stdlibDir",
     "决定 stdlib 依赖解析路径→经闭包 filePath 与 digest 入键"},
    {"closure.cargo_dir", "依赖", "间接", "options.cargoDir",
     "src/cn_compiler/driver/driver_module.cpp#const std::string depRoot = options.cargoDir",
     "本地依赖查找基准→经闭包路径入键"},
    {"closure.has_cargo", "依赖", "间接", "options.hasCargoConfig",
     "src/cn_compiler/driver/driver_module.cpp#if (options.hasCargoConfig)",
     "是否加载货舱配置→依赖解析路径形态（结果入闭包键）"},
    // ---- 旁路开关（产物影响面无法入键时禁用缓存） ----
    {"bypass.verify_ir", "缓存开关", "旁路", "options.verifyIr", "-",
     "--验证-ir 有验证副作用不可省→缓存禁用"},
    {"bypass.str_accum_field", "缓存开关", "旁路", "options.disableStrAccum",
     "src/cn_main.cpp#dopts.disableStrAccum = cn_compiler::driver::strAccumDisabledByEnv();",
     "CN_NO_STR_ACCUM 产物影响经环境判定禁用缓存（driver 单点判定下发）"},
    {"bypass.no_cache", "缓存开关", "旁路", "env:CN_NO_PROGRAM_CACHE", "-",
     "缓存总开关逃生门：不读不写（产物仍由完整编译保证）"},
    {"bypass.str_accum_env", "缓存开关", "旁路", "env:CN_NO_STR_ACCUM", "-",
     "改变产物且未入键→激活时禁用缓存（精确等于字符串 1）"},
    // ---- 观测面（统计/验证通道·不改变产物与键） ----
    {"obs.stats", "观测面", "观测", "env:CN_PROGRAM_CACHE_STATS", "-",
     "统计行输出开关"},
    {"obs.verify", "观测面", "观测", "env:CN_PROGRAM_CACHE_VERIFY", "-",
     "验证通道：命中后完整重编译并逐字节比对·不改变产物"},
};

// 登记项数（供实现侧自检/门禁快速判空）
inline constexpr std::size_t kProgramCacheInputCount =
    sizeof(kProgramCacheInputs) / sizeof(kProgramCacheInputs[0]);

}  // namespace driver
}  // namespace cn_compiler
