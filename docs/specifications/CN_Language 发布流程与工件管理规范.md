# CN_Language 发布流程与工件管理规范

## 1. 概述

本文档定义 CN_Language 编译器、工具链及运行时库的发布流程、工件管理规范和打包标准，确保每次发布的一致性、可追溯性和用户体验。

---

## 2. 发布类型与分类

### 2.1 发布类型

| 发布类型 | 版本标识 | 用途 | 发布频率 |
|---------|---------|------|---------|
| **开发版** (Dev Build) | `0.x.x-alpha.N` | 内部开发验证，功能不完整 | 每日/每周 |
| **预览版** (Preview) | `0.x.x-beta.N` | 公开测试，功能基本完整 | 每月 |
| **候选版** (Release Candidate) | `x.y.z-rc.N` | 正式发布前的最终测试 | 按需 |
| **正式版** (Stable Release) | `x.y.z` | 生产环境使用 | 按计划 |
| **长期支持版** (LTS) | `x.y.z` + LTS 标记 | 长期维护和支持 | 每个 MAJOR 版本一次 |

### 2.2 发布渠道

| 渠道 | 说明 | 目标用户 |
|------|------|---------|
| **Nightly** | 每日自动构建，最新代码 | 开发者、早期测试者 |
| **Beta** | 预览版发布渠道 | 早期采用者、测试用户 |
| **Stable** | 正式版发布渠道 | 生产环境用户 |
| **LTS** | 长期支持版本渠道 | 企业用户、稳定性优先用户 |

---

## 3. 发布工件清单

### 3.1 核心工件

#### 3.1.1 编译器工具链

| 工件名称 | 文件名格式 | 说明 |
|---------|-----------|------|
| **cnc** | `cnc-{version}-{platform}-{arch}[.exe]` | CN_Language 编译器 |
| **cnfmt** | `cnfmt-{version}-{platform}-{arch}[.exe]` | 代码格式化工具 |
| **cncheck** | `cncheck-{version}-{platform}-{arch}[.exe]` | 静态检查工具 |
| **cnlsp** | `cnlsp-{version}-{platform}-{arch}[.exe]` | 语言服务器 |
| **cnrepl** | `cnrepl-{version}-{platform}-{arch}[.exe]` | 交互式解释器 |
| **cnperf** | `cnperf-{version}-{platform}-{arch}[.exe]` | 性能分析工具 |

**示例**：
- `cnc-1.0.0-windows-x64.exe`
- `cnc-1.0.0-linux-x64`
- `cnc-1.0.0-macos-arm64`

#### 3.1.2 运行时库

| 工件名称 | 文件名格式 | 说明 |
|---------|-----------|------|
| **静态运行时库** | `libcnrt-{version}-{platform}-{arch}.a` | 静态链接用运行时 |
| **动态运行时库** | `libcnrt-{version}-{platform}-{arch}.so/.dll/.dylib` | 动态链接用运行时 |
| **Freestanding 运行时** | `libcnrt-freestanding-{version}-{platform}-{arch}.a` | OS 开发专用运行时 |

**示例**：
- `libcnrt-1.0.0-windows-x64.dll`
- `libcnrt-1.0.0-linux-x64.so`
- `libcnrt-freestanding-1.0.0-x64.a`

#### 3.1.3 头文件与开发包

| 工件名称 | 文件名格式 | 说明 |
|---------|-----------|------|
| **运行时头文件** | `cnrt.h` | 运行时库公共接口头文件 |
| **开发包** | `cnlang-dev-{version}-{platform}-{arch}.tar.gz/.zip` | 包含所有头文件和库文件 |

### 3.2 文档工件

| 工件名称 | 文件名格式 | 说明 |
|---------|-----------|------|
| **语言规范** | `language-spec-{version}.pdf` | 语言规范快照 |
| **用户手册** | `user-manual-{version}.pdf` | 用户使用手册 |
| **API 文档** | `api-docs-{version}.tar.gz` | HTML 格式 API 文档 |
| **变更日志** | `CHANGELOG-{version}.md` | 版本变更记录 |

### 3.3 示例与模板

| 工件名称 | 文件名格式 | 说明 |
|---------|-----------|------|
| **示例代码** | `examples-{version}.tar.gz/.zip` | 官方示例代码集合 |
| **项目模板** | `templates-{version}.tar.gz/.zip` | 项目脚手架模板 |

