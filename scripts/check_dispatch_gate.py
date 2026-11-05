#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_dispatch_gate.py —— 分派默认/可选缺省/哨兵还原/枚举默认/旗标豁免/
else业务体机械门禁（#1959 防静默错误规范机械化三件之件1+件2 + #2074 执法面
收口 + #2089 else 退役执法面）。

权威条款集=「防静默错误零裁量十二条款」（看板 #1964·用户批准）；扫描面判据=
scripts/静默兜底评估扫描.py（#1958 评估轮·十面扫描已入库）条款①②⑥⑦正则面
**精化**（收窄交付·误报纪律：宁窄勿滥·门禁 credibility 优先）。

六面（#1959 看板备注件1+件2 + #2074 面④⑤ + #2089 面⑥）：
  面①「分派默认」（条款①②）：
    (a) 宿主 switch default 段内「顶层语句」return 字面量默认值
        （false/0/nullptr/""/''/假/空指针——true 非默认值不入表；-1 归 1640 哨兵面）；
    (b) if-else 终态裸 else 块（else 非 else-if 链延续）顶层 return 字面量（宿主）；
    (c) v2 否则块顶层 返回 假/0/空指针/空串(...)/正常(驻留(...))——默认类型兜底
        （含调用终态整64兜底「返回 正常(驻留("整64"))」·评估轮条款②⑤并入此面）；
    精化（比评估轮窄）：只判段/块「顶层语句」——嵌套 {} 内 return 不判（评估轮
    多收主源=嵌套多收）；注释/字符串内不判（掩码）；default 段定位跳嵌套 switch
    （沿评估轮）。与 check_miss_gate「else业务」面分工：彼面判 else 块业务语句
    （报诊断+收尾模式），本面只判字面量回退形态——同位置可两面同红·基线各自管。
  面②「value_or缺省」（条款⑥）：宿主 .value_or( / .value_or{ 全量检出（全禁·
    合法用例逐条走豁免清单）。
  面③「哨兵还原」（条款⑦）：`= 0 - 1;` 还原赋值（声明初始化/纯赋值）与
    `return/返回 0 - 1;` 还原写法返回——1640 负一哨兵面只抓 return -1 字面量·
    0-1 还原写法=其盲区补；判据面 ==/!= 0-1 **不入**本面（消费守卫·评估轮分型
    合法接力族·防误报）。
  面④「枚举默认」（#2074·2026-11-05 用户令「default 退役+全 case 显式+旗标
    强制」执法面收口·001 §十四）：对**枚举分派 switch**（文本特征=条件含
    getType()·本库 AST 节点类型分派惯用形态），default 段唯一合法形态=报内部
    错误并中止编译（001 §十四条文）——default 段顶层 return 且整段无报错调用
    词面（report/abort/assert/throw/terminate/exit）=生产返回值/默认值，红。
    与面①分工：面①抓 default 返**字面量默认值**；面④抓 default 返**任何值**
    （变量/调用结果/表达式·如 `default: return 兜底值;`）且段内无报错显形——
    同位置可两面同红·基线各自管。
    诚实边界：①文本级不可识 switch 条件的静态类型——枚举识别=getType() 词面
    特征（样板件形态），其余枚举分派（如 switch(op) 裸枚举变量）的「漏 case」
    归编译旗标 -Werror=switch-enum（宿主 CMakeLists 挂载）/MSVC /we4061 全量
    机械执法，本面不假装全覆盖；②诊断消息字符串被掩码抹除——报错型判定靠
    **代码调用词面**（report( 等），消息内容不参与判定；③return 与报错调用
    分处 if/else 两支的形态（报错支+合法支混合）按「段内有报错词=放行」宽判
    ——漏放面由旗标+审计承担。
  面⑤「旗标豁免」（#2074）：构建旗标（-Werror=switch-enum + MSVC /we4061/
    /we4062）的存量清偿豁免对账——每个 CN_SWITCH_ENUM_EXEMPT_BEGIN 包裹宏
    （common/switch_enum_gate.hpp·双编译器形态）必须在
    scripts/switch_enum_baseline.json 在册（键=「文件::归一 switch 行锚」）：
    代码有包裹而基线无=红「新增旗标豁免」（禁裸堆 pragma）；基线有而代码无=
    消减棘轮提示（逐处显式补 case 清偿后删包裹·--update-baseline 收账）；
    BEGIN 后窗口内无 switch=形态异常红。与面④分工：面④管「default 生产值」
    （语义形态），面⑤管「包裹↔基线一一对应」（账实一致）——旗标红点可以既
    有包裹（漏 case 靠 default 挡）又落面④基线（default 还生产值）·两账各自
    管理。
  面⑥「else业务体」（#2089·001 §14.7 要点①else 退役）：终态裸 else/否则 块内
    **非诊断语句=红**（2026-11-05 用户裁决「else 里只允许报错」收严执法·对称业务
    分支豁免被否定）。块全语句分类（词法 token 化·注释/字符串整体成 token 不误判）：
    白名单=诊断中止类（报错调用/throw/abort/assert/断言/打印到错误/cerr/stderr/
    v2 显式错误构造「返回 错误(...)=错误上报传播」）；伴随诊断的 return/返回 收尾
    放行（648 报诊断+返回收尾先例=「报内部错误中止」机械等价形态）；无诊断裸
    return（含返回错误码 false/nullptr/负数——882 面②放行·本面收严为红）=红；
    赋值/业务调用/声明/控制流=红；空块/纯分号=合法。单语句 else（无块形）同判
    （切分号·面②同口径）。与面①(b)(c) 分工：面①只抓顶层 return 字面量；与
    check_miss_gate 面②「else业务」分工：面②=882 宽白名单（返回错误码放行）·
    本面=else 彻底退役收严版——同位置可两面同红·基线各自管。

豁免通道（复用 882 登记制·同 1640 先例）：协议注释「// 静默豁免<类别>: 原因」
  （命中行同行或上一行·锚相容）+ miss_registry.json 登记条（批准=已批/存量过渡）。
  协议注释类别枚举扩「分派默认」（check_miss_gate.豁免类别枚举·#1959 任务令扩·
  登记表共用：一条豁免同时满足各门禁同位置）。

冻结基线（只减不增·照 miss/sentinel_freeze_baseline.json 先例）：
  scripts/dispatch_freeze_baseline.json，键=「文件::归一锚文本」·同键重复=计数。
  基线内位置=存量放行；新增位置=红（除非同行协议注释+登记条且批准=已批/存量过渡）；
  键消失=消减提示（自动棘轮·不改红）。基线记录「规则版本+规则指纹+生成sha」——
  规则指纹变化打印黄字提示，须复核消减后 --update-baseline 收账。

诚实边界（不假装全覆盖）：
  ① 语义级「默认值 vs 业务值」机器不可全识（default: return 0 千种语境）——本门禁
    抓形态+核留痕（业界上限），合法接力/违规嫌疑三型分型由人判（评估轮承担）；
  ② 跨行 return（return 与字面量分行）不判（只抓单行形态·可解释性优先）；
  ③ else 单语句形式（无块 else return false;）不判（评估轮同口径·只判块形）；
  ④ default 段若与 case 共享直落（fallthrough）语义不可见——段定位按标签切分·
    多收取向（收严）；
  ⑤ value_or 全禁面按条款⑥口径全量检出（不分缺省值型）——合法用例走基线冻结/
    豁免通道，不在正则层分类；
  ⑥ 本工具只读不写（--update-baseline 除外·禁手改基线）；扫描面=src/ + CN语言编译器v2/。

用法：
  python3 scripts/check_dispatch_gate.py                    # 门禁（rc=1=有红·L1 静态段）
  python3 scripts/check_dispatch_gate.py --report           # 报告模式（全列·rc=0）
  python3 scripts/check_dispatch_gate.py --json             # 机读输出（stdout）
  python3 scripts/check_dispatch_gate.py --update-baseline  # 以当前实测重写冻结基线
  python3 scripts/check_dispatch_gate.py --selftest         # 自测（临时样例树·正反两态）

退出码：0=绿；1=红（L1 硬拦）；2=用法/环境错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
仓库根 = 脚本目录.parent

# 复用既有扫描核（纪律二·不重复实现；同 check_sentinel_gate 先例）：950 拆行/
# 登记表读取 + 882 掩码/配对括号/行号/协议注释/锚相容/登记条命中/豁免类别枚举。
# 两被导入模块模块级各自重包 stdout 为 utf-8——沿用 950 的重包·本脚本不再重包。
import check_miss_defaults as 默认值扫描          # noqa: E402
import check_miss_gate as 静默门禁              # noqa: E402
import check_return_paths as 返回路径             # noqa: E402（面⑥词法复用·同面②通道）

登记表文件 = 仓库根 / "scripts" / "miss_registry.json"
基线文件 = 仓库根 / "scripts" / "dispatch_freeze_baseline.json"
旗标基线文件 = 仓库根 / "scripts" / "switch_enum_baseline.json"

扫描后缀 = {".cpp", ".hpp", ".cn"}
扫描前缀 = ("src/", "CN语言编译器v2/")
面名们 = ("分派默认", "value_or缺省", "哨兵还原", "枚举默认", "旗标豁免",
        "else业务体")

规则版本 = "1959-v2"

# ============ 面⑥判据常量（#2089 else业务体·001 §14.7 要点①） ============

# 面⑥：诊断中止类首 token 白名单（else 块内合法终态动作）
终止首词 = {"throw", "abort", "assert", "static_assert", "CN_ASSERT",
          "断言", "不可达", "panic", "Panic", "exit", "_exit", "terminate"}
# 面⑥：打印到错误通道词（语句含任一=诊断类）
打印错误词 = ("cerr", "stderr", "打印到错误", "打印警告")
# 面⑥：显式错误构造词（v2「返回 错误(...)」=错误上报传播·与面①(c) 不判
# 「返回 错误(n)=合法传播」口径一致；归一无空白后子串判定）
错误构造词 = ("错误(", "Error(", "失败(")

# ============ 判据常量（条款①②⑥⑦正则面精化 + #2074 面④） ============

# 条款①：switch 头（宿主）
switch头模式 = re.compile(r"\bswitch\s*\(")

# 面④（#2074）：枚举分派特征（条件文本含 getType() 调用=本库 AST 节点类型
#   分派惯用形态·其余枚举形态归编译旗标 -Werror=switch-enum 执法·见文档头）
枚举分派特征 = re.compile(r"getType\s*\(\s*\)")
# 面④：报错型 default 调用词面（字符串被掩码·只能认代码调用形态——诊断
#   report/断言 assert/终止 abort·terminate·exit/异常 throw）。default 段内
#   任一命中=default 报内部错误中止形态（001 §十四唯一合法形态·放行）。
default报错形态 = re.compile(
    r"\breport\s*\(|\babort\s*\(|\bassert\s*\(|\bthrow\b|"
    r"\bterminate\s*\(|\bexit\s*\(")
# 面④：default 段顶层 return（任意值·与面①字面量表正交——面④抓「任何值」）
return任意宿主 = re.compile(r"(?<![A-Za-z0-9_])return\b")
# 面⑤：旗标豁免包裹宏词面（common/switch_enum_gate.hpp 定义·BEGIN/END 成对）
包裹词 = re.compile(r"\bCN_SWITCH_ENUM_EXEMPT_BEGIN\b")

# 条款②：分支体禁字面量回退——default 段/裸 else 块「顶层语句」（宿主·掩码上扫）。
# 字面量表=默认兜底值语义（false/0/nullptr/空串/假/空指针）：true 非默认值不入表；
# -1 归 1640 负一哨兵面（不重复计）；返回 错误(n) 等显式错误构造不入表。
return字面量宿主 = re.compile(
    r"(?<![A-Za-z0-9_\u4e00-\u9fff])(?:return|返回)\s+"
    r"(?:false\b|nullptr\b|0\b|\"\"|''|假\b|空指针\b)\s*;")
else词宿主 = re.compile(r"(?<![A-Za-z0-9_])else(?![A-Za-z0-9_])")

# 条款②⑤：v2 否则块顶层返回字面量/默认类型兜底（返回 错误(n)=合法传播不判；
# 真/真值不入表=成功值；空串(...)与 正常(驻留(...))=构造默认类型值形态）。
否则词v2 = re.compile(r"(?<![\u4e00-\u9fff])否则")
返回字面量v2 = re.compile(
    r"(?<![\u4e00-\u9fffA-Za-z])返回\s+(?:假\b|空指针\b|空串\s*\(|"
    r"正常\s*\(\s*驻留\s*\()|(?<![\u4e00-\u9fffA-Za-z])返回\s+0\s*;")

# 条款⑥：宿主 optional 消费缺省（value_or 函数形与模板花括号形）
value_or模式 = re.compile(r"\.value_or\s*[\({]")

# 条款⑦：哨兵还原——赋值（= 0 - 1;·= 前禁 ==/!=/<=/>=/复合运算符·防判据误配）
还原赋值模式 = re.compile(r"(?<![=!<>+\-*/%&|^])=\s*0\s*-\s*1\s*;")
还原返回模式 = re.compile(
    r"(?<![A-Za-z0-9_\u4e00-\u9fff])(?:return|返回)\s*\(?\s*0\s*-\s*1\s*\)?\s*;")

条件跨行上限 = 400     # switch 条件文本长度上限（超出跳过·防病态）

# 规则指纹：判据常量序列化后取 sha256 前 16 位（基线记录·可复现审计·1640 先例）
_指纹件 = [
    规则版本, ",".join(面名们),
    switch头模式.pattern, return字面量宿主.pattern, else词宿主.pattern,
    否则词v2.pattern, 返回字面量v2.pattern, value_or模式.pattern,
    还原赋值模式.pattern, 还原返回模式.pattern, str(条件跨行上限),
    枚举分派特征.pattern, default报错形态.pattern, return任意宿主.pattern,
    包裹词.pattern,
    ",".join(sorted(终止首词)), ",".join(打印错误词), ",".join(错误构造词),
]
规则指纹 = hashlib.sha256(
    "|".join(str(x) for x in _指纹件).encode("utf-8")).hexdigest()[:16]


# ============ 一、扫描核 ============

def 跳空白(文本: str, i: int) -> int:
    n = len(文本)
    while i < n and 文本[i] in " \t\r\n":
        i += 1
    return i


def 段顶位(段: str, pos: int) -> bool:
    """pos 在段文本的「顶层语句」位（相对段起点的 {} 深度=0）——嵌套块内不判
    （精化核心：评估轮多收主源=default/else 段内嵌套 if/switch 的 return 多收）。"""
    深 = 0
    for k in range(pos):
        c = 段[k]
        if c == "{":
            深 += 1
        elif c == "}":
            深 -= 1
    return 深 <= 0


def 找嵌套switch区间(块体: str):
    """块体内嵌套 switch 块的区间列表（default 标签归属降噪·沿评估轮）。"""
    区间 = []
    for sm in switch头模式.finditer(块体):
        c1 = 静默门禁.配对括号(块体, sm.end() - 1)
        if c1 is None:
            continue
        k = 跳空白(块体, c1 + 1)
        if k < len(块体) and 块体[k] == "{":
            c2 = 静默门禁.配对括号(块体, k)
            if c2 is not None:
                区间.append((k, c2))
    return 区间


def 收命中(命中: list, 面: str, 相对: str, 段: str, 段基: int, 模式,
           行首表: list, 行们: list, 因: str):
    """段内顶层语句命中收集（嵌套块内 return 不判·见 段顶位）。"""
    for m in 模式.finditer(段):
        if not 段顶位(段, m.start()):
            continue
        绝对 = 段基 + m.start()
        行 = 静默门禁.取行号(行首表, 绝对)
        锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
        if 锚:
            命中.append({"面": 面, "文件": 相对, "行": 行, "锚": 锚, "因": 因})


def 扫分派默认(相对: str, 掩: str, 行首表: list, 原文: str, 命中: list):
    """面①：宿主 default 段 + 宿主裸 else 块 + v2 否则块 的顶层 return 字面量。"""
    行们 = 原文.split("\n")
    # (a) 宿主 switch default 段（嵌套 switch 标签跳一层·沿评估轮）
    for m in switch头模式.finditer(掩):
        开 = m.end() - 1
        条件闭 = 静默门禁.配对括号(掩, 开)
        if 条件闭 is None or 条件闭 - 开 > 条件跨行上限:
            continue
        j = 跳空白(掩, 条件闭 + 1)
        if j >= len(掩) or 掩[j] != "{":
            continue
        块闭 = 静默门禁.配对括号(掩, j)
        if 块闭 is None:
            continue
        块体 = 掩[j + 1:块闭]
        嵌套 = 找嵌套switch区间(块体)

        def 在嵌套(pos):
            return any(a <= pos <= b for a, b in 嵌套)

        标签 = [(mt.start(), mt.group(1)) for mt in
              re.finditer(r"\b(case|default)\b", 块体) if not 在嵌套(mt.start())]
        for idx, (p, t) in enumerate(标签):
            if t != "default":
                continue
            段终 = 标签[idx + 1][0] if idx + 1 < len(标签) else len(块体)
            收命中(命中, "分派默认", 相对, 块体[p:段终], j + 1 + p,
                 return字面量宿主, 行首表, 行们,
                 "switch default 段返字面量默认值（条款①②）")
    # (b) 宿主终态裸 else 块（else-if 链延续跳过·#else 预处理跳过·照 882 else面）
    for m in else词宿主.finditer(掩):
        if m.start() > 0 and 掩[m.start() - 1] == "#":
            continue
        j = 跳空白(掩, m.end())
        if 掩.startswith("if", j) and not 掩[j + 2:j + 3].isalnum():
            continue                     # else if：链延续（非终态）
        if j >= len(掩) or 掩[j] != "{":
            continue                     # 只判块形（诚实边界③）
        块闭 = 静默门禁.配对括号(掩, j)
        if 块闭 is None:
            continue
        收命中(命中, "分派默认", 相对, 掩[j + 1:块闭], j + 1,
             return字面量宿主, 行首表, 行们,
             "裸 else 块承载 return 字面量（条款②）")
    # (c) v2 否则块（否则如果 链延续跳过）
    for m in 否则词v2.finditer(掩):
        j = 跳空白(掩, m.end())
        if 掩.startswith("如果", j):
            continue                     # 否则 如果：链延续
        if j >= len(掩) or 掩[j] != "{":
            continue
        块闭 = 静默门禁.配对括号(掩, j)
        if 块闭 is None:
            continue
        收命中(命中, "分派默认", 相对, 掩[j + 1:块闭], j + 1,
             返回字面量v2, 行首表, 行们,
             "否则块承载返回字面量/默认类型兜底（条款②⑤）")


def 扫枚举默认(相对: str, 掩: str, 行首表: list, 原文: str, 命中: list):
    """面④（#2074）：枚举分派 switch（条件含 getType()）的 default 段——
    顶层 return 且整段无报错调用词面=生产返回值/默认值（001 §十四唯一合法
    形态=报内部错误中止编译）。段定位复用 找嵌套switch区间/段顶位（与面①(a)
    同构·跳嵌套 switch 标签）。"""
    行们 = 原文.split("\n")
    for m in switch头模式.finditer(掩):
        开 = m.end() - 1
        条件闭 = 静默门禁.配对括号(掩, 开)
        if 条件闭 is None or 条件闭 - 开 > 条件跨行上限:
            continue
        if not 枚举分派特征.search(掩[开 + 1:条件闭]):
            continue                       # 非识别枚举分派面（诚实边界①）
        j = 跳空白(掩, 条件闭 + 1)
        if j >= len(掩) or 掩[j] != "{":
            continue
        块闭 = 静默门禁.配对括号(掩, j)
        if 块闭 is None:
            continue
        块体 = 掩[j + 1:块闭]
        嵌套 = 找嵌套switch区间(块体)

        def 在嵌套(pos):
            return any(a <= pos <= b for a, b in 嵌套)

        标签 = [(mt.start(), mt.group(1)) for mt in
              re.finditer(r"\b(case|default)\b", 块体) if not 在嵌套(mt.start())]
        for idx, (p, t) in enumerate(标签):
            if t != "default":
                continue
            段终 = 标签[idx + 1][0] if idx + 1 < len(标签) else len(块体)
            段 = 块体[p:段终]
            if default报错形态.search(段):
                continue                   # 报错型 default=001 §十四合法形态
            收命中(命中, "枚举默认", 相对, 段, j + 1 + p,
                 return任意宿主, 行首表, 行们,
                 "枚举分派 default 段生产返回值且无报错显形（#2074·001 §十四）")


def 扫旗标豁免(相对: str, 掩: str, 行首表: list, 原文: str, 命中: list):
    """面⑤（#2074）：旗标豁免包裹对账——每个 CN_SWITCH_ENUM_EXEMPT_BEGIN
    位置入候选（基线在册=放行·无=红「新增旗标豁免」）。锚=BEGIN 后最近
    switch 头行文本（包裹形态约定 BEGIN/注释/switch 相邻·窗口 5 行外无
    switch=形态异常·锚记异常文本）。"""
    行们 = 原文.split("\n")
    掩行们 = 掩.split("\n")
    for m in 包裹词.finditer(掩):
        行 = 静默门禁.取行号(行首表, m.start())
        if 行们[行 - 1].lstrip().startswith("#define"):
            continue     # 宏定义原语行（switch_enum_gate.hpp 双平台 #define）
        锚 = ""
        for k in range(行 - 1, min(行 + 4, len(掩行们))):
            if switch头模式.search(掩行们[k]):
                锚 = 行们[k].strip()
                break
        因 = "旗标豁免包裹（漏 case 存量清偿位）"
        if not 锚:
            锚 = "CN_SWITCH_ENUM_EXEMPT_BEGIN 后无 switch（形态异常）"
            因 = "包裹形态异常：BEGIN 窗口内无 switch 头行"
        命中.append({"面": "旗标豁免", "文件": 相对, "行": 行,
                   "锚": 锚, "因": 因})


# ============ 面⑥·else业务体（#2089·001 §14.7 要点①else 退役） ============

def 分类语句六(语: list) -> str:
    """面⑥语句分类：诊断|返回|其他（else 退役收严白名单·机械判定定律甲）。

    与 check_miss_gate.分类语句（面②882 宽白名单）重复·待归纳——本面收严版
    分类器：无「返回错误码放行」槽（返回 错误构造=诊断上报入白名单·其余返回
    统归「返回」由状态机判诊断伴随），无 1360 .value() 取值豁免（取值赋值在
    else 块内仍属业务·从严基线兜底）。"""
    if not 语:
        return "空"
    首 = 语[0]
    if 首 in ("return", "返回"):
        if 错误构造词[0] in "".join(语[1:]):
            return "诊断"                  # 返回 错误(...)=错误上报传播
        return "返回"
    if 首 in 终止首词:
        return "诊断"
    文本 = "".join(语)
    if any(w in 文本 for w in 打印错误词):
        return "诊断"                      # 打印到错误通道=诊断类
    if any(子串 in 名 or 名 in 子串 for 名 in 静默门禁.调用名们(语)
           for 子串 in 静默门禁.报错调用名):
        return "诊断"                      # 报错/收集诊断类调用
    return "其他"


def 判else块六(块源: str):
    """面⑥块源→(是否红, 首违语句文本)。状态机：无「其他」且（无「返回」或
    有「诊断」）→ 合法（伴随诊断的 return 收尾=报内部错误中止机械等价·放行）。"""
    语们 = 静默门禁.切语句(返回路径.词法(块源))
    有诊断 = False
    返回语 = None
    for 语 in 语们:
        类 = 分类语句六(语)
        if 类 == "诊断":
            有诊断 = True
        elif 类 == "返回" and 返回语 is None:
            返回语 = "".join(语)
        elif 类 == "其他":
            return True, "".join(语)
    if 返回语 is not None and not 有诊断:
        return True, 返回语
    return False, ""


def 扫else业务体(相对: str, 掩: str, 行首表: list, 原文: str, 命中: list):
    """面⑥：终态裸 else/否则 块内非诊断语句（else 退役·同位置与面①(b)(c)/
    面②可同红·基线各自管）。单语句 else（无块形）切分号同判（面②口径）。"""
    行们 = 原文.split("\n")

    def 收红(位: int, 违: str):
        行 = 静默门禁.取行号(行首表, 位)
        锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
        if 锚:
            命中.append({"面": "else业务体", "文件": 相对, "行": 行,
                       "锚": 锚 + " │ " + 违[:80],
                       "因": "else/否则 块承载非诊断语句（001 §14.7 要点①）"})

    # (a) 宿主 else（#else 预处理跳过·else-if 链延续跳过）
    for m in else词宿主.finditer(掩):
        if m.start() > 0 and 掩[m.start() - 1] == "#":
            continue
        j = 跳空白(掩, m.end())
        if 掩.startswith("if", j) and not 掩[j + 2:j + 3].isalnum():
            continue                     # else if：链延续（非终态）
        首 = 掩[j] if j < len(掩) else ""
        if 首 == "{":
            块闭 = 静默门禁.配对括号(掩, j)
            if 块闭 is None:
                continue
            红, 违 = 判else块六(原文[j + 1:块闭])
            if 红:
                收红(m.start(), 违)
        elif 首 and 首 != ";":
            k = 掩.find(";", j)
            if k < 0 or k - j > 条件跨行上限:
                continue
            红, 违 = 判else块六(原文[j:k])
            if 红:
                收红(m.start(), 违)

    # (b) v2 否则（否则如果 链延续跳过）
    for m in 否则词v2.finditer(掩):
        j = 跳空白(掩, m.end())
        if 掩.startswith("如果", j):
            continue                     # 否则 如果：链延续
        首 = 掩[j] if j < len(掩) else ""
        if 首 == "{":
            块闭 = 静默门禁.配对括号(掩, j)
            if 块闭 is None:
                continue
            红, 违 = 判else块六(原文[j + 1:块闭])
            if 红:
                收红(m.start(), 违)
        elif 首 and 首 != ";":
            k = 掩.find(";", j)
            if k < 0 or k - j > 条件跨行上限:
                continue
            红, 违 = 判else块六(原文[j:k])
            if 红:
                收红(m.start(), 违)


def 扫value_or(相对: str, 掩: str, 行首表: list, 原文: str, 命中: list):
    """面②：宿主 .value_or( / .value_or{ 全量（条款⑥全禁·不分缺省值型）。"""
    行们 = 原文.split("\n")
    for m in value_or模式.finditer(掩):
        行 = 静默门禁.取行号(行首表, m.start())
        锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
        if 锚:
            命中.append({"面": "value_or缺省", "文件": 相对, "行": 行,
                       "锚": 锚, "因": "宿主 optional 消费缺省（条款⑥全禁）"})


def 扫哨兵还原(相对: str, 掩: str, 行首表: list, 原文: str, 命中: list):
    """面③：= 0 - 1; 还原赋值 + return/返回 0 - 1; 还原写法返回（条款⑦）。"""
    行们 = 原文.split("\n")
    for 模式, 因 in ((还原赋值模式, "0-1 哨兵还原赋值（条款⑦）"),
                  (还原返回模式, "0-1 哨兵还原写法返回（条款⑦·1640 盲区补）")):
        for m in 模式.finditer(掩):
            行 = 静默门禁.取行号(行首表, m.start())
            锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
            if 锚:
                命中.append({"面": "哨兵还原", "文件": 相对, "行": 行,
                           "锚": 锚, "因": 因})


# ============ 二、单文件/全库扫描 ============

def 扫文件(相对: str, 文本: str):
    """单文件三面扫描。返回 (面点列表, 协议点们)。"""
    行们 = 默认值扫描.拆行(文本)
    掩 = 静默门禁.掩码(文本)
    行首表 = 静默门禁.建行首表(文本)
    协议点们 = 静默门禁.扫协议注释(相对, 行们)
    命中 = []
    扫分派默认(相对, 掩, 行首表, 文本, 命中)
    扫枚举默认(相对, 掩, 行首表, 文本, 命中)
    扫value_or(相对, 掩, 行首表, 文本, 命中)
    扫哨兵还原(相对, 掩, 行首表, 文本, 命中)
    扫else业务体(相对, 掩, 行首表, 文本, 命中)
    扫旗标豁免(相对, 掩, 行首表, 文本, 命中)
    for 面 in 面名们:                       # 同位置同锚重复=去重（计数由键承载）
        去重 = {}
        for 点 in 命中:
            if 点["面"] == 面:
                去重.setdefault((点["行"], 静默门禁.归一(点["锚"])), 点)
        命中 = [x for x in 命中 if x["面"] != 面] + list(去重.values())
    return 命中, 协议点们


def 全库扫描(根: Path):
    """扫 src/ 与 CN语言编译器v2/（三面·882/1640 同扫描面口径）。"""
    候选 = {面: [] for 面 in 面名们}
    协议点们 = []
    文件们 = {}                          # 相对路径 → 归一全文（登记条对账用）
    统计 = {"文件": 0, "读取失败": 0}
    for 前缀 in 扫描前缀:
        base = 根 / 前缀.rstrip("/")
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if not p.is_file() or p.suffix not in 扫描后缀:
                continue
            相对 = 前缀 + p.relative_to(base).as_posix()
            try:
                文本 = p.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                统计["读取失败"] += 1
                continue
            统计["文件"] += 1
            文件们[相对] = 静默门禁.归一(文本)
            命中此, 协议此 = 扫文件(相对, 文本)
            for 点 in 命中此:
                候选[点["面"]].append(点)
            协议点们.extend(协议此)
    return {"候选": 候选, "协议点": 协议点们, "文件文本": 文件们, "统计": 统计}


# ============ 三、基线读写 ============

def 归一(文本: str) -> str:
    return re.sub(r"\s+", "", 文本 or "")


def 点键(点: dict) -> str:
    return 点["文件"] + "::" + 归一(点["锚"])


def 读基线(路径: Path):
    """读冻结基线；缺失=异常（门禁拒跑·防绕过·照 882）；损坏=异常。"""
    if not 路径.exists():
        raise SystemExit(f"[门禁] 基线缺失：{路径.name}——先跑 "
                         f"python3 scripts/check_dispatch_gate.py --update-baseline 生成")
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁] 基线损坏（{e.__class__.__name__}）：{路径.name}")
    指纹 = 数据.get("规则指纹", "")
    return {面: Counter(数据.get("面", {}).get(面, [])) for 面 in 面名们}, 指纹


def 写基线(路径: Path, 扫描果: dict) -> dict:
    面 = {名: sorted(点键(p) for p in 扫描果["候选"][名])
         for 名 in 面名们 if 名 != "旗标豁免"}   # 旗标豁免独立基线（写旗标基线）
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=路径.parent.parent,
                             capture_output=True, text=True, timeout=10).stdout.strip()
    except Exception:
        sha = ""
    数据 = {
        "版本": 1,
        "工具": "scripts/check_dispatch_gate.py",
        "任务": "1959",
        "规则版本": 规则版本,
        "规则指纹": 规则指纹,
        "说明": ("#1959 分派默认/value_or/哨兵还原 + #2089 else业务体冻结基线"
               "（只减不增）：键=「文件::归一锚文本」，同键重复=计数。基线内=存量放行；"
               "新增位置=红（除非协议注释+miss_registry.json 登记条且批准=已批/存量"
               "过渡·类别可用新扩「分派默认」）；键消失=消减提示。手改基线=绕过门禁"
               "行为·禁（收账=--update-baseline·仅确认清偿后）。"),
        "生成sha": sha,
        "面": 面,
    }
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1),
                    encoding="utf-8")
    return {名: len(面[名]) for 名 in 面}


