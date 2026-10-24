// 语义只读视图接口（346 重构D：接口收缩手术核心）
// 职责：ir/opt/codegen 对语义分析器的全部消费面收敛为此纯虚接口——
//   SemanticAnalyzer 公有继承实现（semantic.hpp）；driver 层唯一组装点注入。
//   ir/codegen 只依赖本接口（model 中立层），不再 include semantic.hpp——
//   消除「下游持有上游活对象指针回查」的架构回指（AGENTS §3 单向依赖执法网）。
// 分区：
//   〔一〕只读查询（布局/符号/类型判据）——语义检查完成后的事实表读取；
//   〔二〕泛型实例化受控回调——IR 生成期触发语义层重查（resolveGenericTypeName/
//         ensureLoweredType/recheckGeneric*），与 v2 侧 语法↔IR 13 处受控环完全
//         同构（编译器实例化期本质形态·021 #336 债行·职责重排独立轮）。
// 签名纪律：与 SemanticAnalyzer 对应方法逐字一致（override 校验）；
//   返回的 ClassInfo/StructDecl 等指针为语义表只读别名，禁止下游变更。
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/symbols.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

class ISemanticView {
public:
    virtual ~ISemanticView() = default;

    // ==================== 〔一〕只读查询 ====================

    // ---- 类型判据 ----
    virtual bool isClassType(const std::string& type) const = 0;
    virtual bool isStructType(const std::string& type) const = 0;
    virtual bool isEnumType(const std::string& type) const = 0;
    virtual bool isInterfaceType(const std::string& type) const = 0;
    // 1360 结果化批3B：无值=尺寸未登记 miss（原终态 return 8 哨兵退役——
    //   #648 显形链收口：miss 时函数内已报内部错误诊断，消费点 value_or(8)
    //   显形回落保持原「报错后按安全值继续」控制流）
    virtual cn::Optional<int> typeSizeOf(const std::string& type) const = 0;

    // ---- 类/结构体/接口布局 ----
    // 1360 结果化（批3A）：查表 miss 无值（原 nullptr 哨兵退役——消费点
    //   has_value() 分流，类型强制检查·漏检=编译期拒绝）
    virtual cn::Optional<const ClassInfo*> findClass(const std::string& name) const = 0;
    // ---- 1501：并行 codegen 语义只读冻结（1145 多 worker 共享视图的并发契约）----
    // 背景：findClass 为 const 但惰性写 classCache（裸名解析缓存）；并行发射时
    //   多 worker 共享同一视图，首查写路径 = 共享 unordered_map 并发 find/insert
    //   （数据竞争 UB）。契约=并行 spawn 前主线程调用本方法：预热全部类基名到
    //   只读缓存并封存（此后 findClass 只读、禁止写缓存）；预热计数经
    //   prewarmedCount 回填（调用方观测/门禁）。返回 false=errorOut 已填（如重入），
    //   调用方必须中止而非按成功继续。默认实现=替身无惰性缓存，直接放行。
    virtual bool beginReadonlyCodegenLookup(std::size_t& prewarmedCount,
                                            std::string& errorOut) {
        prewarmedCount = 0;
        (void)errorOut;
        return true;
    }
    // 解除只读冻结（主线程 join 后调用）。返回有值=冻结期出现未预热查询
    //   （首个查询名）——调用方必须以内部错误中止编译（miss 不可静默）；
    //   无值=全部命中预热缓存（正常路径）。
    virtual cn::Optional<std::string> endReadonlyCodegenLookup() {
        return std::nullopt;
    }
    virtual cn::Optional<const StructDecl*> findStruct(const std::string& name) const = 0;
    virtual cn::Optional<const InterfaceInfo*> findInterface(const std::string& name) const = 0;
    virtual cn::Optional<const GenericInfo*> findGeneric(const std::string& name) const = 0;
    virtual cn::Optional<const ClassMemberInfo*> lookupClassMember(const std::string& className,
                                                     const std::string& memberName,
                                                     std::string& ownerClass) const = 0;
    virtual cn::Optional<const ClassMemberInfo*> findCopyConstructor(const std::string& className) const = 0;
    virtual cn::Optional<int> classFieldOffset(const std::string& className, const std::string& fieldName) const = 0;
    // 1472 根治：带名字查找起点限定的类字段偏移——layoutClass=对象布局类
    //   （偏移沿其继承链布局累加），resolveFrom=字段名查找起点类（从它沿继承
    //   链向上第一个含该字段的层为命中条目——C++ 名字隐藏：最派生遮蔽条目
    //   优先）。方法体内裸字段访问须传方法声明所属类作起点：基类方法体内的
    //   v 永远是 基::v（声明类作用域解析），禁解析到派生类遮蔽字段（错槽/
    //   错类型）。缺省实现=忽略起点退化为按布局类查找（供测试替身回退）。
    virtual cn::Optional<int> classFieldOffsetFrom(const std::string& layoutClass,
                                                   const std::string& fieldName,
                                                   const std::string& resolveFrom) const {
        (void)resolveFrom;
        return classFieldOffset(layoutClass, fieldName);
    }
    // 1472：sub 是否为 base 的派生类（同类=真）——emitClassMethod 校验方法
    //   声明类在发射类继承链上（不在=异常形态，置空回退发射类同原行为）。
    //   缺省实现=false（替身未覆盖派生关系时消费方按发射类解析，行为明确）。
    virtual bool isDerivedFrom(const std::string& sub, const std::string& base) const {
        (void)sub;
        (void)base;
        return false;
    }
    virtual cn::Optional<int> fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const = 0;
    virtual cn::Optional<int> classVtableIndex(const std::string& className, const std::string& methodName) const = 0;
    virtual cn::Optional<int> interfaceSlot(const std::string& ifaceName, const std::string& methodName) const = 0;
    virtual std::vector<std::string> interfaceImplClasses(const std::string& ifaceName) const = 0;
    virtual bool enumValueOf(const std::string& enumName, const std::string& memberName,
                             std::int64_t& outValue) const = 0;
    virtual const std::unordered_map<std::string, ClassInfo>& classes() const = 0;