### 3.4 源代码包

| 工件名称 | 文件名格式 | 说明 |
|---------|-----------|------|
| **源代码归档** | `cnlang-{version}-source.tar.gz/.zip` | 完整源代码（含子模块） |

---

## 4. 工件命名规则

### 4.1 命名约定

```
{tool-name}-{version}-{platform}-{arch}[.ext]
```

**组成部分**：
- `{tool-name}` - 工具名称（如 `cnc`、`cnfmt`）
- `{version}` - 版本号（如 `1.0.0`、`1.2.3-beta.1`）
- `{platform}` - 目标平台（如 `windows`、`linux`、`macos`）
- `{arch}` - 目标架构（如 `x64`、`x86`、`arm64`）
- `[.ext]` - 文件扩展名（如 `.exe`、`.tar.gz`）

### 4.2 平台标识符

| 平台 | 标识符 | 说明 |
|------|-------|------|
| **Windows** | `windows` | Windows 7+ |
| **Linux** | `linux` | GNU/Linux |
| **macOS** | `macos` | macOS 10.15+ |
| **FreeBSD** | `freebsd` | FreeBSD |
| **通用** | `any` | 平台无关工件（如纯文本文档） |

### 4.3 架构标识符

| 架构 | 标识符 | 说明 |
|------|-------|------|
| **x86_64** | `x64` | 64 位 x86 架构 |
| **x86** | `x86` | 32 位 x86 架构 |
| **ARM64** | `arm64` | 64 位 ARM 架构 |
| **ARM** | `arm` | 32 位 ARM 架构 |

### 4.4 文件扩展名约定

| 文件类型 | Windows | Linux/macOS |
|---------|---------|-------------|
| **可执行文件** | `.exe` | 无扩展名 |
| **动态库** | `.dll` | `.so` / `.dylib` |
| **静态库** | `.lib` | `.a` |
| **压缩包** | `.zip` | `.tar.gz` |

---

## 5. 发布流程

### 5.1 发布准备阶段

#### 5.1.1 版本规划
1. **确定版本号**：根据 [CN_Language 版本号规范](CN_Language%20版本号规范.md) 确定新版本号
2. **制定发布计划**：明确发布日期、功能范围、测试计划
3. **创建发布分支**：
   ```bash
   git checkout -b release/v1.0.0 develop
   ```

#### 5.1.2 代码冻结
1. **功能冻结**：停止新功能合并，仅接受 Bug 修复
2. **更新版本信息**：
   - 更新 `CMakeLists.txt` 中的 `project(VERSION ...)`
   - 更新 `include/cnlang/support/version.h` 中的版本宏
   - 更新 `CHANGELOG.md`

#### 5.1.3 测试验证
1. **运行完整测试套件**：
   ```bash
   cmake --build build --target all
   ctest --test-dir build --output-on-failure
   ```
2. **执行夜间测试**：
   ```bash
   # Windows
   .\tools\nightly_tests.ps1
   
   # Linux
   bash tools/nightly_tests.sh
   ```
3. **人工验证**：
   - 测试关键功能（编译、运行、调试）
   - 验证文档准确性
   - 检查示例代码可用性

---

### 5.2 构建阶段

#### 5.2.1 清洁构建

所有平台均从干净状态开始构建：

```bash
# 清理旧构建
rm -rf build

# 配置构建（Release 模式）
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 执行构建
cmake --build build --config Release
```

#### 5.2.2 多平台构建矩阵

| 平台 | 架构 | 编译器 | 构建环境 |
|------|------|--------|---------|
| **Windows** | x64 | MSVC 2019+ | GitHub Actions / 本地 |
| **Windows** | x86 | MSVC 2019+ | GitHub Actions / 本地 |
| **Linux** | x64 | GCC 9+ / Clang 10+ | Docker / GitHub Actions |
| **macOS** | x64 | Clang (Xcode 12+) | GitHub Actions |
| **macOS** | arm64 | Clang (Xcode 12+) | GitHub Actions |

#### 5.2.3 构建产物收集

构建完成后，从 `build/` 目录收集以下产物：

