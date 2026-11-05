#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""dispatch_gate_selftest.py —— 分派门禁判别力自测（#1959+#2074+#2089·从
check_dispatch_gate.py 拆出·体量红线「先拆再加」：门禁主体与自测场景单一职责
分离）。被 check_dispatch_gate --selftest 入口调用·判据/扫描核经延迟导入复用
主模块（防模块级循环导入）。

场景架构：正反两态（六面命中形态真拦/精化证伪不误报）+三红各一例（漏登记/
登记无批准/协议注释类别非法）+基线演化与消减棘轮+幂等——零触碰真实树
（TemporaryDirectory 样例树·秒级）。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import json
import tempfile
from pathlib import Path


def _主():
    import check_dispatch_gate as g          # 延迟导入（防循环·sys.modules 缓存）
    return g


# 跨模块名绑定：搬移段内裸名引用（判据/扫描核/基线件在主模块）——入口延迟导入
# 次模块时主模块已加载完整，此处一次性绑定（sys.modules 缓存·零重复加载）
_绑定 = _主()
写基线 = _绑定.写基线
读基线 = _绑定.读基线
写旗标基线 = _绑定.写旗标基线             # 面⑤独立基线（#2074 双写之一）
读旗标基线 = _绑定.读旗标基线
判定 = _绑定.判定
全库扫描 = _绑定.全库扫描
默认值扫描 = _绑定.默认值扫描
面名们 = _绑定.面名们                 # 场景二/三 面遍历断言用
del _绑定


# ============ 自测场景（六面正反两态+三红+精化证伪+幂等·零触碰真实树） ============

样例宿主 = """// 分派默认样例（宿主）
int 查形态(int k) {
    switch (取种类(k)) {
    case 1: return 1;
    case 2: return 2;
    default:
        return false;          // default 段返字面量=命中
    }
}
int 查宽(表& t, 文本 k) {
    if (t.有(k)) { return t.宽(k); }
    else { return nullptr; }   // 裸 else 承载字面量=命中
}
int 用长(std::optional<int> o) {
    return o.value_or(8);      // 条款⑥命中
}
"""

样例嵌套 = """// 嵌套精化样例：嵌套块内 return 与 true 不判
int 嵌套样(int k) {
    switch (取种类(k)) {
    default:
        if (k < 0) { return false; }   // 嵌套块内·不判（精化）
        return true;                    // true 非默认值·不判
    }
}
int 链样(int x) {
    if (x > 0) { return 1; }
    else if (x < 0) { return 2; }
    else { return 0; }          // ← 终态 else 返 0=命中（此文件只放链样时命中 1）
}
"""

样例v2 = """// v2 样例
函数 取宽(文本 k) -> 整64 {
    如果 (k == "") {
        返回 0 - 1;        // 条款⑦还原写法返回=命中
    } 否则 {
        返回 假;           // 否则块返字面量=命中
    }
}
函数 槽样() -> 整64 {
    整64 槽 = 0 - 1;       // 还原赋值=命中
    如果 (槽 == 0 - 1) {   // 判据面·不判（不入还原面）
        中止();
    }
    返回 槽;
}
函数 整64样(文本 s) -> 类型 {
    如果 (s == "") {
        返回 正常(驻留("整64"));   // 整64兜底=命中
    }
    返回 正常(驻留(s));
}
"""

# 面④样例（#2074）：枚举分派 default 段三分形态（生产值/报错型/非识别面）
样例枚举默认 = """// 面④样例
int 分派样(int k) {
    switch (取节点(k).getType()) {
    case 1: return 1;
    default:
        return 兜底值(k);      // 生产值·无报错调用=面④命中
    }
}
int 报错样(int k) {
    switch (取节点(k).getType()) {
    case 1: return 1;
    default:
        diag.report(Error, 位置, "新枚举值未穷举");
        return 0;              // 段内 report( 词面=报错型合法·面④不命中
    }
}
int 裸枚举样(int op) {
    switch (op) {
    case 1: return 1;
    default:
        return 兜底值(op);     // 无 getType() 特征=不识别·面④不命中（诚实边界）
    }
}
int 赋值样(int k) {
    switch (取节点(k).getType()) {
    case 1: break;
    default:
        k = 2;                 // 赋值非 return=不入面④（只判 return 形态）
        break;
    }
    return k;
}
"""


