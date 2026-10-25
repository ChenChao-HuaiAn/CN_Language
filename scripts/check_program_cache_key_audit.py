#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""程序级缓存键输入面机械对账门禁（任务 1376·1156 缓存键完备性机械化）。

背景：1156 程序缓存把「编译器身份/格式版本/源闭包/编译选项/宏集合/依赖」全部并入
键，但完备性一直靠人工逐项审计——新增一个影响产物的编译输入（DriverOptions 字段/
闭包字段/环境开关）而忘记入键=静默错产物。本门禁把「输入面清单」收敛到缓存代码内
的结构化登记表（src/cn_compiler/driver/driver_program_cache_inputs.hpp），并与实现
代码/文档双向对账：

  ① 代码有而表无（防新增输入忘记入键）：
     - DriverOptions / ProgramUnitKey / CargoDependency 新增字段未登记；
     - 键构造函数（appendOptions/appendUnits/computeProgramCacheKey/cacheFilePath）
       新增 options.* / unit.* 引用未登记；
     - 驱动层新增 CN_* 环境开关未登记。
  ② 表有而代码无（防登记表变过期文档）：
     - 登记为「键/条目身份」的载体在实现中消失；
     - 证据锚文本不存在（分类声明无现场证据）。
  ③ 文档对账：driver_program_cache.hpp 必须指向登记表并覆盖全部主类别。

用法：
  python scripts/check_program_cache_key_audit.py            # 审计真实仓库
  python scripts/check_program_cache_key_audit.py --selftest # 正反两态自测（fixture）
退出码：0=通过；1=有问题（打印人可读差异）。
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent

# 审计面文件（相对仓库根；selftest fixture 逐份复制）
文件清单 = (
    "src/cn_compiler/driver/driver.hpp",                     # DriverOptions
    "src/cn_compiler/driver/driver_program_cache.hpp",       # ProgramUnitKey + 文档
    "src/cn_compiler/driver/driver_program_cache_inputs.hpp",  # 登记表（单一事实源）
    "src/cn_compiler/driver/driver_program_cache.cpp",       # 键构造/旁路判定
    "src/cn_compiler/driver/driver_cache_util.cpp",          # CN_NO_STR_ACCUM
    "src/cn_compiler/driver/driver_module.cpp",              # CN_PROGRAM_CACHE_DIR + 闭包
    "src/cn_compiler/driver/cargo_parser.hpp",               # CargoDependency
    "src/cn_main.cpp",                                       # 派生/非产物字段证据锚
)
驱动头 = "src/cn_compiler/driver/driver.hpp"
缓存头 = "src/cn_compiler/driver/driver_program_cache.hpp"
登记表头 = "src/cn_compiler/driver/driver_program_cache_inputs.hpp"
缓存实现 = "src/cn_compiler/driver/driver_program_cache.cpp"
环境扫描文件 = (
    "src/cn_compiler/driver/driver_program_cache.cpp",
    "src/cn_compiler/driver/driver_cache_util.cpp",
    "src/cn_compiler/driver/driver_module.cpp",
)
主类别 = ("格式版本", "编译器身份", "源闭包", "编译选项", "宏集合", "依赖", "条目身份")
全部类别 = 主类别 + ("派生面", "非产物面", "观测面", "缓存开关")
角色集 = ("键", "条目身份", "派生", "间接", "非产物", "旁路", "观测")
允许常量 = ("kKeyFormatVersion", "compilerIdentity")
允许参数 = ("units", "cargoDepNames", "entryFilePath")
键函数签名 = {
    "appendOptions": r"\bvoid\s+appendOptions\s*\(",
    "appendUnits": r"\bvoid\s+appendUnits\s*\(",
    "computeProgramCacheKey": r"\bbool\s+computeProgramCacheKey\s*\(",
    "cacheFilePath": r"std::string\s+cacheFilePath\s*\(",
    "programCacheActive": r"\bbool\s+programCacheActive\s*\(",
}


def 去注释(文本: str) -> str:
    文本 = re.sub(r"/\*.*?\*/", "", 文本, flags=re.S)
    return re.sub(r"//[^\n]*", "", 文本)


