# HANDOFF - CN语言编译器项目交接文档

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++从零编写编译器，直接生成汇编代码。

## 二、已经完成了什么

### 2.1 设计规格书（已批准）
- 文件：`plans/001 CN语言编译器设计规格书.md`（984行，14章）
- 状态：用户已批准，已通过规格自审（13个问题全部修复）
- 内容：语言设计概述、53个关键字、完整类型系统（整8~整128/正8~正128/浮32/浮64等）、词法结构（含Unicode转义/原始字符串/多行字符串）、语法规范（变量/函数/错误处理/控制流/OOP/结构体枚举/函数指针/模块系统）、6阶段编译器架构（词法->语法->语义->IR->优化->代码生成）、CN-IR设计（SSA形式）、代码生成（Win x64 + Linux ARM64）、优化器（12个Pass）、运行时设计、8阶段开发路线图（含自举）、4层测试体系、项目目录结构

### 2.2 cn-language-spec 技能（已创建）
- 目录：`.ai-coder/skills/cn-language-spec/`
- 文件数：16个
- 结构：SKILL.md + specs/(9个规范文档) + config/(2个配置) + templates/(3个模板) + README.md
- 内容：基于设计规格书创建，覆盖词法/类型/控制流/函数/结构体枚举/OOP/错误处理/模块系统全部语法规范

### 2.3 实施计划（已创建，Task 0.1~0.3已完成）
- 文件：`plans/002 CN语言编译器实施计划.md`（3904行）
- 状态：阶段零 Task 0.1（CMake骨架+CLI+诊断系统）、Task 0.3（E2E测试框架）已完成；Task 0.2（Google Test）实际已在Task 0.1中一并完成（tests/unit/CMakeLists.txt已存在，`cn_unit_tests`可构建）

### 2.4 阶段零已完成的工程基础设施
1. **Task 0.1**（commit 7bf427e）：CMake项目骨架、CLI框架（build/compile/run/check/ir/ast/token命令）、诊断系统（SourceLocation + 诊断引擎）、Google Test集成
   - 文件：`CMakeLists.txt`、`src/cn_main.cpp`、`src/cn_compiler/common/source_location.hpp`、`src/cn_compiler/common/diagnostics.hpp/.cpp`、`tests/unit/CMakeLists.txt`、`tests/unit/common/test_source_location.cpp`、`tests/unit/common/test_diagnostics.cpp`
   - 产物：`target/Debug/cn.exe`、`target/Debug/cn_unit_tests.exe`（MSVC单配置生成器，输出到 target/Debug/）
2. **Task 0.3**（commit c3eb089）：E2E测试框架
   - 文件：`tests/e2e/run_e2e.py`（运行器：编译→运行→比对，--verbose/--filter/--strict/--target-dir参数，彩色输出，UTF-8强制）、`tests/e2e/01_hello/hello.cn` + `hello.expected`、`tests/e2e/README.md`
   - CMake集成：Python3检测、`e2e`自定义目标（`$<TARGET_FILE:cn>`自动传编译器路径）、CTest注册（`ctest -C Debug -R e2e`）
   - 验证：`python tests/e2e/run_e2e.py --cn target/Debug/cn.exe` 正常识别用例并标记"未实现"（SKIP），框架本身可用

### 2.5 关键设计决策汇总（13项）
1. 语法基准：以plans/001风格为准（C风格OOP+异常已改为错误码+指针+手动内存管理）
2. 编译器实现：从零用C++重写
3. 代码生成后端：直接生成汇编代码，无外部依赖
4. 目标平台：初期 Win x64 + Linux ARM64
5. 内存管理：指针+手动内存管理（malloc/free式）
6. 错误处理：错误码+可选值（结果<T,E>/可选<T>），去掉异常关键字
7. OOP：完整保留（类/接口/继承/保护/虚拟/重写/抽象/实现/自身/父类）
8. 模块系统：文件即模块，导入/从，.路径，公开:/私有:标签式
9. 构建系统：CMake
10. 编译器架构：方案C（多趟+自定义IR）
11. 开发路径：参考Rust参考/003路线图，直到自举
12. 类型命名：中文位宽命名（整8/正8/整16/正16...整128/正128/浮32/浮64）
13. 字面量增强：Unicode字符转义、原始字符串、多行字符串（不支持数字下划线）

## 三、总结发现的问题

### 3.1 设计文档自审发现并修复的13个问题
- 严重(3)：字符类型未计入关键字、示例变量名与关键字冲突、原始/多行/在未登记
- 中等(5)：基类->父类未记录、联合体未定义、后缀与位宽类型不匹配、多行字符串转义未定义、强制检查规则未细化
- 轻微(5)：双源风险、阶段映射、构造器身份、入口文件混淆、函数指针风格

### 3.2 旧项目与新项目的关键差异
旧项目(CN_Language)是Rust风格（所有权/特征/结果+?运算符），新项目(CN_Language_C)是C++风格（指针/OOP/错误码）。cn-language-spec技能已完全基于新设计重写。

## 四、当前卡在哪

无卡点。阶段零 Task 0.1~0.3（工程骨架+CLI+诊断系统+单元测试+E2E框架）已完成。下一步是阶段一 Task 1.1（Token定义）。

## 五、下一步计划是什么

### 5.1 立即需要做的
执行 `plans/002 CN语言编译器实施计划.md` 阶段一：
- Task 1.1：Token定义（token.hpp/.cpp + test_token.cpp）
- Task 1.2：词法分析器
- Task 1.3：语法分析器（基础子集）
- ... 直到 Task 1.9：Hello World全链路打通（`cn build tests/e2e/01_hello/hello.cn` 输出"你好，世界"）