```
build/
├── src/cli/cnc/cnc[.exe]           → cnc 编译器
├── src/cli/cnfmt/cnfmt[.exe]       → 代码格式化工具
├── src/cli/cncheck/cncheck[.exe]   → 静态检查工具
├── src/cli/cnlsp/cnlsp[.exe]       → 语言服务器
├── src/cli/cnrepl/cnrepl[.exe]     → REPL
├── src/cli/cnperf/cnperf[.exe]     → 性能分析工具
└── src/runtime/libcnrt[.a/.so/.dll] → 运行时库
```

---

### 5.3 打包阶段

#### 5.3.1 完整发布包结构

```
cnlang-{version}-{platform}-{arch}/
├── bin/                           # 可执行文件
│   ├── cnc[.exe]
│   ├── cnfmt[.exe]
│   ├── cncheck[.exe]
│   ├── cnlsp[.exe]
│   ├── cnrepl[.exe]
│   └── cnperf[.exe]
├── lib/                           # 运行时库
│   ├── libcnrt.a
│   ├── libcnrt.so/.dll/.dylib     (如果构建了动态库)
│   └── freestanding/
│       └── libcnrt-freestanding.a
├── include/                       # 头文件
│   ├── cnrt.h
│   └── cnlang/
│       ├── support/
│       ├── runtime/
│       └── ...
├── share/                         # 共享资源
│   ├── examples/                  # 示例代码
│   ├── templates/                 # 项目模板
│   └── doc/                       # 文档
│       ├── CHANGELOG.md
│       ├── README.md
│       └── LICENSE
├── INSTALL.txt                    # 安装说明
└── VERSION                        # 版本信息文件
```

#### 5.3.2 VERSION 文件格式

```ini
VERSION=1.0.0
BUILD_DATE=2026-01-24
GIT_COMMIT=a1b2c3d4e5f6
PLATFORM=windows
ARCH=x64
COMPILER=MSVC 19.29
```

#### 5.3.3 打包命令

**Windows (PowerShell)**：
```powershell
# 创建压缩包
Compress-Archive -Path cnlang-1.0.0-windows-x64 -DestinationPath cnlang-1.0.0-windows-x64.zip

# 计算校验和
Get-FileHash cnlang-1.0.0-windows-x64.zip -Algorithm SHA256 | Format-List
```

**Linux/macOS**：
```bash
# 创建 tar.gz 包
tar -czf cnlang-1.0.0-linux-x64.tar.gz cnlang-1.0.0-linux-x64/

# 计算校验和
sha256sum cnlang-1.0.0-linux-x64.tar.gz > cnlang-1.0.0-linux-x64.tar.gz.sha256
```

---

### 5.4 签名与校验

#### 5.4.1 数字签名（可选，正式版推荐）

**Windows 代码签名**：
```powershell
# 使用 signtool 签名可执行文件
signtool sign /f certificate.pfx /p password /t http://timestamp.server cnc.exe
```

**GPG 签名**：
```bash
# 对发布包进行 GPG 签名
gpg --armor --detach-sign cnlang-1.0.0-linux-x64.tar.gz
```

#### 5.4.2 校验和文件

为每个发布包生成 SHA256 校验和文件：

```
# cnlang-1.0.0-checksums.txt
a1b2c3d4... cnlang-1.0.0-windows-x64.zip
e5f6g7h8... cnlang-1.0.0-linux-x64.tar.gz
i9j0k1l2... cnlang-1.0.0-macos-arm64.tar.gz
```

---

### 5.5 发布阶段

#### 5.5.1 Git 标签创建

```bash
# 创建带注释的标签
git tag -a v1.0.0 -m "Release version 1.0.0

Major Changes:
- Feature A
- Feature B

Bug Fixes:
- Fixed issue #123
"

# 推送标签到远程仓库
git push origin v1.0.0
```

#### 5.5.2 发布渠道上传

| 渠道 | 位置 | 说明 |
|------|------|------|
| **GitHub Releases** | `https://github.com/{org}/CN_Language/releases` | 主要发布渠道 |
| **官方网站** | `https://{官方域名}/downloads/` | 镜像下载站点 |
| **包管理器** | `apt`/`yum`/`brew`/`chocolatey` | 系统包管理器（后续） |

#### 5.5.3 GitHub Release 创建清单

在 GitHub Releases 页面创建新发布时，需包含：