def 读旗标基线(路径: Path):
    """读旗标豁免基线（面⑤·#2074）；缺失=异常（防绕过·同 dispatch 基线口径）；
    损坏=异常。返回 Counter（键=文件::归一锚）。"""
    if not 路径.exists():
        raise SystemExit(f"[门禁] 旗标基线缺失：{路径.name}——包裹与基线须同轮"
                         f"落库（#2074 首挂轮先例）或 --update-baseline 生成")
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁] 旗标基线损坏（{e.__class__.__name__}）："
                         f"{路径.name}")
    return Counter(条["文件"] + "::" + 归一(条.get("锚", ""))
                   for 条 in 数据.get("条目", []))


def 写旗标基线(路径: Path, 扫描果: dict) -> int:
    """以现测重写旗标豁免基线（--update-baseline 双写之一）。"""
    条目 = sorted(扫描果["候选"]["旗标豁免"],
                  key=lambda x: (x["文件"], x["行"]))
    数据 = {
        "版本": 1,
        "工具": "scripts/check_dispatch_gate.py（面⑤对账）",
        "任务": "2074",
        "说明": ("#2074 枚举 switch 穷举旗标豁免基线（只减不增）："
                 "键=「文件::归一锚文本」。每个 CN_SWITCH_ENUM_EXEMPT_BEGIN "
                 "包裹必须在册；清偿（显式补 case 删包裹）后键消失=消减，"
                 "--update-baseline 收账。新增包裹禁裸堆（未登记=红）。"),
        "条目": [{"文件": p["文件"], "行": p["行"], "锚": p["锚"],
                 "因由": p["因"]} for p in 条目],
    }
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1),
                    encoding="utf-8")
    return len(条目)


