// SemanticAnalyzer 借用与所有权检查族声明片段（自 semantic.hpp 拆分·1855 拆分轮追加·行数门禁 ≤500 立法 2026-11-04 用户令·逐字搬移零语义变化）
//   承载（原 semantic.hpp 行区间）：任务 094 字符串借用污染名单 249~261；A21 借出视图生命周期 262~315（BorrowViewInfo/ContainerMutationInfo 随迁）；plans/019 借用纪律与转移判域辅助（常量引用借用/安全区边界/局部地址/转移实参/左值诊断/引用实参包装）316~340
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"

    // ==================== 任务 094：字符串借用污染名单（语义层等价实现） ====================
    // 登记（当前层；与 IR markStringTainted 五登记点判据逐条对齐·注释互指）；
    //   查询（从内到外第一层含该名者——遮蔽语义与 lookupMoved 同构）。
    void markStrTainted(const std::string& name);
    bool isStrTainted(const std::string& name) const;
    // 右值表达式是否产生「拥有串」（与 IR 层 ownAssign/ownRet 判定同构：
    //   StringLiteral/IdentifierExpr 恒拥有；CallExpr=retOwnedString ∪ 白名单；
    //   其余形态=非拥有）。白名单与 IR 层字符串赋值/初始化路径同一集合。
    bool isOwnedStrRvalue(const Expr* rhs) const;
    // 094：名字是否为当前类的字段（方法体裸字段名）——IR 层 lookupVarName 对
    //   字段返回空（字段走 this 寻址·装箱/出参移交不清零源槽），语义层标记
    //   须排除防误禁（探针 p6 实证：字段装箱后字段仍可用）。
    bool isCurrentClassFieldName(const std::string& name) const;

    // ==================== plans/019 阶段3 扩展：A21 借出视图生命周期检查 ====================
    // （第七十七轮；plans/020 矩阵 A21 格靶子：借出视图 × 容器移除=UAF）
    struct BorrowViewInfo {
        std::string viewVar;        // 借出视图变量名
        std::string container;      // 来源容器引用键（标识符=名/成员链=路径文本）
        int containerVarId = 0;     // 容器（基础名）身份 ID——同名遮蔽防误配
        int bindLine = 0;           // 绑定行（诊断定位）
        int lastUseLine = 0;        // 最后使用行（NLL 活跃区间右端）
        std::string containerType;  // 容器实例化名（诊断展示）
        SourceLocation bindLoc;     // 绑定位置（诊断用）
        bool reported = false;      // 已报错（同绑定不重复刷屏）
    };
    struct ContainerMutationInfo {
        std::string container;      // 容器引用键（同 BorrowViewInfo.container 口径）
        int containerVarId = 0;
        int line = 0;
        std::string method;
        std::string containerType;
    };
    // 借出方法名判定（元素/读取/栈顶/队首/头部元素/读取头部/读取尾部/获取）
    //   ——85-a：声明上提 public（IR 侧同用，见 public 段同名声明的注释）
    static bool isContainerInvalidateCall(const std::string& containerCanon,
                                          const std::string& methodName);
    // 字符串元素容器/字符串值映射统一判定（types:: 共享，与 IR 释放面同口径）
    static bool isBorrowSourceContainer(const std::string& canonType);
    // 变量身份 ID：沿作用域链解析（与 lookupVar 同序）——1360 结果化批3A 三态显形：
    //   无值=不可见/未命中，0=可见但无 ID（未追踪绑定），>0=身份 ID；
    //   outScopeIndex 回填层索引
    cn::Optional<int> lookupVarId(const std::string& name, int* outScopeIndex) const;
    // 表达式 → 容器引用键（标识符=名；成员链/下标链/解引用=基础名解析 + 路径文本；
    //   其余形态（调用/字面量等）返回 false）——成员链接收者支持（77-a 扩展②）
    bool resolveContainerRef(const class Expr* e, std::string& outKey,
                             int& outVarId) const;
    // 表达式路径文本重建（标识符/成员/下标/解引用；供容器引用键用）
    static void buildPathText(const class Expr* e, std::string& out);
    // 按名解析活跃借出登记（与 lookupVar 同序的作用域链 + 内层遮蔽）
    // 1360 结果化批3A：无值=未命中（原 SIZE_MAX 哨兵退役）
    cn::Optional<std::size_t> findActiveBorrowView(const std::string& name) const;
    // 调用点同源互斥（77-a 扩展①，跨函数别名窄面）：同一调用中「容器 C」与
    //   「来源为 C 的借出视图实参」并存 → 拒绝（被调函数可能修改容器；Rust 借用
    //   检查器对 `f(&mut v, &v[0])` 同类拒绝）。checkExpr 出口统一挂点。
    void checkBorrowViewCallArgs(class CallExpr* node);
    // 方法调用点登记（visitCallExpr 方法分支）：借出视图标记 或 容器失效点登记
    void noteBorrowCallSite(const class MemberExpr& mem, const std::string& clsName,
                            const std::string& methodName,
                            const class CallExpr* callNode);
    // 借出绑定登记（声明初始化位/赋值位；同层同名=重新绑定，更新既有记录）
    void registerBorrowView(const std::string& viewVar, const SourceLocation& loc);
    // 借出视图使用登记（visitIdentifierExpr 根拦截点：更新活跃区间右端 +
    //   跨作用域逃逸实时判定——容器已不可见即容器先亡）
    void noteBorrowViewUse(const std::string& name, const SourceLocation& loc);
    // 函数级结算（函数体/方法体尾部）：同作用域失效形态报错 + 状态清空
    void checkBorrowViewLifetimes();
    void clearBorrowViewState();

    // plans/019 阶段3（2026-09-10）：常量引用借用纪律——①实参为当前函数
    //   常量引用参数而形参为可变引用（只读借用不能借出可变）；②同一调用中
    void checkConstRefBorrowDiscipline(class CallExpr* node,
                                       const std::vector<std::string>& paramTypes,
                                       const std::vector<bool>& constParams);
    // plans/019 阶段4（2026-09-10 立）/ plans/023 §6.5（2026-09-17 157-a 收口）：
    //   安全区边界硬错误——安全函数（非 不安全）内出现越界操作=编译错误
    void reportUnsafeBoundary(const SourceLocation& loc, const std::string& kind,
                            const std::string& detail);
    // plans/019 阶段2（2026-09-10）：表达式是否求值为「当前函数局部的地址」——
    //   ①取地址 &局部（AddressOf 一元，基础名经 refReturnLvalueBase 解剖）
    bool isLocalAddressValue(const Expr* e, std::string& baseName) const;
    // plans/019 阶段1：转移实参类型放行判定（资源语义类型白名单）——
    //   返回 0=放行（指针/字符串，任意表达式位=值交接）；1=标量（复制语义拒绝）；
    //   2=声明初始化位限定类型（容器/类/结构体/结果/可选/数组——表达式位随阶段3）
    int transferArgKind(const std::string& type) const;
    // 缺陷②：赋值目标非左值统一拒绝（含诊断③「想写分号」跨行提示——
    //   换行≡空格规范行为下行首运算符并入上一行的粘连形态）
    void reportNonLvalueTarget(class AssignmentExpr* node);
    // A-1（引用参数）：实参自动取地址——引用参数按地址传递，调用点把实参重写为
    //   &左值（AddressOf UnaryExpr）；实参须为左值（变量/下标/解引用/字段）
    void wrapRefArgs(CallExpr* node, const std::vector<std::string>& paramTypes);
    // 1675 根修：引用实参取地址包装的单实参核心（wrapRefArgs 循环体抽出）——
    //   运算符重载决议命中点（族⑦算术/族⑧比较）对右操作数同口径包装
    void wrapRefArg(std::unique_ptr<Expr>& arg, const std::string& paramType);
