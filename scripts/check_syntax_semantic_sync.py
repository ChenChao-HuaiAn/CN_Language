#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_syntax_semantic_sync.py —— 语法接受面↔语义实现面同步机械门禁（871·2026-10-18）。

背景（台账 871·病例 762/775）：宿主 parser 接受类字段初始化器（`整32 基 = 100;`
填 ClassMember.initializer）而 semantic/IR 全链零消费=静默丢弃，静默错值缺陷存活
多轮靠探针发现（775 甲案 10-15 才收口）——「实现了」与「实现全了」不能靠收尾轮
补齐。用户令立案：解析器（宿主 parser + v2 语法包）新接受的语法节点形态必须在
登记表（脚本 scripts/syntax_semantic_registry.json）登记语义实现点与覆盖用例；
登记锚/用例须真实存在（防「注释声称」·728n 纪律五同源）。

三面自动枚举（语法接受面）：
  面A 宿主节点类型 = model/ast_base.hpp 的 NodeType 枚举名（accept 分派全体）；
  面B v2 节点类型   = 协议常量.cn 的 节点_* 常量（v2 语法树节点全体）；
  面C 宿主 AST 字段 = model/ast_{base,decl,expr,stmt}.hpp 的类字段（762 病例精确
       抓捕面：parser 写好字段而语义/IR/代码生成零引用=静默丢弃形态：initializer）。

判定（冻结基线只减不增 + 登记表·借鉴 check_miss_gate.py 先例）：
  红① 新增节点类型未登记（基线外节点名在登记表无条）
  红② 新增零消费字段未登记（基线外零消费字段在登记表无条）
  红③ 登记条形态陈旧（形态名已从代码消失——改代码须同步修表）
  红④ 登记条语义锚缺失（锚文本在所指文件不存在——「注释声称」显形）
  红⑤ 登记条用例不存在（tests/e2e 无匹配目录——「用例覆盖」机械核）
  红⑥ 登记条缺必填（登记人必填；节点面语义锚+用例必填；字段面备注必填）
  红⑦ 扫描面异常（现测数不足基线 60%=提取失效或大规模删除——miss 不可静默）
  基线/登记表缺失或损坏=拒跑（防绕过）。
  黄（不拦）：存量零消费节点/字段观察清单、基线消减提示。

注册/挂载：gate_quick.py 静态段（L1 硬拦）+ ci.ps1 win 全量 +
  check_registry.json（check_registry_audit 强制新门禁注册）。

诚实边界（不假装全覆盖·机械筛+登记留痕+人判闭环）：
  ① 节点/字段消费=文本级粗判（加入注释命中=偏松·宁少红不误红）——同名字段
     消费难区分实例（`type`/`name` 等通用名检测弱；独特名如 initializer 强）。
  ② 字段提取=行级正则（≥4 空格缩进+行尾分号+排除括号行）——新风格漏检由
     「数量骤降红」+ selftest 兜底；漏检方向=偏松（不误红）。
  ③ 登记条锚/用例=存在性核（不证明语义正确——那是 E2E/覆盖矩阵职责）；
     「假装登记」由人工审查兜底（登记人字段留痕）。
  ④ 「新增」=相对冻结基线判定（只减不增）；大规模合法变更（重命名/删除）走
     --update-baseline 收账（与 miss_gate 同流程）。

用法：
  python scripts/check_syntax_semantic_sync.py              # 门禁（rc=1=有红·L1 硬拦）
  python scripts/check_syntax_semantic_sync.py --report     # 报告模式（全列·rc=0）
  python scripts/check_syntax_semantic_sync.py --json       # 机读输出（stdout）
  python scripts/check_syntax_semantic_sync.py --update-baseline   # 重写冻结基线
  python scripts/check_syntax_semantic_sync.py --selftest   # 自测（临时样例树·正反两态）
  python scripts/check_syntax_semantic_sync.py --probe      # 探针抽查：对登记条用例
                                                            #   跑真实 E2E（不进 gate_quick）

