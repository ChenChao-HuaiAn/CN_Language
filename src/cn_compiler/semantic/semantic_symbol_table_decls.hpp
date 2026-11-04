// SemanticAnalyzer 符号表管理族声明片段（自 semantic.hpp 拆分·1855 拆分轮追加·行数门禁 ≤500 立法 2026-11-04 用户令·逐字搬移零语义变化）
//   承载（原 semantic.hpp 行区间）：符号表管理 169~230
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/symbols.hpp"
#include "cn_compiler/semantic/semantic_name_index.hpp"

    // ==================== 符号表管理 ====================
    void pushScope();                              // 进入新作用域
    void popScope();                               // 退出当前作用域
    // 声明变量（同作用域重复声明返回false并报告错误）
    bool declareVar(const std::string& name, const std::string& type, const SourceLocation& loc);
    // 注册结构体/枚举类型名（同模块重复注册报错；跨模块同名允许 = crate 分桶，
    //   A-2 2026-08：类型按模块隔离，引用经 resolveTypeName 解析到所属模块）
    void declareTypeName(const std::string& name, const std::string& module,
                         const SourceLocation& loc);
    // A-2：类型引用解析——裸名按当前模块解析，多模块同名时改写为限定键
    //   （模块名::类型）；指针/数组/模板后缀递归处理。返回解析后的类型键。
    std::string resolveTypeName(const std::string& type, const std::string& module,
                                const SourceLocation& loc);
    // #1949：类型引用存在性检查（对齐 v2 语义分析中 检查类型引用·plans/018
    //   类型分桶）——挂点=变量声明/函数参数/返回类型（v2 三挂点同构）。
    //   未声明类型名此前宿主静默放行产出错误程序（v2 同位拒绝=零分叉缺陷）。
    //   诊断落递归 miss 内层（v2 递归文案同构：幽灵[3] 报 幽灵）。
    void checkTypeReference(const std::string& type, const SourceLocation& loc);
    // #1949：类型引用存在性判定内核（checkTypeReference 与返回转换检查跳过
    //   判定共用；diagnose=假纯判定·真=miss 内层报诊断）。depth=递归深度守卫。
    bool checkTypeReferenceInner(const std::string& type, const SourceLocation& loc,
                                 bool diagnose, int depth) const;
    // #1949：类型引用存在性纯判定（返回转换检查跳过判定用——上游已诊断豁免
    //   防级联第二诊断）。
    bool isTypeReferenceDeclared(const std::string& type) const;
    // A-2：拆分限定类型键（甲::记录 -> ("甲","记录")；无 :: -> ("", 原名)）
    static void splitQualifiedType(const std::string& type, std::string& module,
                                   std::string& base);
    // A-2：按模块查结构体/枚举（限定键或裸名+模块集合；1360 结果化批3A：
    //   无值=未命中——原「裸名且多模块定义返回nullptr」哨兵退役）
    cn::Optional<const StructDecl*> findStructInModule(const std::string& module,
                                         const std::string& name) const;
    cn::Optional<const EnumDecl*> findEnumInModule(const std::string& module,
                                     const std::string& name) const;
    // 1155：类型名查询索引惰性构建（三表 size 变化整表重建·见 semantic_name_index.hpp）
    const TypeLookupIndex& ensureTypeLookupIndex() const;
    // 计算结构体/联合体布局（C风格对齐，Task 2.7）
    void computeLayout(StructDecl* decl);
    // T3（306-a 波次2）：布局计算中的结构体名栈——值字段递归环检测（无穷大小类型·编译期拒绝；Rust E0072 同类）
    std::unordered_set<std::string> layoutVisiting_;
    // 按名取非常量结构体声明（306-a T3 惰性递归布局用；跨 program_->structs 线性查）
    // 1360 结果化批3A：无值=未命中
    cn::Optional<StructDecl*> findMutableStruct(const std::string& name);
    // 枚举成员值求值（自动递增/显式赋值/负数，Task 2.7）
    void computeEnumValues(EnumDecl* decl);
    // 从内到外查找变量类型（未找到返回false）
    bool lookupVar(const std::string& name, std::string& type) const;
    bool hitsNonStaticLocal(const std::string& name) const;  // 1337 遮蔽写豁免判据（函数内非静态局部）
    // 缺陷②配套（2026-09-03 用户裁决）：名字在作用域链解析处是否为常量（局部=
    //   scopeConsts_·顶层=globalConstValues_ 裸名；同序遮蔽）·供赋值/自增拒绝。
    bool isConstVarName(const std::string& name) const;
    // plans/019 阶段1（2026-09-10）：转移(变量) 标记——沿作用域链找到 name 的
    //   声明层，写入该层 scopeMoved_（名 -> 转移点行号，诊断定位用）。
    //   幂等：重复标记（理论上不会再转移已转移变量——visitCallExpr 已拒）。
    void markMovedVar(const std::string& name, int line);
    // plans/019 阶段1：名字在作用域链解析处是否已转移（与 lookupVar 同序·内层同名
    //   新声明=新变量不影响）；命中返回 true 并回填转移点行号。
    bool lookupMoved(const std::string& name, int& outLine) const;
    // 已转移变量使用拒绝（读值/左值共用）——命中即报 E0382 对标诊断并返回 true
    bool reportMovedUse(const std::string& name, const SourceLocation& loc);
    // 981 赋值复活（001 条文⑥·用户裁决甲）：清除 name 的已转移标记（纯赋值写点=复活点·Rust assign-to-move 同款——与 markMovedVar 同序找声明层）
    void reviveMovedVar(const std::string& name);
