#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""docs_data 内嵌示例 → E2E 用例引用 改写工具（任务 1096）。

背景：1060 的 docs_migrate_e2e.py --改写 只识别 tuple「码」块与 dict「示例」键，
  漏了 dict「代码」键 → 1176_官网示例-cli-手工线程栈守卫、
  1153_官网示例-reference-types-控制流敏感 两条未引用化（官网展示仍读内嵌副本）。
  1295 已把 1060 的反向迁移工具整体退役（不再新建官网示例用例目录）；本工具
  只保留正向「内嵌示例 → 已有 E2E 用例引用」的改写面，不建用例、不维护状态
  文件，并把 dict「代码」键纳入识别（1096 缺口根治）。

用法（仓库根目录）：
  python website/tools/docs_migrate.py --报告
  python website/tools/docs_migrate.py --改写 \
      docs_data_ref3.py:手工线程栈守卫.cn=手工线程栈守卫 \
      docs_data_ref1.py:控制流敏感.cn=未初始化读放行矩阵
  python website/tools/docs_migrate.py --selftest

改写规则：目标窗名必须命中「内嵌示例」（tuple 码块 / dict 示例键 / dict 代码键），
  命中后整窗替换为 用例="<E2E 用例名>"（dict 同时删除 输出 键，避免残留旧副本
  输出）；负例标记（预期失败/预期诊断）与「编译足矣」标记无法随引用保留 =
  显式报错（须人工先确认正例来源）。窗名未命中、重名、同块双代码键、目标用例
  目录不存在 = 显式报错退出，绝不静默跳过。
