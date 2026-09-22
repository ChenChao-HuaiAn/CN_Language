#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""共享文档结构门禁（AGENTS.md §8.3「共享文档编辑六纪律」的机械自检；只读，不改任何文件）。

背景：交接.md（原 HANDOFF.md·425-a 改名）曾因「锚点局部插入＋旧文保留」的编辑方式逐轮翻倍（642 处重复标题、9268 行），
本脚本在提交前拦截同类结构缺陷——健康态约 140 行（三机节各 ~40 行）。

检查面：
  交接.md   — ① 三机节齐全 ② 每机节「### 一、」恰好 1 次（一分区一记录）
                ③ 节内标题无重复 ④ 无行中粘连标题（「正文。### 标题」= 事故签名）
                ⑤ 全文 ≤400 行、每机节 ≤120 行
  更新日志.md — ① 三机节齐全 ② 无行中粘连标题 ③ 全文 ≤400 行
                （注：机节内含 `## 功能完善总结` 子标题，故不做 `^## ` 切分与节内去重）
  plans/021   — ①「最后更新：第」戳恰好 1 段（一分区一记录——曾膨胀至 51 段叠加/300KB）
                ② §〇~§六 章节标题各恰好 1 次（曾出现整文件两份副本）
                ③ 全文 ≤620 行（健康态约 490 行·387-a 随台账实态修正 600→620）
                ④ §3-B「待呈报/待用户裁决」区禁止残留已裁决内容（含「原文留档」——
                   B5 三犯案·2026-09-16 用户令生命周期铁律）
  三机任务看板 — 每机状态行恰好 1 行（曾出现同机 2~3 行并存：旧开工行/旧收工行未随收工删除）；
                  空闲行（⬜）各列必须为「-」（曾出现空闲行残留收工总结/通告·232-a 用户令：有任务才填行）；
                  开发态（🏃）任务列必含 任务/ 分支名（协议 v2·AGENTS.md §8.1·2026-09-18）；
                  开工自检（332-a·用户令）：当前分支为 `任务/*` 时看板必含该分支名——
                  「开工第一动作＝直推看板本机行」的机械拦截（先动手后填板不可通过）；
                  结构=每机独立小节（332-a 变更：单表三行相邻→两机同时改各自行 rebase 必冲突）
  采样日志     — tests/cnsmith_hits/：①采样日志.md 为固定索引指针（存在、≤15 行、无「## 轮/## 汇总」
                  条目——286-a·S1b 起按月分文件，单文件追加式旧记法日增 1~3 万行曾达 2500+ 行）
                  ②日志/ 日文件名=YYYY-MM-DD.md（313-b 月→日·用户令）③单日文件 ≤1500 行（汇总记法回退=体积再爆炸的信号）

用法：python3 scripts/check_handoff.py
退出码：0 = 结构正常；1 = 存在结构缺陷（禁止提交，见 AGENTS.md §6.6）。
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent

HANDOFF_总行上限 = 400   # 健康态约 140 行
HANDOFF_单节上限 = 120   # 一分区一记录（六纪律 2）
日志_总行上限 = 400
机器关键词 = ("家机", "深度机", "单位机")
粘连标题模式 = re.compile(r"#{2,6}\s*[\u4e00-\u9fffA-Za-z0-9]")
看板_v2首轮号 = 328  # 协议 v2 生效轮（2026-09-18·328-a）：≥此号的 🏃 行必含 任务/ 分支名；
                    # <此号=旧协议直推在飞轮的过渡豁免（无分支可标·收工清行后豁免自然消亡）


def 粘连行清单(行们: list[str]) -> list[str]:
    """找出「行中粘连标题」行：非行首处出现 ##~###### 且其后直接跟文字（六纪律 3 的事故签名）。

    315-a 增强：行首标题行同样受检——剥离行首合法标题前缀后再查（「## 节名## 节名## …」
    行内重复拼接形态=308-a 实锤事故，旧版把整行当合法标题直接跳过=假绿根因）。
    """
    命中: list[str] = []
    for 序, 行 in enumerate(行们, 1):
        检查文本 = 行.split("](#", 1)[0]  # 排除链接锚点 ](#xxx)
        检查文本 = re.sub(r"`[^`]*`", "", 检查文本)  # 排除行内代码对标题字样的提及（如「`## 功能完善总结`」）
        if re.match(r"\s*#{1,6}\s", 检查文本):
            # 行首标题行：剥离首个标题前缀，行内再出现标题字样=行内粘连（事故形态）
            检查文本 = re.sub(r"^\s*#{1,6}\s*", "", 检查文本, count=1)
        else:
            检查文本 = 检查文本.lstrip()
        if 粘连标题模式.search(检查文本):
            命中.append(f"      L{序}: {行.strip()[:72]}")
    return 命中