1. **Release Title**：`CN_Language v1.0.0`
2. **Release Notes**：从 `CHANGELOG.md` 提取对应版本的变更内容
3. **Attached Assets**：
   - 所有平台的发布包（`.zip`/`.tar.gz`）
   - 校验和文件（`checksums.txt`）
   - 签名文件（`.asc`，如果有）
   - 源代码归档（GitHub 自动生成）

---

### 5.6 发布后操作

#### 5.6.1 文档更新
1. 更新官方网站下载链接
2. 发布 Release Notes 博客文章
3. 更新版本兼容性矩阵

#### 5.6.2 社区通知
1. 发布公告到社区论坛/邮件列表
2. 更新社交媒体（如有）
3. 通知第三方集成维护者

#### 5.6.3 分支管理
```bash
# 合并发布分支回主干
git checkout develop
git merge release/v1.0.0

git checkout main
git merge release/v1.0.0

# 删除发布分支
git branch -d release/v1.0.0
```

---

## 6. 自动化构建与 CI/CD

### 6.1 CI/CD 流水线设计

#### 6.1.1 持续集成（CI）流程

```yaml
# .github/workflows/ci.yml (示例)
name: CI Build

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        arch: [x64]
    
    runs-on: ${{ matrix.os }}
    
    steps:
      - uses: actions/checkout@v3
      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      - name: Build
        run: cmake --build build --config Release
      - name: Test
        run: ctest --test-dir build --output-on-failure
      - name: Upload Artifacts
        uses: actions/upload-artifact@v3
        with:
          name: cnlang-${{ matrix.os }}-${{ matrix.arch }}
          path: build/src/cli/cnc/
```

#### 6.1.2 发布流水线（CD）

```yaml
# .github/workflows/release.yml (示例)
name: Release Build

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build all platforms
        run: ./scripts/build-release.sh
      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            dist/*.zip
            dist/*.tar.gz
            dist/checksums.txt
```

### 6.2 夜间构建（Nightly Builds）

- **触发时间**：每天 UTC 00:00
- **构建范围**：所有平台和架构
- **产物保留期**：30 天
- **访问方式**：GitHub Actions Artifacts 或专用 Nightly 频道

---

## 7. 发布检查清单

### 7.1 发布前检查

- [ ] 所有单元测试通过
- [ ] 所有集成测试通过
- [ ] 夜间测试套件通过
- [ ] 代码覆盖率达标（≥ 80%）
- [ ] 文档已更新
- [ ] CHANGELOG.md 已更新
- [ ] 版本号已在所有位置更新
- [ ] 无已知 P0/P1 级别 Bug

### 7.2 构建检查

- [ ] 所有目标平台构建成功
- [ ] 构建产物大小合理（无异常增长）
- [ ] 可执行文件可正常运行
- [ ] 运行时库链接正常
- [ ] 无编译警告（或已评估并豁免）

### 7.3 打包检查

- [ ] 发布包结构完整
- [ ] 版本信息文件正确
- [ ] 示例代码可运行
- [ ] 文档文件齐全
- [ ] 压缩包完整性校验通过

### 7.4 发布检查

- [ ] Git 标签已创建并推送
- [ ] GitHub Release 已创建
- [ ] 校验和文件已上传
- [ ] Release Notes 准确完整
- [ ] 下载链接可访问
- [ ] 安装说明已验证

---

## 8. 工件存储与归档

### 8.1 短期存储（活跃版本）

- **位置**：GitHub Releases + CDN
- **保留期**：永久（正式版）/ 90 天（预发布版）
- **访问方式**：公开下载

### 8.2 长期归档（历史版本）

- **位置**：归档存储（如 S3/对象存储）
- **保留期**：所有正式版永久保留
- **访问方式**：按需提供

### 8.3 内部构建产物

- **位置**：CI/CD Artifacts 存储
- **保留期**：30 天（开发版）/ 90 天（候选版）
- **访问方式**：团队内部访问

---

## 9. 工件存放位置

### 9.1 源代码仓库结构（不包含二进制文件）

```
CN_Language/
├── src/          # 源代码
├── include/      # 头文件
├── docs/         # 文档
├── examples/     # 示例代码
└── README.md
```

### 9.2 发布产物存储结构

