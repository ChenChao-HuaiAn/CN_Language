// 1963 执法四（件2）：拼接产物定义点溯源族声明（#1963 拆分件·类内 .inc 包含·
//   本头冻结线核算后——ir_generator.hpp 1000 硬线先例 ir_string_accum_decl.inc）。
//   完整判据注释见 ir.cpp 实现段：emit 发射单点登记（键=regCounter_ 全局唯一·
//   函数入口清空防无界）→ 三元槽 Store 统计（纯拼接槽才登记·混合槽=分支敏感
//   借用句柄禁盲登记）→ 查询单点 表优先→块内倒序兜底→查无=保守不 free+显形
//   登记（声明/赋值/入容器/比较/语句 6 消费面统一）。
    // 1963 件2：拼接产物溯源表（键=regCounter_ 全局唯一·入口清空·细节见 ir.cpp）。
    std::unordered_map<int, std::string> concatTempDefs_;
    // 1963 件2：三元槽 Store 统计（总数/拼接源数·相等=纯拼接槽·细节见 ir.cpp）。
    std::unordered_map<std::string, int> ternarySlotStoreTotal_;
    std::unordered_map<std::string, int> ternarySlotStoreConcat_;
    // 1963 件2：拼接产物查询单点（表优先→块内兜底→查无保守不 free+显形）。
    bool isConcatResultTraced(int regId);
