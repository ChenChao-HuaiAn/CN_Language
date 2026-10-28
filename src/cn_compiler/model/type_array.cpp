// 类型数组工具（#1459 轮拆分 自 type_system.cpp 纯机械搬移·函数体逐字保留）：
//   arrayBaseElemOf / arrayTotalElemCount——多维数组基元素剥取与总元素数
//   求积（二维结果/可选数组释放/布局面消费）。先拆再加纪律（type_system.cpp
//   新增后破千行冻结线·242 冻结线拆分先例同款）。
#include <limits>
#include <string>

#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
namespace types {

// #1459：数组**基元素**——循环剥全部维度至非数组文本（整32[2][3] -> 整32、
//   结果<字符串,整32>[2][3] -> 结果<字符串,整32>；一维与 arrayElemOf 等价）。
//   释放/布局面（emitStrArrayElemFreesFor/registerOwnedArrayElems）的
//   「元素是否携资源」「逐元素尺寸」判定落在基元素上——带维度尾缀的中间
//   文本使 isResultType 前缀判定携带 [N] 尾巴、ownedStrFieldsOf 收集 miss
//   （二维结果数组资源载荷漏释放·双侧同病实证）。剥层数上限 8：超深=形态
//   异常返回空串（消费方按空串分流·显式否定，不猜不静默）。
std::string arrayBaseElemOf(const std::string& type) {
    std::string cur = type;
    for (int depth = 0; depth < 8; ++depth) {
        if (!isArray(cur)) {
            return cur;
        }
        cur = arrayElemOf(cur);
    }
    return std::string();  // 超 8 维=形态异常：空串显式否定
}

// #1459：数组**总元素数**——各维度长度乘积（整32[2][3] -> 6；一维=长度
//   自身，与 arrayLenOf 等价）。任一维非纯数字/超承载=无值（消费方显式
//   分流，miss 不静默）；乘积超 long long 承载=无值（防溢出静默回绕）。
cn::Optional<long long> arrayTotalElemCount(const std::string& type) {
    long long total = 1;
    int dims = 0;
    std::string cur = type;
    for (int depth = 0; depth < 8; ++depth) {
        if (!isArray(cur)) {
            break;
        }
        const cn::Optional<int> len = arrayLenOf(cur);
        if (!len.has_value()) {
            return std::nullopt;
        }
        // 乘前防溢出预检（除法判界）：乘积将超 long long 正域=显式否定，
        //   零长度维合法（乘积 0=空数组·def-init 面已有零长兜 1 槽先例）
        const long long v = static_cast<long long>(len.value());
        if (v != 0 && total > ((std::numeric_limits<long long>::max)()) / v) {
            return std::nullopt;  // 乘积将溢出（界外不可承载）=显式否定
        }
        total *= v;
        dims++;
        cur = arrayElemOf(cur);
    }
    if (dims == 0) {
        return std::nullopt;  // 非数组形态：无值
    }
    return total;
}

} // namespace types
} // namespace cn_compiler
