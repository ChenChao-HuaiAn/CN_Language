// cn::Optional<T>——结果/可选类型化基建（#1303 一期·common 中立层）
// 设计动机（用户 2026-10-21 三连定音）：
//   1. 882 三处置不能 100% 杜绝兜底错误——「显式哨兵+消费点人工检查」面依赖
//      人工纪律；哨兵值与合法值域重叠时漏检即静默错值（110 实录：FFI 返回
//      类型查表 miss 静默默认整32→三平台错值；同族曾致常量 0/8 字节过写）。
//   2. 本类型把「检查」从人工纪律升为类型强制：消费点不写 has_value() 分流
//      或 value_or(默认) 就拿不到值——无 operator*/operator->，裸解引用在
//      编译期即被拒绝（漏检查=编译错，不是运行时事故）。
//   3. Rust 对齐（用户令）：性能第一（零成本——存储复用 std::optional·
//      全内联委托·错误分支不可达零成本）；安全第二（坏值响亮终止——对齐
//      Rust unwrap 的 panic 语义·禁 UB）。
// 接口纪律（本类型的全部意义，禁扩面）：
//   只提供四条显形通道：构造 / has_value() / value() / value_or(默认)。
//   value_or 的默认值表达式在调用点强制写出（默认值显形，禁止藏）。
//   类级 [[nodiscard]]：返回本类型的函数调用若丢弃结果即编译告警。
#pragma once

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <type_traits>
#include <utility>

namespace cn_compiler {
namespace cn {

template <typename T>
class [[nodiscard]] Optional {
public:
    // 构造族：默认=空；nullopt=空；T=有值（隐式——供 `return 值;` /
    //   `Optional<std::string> o = "文本";` 自然书写）
    Optional() = default;
    Optional(std::nullopt_t) {}
    // 转发构造：U 直接构造 T 于 storage 内（一次转换）——容许 `return "文本";`
    //   （const char*→string 单跳完成·双用户转换是 GCC 拒/MSVC 放的双编译差异面·
    //   1360 实录）与任意可构造 T 的形态；std::optional 自身 SFINAE 约束防误吞。
    template <typename U, typename = std::enable_if_t<
        std::is_constructible_v<std::optional<T>, U&&> &&
        !std::is_same_v<std::decay_t<U>, std::nullopt_t>>>
    Optional(U&& v) : storage_(std::forward<U>(v)) {}

    // 显形通道①：是否有值——消费点分流的第一判据
    bool has_value() const { return storage_.has_value(); }

    // 显形通道②：取值（只读）。坏值=打印原因并 std::abort 响亮终止——
    //   编译器内部检查缺失即缺陷显形（fail-loud·对齐 Rust unwrap panic
    //   语义·禁 UB；留痕先例=string_intern::text 非法 ID 中止）。
    const T& value() const {
        if (!storage_.has_value()) {
            std::fprintf(stderr,
                         "内部错误: cn::Optional 坏值访问（消费点检查缺失——"
                         "应先 has_value() 分流或用 value_or(默认)）\n");
            std::abort();
        }
        return *storage_;
    }

    // 显形通道③：带显形默认取值——默认值表达式在调用点必须写出
    //   （模板转发：容许 const char* 字面量等可构造形态）
    template <typename U>
    T value_or(U&& default_value) const {
        return storage_.value_or(std::forward<U>(default_value));
    }

private:
    std::optional<T> storage_;  // 零成本存储：全部通道内联委托
};

} // namespace cn
} // namespace cn_compiler
