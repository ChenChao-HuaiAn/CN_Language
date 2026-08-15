// CN语言编译器 货舱.toml 解析器（模块系统 v2.0 第 5 层，规格书09-二 包与依赖）
// 对标 Cargo.toml：包元数据（[货舱] 名称/版本）+ 依赖声明（[依赖] 名称 = "版本"）。
// 设计要点：
//   1. TOML 子集解析（[节] + 键 = "值" + # 注释），不引入外部 TOML 库（避免依赖）
//   2. 版本当前语义：内置 = 编译器内置 stdlib；其他 = 本地路径查找（依赖/<名>/ 或同目录）
//   3. 结构体 CargoConfig / CargoDependency 供 DriverOptions 与依赖查找共用
// 单文件 <=1000 行、单函数 <=100 行约束。
#pragma once
#include <string>
#include <vector>

namespace cn_compiler {
namespace driver {

// 单个依赖声明：名称 = "版本"
//   版本 = "内置" -> 编译器内置 stdlib（如 核心 = "内置"）
//   版本 = 其他   -> 本地路径查找（相对入口目录：依赖/<名称>/<名称>.cn 等）
struct CargoDependency {
    std::string name;     // 依赖包名（跨包导入路径首段，如 网络库）
    std::string version;  // 版本约束（"内置" = 内置 stdlib；其他 = 本地路径）
};

// 货舱.toml 配置：解析 [货舱] 与 [依赖] 节后的结果
struct CargoConfig {
    std::string name;                         // 包（crate）名（[货舱] 名称，可选）
    std::string version;                      // 语义化版本（[货舱] 版本，可选）
    std::vector<CargoDependency> deps;        // [依赖] 声明列表

    // 按依赖名查找（未找到返回 nullptr）
    const CargoDependency* findDependency(const std::string& depName) const;
};

// 解析 货舱.toml 文本（TOML 子集）：
//   支持 [节] 标题、键 = "值"（双引号字符串）、# 注释（整行与行尾）、空行/空白
//   未知节/未知键忽略（宽容解析，方便扩展）；[货舱] 名称/版本 与 [依赖] 项提取。
// 返回 false 时 error 记录具体解析错误（如缺少右引号）；成功返回 true。
bool parseCargoToml(const std::string& text, CargoConfig& out, std::string& error);

// 读取并解析 货舱.toml 文件（UTF-8，自动去除 BOM）。
// 文件不存在返回 false 且 error 为空（非错误——自动发现场景允许无配置）；
// 文件存在但解析失败返回 false 且 error 非空（调用方应报错）。
bool loadCargoConfig(const std::string& tomlPath, CargoConfig& out, std::string& error);

} // namespace driver
} // namespace cn_compiler
