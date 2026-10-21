#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""1170 并行 codegen 标签前缀表维护守卫（1145 风险的机械化防复发）。

背景（1145 实现）：函数级并行 codegen 让每函数从 0 局部编号标签，拼接期按
「前序函数标签总数」做文本平移（parallel_codegen::shiftLabelNumbers），平移
范围=各后端显式维护的 prefix 表（x64LabelPrefixes / linuxX64LabelPrefixes /
arm64LabelPrefixes）。风险：未来某后端新增由 ptrCheckCounter_ 派生的标签前缀
（如 "@new_okN" / "LnewN"）而忘记同步 prefix 表——并行模式漏平移，局部编号
串到别的函数编号区间，产物静默漂移（串行/并行都「看起来对」，仅逐字节对拍
才可见）。本守卫把「发射源里的计数器标签前缀集合」与「prefix 表」机械对齐：

  ① 发射点扫描：三后端目录（.cpp/.hpp）内，`"<前缀>" + std::to_string(<表达式>)`
     且表达式=ptrCheckCounter_ 或由 ptrCheckCounter_ 赋值的局部计数变量
     （如 `const int checkId = ptrCheckCounter_++;`）→ 必属函数级标签前缀；
  ② 表提取：从各后端表函数体内的 `prefixes = { ... }` 提取全部字符串字面量；
  ③ 对照三面（任一红即失败）：
     - 缺失：发射点前缀 ∉ 表（本守卫的核心目的——并行漏平移静默漂移）；
     - 陈旧：表项在源码无发射点（表必须与实际发射一一对应，防复制粘贴脏表）；
     - 未分类：标签样式字面量（@xxx / Lxxx）参与了 to_string 拼接但既非计数器
       派生、也不在「非函数级标签白名单」（模块级 .data 标签等）——提示人工
       归类，禁静默放过；
  ④ 接线核验：表函数必须以实参形态传入 generateFunctionBodies（防表存在但
     并行入口接错表）。

诚实边界：本守卫是源码形态静态扫描，覆盖标准拼接形态（`"前缀" + std::to_string`）；
若未来以非常规形态（运行时拼前缀/间接函数返回）发射标签，扫描器会以「陈旧表项」
或「未分类字面量」变红而非静默放过——红即人工核查（宁可误报不可漏报）。

用法：
  python scripts/check_parallel_prefix.py              # 检查真实仓库
  python scripts/check_parallel_prefix.py --selftest    # 正反两态自测（临时夹具）
  python scripts/check_parallel_prefix.py --root <目录> # 指定仓库根（自测/复用）
