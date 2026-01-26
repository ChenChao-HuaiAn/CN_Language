# CN Language 内核 QEMU 测试指南

## 概述

本文档说明如何在QEMU中验证CN Language操作系统内核的运行。

## Windows环境的特殊说明

### 已安装QEMU的Windows用户

如果您的Windows系统已安装QEMU（如：`C:\Program Files\qemu`），您可以：

1. **将QEMU添加到PATH**
   ```powershell
   setx PATH "%PATH%;C:\Program Files\qemu"
   ```

2. **验证QEMU安装**
   ```powershell
   & "C:\Program Files\qemu\qemu-system-x86_64.exe" --version
   ```

**但请注意**：即使安装了QEMU，Windows MinGW环境仍然无法生成真正的ELF格式内核。要进行完整测试，您必须：
- 使用WSL2环境
- 或使用交叉编译工具链（如x86_64-elf-gcc）

### 为什么Windows上不能直接生成ELF内核？

1. **MinGW限制**：Windows的MinGW GCC默认生成PE/COFF格式，不支持裸机ELF格式
2. **链接器限制**：需要特殊的链接脚本和freestanding支持
3. **运行时差异**：Freestanding内核需要不依赖Windows C运行时

### 解决方案对比

| 方案 | 编译CN代码 | 生成ELF | QEMU运行 | 难度 |
|------|-----------|---------|----------|------|
| WSL2 | ✓ | ✓ | ✓ | 简单 |
| 交叉编译 | ✓ | ✓ | ✓ | 复杂 |
| 纯Windows | ✓ | ✗ | ✗ | N/A |

**推荐**：使用WSL2是最简单可靠的方案。

---

### Windows用户

**重要说明**：虽然Windows可以安装QEMU，但MinGW GCC不支持生成真正的x86_64 ELF格式内核。Windows环境只能进行编译验证，无法生成可在QEMU中运行的内核镜像。

**推荐方案**：使用WSL2（最佳选择）

#### 方案1：使用WSL2（推荐）

1. **安装WSL2**
   ```powershell
   wsl --install
   ```

2. **安装Ubuntu**
   ```powershell
   wsl --install -d Ubuntu
   ```

3. **在WSL2中安装工具**
   ```bash
   sudo apt update
   sudo apt install -y build-essential qemu-system-x86 powershell
   ```

#### 方案2：使用MSYS2

1. **下载安装MSYS2**
   - 访问 https://www.msys2.org/
   - 下载并安装

2. **安装工具链**
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-qemu
   ```

**注意**：根据记忆经验，MSYS2环境下ELF内核构建存在限制，WSL2是更好的选择。

### Linux用户

直接安装所需工具：

```bash
# Debian/Ubuntu
sudo apt install -y build-essential qemu-system-x86

# Fedora/RHEL
sudo dnf install -y gcc qemu-system-x86

# Arch Linux
sudo pacman -S gcc qemu-system-x86
```

## 构建和测试流程

### 1. 编译内核

#### 在WSL2/Linux环境

```bash
cd examples
pwsh build_os_kernel.ps1 -Build
```

#### 仅Windows编译验证（不生成ELF）

```powershell
cd examples
pwsh build_os_kernel.ps1
```

### 2. 在QEMU中运行

#### 方法1：使用构建脚本（推荐）

```bash
cd examples
pwsh build_os_kernel.ps1 -Build -RunQemu
```

#### 方法2：手动运行QEMU

```bash
qemu-system-x86_64 \
  -kernel examples/os_kernel_demo.elf \
  -nographic \
  -serial stdio \
  -no-reboot
```

**参数说明**：
- `-kernel`: 指定内核镜像文件
- `-nographic`: 无图形界面模式
- `-serial stdio`: 串口重定向到标准输入输出
- `-no-reboot`: 内核停止后不自动重启

### 3. 退出QEMU

按 `Ctrl+A` 然后按 `X`

## 使用集成测试框架

项目提供了完整的集成测试框架：

### 编译测试程序

```bash
cd build
cmake ..
make integration_os_test
```

### 运行测试

```bash
cd tests/integration/os
./integration_os_test
```

测试包括：
1. ✓ 最小内核编译
2. ✓ 带输出验证的内核（需要QEMU）
3. ✓ Hello Kernel示例（阶段5验收）
4. ✓ OS Kernel Demo示例（阶段8验收）

## 预期输出

成功运行时，QEMU应显示类似输出：

```
====================================================
[BOOT] CN Language OS 内核启动中...
====================================================
[BOOT] 架构: x86_64
[BOOT] 模式: Freestanding
[BOOT] 栈大小: 32KB
[BOOT] 初始化栈空间...
[BOOT] 栈初始化完成
[BOOT] 初始化 CN 运行时...
[BOOT] CN 运行时初始化完成
[BOOT] 注册中断处理程序...
[BOOT] 中断处理程序注册完成
[BOOT] 调用内核主函数...