```
发布存储根目录/
├── releases/
│   ├── 1.0.0/
│   │   ├── cnlang-1.0.0-windows-x64.zip
│   │   ├── cnlang-1.0.0-linux-x64.tar.gz
│   │   ├── cnlang-1.0.0-macos-arm64.tar.gz
│   │   └── checksums.txt
│   └── 1.1.0/
│       └── ...
├── nightly/
│   ├── 2026-01-24/
│   └── 2026-01-25/
└── archive/
    └── 0.x.x/  # 旧版本归档
```

---

## 10. 应急回滚流程

### 10.1 触发条件

- 发现严重安全漏洞
- 关键功能缺陷导致无法使用
- 数据损坏或丢失风险

### 10.2 回滚步骤

1. **立即下架问题版本**：
   - 从 GitHub Releases 标记为 "Pre-release"
   - 从下载页面移除链接
   - 发布安全公告

2. **评估影响范围**：
   - 统计下载量
   - 联系已知受影响用户
   - 评估数据完整性风险

3. **发布修复版本**：
   - 快速修复问题
   - 递增 PATCH 版本号
   - 按紧急流程发布

4. **事后分析**：
   - 记录问题根因
   - 更新测试用例
   - 改进发布检查流程

---

## 11. 发布周期建议

### 11.1 开发阶段（0.x.x）

- **MINOR 版本**：每 1-2 个月
- **PATCH 版本**：按需发布（Bug 修复）
- **Nightly 构建**：每日自动

### 11.2 稳定阶段（1.x.x+）

- **MAJOR 版本**：每 12-18 个月
- **MINOR 版本**：每 3-4 个月
- **PATCH 版本**：每 2-4 周（按需）
- **LTS 更新**：每月安全补丁

---

## 12. 工具与脚本

### 12.1 发布辅助脚本

#### 12.1.1 版本号更新脚本

```bash
# scripts/update-version.sh
#!/bin/bash
VERSION=$1

# 更新 CMakeLists.txt
sed -i "s/VERSION [0-9.]*/VERSION $VERSION/" CMakeLists.txt

# 更新 version.h
# ...（实现细节）

echo "Version updated to $VERSION"
```

#### 12.1.2 打包脚本

```bash
# scripts/package-release.sh
#!/bin/bash
VERSION=$1
PLATFORM=$2
ARCH=$3

PKG_DIR="cnlang-${VERSION}-${PLATFORM}-${ARCH}"
mkdir -p $PKG_DIR/{bin,lib,include,share/examples}

# 复制文件
cp build/src/cli/cnc/cnc $PKG_DIR/bin/
cp build/src/runtime/libcnrt.a $PKG_DIR/lib/
# ...

# 打包
tar -czf $PKG_DIR.tar.gz $PKG_DIR
sha256sum $PKG_DIR.tar.gz > $PKG_DIR.tar.gz.sha256
```

### 12.2 验证脚本

```bash
# scripts/verify-release.sh
#!/bin/bash
# 验证发布包完整性

echo "Verifying release package..."
# 检查必需文件存在
# 验证校验和
# 测试可执行文件运行
```

---

## 13. 示例：完整发布流程执行

### 13.1 准备发布 v1.0.0

```bash
# 1. 创建发布分支
git checkout -b release/v1.0.0 develop

# 2. 更新版本号
./scripts/update-version.sh 1.0.0

# 3. 更新 CHANGELOG
vim CHANGELOG.md

# 4. 提交更改
git add .
git commit -m "Prepare release v1.0.0"

# 5. 构建所有平台
./scripts/build-all-platforms.sh 1.0.0

# 6. 运行测试
ctest --test-dir build

# 7. 打包
./scripts/package-all-platforms.sh 1.0.0

# 8. 创建标签
git tag -a v1.0.0 -m "Release v1.0.0"

# 9. 推送
git push origin release/v1.0.0
git push origin v1.0.0

# 10. 上传到 GitHub Releases（手动或自动）
```

---

## 14. 参考资源

- [CN_Language 版本号规范](CN_Language%20版本号规范.md)
- [语义化版本控制](https://semver.org/)
- [GitHub Releases 最佳实践](https://docs.github.com/en/repositories/releasing-projects-on-github)

---

**文档版本**：v1.0  
**最后更新**：2026-01-24  
**维护者**：CN_Language 开发团队
