# -*- coding: utf-8 -*-
"""官网示例「平台」限定字段的值域与匹配（任务 1096·1017 over-skip 根治）。

背景（1096）：gen_docs verify 的 1017 平台限定原判据是
  `平台 not in 条["平台"]`，而 docs_data 声明值 ["linux"] 与三分平台名
  （linux-x86_64 / linux-arm64 / win-x64）永不相等 → 三平台全跳过
  （1017 注释宣称「linux 真跑」实际未达）。本模块把判定收口为
  「已登记族名/精确名」并明确族匹配语义：

    - 族名 linux        → {linux-x86_64, linux-arm64}（集合别名，覆盖两个 linux
      目标；不是任意字符串前缀——不得用 startswith 推断，新平台须登记）
    - 精确名三分        → 各自单点：linux-x86_64 / linux-arm64 / win-x64

语义：先按上表**展开成精确平台集合**，再做成员比较（不再有第二套判据）。

未登记值（拼写错误、空列表、非列表、含非字符串）= 显式报错退出，
禁止「未知平台值静默跳过」（AGENTS §3 miss 不可静默·1096 用户令）。
本模块不依赖 gen_docs，可独立自测：python website/tools/doc_platforms.py --selftest
"""
import sys

# 平台值域（唯一权威）：族名 + 精确名。新增平台须同时登记于此，
# 否则 gen_docs --verify 显式报错——不允许散写未校验的平台字符串。
平台族 = {
    "linux": ("linux-x86_64", "linux-arm64"),
}
平台精确名 = ("linux-x86_64", "linux-arm64", "win-x64")


def 展开(声明, 场景="?"):
    """平台声明 → 展开后的精确平台元组（去重保序）。

    非法声明一律 raise SystemExit（脚本退出码 1），不返回空/默认值：
      非 list/tuple、空列表、非字符串成员、未登记值。
    """
    if not isinstance(声明, (list, tuple)):
        raise SystemExit(
            "平台声明非法（%s）：须为平台值列表，实际 %r（1096·禁止静默跳过）"
            % (场景, 声明))
    if not 声明:
        raise SystemExit(
            "平台声明为空列表（%s）：空声明=永不执行，禁止静默绕过（1096）" % 场景)
    展开值 = []
    for 值 in 声明:
        if not isinstance(值, str):
            raise SystemExit(
                "平台声明含非字符串值（%s）：%r（1096·值域校验）" % (场景, 值))
        if 值 in 平台族:
            展开值.extend(平台族[值])
        elif 值 in 平台精确名:
            展开值.append(值)
        else:
            raise SystemExit(
                "未知平台值「%s」（%s）——合法值：族名 %s / 精确名 %s；"
                "未登记值禁止静默跳过（1096）"
                % (值, 场景, " / ".join(平台族), " / ".join(平台精确名)))
    结果 = []
    for 值 in 展开值:
        if 值 not in 结果:
            结果.append(值)
    return tuple(结果)


def 命中(声明, 目标平台, 场景="?"):
    """声明是否覆盖目标平台（族展开后精确比较——不做任意前缀/子串匹配）。"""
    return 目标平台 in 展开(声明, 场景)


# ---------------------------------------------------------------- 自测
def 自测():
    """正反用例：linux 族不跳过 / 三平台精确匹配 / 非法平台值报错。"""
    失败 = []
    计数 = [0]

    def 断言(名, 实际, 期望):
        计数[0] += 1
        if 实际 != 期望:
            失败.append("%s：期望 %r，实际 %r" % (名, 期望, 实际))

    # ① linux 族：两个 linux 平台都命中（over-skip 消除的回归锁）
    for 目标 in ("linux-x86_64", "linux-arm64"):
        断言("linux 族命中 %s" % 目标, 命中(["linux"], 目标, "自测"), True)
    断言("linux 族不命中 win-x64", 命中(["linux"], "win-x64", "自测"), False)

    # ② 三平台精确名：各只命中自己
    for 名, 目标 in (("linux-x86_64", "linux-x86_64"),
                     ("linux-arm64", "linux-arm64"),
                     ("win-x64", "win-x64")):
        断言("精确名 %s 命中自身" % 名, 命中([名], 目标, "自测"), True)
        for 其它 in 平台精确名:
            if 其它 != 目标:
                断言("精确名 %s 不命中 %s" % (名, 其它),
                     命中([名], 其它, "自测"), False)

    # ③ 混合声明=并集；重复值去重保序
    断言("混合族+精确名命中 win", 命中(["linux", "win-x64"], "win-x64", "自测"), True)
    断言("重复值去重", 展开(["linux", "linux-x86_64"], "自测"),
         ("linux-x86_64", "linux-arm64"))

    # ④ 非法值必须报错退出（禁止静默跳过）
    for 声明 in (["lunix"], ["linux-x86"], ["Linux"], "linux", [], [123],
                 ["linux", "lunix"], ("linux", "")):
        计数[0] += 1
        try:
            展开(声明, "非法样本")
        except SystemExit:
            continue
        else:
            失败.append("非法声明未报错：%r" % (声明,))

    if 失败:
        print("平台过滤自测：失败 %d 项" % len(失败))
        for 项 in 失败:
            print("  [×] " + 项)
        return 1
    print("平台过滤自测：全部通过（linux 族正反/三平台精确匹配/非法值报错 共 %d 项）"
          % 计数[0])
    return 0


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        sys.exit(自测())
    print(__doc__)
    sys.exit(1)
