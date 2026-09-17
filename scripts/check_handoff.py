#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""共享文档结构门禁（AGENTS.md §8.3「共享文档编辑六纪律」的机械自检；只读，不改任何文件）。

背景：HANDOFF.md 曾因「锚点局部插入＋旧文保留」的编辑方式逐轮翻倍（642 处重复标题、9268 行），
本脚本在提交前拦截同类结构缺陷——健康态 HANDOFF.md 约 140 行（三机节各 ~40 行）。

检查面：
  HANDOFF.md  — ① 三机节齐全 ② 每机节「### 一、」恰好 1 次（一分区一记录）
                ③ 节内标题无重复 ④ 无行中粘连标题（「正文。### 标题」= 事故签名）
                ⑤ 全文 ≤400 行、每机节 ≤120 行
  更新日志.md — ① 三机节齐全 ② 无行中粘连标题 ③ 全文 ≤400 行
                （注：机节内含 `## 功能完善总结` 子标题，故不做 `^## ` 切分与节内去重）
  plans/021   — ①「最后更新：第」戳恰好 1 段（一分区一记录——曾膨胀至 51 段叠加/300KB）
                ② §〇~§六 章节标题各恰好 1 次（曾出现整文件两份副本）
                ③ 全文 ≤600 行（健康态约 490 行）
                ④ §3-B「待呈报/待用户裁决」区禁止残留已裁决内容（含「原文留档」——
                   B5 三犯案·2026-09-16 用户令生命周期铁律）
  三机任务看板 — 每机状态行恰好 1 行（曾出现同机 2~3 行并存：旧开工行/旧收工行未随收工删除）；
                  空闲行（⬜）各列必须为「-」（曾出现空闲行残留收工总结/通告·232-a 用户令：有任务才填行）
  采样日志     — tests/cnsmith_hits/：①采样日志.md 为固定索引指针（存在、≤15 行、无「## 轮/## 汇总」
                  条目——286-a·S1b 起按月分文件，单文件追加式旧记法日增 1~3 万行曾达 2500+ 行）
                  ②日志/ 日文件名=YYYY-MM-DD.md（313-b 月→日·用户令）③单日文件 ≤1500 行（汇总记法回退=体积再爆炸的信号）

用法：python3 scripts/check_handoff.py
退出码：0 = 结构正常；1 = 存在结构缺陷（禁止提交，见 AGENTS.md §6.6）。
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent

HANDOFF_总行上限 = 400   # 健康态约 140 行
HANDOFF_单节上限 = 120   # 一分区一记录（六纪律 2）
日志_总行上限 = 400
机器关键词 = ("家机", "深度机", "单位机")
粘连标题模式 = re.compile(r"#{2,6}\s*[\u4e00-\u9fffA-Za-z0-9]")


def 粘连行清单(行们: list[str]) -> list[str]:
    """找出「行中粘连标题」行：非行首处出现 ##~###### 且其后直接跟文字（六纪律 3 的事故签名）。"""
    命中: list[str] = []
    for 序, 行 in enumerate(行们, 1):
        if re.match(r"\s*#{1,6}\s", 行):
            continue  # 行首标题本就合法
        检查文本 = 行.split("](#", 1)[0]  # 排除链接锚点 ](#xxx)
        检查文本 = re.sub(r"`[^`]*`", "", 检查文本)  # 排除行内代码对标题字样的提及（如「`## 功能完善总结`」）
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
    """HANDOFF.md 结构检查，返回问题清单（空=通过）。"""
    问题: list[str] = []
    路径 = 仓库根 / "HANDOFF.md"
    if not 路径.exists():
        return ["HANDOFF.md 缺失"]
    行们 = 路径.read_text(encoding="utf-8").splitlines()
    节s = 按二号标题分节(行们)
    机器节 = {(名, 标题): 内容
              for 标题, 内容 in 节s.items()
              for 名 in 机器关键词 if 名 in 标题}

    # ① 三机节齐全
    for 名 in 机器关键词:
        if not any(名 == 短名 for (短名, _标题) in 机器节):
            问题.append(f"① HANDOFF.md 缺「{名}」分节")

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
        问题.append("④ HANDOFF.md 存在行中粘连标题（六纪律 3 事故签名，编辑锚点必选错）：\n"
                    + "\n".join(粘连[:6]) + (f"\n      …等共 {len(粘连)} 行" if len(粘连) > 6 else ""))

    # ⑤ 全文行数
    if len(行们) > HANDOFF_总行上限:
        问题.append(f"⑤ HANDOFF.md 全文 {len(行们)} 行 > 上限 {HANDOFF_总行上限}")
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


