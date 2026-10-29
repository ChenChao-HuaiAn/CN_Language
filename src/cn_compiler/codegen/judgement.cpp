// 平台指令选择判定层·实现（358 一期·归域核对实录见 judgement.hpp 头注）
// 每个判定分支的注释=抽取前的三方 if 链归域核对结论；改本文件前必读：
//   任何归域变化都可能破坏「asm 逐字节对拍零变化」硬验收（scripts/parity_dual_compile.py）。
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

// 1281：整型运算/比较的寄存器位宽（三后端单点）——i64/u64/ptr=64；
//   i8/i16/i32/u8/u16/u32/i1=32（窄整型按 32 位提升形态消费）；其余=0（不适用哨兵）。
int intOpWidthBits(const std::string& type) {
    if (type == "i64" || type == "u64" || type == "ptr") return 64;
    if (type == "i8" || type == "i16" || type == "i32" ||
        type == "u8" || type == "u16" || type == "u32" || type == "i1") {
        return 32;
    }
    return 0;
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
        (is64IntType(to) || to == "ptr" || to == "i32" || to == "u32" ||
         isNarrowIntType(to) || to == "i1")) {
        // 655（648 余量③·四族可达性枚举）：to 域增 ptr——i128→ptr 显式转换
        //   （指针(整128值)·语义层 srcNumeric&&dstPtr 放行）曾落 DefaultPass：
        //   win 体恒 32 位 mov 传递=只搬低 32 位（探针 p655-③ 实锤：i128
        //   0xAAAAAAAAAAAAAAAA → 指针 → 整64 回读=0，应 12297829382473034410；
        //   arm64/linux DefaultPass 体=64 位装载+按目标宽存=正确值=三后端
        //   分叉）。归域 I128ToNarrow（取低半槽·三后端发射体对 to=ptr 均
        //   64 位存：win mov rax/dst·linux qword·arm64 str xN/寄存器分配面
        //   i64/u64/ptr 同域）=与 i128→i64 位重解释同语义。
        return CastKind::I128ToNarrow;  // 302-a：须先于通用窄截断（否则读高半槽）
    }
    // ---- 655 DefaultPass 可达性枚举归档（648 余量③·四族逐一实证）----
    // 走到此处=（from,to）未落上方任何域。经 visitCastExpr（显式转换唯一
    //   用户通道·语义层 visitCastExpr 准入=数值族/指针族矩阵）逐一枚举：
    //   ①ptr→ptr：语义放行，但 IR 层 visitCastExpr 同型透传 no-op
    //     （274-a）不发射 Cast——不可达。
    //   ②i128→ptr：原落此（32 位 mov 截断缺陷）——已上移 I128ToNarrow 域。
    //   ③ptr→窄整（i32/u32/i8~u16）：语义放行（srcPtr&&dstNumeric）可达；
    //     win 体 32 位 mov 取低 32=窄整截断语义（探针 p655-③ 族3：整32(p)
    //     ==整32(整64(p)) 恒真）；arm64/linux 体=64 装载+按目标宽存=同值
    //     ——三后端值一致，落 DefaultPass=契约正确非巧合。
    //   ④整→i1（布尔目标）：dstNumeric 不含 布尔（isNumeric=整数+浮点族）
    //     ——语义层显式拒绝「无法显式转换为 布尔」，Cast 不可达（内部
    //     发射面无 to=i1 的 Cast·50 处枚举）。
    //   余下组合（i32→i32 同型透传等）均被前域或透传拦截——DefaultPass
    //   现存可达域仅族③，值语义三后端一致。
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

// 528（475 甲案收族·Rust as 饱和对齐）：浮→整（≤64 全宽）运行时饱和辅助符号名
//   ——三后端唯一份（464 浮→i128 专用 ABI 通道同构收族；runtime __cn_fXX_to_YY
//   族同名单源）。调用方保证 from∈{f32,f64}、to∈全窄/64 整族（classifyCast
//   FloatToInt 归域）；未登记 to 防御式落 i64（不发射=调用方 bug，映射自身无分支歧义）。
std::string floatToIntHelperName(const std::string& from, const std::string& to) {
    std::string name = (from == "f64") ? "__cn_f64_to_" : "__cn_f32_to_";
    if (to == "i8")  return name + "i8";
    if (to == "i16") return name + "i16";
    if (to == "i32") return name + "i32";
    if (to == "u8")  return name + "u8";
    if (to == "u16") return name + "u16";
    if (to == "u32") return name + "u32";
    if (to == "u64") return name + "u64";
    return name + "i64";  // i64 及未登记防御兜底
}

} // namespace judgement
} // namespace cn_compiler
