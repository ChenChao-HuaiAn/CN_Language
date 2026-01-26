# cnc - CN_Language 编译器

**命令**: `cnc`  
**版本**: 1.0.0  
**用途**: 编译CN Language源文件

---

## 概述

`cnc`是CN_Language的编译器命令行工具，可以将`.cn`源文件编译为可执行程序或C代码。

### 特性

- ✅ 单文件和多文件编译
- ✅ C代码生成后端
- ✅ Freestanding模式（内核开发）
- ✅ 性能分析
- ✅ 详细的错误诊断
- ✅ 多平台支持（Windows/Linux）

---

## 基本用法

### 编译单个文件

```bash
cnc program.cn -o program
```

### 编译多个文件

```bash
cnc main.cn utils.cn helper.cn -o program
```

### 仅生成C代码

```bash
cnc program.cn --emit-c -o program.c
```

---

## 命令行选项

### 基本选项

#### `-o, --output <file>`

指定输出文件名。

```bash
cnc hello.cn -o hello
cnc hello.cn -o hello.exe  # Windows
```

**默认值**: 如果未指定，输出为`a.out`（Linux）或`a.exe`（Windows）

---

#### `--emit-c`

仅生成C代码，不调用C编译器。

```bash
cnc program.cn --emit-c -o program.c
```

**用途**:
- 查看生成的C代码
- 手动控制C编译过程
- 调试代码生成

---

#### `-h, --help`

显示帮助信息。

```bash
cnc --help
```

---

#### `-v, --version`

显示编译器版本。

```bash
cnc --version
# 输出: CN_Language Compiler v1.0.0
```

---

### 优化选项

#### `-O, --optimize <level>`

设置优化级别（0-3）。

```bash
cnc program.cn -O0 -o program  # 无优化（快速编译）
cnc program.cn -O1 -o program  # 基本优化（默认）
cnc program.cn -O2 -o program  # 标准优化
cnc program.cn -O3 -o program  # 激进优化
```

**推荐**:
- 开发阶段: `-O0`或`-O1`
- 生产环境: `-O2`或`-O3`

---

### Freestanding模式

#### `--freestanding`

启用freestanding模式，用于内核和裸机开发。

```bash
cnc kernel.cn --freestanding -o kernel.elf
```

**效果**:
- 禁止使用标准库函数
- 不链接标准C运行时
- 检查是否使用了依赖OS的特性

**示例**:
```bash
# 编译操作系统内核
cnc os_kernel.cn \
    --freestanding \
    -O2 \
    -o kernel.elf

# 配合链接器脚本
cnc kernel.cn \
    --freestanding \
    --linker-script=kernel.ld \
    -o kernel.elf
```

详见: [examples/os-kernel](../../examples/os-kernel/)

---

### 诊断选项

#### `--verbose`

显示详细的编译过程信息。

```bash
cnc program.cn --verbose -o program
```

输出包括：
- 编译各阶段耗时
- 中间文件生成
- C编译器调用命令

---

#### `--perf-json <file>`

输出性能分析数据（JSON格式）。

```bash
cnc program.cn --perf-json=perf.json -o program
```

**perf.json内容**:
```json
{
  "total_duration_ms": 25.4,
  "lexer_duration_ms": 3.2,
  "parser_duration_ms": 8.1,
  "semantic_duration_ms": 6.5,
  "codegen_duration_ms": 7.6,
  "stages": [
    {
      "name": "lexer",
      "start_ms": 0.0,
      "end_ms": 3.2,
      "duration_ms": 3.2
    },
    ...
  ]
}
```

---

#### `--dump-ast`

输出抽象语法树（AST）。

```bash
cnc program.cn --dump-ast -o program
```

**用途**: 调试编译器前端

---

#### `--dump-ir`

输出中间表示（IR）。

```bash
cnc program.cn --dump-ir -o program
```

**用途**: 调试代码生成和优化

---

### 目标平台选项

#### `--target <triple>`

指定目标三元组。

```bash
cnc program.cn --target=x86_64-pc-linux-sysv -o program
cnc program.cn --target=x86_64-pc-windows-msvc -o program.exe
```

**支持的目标**:
- `x86_64-pc-linux-sysv` - Linux x86_64
- `x86_64-pc-windows-msvc` - Windows x86_64 (MSVC ABI)
- `x86_64-unknown-linux-freestanding` - Freestanding x86_64

---

#### `--cc <compiler>`

指定C编译器。

```bash
cnc program.cn --cc=gcc -o program
cnc program.cn --cc=clang -o program
cnc program.cn --cc=cl -o program  # MSVC
```

**默认值**:
- Linux: `gcc`
- Windows: `gcc`或`cl`（如果可用）

---

### 链接选项

#### `--static`

生成静态链接的可执行文件。

```bash
cnc program.cn --static -o program
```

---

#### `-L <dir>`

添加库搜索路径。

```bash
cnc program.cn -L/usr/local/lib -lmylib -o program
```

---

#### `-l <library>`

链接指定库。

```bash
cnc program.cn -lm -lpthread -o program
```

---

## 使用场景

### 场景1: 快速开发测试

