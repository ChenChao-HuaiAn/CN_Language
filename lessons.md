# lessons.md - AI错误记录与经验教训

> 本文件记录项目开发中遇到的问题、原因与解决方案，防止重复犯错。
> 权重规则：≥15 必须提及；10-14 应该阅读；5-9 可选；<5 可跳过。

## 高权重问题（必须避免）

- [2026-08-12 16:44] **问题类型**: 工具执行错误（权重 15）
  - **描述**: `TEST(SemanticTest, 递归函数)` 中文测试名导致编译失败：`expected initializer before ':' token`（GCC 7 不支持中文标识符）
  - **原因**: GCC 7 的 gtest 宏 `GTEST_TEST_` 用测试名生成 C++ 类名/函数名，中文标识符不合法；注释中的中文虽可编译，但宏展开的标识符不行
  - **解决**: 全部测试名改为英文（如 `TEST(SemanticTest, RecursiveFunction)`），中文仅用于注释/字符串
  - **预防**: 新建测试文件时测试名一律英文；标识符（变量/函数/类/枚举）一律英文

- [2026-08-12 16:44] **问题类型**: 工具执行错误（权重 12）
  - **描述**: `from[0] == '正'` 触发 `-Werror=multichar` 和 `-Werror=type-limits` 编译失败
  - **原因**: '正' 是 UTF-8 多字节字符（3字节），不能与单字节 char 比较；GCC 将多字符常量视为警告并在 -Werror 下报错
  - **解决**: 改用 `from.compare(0, 3, "正") == 0` 字符串前缀比较
  - **预防**: 任何中文字符比较必须用 std::string 的 compare/substr，禁止 `str[i] == '中文字符'`

- [2026-08-12 16:44] **问题类型**: 集成问题（权重 10）
  - **描述**: IRGenerator 继承 AstVisitor 后未实现 `visitIfStmt/visitWhileStmt/visitForStmt`，链接报 `undefined reference to ... vtable`
  - **原因**: 三个虚函数在头文件声明但实现遗漏，vtable 引用缺失
  - **解决**: 补充实现，转发到 genIf/genWhile/genFor
  - **预防**: 继承 AstVisitor 后逐一核对所有纯虚函数是否实现；链接错误先看 undefined reference 到哪个类

- [2026-08-12 16:44] **问题类型**: 逻辑错误（权重 8）
  - **描述**: 测试中 `变量 结果 = 阶乘(5)` 解析失败："预期变量名，实际为 '结果'"
  - **原因**: `结果`（Kw_Result）是 CN 语言53个保留关键字之一（结果<T,E>类型），不能作标识符
  - **解决**: 测试源码改用 `答案` 等非关键字变量名
  - **预防**: 编写 CN 测试代码时避免使用53个关键字作变量/函数名（结果/变量/选择/情况/默认等）

## 既有经验（继承自 HANDOFF，权重 10）

- **GCC 7 不支持中文标识符**：所有标识符（含 gtest 测试名）必须英文，中文仅用于注释/字符串
- **无外网**：googletest 用本地 `~/third-party/unittest/googletest`（LLVM定制版，需 `-DGTEST_NO_LLVM_SUPPORT`）
- **本机构建命令**：`export PATH=$HOME/gcc7/usr/bin:$PATH && cmake -B target/Debug -S . -DCMAKE_CXX_COMPILER=$HOME/gcc7/usr/bin/g++`
- **编译产物在 target/**：测试二进制在 `target/cn_unit_tests`
- **pkill 自匹配**：`pkill -f` 会匹配自身 shell，先 `pgrep -af` 再精确 kill
- **synchronize() 必须前进**：错误恢复停在边界字符会死循环
- **parseStmt() 必须分发 LeftBrace**：嵌套代码块是合法语句
- **`0 ++i` 词法歧义**：字面量后紧跟 ++ 解析为后缀自增，测试需分号分隔
- **peekNext(n) 先跳过当前位置**（`i <= n`）
- **运算符贪婪匹配**：三字符 > 双字符 > 单字符
- **字符串Token保留完整原始文本**

## 实施规范（用户规则）

- 中文回复用户；代码添加中文注释但标识符英文
- 文件 ≤1000行、函数 ≤100行
- E2E先行；禁止 skip()；产物在 target/
- 编译零警告（-Wall -Wextra -Werror）
- plans 文档完成后打勾
- 提交前更新"更新日志.md"（覆盖写入本次内容）
- 会话结束前写 HANDOFF.md 交接
- 实施计划示例代码用中文API命名，但**用户明确要求英文API命名**（指令优先）
