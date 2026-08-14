// 数学运行时 API（Task 6.3，规格书10.5 数学库）
// 对应 CN 层模块 stdlib/数学.cn 的函数：平方根/幂/正弦/余弦/正切/绝对值/向上取整/向下取整
// 命名约定：运行时 C++ 实现用英文 API 命名（用户要求），
//   CN 层中文函数名由编译器 IR 层映射到这些运行时符号（__cn_* 前缀，extern "C" 导出）。
// 实现：直接封装 C 标准库 <cmath>（sqrt/pow/sin/cos/tan/fabs/ceil/floor），
//   输入输出均为 double（浮64）；P1 的对数/反三角/随机数本次不做，留待后续。
// 单文件 <=1000 行、单函数 <=100 行约束。
#include <cmath>
#include <cstddef>

#include "runtime.hpp"

// 与 MSVC /W4 /WX 与 GCC -Wall -Wextra -Werror 兼容：
//   <cmath> 的 std::sqrt 等为重载函数，double 入参无歧义；
//   全部函数在 extern "C" 块内（runtime.hpp 已声明）。
extern "C" {

// 平方根：__cn_sqrt(浮64) -> 浮64（数学.平方根）
CNRT_EXPORT double __cn_sqrt(double value) {
    return std::sqrt(value);
}

// 幂：__cn_pow(浮64, 浮64) -> 浮64（数学.幂，底数^指数）
CNRT_EXPORT double __cn_pow(double base, double exponent) {
    return std::pow(base, exponent);
}

// 正弦：__cn_sin(浮64) -> 浮64（数学.正弦，弧度制）
CNRT_EXPORT double __cn_sin(double radians) {
    return std::sin(radians);
}

// 余弦：__cn_cos(浮64) -> 浮64（数学.余弦，弧度制）
CNRT_EXPORT double __cn_cos(double radians) {
    return std::cos(radians);
}

// 正切：__cn_tan(浮64) -> 浮64（数学.正切，弧度制）
CNRT_EXPORT double __cn_tan(double radians) {
    return std::tan(radians);
}

// 绝对值：__cn_fabs(浮64) -> 浮64（数学.绝对值）
CNRT_EXPORT double __cn_fabs(double value) {
    return std::fabs(value);
}

// 向上取整：__cn_ceil(浮64) -> 浮64（数学.向上取整）
CNRT_EXPORT double __cn_ceil(double value) {
    return std::ceil(value);
}

// 向下取整：__cn_floor(浮64) -> 浮64（数学.向下取整）
CNRT_EXPORT double __cn_floor(double value) {
    return std::floor(value);
}

} // extern "C"
