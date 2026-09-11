#!/bin/bash
# cn_release_parity.sh — 释放路径两侧对照（plans/020 维护纪律·移植纪律 8；73-a 订立）
#
# 用途：改动任何释放路径（RAII 析构 / drop-on-jump / 清槽）后，用 grep 级动作
#   拉出宿主（src/cn_compiler）与 v2（CN语言编译器v2）两侧的释放挂点清单，
#   逐条核对「时机 / 触发条件 / 依赖不变量」三列是否等价——重实现两侧分叉
#  （73-a：v2 缺 drop-on-jump、预扫登记使基线恒等）只能由此类对照发现。
#
# 本脚本只做**清单呈现与计数**，不做判定：语义等价性须人工阅读对照（机械
#   对比会漏掉「时机」这类语义差异）。计数为 0 或单侧为空时报警——那通常
#   意味着机制只存在于一侧（73-a 的缺陷形态）。
#
# 用法：bash scripts/cn_release_parity.sh
set -u
cd "$(dirname "$0")/.."

HOST_DEFS=("genStringFrees" "genClassDestructorCalls" "genJumpDestructFrom" "genBlockExitDestruct")
V2_DEFS=("发射字符串释放" "发射容器析构" "发射容器字段析构" "循环跳出析构" "块出口析构")

echo "═══ 释放路径两侧对照（plans/020 移植纪律 8）═══"
echo
echo "【宿主 src/cn_compiler】定义点："
for f in "${HOST_DEFS[@]}"; do
    n=$(grep -rh "^void IRGenerator::$f" src/cn_compiler/ir/ 2>/dev/null | wc -l)
    printf "  %-26s 定义 %s 处\n" "$f" "$n"
done
echo "  调用点："
grep -rn "genStringFrees()\|genClassDestructorCalls()\|genJumpDestructFrom(\|genBlockExitDestruct()" src/cn_compiler/ir/ 2>/dev/null \
    | grep -v "::" | sed 's|src/cn_compiler/ir/||' | cut -d: -f1,2 | sed 's/^/    /' | sort
echo
echo "【v2 CN语言编译器v2】定义点："
for f in "${V2_DEFS[@]}"; do
    n=$(grep -rh "^不安全 函数 $f\|^函数 $f" CN语言编译器v2/IR/ 2>/dev/null | wc -l)
    printf "  %-26s 定义 %s 处\n" "$f" "$n"
done
echo "  调用点："
grep -rn "发射容器析构(\|发射字符串释放(\|发射容器字段析构(\|循环跳出析构(\|块出口析构(" CN语言编译器v2/IR/ 2>/dev/null \
    | grep -v "函数 " | sed 's|CN语言编译器v2/IR/||' | cut -d: -f1,2 | sed 's/^/    /' | sort
echo
echo "── 人工核对三列（逐条问，勿跳）──────────────────"
echo "  时机：释放发生在块出口 / 跳出前 / 函数尾 的哪一种？两侧一致吗？"
echo "  条件：跳过名单（返回移出/装箱 move/污染名）两侧一致吗？"
echo "  不变量：是否依赖「释放后槽清零」幂等模型？两侧都搬了吗？（73-a 首版 SIGSEGV 即漏此条）"
echo
echo "── 不变量证据：释放+槽清零 幂等模型（两侧都应有非零计数）──"
# 宿主：emitStringFreeFor / emitClassDeleteFor（内部 Load -> free -> Store 0）
# v2  ：函数IR.零寄存器 驱动的释放后槽清零（73-a）
h1=$(grep -rh "void IRGenerator::emitStringFreeFor" src/cn_compiler/ir/ 2>/dev/null | wc -l)
h2=$(grep -rc "void IRGenerator::emitClassDeleteFor" src/cn_compiler/ir/ 2>/dev/null | awk -F: '{s+=$2} END{print s+0}')
v1=$(grep -rh "零寄存器" CN语言编译器v2/IR/ 2>/dev/null | wc -l)
echo "  宿主 emitStringFreeFor=$h1（内含 Store 0 清零）emitClassDeleteFor=$h2（同）"
echo "  v2   零寄存器 引用=$v1（释放点清零 + 预扫回填）"
if [ "$h1" -eq 0 ] || [ "$h2" -eq 0 ] || [ "$v1" -eq 0 ]; then
    echo "  ⚠ 单侧为 0——「释放+清零」不变量可能未同步到该侧（73-a 首版 SIGSEGV 即漏此条）"
fi
echo
echo "提示：语义等价性不可机械判定；本清单是核对入口，不是结论。"
