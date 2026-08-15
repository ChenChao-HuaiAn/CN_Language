# HANDOFF 交接文档

## 我们在做什么任务

CN 语言编译器「模块系统 v2.0 全面改造」第 4 层：**crate 模型核心重构**（本改造的实质价值——命名空间隔离）。任务指令见 `plans/002-阶段6b-模块系统 v2.0」全面改造方案.md`（已批复）。

## 已经完成

### 第 1 层（规范更新）✅ 已完成
specs/08-模块系统.md 重写 v2.0（crate 模型/导入全形式/默认私有/可见性交集）、specs/09-包与依赖.md 新增

### 第 2 层（词法）✅ 已完成
ColonColon + 新关键字 4 个（模块/作为/包/货舱）+ 删 从

### 第 3 层（语法）✅ 已完成
ImportDecl 结构化（segments/alias/names/wildcard/isModuleDecl）+ parseImportDecl v2.0 全形式 + parseModuleDecl + 默认私有（模块级+类内）+ ast_printer 更新；单测 1078/1078 全绿

### 第 4 层（crate 模型核心重构）✅ 本层已完成（2026-08-15）
- **A. crate 分桶**：module.cpp mergeModuleDecls seenTypes 全局去重 → 按模块分桶（跨模块同名允许）；AST 节点加 moduleName 字段
- **B. 目录层级**：driver loadModuleTree 支持 `net/transport.cn`（模块名=相对入口完整路径，修正 pathStem 去目录 bug）；模块声明双路径搜索
- **C. use 导入表**：useImports_（模块名→符号集合/别名/通配符）；花括号别名纯名调用重写
- **D. prelude + 内置 key `::` 化**：27 个限定名 `.`→`::`（+旧点号兼容）；内置无需导入可用
- **E. IR/codegen 包前缀**：多模块场景 `模块名$` 前缀（定义/调用两侧同步）；单文件与 主 不加
- **F. 顶层常量/静态**：parser 顶层 `常量`/`静态` 分支 + 编译期常量折叠
- **G. 可见性交集检查**：模块私有类 merge 阶段不导出（类内公开成员不突破）
- **H. 单测**：反转 4 个 + 新增 9 个

## 总结发现的问题（本层踩坑，已修复）

1. **pathStem 去目录**：模块名计算必须用相对入口完整路径（`net/transport.cn` → `net::transport`），不能用 pathStem（只取 transport）
2. **单文件链接前缀**：单文件（无导入）不能加模块前缀（破坏 cn_main/内置映射）→ mergeModules singleModule 时 crateName 空
3. **泛型实例名 `$` 误剥**：resolveOverload 的模块前缀剥离须限定 `#` 存在且 `$` 在 `#` 前（`排序$整32` 不剥）
4. **对象方法调用误判 P1-1**：`动物.描述()` 的 动物 变量 → lookupVar 排除
5. **内置 `::` 化三处同步**：registerBuiltins + IR 映射 + 语义限定名拼接
6. **MSVC 时间戳粒度**：apply_diff 后构建可能不重编 → 用 PowerShell touch 强制

## 当前卡在哪

无阻塞。第 4 层全部完成，单测 1087/1087 全绿、编译零警告、E2E 35/45（10 失败全部为第 6 层预期迁移项）。

## 下一步计划（第 5~10 层）

- **第 5 层**：货舱.toml（解析器 + CLI + 依赖查找 + E2E）
- **第 6 层**：全量迁移——8 E2E（27/29/30/31/32/35/38/43 旧语法 `.`/`从` → `::`/花括号）+ 2 E2E 类内补 `公开:`（41/42）+ stdlib 10 模块 `公开:` 显式化
- **第 7 层**：新增 E2E（44_crate_isolate/45_import_syntax/46_module_tree/47_package_cargo/48_prelude/49_default_private/50_class_private/51_visibility_intersection）
- **第 8 层**：全链路集成（38_tool 迁移 v2.0）
- **第 9 层**：Debug 全面审查
- **第 10 层**：收尾

## 踩过的坑（绝对不要再踩，已同步 lessons.md）

1. **`::` 化同步点**：内置 key 从 `.` 改 `::` 涉及 registerBuiltins/IR 映射/语义限定名三处，漏改则内置静默失效——本层已验证 25_math/33_io/34_file/36_time/37_system 全通过
2. **模块名前缀副作用**：crate 前缀不能用于单文件与 主 函数，否则全量 E2E 链接失败 1120
3. **MSVC 增量编译时间戳**：改动后必须确认重编（findstr 看编译行），必要时 touch

## 当前分支

develop（未提交本层改动，待中文提交 + gitcode 推送；如 403 保留本地）
