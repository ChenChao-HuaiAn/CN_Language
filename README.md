# CN 语言编译器

全中文语法的系统级编程语言（C++ 风格），从零用 C++17 实现完整编译器：
**词法 → 语法 → 语义 → IR → 优化 → 代码生成**，三后端：**Win x64（MASM）/ Linux x86_64（SysV GAS）/ Linux ARM64（GAS）**。

项目正在 **v2 自举重建**阶段：宿主编译器（`src/cn_compiler`）负责编译「用 CN 语言写成的编译器 v2 源码树」
（`CN语言编译器v2/`），v2 已达**自编译固定点**（自己编译自己，两代产物逐字节一致，见 `79_v2_自举闭环` E2E）。
v2 的两条最高准绳（2026-09-01 用户令）：① **性能超过 C++ 编译器**；② **安全接近 Rust 语言**
（悬垂指针 / 越界访问 / 泄漏零容忍）。

> 语法规范（唯一权威）：[plans/001 CN语言编译器设计规格书.md](plans/001%20CN语言编译器设计规格书.md)
> ｜ 任务进度唯一总入口：[plans/021-任务进度观察表.md](plans/021-任务进度观察表.md)
> ｜ v2 架构设计：[plans/013-自举重建-性能最优架构.md](plans/013-自举重建-性能最优架构.md)
> ｜ 三机在飞计划：[plans/025-三机任务统筹与实施计划.md](plans/025-三机任务统筹与实施计划.md)
> ｜ 语言成熟度验收体系：[plans/026-语言成熟度验收体系.md](plans/026-语言成熟度验收体系.md)
> ｜ 会话交接：[HANDOFF.md](HANDOFF.md)
> ｜ 对比分析报告：[plans/003-与C++和Rust对比分析报告.md](plans/003-与C++和Rust对比分析报告.md)（历史快照·含现状校正）

## 特性一览

- **类型**：整8~整128 / 正8~正128 / 浮32 / 浮64 / 布尔 / 字符 / 字符串 / 结构体 / 枚举 / 联合体 / 指针 / 数组 / 函数指针 / 结果<T,E> / 可选<T>
- **函数**：重载 / 默认参数 / lambda（捕获）/ 引用参数（T&）与引用返回 / 变参打印 / 类型大小（sizeof）
- **OOP**：类 / 继承 / 虚函数 / 接口（多态：接口变量/参数/返回，C2；单一实现去虚拟化直接调用，D3A）/ 抽象 / 友元 / 静态成员 / 常量成员 / 运算符重载（成员）
- **泛型**：泛型类 / 泛型函数 / 编译期单态化 / 接口约束（对标 trait bound）
- **错误处理**：结果 / 可选 + 强制检查 3 规则（对标 Rust must_use）
- **模块系统 v2.0**：文件即模块 / 包=crate / `::` 路径 / 导入全形式 / 默认私有 / 可见性交集 / 货舱.toml 依赖
- **编译期安全**（对标 Rust 借用检查器，详见 plans/019 / plans/023）：
  - 显式转移 `转移(x)`（拥有权 moves）+ 引用逃逸检查 + 只读借用与互斥
  - 不安全区边界：`不安全` 修饰的函数内才允许 10 类底层指针操作，越界一律编译期硬错误
  - 拥有型字符串与容器：字符串字段 / 容器元素的结构化自动释放（RAII）与深拷纪律
  - 联合体拥有型成员编译期限定 + `手动释放` 逃生舱（对标 Rust ManuallyDrop）
