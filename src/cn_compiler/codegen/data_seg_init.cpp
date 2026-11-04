// 2051：三后端 .data 静态数组初始化列表共享打包实现（声明见 data_seg_init.hpp）
#include "cn_compiler/codegen/data_seg_init.hpp"

#include "cn_compiler/model/semantic_const_eval.hpp"

namespace cn_compiler {
namespace codegen {

cn::Optional<std::vector<unsigned long long>> packStaticArrayInitQuads(
    const std::vector<std::string>& elemTexts, std::size_t elemBytes,
    std::size_t slotBytes) {
    // 元素宽度域：1/2/4/8 字节标量；槽尺寸=8 的倍数（#259 数组分配口径）
    if (elemBytes == 0 || elemBytes > 8 || elemBytes > slotBytes ||  // 静默豁免业务值: 搬移既有面（锚随代码搬移·原文件基线放行·2051 拆分连带）
        slotBytes == 0 || slotBytes % 8 != 0) {
        return std::nullopt;
    }
    // 槽字节图（零初始化=缺格补零·1473 甲案零位默认态）
    std::vector<unsigned char> bytes(slotBytes, 0);
    std::size_t off = 0;
    for (const std::string& t : elemTexts) {
        if (off + elemBytes > slotBytes) return std::nullopt;  // 超槽=个数校验失守（语义层已拒·内部缺陷显形）
        long long v = 0;
        // 非整数文本（浮点/字符串）=不支持直存（调用方分流运行期注入通道）
        if (!cnParseInt(t, v)) return std::nullopt;
        // 两补码低位字节序（元素按宽度截取——语义层 919/756 值域门槛已保证域内）
        const unsigned long long u =
            static_cast<unsigned long long>(v);
        for (std::size_t b = 0; b < elemBytes; ++b) {
            bytes[off + b] =
                static_cast<unsigned char>((u >> (8 * b)) & 0xFFULL);
        }
        off += elemBytes;
    }
    // 8 字节组小端 → uint64（发射端 .quad/.dq 再次小端写出=字节序保持）
    std::vector<unsigned long long> quads(slotBytes / 8, 0);
    for (std::size_t q = 0; q < quads.size(); ++q) {
        unsigned long long u = 0;
        for (std::size_t b = 0; b < 8; ++b) {
            u |= static_cast<unsigned long long>(bytes[q * 8 + b]) << (8 * b);
        }
        quads[q] = u;
    }
    return quads;
}

}  // namespace codegen
}  // namespace cn_compiler