# ============ 四、判定引擎（三红·同 1640 口径） ============

def 同行协议(协议点们: list, 行号: int, 锚: str):
    for 协议 in 协议点们:
        if 协议["锚定行"] == 行号 and 静默门禁.锚相容(协议["锚"], 锚):
            return 协议
    return None


def 批准问题(条: dict):
    """#1238 批准制复用（同 1640）：返回 (红种类, 说明) 或 None。"""
    if 条 is None:
        return ("漏登记", "代码有协议注释而 miss_registry.json 无条——补登记条目")
    批 = (条.get("批准") or "").strip()
    if 批 == "已毙":
        return ("已毙未清偿", "登记条批准=已毙而兜底代码仍在——清偿后删条")
    if 批 not in ("已批", "存量过渡"):
        return ("登记无批准",
                f"登记条批准状态={批 or '（缺字段）'}≠已批/存量过渡——"
                f"#1238 兜底禁令：新豁免须呈报批准")
    return None


def 判定(扫描果: dict, 条目们: list, 基线: dict):
    """三红：①新增无豁免 ②漏登记 ③类别非法/登记无批准/已毙未清偿（1640 口径）。"""
    红们, 豁免们, 消减, 面统 = [], [], {}, {}
    for 面 in 面名们:
        点们 = 扫描果["候选"][面]
        现计 = {}
        for 点 in 点们:
            现计.setdefault(点键(点), []).append(点)
        基计 = 基线.get(面, Counter())
        新增 = []
        for 键 in sorted(现计):
            b = 基计.get(键, 0)
            序 = sorted(现计[键], key=lambda x: (x["行"], x["锚"]))
            if len(序) > b:
                新增.extend(序[b:])        # 前 b 个视作基线存量·超出的才是新增
        减 = []
        for 键, b in sorted(基计.items()):
            c = len(现计.get(键, []))
            if c < b:
                减.extend([键] * (b - c))
        消减[面] = 减
        放行 = 红数 = 0
        for 点 in 新增:
            协议 = 同行协议(扫描果["协议点"], 点["行"], 点["锚"])
            if 协议 is not None:
                if 协议["类别"] not in 静默门禁.豁免类别枚举:
                    红们.append({"种类": "协议注释类别非法", "文件": 点["文件"],
                               "行": 点["行"], "锚": 点["锚"][:120],
                               "说明": f"类别「{协议['类别']}」不在枚举 "
                                     f"{sorted(静默门禁.豁免类别枚举)}"})
                    红数 += 1
                    continue
                条 = 静默门禁.条命中(点["文件"], 点["锚"], 条目们)
                问题 = 批准问题(条)
                if 问题 is not None:
                    红们.append({"种类": 问题[0], "文件": 点["文件"], "行": 点["行"],
                               "锚": 点["锚"][:120], "说明": 问题[1]})
                    红数 += 1
                    continue
                放行 += 1
                豁免们.append({"面": 面, "文件": 点["文件"], "行": 点["行"],
                            "锚": 点["锚"][:100], "豁免": "协议注释+登记条目"})
                continue
            条 = 静默门禁.条命中(点["文件"], 点["锚"], 条目们)
            if 条 is not None:
                红们.append({"种类": "新增位置缺协议注释", "文件": 点["文件"],
                           "行": 点["行"], "锚": 点["锚"][:120],
                           "说明": f"{面}：登记条目在但代码无协议注释——补"
                                 f"「// 静默豁免<类别>: 原因」"})
            else:
                红们.append({"种类": "新增" + 面, "文件": 点["文件"], "行": 点["行"],
                           "锚": 点["锚"][:120],
                           "说明": f"新增分派默认/缺省/还原/枚举默认/else业务体位置"
                                 f"无登记（{面}）——改写为显式报错/守卫式"
                                 f"（001 §14.7 三式），或走协议注释+登记条通道"})
            红数 += 1
        面统[面] = {"现测": len(点们), "基线": sum(基计.values()), "新增": len(新增),
                   "豁免放行": 放行, "红": 红数, "消减": len(减)}
    return {"红": 红们, "豁免": 豁免们, "消减": 消减, "面统": 面统,
            "条目数": len(条目们), "协议点数": len(扫描果["协议点"])}


