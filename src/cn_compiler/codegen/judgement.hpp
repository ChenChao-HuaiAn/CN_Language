// 平台指令选择判定层（358 一期·plans/001 §阶段6「后端抽象层+TD 映射」立法补课）
// 职责：三后端（win-x64 / linux-x86_64 / linux-arm64）共享的「架构无关纯判定函数族」——
//   类型谓词（宽度秩面）/ 类型转换矩阵（857 分支序固化）/ i1 常量文本归一。
// 边界：只做「输入类型 → 判定类别」的纯函数；任何汇编助记符/寄存器名/寻址语法
//   的发射方言仍留在各后端 instructions（性能护城河·358 用户裁决原文）。
// 等价性铁律：本层判定结果与抽取前三后端各自内联 if 链的「首个命中分支」逐域一致；
//   验收=scripts/parity_358.py 三后端 asm 逐字节对拍零变化（358 硬验收面）。
// 已探明并如实保留的历史分叉（勿在本层「顺手修齐」——语义变更须立法先行、另轮处理）：
//   i32→u64 整型转换：win-x64 零扩展（I32ToU64 独立体）vs linux-x86_64/arm64
//   符号扩展（与 I32ToI64 同体）——021 已立案待用户裁决，本层按现状分域保留。
#pragma once

#include <string>

namespace cn_compiler {
namespace judgement {

// ---- 类型谓词族（宽度秩计算面·三后端原内联判定单点化）----
bool isFloatType(const std::string& type);        // f32|f64
bool isNarrowIntType(const std::string& type);    // i8|i16|u8|u16（窄整：装载即扩展面）
bool is128Type(const std::string& type);          // i128|u128（双槽类型）
bool is64IntType(const std::string& type);        // i64|u64（64 位整槽）
bool isSignedSourceType(const std::string& type); // i8|i16|i32|i64（符号扩展源域）
bool isTrueLikeI1(const std::string& text);       // i1 常量文本「真|1」两形态（322 归一）

// ---- 类型转换矩阵（指令选择判据面·emitCast 三后端 if 链的枚举化）----
// 原三后端 if 链中「分支优先序」承载语义（857/302-a 教训：特例须先于通例，否则
//   窄8/16→i128 被通例截胡只发低半槽）。本枚举判定把序固化为一一归域——
//   调用方 switch 无序，截胡类缺陷在结构上绝根。
// 判定顺序即原 arm64/linux 链序（两份一致方），win 链序差异处均为互斥域（已逐域核对）。
enum class CastKind {
    FloatToI128Helper,      // 浮 -> i128/u128（运行时辅助·须先于 FloatToInt·122-a）
    FloatToInt,             // 浮 -> ≤64 整（fcvtzs/cvttss2si 截断·to 任意整槽）
    FloatIdentity,          // 浮恒等 from==to（274-a：不插转换直拷）
    FloatToFloat,           // f32<->f64 降/升精度
    IntToFloatI128Helper,   // i128/u128 -> 浮（运行时辅助）
    IntToFloatU64Helper,    // u64 -> 浮（无符号语义辅助）
    IntToFloat,             // 其余整 -> 浮（win 侧 u32/i64 特例在 case 体内）
    PtrIntReinterpret,      // ptr <-> i64/u64（位重解释）
    Int64Reinterpret,       // i64/u64 <-> i64/u64（位重解释·win 独立分支、arm64 原与上同分支）
    I128SameCopy,           // i128/u128 -> 同类（双槽复制）
    IntToI128,              // 8 整族 -> i128/u128（宽化双槽·高半按源符号性）
    OtherToI128,            // 其余 -> i128/u128（ptr/i1 等·兜底宽化）
    NarrowSourceExtend,     // i8/i16/u8/u16 -> 宽（装载即扩展·to 已非 128）
    I128ToNarrow,           // i128/u128 -> 全窄整/i1（取低半·须先于通用截断·302-a）
    TruncateTo8,            // -> i8/u8（写低字节）
    TruncateTo16,           // -> i16/u16（写低 16 位）
    I1To64,                 // i1 -> i64/u64（零扩展）
    I1To32,                 // i1 -> i32/u32（零扩展·win 既有分支）
    I32ToI64,               // i32 -> i64（符号扩展）
    I32ToU64,               // i32 -> u64（⚠历史分叉见头注：win 零扩 / arm64+linux 符号扩）
    U32ToI64,               // u32 -> i64（零扩展）
    U32ToU64,               // u32 -> u64（零扩展·写 32 位天然清高半）
    U64ToI64,               // u64 -> i64（64 位位重解释·win 既有分支）
    I64ToI32,               // i64 -> i32（截断）
    DefaultPass,            // 兜底：装载-存储直传（三方原 default 体各自保留）
};
CastKind classifyCast(const std::string& from, const std::string& to);

} // namespace judgement
} // namespace cn_compiler
