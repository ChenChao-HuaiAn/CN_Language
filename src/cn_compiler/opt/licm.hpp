// CN语言优化器：循环不变量外提（LICM）Pass（阶段B Task 4.2，规格书9.2）
// 设计要点：
//   1. 自然循环识别：复用 opt/cfg.hpp DomTree::findNaturalLoops()
//      （back edge + 循环体块集合）
//   2. 不变量判定：循环体内指令，其操作数全部为"常量 或 循环外定义的
//      寄存器 或 循环外写入的变量"-> 不变量
//      - 寄存器不变量：该寄存器在该循环体内无定义（无 result.id == 它）
//      - 变量槽不变量：该变量在该循环体内无 Store（extra==它）
//   3. 外提目标：循环 header 的前驱（非循环体内的）——即 preheader；
//      无 preheader 时跳过该循环（保守，不改变 CFG 结构）
//   4. 只外提"纯运算指令"（isPureArith 白名单），副作用指令
//      （Load/Store/Call 等）绝不外提（可能改变执行次数语义）
//   5. 安全：外提后指令移到 preheader 末尾，保持操作数可用性
//      （preheader 位于循环外，操作数若为循环内定义则不会判为不变量）
//   6. 幂等：重复运行不重复外提（外提后指令不再在循环体内）
// 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 循环不变量外提 Pass（规格书9.2：将循环内不变量移到循环前）
class LICMPass : public Pass {
public:
    // 遍历模块全部函数：识别自然循环，将不变量指令外提到 preheader
    // 返回: 是否发生了至少一次外提
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "licm"; }

private:
    // ---- 1207 跨轮缓存：CFG 快照 + 循环分析（fixpoint 轮间复用） ----
    //   DomTree 的输入只有块序/标签/终止边（指令不参与）——逐块四字段快照
    //   相同即可证明 CFG 未变，支配树/循环集合/preheader 逐位可复用。
    struct CfgBlock {
        std::string label;      // 块标签（索引顺序有意义）
        std::string termKind;   // 有效终止形态（未终止块=空=无出边）
        std::string target0;    // 跳转目标 / 条件跳转真目标
        std::string target1;    // 条件跳转假目标
    };
    struct LoopCache {
        std::vector<CfgBlock> cfg;          // 上次分析时的 CFG 快照
        std::vector<DomTree::Loop> loops;   // 自然循环集合（空=无循环）
        std::vector<int> preheaders;        // 与 loops 同下标（-1=无 preheader）
    };
    std::unordered_map<std::string, LoopCache> loopCache_;
    const ir::IRModule* lastModule_ = nullptr;  // 缓存所属模块（换模块即清空）

    // 单函数 LICM：返回是否外提（非静态——持有跨轮缓存）
    bool runFunction(ir::IRFunction& fn);

    // CFG 快照采集/比较（采集与比较共用 cfgFieldsOf 单一读取口径）
    static void captureCfg(const ir::IRFunction& fn, std::vector<CfgBlock>& out);
    static bool sameCfg(const ir::IRFunction& fn, const std::vector<CfgBlock>& cfg);

    // 循环存在性快筛（1207）：返回 true = 已证明 CFG 无环（全部边严格
    //   前向，按块生成序号全序）=> 必然无自然循环，调用方可跳过 DomTree
    //   构建与整个 LICM。返回 false 仅表示"未证明无环"（含标签不可解析），
    //   仍走完整支配树判定——宽进严出，不改变任何优化语义。
    static bool hasNoCycle(const ir::IRFunction& fn);

    // 指令是否不变量（体内定义集合 + 不变寄存器集合 + 体内写入槽集合）
    //   操作数寄存器：体内定义且未判不变 -> 非不变量；循环外定义 -> 不变
    static bool isInvariant(
        const ir::IRInstruction& inst,
        const std::unordered_set<int>& bodyDefRegs,
        const std::unordered_set<int>& invariantRegs,
        const std::unordered_set<std::string>& bodyWrittenSlots);

    // 查找循环体内定义的寄存器集合（result.id）与写入的槽集合（Store extra）
    static void collectBodyDefs(
        const ir::IRFunction& fn,
        const std::vector<int>& bodyIndexes,
        std::unordered_set<int>& defRegs,
        std::unordered_set<std::string>& writtenSlots);
};

} // namespace opt
} // namespace cn_compiler
