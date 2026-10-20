// CN语言优化器：块内公共子表达式消除 Pass 实现（Task 完善C 优化器增强）
// 实现要点：
//   1. 模式判定 = 操作码 + 结果类型 + 操作数身份（同寄存器/同常量文本/同变量名），
//      相同模式 = 相同输入 -> 结果可复用。
//      【1153 热点治理】旧实现为每条纯运算指令拼接 std::string 模式键
//      （opcode + 逐操作数前缀+类型+id/文本），每轮每条指令一次堆分配+字符串哈希
//      +键比较（perf：CSE run 3.45% + patternOf 1.02% + string 哈希/比较/分配）。
//      现改为 64 位结构哈希（opcode/类型/操作数身份直接混入，零键分配）+
//      哈希桶内结构等值比较；等值语义与旧字符串键逐字段一致。
//   2. 纯运算模式表跨副作用指令存活（无副作用，结果恒定）
//   3. Load 表独立维护：变量名 -> Load 结果寄存器；遇写内存清空
//   4. 替换复用 opt_common 的 replaceUses（寄存器替换无条件安全）
//   5. 哈希碰撞处理：桶内逐条 samePattern 结构比较（含常量文本归一化），
//      碰撞不影响正确性；代表指令按插入序首个（与旧 map 首插入一致）。
//   6. 块终止条件字段（283-a 字段化）同步替换（1206）：条件值不再寄生于
//      块尾指令操作数，replaceUses 覆盖不到；块内指令遍历结束后以完整
//      regRewrite 调用 replaceTermCondition，重复比较结果作分支条件时
//      替换不再丢失（代表指令在本块内先于终止点，支配关系不变）。
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cn_compiler/opt/cse.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

