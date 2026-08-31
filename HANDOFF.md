# HANDOFF 交接文档

**交接时间**: 2026-08-31（P6h 链接环境对齐 + 多源文件支持完成）

## 当前任务（2026-08-31）

**v2 自举重建（plans/014）**：上轮完成 P6f（外部文件读取+类型转换，edc6877）。
本轮完成 **P6h：链接环境对齐 + 多源文件支持**（未提交，工作区已就绪）。

## 本轮完成

- **链接环境对齐**：代码生成.cn 符号修饰 补 主→cn_main（对齐宿主 x64_codegen
  symbolName，extern "C" 裸符号）；宿主 Linux 版 compile 产物 asm 显示 cn_main PROC
  与 v2 输出一致；119 特例按宿主链接命令（/ENTRY:WinMainCRTStartup /STACK:8388608
  + 10 个运行时 obj + CRT 库）链接 v2 产物。
- **多源文件支持**：语法层 扫描导入（登记模块名，支持 导入 路径/::{项}/::*）+ 静态
  导入表（取走导入模块们）+ 变量结构体表清空外移（跨文件保留）；语义层 语义分析多文件
  （全量声明收集→逐文件检查）；IR 层 IR生成多文件；主.cn 驱动器：入口=命令行参数 1
  （系统::参数个数/参数，默认 target/v2src.cn）+ 递归加载导入模块（依赖先解析、
  已加载表防循环、标准库模块跳过）。
- **E2E 119_v2_多文件链接闭环**：宿主 build v2 组件→v2p→编译 主.cn+计算.cn（多文件）
  →v2asm.asm（断言 cn_main PROC）→ml64/link（对齐宿主链接命令）→运行 退出码=14
  （加倍(7)）。run_e2e.py 新增 执行119v2闭环 特例（仅 win-x64，与 79 同源工具链探测）。

## 本环境验证（Linux x86_64，无 cmake/ml64/wine）

- 宿主编译器 Linux 版：手动 g++ 构建（排除 src/runtime/，runtime.cpp 的 entry 引用
  cn_main 会链接失败——编译器本体不依赖运行时）：`find src/cn_compiler -name '*.cpp' |
  xargs g++ -std=c++17 -finput-charset=UTF-8 -Wall -Wextra -I src ... src/cn_main.cpp`
- 全部 v2 组件 check/compile PASS；119 用例宿主语义 check PASS；run_e2e.py py_compile PASS。
- **卡点**：v2p 是 win-x64 exe，本环境无法运行；ml64/link/运行 验证依赖 Windows——
  **CI（windows-latest，push 后自动触发）全量门禁真实执行 119**。

## 踩过的坑

- **「结果」是 CN 关键字**（错误处理类型）：v2 组件里变量名不能叫 结果（宿主解析报
  「预期变量名」）。用的 模块列表 替代。
- 宿主编译器 Linux 构建若包含 src/runtime/*.cpp 会报 undefined reference to cn_main。

## 下一步

HANDOFF 第三步：**v2 编译自身（自举闭环）**——组件自身语法面逐步补齐：
① 常量 顶层声明 + 跨模块常量引用（组件源码大量 常量 节点_*/IR_*/T_*，语义层现在会报
  未声明）；② 容器类型 向量<T>/映射<...> 实例化与 方法调用（字符串方法/容器方法）；
③ 之后 v2 编译 7 组件源码 → 7 asm → 链接成 v2p2，与 v2p 产物对拍（对标 79 闭环）。
另外 P4 代码清理：结果.值 临时绑定规避可去除（链式方法调用已根治）。
