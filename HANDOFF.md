# HANDOFF 交接文档

**交接时间**: 2026-08-15 14:05（第 5 层完成后）

## 我们在做什么任务

CN 语言编译器「模块系统 v2.0 全面改造」共 10 层。当前已完成第 1-5 层：
- 第 1 层：规范更新（specs/08 v2.0 + 09-包与依赖.md 新增）✅
- 第 2 层：词法（ColonColon + 新关键字 模块/作为/包/货舱）✅
- 第 3 层：语法（ImportDecl 结构化 v2.0 + parseModuleDecl + 默认私有）✅
- 第 4 层：crate 重构（module.cpp 分桶 + 目录层级 + 语义分域 + use 导入表 + prelude + 内置 key `::` 化 + IR/codegen 包前缀 + 可见性交集）✅
- **第 5 层：货舱.toml 依赖管理（本次完成）** ✅
- 剩余：第 6 层（全量迁移旧语法 E2E + stdlib 公开: 显式化）、第 7-10 层

## 已经完成了什么（第 5 层）

1. **货舱.toml 解析器**：`src/cn_compiler/driver/cargo_parser.hpp/.cpp`——TOML 子集解析（[货舱] 名称/版本 + [依赖] 名称="版本"），宽容未知节，# 注释，Windows UTF-8 宽路径兼容（先窄后宽）。10 个单测全绿。
2. **CLI 集成**：`cn_main.cpp` 新增 `--货舱 <路径>` / `--stdlib <路径>` 选项 + 入口同目录自动发现 货舱.toml（对标 Cargo）+ stdlib 相对 exe 逐级上溯探测（最多 3 层）。
3. **依赖查找**：`driver_module.cpp` loadModuleTree 扩展候选 3/4——入口同目录 → 货舱 [依赖]（版本=内置 → stdlib；其他 → 依赖/<名>/<名>.cn 或 包.cn）→ stdlib 兜底。
4. **包.cn 支持**：parser 顶层循环新增 `公开 导入` 分支（无冒号再导出，规格书09-三）+ ImportDecl.access 字段。
5. **E2E 47_package_cargo**：货舱.toml（本地 网络库 + 内置 核心）+ 依赖/网络库/（包.cn crate 根 + 网络.cn + 网络/传输控制.cn 子模块）全通过。

## 测试结果

- 全量单测：**1097/1097 全绿**（1087 + 10 新增）
- 编译警告：**0**（MSVC /W4 /WX）
- E2E：**36/46 通过**（第 4 层 35/45 → 新增 47 通过，无新增回归）
- 10 个失败均为**第 6 层迁移项**（预期）：旧语法（`.`/`从`）8 个 = 27_module、29_core、30_container、31_map_set、32_algorithm、35_string_ext、38_tool、43_module_import；默认私有 2 个 = 41_ctor_overload、42_generic_field_loop

## 总结发现的问题（坑）

1. **Windows 中文路径三重编码**（最严重）：命令行 argv（GBK）↔ 源码 UTF-8 字面量 ↔ 磁盘 UTF-8 文件名。
   - 解法：argv **保持 GBK** 供工具链（ml64/link 按 ANSI 代码页解释路径）；runModulePipeline 入口文件 + 货舱.toml 自动发现路径 **转 UTF-8**（driver 层统一 UTF-8）。
   - `ansiToUtf8()` 在 cn_main.cpp：ANSI → UTF-8（经宽字符中转）。
2. **argv 转 UTF-8 会破坏 E2E 18/19**：pathStem(file) 变 UTF-8 后中间文件名（.asm/.exe）传给 ml64/link 乱码 → 不能整体转 argv，只能 driver 层入口转 UTF-8。
3. **依赖/ 目录误判 crate 内部模块**：`依赖/网络库/网络.cn` 以 entryDir 前缀开头被当入口模块树，moduleDir 与 depRoot 重复拼接 → external 判定加 `依赖/`、`stdlib/` 前缀。
4. **cargo_parser 未引用形参 C4100**（/WX 转错误）：readTomlFile 的 error 参数未用 → 移除，由 loadCargoConfig 统一写 error。

## 当前卡在哪

无卡点。第 5 层全部完成并推送 gitcode（`2680335..eef0cd9`）。

## 下一步计划（第 6 层）

全量迁移：
- 8 个旧语法 E2E（`.`/`从` → `::`/花括号）：27_module、29_core、30_container、31_map_set、32_algorithm、35_string_ext、38_tool、43_module_import
- 3 个单测迁移（旧语法导入）
- stdlib 10 模块 `公开:` 显式化（当前文件顶部 公开: 已是显式，需核对）
- 2 个 `函数 使用()` 改名
- 迁移后 E2E 应 46/46 全绿

## 踩过的坑绝对不要再踩（已同步 lessons.md）

- **不要整体转换 Windows argv 为 UTF-8**——工具链（ml64/link/ml64 汇编）按 ANSI 代码页解释路径，UTF-8 中文文件名会乱码；只在 driver 层（runModulePipeline 入口 + 货舱.toml 自动发现）转 UTF-8。
- **依赖/、stdlib/ 目录内的文件必须视为外部 crate 模块**（moduleName = 文件名主干），否则与入口 crate 模块树混淆导致路径重复拼接。
- 编译验证必须用 VS 自带 cmake 完整路径（`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`），系统 PATH 无 cmake。
- MSVC /WX 下任何未引用形参（C4100）都会中断构建——新函数签名参数要么使用要么删。
