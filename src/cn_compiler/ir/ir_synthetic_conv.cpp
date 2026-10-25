// CN-IR生成器实现：合成体（结果/可选）异型隐式转换（#1592·P1·1589/1592）
// 职责（单一族）：结果<A,B> -> 结果<C,D>（A→C 且 B→D 可转）/可选<T> -> 可选<U>
//   的**值保真**整体拷贝——同型/等价载荷=调用方原整块 CopyStruct（零开销），
//   异型=逐字段转换（判别位@0 + 按 tag 分支，读按源类型、Cast 桥接、写按目标）。
// 归属：IR 层合成体布局消费族（与 ir_fields.cpp 的 emitStructCopyWithFields
//   同域）——先拆再加：#1592 落地前 ir_fields.cpp 884 行贴 900 警戒线，按纪律
//   把本族独立成文件（纯新增实现·hpp 声明单源）。
// 哨兵纪律（#1360 结果化）：查表/形态解析一律返回 cn::Optional（无值=非合成体
//   或布局未降级），不用空串/0 哨兵与 bool 输出参数。
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 1589/1592：合成体（结果/可选）降级结构体形状——载荷类型/偏移从降级声明读取
//   （$ 合成名与模板文本双形态统一入口·不依赖文本切分）。
namespace {
struct SyntheticShape {
    bool isResult = false;
    std::string key;
    std::string valueType;
    std::string errorType;
    int tagOffset = 0;
    int payloadOffset = 0;
};
}  // namespace