====================================================
   CN Language 操作系统内核演示 v1.0
====================================================
  系统编程能力验收示例
  包含: 内存管理 | 中断处理 | 任务调度 | 设备驱动
====================================================

========== 内核初始化 ==========
[INIT] CN Language OS 内核启动中...
[MEMORY] 初始化内存管理器...
[INTERRUPT] 初始化中断系统...
[DEVICE] 初始化设备驱动...
[SCHEDULER] 初始化任务调度器...
[INIT] 内核初始化完成

========== 运行内核测试 ==========
[TEST] 开始运行内核功能测试...
...

[BOOT] 内核主函数返回，返回码: 0
[BOOT] 内核执行成功
[BOOT] 系统停机（HLT）
```

## 故障排除

### 问题1：CN编译器解析失败

**症状**：
```
解析失败
错误(3): ...
```

**原因**：当前CN编译器对复杂内核代码的支持有限

**解决**：
1. 使用简化版本测试：`examples/os_kernel_simple.cn`
2. 在Linux/WSL2环境下测试
3. 等待编译器功能增强

### 问题2：Windows上无法生成ELF

**症状**：
```
[跳过] Windows 上无法生成 ELF 格式
```

**解决**：
- 使用WSL2或Linux环境
- 或安装x86_64-elf-gcc交叉编译器

### 问题3：QEMU未找到

**症状**：
```
[跳过] QEMU 未安装
```

**解决**：
```bash
# WSL2/Ubuntu
sudo apt install qemu-system-x86

# 验证安装
which qemu-system-x86_64
```

### 问题4：内核启动后无输出

**原因**：I/O回调未正确注册

**解决**：
1. 确保使用正确的启动代码（boot_kernel_demo.c）
2. 验证cn_rt_kernel_print回调已实现
3. 检查串口初始化代码

根据内核I/O回调系统的约束：
- **必须在`cn_rt_init()`之前注册I/O回调**
- `putchar_func`为必填项
- `puts_func`可选，为NULL时自动回退

## 性能测试

### 启动时间测试

```bash
time qemu-system-x86_64 -kernel os_kernel_demo.elf -nographic -serial stdio -no-reboot
```

### 内存使用测试

在QEMU中使用`-m`参数限制内存：

```bash
qemu-system-x86_64 \
  -kernel os_kernel_demo.elf \
  -m 64M \
  -nographic \
  -serial stdio
```

## 自动化测试

项目提供了PowerShell脚本用于自动化QEMU测试：

```bash
cd tests/integration/os
pwsh scripts/run_qemu_test.ps1 \
  -KernelImage test_kernel.elf \
  -TimeoutSeconds 5 \
  -ExpectedOutput "Kernel Ready"
```

## 调试技巧

### 1. 启用QEMU日志

```bash
qemu-system-x86_64 \
  -kernel os_kernel_demo.elf \
  -nographic \
  -serial stdio \
  -d int,cpu_reset \
  -D qemu.log
```

### 2. 使用GDB调试

```bash
# 终端1：启动QEMU（等待GDB连接）
qemu-system-x86_64 \
  -kernel os_kernel_demo.elf \
  -s -S \
  -nographic

# 终端2：启动GDB
gdb os_kernel_demo.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

### 3. 查看内存映射

在内核代码中添加调试输出，或使用QEMU监控：

```bash
qemu-system-x86_64 \
  -kernel os_kernel_demo.elf \
  -monitor stdio
```

## 参考资料

- QEMU文档：https://www.qemu.org/docs/master/
- CN Language 语法标准：`docs/design/CN_Language 语法标准.md`
- Freestanding规范：`docs/specifications/CN_Language 运行时绑定规范.md`
- OS集成测试：`tests/integration/os/README.md`

---

**最后更新**：2026-01-26  
**状态**：Windows环境需WSL2/Linux完整测试