def 自测写表(路径: Path, 条目们: list):
    路径.write_text(json.dumps({"版本": 1, "条目": 条目们}, ensure_ascii=False),
                 encoding="utf-8")


def 自测建场景(保活: list, 文件们: dict):
    """独立临时树 + 以当前实测生成基线（dispatch 面+旗标豁免面双基线）。
    返回 (根, 表, 基, 旗基)。"""
    td = tempfile.TemporaryDirectory(prefix="cdg1959_")
    保活.append(td)
    根 = Path(td.name)
    for 相对, 内容 in 文件们.items():
        p = 根 / 相对
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(内容, encoding="utf-8")
    表 = 根 / "registry.json"
    自测写表(表, [])
    基 = 根 / "baseline.json"
    旗基 = 根 / "flag_baseline.json"
    扫描果 = 全库扫描(根)
    写基线(基, 扫描果)
    写旗标基线(旗基, 扫描果)
    return 根, 表, 基, 旗基


def 自测跑(根: Path, 表: Path, 基: Path, 旗基: Path):
    """临时树门禁调用：返回 (rc, 判定果, 扫描果)。"""
    扫描果 = 全库扫描(根)
    条目们, _态 = 默认值扫描.读登记表(表)
    基线, _指纹 = 读基线(基)
    基线["旗标豁免"] = 读旗标基线(旗基)   # 面⑤并入判定（#2074）
    果 = 判定(扫描果, 条目们, 基线)
    return (1 if 果["红"] else 0), 果, 扫描果


def 红种们(果) -> set:
    return {r["种类"] for r in 果["红"]}