def 提取结构体字段(文本: str, 结构名: str):
    m = re.search(r"struct\s+" + re.escape(结构名) + r"\s*\{(.*?)\n\};", 文本, re.S)
    if m is None:
        return None
    字段 = []
    for 语句 in 去注释(m.group(1)).split(";"):
        语 = 语句.strip()
        if not 语 or "(" in 语 or "{" in 语 or "}" in 语:
            continue
        左 = 语.split("=", 1)[0].strip()
        名 = re.search(r"([A-Za-z_]\w*)\s*$", 左)
        if 名 is not None:
            字段.append(名.group(1))
    return 字段


def 提取函数体(文本: str, 签名: str):
    m = re.search(签名, 文本)
    if m is None:
        return None
    起 = 文本.find("{", m.end() - 1)
    if 起 < 0:
        return None
    深 = 0
    for 位 in range(起, len(文本)):
        if 文本[位] == "{":
            深 += 1
        elif 文本[位] == "}":
            深 -= 1
            if 深 == 0:
                return 文本[起 + 1:位]
    return None


def 解析登记表(文本: str):
    m = re.search(r"kProgramCacheInputs\s*\[\s*\]\s*=\s*\{(.*?)\n\};", 文本, re.S)
    if m is None:
        return None, "登记表 kProgramCacheInputs[] 未找到"
    行们 = []
    for 块 in re.finditer(r"\{([^{}]*)\}", 去注释(m.group(1)), re.S):
        串们 = re.findall(r'"((?:[^"\\]|\\.)*)"', 块.group(1))
        if len(串们) != 6:
            return None, "登记项字段数 %d != 6（id/类别/角色/载体/锚/说明）" % len(串们)
        行们.append(dict(zip(("id", "类别", "角色", "载体", "锚", "说明"), 串们)))
    if not 行们:
        return None, "登记表为空（单一事实源缺失）"
    return 行们, ""


def 扫描环境开关(文本们: dict) -> set:
    名们 = set()
    for 文本 in 文本们.values():
        for 行 in 去注释(文本).splitlines():
            if "getenv(" in 行 or "_dupenv_s(" in 行 or "cacheLeafDir(" in 行:
                名们.update(re.findall(r'"(CN_[A-Z0-9_]+)"', 行))
    return 名们


def 核对载体(载体: str, 字段们: dict):
    """返回 (种类, 名称, 问题)；种类 ∈ options/unit/cargo/const/env/arg。"""
    if 载体.startswith("options."):
        名 = 载体[len("options."):]
        种类 = "options"
    elif 载体.startswith("unit."):
        名 = 载体[len("unit."):]
        种类 = "unit"
    elif 载体.startswith("cargo."):
        名 = 载体[len("cargo."):]
        种类 = "cargo"
    elif 载体.startswith("const:"):
        名 = 载体[len("const:"):]
        种类 = "const"
    elif 载体.startswith("env:"):
        名 = 载体[len("env:"):]
        种类 = "env"
    elif 载体.startswith("arg:"):
        名 = 载体[len("arg:"):]
        种类 = "arg"
    else:
        return None, None, "载体语法非法（须 options./unit./cargo./const:/env:/arg:）: " + 载体
    if 种类 == "options" and 名 not in 字段们["options"]:
        return 种类, 名, "登记引用了不存在的 DriverOptions 字段: " + 载体
    if 种类 == "unit" and 名 not in 字段们["unit"]:
        return 种类, 名, "登记引用了不存在的 ProgramUnitKey 字段: " + 载体
    if 种类 == "cargo" and 名 not in 字段们["cargo"]:
        return 种类, 名, "登记引用了不存在的 CargoDependency 字段: " + 载体
    if 种类 == "const" and 名 not in 允许常量:
        return 种类, 名, "登记引用了未纳入审计的常量: " + 载体
    if 种类 == "arg" and 名 not in 允许参数:
        return 种类, 名, "登记引用了未知键构造参数: " + 载体
    return 种类, 名, ""


def 读审计面(根: Path):
    """读取全部审计面文件；返回 (文本表, 问题列表)。"""
    文本 = {}
    for 相对 in 文件清单:
        try:
            文本[相对] = (根 / 相对).read_text(encoding="utf-8")
        except OSError as 异常:
            return {}, ["读取审计面文件失败: %s（%s）" % (相对, 异常)]
    return 文本, []


