// 发射后窥孔·第二遍：死槽写消除 + 近邻槽转发（任务 1320）
// 单一职责：在第一遍（相邻 store→load 转发·peephole_slot.cpp / _arm64.cpp 共用
//   内核）之后，处理两类别残留——两遍都通过 SlotRules 注入方言解析/改写，
//   本文件不含任何具体语法面知识。
//   ① 死槽写（DSE）：store 的值从未被读——
//      · 槽 token 全函数仅此一处（含注释提及一律计数，沿用第一遍保守口径）；
//      · 或同一槽的连续两次 store 之间无读、后写位宽覆盖前写，且两写之间
//        仍是直线段（无标签/分支——否则跳转可绕过覆盖直接读到前写）。
//   ② 近邻槽转发（多跳中继 A→B→…）：槽在前述死写清理后恰好剩 store+load
//      两处引用，但两行不相邻（实测产物中间多为 call 前 `sub rsp` 与参数
//      装载）——行距 <= kForwardScanLimit、中间无控制流/标签、store 源寄存器
//      未被改写（writesReg 含 8/16 位部分写与隐式写）、无槽字节别名访问时：
//      删 store 行、load 行改写为寄存器搬运（同寄存器同宽按既有判据全删）。
// 安全判据（缺一不可·不满足即原样保留；宁漏勿错）：
//   a. 单槽出现次数 <= 4（既有「恰好两次」保守口径的扩展）。>=5 次不参与：
//      小窗口内无法证明无第三处读；注释/未识别行提及槽一律按「未知引用」拒绝。
//   b. 槽字节范围别名：全函数内任何其它槽访问与候选槽 [off, off+bytes) 相交
//      即拒绝（16 字节整宽访存按 16 计、取址 lea 按地址本身 1 字节计）。
//      产物族槽偏移恒 8 字节对齐，故实际等价于「同偏移不同 token」与
//      「movups 跨槽」两类；判据仍按字节区间实现，不依赖对齐假设。
//   c. 近邻转发窗口内：方言 classifyReg=Control（标签/分支/调用/双写）拒绝；
//      writesReg(store.reg64) 拒绝（值可能被改写）；行内出现其它槽访问且
//      字节范围相交拒绝。
//   d. 浮点对沿用 1351 高半观察证明（decidePairAction 单点）。
//   e. 纯文本变换：不依赖后端状态；未命中返回 false 且文本逐字节不变。
// 视图生命周期（1217）：lines/槽键全部指向 functionText 当次缓冲；渲染域结束
//   （视图销毁）后才替换源存储，域尾 Debug 断言核对缓冲身份未提前改变。
//   槽键所有权补充（#1874）：不是每个槽 token 都来自 functionText——取帧地址
//   钩子拼出的 token 是解析现场构造的临时串，视图入表后随局部变量销毁即悬垂。
//   故表对这类键自持有（findOrAddOwned·deque 稳定地址），不把「调用方保证键
//   存活」的隐式契约留给未来的调用点。
#include "cn_compiler/codegen/peephole_slot_internal.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace cn_compiler {
namespace peephole {
namespace detail {

namespace {

using namespace std::string_view_literals;  // 助记符比较免 strlen 构造


// 单槽引用跟踪上限（超出=overflow·不参与本遍：小窗口内无法证明无第三处读）
constexpr int kMaxTrackedOcc = 4;
// 计划缺省哨兵（pairStore/pairLoad 未登记）：显式哨兵+消费点检查（#1239）
constexpr int kNoPlan = -1;

// 单槽引用种类（Unknown=注释/未识别行提及——保守拒绝该槽）
enum class OccKind : unsigned char { Unknown = 0, Store = 1, Load = 2 };

// 单次引用：行号 + 种类 + 该行访问宽度（字节；0=未识别）
struct SlotOccurrence {
    int pos = 0;
    OccKind kind = OccKind::Unknown;
    int bytes = 0;
};

// 单槽引用表项（出现次数上限 4·超限=overflow 不参与）
struct SlotEntry {
    std::string_view slot;
    std::uint32_t id = 0;  // 稳定条目下标（别名表 owner 对比用）
    SlotOccurrence occ[4];
    int count = 0;
    bool overflow = false;
};

// 槽访问区间（别名判定用）：owner 为 SlotTraceTable 的稳定条目下标
struct AccessRange {
    long long offset = 0;
    int bytes = 0;
    std::uint32_t owner = 0;
};

// 槽引用表：稳定条目数组（下标=稳定 id）+ 开放寻址索引（线性探测·装载因子
//   1/2·string_view 键零拷贝）。条目数组只增不删——grow/扩容不影响既有 id，
//   别名表可安全保存 id（1217 生命周期：id 与槽键视图只在本次调用域内使用）。
//   键所有权契约：findOrAdd 的键视图必须覆盖表的使用期（现有调用点全部传
//   functionText 缓冲内视图）；键文本来自临时局部串时一律走 findOrAddOwned，
//   由表自持有——否则函数返回即 stack-use-after-return（#1874 实录：取帧地址
//   钩子的 addrSlot 入表后悬垂，读取点 parseSlotOffset 撞 ASan）。
class SlotTraceTable {
public:
    // 查找或新建槽条目，返回稳定 id（>=0；调用方用 at(id) 取条目）
    std::uint32_t findOrAdd(std::string_view slot) {
        if ((used_ + 1) * 2 > mask_ + 1) growIndex();
        const std::uint32_t h = hashSlot(slot);
        std::size_t idx = h & mask_;
        while (index_[idx].used) {
            if (index_[idx].hash == h && index_[idx].slot == slot) return index_[idx].id;
            idx = (idx + 1) & mask_;
        }
        const std::uint32_t id = static_cast<std::uint32_t>(entries_.size());
        SlotEntry entry;
        entry.slot = slot;
        entry.id = id;
        entries_.push_back(entry);
        index_[idx].used = true;
        index_[idx].slot = slot;
        index_[idx].hash = h;
        index_[idx].id = id;
        ++used_;
        return id;
    }

