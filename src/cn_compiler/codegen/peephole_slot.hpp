// 发射后窥孔·冗余槽读写消除（任务 1144·波2）：
//   消除相邻「store 到 vreg 槽 → 立即 load 同槽」的纯转发搬运——
//   槽在本对之后无任何后续访问（槽死）时：删 store 行，load 行改写为
//   从 store 源寄存器取值的寄存器搬运；同寄存器同宽度时两行全删。
// 设计要点：
//   1. 纯文本变换（行级、函数文本内），无后端状态依赖——单测可直接喂文本；
//   2. 仅识别本后端发射族实际产出的行形态（Linux x86_64 GAS Intel 语法）：
//        store：mov qword ptr [rbp-N], r10 / mov dword ptr [rbp-N], r10d /
//               mov byte ptr [rbp-N], r10b / movsd [rbp-N], xmm0（浮点无 ptr）
//        load ：mov r10, qword ptr [rbp-N] / mov r10d, dword ptr [rbp-N] /
//               movsx r10, byte ptr [rbp-N] / movzx r10, word ptr [rbp-N] /
//               movsd xmm0, [rbp-N]
//   3. 安全条件（缺一不可）：
//      a. 两行物理相邻（中间无标签/指令——注释与空行不跨，直接要求相邻）；
//      b. store 行的前一行不是标签（否则存在跳过 store 直达 load 的路径，
//         改写后该路径读不到 store 源寄存器的值）；
//      c. 槽 token 在整个函数文本中恰好出现两次（=本对两行；任何形式：读/写/
//         取址/注释提及一律计数，保守拒绝）。不能用「本对之后无出现」——循环
//         回边可自后文跳回前文再次读槽（行号=代码位置，非动态执行序）；
//      d. 改写 load 时读宽度 ≤ 存宽度（存窄读宽会读入槽内残留高位，寄存器
//         搬运无法复现该残留 -> 拒绝）。
//   4. 门控：调用方按优化档位开关（-O2 起），低档位产物逐字节不变。
// 规范：英文API命名，中文仅注释；函数<=100行
#pragma once
#include <string>

namespace cn_compiler {
namespace peephole {

// 对单函数汇编文本做冗余槽读写消除，返回处理后文本（无命中时原样返回）。
std::string eliminateRedundantSlotTraffic(const std::string& functionText);

} // namespace peephole
} // namespace cn_compiler
