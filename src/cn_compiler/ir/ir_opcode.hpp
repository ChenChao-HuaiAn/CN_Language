// CN-IR 指令操作码（357 重构F2 自 ir.hpp 纯机械搬移·枚举体逐字节保留）
// 职责：Opcode 枚举（规格书7.3 指令分类）单一归属——ir.hpp include 本头，下游透明。
#pragma once

namespace cn_compiler {
namespace ir {

// IR指令操作码（规格书7.3指令分类，阶段一子集 + Task 2.3 类型系统完善）
enum class Opcode {
    // ---- 常量加载 ----
    ConstInt,       // 加载整数常量（extra=十进制值）
    ConstFloat,     // 加载浮点常量（extra=文本值）
    ConstString,    // 加载字符串常量（extra=常量池ID，如 @str0）
    ConstBool,      // 加载布尔常量（extra=真/假）

    // ---- 算术运算 ----
    Add,            // 整数/浮点加法
    Sub,            // 整数/浮点减法
    Mul,            // 整数/浮点乘法
    Div,            // 整数/浮点除法
    Mod,            // 整数取余

    // ---- 位运算（Task 2.3 新增，整型专用） ----
    BitAnd,         // 按位与 &
    BitOr,          // 按位或 |
    BitXor,         // 按位异或 ^
    Shl,            // 左移 <<
    Shr,            // 右移 >>（有符号算术右移；无符号逻辑右移由codegen按类型分派）

    // ---- 比较运算（结果为布尔） ----
    Eq,             // ==
    Ne,             // !=
    Lt,             // <
    Le,             // <=
    Gt,             // >
    Ge,             // >=

    // ---- 逻辑运算 ----
    And,            // &&（短路，阶段一简化非短路）
    Or,             // ||（短路，阶段一简化非短路）
    Not,            // !

    // ---- 类型转换（Task 2.3 新增） ----
    // 扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
    // 目标类型存 inst.type，源类型为 operand[0].type
    Cast,           // 类型转换（extra 为空）

    // ---- 内存操作 ----
    Copy,           // 寄存器搬运（F1-26 方案 A，2026-09-16）：operand[0]=源寄存器，
                    //   结果为目标寄存器；Phi 降级（前驱块尾并行拷贝）产物
    Load,           // 从变量加载（operand[0]=变量名）
    Store,          // 存储到变量（operand[0]=值寄存器, extra=变量名）
    Alloca,         // 栈上分配（extra=变量名）
    AddrOf,         // 取地址（Task 2.4）：operand[0]=变量引用，结果为该变量地址（ptr）
    LoadPtr,        // 通过指针值加载（Task 2.4）：operand[0]=指针寄存器/常量，结果类型 inst.type
    StorePtr,       // 通过指针值存储（Task 2.4）：operand[0]=目标地址(ptr), operand[1]=值
    FieldAddr,      // 结构体字段地址（Task 2.7）：operand[0]=结构体基址(ptr)，
                    //   extra=字段偏移字节（十进制），结果为该字段地址（ptr）；
                    //   对 -> 访问隐含空指针检查（错误码3）
    CopyStruct,     // 结构体整体赋值（Task 完善A）：operand[0]=目标地址(ptr),
                    //   operand[1]=源地址(ptr)，extra=拷贝字节数（十进制），无结果
    StrFieldAddr,   // 991（008 挂账②·094 空安全）：串字段地址=FieldAddr 判空豁免形态
                    //   （operand[0]=基址(ptr)，extra=偏移字节，结果=字段地址(ptr)）——
                    //   串句柄 0=空串（094 空安全语义·运行时串函数已容错），不触发
                    //   错误码 3；仅限字符串类型字段（生成层类型知情专发）

    // ---- 控制流 ----
    Jump,           // 无条件跳转（extra=目标块标签）
    Branch,         // 条件跳转（operand[0]=条件寄存器, extra=真块|假块）
    Call,           // 直接函数调用（extra=函数名）
    CallIndirect,   // 间接调用函数指针（operand[0]=指针寄存器，Task 2.2）
    Return,         // 返回（operand[0]=返回值寄存器，可为空）

    // ---- 阶段3 OOP：对象/虚调用（Task 3.1/3.2，规格书06） ----
    NewObject,      // 新建对象（Task 3.1）：operand[0]=类名（extra=类名+对象大小），
                    //   结果=对象指针；codegen 分配堆内存并初始化虚表指针
    DeleteObject,   // 删除对象（Task 3.1）：operand[0]=对象指针，无结果；
                    //   codegen 调用析构并释放内存
    VirtualCall,    // 虚调用（Task 3.2，规格书06-四/五）：operand[0]=对象指针，
                    //   operand[1..]=实参，extra=虚函数名（codegen 查虚表槽位间接跳转），
                    //   结果类型 inst.type（返回类型）
    VtableAddr,     // 加载虚表地址（Task 3.2）：operand[0]=对象指针，结果=虚表指针；
                    //   供 codegen 初始化子类虚表/虚调用前取表

    // ---- 其他 ----
    FuncAddr,       // 加载函数地址（extra=函数名，Task 2.2 函数指针赋值）
    Phi,            // Phi节点（SSA汇合点，阶段一预留）
};

} // namespace ir
} // namespace cn_compiler
