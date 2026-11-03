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
  ④生成面未退化+折叠保护审计面有捕获（miss 不可静默）；
  ⑤1022/1740/1772/1869/类字段组合/#1977/#2010 族覆盖+结构+形态锚（二十八族
    两轮 58 seed）。
编译对拍（--编译 <cn.exe>）：⑥样本全编译通过；⑦审计危险表达式全被拒
  （折叠模拟与编译器口径双向闭环——偏差即显形）；⑧混合面 seed 1~28 全族
  编译锁（预期红锚表校验：宿主当前拒的 #1991/#1993/#1998 族样本必须命中
  对应诊断锚）+扩域十一族两变体显式编译锁（每族 2 变体·任务验收面）。
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
from cnsmith_gen_families_ext import 预期拒锚  # noqa: E402  （#2010 预期红锚表）

# 混合面轮转周期（标量面 + 各族各一）= 族数 + 1（families.program_mixed 同式）
混合周期 = len(族名们) + 1


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

    # 1022/1740/1772/1869/类字段组合/#1977/#2010 族扩面锁（#1398 随批·逐轮续扩）：
    #   混合面 58 seed（两轮 29）覆盖二十八族×2 变体+标量回退；族结构完整
    #   （删族=自动面重新致盲 → 本轮 #1393~#1397/#1971 等永不再显形）
    混合样本 = [(seed, Gen(seed).program_mixed(语句数))
               for seed in range(1, 2 * 混合周期 + 1)]
    混合文本 = NL.join(src for _s, src in 混合样本)
    缺族 = [族 for 族 in 族名们 if ("// 族=" + 族) not in 混合文本]
    签("1022/1740/1772/1869/类字段组合/#1977/#2010 族生成面覆盖（%d/%d）"
       % (len(族名们) - len(缺族), len(族名们)),
       not 缺族, "缺 %s" % 缺族)
    签("混合面保留标量回退（seed%%%d==0）" % 混合周期,
       any("=== CN-Smith 采样 ===" in src and "// 族=" not in src
           for _s, src in 混合样本))
    族样本 = [(s, src) for s, src in 混合样本 if "// 族=" in src]
    签("混合面族程序结构完整（主函数唯一·%d 件）" % len(族样本),
       len(族样本) == 2 * 混合周期 - (2 * 混合周期 // 混合周期)
       and all(src.count("函数 主()") == 1 for _s, src in 族样本))
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
    # 1869 八新族源码形态锚锁：拼接隐式转换/整32 复合位赋值/空初始化器/
    #   按值传参值语义/饱和锚例/不安全静态累加/多值分组/容器接结果 八形态
    #   在各自族样本必现（1030 轮八域零自动覆盖实录重演防线；模板退化=致盲）
    锚新坏 = []
    for _s, src in 混合样本:
        族m = re.match(r"// 族=(\S+)", src)
        if not 族m or 族m.group(1) not in ("字符串拼接", "位运算复合",
                                          "数组初始化器", "结构体传值",
                                          "浮点饱和", "静态局部",
                                          "控制流贯穿", "容器组合"):
            continue
        名 = 族m.group(1)
        坏 = None
        if 名 == "字符串拼接" and "+ 分数" not in src and \
                "字符串查找(" not in src:
            坏 = "缺拼接转换/子串查找形态"
        if 名 == "位运算复合" and (
                ("&=" not in src or ">>=" not in src)
                and "整64(基) << 量" not in src):
            坏 = "缺复合位赋值/混合宽度移位"
        if 名 == "数组初始化器" and "= {};" not in src and \
                re.search(r"部分 = \{ 10, 20 \}", src) is None:
            坏 = "缺空初始化器/部分初始化形态"
        if 名 == "结构体传值" and ".甲 = 10;" not in src and \
                "序[2] = 序[0];" not in src:
            坏 = "缺逐字段赋值/元素整体赋值形态"
        if 名 == "浮点饱和" and "整32(1.0e18)" not in src and \
                "正8(300.0)" not in src:
            坏 = "缺饱和锚例形态"
        if 名 == "静态局部":
            # 族双变体锚（类字段组合扩族后 seed 36 映射漂移实证变体二样本
            #   命中本断言——原断言按「必现变体一」过严假红）：变体一=不安全
            #   静态累加（480 执法横切锚：凡静态写必带 不安全 修饰）、
            #   变体二=遮蔽全局同名形态；两形态缺一才坏。
            if "静态 整32 计数" in src and "不安全" not in src:
                坏 = "静态写缺 不安全 修饰（480 执法）"
            elif "静态 整32 计数" not in src and "读全局()" not in src:
                坏 = "缺静态累加/遮蔽全局形态"
        if 名 == "控制流贯穿" and "情况 0, 1:" not in src and \
                "中断;" not in src:
            坏 = "缺多值分组/嵌套中断形态"
        if 名 == "容器组合" and "结果<空类型, 整32>" not in src:
            坏 = "缺容器接结果形态"
        if 坏:
            锚新坏.append((_s, 名, 坏))
    签("1869 八新族源码形态锚（拼接/复合位赋值/初始化器/传值/饱和/静态/"
       "贯穿/容器接结果）", not 锚新坏, str(锚新坏[:3]))
    # 类字段组合族源码形态锚锁（#1943 域自动面收口）：类类型字段声明+点链
    #   访问+方法内访问（变体一）/容器字段接结果+可选字段有值检查（变体二）
    #   两形态在各自族样本必现（1101 采样轮自动面零覆盖实录重演防线——
    #   #1943 全靠手动探针实锤的生成面缺口，删锚=重新致盲）
    锚类字段坏 = []
    for _s, src in 混合样本:
        族m = re.match(r"// 族=(\S+)", src)
        if not 族m or 族m.group(1) != "类字段组合":
            continue
        if "公开:" not in src or "里的;" not in src or ".里的.v" not in src:
            锚类字段坏.append((_s, "缺类字段声明/点链访问形态"))
        if "函数 写入" not in src and "h.列.追加(9)" not in src:
            锚类字段坏.append((_s, "缺方法内访问/容器字段追加形态"))
        if "外件 w2 = 外件();" not in src and "s.记.有值" not in src:
            锚类字段坏.append((_s, "缺显式构造调用/可选字段检查形态"))
        # 补析构形态横切锚（警告分叉防线·#1950 立项在案）：变体二持容器
        #   字段的类必须带空析构——#1943 修后宿主对「有析构类字段+类无
        #   析构」发级联释放警告而 v2 未对齐，模板退化回无析构形态=对拍
        #   轨道警告面复红。
        if "函数 写入" not in src and "~持列() { }" not in src:
            锚类字段坏.append((_s, "容器字段类缺补析构形态"))
    签("类字段组合族源码形态锚（类字段声明点链访问/方法内访问与容器字段/"
       "显式构造与可选检查/容器字段补析构）", not 锚类字段坏, str(锚类字段坏[:3]))
    # #1977 四新族源码形态锚锁（1102 采样面扩域）：宽位占位符+泄漏轴差值
    #   断言行/联合体不安全域/接口签名分号+重写/堆分配释放配对 四形态在各自
    #   族样本必现（1102 轮实证自动四轨道 5 命中全已知而手动 74 探针实锤
    #   7 新缺陷=生成面结构性缺口——删锚=重新致盲）。泄漏轴横切锚：差值行
    #   必现且 基 取值先于族内首个格式化调用（取点后移=断言语义破）。
    锚扩域坏 = []
    for _s, src in 混合样本:
        族m = re.match(r"// 族=(\S+)", src)
        if not 族m or 族m.group(1) not in ("格式化占位符", "联合体",
                                           "泛型继承接口", "堆指针链"):
            continue
        名 = 族m.group(1)
        坏 = None
        if 名 == "格式化占位符":
            if "%d64" not in src or "格式化(" not in src:
                坏 = "缺宽位占位符/格式化形态"
            if "打印(内存::活动分配数() - 基);" not in src:
                坏 = "缺泄漏轴差值断言行"
            else:
                # 基 取值点须早于主函数内首个格式化调用（变体一 组行 内也有
                #   格式化调用——锚定 主() 之后，否则 #2010 周期迁移选中变体一
                #   即假红；锚错=泄漏轴语义判据错位）
                主位 = src.index("函数 主()")
                基位 = src.index("整64 基 = 内存::活动分配数();")
                串位 = src.index("格式化(", 主位)
                if 基位 > 串位:
                    坏 = "基 取值晚于串产出"
        if 名 == "联合体" and (
                "联合体 " not in src or "不安全 函数" not in src):
            坏 = "缺联合体声明/不安全域"
        if 名 == "泛型继承接口":
            # 双变体分流锚：变体一=继承非泛型基类+基类指针分派（无接口）、
            #   变体二=实现两接口（签名收分号·执法面）；泛型声明+重写两形态
            #   恒必现。
            if "泛型 <类型 T>" not in src or "重写 函数" not in src:
                坏 = "缺泛型类声明/重写形态"
            if "接口 可取值" in src and "-> 整32;" not in src:
                坏 = "接口方法签名缺分号"
            if "类 基 {" in src and "基* p = 实;" not in src:
                坏 = "继承变体缺基类指针分派形态"
        if 名 == "堆指针链" and (
                "分配(类型大小(" not in src or "释放(" not in src
                or "无;" not in src):
            坏 = "缺堆分配/释放/无 判空形态"
        if 坏:
            锚扩域坏.append((_s, 名, 坏))
    签("#1977 四新族源码形态锚（宽位占位符/泄漏轴差值行/联合体不安全域/"
       "接口分号重写/堆分配释放配对）", not 锚扩域坏, str(锚扩域坏[:3]))
    # #2010 五新族源码形态锚锁（1103 轮实证：自动三轨零新命中而手动 64 探针
    #   实锤 #1991~#1998 五域零覆盖——删锚=自动面重新致盲）：
    #   类成员链指针=类字段指针声明+构造/成员函数写或主内写+复合赋值/嵌套链；
    #   泛型多实例=泛型函数三实例交错+恒等(恒等(...)) 嵌套调用；
    #   接口浮点返回=接口方法 浮64 返回签名分号+值/指针分发；
    #   枚举混合算术=裸枚举下标或枚举元素与整数混算；
    #   拥有契约=泛型 恒等(T v)->T 以字符串实参调用。
    锚新五坏 = []
    for _s, src in 混合样本:
        族m = re.match(r"// 族=(\S+)", src)
        if not 族m or 族m.group(1) not in ("类成员链指针", "泛型多实例",
                                           "接口浮点返回", "枚举混合算术",
                                           "拥有契约"):
            continue
        名 = 族m.group(1)
        坏 = None
        if 名 == "类成员链指针":
            if "整32* 针;" not in src or "不安全 函数" not in src:
                坏 = "缺类字段指针声明/不安全域"
            elif "针[0] = 初;" in src:
                if "加倍()" not in src:
                    坏 = "构造内变体缺成员函数写形态"
            elif "a.针[0] = 99;" not in src:
                坏 = "缺构造内/主内指针下标写形态"
            elif "+=" not in src or "b.内.针[0] = 7;" not in src:
                坏 = "主内变体缺复合赋值/嵌套成员链形态"
        if 名 == "泛型多实例":
            if "泛型 <类型 T>" not in src or "恒等(" not in src:
                坏 = "缺泛型声明/恒等调用形态"
            elif "取大(1.5, 2.5)" not in src:
                坏 = "缺浮64 实例形态"
            elif '恒等("串")' in src:
                if "取大(1.5, 2.5)" not in src.split("取大(3, 7)")[0]:
                    坏 = "浮先整后交错形态缺失"
            elif "恒等(恒等(" not in src or "10L" not in src:
                坏 = "整先浮后变体缺嵌套调用/整64 实例形态"
        if 名 == "接口浮点返回":
            if "接口 可算" not in src or "-> 浮64;" not in src:
                坏 = "缺接口浮点返回签名分号形态"
            elif "重写 函数" not in src:
                坏 = "缺重写实现形态"
            elif "可算 s = f;" not in src and "可算* p = &b;" not in src:
                坏 = "缺值/指针接口分派形态"
        if 名 == "枚举混合算术":
            if "枚举 " not in src:
                坏 = "缺枚举声明形态"
            elif "表[c]" in src:
                if "表[通道.蓝]" not in src:
                    坏 = "枚举下标变体缺字面量下标形态"
            elif "和 = 和 + 路径[i];" not in src or "方位.北 + 2" not in src:
                坏 = "枚举混算变体缺元素累加/字面量加整数形态"
        if 名 == "拥有契约":
            if "泛型 <类型 T>" not in src or "恒等(T v) -> T" not in src:
                坏 = "缺泛型恒等签名形态"
            elif "返回 v;" not in src or (
                    "恒等(源)" not in src and '恒等("丙丁")' not in src):
                坏 = "缺返回形参/字符串实参调用形态"
        if 坏:
            锚新五坏.append((_s, 名, 坏))
    签("#2010 五新族源码形态锚（类字段指针下标写/泛型多实例交错与嵌套/"
       "接口浮点返回分号/裸枚举下标与混算/拥有契约返形参）",
       not 锚新五坏, str(锚新五坏[:3]))
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

            # 1022/1740/1772/1869/类字段组合/#1977/#2010 族面编译锁：seed 1~28
            #   各命中一面（二十八族；宿主对默认参数常量算式放行但值错——合法性
            #   契约只要求编译通过，值错由对拍轨道显形）。#1771 已集成（1869 轮
            #   实测多维数组族编译转绿）——原「未初始化」预期红白名单按预告收口
            #   为全通过断言（族程序再编译失败=真缺陷当场显形）。类字段组合族
            #   依赖 #1943 修后编译器（收口顺序=1943→1945）。#1977 四新族=
            #   develop 头 #1972~#1974 修复集成后双侧应编译绿。
            #   #2010 五族=plans/001 合法程序、当前实现缺陷面单侧预期红：宿主
            #   当前拒的样本必须命中 预期拒锚们 诊断锚（#1991/#1993/#1998），
            #   其余样本应编译绿；命中数与预期样本数不等=修复已集成/模板漂移
            #   （红在明处：收口预期拒锚表后本断言转全绿）。
            族坏 = []
            红锚失效 = []
            预期红数 = 0
            命中红数 = 0
            for seed in range(1, len(族名们) + 1):
                源码 = Gen(seed).program_mixed(语句数)
                锚 = 预期拒锚(源码)
                预期红数 += 1 if 锚 is not None else 0
                源 = os.path.join(临时, "f%d.cn" % seed)
                with open(源, "w", encoding="utf-8", newline="") as f:
                    f.write(源码)
                r = subprocess.run(
                    [编译路径, "build", 源, "--target", 目标, "-O0",
                     "--output", 源[:-3] + ".exe"],
                    capture_output=True, timeout=120)
                输出 = (r.stdout + r.stderr).decode("utf-8", "replace")
                if r.returncode == 0:
                    if 锚 is not None:
                        红锚失效.append((seed, "预期红转绿", 锚))
                    continue
                if 锚 is None:
                    族坏.append((seed, 输出.strip().splitlines()[:1]))
                elif 锚 not in 输出:
                    族坏.append((seed, "诊断锚不符", 锚, 输出[:120]))
                else:
                    命中红数 += 1
            签("1022/1740/1772/1869/类字段组合/#1977/#2010 混合面编译锁"
               "（应绿 %d 件全通过·预期红锚 %d/%d）"
               % (len(族名们) - 预期红数, 命中红数, 预期红数),
               not 族坏 and not 红锚失效 and 命中红数 == 预期红数,
               ("异常=%s" % (族坏[:3],) if 族坏 else
                "预期红锚失效=%s（#1991/#1993/#1998 修复已集成？"
                "→ 收口 cnsmith_gen_families_ext.预期拒锚们）" % (红锚失效[:3],)))

            # #1977/#2010 扩域族两变体显式编译锁（每族 2 变体·任务验收面）：扫
            #   seed 按变体文本去重取前两个变体样本各编译一次——变体选择=rng 域，
            #   两轮混合面未必保证每族双变体命中，显式扫描保证确定性覆盖。
            #   #2010 五族按 预期拒锚 分流：应绿变体必须编译通过、预期红变体必须
            #   以对应诊断锚被拒（防真语法错混作预期红·修复集成后按锚表收口）。
            from cnsmith_gen_families import 族程序 as _族程序
            变体坏 = []
            变体红失 = []
            变体红锚 = 0
            for 新族 in ("格式化占位符", "联合体", "泛型继承接口", "堆指针链",
                        "转义字符串", "字符串拼接",
                        "类成员链指针", "泛型多实例", "接口浮点返回",
                        "枚举混合算术", "拥有契约"):
                变体集 = {}
                for seed in range(1, 200):
                    源码 = _族程序(新族, seed)
                    变体集.setdefault(源码, seed)
                    if len(变体集) >= 2:
                        break
                if len(变体集) < 2:
                    变体坏.append("%s(变体不足 2)" % 新族)
                    continue
                for 变体序, (源码, seed) in enumerate(变体集.items()):
                    锚 = 预期拒锚(源码)
                    源 = os.path.join(临时, "n%d_%d.cn" % (
                        族名们.index(新族), 变体序))
                    with open(源, "w", encoding="utf-8", newline="") as f:
                        f.write(源码)
                    r = subprocess.run(
                        [编译路径, "build", 源, "--target", 目标, "-O0",
                         "--output", 源[:-3] + ".exe"],
                        capture_output=True, timeout=120)
                    输出 = (r.stdout + r.stderr).decode("utf-8", "replace")
                    if 锚 is None:
                        if r.returncode != 0:
                            变体坏.append((新族, 变体序, "应绿被拒", 输出[:120]))
                    elif r.returncode == 0:
                        变体红失.append((新族, 变体序, "预期红转绿", 锚))
                    elif 锚 not in 输出:
                        变体坏.append((新族, 变体序, "诊断锚不符", 锚, 输出[:120]))
                    else:
                        变体红锚 += 1
            签("#1977/#2010 扩域十一族两变体编译锁（应绿全通过·预期红锚 %d 件）"
               % 变体红锚,
               not 变体坏 and not 变体红失,
               "异常=%s 红锚失效=%s" % (变体坏[:3], 变体红失[:3]))

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
