// 1963 执法四（件2）立·2075 改造（用户 2026-11-05 裁决「查询不可判定=显式报
//   错中止·禁宁漏勿崩」）：拼接产物定义点溯源族声明（#1963 拆分件·类内 .inc
//   包含·本头冻结线核算后——ir_generator.hpp 1000 硬线先例 ir_string_accum_
//   decl.inc）。完整判据注释见 ir.cpp 实现段：emit 发射单点**全定义登记**
//   （键=regCounter_ 全局唯一·值=定义符号 extra·函数入口清空防无界）→ 三元
//   槽 Store 统计（纯拼接槽才按拼接语义登记·混合槽=分支敏感借用句柄禁盲登
//   记）→ 查询单点 表判定=单一事实源·查无=内部错误显形中止（声明/赋值/入容
//   器/比较/语句 6 消费面统一）。
    // 寄存器定义点符号表（键=regCounter_ 全局唯一·入口清空·查询单一事实源·细节见 ir.cpp）。
    std::unordered_map<int, std::string> regDefSymbols_;
    // 1963 件2：三元槽 Store 统计（总数/拼接源数·相等=纯拼接槽·细节见 ir.cpp）。
    std::unordered_map<std::string, int> ternarySlotStoreTotal_;
    std::unordered_map<std::string, int> ternarySlotStoreConcat_;
    // 拼接产物查询单点（表判定·查无=内部错误中止·2075）。
    bool isConcatResultTraced(int regId, const SourceLocation& loc);
