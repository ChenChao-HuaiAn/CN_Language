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
V2_DEFS=("发射字符串释放" "发射容器析构" "发射资源字段析构" "循环跳出析构" "块出口析构")

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
grep -rn "发射容器析构(\|发射字符串释放(\|发射资源字段析构(\|循环跳出析构(\|块出口析构(" CN语言编译器v2/IR/ 2>/dev/null \
    | grep -v "函数 " | sed 's|CN语言编译器v2/IR/||' | cut -d: -f1,2 | sed 's/^/    /' | sort
echo
echo "── 76-a（第七十六轮）移除路径释放挂点（映射/集合/向量）──"
echo "  宿主 运行时释放函数定义（全量+单槽）："
for f in __cn_vector_free_strings __cn_chain_free_strings __cn_map_free_strings \
         __cn_map_free_slot __cn_seq_free_slot; do
    n=$(grep -rh "extern \"C\" void $f" src/runtime/ 2>/dev/null | wc -l)
    printf "    %-28s 定义 %s 处\n" "$f" "$n"
done
echo "  宿主 注入挂点方法名（injectContainerElemDestroy 匹配表；映射 与 容器 分支）："
grep -n 'mi.name == "析构\|mi.name == "释放内部数组' src/cn_compiler/ir/ir_oop.cpp | sed 's/^/    /'
echo "  v2 判定函数（元素串释放面 / 归一化面）："
grep -n "^函数 是字符串元素容器\|^函数 是字符串值映射" CN语言编译器v2/IR/IR容器.cn | sed 's/^/    /'
echo "  stdlib 挂点调用点（移除-覆盖-清空路径）："
grep -rn "自身.析构元素(\|自身.析构被移除(\|自身.析构键值(\|自身.析构值(" stdlib/*.cn | sed 's/^/    /'
echo
echo "── 81-a（第八十一轮）容器元素结构体字段 释放/移动挂点 ──"
echo "  宿主 元素遍历循环（全量释放单点事实源；平铺/链游分派）："
grep -rn "IRGenerator::emitContainerElemWalk\|IRGenerator::emitSingleElemRelease\|IRGenerator::isOwnedStrFieldElemContainer" \
    src/cn_compiler/ir/ | sed 's|src/cn_compiler/ir/||' | sed 's/^/    /'
echo "  宿主 元素移动挂点注入（memcpy + 源槽清零；stdlib 移位循环单点）："
grep -n 'mi.name == "移动元素"' src/cn_compiler/ir/ir_oop.cpp | sed 's/^/    /'
grep -rn "自身.移动元素(" stdlib/*.cn | sed 's/^/    /'
echo "  宿主 字段释放单点事实源（79-a 延续；81-a 元素字段复用）："
grep -n "IRGenerator::emitOwnedStrFieldFreesAt\|IRGenerator::emitOwnedFieldFreesFor" \
    src/cn_compiler/ir/ir_fields.cpp | sed 's/^/    /'
echo "  v2 判定函数（元素串释放经供给 obj 的 ~容器 注入；结构体元素不在 v2 侧生成）："
grep -n "^函数 是字符串元素容器\|^函数 是纯串字段结构体" CN语言编译器v2/IR/IR容器.cn | sed 's/^/    /'
echo "  v2 容器元素释放调用点（v2 生成代码路径；结构体元素经 obj）："
grep -rn "发射容器元素释放(" CN语言编译器v2/IR/ | grep -v "函数 " | sed 's|CN语言编译器v2/IR/||' | cut -d: -f1,2 | sed 's/^/    /' | sort
echo "  注：结构体元素字段串释放**单点在宿主**（stdlib 容器方法体注入，v2 经 obj 复用）"
echo "      ——两侧机制不同（v2 无对应生成面）= 设计选择，三列核对时按「v2 侧无此挂点」判读。"
echo
echo "── 82-a（第八十二轮）v2 返回物化（帧尾共享 retbuf 根治）──"
echo "  v2 物化/清理/限定 单点（调用发射后立即物化到独立槽 = LLVM sret 各自分配语义）："
grep -rn "物化返回值到独立槽(\|清理调用实参临时(\|限定返回类型ID(" CN语言编译器v2/IR/ \
    | grep -v "函数 " | sed 's|CN语言编译器v2/IR/||' | cut -d: -f1,2 | sort -u | sed 's/^/    /'
echo "  v2 槽分配纪律（预扫槽区 / retbuf 区 / 生成期临时槽）："
grep -n "函数 扫描函数\|函数 retbuf基槽\|函数 变量槽偏移" CN语言编译器v2/代码生成/代码生成共用.cn | sed 's/^/    /'
echo "  注：物化槽=生成期临时槽（槽计数递增；扫描函数 按 IR_分配 统计最大槽 → retbuf 区随之上移，两者不可能重叠）；"
echo "      基址槽=槽起+槽数-1（最大槽号=最低地址=结构体基址，②b B1 约定）。"
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
# 76-a：单槽释放的清零（__cn_map_free_slot / __cn_seq_free_slot 内的槽清零）
h3=$(grep -rhE "keys\[index\] = nullptr|values\[index\] = nullptr|data\[index\] = nullptr" \
     src/runtime/io_api.cpp 2>/dev/null | wc -l)
echo "  宿主 emitStringFreeFor=$h1（内含 Store 0 清零）emitClassDeleteFor=$h2（同）"
echo "  宿主 76-a 单槽释放清零=$h3（__cn_map_free_slot/__cn_seq_free_slot 槽清零）"
echo "  v2   零寄存器 引用=$v1（释放点清零 + 预扫回填）"
if [ "$h1" -eq 0 ] || [ "$h2" -eq 0 ] || [ "$v1" -eq 0 ] || [ "$h3" -eq 0 ]; then
    echo "  ⚠ 单侧为 0——「释放+清零」不变量可能未同步到该侧（73-a 首版 SIGSEGV 即漏此条）"
fi
echo
echo "提示：语义等价性不可机械判定；本清单是核对入口，不是结论。"