def 按二号标题分节(行们: list[str]) -> dict[str, list[str]]:
    """按 `^## ` 切分为 {节标题: 节内容行}；首个 `## ` 之前归入「(序章)」。"""
    节s: dict[str, list[str]] = {"(序章)": []}
    当前 = "(序章)"
    for 行 in 行们:
        if re.match(r"^## \S", 行):
            当前 = 行.strip()
            节s[当前] = []
        else:
            节s[当前].append(行)
    return 节s


def 查交接() -> list[str]:
    """交接.md（原 HANDOFF.md）结构检查，返回问题清单（空=通过）。"""
    问题: list[str] = []
    路径 = 仓库根 / "交接.md"
    if not 路径.exists():
        return ["交接.md 缺失（原 HANDOFF.md·425-a 改名）"]
    行们 = 路径.read_text(encoding="utf-8").splitlines()
    节s = 按二号标题分节(行们)
    机器节 = {(名, 标题): 内容
              for 标题, 内容 in 节s.items()
              for 名 in 机器关键词 if 名 in 标题}

    # ① 三机节齐全
    for 名 in 机器关键词:
        if not any(名 == 短名 for (短名, _标题) in 机器节):
            问题.append(f"① 交接.md 缺「{名}」分节")

    # ②③⑤ 逐机节检查
    for (名, 标题), 内容 in 机器节.items():
        一们 = [行 for 行 in 内容 if re.match(r"^### 一、", 行)]
        if len(一们) != 1:
            问题.append(f"② 「{名}」节『### 一、』出现 {len(一们)} 次（应恰 1 次——一分区一记录，"
                        f"上轮记录须随整节替换消失）")
        标题们 = [行.strip() for 行 in 内容 if re.match(r"#{1,6}\s", 行)]
        重复 = sorted({标题 for 标题 in 标题们 if 标题们.count(标题) > 1})
        if 重复:
            样例 = "；".join(重复[:3])
            问题.append(f"③ 「{名}」节内标题重复 {len(重复)} 种（应每轮整节替换后仅 1 份）：{样例}")
        if len(内容) > HANDOFF_单节上限:
            问题.append(f"⑤ 「{名}」节 {len(内容)} 行 > 上限 {HANDOFF_单节上限}（一分区一记录）")

    # ④ 全文粘连标题
    粘连 = 粘连行清单(行们)
    if 粘连:
        问题.append("④ 交接.md 存在行中粘连标题（六纪律 3 事故签名，编辑锚点必选错）：\n"
                    + "\n".join(粘连[:6]) + (f"\n      …等共 {len(粘连)} 行" if len(粘连) > 6 else ""))

    # ⑤ 全文行数
    if len(行们) > HANDOFF_总行上限:
        问题.append(f"⑤ 交接.md 全文 {len(行们)} 行 > 上限 {HANDOFF_总行上限}")
    return 问题


def 查日志() -> list[str]:
    """更新日志.md 结构检查，返回问题清单（空=通过）。"""
    问题: list[str] = []
    路径 = 仓库根 / "更新日志.md"
    if not 路径.exists():
        return ["更新日志.md 缺失"]
    文本 = 路径.read_text(encoding="utf-8")
    行们 = 文本.splitlines()
    for 名 in 机器关键词:
        if not re.search(rf"^## .*{名}", 文本, re.M):
            问题.append(f"更新日志.md 缺「{名}」节")
    粘连 = 粘连行清单(行们)
    if 粘连:
        问题.append("更新日志.md 存在行中粘连标题：\n" + "\n".join(粘连[:6]))
    if len(行们) > 日志_总行上限:
        问题.append(f"更新日志.md 全文 {len(行们)} 行 > 上限 {日志_总行上限}")
    return 问题


总表_行上限 = 620        # 健康态约 490 行（230-a 指针化后）；387-a 调 600→620——台账「只增不删」
                         # 属性（§4.2/§五 单调增）使 develop 实态 599 已贴 600 顶（余量 1 行·任何机
                         # 正常轮必撞）——按台账现实增长修正余量（+3.3%·非无限放开），随看板契约广播
