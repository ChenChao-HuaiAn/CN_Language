// 发射后窥孔·冗余槽读写消除（任务 1144·波2；任务 1157 三语法面适配；
//   任务 1243 AArch64 大偏移间接寻址面；任务 1320 死槽写 + 近邻转发；
//   任务 1351 浮点同寄存器全删的高位清零语义）：
//   ① 槽转发（1144/1320）：store 到 vreg 槽 → 之后 load 同槽的纯转发搬运——槽死
//      （全函数存活引用 = 本对两处）且两行行距 <= 8、中间无控制流/标签/源寄存器
//      改写/槽别名访问时：删 store 行，load 行改写为从 store 源寄存器取值的寄存器
//      搬运；同寄存器同宽度时两行全删（32 位同宽对须先证明高 32 位已为零·1279）。
//   ② 死槽写（1320）：store 的值从未被读——槽全函数仅一处引用（含注释提及一律
//      计数），或同一槽的连续两次 store 之间无读且后写位宽覆盖前写（直线段内）。
//   ③ 浮点高半（1351）：x86 movss/movsd 的 load 形态清零高 96/64 位——同寄存器
//      全删/异寄存器改写都须先证明该寄存器高半从未被 128 位宽读观察，否则原样
//      保留两行；ARM64 标量寄存器搬运（fmov）本身清零上位，全删被拒时回落改写。
//   ④ 1243 AArch64 大偏移形态：|off|>255 时发射器以 x13 物化地址，真实产物为
//      「sub x13,x29,#N / str ..,[x13] / sub x13,x29,#N / ldr ..,[x13]」四行
//      （|off|>4095 为 movz/movk 链 + sub/add x13,x29,x13）；两块同文+同偏移全函数
//      仅两处引用+尾部无未定义读 ⇒ 四行整体消除（详见 .cpp 该节安全判据）。
// 设计要点：
//   1. 纯文本变换（行级、函数文本内），无后端状态依赖——单测可直接喂文本；
//   2. 识别本产物族实际产出的行形态，三方言语义同一、方言解析器分别注入：
//      ① linux_x86_64（GAS Intel 语法）：
//        store：mov qword ptr [rbp-N], r10 / mov dword ptr [rbp-N], r10d /
//               mov byte ptr [rbp-N], r10b / movsd [rbp-N], xmm0（浮点无 ptr）
//        load ：mov r10, qword ptr [rbp-N] / mov r10d, dword ptr [rbp-N] /
//               movsx r10, byte ptr [rbp-N] / movzx r10, word ptr [rbp-N] /
//               movsd xmm0, [rbp-N]
//      ② win-x64（MASM；组成器小写输出·MASM 语法大小写不敏感）：
//        同①文本形态，另接受无 ptr 前缀（mov [rbp-N], rax / mov rax, [rbp-N]）
//        与 32 位目的扩展（movsx eax, byte ptr [rbp-N]——x64 后端实际产物）；
//        大小写不敏感仅放宽识别面（QWORD PTR 亦识别），产物仍原样保留。
//      ③ linux-arm64（AArch64 GAS）：
//        store：str x3, [x29,#-N] / str w3, [x29,#-N] / strb w3 / strh w3 /
//               str d0, [x29,#-N] / str s0, [x29,#-N]
//        load ：ldr x9 / ldr w9 / ldrb w9 / ldrh w9 / ldrsb x9 / ldrsh x9 /
//               ldr d1 / ldr s1, [x29,#-N]
//        改写：mov（x/w）、uxtb/uxth、sxtb/sxth、fmov（d/s）。
//   3. 安全条件（缺一不可）：
//      a. store→load 行距 <= kForwardScanLimit（8），且两者之间（不含端点）每一行
//         都是直线段中性行：非标签/控制流（方言分类器 Control=拒绝，防跳转绕过
//         store）、不写 store 源寄存器（含 8/16 位部分写与隐式写）、不访问与候选槽
//         字节范围相交的其它槽。相邻对（行距 1）是其中的特例；store 之前紧邻标签
//         不影响（跳转到该标签仍顺序执行本对，见单测 LabelBeforeStoreStillEliminated）。
//      b. 槽引用判据：单槽全函数引用计数 <= 4（任何形式：读/写/取址/注释提及一律
//         计数，超限/未识别提及保守拒绝）；转发对要求「剔除死写后」存活引用恰好
//         两处 = store + load。不能用「本对之后无出现」——循环回边可自后文跳回
//         前文再次读槽（行号=代码位置，非动态执行序）。
//      c. 改写 load 时读宽度 ≤ 存宽度（存窄读宽会读入槽内残留高位，寄存器
//         搬运无法复现该残留 -> 拒绝）。
//      d. 全删（删除 store 与 load 两行）：同寄存器；64/64 或同宽浮点直接全删
//         （寄存器保持值即 load 语义）；32 位 dword/dword 同寄存器对须追加
//         「load 前该寄存器高 32 位已为零」证明（x64/ARM64 的 32 位目的写零扩展
//         高 32，删除后该语义必须由前态保证）。证明窗口=自 store 行向前至多 128 行
//         （kProofScanLimit）：x86 任一 32 位目的写/movzx/非负 imm32 装载；
//         ARM64 任一 W 目的写/uxt*/movz（lsl #32/#48 除外）/cset/mov xN,wM/非负
//         imm32。标签/控制流/未知 64 位写/call/cmov/ldp 等一律截断拒绝；8/16 位
//         部分写视为高位保持、继续向前；无定值点=拒绝。证明不成立时回落
//         「删 store + load 改写为寄存器搬运」，产物语义不变。
//      e. 死槽写（1320）：槽全函数仅一处引用（值从未被读）可删；或同一槽的连续
//         两次 store 之间无读、后写位宽覆盖前写且两写之间为直线段（跳转不得绕过
//         覆盖直接读前写）时可删前写。
//      f. 槽字节别名（1320）：全函数任一其它槽访问与候选槽 [off, off+bytes) 相交
//         即拒绝。整宽访存（x86 movups 族）按 16 字节；lea 取址按 16 字节聚合窗口
//         （本产物族多槽变量中只有 i128 以 $s1 双槽直接寻址=16 字节；数组/结构体
//         字段一律经 lea 基址 + 指针偏移访问、不出现在 [rbp±N] token 中）。
//      g. 浮点高半（1351）：x86 浮点对（同寄存器全删/异寄存器改写）须先证明该
//         寄存器高半从未被 128 位宽读观察（整函数扫描·保守不假设控制流）；不成立
//         时原样保留两行。ARM64 标量寄存器搬运清零上位，全删被拒时回落 fmov。
//      说明：AArch64 槽文本按组成器发射形态「[x29,#±N]」精确识别；含空格变体
//      （[x29, #-8]）不在本产物族，保守拒绝（该行原样保留）。
//   4. 门控：调用方按优化档位开关（-O2 起），低档位产物逐字节不变。
//   5. 1243 间接寻址面（仅 AArch64·产物族地址临时寄存器恒 x13）：地址物化块文本
//      逐行相同（同 x29+同常量 ⇒ 同地址）、该偏移全函数仅两块引用（帧布局偏移唯一，
//      含不同寄存器物化/直接寻址一律计数）、删除点之后 x13 必须先完整定义再被读
//      （尾部两态线性状态表·标签重置）、store 源/load 目的非地址寄存器。
//   6. 收敛性（1320 实测）：单次应用后再次应用不再命中（v2 全树 win/linux/arm64
//      均为 0 行，除 win 极少数 195 行需第二次——见提交信息残差说明）。
// 规范：英文API命名，中文仅注释；函数<=100行
#pragma once
#include <string>