def 自测场景一(断言, 保活):
    """场景一：面①②③命中形态+精化证伪+基线演化（同一棵树顺序演化）。"""
    根, 表, 基, 旗基 = 自测建场景(保活, {
        "src/样例.cpp": 样例宿主 + 样例嵌套,
        "CN语言编译器v2/源.cn": 样例v2})
    # 基线生成前直接看扫描面形态（候选计数断言）
    扫描果 = 全库扫描(根)
    断言(len(扫描果["候选"]["分派默认"]) == 4,
        f"分派默认=4（default false/else nullptr/链尾 else 0/v2 否则假）: "
        f"{[p['锚'] for p in 扫描果['候选']['分派默认']]}")
    断言(len(扫描果["候选"]["value_or缺省"]) == 1, "value_or=1")
    断言(len(扫描果["候选"]["哨兵还原"]) == 2,
        f"哨兵还原=2（还原返回+还原赋值·判据 == 0-1 不判）: "
        f"{[p['锚'] for p in 扫描果['候选']['哨兵还原']]}")
    断言(len(扫描果["候选"]["else业务体"]) == 3,
        f"else业务体=3（查宽 else nullptr/链样 else 0/v2 否则假·无诊断 return=红）: "
        f"{[p['锚'] for p in 扫描果['候选']['else业务体']]}")
    断言(not any("嵌套" in p["锚"] and "0 - 1" in p["锚"]
               for p in 扫描果["候选"]["哨兵还原"]), "判据 == 0-1 不入还原面")
    # 基线生成→存量放行=绿
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0, f"基线生成后存量放行=绿: 红={果['红']!r}")
    断言(果["面统"]["分派默认"]["现测"] == 4, "分派默认现测=4")
    # 反态①：新增 default 返字面量位置=红
    样 = 根 / "src" / "样例.cpp"
    样.write_text(样例宿主 + 样例嵌套 +
                "int 新样(int k) {\n    switch (取种类(k)) {\n"
                "    default:\n        return 0;\n    }\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 1 and "新增分派默认" in 红种们(果), f"反态①新增 default 返 0 真拦: {红种们(果)}")
    # 正态①：同行协议注释+登记条（存量过渡）=放行
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "新样",
                "锚文本": "return 0;", "来源": "1959selftest",
                "类别": "分派默认", "静默风险": "低", "原因": "自测登记",
                "登记人": "selftest", "批准": "存量过渡"}])
    样.write_text(样例宿主 + 样例嵌套 +
                "int 新样(int k) {\n    switch (取种类(k)) {\n"
                "    default:\n        return 0;  // 静默豁免分派默认: 自测\n    }\n}\n",
                encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0, f"正态①协议注释(分派默认类)+登记条=放行: 红={果['红']!r}")
    断言(any(h["面"] == "分派默认" for h in 果["豁免"]), "豁免放行列表含分派默认")
    # 反态②：协议注释而登记表无条=漏登记
    自测写表(表, [])
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 1 and "漏登记" in 红种们(果), f"反态②漏登记（注释无条）: {红种们(果)}")
    # 反态③：登记条批准缺失=登记无批准（1238）
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "新样",
                "锚文本": "return 0;", "来源": "1959selftest",
                "类别": "分派默认", "静默风险": "低", "原因": "自测登记",
                "登记人": "selftest"}])
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言("登记无批准" in 红种们(果), f"反态③登记无批准（1238）: {红种们(果)}")
    # 反态④：协议注释类别不在枚举=红（枚举格式强制）
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "新样",
                "锚文本": "return 0;", "来源": "1959selftest",
                "类别": "随便写", "静默风险": "低", "原因": "自测登记",
                "登记人": "selftest", "批准": "已批"}])
    样.write_text(样例宿主 + 样例嵌套 +
                "int 新样(int k) {\n    switch (取种类(k)) {\n"
                "    default:\n        return 0;  // 静默豁免随便写: 类别非法\n    }\n}\n",
                encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言("协议注释类别非法" in 红种们(果), f"反态④协议注释类别非法=红: {红种们(果)}")
    # 正态②：位置消失=自动消减不红
    自测写表(表, [])
    样.write_text(样例嵌套, encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0 and 果["消减"]["分派默认"], f"正态②基线位置消失=自动消减: {果['消减']}")


def 自测场景二(断言, 保活):
    """场景二：value_or 与哨兵还原演化+幂等。"""
    根, 表, 基, 旗基 = 自测建场景(保活, {
        "src/干净.cpp": "int 求和(int a, int b) {\n    return a + b;\n}\n",
        "CN语言编译器v2/干净.cn": "函数 求和(整32 甲, 整32 乙) -> 整32 {\n    返回 甲 + 乙;\n}\n"})
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0 and all(果["面统"][面]["现测"] == 0 for 面 in 面名们),
        f"干净样例六面零命中: {果['面统']!r}")
    v2 = 根 / "CN语言编译器v2" / "干净.cn"
    v2.write_text("函数 求和(整32 甲, 整32 乙) -> 整32 {\n    返回 甲 + 乙;\n}\n"
                "函数 取槽(整64 甲) -> 整64 {\n    整64 槽 = 0 - 1;\n    返回 槽;\n}\n",
                encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 1 and "新增哨兵还原" in 红种们(果), f"反态⑤新增还原赋值真拦: {红种们(果)}")
    v2.write_text("函数 求和(整32 甲, 整32 乙) -> 整32 {\n    返回 甲 + 乙;\n}\n",
                encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0, f"正态③新增位置删除后回绿: 红={果['红']!r}")
    # 幂等：同一棵树重扫全等
    根2, 表2, 基2, _旗基2 = 自测建场景(保活, {
        "src/样例.cpp": 样例宿主, "CN语言编译器v2/源.cn": 样例v2})
    扫1 = 全库扫描(根2)
    扫2 = 全库扫描(根2)
    断言(json.dumps(扫1["候选"], ensure_ascii=False, sort_keys=True) ==
        json.dumps(扫2["候选"], ensure_ascii=False, sort_keys=True), "幂等：两遍全等")


def 自测场景三(断言, 保活):
    """场景三：基线缺失拒跑（防绕过）+ 字符串内形态不判。"""
    根3, 表3, _基3, _旗基3 = 自测建场景(保活, {
        "src/字符串样.cpp":
        'int 例() {\n    const char* s = "return false;  .value_or(0)";\n'
        "    if (s == nullptr) return 1;\n    return 0;\n}\n"})
    扫描果 = 全库扫描(根3)
    断言(all(len(扫描果["候选"][面]) == 0 for 面 in 面名们),
        f"字符串内形态不判（掩码）: {扫描果['候选']!r}")
    try:
        读基线(根3 / "不存在.json")
        断言(False, "基线缺失应拒跑")
    except SystemExit:
        断言(True, "基线缺失=拒跑（防绕过）")


