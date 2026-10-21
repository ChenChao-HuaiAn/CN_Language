#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""官网文档覆盖对照脚本（任务 993）——三层真相源对账工具。

对照面：
  ① 台账  website/tools/doc_coverage.json —— 人工/AI 整理的「001 语言面小节 ×
     已实现 E2E 证据 × 官网覆盖」机器台账（唯一事实输入，本脚本只读不写）；
  ② 实现  tests/e2e/ —— 用例目录名即特性词（实现的 E2E 事实源）；
  ③ 文档  website/docs/ —— gen_docs 生成的 HTML（官网覆盖锚点的落地核验面）。

输出四段：
  ① 未被任何台账行归类的 E2E 用例（新用例漏归类提醒——台账未随新功能更新）；
  ② 缺口清单（判定 = 讲了不全 / 完全没讲；含陈旧失实行标记）；
  ③ 实现待核行（001 有条文但 tests/e2e 找不到直接用例——证据缺口而非文档缺口）；
  ④ 官网锚点核验（台账引用 页面#章节 是否真实存在于生成 HTML——同时兜底
     检测「HTML 被手工改过、与 docs_data_*.py 数据不一致」的形态）。

设计为提示模式：默认 exit 0（v5 纪律「门禁尽量少」）；--strict 供挂门禁
（存在缺口/未归类/锚点缺失时 exit 1）。重跑幂等：纯读操作、输出稳定排序。

1013（2026-10-17 用户裁决）：基线清零（61/61 已覆盖·未归类 0）后 --strict 挂
L1 gate_quick 静态段+win 全量+云端兜底——基线零即等价棘轮（只拦新增缺口/新增
未归类）。豁免通道=台账顶层『归类豁免』（字段：用例/理由）：确不属于任何特性/
兜底的用例走豁免并注明理由，理由空白或豁免条目无命中（陈旧豁免）=strict 红。

用法：
  python scripts/check_doc_coverage.py                # 提示模式
  python scripts/check_doc_coverage.py --strict       # 门禁模式（exit 1 表示有信号）
  python scripts/check_doc_coverage.py --top 40       # 未归类清单最多展示 40 条
  python scripts/test_doc_coverage_selftest.py        # 两态自测（临时镜像树·不触真树）
