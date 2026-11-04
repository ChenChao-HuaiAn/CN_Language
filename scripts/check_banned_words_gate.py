#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_banned_words_gate.py —— 注释禁词机械门禁（#1959·防静默错误规范机械化三件
之件3·条款⑪执法面）。

权威条款集=「防静默错误零裁量十二条款」（看板 #1964·用户批准）；禁词表=
scripts/静默兜底评估扫描.py（#1958 评估轮）条款⑪注释禁词**全表**（十一词）——
#1959 任务令九词 ∪ 评估轮补「保守放行」「best-effort」两变体：
  诚实边界 / 保守豁免 / 保守不判 / 保守放行 / 宁漏勿崩 / 宁漏勿误 /
  回退 / 兜底 / fallback / best-effort / best effort
（英文词大小写不敏感；比 #882 红旗词面 4 词更全·红旗词面是「词+语境」组合面·
本面是裸词面红线·两者互补不互替）。

扫描面：src/ + CN语言编译器v2/ 的 .cpp/.hpp/.cn 注释段（// 与 /* */·字符串
内容不判）。scripts/ 不扫（门禁/基建脚本自身的立法文档注释合法使用禁词——
扫了即自指灾难·98% 假红）。

放行通道（#1959 任务令：同行带「命名豁免/登记条」引用放行）：
  命中注释段文本含「命名豁免」（1716 协议注释）/「登记条」（miss_registry 登记
  条目引用）/「静默豁免」（882 协议注释·登记条代码侧留痕·与登记条成对）任一
  →登记通道引用·放行。三词=既有登记机制的全部代码侧留痕形态。

冻结基线（只减不增·照 naming_freeze_baseline.json 先例）：
  scripts/banned_words_freeze_baseline.json，键=「文件::词::归一注释段」→次数。
  新增键=红；同键计数增长=红；键消失/计数下降=绿（提示可 --freeze 收缩）。
  基线缺失=红拒跑（防绕过·照 check_naming）。首次落地=以当前实测生成基线
  （评估轮分型：存量大头=648/1360/882 登记性注释的在案自认·豁免清单素材非
  违规——冻结放行·逐批清偿挂后续；新增注释写禁词即红·逼走登记通道）。

诚实边界（不假装全覆盖）：
  ① 裸词面红线不判语义（「兜底」出现在否定语境如「禁兜底」也命中）——存量由
    基线冻结兜住，新增一律走引用放行或改写；机械面可解释性优先；
  ② 跨行块注释整段收集（行号=段起始行）——段内同词多次出现计 1（键=段·计数
    按「文件::词::段」同段去重）；
  ③ 字符串字面量内的禁词不判（只扫注释段·掩码式收集跳字符串）；
  ④ .cn 文件的三引号字符串内的 // 不误判为注释（串收集先行·同评估轮收注释段）；
  ⑤ 本工具只读不写（--freeze 除外·禁手改基线）。

用法：
  python3 scripts/check_banned_words_gate.py            # 门禁（rc=1=有红·L1 静态段）
  python3 scripts/check_banned_words_gate.py --report   # 报告模式（按词统计·rc=0）
  python3 scripts/check_banned_words_gate.py --freeze   # 以当前实测重写冻结基线
  python3 scripts/check_banned_words_gate.py --selftest # 自测（临时样例树·正反两态）

退出码：0=绿；1=红（有新增/增长/基线缺失/读取失败）；2=用法错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
import json
import re
import sys
import tempfile
from collections import Counter
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
仓库根 = 脚本目录.parent

# 复用既有扫描核（纪律二·不重复实现；同 1640/1959 先例）：882 串尾/建行首表/
# 取行号。两被导入模块模块级各自重包 stdout 为 utf-8——沿用其重包·本脚本不再重包。
import check_miss_gate as 静默门禁              # noqa: E402

基线文件 = 脚本目录 / "banned_words_freeze_baseline.json"

# ============ 常量与判据表 ============

扫描后缀 = {".cpp", ".hpp", ".cn"}
扫描前缀 = ("src/", "CN语言编译器v2/")