def 自测场景四(断言, 保活):
    """场景四：面④「枚举默认」三态（#2074·001 §十四 default 唯一合法形态=
    报内部错误中止）——生产值红/报错型放行/非识别面与赋值形态不误报。"""
    根, 表, 基, 旗基 = 自测建场景(保活, {"src/枚举样.cpp": 样例枚举默认})
    # 基线生成前：面④命中=1（分派样生产值）；报错样/裸枚举样/赋值样不命中
    扫描果 = 全库扫描(根)
    枚四 = 扫描果["候选"]["枚举默认"]
    断言(len(枚四) == 1 and "兜底值(k)" in 枚四[0]["锚"],
        f"面④命中=1（仅分派样生产值）: {[p['锚'] for p in 枚四]}")
    断言(any(p["锚"].startswith("return 0") for p in 扫描果["候选"]["分派默认"]),
        "报错样的 return 0 仍归面①（字面量面无报错豁免·基线各自管）")
    # 基线生成→存量放行=绿
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0, f"面④基线生成后存量放行=绿: 红={果['红']!r}")
    # 反态①：新增「default 生产值且无报错」位置=红（锁面三态·态2）
    样 = 根 / "src" / "枚举样.cpp"
    样.write_text(样例枚举默认 +
                "int 新增样(int k) {\n    switch (取节点(k).getType()) {\n"
                "    case 1: return 1;\n    default:\n"
                "        return 备用表[k];\n    }\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 1 and "新增枚举默认" in 红种们(果),
        f"面④反态①新增 default 生产值真拦: {红种们(果)}")
    # 正态①：协议注释（复用「分派默认」类别）+登记条=放行
    自测写表(表, [{"文件": "src/枚举样.cpp", "函数": "新增样",
                "锚文本": "return 备用表[k];", "来源": "2074selftest",
                "类别": "分派默认", "静默风险": "低", "原因": "自测登记",
                "登记人": "selftest", "批准": "存量过渡"}])
    样.write_text(样例枚举默认 +
                "int 新增样(int k) {\n    switch (取节点(k).getType()) {\n"
                "    case 1: return 1;\n    default:\n"
                "        return 备用表[k];  // 静默豁免分派默认: 自测\n    }\n}\n",
                encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0, f"面④正态①协议注释+登记条=放行: 红={果['红']!r}")
    # 正态②：收账（--update-baseline 等效·放行位置正式入基线）后位置消失=
    # 自动消减不红（棘轮）
    写基线(基, 全库扫描(根))
    自测写表(表, [])
    样.write_text(样例枚举默认, encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0 and 果["消减"]["枚举默认"],
        f"面④正态②基线位置消失=自动消减: {果['消减']}")
    # 锁面三态·态3（正例全绿）：全 case + default 报错形态零命中
    样.write_text(
        "int 全case报错样(int k) {\n    switch (取节点(k).getType()) {\n"
        "    case 1: return 1;\n    case 2: return 2;\n    default:\n"
        "        diag.report(Error, 位置, \"缺口\");\n        return 0;\n"
        "    }\n}\n", encoding="utf-8")
    扫描果 = 全库扫描(根)
    断言(len(扫描果["候选"]["枚举默认"]) == 0,
        f"态3 全 case+default 报错=面④零命中: "
        f"{[p['锚'] for p in 扫描果['候选']['枚举默认']]}")