namespace cn_compiler {
namespace peephole {

// 对单函数汇编文本做冗余槽读写消除，返回处理后文本（无命中时原样返回）。
//   linux-x86_64 面（GAS Intel 语法·1144 原判据）
std::string eliminateRedundantSlotTraffic(const std::string& functionText);

//   win-x64 面（MASM 语法·含无 ptr 前缀与 32 位目的 movsx/movzx 形态·1157）
std::string eliminateRedundantSlotTrafficMasm(const std::string& functionText);

//   linux-arm64 面（AArch64 GAS·str/ldr 族·1157）
std::string eliminateRedundantSlotTrafficAArch64(const std::string& functionText);

// 就地入口（任务 1248·代码生成热路径）：命中返回 true 并原地替换文本；
//   未命中返回 false，不复制/不替换文本（仅锚点存在时构建扫描结构）。
//   string_view 生命周期契约（1217·001 §1.1a① 悬垂零容忍）：
//     · 调用前已取得的任何指向 functionText 的 view/引用/迭代器在命中后一律失效
//       （命中可能重分配缓冲；内核内部行/令牌视图不逃逸单次调用）；
//     · 未命中时文本内容与存储均不做任何修改（不复制、不重排）；
//     · 内核内部先销毁全部视图、后替换源缓冲（作用域顺序 + 调试断言机械锁）。
//   AArch64 入口两阶段（固定帧槽→大偏移间接寻址）顺序执行：第一阶段命中后的新
//   缓冲即第二阶段输入，调用方旧视图同样在两阶段结束后统一失效。
//   产物与值语义入口逐字节一致。
bool eliminateRedundantSlotTrafficInPlace(std::string& functionText);
bool eliminateRedundantSlotTrafficMasmInPlace(std::string& functionText);
bool eliminateRedundantSlotTrafficAArch64InPlace(std::string& functionText);

} // namespace peephole
} // namespace cn_compiler