# ============ 五、输出 ============

def 打印红们(红们: list):
    排序 = sorted(红们, key=lambda r: (r["种类"], r["文件"], r["行"]))
    print(f"\n【红清单】（{len(排序)} 条·L1 硬拦）")
    for r in 排序:
        位 = f"{r['文件']}:{r['行']}" if r.get("行") else r["文件"]
        print(f"  [×] ({r['种类']}) {位}")
        if r.get("锚"):
            print(f"      锚：{r['锚'][:110]}")
        print(f"      处置：{r['说明']}")


def 打印报告(扫描果: dict, 判定果: dict, 指纹: str, 详列: bool):
    统计 = 扫描果["统计"]
    print("== 分派默认/可选缺省/哨兵还原/枚举默认/旗标豁免/else业务体门禁"
          "（check_dispatch_gate.py·#1959+#2074+#2089）==")
    print(f"扫描面：src/ + CN语言编译器v2/（文件 {统计['文件']}·读取失败 "
          f"{统计['读取失败']}）；规则 {规则版本} 指纹 {规则指纹}；"
          f"登记表条目 {判定果['条目数']} 条；协议注释点 {判定果['协议点数']} 处")
    if 指纹 and 指纹 != 规则指纹:
        print(f"[提示] 基线规则指纹 {指纹} ≠ 当前 {规则指纹}——判据已变更，"
              f"复核消减面后 --update-baseline 收账")
    print("【面级统计】面 | 现测 | 基线 | 新增 | 豁免放行 | 红 | 消减")
    for 面 in 面名们:
        u = 判定果["面统"][面]
        print(f"  {面} | {u['现测']} | {u['基线']} | {u['新增']} | "
              f"{u['豁免放行']} | {u['红']} | {u['消减']}")
    for 面 in 面名们:
        减 = 判定果["消减"].get(面, [])
        if 减:
            print(f"  [↓] {面} 基线位置消失 {len(减)} 处（自动消减·确认无误报后 "
                  f"--update-baseline 收基线）：{减[:3]}…")
    if 判定果["豁免"]:
        print(f"\n【豁免放行】（{len(判定果['豁免'])} 条·协议注释+登记条目）")
        for h in 判定果["豁免"][:10]:
            print(f"  [✓] {h['文件']}:{h['行']}（{h['面']}）{h['锚'][:80]}")
        if len(判定果["豁免"]) > 10:
            print(f"  …（其余 {len(判定果['豁免']) - 10} 条）")
    if 详列:
        print("\n【在测位置明细】（--report·全列）")
        for 面 in 面名们:
            print(f"-- {面}（{len(扫描果['候选'][面])}）--")
            for 点 in sorted(扫描果["候选"][面], key=lambda x: (x["文件"], x["行"])):
                print(f"  {点['文件']}:{点['行']}  {点['锚'][:100]}")
                print(f"      因：{点['因']}")
    if 判定果["红"]:
        打印红们(判定果["红"])
        print(f"\n[门禁·红] {len(判定果['红'])} 条——L1 硬拦；唯一合法通道="
              f"协议注释+miss_registry.json 登记条（批准=已批/存量过渡），或原地清偿。")
    else:
        print("\n[门禁·绿] 六面零红（存量冻结只减不增·新增面严）")


