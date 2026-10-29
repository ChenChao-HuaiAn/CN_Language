#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 生成器·自校验（1772 拆出面：cnsmith_gen.py 923 行触 900 警戒线，
selftest 段按 cnsmith_dual_selftest.py 独立文件先例拆出——接口不变，
cnsmith_gen.main 延迟导入本模块）。

生成器自校验：移位域（474）+折叠域（847 同族）+采样生成面。

静态面（零依赖）：
  ①样本面=847 实录 12 样本 seed（1..12）+3 个实录危险 seed 采样；
  ②全部移位量 ∈ [0, 32)（474——防 840 缺陷重演锁面）；
  ③折叠值不变量（生成期 _域断言 内建·超域即异常显形）；
  ④生成面未退化+折叠保护审计面有捕获（miss 不可静默）。
编译对拍（--编译 <cn.exe>）：⑤样本全编译通过；⑥审计危险表达式全被拒
  （折叠模拟与编译器口径双向闭环——偏差即显形）。
"""
import os
import random
import re
import shutil
import subprocess

from cnsmith_gen import (  # noqa: E402  （本体拆出面）
    Gen, NL, _字面量内索引集, inject_error,
)
from cnsmith_gen_families import 族名们  # noqa: E402  （族名单直接源·families 无反向依赖）


def selftest(编译路径=None, 目标=None):
    """生成器自校验入口（判据=全绿 0/红 1——对齐 cnsmith_run 判定风格）。"""
    通过, 失败 = [], []
    语句数 = 14   # 对齐 CLI 默认 --stmts（实录样本生成口径）

    def 签(名, 条件, 详情=""):
        if 条件:
            通过.append(名)
        else:
            失败.append((名, 详情))

    # 实录危险 seed（2026-10-17 复核：s1019/s251015000020/s251017000004 曾被
    #   「编译期整数溢出」拒绝——常量折叠超域）恒入锁面，修复回归即 selftest 红
    种子们 = list(range(1, 13)) + [1019, 251015000020, 251017000004]
    样本们 = []
    审计 = []
    for seed in 种子们:
        g = Gen(seed)
        样本们.append((seed, g.program(语句数)))   # 语句数=CLI 默认（实录口径）
        审计.extend(g.审计危险)

    移位量们 = []
    for _seed, src in 样本们:
        移位量们.extend(int(t) for t in re.findall(r"(?:<<|>>)\s*(-?\d+)", src))
    签("样本数=%d（12 实录 seed+3 危险 seed）" % len(样本们), len(样本们) == 15)
    签("移位量全部 ∈ [0,32)（474·%d 个）" % len(移位量们),
       len(移位量们) > 0 and all(0 <= v < 32 for v in 移位量们))
    文本全集 = NL.join(src for _s, src in 样本们)
    签("生成面未退化（移位/算术/除模均有产出）",
       bool(re.search(r"<<|>>", 文本全集))
       and bool(re.search(r" [\+\-\*] ", 文本全集))
       and bool(re.search(r" [/%] ", 文本全集)))
    # 847 折叠保护确定性锁面（1306：推断语句消耗随机流致 15 实录 seed 采样
    #   漂移·「随机面审计>=1」断言概率化失效——847 注释实录危险式直接单元化，
    #   同 seed 恒绿；随机面审计列表继续积累供编译对拍面双向对拍）
    g847 = Gen(847)
    t1_847, v1_847 = g847._算术折叠("*", "(-379 * -130)", "411",
                                    -379 * -130, 411)
    t2_847, v2_847 = g847._算术折叠("*", t1_847, "110", v1_847, 110)
    签("折叠保护修正确定面（847 实录式超域→右置 0+审计捕获）",
       v1_847 == 49270 * 411 and v2_847 == 0
       and len(g847.审计危险) == 1 and t2_847.endswith("* 0)"))
    审计.extend(g847.审计危险)   # 编译对拍面恒有实录超域式（非空锁）
    # 1306 扩面锁：推断声明三型（拼接/浮整混算/三元跨类）均有产出——
    #   形态域即 #1304/#1305 命中域，生成面回退=自动面重新致盲
    推断语句 = re.findall(r"变量 \S+ = [^\n]+", 文本全集)
    签("推断声明有产出（%d 条）" % len(推断语句),
       len(推断语句) >= 15, "样本集推断语句过少")
    签("推断声明三型覆盖（拼接/浮整混算/三元跨类）",
       bool(re.search(r'变量 \S+ = "[^"]*" \+', 文本全集))
       and bool(re.search(r"变量 \S+ = \S+ [\+\-\*] \d+\.\d+", 文本全集))
       and bool(re.search(r"变量 \S+ = \(\S+ > 0\) \? 1 : 2\.5", 文本全集)),
       "三型缺一")

    # 1022/1740/1772 族扩面锁（#1398 随批）：混合面 20 seed 覆盖十族+标量回退；
    #   族结构完整（删族=自动面重新致盲 → 本轮 #1393~#1397 等永不再显形）
    混合样本 = [(seed, Gen(seed).program_mixed(语句数)) for seed in range(1, 21)]
    混合文本 = NL.join(src for _s, src in 混合样本)
    缺族 = [族 for 族 in 族名们 if ("// 族=" + 族) not in 混合文本]
    签("1022/1740/1772 族生成面覆盖（%d/%d）" % (len(族名们) - len(缺族), len(族名们)),
       not 缺族, "缺 %s" % 缺族)
    签("1022 混合面保留标量回退（seed%11==0）",
       any("=== CN-Smith 采样 ===" in src and "// 族=" not in src
           for _s, src in 混合样本))
    族样本 = [(s, src) for s, src in 混合样本 if "// 族=" in src]
    签("1022 族程序结构完整（主函数唯一·%d 件）" % len(族样本),
       len(族样本) == 19 and all(src.count("函数 主()") == 1
                                 for _s, src in 族样本))
    # 1740 三新族源码形态锚锁（#1735/#1737/#1739）：枚举声明转整打印
    #   （#1760 立法反转后族体=声明即赋值正形态）/句柄链调用/下标源声明
    #   三形态在各自族样本必现——模板退化=自动面重新致盲
    #   （2026-10-27 采样轮四缺陷零自动覆盖实录重演）；聚合比较负形态不入
    #   随机合法面（走对拍内置双侧拒绝锁·模块 docstring 1740 段）
    锁锚缺失 = []
    for _s, src in 混合样本:
        族m = re.match(r"// 族=(\S+)", src)
        if not 族m or 族m.group(1) not in ("枚举默认初始化", "方法句柄链", "结构体拷贝"):
            continue
        if 族m.group(1) == "枚举默认初始化" and \
                not re.search(r"打印\(整32\(", src):
            锁锚缺失.append((_s, "枚举默认初始化缺转整打印"))
        if 族m.group(1) == "方法句柄链" and not re.search(r"\)\.\S+\(\)", src):
            锁锚缺失.append((_s, "句柄链调用形态缺失"))
        if 族m.group(1) == "结构体拷贝" and \
                not re.search(r"点 \w+ = (\w+\.)?\w+\[\d\];", src):
            # 两变体形态：直下标源 路[1] / 嵌套成员链下标源 外.路[2]
            锁锚缺失.append((_s, "下标源声明形态缺失"))
    签("1740 三新族源码形态锚（声明转整打印/句柄链/下标源声明）",
       not 锁锚缺失, str(锁锚缺失[:3]))
    # 1772 三新族源码形态锚锁（#1770/#1771）：双层可选构造+.值.值 读取链/
    #   多维下标赋值/取地址与指针下标三形态在各自族样本必现（2026-10-28
    #   采样轮 4 项新发现全靠手动探针挖出、自动轨道零覆盖——删锚=重新致盲）
    锚扩面坏 = []
    for _s, src in 混合样本:
        族m = re.match(r"// 族=(\S+)", src)
        if not 族m or 族m.group(1) not in ("嵌套构造", "多维数组", "指针出参"):
            continue
        if 族m.group(1) == "嵌套构造" and (
                "某些(某些(" not in src or ".值.有值" not in src):
            锚扩面坏.append((_s, "嵌套构造缺双层构造/读取链"))
        if 族m.group(1) == "多维数组" and not re.search(r"\w+\[\w\]\[\w\]", src):
            锚扩面坏.append((_s, "多维数组缺双重下标"))
        if 族m.group(1) == "指针出参" and (
                "&槽[" not in src or "不安全" not in src):
            锚扩面坏.append((_s, "指针出参缺取地址/不安全面"))
    签("1772 三新族源码形态锚（双层构造/双重下标/取地址不安全面）",
       not 锚扩面坏, str(锚扩面坏[:3]))
    # 1022 注入器候选行锁（#1398 随批）：族程序首行注释不得吞掉注入；
    #   截断点必须是代码行（原实现两缺陷=样本实际仍合法→宿主误接受假命中）
    注入坏 = []
    for seed in range(1, 5):
        基 = Gen(seed).program_mixed(语句数)
        基行们 = 基.split(NL)
        for k in range(8):
            注入, kind, 行号 = inject_error(基, random.Random(seed * 7 + k))
            if 注入 is None:
                注入坏.append((seed, kind, "跳过"))
                continue
            原行 = 基行们[行号 - 1] if 0 < 行号 <= len(基行们) else ""
            if kind == "非法字符" and 原行.strip().startswith("//"):
                注入坏.append((seed, kind, "注释行"))
            if kind == "截断文件" and not 原行.strip():
                注入坏.append((seed, kind, "空行"))
    签("1022 注入器候选行锁（注释不吞注入·截断必删代码行）", not 注入坏,
       str(注入坏[:5]))

    # 1618 注入点字面量避让锁：注入点必须落在代码位，禁落字符串/字符字面量内
    #   （s20261259 实录：@@ 追加进多行字符串体内=样本仍合法→宿主误接受误报）。
    #   最小样本四形态：①多行字符串体（行尾=未闭合三引号·缺陷形态+体中行）
    #   ②普通字符串体内分号/括号 ③字符字面量 ④字面量外正常代码行——
    #   断言注入点不落①②③、④正常注入；整体字面量样本全类型跳过（标注跳过）
    避让样本 = (
        "函数 主() -> 整32 {\n"
        "    字符串 甲 = 多行\"\"\"第一行\n"          # ① 行尾=未闭合三引号体内
        "体中行\n"                                     # ① 体中整行（截断/花括号面）
        "第二行\"\"\";\n"
        "    字符串 乙 = \"a;b)c';\";\n"               # ② 普通字符串体内 ; ) '
        "    字符 c = ';';\n"                          # ③ 字符字面量
        "    打印(字符串长度(乙));\n"                  # ④ 字面量外正常代码行
        "    返回 0;\n"
        "}\n"
    )
    避让行们 = 避让样本.split(NL)
    避让内 = _字面量内索引集(避让样本)
    避让基 = [0]
    for l in 避让行们:
        避让基.append(避让基[-1] + len(l) + 1)
    避让坏 = []
    for k in range(40):
        注入, kind2, 行号2 = inject_error(避让样本, random.Random(k))
        if 注入 is None:
            避让坏.append((k, kind2, "全类型跳过（样本有充分代码位·不应发生）"))
            continue
        if kind2 == "截断文件":
            if 避让基[行号2 - 1] in 避让内:
                避让坏.append((k, kind2, "截断行首在字面量内"))
            continue
        原行 = 避让行们[行号2 - 1]
        if kind2 == "缺右花括号":
            if 避让基[行号2 - 1] + 原行.index("}") in 避让内:
                避让坏.append((k, kind2, "}} 注入列在字面量内"))
            continue
        新行 = 注入.split(NL)[行号2 - 1]
        if kind2 == "非法字符":
            改列 = len(原行)            # 追加位置=行尾（s20261259 缺陷判定式）
        else:
            改列 = 0                    # 删字符：首差异列=被删字符列
            while (改列 < len(原行) and 改列 < len(新行)
                   and 原行[改列] == 新行[改列]):
                改列 += 1
        if 避让基[行号2 - 1] + 改列 in 避让内:
            避让坏.append((k, kind2, "注入列在字面量内"))
    签("1618 注入点字面量避让（多行/普通字符串体+字符字面量不吞注入·"
       "正常代码行照常注入）", not 避让坏, str(避让坏[:5]))
    纯体样本 = "多行\"\"\"体一\n体二\n体三\n"   # 未闭合三引号延伸到文件尾——
    #   各行行尾/行首全部落在跨行字面量内（含文件尾哨兵）→ 全类型无代码位
    纯注入, 纯kind, 纯行 = inject_error(纯体样本, random.Random(0))
    签("1618 整体字面量样本跳过注入（标注跳过·不产假样本）", 纯注入 is None,
       "kind=%s 行=%s" % (纯kind, 纯行))

    if 编译路径 is not None:
        目标 = 目标 or ("win-x64" if os.name == "nt" else "linux-x64")
        临时 = os.path.join("target", "cnsmith_selftest_tmp")
        os.makedirs(临时, exist_ok=True)
        try:
            过, 坏 = 0, []
            for seed, src in 样本们:
                源 = os.path.join(临时, "s%d.cn" % seed)
                with open(源, "w", encoding="utf-8", newline="") as f:
                    f.write(src)
                r = subprocess.run(
                    [编译路径, "build", 源, "--target", 目标, "-O0",
                     "--output", 源[:-3] + ".exe"],
                    capture_output=True, timeout=120)
                if r.returncode == 0:
                    过 += 1
                else:
                    坏.append(seed)
            签("样本全部编译通过（%d/%d）" % (过, len(样本们)),
               过 == len(样本们), "被拒 seed=%s" % 坏)

            # 1022/1740/1772 族面编译锁：seed 1~11 各命中一面（十族+标量；
            #   宿主对默认参数常量算式放行但值错——合法性契约只要求编译通过，
            #   值错由对拍轨道显形）。1772 多维数组族在 #1771 集成前=预期红
            #   （def-init 误拦「循环下标写后读」）——白名单+诊断锚校验：
            #   编译失败必须匹配「未初始化」锚（防族程序语法错误假红混入；
            #   #1771 集成后本白名单收口为全通过断言）
            族坏 = []
            族红锚命中们 = []
            for seed in range(1, 12):
                源 = os.path.join(临时, "f%d.cn" % seed)
                with open(源, "w", encoding="utf-8", newline="") as f:
                    f.write(Gen(seed).program_mixed(语句数))
                r = subprocess.run(
                    [编译路径, "build", 源, "--target", 目标, "-O0",
                     "--output", 源[:-3] + ".exe"],
                    capture_output=True, timeout=120)
                if r.returncode == 0:
                    continue
                输出 = (r.stdout + r.stderr).decode("utf-8", "replace")
                if "// 族=多维数组" in open(源, encoding="utf-8").read() \
                        and "未初始化" in 输出:
                    族红锚命中们.append((seed, "预期红锚命中"))   # 在案复红·锚对=合法
                    continue
                族坏.append(seed)
            签("1022/1740/1772 混合面编译通过（%d/11·多维数组族按预期红白名单）"
               % (11 - len(族坏)), not 族坏, "异常被拒 seed=%s" % 族坏)
            签("1772 多维数组族预期红锚命中（def-init 误拦在案·勿误 revert）",
               len(族红锚命中们) >= 1, "白名单面未命中=%s" % 族红锚命中们)

            帽子 = list(dict.fromkeys(审计))   # 去重（多样本可能同式）
            拒 = 0
            for i, 表达式 in enumerate(帽子):
                源 = os.path.join(临时, "d%d.cn" % i)
                with open(源, "w", encoding="utf-8", newline="") as f:
                    f.write("函数 主() -> 整32 {\n    整32 探针值 = %s;\n"
                            "    打印(探针值);\n    返回 0;\n}\n" % 表达式)
                r = subprocess.run(
                    [编译路径, "build", 源, "--target", 目标, "-O0",
                     "--output", 源[:-3] + ".exe"],
                    capture_output=True, timeout=120)
                输出 = (r.stdout + r.stderr).decode("utf-8", "replace")
                if r.returncode != 0 and "编译期整数溢出" in 输出:
                    拒 += 1
            签("危险表达式全被编译器拒（%d/%d·双向对拍）" % (拒, len(帽子)),
               len(帽子) >= 1 and 拒 == len(帽子))
        finally:
            shutil.rmtree(临时, ignore_errors=True)

    for 名 in 通过:
        print("  [通过] %s" % 名)
    for 名, 详情 in 失败:
        print("  [失败] %s%s" % (名, ("—— " + 详情) if 详情 else ""))
    print("selftest: %d 通过 / %d 失败" % (len(通过), len(失败)))
    return 1 if 失败 else 0



if __name__ == "__main__":  # 唯一正式入口=python scripts/cnsmith_gen.py --selftest（延迟导入架构）——本文件直跑无操作，转发防误用假绿
    raise SystemExit("请用 python scripts/cnsmith_gen.py --selftest 运行（本模块为拆出面·直跑无操作）")