// ==================== 1589/1592：合成体异型隐式转换 ====================
// 语义层放行 结果<A,B> -> 结果<C,D>（A→C 且 B→D 可转；可选<T> -> 可选<U> 同理），
// 但旧实现按**目标**类型大小整块 CopyStruct：源载荷窄于目标时高位残留（宿主
// 结果<整32,整32> -> 结果<整64,整32> 负数读 4294967295 实测）、符号/零扩展语义
// 丢失。本族=逐字段值保真转换（v2 侧 生成语句结构体赋值 同构）：
//   ① 载荷对完全相同 -> 返回假（调用方原整块拷贝路径·零开销）；
//   ② 异型 -> 读判别位@0（i32 4B·构造端同宽）+ 按 tag 分支：正常支 T1→T2、
//      错误支 E1→E2（可选无错误支）；读按源 IR 类型（符号扩展语义随类型）、
//      Cast 桥接、写按目标 IR 类型（窄写截断）。返回 真=已接管。
// 每侧独立判据（1478 实镜：T=边界盒 同型结构体 + E 整32→整64）——同型侧走
//   聚合拷贝/字符串深拷（含串字段递归原路径），异型侧必须标量可转换；异型非
//   标量=内部错误显形（语义层 canConvertType 仅同型放行该面，禁静默 8 字节错拷）。
bool IRGenerator::emitSyntheticConvCopyIfNeeded(const ir::IRValue& dstAddr,
                                                const ir::IRValue& srcAddr,
                                                const std::string& dstCanonRaw,
                                                const std::string& srcCanonRaw,
                                                const SourceLocation& loc, bool deepCopy) {
    // 类型键：尖括号模板文本 / $ 合成名 双形态 -> 降级结构体名（无值=非合成体）
    const auto keyOf = [](const std::string& raw) -> cn::Optional<std::string> {
        const std::string canon = types::canonical(raw);
        if (isResultType(canon)) {
            const auto args = resultTypeArgs(canon);
            if (args.has_value() && args.value().size() == 2) {
                return cn::Optional<std::string>(
                    resultStructName(args.value()[0], args.value()[1]));
            }
            return std::nullopt;
        }
        if (isOptionalType(canon)) {
            const auto t = optionalTypeArg(canon);
            if (t.has_value() && t.value().size() != 0) {
                return cn::Optional<std::string>(optionalStructName(t.value()));
            }
            return std::nullopt;
        }
        if (canon.rfind("结果$", 0) == 0 || canon.rfind("可选$", 0) == 0) {
            return cn::Optional<std::string>(canon);
        }
        return std::nullopt;
    };
    const cn::Optional<std::string> dstKeyOpt = keyOf(dstCanonRaw);
    if (!dstKeyOpt.has_value()) return false;  // 目标非合成体：与调用方原路径同义
    const std::string dstKey = dstKeyOpt.value();
    // 形状解析（无值=布局未降级/形态不完整——调用方显形，禁继续错拷）
    const auto shapeOf = [this](const std::string& key) -> cn::Optional<SyntheticShape> {
        const StructDecl* decl = semantic_->findStruct(key).value_or(nullptr);
        if (decl == nullptr) return std::nullopt;
        SyntheticShape out;
        out.key = key;
        out.isResult = key.rfind("结果$", 0) == 0;
        const int tagOff =
            semantic_->fieldOffsetOf(decl, out.isResult ? "正常" : "有值").value_or(-1);
        const int payOff = semantic_->fieldOffsetOf(decl, "值").value_or(-1);
        if (tagOff == -1 || payOff == -1) return std::nullopt;
        out.tagOffset = tagOff;
        out.payloadOffset = payOff;
        if (out.isResult) {
            const StructDecl* unionDecl = nullptr;
            for (const auto& f : decl->fields) {
                if (f.name == "错误值联合") {
                    unionDecl =
                        semantic_->findStruct(types::canonical(f.type)).value_or(nullptr);
                    break;
                }
            }
            if (unionDecl == nullptr) return std::nullopt;
            for (const auto& f : unionDecl->fields) {
                if (f.name == "值") out.valueType = types::canonical(f.type);
                else if (f.name == "错误值") out.errorType = types::canonical(f.type);
            }
            if (out.valueType.size() == 0 || out.errorType.size() == 0) return std::nullopt;
            return cn::Optional<SyntheticShape>(out);
        }
        for (const auto& f : decl->fields) {
            if (f.name == "值") out.valueType = types::canonical(f.type);
        }
        if (out.valueType.size() == 0) return std::nullopt;
        return cn::Optional<SyntheticShape>(out);
    };
    const cn::Optional<SyntheticShape> dstShapeOpt = shapeOf(dstKey);
    if (!dstShapeOpt.has_value()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "内部错误: 合成体布局未降级 '" + dstKey + "'");
        return true;  // 已消费：诊断已报，禁再整块错拷
    }
    const SyntheticShape& dstShape = dstShapeOpt.value();
    if (srcCanonRaw.size() == 0) return false;  // 调用点未提供源类型（同型原路径）
    const cn::Optional<std::string> srcKeyOpt = keyOf(srcCanonRaw);
    if (!srcKeyOpt.has_value()) return false;  // 源非合成体：原路径承载同型语义
    const std::string srcKey = srcKeyOpt.value();
    const cn::Optional<SyntheticShape> srcShapeOpt = shapeOf(srcKey);
    if (!srcShapeOpt.has_value()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "内部错误: 合成体布局未降级 '" + srcKey + "'");
        return true;
    }
    const SyntheticShape& srcShape = srcShapeOpt.value();
    if (srcShape.isResult != dstShape.isResult) return false;  // 结果↔可选不放行
    const bool tSame = srcShape.valueType == dstShape.valueType;
    const bool eSame =
        !dstShape.isResult || srcShape.errorType == dstShape.errorType;
    if (tSame && eSame) return false;  // 等价载荷：原整块拷贝（零开销）
    // 每侧独立：异型侧必须标量可转换（同型侧可聚合/资源——走原拷贝语义）
    const auto isConvScalar = [this](const std::string& t) -> bool {
        if (t.size() == 0) return false;
        if (types::isArray(t) || types::isPointer(t) || types::isReference(t)) return false;
        if (types::isI128(t)) return false;
        if (semantic_->isStructType(t) || semantic_->isClassType(t)) return false;
        const std::string ir = mapType(t);
        return ir == "i8" || ir == "i16" || ir == "i32" || ir == "i64" ||
               ir == "u8" || ir == "u16" || ir == "u32" || ir == "u64" ||
               ir == "f32" || ir == "f64" || ir == "i1";
    };
    if ((!tSame && (!isConvScalar(srcShape.valueType) ||
                    !isConvScalar(dstShape.valueType))) ||
        (!eSame && (!isConvScalar(srcShape.errorType) ||
                    !isConvScalar(dstShape.errorType)))) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "内部错误: 合成体载荷隐式转换不支持 '" + dstCanonRaw +
                                "' <- '" + srcCanonRaw + "'（非标量载荷）");
        return true;
    }
    // 判别位（源@tag → 目标@tag，构造端 i32 4B 同宽）
    ir::IRValue tag = emitResult(ir::Opcode::LoadPtr, {srcAddr}, "i32", "", loc);
    ir::IRValue tagAddr =
        dstShape.tagOffset == 0
            ? dstAddr
            : emitResult(ir::Opcode::FieldAddr, {dstAddr}, "ptr",
                         std::to_string(dstShape.tagOffset), loc);
    emit(ir::Opcode::StorePtr, {tagAddr, tag}, ir::IRValue(), "", "i32", loc);
    // 载荷地址（各自降级布局偏移——结果联合/可选值）
    ir::IRValue srcPayload = emitResult(ir::Opcode::FieldAddr, {srcAddr}, "ptr",
                                        std::to_string(srcShape.payloadOffset), loc);
    ir::IRValue dstPayload = emitResult(ir::Opcode::FieldAddr, {dstAddr}, "ptr",
                                        std::to_string(dstShape.payloadOffset), loc);
    const auto emitPayloadConv = [this, &loc, deepCopy](const ir::IRValue& sAddr,
                                                        const ir::IRValue& dAddr,
                                                        const std::string& sType,
                                                        const std::string& dType) {
        const std::string sCanon = types::canonical(sType);
        if (sCanon == types::canonical(dType)) {
            // 同型侧：字符串=独立副本（deepCopy）/句柄接管；聚合=原拷贝路径
            //   （含串字段 preFree/postCopy 深拷）；标量/指针/句柄=按 IR 宽度。
            if (sCanon == "字符串") {
                ir::IRValue v = emitResult(ir::Opcode::LoadPtr, {sAddr}, "ptr", "", loc);
                if (deepCopy) {
                    v = emitResult(ir::Opcode::Call, {v}, "ptr", "__cn_str_copy", loc);
                }
                emit(ir::Opcode::StorePtr, {dAddr, v}, ir::IRValue(), "", "ptr", loc);
                return;
            }
            if (semantic_->isStructType(sCanon) || types::isArray(sCanon) ||
                types::isI128(sCanon)) {
                emitStructCopyWithFields(dAddr, sAddr, sCanon, loc,
                                         /*preFree=*/false, deepCopy);
                return;
            }
            const std::string ir = mapType(sCanon);
            ir::IRValue v = emitResult(ir::Opcode::LoadPtr, {sAddr}, ir, "", loc);
            emit(ir::Opcode::StorePtr, {dAddr, v}, ir::IRValue(), "", ir, loc);
            return;
        }
        // 异型侧：标量数值/字符/枚举/布尔（入口已校验）——读源/Cast/写目标
        const std::string sIr = mapType(sType);
        const std::string dIr = mapType(dType);
        ir::IRValue v = emitResult(ir::Opcode::LoadPtr, {sAddr}, sIr, "", loc);
        ir::IRValue c = emitResult(ir::Opcode::Cast, {v}, dIr, "", loc);
        emit(ir::Opcode::StorePtr, {dAddr, c}, ir::IRValue(), "", dIr, loc);
    };
    const std::string okLabel = "bb" + std::to_string(blockCounter_++);
    const std::string errLabel = "bb" + std::to_string(blockCounter_++);
    const std::string joinLabel = "bb" + std::to_string(blockCounter_++);
    endBranch(tag.toString(), okLabel, errLabel);
    setCurrentBlock(newBlock(okLabel));
    emitPayloadConv(srcPayload, dstPayload, srcShape.valueType, dstShape.valueType);
    endJump(joinLabel);
    setCurrentBlock(newBlock(errLabel));
    if (dstShape.isResult) {
        emitPayloadConv(srcPayload, dstPayload, srcShape.errorType, dstShape.errorType);
    }
    endJump(joinLabel);
    setCurrentBlock(newBlock(joinLabel));
    return true;
}

}  // namespace cn_compiler
