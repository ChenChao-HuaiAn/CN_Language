"""ci_static_gates.py —— 云端静态门禁清单（#1221 抽出·1148 外迁续）。

单一职责：③.5 静态门禁清单（跑一轮/分片轮 消费）——ci_daemon.py 660 行数红线
（1005>1000）外迁承载；#1221「自 静态门禁与e2e段 抽出·守 ≤100 行」同构续（外迁
到独立模块·机械搬移零行为变更——1148 新增 check_miss_gate 一并承载）。
"""

from __future__ import annotations


def 静态门禁清单(cn, 平台) -> tuple:
    """③.5 静态门禁清单（#1221 复审 P2：自 静态门禁与e2e段 抽出·守 ≤100 行）。
    1486 起按平台条件追加：窥孔 sanitizer 锁仅非 win 平台（检查面仅 Linux/WSL）。"""
    基础清单 = (("check_spec_coverage", []),   # 243：非 strict（TX_02「指针有效性」存量红=244 修·红不拦兜底轮）
                  # 1013（2026-10-17 用户裁决）：--strict 挂云端兜底轮——基线零（1013 归零）
                  # 即等价棘轮·只拦新增（新 E2E 用例须 doc_coverage 台账归类·豁免理由必填），
                  # 与 L1 gate_quick 静态段/win 全量 ci.ps1 同口径（防漂移·单一注册点=
                  # check_registry.json）。纯本地文件 <1s。
                  ("check_doc_coverage", ["--strict"]),
                  # 1148（2026-10-25·891 盘点§五）：miss 门禁上挂云端静态段——
                  #   静默值登记制/兜底批准制（#1238）的机械守门原只在 L1 gate_quick
                  #   跑：修复批 rebase/组链后的**组合结果**云端无第二道复跑
                  #   （913/933 登记行丢行类云端不可见）。秒级纯本地·与 L1 同口径
                  #   （单一注册点=check_registry.json）。
                  ("check_miss_gate", []),
                  # 896（2026-10-17）：部署清单一致性——deploy_*.sh 拷贝清单 ⊇ 服务运行
                  # 依赖闭包+静态资源覆盖（406/824 漏部署类防复发）；纯本地文件秒级，
                  # 与 L1 gate_quick 静态段/win 全量 ci.ps1 同口径。
                  ("check_deploy_manifest", []),
                  # 822（2026-10-15）：显式传验证面——交叉模拟实例（CN_CI_PLATFORM=linux-arm64·
                  # CN_AS/CN_CXX=交叉工具链）不传则脚本自探=linux-x86_64 面，运行级格把
                  # x86_64 汇编喂给 arm64 工具链必败（42/50 恒红 12 轮兜底存量实录）；传后
                  # target=arm64≠本机运行时平台→运行级格 SKIP（红例三返工机制生效）。
                  ("check_cli_contract", ["--cn", str(cn), "--target", 平台]),
                  ("check_asm_width", ["--cn", str(cn)]),
                  # 887：交叉汇编守卫序列对照（835 病根立法化·内嵌探针×三后端 -O0
                  #   只产汇编不调 as/ld·秒级）——守备溢出/判零/移位域守卫序列的
                  #   条件码级正确性机械对照。
                  ("check_cross_asm_guards", ["--cn", str(cn)]),
                  # 1170：并行 codegen 标签前缀表守卫——1145 并行发射按三后端
                  #   prefix 表平移局部标签编号；新增 ptrCheckCounter_ 派生前缀未
                  #   同步表=并行漏平移产物静默漂移。纯源码静态核对（缺失/陈旧/
                  #   未分类+接线），与 L1 gate_quick 静态段/win 全量 ci.ps1 同口径。
                  ("check_parallel_prefix", []),
                  # 1376：程序缓存键输入面审计——1156 缓存键完备性机械化（登记表
                  #   driver_program_cache_inputs.hpp ⇔ 实现/文档双向对账：新增编译
                  #   输入/闭包字段/环境开关未登记=红；登记项消失/证据锚失效=红）。
                  #   秒级纯源码，与 L1 gate_quick 静态段/win 全量 ci.ps1 同口径。
                  ("check_program_cache_key_audit", []),
                  # 1219：并行 codegen 产物确定性常态门禁——并行4/4/2+自动+
                  #   串行五口径逐字节对照（4 样本×三后端=60 次 compile），
                  #   观测通道断言 parallel/workers 实际数/shifted 平移面覆盖；
                  #   云端全量=唯一恒覆盖面（本地 L1 仅写集触发），实测 ~15s。
                  ("check_parallel_determinism", ["--cn", str(cn)]),
                  # #1310：字符串下标已知长度快路径回归（宿主 IR + v2 产物 asm
                  #   双计数 1/2 正反对照 + v2 源码链路断言）——v2p 未就绪时脚本
                  #   自黄跳产物段（源码断言照跑），v2p 路径按平台自动探测。
                  ("check_knownlen_fastpath", ["--cn", str(cn)]),
                  ("check_matrix_coverage", []),
                  ("check_file_length", ["--freeze"]))
    # 1486（2026-10-26）：窥孔 sanitizer 完整锁（ASan+UBSan 与 _GLIBCXX_DEBUG
    #   两口径真实编译运行 Peephole* 单测·实测 ~1 分钟）挂云端恒跑——1217 只挂了
    #   判别力自测（假输出正反例），完整编译跑「登记不执行」（#1486 缺口）。
    #   云端全量=唯一恒覆盖面（本地 L1 仅写集触发·gate_quick 同款哲学）；
    #   linux 侧 cmake 全量构建后 target/build/_deps/googletest-src 命中依赖
    #   自检，环境缺失 rc=2=红（显形·#1462 缺 gtest 假红教训的显形方向）。
    #   win 平台不挂：检查面仅 Linux/WSL（os.name 恒 rc=2=恒红无意义）。
    if 平台 != "win-x64":
        return 基础清单 + (("check_peephole_sanitizer", []),)
    return 基础清单