总表_戳块行上限 = 5      # 文首「最后更新」戳块（戳行+连续引用行）——230-a 起指针化，禁止轮次总结复述
总表_单行上限 = 400      # 任意单行字符上限（防 §五 类「千字索引行」回归；旧态最长 1225 字符）
总表_章节标题们 = ("〇、", "一、", "二、", "三、", "四、", "五、", "六、")
总表_B区_完成标记们 = ("已裁决", "已批", "已选定", "✅")
# ^ §3-B 生命周期铁律（2026-09-16 用户令·B5 三犯后立）：条目经用户裁决必须**当轮迁出**——
#   已明确实施迁 §3-A、已定方案待排程迁 §3-C、完成销项进 §4.2；「✅ 用户裁决…以下为呈报原文留档」
#   式留档同属违规（历史由 git/plans/019/020 承载，本区只准有「等用户裁决」状态的条目）。
总表_队列区_完成标记模式 = re.compile(r"✅\s*(?:全销项|销项|实质销项|销项确认|收官)|撤案")
# ^ §3 队列区「只列未完成」（481-a 清账轮立·2026-09-20）：§3 实测曾累积 31 行「原地标 ✅ 销项不迁」
#   的已完成条目（S1/D28~D47/F2-36a/G 区 20 行）致队列失真——完成即删并迁 §4.2（§4.2 台账规则），
#   本模式机械拦截复发；「波次1=B11 ✅（304-a）」类子项进度注记（✅ 后无销项字样）不误伤。
总表_队列_状态符号模式 = re.compile(r"[🔄⬜⏸]")
# ^ 队列纪律②（2026-09-21 用户批方案甲）：凡带 🔄/⬜/⏸ 的行必须写明「下一棒」（下一步动作与归属
#   ——待认领/挂等 X 回签后做 Y/随 Z 轮/重启触发），治「只有状态没有下一棒、接手会话无从接力」；
#   🏃（在飞）豁免——在飞条目的下一棒天然=集成回签。
看板_机器名们 = ("家机 win-x64", "深度机 linux-x86_64", "单位机 ARM64")
看板_行宽上限 = 300      # 状态板行宽（230-a 指针化后 150~220；旧态最长 1387 字符=收工小作文）


def 查总表B区(行们: list[str]) -> list[str]:
    """plans/021 §3-B「待呈报/待用户裁决」区检查：
    ① 禁止残留任何已裁决内容（含原文留档）；
    ② C21（2026-09-17 用户令·B5 三犯后第四形态）：无条目时须**完全空白**——
       区内非空行必须以呈报条目格式（`> **B`）开头；「迁出说明/纪律复述/（无）」
       式说明性残留一律拦截（迁出信息只写迁移目标行，不留在本区）。
    """
    问题: list[str] = []
    在B区 = False
    for 序, 行 in enumerate(行们, 1):
        if re.match(r"^### ", 行):
            在B区 = 行.strip().startswith("### B. ")
            continue
        if not 在B区:
            continue
        检查文本 = re.sub(r"`[^`]*`", "", 行)  # 排除行内代码对标记字样的提及
        命中 = [标记 for 标记 in 总表_B区_完成标记们 if 标记 in 检查文本]
        if 命中:
            问题.append(f"plans/021 §3-B（待呈报/待用户裁决）L{序} 出现裁决完成标记「{'」「'.join(命中)}」"
                        "——已裁决条目必须当轮迁出（§3-A/§3-C/§4.2），本区禁留任何已裁决内容"
                        f"（含「原文留档」·2026-09-16 用户令）：{行.strip()[:60]}")
        # C21：非空行必须是呈报条目本体（`> **B` 开头）——其余任何非空行都是
        # 说明性残留（迁出说明/纪律复述/（无）占位等），无条目=完全空白
        if 行.strip() and not 行.lstrip().startswith("> **B"):
            问题.append(f"plans/021 §3-B L{序} 为非条目非空行（C21：本区无条目时须完全空白，"
                        "呈报条目必须以「> **B」格式开头；迁出信息只写迁移目标行不留在本区）："
                        f"{行.strip()[:60]}")
    return 问题


