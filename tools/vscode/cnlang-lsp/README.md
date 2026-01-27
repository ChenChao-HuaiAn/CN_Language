# CN_Language LSP Extension for VS Code

CN_Language 的 Visual Studio Code 语言服务器协议(LSP)扩展。

## 功能特性

- ✅ **语法高亮**: 完整支持 CN_Language 中文关键字语法高亮
- ✅ **代码补全**: 智能关键字补全，包含 33 个 CN_Language 关键字
- ✅ **语义分析**: 实时错误诊断和提示
- ✅ **跳转定义**: 支持跳转到符号定义位置
- ✅ **自动括号**: 自动闭合括号、引号等

## 安装方法

1. 确保已构建 CN_Language 编译器工具链：
   ```powershell
   cmake -B build
   cmake --build build --target cnlsp
   ```

2. 将本目录复制到 VSCode 扩展目录：
   ```powershell
   # Windows
   cp -r tools/vscode/cnlang-lsp ~/.vscode/extensions/
   ```

3. 重启 VS Code

## 配置

扩展会自动在工作区的 `build/src/Debug/cnlsp.exe` (Windows) 或 `build/src/cnlsp` (Linux/macOS) 查找语言服务器。

如需自定义路径，在 VS Code 设置中配置：
```json
{
  "cnlang.lsp.serverPath": "/path/to/cnlsp"
}
```

## 关键字列表

### 控制流关键字 (10个)
如果、否则、当、循环、返回、中断、继续、选择、情况、默认

### 类型关键字 (7个)
整数、小数、字符串、布尔值、空类型、结构体、枚举

### 声明关键字 (6个)
函数、变量、模块、导入、公开、私有

### 常量关键字 (3个)
真、假、无

### 预留关键字 (7个)
命名空间、接口、类、模板、常量、静态、保护、虚拟、重写、抽象

## 技术实现

- **TextMate 语法**: `cnlang.tmLanguage.json` - 基于正则表达式的语法高亮
- **LSP 服务器**: `cnlsp` - C 语言实现的完整 LSP 服务器
- **代码补全**: 基于 `src/frontend/lexer/lexer.c` 实际关键字定义

## 相关文档

- [CN_Language 语言规范](../../../docs/specifications/CN_Language%20语言规范草案（核心子集）.md)
- [LSP 架构设计](../../../docs/design/CN_Language%20LSP%20语言服务架构设计.md)
- [精简关键字完成报告](../../../docs/implementation-plans/阶段%209：CN语法完善/精简关键字完成报告.md)

## 更新日志

### v0.1.0 (2026-01-28)
- ✅ 添加 TextMate 语法高亮文件
- ✅ 实现代码补全功能 (33个关键字)
- ✅ 支持自动括号闭合
- ✅ 支持注释和代码折叠
