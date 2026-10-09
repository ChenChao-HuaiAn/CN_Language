#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分层依赖方向门禁（334·重构A 执法网）——AGENTS §3「模块单向依赖」的机械化执行点。

背景（2026-10-09 勘察实证）：宿主 ir 层 19/20 文件反向 include semantic（38 次）、
codegen 三后端 15 文件同款（21 次）、parser→semantic 2 次、ir.hpp→parser/ast.hpp
传染 opt 全层；v2 侧语法↔IR 包级互引；CMakeLists 把违规边写成 PUBLIC 链接——
纪律此前只存在于 AGENTS.md 一句话，零执行点。本脚本=双面执法：
  - C++ 面：扫 src/ 的 #include "cn_compiler/X/..." 对层白名单矩阵；
  - CN 面：扫 CN语言编译器v2/ 各包的 导入 X; 对包级白名单矩阵（stdlib 包放行）。

冻结线（242 同款·file_length_baseline 惯例）：存量违规入基线（只减不增），
新增违规零容忍——重构任务 B/C/D 逐批清基线，清零后白名单即硬门禁。

目标态白名单（与 AGENTS §3 同步维护·改白名单须同轮改 AGENTS 并随重构任务批）：
  C++：common←model←{lexer→parser→semantic}；ir={common,model}；
       opt/codegen={common,model,ir}；module={common,model,lexer,parser}；
       driver/cn_main 组合全部；runtime 独立（零 cn_compiler 依赖）。
  CN： 词法→语法树→IR→语法→语义→代码生成→主（上游=可依赖集按表；
       布局权威表住 IR 包·语义/代码生成→IR 属下游消费上游=合规——与宿主
       「下游回查上游实现」的违规形态不同：v2 包声明序=加载序天然单向拓扑）。

用法：
  python scripts/check_layer_deps.py                 # 冻结线检查（门禁用·rc=1=有新增违规）
  python scripts/check_layer_deps.py --report        # 报告当前全部违规（不拦·重构排班清账用）
  python scripts/check_layer_deps.py --update-baseline   # 以当前实测重写基线（清账后收基线）
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

基线文件 = Path("scripts/layer_deps_baseline.json")

# ============ C++ 面：目录→层映射 + 层白名单（目标态） ============
CPP_DIRS = {
    "src/cn_compiler/common": "common",
    "src/cn_compiler/model": "model",        # 任务C 建树；白名单先注册（目录缺席不报错）
    "src/cn_compiler/lexer": "lexer",
    "src/cn_compiler/parser": "parser",
    "src/cn_compiler/semantic": "semantic",
    "src/cn_compiler/ir": "ir",
    "src/cn_compiler/opt": "opt",
    "src/cn_compiler/codegen": "codegen",
    "src/cn_compiler/module": "module",
    "src/cn_compiler/driver": "driver",
    "src/runtime": "runtime",
}
CPP_SRC_SUFFIXES = (".cpp", ".hpp", ".h", ".cc")

CPP_ALLOW = {
    "common": set(),
    "model": {"common"},
    "lexer": {"common"},
    "parser": {"common", "model", "lexer"},
    "semantic": {"common", "model", "lexer", "parser"},
    "ir": {"common", "model"},                      # 目标态：不依赖 semantic（SemanticView 接口化）
    "opt": {"common", "model", "ir"},
    "codegen": {"common", "model", "ir"},
    "module": {"common", "model", "lexer", "parser"},
    "driver": {"common", "model", "lexer", "parser", "semantic", "ir", "opt", "codegen", "module"},
    "main": {"common", "model", "lexer", "parser", "semantic", "ir", "opt", "codegen", "module", "driver"},
    "runtime": set(),                                # 只许标准库+自含
}

_INCLUDE_RE = re.compile(r'#include\s+["<](cn_compiler/[^">]+)[">]')