def 解析审计面(文本: dict):
    """解析结构体字段与键构造函数体；返回 (字段们, 体们, 问题列表)。"""
    问题 = []
    字段们 = {
        "options": 提取结构体字段(文本[驱动头], "DriverOptions"),
        "unit": 提取结构体字段(文本[缓存头], "ProgramUnitKey"),
        "cargo": 提取结构体字段(文本["src/cn_compiler/driver/cargo_parser.hpp"],
                                 "CargoDependency"),
    }
    for 键, 值 in 字段们.items():
        if not 值:
            问题.append("结构体字段提取失败（%s）——审计面代码形态变化须同步更新门禁" % 键)
    if 问题:
        return 字段们, {}, 问题
    体们 = {}
    for 名, 签名 in 键函数签名.items():
        体 = 提取函数体(文本[缓存实现], 签名)
        if 体 is None:
            问题.append("键构造函数缺失: %s（缓存实现形态变化）" % 名)
        else:
            体们[名] = 去注释(体)
    return 字段们, 体们, 问题


def 核对登记项(根: Path, 登记们: list, 字段们: dict, 函数字段: dict,
               体们: dict, 环境们: set):
    """逐条核验登记项（载体/类别/角色/证据锚/角色规则）；返回
    (问题列表, 已登记选项{字段:角色集}, 已登记单元, 已登记依赖)。"""
    问题 = []
    已登记id = set()
    已登记选项 = {}
    已登记单元 = set()
    已登记依赖 = set()
    for 行 in 登记们:
        if 行["id"] in 已登记id:
            问题.append("登记项 id 重复: " + 行["id"])
        已登记id.add(行["id"])
        if 行["类别"] not in 全部类别:
            问题.append("登记项类别非法: %s（%s）" % (行["类别"], 行["id"]))
        if 行["角色"] not in 角色集:
            问题.append("登记项角色非法: %s（%s）" % (行["角色"], 行["id"]))
            continue
        种类, 名, 载体问题 = 核对载体(行["载体"], 字段们)
        if 载体问题:
            问题.append(载体问题 + "（" + 行["id"] + "）")
            continue
        if 种类 == "options":
            已登记选项.setdefault(名, set()).add(行["角色"])
        elif 种类 == "unit":
            已登记单元.add(名)
        elif 种类 == "cargo":
            已登记依赖.add(名)
        if 行["锚"] != "-":
            锚问题 = 核验锚(根, 行["锚"])
            if 锚问题:
                问题.append("登记项 %s 证据锚失效: %s" % (行["id"], 锚问题))
        问题.extend(核对角色(行, 种类, 名, 函数字段, 体们, 环境们))
    return 问题, 已登记选项, 已登记单元, 已登记依赖


def 核对字段覆盖(字段们: dict, 登记们: list, 闭包字段: set) -> list:
    """① 字段覆盖（代码有而表无）+ ProgramUnitKey 角色/入键面。"""
    问题 = []
    已登记选项 = {}
    已登记单元 = set()
    已登记依赖 = set()
    for 行 in 登记们:
        载体 = 行["载体"]
        if 载体.startswith("options."):
            已登记选项.setdefault(载体[len("options."):], set()).add(行["角色"])
        elif 载体.startswith("unit."):
            已登记单元.add(载体[len("unit."):])
        elif 载体.startswith("cargo."):
            已登记依赖.add(载体[len("cargo."):])
    for 字段 in 字段们["options"]:
        if 字段 not in 已登记选项:
            问题.append("DriverOptions 字段未登记输入面（新增输入必须入表）: " + 字段)
    for 字段 in 字段们["unit"]:
        if 字段 not in 已登记单元:
            问题.append("ProgramUnitKey 字段未登记输入面: " + 字段)
    for 字段 in 字段们["cargo"]:
        if 字段 not in 已登记依赖:
            问题.append("CargoDependency 字段未登记输入面: " + 字段)
    单元角色 = {}
    for 行 in 登记们:
        if 行["载体"].startswith("unit."):
            单元角色.setdefault(行["载体"][len("unit."):], set()).add(行["角色"])
    for 字段 in 字段们["unit"]:
        if 单元角色.get(字段, set()) != {"键"}:
            问题.append("ProgramUnitKey 字段角色必须且只能为「键」: " + 字段)
        if 字段 not in 闭包字段:
            问题.append("ProgramUnitKey 字段未在 appendUnits 入键: " + 字段)
    return 问题