# 条款⑪禁词全表（#1958 评估轮口径·十一词；英文大小写不敏感）
禁词表 = ("诚实边界", "保守豁免", "保守不判", "保守放行", "宁漏勿崩", "宁漏勿误",
        "回退", "兜底", "fallback", "best-effort", "best effort")

# 放行通道引用词（同行=命中注释段文本内·#1959 任务令 + 882 协议注释实态）
引用放行词 = ("命名豁免", "登记条", "静默豁免")


# ============ 一、注释段收集（字符串跳过·防 URL // 与文案内禁词误判） ============

def 收注释段(文本: str):
    """收集注释段 [(起偏移, 段文本)]——字符串内容跳过（同 静默兜底评估扫描 收注释段）。"""
    段们 = []
    n = len(文本)
    i = 0
    while i < n:
        c = 文本[i]
        if c == '"' or c == "'":
            i = 静默门禁.串尾(文本, i, True)
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            j = n if j < 0 else j
            段们.append((i, 文本[i:j]))
            i = j
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            j = n if j < 0 else j + 2
            段们.append((i, 文本[i:j]))
            i = j
            continue
        i += 1
    return 段们


def 注释净(段文本: str) -> str:
    """剥注释符（//、/*、*、*/）后的净文本（键/展示用）。"""
    return re.sub(r"^(?://+|/\*+|\*+|\*/)\s*", "", 段文本.strip()).strip()


# ============ 二、单文件扫描 ============

def 扫内容(相对: str, 原文: str, 行首表: list) -> list:
    """注释段×禁词表逐词查·引用放行先行。命中=[{文件,行,词,锚}]。"""
    命中们 = []
    快筛 = False
    for 词 in 禁词表:
        if 词 in 原文 or (词.isascii() and 词 in 原文.lower()):
            快筛 = True
            break
    if not 快筛:
        return 命中们                     # 原文无任何禁词（含大小写变体）=零扫描
    for 起, 段文本 in 收注释段(原文):
        小写 = 段文本.lower()
        if any(词 in 段文本 for 词 in 引用放行词):
            continue                     # 登记通道引用·放行（#1959 任务令）
        行 = 静默门禁.取行号(行首表, 起)
        锚 = 注释净(段文本)
        for 词 in 禁词表:
            if (词 in 小写) if 词.isascii() else (词 in 段文本):
                命中们.append({"文件": 相对, "行": 行, "词": 词, "锚": 锚})
    return 命中们


def 枚举文件(根: Path):
    for 前缀 in 扫描前缀:
        base = 根 / 前缀.rstrip("/")
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if not p.is_file() or p.suffix not in 扫描后缀:
                continue
            yield 前缀 + p.relative_to(base).as_posix(), p


def 全库扫描(根: Path) -> dict:
    命中们 = []
    统计 = {"文件": 0, "读取失败": []}
    for 相对, p in 枚举文件(根):
        统计["文件"] += 1
        try:
            原文 = p.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as e:
            统计["读取失败"].append((相对, e.__class__.__name__))
            continue
        命中们.extend(扫内容(相对, 原文, 静默门禁.建行首表(原文)))
    return {"命中": 命中们, "统计": 统计}


# ============ 三、基线读写与判定（naming 先例·计数棘轮） ============

def 建键(命中: dict) -> str:
    return 命中["文件"] + "::" + 命中["词"] + "::" + 静默门禁.归一(命中["锚"])


def 读基线(路径: Path):
    """读冻结基线（JSON 对象·「键」字段=键→次数）；缺失返回 None；损坏=退出。"""
    if not 路径.exists():
        return None
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[禁词门禁] 基线损坏（{e.__class__.__name__}）：{路径}")
    if not isinstance(数据, dict) or not isinstance(数据.get("键"), dict):
        raise SystemExit(f"[禁词门禁] 基线格式非法（缺「键」对象）：{路径}")
    计 = Counter()
    for 键, 次 in 数据["键"].items():
        if not isinstance(键, str) or 键.count("::") < 2:
            raise SystemExit(f"[禁词门禁] 基线键非法：{键!r}")
        if not isinstance(次, int) or isinstance(次, bool) or 次 <= 0:
            raise SystemExit(f"[禁词门禁] 基线计数非法：{键}={次!r}")
        计[键] = 次
    return 计


