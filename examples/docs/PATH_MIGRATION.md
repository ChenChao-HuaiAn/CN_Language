# Examples 目录重组后的路径变更说明

## 变更日期
2026-01-26

## 变更原因
为了更好地组织examples目录，将所有示例文件按功能分类到子目录中。

## 主要变更

### 1. OS内核相关文件
**旧位置**: `examples/`  
**新位置**: `examples/os-kernel/`

变更的文件：
- `os_kernel_demo.cn` → `os-kernel/os_kernel_demo.cn`
- `os_kernel_simple.cn` → `os-kernel/os_kernel_simple.cn`
- `boot_kernel_demo.c` → `os-kernel/boot_kernel_demo.c`
- `build_os_kernel.ps1` → `os-kernel/build_os_kernel.ps1`
- `device_driver_demo.cn` → `os-kernel/device_driver_demo.cn`
- `freestanding_example.cn` → `os-kernel/freestanding_example.cn`
- 其他内核相关文件...

### 2. 文档文件
**旧位置**: `examples/`  
**新位置**: `examples/docs/`

变更的文件：
- `KERNEL_DEMO_SUMMARY.md` → `docs/KERNEL_DEMO_SUMMARY.md`
- `OS_KERNEL_README.md` → `docs/OS_KERNEL_README.md`
- `QEMU_TESTING_GUIDE.md` → `docs/QEMU_TESTING_GUIDE.md`

### 3. 其他分类
详见 `examples/README.md` 中的完整目录结构说明。

## 使用影响

### 构建命令更新

#### 旧方式：
```bash
cd examples
pwsh build_os_kernel.ps1
```

#### 新方式：
```bash
cd examples/os-kernel
pwsh build_os_kernel.ps1
```

### 文档路径更新

所有文档内部的相对路径引用已自动更新：
- ✅ `build_os_kernel.ps1` 中的路径变量已更新
- ✅ `KERNEL_DEMO_SUMMARY.md` 中的链接已更新
- ✅ `README.md` 中的示例路径已更新

### WSL2构建脚本

WSL2构建脚本位置**未变更**：
```bash
# 仍然在原位置
tests/integration/os/build_kernel_wsl2.sh
```

此脚本用于构建测试用的简单内核（hello_kernel），与examples中的完整演示内核分开。

## 路径映射表

| 功能 | 旧路径 | 新路径 |
|------|--------|--------|
| 内核演示 | `examples/*.cn` | `examples/os-kernel/*.cn` |
| 构建脚本 | `examples/build_os_kernel.ps1` | `examples/os-kernel/build_os_kernel.ps1` |
| 文档 | `examples/*.md` | `examples/docs/*.md` |
| 基础示例 | `examples/hello_world.cn` | `examples/basic/hello_world.cn` |
| 语法示例 | `examples/array_*.cn` | `examples/syntax/arrays/*.cn` |
| 系统编程 | `examples/memory_*.cn` | `examples/system/memory/*.cn` |
| 测试用例 | `examples/test_*.cn` | `examples/tests/*/test_*.cn` |

## 需要更新的外部引用

如果您在其他地方引用了examples目录下的文件，请检查以下位置：

1. **测试脚本** - 检查 `tests/` 目录下的测试脚本
2. **CI/CD配置** - 检查构建配置文件
3. **IDE配置** - 更新项目文件路径
4. **自定义脚本** - 检查您自己编写的脚本

## 验证变更

运行以下命令验证新结构：

```bash
# 查看新目录结构
tree examples

# 测试构建（Windows）
cd examples/os-kernel
pwsh build_os_kernel.ps1

# 测试构建（Linux/WSL2）
wsl bash /mnt/c/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/os/build_kernel_wsl2.sh
```

## 回滚方法

如果需要回滚到旧结构，可以：

1. 备份数据后，将子目录中的文件移回examples根目录
2. 恢复旧版本的文档和脚本（从git历史）

不过**强烈建议使用新结构**，因为它更清晰、更易维护。

---

**注意**: 所有路径已在相关文件中自动更新，正常使用不需要手动修改任何配置。