总表_行上限 = 600        # 健康态约 480 行（230-a 指针化后）
总表_戳块行上限 = 5      # 文首「最后更新」戳块（戳行+连续引用行）——230-a 起指针化，禁止轮次总结复述
总表_单行上限 = 400      # 任意单行字符上限（防 §五 类「千字索引行」回归；旧态最长 1225 字符）
总表_章节标题们 = ("〇、", "一、", "二、", "三、", "四、", "五、", "六、")
总表_B区_完成标记们 = ("已裁决", "已批", "已选定", "✅")
# ^ §3-B 生命周期铁律（2026-09-16 用户令·B5 三犯后立）：条目经用户裁决必须**当轮迁出**——
#   已明确实施迁 §3-A、已定方案待排程迁 §3-C、完成销项进 §4.2；「✅ 用户裁决…以下为呈报原文留档」
#   式留档同属违规（历史由 git/plans/019/020 承载，本区只准有「等用户裁决」状态的条目）。
看板_机器名们 = ("家机 win-x64", "深度机 linux-x86_64", "单位机 ARM64")
看板_行宽上限 = 300      # 状态板行宽（230-a 指针化后 150~220；旧态最长 1387 字符=收工小作文）


def 查总表B区(行们: list[str]) -> list[str]:
    """plans/021 §3-B「待呈报/待用户裁决」区检查：禁止残留任何已裁决内容（含原文留档）。"""
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
    # ④ §3-B 区生命周期铁律（禁止已裁决内容残留）
    问题 += 查总表B区(行们)
    return 问题


def 查看板() -> list[str]:
    """三机任务看板.md 结构检查，返回问题清单（空=通过）。"""
    问题: list[str] = []
    路径 = 仓库根 / "三机任务看板.md"
    if not 路径.exists():
        return ["三机任务看板.md 缺失"]
    行们 = [行 for 行 in 路径.read_text(encoding="utf-8").splitlines()
            if re.match(r"^\| (家机|深度机|单位机)", 行)]
    for 名 in 看板_机器名们:
        行们机器 = [行 for 行 in 行们 if 行.startswith(f"| {名} |")]
        if len(行们机器) != 1:
            样例 = "；".join(行.strip()[:48] for 行 in 行们机器[:3])
            问题.append(f"看板「{名}」状态行出现 {len(行们机器)} 行（应恰 1 行——"
                        f"开工/收工须替换本机旧行而非追加新行）：{样例}")
            continue
        行 = 行们机器[0].rstrip()
        if len(行) > 看板_行宽上限:
            问题.append(f"看板「{名}」行 {len(行)} 字符 > 上限 {看板_行宽上限}"
                        "（状态板行=状态+任务名+基线+时间+指针；总结/教训进 plans/025 回填与 lessons）")
        列们 = [列.strip() for 列 in 行.split("|")[1:-1]]
        if len(列们) >= 3 and 列们[1] == "⬜" and any(列 != "-" for 列 in 列们[2:]):
            问题.append(f"看板「{名}」为空闲态（⬜）但行内残留内容（空闲行=各列「-」——"
                        f"有任务才填行，结论/通告一律不留看板·232-a 用户令）：{行[:60]}…")
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


def 主流程() -> int:
    print("=== 共享文档结构门禁（六纪律机械自检，只读）===")
    交接路径 = 仓库根 / "HANDOFF.md"
    if 交接路径.exists():
        print("  HANDOFF.md 各节行数：")
        for 标题, 内容 in 按二号标题分节(交接路径.read_text(encoding="utf-8").splitlines()).items():
            print(f"    {标题[:44]}：{len(内容)} 行")

    全部问题 = 查交接() + 查日志() + 查总表() + 查看板() + 查冲突标记() + 查采样日志()
    if 全部问题:
        print(f"\n结论：结构缺陷 {len(全部问题)} 项，禁止提交 ✗")
        for 问题 in 全部问题:
            print("[×] " + 问题)
        return 1
    print("\n结论：HANDOFF.md / 更新日志.md / plans/021 / 三机任务看板 / 采样日志 结构正常 ✓")
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
