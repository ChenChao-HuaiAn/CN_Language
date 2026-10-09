// 语义符号数据模型（346 重构D：自 semantic.hpp 下沉中立层）
// 职责：语义分析产出的符号表条目纯数据结构——FunctionInfo/ClassMemberInfo/
//   ClassInfo/InterfaceInfo/GenericInfo/GenericFuncInstance + ErrorCheckState。
//   semantic 填充、ir/opt/codegen 只读消费（经 ISemanticView 接口·semantic_view.hpp）。
//   与 AST/类型系统同住 model 中立层（AGENTS §3 目标态·与 v2 协议常量包对称）。
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cn_compiler/model/ast.hpp"

namespace cn_compiler {

// 函数符号信息：返回类型 + 参数类型列表 + 是否有函数体
struct FunctionInfo {
    std::string returnType;                // 返回类型（"空类型"表示无返回值）
    std::vector<std::string> paramTypes;   // 参数类型列表
    bool hasBody = false;                  // 是否有函数体（函数原型声明无体）
    bool variadic = false;                 // 是否变参函数（Task 2.5：打印行 多参数）
    bool isExtern = false;                 // C-3：外部 函数 声明（C 链接符号=纯名）
    // ---- Task 2.10：默认参数 ----
    std::vector<bool> hasDefault;          // 每个参数是否有默认值（与 paramTypes 等长）
    // plans/019 阶段4（2026-09-10）：不安全 函数 修饰——安全区边界（观察期
    //   =警告：安全函数体内指针算术/指针下标写/联合体访问/外部函数调用/裸
    //   释放 发警告不报错；分批收口后变错误）
    bool isUnsafe = false;
    // plans/019 阶段3（2026-09-10）：常量 只读引用参数位表（与 paramTypes 等长；
    //   常量 T& 形参=只读借用——体内赋值/传可变引用/与可变借用互斥均拒绝）
    std::vector<bool> constParams;
    // 默认值表达式按需求值：IR 层展开；语义层仅记录个数（defaultCount 为尾部连续
    // 带默认值的参数个数，调用时用于"实参个数 + 可补全"匹配）
    int defaultCount = 0;                  // 尾部默认参数个数（从右向左连续声明）
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    // 所属模块（crate 域）名：registerFunction 写入（FunctionDecl::moduleName）。
    std::string moduleName;
    // P3-18 补完（2026-08）：函数返回类型为 T&（引用返回，返回被引用左值地址）。
    // 不参与重载签名（返回类型不构成重载）；isRefReturn 供 IR（返回类型映射 ptr）
    // 与调用方（引用绑定 / 赋值写回 / 取地址）识别。
    bool isRefReturn = false;
};

// ==================== 阶段3：类成员信息（Task 3.1） ====================

// 类成员符号信息（字段/方法/构造/析构/运算符重载）
struct ClassMemberInfo {
    std::string name;                          // 成员名（方法名/字段名）
    std::string type;                          // 字段类型 或 方法返回类型
    std::vector<std::string> paramTypes;       // 方法参数类型列表（字段为空）
    AccessSpecifier access = AccessSpecifier::Public;  // 可见性
    bool isStatic = false;                     // 静态成员（Task 3.9）
    bool isConstMethod = false;                // 常量成员函数（Task 3.9）
    bool isVirtual = false;                    // 虚函数（Task 3.2）
    bool isAbstract = false;                   // 抽象方法（纯虚）
    bool isOverride = false;                   // 重写修饰
    int vtableIndex = -1;                      // 虚函数表槽位（-1=非虚；Task 3.2）
    std::string operatorSym;                   // 运算符符号（"+"；kind=Operator 时非空）
    std::string ownerClass;                    // 所属类名（沿继承链查找时记录来源类）
    bool isConstructor = false;                // 构造函数（函数名 == 类名）
    bool isDestructor = false;                 // 析构函数（~类名）
    bool isCopyConstructor = false;            // 拷贝构造（单参同类型引用：类名(类名& 其他)）
    bool hasBody = false;                      // 是否有方法体（抽象/接口签名为空）
    const ClassMember* ast = nullptr;          // AST 节点指针（供 IR 层生成）
    std::string sigKey;                        // 方法签名 key（名#参数串，mangling 用）
    // plans/019 阶段3b（2026-09-10）：常量 只读引用参数位表（构造/方法调用面
    //   借用纪律用；与 paramTypes 等长——普通函数 FunctionInfo.constParams 同构）
    std::vector<bool> constParams;
    // D23 根治（248-a）：构造函数尾部默认参数个数（从右向左连续声明）——
    //   构造调用决议按「实参个数 + 可补全」匹配（FunctionInfo.defaultCount 同构）；
    //   缺省实参值由 IR 层 funcDefaultArgs_（emitClassMethod 收集）展开。
    int defaultCount = 0;
    // plans/019 阶段4 第二层第一批（2026-09-10）：不安全 方法修饰（安全区边界
    //   ——方法体内五类越界操作豁免观察期警告）
    bool isUnsafe = false;
    // 118（929·2026-10-01）：引用返回方法（-> T&·如 向量.元素引用）——按 AST
    //   返回类型原文判定（type 字段经 canonical 剥 & 不可判）；供语义层左值
    bool isRefReturn = false;
};

// 类符号信息：成员表 + 继承 + 虚表 + 接口实现 + 布局（Task 3.1~3.3）
struct ClassInfo {
    std::string name;                          // 类名
    // ---- 第 4 层（v2.0 决策11，可见性交集检查）----
    // 所属模块（crate 域）名 + 模块级可见性：registerClassAndInterfaces 写入。
    //   可见性交集：跨模块访问类成员须 模块公开 × 类内公开（交集最严格）。
    std::string moduleName;                    // 所属模块名（空=单文件）
    AccessSpecifier moduleAccess = AccessSpecifier::Public;  // 模块级可见性
    std::string baseName;                      // 父类名（空=无继承）
    std::vector<std::string> interfaces;       // 实现的接口名列表
    std::unordered_map<std::string, ClassMemberInfo> fields;    // 字段表（含继承并入）
    std::unordered_map<std::string, ClassMemberInfo> methods;   // 方法表（含继承并入）
    std::vector<std::string> fieldOrder;       // 字段声明顺序（父类字段在前，布局用）
    std::vector<std::string> methodOrder;      // 方法声明顺序（含继承）
    std::vector<std::string> vtableOrder;      // 虚函数表槽位顺序（方法名列表，Task 3.2）
    std::vector<std::string> friendFuncs;      // 友元函数名（Task 3.9）
    std::vector<std::string> friendClasses;    // 友元类名（Task 3.9）
    int totalSize = 0;                         // 实例大小（字节，含虚表指针）
    int align = 8;                             // 对齐（含虚表指针后按8对齐）
    bool hasVtable = false;                    // 是否有虚函数表
    // P3-19：接口分派区（B1 全局槽位；对象首 8 字节虚表指针之后，槽=8+全局槽*8）
    std::vector<std::pair<int, std::string>> ifaceDisp;  // (全局槽, 接口方法名)
    // P3/D3A：本类实现的全部接口名（含继承链并入；来源=class_resolver ifaceDisp 收集）。
    //   供 接口→实现类集合 统计（去虚拟化唯一实现判定 + CFI 目标表）。
    std::vector<std::string> ifaceNames;
    int ifaceMaxSlot = -1;                     // 本类实现的接口方法最大全局槽
    int ifaceRegionSize = 0;                   // 接口分派区字节数 (maxSlot+1)*8
    bool isAbstract = false;                   // 含抽象方法（不可实例化）
    const ClassDecl* ast = nullptr;            // AST 节点指针
    // H8 根治（2026-08-25）：泛型类实例化实参列表（instantiateGeneric 存储）。
    //   方法体 genericTypeParams_ 解析用——嵌套实参（向量$映射$整64$整64 的
    std::vector<std::string> typeArgs;
};

// 接口符号信息：只含虚函数签名（Task 3.3）
struct InterfaceInfo {
    std::string name;                          // 接口名
    std::unordered_map<std::string, ClassMemberInfo> methods;   // 方法签名表
    std::vector<std::string> methodOrder;      // 方法声明顺序
    const InterfaceDecl* ast = nullptr;        // AST 节点指针
};

// 泛型声明信息（Task 3.8）：记录泛型模板供实例化
struct GenericInfo {
    std::vector<std::string> typeParams;       // 类型参数名（如 [T, U]）
    std::vector<std::string> constraints;      // 接口约束（与 typeParams 一一对应，空串=无）
    const GenericDecl* ast = nullptr;          // 泛型 AST（内嵌类/函数）
};

// 泛型函数实例化记录（Task 6.1 打通泛型函数调用）：
//   泛型函数 名<实参>(...) 调用时单态化注册 名$实参 函数符号，此处记录
//   实例化信息供 IR 层生成函数体（替换类型参数 T -> 实参）。
struct GenericFuncInstance {
    std::string instanceName;                  // 实例化函数名（名$实参串）
    const GenericDecl* gen = nullptr;          // 原泛型声明 AST（内嵌 innerFunc）
    std::vector<std::string> args;             // 类型实参列表（如 ["整32"]）
};

// 错误码传播分析状态（Task 3.5，规则1~3）：
//   变量名 -> 已检查标记（"正常"=结果.正常已检查 / "有值"=可选.有值已检查）
using ErrorCheckState = std::unordered_map<std::string, std::string>;

} // namespace cn_compiler