def 核对构造引用(函数字段: dict, 登记们: list, 签名闭包字段: set) -> list:
    """② 键构造引用面（代码有而表无）+ 条目名签名面。"""
    问题 = []
    已登记选项 = {}
    for 行 in 登记们:
        if 行["载体"].startswith("options."):
            已登记选项.setdefault(行["载体"][len("options."):], set()).add(行["角色"])
    for 名 in ("appendOptions", "computeProgramCacheKey"):
        for 字段 in sorted(函数字段[名]):
            if "键" not in 已登记选项.get(字段, set()):
                问题.append("%s 引用未登记为「键」的字段: options.%s" % (名, 字段))
    for 字段 in sorted(函数字段["cacheFilePath"]):
        if "条目身份" not in 已登记选项.get(字段, set()):
            问题.append("cacheFilePath 引用未登记为「条目身份」的字段: options." + 字段)
    for 字段 in sorted(签名闭包字段):
        问题.append("cacheFilePath 出现 unit.%s（条目名只含入口/选项/宏签名）" % 字段)
    return 问题


def 核对环境与文档(登记们: list, 环境们: set, 文档: str) -> list:
    """③ 环境开关覆盖/陈旧 + ④ 文档对账。"""
    问题 = []
    登记环境 = {行["载体"][len("env:"):] for 行 in 登记们
                if 行["载体"].startswith("env:")}
    for 名 in sorted(环境们 - 登记环境):
        问题.append("驱动层环境开关未登记输入面: env:" + 名)
    for 名 in sorted(登记环境 - 环境们):
        问题.append("登记的环境开关在实现中不存在（陈旧登记）: env:" + 名)
    if "driver_program_cache_inputs.hpp" not in 文档:
        问题.append("缓存头注释未指向登记表单一事实源（driver_program_cache_inputs.hpp）")
    for 类别 in 主类别:
        if 类别 not in 文档:
            问题.append("缓存头注释缺少输入面类别: " + 类别)
    return 问题


def 审计(根: Path) -> list:
    文本, 问题 = 读审计面(根)
    if 问题:
        return 问题
    登记们, 错误 = 解析登记表(文本[登记表头])
    if 登记们 is None:
        return [错误]
    字段们, 体们, 问题 = 解析审计面(文本)
    if 问题:
        return 问题
    函数字段 = {名: set(re.findall(r"options\.([A-Za-z_]\w*)", 体))
               for 名, 体 in 体们.items()}
    闭包字段 = set(re.findall(r"unit\.([A-Za-z_]\w*)", 体们["appendUnits"]))
    签名闭包字段 = set(re.findall(r"unit\.([A-Za-z_]\w*)", 体们["cacheFilePath"]))
    环境们 = 扫描环境开关({相对: 文本[相对] for 相对 in 环境扫描文件})
    问题, _, _, _ = 核对登记项(根, 登记们, 字段们, 函数字段, 体们, 环境们)
    问题 += 核对字段覆盖(字段们, 登记们, 闭包字段)
    问题 += 核对构造引用(函数字段, 登记们, 签名闭包字段)
    问题 += 核对环境与文档(登记们, 环境们, 文本[缓存头])
    return 问题


def 核验锚(根: Path, 锚: str) -> str:
    if "#" not in 锚:
        return "锚格式须为 文件#锚文本: " + 锚
    相对, 片段 = 锚.split("#", 1)
    片段 = 片段.strip()
    if not 片段:
        return "锚文本为空: " + 锚
    try:
        内容 = (根 / 相对).read_text(encoding="utf-8")
    except OSError as 异常:
        return "锚文件不可读 %s（%s）" % (相对, 异常)
    if 片段 not in 内容:
        return "锚文本不存在于 %s: %s" % (相对, 片段)
    return ""