    // ---- 函数/全局符号 ----
    // 1360 结果化批3B：无值=查询 miss（原空串哨兵退役——消费点 value_or 显形
    //   回落或 has_value() 分流，类型强制检查·漏检=编译期拒绝）
    virtual cn::Optional<std::string> funcReturnTypeOf(const std::string& funcName) const = 0;
    // 662（方法链消费位）：函数返回类对象是否为「拥有形态」（本地移交/构造新建）
    //   ——IR 调用点对「调用返回对象」临时做析构的判定（借用返回共享对象不释放）。
    //   非纯虚+默认保守 false（未覆盖实现=不析构=现状行为·零风险安全默认）。
    virtual bool funcReturnOwnsClassOf(const std::string& funcName) const {
        (void)funcName;
        return false;
    }
    // 1360 结果化批3B：无值=函数未注册 miss；有值=形参类型表（空表=已登记
    //   无参函数·合法形态与 miss 显形可分——110 同族双语义消除）
    virtual cn::Optional<std::vector<std::string>> funcParamTypesOf(const std::string& funcName) const = 0;
    virtual bool funcReturnsRef(const std::string& funcName) const = 0;
    // 1360 结果化批3B：无值=无同名函数条目（原空串哨兵退役）
    virtual cn::Optional<std::string> funcFirstSigKey(const std::string& name) const = 0;
    virtual bool isExternFunc(const std::string& sigKey) const = 0;
    virtual bool isGlobalStatic(const std::string& name) const = 0;
    // 1360 结果化批3B：无值=未登记（原空串哨兵退役）
    virtual cn::Optional<std::string> globalStaticType(const std::string& name) const = 0;
    // 1360 结果化批3B：无值=非常量名（原空串哨兵退役）
    virtual cn::Optional<std::string> globalConstValue(const std::string& name) const = 0;
    virtual bool isTransferDecl(const void* varDeclNode, std::string& outSrcName) const = 0;
    virtual const std::vector<GenericFuncInstance>& genericFuncInstances() const = 0;

    // ==================== 〔二〕泛型实例化受控回调（021 #336 债） ====================
    virtual std::string resolveGenericTypeName(const std::string& typeName,
                                               const SourceLocation& loc) = 0;
    virtual void ensureLoweredType(const std::string& type) = 0;
    virtual void recheckGenericFuncBody(const GenericFuncInstance& gfi) = 0;
    virtual void recheckGenericMethodBody(const std::string& instanceName,
                                          const ClassMember* member) = 0;
};

} // namespace cn_compiler
