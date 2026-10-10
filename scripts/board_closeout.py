# board_closeout.py — 看板账务收口·伞批销账号解析（392）
#
# 病根（392 呈报）：integrate 总账收口只认分支名里的任务号——伞批（一个分支
# 顺带修多件）的成员号天然漏销；021→服务端迁移窗口期漏销无兜底 → 僵尸行
# 长期冒充待办（373/374/375/376/332 五行实录）。
#
# 约定（登记制·零猜测）：伞批成员号写进**集成提交信息尾行**：
#     [销账:372,374,375,376]
# 总账收口除分支名号外，解析本批提交信息中的销账行，逐号 /api/task_update
# 置 ✅+收口sha。解析是唯一事实来源——提交信息没登记的号不销（防误销：
# 「立 N」「让位改 N」类提及绝不当销账依据）。
import re

# 登记行：[销账:372,374] / 全角冒号逗号同收；号=数字+可选小写字母后缀（308a 型）
销账行模式 = re.compile(r"\[销账\s*[:：]\s*([0-9a-zA-Z,，、\s]+?)\]")
号模式 = re.compile(r"[0-9]+[a-z]?")


def 解析销账号(提交信息: str) -> list:
    """从一条提交信息提取 [销账:…] 登记号们（去重·保出现序）。无登记返回空。"""
    号们: list = []
    for m in 销账行模式.finditer(提交信息 or ""):
        for 号 in 号模式.findall(m.group(1)):
            if 号 not in 号们:
                号们.append(号)
    return 号们


def _git文本(参数们: list) -> str:
    import subprocess
    r = subprocess.run(参数们, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return r.stdout if r.returncode == 0 else ""


def 收集远端分支销账号(remote: str, 分支: str, 基底: str = "") -> list:
    """收集一个远端任务分支全部提交的登记号们（solo 路径·删远端分支前调）。
    基底空=全分支历史扫描（登记幂等，多扫无害）；给基底则只扫分支自有提交。"""
    范围 = f"refs/remotes/{remote}/{分支}" if not 基底 else f"{基底}..refs/remotes/{remote}/{分支}"
    信息们 = _git文本(["git", "log", "--format=%B", 范围])
    return 解析销账号(信息们)


def 收集提交们销账号(提交们: list) -> list:
    """收集组链叠入提交（sha 列表）的登记号们（批量路径·cherry-pick 前的枚举）。"""
    号们: list = []
    for 提交 in 提交们:
        for 号 in 解析销账号(_git文本(["git", "log", "--format=%B", "-1", 提交])):
            if 号 not in 号们:
                号们.append(号)
    return 号们


# ===== 内置自测（python scripts/board_closeout.py）=====
if __name__ == "__main__":
    断言们 = [
        # (提交信息, 期望号们)
        ("修复某某\n[销账:372,374,375,376]", ["372", "374", "375", "376"]),
        ("〔通告〕xyz\n[销账：372，374]", ["372", "374"]),      # 全角冒号逗号
        ("[销账:308a、316]\n正文", ["308a", "316"]),            # 字母后缀号+顿号
        ("a[销账:1]b[销账:2]c", ["1", "2"]),                    # 多标记合并
        ("[销账:5,5,6]", ["5", "6"]),                           # 重复号去重
        ("立 379·撞号让位改 372", []),                           # 无登记=不销（提及≠销账）
        ("[销账:]", []),                                        # 空登记
        ("", []),                                               # 空信息
    ]
    for 信息, 期望 in 断言们:
        实际 = 解析销账号(信息)
        assert 实际 == 期望, f"解析失败：{信息!r} → {实际}（期望 {期望}）"
    print(f"board_closeout 自测：{len(断言们)} 例全过")