def 自测场景五(断言, 保活):
    """场景五：面⑥ else业务体 正反两态（else 退役·001 §14.7 要点①收严执法）。"""
    根, 表, 基, 旗基 = 自测建场景(保活, {
        "src/else样.cpp":
        "int 例甲(int k) {\n"
        "    if (k > 0) { return 1; }\n"
        "    else { k = 0; }              // 业务赋值=红\n"
        "}\n"
        "int 例乙(std::optional<int> o) {\n"
        "    if (o) { return *o; }\n"
        "    else { return -1; }           // 无诊断返回错误码=红（面②宽放行·本面收严）\n"
        "}\n"
        "int 例丙(int k) {\n"
        "    if (k > 0) { return 1; }\n"
        "    else if (k < 0) { return 2; }\n"
        "    else { 诊断.报错(\"k 非法\"); return 3; }   // 诊断+return 收尾=绿\n"
        "}\n"
        "int 例丁(int k) {\n"
        "    if (k > 0) { return 1; }\n"
        "    else { abort(); }             // 终止类=绿\n"
        "}\n"
        "int 例辛(int k) {\n"
        "    if (k > 0) { return 1; }\n"
        "    else { 打印到错误(\"否则 返回 假\"); }   // 字符串词面不触发·打印到错误=诊断绿\n"
        "}\n"})
    扫描果 = 全库扫描(根)
    断言(len(扫描果["候选"]["else业务体"]) == 2,
        f"面⑥命中=2（业务赋值+无诊断返错误码·诊断收尾/终止/字符串词面不判）: "
        f"{[p['锚'] for p in 扫描果['候选']['else业务体']]}")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0, f"基线生成后存量放行=绿: 红={果['红']!r}")
    # 反态⑥：新增 else 业务赋值=红
    样 = 根 / "src" / "else样.cpp"
    原文 = 样.read_text(encoding="utf-8")
    样.write_text(原文 + "int 例戊(int k) {\n    if (k > 0) { return 1; }\n"
                "    else { k = 5; }              // 新增业务赋值=红\n}\n",
                encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 1 and "新增else业务体" in 红种们(果),
        f"反态⑥新增 else 业务真拦: {红种们(果)}")
    # 正态⑥：诊断化改写=自动消减回绿
    样.write_text(原文.replace(
        "else { k = 0; }              // 业务赋值=红",
        "else { 诊断.报错(\"k 非法\"); }   // 诊断化改写=消减"), encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基, 旗基)
    断言(rc == 0 and 果["消减"]["else业务体"],
        f"正态⑥诊断化改写=自动消减: {果['消减']!r}")
    # v2 侧：否则 业务=红；显式错误构造「返回 错误(...)=诊断传播」绿
    根2, _t2, _b2, _旗基2 = 自测建场景(保活, {
        "CN语言编译器v2/否则样.cn":
        "函数 例一(甲: 整32) -> 整32 {\n"
        "    如果 (甲 > 0) { 返回 甲; }\n"
        "    否则 { 甲 = 0; }              // 业务赋值=红\n"
        "}\n"
        "函数 例二(甲: 整32) -> 结果<整32, 整32> {\n"
        "    如果 (甲 > 0) { 返回 正常(甲); }\n"
        "    否则 { 返回 错误(6); }        // 显式错误构造=诊断传播绿\n"
        "}\n"})
    扫描果2 = 全库扫描(根2)
    断言(len(扫描果2["候选"]["else业务体"]) == 1,
        f"v2 面⑥命中=1（否则业务赋值·错误构造传播不判）: "
        f"{[p['锚'] for p in 扫描果2['候选']['else业务体']]}")
    # 单语句 else（无块形）同判（面②同口径·切分号）
    根3, _t3, _b3, _旗基3 = 自测建场景(保活, {
        "src/单语句样.cpp":
        "int 例己(int k) {\n"
        "    if (k > 0) { return 1; }\n"
        "    else k = 2;                   // 单语句 else 业务=红\n"
        "}\n"})
    扫描果3 = 全库扫描(根3)
    断言(len(扫描果3["候选"]["else业务体"]) == 1,
        f"单语句 else 业务=红: {扫描果3['候选']['else业务体']!r}")
    # else-if 链延续不判（非终态）+ 空 else 块=合法
    根4, _t4, _b4, _旗基4 = 自测建场景(保活, {
        "src/链样.cpp":
        "int 例壬(int k) {\n"
        "    if (k > 0) { return 1; }\n"
        "    else if (k < 0) { return 2; }\n"
        "    else if (k == 0) { return 0; }\n"
        "    else { }\n"
        "}\n"})
    扫描果4 = 全库扫描(根4)
    断言(len(扫描果4["候选"]["else业务体"]) == 0,
        f"else-if 链延续与空 else 块不判: {扫描果4['候选']['else业务体']!r}")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    保活 = []                            # TemporaryDirectory 保活（防提前清理）
    for 场景 in (自测场景一, 自测场景二, 自测场景三, 自测场景四, 自测场景五):
        try:
            场景(断言, 保活)
        except Exception as e:           # 场景异常=自测失败（显形不静默）
            失败.append(f"{场景.__name__} 异常：{e!r}")
    if 失败:
        print("== 门禁自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 门禁自测全过（{通过[0]} 项断言·六面正反两态/精化证伪/三红各一例/幂等）==")
    return 0
