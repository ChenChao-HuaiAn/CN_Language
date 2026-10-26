// env_query.hpp —— 跨平台诊断环境变量读取（#1443 诊断通道共用原语）。
// 归纳来源：opt/pass_manager.cpp 与 opt/ssa.cpp 各持一份同构 static（1443 引入）
//   + driver 侧 CN_LINK_MAP 直用 _dupenv_s 致 linux 构建红（云端预验 c1c49daa40
//   实证：_dupenv_s 为 MSVC 专有·g++ 无声明）——三处收口到此（common 中立层·
//   opt/driver 依赖方向均合法·334）。
#pragma once

#include <cstdlib>
#include <string>

namespace cn::env_query {

// 读取环境变量原值：已设置=true 并写 valueOut；未设置/读取失败=false。
// 否定回答非错误——诊断开关语义=未设置即不启用（调用方显式分流默认行为，
// 默认行为零变化）。
inline bool readValue(const char* name, std::string& valueOut) {
#ifdef _WIN32
    char* raw = nullptr;
    size_t len = 0;
    // 静默豁免运行时API: 环境未设置/读取失败=诊断开关不启用（否定回答·默认全量）
    if (_dupenv_s(&raw, &len, name) != 0 || raw == nullptr) return false;
    valueOut.assign(raw);
    std::free(raw);
    return true;
#else
    const char* raw = std::getenv(name);
    // 静默豁免运行时API: 环境未设置=诊断开关不启用（否定回答·默认全量）
    if (raw == nullptr) return false;
    valueOut.assign(raw);
    return true;
#endif
}

}  // namespace cn::env_query
