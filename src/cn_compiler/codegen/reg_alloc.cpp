// 线性扫描寄存器分配器实现（阶段C Task 4.3，规格书8.3）
// 设计要点：
//   1. 活跃区间计算：
//      - 指令序数：函数内基本块按顺序线性编号（块间跳转不影响序数单调性）
//      - def-use：每条指令的结果寄存器（def）在其序数处定义；操作数（use）在其序数处使用
//      - 跨块活跃：块级活跃变量迭代求解（in[B] = use[B] ∪ (out[B] - def[B])，
//        out[B] = ∪ succ in[S]），将块级活跃传播到指令序数（块入口处 in 集活跃到块出口）
//      - 保守起见：块入口处活跃的寄存器，其区间 end 延伸到该块的出口序数；
//        跨块场景（use 在后续块）由活跃传播保证 out 覆盖
//   2. 线性扫描：区间按 start 升序，活跃集合按 end 升序（小顶堆）；
//      区间开始弹出已结束的活跃区间（end < 当前 start），空闲寄存器回收；
//      分配最早空闲的物理寄存器；无空闲寄存器时溢出（spillSlot 递增）
//   3. 保守策略：仅整型/指针且非浮点/非 i128 的虚拟寄存器参与分配
//      （1267 续：i1 纳入 LinuxX64 可分配面，见 isAllocableIntType）
//   4. 溢出策略：spillSlot 从 0 递增（语义：每个溢出值一个独立溢出槽，
//      由 codegen 按需映射到栈偏移；不与其他栈槽区冲突）
//   5. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#include "cn_compiler/codegen/reg_alloc.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <unordered_set>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {
namespace regalloc {

namespace {

// #1212 分步开关（提交粒度：a/b/c 三步各自可回退）：
//   kI128AllocEnabled  = 步骤 b（x64 i128 双 GPR 成对分配）
//   kArm64FloatEnabled = 步骤 c（arm64 v8~v15 浮点寄存器）
// 关闭时对应区间不进入可分配面（保持原栈槽映射，产物与基线逐字节一致）。
constexpr bool kI128AllocEnabled = true;
constexpr bool kArm64FloatEnabled = true;

// IR 类型是否参与寄存器分配（1281：64/32 位整型、指针与窄整型；三架构同面——
//   1256 的架构门控已随 1159 合入退场）。1267 续：i1 布尔也进入本类型面——
//   寄存器内按 32 位规范承载（低 32 位 0/1、高 32 位清零），由 allocate() 的
//   i1 架构过滤限定在 LinuxX64（win-x64/arm64 的 i1 落位面未审计=后续任务，
//   显式过滤保持其产物逐字节不变）。#1212：浮点 f32/f64 进入独立浮点寄存器堆
//   （isFloatAllocableType）；i128 双半成对分配见步骤 b。
bool isAllocableIntType(const std::string& type) {
    return type == "i64" || type == "u64" || type == "ptr" ||
           type == "i32" || type == "u32" || type == "i1" ||
           type == "i8" || type == "u8" || type == "i16" || type == "u16";
}

// #1212：浮点类型（独立寄存器类）——f32/f64 进 SSE/V 寄存器堆
bool isFloatAllocableType(const std::string& type) {
    return type == "f32" || type == "f64";
}

// #1212 步骤 b：128 位整型（i128/u128）——%vN（高半）区间成对占用两个 GPR；
//   %vN+1（低半）不是独立虚拟寄存器（无 def/use），由成对分配统一承载。
bool isI128AllocableType(const std::string& type) {
    return type == "i128" || type == "u128";
}

// 64 位整型/指针（所有架构的基础可分配面）
bool isWideIntType(const std::string& type) {
    return type == "i64" || type == "u64" || type == "ptr";
}

// 1281：三架构可分配面统一（1256 架构门控退场）。1159 已合入且三后端
//   32/窄整型直写与宽度判据已由 1281 全族审计收口（compare/算术/移位/
//   除法/截断/参数搬运），win-x64 / arm64 与 linux-x86_64 同等参与分配。
//   类型面唯一权威=isAllocableIntType（见上）。
//   1267 续：i1 是唯一例外——仅 LinuxX64 参与（allocate() 显式按架构过滤，
//   理由见 isAllocableIntType 注释）。

// 活跃性参与面：活跃集只需含可分配寄存器（非可分配类型不参与传播——1256
//   性能：v2 全树窄/32 位 id 数量使活跃集膨胀，过滤后编译期回到基线量级）。
//   #1212：浮点始终参与（独立寄存器堆，与整数活跃集共享 id 空间但区间分离）。
bool livenessAllowsType(const std::string& type, bool allowNarrow) {
    return isFloatAllocableType(type) ||
           (kI128AllocEnabled && isI128AllocableType(type)) ||
           isWideIntType(type) || (allowNarrow && isAllocableIntType(type));
}


// 终止指令使用的寄存器ID（条件跳转的条件寄存器/返回寄存器）
std::vector<int> termUsedRegIds(const ir::IRBlock& block) {
    std::vector<int> ids;
    if (!block.terminated) return ids;
    int id = 0;
    if (block.termKind == "条件跳转") {
        // 280-a T12 字段化：条件值显式存 termCondition（不再寄生于块尾指令）
        if (ir::parseVirtualRegIdText(block.termCondition, id)) ids.push_back(id);
    } else if (block.termKind == "返回") {
        if (ir::parseVirtualRegIdText(block.termReturnValue, id)) ids.push_back(id);
    }
    return ids;
}

// 指令的结果寄存器ID（无结果 -1）
int instResultId(const ir::IRInstruction& inst) {
    return inst.result.id;
}

} // namespace

// 静态：判断 IR 类型是否可参与寄存器分配（i64/u64/ptr/i32/u32/i1/i8/u8/i16/u16
//   + f32/f64）。1281 起三架构类型面统一；1267 续：i1 由 allocate() 按 TargetArch
//   再过滤（仅 LinuxX64 分配，其余架构保持槽映射）。#1212：浮点独立寄存器类。
bool LinearScanAllocator::isAllocableType(const std::string& type) {
    return isAllocableIntType(type) || isFloatAllocableType(type) ||
           (kI128AllocEnabled && isI128AllocableType(type));
}

// #1212：浮点可分配类型（f32/f64）——独立寄存器类判据
bool LinearScanAllocator::isAllocableFloatType(const std::string& type) {
    return isFloatAllocableType(type);
}

// 构造函数：按平台配置被调用者保存寄存器集合，扣除保留寄存器
LinearScanAllocator::LinearScanAllocator(TargetArch arch,
                                         const std::vector<std::string>& reservedRegs)
    : arch_(arch) {
    // 被调用者保存寄存器（规格书8.3）
    //   x64：rbx/rbp/r12~r15（rbp 为帧指针，分配器不用；rbx/r12~r15 可用，
    //        r12 由调用方在隐藏返回指针场景保留）
    //   arm64：x19~x28（x29 帧指针、x30 链接寄存器不在集合内）
    std::vector<std::string> base;
    if (arch == TargetArch::X64 || arch == TargetArch::LinuxX64) {
        base = {"rbx", "r12", "r13", "r14", "r15"};
    } else {
        base = {"x19", "x20", "x21", "x22", "x23",
                "x24", "x25", "x26", "x27", "x28"};
    }
    // 扣除保留寄存器（如隐藏返回指针场景的 r12/x19）
    std::set<std::string> reserved(reservedRegs.begin(), reservedRegs.end());
    for (const auto& reg : base) {
        if (reserved.count(reg) == 0) {
            regs_.push_back(reg);
        }
    }
    // #1212：浮点寄存器堆（独立寄存器类）。
    //   win-x64：xmm6~xmm15 为 MS ABI 非易失寄存器（低 128 位被调用者保存），
    //     序言/尾声按函数实际使用集合 movups 保存/恢复；xmm0~xmm5 为调用者
    //     保存（发射器临时与参数位使用），不分配。
    //   linux-x64/arm64：浮点分配后续步骤接入（此处空池=全走原栈槽，产物不变）。
    if (arch == TargetArch::X64) {
        for (int i = 6; i <= 15; ++i) {
            fpRegs_.push_back("xmm" + std::to_string(i));
        }
    }
    // arm64：v8~v15 低 64 位为 AAPCS64 非易失（d8~d15 视图）；f32 用 sN、
    //   f64 用 dN 文本（同一物理寄存器的两种宽度视图——占用按 vN 基准名计）。
    if (arch == TargetArch::Arm64 && kArm64FloatEnabled) {
        for (int i = 8; i <= 15; ++i) {
            fpRegs_.push_back("v" + std::to_string(i));
        }
    }
}

// 静态：活跃区间计算（def-use 链 + 跨块活跃传播）
// 返回按 start 升序排序的活跃区间列表
std::vector<LiveInterval> LinearScanAllocator::computeLiveIntervals(
    const ir::IRFunction& function) {
    // 公共 API：全可分配类型面（单元测试/调试口径）
    return computeLiveIntervalsInternal(function, true,
                                        ir::maxVirtualRegId(function));
}

// 1256：活跃性内部流水线（allowNarrow=架构是否支持 32/窄整型分配）。
//   性能要点：活跃集只含可分配类型（v2 全树窄/32 位 id 膨胀 → 编译期回基线）。
std::vector<LiveInterval> LinearScanAllocator::computeLiveIntervalsInternal(
    const ir::IRFunction& function, bool allowNarrow, int maxRegId) {
    // 190-a：本函数 179 行按「四步流水线」提取为 4 个静态族子方法（纯搬运
    //   零行为变更——多重集核验先行于构建）。
    // ---- 第1步：函数级线性化指令序数 ----
    const auto blockRanges = computeBlockRanges(function);
    // ---- 第2步：块级 def/use 集合（跨块活跃传播用） ----
    std::vector<std::unordered_set<int>> blockDef(function.blocks.size());
    std::vector<std::unordered_set<int>> blockUse(function.blocks.size());
    computeBlockDefUse(function, blockRanges, blockDef, blockUse, allowNarrow, maxRegId);
    // ---- 第3步：跨块活跃传播（迭代数据流，in 集合） ----
    const auto in = computeLivenessIn(function, blockDef, blockUse);
    // ---- 第4步：生成活跃区间（def/use 精确序数 + 跨块活跃扩展） ----
    auto intervalMap = buildLiveIntervalMap(function, blockRanges, in, blockDef,
                                            allowNarrow, maxRegId);
    // 结束点修正：仅 def 无 use 的寄存器 end < start 时，end = start（单点活跃）
    std::vector<LiveInterval> intervals;
    intervals.reserve(intervalMap.size());
    for (auto& kv : intervalMap) {
        if (kv.second.end < kv.second.start) {
            kv.second.end = kv.second.start;
        }
        intervals.push_back(kv.second);
    }
    // 按 start 升序排序（线性扫描要求）
    std::sort(intervals.begin(), intervals.end(),
              [](const LiveInterval& a, const LiveInterval& b) {
                  if (a.start != b.start) return a.start < b.start;
                  return a.regId < b.regId;
              });
    return intervals;
}

// ==================== 190-a 流水线族子方法（原 computeLiveIntervals 112~287 段） ====================

// 族①：块序线性化——块序 -> (起始序数, 块内指令数)。每条指令一个序数；
//   终止指令附在块末。块间跳转不分配序数（活跃传播用块级集合，与线性序数解耦）。
std::vector<std::pair<int, int>> LinearScanAllocator::computeBlockRanges(
    const ir::IRFunction& function) {
    // ---- 第1步：函数级线性化指令序数 ----
    // 块序 -> (块起始序数, 块内指令数)。每条指令一个序数；终止指令附在块末。
    // 块间跳转不分配序数（活跃传播用块级集合，与线性序数解耦）。
    std::vector<std::pair<int, int>> blockRanges;  // (起始序数, 指令数)
    int cursor = 0;
    for (const auto& block : function.blocks) {
        blockRanges.emplace_back(cursor, static_cast<int>(block->instructions.size()));
        cursor += static_cast<int>(block->instructions.size());
    }
    return blockRanges;
}

// 族②：块级 def/use 集合（原 123~148 段）——def[B]：块内定义的寄存器（结果）；
//   use[B]：块内使用（操作数 + 终止）。同时收集每块指令的 def/use（供区间 end 精确化）。
void LinearScanAllocator::computeBlockDefUse(
    const ir::IRFunction& function,
    const std::vector<std::pair<int, int>>& blockRanges,
    std::vector<std::unordered_set<int>>& blockDef,
    std::vector<std::unordered_set<int>>& blockUse, bool allowNarrow,
    int maxRegId) {
    // ---- 第2步：块级 def/use 集合（跨块活跃传播用） ----
    // def[B]：块内定义的寄存器（结果）；use[B]：块内使用（操作数 + 终止）
    // 同时收集每块指令的 def/use（供区间 end 精确化）
    const int maxId = maxRegId + 1;
    for (std::size_t b = 0; b < function.blocks.size(); ++b) {
        const auto& block = function.blocks[b];
        const int base = blockRanges[b].first;
        for (std::size_t i = 0; i < block->instructions.size(); ++i) {
            const auto& inst = block->instructions[i];
            const int point = base + static_cast<int>(i);
            const int rid = instResultId(inst);
            if (rid >= 0 && rid < maxId &&
                livenessAllowsType(inst.result.type, allowNarrow)) {
                blockDef[b].insert(rid);
                (void)point;
            }
            // 操作数直插（set 天然去重；免 usedRegIds 临时向量）
            for (const auto& op : inst.operands) {
                if (op.id >= 0 && op.id < maxId && !op.isConstant &&
                    livenessAllowsType(op.type, allowNarrow)) {
                    blockUse[b].insert(op.id);
                }
            }
        }
        // 终止指令的使用（条件/返回）
        for (int tid : termUsedRegIds(*block)) {
            blockUse[b].insert(tid);
        }
    }
}

// 族③：跨块活跃传播（原 150~203 段）——块索引->label 映射构建前驱/后继，
//   迭代求解 in[B] = use[B] ∪ (out[B] - def[B])；out[B] = ∪ succ in[S]。返回 in 集合。
std::vector<std::unordered_set<int>> LinearScanAllocator::computeLivenessIn(
    const ir::IRFunction& function,
    const std::vector<std::unordered_set<int>>& blockDef,
    const std::vector<std::unordered_set<int>>& blockUse) {
    // ---- 第3步：跨块活跃传播（迭代数据流） ----
    // 块索引 -> label 映射（构造前驱/后继）
    const std::size_t n = function.blocks.size();
    std::unordered_map<std::string, int> labelIndex;
    labelIndex.reserve(n * 2);
    for (std::size_t b = 0; b < n; ++b) {
        labelIndex[function.blocks[b]->label] = static_cast<int>(b);
    }
    std::vector<std::vector<int>> succ(n);
    std::vector<std::vector<int>> pred(n);
    for (std::size_t b = 0; b < n; ++b) {
        const auto& block = function.blocks[b];
        if (!block->terminated) continue;
        auto addEdge = [&](const std::string& target) {
            auto it = labelIndex.find(target);
            if (it == labelIndex.end()) return;  // 目标块不存在（防御）
            succ[b].push_back(it->second);
            pred[static_cast<std::size_t>(it->second)].push_back(static_cast<int>(b));
        };
        if (block->termKind == "跳转") {
            addEdge(block->termTarget);
        } else if (block->termKind == "条件跳转") {
            addEdge(block->termTrueTarget);
            addEdge(block->termFalseTarget);
        }
    }

    // 迭代求解：in[B] = use[B] ∪ (out[B] - def[B])；out[B] = ∪ succ in[S]。
    // 1256 性能：①worklist（只有 in 变化的块才回推前驱——巨型 CFG 不做全表扫描）；
    //   ②out 不跨轮保存（out 变化必伴随某 in 变化——只比较 in 即等价）；
    //   ③每块复用 scratch 集合并保留容量（swap 保留旧容量），消除每轮构造/析构。
    std::vector<std::unordered_set<int>> in(n);
    std::vector<std::unordered_set<int>> scratch(n);
    std::vector<char> queued(n, 1);
    std::vector<int> worklist;
    worklist.reserve(n);
    for (std::size_t b = 0; b < n; ++b) worklist.push_back(static_cast<int>(b));
    while (!worklist.empty()) {
        const std::size_t b = static_cast<std::size_t>(worklist.back());
        worklist.pop_back();
        queued[b] = 0;
        auto& newIn = scratch[b];
        newIn.clear();
        if (newIn.bucket_count() < blockUse[b].size()) {
            newIn.reserve(blockUse[b].size() + 8);
        }
        // in[B] = use[B] ∪ (out[B] - def[B])；out[B] = ∪ succ in[S]
        for (int s : succ[b]) {
            for (int r : in[static_cast<std::size_t>(s)]) {
                if (blockDef[b].count(r) == 0) newIn.insert(r);
            }
        }
        for (int r : blockUse[b]) {
            newIn.insert(r);
        }
        if (newIn != in[b]) {
            in[b].swap(newIn);
            for (int p : pred[b]) {
                if (!queued[static_cast<std::size_t>(p)]) {
                    queued[static_cast<std::size_t>(p)] = 1;
                    worklist.push_back(p);
                }
            }
        }
    }
    return in;
}

// 族④：活跃区间生成（原 205~255 段）——寄存器 -> 区间（start=首次 def 或 use，
//   end=最后一次 use；def 处 start）。Alloca 结果排除（栈上分配无实际值）。
std::unordered_map<int, LiveInterval> LinearScanAllocator::buildLiveIntervalMap(
    const ir::IRFunction& function,
    const std::vector<std::pair<int, int>>& blockRanges,
    const std::vector<std::unordered_set<int>>& in,
    const std::vector<std::unordered_set<int>>& blockDef, bool allowNarrow,
    int maxRegId) {
    const std::size_t n = function.blocks.size();
    const int maxId = maxRegId + 1;
    // ---- 第4步：生成活跃区间（def/use 精确序数 + 跨块活跃扩展） ----
    // 寄存器 -> 区间（start=首次 def 或 use，end=最后一次 use；def 处 start）
    std::unordered_map<int, LiveInterval> intervalMap;
    // 第一遍：登记所有可分配寄存器的 def/use 序数
    for (std::size_t b = 0; b < function.blocks.size(); ++b) {
        const auto& block = function.blocks[b];
        const int base = blockRanges[b].first;
        for (std::size_t i = 0; i < block->instructions.size(); ++i) {
            const auto& inst = block->instructions[i];
            const int point = base + static_cast<int>(i);
            // def（首次创建时 start=point；后续 def 更新最早 def 点，并同样
            //   延长 end——1267 云预验 09-D2 根治：φ 降级在前驱块尾重复定义
            //   φ 结果寄存器（多 def 是合法 IR 形态），若像原实现只对 use 延长
            //   end，末次使用之后的再定义点落在区间外 → 寄存器被提前回收给
            //   另一区间，并行拷贝的源值被目标覆写（p++/索引++ 后 p 变成索引值，
            //   第二次迭代解引用野指针 SIGSEGV）。def 点必须占住本寄存器。
            // Alloca 结果排除：栈上分配无实际值，且结果寄存器活到函数尾
            //   会无意义占满物理寄存器（真实 IR 中 Alloca 结果被后续 Load 引用）
            if (inst.opcode == ir::Opcode::Alloca) continue;
            if (inst.result.id >= 0 && inst.result.id < maxId &&
                livenessAllowsType(inst.result.type, allowNarrow)) {
                auto it = intervalMap.find(inst.result.id);
                if (it == intervalMap.end()) {
                    LiveInterval li;
                    li.regId = inst.result.id;
                    li.start = point;
                    li.end = point;
                    li.type = inst.result.type;   // 1256：架构可分配面过滤用
                    li.isFloat = isFloatAllocableType(inst.result.type);  // #1212
                    li.is128 = kI128AllocEnabled &&
                               isI128AllocableType(inst.result.type);  // #1212 步骤 b
                    intervalMap[inst.result.id] = li;
                } else {
                    it->second.start = std::min(it->second.start, point);
                    it->second.end = std::max(it->second.end, point);
                }
            }
            // use（含 isConstant 检查：常量无寄存器）
            for (const auto& op : inst.operands) {
                if (op.id >= 0 && op.id < maxId && !op.isConstant &&
                    livenessAllowsType(op.type, allowNarrow)) {
                    auto it = intervalMap.find(op.id);
                if (it == intervalMap.end()) {
                    LiveInterval li;
                    li.regId = op.id;
                    li.start = point;
                    li.end = point;
                    li.type = op.type;            // 1256：架构可分配面过滤用
                    li.isFloat = isFloatAllocableType(op.type);  // #1212
                    li.is128 = kI128AllocEnabled &&
                               isI128AllocableType(op.type);  // #1212 步骤 b
                    intervalMap[op.id] = li;
                } else {
                    it->second.end = std::max(it->second.end, point);
                }
                }
            }
        }
        // 终止指令 use（条件/返回）
        for (int tid : termUsedRegIds(*block)) {
            auto it = intervalMap.find(tid);
            if (it != intervalMap.end()) {
                it->second.end = std::max(it->second.end, base +
                    static_cast<int>(block->instructions.size()) - 1);
            }
        }
    }
    // 第二遍：跨块活跃扩展——块入口 in 集合中、且在该块内未被 def 的寄存器，
    //   其 end 延伸到该块出口序数（保证跨块使用被覆盖）。
    //   ★183-a 复验根治（win 合并重建 E2E 115/235/245 三败定位）：190-a 拆分时
    //   本段被组装脚本错位到 return 之后=死代码（GCC 不报 C4702、MSVC /WX 拦截
    //   才暴露；「v2p md5 不变」系 v2 树恰无跨块活跃形态=假绿）——跨块活跃扩展
    //   静默丢失→寄存器被提前回收重分配→运行行为改变。本轮移回 return 之前
    //   （完成 190-a 本意的纯搬运落位）；以「190-a 前基线 E2E 全绿」为等价锚。
    for (std::size_t b = 0; b < n; ++b) {
        if (in[b].empty()) continue;
        const int blockStart = blockRanges[b].first;
        const int blockEnd = blockStart + blockRanges[b].second - 1;
        for (int r : in[b]) {
            auto it = intervalMap.find(r);
            if (it == intervalMap.end()) continue;
            // 仅在块内无 def 时扩展 end（有 def 时 end 由块内 use 决定，
            //   跨块 out 传播到后继块的 in 已在下一块扩展）
            if (blockDef[b].count(r) == 0) {
                it->second.end = std::max(it->second.end, blockEnd);
            }
        }
    }
    return intervalMap;
}

// 线性扫描主循环：按 start 升序扫描，活跃集合按 end 小顶堆
//  - #1212：整数/浮点按寄存器类拆成两条独立扫描——资源池、活跃集合与溢出
//    通道各自独立（浮点区间不挤占 GPR，反之亦然）。
void LinearScanAllocator::scan(const std::vector<LiveInterval>& intervals,
                               RegAssignmentMap& out) {
    std::vector<LiveInterval> intIntervals;
    std::vector<LiveInterval> fpIntervals;
    intIntervals.reserve(intervals.size());
    fpIntervals.reserve(intervals.size());
    for (const auto& li : intervals) {
        // 三目选桶（无 else 业务块——检查点纪律）
        std::vector<LiveInterval>& bucket = li.isFloat ? fpIntervals : intIntervals;
        bucket.push_back(li);
    }
    scanPool(intIntervals, regs_, RegClass::Int, out);
    scanPool(fpIntervals, fpRegs_, RegClass::Float, out);
}

// 单一寄存器堆的线性扫描（#1212：原 scan 主体，新增 class/pool 参数）
//  - 活跃集合：仍在使用的区间（end >= 当前 start 或尚未结束）
//  - 分配策略：区间开始时，先弹出已结束区间（end < start）回收寄存器；
//    再检查是否有空闲物理寄存器（未被任何活跃区间占用），有则分配最早空闲的；
//    无则溢出（spillSlot 递增，发射端按原栈槽映射处理）
void LinearScanAllocator::scanPool(const std::vector<LiveInterval>& intervals,
                                   const std::vector<std::string>& pool,
                                   RegClass regClass, RegAssignmentMap& out) {
    // 活跃区间集合：按 end 升序（小顶堆，存区间索引）
    // 注册表：regId -> 分配的物理寄存器列表（普通值 1 个；i128 成对 2 个）
    std::unordered_map<int, std::vector<std::string>> active;
    std::unordered_map<std::string, int> regOwner;     // 物理寄存器 -> regId（占用者）
    std::set<std::string> freeRegs(pool.begin(), pool.end());  // 空闲寄存器
    std::vector<std::pair<int, int>> activeEnds;       // (end, regId)，按 end 升序维护
    int nextSpillSlot = 0;

    // 活跃集合弹出辅助：弹出所有 end < 当前 start 的区间
    auto expire = [&](int point) {
        // 收集需回收的寄存器
        std::vector<int> expired;
        for (auto it = activeEnds.begin(); it != activeEnds.end();) {
            if (it->first < point) {
                expired.push_back(it->second);
                it = activeEnds.erase(it);
            } else {
                ++it;
            }
        }
        for (int rid : expired) {
            auto ait = active.find(rid);
            if (ait == active.end()) continue;
            for (const std::string& reg : ait->second) {
                freeRegs.insert(reg);
                regOwner.erase(reg);
            }
            active.erase(ait);
        }
    };

    for (const auto& li : intervals) {
        const int point = li.start;
        expire(point);
        // 已分配？跳过（理论上线性扫描每个寄存器一次）
        if (out.count(li.regId) != 0) continue;
        // 尝试分配空闲寄存器（#1212 步骤 b：i128 成对占两个 GPR，禁单半捷径）
        const int needCount = li.is128 ? 2 : 1;
        if (static_cast<int>(freeRegs.size()) >= needCount) {
            std::vector<std::string> taken;
            taken.reserve(needCount);
            for (int k = 0; k < needCount; ++k) {
                const std::string reg = *freeRegs.begin();
                freeRegs.erase(freeRegs.begin());
                regOwner[reg] = li.regId;
                taken.push_back(reg);
            }
            active[li.regId] = taken;
            RegAssignment ra;
            ra.assignedReg = taken[0];                       // i128：高半（%vN）
            if (needCount == 2) ra.assignedReg2 = taken[1];  // i128：低半（%vN+1）
            ra.regClass = regClass;
            // arm64 浮点：vN 基准名 -> sN/dN 宽度视图（同物理寄存器；
            //   占用/回收始终按 vN 基准名，宽度名仅供发射端操作数）
            if constexpr (kArm64FloatEnabled) {
                if (regClass == RegClass::Float &&
                    arch_ == TargetArch::Arm64 &&
                    ra.assignedReg.size() >= 2 && ra.assignedReg.size() <= 3 &&
                    ra.assignedReg[0] == 'v') {
                    const std::string num = ra.assignedReg.substr(1);
                    ra.assignedReg = (li.type == "f32" ? "s" : "d") + num;
                }
            }
            out[li.regId] = ra;
        } else {
            // 溢出：分配溢出槽（不占用物理寄存器；i128 双半同时溢出）
            RegAssignment ra;
            ra.spillSlot = nextSpillSlot++;
            ra.regClass = regClass;
            out[li.regId] = ra;
        }
        // 登记活跃（end 处）
        activeEnds.emplace_back(li.end, li.regId);
        // 保持 activeEnds 按 end 升序（线性扫描区间 start 单调，end 无序需排序）
        std::sort(activeEnds.begin(), activeEnds.end(),
                  [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                      return a.first < b.first;
                  });
        (void)regOwner;
    }
}

// 主入口：对函数做寄存器分配
RegAssignmentMap LinearScanAllocator::allocate(const ir::IRFunction& function,
                                               int maxRegId) {
    // 1268：最大虚拟寄存器编号单一事实源=ir::maxVirtualRegId（原 computeMaxRegId
    //   七处重复扫描已收敛删除）。
    const int maxId = (maxRegId >= 0) ? maxRegId : ir::maxVirtualRegId(function);
    // 1281：全架构活跃性参与面=可分配整型/指针全集（浮点/i128 不参与；
    //   1267 续：i1 进入可分配类型面，实际分配面由下方按架构过滤）
    intervals_ = computeLiveIntervalsInternal(function, true, maxId);
    // 1267 续：i1 布尔仅 LinuxX64 分配（该后端 i1 结果落位/条件值读取已全接
    //   storeCanonicalResult/physRegOf 并经自举闭环验证；win-x64/arm64 的 i1
    //   落位面未审计=后续任务）。非 LinuxX64 显式剔除 i1 区间——保留原栈槽
    //   映射，产物逐字节不变。
    if (arch_ != TargetArch::LinuxX64) {
        intervals_.erase(
            std::remove_if(intervals_.begin(), intervals_.end(),
                           [](const LiveInterval& li) { return li.type == "i1"; }),
            intervals_.end());
    }
    RegAssignmentMap out;
    scan(intervals_, out);
    return out;
}

} // namespace regalloc
} // namespace cn_compiler
