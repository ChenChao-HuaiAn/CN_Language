# CN语言自举测试

## 目录

- [概述](#概述)
- [自举测试原理](#自举测试原理)
- [使用方法](#使用方法)
- [预期结果](#预期结果)
- [故障排查](#故障排查)
- [配置说明](#配置说明)

---

## 概述

### 什么是自举测试？

**自举测试（Bootstrap Testing）** 是验证编译器能够正确编译自身源代码的测试过程。这是编译器实现自托管的关键验证步骤，也是语言成熟度的重要标志。

### 为什么需要自举测试？

| 意义 | 说明 |
|------|------|
| **语言完整性验证** | 证明CN语言具备实现编译器的全部能力 |
| **编译器正确性验证** | 三阶段验证确保编译器逻辑正确 |
| **生态成熟度标志** | 自举是语言成熟的里程碑 |
| **开发者信心保障** | 通过自举的编译器更可靠 |

---

## 自举测试原理

### 三阶段自举验证

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        三阶段自举验证原理                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   阶段1: cnc.exe (C实现) ──编译──> cn_v1.exe                           │
│                                     │                                   │
│   阶段2: cn_v1.exe ──编译──> cn_v2.exe                                 │
│                           │                                             │
│   阶段3: cn_v2.exe ──编译──> cn_v3.exe                                 │
│                           │                                             │
│   验证: cn_v2.exe 与 cn_v3.exe 二进制完全一致                          │
│                                                                         │
│   原理: 如果编译器正确，用它编译自身应该产生相同的输出                  │
│         任何编译器bug都会导致v2和v3不同                                 │
└─────────────────────────────────────────────────────────────────────────┘
```

### 为什么三阶段验证有效？

1. **阶段1**：使用C实现的编译器（cnc.exe）编译CN源码，生成第一个自编译版本（cn_v1.exe）
2. **阶段2**：使用cn_v1.exe再次编译CN源码，生成cn_v2.exe
3. **阶段3**：使用cn_v2.exe第三次编译CN源码，生成cn_v3.exe
4. **验证**：如果编译器逻辑正确，cn_v2.exe和cn_v3.exe应该完全相同

**核心原理**：一个正确的编译器，无论用哪个版本编译相同的源码，都应该产生相同的输出。如果v2和v3不同，说明编译器存在不确定性或bug。

---

## 使用方法

### 前置条件

| 要求项 | 说明 |
|--------|------|
| C编译器 | GCC 9+ 或 MSVC 2019+ |
| CMake | 3.16+ |
| 引导编译器 | build/cnc.exe 已构建 |
| 源码完整 | src_cn 目录包含所有CN源码 |

### 快速开始

#### Windows (PowerShell)

```powershell
# 基本用法
./tools/bootstrap_test.ps1

# 详细输出模式
./tools/bootstrap_test.ps1 -Verbose

# 保留所有构建产物
./tools/bootstrap_test.ps1 -KeepArtifacts

# 使用配置文件
./tools/bootstrap_test.ps1 -Config ./tools/bootstrap_config.json
```

#### Linux (Bash)

```bash
# 基本用法
./tools/bootstrap_test.sh

# 详细输出模式
./tools/bootstrap_test.sh -v

# 保留所有构建产物
./tools/bootstrap_test.sh -k
```

### 命令行参数

| 参数 | 说明 |
|------|------|
| `-Verbose` | 显示详细输出信息 |
| `-KeepArtifacts` | 保留所有中间构建产物 |
| `-Config <路径>` | 指定配置文件路径 |
| `-CompilerPath <路径>` | 覆盖编译器路径 |
| `-SourceDir <路径>` | 覆盖源码目录 |
| `-OutputDir <路径>` | 覆盖输出目录 |

### 使用CMake

```bash
# 构建并运行自举测试
cmake --build build --target bootstrap_test

# Windows版本
cmake --build build --target bootstrap_test_win
```

---

## 预期结果

### 成功场景

```
=== CN语言自举测试 ===
[INFO] 项目根目录: /path/to/CN_Language
[INFO] 构建目录: /path/to/CN_Language/build/bootstrap_test

=== 检查前置条件 ===
[PASS] 引导编译器: /path/to/CN_Language/build/cnc.exe
[PASS] src_cn目录: /path/to/CN_Language/src_cn (35 个.cn文件)
[PASS] C编译器: gcc (GCC) 12.2.0

=== 准备构建环境 ===
[PASS] 构建目录已准备完成

=== 阶段1: 引导编译 (cnc.exe -> cn_v1.exe) ===
[INFO] [阶段1] 使用 cnc.exe 编译 src_cn...
[PASS] [阶段1] 编译成功
[INFO] [阶段1] 输出: cn_v1.exe (2.5 MB)
[INFO] [阶段1] SHA256: abc123def456...

=== 阶段2: 首次自举 (cn_v1.exe -> cn_v2.exe) ===
[INFO] [阶段2] 使用 cn_v1.exe 编译 src_cn...
[PASS] [阶段2] 编译成功
[INFO] [阶段2] 输出: cn_v2.exe (2.5 MB)
[INFO] [阶段2] SHA256: def456ghi789...

=== 阶段3: 二次自举 (cn_v2.exe -> cn_v3.exe) ===
[INFO] [阶段3] 使用 cn_v2.exe 编译 src_cn...
[PASS] [阶段3] 编译成功
[INFO] [阶段3] 输出: cn_v3.exe (2.5 MB)
[INFO] [阶段3] SHA256: def456ghi789...

=== 验证阶段 ===
[INFO] 比较生成的C代码 (v2 vs v3)...
[PASS] C代码完全一致
[INFO] 比较 cn_v2.exe 与 cn_v3.exe...
[PASS] cn_v2.exe 与 cn_v3.exe 完全一致

=== 测试结果 ===
[PASS] ✅ 自举验证通过！
[PASS] cn_v2.exe 与 cn_v3.exe 完全一致
[INFO] 哈希值: def456ghi789...
[INFO] 最终编译器已复制到: build/cn.exe
```

### 失败场景

```
=== 验证阶段 ===
[INFO] 比较生成的C代码 (v2 vs v3)...
[WARN] C代码存在差异
[INFO] 比较 cn_v2.exe 与 cn_v3.exe...
[FAIL] cn_v2.exe 与 cn_v3.exe 不一致
[INFO] v2 哈希: def456ghi789...
[INFO] v3 哈希: ghi789jkl012...

=== 测试结果 ===
[FAIL] ❌ 自举验证失败！
[FAIL] cn_v2.exe 与 cn_v3.exe 不一致
[INFO] 差异报告已保存到: build/bootstrap_test/bootstrap_report.txt
```

---

## 故障排查

### 常见问题

#### 1. 未找到引导编译器

**错误信息**：
```
[FAIL] 未找到引导编译器: build/cnc.exe
```

**解决方案**：
```bash
# 先构建C实现的编译器
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### 2. src_cn目录不存在

**错误信息**：
```
[FAIL] 未找到src_cn目录: src_cn
```

**解决方案**：
确保项目完整克隆，src_cn目录应包含CN语言源码。

#### 3. C编译器未找到

**错误信息**：
```
[FAIL] 未找到C编译器: gcc
```

**解决方案**：
- Windows: 安装MinGW或MSVC
- Linux: `sudo apt install gcc`

#### 4. 编译失败

**错误信息**：
```
[FAIL] [阶段1] 生成C代码失败
```

**排查步骤**：
1. 使用 `-Verbose` 参数查看详细错误
2. 检查CN源码是否有语法错误
3. 检查模块导入是否正常

#### 5. 自举验证失败

**错误信息**：
```
[FAIL] ❌ 自举验证失败！
[FAIL] cn_v2.exe 与 cn_v3.exe 不一致
```

**排查步骤**：

1. **查看差异报告**
   ```bash
   cat build/bootstrap_test/bootstrap_report.txt
   ```

2. **比较生成的C代码**
   ```bash
   # Windows
   fc build/bootstrap_test/v2/xxx.c build/bootstrap_test/v3/xxx.c
   
   # Linux
   diff build/bootstrap_test/v2/xxx.c build/bootstrap_test/v3/xxx.c
   ```

3. **常见原因分析**

   | 原因 | 症状 | 解决方案 |
   |------|------|---------|
   | 非确定性输出 | 时间戳、随机数 | 移除非确定性代码 |
   | 未初始化变量 | 每次运行结果不同 | 初始化所有变量 |
   | 内存问题 | 随机崩溃 | 使用内存检测工具 |
   | 编译器bug | 特定代码段差异 | 定位并修复bug |

### 差异报告分析

差异报告位于 `build/bootstrap_test/bootstrap_report.txt`，包含：

```
========================================
CN语言自举测试报告
========================================

测试时间: 2026-04-11 20:00:00
测试环境: Windows 11 / GCC 12.2.0

----------------------------------------
编译器版本
----------------------------------------
引导编译器: cnc.exe (C实现)
CN编译器v1: cn_v1.exe (自编译)
CN编译器v2: cn_v2.exe (自举1)
CN编译器v3: cn_v3.exe (自举2)

----------------------------------------
哈希值
----------------------------------------
cn_v1.exe: abc123...
cn_v2.exe: def456...
cn_v3.exe: ghi789...  # 与v2不同

----------------------------------------
验证结果
----------------------------------------
C代码一致性: ❌ 失败
二进制一致性: ❌ 失败

========================================
结论: ❌ 自举验证失败
========================================
```

---

## 配置说明

### 配置文件格式

配置文件 `tools/bootstrap_config.json`：

```json
{
    "编译器路径": "build/cnc.exe",
    "源码目录": "src_cn",
    "输出目录": "build/bootstrap_test",
    
    "编译选项": ["-O2"],
    "包含目录": "include",
    "库目录": "build/lib",
    "库名称": "cnrt",
    
    "超时秒数": 300,
    "C编译器": "gcc",
    
    "验证选项": {
        "比较C代码": true,
        "比较二进制": true,
        "生成差异报告": true
    },
    
    "输出选项": {
        "详细日志": false,
        "保留中间文件": false,
        "生成时间戳文件": true
    }
}
```

### 配置项说明

| 配置项 | 类型 | 说明 |
|--------|------|------|
| 编译器路径 | string | 引导编译器路径 |
| 源码目录 | string | CN源码目录 |
| 输出目录 | string | 测试输出目录 |
| 编译选项 | array | GCC编译选项 |
| 包含目录 | string | 头文件目录 |
| 库目录 | string | 库文件目录 |
| 库名称 | string | 链接库名称 |
| 超时秒数 | number | 单阶段超时时间 |
| C编译器 | string | C编译器命令 |

---

## 相关文档

- [`plans/015 CN语言自举测试实现方案.md`](../../plans/015%20CN语言自举测试实现方案.md) - 详细设计文档
- [`plans/014 CN语言自托管编译器技术设计文档.md`](../../plans/014%20CN语言自托管编译器技术设计文档.md) - 自托管编译器设计

---

## 更新历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-04-11 | 初始版本 |
