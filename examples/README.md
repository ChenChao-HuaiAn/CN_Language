# CN Language 示例代码目录

本目录包含CN Language的各类示例代码，按功能和用途分类组织。

## 📁 目录结构

```
examples/
├── basic/                  # 基础入门示例
├── syntax/                 # 语法特性示例
│   ├── arrays/            # 数组相关
│   ├── pointers/          # 指针相关
│   ├── structs/           # 结构体相关
│   ├── enums/             # 枚举相关
│   ├── functions/         # 函数相关
│   ├── preprocessor/      # 预处理器相关
│   ├── control-flow/      # 控制流相关
│   └── floats/            # 浮点数相关
├── system/                 # 系统编程示例
│   ├── memory/            # 内存管理
│   ├── concurrency/       # 并发控制
│   ├── io/                # I/O系统
│   ├── inline-asm/        # 内联汇编
│   ├── bitwise/           # 位操作
│   └── interrupts/        # 中断处理
├── os-kernel/              # 操作系统内核相关
├── tests/                  # 测试用例集合
│   ├── modules/           # 模块系统测试
│   ├── features/          # 功能特性测试
│   └── edge-cases/        # 边界情况测试
├── docs/                   # 文档
└── legacy/                 # 遗留的C文件（编译器生成）
```

## 🚀 快速开始

### 1. 基础示例 (basic/)

适合初学者，展示CN Language的基本语法：

- `hello_world.cn` - Hello World程序
- `hello_compile.cn` - 简单的编译示例
- `print_only.cn` - 打印输出示例

**运行方式**：
```bash
cnc basic/hello_world.cn -o hello
./hello
```

### 2. 语法特性 (syntax/)

展示CN Language的各种语法特性：

#### 数组 (arrays/)
- `array_examples.cn` - 数组完整示例
- `array_assign_example.cn` - 数组赋值
- `test_array_length.cn` - 数组长度测试

#### 指针 (pointers/)
- `pointer_example.cn` - 指针基础
- `pointer_arithmetic.cn` - 指针运算
- `pointer_reference.cn` - 指针引用

#### 结构体 (structs/)
- `struct_complete_example.cn` - 完整结构体示例
- `struct_member_access.cn` - 成员访问

#### 函数 (functions/)
- `function_examples.cn` - 函数定义和调用
- `function_pointer_advanced.cn` - 函数指针
- `callback_example.cn` - 回调函数

#### 预处理器 (preprocessor/)
- `preprocessor_example.cn` - 预处理器基础
- `preprocessor_chinese.cn` - 中文宏支持

### 3. 系统编程 (system/)

展示CN Language的系统级编程能力：

#### 内存管理 (memory/)
- `memory_manager_demo.cn` - 内存管理器演示（12KB）
- `memory_access_example.cn` - 内存访问
- `allocator_example.c` - 分配器示例

#### 并发控制 (concurrency/)
- `task_scheduler_demo.cn` - 任务调度器（16KB）
- `atomic_basic_test.cn` - 原子操作
- `mutex_basic_test.cn` - 互斥锁
- `spinlock_test.cn` - 自旋锁
- `rwlock_test.cn` - 读写锁

#### 内联汇编 (inline-asm/)
- `inline_asm_basic_test.cn` - 基础内联汇编
- `inline_asm_advanced_test.cn` - 高级内联汇编

#### 位操作 (bitwise/)
- `bitwise_basic_test.cn` - 位操作基础
- `bitwise_advanced_test.cn` - 位操作高级

### 4. 操作系统内核 (os-kernel/)

**阶段8重大成就** - CN Language首次成功生成真正的ELF内核！

- `os_kernel_demo.cn` - 完整内核演示（363行）
- `os_kernel_simple.cn` - 简化内核
- `boot_kernel_demo.c` - x86_64启动代码（231行）
- `build_os_kernel.ps1` - 跨平台构建脚本（291行）
- `device_driver_demo.cn` - 设备驱动演示（16KB）
- `freestanding_example.cn` - Freestanding模式示例

**构建内核**：
```bash
cd os-kernel
pwsh build_os_kernel.ps1
```

**WSL2环境下构建ELF内核**：
```bash
wsl bash /mnt/c/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/os/build_kernel_wsl2.sh
```

详见：[docs/KERNEL_DEMO_SUMMARY.md](docs/KERNEL_DEMO_SUMMARY.md)

### 5. 测试用例 (tests/)

#### 模块测试 (modules/)
- 36个模块系统测试文件
- 覆盖模块定义、导入、可见性、初始化等

#### 功能测试 (features/)
- `syntax_verification_test.cn` - 语法验证（12KB）
- `perf_test_large.cn` - 性能测试
- 常量、字符串、导入等功能测试

#### 边界测试 (edge-cases/)
- 初始化边界情况
- 可见性边界测试
- 表达式嵌套测试

### 6. 文档 (docs/)

- `KERNEL_DEMO_SUMMARY.md` - 内核演示总结（v2.0）
- `OS_KERNEL_README.md` - OS内核说明
- `QEMU_TESTING_GUIDE.md` - QEMU测试指南

## 📊 统计信息

| 类别 | 文件数 | 说明 |
|------|--------|------|
| 基础示例 | 5 | 入门级示例 |
| 语法特性 | 50+ | 覆盖所有核心语法 |
| 系统编程 | 20+ | 内存、并发、I/O等 |
| OS内核 | 12 | 完整内核实现 |
| 测试用例 | 50+ | 全面的测试覆盖 |
| **总计** | **140+** | 完整的示例库 |

## 🎯 推荐学习路径

1. **入门** → `basic/hello_world.cn`
2. **语法** → `syntax/functions/function_examples.cn`
3. **数组** → `syntax/arrays/array_examples.cn`
4. **指针** → `syntax/pointers/pointer_example.cn`
5. **结构体** → `syntax/structs/struct_complete_example.cn`
6. **系统编程** → `system/memory/memory_manager_demo.cn`
7. **并发** → `system/concurrency/task_scheduler_demo.cn`
8. **操作系统** → `os-kernel/os_kernel_demo.cn`

## 🔧 编译和运行

### 编译单个文件
```bash
cnc <文件路径> -o <输出文件>
```

### 编译并运行
```bash
cnc <文件路径> -o <输出文件> && ./<输出文件>
```

### 查看生成的C代码
```bash
cnc <文件路径> --emit-c -o <输出.c>
```

### Freestanding模式（无OS依赖）
```bash
cnc <文件路径> --freestanding -o <输出>
```

## 📖 相关文档

- [CN Language 语法标准](../docs/design/CN_Language%20语法标准.md)
- [阶段8 TODO列表](../docs/implementation-plans/阶段%208/阶段%208%20TODO%20列表.md)
- [测试规范](../docs/specifications/CN_Language%20测试规范.md)
- [C代码风格规范](../docs/specifications/CN_Language%20C%20代码风格规范.md)

## 🎉 重大里程碑

### 阶段8成就（2026-01-26）

- ✅ **首次生成真正的ELF可执行文件**
- ✅ **首次实现完整的freestanding模式**
- ✅ **首次证明可以编写操作系统内核**
- ✅ 跨平台构建流程（Windows + WSL2）
- ✅ 自动化测试和构建

详见：[docs/KERNEL_DEMO_SUMMARY.md](docs/KERNEL_DEMO_SUMMARY.md)

---

**最后更新**：2026-01-26  
**版本**：v2.0（目录重组后）  
**维护者**：CN Language项目团队
