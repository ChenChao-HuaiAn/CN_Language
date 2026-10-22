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
    virtual int typeSizeOf(const std::string& type) const = 0;

    // ---- 类/结构体/接口布局 ----
    // 1360 结果化（批3A）：查表 miss 无值（原 nullptr 哨兵退役——消费点
    //   has_value() 分流，类型强制检查·漏检=编译期拒绝）
    virtual cn::Optional<const ClassInfo*> findClass(const std::string& name) const = 0;
    virtual cn::Optional<const StructDecl*> findStruct(const std::string& name) const = 0;
    virtual cn::Optional<const InterfaceInfo*> findInterface(const std::string& name) const = 0;
    virtual cn::Optional<const GenericInfo*> findGeneric(const std::string& name) const = 0;
    virtual cn::Optional<const ClassMemberInfo*> lookupClassMember(const std::string& className,
                                                     const std::string& memberName,
                                                     std::string& ownerClass) const = 0;
    virtual cn::Optional<const ClassMemberInfo*> findCopyConstructor(const std::string& className) const = 0;
    virtual cn::Optional<int> classFieldOffset(const std::string& className, const std::string& fieldName) const = 0;
    virtual cn::Optional<int> fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const = 0;
    virtual cn::Optional<int> classVtableIndex(const std::string& className, const std::string& methodName) const = 0;
    virtual cn::Optional<int> interfaceSlot(const std::string& ifaceName, const std::string& methodName) const = 0;
    virtual std::vector<std::string> interfaceImplClasses(const std::string& ifaceName) const = 0;
    virtual bool enumValueOf(const std::string& enumName, const std::string& memberName,
                             std::int64_t& outValue) const = 0;
    virtual const std::unordered_map<std::string, ClassInfo>& classes() const = 0;

    // ---- 函数/全局符号 ----
    virtual std::string funcReturnTypeOf(const std::string& funcName) const = 0;
    // 662（方法链消费位）：函数返回类对象是否为「拥有形态」（本地移交/构造新建）
    //   ——IR 调用点对「调用返回对象」临时做析构的判定（借用返回共享对象不释放）。
    //   非纯虚+默认保守 false（未覆盖实现=不析构=现状行为·零风险安全默认）。
    virtual bool funcReturnOwnsClassOf(const std::string& funcName) const {
        (void)funcName;
        return false;
    }
    virtual std::vector<std::string> funcParamTypesOf(const std::string& funcName) const = 0;
    virtual bool funcReturnsRef(const std::string& funcName) const = 0;
    virtual std::string funcFirstSigKey(const std::string& name) const = 0;
    virtual bool isExternFunc(const std::string& sigKey) const = 0;
    virtual bool isGlobalStatic(const std::string& name) const = 0;
    virtual std::string globalStaticType(const std::string& name) const = 0;
    virtual std::string globalConstValue(const std::string& name) const = 0;
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