def 写基线(路径: Path, 当前: Counter) -> int:
    数据键 = {键: 当前[键] for 键 in sorted(当前) if 当前[键] > 0}
    try:
        import subprocess
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=str(仓库根),
                             capture_output=True, text=True, timeout=10).stdout.strip()
    except Exception:
        sha = ""
    数据 = {
        "版本": 1,
        "工具": "scripts/check_banned_words_gate.py",
        "任务": "1959",
        "说明": ("#1959 注释禁词冻结基线（只减不增·条款⑪）：键=「文件::词::归一注释段」"
               "→次数。新增键/计数增长=红；键消失/计数下降=绿（--freeze 收缩）。"
               "同行带「命名豁免/登记条/静默豁免」引用=登记通道放行不入基线。"
               "存量大头=648/1360/882 登记性注释在案自认（评估轮 #1958 分型）——"
               "冻结放行逐批清偿。手改基线=绕过门禁行为·禁。"),
        "生成sha": sha,
        "键数": len(数据键),
        "键": 数据键,
    }
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1) + "\n",
                 encoding="utf-8")
    return len(数据键)


def 判定(当前: Counter, 基线: Counter) -> dict:
    """新键=红·计数增长=红·消失/下降=绿（可收缩）。"""
    新键 = sorted(set(当前) - set(基线))
    增长 = sorted((键, 基线[键], 当前[键]) for 键 in set(当前) & set(基线)
                if 当前[键] > 基线[键])
    消失 = sorted(set(基线) - set(当前))
    下降 = sorted((键, 基线[键], 当前[键]) for 键 in set(当前) & set(基线)
                if 当前[键] < 基线[键])
    return {"新键": 新键, "增长": 增长, "消失": 消失, "下降": 下降,
            "红": bool(新键 or 增长)}


# ============ 四、报告输出 ============

def 打印扫描摘要(扫描果: dict):
    统计 = 扫描果["统计"]
    计 = Counter(h["词"] for h in 扫描果["命中"])
    文计 = Counter(h["文件"] for h in 扫描果["命中"])
    print(f"[扫描] 文件 {统计['文件']} · 禁词命中 {len(扫描果['命中'])} 处 · "
          f"唯一键 {len({建键(h) for h in 扫描果['命中']})}")
    print(f"[按词] " + " · ".join(f"{词} {计[词]}" for 词 in 禁词表 if 词 in 计))
    print("[top 文件]")
    for 文件, 次数 in 文计.most_common(10):
        print(f"  {次数:6d}  {文件}")


def 打印读取失败(扫描果: dict):
    失败 = 扫描果["统计"]["读取失败"]
    if not 失败:
        return
    print(f"[禁词门禁·红] 读取失败 {len(失败)} 文件（无法核验·不静默跳过）：")
    for 相对, 类 in 失败[:20]:
        print(f"  ! {相对}（{类}）")


def 打印门禁(判定果: dict, 当前: Counter):
    if 判定果["红"]:
        if 判定果["新键"]:
            print(f"[禁词门禁·红] 新增禁词注释 {len(判定果['新键'])} 键"
                  f"（{sum(当前[键] for 键 in 判定果['新键'])} 处）：")
            for 键 in 判定果["新键"][:30]:
                print(f"  + {键[:160]} ×{当前[键]}")
        if 判定果["增长"]:
            print(f"[禁词门禁·红] 既有禁词注释出现次数增加 {len(判定果['增长'])} 键：")
            for 键, 旧, 新 in 判定果["增长"][:30]:
                print(f"  ~ {键[:150]} {旧}→{新}")
        print("[禁词门禁] 条款⑪：注释禁写保守面/兜底/回退词——修复=改写为事实陈述；"
              "放行=同行带「命名豁免<类别>: 原因」或「登记条」（miss_registry）引用。")
    else:
        print(f"[禁词门禁] 绿：新增 0 · 计数增长 0 · 存量键 {len(当前)}"
              f"（命中 {sum(当前.values())} 处冻结放行）")
    收缩 = len(判定果["消失"]) + len(判定果["下降"])
    if 收缩:
        print(f"[提示] 基线有 {收缩} 键可收缩（键消失/计数下降）——确认后 --freeze 收账")


