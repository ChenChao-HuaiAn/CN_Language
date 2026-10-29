# runtime 分配失败消费点核查清单（#1012 批甲随批·2026-10-29）

裁决：批甲——运行期内存分配失败=打印诊断后终止（`__cn_runtime_error(4)`·退出码 4），
nullptr/空句柄哨兵退役。001 附录 B 已补条文（错误码 4 处置语义）。

## 一、改造面（本批实施）

| 文件 | 改造点 | 处数 |
|---|---|---|
| src/runtime/string_api.cpp | 字符串族 16 函数 `result==nullptr→return nullptr` 改 `cnStrOomAbort(函数名,字节数)` | 13 |
| src/runtime/string_api.cpp | `__cn_str_sub` 空串路径分配失败静默 → 同上 | 1 |
| src/runtime/string_api.cpp | `__cn_format` vsnprintf 负返回（内部不一致）→ 错误码 6 终止；分配失败 → cnStrOomAbort | 2 |
| src/runtime/string_api.cpp | `__cn_strbuf_new` 初始缓冲 malloc 失败空句柄哨兵 → cnStrbufOomAbort（miss_registry 条 133 同步删·该条自述「改语义须用户批准」今已批） | 1 |
| src/runtime/arena.cpp | `newArenaBlock` malloc 失败 → 打印+错误码 4 终止；`__cn_arena_alloc` 死分支清理 | 2 |
| src/runtime/thread_api.cpp | win 桩 6 处 `fprintf+exit(9)` → `__cn_runtime_error(9)`（标题③风格两制统一） | 6 |
| src/runtime/runtime.hpp + io_api.cpp | `__cn_runtime_error` 声明/定义标 `[[noreturn]]`（MSVC C4716 根治） | 2 |

## 二、消费点核查（批甲随批项）

| 消费面 | 核查方法 | 结论 |
|---|---|---|
| 宿主 IR 发射侧（生成对 16 函数的 call） | grep ir_*.cpp 无「对返回值判 null」的生成逻辑；null 相关仅释放路径空安全（`__cn_str_free(nullptr)` 忽略·与分配失败无关） | ✅ 无依赖 |
| `新建()` 空句柄消费（ir_call_builtin/ir_string_accum/ir_expr_concat） | 原空句柄哨兵消费点在批甲后=不可达死代码（空句柄不再产生）；释放路径空安全惯例无害保留 | ✅ 无依赖 |
| v2 自举侧 | v2p 编译产物链接同一份 C++ runtime（无第二实现） | ✅ 同批覆盖 |
| stdlib CN 层（字符串扩展/JSON 族） | CN 层把返回值当字符串正常消费；合法路径行为零变化 | ✅ 无依赖 |
| 官网文档 | `内存::竞技场分配` 等签名页补「分配失败=程序终止」语义（docs_data_builtin.py） | ✅ 已同步 |
| 保留的 nullptr（甄别非分配失败） | string_api 578/594/740=句柄入参空豁免契约（调用方传 null 的静默面·非分配失败）；file_api 13 处=文件打开失败业务语义（错误码 5·另案） | ✅ 甄别保留 |

## 三、锁面与验证

- OOM 不可注入（无运行期用例可写）——锁面=本清单+001 条文+L1/L2 全绿（合法路径行为零变化）
- L1 全绿（MSVC 全量构建含 runtime+单测+静态门禁四面零红）；`__cn_runtime_error` 标 `[[noreturn]]` 后 C4716（必须有返回值）根治
