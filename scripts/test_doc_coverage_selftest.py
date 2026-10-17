#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_doc_coverage 两态自测（1013·裁决「改检查工具须带 selftest」配套）。

临时镜像树五态（不触真树·跑完即清理）：
  态1 正证：最小干净树（1 特性行全命中）--strict → rc=0（好样本零误伤）
  态2 反证：新增未归类用例 --strict → rc=1（未归类拦截真实生效）
  态3 豁免绿：豁免条目（理由非空）→ rc=0（豁免通道生效）
  态4 豁免理由空白 → rc=1（理由必填核验生效）
  态5 陈旧豁免（条目在用例树无命中）→ rc=1（陈旧豁免核验生效）

机制：check_doc_coverage 三处默认参延迟绑定（1013）——monkeypatch 模块级
台账路径/用例根目录/官网根目录 指向临时树后直调 主流程()（绕开 __main__ 的
仓库根定位，天然隔离真树）。
用法：python scripts/test_doc_coverage_selftest.py   （全绿 rc=0）
"""
import argparse
import contextlib
import importlib.util
import io as _io
import json
import os
import shutil
import sys
import tempfile

脚本路径 = os.path.join(os.path.dirname(os.path.abspath(__file__)), "check_doc_coverage.py")


def _载模块():
    """从源码路径加载 check_doc_coverage 为独立模块（不依赖 sys.path）。"""
    spec = importlib.util.spec_from_file_location("check_doc_coverage_selftest", 脚本路径)
    模 = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(模)
    return 模


def _建最小树(根):
    """最小镜像树：1 特性行（e2e=01_a 精确名·官网覆盖=base.html#锚）+对应物理文件。"""
    台账目录 = os.path.join(根, "website", "tools")
    用例目录 = os.path.join(根, "tests", "e2e")
    文档目录 = os.path.join(根, "website", "docs")
    for d in (台账目录, os.path.join(用例目录, "01_a"), 文档目录):
        os.makedirs(d, exist_ok=True)
    with _io.open(os.path.join(用例目录, "01_a", "主.cn"), "w", encoding="utf-8") as f:
        f.write("// 自测用例\n")
    with _io.open(os.path.join(文档目录, "base.html"), "w", encoding="utf-8") as f:
        f.write('<html><body><h2 id="锚">节</h2></body></html>\n')
    _写台账(根, {
        "特性清单": [{"特性名": "自测特性", "章节": "001 §0", "e2e": ["01_a"],
                     "官网覆盖": ["base.html#锚"], "判定": "已覆盖"}],
        "归类兜底": [],
        "归类豁免": [],
    })


def _写台账(根, 台账):
    路径 = os.path.join(根, "website", "tools", "doc_coverage.json")
    with _io.open(路径, "w", encoding="utf-8") as f:
        json.dump(台账, f, ensure_ascii=False, indent=1)


def _读台账(根):
    路径 = os.path.join(根, "website", "tools", "doc_coverage.json")
    with _io.open(路径, encoding="utf-8") as f:
        return json.load(f)


def _跑strict(模):
    """直调 主流程(--strict)；stdout 重定向保 selftest 输出单屏可读。"""
    缓 = _io.StringIO()
    with contextlib.redirect_stdout(缓):
        return 模.主流程(argparse.Namespace(strict=True, top=80))


def 主流程():
    模 = _载模块()
    根 = tempfile.mkdtemp(prefix="cdc_selftest_")
    失败们 = []

    def 检(名, 期望, 实际):
        标 = "✓" if 期望 == 实际 else "✗"
        print("  [%s] %s：期望 rc=%d 实际 rc=%d" % (标, 名, 期望, 实际))
        if 期望 != 实际:
            失败们.append(名)

    try:
        _建最小树(根)
        模.台账路径 = os.path.join(根, "website", "tools", "doc_coverage.json")
        模.用例根目录 = os.path.join(根, "tests", "e2e")
        模.官网根目录 = os.path.join(根, "website", "docs")

        # 态1 正证：干净树 --strict 绿
        检("态1 干净树 --strict", 0, _跑strict(模))

        # 态2 反证：新增未归类用例 --strict 红（拦截真实生效）
        os.makedirs(os.path.join(模.用例根目录, "02_b"), exist_ok=True)
        with _io.open(os.path.join(模.用例根目录, "02_b", "主.cn"), "w", encoding="utf-8") as f:
            f.write("// 未归类用例（自测造缺口）\n")
        检("态2 新增未归类 --strict", 1, _跑strict(模))

        # 态3 豁免通道：理由非空 → 复绿
        台账 = _读台账(根)
        台账["归类豁免"] = [{"用例": "02_b", "理由": "自测豁免（工具自身两态自测）"}]
        _写台账(根, 台账)
        检("态3 豁免（理由非空） --strict", 0, _跑strict(模))

        # 态4 豁免理由空白 → 红（理由必填核验）
        台账["归类豁免"] = [{"用例": "02_b", "理由": "   "}]
        _写台账(根, 台账)
        检("态4 豁免理由空白 --strict", 1, _跑strict(模))

        # 态5 陈旧豁免：条目在用例树无命中 → 红
        台账["归类豁免"] = [{"用例": "999_不存在", "理由": "陈旧条目自测"}]
        _写台账(根, 台账)
        检("态5 陈旧豁免（无命中） --strict", 1, _跑strict(模))
    finally:
        shutil.rmtree(根, ignore_errors=True)

    if 失败们:
        print("test_doc_coverage_selftest FAIL：%s" % "、".join(失败们))
        return 1
    print("test_doc_coverage_selftest PASS ✓（5 态全过·临时树已清理）")
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