# ============ 五、自测（正反两态·临时样例树·零触碰真实树） ============

样例命中 = """// 注释含兜底默认——命中
/* 块注释含 fallback ——命中（跨行段·行=起始行）*/
// 大小写变体 Best-Effort——命中
"""

样例干净 = """// 干净样例：禁词不出现的注释
int 求和(int a, int b) {
    return a + b;          // 返回和
}
const char* 提示 = "text";   // 字符串内容不判（本注释无禁词）
const char* 案 = "兜底";     // 字符串值含禁词·注释无禁词=不判
"""

样例放行 = """// 静默豁免业务值: 查表 miss 按 0 处理（含「兜底」词但带引用·放行）
// 命名豁免规范编号: 条款⑪清偿前过渡（含「回退」词但带引用·放行）
// 见 miss_registry 登记条：本处保守放行已呈报（含词但带引用·放行）
"""

样例v2 = """// v2 样例：否则块注释含「宁漏勿崩」——命中
函数 例(整32 甲) -> 布尔 {
    返回 真;
}
"""


def 自测建树(根: Path):
    文件们 = {
        "src/命中.cpp": 样例命中,
        "src/干净.cpp": 样例干净,
        "src/放行.cpp": 样例放行,
        "CN语言编译器v2/命中.cn": 样例v2,
    }
    for 相对, 内容 in 文件们.items():
        p = 根 / 相对
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(内容, encoding="utf-8")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    def 键计(根: Path) -> Counter:
        return Counter(建键(h) for h in 全库扫描(根)["命中"])

    with tempfile.TemporaryDirectory(prefix="cbwg1959_") as td:
        根 = Path(td)
        自测建树(根)
        扫描果 = 全库扫描(根)
        当前 = Counter(建键(h) for h in 扫描果["命中"])
        词们 = sorted(h["词"] for h in 扫描果["命中"])
        断言(len(扫描果["命中"]) == 4, f"命中=4（兜底/fallback/best-effort/宁漏勿崩）: {词们}")
        断言(词们 == sorted(["兜底", "fallback", "best-effort", "宁漏勿崩"]),
            f"按词分布: {词们}")
        断言(not any(h["文件"] == "src/干净.cpp" for h in 扫描果["命中"]),
            "干净样例零命中（字符串内禁词不判）")
        断言(not any(h["文件"] == "src/放行.cpp" for h in 扫描果["命中"]),
            "引用放行（静默豁免/命名豁免/登记条三形态）全放行")
        断言(all(h["行"] >= 1 for h in 扫描果["命中"]), "行号健康")
        # 基线冻结与演化
        基 = 根 / "baseline.json"
        断言(写基线(基, 当前) == 4, "基线写键数=4")
        基线 = 读基线(基)
        断言(基线 == 当前, "基线读回与实测一致")
        果 = 判定(当前, 基线)
        断言(not 果["红"], f"存量全在基线=绿: {果!r}")
        # 反态①：新增禁词注释=红
        (根 / "src" / "新增.cpp").write_text("// 这是个保守不判的分支\nint 例() { return 0; }\n",
                                        encoding="utf-8")
        果 = 判定(键计(根), 基线)
        断言(果["红"] and len(果["新键"]) == 1 and "保守不判" in 果["新键"][0],
            f"反态①新增禁词真拦: {果['新键']!r}")
        (根 / "src" / "新增.cpp").unlink()
        # 反态②：同文件新段禁词=新键（键含段文本·段不同即新键）=红
        (根 / "src" / "命中.cpp").write_text(样例命中 + "// 再提一次兜底\n", encoding="utf-8")
        果 = 判定(键计(根), 基线)
        断言(果["红"] and any("兜底" in k and "再提一次兜底" in k for k in 果["新键"]),
            f"反态②同文件新段禁词=新键真拦: {果['新键']!r}")
        (根 / "src" / "命中.cpp").write_text(样例命中, encoding="utf-8")
        # 反态③：引用放行不生效形态——命中段缺引用词=不放行
        (根 / "src" / "新增.cpp").write_text("// 保守放行（无引用词）\nint 例() { return 0; }\n",
                                        encoding="utf-8")
        果 = 判定(键计(根), 基线)
        断言(果["红"] and any("保守放行" in k for k in 果["新键"]),
            f"反态③无引用词禁词=红: {果['新键']!r}")
        # 正态：引用词补上=键消失回绿
        (根 / "src" / "新增.cpp").write_text("// 保守放行——见登记条：已呈报待批\nint 例() { return 0; }\n",
                                        encoding="utf-8")
        果 = 判定(键计(根), 基线)
        断言(not 果["红"], f"补引用词后回绿: {果!r}")
        # 消失收缩
        (根 / "src" / "命中.cpp").write_text(样例干净, encoding="utf-8")
        果 = 判定(键计(根), 基线)
        断言(not 果["红"] and len(果["消失"]) == 3,
            f"位置清偿=消失可收缩: {果['消失']!r}")
        # 基线缺失=None（门禁层红拒跑）；损坏=拒跑
        断言(读基线(根 / "不存在.json") is None, "基线缺失=None（门禁层红拒跑）")
        (根 / "坏.json").write_text("{ 不是 json", encoding="utf-8")
        try:
            读基线(根 / "坏.json")
            断言(False, "基线损坏应拒跑")
        except SystemExit:
            断言(True, "基线损坏=拒跑（防静默）")

    if 失败:
        print("== 禁词门禁自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 禁词门禁自测全过（{通过[0]} 项断言·正反两态/引用放行/基线棘轮齐备）==")
    return 0


