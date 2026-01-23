# OS 集成测试目录结构

此目录包含 CN Language 编译器的 OS 开发场景集成测试。

## 目录说明

- **boot/** - 启动代码和链接脚本
  - `boot.c` - x86_64 基本启动代码
  - `boot_with_io.c` - 带串口 I/O 回调的启动代码
  - `boot_test.c` - 带输出验证的测试启动代码
  - `linker.ld` - 内核链接脚本
  
- **kernels/** - 内核示例程序
  - `minimal_kernel.cn` - 最小内核示例
  - `kernel_with_io.cn` - 带 I/O 回调的内核示例
  - `test_kernel.cn` - 用于自动化测试的内核（返回 42）
  
- **scripts/** - 构建和测试脚本
  - `build_kernel.ps1` - PowerShell 构建脚本
  - `run_qemu_test.ps1` - QEMU 测试脚本
  - `verify_boot.ps1` - 启动代码与链接脚本验证

- **os_integration_test.c** - C 驱动程序（自动化测试）

## 内核 I/O 回调使用说明

### 1. 基本概念

CN Language 在 freestanding 模式下支持通过回调函数实现 I/O 操作。这使得 CN Language 的标准函数（如 `打印`）能够在没有操作系统的裸机环境中工作。

### 2. 需要实现的回调函数

在启动代码中实现以下函数：

```c
// 打印字符串
void cn_rt_kernel_print(const char *str);

// 输出单个字符
void cn_rt_kernel_putchar(int ch);
```

### 3. 示例：串口输出

`boot/boot_with_io.c` 提供了一个完整的例子：

```c
// x86_64 串口端口（COM1 = 0x3F8）
#define SERIAL_PORT 0x3F8

// 向串口写入一个字节
static inline void serial_write_byte(uint8_t byte) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(byte), "Nd"((uint16_t)SERIAL_PORT)
    );
}

// 实现 CN Runtime 回调
void cn_rt_kernel_print(const char *str) {
    if (!str) return;
    while (*str) {
        serial_write_byte((uint8_t)*str);
        str++;
    }
}

void cn_rt_kernel_putchar(int ch) {
    serial_write_byte((uint8_t)ch);
}
```

### 4. 其他平台支持

针对不同的平台，你可以实现不同的 I/O 机制：

- **x86_64**: 串口、VGA 文本模式
- **ARM**: UART、平台特定 I/O
- **RISC-V**: SBI 接口、UART

## 测试流程

### 快速验证

```powershell
# 使用验证脚本快速检查整个流程
cd tests/integration/os
pwsh scripts/verify_boot.ps1

# 如果安装了 QEMU，可以启用 QEMU 测试
pwsh scripts/verify_boot.ps1 -RunQemu
```

### Windows （当前平台）

```powershell
# 编译验证（不生成 ELF 镜像）
cd tests/integration/os
../../../build/src/cnc.exe kernels/minimal_kernel.cn --freestanding -c
gcc -c boot/boot.c -o boot.o -ffreestanding -nostdlib
gcc -c kernels/minimal_kernel.c -o kernel.o -ffreestanding -nostdlib
```

### Linux/macOS

```bash
# 完整流程（生成 ELF 镜像）
cd tests/integration/os
pwsh scripts/build_kernel.ps1 kernels/minimal_kernel.cn test_kernel.elf
pwsh scripts/run_qemu_test.ps1 test_kernel.elf
```

### QEMU 输出验证

`run_qemu_test.ps1` 脚本支持自动验证内核输出：

```powershell
# 验证串口输出包含特定字符串
pwsh scripts/run_qemu_test.ps1 test_kernel.elf -ExpectedOutput "PASS"

# 检查成功标记（SUCCESS/PASS/OK/[OK]）
pwsh scripts/run_qemu_test.ps1 test_kernel.elf -CheckSuccess
```

**示例：使用测试启动代码**

```powershell
# 构建带输出验证的内核
pwsh scripts/build_kernel.ps1 kernels/test_kernel.cn test.elf -BootCode "boot/boot_test.c"

# 运行并验证输出
pwsh scripts/run_qemu_test.ps1 test.elf -ExpectedOutput "PASS"
```

## 注意事项

1. **Windows 限制**：MinGW gcc 不支持生成 ELF 格式，仅能验证编译
2. **交叉编译**：要在 Windows 上生成内核镜像，需要安装 x86_64-elf-gcc 交叉编译器
3. **QEMU 测试**：需要安装 QEMU 或 Bochs 模拟器
4. **当前限制**：freestanding 模式下禁止使用 `打印` 函数，后续将支持通过回调使用