def 查总表队列区(行们: list[str]) -> list[str]:
    """plans/021 §3 排班队列区检查（481-a 立）：队列只列未完成任务（§3 说明+§4.2 销项规则），
    禁止「原地标 ✅ 销项/收官/撤案而不迁 §4.2」的残留——31 行累积事故（481-a 清账轮根治）的防复发门禁。"""
    问题: list[str] = []
    在队列区 = False
    for 序, 行 in enumerate(行们, 1):
        if re.match(r"^## ", 行):
            在队列区 = 行.startswith("## 三、排班队列")
            continue
        if not 在队列区:
            continue
        检查文本 = re.sub(r"`[^`]*`", "", 行)  # 排除行内代码对标记字样的提及
        命中 = 总表_队列区_完成标记模式.search(检查文本)
        if 命中:
            问题.append(f"plans/021 §3 队列区（只列未完成）L{序} 出现完成标记「{命中.group(0)}」——"
                        "完成即从 §3 删除并迁 §4.2 台账留痕（销项编号退役不复用），禁止原地标注不迁"
                        f"（481-a 清账轮立门禁）：{行.strip()[:60]}")
        # 606-a 扩（树状重组轮）：§三 由平表改多级缩进树后，树节点行（缩进 `- `）与平表行（`|`）
        #   同受「状态符号→必带下一棒」与「下一棒依赖语义→必带任务号」校验——结构换了防线不弱化。
        是队列行 = 行.startswith("|") or bool(re.match(r"\s*- ", 行))
        if 是队列行 and 总表_队列_状态符号模式.search(检查文本) and "下一棒" not in 检查文本:
            问题.append(f"plans/021 §3 队列区 L{序} 状态符号行缺「下一棒」——凡 🔄/⬜/⏸ 行必须写明"
                        "下一步动作与归属（待认领/挂等X回签后做Y/随Z轮/重启触发；方案甲·队列纪律②）："
                        f"{行.strip()[:60]}")
        # 队列纪律⑥（2026-09-22 用户令「依赖即父子号」）：下一棒含依赖语义而无任务号引用=拦截——
        #   轮次号=执行批次非任务实体，只写轮次号则轮次销账后依赖链断、接手会话无从追溯。
        #   合格引用形态：子任务号 `NNN-NNN` ／ 「任务 NNN」／ 「NNN 行」。
        if 是队列行 and "下一棒" in 检查文本:
            下一棒列 = 检查文本.split("下一棒", 1)[1]
            if re.search(r"依赖|挂等|接力|解锁|先于|随\s*\d+\s*轮|随\s*\d+\s*集成", 下一棒列):
                if not re.search(r"\d{3}-\d{3}|任务\s*\d{3}|\d{3}\s*行", 下一棒列 + 检查文本.split("下一棒", 1)[0]):
                    问题.append(f"plans/021 §3 队列区 L{序} 下一棒含依赖语义但无任务号引用——"
                                "依赖即父子号（AGENTS §6.8⑥·2026-09-22 用户令）：立子任务号 `父号-三位子序` "
                                "或显式引用「任务 NNN」/「NNN 行」，禁止只写轮次号（轮次销账后依赖链断）："
                                f"{行.strip()[:60]}")
    return 问题


def 查总表() -> list[str]:
    """plans/021 任务进度观察表结构检查，返回问题清单（空=通过）。"""
    问题: list[str] = []
    路径 = 仓库根 / "plans" / "021-任务进度观察表.md"
    if not 路径.exists():
        return ["plans/021 任务进度观察表缺失"]
    行们 = 路径.read_text(encoding="utf-8").splitlines()
    # ① 戳恰好 1 段（一分区一记录——历史由 git 承载）
    戳们 = [行 for 行 in 行们 if "最后更新：第" in 行]
    if len(戳们) != 1:
        问题.append(f"① plans/021 文首「最后更新」戳出现 {len(戳们)} 段（应恰 1 段——"
                    "旧戳须随刷新删除，历史由 git 承载）")
    else:
        # ①b 戳块行数（戳行 + 其后连续引用行 ≤ 上限——指针化后禁止轮次总结复述）
        s = 行们.index(戳们[0])
        块 = 1
        while s + 块 < len(行们) and 行们[s + 块].startswith(">"):
            块 += 1
        if 块 > 总表_戳块行上限:
            问题.append(f"①b plans/021 戳块 {块} 行 > 上限 {总表_戳块行上限}"
                        "（指针化：轮次总结留在 git/plans/019/020，本表不复述）")
    # ①c 单行宽度
    宽行 = [(序, len(行.rstrip())) for 序, 行 in enumerate(行们, 1)
            if len(行.rstrip()) > 总表_单行上限]
    if 宽行:
        样例 = "；".join(f"L{序}({宽}字符)" for 序, 宽 in 宽行[:4])
        问题.append(f"①c plans/021 单行超 {总表_单行上限} 字符共 {len(宽行)} 行"
                    f"（超长=流水账复述签名，应拆到权威源+留指针）：{样例}")
    # ② 章节标题各恰好 1 次
    for 章节前缀 in 总表_章节标题们:
        n = sum(1 for 行 in 行们 if re.match(rf"^## {re.escape(章节前缀)}", 行))
        if n != 1:
            问题.append(f"② plans/021 章节『## {章节前缀}…』出现 {n} 次（应恰 1 次——"
                        "整文件重复副本=「追加代替替换」事故签名）")
    # ③ 全文行数
    if len(行们) > 总表_行上限:
        问题.append(f"③ plans/021 全文 {len(行们)} 行 > 上限 {总表_行上限}")
    # ④ §3-B 区生命周期铁律（禁止已裁决内容残留）+ §3 队列区完成标记残留（481-a）
    问题 += 查总表B区(行们)
    问题 += 查总表队列区(行们)
    return 问题