- **运行时安全**：数组越界 / 空指针解引用 运行时检查默认开启（-O2 亦生效）
- **控制流完整性**：`--cfi` 开启接口间接调用校验（分派目标 ∈ 已知实现表，默认关保性能；见 plans/008）
- **优化器**：SSA / 常量折叠 / DCE / CSE / LICM / 强度削减 / 内联 / 尾调用 / 线性扫描寄存器分配（-O0~-O3）
- **关键字体系**：46 个保留字 + 17 个上下文关键字（2026-09 关键字体系治理后，常用词占用最小化；见 `plans/001` §2.1a 与 plans/024）
- **标准库**：12 个模块——核心 / 容器 / 映射集合 / 算法 / 数学 / 随机数（xoshiro256++）/ 正则 / IO / 文件 / 时间 / 系统 / 字符串扩展（全部 CN 语言实现）
- **测试与度量设施**（plans/026 五支柱）：E2E 全量并行 runner / CN-Smith 差分采样与自治 daemon / 覆盖率与规范覆盖矩阵 / 构件面检测台账（B8）/ 缺陷族面扩散探针 famscan（B9）

## 快速开始

Windows（需 VS2022 MSVC 工具链）：

```powershell
# 一键构建（自动探测 VS2022 自带 cmake；-Test 追加全量测试门禁）
.\build.ps1 -Test

# 或者手动
cmake -S . -B target/build
cmake --build target/build --config Debug
```

```powershell
# 编译并运行 Hello World
target\Debug\cn.exe build tests\e2e\01_hello\hello.cn --output target\hello.exe
target\hello.exe

# 只做语法/语义检查（不生成代码）
target\Debug\cn.exe check <文件.cn>

# 全量单元测试
target\Debug\cn_unit_tests.exe

# 全量 E2E 测试（编译→链接→运行→输出比对）
python tests/e2e/run_e2e.py --cn target\Debug\cn.exe --jobs 8
```

Linux（x86_64 / ARM64）：

```bash
cmake -S . -B target/build && cmake --build target/build -j"$(nproc)"

# 编译并运行 Hello World（Linux 上 build 必须显式指定 --target；缺省目标为 win-x64）
./target/cn build tests/e2e/01_hello/hello.cn --target linux-x86_64 --output target/hello
./target/hello

# 只做语法/语义检查（不生成代码；无需 --target）
./target/cn check <文件.cn>

# 全量单元测试
./target/cn_unit_tests

# 全量 E2E 测试（建议显式并行度）
python3 tests/e2e/run_e2e.py --cn target/cn --jobs 8
```

## CLI

```
cn <命令> [选项] <文件>
命令: build（编译+链接）/ compile（仅汇编）/ run / check / ir / ast / token
选项:
  --target win-x64|linux-x86_64|linux-arm64   目标平台（Linux 上 build 必须显式指定）
  -O0 / -O1 / -O2 / -O3       优化级别（--opt <N> 等价写法；--no-regalloc 关寄存器分配）
  --debug / --发布（--release）
  --验证-ir                    IR 结构不变量校验（SSA/CFG/def-use；D25 覆盖率语料亦用它触达）
  --cfi                        接口间接调用校验（默认关）
  --output <路径>              产物路径（深层目录自动创建）
  --货舱 / --stdlib            包清单与标准库路径选项
  --verbose / --version / --help
```

## 示例代码（CN 语言）

```cn
// 引用参数实现交换
函数 交换(整32& a, 整32& b) -> 空类型 {
    整32 临时 = a;
    a = b;
    b = 临时;
}

函数 主() -> 整32 {
    整32 x = 1;
    整32 y = 2;
    交换(x, y);
    打印("交换后: ", x, " ", y);   // 交换后: 2 1
    返回 0;
}
```

> 示例实测口径（2026-09-17，291-a 深度机 linux-x86_64）：`check` 通过；`build --target linux-x86_64` 后运行输出 `交换后: 2 1`，退出码 0。
> **Linux 提示**：`build` 不指定 `--target` 时按缺省目标 win-x64 处理（路径与工具链按 Windows 走）会失败——跨平台构建务必显式 `--target`。
> 结构体用构造字面量 `类型{ 字段 = 值 }`（字段穷举），结果/可选 用 `正常/错误/某些/无` 构造——更多形态见 `tests/e2e/`。

## 项目结构

