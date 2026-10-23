// SemanticAnalyzer 查询族声明片段（自 semantic.hpp 拆分·1360 批3A 行数门禁·逐字搬移零语义变化）
//   承载：public 区两连续查询段——①结构体/枚举查询（Task 2.7，供IR层复用布局）；
//   ②阶段3：类/接口/泛型查询（供IR层复用，Task 3.x；段尾含第 4 层顶层常量查询）。
//   1360 结果化批3A 哨兵结果化（cn::Optional 化）落点主体即此两段。
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 public 区语义），不可独立包含、不可重复包含。
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/symbols.hpp"

    // ==================== 结构体/枚举查询（Task 2.7，供IR层复用布局） ====================
    // 是否结构体/联合体类型名
    bool isStructType(const std::string& type) const override ;
    // 是否枚举类型名
    bool isEnumType(const std::string& type) const override ;
    // 查找结构体/联合体定义（1360 结果化批3A：无值=未命中）
    cn::Optional<const StructDecl*> findStruct(const std::string& name) const override ;

    // 164-a（A4 方案A·plans/023 §十二）：可平凡复制判定（对标 Rust Copy）——
    //   标量/指针/枚举/函数指针；递归聚合（结构体/联合体/数组/结果/可选实参）
    bool isTriviallyCopyable(const std::string& type,
                             std::vector<std::string>& visiting) const;
    // 217（027 波2a·001 §5.8a）：类型可搬运判定（对标 Rust Send）——可安全按值
    //   拷贝进入另一线程（新线程 实参位）。标量/字符串/函数指针恒可；结果/可选/
    bool isPortableType(const std::string& type, std::string& offender,
                        int depth = 0) const;
    // 查找枚举定义（1360 结果化批3A：无值=未命中）
    cn::Optional<const EnumDecl*> findEnum(const std::string& name) const;
    // 计算类型大小（字节）：基本类型/指针/数组/结构体/枚举/结果/可选/类
    //   （1360 结果化批3B：无值=尺寸未登记 miss——原终态 return 8 哨兵退役）
    cn::Optional<int> typeSizeOf(const std::string& type) const override ;
    // 计算类型对齐（字节）（1360 结果化批3B：无值=对齐未登记 miss——原终态
    //   return 1 哨兵退役·#1238 布局兜底显形）
    cn::Optional<int> typeAlignOf(const std::string& type) const;
    // 查找结构体字段偏移（-1表示无此字段）
    cn::Optional<int> fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const override ;
    // 查找枚举成员值（未找到返回false）
    bool enumValueOf(const std::string& enumName, const std::string& memberName,
                     std::int64_t& outValue) const override ;
    // 查询函数返回类型（1360 结果化批3B：无值=未注册 miss——原空串哨兵退役；
    //   供IR层推导调用结果类型，Task 2.7 集成修复）
    cn::Optional<std::string> funcReturnTypeOf(const std::string& funcName) const override ;
    // P3-18 补完（2026-08）：函数返回类型是否为引用（T&）——调用点将结果当"左值地址"
    bool funcReturnsRef(const std::string& funcName) const override ;
    // 662：函数返回类对象「拥有形态」查询（IR 调用点析构判定·键域同 funcReturnTypeOf）。
    bool funcReturnOwnsClassOf(const std::string& funcName) const override ;
    // 返回该函数名的第一个签名 key（函数名作值/取地址用，Task 2.10；
    //   1360 结果化批3B：无值=无同名条目——原空串哨兵退役）
    cn::Optional<std::string> funcFirstSigKey(const std::string& name) const override ;
    // 查询函数参数类型列表（1360 结果化批3B：无值=未注册 miss；有值空表=已登记
    //   无参函数·110 同族双语义显形消除；供IR层推导结构体按值实参传递，Task 完善A）
    cn::Optional<std::vector<std::string>> funcParamTypesOf(const std::string& funcName) const override ;
    // C-3：是否 外部 函数（链接符号=纯名，IR 调用侧按此映射）
    bool isExternFunc(const std::string& sigKey) const override ;
    // 程序AST（供结构体/枚举符号表查询）
    Program* program_ = nullptr;

    // ==================== 阶段3：类/接口/泛型查询（供IR层复用，Task 3.x） ====================
    // 是否类类型名
    bool isClassType(const std::string& type) const override ;
    // 是否接口类型名
    bool isInterfaceType(const std::string& type) const override ;
    // 是否 结果<T,E> 模板类型
    // 是否 可选<T> 模板类型
    // 解析 结果<T,E> 参数（未匹配返回空向量）
    // 解析 可选<T> 参数（未匹配返回空串）
    // 生成 结果<T,E>/可选<T> 的合成结构体名（IR 层布局用）
    // 内置合成模板文本 → 合成结构体名统一形态（061-d：结果<T,E> -> 结果$T$E；
    //   可选<T> -> 可选$T；非合成文本原样返回）——泛型容器实例化与 IR 层
    //   名字拼装统一经此，消除 尖括号原文 vs $ 形态 两套注册/查询键
    // 查找类符号（1360 结果化批3A：无值=未命中——原 nullptr 哨兵退役）
    cn::Optional<const ClassInfo*> findClass(const std::string& name) const override ;
    // 全部类符号表只读访问（Task 3.1，供 codegen 遍历生成虚表/静态字段/类方法符号）
    const std::unordered_map<std::string, ClassInfo>& classes() const override { return classes_; }
    // 全部泛型函数实例化记录只读访问（Task 6.1，供 IR 层生成函数体）
    //   （548 行数门禁腾挪：原类内 inline 体机械迁移 semantic.cpp·零语义变化）
    const std::vector<GenericFuncInstance>& genericFuncInstances() const override;
    // 确保单个类型的结果/可选合成结构体已降级（Task 6.1 泛型类实例化后调用）
    void ensureLoweredType(const std::string& type)override ;
    // 全部接口符号表只读访问（Task 3.3，供 codegen 预留接口信息）
    const std::unordered_map<std::string, InterfaceInfo>& interfaces() const { return interfaces_; }
    // 查找接口符号（1360 结果化批3A：无值=未命中）
    cn::Optional<const InterfaceInfo*> findInterface(const std::string& name) const override ;
    // P3-19：接口成员全局槽位（未登记返回 -1）
    cn::Optional<int> interfaceSlot(const std::string& ifaceName, const std::string& methodName) const override ;
    // P3/D3A：接口的非抽象具体实现类集合（含继承链并入；登记在 ClassInfo.ifaceNames）。
    //   去虚拟化：集合恰 1 项 → 接口调用点编译期直接调用；CFI：集合即该接口已知实现目标表。
    std::vector<std::string> interfaceImplClasses(const std::string& ifaceName) const override ;
    // P3-19：类（含继承链）是否实现指定接口
    bool classImplementsInterface(const std::string& className,
                                  const std::string& ifaceName) const;
    // 234（2026-10-06·91303）：sub 是否为 base 的派生类（沿 baseName 链上溯·
    //   同类也算真=调用方按需排除）
    bool isDerivedFrom(const std::string& sub, const std::string& base) const;
    // 234：类→基类值形态切片拒绝专用诊断（报了返回 true·立法 234 防切片）
    bool reportSliceReject(const std::string& from, const std::string& to,
                           const SourceLocation& loc);
    // 沿继承链查找类成员（含父类；1360 结果化批3A：无值=未命中）
    cn::Optional<const ClassMemberInfo*> lookupClassMember(const std::string& className,
                                             const std::string& memberName,
                                             std::string& ownerClass) const override ;
    // 2026-08-25 方案A：查类的拷贝构造（单参同类型引用 类名(类名& 其他)）；
    //   有值=按值拷贝走深拷贝；1360 结果化批3A：无值=无拷贝构造
    cn::Optional<const ClassMemberInfo*> findCopyConstructor(const std::string& className) const override ;
    // 2026-08-25 方案A 强制规则：有析构类按值拷贝（初始化/赋值）须有拷贝构造，
    //   否则编译报错（浅拷贝裸指针字段析构双释放 0xC0000374）。调用方仅在
    //   确认发生"类对象拷贝"时调用（无析构类保持浅拷贝，不触发）。
    void checkCopyRequiresCtor(const std::string& className,
                               const SourceLocation& loc);
    // 查询类虚函数表槽位（方法名 -> 槽位索引；非虚/未找到返回-1）
    cn::Optional<int> classVtableIndex(const std::string& className, const std::string& methodName) const override ;
    // 查询类布局（实例总大小/对齐）。1360 结果化批3A：无值=类不存在
    cn::Optional<int> classTotalSize(const std::string& className) const;
    // 查询类字段偏移（沿继承链，-1表示无此字段；含虚表指针偏移调整）
    cn::Optional<int> classFieldOffset(const std::string& className, const std::string& fieldName) const override ;
    // 查询泛型声明（1360 结果化批3A：无值=未命中）。Debug 子任务修复（泛型
    //   类方法体提升需解析 实例化类名$实参 的类型参数映射）——公开转发供 IR 层访问。
    cn::Optional<const GenericInfo*> findGeneric(const std::string& name) const override ;
    // 234-a（A7 根治·plans/020 第七十五节）：IR 生成泛型实例方法体前的重检查。
    //   缺陷：实例化类方法体 AST 为全实例共享（mi.ast 指向模板成员），语义检查
    void recheckGenericMethodBody(const std::string& instanceName,
                                  const ClassMember* member)override ;
    // 317-a（T19/T20 波次4·D10 面汇合）：泛型函数实例体生成前的重放检查——
    //   原泛型函数体从未被语义检查（26_generics 遗留）：体内泛型类实例化触发
    void recheckGenericFuncBody(const GenericFuncInstance& gfi)override ;
    // 317-a：泛型函数实例化记录登记（去重）——显式 <> 调用段与推断段共用
    void registerGenericFuncInstance(const std::string& instName,
                                     const GenericInfo* ginfo,
                                     const std::vector<std::string>& args);
    // 泛型实例化类型名替换（Task 3.8）：名<实参> -> 实例化类名（容器$整32）；
    //   非泛型类型原样返回。H8 补完（2026-08-25）：公开供 IR 层 类型大小(T)
    std::string resolveGenericTypeName(const std::string& typeName,
                                       const SourceLocation& loc)override ;
    // ---- 第 4 层（v2.0 决策9，P1-4）：顶层常量查询（IR 层编译期折叠）----
    // 查询顶层常量值文本（1360 结果化批3B：无值=非常量名——原空串哨兵退役；
    //   值为字面量 raw 文本）
    //   （548 行数门禁腾挪：原类内 inline 体机械迁移 semantic.cpp·零语义变化）
    cn::Optional<std::string> globalConstValue(const std::string& name) const override;
    // 是否顶层静态变量名（IR 层生成全局存储）；查询顶层静态变量源码类型
    //   （1360 结果化批3B：无值=未登记——原空串哨兵退役·IR 层映射全局存储类型）
    //   ——同上腾挪 semantic.cpp
    bool isGlobalStatic(const std::string& name) const override;
    cn::Optional<std::string> globalStaticType(const std::string& name) const override;