def 查看板() -> list[str]:
    """三机任务看板.md 结构检查，返回问题清单（空=通过）。

    332-a 结构变更（2026-09-18 用户令同日）：看板表＝**每机独立小节**
    （`### <机名>` + 4 列单行表），各机只编辑自己小节——单表三行相邻结构下，
    两机同时更新各自行 rebase **必冲突**（332-a 当日实测两次）。
    """
    问题: list[str] = []
    在飞号们: list[tuple[str, int]] = []   # (机器名, 轮次号)——🏃 行任务列提取（341-a 跨机撞号防线）
    路径 = 仓库根 / "三机任务看板.md"
    if not 路径.exists():
        return ["三机任务看板.md 缺失"]
    行们 = 路径.read_text(encoding="utf-8").splitlines()
    for 名 in 看板_机器名们:
        节起 = next((n for n, 行 in enumerate(行们) if 行.strip() == f"### {名}"), None)
        if 节起 is None:
            问题.append(f"看板缺「### {名}」小节（332-a 结构：每机独立小节·各机只改自己节）")
            continue
        状态行们: list[str] = []
        for 行 in 行们[节起 + 1:]:
            if 行.startswith("## ") or 行.startswith("### "):
                break
            if not 行.startswith("|"):
                continue
            列们 = [列.strip() for 列 in 行.split("|")[1:-1]]
            if len(列们) != 4 or 列们[0] == "状态" or set(列们[0]) <= set("-: "):
                continue  # 表头/分隔行
            状态行们.append(行.rstrip())
        if not (1 <= len(状态行们) <= 8):
            样例 = "；".join(行[:48] for 行 in 状态行们[:3])
            问题.append(f"看板「{名}」状态行出现 {len(状态行们)} 行（应 1~8 行——单任务 1 行/"
                        "本机多开每任务 1 行〔449-a·AGENTS.md §8.8·459-a 上限 4→6 放宽："
                        "多开 5 任务+挂起轮并存实态；**548-a 上限 6→8 再放宽：多会话 7+ 并行任务实态·待用户追认**〕；开工/收工须替换本机"
                        f"对应任务行而非追加新行）：{样例}")
            continue
        for 行 in 状态行们:
            if len(行) > 看板_行宽上限:
                问题.append(f"看板「{名}」行 {len(行)} 字符 > 上限 {看板_行宽上限}"
                            "（状态板行=状态+任务名+基线+时间+指针；总结/教训进 plans/025 回填与 项目记忆/教训.md）")
            列们 = [列.strip() for 列 in 行.split("|")[1:-1]]
            if 列们[0] == "⬜" and any(列 != "-" for 列 in 列们[1:]):
                问题.append(f"看板「{名}」为空闲态（⬜）但行内残留内容（空闲行=各列「-」——"
                            f"有任务才填行，结论/通告一律不留看板·232-a 用户令）：{行[:60]}…")
            if 列们[0] == "⬜" and len(状态行们) > 1:
                问题.append(f"看板「{名}」多任务行中含空闲行（⬜ 只在整节无任务时出现·449-a）：{行[:60]}…")
            # 协议 v2（2026-09-18·AGENTS.md §8.1）：🏃=任务分支开发中，任务列必含分支名
            # ——轮次号 <328（v2 首轮前）的旧协议直推在飞轮豁免（无分支可标·收工清行即不再触发）
            if 列们[0] == "🏃" and "任务/" not in 列们[1]:
                号匹配 = re.search(r"(\d{1,4})-[a-z]", 列们[1])
                轮次号 = int(号匹配.group(1)) if 号匹配 else None
                if 轮次号 is None or 轮次号 >= 看板_v2首轮号:
                    问题.append(f"看板「{名}」为开发态（🏃）但任务列缺分支名（协议 v2：🏃 行必含"
                                f" 任务/<机>-<轮次>-<标识> 分支名·AGENTS.md §8.1）：{行[:60]}…")
            # 在飞轮次号登记（跨机撞号防线·341-a）：ASCII 大小写/中文标识均提取
            # （340-D38批3b / 341-撞号防线 / 329-D12）；「2026-09」类日期因连字符后为数字不误匹配
            if 列们[0] == "🏃":
                for 号文本 in re.findall(r"(\d{1,4})-(?:[a-zA-Z]|[\u4e00-\u9fff])", 列们[1]):
                    在飞号们.append((名, int(号文本)))
    # 跨机撞号检查（341-a·用户裁决方案①「认领前先看板、先到先得」）：同一轮次号
    # 被 ≥2 台机器的在飞行占用即拦截——「全表最大+1」三机并行必撞（332-a 实证双 329-a）
    号主们: dict[int, set[str]] = {}
    for 名, 号 in 在飞号们:
        号主们.setdefault(号, set()).add(名)
    for 号, 主们 in sorted(号主们.items()):
        if len(主们) > 1:
            问题.append(f"看板在飞轮次号 {号} 被多机占用（{'/'.join(sorted(主们))}）——跨机撞号："
                        "认领前先 `git fetch` 读看板取未占号（先到先得·AGENTS.md §8.1 用户裁决"
                        " 2026-09-18），后到者当场换号重推")
    # 通告段治理（570-a·用户批准 2026-09-21）：§8.6「影响面执行完毕后删除本机通告段」
    # 此前无机械校验——4 天 230→830 行（通告段 768 行只增不删·同 HANDOFF 粘连事故规律）。
    # 每机活跃通告段小节 ≤1；全文行数另由查棘轮()承载（基线登记 三机任务看板.md）。
    for 名 in 看板_机器名们:
        段标题们 = [行 for 行 in 行们 if 行.startswith("### ") and 名 in 行 and "通告段" in 行]
        if len(段标题们) > 1:
            问题.append(f"看板「{名}」通告段出现 {len(段标题们)} 个小节（应 ≤1——§8.6 一分区一记录："
                        "影响面执行完毕后删除本机通告段·历史由 git 历史与 plans/021 承载）："
                        + "；".join(段.strip()[:44] for 段 in 段标题们))
    return 问题


