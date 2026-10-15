// Win x64 代码生成·栈帧管理族：prologue（大帧 __chkstk 探测）/参数装载/
//   epilogue/溢出陷阱与错误序列。
// #836 先拆再加：x64_codegen.cpp 已顶 1000 行硬红线（999 行），按「先拆再加」
//   纪律将栈帧构建内聚职责拆出本文件——纯搬运零语义变化（函数体逐字未动）。
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {

// 生成函数 prologue（push rbp / mov rbp,rsp / 预留栈帧 / 保存被调用者保存寄存器）
// Debug 子任务（Task 6.11 后续）修复：大栈帧无 __chkstk 栈探测崩溃——
//   Windows 栈 guard 页机制：`sub rsp, N`（N > 4KB）一次性越过已提交栈页，
//   访问未提交页 -> 0xC0000005（函数无任何输出即崩）。MSVC 惯例：栈帧
//   > 4KB（保守阈值，实际 guard 页 + 已提交页约 8KB）时必须先 `call __chkstk`
//   按 4KB 页逐步提交栈空间（__chkstk 契约：rax=所需字节数，返回后 rsp 已减）。
//   __chkstk 会破坏 rax/rcx/r10/r11（MSVC chkstk.asm volatile 集合），
//   因此隐藏返回指针（rcx -> r12）的保存必须在其之前执行。
void X64CodeGenerator::emitPrologue(AsmWriter& writer, const ir::IRFunction& function) {
    int frameSize = computeFrameSize(function);
    // 阶段C（Task 4.3）栈对齐修复：被调用者保存寄存器（rbx/r12~r15）在
    //   sub rsp 之后压栈，若 push 数量为奇数，函数体内 rsp≡8 (mod 16)，
    //   call 前 rsp 不再 16 字节对齐，违反 Win x64 ABI——被调方（MSVC 编译
    //   的运行时）用 movaps/movdqa 对齐访问立即崩溃（0xC0000005）。
    //   正确约束：sub 大小 F 满足 (F + 8N) ≡ 0 (mod 16)；computeFrameSize
    //   已保证 F ≡ 0 (mod 16)，故 N 为奇数时 F 额外 +8。
    if (calleeSavedRegs_.size() % 2 == 1) {
        frameSize += 8;
    }
    // 2026-08 自举检查修复：帧内固定 16 字节返回缓冲区（调用方侧）——
    //   原实现在 emitCall 用 rsp 临时区（[rsp+32]），add rsp 后结果指针悬垂，
    //   调用方跨调用读 .值 读到被覆盖的垃圾（结果/可选 返回 + 空类型结果场景）。
    //   每函数统一预留 16 字节（帧底，sub 后即 [rsp+0..15]，与调用影子空间
    //   [新rsp+0..31]=[旧rsp-32..] 不冲突；递归同函数嵌套覆盖为已知限制）。
    frameSize += 16;
    retbufFrameOffset_ = -frameSize;
    // 2026-08（自举阶段7 修复）：大栈帧（>4KB）参数/返回指针的保存一律延后到
    //   call __chkstk 之后——参数槽 [rbp-offset] 深达帧底，__chkstk 之前写入
    //   会一次性越过 guard 页（未探测区域）直接访问违例（0xC0000005，构造
    //   36KB 栈帧参数槽写入实测）。__chkstk 破坏 rcx/rdx/r8/r9，故先将
    //   整型参数寄存器保存到调用方影子空间（入口 [rsp+8..+40]，Win x64 ABI
    //   保证已提交且归被调方自由使用），__chkstk 完成探测分配后从影子空间
    //   （push rbp 后即 [rbp+16..+48]）搬运。xmm0-3 不在 __chkstk volatile
    //   集合（仅 rax/rcx/r10/r11），浮点参数无需预存。
    const bool useShadowParams =
        frameSize > 4096 &&
        (!function.params.empty() || function.structReturn ||
         function.returnType == "i128" || function.returnType == "u128");
    if (useShadowParams) {
        writer.comment("大栈帧：整型参数寄存器预存影子空间（__chkstk 破坏 rcx/rdx/r8/r9）");
        writer.line("mov [rsp+8], rcx");
        writer.line("mov [rsp+16], rdx");
        writer.line("mov [rsp+24], r8");
        writer.line("mov [rsp+32], r9");
    }
    writer.line("push rbp");
    writer.line("mov rbp, rsp");
    if (frameSize > 0) {
        // 栈帧 > 4KB：MSVC 惯例三段式——mov rax, N / call __chkstk / sub rsp, rax。
        //   __chkstk 契约（MSVC x64 CRT chkstk.asm）：rax = 要探测的字节数；
        //   **只按 4KB 页 touch 栈页触发栈增长，不修改 rsp**（"The stack pointer
        //   is not adjusted"），且返回后 rax 仍为输入值；随后编译器自己
        //   `sub rsp, rax` 完成栈帧分配。frameSize 已 16 对齐，sub 后 rsp
        //   保持 16 字节对齐，满足 Win x64 ABI。
        if (frameSize > 4096) {
            writer.line("mov rax, " + std::to_string(frameSize));
            writer.comment("栈帧 " + std::to_string(frameSize) +
                           " 字节 > 4KB：call __chkstk 按页探测提交");
            writer.line("call __chkstk");
            writer.line("sub rsp, rax");
        } else {
            writer.line("sub rsp, " + std::to_string(frameSize));
        }
    }
    // 隐藏返回指针：__chkstk 完成后从影子空间（大帧）或寄存器（小帧）读入返回槽
    if (function.structReturn || function.returnType == "i128" ||
        function.returnType == "u128") {
        if (useShadowParams) {
            writer.line("mov rax, [rbp+16]");  // 影子空间保存的 rcx
            writer.line("mov [rbp" + std::to_string(retbufSlotOffset_) + "], rax");
        } else {
            writer.line("mov [rbp" + std::to_string(retbufSlotOffset_) + "], rcx");
        }
    }
    // 参数槽写入：大帧从影子空间读（__chkstk 破坏 rcx/rdx/r8/r9），小帧直接读寄存器
    emitParamSetup(writer, function, useShadowParams);
    // 阶段C（Task 4.3）：寄存器分配使用的被调用者保存寄存器（rbx/r12~r15）压栈保存
    //   ——分配器只使用被调用者保存寄存器，调用者不期望其被修改，须保存/恢复。
    //   注：压栈顺序固定（rbx/r12/r13/r14/r15 顺序），恢复时逆序 pop。
    //   r12 若被分配器使用（无隐藏返回指针场景），同样在此保存。
    emitSaveCalleeSaved(writer);
}

// 生成函数参数加载：前4参数从寄存器存入参数槽，第5起直接从栈读（无需搬运）
// 栈参数偏移计算：caller 在 call 前预留 32 字节影子空间，第5参数位于 caller_rsp+32；
// caller_rsp = rbp + 16（push rbp + mov rbp,rsp 后，rbp = caller_rsp - 16）
// 故第 i（>=4）参数位于 [rbp + 48 + (i-4)*8]
// 参数槽名使用 paramUniques（唯一内部名，与 Alloca 的 extra 一致），
// 保证遮蔽参数/变量的引用与槽登记一致
void X64CodeGenerator::emitParamSetup(AsmWriter& writer, const ir::IRFunction& function,
                                    bool useShadowParams) {
    // Task 完善A：结构体返回值函数——隐藏返回指针（rcx）占第一个整型参数位，
    //   真实参数从 index 1 起（Win x64 ABI）
    // 修复（集成验证发现）：i128/u128 返回同样占用 rcx 作为隐藏返回指针，
    //   结构体按值参数须从 index 1 起读取（与结构体返回一致）
    // useShadowParams（2026-08 自举阶段7）：大栈帧函数经 __chkstk 探测后，
    //   整型参数寄存器（rcx/rdx/r8/r9 被 __chkstk 破坏）已预存调用方影子空间
    //   （push rbp 后即 [rbp+16]/[+24]/[+32]/[+40]），此处从影子空间读源；
    //   浮点参数 xmm0-3 不在 __chkstk volatile 集合，直接读寄存器。
    const std::size_t paramOffset =
        (function.structReturn || function.returnType == "i128" ||
         function.returnType == "u128") ? 1 : 0;
    // 整型/指针参数源（前4位）：影子空间（大帧）或原寄存器（小帧）
    auto intParamSrc = [this, useShadowParams](int actualIdx) -> std::string {
        if (useShadowParams) {
            return "[rbp+" + std::to_string(16 + 8 * actualIdx) + "]";
        }
        return parameterRegister(actualIdx);
    };
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        // 参数唯一名（paramUniques 与 params 一一对应，防御性回退到源码名）
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        int slotOffset = varSlotOf(unique);
        std::string slot = "[rbp" + std::to_string(slotOffset) + "]";
        const std::string& paramType = function.params[i].second;
        // 结构体按值参数（Task 完善A）：IR 层参数类型为 ptr（指向调用方传入的
        //   结构体临时副本地址），此处将指针指向的 结构体数据 拷贝到本函数参数槽
        //   （结构体变量占 槽数 = size/8，按源码类型大小拷贝——见 varSlots 登记）
        if (paramType == "ptr" && function.structParamIndexes.count(static_cast<int>(i)) > 0 &&
            function.varSlots.count(unique) > 0 && function.varSlots.at(unique) >= 1) {
            // 参数是结构体（多槽登记）：传入指针在 寄存器/栈（隐藏返回指针占位时偏移）
            std::string srcPtr;
            if (i + paramOffset < 4) {
                srcPtr = intParamSrc(static_cast<int>(i + paramOffset));
            } else {
                srcPtr = "[rbp+" + std::to_string(48 + (static_cast<int>(i + paramOffset) - 4) * 8) + "]";
            }
            // 拷贝 size = 槽数*8 字节到参数槽（槽0 = 基址，槽1..N-1 上方）
            const int bytes = function.varSlots.at(unique) * 8;
            writer.line("mov rsi, " + srcPtr);            // 源
            writer.line("lea rdi, " + slot);              // 目标（槽0）
            writer.line("mov rcx, " + std::to_string(bytes));  // 字节数
            writer.line("rep movsb");
            writer.comment("结构体参数 " + function.params[i].first + " 拷贝 " +
                           std::to_string(bytes) + " 字节");
            continue;
        }
        // 参数实际位号：整型/指针/i128 参数受隐藏返回指针（rcx 占位）影响，
        //   位号 = i + paramOffset（Win x64 ABI）；浮点参数同样按「参数位」配对
        //   xmm0-3（381 旁证根治：微软 ABI 的 xmm 与整型寄存器按位置一一配对——
        //   隐藏返回指针占位后浮点位号同被推移；调用侧 emitCall 已按参数位
        //   （x64_instructions_call.cpp「N=regIdx」·MSVC FormatFloat 反汇编实证），
        //   本侧原用参数索引 i —— 无 retbuf 时 i==位号两侧一致（修复6 用例全绿），
        //   结构体返回容器方法（结果<T,E> 走 retbuf）的浮点参数被调读 xmm(i)
        //   调用放 xmm(i+1) 错位丢值：向量<浮64>.追加/设置 读回恒 0（669 JSON
        //   浮点全丢探针+p3.asm 铁证：调用 movsd xmm2 / 被调读 xmm1）。
        //   （修复集成审查 BUG #2 的 i128 位号修复不受影响——整型分支本就
        //   用 actualIdx）
        const int actualIdx = static_cast<int>(i) + static_cast<int>(paramOffset);
        if (isFloatType(paramType)) {
            if (actualIdx < 4) {
                // 修复6（浮点参数）：Win x64 浮点参数经 xmm0-3 传递，
                // 原实现从 rcx/rdx/r8/r9（整型寄存器）读取——读到垃圾值。
                // 浮点值存 8 字节槽（f32 只低 4 字节有效），movsd/movss 从 xmmN 存槽
                // （xmm0-3 不在 __chkstk volatile 集合，大帧同样直接读）
                const std::string store = (paramType == "f64") ? "movsd" : "movss";
                const std::string mp = (paramType == "f64") ? "qword ptr " : "dword ptr ";
                const std::string xmm = "xmm" + std::to_string(actualIdx);
                writer.line(store + " " + mp + slot + ", " + xmm);
            } else {
                // 浮点栈参数（第5位起）：[rbp+48+(位号-4)*8]——与整型栈参同口径
                const std::string stackSrc =
                    "[rbp+" + std::to_string(48 + (actualIdx - 4) * 8) + "]";
                writer.line("mov rax, " + stackSrc);
                writer.line("mov " + slot + ", rax");
            }
        } else if (actualIdx < 4) {
            if (paramType == "i128" || paramType == "u128") {
                // i128 参数（集成验证修复 Bug）：调用方传双槽地址指针（低64位槽地址），
                //   原实现只存 8 字节（mov reg）——高64位丢失 -> i128 参数值错误。
                //   此处从指针地址拷贝 16 字节到参数双槽（槽0=低64位、槽1=高64位，
                //   与 registerVarSlots 的 2 槽登记一致：槽1 在槽0 上方 8 字节）
                const std::string srcPtr = intParamSrc(actualIdx);
                writer.line("mov rsi, " + srcPtr);          // 源：i128 双槽地址
                writer.line("lea rdi, " + slot);            // 目标：参数槽0
                writer.line("mov rcx, 16");
                writer.line("rep movsb");
                writer.comment("i128 参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                // 前4整型/指针参数：寄存器 -> 栈槽（隐藏返回指针占位时偏移 paramOffset）
                if (useShadowParams) {
                    // 大帧：从影子空间读（__chkstk 已破坏原寄存器），经 rax 中转
                    writer.line("mov rax, [rbp+" +
                                std::to_string(16 + 8 * actualIdx) + "]");
                    writer.line("mov " + slot + ", rax");
                } else {
                    std::string reg = parameterRegister(actualIdx);
                    std::string width = widthFor(paramType, reg);
                    writer.line("mov " + slot + ", " + width);
                }
            }
        } else {
            // 第5参数位起：从调用者栈帧拷贝到本函数参数槽
            // 偏移 = 48 + (actualIdx-4)*8（第5参数位 actualIdx=4 位于 [rbp+48]，
            // 第6参数位 actualIdx=5 位于 [rbp+56]，依此类推）
            const std::string stackSrc =
                "[rbp+" + std::to_string(48 + (actualIdx - 4) * 8) + "]";
            if (paramType == "i128" || paramType == "u128") {
                // i128 栈参数（集成验证修复）：调用方栈槽存放的是双槽地址指针，
                //   从指针拷贝 16 字节到参数双槽（与寄存器 i128 参数一致）
                writer.line("mov rsi, " + stackSrc);      // 源：i128 双槽地址
                writer.line("lea rdi, " + slot);          // 目标：参数槽0
                writer.line("mov rcx, 16");
                writer.line("rep movsb");
                writer.comment("i128 栈参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                writer.line("mov rax, " + stackSrc);
                writer.line("mov " + slot + ", rax");
            }
        }
        writer.comment("参数 " + function.params[i].first + " -> " + slot);
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
// 修复7（浮点返回）：Win x64 浮点返回值经 xmm0 传递。
//   原实现一律 mov rax（整型寄存器）——浮点函数返回后 caller 读 xmm0
//   得到残留垃圾值（09集成用例曾靠运气通过）。返回类型为浮点时
//   用 movsd/movss 把返回值槽搬到 xmm0。
void X64CodeGenerator::emitEpilogue(AsmWriter& writer, const std::string& returnReg) {
    // 结构体返回值（Task 完善A）：被调方把 returnReg（结构体地址）指向的数据
    //   拷贝到隐藏返回缓冲区（r12 保存的入口 rcx 值），并返回缓冲区指针（rax）。
    //   入口 prologue 已执行 "mov r12, rcx"（保存隐藏返回指针）。
    //   结构体大小从 IRFunction 不可直接得，用约定 64 字节上限拷贝。
    if (currentStructReturn_ && !returnReg.empty()) {
        const std::string rbSlot = "[rbp" + std::to_string(retbufSlotOffset_) + "]";
        writer.line("mov rax, " + rbSlot);           // 缓冲区地址（入口保存到栈槽）
        writer.line("mov rsi, " + returnReg);        // 源：结构体地址
        writer.line("mov rdi, " + rbSlot);           // 目标：返回缓冲区
        // 按精确大小拷贝（structReturnSize，如 16 字节）——避免 64 字节硬编码
        //   越界写破坏相邻栈变量（班级 16 字节被写 64 字节越界 48 字节）
        const int copyBytes = (currentStructReturnSize_ > 0)
                                  ? currentStructReturnSize_ : 16;
        writer.line("mov rcx, " + std::to_string(copyBytes));
        writer.line("rep movsb");
        writer.comment("结构体返回：按 " + std::to_string(copyBytes) +
                       " 字节拷贝到隐藏返回缓冲区");
        // 公共 epilogue 尾部：恢复栈帧并返回（不可漏掉 ret，
        //   否则执行流落入下一函数 PROC 造成无限递归栈溢出 0xC00000FD）
        writer.line("mov rsp, rbp");
        writer.line("pop rbp");
        writer.line("ret");
        return;
    }
    if (!returnReg.empty()) {
        if (currentReturnType_ == "i128" || currentReturnType_ == "u128") {
            // i128 返回（Task 完善A）：调用方隐藏指针指向返回缓冲区，
            //   被调方将返回值双寄存器拷贝到返回缓冲区（rax 指向缓冲区）
            //   returnReg = regSlot(hiId)（高64位槽），低64位槽 = regSlot(hiId+1）
            //   regSlot(id) 文本 = "[rbp-{8*id+8}]"（如 id=1 -> "[rbp-16]"）
            int hiId = -1;
            if (returnReg.size() > 6 && returnReg.compare(0, 5, "[rbp-") == 0) {
                const std::string num = returnReg.substr(5);  // 如 "8]" / "16]"
                const std::size_t rb = num.rfind(']');
                if (rb != std::string::npos) {
                    const int off = std::stoi(num.substr(0, rb));  // 8*id+8
                    hiId = (off - 8) / 8;
                }
            }
            if (hiId >= 0) {
                // 隐藏返回指针在 rcx（调用方传入）；prologue 已保存到专用栈槽
                //   （A-4 2026-08：原存 r12，内层函数入口覆盖物理 r12）。
                // 修复（集成验证发现）：函数体内调用会破坏 rcx，必须用栈槽恢复，
                //   否则 epilogue 把已破坏的 rcx 当缓冲区地址写入 -> 崩溃（0xC0000005）
                const std::string rbSlot2 = "[rbp" + std::to_string(retbufSlotOffset_) + "]";
                writer.line("mov rax, " + rbSlot2);  // 返回缓冲区地址（入口保存）
                writer.line("mov rdx, " + regSlot(hiId + 1));  // 低64位
                writer.line("mov [rax], rdx");
                writer.line("mov rdx, " + regSlot(hiId));      // 高64位
                writer.line("mov [rax+8], rdx");
                writer.line("mov rax, " + rbSlot2);  // ABI：返回缓冲区指针放 rax
            }
        } else if (currentReturnType_ == "f64" || currentReturnType_ == "f32") {
            const std::string load = (currentReturnType_ == "f64") ? "movsd" : "movss";
            const std::string mp = (currentReturnType_ == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + returnReg);
        } else {
            // 111-a（2026-09-13）：按返回类型宽度装载（与 linux 后端
            //   emitStackLoad(..., "rax", currentReturnType_) 对齐）——≤32 位返回类型
            //   （i1/i8/i16/i32/u8/u16/u32）用 32 位读（mov eax，32 位写自动清零
            //   rax 高 32 位），保证 ABI 契约「窄返回值高位干净」（Rust/LLVM
            //   zeroext 语义）；原无条件 8 字节读（mov rax）使 rax 高 32 位携带
            //   栈垃圾——v2 win 后端消费者按 64 位比较（cmp rax, 0）时被垃圾
            //   污染（E2E 221 形态①实证：映射$包含 布尔返回 → 校验位丢失）。
            //   槽内窄类型为 32 位提升形式（widthFor 注释同款纪律），故 32 位读
            //   与槽内实际有效宽度一致。
            writer.line("mov " + widthFor(currentReturnType_, "rax") + ", " + returnReg);
        }
    }
    // 阶段C（Task 4.3）：恢复被调用者保存寄存器（逆序 pop，与 prologue 压栈相反）
    //   structReturn 提前 return 路径已由 regAllocUsed_=false 保证不在此恢复
    emitRestoreCalleeSaved(writer);
    writer.line("mov rsp, rbp");
    writer.line("pop rbp");
    writer.line("ret");
}

// 任务 474：溢出错误出口（错误码 8『溢出错误』）——冷路径错误块发射。
//   与除零（错误码1）/判空（错误码3）同款出口序列：错误码入 rcx（Win64 ABI
//   首参）+ 影子空间 + __cn_runtime_error + ret；热路径以几乎不跳转的
//   条件分支（jo/jc/ja）跳入。
void X64CodeGenerator::emitOverflowTrap(AsmWriter& writer, const std::string& label) {
    writer.raw(label + ":");
    emitOverflowErrorBody(writer);
}

// 溢出错误序列（无标签）：错误码 8 入 rcx（Win64 ABI 首参）+ 影子空间 + 终止
void X64CodeGenerator::emitOverflowErrorBody(AsmWriter& writer) {
    writer.line("mov rcx, 8");
    writer.line("sub rsp, 32");
    writer.line("call __cn_runtime_error");
    writer.line("add rsp, 32");
    writer.line("ret");
}

} // namespace cn_compiler