```
src/cn_compiler/    宿主编译器（C++17：lexer/parser/semantic/ir/opt/codegen/module/driver；三后端）
src/runtime/        CN 运行时库（cnrt：IO/字符串/i128/数学/文件/时间/系统）
CN语言编译器v2/     v2 自举源码树（CN 语言写成：词法/语法/语义/IR/代码生成/货舱解析）
CN语言编译器/       v1 自举组件（已推倒：字符串行 IR 性能失控；70~77 self_host 用例仍引用其残留）
stdlib/             标准库（CN 语言实现，12 个模块）
tests/unit/         单元测试（Google Test，1353 个 / 115 套件）
tests/e2e/          E2E 测试（编译→链接→运行→输出比对，423 个用例，含负向规范用例与双编译对照用例）
tests/matrix/       构件面检测台账（B8 制度）｜ tests/cnsmith_*/ CN-Smith 采样设施
scripts/            门禁与自检脚本（ci.ps1 / check_*.py / cnsmith_*.py / famscan.py）
plans/              设计规格书（001）/ 实施计划 / 进度总入口（021）/ 三机统筹（025）/ 验收体系（026）
docs/               战略呈报（判定标准体系等）
tools/              辅助脚本（文件切分等）
```

## 测试基线（2026-09-17，第 291-a 轮 · 深度机 linux-x86_64 实测）

- 单元测试：**1353/1353**（115 个测试套件）
- E2E：**423 用例 421 过 / 0 败 / 0 未实现 / 2 跳过**（linux 两平台各 2 跳＝`62_ffi`〔Windows API〕/`69_memory_management`〔计数平台差异〕，非缺陷；win-x64 无跳）
- 构建警告：0（GCC `-Wall -Wextra -Werror`；MSVC `/W4 /WX`）
- CLI 契约矩阵：50/50（linux-x86_64）
- 自举锚定链：v2 自编译固定点 `fix_p ≡ fix_s` 逐字节一致（E2E `78_v2`/`79_v2` 三平台承载）
- 组件对拍：宿主 vs v2 自举链产物逐字节对照（40+ 形态，见 plans/021 §4.2）
- 覆盖：阶段 0~7 全部 + 模块系统 v2.0 + 编译期安全体系（转移/逃逸/借用/不安全区/拥有型字符串）+ v2 自举闭环
- 门禁命令：Windows=`powershell -ExecutionPolicy Bypass -File scripts/ci.ps1`（构建零警告 + 单测 + 全量 E2E + 各静态检查）；Linux=`./target/cn_unit_tests` + `python3 tests/e2e/run_e2e.py --cn target/cn --jobs 8` + `python3 scripts/check_*.py`
- 最新门禁数字以 [plans/021-任务进度观察表.md](plans/021-任务进度观察表.md) §二 与各机 `HANDOFF.md` 节为准（每轮刷新）

## 已知限制（详见 plans/021 §三 排班队列）

- **v2 欠账族**（双编译对照口径 D33~D38，plans/021 §三-D）：v2p 优化旗标静默忽略；`?` 错误传播运算符 v2 侧缺失（宿主已实现）；v2 遮蔽恢复读错槽；v2 win 后端无 `__chkstk` 发射；v2 负测防线 17 例待对齐
- 编译期安全体系仍在推进：真 move 设施与 `复制(x)` 内置、跨语句 NLL 活跃区间、并发安全（Send/Sync，E1 待并发标准库立项）未开工
- 结果/可选 的部分复杂形态（嵌套成员链检查、赋值位构造器）当前采取保守拒绝；v2 侧联合体对结果/可选成员保守拒绝
- 映射集合为 O(n) 线性表（字符串键待 P1，键比较要求 `==`）；正则/随机数为子集实现（见各模块头注的诚实边界）
- 无 线程 / 网络 / 迭代器抽象 / 完整模式匹配（长线清单 plans/007 F 组，v2 时代待重评）；
  FFI 最小级「外部 函数」与模式匹配第一版已落地（e2e 62_ffi / 63_pattern_match）
