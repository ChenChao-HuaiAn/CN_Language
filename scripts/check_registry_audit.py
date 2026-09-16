#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""检查网第 9 层：门禁清单审计（251-a 立·plans/026 §2.6）。

「新门禁必须注册」的机械化强制——两条审计方向：
  ①scripts/check_*.py 门禁必须全部注册于 check_registry.json（漏注册=FAIL）
   + 清单内脚本文件必须存在 + cn_release_parity.sh 已注册；
  ②scripts/ci.ps1 直接调用的 scripts/check_*.py 必须已在清单（防绕过清单私加门禁）。

用法：python3 scripts/check_registry_audit.py
"""

import json
import re
import sys
from pathlib import Path

项目根 = Path(__file__).resolve().parent.parent
清单文件 = 项目根 / "scripts" / "check_registry.json"
ci文件 = 项目根 / "scripts" / "ci.ps1"
对照脚本 = "scripts/cn_release_parity.sh"


def main():
    if not 清单文件.exists():
        sys.exit(f"[清单审计] FAIL：清单不存在（{清单文件}）")
    清单 = json.loads(清单文件.read_text(encoding="utf-8"))
    注册表 = {}
    for 项 in 清单.get("门禁", []):
        脚本 = 项.get("脚本", "")
        if 脚本 in 注册表:
            sys.exit(f"[清单审计] FAIL：清单内重复注册 {脚本}")
        注册表[脚本] = 项
    问题 = []

    # 审计①a：glob 全部 check_*.py 门禁 → 必须已注册
    实际门禁 = sorted(p.relative_to(项目根).as_posix()
                    for p in (项目根 / "scripts").glob("check_*.py"))
    for 脚本 in 实际门禁:
        if 脚本 not in 注册表:
            问题.append(f"门禁脚本未注册清单（新门禁必须注册）: {脚本}")
    # 审计①b：清单内脚本必须存在 + cn_release_parity.sh 必须注册
    for 脚本 in 注册表:
        if not (项目根 / 脚本).exists():
            问题.append(f"清单注册的脚本不存在: {脚本}")
    if 对照脚本 not in 注册表:
        问题.append(f"纪律对照脚本未注册清单: {对照脚本}")
    # 审计①c：每项必须有 组/名称/检查网层
    for 脚本, 项 in 注册表.items():
        for 键 in ("名称", "组", "检查网层"):
            if 键 not in 项:
                问题.append(f"清单项缺字段 {键}: {脚本}")

    # 审计②：ci.ps1 直接调用的 check_* 必须已注册
    ci文本 = ci文件.read_text(encoding="utf-8") if ci文件.exists() else ""
    for 调用 in sorted(set(re.findall(r"scripts/(check_[A-Za-z_]+\.py)", ci文本))):
        脚本 = f"scripts/{调用}"
        if 脚本 not in 注册表:
            问题.append(f"ci.ps1 调用了未注册门禁（须先进清单）: {脚本}")

    if 问题:
        print("[清单审计] FAIL：", flush=True)
        for x in 问题:
            print(f"  - {x}", flush=True)
        print(f"  → 在 {清单文件} 注册（含 组/检查网层/说明）后重试", flush=True)
        sys.exit(1)
    print(f"[清单审计] PASS ✓（注册 {len(注册表)} 件·check_* glob {len(实际门禁)} 件全注册·"
          f"ci.ps1 调用面无绕过）", flush=True)


if __name__ == "__main__":
    main()
