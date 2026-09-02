# HANDOFF 交接文档

**交接时间**: 2026-09-02（AGENTS.md ZCode 规范化 + ARM64 计划阶段0 GCC 兼容修复入库）

## 当前任务（2026-09-02）

1. **AGENTS.md 已改造为 ZCode 规范**：只写规则不记进度（原第 8 节速查已删，进度以
   plans/ 内对应任务文档 + 本文件为准）；新增「中间产物与实验清理」规则；推送 remote
   修正为 gitcode.com（本机 remote 名 `origin`）。
2. **v2 ARM64 GAS 后端 → ②b 结构体元素容器 计划已立**（计划文件
   `.zcode/plans/plan-sess_6aabdb49-c38e-464e-aa3a-32dbdcd80603.md`；用户裁决：
   ①先做 v2 的 Linux ARM64 GAS 后端使本机闭环可运行时验证，②b 随后；
   ②②b 一步到位含资源字段结构体）。**阶段0 已完成**：GCC 兼容修复
   （io_api 中文标识符改 ASCII + 单测 fd 操作宏化），本机验证 GCC 零警告 + 单测 1196/1196。
3. P7b（容器全链路/三项裁决/__cn_intern 根治）已于 2026-09-01 完成，详见 git 历史上一版 HANDOFF。

## 下一步

1. **阶段A（ARM64 计划）**：新组件 `CN语言编译器v2/代码生成ARM64.cn` + `主.cn` 目标平台参数
   + run_e2e.py 双平台闭环；验收 = E2E 119/120/123 在 linux-arm64 真实跑通
   （当前这三例在 linux-arm64 按代码逻辑会**失败**而非跳过——阶段A 就是去修它）。
2. **阶段B（②b）**：`向量<结构体>` 全链路（布局对齐宿主 typeSizeOf/元素(i) 视图语义/
   按值传参 ABI/深拷贝与析构注入），E2E 125 新用例先行（先写用例再实现到绿）。
3. 其余后续（嵌套容器/容器作字段/顶层静态容器/③ v2 自举对拍）见 plans/014 与 ARM64 计划文件。
4. **验证边界（诚实门禁）**：本机 linux-arm64 只能验证 GCC 构建 + 单测 + E2E(linux-arm64)；
   win-x64 运行时门禁（119/120/123/125 的 MASM 路径）必须待 Windows 会话执行，不得写假绿。

## 踩过的坑（沿用，新增一条）

- **本轮新增**：单测/编译器可执行文件输出在 `target/` 根（CMakeLists 设
  CMAKE_RUNTIME_OUTPUT_DIRECTORY），不在 target/build/ 下——别在 build 目录找二进制。
- AGENTS.md 写的 remote 名（gitcode）与本仓库实际（origin → gitcode.com）不符，已修正规则。
- bash $? 8 位截断（300 显示 44）——验证退出码用 python subprocess。
- python 写 CN 源码：字符串里的 \n 必须拼 chr(92)+'n'（heredoc 内 '\n' 会变真实换行/裸反斜杠）。
- 结果.值 访问须「如果 (x.正常) 真分支」模式（前置 !正常 返回 不算）。
- v2 词法关键字表 25 个——stdlib 的 泛型/类型 等不在内（token 是标识符）。
- MASM 247 符号限制：v2 函数 >=8 参须核算 mangled 长度。
- 调试产物 target/dbg/（不入库），任务完成按「中间产物与实验清理」规则删除。