def 转json(判定果: dict, 扫描果: dict) -> str:
    输出 = {"工具": "check_dispatch_gate.py", "任务": "1959+2074+2089",
          "规则版本": 规则版本, "规则指纹": 规则指纹,
          "统计": 扫描果["统计"], "面统": 判定果["面统"],
          "条目数": 判定果["条目数"], "协议点数": 判定果["协议点数"],
          "豁免": 判定果["豁免"],
          "消减": {k: v[:50] for k, v in 判定果["消减"].items()},
          "红": 判定果["红"]}
    return json.dumps(输出, ensure_ascii=False, indent=1)


# ============ 七、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="分派默认/可选缺省/哨兵还原/枚举默认/旗标豁免/else业务体机械门禁"
                    "（#1959·#2074·#2089·六面+冻结基线）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--registry", default=str(登记表文件), help="登记表路径")
    解析器.add_argument("--baseline", default=str(基线文件), help="冻结基线路径")
    解析器.add_argument("--flag-baseline", default=str(旗标基线文件),
                      help="旗标豁免基线路径（面⑤·#2074 独立基线）")
    解析器.add_argument("--report", action="store_true", help="报告模式（全列·rc=0 不拦）")
    解析器.add_argument("--json", action="store_true", help="机读输出（stdout）")
    解析器.add_argument("--update-baseline", action="store_true",
                      help="以当前实测重写冻结基线")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        import dispatch_gate_selftest as 自测件         # 延迟导入（selftest 拆件·非 check_ 前缀=工具类自测件不注册清单·ci_daemon_selftest 先例）
        return 自测件.自测()

    根 = Path(参数.root).resolve()
    扫描果 = 全库扫描(根)
    if 参数.update_baseline:
        统计 = 写基线(Path(参数.baseline), 扫描果)
        旗数 = 写旗标基线(Path(参数.flag_baseline), 扫描果)
        print(f"[基线] 已重写 {Path(参数.baseline).name}："
              + " · ".join(f"{面} {统计[面]}" for 面 in 面名们 if 面 in 统计)
              + f" · 旗标豁免 {旗数}（独立基线 {Path(参数.flag_baseline).name}·#2074）")
        return 0
    条目们, 表态 = 默认值扫描.读登记表(Path(参数.registry))
    if 表态 != "就绪":
        print(f"[门禁·红] 登记表状态异常：{表态}（{参数.registry}）")
        return 1
    基线, 指纹 = 读基线(Path(参数.baseline))
    基线["旗标豁免"] = 读旗标基线(Path(参数.flag_baseline))   # 面⑤并入判定（#2074）
    判定果 = 判定(扫描果, 条目们, 基线)

    if 参数.json:
        print(转json(判定果, 扫描果))
    else:
        打印报告(扫描果, 判定果, 指纹, 详列=参数.report)
    if 参数.report:
        return 0
    return 1 if 判定果["红"] else 0


if __name__ == "__main__":
    sys.exit(main())
