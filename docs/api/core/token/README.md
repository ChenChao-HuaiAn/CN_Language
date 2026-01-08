# CN_Language 令牌模块 API 文档

## 文档概述

本文档目录包含CN_Language令牌模块的完整API文档，为开发者提供详细的接口参考和使用指南。

## 文档结构

### 1. 核心文档
- **CN_token.md** - 核心数据结构文档
  - 令牌类型枚举（70个中文关键字）
  - 令牌结构体定义
  - 关键字信息结构体
  - 抽象接口定义

### 2. API参考文档
- **CN_token_api_reference.md** - 完整API参考
  - 传统API（向后兼容）
  - 模块化接口API
  - 子模块API
  - 错误处理和使用示例

### 3. 模块化架构文档
- **CN_token_module.md** - 模块化架构文档
  - 模块化设计原理
  - 接口使用指南
  - 测试框架和迁移指南

## 文档特点

### 完整性
- 覆盖所有70个中文关键字的类型定义
- 包含传统API和模块化API
- 提供详细的错误处理指南
- 包含丰富的使用示例

### 结构化
- 清晰的文档层次结构
- 统一的格式和风格
- 详细的函数签名和参数说明
- 模块归属标注

### 实用性
- 实际可运行的代码示例
- 错误处理最佳实践
- 性能考虑和建议
- 迁移指南

## 快速开始

### 查看核心数据结构
```bash
# 查看令牌类型定义
cat docs/api/core/token/CN_token.md | head -100
```

### 查找API函数
```bash
# 搜索特定函数
grep -n "F_create_token" docs/api/core/token/CN_token_api_reference.md
```

### 查看使用示例
```bash
# 查看基本使用示例
grep -A 20 "基本使用" docs/api/core/token/CN_token_api_reference.md
```

## 相关文档

### 项目文档
- **项目根目录**：`src/core/token/README.md` - 模块概述和开发指南
- **架构文档**：`docs/architecture/` - 项目架构设计
- **规范文档**：`docs/specifications/` - 技术规范和编码标准

### 测试文档
- **测试代码**：`tests/core/token/` - 单元测试和集成测试
- **测试运行器**：`tests/core/token/test_token_runner.c` - 测试总运行器

## 文档维护

### 更新指南
1. **添加新API**：更新`CN_token_api_reference.md`和`CN_token.md`
2. **修改数据结构**：更新`CN_token.md`中的结构体定义
3. **添加示例**：在相应文档的"使用示例"部分添加
4. **更新错误处理**：更新错误代码表和使用示例

### 版本控制
- 文档版本与代码版本同步
- 重大变更需要更新版本历史
- 保持向后兼容性说明

## 贡献指南

欢迎对API文档进行改进和补充：

1. **报告问题**：在Issue中报告文档错误或不清晰之处
2. **提交改进**：通过Pull Request提交文档改进
3. **添加示例**：提供更多实际使用示例
4. **完善说明**：补充函数说明和注意事项

## 联系方式

- **项目维护者**：CN_Language架构委员会
- **问题反馈**：通过项目Issue跟踪系统
- **文档维护**：docs/api/core/token/目录

## 许可证

本文档是CN_Language项目的一部分，遵循项目许可证条款（MIT License）。