# ============ CN 面：v2 树内包 + 包白名单（目标态） ============
V2_ROOT = Path("CN语言编译器v2")
CN_PACKS = ("词法", "语法树", "IR", "语法", "语义", "代码生成", "货舱解析")  # 语法树=任务B 建

CN_ALLOW = {
    "词法": set(),
    "语法树": {"词法"},
    "IR": {"词法", "语法树"},
    "语法": {"词法", "语法树"},                       # 目标态：节点常量下沉语法树后不再依赖 IR
    "语义": {"词法", "语法树", "IR", "语法"},
    "代码生成": {"词法", "语法树", "IR"},
    "货舱解析": set(),
    "主": set(CN_PACKS),                             # 主 导入全部（组合入口）
}

_IMPORT_RE = re.compile(r'^\s*导入\s+([^\s;]+)\s*;')


def cpp_层归属(仓库根: Path, 文件: Path):
    """文件路径→层名；未知位置返回 None（打印警告放行·防新目录误杀）。"""
    相对 = 文件.relative_to(仓库根).as_posix()
    if 相对 == "src/cn_main.cpp":
        return "main"
    # 最长前缀匹配（防前缀串扰：codegen/x64 等子目录落 codegen）
    最佳, 层名 = "", None
    for 前缀, 层 in CPP_DIRS.items():
        if 相对.startswith(前缀 + "/") and len(前缀) > len(最佳):
            最佳, 层名 = 前缀, 层
    return 层名


def cpp_违规集(仓库根: Path):
    违规 = []
    for f in sorted((仓库根 / "src").rglob("*")):
        if f.suffix not in CPP_SRC_SUFFIXES or not f.is_file():
            continue
        来源层 = cpp_层归属(仓库根, f)
        if 来源层 is None:
            print(f"[警告] 未分层位置的源文件，跳过检查：{f.relative_to(仓库根).as_posix()}")
            continue
        try:
            文本 = f.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for m in _INCLUDE_RE.finditer(文本):
            目标路径 = m.group(1)                    # 如 cn_compiler/semantic/type_system.hpp
            段 = 目标路径.split("/")
            目标层 = 段[1] if len(段) >= 2 else None
            if 目标层 is None:
                print(f"[警告] cn_compiler 根级 include 未能定层，跳过：{目标路径}")
                continue
            if 目标层 not in CPP_ALLOW:
                print(f"[警告] 未知层目录 include，跳过：{目标路径}")
                continue
            if 目标层 == 来源层:
                continue                            # 同层内部依赖不拦
            if 目标层 not in CPP_ALLOW[来源层]:
                违规.append((f.relative_to(仓库根).as_posix(), 目标路径))
    return 违规


def cn_违规集(仓库根: Path):
    v2 = 仓库根 / V2_ROOT
    if not v2.exists():
        return []
    违规 = []
    for f in sorted(v2.rglob("*.cn")):
        相对 = f.relative_to(仓库根).as_posix()
        if f.name == "包.cn":
            所在包 = f.parent.name                   # <包>/包.cn（包根在包目录自身）
        else:
            所在包 = f.parent.name if f.parent != v2 else "主"
        if 所在包 not in CN_ALLOW:
            print(f"[警告] v2 树内未知包位置，跳过检查：{相对}")
            continue
        try:
            文本 = f.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for 行 in 文本.splitlines():
            m = _IMPORT_RE.match(行)
            if not m:
                continue
            包名 = m.group(1).split("::")[0]
            if 包名 == 所在包:
                continue                            # 同包导入不构成跨包依赖
            if 包名 in CN_PACKS and 包名 not in CN_ALLOW[所在包]:
                违规.append((相对, 包名))
            # 包名不在 CN_PACKS=stdlib 宿主包（容器/文件/…）→放行
    return 违规


def 当前违规(仓库根: Path):
    cpp = cpp_违规集(仓库根)
    cn = cn_违规集(仓库根)
    return {"cpp": cpp, "cn": cn}