def 当前分支名() -> str | None:
    """读取当前 git 分支名（非 git 环境或异常时返回 None，供开工自检用）。"""
    try:
        结果 = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"],
                             cwd=仓库根, capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.SubprocessError):
        return None
    if 结果.returncode != 0:
        return None
    名 = 结果.stdout.strip()
    return 名 or None


def 分支已集成() -> bool:
    """当前 HEAD 是否已包含在集成分支（gitcode/develop 或本地 develop）中。

    已集成分支不再要求看板行——**收工销账（清行）后分支名自然不在看板**，
    332-a 实测：销账动作本身会触发开工自检误拦（自建制度首日自证 bug），
    故判据加本条豁免；引用不存在（未 fetch）时保守不豁免。
    """
    for 候选 in ("gitcode/develop", "develop"):
        try:
            结果 = subprocess.run(["git", "merge-base", "--is-ancestor", "HEAD", 候选],
                                 cwd=仓库根, capture_output=True, text=True, timeout=10)
        except (OSError, subprocess.SubprocessError):
            return False
        if 结果.returncode == 0:
            return True
    return False


def 查开工自检() -> list[str]:
    """开工自检（332-a·用户令 2026-09-18）：当前分支为任务分支时，看板必须已含该分支名。

    判据＝分支名出现在看板文件任一行——即「开工第一动作＝直推看板本机行到 develop」已执行
    （分支自 develop 开出，故看板内必含本分支名）。把看板更新写进任务分支而未直推时，
    分支内看板不含本分支名 → 本检查拦截（「先动手后填板」不可通过门禁）。跨机 try-build
    在他机分支上跑同样满足（分支名在其机行内）。服务轮/develop 分支不触发。
    豁免：分支已集成（收工销账清行后，分支名自然不在看板——332-a 自证实测）。
    """
    分支 = 当前分支名()
    if not 分支 or not 分支.startswith("任务/"):
        return []
    路径 = 仓库根 / "三机任务看板.md"
    if not 路径.exists():
        return ["三机任务看板.md 缺失（开工自检无法执行）"]
    if 分支 in 路径.read_text(encoding="utf-8"):
        return []
    # 接力分支补判据（351-a·2026-09-18）：接力分支基点＝上一轮分支头，可能**早于**本轮
    #   看板直推提交（直推落在 develop 窄通道·不在接力基点内）——本地看板不含≠未直推。
    #   再查**远端 develop 的看板**（已 fetch 的 gitcode/develop）：远端已含分支名＝直推
    #   已执行（他机可见·门禁本义已满足）→ 放行；远端引用不存在/未 fetch 时保守不豁免。
    #   反例（本地+远端均不含=真未直推）仍拦截。
    try:
        _远端 = subprocess.run(["git", "show", "gitcode/develop:三机任务看板.md"],
                              cwd=仓库根, capture_output=True, text=True, timeout=10)
        if _远端.returncode == 0 and 分支 in _远端.stdout:
            return []
    except (OSError, subprocess.SubprocessError):
        pass
    if 分支已集成():
        return []
    return [f"开工自检：当前分支 {分支} 未出现在看板任何行中（且尚未集成）——开工第一动作应为"
            "「编辑本机小节（🏃+分支名）并直推 develop」（AGENTS.md §8.1 看板先行·§8.5 窄通道①）："
            "看板写进任务分支＝他机不可见＝等于没写（332-a 实测三机行全在各自分支内·develop 全空）"]