退出码：0=绿；1=红（硬拦）；2=用法/环境错误（含基线缺失拒跑）。
"""

from __future__ import annotations

# 兼容 python3.8（深度机/单位机系统 python——同 gate_quick/check_miss_gate 先例）

import argparse
import glob as _glob
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
仓库根 = 脚本目录.parent

登记表文件 = 仓库根 / "scripts" / "syntax_semantic_registry.json"
基线文件 = 仓库根 / "scripts" / "syntax_semantic_baseline.json"

# ============ 扫描源（语法接受面·相对路径；临时树 selftest 可复用） ============

节点枚举文件 = "src/cn_compiler/model/ast_base.hpp"
AST头文件们 = ("src/cn_compiler/model/ast_base.hpp",
            "src/cn_compiler/model/ast_decl.hpp",
            "src/cn_compiler/model/ast_expr.hpp",
            "src/cn_compiler/model/ast_stmt.hpp")
v2常量文件 = "CN语言编译器v2/协议常量/协议常量.cn"
E2E目录 = "tests/e2e"

# 消费面（语义实现侧）：宿主=语义/IR/代码生成包；v2=语义/IR/代码生成包+主链
消费面_宿主 = ("src/cn_compiler/semantic", "src/cn_compiler/ir", "src/cn_compiler/codegen")
消费面_v2 = ("CN语言编译器v2/语义", "CN语言编译器v2/IR",
           "CN语言编译器v2/代码生成", "CN语言编译器v2/主.cn")

# ============ 判据常量 ============

扫描后缀 = (".cpp", ".hpp", ".cn")
形态前缀们 = ("宿主节点:", "v2节点:", "宿主字段:")
面名们 = ("宿主节点", "v2节点", "宿主字段")

# NodeType 名 → SemanticAnalyzer visit 方法名映射特例（ast_base.hpp accept 分派：
#   TypeNode 的 accept 调 visitType——同族命名唯一特例·新增特例须同步此表）。
visit特例 = {"TypeNode": "Type"}

# 字段声明提取消歧（行级·≥4 空格缩进+行尾分号；含 `(` 的函数声明行天然不匹配）
字段模式 = re.compile(
    r"^ {4,}([A-Za-z_][\w:<>,*&\s]*?)\s+([A-Za-z_]\w*)\s*(?:=\s*[^;(){}]*)?;"
    r"\s*(?://.*)?$")
成员访问模式 = re.compile(r"(?:->|\.)([A-Za-z_]\w*)")

# 扫描面健康度（数量骤降=提取失效/大规模删除——相对基线判·不足 60%=红）
骤降比 = 0.6


# ============ 一、文件读取（miss 不静默：读失败/提取失败=拒跑） ============

def 读文本(路径: Path) -> str:
    try:
        return 路径.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁·环境错误] 读文件失败：{路径}（{e.__class__.__name__}）")


def 归一(文本: str) -> str:
    """空白折叠（锚文本比对用——防格式重排误报）。"""
    return re.sub(r"\s+", "", 文本 or "")


# ============ 二、语法接受面提取（三面） ============

def 提取宿主节点(根: Path):
    """model/ast_base.hpp NodeType 枚举名列表（accept 分派全体）。"""
    文本 = 读文本(根 / 节点枚举文件)
    m = re.search(r"enum class NodeType \{(.*?)\n\};", 文本, re.S)
    if not m:
        raise SystemExit(f"[门禁·环境错误] {节点枚举文件} 未找到 NodeType 枚举段"
                         "（提取面失效——拒跑防静默变松）")
    return re.findall(r"^\s*([A-Z]\w+)\s*,", m.group(1), re.M)


def 提取v2节点(根: Path):
    """协议常量.cn 节点_* 常量名列表（v2 语法树节点全体）。"""
    文本 = 读文本(根 / v2常量文件)
    名们 = re.findall(r"常量 (节点_\w+) = \d+;", 文本)
    if not 名们:
        raise SystemExit(f"[门禁·环境错误] {v2常量文件} 未提取到任何 节点_* 常量"
                         "（提取面失效——拒跑防静默变松）")
    return 名们


def 提取宿主字段(根: Path):
    """AST 头文件类字段 {字段名: [(文件, 行), …]}（同名多处合并·762 病例抓捕面）。"""
    字段们 = {}
    for 相对 in AST头文件们:
        文本 = 读文本(根 / 相对)
        for 行号, 行 in enumerate(文本.splitlines(), 1):
            m = 字段模式.match(行)
            if m:
                字段们.setdefault(m.group(2), []).append((相对, 行号))
    if not 字段们:
        raise SystemExit("[门禁·环境错误] AST 头文件未提取到任何字段"
                         "（提取面失效——拒跑防静默变松）")
    return 字段们


def 扫消费面(根: Path, 目录们):
    """消费面全文本+成员访问名集合（语义实现侧引用粗判·注释命中亦计=偏松）。"""
    成员们 = set()
    段们 = []
    for 相对 in 目录们:
        p = 根 / 相对
        if p.is_file():
            文件们 = [p]
        elif p.is_dir():
            文件们 = sorted(x for x in p.rglob("*") if x.suffix in 扫描后缀)
        else:
            raise SystemExit(f"[门禁·环境错误] 消费面目录不存在：{相对}（拒跑防静默变松）")
        for f in 文件们:
            文本 = 读文本(f)
            段们.append(文本)
            成员们.update(成员访问模式.findall(文本))
    return 成员们, "\n".join(段们)


def 现测计算(根: Path):
    """三面现测：节点名们/字段们/零消费字段/存量零消费观察。"""
    宿主节点 = 提取宿主节点(根)
    v2节点 = 提取v2节点(根)
    字段们 = 提取宿主字段(根)
    宿主成员, 宿主全文 = 扫消费面(根, 消费面_宿主)
    v2成员, v2全文 = 扫消费面(根, 消费面_v2)
    零消费字段 = sorted(名 for 名 in 字段们 if 名 not in 宿主成员)
    宿主零实现 = sorted(名 for 名 in 宿主节点
                    if ("SemanticAnalyzer::visit" + visit特例.get(名, 名)) not in 宿主全文)
    v2零消费 = sorted(名 for 名 in v2节点 if 名 not in v2全文)
    return {
        "宿主节点": 宿主节点, "v2节点": v2节点, "字段们": 字段们,
        "零消费字段": 零消费字段,
        "宿主零实现": 宿主零实现, "v2零消费": v2零消费,
        "统计": {"宿主节点": len(宿主节点), "v2节点": len(v2节点),
               "宿主字段": len(字段们), "零消费字段": len(零消费字段)},
    }


# ============ 三、登记表/基线读写 ============

def 读登记表(路径: Path):
    """读登记表；缺失=环境错误（拒跑）；损坏/结构非法=拒跑。返回条目们。"""
    if not 路径.exists():
        raise SystemExit(f"[门禁·环境错误] 登记表缺失：{路径.name}"
                         "（871 登记表=对账唯一通道·须入库）")
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁·环境错误] 登记表损坏（{e.__class__.__name__}）：{路径.name}")
    if not isinstance(数据, dict) or not isinstance(数据.get("条目们", []), list):
        raise SystemExit(f"[门禁·环境错误] 登记表结构非法（须为 {{\"条目们\": […]}}）：{路径.name}")
    return 数据.get("条目们", [])


def 读基线(路径: Path):
    """读冻结基线；缺失/损坏=拒跑（防绕过——官方唯一通道 --update-baseline）。"""
    if not 路径.exists():
        raise SystemExit(f"[门禁·环境错误] 基线缺失：{路径.name}——先跑 "
                         f"python scripts/check_syntax_semantic_sync.py --update-baseline 生成")
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁·环境错误] 基线损坏（{e.__class__.__name__}）：{路径.name}")
    return {"宿主节点": set(数据.get("节点", {}).get("宿主", [])),
          "v2节点": set(数据.get("节点", {}).get("v2", [])),
          "零消费字段": set(数据.get("零消费字段", [])),
          "字段数": int(数据.get("字段数", 0))}


def 写基线(路径: Path, 现测: dict) -> dict:
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=路径.parent.parent,
                             capture_output=True, text=True, timeout=10).stdout.strip()
    except Exception:
        sha = ""
    数据 = {
        "版本": 1,
        "工具": "scripts/check_syntax_semantic_sync.py",
        "说明": ("871 语法接受面↔语义实现面同步门禁冻结基线（只减不增）：①节点=上线时"
               "两侧节点类型全量快照（主要宿主 NodeType 枚举 / v2 节点_* 常量）——基线外"
               "（新增）节点类型必须在 syntax_semantic_registry.json 登记（语义实现锚+"
               "用例）；②零消费字段=上线时语义/IR/代码生成面零引用的宿主 AST 字段快照"
               "（762 病例面）——基线外新增零消费字段必须登记。位置消失=自动消减提示；"
               "数量骤降（不足 60%）=扫描面异常红。手改基线=绕过门禁行为·禁。"),
        "生成sha": sha,
        "节点": {"宿主": sorted(现测["宿主节点"]), "v2": sorted(现测["v2节点"])},
        "零消费字段": sorted(现测["零消费字段"]),
        "字段数": len(现测["字段们"]),
    }
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1), encoding="utf-8")
    return {"宿主节点": len(数据["节点"]["宿主"]), "v2节点": len(数据["节点"]["v2"]),
          "零消费字段": len(数据["零消费字段"])}


# ============ 四、判定 ============

def 解析形态(形态: str):
    """登记条形态 → (前缀, 名)；前缀非法=None。"""
    for 前缀 in 形态前缀们:
        if isinstance(形态, str) and 形态.startswith(前缀):
            return 前缀[:-1], 形态[len(前缀):]
    return None, ""


def 查用例存在(根: Path, 值: str) -> bool:
    """登记用例（编号或编号_名）→ tests/e2e 存在匹配目录（+含主.cn 更佳）。"""
    if not 值:
        return False
    e2e = 根 / E2E目录
    if not e2e.is_dir():
        return False
    命中 = [p for p in e2e.glob(_glob.escape(值) + "*") if p.is_dir()]
    return bool(命中)


def 查锚存在(根: Path, 锚: str) -> bool:
    """登记语义锚 「文件::锚文本」→ 锚文本（归一）在所定文件全文（归一）存在。"""
    if not isinstance(锚, str) or "::" not in 锚:
        return False
    文件, 锚文本 = 锚.split("::", 1)
    p = 根 / 文件
    if not p.exists() or not p.is_file():
        return False
    return 归一(锚文本) in 归一(读文本(p))


def 判形态存在(形态, 现测: dict) -> bool:
    """登记条形态名在现测清单中存在（防陈旧——形态名是代码派生物）。"""
    前缀, 名 = 解析形态(形态)
    if 前缀 is None or not 名:
        return False
    if 前缀 == "宿主节点":
        return 名 in 现测["宿主节点"]
    if 前缀 == "v2节点":
        return 名 in 现测["v2节点"]
    if 前缀 == "宿主字段":
        return 名 in 现测["字段们"]
    return False


def 判扫描面(现测: dict, 基线: dict):
    """扫描面数量骤降检测（提取失效/大规模删除=红·miss 不可静默）。"""
    红 = []
    检查面们 = (("宿主节点", 现测["统计"]["宿主节点"], len(基线["宿主节点"])),
             ("v2节点", 现测["统计"]["v2节点"], len(基线["v2节点"])),
             ("宿主字段", 现测["统计"]["宿主字段"], 基线["字段数"]))
    for 面, 现数, 基数 in 检查面们:
        if 基数 and 现数 < 基数 * 骤降比:
            红.append({"种类": "扫描面异常", "形态": 面,
                       "说明": f"现测 {现数} < 基线 {基数}×{骤降比:g}——提取失效或"
                             f"大规模删除（核对后 --update-baseline 收账）"})
    return 红


def 判新增面(现测: dict, 基线: dict, 条形态们: set):
    """基线外新增（节点类型/零消费字段）必须登记。"""
    红 = []
    新增节点 = []
    for 面, 现名们, 基名们 in (("宿主节点", 现测["宿主节点"], 基线["宿主节点"]),
                            ("v2节点", 现测["v2节点"], 基线["v2节点"])):
        for 名 in 现名们:
            if 名 not in 基名们:
                新增节点.append(名)
                形态 = f"{面}:{名}"
                if 形态 not in 条形态们:
                    红.append({"种类": "新增节点类型未登记", "形态": 形态,
                               "说明": "解析器新接受的语法节点形态必须在登记表登记"
                                     "（语义实现锚+用例）——登记后跑 --update-baseline 收基线"})
    新增零消费 = [名 for 名 in 现测["零消费字段"]
               if 名 not in 基线["零消费字段"]]
    for 名 in 新增零消费:
        形态 = f"宿主字段:{名}"
        if 形态 not in 条形态们:
            红.append({"种类": "新增零消费字段未登记", "形态": 形态,
                       "说明": "新增 AST 字段在语义/IR/代码生成面零引用（762 型静默面）"
                             "——补语义实现或登记豁免（备注写消费面/理由）"})
    return 红, 新增节点, 新增零消费


def 判登记条(根: Path, 条目们: list, 现测: dict):
    """登记条校验：必填/格式/形态存在/锚存在/用例存在。"""
    红 = []
    for i, 条 in enumerate(条目们, 1):
        if not isinstance(条, dict):
            红.append({"种类": "登记条格式非法", "形态": f"条目{i}",
                       "说明": "条目须为 JSON 对象（含 形态/登记人/语义锚/用例 等字段）"})
            continue
        形态 = 条.get("形态", "")
        前缀, 名 = 解析形态(形态)
        位 = f"条目{i}（{形态 or '缺形态'}）"
        if not 条.get("登记人"):
            红.append({"种类": "登记条缺必填", "形态": 位, "说明": "缺「登记人」字段"})
        if 前缀 is None or not 名:
            红.append({"种类": "登记条格式非法", "形态": 位,
                       "说明": f"形态前缀须为 {形态前缀们} 之一且名非空"})
            continue
        if not 判形态存在(形态, 现测):
            红.append({"种类": "登记条陈旧", "形态": 形态,
                       "说明": "形态名已从代码消失（节点改名/删除）——改代码须同步修表"})
        锚们 = 条.get("语义锚", [])
        用例们 = 条.get("用例", [])
        if 前缀 in ("宿主节点", "v2节点"):
            if not 锚们 or not 用例们:
                红.append({"种类": "登记条缺必填", "形态": 形态,
                           "说明": "节点类型条必填「语义锚」+「用例」（语法形态须有"
                                 "语义实现与覆盖用例）"})
        else:                            # 宿主字段：备注必填（消费面/豁免理由）
            if not 条.get("备注"):
                红.append({"种类": "登记条缺必填", "形态": 形态,
                           "说明": "字段条必填「备注」（写语义消费面或豁免理由——"
                                 "762 型零消费豁免须留痕）"})
        for 锚 in 锚们:
            if not 查锚存在(根, 锚):
                红.append({"种类": "语义锚缺失", "形态": 形态,
                           "说明": f"锚「{锚}」在所指文件不存在——语义实现点漂移/"
                                 f"未实现（「注释声称」显形）"})
        for 值 in 用例们:
            if not 查用例存在(根, str(值)):
                红.append({"种类": "用例不存在", "形态": 形态,
                           "说明": f"用例「{值}」在 {E2E目录}/ 无匹配目录——覆盖缺失"})
    return 红


def 判消减(现测: dict, 基线: dict):
    """基线位置消失=自动消减提示（黄·不红）。"""
    黄 = []
    for 名 in sorted(基线["宿主节点"] - set(现测["宿主节点"])):
        黄.append(f"宿主节点 {名} 自基线消失（删除/改名——确认无误报后 --update-baseline 收账）")
    for 名 in sorted(基线["v2节点"] - set(现测["v2节点"])):
        黄.append(f"v2节点 {名} 自基线消失（同上）")
    for 名 in sorted(基线["零消费字段"] - set(现测["零消费字段"])):
        黄.append(f"零消费字段 {名} 已被消费或消失（762 型静默面收口——收账后可移出基线）")
    return 黄


def 判定(根: Path, 现测: dict, 条目们: list, 基线: dict):
    """汇总判定：红（硬拦）+ 黄（观察）+ 新增面记录。"""
    if not isinstance(条目们, list):
        raise SystemExit("[门禁·环境错误] 登记表「条目们」非列表（格式损坏）")
    条形态们 = set()
    for 条 in 条目们:
        if isinstance(条, dict):
            条形态们.add(条.get("形态", ""))
    红 = []
    红 += 判扫描面(现测, 基线)
    红新增, 新增节点, 新增零消费 = 判新增面(现测, 基线, 条形态们)
    红 += 红新增
    红 += 判登记条(根, 条目们, 现测)
    黄 = 判消减(现测, 基线)
    return {"红": 红, "黄": 黄, "新增节点": 新增节点, "新增零消费": 新增零消费,
          "条目数": len(条目们)}


# ============ 五、报告输出 ============

def 打印红们(红们: list):
    print(f"\n【红清单】（{len(红们)} 条·L1 硬拦）")
    for r in 红们:
        print(f"  [×] ({r['种类']}) {r.get('形态', '')}")
        print(f"      处置：{r['说明']}")


def 打印报告(现测: dict, 判定果: dict, 详列: bool):
    统 = 现测["统计"]
    print("== 语法接受面↔语义实现面同步门禁（check_syntax_semantic_sync.py·871）==")
    print(f"扫描面：宿主节点 {统['宿主节点']}·v2 节点 {统['v2节点']}·"
          f"宿主 AST 字段 {统['宿主字段']}（零消费 {统['零消费字段']}）；"
          f"登记表条目 {判定果['条目数']} 条")
    if 判定果["新增节点"] or 判定果["新增零消费"]:
        print("【新增面】（基线外·必须登记）")
        for 名 in 判定果["新增节点"]:
            print(f"  [新增节点] {名}")
        for 名 in 判定果["新增零消费"]:
            print(f"  [新增零消费字段] {名}")
    else:
        print("【新增面】无（节点类型与零消费字段均在基线内）")
    print("【存量观察】（黄·既有面·不作为拦依据）")
    print(f"  宿主节点语义实现缺（visit 缺席）：{现测['宿主零实现'] or '无'}")
    print(f"  v2 节点语义/IR 零消费：{现测['v2零消费'] or '无'}")
    print(f"  宿主 AST 字段语义/IR/代码生成零引用：{现测['零消费字段'] or '无'}")
    if 判定果["黄"]:
        print(f"【基线消减】（{len(判定果['黄'])} 条·黄）")
        for 行 in 判定果["黄"][:10]:
            print(f"  [↓] {行}")
        if len(判定果["黄"]) > 10:
            print(f"  …（其余 {len(判定果['黄']) - 10} 条）")
    if 详列:
        print("\n【在测明细】（--report·全列）")
        print(f"-- 宿主节点（{len(现测['宿主节点'])}）--")
        print("  " + " ".join(现测["宿主节点"]))
        print(f"-- v2 节点（{len(现测['v2节点'])}）--")
        print("  " + " ".join(现测["v2节点"]))
        print(f"-- 宿主 AST 字段（{len(现测['字段们'])}）--")
        print("  " + " ".join(sorted(现测["字段们"])))
    if 判定果["红"]:
        打印红们(判定果["红"])
        print(f"\n[门禁·红] {len(判定果['红'])} 条——L1 硬拦；唯一合法通道="
              f"syntax_semantic_registry.json 补条（语义实现锚+用例）+跑通用例，"
              f"或原地补语义实现。")
    else:
        print("\n[门禁·绿] 新增面零红（冻结基线只减不增·登记锚/用例存在性核过）")


def 转json(现测: dict, 判定果: dict) -> str:
    输出 = {"工具": "check_syntax_semantic_sync.py", "任务": "871",
          "统计": 现测["统计"], "条目数": 判定果["条目数"],
          "新增节点": 判定果["新增节点"], "新增零消费": 判定果["新增零消费"],
          "存量观察": {"宿主零实现": 现测["宿主零实现"], "v2零消费": 现测["v2零消费"],
                    "零消费字段": 现测["零消费字段"]},
          "黄": 判定果["黄"], "红": 判定果["红"]}
    return json.dumps(输出, ensure_ascii=False, indent=1)


# ============ 六、探针抽查（最小 b·不进 gate_quick：需构建产物+分钟级） ============

def 探针抽查(根: Path, 条目们: list) -> int:
    """对登记表所有「用例」跑真实 E2E（--filter），rc=0 全绿才算过。

    这是「探针矩阵最小版」：编译→链接→运行→比对 全链路；语义未实现/静默
    丢弃会在运行输出比对中显形（775 病例复盘面）。"""
    runner = 根 / E2E目录 / "run_e2e.py"
    if not runner.exists():
        raise SystemExit(f"[门禁·环境错误] E2E runner 缺失：{runner}（探针无法执行）")
    用例们 = []
    for 条 in 条目们:
        if isinstance(条, dict):
            for 值 in 条.get("用例", []) or []:
                if str(值) not in 用例们:
                    用例们.append(str(值))
    if not 用例们:
        print("[探针] 登记表无用例——无可抽查（先登记语法形态×用例）")
        return 0
    print(f"[探针] 抽查 {len(用例们)} 个登记用例（真实编译+运行+比对）：{用例们}")
    失败 = []
    for 值 in 用例们:
        r = subprocess.run([sys.executable, str(runner), "--filter", 值],
                           cwd=str(根))
        print(f"[探针] {值} → rc={r.returncode}")
        if r.returncode != 0:
            失败.append(值)
    if 失败:
        print(f"[探针·红] {len(失败)} 个用例未过：{失败}")
        return 1
    print(f"[探针·绿] {len(用例们)} 个登记用例全过")
    return 0


# ============ 七、自测（临时样例树·正反两态·零触碰真实树） ============

样例宿主枚举 = """#pragma once
namespace cn_compiler {
class SourceLocation {};
enum class NodeType {
    Program,
    FunctionDecl,
    ExprStmt,
    TypeNode,
};
class AstNode {
public:
    SourceLocation location;
    NodeType type_;
};
class Expr : public AstNode {};
class Stmt : public AstNode {};
}
"""

样例宿主声明 = """#pragma once
#include "cn_compiler/model/ast_base.hpp"
namespace cn_compiler {
class ClassMember : public AstNode {
public:
    std::string name;
    std::string typeName;
    std::unique_ptr<Expr> initializer;
    bool fromImport = false;
};
}
"""

样例语义 = """#include "cn_compiler/model/ast_decl.hpp"
namespace cn_compiler {
void SemanticAnalyzer::visitProgram(Program* node) {}
void SemanticAnalyzer::visitFunctionDecl(FunctionDecl* node) {}
void SemanticAnalyzer::visitExprStmt(ExprStmt* node) {}
void SemanticAnalyzer::visitType(Type* node) {}
void SemanticAnalyzer::检查类字段(ClassMember* member) {
    use(member->name); use(member->typeName); use(member->location);
    if (member->initializer != nullptr) { checkExpr(member->initializer.get()); }
}
}
"""

样例IR = """#include "cn_compiler/model/ast_stmt.hpp"
namespace cn_compiler {
void Lowerer::genExpr(Expr* e) { if (e->type_ == NodeType::IdentifierExpr) {} }
}
"""

样例v2常量 = """模块 协议常量;
公开:
常量 节点_程序 = 1;
常量 节点_标识符 = 10;
常量 节点_类字段 = 42;
"""

样例v2语义 = """模块 语义;
函数 检查程序(节点 程序) -> 空类型 {
    如果 (程序.类型 == 节点_程序) { }
}
函数 检查字段(节点 成员) -> 空类型 {
    如果 (成员.类型 == 节点_类字段) { }
}
"""


def 自测建树(保活: list, 文件们: dict):
    临时 = tempfile.TemporaryDirectory(prefix="css871_")
    保活.append(临时)
    根 = Path(临时.name)
    for 相对, 文本 in 文件们.items():
        p = 根 / 相对
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(文本, encoding="utf-8")
    return 根


def 自测基础树(保活: list):
    """基础树：宿主 4 节点+1 引用字段+1 零消费字段；v2 3 节点；1 个 E2E 用例。"""
    return 自测建树(保活, {
        "src/cn_compiler/model/ast_base.hpp": 样例宿主枚举,
        "src/cn_compiler/model/ast_decl.hpp": 样例宿主声明,
        "src/cn_compiler/model/ast_expr.hpp": "// 空占位\nnamespace cn_compiler {}\n",
        "src/cn_compiler/model/ast_stmt.hpp": "// 空占位\nnamespace cn_compiler {}\n",
        "src/cn_compiler/semantic/semantic.cpp": 样例语义,
        "src/cn_compiler/ir/ir.cpp": 样例IR,
        "src/cn_compiler/codegen/codegen.cpp": "// 空占位\n",
        "CN语言编译器v2/协议常量/协议常量.cn": 样例v2常量,
        "CN语言编译器v2/语义/语义分析.cn": 样例v2语义,
        "CN语言编译器v2/IR/IR生成.cn": "模块 IR;\n",
        "CN语言编译器v2/代码生成/生成.cn": "模块 代码生成;\n",
        "CN语言编译器v2/主.cn": "模块 主;\n",
        "tests/e2e/900_样例/主.cn": "函数 主() -> 整32 { 返回 0; }\n",
        "tests/e2e/900_样例/主.expected": "0\n",
    })


def 自测写表(表: Path, 条目们: list):
    表.write_text(json.dumps({"版本": 1, "条目们": 条目们}, ensure_ascii=False),
                 encoding="utf-8")


def 自测跑(根: Path, 表: Path, 基: Path):
    return 判定(根, 现测计算(根), 读登记表(表), 读基线(基))


def 红种们(果) -> set:
    return {r["种类"] for r in 果["红"]}


def 自测场景一(断言, 保活):
    """场景一：节点面（新增/登记/锚缺失/用例缺失/陈旧）全链。"""
    根 = 自测基础树(保活)
    表 = 根 / "registry.json"
    基 = 根 / "baseline.json"
    自测写表(表, [])
    写基线(基, 现测计算(根))
    果 = 自测跑(根, 表, 基)
    断言(not 果["红"], f"基线生成后存量放行=绿: {果['红']!r}")
    # 反态①：新增 NodeType 未登记
    枚举 = 根 / "src/cn_compiler/model/ast_base.hpp"
    枚举.write_text(样例宿主枚举.replace("    TypeNode,\n", "    TypeNode,\n    AwaitNode,\n"),
                 encoding="utf-8")
    果 = 自测跑(根, 表, 基)
    断言("新增节点类型未登记" in 红种们(果), f"反态①新增节点未登记=红: {果['红']!r}")
    # 正态①：补登记（语义锚=真实存在的消费点文本+用例存在）
    自测写表(表, [{
        "形态": "宿主节点:AwaitNode",
        "语义锚": ["src/cn_compiler/semantic/semantic.cpp::SemanticAnalyzer::visitProgram"],
        "用例": ["900"], "登记人": "871selftest",
        "备注": "自测：锚先借用 visitProgram（仅验证存在性机制）"}])
    果 = 自测跑(根, 表, 基)
    断言(not 果["红"], f"正态①新增节点登记齐全=绿: {果['红']!r}")
    # 反态②：语义锚缺失（领域词不存在）
    自测写表(表, [{
        "形态": "宿主节点:AwaitNode",
        "语义锚": ["src/cn_compiler/semantic/semantic.cpp::visitAwaitNode缺失锚XXXX"],
        "用例": ["900"], "登记人": "871selftest", "备注": "自测"}])
    果 = 自测跑(根, 表, 基)
    断言("语义锚缺失" in 红种们(果), f"反态②语义锚缺失=红: {果['红']!r}")
    # 反态③：用例不存在
    自测写表(表, [{
        "形态": "宿主节点:AwaitNode",
        "语义锚": ["src/cn_compiler/semantic/semantic.cpp::SemanticAnalyzer::visitProgram"],
        "用例": ["888"], "登记人": "871selftest", "备注": "自测"}])
    果 = 自测跑(根, 表, 基)
    断言("用例不存在" in 红种们(果), f"反态③用例不存在=红: {果['红']!r}")
    # 反态④：登记条缺必填（节点面缺用例）
    自测写表(表, [{
        "形态": "宿主节点:AwaitNode",
        "语义锚": ["src/cn_compiler/semantic/semantic.cpp::SemanticAnalyzer::visitProgram"],
        "登记人": "871selftest"}])
    果 = 自测跑(根, 表, 基)
    断言("登记条缺必填" in 红种们(果), f"反态④节点条缺用例=红: {果['红']!r}")
    # 反态⑤：登记条陈旧（形态从代码消失）
    枚举.write_text(样例宿主枚举, encoding="utf-8")
    自测写表(表, [{
        "形态": "宿主节点:AwaitNode",
        "语义锚": ["src/cn_compiler/semantic/semantic.cpp::SemanticAnalyzer::visitProgram"],
        "用例": ["900"], "登记人": "871selftest", "备注": "自测"}])
    果 = 自测跑(根, 表, 基)
    断言("登记条陈旧" in 红种们(果), f"反态⑤登记条陈旧=红: {果['红']!r}")
    # 正态②：删条+收基线=绿
    自测写表(表, [])
    写基线(基, 现测计算(根))
    果 = 自测跑(根, 表, 基)
    断言(not 果["红"], f"正态②删条收基线=绿: {果['红']!r}")


def 自测场景二(断言, 保活):
    """场景二：字段零消费面（762 病例形态·新增零消费/消费/豁免/消减）。"""
    根 = 自测基础树(保活)
    表 = 根 / "registry.json"
    基 = 根 / "baseline.json"
    自测写表(表, [])
    写基线(基, 现测计算(根))
    断言(现测计算(根)["零消费字段"] == ["fromImport"],
        f"基础树零消费=fromImport（initializer 已消费）: {现测计算(根)['零消费字段']}")
    声明 = 根 / "src/cn_compiler/model/ast_decl.hpp"
    # 反态⑥：新增字段零消费（parser 风格字段·语义忽略=762 型）
    声明.write_text(样例宿主声明.replace(
        "    bool fromImport = false;\n", "    bool fromImport = false;\n    bool newHook = false;\n"),
        encoding="utf-8")
    果 = 自测跑(根, 表, 基)
    断言("新增零消费字段未登记" in 红种们(果), f"反态⑥新增零消费字段=红: {果['红']!r}")
    # 正态③：语义面消费该字段→自动绿
    语义 = 根 / "src/cn_compiler/semantic/semantic.cpp"
    语义.write_text(样例语义.replace(
        "if (member->initializer != nullptr) { checkExpr(member->initializer.get()); }",
        "if (member->initializer != nullptr) { checkExpr(member->initializer.get()); }\n"
        "    if (member->newHook) { warn(); }"), encoding="utf-8")
    果 = 自测跑(根, 表, 基)
    断言(not 果["红"], f"正态③新增字段被语义消费=绿: {果['红']!r}")
    # 回归零消费+登记豁免条（字段面须备注）
    语义.write_text(样例语义, encoding="utf-8")
    自测写表(表, [{"形态": "宿主字段:newHook", "登记人": "871selftest",
                "备注": "自测：仅解析器内部使用（豁免理由留痕）"}])
    果 = 自测跑(根, 表, 基)
    断言(not 果["红"], f"正态④零消费字段登记豁免=绿: {果['红']!r}")
    # 反态⑦：字段条缺备注
    自测写表(表, [{"形态": "宿主字段:newHook", "登记人": "871selftest"}])
    果 = 自测跑(根, 表, 基)
    断言("登记条缺必填" in 红种们(果), f"反态⑦字段条缺备注=红: {果['红']!r}")
    # 正态⑤：消减提示（fromImport 删除=黄不红）
    声明.write_text(样例宿主声明.replace(
        "    bool fromImport = false;\n", ""), encoding="utf-8")
    自测写表(表, [])
    果 = 自测跑(根, 表, 基)
    断言(not 果["红"], f"正态⑤基线消减不红: {果['红']!r}")
    断言(any("fromImport" in 行 for 行 in 果["黄"]), f"消减黄提示含 fromImport: {果['黄']!r}")


def 自测场景三(断言, 保活):
    """场景三：v2 节点面与扫描面异常/基线缺失拒跑。"""
    根 = 自测基础树(保活)
    表 = 根 / "registry.json"
    基 = 根 / "baseline.json"
    自测写表(表, [])
    写基线(基, 现测计算(根))
    const = 根 / "CN语言编译器v2/协议常量/协议常量.cn"
    const.write_text(样例v2常量 + "常量 节点_新形态 = 99;\n", encoding="utf-8")
    果 = 自测跑(根, 表, 基)
    断言("新增节点类型未登记" in 红种们(果), f"反态⑧v2 新增节点未登记=红: {果['红']!r}")
    自测写表(表, [{
        "形态": "v2节点:节点_新形态",
        "语义锚": ["CN语言编译器v2/语义/语义分析.cn::函数 检查字段"],
        "用例": ["900"], "登记人": "871selftest", "备注": "自测"}])
    果 = 自测跑(根, 表, 基)
    断言(not 果["红"], f"正态⑥v2 节点登记齐全=绿: {果['红']!r}")
    # 反态⑨：形态前缀非法
    自测写表(表, [{"形态": "随便节点:X", "语义锚": [], "用例": [], "登记人": "871selftest"}])
    果 = 自测跑(根, 表, 基)
    断言("登记条格式非法" in 红种们(果), f"反态⑨形态前缀非法=红: {果['红']!r}")
    # 拒跑：基线缺失
    try:
        读基线(根 / "不存在.json")
        断言(False, "基线缺失应拒跑")
    except SystemExit:
        断言(True, "基线缺失=拒跑（防绕过）")


def 自测场景四(断言, 保活):
    """场景四：扫描面异常（数量骤降）与提取失效拒跑。"""
    根 = 自测基础树(保活)
    表 = 根 / "registry.json"
    基 = 根 / "baseline.json"
    自测写表(表, [])
    写基线(基, 现测计算(根))
    # 反态⑩：宿主节点删至 1 个（4×0.6=2.4 → 1<2.4=骤降红）
    枚举 = 根 / "src/cn_compiler/model/ast_base.hpp"
    枚举.write_text(样例宿主枚举.replace(
        "    Program,\n    FunctionDecl,\n    ExprStmt,\n    TypeNode,\n", "    Program,\n"),
        encoding="utf-8")
    果 = 自测跑(根, 表, 基)
    断言("扫描面异常" in 红种们(果), f"反态⑩节点数骤降=红: {果['红']!r}")
    # 拒跑：枚举段整体消失（提取失效≠静默）
    枚举.write_text("// NodeType 段被删\n", encoding="utf-8")
    try:
        现测计算(根)
        断言(False, "提取失效应拒跑")
    except SystemExit:
        断言(True, "NodeType 枚举段缺失=拒跑（拒跑防静默变松）")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    保活 = []
    for 场景 in (自测场景一, 自测场景二, 自测场景三, 自测场景四):
        try:
            场景(断言, 保活)
        except Exception as e:            # 场景异常=自测失败（显形不静默）
            失败.append(f"{场景.__name__} 异常：{e!r}")
    if 失败:
        print("== 门禁自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 门禁自测全过（{通过[0]} 项断言·正反两态+拒跑防线齐备）==")
    return 0


# ============ 八、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="语法接受面↔语义实现面同步机械门禁（871·登记表+冻结基线+硬拦）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--registry", default=str(登记表文件), help="登记表路径")
    解析器.add_argument("--baseline", default=str(基线文件), help="冻结基线路径")
    解析器.add_argument("--report", action="store_true", help="报告模式（全列·rc=0 不拦）")
    解析器.add_argument("--json", action="store_true", help="机读输出（stdout）")
    解析器.add_argument("--update-baseline", action="store_true",
                      help="以当前实测重写冻结基线")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    解析器.add_argument("--probe", action="store_true",
                      help="探针抽查：对登记条用例跑真实 E2E（--filter）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    根 = Path(参数.root).resolve()
    现测 = 现测计算(根)
    if 参数.update_baseline:
        统计 = 写基线(Path(参数.baseline), 现测)
        print(f"[基线] 已重写 {Path(参数.baseline).name}：宿主节点 {统计['宿主节点']}·"
              f"v2节点 {统计['v2节点']}·零消费字段 {统计['零消费字段']}")
        return 0
    条目们 = 读登记表(Path(参数.registry))
    if 参数.probe:
        return 探针抽查(根, 条目们)
    if 参数.report and not Path(参数.baseline).exists():
        基线 = {"宿主节点": set(), "v2节点": set(), "零消费字段": set(), "字段数": 0}
        print(f"[提示] 基线缺失（{Path(参数.baseline).name}）——报告模式按空基线全量展示（不拦）")
    else:
        基线 = 读基线(Path(参数.baseline))
    判定果 = 判定(根, 现测, 条目们, 基线)

    if 参数.json:
        print(转json(现测, 判定果))
    else:
        打印报告(现测, 判定果, 详列=参数.report)
    if 参数.report:
        return 0
    return 1 if 判定果["红"] else 0


if __name__ == "__main__":
    sys.exit(main())
