// 平台指令选择判定层·实现（358 一期·归域核对实录见 judgement.hpp 头注）
// 每个判定分支的注释=抽取前的三方 if 链归域核对结论；改本文件前必读：
//   任何归域变化都可能破坏「asm 逐字节对拍零变化」硬验收（scripts/parity_358.py）。
#include "cn_compiler/codegen/judgement.hpp"

namespace cn_compiler {
namespace judgement {

bool isFloatType(const std::string& type) {
    return type == "f32" || type == "f64";
}

bool isNarrowIntType(const std::string& type) {
    return type == "i8" || type == "i16" || type == "u8" || type == "u16";
}

bool is128Type(const std::string& type) {
    return type == "i128" || type == "u128";
}

bool is64IntType(const std::string& type) {
    return type == "i64" || type == "u64";
}

bool isSignedSourceType(const std::string& type) {
    return type == "i8" || type == "i16" || type == "i32" || type == "i64";
}

bool isTrueLikeI1(const std::string& text) {
    // 322：兼容「真|1」两形态 i1 文本（guard 写"1"曾被折算 0=守卫恒失效·三后端同构）
    return text == "真" || text == "1";
}

CastKind classifyCast(const std::string& from, const std::string& to) {
    const bool fromFloat = isFloatType(from);
    const bool toFloat = isFloatType(to);
    // ---- 浮点源域（原 arm64/linux 分支 1~4、win emitCastFloatFamily 前段）----
    if (fromFloat) {
        if (is128Type(to)) return CastKind::FloatToI128Helper;
        if (!toFloat) return CastKind::FloatToInt;
        if (from == to) return CastKind::FloatIdentity;
        return CastKind::FloatToFloat;
    }
    // ---- 浮点目标域（原分支 3a/3b/3c：辅助函数特例先于普通整->浮）----
    if (toFloat) {
        if (is128Type(from)) return CastKind::IntToFloatI128Helper;
        if (from == "u64") return CastKind::IntToFloatU64Helper;
        return CastKind::IntToFloat;
    }
    // ---- 指针/64 位重解释域（arm64 原分支 5 合并域、win PtrAndFloatPair 两分支——互斥序差已核对）----
    if (from == "ptr" && is64IntType(to)) return CastKind::PtrIntReinterpret;
    if (to == "ptr" && is64IntType(from)) return CastKind::PtrIntReinterpret;
    if (is64IntType(from) && is64IntType(to)) return CastKind::Int64Reinterpret;
    // ---- 128 位域（857 铁律固化：128 目标/同类先于窄源扩展与通用截断）----
    if (is128Type(from) && is128Type(to)) return CastKind::I128SameCopy;
    if (is128Type(to)) {
        // win 316-a 域=8 整族（i1/ptr 落其 W3m 兜底；arm64/linux 原分支 7 不分 from——
        //   两域在 arm64/linux 侧 case 合并同体）
        if (isSignedSourceType(from) || from == "u8" || from == "u16" ||
            from == "u32" || from == "u64") {
            return CastKind::IntToI128;
        }
        return CastKind::OtherToI128;
    }
    if (is128Type(from) &&
        (is64IntType(to) || to == "i32" || to == "u32" ||
         isNarrowIntType(to) || to == "i1")) {
        return CastKind::I128ToNarrow;  // 302-a：须先于通用窄截断（否则读高半槽）
    }
    // ---- 窄源扩展域（to 非 128 已由上方保证——win W3a 的 to!=128 排除条件等价固化）----
    if (isNarrowIntType(from)) return CastKind::NarrowSourceExtend;
    // ---- 截断域（原链 10/11：from 已非窄/128，域=「to 窄且前序全不中」）----
    if (to == "i8" || to == "u8") return CastKind::TruncateTo8;
    if (to == "i16" || to == "u16") return CastKind::TruncateTo16;
    // ---- i1 扩展域（win 既有 I1To64/I1To32 分支；arm64/linux 无 I1To32——落 DefaultPass）----
    if (from == "i1" && is64IntType(to)) return CastKind::I1To64;
    if (from == "i1" && (to == "i32" || to == "u32")) return CastKind::I1To32;
    // ---- 32↔64 逐向域（⚠ I32ToU64 历史分叉：win 零扩体 / arm64+linux 与 I32ToI64 同体）----
    if (from == "i32" && to == "i64") return CastKind::I32ToI64;
    if (from == "i32" && to == "u64") return CastKind::I32ToU64;
    if (from == "u32" && to == "i64") return CastKind::U32ToI64;
    if (from == "u32" && to == "u64") return CastKind::U32ToU64;
    if (from == "u64" && to == "i64") return CastKind::U64ToI64;
    if (from == "i64" && to == "i32") return CastKind::I64ToI32;
    // ---- 兜底（arm64/linux：64 位装载+按目标宽存；win：32 位 mov 传递——各自原 default 体）----
    return CastKind::DefaultPass;
}

} // namespace judgement
} // namespace cn_compiler