# ============ 六、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="注释禁词机械门禁（#1959·条款⑪·冻结基线只减不增）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--baseline", default=str(基线文件), help="冻结基线路径")
    解析器.add_argument("--report", action="store_true", help="报告模式（不判红）")
    解析器.add_argument("--check", action="store_true", help="门禁模式（默认行为）")
    解析器.add_argument("--freeze", action="store_true", help="以当前实测重写冻结基线")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    根 = Path(参数.root).resolve()
    if not 根.is_dir():
        print(f"[禁词门禁] 根目录不存在：{根}")
        return 2
    扫描果 = 全库扫描(根)
    当前 = Counter(建键(h) for h in 扫描果["命中"])

    if 参数.freeze:
        键数 = 写基线(Path(参数.baseline), 当前)
        print(f"[基线] 已写 {Path(参数.baseline).name}：键 {键数} · "
              f"命中 {sum(当前.values())} 处")
        打印扫描摘要(扫描果)
        打印读取失败(扫描果)
        return 0

    基路径 = Path(参数.baseline)
    if 参数.report:
        基线 = 读基线(基路径) if 基路径.exists() else Counter()
        print("== 注释禁词门禁报告（#1959·条款⑪）==")
        print(f"[判据] 注释禁词 {len(禁词表)} 词（诚实边界/保守豁免/保守不判/保守放行/"
              f"宁漏勿崩/宁漏勿误/回退/兜底/fallback/best-effort/best effort）· "
              f"同行「命名豁免/登记条/静默豁免」引用放行")
        打印扫描摘要(扫描果)
        if not 基路径.exists():
            print("[提示] 基线缺失——报告模式不判红（先 --freeze 生成基线）")
            打印读取失败(扫描果)
            return 0
        果 = 判定(当前, 基线)
        收缩 = len(果["消失"]) + len(果["下降"])
        print(f"[基线] 新增键 {len(果['新键'])} · 计数增长 {len(果['增长'])} · "
              f"可收缩 {收缩}")
        for 键 in 果["新键"][:20]:
            print(f"  + {键[:160]}")
        打印读取失败(扫描果)
        return 0

    基线 = 读基线(基路径)
    if 基线 is None:
        print(f"[禁词门禁·红] 基线缺失：{基路径.name}——先跑 "
              f"python3 scripts/check_banned_words_gate.py --freeze 生成")
        打印扫描摘要(扫描果)
        打印读取失败(扫描果)
        return 1
    判定果 = 判定(当前, 基线)
    打印门禁(判定果, 当前)
    打印读取失败(扫描果)
    return 1 if (判定果["红"] or 扫描果["统计"]["读取失败"]) else 0


if __name__ == "__main__":
    sys.exit(main())