namespace {

// 64 位混合（splitmix64 终混），降低结构字段简单拼接的碰撞率
inline void mixU64(std::uint64_t& h, std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    value ^= value >> 31;
    h ^= value;
    h *= 0x100000001b3ULL;  // FNV-64 素数
}

// 字节串混入（类型名/变量名/常量文本）；按 8 字节块装载减少逐字节循环
inline void mixBytes(std::uint64_t& h, const char* data, std::size_t size) {
    std::size_t i = 0;
    for (; i + 8 <= size; i += 8) {
        std::uint64_t word = 0;
        std::memcpy(&word, data + i, 8);
        mixU64(h, word);
    }
    if (i < size) {
        std::uint64_t word = 0;
        std::memcpy(&word, data + i, size - i);
        mixU64(h, word);
    }
    mixU64(h, size);  // 长度参与：防 "ab"+"c" 与 "a"+"bc" 同哈希
}

// 常量文本归一化视图：十进制文本直接返回原视图（零分配）；0x/0b/0o 前缀
//   解析为十进制写入 scratch（与旧模式键 normalizeIntText 同语义）。
std::string_view normalizedConstView(const std::string& text, std::string& scratch) {
    if (hasRadixPrefix(text)) {
        scratch = normalizeIntText(text);
        return scratch;
    }
    return text;
}

// 模式结构哈希：与旧字符串键字段一一对应（opcode|结果类型|逐操作数身份）
//   操作数身份：常量=类型+归一化文本；寄存器=类型+id；变量=类型+名。
std::uint64_t patternHash(const ir::IRInstruction& inst) {
    std::uint64_t h = 0xcbf29ce484222325ULL;  // FNV-64 偏移基
    mixU64(h, static_cast<std::uint64_t>(inst.opcode));
    mixBytes(h, inst.type.data(), inst.type.size());
    mixU64(h, inst.operands.size());
    std::string scratch;  // 仅带进制前缀常量使用
    for (const auto& op : inst.operands) {
        const bool isRegister = !op.isConstant && op.id >= 0;
        if (op.isConstant) {
            mixU64(h, 1);
            mixBytes(h, op.type.data(), op.type.size());
            const std::string_view text = normalizedConstView(op.extra, scratch);
            mixBytes(h, text.data(), text.size());
            continue;
        }
        if (isRegister) {
            mixU64(h, 2);
            mixBytes(h, op.type.data(), op.type.size());
            mixU64(h, static_cast<std::uint64_t>(op.id));
            continue;
        }
        // 变量引用（按名）
        mixU64(h, 3);
        mixBytes(h, op.type.data(), op.type.size());
        mixBytes(h, op.extra.data(), op.extra.size());
    }
    return h;
}

// 结构等值：与旧字符串键等值逐字段一致（哈希桶内碰撞时使用）。
//   单出口累计失配（避免新增大量 if-return-false 形态）；字段与旧键一一对应。
bool samePattern(const ir::IRInstruction& a, const ir::IRInstruction& b) {
    bool same = a.opcode == b.opcode && a.type == b.type &&
                a.operands.size() == b.operands.size();
    for (std::size_t i = 0; same && i < a.operands.size(); ++i) {
        const ir::IRValue& x = a.operands[i];
        const ir::IRValue& y = b.operands[i];
        const bool xIsReg = !x.isConstant && x.id >= 0;
        const bool yIsReg = !y.isConstant && y.id >= 0;
        const bool kindMismatch =
            x.isConstant != y.isConstant || x.type != y.type || xIsReg != yIsReg;
        if (kindMismatch) {
            same = false;
            break;
        }
        if (x.isConstant) {
            // 常量比较归一化文本（0xFF 与 255 等价；与旧键 normalizeIntText 一致）
            std::string sx, sy;
            same = normalizedConstView(x.extra, sx) ==
                   normalizedConstView(y.extra, sy);
            continue;
        }
        if (xIsReg) {
            same = x.id == y.id;
            continue;
        }
        same = x.extra == y.extra;  // 变量引用按名
    }
    return same;
}

// 是否 CSE 模式候选（纯运算；常量指令与非纯/副作用指令不入表；
//   浮点仅 allowFloat 启用时入表）。
bool isPatternCandidate(const ir::IRInstruction& inst, bool allowFloat) {
    const bool isConstInst =
        inst.opcode == ir::Opcode::ConstInt ||
        inst.opcode == ir::Opcode::ConstFloat ||
        inst.opcode == ir::Opcode::ConstBool;
    return !isConstInst && isPureArith(inst.opcode) &&
           (allowFloat || !isFloatType(inst.type));
}

} // namespace

// 是否可能修改任意内存的指令（清空 Load 表）
bool CSEPass::mayWriteMemory(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::Store:
        case ir::Opcode::StorePtr:
        case ir::Opcode::Call:
        case ir::Opcode::CallIndirect:
        case ir::Opcode::LoadPtr:    // 指针加载可能触发空指针检查（副作用）
        case ir::Opcode::FieldAddr:  // 隐含空指针检查
        case ir::Opcode::AddrOf:     // 地址逃逸
            return true;
        default:
            return false;
    }
}