退出码：0 = 一致；1 = 违例；2 = 环境/用法错误。
"""
import argparse
import io
import re
import sys
import tempfile
from pathlib import Path

try:  # 控制台编码（中文诊断防 GBK 崩——与 check_cross_asm_guards 同口径）
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
except Exception:
    pass

仓库根 = Path(__file__).resolve().parent.parent

# 三后端配置：发射源目录 + 表函数名 + 表所属文件（相对仓库根）+ 非函数级标签白名单。
#   白名单=参与 `"字面量" + std::to_string(...)` 但**不属于** ptrCheckCounter_ 派生
#   函数级标签的字面量（模块级 .data 段标签：串行/并行都由主线程先行发射，不参与
#   函数内标签平移）。白名单条目陈旧（源码不再出现）只黄提示不阻断。
后端们 = [
    {
        "名": "x64",
        "目录": "src/cn_compiler/codegen/x64",
        "表函数": "x64LabelPrefixes",
        "表文件": "src/cn_compiler/codegen/x64/x64_codegen.cpp",
        "非函数级标签": {
            "@fp": "浮点常量池标签（模块级 .data 段，主线程预发射，不参与函数级平移）",
            "@str": "字符串常量池标签（模块级 .data 段，同上）",
        },
    },
    {
        "名": "linux_x64",
        "目录": "src/cn_compiler/codegen/linux_x64",
        "表函数": "linuxX64LabelPrefixes",
        "表文件": "src/cn_compiler/codegen/linux_x64/linux_x64_codegen.cpp",
        "非函数级标签": {
            "Lfp": "浮点常量池标签（模块级 .data 段，主线程预发射）",
            "L128c": "i128 常量池标签（模块级 .data 段，主线程预发射）",
            "Lstr": "字符串常量池标签（模块级 .data 段）",
        },
    },
    {
        "名": "arm64",
        "目录": "src/cn_compiler/codegen/arm64",
        "表函数": "arm64LabelPrefixes",
        "表文件": "src/cn_compiler/codegen/arm64/arm64_codegen.cpp",
        "非函数级标签": {
            "Lfp": "浮点常量池标签（模块级 .data 段，主线程预发射）",
            "Lstr": "字符串常量池标签（模块级 .data 段）",
        },
    },
]

# 局部计数变量声明：`[const] int/auto/... 名 = ptrCheckCounter_[++|--|± 常量];`
#   例：`const int checkId = ptrCheckCounter_++;`（含前/后缀自增与算术偏移形态）
计数变量模式 = re.compile(
    r"(?:const\s+)?(?:unsigned\s+int|unsigned|long\s+long|long|int|std::size_t|"
    r"size_t|auto)\s+([A-Za-z_]\w*)\s*=\s*"
    r"(?:\+\+|--)?\s*ptrCheckCounter_\s*(?:\+\+|--|[+-]\s*\d+)?\s*;")

# 标签样式字面量：`@xxx`（MASM）或 `Lxxx`（GAS）纯标识符形态；其余指令文本/栈地址
#   （"[rbp"、", "、"xmm" 等）不参与标签归类。
标签样式模式 = re.compile(r"@?[A-Za-z_][A-Za-z0-9_]*\Z")


def 匹配右括号(文本: str, 左括号: int) -> int:
    """返回与 文本[左括号]=='(' 配对的 ')' 下标；未配对返回 -1（跳过字符串字面量）。"""
    深度 = 0
    i = 左括号
    in_str = False
    while i < len(文本):
        c = 文本[i]
        if in_str:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c == "(":
            深度 += 1
        elif c == ")":
            深度 -= 1
            if 深度 == 0:
                return i
        i += 1
    return -1


def 向前取字面量(文本: str, 位置: int):
    """从 位置（std::to_string 起点）向前找 `<字面量> + ` 形态。

    返回 (字面量内容, 字面量起始偏移) 或 None。跨行/空白容错。"""
    j = 位置 - 1
    while j >= 0 and 文本[j] in " \t\r\n":
        j -= 1
    if j < 0 or 文本[j] != "+":
        return None
    j -= 1
    while j >= 0 and 文本[j] in " \t\r\n":
        j -= 1
    if j < 0 or 文本[j] != '"':
        return None
    起 = j - 1
    while 起 >= 0:
        if 文本[起] == '"' and (起 == 0 or 文本[起 - 1] != "\\"):
            break
        起 -= 1
    if 起 < 0:
        return None
    return 文本[起 + 1:j], 起


def 收集计数变量(文本: str) -> set:
    """文件内由 ptrCheckCounter_ 直接赋值的局部变量名集合。"""
    return set(计数变量模式.findall(文本))


def 扫描发射点(根: Path, 后端: dict):
    """扫描后端目录内全部 .cpp/.hpp，返回 (计数器标签命中, 非计数器标签样式命中)。

    命中=(字面量, 相对路径, 行号)。"""
    计数器命中 = []
    非计数器命中 = []
    目录 = 根 / 后端["目录"]
    if not 目录.is_dir():
        raise FileNotFoundError(f"后端目录不存在: {目录}")
    for 文件 in sorted(list(目录.glob("*.cpp")) + list(目录.glob("*.hpp"))):
        文本 = 文件.read_text(encoding="utf-8", errors="replace")
        计数变量 = 收集计数变量(文本)
        for m in re.finditer(r"std::to_string\s*\(", 文本):
            左 = m.end() - 1
            右 = 匹配右括号(文本, 左)
            if 右 < 0:
                continue
            表达式 = 文本[左 + 1:右].strip()
            取出 = 向前取字面量(文本, m.start())
            if 取出 is None:
                continue
            字面量, 偏移 = 取出
            if not 标签样式模式.fullmatch(字面量):
                continue
            if not (字面量.startswith("@") or 字面量.startswith("L")):
                continue
            行号 = 文本.count("\n", 0, 偏移) + 1
            首标识 = re.match(r"[A-Za-z_]\w*", 表达式)
            派生 = ("ptrCheckCounter_" in 表达式 or
                    (首标识 is not None and 首标识.group(0) in 计数变量))
            项 = (字面量, 文件.relative_to(根).as_posix(), 行号)
            if 派生:
                计数器命中.append(项)
            else:
                非计数器命中.append(项)
    return 计数器命中, 非计数器命中


def 提取前缀表(文本: str, 表函数: str):
    """从表函数体的 prefixes 初始化列表提取前缀集合；返回 (集合, 错误或 None)。"""
    签名 = re.search(
        r"const\s+std::vector<std::string>\s*&\s*" + re.escape(表函数) +
        r"\s*\(\s*\)\s*\{", 文本)
    if 签名 is None:
        return None, f"未找到前缀表函数定义 {表函数}()"
    左花括号 = 签名.end() - 1
    深度 = 0
    i = 左花括号
    右花括号 = -1
    while i < len(文本):
        if 文本[i] == "{":
            深度 += 1
        elif 文本[i] == "}":
            深度 -= 1
            if 深度 == 0:
                右花括号 = i
                break
        i += 1
    if 右花括号 < 0:
        return None, f"前缀表函数 {表函数}() 花括号未配对"
    体 = 文本[左花括号 + 1:右花括号]
    if "prefixes" not in 体:
        return None, f"前缀表函数 {表函数}() 内未找到 prefixes 初值列"
    表 = set(re.findall(r'"((?:[^"\\\n]|\\.)*)"', 体))
    if not 表:
        return None, f"前缀表函数 {表函数}() 的 prefixes 为空"
    return 表, None


def 检查接线(文本: str, 表函数: str) -> bool:
    """表函数是否以实参形态（<表函数>()）传入 generateFunctionBodies 调用。"""
    for m in re.finditer(r"generateFunctionBodies\s*\(", 文本):
        左 = m.end() - 1
        右 = 匹配右括号(文本, 左)
        if 右 < 0:
            continue
        调用文本 = 文本[左 + 1:右]
        if re.search(re.escape(表函数) + r"\s*\(\s*\)", 调用文本):
            return True
    return False


def 检查后端(根: Path, 后端: dict):
    """单后端核对；返回 (错误列表, 警告列表, 统计文本)。"""
    错误 = []
    警告 = []
    计数器命中, 非计数器命中 = 扫描发射点(根, 后端)
    表文件 = 根 / 后端["表文件"]
    if not 表文件.is_file():
        return ([f"[{后端['名']}] 表文件不存在: {后端['表文件']}"], 警告, "扫描失败")
    表文本 = 表文件.read_text(encoding="utf-8", errors="replace")
    表, 表错误 = 提取前缀表(表文本, 后端["表函数"])
    if 表错误 is not None:
        return ([f"[{后端['名']}] {表错误}"], 警告, "扫描失败")

    发射前缀 = {项[0] for 项 in 计数器命中}
    for 字面量, 路径, 行号 in 计数器命中:
        if 字面量 not in 表:
            错误.append(f"[{后端['名']}] 缺失：标签前缀 \"{字面量}\" 在 {路径}:{行号} "
                        f"由 ptrCheckCounter_ 派生发射，但未登记 {后端['表函数']} 前缀表"
                        f"（并行模式将漏平移 → 产物静默漂移）")
    for 前缀 in sorted(表 - 发射前缀):
        错误.append(f"[{后端['名']}] 陈旧：前缀表项 \"{前缀}\" 在 {后端['目录']} 源码中"
                    f"无 ptrCheckCounter_ 派生发射点（前缀表必须与实际发射一一对应）")

    白名单 = 后端["非函数级标签"]
    已见白名单 = set()
    for 字面量, 路径, 行号 in 非计数器命中:
        if 字面量 in 表:
            错误.append(f"[{后端['名']}] 归类冲突：\"{字面量}\" 既是非计数派生字面量"
                        f"（{路径}:{行号}）又登记在函数级前缀表中")
        elif 字面量 in 白名单:
            已见白名单.add(字面量)
        else:
            错误.append(f"[{后端['名']}] 未分类：标签样式字面量 \"{字面量}\"（{路径}:{行号}）"
                        f"参与 to_string 拼接但非 ptrCheckCounter_ 派生，且不在非函数级"
                        f"标签白名单——请人工归类（函数级→补前缀表；模块级→补白名单+理由）")
    for 字面量 in sorted(set(白名单) - 已见白名单):
        警告.append(f"[{后端['名']}] 白名单陈旧：\"{字面量}\" 未在源码中检出（可清理）")

    if not 检查接线(表文本, 后端["表函数"]):
        错误.append(f"[{后端['名']}] 接线缺失：{后端['表文件']} 未把 {后端['表函数']}() "
                    f"传入 generateFunctionBodies（并行入口未接表）")

    统计 = (f"{后端['名']}: 发射前缀 {len(发射前缀)} 项 / 表 {len(表)} 项 / "
            f"非函数级 {len(已见白名单)} 项")
    return 错误, 警告, 统计


def 检查(根: Path):
    """逐后端核对；返回 (错误, 警告, 统计行列表)。"""
    错误 = []
    警告 = []
    统计 = []
    for 后端 in 后端们:
        try:
            本次错误, 本次警告, 本次统计 = 检查后端(根, 后端)
        except FileNotFoundError as 异常:
            错误.append(f"[环境] {异常}")
            continue
        错误.extend(本次错误)
        警告.extend(本次警告)
        统计.append(本次统计)
    return 错误, 警告, 统计


# ==================== 自检（正反两态夹具） ====================

def _写(路径: Path, 文本: str) -> None:
    路径.parent.mkdir(parents=True, exist_ok=True)
    路径.write_text(文本, encoding="utf-8")


def _造夹具(根: Path, 后端: dict, 额外发射: str = "", 额外表项: str = "",
             去掉接线: bool = False) -> None:
    """在 根 下构造某后端的最小源码夹具（发射文件 + 表文件）。"""
    前缀甲 = "@ptr_ok" if 后端["名"] == "x64" else "Lptr_ok"
    前缀乙 = "@ovf" if 后端["名"] == "x64" else "Lovf"
    数据标签 = "@str" if 后端["名"] == "x64" else "Lstr"
    表函数 = 后端["表函数"]
    发射文件 = 根 / 后端["目录"] / f"{后端['名']}_instructions.cpp"
    _写(发射文件, f"""// 自检夹具：标准发射点形态（直接计数器 + 局部计数变量 + 模块级数据标签）
