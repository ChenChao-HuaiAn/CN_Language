#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""官网示例 → E2E 用例同源化迁移工具（任务 1060·2026-10-18 用户令）。

背景：官网 269 个示例此前独立手写在 docs_data_*.py 并以 gen_docs --verify
  每轮重编重跑——与 E2E 题库「两套源、两套考试」（重复劳动 + 双源漂移风险）。
  用户令：官网示例直接引用 E2E 用例（「进了 develop 的必然绿灯，无需重复验证
  官方案例」）——本工具把示例一次性迁移成 E2E 用例 + docs_data 改引用形态。

分段（幂等·状态文件 website/tools/docs_migrate_state.json）：
  --报告      枚举全站可迁移示例（块 tuple「码」/dict 块「代码」/条目「示例」）
              与排除项（编译足矣 / 原始多文件 / 综合），输出规模清单。
  --预检      对可迁移示例逐例真编真跑（本树 target/cn），真跑 stdout 与 docs
              期望逐行比对——一致=合格；不一致=报告（人工分诊·不领号）。
  --建用例    合格例：领号（服务端）→ tests/e2e/<号>_<名>/（主.cn=壳化程序+
              两行来源注释；主.expected=预检真跑 stdout）。
  --改写      ast 精确定位 docs_data_*.py 示例节点（按代码文本 sha1 匹配）→
              tuple 替换 ("用例", 名) / dict 键 示例=…,输出=… → 用例='名'。
  --登记      覆盖矩阵：board_cli 用例 官网示例 正例 <名>（逐例服务端登记）。

平台限定（1077）：docs_data 条目「平台」字段（#1017·gen_docs verify 平台分流同源）
  非覆盖三平台全集（win-x64/linux-x86_64/linux-arm64）者=非全平台——迁移面跳过：
  用例不建（state 记「平台」事实与「排」理由·--报告 列明细）。E2E 用例级平台限定
  的唯一真值源=tests/e2e/run_e2e.py 平台跳过表（用例目录内无平台标记机制·不自创
  ——见教训 #96「平台跳过=运行期整例跳过非编译期标记」）；迁入先例=
  1176_官网示例-cli-手工线程栈守卫（win 表登记·1077 补）。

用法（仓库根目录）：
  python website/tools/docs_migrate_e2e.py --报告
  python website/tools/docs_migrate_e2e.py --预检 [--并发 8]
  python website/tools/docs_migrate_e2e.py --建用例
  python website/tools/docs_migrate_e2e.py --改写
  python website/tools/docs_migrate_e2e.py --登记