// 遍历模块全部函数的全部基本块，消除块内公共子表达式
bool CSEPass::run(ir::IRModule& module) {
    // 换模块=新一轮优化（去重状态仅用于同一模块的 fixpoint 轮）
    if (&module != lastModule_) {
        reportedDuplicates_.clear();
        lastModule_ = &module;
    }
    bool changed = false;
    for (auto& fn : module.functions) {
        for (auto& block : fn.blocks) {
            auto& insts = block->instructions;
            // 纯运算模式表：结构哈希 -> 首个代表指令的块内下标
            //   （unordered_multimap：一次遍历建表；哈希碰撞由 samePattern 消歧）
            std::unordered_multimap<std::uint64_t, int> patterns;
            patterns.reserve(32);
            // Load 表：变量名（唯一内部名）-> Load 结果寄存器 id
            std::unordered_map<std::string, int> loadResults;
            // 寄存器替换表：重复指令结果 -> 第一个结果
            RegRewriteMap regRewrite;
            for (std::size_t i = 0; i < insts.size(); ++i) {
                ir::IRInstruction& inst = insts[i];
                // 第一步：应用已收集的替换（本指令引用点）
                if (replaceUses(inst, regRewrite, ConstRewriteMap())) changed = true;
                // 第二步：按指令类别维护表
                const bool isPattern = isPatternCandidate(inst, allowFloat_);
                if (isPattern) {
                    const std::uint64_t hash = patternHash(inst);
                    // 桶内逐条结构等值比较（哈希碰撞消歧）
                    const auto range = patterns.equal_range(hash);
                    int repIndex = -1;
                    for (auto it = range.first; it != range.second; ++it) {
                        const int candidate = it->second;
                        if (samePattern(
                                insts[static_cast<std::size_t>(candidate)], inst)) {
                            repIndex = candidate;
                            break;
                        }
                    }
                    const bool hasResult = inst.result.id >= 0;
                    const bool hit = repIndex != -1;
                    if (hit && hasResult) {
                        // 命中：登记替换（代表指令的结果寄存器）。
                        //   changed 仅在首次发现该重复时报告——登记本身不改 IR
                        //   （实际引用替换由 replaceUses 报告）；重复 Load/暂存
                        //   重复指令会在后续 fixpoint 轮被再次检出，若每轮都报
                        //   changed 则永不收敛（1153 根因）。见 hpp 成员说明。
                        regRewrite[inst.result.id] = insts[static_cast<std::size_t>(
                            repIndex)].result.id;
                        const bool firstReport =
                            reportedDuplicates_[block.get()].insert(
                                inst.result.id).second;
                        if (firstReport) changed = true;
                    }
                    const bool firstSeen = !hit;
                    if (firstSeen && hasResult) {
                        // 未命中：登记首条代表指令（仅需结果寄存器非空）
                        patterns.emplace(hash, static_cast<int>(i));
                    }
                } else if (inst.opcode == ir::Opcode::Load) {
                    // Load：operand[0]=变量名（唯一内部名），result=寄存器
                    if (inst.operands.empty() || inst.result.id < 0) continue;
                    const std::string& name = inst.operands[0].extra;
                    if (name.empty() || inst.operands[0].id >= 0) continue;
                    auto it = loadResults.find(name);
                    if (it != loadResults.end()) {
                        // 同槽 Load 命中（两者间无写内存）-> 复用。
                        //   changed 仅在首次发现该重复 Load 时报告（Load 有副作用
                        //   永不删除，后续轮次会重复检出同一登记——见 hpp 成员说明）。
                        regRewrite[inst.result.id] = it->second;
                        const bool firstReport =
                            reportedDuplicates_[block.get()].insert(
                                inst.result.id).second;
                        if (firstReport) changed = true;
                    } else {
                        loadResults[name] = inst.result.id;
                    }
                } else if (mayWriteMemory(inst.opcode)) {
                    // 写内存指令：清空 Load 表（内存可能被修改，保守）
                    loadResults.clear();
                }
                // 块末返回字段同步替换（1254·与条件字段同族，283-a 字段化）：
                //   重复表达式结果作返回值时 regRewrite 须落到 termReturnValue，
                //   否则返回仍引用幸存重复指令、代表指令反被 DCE 删除。
                //   置于最后一条指令处理之后（此时本块 regRewrite 已完整；仅
                //   最后一条指令自身的登记也已完成）——与 1206 的块后条件字段
                //   挂点分离，避免两任务同锚点集成冲突。
                if (i + 1 == insts.size() &&
                    replaceTermReturnValue(*block, fn.returnType, regRewrite,
                                           ConstRewriteMap())) {
                    changed = true;
                }
            }
            // 第三步：块终止条件字段（283-a 字段化）同步替换（1206）。
            //   循环结束后 regRewrite 才是本块完整替换集合（1153 删除的旧
            //   调用点在循环前，必为空）；条件值的定义（代表指令）在本块内
            //   先于重复指令与终止点，寄存器替换无条件安全。
            if (replaceTermCondition(*block, regRewrite, ConstRewriteMap())) {
                changed = true;
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