static void 示例发射(void* writer) {{
    const int checkId = ptrCheckCounter_++;
    const std::string a = "{前缀甲}" + std::to_string(checkId);
    const std::string b = "{前缀乙}" + std::to_string(ptrCheckCounter_++);
    const std::string c = "{数据标签}" + std::to_string(i);
    writer->line(a + b + c);
{额外发射}}}
""")
    额外 = ("\n        " + 额外表项) if 额外表项 else ""
    接线 = "" if 去掉接线 else f"""    if (!parallel_codegen::generateFunctionBodies(
            module, {表函数}(), makeGenerator, diagnostics_, texts)) {{
        return false;
    }}
"""
    _写(根 / 后端["表文件"], f"""// 自检夹具：前缀表 + 并行入口接线
namespace {{
const std::vector<std::string>& {表函数}() {{
    static const std::vector<std::string> prefixes = {{
        "{前缀甲}", "{前缀乙}"{额外}}};
    return prefixes;
}}
}}  // namespace

bool Gen::emitFunctionBodiesParallel(...) {{
    std::vector<std::string> texts;
{接线}    return true;
}}
""")


def 自检() -> bool:
    """正反两态自测：正常=绿；每类漏登记/脏表/未分类/接线缺失=必红。"""
    用例们 = [
        ("完整登记", "", "", False, False, ""),
        ("新增直接派生前缀未登记", '    const std::string d = "@new_ok" + '
         'std::to_string(ptrCheckCounter_++);\n', "", False, True, "@new_ok"),
        ("新增局部变量派生前缀未登记", '    const int newId = ptrCheckCounter_++;\n'
         '    const std::string e = "@ind_ok" + std::to_string(newId);\n',
         "", False, True, "@ind_ok"),
        ("前缀自增派生前缀未登记", '    const int preId = ++ptrCheckCounter_;\n'
         '    const std::string g = "@pre_ok" + std::to_string(preId);\n',
         "", False, True, "@pre_ok"),
        ("表项陈旧（源码无发射点）", "", ', "@ghost_ok"', False, True, "@ghost_ok"),
        ("标签样式字面量未分类", '    const std::string f = "@newdata" + '
         'std::to_string(i);\n', "", False, True, "@newdata"),
        ("并行入口接线缺失", "", "", True, True, "接线缺失"),
    ]
    全部通过 = True
    for 名称, 额外发射, 额外表项, 去掉接线, 期望红, 锚 in 用例们:
        with tempfile.TemporaryDirectory(prefix="prefix_guard_") as 临时:
            根 = Path(临时)
            for 后端 in 后端们:
                _造夹具(根, 后端, 额外发射 if 后端["名"] == "x64" else "",
                        额外表项 if 后端["名"] == "x64" else "", 去掉接线)
            错误, 警告, _ = 检查(根)
            实际红 = bool(错误)
            命中锚 = any(锚 in 错误项 for 错误项 in 错误) if 期望红 else True
            通过 = (实际红 == 期望红) and 命中锚
            print(f"  [自检] {名称}：期望{'红' if 期望红 else '绿'} "
                  f"实际{'红' if 实际红 else '绿'} → {'✓' if 通过 else '✗'}")
            if not 通过:
                全部通过 = False
                for 错误项 in 错误:
                    print(f"         {错误项}")
    print(f"[自检] {'PASS ✓' if 全部通过 else 'FAIL ✗'}"
          f"（{len(用例们)} 例正反两态）")
    return 全部通过


def main() -> int:
    ap = argparse.ArgumentParser(
        description="1170 并行 codegen 标签前缀表维护守卫（三后端发射源 ⇔ prefix 表）")
    ap.add_argument("--root", default=str(仓库根), help="仓库根目录（默认脚本上一级）")
    ap.add_argument("--selftest", action="store_true", help="运行正反两态自测夹具")
    参数 = ap.parse_args()
    根 = Path(参数.root).resolve()
    if 参数.selftest:
        return 0 if 自检() else 1
    错误, 警告, 统计 = 检查(根)
    for 行 in 警告:
        print(f"[黄] {行}")
    for 行 in 统计:
        print(f"[统计] {行}")
    if 错误:
        print(f"[并行前缀守卫] FAIL：{len(错误)} 项违例", file=sys.stderr)
        for 行 in 错误:
            print(f"  - {行}", file=sys.stderr)
        return 1
    print("[并行前缀守卫] PASS ✓（三后端发射前缀与 prefix 表逐项一致·接线在）")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
