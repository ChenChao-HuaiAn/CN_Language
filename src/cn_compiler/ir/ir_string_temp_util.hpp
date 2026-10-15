// 字符串临时收口族共享判据（645/663/732/758 链）——自 ir_generator.hpp 迁出
//   （758 附属 2026-10-15·hpp 1000 冻结线腾行；ir_stmt_decl_ctx.hpp 小头同款
//   模式）。判据只读块指令表与寄存器 id，不依赖 IRGenerator 可变状态=纯自由
//   函数（成员壳 isConcatResultReg/freeIfConcatTemp 随之降级删除·调用点直用）。
#pragma once

#include "ir.hpp"

namespace cn_compiler {

// 定义点溯源：寄存器 regId 在块内倒序首条定义是否连接产物调用——
//   __cn_str_concat（逐节连接产物）或 __cn_strbuf_finish（728o 融合路径产物·
//   唯一返回堆串的 strbuf 符号·758 附属扩集）；其余符号（new=句柄/append=void/
//   drop=弃置）与非连接调用定义=假。id<0（变量/常量形态·IRValue 默认 -1）或
//   查无定义（跨块形态）=false=保守不 free（漏放不误放·同 663 带）。
inline bool isConcatResultInst(const ir::IRBlock* block, int regId) {
    if (block == nullptr || regId < 0) return false;
    const auto& insts = block->instructions;
    for (auto it = insts.rbegin(); it != insts.rend(); ++it) {
        if (it->result.id == regId) {
            return it->opcode == ir::Opcode::Call &&
                   (it->extra == "__cn_str_concat" ||
                    it->extra == "__cn_strbuf_finish");
        }
    }
    return false;
}

} // namespace cn_compiler