"""
import argparse
import ast
import glob
import io
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
用例根 = os.path.join(REPO, "tests", "e2e")

# 内嵌示例的代码键（dict 示例 与 dict 代码 同权——1096 缺口）
代码键名 = ("示例", "代码")
# 无法随「用例引用」保留的标记：改写前必须人工处置（防静默丢语义）
不可迁标记 = ("预期失败", "预期诊断", "编译足矣")
# dict 形态的窗口名字段（缺省=无名示例）
名键名 = ("名", "窗口名")
无名窗名 = "<无名>"


def 节点列(行, 字节列):
    """ast col_offset（UTF-8 字节偏移）→ 源码字符串列（中文源码必需）。"""
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
    """dict 字面量节点 → {键: (键节点, 值节点)}（仅字符串常量键）。"""
    out = {}
    for k, v in zip(node.keys, node.values):
        if isinstance(k, ast.Constant) and isinstance(k.value, str):
            out[k.value] = (k, v)
    return out


def 关键字范围(行们, kw):
    """ast.keyword → 替换范围。Python 3.9+ 有位置属性；3.8 回退按值向前搜 名=。"""
    if hasattr(kw, "lineno"):
        return 替换范围(行们, kw, kw)
    值 = kw.value
    sl, sc = 值.lineno, 值.col_offset
    行 = 行们[sl - 1]
    前缀 = 行[:节点列(行, sc)]
    m = re.search(r"%s\s*=\s*$" % re.escape(kw.arg or ""), 前缀)
    if not m:
        raise SystemExit("无法定位关键字 %r（第 %d 行）——Python 3.8 AST 回退失败"
                         % (kw.arg, sl))
    return (sl, m.start(), 值.end_lineno, 节点列(行们[值.end_lineno - 1],
                                               值.end_col_offset))


def 代码键们(节点):
    """示例节点 → 命中的代码键名列表（去重保序·同块双键=歧义）。"""
    if isinstance(节点, ast.Call):
        键们 = [kw.arg for kw in 节点.keywords if kw.arg in 代码键名]
    elif isinstance(节点, ast.Dict):
        对 = 找dict键对(节点)
        键们 = [键 for 键 in 代码键名 if 键 in 对]
    else:
        return []
    去重 = []
    for 键 in 键们:
        if 键 not in 去重:
            去重.append(键)
    return 去重


def 块形态(节点):
    """ast 节点 → 内嵌示例形态（tuple / dict·代码 / dict·示例）；非示例=None。

    注意：docs_data 的 tuple 码块在 AST 里是 ast.Tuple（不是 Python tuple）——
    1060 工具同病「按 Python tuple 判断」致 tuple 形态漏识别，本实现按 AST 形态。
    """
    if isinstance(节点, ast.Tuple):
        elts = 节点.elts
        if len(elts) > 2 and isinstance(elts[0], ast.Constant) and \
                elts[0].value == "码" and \
                isinstance(elts[1], ast.Constant) and \
                isinstance(elts[1].value, str) and \
                isinstance(elts[2], ast.Constant) and \
                isinstance(elts[2].value, str):
            return "tuple"
        return None
    if isinstance(节点, ast.Call):
        if not (isinstance(节点.func, ast.Name) and 节点.func.id == "dict"):
            return None
    elif not isinstance(节点, ast.Dict):
        return None
    键们 = 代码键们(节点)
    if not 键们:
        return None
    if len(键们) > 1:
        raise SystemExit("内嵌示例同时含「代码」与「示例」键，无法确定改写目标"
                         "（第 %d 行·1096）" % 节点.lineno)
    return "dict·" + 键们[0]


def 取窗口名(节点):
    """块 → 窗名；不是内嵌示例返回 None。dict 示例/代码 两键同权（1096）。"""
    形态 = 块形态(节点)
    if 形态 is None:
        return None
    if 形态 == "tuple":
        return 节点.elts[1].value
    if isinstance(节点, ast.Call):
        关键字 = {kw.arg: kw for kw in 节点.keywords if kw.arg}
        for 名键 in 名键名:
            kw = 关键字.get(名键)
            if kw is not None and isinstance(kw.value, ast.Constant) and \
                    isinstance(kw.value.value, str):
                return kw.value.value
        return 无名窗名
    对 = 找dict键对(节点)
    for 名键 in 名键名:
        if 名键 in 对 and isinstance(对[名键][1], ast.Constant) and \
                isinstance(对[名键][1].value, str):
            return 对[名键][1].value.value
    return 无名窗名


def 枚举文件(路径):
    """源文件 → [(窗名, 块节点, 行们)]；窗名重复=改写时显式报错（不可猜）。"""
    with io.open(路径, encoding="utf-8") as f:
        源 = f.read()
    行们 = 源.splitlines(keepends=True)
    结果 = []
    for node in ast.walk(ast.parse(源)):
        名 = 取窗口名(node)
        if 名 is not None:
            结果.append((名, node, 行们))
    return 结果


def 用例存在(名, 根=用例根):
    """用例名 → 目录（支持全名或 <号>_<名> 后缀唯一匹配，与 gen_docs 同规）。"""
    直 = os.path.join(根, 名)
    if os.path.isdir(直):
        return 直
    命中 = sorted(glob.glob(os.path.join(根, "*_" + 名)))
    if len(命中) > 1:
        非v2 = [h for h in 命中 if "_v2_" not in os.path.basename(h)]
        if len(非v2) == 1:
            命中 = 非v2
    if len(命中) == 1:
        return 命中[0]
    if len(命中) > 1:
        raise SystemExit("用例名不唯一（多目录后缀匹配）：%s -> %s" % (名, 命中))
    return None


def 检查不可迁标记(节点, 键们, 窗名):
    """改写前拦截无法随引用保留的标记（负例/编译足矣）——静默丢弃=语义丢失。"""
    命中 = [键 for 键 in 键们 if 键]
    if isinstance(节点, ast.Tuple):
        if len(节点.elts) > 4:
            raise SystemExit(
                "窗名「%s」的 tuple 码块带第 5 元素（编译足矣标记）——不可直接改引用，"
                "须人工确认目标用例的验证面（1096）" % 窗名)
        return
    for 键 in 不可迁标记:
        if 键 in 命中:
            raise SystemExit(
                "窗名「%s」带「%s」标记——引用形态不承载该语义，须人工确认正例来源后"
                "先行删除标记（1096）" % (窗名, 键))


def 删除条目替换(行们, 键们, 目标下标):
    """生成删除 dict 键值对/关键字的替换项：从前一值末尾删到本条目末尾。

    键们=ast.keyword 列表或 (键节点, 值节点) 列表；目标下标=待删项在列表中的位置。
    """
    目标 = 键们[目标下标]
    末范围 = (替换范围(行们, 目标, 目标) if isinstance(目标, ast.keyword)
              else 替换范围(行们, 目标[0], 目标[1]))
    if 目标下标 > 0:
        前一 = 键们[目标下标 - 1]
        前值 = 前一.value if isinstance(前一, ast.keyword) else 前一[1]
        # 取前值**末尾**（删除起点）——取起点会把前一键值对一并截断（1096 自测实录）
        _, _, sl, sc = 替换范围(行们, 前值, 前值)
    else:
        # 首项：删到下一项起点（连同尾随逗号空白）
        if len(键们) < 2:
            raise SystemExit("单项 dict 首项删除无后继可定位（1096 内部错误）")
        次 = 键们[1]
        次起 = 次 if isinstance(次, ast.keyword) else 次[0]
        sl, sc, _, _ = 替换范围(行们, 次起, 次起)
    return (sl, sc, 末范围[2], 末范围[3], "")


def 改写单块(行们, 节点, 用例名, 窗名):
    """块节点 → 替换表（tuple=整块替换；dict=代码键→用例键+删输出键）。"""
    形态 = 块形态(节点)
    if 形态 is None:
        raise SystemExit("内部错误：非内嵌示例节点不可改写（%s）" % 窗名)
    if 形态 == "tuple":
        检查不可迁标记(节点, (), 窗名)
        return [替换范围(行们, 节点, 节点) + ('("用例", "%s")' % 用例名,)]
    替换 = []
    if isinstance(节点, ast.Call):
        关键字 = [kw for kw in 节点.keywords if kw.arg]
        检查不可迁标记(节点, [kw.arg for kw in 关键字], 窗名)
        旧键 = next(kw for kw in 关键字 if kw.arg in 代码键名)
        起, 止 = 关键字范围(行们, 旧键)[:2], 关键字范围(行们, 旧键)[2:]
        替换.append(起 + 止 + ("用例='%s'" % 用例名,))
        if any(kw.arg == "输出" for kw in 关键字):
            出序 = [i for i, kw in enumerate(关键字) if kw.arg == "输出"][0]
            替换.append(删除条目替换(行们, 关键字, 出序))
    else:
        对 = 找dict键对(节点)
        键对们 = [(k, v) for k, v in zip(节点.keys, 节点.values)]
        检查不可迁标记(节点, [k.value for k in 节点.keys
                              if isinstance(k, ast.Constant) and
                              isinstance(k.value, str)], 窗名)
        旧键 = 对["代码"] if "代码" in 对 else 对["示例"]
        替换.append(替换范围(行们, 旧键[0], 旧键[1]) + ("用例='%s'" % 用例名,))
        if "输出" in 对:
            出序 = [i for i, (k, _v) in enumerate(键对们) if k is 对["输出"][0]][0]
            替换.append(删除条目替换(行们, 键对们, 出序))
    return 替换


def 改写文件(路径, 映射, 根=用例根):
    """映射={窗名: 用例名} → (改写处数, 明细列表)。未命中/重名/目标缺失=报错。"""
    候选 = 枚举文件(路径)
    名下 = {}
    for 名, 块, 行们 in 候选:
        名下.setdefault(名, []).append((块, 行们))
    缺失 = sorted(set(映射) - set(名下))
    if 缺失:
        raise SystemExit("%s 未找到内嵌示例窗名：%s（现有：%s）"
                         % (os.path.basename(路径), "、".join(缺失),
                            "、".join(sorted(名下)) or "无"))
    重名 = sorted(名 for 名 in 映射 if len(名下[名]) > 1)
    if 重名:
        raise SystemExit("%s 窗名重复，无法确定改写目标：%s"
                         % (os.path.basename(路径), "、".join(重名)))
    for 窗名, 用例名 in 映射.items():
        if 用例存在(用例名, 根) is None:
            raise SystemExit("目标用例不存在：tests/e2e/*_%s（%s）" % (用例名, 窗名))
    替换表 = []
    明细 = []
    for 窗名, 用例名 in 映射.items():
        块, 行们 = 名下[窗名][0]
        替换表.extend(改写单块(行们, 块, 用例名, 窗名))
        明细.append("%s → %s" % (窗名, 用例名))
    with io.open(路径, encoding="utf-8") as f:
        源 = f.read()
    新源 = 应用替换(源, 替换表)
    ast.parse(新源)                       # 语法自检——不过即中止
    with io.open(路径, "w", encoding="utf-8", newline="\n") as f:
        f.write(新源)
    return len(明细), 明细


def 数据文件们():
    return sorted(glob.glob(os.path.join(HERE, "docs_data_*.py")))


def cmd_报告(_a):
    """枚举全部内嵌示例（按文件/窗名/形态），并单列 dict「代码」键面。"""
    总 = 0
    分布 = {"tuple": 0, "dict·代码": 0, "dict·示例": 0}
    for 路径 in 数据文件们():
        for 名, 节点, _行们 in 枚举文件(路径):
            形 = 块形态(节点)
            分布[形] += 1
            总 += 1
            print("%s  %-10s  %s" % (os.path.basename(路径), 形, 名))
    print("内嵌示例合计：%d（tuple %d / dict·代码 %d〔1096 原漏识别面〕/ dict·示例 %d）"
          % (总, 分布["tuple"], 分布["dict·代码"], 分布["dict·示例"]))
    return 0


def cmd_改写(a):
    if not a.改写:
        raise SystemExit("--改写 需要至少一个 文件:窗名=用例名 参数")
    按文件 = {}
    for 项 in a.改写:
        if ":" not in 项 or "=" not in 项.split(":", 1)[1]:
            raise SystemExit("改写参数格式=文件:窗名=用例名，实际：%r" % 项)
        文件, 余 = 项.split(":", 1)
        窗名, 用例名 = 余.split("=", 1)
        按文件.setdefault(文件, {})[窗名] = 用例名
    改数 = 0
    for 文件, 映射 in sorted(按文件.items()):
        路径 = os.path.join(HERE, 文件)
        if not os.path.isfile(路径):
            raise SystemExit("docs_data 文件不存在：%s" % 路径)
        n, 明细 = 改写文件(路径, 映射)
        改数 += n
        print("已改写 %s：%d 处" % (文件, n))
        for 项 in 明细:
            print("  " + 项)
    print("改写合计：%d 处" % 改数)
    return 0


# ---------------------------------------------------------------- 自测
def 自测():
    """正反用例：dict 代码键/dict 示例键/tuple 码块三形态改写 + 非法形态报错。"""
    失败 = []
    计数 = [0]

    def 断言(名, 实际, 期望):
        计数[0] += 1
        if 实际 != 期望:
            失败.append("%s：期望 %r，实际 %r" % (名, 期望, 实际))

    def 期望报错(名, 动作):
        计数[0] += 1
        try:
            动作()
        except SystemExit:
            return
        失败.append("%s 未报错" % 名)

    with tempfile.TemporaryDirectory(prefix="cn_migrate_") as tmp:
        用例目录 = os.path.join(tmp, "tests", "e2e", "900_目标用例")
        os.makedirs(用例目录)
        for 文件 in ("主.cn", "主.expected"):
            with io.open(os.path.join(用例目录, 文件), "w", encoding="utf-8") as f:
                f.write("// 自测\n")
        根 = os.path.join(tmp, "tests", "e2e")
        源 = (
            "# -*- coding: utf-8 -*-\n"
            "PAGES = [dict(文件='x.html', 章节=[dict(id='c', 块=[\n"
            "    dict(名='代码键.cn', 平台=['linux'],\n"
            "         代码='函数 主() -> 整32 { 返回 0; }', 输出='0\\n'),\n"
            "    dict(名='示例键.cn', 示例='函数 主() -> 整32 { 返回 1; }', 输出='1\\n'),\n"
            "    ('码', '元组窗.cn', '函数 主() -> 整32 { 返回 2; }', '2\\n'),\n"
            "])])]\n")
        路径 = os.path.join(tmp, "docs_data_测试.py")
        with io.open(路径, "w", encoding="utf-8") as f:
            f.write(源)
        映射 = {"代码键.cn": "900_目标用例",
                "示例键.cn": "目标用例",
                "元组窗.cn": "900_目标用例"}
        n, _明细 = 改写文件(路径, 映射, 根=根)
        断言("三形态改写处数", n, 3)
        with io.open(路径, encoding="utf-8") as f:
            新源 = f.read()
        断言("代码键已消除", "代码=" in 新源, False)
        断言("示例键已消除", "示例=" in 新源, False)
        断言("旧输出已消除", "输出=" in 新源, False)
        断言("三处引用写入",
             新源.count("用例='") + 新源.count('("用例", "'), 3)
        # 数据可加载且三窗均为引用形态
        import importlib.util
        规格 = importlib.util.spec_from_file_location("docs_data_自测", 路径)
        模块 = importlib.util.module_from_spec(规格)
        规格.loader.exec_module(模块)
        块们 = 模块.PAGES[0]["章节"][0]["块"]
        断言("dict 代码键窗名保留", 块们[0].get("名"), "代码键.cn")
        断言("dict 代码键平台字段保留", 块们[0].get("平台"), ["linux"])
        断言("dict 代码键引用", 块们[0].get("用例"), "900_目标用例")
        断言("dict 示例键引用", 块们[1].get("用例"), "目标用例")
        断言("tuple 已替换为用例引用", 块们[2], ("用例", "900_目标用例"))
        # 非法输入：窗名未命中 / 目标用例不存在 必须报错
        期望报错("窗名未命中",
                  lambda: 改写文件(路径, {"不存在的窗.cn": "900_目标用例"}, 根=根))
        期望报错("目标用例不存在",
                  lambda: 改写文件(路径, {"代码键.cn": "不存在的用例"}, 根=根))
        # 重名窗（两处同名）必须报错
        重名路径 = os.path.join(tmp, "docs_data_重名.py")
        with io.open(重名路径, "w", encoding="utf-8") as f:
            f.write("PAGES = [dict(名='同名.cn', 示例='a'),\n"
                    "         dict(名='同名.cn', 示例='b')]\n")
        期望报错("重名窗",
                  lambda: 改写文件(重名路径, {"同名.cn": "900_目标用例"}, 根=根))
        # 同块双代码键 / 负例标记 / tuple 编译足矣标记 必须报错（防静默丢语义）
        双键路径 = os.path.join(tmp, "docs_data_双键.py")
        with io.open(双键路径, "w", encoding="utf-8") as f:
            f.write("PAGES = [dict(名='双键.cn', 示例='a', 代码='b')]\n")
        期望报错("同块双代码键",
                  lambda: 改写文件(双键路径, {"双键.cn": "900_目标用例"}, 根=根))
        负例路径 = os.path.join(tmp, "docs_data_负例.py")
        with io.open(负例路径, "w", encoding="utf-8") as f:
            f.write("PAGES = [dict(名='负例.cn', 代码='a', 预期失败=True)]\n")
        期望报错("负例标记",
                  lambda: 改写文件(负例路径, {"负例.cn": "900_目标用例"}, 根=根))
        足矣路径 = os.path.join(tmp, "docs_data_足矣.py")
        with io.open(足矣路径, "w", encoding="utf-8") as f:
            f.write("PAGES = [('码', '足矣.cn', 'a', None, True)]\n")
        期望报错("tuple 编译足矣标记",
                  lambda: 改写文件(足矣路径, {"足矣.cn": "900_目标用例"}, 根=根))
        # 报告枚举三形态（含 tuple——旧实现按 Python tuple 判断的漏识别面）
        枚举 = {名: 形 for 名, 形 in
                ((名, 块形态(节点)) for 名, 节点, _行们 in 枚举文件(重名路径))}
        断言("枚举识别 dict 示例键", 枚举.get("同名.cn"), "dict·示例")

    if 失败:
        print("docs_migrate 自测：失败 %d 项" % len(失败))
        for 项 in 失败:
            print("  [×] " + 项)
        return 1
    print("docs_migrate 自测：全部通过（dict 代码键/dict 示例键/tuple 三形态改写"
          "+窗名未命中/目标缺失/重名/双代码键/负例标记/编译足矣标记报错 共 %d 项）"
          % 计数[0])
    return 0


def main():
    p = argparse.ArgumentParser(description="docs_data 内嵌示例→E2E 用例引用改写（1096）")
    p.add_argument("--报告", action="store_true")
    p.add_argument("--改写", nargs="*", metavar="文件:窗名=用例名")
    p.add_argument("--selftest", action="store_true")
    a = p.parse_args()
    if a.selftest:
        return 自测()
    if a.改写:
        return cmd_改写(a)
    if a.报告:
        return cmd_报告(a)
    p.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