"""
import argparse
import fnmatch
import io
import json
import os
import re
import sys

仓库根 = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
台账路径 = os.path.join(仓库根, "website", "tools", "doc_coverage.json")
用例根目录 = os.path.join(仓库根, "tests", "e2e")
官网根目录 = os.path.join(仓库根, "website", "docs")

判定录入顺序 = ("完全没讲", "讲了不全", "实现待核", "已覆盖")


def 读台账(路径=None):
    """读台账 JSON；失败即显式报错（不静默兜底）。

    路径=None 时取模块级默认——延迟绑定（1013）：默认参数在 def 时求值会锁死
    模块常量，selftest 的临时镜像树 monkeypatch 就无法生效，故三处默认参一律
    None+函数体内取模块变量（test_doc_coverage_selftest 覆盖入口）。
    """
    if 路径 is None:
        路径 = 台账路径
    if not os.path.isfile(路径):
        raise SystemExit("台账不存在：%s" % 路径)
    with io.open(路径, encoding="utf-8") as f:
        return json.load(f)


def 列用例(根=None):
    """枚举 tests/e2e 下的用例目录（不递归；跳过 assets 等非用例目录）。

    只收「名字含目录内 .cn 文件」或「含数字前缀」的目录——e2e 目录约定：
    每用例一目录、内含 主.cn（或多文件入口 主.cn）。根=None 同上（延迟绑定）。
    """
    if 根 is None:
        根 = 用例根目录
    结果 = []
    for 名 in sorted(os.listdir(根)):
        全 = os.path.join(根, 名)
        if not os.path.isdir(全):
            continue
        含_cn = any(n.endswith(".cn") for n in os.listdir(全))
        if 含_cn or 名[:1].isdigit():
            结果.append(名)
    结果.sort(key=排序键)
    return 结果


def 排序键(名):
    """数字编号用例按数值序（01 < 24_preprocessor < 1000），其余按字典序。"""
    m = 名.split("_", 1)[0]
    if m.isdigit():
        return (0, int(m), 名)
    return (1, 0, 名)


def 命中(条目, 用例名):
    """台账 e2e 条目匹配：含 * ? 视作 glob，否则精确名。"""
    if "*" in 条目 or "?" in 条目:
        return fnmatch.fnmatchcase(用例名, 条目)
    return 条目 == 用例名


def 镜像剥离(用例名):
    """v2 镜像用例名 → 宿主基础名（NNN_v2_XYZ / NNN_v2XYZ → NNN_XYZ 形态）。

    双编译对照约定：v2 侧镜像用例与宿主同名同义，归属同一特性行——
    台账只登记基础名时，镜像名经本函数剥离后再匹配（找不到剥离形态返回原串）。
    """
    for 片段 in ("_v2_", "_v2p_", "_v2"):
        if 片段 in 用例名:
            基 = 用例名.replace(片段, "_", 1)
            while "__" in 基:
                基 = 基.replace("__", "_")
            return 基.rstrip("_")
    return 用例名


def 归类(台账, 用例名):
    """返回命中该用例的特性行列表（允许一行内重复条目，去重后返回）。"""
    行 = []
    for r in 台账["特性清单"]:
        for e in r.get("e2e") or []:
            if 命中(e, 用例名):
                行.append(r)
                break
    return 行


def 归类兜底(台账, 用例名):
    """归类兜底表（台账顶层『归类兜底』）：标准库/内部面/综合回归家族。

    兜底表条目不计入特性统计（保持 001 语言面行数纯净），只用于把
    「不该进对照面」的用例族收敛掉，让未归类清单只剩真正的漏归类信号。
    """
    for b in 台账.get("归类兜底") or []:
        for e in b.get("模式") or []:
            if 命中(e, 用例名):
                return b
    return None


def 豁免列表(台账):
    """台账顶层『归类豁免』：确不属于任何特性/兜底的用例的白名单（字段：用例/理由）。

    1013（2026-10-17 用户裁决）：--strict 挂 L1 后的唯一合法逃生门——豁免须填
    理由（核验理由非空），豁免条目指向的用例树中无命中=陈旧豁免，两态均 strict 红。
    """
    return 台账.get("归类豁免") or []


def 是豁免(豁免们, 用例名):
    """豁免条目匹配：与 e2e 条目同口径（精确名或 glob）。"""
    for e in 豁免们:
        if 命中(e.get("用例", ""), 用例名):
            return True
    return False


def 校验豁免(豁免们, 用例集):
    """豁免表自检：理由空白（豁免必填理由）/条目在用例树无命中（陈旧豁免）=问题。"""
    问题 = []
    for e in 豁免们:
        条 = e.get("用例", "")
        if not str(e.get("理由", "") or "").strip():
            问题.append((条, "理由空白（归类豁免必填理由）"))
        elif not any(命中(条, n) for n in 用例集):
            问题.append((条, "豁免条目在用例树无命中（陈旧豁免·删条或改台账）"))
    return 问题


def 全量归类(台账, 用例名):
    """三层归类：特性行 → 镜像剥离重试 → 归类兜底。返回 (特性行列表, 兜底项|None)。"""
    行 = 归类(台账, 用例名)
    if 行:
        return 行, None
    基 = 镜像剥离(用例名)
    if 基 != 用例名:
        行 = 归类(台账, 基)
        if 行:
            return 行, None
    return [], 归类兜底(台账, 用例名)


def 校验证据条目(台账, 用例集):
    """台账 e2e 条目自检：精确名须存在；glob 须至少命中一个（陈条目即红信号）。"""
    问题 = []
    for r in 台账["特性清单"]:
        for e in r.get("e2e") or []:
            if "*" in e or "?" in e:
                if not any(命中(e, n) for n in 用例集):
                    问题.append((r["特性名"], e, "glob 无命中"))
            elif e not in 用例集:
                问题.append((r["特性名"], e, "精确名不存在"))
    return 问题


def 校验锚点(台账, 官网根=None):
    """校验 页面路径#锚点：文件存在 + HTML 内含 id="锚点"（同时兜底 HTML 与数据分叉）。

    官网根=None 时取模块级默认（延迟绑定·同 读台账 1013 注）。
    """
    if 官网根 is None:
        官网根 = 官网根目录
    问题 = []
    for r in 台账["特性清单"]:
        引用集 = r.get("官网覆盖")
        if not 引用集:
            continue
        if isinstance(引用集, str):
            引用集 = [引用集]
        for 引用 in 引用集:
            if "#" in 引用:
                文件, 锚点 = 引用.split("#", 1)
            else:
                文件, 锚点 = 引用, ""
            网页 = os.path.join(官网根, 文件)
            if not os.path.isfile(网页):
                问题.append((r["特性名"], 引用, "HTML 文件不存在"))
                continue
            if not 锚点:
                continue
            with io.open(网页, encoding="utf-8") as f:
                正文 = f.read()
            if ('id="%s"' % 锚点) not in 正文:
                问题.append((r["特性名"], 引用, "锚点在 HTML 中缺失"))
    return 问题


def 汇总判定(台账):
    计数 = dict((k, 0) for k in 判定录入顺序)
    未知 = []
    for r in 台账["特性清单"]:
        判 = r.get("判定", "")
        if 判 in 计数:
            计数[判] += 1
        else:
            未知.append((r["特性名"], 判))
    return 计数, 未知


def 打印分节(标题):
    print("")
    print("----- %s -----" % 标题)


def 枚举文档引用():
    """1060：读 docs_data（经 gen_docs 装配）→ docs 里「用例引用」的用例名集合。

    selftest 可 monkeypatch 本函数（镜像树不搭 docs_data 全量数据）。"""
    sys.path.insert(0, os.path.join(仓库根, "website", "tools"))
    import gen_docs as gd
    引用 = set()

    def 收(条):
        if isinstance(条, dict) and 条.get("用例"):
            引用.add(条["用例"])

    for 页 in gd.GEN_PAGES:
        for ch in 页["章节"]:
            for 块 in ch.get("块", []):
                if isinstance(块, tuple) and 块[0] == "用例":
                    引用.add(块[1])
                else:
                    收(块)
            for 条 in ch.get("条目", []):
                收(条)
    for m in gd.MODULES:
        for ch in m["章节"]:
            for 条 in ch.get("条目", []):
                收(条)
    return 引用


def 用例入口文件(目录):
    """入口真值源=双编译对照.txt「源文件=」键（1295 与 gen_docs.用例入口 同规：
    多文件用例入口不叫 主.cn——01_hello/15_expressiveness 等；多值「主.cn,容器.cn」
    取首个），expected=<入口基名>.expected。缺对照/缺键回退 主.cn。"""
    名 = "主.cn"
    对照 = os.path.join(目录, "双编译对照.txt")
    if os.path.isfile(对照):
        with io.open(对照, encoding="utf-8", errors="replace") as f:
            for 行 in f:
                m = re.match(r"^源文件\s*=\s*(\S+)", 行.strip())
                if m:
                    名 = re.split(r"[,，\s]+", m.group(1))[0]
                    break
    return 名, os.path.splitext(名)[0] + ".expected"


def 校验引用集(引用集):
    """1060：引用用例轻校验（不依赖编译器）——目录/入口源码/期望文件齐备且非负测。
    返回 [(用例名, 原因)]。用例根目录=模块常量（延迟绑定·selftest 可 monkeypatch）。
    1295：宿主+v2 成对共号（合法共号族）引用不带号名时宿主侧优先（与 gen_docs 同规）。"""
    import glob as _glob
    问题 = []
    for 名 in sorted(引用集):
        目录 = os.path.join(用例根目录, 名)
        if not os.path.isdir(目录):
            命中 = sorted(_glob.glob(os.path.join(用例根目录, "*_" + 名)))
            if len(命中) > 1:
                非v2 = [h for h in 命中 if "_v2_" not in os.path.basename(h)]
                if len(非v2) == 1:
                    命中 = 非v2
            if len(命中) == 1:
                目录 = 命中[0]
            elif len(命中) > 1:
                问题.append((名, "用例名不唯一（多目录后缀匹配）"))
                continue
        if not os.path.isdir(目录):
            问题.append((名, "用例目录不存在"))
            continue
        源名, 期名 = 用例入口文件(目录)
        if not os.path.isfile(os.path.join(目录, 源名)):
            问题.append((名, "缺 " + 源名))
            continue
        if not os.path.isfile(os.path.join(目录, 期名)):
            问题.append((名, "缺 " + 期名))
            continue
        with io.open(os.path.join(目录, 源名), encoding="utf-8") as f:
            头 = f.read(800)
        if "预期失败" in 头:
            问题.append((名, "负测用例（预期失败）不可作教学示例"))
    return 问题


def 主流程(args):
    台账 = 读台账()
    用例集 = 列用例()
    行集 = 台账["特性清单"]

    未归类 = []
    归类张数 = dict()
    兜底张数 = dict()
    豁免命中数 = 0
    豁免们 = 豁免列表(台账)
    for n in 用例集:
        命中行, 兜 = 全量归类(台账, n)
        if 命中行:
            归类张数[n] = len(命中行)
        elif 兜:
            兜底张数[n] = 兜["名称"]
        elif 是豁免(豁免们, n):
            豁免命中数 += 1
        else:
            未归类.append(n)

    计数, 未知判定 = 汇总判定(台账)
    缺口行 = [r for r in 行集 if r.get("判定") in ("讲了不全", "完全没讲")]
    陈旧行 = [r for r in 行集 if r.get("陈旧")]
    待核行 = [r for r in 行集 if r.get("判定") == "实现待核"]
    证据问题 = 校验证据条目(台账, 用例集)
    锚点问题 = 校验锚点(台账)
    豁免问题 = 校验豁免(豁免们, 用例集)
    引用集 = 枚举文档引用()
    引用问题 = 校验引用集(引用集)

    print("===== 官网文档覆盖对照（任务 993）=====")
    print("台账：website/tools/doc_coverage.json（%d 行：已覆盖 %d / 讲了不全 %d / 完全没讲 %d / 实现待核 %d）"
          % (len(行集), 计数["已覆盖"], 计数["讲了不全"], 计数["完全没讲"], 计数["实现待核"]))
    print("E2E 用例：%d 个目录；特性行归类 %d 个（%.1f%%）+ 兜底归类 %d 个（标准库/内部面/综合回归家族）+ 未归类 %d 个"
          % (len(用例集), len(归类张数), 100.0 * len(归类张数) / max(1, len(用例集)),
             len(兜底张数), len(未归类)))
    for 名, 判 in 未知判定:
        print("  [台账异常] 未知判定值：%s -> %r" % (名, 判))

    打印分节("① 未被任何台账行归类的 E2E 用例（%d 个，新用例漏归类提醒）" % len(未归类))
    if 兜底张数:
        桶计 = dict()
        for 名, 桶 in 兜底张数.items():
            桶计[桶] = 桶计.get(桶, 0) + 1
        for 桶, k in sorted(桶计.items()):
            print("  （兜底归类 %d 个：%s）" % (k, 桶))
    if 豁免命中数:
        print("  （归类豁免 %d 个：台账『归类豁免』·1013 通道·理由必填）" % 豁免命中数)
    if 豁免问题:
        for 条, 原因 in 豁免问题:
            print("  [豁免异常] %s（%s）" % (条, 原因))
    if not 未归类:
        print("  无——全部 %d 个用例均有归属" % len(用例集))
    for i, n in enumerate(未归类):
        if i >= args.top:
            print("  …… 其余 %d 条省略（--top 调大查看）" % (len(未归类) - args.top))
            break
        print("  %s" % n)

    打印分节("② 缺口清单（判定≠已覆盖；★=陈旧失实，优先修）")
    for r in 缺口行:
        标 = "★" if r.get("陈旧") else " "
        print("  [%s]%s %s（%s）" % (r["判定"], 标, r["特性名"], r["章节"]))
    for r in 待核行:
        print("  [实现待核]  %s（%s）——E2E 证据缺口" % (r["特性名"], r["章节"]))

    打印分节("③ 台账证据条目自检（陈条目/错名 = 台账与用例树脱节信号）")
    if 证据问题:
        for 名, e, 原因 in 证据问题:
            print("  %s -> %s（%s）" % (名, e, 原因))
    else:
        print("  全部命中（%d 条目）" % sum(len(r.get("e2e") or []) for r in 行集))

    打印分节("④ 官网锚点核验（页面#章节 是否真实存在于生成 HTML）")
    if 锚点问题:
        for 名, 引用, 原因 in 锚点问题:
            print("  %s -> %s（%s）" % (名, 引用, 原因))
    else:
        引用数 = 0
        for r in 行集:
            覆盖引用 = r.get("官网覆盖")
            if isinstance(覆盖引用, str):
                引用数 += 1
            elif 覆盖引用:
                引用数 += len(覆盖引用)
        print("  全部命中（%d 处引用）" % 引用数)

    打印分节("⑤ 官网示例引用校验（1060·docs_data 用例引用 → tests/e2e 存在性）")
    if 引用问题:
        for 名, 原因 in 引用问题:
            print("  [引用失效] %s：%s" % (名, 原因))
    else:
        print("  全部命中（%d 个引用用例·目录/主.cn/主.expected 齐备且非负测）" % len(引用集))

    打印分节("汇总")
    print("缺口（讲了不全/完全没讲）：%d 行；其中陈旧失实 %d 行" % (len(缺口行), len(陈旧行)))
    print("实现待核：%d 行；未归类用例：%d 个；豁免异常：%d 项；台账自检问题：%d 项；锚点问题：%d 项；引用问题：%d 项"
          % (len(待核行), len(未归类), len(豁免问题), len(证据问题), len(锚点问题), len(引用问题)))

    if args.strict:
        有信号 = bool(缺口行 or 未归类 or 证据问题 or 锚点问题 or 未知判定 or 豁免问题 or 引用问题)
        return 1 if 有信号 else 0
    return 0


def main():
    ap = argparse.ArgumentParser(description="官网文档覆盖对照（任务 993·默认提示模式 exit 0）")
    ap.add_argument("--strict", action="store_true", help="门禁模式：存在信号即 exit 1")
    ap.add_argument("--top", type=int, default=80, help="未归类清单最多展示条数（默认 80）")
    args = ap.parse_args()
    if args.top <= 0:
        raise SystemExit("--top 须为正数")
    sys.exit(主流程(args))


if __name__ == "__main__":
    main()