def 查重复节标题() -> list[str]:
    """同名 `## 节标题` 重复检测（332-a 立·lessons 206「新事故同轮扩门禁」）。

    事故：整节替换时切片 off-by-one——`lines[:73]` 把 index 72 的节标题行纳入保留区，
    再拼新标题 → 标题重复（HANDOFF.md／更新日志.md 单位机节各一处）；旧版门禁只查
    节内 ### 标题去重与粘连，**未覆盖 ## 节标题本身** = 假绿。
    """
    问题: list[str] = []
    for 名 in ("HANDOFF.md", "更新日志.md", "plans/025-三机任务统筹与实施计划.md"):
        路径 = 仓库根 / 名
        if not 路径.exists():
            continue
        标题们 = [行.strip() for 行 in 路径.read_text(encoding="utf-8").splitlines()
                 if 行.startswith("## ")]
        for t in sorted({t for t in 标题们 if 标题们.count(t) > 1}):
            问题.append(f"{名} 出现重复节标题「{t}」×{标题们.count(t)}——分机分区整节替换必须"
                        "整段覆盖（切片 off-by-one「标题行被纳入保留区再拼新标题」是典型成因·332-a 事故）")
    return 问题


def 查冲突标记() -> list[str]:
    """四文件 git 冲突标记检查（233-a 立：229-a 收尾曾把未解决冲突标记推入 plans/021 在库多日）。

    判据=「<<<<<<< 」与「>>>>>>> 」在同一文件成对出现即拦截（单独的 ======= 可能是合法分隔线，不误报）。
    """
    问题: list[str] = []
    for 名 in ("HANDOFF.md", "更新日志.md", "plans/021-任务进度观察表.md", "三机任务看板.md"):
        路径 = 仓库根 / 名
        if not 路径.exists():
            continue
        行们 = 路径.read_text(encoding="utf-8").splitlines()
        起们 = [n + 1 for n, 行 in enumerate(行们) if 行.startswith("<<<<<<< ")]
        止们 = [n + 1 for n, 行 in enumerate(行们) if 行.startswith(">>>>>>> ")]
        if 起们 or 止们:
            问题.append(f"{名} 存在未解决的 git 冲突标记（<<<<<<< 行 {起们} / >>>>>>> 行 {止们}）——"
                        "rebase 冲突必须当场解决并 grep 复查后才能提交")
    return 问题