注意：Task 0.2（Google Test）的复选框在plans/002中尚未打勾，但实际工作已在Task 0.1完成（`tests/unit/CMakeLists.txt`、`cn_unit_tests`目标、CTest发现均可用），执行阶段一时可顺手确认/打勾。

### 5.2 实施时的注意事项
- E2E先行：每个功能必须先写E2E测试
- 禁止skip()：所有测试必须真正运行
- 编译产物放在target/目录
- 每个源文件不超过1000行，每个函数不超过100行
- 代码添加中文注释
- 完成后更新plans目录中的计划文档（打勾标记已完成）

## 六、踩过的坑绝对不要再踩

### 6.1 设计阶段经验
1. **plans/001与Rust参考存在根本性路线分歧**：必须明确以哪个为准。本项目已确认以plans/001（C++风格）为准，Rust参考仅作技术借鉴。
2. **异常处理与直接生成汇编不兼容**：异常需要栈展开表（SEH/DWARF），实现极其复杂。已改为错误码+可选值方案。
3. **数字下划线分隔**：用户明确不要此特性，不要自作主张添加。
4. **"基类"改为"父类"**：用户要求的命名变更，文档中已统一。
5. **关键字总数变更**：从最初的36个最终确定为53个（增加了字面量前缀"原始"/"多行"，字符计入类型关键字，保留了选择/情况/默认）。

### 6.2 技能创建经验
1. 旧项目的cn-language-spec技能内容基于Rust风格语法，不能直接复用，必须基于新设计规格书重写。
2. 技能目录结构（SKILL.md + specs/ + config/ + templates/）是优秀的组织模式，可以复用结构但重写内容。

### 6.3 编译器实现预期坑点
1. **中文词法分析**：UTF-8多字节字符识别，中文关键字最长匹配，关键字后必须有空格或符号分隔。
2. **虚表生成**：OOP的虚函数需要生成vtable，内存布局为vtable指针+父类字段+子类字段。
3. **多平台调用约定**：Win x64和Linux ARM64的寄存器约定完全不同，需要通过抽象后端接口隔离。
4. **128位整数**：硬件支持有限，可能需要运行时辅助函数。

### 6.4 Task 0.3经验（E2E框架）
1. **Windows控制台中文乱码**：Python脚本需 `sys.stdout.reconfigure(encoding="utf-8")` 强制UTF-8输出，否则管道/控制台代码页（GBK）导致中文乱码。
2. **项目根目录计算**：`run_e2e.py` 位于 `tests/e2e/`，项目根要用 `.parent.parent`（e2e→tests→根），用 `.parent` 会错误指向 tests/。
3. **cmake不在PATH**：本机cmake需用VS自带路径 `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`（ctest.exe同目录）。
4. **MSVC多配置生成器**：ctest需指定 `-C Debug`，否则报"Test not available without configuration"。
5. **findstr过滤中文会因编码不匹配导致退出码1/255**：验证命令避免用findstr过滤中文输出，直接看完整输出或过滤英文关键词。
6. **编译"尚未实现"检测**：阶段零编译器build命令返回"尚未实现"（stderr），运行器将其标记为SKIP而非FAIL，退出码0；`--strict`参数可将其视为失败（阶段一完成后全量验证用）。

## 七、项目文件结构现状

```
CN_Language_C/
├── .ai-coder/
│   ├── rules/                    # 项目规则文件
│   └── skills/
│       └── cn-language-spec/     # CN语言规范技能（16个文件）
│           ├── SKILL.md
│           ├── README.md
│           ├── specs/            # 9个规范文档
│           ├── config/           # 2个配置文件
│           └── templates/        # 3个模板文件
├── CMakeLists.txt                 # 顶层构建配置（含E2E测试CTest集成）
├── 更新日志.md                    # 提交前更新（覆盖式）
├── plans/
│   ├── 001 CN语言编译器设计规格书.md  # 设计规格书（已批准）
│   └── 002 CN语言编译器实施计划.md    # 实施计划（Task 0.1/0.3已打勾）
├── Rust参考/                      # Rust参考文档（只读参考，15个文件）
├── src/
│   ├── cn_main.cpp                # CLI入口
│   └── cn_compiler/common/        # 诊断系统（source_location.hpp/diagnostics.hpp/.cpp）
├── tests/
│   ├── unit/                      # 单元测试（common/下2个测试文件，cn_unit_tests目标）
│   └── e2e/                       # E2E测试框架（run_e2e.py + 01_hello用例 + README.md）
├── target/                        # 编译产物（Debug/下 cn.exe、cn_unit_tests.exe）
├── HANDOFF.md                    # 本文档
└── （stdlib/ 等目录尚未创建，待后续阶段）
```

## 八、相关文档索引

| 文档 | 路径 | 状态 |
|------|------|------|
| 设计规格书 | plans/001 CN语言编译器设计规格书.md | ✅ 已批准 |
| 实施计划 | plans/002 CN语言编译器实施计划.md | ✅ Task 0.1/0.3已打勾，0.2实际已完成 |
| cn-language-spec技能 | .ai-coder/skills/cn-language-spec/ | ✅ 已创建 |
| E2E测试说明 | tests/e2e/README.md | ✅ 已创建 |
| Rust参考-路线图 | Rust参考/003-CN语言开发路线图.md | 📖 参考用 |
| Rust参考-设计规格书 | Rust参考/002-CN语言设计规格书.md | 📖 参考用 |
| Rust参考-挑战与风险 | Rust参考/004-CN语言挑战与风险分析.md | 📖 参考用 |

---

*最后更新: 2026-08-12*