def 核对角色(行: dict, 种类: str, 名: str, 函数字段: dict, 体们: dict,
             环境们: set) -> list:
    问题 = []
    角色 = 行["角色"]
    if 种类 == "env":
        if 名 not in 环境们:
            问题.append("登记项 %s 的 env:%s 未在驱动层实现中出现" % (行["id"], 名))
        return 问题
    if 种类 == "arg":
        if 名 == "entryFilePath" and 名 not in 体们["cacheFilePath"]:
            问题.append("登记项 %s：cacheFilePath 未使用参数 %s" % (行["id"], 名))
        if 名 in ("units", "cargoDepNames") and 名 not in 体们["computeProgramCacheKey"]:
            问题.append("登记项 %s：computeProgramCacheKey 未使用参数 %s" % (行["id"], 名))
        return 问题
    if 种类 == "const":
        if 名 == "kKeyFormatVersion" and 名 not in 体们["computeProgramCacheKey"]:
            问题.append("登记项 %s：键构造未使用 kKeyFormatVersion（格式版本未入键）"
                        % 行["id"])
        if 名 == "compilerIdentity" and 名 not in 体们["computeProgramCacheKey"]:
            问题.append("登记项 %s：键构造未使用 compilerIdentity（编译器身份未入键）"
                        % 行["id"])
        return 问题
    if 种类 == "cargo":
        if 角色 == "键" and 行["锚"] == "-":
            问题.append("登记项 %s：cargo.%s 入键须给证据锚（键构造在驱动模块）"
                        % (行["id"], 名))
        return 问题
    if 种类 == "unit":
        if 角色 != "键":
            问题.append("登记项 %s：ProgramUnitKey 字段角色必须为「键」" % 行["id"])
        return 问题
    # options
    全部键体 = (函数字段["appendOptions"] | 函数字段["computeProgramCacheKey"] |
                函数字段["cacheFilePath"])
    if 角色 == "键" and 名 not in (函数字段["appendOptions"] |
                                  函数字段["computeProgramCacheKey"]):
        问题.append("登记项 %s：登记为「键」但键构造未使用 options.%s"
                    % (行["id"], 名))
    elif 角色 == "条目身份" and 名 not in 函数字段["cacheFilePath"]:
        问题.append("登记项 %s：登记为「条目身份」但 cacheFilePath 未使用 options.%s"
                    % (行["id"], 名))
    elif 角色 == "旁路" and 名 not in 函数字段["programCacheActive"] and 行["锚"] == "-":
        问题.append("登记项 %s：旁路字段既不在 programCacheActive 判定中，也无证据锚"
                    % 行["id"])
    elif 角色 in ("派生", "间接", "非产物", "观测") and 名 in 全部键体:
        问题.append("登记项 %s：options.%s 被声明为「%s」却出现在键构造中（角色应为键）"
                    % (行["id"], 名, 角色))
    if 角色 in ("派生", "间接", "非产物", "观测") and 行["锚"] == "-":
        问题.append("登记项 %s：角色「%s」必须给证据锚（禁无证据分类）"
                    % (行["id"], 角色))
    return 问题


def 主流程(根: Path) -> int:
    问题 = 审计(根)
    if 问题:
        print("[程序缓存键审计] FAIL：发现 %d 个差异" % len(问题), flush=True)
        for 项 in 问题:
            print("  - " + 项, flush=True)
        print("  → 合法通道：在 %s 登记/修正（新增输入必须入键并登记）" % 登记表头,
              flush=True)
        return 1
    print("[程序缓存键审计] PASS ✓（输入面登记表与实现/文档逐项一致）", flush=True)
    return 0


# ==================== 正反两态自测（零仓库状态依赖·fixture 副本） ====================

def 建fixture(根: Path) -> Path:
    fixture = 根 / "fixture"
    if fixture.exists():
        shutil.rmtree(fixture)
    for 相对 in 文件清单:
        目标 = fixture / 相对
        目标.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(仓库根 / 相对, 目标)
    return fixture


def 副本(fixture: Path, 根: Path, 名: str) -> Path:
    目标 = 根 / 名
    if 目标.exists():
        shutil.rmtree(目标)
    shutil.copytree(fixture, 目标)
    return 目标


def 改(路径: Path, 旧: str, 新: str, 全部: bool = False) -> None:
    文本 = 路径.read_text(encoding="utf-8")
    if 旧 not in 文本:
        raise AssertionError("自测夹具替换锚不存在: " + 旧[:60])
    路径.write_text(文本.replace(旧, 新) if 全部 else 文本.replace(旧, 新, 1),
                    encoding="utf-8")


