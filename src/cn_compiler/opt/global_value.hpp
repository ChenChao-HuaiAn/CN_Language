// CN语言优化器：全局值传播 Pass（Task 完善C 优化器增强）
// 设计要点（规格书9.2：全局常量传播增强）：
//   1. 安全子集设计（非 SSA IR，以安全性为第一）：
//      函数内"线性扫描"常量 Store->Load 转发——
//      沿函数基本块顺序遍历（blocks 列表顺序近似源码顺序），
//      维护 唯一内部名 -> 常量 表：
//      - Store 常量到槽（extra=变量名, operand[0]=常量）-> 记录
//      - Load 同槽且中间无其他写入 -> 替换结果引用为常量
//      - 任何可能写入的指令（StorePtr/Call/...）或"Store 寄存器值"-> 清空
//   2. 保守要点：
//      - 只传播"常量"（不传播寄存器值——寄存器替换由复写传播负责）
//      - 槽被 Store 非常量值后清空（无法确定值）
//      - 多槽变量（结构体/数组）不传播
//      - 跨块不做真正数据流（无前驱/多前驱交集分析），
//        仅沿线性序保守转发；复杂全局数据流留 SSA 时代
//   3. 结果替换复用 opt_common 的 replaceUses（常量白名单过滤）
#pragma once

#include <string>
#include <unordered_map>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 全局值传播 Pass（规格书9.2：常量 Store->Load 转发安全子集）
class GlobalValuePass : public Pass {
public:
    // 遍历模块全部函数：线性扫描常量 Store->Load 转发
    // 返回: 是否发生了至少一次转发
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "global-value"; }

private:
    // 是否可能写入任意内存的指令（清空常量表）
    static bool mayAliasWrite(ir::Opcode op);

    // 是否多槽变量（结构体/数组，保守跳过）
    static bool isMultiSlot(const std::string& uniqueName,
                            const std::unordered_map<std::string, int>& varSlots);
};

} // namespace opt
} // namespace cn_compiler