```bash
# 快速编译运行
cnc test.cn -o test && ./test

# 或使用cnrepl交互式测试
cnrepl
```

### 场景2: 生产环境构建

```bash
# 优化编译
cnc main.cn module1.cn module2.cn \
    -O3 \
    --static \
    -o production_app
```

### 场景3: 查看生成的C代码

```bash
# 生成C代码
cnc program.cn --emit-c -o program.c

# 手动编译C代码
gcc program.c -o program
```

### 场景4: 内核开发

```bash
# 编译内核
cnc kernel.cn \
    --freestanding \
    --target=x86_64-unknown-linux-freestanding \
    -O2 \
    -o kernel.elf

# 配合启动代码和链接脚本
gcc -c boot.S -o boot.o
ld -T kernel.ld boot.o kernel.o -o kernel.elf
```

### 场景5: 性能分析

```bash
# 记录编译性能
cnc large_project.cn \
    --perf-json=build_perf.json \
    --verbose \
    -O2 \
    -o large_project

# 分析性能数据
cat build_perf.json
```

---

## 错误处理

### 编译错误

编译器会输出详细的错误信息：

```
错误: 未定义的变量 'x'
  --> program.cn:5:12
   |
 5 |     打印("%d\n", x);
   |                ^
   |
帮助: 变量 'x' 未声明，是否忘记声明？
```

### 警告

可以控制警告级别：

```bash
# 将警告视为错误
cnc program.cn -Werror -o program

# 禁用特定警告
cnc program.cn -Wno-unused-variable -o program
```

---

## 环境变量

### `CNLANG_CC`

默认C编译器。

```bash
export CNLANG_CC=clang
cnc program.cn -o program  # 使用clang
```

### `CNLANG_CFLAGS`

传递给C编译器的额外标志。

```bash
export CNLANG_CFLAGS="-g -Wall"
cnc program.cn -o program
```

### `CNLANG_HOME`

CN_Language安装目录。

```bash
export CNLANG_HOME=/opt/cnlang
cnc program.cn -o program
```

---

## 配置文件

### `.cnconfig`

项目根目录下的配置文件：

```toml
[build]
optimize_level = 2
target = "x86_64-pc-linux-sysv"
cc = "gcc"

[freestanding]
enabled = false

[diagnostics]
verbose = false
dump_ast = false
```

---

## 集成构建系统

### Makefile示例

```makefile
CC = cnc
CFLAGS = -O2
SOURCES = main.cn utils.cn
TARGET = myapp

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
```

### CMake示例

```cmake
# 定义CN_Language编译规则
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/program.c
    COMMAND cnc ${CMAKE_SOURCE_DIR}/program.cn 
            --emit-c 
            -o ${CMAKE_BINARY_DIR}/program.c
    DEPENDS ${CMAKE_SOURCE_DIR}/program.cn
)

add_executable(program ${CMAKE_BINARY_DIR}/program.c)
```

---

## 性能建议

### 提高编译速度

1. **并行编译**（计划中）:
   ```bash
   cnc --jobs=4 *.cn -o program
   ```

2. **增量编译**（计划中）:
   ```bash
   cnc --incremental *.cn -o program
   ```

3. **降低优化级别**（开发阶段）:
   ```bash
   cnc -O0 program.cn -o program
   ```

### 提高运行性能

1. **启用优化**:
   ```bash
   cnc -O3 program.cn -o program
   ```

2. **使用静态链接**（减少动态链接开销）:
   ```bash
   cnc --static program.cn -o program
   ```

3. **分析瓶颈**:
   ```bash
   cnc --perf-json=perf.json program.cn -o program
   cnperf analyze perf.json
   ```

---

## 相关工具

- [cnrepl](cnrepl.md) - 交互式解释器
- [cnfmt](cnfmt.md) - 代码格式化
- [cncheck](cncheck.md) - 静态检查
- [cnlsp](cnlsp.md) - 语言服务器
- [cnperf](cnperf.md) - 性能分析

---

## 示例

### 完整编译流程示例

```bash
# 1. 格式化代码
cnfmt --check src/*.cn

# 2. 静态检查
cncheck src/*.cn

# 3. 编译
cnc src/main.cn src/utils.cn \
    -O2 \
    --verbose \
    -o myapp

# 4. 运行
./myapp

# 5. 性能分析
cnc src/main.cn \
    --perf-json=perf.json \
    -O2 \
    -o myapp
cnperf analyze perf.json
```

---

## 故障排除

### 问题: 找不到cnc命令

**解决**:
```bash
# 检查安装
which cnc

# 添加到PATH
export PATH=$PATH:/path/to/cnlang/bin
```

### 问题: C编译器错误

**解决**:
```bash
# 检查C代码
cnc program.cn --emit-c -o program.c
gcc -c program.c -o program.o

# 指定其他C编译器
cnc program.cn --cc=clang -o program
```

### 问题: Freestanding模式链接失败

**解决**:
```bash
# 使用自定义链接脚本
cnc kernel.cn \
    --freestanding \
    --linker-script=kernel.ld \
    -o kernel.elf
```

---

**文档版本**: 1.0  
**最后更新**: 2026-01-26  
**维护者**: CN_Language开发团队
