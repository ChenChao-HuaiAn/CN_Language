// 全局字符串驻留表（Task #1154 阶段A·common 中立层）
// 职责：内容 → 稳定 ID 的单射；ID → 文本的稳定只读引用。
//   设计目标（对照 v2 词法/词法分析.cn 驻留() 同构）：
//     1. 同内容恒同 ID —— 消费方可用整数比较替代字符串比较；
//     2. 内容只存一份 —— ID 化字段的拷贝成本 = 一个 32 位整数；
//     3. ID → 文本永不失效 —— std::deque 只在尾部追加、不做删除/搬移，
//        std::deque 尾部插入不使既有元素引用失效（C++ 标准保证）。
//   线程安全：intern/find/text/size 全部内部互斥（std::mutex）。本设施当前
//     只在单线程相位（语义/IR）与并行 codegen 的只读消费点使用——互斥量在
//     单线程相位零竞争，成本可忽略；显式加锁是为了让并行 codegen 线程
//     读 ID→文本时也不依赖调用方约定。
//   ID 约定：0 = 空串（固定存在）；kInvalidId = 未驻留/非法（仅 find 返回值）。
//   miss 处置：text() 收到非法 ID 属调用方契约破坏（编译器内部错误）——
//     打印原因并 std::abort（miss 不可静默纪律：不返回空串假装成功）。
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace cn_compiler {
namespace intern {

// 驻留 ID 类型（32 位：单次编译的驻留文本量远小于 40 亿）
using InternId = std::uint32_t;

// 空串的固定 ID（intern("") 恒返回本值）
constexpr InternId kEmptyId = 0;

// find() 的未命中哨兵（显式哨兵 + 消费点强制检查分流）
constexpr InternId kInvalidId = 0xFFFFFFFFu;

// 驻留文本并返回稳定 ID：同内容恒同 ID（空串恒 kEmptyId）
InternId intern(std::string_view text);

// 仅查询不插入：命中返回 ID；未驻留（含空串以外的未登记文本）返回 kInvalidId
InternId find(std::string_view text);

// ID → 文本（稳定引用，永不失效）；kInvalidId/越界=调用方契约破坏 → 中止
const std::string& text(InternId id);

// 已驻留条目数（含空串；调试/单测用）
std::size_t size();

} // namespace intern
} // namespace cn_compiler