def 查采样日志() -> list[str]:
    """tests/cnsmith_hits/ 采样日志结构检查（286-a·S1b 分文件化防回退）。"""
    问题: list[str] = []
    指针 = 仓库根 / "tests" / "cnsmith_hits" / "采样日志.md"
    月目录 = 仓库根 / "tests" / "cnsmith_hits" / "日志"
    if not 指针.exists():
        return ["tests/cnsmith_hits/采样日志.md 缺失（索引指针·286-a 起=固定指针不追加轮次）"]
    行们 = 指针.read_text(encoding="utf-8").splitlines()
    if len(行们) > 15:
        问题.append(f"采样日志.md（索引指针）{len(行们)} 行 > 上限 15——指针只写判据与路径规则，"
                    "轮次/汇总条目一律写 日志/YYYY-MM-DD.md（286-a·S1b+313-b 日文件制）")
    条目 = [行 for 行 in 行们 if re.match(r"^## (轮|汇总)", 行)]
    if 条目:
        问题.append(f"采样日志.md（索引指针）出现轮次/汇总条目 {len(条目)} 处——"
                    "daemon 旧版或手工追加写回了旧单文件结构，须移入 日志/ 日文件")
    if not 月目录.is_dir():
        问题.append("tests/cnsmith_hits/日志/ 目录缺失（286-a 起分文件·313-b 起按日）")
        return 问题
    for p in sorted(月目录.glob("*.md")):
        if not re.fullmatch(r"\d{4}-\d{2}-\d{2}\.md", p.name):
            问题.append(f"日志/{p.name} 文件名不符 YYYY-MM-DD.md 约定（313-b 日文件制）")
        n = len(p.read_text(encoding="utf-8").splitlines())
        if n > 1500:
            问题.append(f"日志/{p.name} {n} 行 > 上限 1500（313-b 日文件制·汇总记法回退=体积再爆炸信号——"
                        "查 daemon 是否旧版在跑/变化轮异常频发）")
    return 问题



def 查棘轮() -> list[str]:
    """文档棘轮基线检查（425-a 立·AGENTS.md §6.7 文档生命周期）：登记文件当前行数 > 基线 → 拦截。
    基线只紧不松：收割/滚转轮随写集收紧基线值；放宽须在提交信息说明理由。"""
    问题: list[str] = []
    基线文件 = 仓库根 / "scripts" / "文档棘轮基线.txt"
    if not 基线文件.exists():
        return ["scripts/文档棘轮基线.txt 缺失（425-a 文档生命周期门禁依赖·AGENTS.md §6.7）"]
    for 行 in 基线文件.read_text(encoding="utf-8").splitlines():
        行 = 行.strip()
        if not 行 or 行.startswith("#"):
            continue
        路径文本, sep, 数文本 = 行.replace("：", ":").partition(":")
        if not sep:
            问题.append(f"棘轮基线行格式异常（应为 路径: 行数）：{行[:60]}")
            continue
        try:
            上限 = int(数文本.strip())
        except ValueError:
            问题.append(f"棘轮基线行数非整数：{行[:60]}")
            continue
        目标 = 仓库根 / 路径文本.strip()
        if not 目标.exists():
            问题.append(f"棘轮登记文件缺失：{路径文本.strip()}")
            continue
        实际 = len(目标.read_text(encoding="utf-8").splitlines())
        if 实际 > 上限:
            问题.append(f"棘轮超限：{路径文本.strip()} 当前 {实际} 行 > 基线 {上限} 行——"
                        f"先滚转至 项目记忆/归档/ 再追加（AGENTS.md §6.7·状态/事件分层）")
    return 问题


def 主流程() -> int:
    print("=== 共享文档结构门禁（六纪律机械自检，只读）===")
    交接路径 = 仓库根 / "交接.md"
    if 交接路径.exists():
        print("  交接.md（原 HANDOFF.md）各节行数：")
        for 标题, 内容 in 按二号标题分节(交接路径.read_text(encoding="utf-8").splitlines()).items():
            print(f"    {标题[:44]}：{len(内容)} 行")

    全部问题 = (查交接() + 查日志() + 查总表() + 查看板() + 查重复节标题() + 查冲突标记()
                + 查采样日志() + 查开工自检() + 查棘轮())
    if 全部问题:
        print(f"\n结论：结构缺陷 {len(全部问题)} 项，禁止提交 ✗")
        for 问题 in 全部问题:
            print("[×] " + 问题)
        return 1
    print("\n结论：交接.md / 更新日志.md / plans/021 / 三机任务看板（含开工自检）/ 采样日志 / 文档棘轮 结构正常 ✓")
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
