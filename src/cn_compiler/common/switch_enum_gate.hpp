// 枚举 switch 穷举执法·存量清偿豁免包裹原语（#2074·2026-11-05 用户令
//   「default 退役+全 case 显式+旗标强制」执法面收口）。
// 职责：为挂载 -Werror=switch-enum（GCC/Clang）与 /we4061（MSVC）后的存量
//   「漏 case 靠 default 挡」位置提供**双编译器同步豁免**的成对宏——包裹段内
//   枚举穷举检查关闭，段外全量强制（新增枚举值未显式 case=编译失败）。
//   001 §十四：对枚举类型的 switch，每一枚举值必须有以其名字显式书写的
//   case 分支；default 唯一合法形态=报内部错误并中止编译。
// 使用约束（三层）：
//   ① 仅限基线登记的存量位置——scripts/switch_enum_baseline.json 逐处登记
//     （文件/锚文本/登记理由），基线只减不增；逐处显式补 case 清偿后删包裹，
//     门禁对账（check_dispatch_gate.py 面⑤）保证宏与基线一一对应；
//   ② 每处包裹必须配中文注释「// 存量清偿挂#2074：<一句话原因>」——
//     裸堆 pragma=违规（任务纪律）；
//   ③ 新增 switch(enum) 禁用本宏——新位置缺 case 直接编译红（执法本意）。
// 平台形态：MSVC 走 warning(push)/disable 4061·4062（4061=有 default 时缺
//   显式 case；4062=无 default 时缺 case——包裹段语义=穷举检查整体关闭）；
//   其余（GCC/Clang）走 GCC diagnostic push/ignored "-Wswitch-enum"。
//   _Pragma 运算符形态（C++17）——宏内多 pragma 的标准写法。
#ifndef CN_COMPILER_COMMON_SWITCH_ENUM_GATE_HPP
#define CN_COMPILER_COMMON_SWITCH_ENUM_GATE_HPP

#if defined(_MSC_VER)
#define CN_SWITCH_ENUM_EXEMPT_BEGIN \
    _Pragma("warning(push)") _Pragma("warning(disable : 4061 4062)")
#define CN_SWITCH_ENUM_EXEMPT_END _Pragma("warning(pop)")
#else
#define CN_SWITCH_ENUM_EXEMPT_BEGIN \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"")
#define CN_SWITCH_ENUM_EXEMPT_END _Pragma("GCC diagnostic pop")
#endif

#endif  // CN_COMPILER_COMMON_SWITCH_ENUM_GATE_HPP