    // 同上，键文本移入表内自持有的池：存进条目/索引的视图指向池元素，
    //   生命周期与表相同（消费点可放心在 collectTrace 返回之后读取）。
    //   池用 deque 而非 vector：vector 扩容会移动元素（短串 SSO 缓冲随对象搬家），
    //   已存的全部视图会集体悬垂；deque push_back 不使既有元素地址失效。
    std::uint32_t findOrAddOwned(std::string token) {
        ownedSlots_.push_back(std::move(token));
        return findOrAdd(ownedSlots_.back());
    }

    SlotEntry& at(std::uint32_t id) { return entries_[id]; }
    const SlotEntry& at(std::uint32_t id) const { return entries_[id]; }
    std::size_t size() const { return entries_.size(); }

private:
    struct IndexEntry {
        std::string_view slot;
        std::uint32_t hash = 0;
        std::uint32_t id = 0;
        bool used = false;
    };
    static constexpr std::size_t kInitialCapacity = 64;

    static std::uint32_t hashSlot(std::string_view slot) {
        return static_cast<std::uint32_t>(std::hash<std::string_view>{}(slot));
    }

    void growIndex() {
        std::vector<IndexEntry> old = std::move(index_);
        const std::size_t newCapacity = (old.empty() ? kInitialCapacity : old.size() * 2);
        index_.assign(newCapacity, IndexEntry{});
        mask_ = newCapacity - 1;
        used_ = 0;
        for (const IndexEntry& e : old) {
            if (!e.used) continue;
            std::size_t idx = e.hash & mask_;
            while (index_[idx].used) idx = (idx + 1) & mask_;
            index_[idx] = e;
            ++used_;
        }
    }