"""
import argparse
import ast
import concurrent.futures
import hashlib
import io
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
仓库根 = os.path.dirname(os.path.dirname(HERE))
状态路径 = os.path.join(HERE, "docs_migrate_state.json")
sys.path.insert(0, HERE)

import gen_docs as gd  # noqa: E402

# docs_data 模块（页/模块归属反查用；与 gen_docs 装配面同源）
数据模块名 = ["docs_data_start", "docs_data_tut", "docs_data_tut2", "docs_data_adv",
              "docs_data_ref1", "docs_data_ref2", "docs_data_ref3",
              "docs_data_core", "docs_data_mapset", "docs_data_text",
              "docs_data_io", "docs_data_builtin"]
for _n in 数据模块名:
    __import__(_n)


def 代码键(文本):
    return hashlib.sha1(文本.encode("utf-8")).hexdigest()[:16]


叙述模块名 = ["docs_data_start", "docs_data_tut", "docs_data_tut2", "docs_data_adv",
              "docs_data_ref1", "docs_data_ref2", "docs_data_ref3"]
标准库模块名 = ["docs_data_core", "docs_data_mapset", "docs_data_text",
                "docs_data_io", "docs_data_builtin"]


def 页归属(页):
    """按「文件」字段值匹配叙述页池（对象身份不可靠：MODULES/GEN_PAGES 均为 .PAGES 拼接）。"""
    for n in 叙述模块名:
        mod = sys.modules.get(n)
        if mod and any(getattr(p, "get", lambda k, d=None: None)("文件") == 页["文件"]
                       for p in getattr(mod, "PAGES", [])):
            return n
    return "?"


def 模块归属(m):
    for n in 标准库模块名:
        mod = sys.modules.get(n)
        if mod and any(getattr(x, "get", lambda k, d=None: None)("文件") == m["文件"]
                       for x in getattr(mod, "PAGES", [])):
            return n
    return "?"


def 短页(页文件):
    return 页文件.replace(".html", "").replace("/", "-")


# 1077：平台限定（1017 字段 / 1060 迁移回溯）——docs_data 条目「平台」字段在 1060
#   迁移链路未被识别，致「官网示例-cli-手工线程栈守卫」（1176）在 win 全量链接必红
#   （LNK1120·pthread 专有符号）而 run_e2e 平台跳过表无登记。非全平台条目=迁移面
#   跳过（state/报告留痕）；用例级平台限定唯一真值源=run_e2e.py 平台跳过表。
全域平台集 = frozenset(("win-x64", "linux-x86_64", "linux-arm64"))


def 平台限定描述(平台值):
    """「平台」字段 → None=全平台/未声明（照常迁移）；否则='仅 <值>'（迁移面跳过）。"""
    if not 平台值:
        return None
    序列 = [平台值] if isinstance(平台值, str) else list(平台值)
    if 全域平台集 <= set(序列):
        return None
    return "仅 %s" % "、".join(str(x) for x in 序列)


def 排标记(平台值, 编译足矣):
    """recs 层排除标记合成（1077）：平台限定优先于「编译足矣」；无排除=None。"""
    限 = 平台限定描述(平台值)
    if 限:
        return "平台限定（%s）" % 限
    return "编译足矣" if 编译足矣 else None


def 枚举():
    """全站示例 → (可迁移记录列表, 排除报告列表)。记录含壳化程序。"""
    recs, 排除 = [], []

    def 加(排, 述):
        if 排:
            排除.append(述)

    for 页 in gd.GEN_PAGES:
        f = 页归属(页)
        for ch in 页["章节"]:
            if ch.get("综合"):
                加("综合", "%s %s·综合（章节级）" % (页["文件"], ch["id"]))
            for i, 块 in enumerate(ch.get("块", [])):
                if isinstance(块, dict) and 块.get("代码"):
                    if 块.get("预期失败"):
                        加("负例", "%s %s（负例·专门波次处置）" % (页["文件"], 块.get("名")))
                        continue
                    recs.append(dict(形状="dict块", 文件=f, 页=页["文件"], 章=ch["id"],
                                     序=i, 窗名=块.get("名", "码"), 代码=块["代码"],
                                     输出=块.get("输出"), 平台=块.get("平台"),
                                     排=排标记(块.get("平台"), 块.get("编译足矣"))))
                    continue
                if not (isinstance(块, tuple) and len(块) > 2 and 块[0] == "码" and 块[2]):
                    continue
                if "负例" in str(块[1]):
                    加("负例", "%s %s（负例·专门波次处置）" % (页["文件"], 块[1]))
                    continue
                recs.append(dict(形状="块", 文件=f, 页=页["文件"], 章=ch["id"], 序=i,
                                 窗名=块[1], 代码=块[2],
                                 输出=块[3] if len(块) > 3 else None,
                                 排="编译足矣" if (len(块) > 4 and 块[4]) else None))
            for 条 in ch.get("条目", []):
                if 条.get("预期失败"):
                    加("负例", "%s #%s（负例·专门波次处置）" % (页["文件"], 条.get("id")))
                elif 条.get("原始"):
                    加("原始", "%s #%s（原始多文件）" % (页["文件"], 条.get("id")))
                elif 条.get("示例"):
                    recs.append(dict(形状="条目", 文件=f, 页=页["文件"], 章=ch["id"],
                                     条id=条["id"], 窗名=条["id"] + ".cn", 代码=条["示例"],
                                     输出=条.get("输出"), 平台=条.get("平台"),
                                     排=排标记(条.get("平台"), 条.get("编译足矣"))))
    for m in gd.MODULES:
        f = 模块归属(m)
        for ch in m["章节"]:
            if ch.get("综合"):
                加("综合", "stdlib/%s %s·综合（章节级）" % (m["文件"], ch["id"]))
            for 条 in ch.get("条目", []):
                if 条.get("预期失败"):
                    加("负例", "stdlib/%s #%s（负例·专门波次处置）" % (m["文件"], 条.get("id")))
                elif 条.get("原始"):
                    加("原始", "stdlib/%s #%s（原始多文件）" % (m["文件"], 条.get("id")))
                elif 条.get("示例"):
                    recs.append(dict(形状="条目", 文件=f, 页="stdlib/" + m["文件"], 章=ch["id"],
                                     条id=条["id"], 窗名=条["id"] + ".cn", 代码=条["示例"],
                                     输出=条.get("输出"), 平台=条.get("平台"),
                                     排=排标记(条.get("平台"), 条.get("编译足矣"))))
    # 壳化程序（与 verify 同口径=条目程序）+ 键
    for r in recs:
        r["键"] = 代码键(r["代码"])
        _净名 = re.sub(r"[/\\:*?\"<>|\n\r\t]", "-", os.path.splitext(r["窗名"])[0])
        r["用例名"] = "官网示例-%s-%s" % (短页(r["页"]), _净名)
    # 同名去重（同页多块同名 → 加序号）
    计 = {}
    for r in recs:
        计[r["用例名"]] = 计.get(r["用例名"], 0) + 1
    重名 = {n for n, c in 计.items() if c > 1}
    序 = {}
    for r in recs:
        if r["用例名"] in 重名:
            序[r["用例名"]] = 序.get(r["用例名"], 0) + 1
            r["用例名"] += "-%d" % 序[r["用例名"]]
    return recs, 排除


def 壳化(r):
    """示例 → 完整程序文本（与 verify 的 条目程序 同口径：页导入头 + 主 壳）。"""
    页对象 = None
    for coll in (gd.GEN_PAGES, [dict(文件="stdlib/" + x["文件"], 导入头=x.get("导入头")) for x in gd.MODULES]):
        for p in coll:
            if p["文件"] == r["页"]:
                页对象 = p
    if 页对象 is None:
        raise SystemExit("页未找到: %s" % r["页"])
    条 = dict(id=r.get("条id") or r["窗名"], 示例=r["代码"], 输出=r["输出"])
    return gd.条目程序(页对象, 条)


def 读状态():
    if os.path.isfile(状态路径):
        with io.open(状态路径, encoding="utf-8") as f:
            return json.load(f)
    return {}


def 写状态(st):
    with io.open(状态路径, "w", encoding="utf-8", newline="\n") as f:
        json.dump(st, f, ensure_ascii=False, indent=1, sort_keys=True)


def 平台限定留痕(st, recs):
    """平台限定条目 → state 留痕（1077·只改组内记录，落盘由调用方 写状态）。

    未建条目加「排」理由（迁移面跳过——同 1060「序列依赖」排除族先例形态）；
    已建/已改写条目（历史已迁·先例 1176）仅补「平台」事实字段，不覆盖状态。
    返回留痕条数。
    """
    n = 0
    for r in recs:
        if not r.get("平台"):
            continue
        e = st.setdefault(r["键"], {})
        e.setdefault("用例名", r["用例名"])
        e.setdefault("页", r["页"])
        e.setdefault("窗名", r["窗名"])
        e.setdefault("文件", r["文件"])
        e["平台"] = r["平台"]
        if e.get("状态") not in ("已建", "已改写"):
            e["排"] = ("平台限定（%s）——迁移面跳过：E2E 用例级平台限定唯一真值源="
                      "run_e2e.py 平台跳过表（先例 929/1176）；迁入须先在该表登记后"
                      "删本标记与 docs_data「平台」字段再按常规流程迁移"
                      % 平台限定描述(r["平台"]))
        n += 1
    return n


def 行比对(期望文本, 实际文本):
    """与 run_e2e 同口径：逐行 rstrip 全等。返回 None=一致 或 差异描述。"""
    期望 = [x.rstrip() for x in (期望文本 or "").splitlines()]
    实际 = [x.rstrip() for x in (实际文本 or "").splitlines()]
    if 期望 == 实际:
        return None
    for i in range(max(len(期望), len(实际))):
        e = 期望[i] if i < len(期望) else "<缺行>"
        a = 实际[i] if i < len(实际) else "<多行>"
        if e != a:
            return "第%d行 期望=%r 实际=%r" % (i + 1, e, a)
    return "行数差"


def 真跑(cn_exe, 程序文本, 工作目录):
    """编译+运行 → (rc, stdout, 诊断)。源文件名固定 主.cn（多文件入口约定）。"""
    源 = os.path.join(工作目录, "主.cn")
    with io.open(源, "w", encoding="utf-8", newline="\n") as f:
        f.write(程序文本)
    出 = os.path.join(工作目录, "exe")
    r = subprocess.run([cn_exe, "build", 源, "--target", "linux-x86_64", "--output", 出],
                       cwd=仓库根, capture_output=True, text=True, timeout=120)
    if r.returncode != 0 or not os.path.exists(出):
        return None, "", (r.stderr or r.stdout).strip()[:300]
    # ETXTBSY 竞态（并发下 linker 刚写完文件、句柄未落即 exec）：短重试
    #   运行 cwd=工作目录（992 教训：示例写 笔记.txt/日志.txt 曾落仓库根——
    #   跨例串味 + 脏树；每例独立目录运行根治）
    for k in range(6):
        try:
            run = subprocess.run([出], capture_output=True, text=True, timeout=60,
                                 cwd=工作目录)
            return run.returncode, run.stdout, ""
        except OSError as e:
            if getattr(e, "errno", None) == 26 and k < 5:
                import time
                time.sleep(0.2 * (k + 1))
                continue
            raise


def cmd_报告(a):
    recs, 排除 = 枚举()
    st = 读状态()
    可迁 = [r for r in recs if not r["排"]]
    待排 = [r for r in recs if r["排"]]
    限 = [r for r in 待排 if r.get("平台")]
    print("枚举：可迁移 %d 例 / 含排除标记 %d 例（平台限定 %d）/ 排除项 %d 条"
          % (len(可迁), len(待排), len(限), len(排除)))
    页计 = {}
    for r in 可迁:
        页计[r["页"]] = 页计.get(r["页"], 0) + 1
    for k in sorted(页计):
        print("  %-28s %d" % (k, 页计[k]))
    if 限:
        print("平台限定（%d 例·迁移面跳过——E2E 用例级平台限定唯一真值源=run_e2e"
              " 平台跳过表）：" % len(限))
        for r in 限:
            e = st.get(r["键"], {})
            注 = ("（已迁·见 %s）" % e["目录"]
                  if e.get("状态") in ("已建", "已改写") and e.get("目录") else "")
            print("  - %s :: %s（%s）%s" % (r["页"], r["窗名"], 平台限定描述(r["平台"]), 注))
        print("  迁入指引：先在 tests/e2e/run_e2e.py 平台跳过表对应平台数组登记用例短名"
              "（先例 929/1176），再删 state「排」标记与 docs_data 条目「平台」字段，"
              "按常规流程迁移。")
    if 排除:
        print("排除项（保留手写+verify 过渡）：")
        for x in 排除[:30]:
            print("  -", x)
        if len(排除) > 30:
            print("  ...共 %d 条" % len(排除))
    return 0


def cmd_预检(a):
    recs, _ = 枚举()
    可迁 = [r for r in recs if not r["排"]]
    cn = os.path.join(仓库根, "target", "cn")
    if not os.path.exists(cn):
        print("缺少本树编译器 target/cn（先构建）")
        return 1
    st = 读状态()
    # 1077：平台限定条目 state 留痕（迁移面跳过·幂等——不进下方可迁面/不领号）
    限数 = 平台限定留痕(st, recs)
    tmp = tempfile.mkdtemp(prefix="mig_pre_")
    合格 = 不一致 = 失败 = 0

    def 一例(项):
        序, r = 项
        # 目录=序号+键（全局唯一——同码示例键相同，纯键目录会并发共写同一 exe）
        d = os.path.join(tmp, "%03d_%s" % (序, r["键"]))
        os.makedirs(d, exist_ok=True)
        rc, out, 诊 = 真跑(cn, 壳化(r), d)
        if rc is None:
            return r, "编译失败: " + 诊, None
        if rc != 0:
            return r, "运行退出码 %d" % rc, out
        if r["输出"] is None:
            return r, None, out           # 无输出基准（docs 不展示输出）——rc=0 即合格
        差 = 行比对(r["输出"], out)
        return r, (None if 差 is None else "输出不一致: " + 差), out

    结 = []
    # 首例先行=串行预热（1015 同款：cn build 首编含 runtime 链；直接并发首编会
    #   打共享 runtime 产物竞态——946/1046 同族）
    预热 = 一例((0, 可迁[0]))
    结2 = [预热]
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.并发) as 池:
        for r, 错, out in list(池.map(一例, list(enumerate(可迁))[1:])):
            结2.append((r, 错, out))
    for r, 错, out in 结2:
            e = st.setdefault(r["键"], {})
            e["用例名"] = r["用例名"]; e["页"] = r["页"]; e["窗名"] = r["窗名"]
            e["文件"] = r["文件"]
            if 错 is None:
                e["预检"] = "合格"; e["真跑输出"] = out; 合格 += 1
            elif 错.startswith("编译失败"):
                e["预检"] = 错; 失败 += 1; 结.append((r, 错))
            else:
                e["预检"] = 错; 不一致 += 1; 结.append((r, 错))
    写状态(st)
    print("预检：合格 %d / 不一致 %d / 编译失败 %d（共 %d）" % (合格, 不一致, 失败, len(可迁)))
    if 限数:
        print("平台限定：%d 例迁移面跳过（state 已留痕·--报告 列明细与迁入指引）" % 限数)
    for r, 错 in 结[:40]:
        print("  [%s] %s :: %s" % (r["页"], r["窗名"], 错))
    return 1 if (不一致 or 失败) else 0


def 领号(用例名):
    """board_cli 领用例号（服务端顺序发放·744）→ 号 或 None。"""
    r = subprocess.run([sys.executable, os.path.join(仓库根, "scripts", "board_cli.py"),
                        "领用例号", 用例名], cwd=仓库根, capture_output=True, text=True, timeout=60)
    m = re.search(r"\[领用例号\]\s*(\d+)", r.stdout or "")
    if m:
        return int(m.group(1))
    print("  领号失败(%s)：%s" % (用例名, (r.stdout + r.stderr).strip()[:160]))
    return None


def cmd_建用例(a):
    recs, _ = 枚举()
    可迁 = [r for r in recs if not r["排"]]
    st = 读状态()
    建 = 0
    for r in 可迁:
        e = st.get(r["键"], {})
        if e.get("预检") != "合格" or e.get("排"):
            continue                      # 预检不合格/未跑/人工排除 → 不建
        if e.get("状态") in ("已建", "已改写"):
            continue
        名 = e["用例名"]
        号 = 领号(名)
        if 号 is None:
            return 1
        目录 = os.path.join(仓库根, "tests", "e2e", "%d_%s" % (号, 名))
        os.makedirs(目录, exist_ok=True)
        程序 = ("// 官网文档示例 · %s · %s（任务 1060 同源化：本文件即官网展示内容，\n"
                "//   经 run_e2e 门禁实测——展示与门禁同一份文件，单一事实源）\n%s"
                % (r["页"], r["章"], 壳化(r)))
        with io.open(os.path.join(目录, "主.cn"), "w", encoding="utf-8", newline="\n") as f:
            f.write(程序)
        with io.open(os.path.join(目录, "主.expected"), "w", encoding="utf-8", newline="\n") as f:
            f.write(e.get("真跑输出", ""))
        e["状态"] = "已建"; e["号"] = 号; e["目录"] = "%d_%s" % (号, 名)
        建 += 1
    写状态(st)
    print("建用例：%d 例（累计已建 %d / 可迁 %d）" % (
        建, sum(1 for r in 可迁 if st.get(r["键"], {}).get("状态") == "已建"), len(可迁)))
    return 0


# ---------------------------------------------------------------- ast 改写
def 节点列(行, 字节列):
    b = 行.encode("utf-8")
    return len(b[:字节列].decode("utf-8", errors="ignore"))


def 替换范围(行们, 起节点, 止节点):
    sl, sc = 起节点.lineno, 起节点.col_offset
    el, ec = 止节点.end_lineno, 止节点.end_col_offset
    return (sl, 节点列(行们[sl - 1], sc), el, 节点列(行们[el - 1], ec))


def 应用替换(源文本, 替换表):
    """替换表=[(sline, scol, eline, ecol, 新文本)]（1 基行·0 基字符列）——从后往前。"""
    行们 = 源文本.splitlines(keepends=True)
    替换表 = sorted(替换表, key=lambda x: (x[0], x[1]), reverse=True)
    for sl, sc, el, ec, 新 in 替换表:
        if sl == el:
            行们[sl - 1] = 行们[sl - 1][:sc] + 新 + 行们[el - 1][ec:]
        else:
            首 = 行们[sl - 1][:sc] + 新 + 行们[el - 1][ec:]
            行们[sl - 1:el] = [首]
    return "".join(行们)


def 找dict键对(node):
    """dict 节点 → {键: (键节点, 值节点)}。"""
    out = {}
    for k, v in zip(node.keys, node.values):
        if isinstance(k, ast.Constant) and isinstance(k.value, str):
            out[k.value] = (k, v)
    return out


def cmd_改写(a):
    recs, _ = 枚举()
    st = 读状态()
    映射 = {}          # (文件, 代码sha) → 用例名
    for r in recs:
        e = st.get(r["键"], {})
        if e.get("状态") == "已建" and e.get("预检") == "合格":
            映射[(r["文件"], r["键"])] = e["用例名"]
    if not 映射:
        print("无可改写条目（先 --建用例）")
        return 0
    按文件 = {}
    for (f, k), 名 in 映射.items():
        按文件.setdefault(f, {})[k] = 名
    改数 = 0
    for f, 表 in sorted(按文件.items()):
        路径 = os.path.join(HERE, f + ".py")
        with io.open(路径, encoding="utf-8") as fh:
            源 = fh.read()
        树 = ast.parse(源)
        行们 = 源.splitlines(keepends=True)
        替换表 = []
        for node in ast.walk(树):
            if isinstance(node, ast.Tuple) and node.elts and \
                    isinstance(node.elts[0], ast.Constant) and node.elts[0].value == "码" and \
                    len(node.elts) > 2 and isinstance(node.elts[2], ast.Constant) and \
                    isinstance(node.elts[2].value, str):
                键 = 代码键(node.elts[2].value)
                if 键 in 表:
                    sl, sc, el, ec = 替换范围(行们, node, node)
                    替换表.append((sl, sc, el, ec, '("用例", "%s")' % 表[键]))
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and \
                    node.func.id == "dict":
                # stdlib 五文件用 dict(...) 调用形态（非 {...} 字面量）——1060 附录
                kw对 = {kw.arg: kw for kw in node.keywords if kw.arg}
                if "示例" in kw对 and isinstance(kw对["示例"].value, ast.Constant) and \
                        isinstance(kw对["示例"].value.value, str):
                    键 = 代码键(kw对["示例"].value.value)
                    if 键 in 表:
                        名 = 表[键]
                        ks, vs = kw对["示例"], kw对["示例"].value
                        sl, sc, _, _ = 替换范围(行们, ks, ks)
                        _, _, el, ec = 替换范围(行们, vs, vs)
                        替换表.append((sl, sc, el, ec, "用例='%s'" % 名))
                        if "输出" in kw对:
                            ko = kw对["输出"]
                            # 连同前置分隔删除：上一个 keyword 的 value 末尾 → 本 keyword 末尾
                            prev_end = None
                            for i2, kw2 in enumerate(node.keywords):
                                if kw2 is ko and i2 > 0:
                                    prev_end = (node.keywords[i2 - 1].value.end_lineno,
                                                node.keywords[i2 - 1].value.end_col_offset)
                                    break
                            if prev_end:
                                sl2, sc2 = prev_end
                                slp, scp = sl2, 节点列(行们[sl2 - 1], sc2)
                                _, _, el2, ec2 = 替换范围(行们, ko, ko)
                                替换表.append((slp, scp, el2, ec2, ""))
            if isinstance(node, ast.Dict):
                对 = 找dict键对(node)
                if "示例" in 对 and isinstance(对["示例"][1], ast.Constant) and \
                        isinstance(对["示例"][1].value, str):
                    键 = 代码键(对["示例"][1].value)
                    if 键 in 表:
                        名 = 表[键]
                        ks, vs = 对["示例"]
                        sl, sc, _, _ = 替换范围(行们, ks, ks)
                        _, _, el, ec = 替换范围(行们, vs, vs)
                        替换表.append((sl, sc, el, ec, "用例='%s'" % 名))
                        if "输出" in 对:
                            kop, vop = 对["输出"]
                            # 连同前置分隔（, 与空白）一并删除：从上一个键值对末尾到本对末尾
                            prev_end = None
                            键对们 = [(k, v) for k, v in zip(node.keys, node.values)]
                            for i, (kk, vv) in enumerate(键对们):
                                if kk is kop:
                                    prev_end = 键对们[i - 1][1].end_lineno, 键对们[i - 1][1].end_col_offset
                                    break
                            if prev_end:
                                sl2, sc2, el2, ec2 = 替换范围(行们, kop, vop)
                                # 从 prev 值末尾到本对末尾整段删
                                psl, psc = prev_end
                                slp, scp = psl, 节点列(行们[psl - 1], psc)
                                替换表.append((slp, scp, el2, ec2, ""))
        if 替换表:
            新源 = 应用替换(源, 替换表)
            ast.parse(新源)                       # 语法自检——不过即中止
            with io.open(路径, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(新源)
            改数 += len(替换表)
            print("  已改写 %s：%d 处" % (f, len(替换表)))
    # 状态标记=按处理后文件文本验证（防「未真改却标已改写」假销账——1060 自踩）
    文本缓存 = {}
    for r in recs:
        e = st.get(r["键"], {})
        if e.get("状态") not in ("已建", "已改写"):
            continue
        f = e.get("文件")
        if not f or f == "?":
            continue
        if f not in 文本缓存:
            路径2 = os.path.join(HERE, f + ".py")
            文本缓存[f] = io.open(路径2, encoding="utf-8").read() if os.path.isfile(路径2) else ""
        名 = e.get("用例名", "")
        文 = 文本缓存[f]
        if 名 and (("用例='%s'" % 名) in 文 or ('用例="%s"' % 名) in 文 or
                   ('("用例", "%s")' % 名) in 文 or ("('用例', '%s')" % 名) in 文):
            e["状态"] = "已改写"
    写状态(st)
    print("改写合计：%d 处（%d 文件）" % (改数, len(按文件)))
    return 0


def cmd_登记(a):
    recs, _ = 枚举()
    st = 读状态()
    n = 0
    for r in recs:
        e = st.get(r["键"], {})
        if e.get("状态") not in ("已建", "已改写") or e.get("登记"):
            continue
        rr = subprocess.run([sys.executable, os.path.join(仓库根, "scripts", "board_cli.py"),
                             "用例", "官网示例", "正例", e["用例名"]],
                            cwd=仓库根, capture_output=True, text=True, timeout=60)
        if rr.returncode != 0:
            print("  登记失败 %s: %s" % (e["用例名"], (rr.stdout + rr.stderr).strip()[:140]))
            return 1
        e["登记"] = 1
        n += 1
    写状态(st)
    print("覆盖矩阵登记：%d 例" % n)
    return 0


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--报告", action="store_true")
    p.add_argument("--预检", action="store_true")
    p.add_argument("--建用例", action="store_true")
    p.add_argument("--改写", action="store_true")
    p.add_argument("--登记", action="store_true")
    p.add_argument("--并发", type=int, default=8)
    a = p.parse_args()
    if a.报告:
        return cmd_报告(a)
    if a.预检:
        return cmd_预检(a)
    if a.建用例:
        return cmd_建用例(a)
    if a.改写:
        return cmd_改写(a)
    if a.登记:
        return cmd_登记(a)
    p.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
