// 三后端 .data 静态数组初始化列表共享打包（2051）：
//   IR 层 globalStaticArrayInits（常量整数列表·行主序扁平十进制文本）→
//   槽内存 8 字节对齐 quad 值序列——元素按 typeSizeOf(元素类型) 宽度小端
//   逐字节打包、缺格补零（1473 甲案零位默认态）；win MASM dq / linux·arm64
//   GAS .quad 发射端再次小端写出=字节序保持。发射行宽口径：每行 ≤32 值
//   折行（#2050 数据段长链分批同款·ml64 A2042 单行过长防线）。
//   归属：目标机发射→codegen 后端共享面（依赖 common+model 合法）。
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"

namespace cn_compiler {
namespace codegen {

// 元素文本序列 → 槽 quad 值序列（小端打包）。
//   elemTexts=十进制规范整数文本（IR 层折叠+cnParseInt 域内判定已收口）；
//   elemBytes=元素字节宽（typeSizeOf(元素类型)·1/2/4/8）；slotBytes=槽总
//   字节（8 的倍数·三后端 #259 数组分配口径）。返回无值=不支持 .data 直存
//   （浮点/字符串文本/越界形态——调用方保持零初始化+入口注入通道；IR 层已
//   保证直存面=整数文本，无值到达=内部缺陷显形由调用方报内部错误）。
cn::Optional<std::vector<unsigned long long>> packStaticArrayInitQuads(
    const std::vector<std::string>& elemTexts, std::size_t elemBytes,
    std::size_t slotBytes);

}  // namespace codegen
}  // namespace cn_compiler