    std::vector<SlotEntry> entries_;  // 稳定条目数组（下标即 id）
    std::vector<IndexEntry> index_;   // 开放寻址索引
    std::size_t mask_ = 0;
    std::size_t used_ = 0;
    std::deque<std::string> ownedSlots_;  // 自持有键文本（findOrAddOwned·地址稳定）
};

// 行内访问宽度（字节）：方言 store/load 解析优先；未识别形态按整宽/取址保守计。
//   movups/movaps/movdqu/movdqa（x86 128 位）按 16 字节；lea 取址按 16 字节
//   （见下）；其余未识别=16（保守按整宽，未知形态不得当作无访问）。
//   storeAcc/loadAcc 为调用方已完成的解析结果（parseStore/parseLoad 成功标志）。
int lineAccessBytes(std::string_view line, bool isStore, const SlotAccess& storeAcc,
                    bool isLoad, const SlotAccess& loadAcc) {
    if (isStore) return storeAcc.width > 0 ? (storeAcc.width + 7) / 8 : 0;
    if (isLoad) return loadAcc.width > 0 ? (loadAcc.width + 7) / 8 : 0;
    const std::string_view s = trimView(line);
    // 静默豁免业务值: 空行/纯注释=无内存访问（别名判定非查表默认值）
    if (s.empty()) return 0;
    // 全行注释（MASM ';' / GAS '//'）：不发射、不访问内存
    if (s.front() == ';' || s.compare(0, 2, "//") == 0) return 0;
    const std::size_t sp = s.find_first_of(" \t");
    const std::string_view mnem = (sp == std::string_view::npos) ? s : s.substr(0, sp);
    // lea = 取址：该地址指向的聚合可能被后续指针访问读写，按「直接槽可寻址的
    //   最大聚合宽度」16 字节计（1360 等价性铁证顺手记录：本产物族多槽变量中
    //   只有 i128 以 $s1 双槽直接寻址（16 字节）；数组/结构体字段一律经
    //   lea 基址 + 指针偏移访问，其元素槽不出现在 [rbp±N] token 中）。
    //   若把 lea 只按 1 字节计，i128 高半槽写会被误判死写（324 整128极值边界实录）。
    if (iequals(mnem, "lea"sv)) return 16;
    if (iequals(mnem, "movups"sv) || iequals(mnem, "movaps"sv) ||
        iequals(mnem, "movdqu"sv) || iequals(mnem, "movdqa"sv) ||
        iequals(mnem, "movdqu64"sv) || iequals(mnem, "movdqa64"sv)) {
        return 16;  // 128 位整宽访存
    }
    return 16;  // 未识别形态：保守按整宽（不得当作无访问）
}

// 全函数槽引用收集：每个 token 一条 SlotEntry + 一条 AccessRange。
//   每行只做一次 store/load 解析（第一遍口径），避免重复解析开销。
void collectTrace(const std::vector<std::string_view>& lines, std::string_view anchor,
                  const SlotRules& rules, SlotTraceTable& table,
                  std::vector<AccessRange>& ranges) {
    for (std::size_t li = 0; li < lines.size(); ++li) {
        const std::string_view ln = lines[li];
        // 取帧槽地址行先行识别（AArch64 `add|sub xN, x29, #N` 不含槽锚·x86 lea
        //   含锚走下方常规收集）：地址逃逸=该槽 16 字节聚合窗引用，必须计入，
        //   否则该槽仅存的 store 被「单现死写」剃除（833 arm64 i128 错值实录）。
        //   溢出/超限=该槽整槽不参与（与下方常规收集同款保守口径）。
        if (rules.parseFrameAddress) {
            std::string addrSlot;  // 解析现场构造的临时 token（非 functionText 视图）
            if (rules.parseFrameAddress(ln, addrSlot)) {
                // 键所有权交给表（#1874）：addrSlot 是本循环局部串，直接以视图入表
                //   会在 collectTrace 返回后悬垂，后续 parseSlotOffset/身份比较读到
                //   已销毁的 SSO 缓冲（GCC 15 ASan 默认 detect_stack_use_after_return
                //   机械捕获）。findOrAddOwned 移入池后所有读取都指向稳定元素。
                SlotEntry& ea = table.at(table.findOrAddOwned(std::move(addrSlot)));
                // 溢出/超限=该槽整槽不参与本遍（保守保留原样·与下方常规收集同款）
                if (ea.overflow || ea.count >= kMaxTrackedOcc) {
                    ea.overflow = true;
                }
                if (!ea.overflow) {
                    SlotOccurrence& occA = ea.occ[ea.count];
                    occA.pos = static_cast<int>(li);
                    occA.kind = OccKind::Unknown;
                    occA.bytes = 16;
                    ++ea.count;
                    long long addrOff = 0;
                    // 偏移解析失败=不入别名表（未知引用仍拦截该槽·保守口径同款）
                    if (rules.parseSlotOffset(ea.slot, addrOff)) {
                        ranges.push_back(AccessRange{addrOff, 16, ea.id});
                    }
                }
            }
        }
        if (ln.find(anchor) == std::string_view::npos) continue;
        SlotAccess storeAcc;
        SlotAccess loadAcc;
        const bool isStore = rules.parseStore(ln, storeAcc);
        // 静默豁免业务值: store 优先（同一行不会同时是 store 与 load 形态）
        const bool isLoad = !isStore && rules.parseLoad(ln, loadAcc);
        int bytes = lineAccessBytes(ln, isStore, storeAcc, isLoad, loadAcc);
        OccKind kind = isStore ? OccKind::Store : (isLoad ? OccKind::Load : OccKind::Unknown);
        // 一行多槽 token（本产物族不发射）：无法判定各 token 的读写角色，
        //   整行按未知引用处理（该批槽不参与本遍），宽度按整宽保守计。
        std::size_t multiPos = ln.find(anchor);
        if (multiPos != std::string_view::npos) {
            multiPos = ln.find(anchor, multiPos + anchor.size());
        }
        if (multiPos != std::string_view::npos) {
            kind = OccKind::Unknown;
            bytes = 16;
        }
        std::size_t pos = 0;
        while ((pos = ln.find(anchor, pos)) != std::string_view::npos) {
            const std::size_t rb = ln.find(']', pos);
            // 静默豁免业务值: 右界缺失=该锚点之后无完整槽 token（续扫后续锚点）
            if (rb == std::string_view::npos) {
                pos += anchor.size();
                continue;
            }
            const std::string_view slot = ln.substr(pos, rb - pos + 1);
            SlotEntry& e = table.at(table.findOrAdd(slot));
            if (e.overflow) {
                // 静默豁免业务值: 该槽已溢出登记上限=本遍不参与（保守保留原样）
                pos = rb + 1;
                continue;
            }
            if (e.count >= kMaxTrackedOcc) {
                e.overflow = true;  // 超出引用计数上限：整槽不参与本遍（保守）
                pos = rb + 1;
                continue;
            }
            SlotOccurrence& occ = e.occ[e.count];
            occ.pos = static_cast<int>(li);
            occ.kind = kind;
            occ.bytes = bytes;
            ++e.count;
            long long offset = 0;
            // 静默豁免业务值: 偏移解析失败=该访问不入别名表（未知引用仍会拦截该槽）
            if (rules.parseSlotOffset(slot, offset)) {
                ranges.push_back(AccessRange{offset, bytes, e.id});
            }
            pos = rb + 1;
        }
    }
}

// 区间相交：候选槽 [offset, offset+bytes) 与 ownerId 之外的任一访问相交即别名。
//   ranges 已按 offset 升序；单次访问宽度 <=16，故只需检查
//   [offset-16, offset+bytes) 窗口内的条目。
bool hasAliasConflict(const std::vector<AccessRange>& ranges, std::uint32_t ownerId,
                      long long offset, int bytes) {
    const long long lo = offset;
    const long long hi = offset + (bytes > 0 ? bytes : 1);
    const long long from = lo - 16;
    const auto it = std::lower_bound(
        ranges.begin(), ranges.end(), from,
        [](const AccessRange& r, long long v) { return r.offset < v; });
    for (auto cur = it; cur != ranges.end() && cur->offset < hi; ++cur) {
        if (cur->owner == ownerId) continue;
        const long long end = cur->offset + (cur->bytes > 0 ? cur->bytes : 1);
        if (cur->offset < hi && lo < end) return true;
    }
    return false;
}

// 单行槽访问宽度（别名判定用）：store/load 解析优先，其余按方言无关启发式
int lineBytesForAlias(std::string_view line, const SlotRules& rules) {
    SlotAccess acc;
    if (rules.parseStore(line, acc)) return acc.width > 0 ? (acc.width + 7) / 8 : 0;
    if (rules.parseLoad(line, acc)) return acc.width > 0 ? (acc.width + 7) / 8 : 0;
    return lineAccessBytes(line, false, acc, false, acc);
}

// 直线段判定：from 与 to 之间（不含端点）无控制流/标签、不写 srcReg、
//   无与候选槽字节范围相交的其它槽访问。已删除行（drop）不再发射，跳过。
//   srcReg 为空=不检查寄存器改写（死写覆盖判定不需要）。
bool straightPath(const std::vector<std::string_view>& lines, const std::vector<char>& drop,
                  const SlotRules& rules, int from, int to, const std::string& srcReg,
                  long long offset, int bytes, std::string_view ownerSlot) {
    const bool checkReg = !srcReg.empty();
    for (int k = from + 1; k < to; ++k) {
        const std::size_t uk = static_cast<std::size_t>(k);
        if (drop[uk]) continue;
        const std::string_view ln = lines[uk];
        // 标签/分支/调用/双写：方言分类器对任意寄存器都先判控制流
        if (rules.classifyReg(ln, srcReg).kind == RegEffect::Kind::Control) return false;
        if (checkReg && rules.writesReg(ln, srcReg)) return false;
        if (ln.find(rules.slotAnchor) == std::string_view::npos) continue;
        // 槽别名：本行出现与候选区间相交的其它槽访问即拒绝
        const int lineBytes = lineBytesForAlias(ln, rules);
        std::size_t pos = 0;
        while ((pos = ln.find(rules.slotAnchor, pos)) != std::string_view::npos) {
            const std::size_t rb = ln.find(']', pos);
            // 静默豁免业务值: 右界缺失=续扫后续锚点（本行该 token 不完整）
            if (rb == std::string_view::npos) {
                pos += rules.slotAnchor.size();
                continue;
            }
            long long other = 0;
            const std::string_view tok = ln.substr(pos, rb - pos + 1);
            // 同一槽的 token：窗口内本不应出现（引用计数已排除），防御性跳过
            if (tok == ownerSlot) {
                pos = rb + 1;
                continue;
            }
            if (rules.parseSlotOffset(tok, other)) {
                const long long end = other + (lineBytes > 0 ? lineBytes : 1);
                if (other < offset + bytes && offset < end) return false;
            }
            pos = rb + 1;
        }
    }
    return true;
}

// 单槽判定结果：待删除 store 行 + 可选转发对
struct SlotPlan {
    int deadPos[4] = {0, 0, 0, 0};
    int deadCount = 0;
    int pairStore = kNoPlan;
    int pairLoad = kNoPlan;
    bool pairDropLoad = false;
    std::string rewritten;
};

// 判定单槽：死写剔除 + 近邻转发（调用方已保证 count 在 [1,4] 且非 overflow）
void planSlot(const std::vector<std::string_view>& lines, const std::vector<char>& drop,
              const std::vector<std::string>& repl, const SlotRules& rules,
              const SlotEntry& e, const std::vector<AccessRange>& ranges, long long offset,
              const std::vector<SlotRange>& aggregates, SlotPlan& plan) {
    // 聚合保护（1320 云端回归根因）：lea 取址的聚合（结构体按值参数逐槽直接写 +
    //   基址 lea 传递，可达数百字节）自基址向上扩展；候选槽与之相交即不参与。
    //   区间由后端帧布局注入（varSlots 真实槽数=类型布局字节数）；未注入时
    //   仅靠文本侧 16 字节 lea 窗口（i128 双槽）兜底——调用方必须注入。
    const int candBytes = (e.occ[0].bytes > 0) ? e.occ[0].bytes : 1;
    for (const SlotRange& r : aggregates) {
        if (offset < r.offset + r.bytes && r.offset < offset + candBytes) return;
    }
    // ---- ① 死写判定 ----
    bool dead[4] = {false, false, false, false};
    for (int t = 0; t < e.count; ++t) {
        if (e.occ[t].kind != OccKind::Store) continue;
        const int storeBytes = (e.occ[t].bytes > 0) ? e.occ[t].bytes : 1;
        bool isDead = false;
        if (e.count == 1) {
            isDead = true;  // 全函数仅此一处：任何路径都读不到写入值
        }
        for (int u = t + 1; u < e.count && !isDead; ++u) {
            const SlotOccurrence& next = e.occ[u];
            if (next.kind != OccKind::Store) break;  // 读到/未知引用：本写存活
            const int nextBytes = (next.bytes > 0) ? next.bytes : 1;
            if (nextBytes < storeBytes) continue;  // 部分覆盖：剩余字节仍可被读
            // 覆盖判定只关心控制流与别名（源寄存器改写不影响「后写覆盖前写」）
            if (!straightPath(lines, drop, rules, e.occ[t].pos, next.pos, std::string(),
                              offset, storeBytes, e.slot)) {
                break;  // 两写之间非直线段：跳转可绕过覆盖，保守保留
            }
            isDead = true;
        }
        // 静默豁免业务值: 别名冲突=不判死（保守保留，非查表默认值）
        if (isDead && hasAliasConflict(ranges, e.id, offset, storeBytes)) isDead = false;
        if (isDead) {
            plan.deadPos[plan.deadCount++] = e.occ[t].pos;
            dead[t] = true;
        }
    }
    // ---- ② 存活引用两现 -> 近邻转发 ----
    int live[4];
    int liveCount = 0;
    for (int t = 0; t < e.count; ++t) {
        if (dead[t]) continue;
        live[liveCount++] = t;
    }
    if (liveCount != 2) return;
    const SlotOccurrence& first = e.occ[live[0]];
    const SlotOccurrence& second = e.occ[live[1]];
    // 静默豁免业务值: 存活两现非 store→load 形态=不转发（原样保留）
    if (first.kind != OccKind::Store || second.kind != OccKind::Load) return;
    if (second.pos <= first.pos) return;
    if (second.pos - first.pos > kForwardScanLimit) return;

    SlotAccess store;
    SlotAccess load;
    // 静默豁免业务值: 形态复解析失败=该槽不参与本遍（原样保留）
    if (!rules.parseStore(lines[static_cast<std::size_t>(first.pos)], store)) return;
    if (!rules.parseLoad(lines[static_cast<std::size_t>(second.pos)], load)) return;
    // 静默豁免业务值: 槽文本/族不一致=非本对形态（原样保留）
    if (store.slot != load.slot) return;
    const int storeBytes = (first.bytes > 0) ? first.bytes : 1;
    if (hasAliasConflict(ranges, e.id, offset, storeBytes)) return;
    if (!straightPath(lines, drop, rules, first.pos, second.pos, store.reg64, offset,
                      storeBytes, e.slot)) {
        return;
    }
    const PairAction act = decidePairAction(lines, drop, repl, first.pos, store, load, rules);
    // 静默豁免业务值: 决策未命中=该对原样保留（非查表默认值）
    if (!act.hit) return;
    plan.pairStore = first.pos;
    plan.pairLoad = second.pos;
    plan.pairDropLoad = act.dropLoad;
    plan.rewritten = act.rewrittenLoad;
}

} // namespace

// 方言无关第二遍（就地）：死槽写消除 + 近邻槽转发。
//   命中返回 true 并原地替换文本；未命中返回 false（不复制、不改文本）。
bool eliminateForwardAndDeadStoresInPlace(std::string& functionText, const SlotRules& rules) {
    static const std::vector<SlotRange> kNoAggregates;
    return eliminateForwardAndDeadStoresInPlace(functionText, rules, kNoAggregates);
}

bool eliminateForwardAndDeadStoresInPlace(std::string& functionText, const SlotRules& rules,
                                          const std::vector<SlotRange>& aggregates) {
    if (functionText.find(rules.slotAnchor) == std::string::npos) return false;
#ifndef NDEBUG
    // 视图存活期源存储身份快照：替换点核对缓冲未被提前改写（release 零开销）
    const char* const sourceData = functionText.data();
    const std::size_t sourceSize = functionText.size();
#endif
    bool any = false;
    std::string rendered;  // 域内产出；域外替换（不含任何指向源文本的视图）
    {
        // ---- 视图生命周期域：以下所有视图只在域内使用，域尾统一销毁 ----
        const std::vector<std::string_view> lines = splitLines(functionText);
        std::vector<char> drop(lines.size(), 0);
        std::vector<std::string> repl(lines.size());
        SlotTraceTable table;
        std::vector<AccessRange> ranges;
        collectTrace(lines, rules.slotAnchor, rules, table, ranges);
        std::sort(ranges.begin(), ranges.end(),
                  [](const AccessRange& a, const AccessRange& b) {
                      return a.offset < b.offset;
                  });
        // 先收集全部槽的判定计划（各槽行集互不相交），再统一应用
        std::vector<SlotPlan> plans;
        for (std::uint32_t id = 0; id < table.size(); ++id) {
            const SlotEntry& e = table.at(id);
            // 静默豁免业务值: 溢出/空槽=本遍不参与（第一遍口径不变）
            if (e.overflow || e.count == 0) continue;
            long long offset = 0;
            // 静默豁免业务值: 槽偏移解析失败=该槽不参与（保持原样）
            if (!rules.parseSlotOffset(e.slot, offset)) continue;
            SlotPlan plan;
            planSlot(lines, drop, repl, rules, e, ranges, offset, aggregates, plan);
            // 静默豁免业务值: 无任何动作=不登记计划（非查表默认值）
            if (plan.deadCount == 0 && plan.pairStore == kNoPlan) continue;
            plans.push_back(std::move(plan));
        }
        for (const SlotPlan& plan : plans) {
            for (int k = 0; k < plan.deadCount; ++k) {
                drop[static_cast<std::size_t>(plan.deadPos[k])] = 1;
                any = true;
            }
            if (plan.pairStore != kNoPlan) {
                drop[static_cast<std::size_t>(plan.pairStore)] = 1;
                if (plan.pairDropLoad) {
                    drop[static_cast<std::size_t>(plan.pairLoad)] = 1;
                }
                if (!plan.pairDropLoad) {
                    repl[static_cast<std::size_t>(plan.pairLoad)] = plan.rewritten;
                }
                any = true;
            }
        }
        if (any) rendered = renderLines(lines, drop, repl, functionText);
    }  // ---- 视图销毁点：此后不再持有任何指向 functionText 缓冲的视图 ----
#ifndef NDEBUG
    // 粗粒度身份检查：仅 data()/size() 不变（边界见 peephole_slot.hpp 说明）
    if (any) {
        assert(functionText.data() == sourceData && functionText.size() == sourceSize &&
               "窥孔视图存活期间源文本不得被改写（视图先销毁、后替换存储）");
    }
#endif
    if (any) functionText = std::move(rendered);  // 就地替换（视图已全部销毁）
    return any;
}

// 值语义包装（单测/通用调用）：复制一次后走就地内核
std::string eliminateForwardAndDeadStores(const std::string& functionText,
                                          const SlotRules& rules) {
    std::string out(functionText);
    eliminateForwardAndDeadStoresInPlace(out, rules);
    return out;
}

} // namespace detail
} // namespace peephole
} // namespace cn_compiler
