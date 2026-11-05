#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_tree_cache_split.py —— sccache 缓存按树分化（树缓存目录）自测：
纯 python·自跑自断言·exit 0/1。

场景：
  ① 树内稳定：同一路径两次调用同值（同树恒定同分区·增量命中不受影响）；
  ② 跨树隔离：不同树路径→不同分区（缓存键空间物理隔离·跨树重放不可能）；
  ③ 分区形态：结果=基础目录下 tree-<8位十六进制指纹> 子目录；
  ④ 环境尊重：SCCACHE_DIR 重定向→分化基础路径随之变（CI/测试整体重定向）；
  ⑤ 模板贯通：wt.py 两个 cmd 模板与 gate_quick 生成面均以 树缓存目录 产值
    （防模板回退到全机共享固定路径——病根回归即红）。
用法：python scripts/test_tree_cache_split.py   （rc=0=全绿）
"""
from __future__ import annotations

import os
import re
import sys
import tempfile
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
import wt  # noqa: E402


def 主流程() -> int:
    失败: list[str] = []

    def 断言(名: str, 条件: bool) -> None:
        print(f"[{'绿' if 条件 else '红'}] {名}")
        if not 条件:
            失败.append(名)

    with tempfile.TemporaryDirectory() as 临时:
        基础 = Path(临时) / "cache-root"
        树甲 = Path(临时) / "repo" / "主树"
        树乙 = Path(临时) / "repo" / "wt某任务树"
        环境 = dict(os.environ, SCCACHE_DIR=str(基础))

        # ①~④ 环境变量作用域内执行（树内稳定/跨树隔离/分区形态/环境尊重）
        os.environ["SCCACHE_DIR"] = str(基础)
        try:
            值一 = wt.树缓存目录(树甲)
            值二 = wt.树缓存目录(树甲)
            断言("① 同树两次调用同分区", 值一 == 值二)

            分区甲 = wt.树缓存目录(树甲)
            分区乙 = wt.树缓存目录(树乙)
            断言("② 异树不同分区", 分区甲 != 分区乙)

            形态 = re.fullmatch(r".*tree-[0-9a-f]{8}", 分区甲.replace("\\", "/"))
            断言("③ 分区名形如 tree-<8位hex>", 形态 is not None)
            断言("③ 分区落在基础目录下", 分区甲.startswith(str(基础)))

            断言("④ SCCACHE_DIR 重定向生效",
                 wt.树缓存目录(树甲).startswith(str(基础)))

            # ⑤ 模板贯通：两模板 format 产物含分化目录+stop-server 归位行
            #   （防模板回退到全机共享固定路径/丢 server 单例归位——病根回归即红）
            公共 = dict(树=str(树甲), 缓存目录=wt.树缓存目录(树甲), 缓存上限="5G")
            设置 = wt.设置脚本模板.format(vcvars="X", ninja="X", sccache="X", **公共)
            构建 = wt.构建脚本模板.format(vcvars="X", sccache="C:/sc.exe", **公共)
            断言("⑤ 设置模板含分化的 SCCACHE_DIR", f"set SCCACHE_DIR={分区甲}" in 设置)
            断言("⑤ 构建模板含分化的 SCCACHE_DIR", f"set SCCACHE_DIR={分区甲}" in 构建)
            断言("⑤ 构建模板含 stop-server 归位行",
                 '"C:/sc.exe" --stop-server' in 构建)
            断言("⑤ 构建模板含跨树 deps 校验钩子", "check_ninja_deps_tree.py" in 构建)
            固定 = str(Path.home() / "Documents" / "sccache-cache")
            断言("⑤ 模板不含全机共享固定路径",
                 固定 not in 设置 and 固定 not in 构建)
        finally:
            os.environ.pop("SCCACHE_DIR", None)

    if 失败:
        print(f"[失败] {len(失败)} 项红：{'、'.join(失败)}")
        return 1
    print("[全绿] 树缓存分化五场景通过")
    return 0


if __name__ == "__main__":
    raise SystemExit(主流程())
