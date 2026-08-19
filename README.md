# CN 语言编译器

全中文语法的系统级编程语言（C++ 风格），从零用 C++17 实现：词法 → 语法 → 语义 → IR → 优化 → 代码生成（Win x64 / Linux ARM64 双后端）。

> 设计规格书：[plans/001 CN语言编译器设计规格书.md](plans/001%20CN语言编译器设计规格书.md) ｜ 实施计划：[plans/002 CN语言编译器实施计划.md](plans/002%20CN语言编译器实施计划.md) ｜ 分析报告：[plans/003-与C++和Rust对比分析报告.md](plans/003-与C++和Rust对比分析报告.md)

## 特性一览

- **类型**：整8~整128 / 正8~正128 / 浮32 / 浮64 / 布尔 / 字符 / 字符串 / 结构体 / 枚举 / 联合体 / 指针 / 数组 / 函数指针 / 结果<T,E> / 可选<T>
- **函数**：重载 / 默认参数 / lambda（捕获）/ 引用参数（T&）/ 变参打印 / 类型大小（sizeof）
- **OOP**：类 / 继承 / 虚函数 / 接口 / 抽象 / 友元 / 静态成员 / 常量成员 / 运算符重载（成员）
- **泛型**：泛型类 / 泛型函数 / 编译期单态化 / 接口约束（对标 trait bound）
- **错误处理**：结果 / 可选 + 强制检查 3 规则（对标 Rust must_use）
- **模块系统 v2.0**：文件即模块 / 包=crate / `::` 路径 / 导入全形式 / 默认私有 / 可见性交集 / 货舱.toml 依赖
- **安全**：数组越界 / 空指针解引用 运行时检查默认开启（-O2 亦生效）
- **优化器**：SSA / 常量折叠 / DCE / CSE / LICM / 强度削减 / 内联 / 尾调用 / 线性扫描寄存器分配（-O0~-O3）
- **后端**：Win x64（MASM）+ Linux ARM64（GAS）双后端
- **标准库**：核心 / 容器（向量/链表/栈/队列）/ 映射集合 / 算法 / 数学 / IO / 文件 / 时间 / 系统 / 字符串扩展

## 快速开始

```bash
# 一键构建（自动探测 VS2022 自带 cmake；-Test 追加全量测试门禁）
.\build.ps1 -Test

# 或者手动
cmake -S . -B target/build
cmake --build target/build --config Debug
```

```bash
# 编译并运行 Hello World
target\Debug\cn.exe build tests\e2e\01_hello\hello.cn --output target\hello.exe
target\hello.exe

# 全量单元测试
target\Debug\cn_unit_tests.exe

# 全量 E2E 测试
python tests/e2e/run_e2e.py --cn target\Debug\cn.exe
```

## CLI

```
cn <命令> [选项] <文件>
命令: build（编译+链接）/ compile（仅汇编）/ run / check / ir / ast / token
选项: --target win-x64|linux-arm64 / -O0~-O3 / --no-regalloc / --debug / --output / --货舱 / --stdlib
```

## 示例代码（CN 语言）

```cn
// 引用参数交换（A-1 2026-08）
函数 交换(整32& a, 整32& b) -> 空类型 {
    整32 临时 = a
    a = b
    b = 临时
}

函数 主() -> 整32 {
    整32 x = 1
    整32 y = 2
    交换(x, y)
    打印("交换后: ", x, " ", y)   // 2 1
    返回 0
}
```

## 项目结构

```
src/cn_compiler/    编译器（lexer/parser/semantic/ir/opt/codegen/module/driver）
src/runtime/        CN 运行时库（cnrt：IO/字符串/i128/数学/文件/时间/系统）
stdlib/             标准库（CN 语言实现，10 个模块）
tests/unit/         单元测试（Google Test，1189 个）
tests/e2e/          E2E 测试（编译→运行→输出比对，91 个用例，含负向规范用例）
plans/              设计规格书 / 实施计划 / 分析报告
tools/              辅助脚本（文件切分、E2E 等）
```

## 测试基线（2026-08-18）

- 单元测试：**1190/1190**（103 个测试套件）
- E2E：**97/97**（含 79_bootstrap_closed_loop 真实自举闭环用例）
- 编译警告：0（MSVC /W4 /WX）
- 覆盖：阶段 0~6 + 6b（模块 v2.0）+ 阶段 7 自举（CN 组件链 v2 真实 x64 MASM 后端 + 真实闭环验证）
- 负向规范 e2e 通道（预期编译失败）+ 补测：53 非法路径 / 80 抽象类 / 81-83 可见性交集 ❌ 象限负测 / 84 强制检查负测 / 85 选择贯穿 / 86 字符↔整数 / 87 跨包同名隔离 / 88 错误码契约(1..8) / 89 关键字穷举

## 已知限制（详见 plans/003 报告）

- 结果<结构体,E> 值字段内联布局（向量 读取 返回结构体结果场景待完善）
- 无 线程 / 正则 / 迭代器抽象 / 完整模式匹配（批次 C 规划中）；FFI 最小级「外部 函数」与模式匹配第一版已落地（e2e 62_ffi / 63_pattern_match）
- 引用变量声明（`整32& r = x`）与引用返回类型暂不支持（仅函数参数支持）