def 自测() -> int:
    根 = 仓库根 / "target" / "check_program_cache_key_audit_selftest"
    if 根.exists():
        shutil.rmtree(根)
    根.mkdir(parents=True)
    fixture = 建fixture(根)
    通过 = 0
    失败 = 0

    def 判(条件: bool, 名称: str, 详情: str = "") -> None:
        nonlocal 通过, 失败
        if 条件:
            通过 += 1
            print("  PASS " + 名称 + ("：" + 详情 if 详情 else ""))
        else:
            失败 += 1
            print("  FAIL " + 名称 + ("：" + 详情 if 详情 else ""))

    try:
        # ① 基线：全部登记=绿
        问题 = 审计(fixture)
        判(not 问题, "基线全登记=绿", "、".join(问题[:2]))

        # ② 人为去掉一项登记=红（optLevel 未登记）
        case = 副本(fixture, 根, "去登记")
        改(case / 登记表头,
          '    {"opt.opt_level", "编译选项", "键", "options.optLevel", "-", "优化级别"},\n',
          "")
        问题 = 审计(case)
        判(any("optLevel" in x for x in 问题), "去掉登记项=红", "、".join(问题[:2]))

        # ③ 新增输入未登记=红（DriverOptions 新字段）
        case = 副本(fixture, 根, "新字段")
        改(case / 驱动头, "    bool useProgramCache = false;",
          "    bool useProgramCache = false;\n    bool newArtifactFlag = false;")
        问题 = 审计(case)
        判(any("newArtifactFlag" in x for x in 问题), "新增输入未登记=红",
           "、".join(问题[:2]))

        # ④ 登记不存在的载体=红（表有代码无）
        case = 副本(fixture, 根, "幽灵登记")
        改(case / 登记表头, "kProgramCacheInputs[] = {\n",
          'kProgramCacheInputs[] = {\n    {"test.ghost", "编译选项", "键", '
          '"options.notAField", "-", "自测"},\n')
        问题 = 审计(case)
        判(any("notAField" in x for x in 问题), "登记不存在的字段=红",
           "、".join(问题[:2]))

        # ⑤ 键构造引用未登记字段=红（appendOptions 新增 useMem2Reg）
        case = 副本(fixture, 根, "键构造新增")
        改(case / 缓存实现, "void appendOptions(std::string& out, const DriverOptions& options) {",
          "void appendOptions(std::string& out, const DriverOptions& options) {\n"
          '    cacheAppendField(out, options.useMem2Reg ? "1" : "0");')
        问题 = 审计(case)
        判(any("useMem2Reg" in x for x in 问题), "键构造新增未登记字段=红",
           "、".join(问题[:2]))

        # ⑥ 环境开关未登记=红
        case = 副本(fixture, 根, "新环境开关")
        路径 = case / 缓存实现
        路径.write_text(路径.read_text(encoding="utf-8") +
                        '\nstatic const char* g_probe = std::getenv("CN_NEW_SWITCH");\n',
                        encoding="utf-8")
        问题 = 审计(case)
        判(any("CN_NEW_SWITCH" in x for x in 问题), "环境开关未登记=红",
           "、".join(问题[:2]))

        # ⑦ 证据锚失效=红
        case = 副本(fixture, 根, "锚失效")
        改(case / "src/cn_main.cpp",
          "dopts.useMem2Reg = dopts.useRegAlloc && options.target",
          "dopts.useMem2Reg = dopts.useRegAlloc && dopts.target")
        问题 = 审计(case)
        判(any("证据锚" in x for x in 问题), "证据锚失效=红", "、".join(问题[:2]))

        # ⑧ 条目身份引用被删=红（cacheFilePath 去掉 requireEntryMain）
        case = 副本(fixture, 根, "签名缺项")
        改(case / 缓存实现, '    subject += options.requireEntryMain ? "1" : "0";\n', "")
        问题 = 审计(case)
        判(any("requireEntryMain" in x for x in 问题), "条目名签名缺登记项=红",
           "、".join(问题[:2]))

        # ⑨ 文档缺类别=红
        case = 副本(fixture, 根, "文档缺项")
        改(case / 缓存头, "宏集合", "宏组合", 全部=True)
        问题 = 审计(case)
        判(any("宏集合" in x for x in 问题), "文档缺输入面类别=红",
           "、".join(问题[:2]))
    except Exception as 异常:  # 自测自身异常=显式失败（不静默绿）
        失败 += 1
        print("  FAIL 自测未捕获异常：" + repr(异常))
    finally:
        shutil.rmtree(根, ignore_errors=True)
    print("自测：%s（PASS %d / FAIL %d）" % ("PASS" if 失败 == 0 else "FAIL", 通过, 失败))
    return 0 if 失败 == 0 else 1


def main() -> int:
    解析 = argparse.ArgumentParser(description="程序级缓存键输入面机械对账门禁（1376）")
    解析.add_argument("--selftest", action="store_true", help="正反两态自测（fixture 副本）")
    解析.add_argument("--根", default=str(仓库根), help="审计根目录（默认仓库根）")
    参数 = 解析.parse_args()
    if 参数.selftest:
        return 自测()
    return 主流程(Path(参数.根).resolve())


if __name__ == "__main__":
    sys.exit(main())