def 报告(仓库根: Path) -> int:
    违规 = 当前违规(仓库根)
    print("== check_layer_deps --report（334 执法网·目标态白名单）==")
    for 面, 条目 in 违规.items():
        print(f"-- {面} 面：{len(条目)} 处违规 --")
        for 文件, 目标 in 条目:
            print(f"  {文件}  ->  {目标}")
    总数 = sum(len(v) for v in 违规.values())
    print(f"[报告] 合计 {总数} 处（报告模式不拦截；清账用 --update-baseline 收基线）")
    return 0


def 按文件分组(条目):
    """[(文件,目标)] → {文件: [目标…]}（目标排序去重）。"""
    分组 = {}
    for f, t in 条目:
        分组.setdefault(f, set()).add(t)
    return {f: sorted(ts) for f, ts in 分组.items()}


def 冻结线检查(仓库根: Path, 更新基线: bool) -> int:
    违规 = 当前违规(仓库根)
    实测 = {"cpp": 按文件分组(违规["cpp"]), "cn": 按文件分组(违规["cn"])}
    基线路径 = 仓库根 / 基线文件
    if 更新基线:
        实测["_注释"] = ("334 重构A 冻结基线：存量依赖违规清单（文件→违规目标）·只减不增——"
                         "新增违规即红；清账（重构B/C/D）后 --update-baseline 收基线")
        基线路径.write_text(json.dumps(实测, ensure_ascii=False, indent=1, sort_keys=True),
                            encoding="utf-8")
        总数 = sum(len(v) for v in 违规.values())
        print(f"[冻结线] 基线已更新（cpp {len(实测['cpp'])} 文件 + cn {len(实测['cn'])} 文件"
              f"·{总数} 处违规·{基线文件}）")
        return 0
    if not 基线路径.exists():
        print(f"[冻结线·红] 基线缺失：{基线文件}（首次用 --update-baseline 生成）")
        return 1
    基线 = json.loads(基线路径.read_text(encoding="utf-8"))
    红们 = []
    收账提示 = []
    for 面 in ("cpp", "cn"):
        基 = 基线.get(面, {})
        现 = 实测[面]
        for 文件 in sorted(set(基) | set(现)):
            旧目标 = set(基.get(文件, []))
            新目标 = set(现.get(文件, []))
            for t in sorted(新目标 - 旧目标):
                红们.append(f"[{面}] 新增违规  {文件}  ->  {t}")
            for t in sorted(旧目标 - 新目标):
                收账提示.append(f"[{面}] 已清账  {文件}  ->  {t}")
    print(f"== check_layer_deps（334 冻结线）== 基线 cpp {len(基线.get('cpp', {}))} 文件 + "
          f"cn {len(基线.get('cn', {}))} 文件")
    if 红们:
        for r in 红们:
            print(f"  [×] {r}")
        print(f"[冻结线·红] {len(红们)} 处新增跨层违规——禁止引入（存量见基线·重构任务逐批清）")
        return 1
    if 收账提示:
        for r in 收账提示:
            print(f"  [↓] {r}")
        print(f"[冻结线·绿] 无新增违规；{len(收账提示)} 处已清账——请跑 --update-baseline 收基线")
        return 0
    print("[冻结线·绿] 依赖方向零新增违规（存量冻结只减不增）")
    return 0


def main():
    ap = argparse.ArgumentParser(description="分层依赖方向门禁（C++ include + CN 导入 双面）")
    ap.add_argument("--root", default=".", help="仓库根目录")
    ap.add_argument("--report", action="store_true", help="报告当前全部违规（不拦截）")
    ap.add_argument("--update-baseline", action="store_true",
                    help="以当前实测重写冻结基线（清账后收基线）")
    args = ap.parse_args()

    仓库根 = Path(args.root).resolve()
    if args.report:
        return 报告(仓库根)
    return 冻结线检查(仓库根, args.update_baseline)


if __name__ == "__main__":
    sys.exit(main())
