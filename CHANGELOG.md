# CHANGELOG

本文档记录 CN_Language 编译器和工具链的版本变更历史。
格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)。

---

## 功能完善总结

### 块注释功能实现

**新增功能**：
- 词法分析器支持块注释 `/* */` 格式
- 支持多行块注释
- 未闭合块注释错误诊断

**修改文件**：
- `src/frontend/lexer/lexer.c` - 添加块注释处理逻辑
- `include/cnlang/support/diagnostics.h` - 添加错误码
- `plans/001 CN Language语法规范设计文档.md` - 更新注释规范
- `tools/vscode/cnlang-lsp/cnlang.tmLanguage.json` - 语法高亮配置（已存在）
- `examples/tests/features/test_block_comment.cn` - 测试用例
- `examples/tests/features/test_unterminated_block_comment.cn` - 未闭合测试用例

**测试结果**：
- 词法分析器测试：5/5 通过
- 块注释测试文件编译成功
