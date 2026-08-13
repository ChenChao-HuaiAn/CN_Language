# lessons.md - AI错误记录与经验教训

> 本文件记录项目开发中遇到的问题、原因与解决方案，防止重复犯错。
> 权重规则：≥15 必须提及；10-14 应该阅读；5-9 可选；<5 可跳过。

## 高权重问题（必须避免）

- [2026-08-13 16:56] **问题类型**: 设计缺陷（权重 10.4）
  - **描述**: 规划"语言表达力增强"时，未经用户确认便引入两个新保留关键字 `运算符`（运算符重载）和 `别名`（类型别名），并错误定义 `打印`/`打印行` 的换行语义（写成 `打印` 不换行、`打印行` 换行）。用户逐条批阅后否决：① 不引入 `运算符` 保留关键字；② 不引入 `别名` 关键字与功能；③ `打印` 应自动换行、`打印行` 应不换行。
  - **原因**: ① 盲目对标 C++ 的 `operator` 保留字策略，未考虑 CN 语言中文命名的"上下文关键字"更优方案（`函数 运算符+` 可凭上下文识别，无需占用标识符命名空间）；② 类型别名功能被误认为必需，实际现有 `整数`/`小数` 内置别名已覆盖主要需求，用户明确"不引入别名功能"；③ 打印换行语义想当然套用 C 的 printf/puts 惯例，未遵循 CN 语言"打印=println、打印行=print"的直觉命名。
  - **解决**: ① 运算符重载改为上下文关键字识别，`运算符` 不进保留字表（关键字 59→58）；② 删除类型别名小节，关键字表移除 `别名`；③ 修正 10.6 API 表：`打印` 自动换行、`打印行` 不换行，并在变更记录中标注三点修正。
  - **预防**: 规划语言特性时，凡涉及"新增保留关键字"或"改变既有内置函数语义"的决策，必须先向用户确认，不得对标其他语言想当然；中文编程语言应优先"上下文关键字"（节省标识符命名空间）而非照搬 C/C++ 保留字；打印/输入等 IO 函数的换行语义以"直觉命名"为准（打印=完整输出一行）。
  - **权重**: 10.4（设计缺陷10 × 详细分析1.5 × 解决方案1.3 × 预防措施1.2 × 已修正0.9 × 待实施0.7）

- [2026-08-13 12:50] **问题类型**: 集成问题（权重 27.3）
  - **描述**: 缺陷完善 Debug 全面审查（模拟生产环境，target/debug_audit/ 9 个边界用例）发现 5 个 BUG：① `i128→浮64` Cast 无分支（`整128 * 1.0` 输出 0.000000——emitCast 整→浮只处理 i64/u64，i128 落默认 32 位 mov 读垃圾低32位）；② i128 混合参数 + i128 返回 ABI 错位崩溃（0xC0000005）——`emitParamSetup` 用 `if (i < 4)` 判定参数位未加 paramOffset（i128 返回占 rcx 后第4参数实际位4=栈），且 `parameterRegister(4)` 返回 `[rsp+40]`（rsp 锚定，被调方 sub frameSize 后读自己栈帧垃圾，应为 rbp 锚定 `[rbp+48]`）；③ 链式结构体赋值 `链3 = 链2 = 链1` 只拷贝 8 字节（外层赋值右值为 AssignmentExpr 时 valueSrcType 推导失败 → 不走 CopyStruct 落简单 Store）；④ `i1→i64` Cast 无分支（字符串后缀/包含 打印 4393751543809 垃圾——i1 落默认 32 位 mov 读槽高32位垃圾）；⑤ i128 数组元素 stride 8（`整128[3] 大数; 大数[1]` 读到 大数[0] 高64槽，求和 92233720368547758086000000000000000000 错误——visitIndexExpr/ptrElemStride/初始化列表 elemStride 漏 i128=16）
  - **原因**: i128 双槽约定（%vN=高/%vN+1=低）在 Cast 整→浮、i1→整 路径未全覆盖；Win x64 参数位判定（i<4）与 paramOffset 分离导致 i128 第4参数错判；parameterRegister 栈偏移 rsp 锚定（被调方 rsp 已 sub frameSize）应 rbp 锚定；链式赋值的结构体源推导未处理 AssignmentExpr 形态；i128 数组/指针 stride 只处理结构体类型漏 i128（16字节）
  - **解决**: ① emitCast 加 i128/u128→f64 分支（调用 __cn_i128_to_f64/__cn_u128_to_f64 双槽地址 + xmm0）；② emitParamSetup 参数位判定改 `i + paramOffset < 4`（浮点独立 i<4），parameterRegister index≥4 改 `[rbp+48+(index-4)*8]`；③ IR 结构体赋值检测补 AssignmentExpr 右值（内层返回值即源地址）；④ emitCast 加 i1→i64/u64/i32 分支（mov eax 零扩展）；⑤ visitIndexExpr/ptrElemStride/数组初始化 elemStride 补 `types::isI128 → 16`
  - **预防**: emitCast 必须全覆盖 i128↔浮/i1↔整 转换矩阵（缺分支落默认 32 位 mov 读垃圾）；被调方参数栈偏移恒用 rbp 锚定（rbp+48 为第5参数位）；参数位判定必须含 paramOffset（整型/i128），浮点独立编址（xmmN）；链式赋值右值形态（AssignmentExpr）必须纳入结构体源推导；i128 数组/指针 stride 恒 16（与 typeSize 一致）
  - **权重**: 27.3（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 10:30] **问题类型**: 集成问题（权重 27.3）
  - **描述**: 完善功能集成验证（14_integration2 员工档案系统）发现 6 个跨组件 BUG：① i128 返回 + 结构体按值参数 ABI 错位——被调方 `paramOffset = structReturn?1:0` 未涵盖 i128 返回（结构体参数应从 rdx 读但被调方从 rcx 读）+ i128 返回 prologue 未保存 rcx→r12（函数体内调用破坏 rcx，epilogue 用已破坏的 rcx → 0xC0000005）；② LoadPtr/StorePtr 漏 i128 双槽分支（`档案.年薪` 读取只 mov 8 字节，高64 残留垃圾）；③ emitCast 无"普通整数→i128"分支（i32→i128 落默认 mov 32位，高64 垃圾）；④ i128 参数 emitParamSetup 只 `mov slot, reg` 存 8 字节（调用方传 16 字节双槽地址指针，被调方只存低 8 字节）；⑤ 指针下标元素成员 `名单[j].年薪` objSrcType 推导只认数组变量（`types::isArray`），指针参数（`员工档案* 名单`）推导失败 → 字段访问降级为 0 → i128 比较恒假排序失效；⑥ 结构体下标赋值 `名单[j] = 名单[j+1]` 走 StorePtr 只存 8 字节（应 CopyStruct 48B）——结构体指针数组元素交换破坏数据
  - **原因**: 完善 A（i128/结构体值）与完善 C（优化器）各自独立测试通过，但"i128 字段 + 结构体按值 + 指针数组 + 整体赋值 + 比较排序"组合场景未覆盖——Win x64 隐藏返回指针（rcx）对 i128/结构体返回的 paramOffset 处理不一致；codegen 各访存/传参路径对 i128 双槽约定（%vN=高64、%vN+1=低64）未全覆盖；IR 层 objSrcType 推导与结构体赋值检测对"指针下标"形态（IndexExpr of ptr）遗漏
  - **解决**: ① paramOffset 涵盖 i128/u128 返回 + prologue `mov r12, rcx` + epilogue 用 r12；② LoadPtr/StorePtr 加 i128/u128 双槽分支；③ emitCast 加整数→i128 分支（符号扩展低64 + sar 63 高64）；④ emitParamSetup i128 参数（寄存器/栈）从指针 rep movsb 16 字节；⑤ visitMemberExpr objSrcType 推导 IndexExpr 分支补 `types::isPointer`（pointeeOf）；⑥ 非标识符左值赋值加结构体检测（CopyStruct 按 typeSizeOf）
  - **预防**: i128 双槽约定（%vN=高/%vN+1=低）必须覆盖全部访存/传参/转换路径（LoadPtr/StorePtr/Cast→i128/i128参数/返回）；隐藏返回指针（rcx→r12）对 i128/结构体返回统一处理；IR 层自定义类型推导必须覆盖 IdentifierExpr/MemberExpr/IndexExpr（含指针下标 pointeeOf）；结构体赋值（变量/下标/成员）统一 CopyStruct；综合用例必须组合 i128 字段 + 结构体值 + 指针数组 + 排序
  - **权重**: 27.3（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 09:00] **问题类型**: 逻辑错误（权重 22.75）
  - **描述**: 结构体按值返回（Win x64 隐藏返回指针）实现中发现 4 个 BUG：① epilogue structReturn 分支直接 return 漏掉 `mov rsp,rbp/pop rbp/ret` → 执行流落入下一函数 PROC 无限递归 → 0xC00000FD 栈溢出；② `mov rcx, 64` 硬编码拷贝 64 字节，16 字节班级结构体越界写 48 字节破坏相邻栈变量（翻倍 160/0/0）；③ `出.分数[1] = ...`（IndexExpr 对象为结构体数组字段 MemberExpr）targetType 推导遗漏 → StorePtr 用 i64 8 字节写入覆盖相邻数组元素；④ i128 字面量越界未报错（2^127 被接受）
  - **原因**: ① structReturn 分支是 emitEpilogue 的提前 return，未走公共尾部；② 结构体返回大小未记录到 IRFunction（codegen 无从得知精确字节）；③ visitAssignmentExpr IndexExpr 分支只处理 IdentifierExpr 对象，未处理 MemberExpr（出.分数）；④ 语义层 literalTypeOf 对无后缀超 int64 提升整128 但无越界检查
  - **解决**: ① structReturn 分支补 `mov rsp,rbp/pop rbp/ret`；② IRFunction 新增 structReturnSize（语义层 typeSizeOf），epilogue 按精确字节数 `mov rcx, N` 拷贝；③ IndexExpr 目标类型推导补 MemberExpr 分支（memberObjStructType 找字段数组元素类型）；④ 语义层 visitIntegerLiteral 加 整128（2^127-1）/正128（2^128-1）上限检查（含无后缀超 int64 提升场景）
  - **预防**: 提前 return 的分支必须补函数尾部（ret）；结构体返回/拷贝大小必须精确记录（IRFunction 字段），禁止 64 字节硬编码；IndexExpr 目标类型推导必须覆盖 MemberExpr 数组字段对象；i128 字面量越界检查放语义层（literalTypeOf 之后，含无后缀自动提升场景）
  - **权重**: 22.75（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 03:34] **问题类型**: 逻辑错误（权重 22.75）
  - **描述**: 阶段2全面审查（模拟生产环境）发现 10 个 BUG：① 除零未插桩（错误码1）→ Windows 0xC0000094 崩溃而非 CN 错误码；② idiv 立即数 A2001；③ 无符号除法用 idiv+有符号扩展（正32 4000000000/2 错算 -295M）；④ 无符号比较用 setg/setl（正32 4294967295>1 判假）；⑤ u32 LoadPtr movsxd 符号扩展；⑥ 浮点参数经 rcx 读（caller 用 xmm0-3 → callee 读到垃圾）；⑦ 浮点返回只 mov rax 未设 xmm0；⑧ const_fold Call 尾部实参传播浮点常量 → `movsd xmm2, qword ptr 4.0` A2050；⑨ const_fold 无符号除/比较用有符号语义（-O1 与运行期不一致）；⑩ 结构体数组字段（方形.顶点[0].x / 指针->顶点[1].x）把数组字段当值 LoadPtr（垃圾指针→空指针错误/访问冲突），且数组字段越界不插桩
  - **原因**: ① 除零检查设计缺失（codegen 只对 LoadPtr/FieldAddr 插桩错误码）；② div/idiv 不接受立即数；③④⑤ 无符号类型（正N/uN）在除法/比较/加载路径未按无符号语义分派；⑥⑦ Win x64 浮点调用约定（xmm0-3 参数/xmm0 返回）callee 侧未实现；⑧ 尾部操作数传播无浮点过滤（Call 第3+实参落尾部）；⑨ 折叠期未区分符号；⑩ IR 层数组字段退化/步进/越界推导缺失（memberObjStructType 未处理 arrow 剥指针）
  - **解决**: ① emitDivMod 除零检查（常量非零跳过、寄存器低32位检查后 div ecx）；② 常量除数先 mov ecx；③ 无符号 div + xor edx,edx；④ 无符号 seta/setb 系列；⑤ u32 LoadPtr mov eax 零扩展；⑥ emitParamSetup 浮点参数从 xmmN 读；⑦ epilogue currentReturnType_ 浮点 movsd xmm0；⑧ 尾部传播排除浮点/i128；⑨ foldIntBinary/compareInt 无符号分支；⑩ visitMemberExpr 数组字段退化返回字段地址 + lvalueAddress/visitIndexExpr 数组字段步进按元素大小 + 越界检查插桩 + memberObjStructType 递归辅助（arrow 剥指针）
  - **预防**: 除零必须插桩错误码1（常量零编译期报错、运行期寄存器检查）；无符号类型（正N）在除/比较/加载路径必须按无符号指令分派（div/seta/movzx）；Win x64 浮点参数走 xmm0-3、返回走 xmm0（callee 侧必须实现）；const_fold 尾部操作数传播必须排除浮点/i128；折叠语义必须与运行期指令一致（有符号 idiv/setg vs 无符号 div/seta）；结构体数组字段必须退化为地址（非 LoadPtr 值）+ 按元素大小步进 + 越界检查；自定义类型推导必须处理 arrow 指针剥除（memberObjStructType 模式）
  - **权重**: 22.75（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 02:56] **问题类型**: 集成问题（权重 22.75）
  - **描述**: 阶段2全链路集成验证（09_integration 学生成绩系统）发现 9 个跨组件集成 bug：① 结构体数组初始化列表 `学生[5] 名单 = { 学生{...} }` 只写占位0（visitStructInitExpr 无法作值）；② 结构体数组栈槽只登记 len 个（元素24字节=3槽，5元素需15槽）；③ 结构体指针算术/`p[i]`/`p++` 固定8字节步进（应24）；④ 结构体/枚举作函数参数（学生* 名单）解析失败；⑤ 函数指针参数含结构体指针（整32(*比较)(学生*, 学生*)）解析失败；⑥ 数组名退化缺失（名单 + 人数 报"学生[5]与整32算术"）；⑦ 长字符串 db 单行超长 A2042 且行尾逗号 A2008；⑧ 用户函数返回 f64 结果类型误标 i32（eax 读 xmm0 除零崩溃）；⑨ 空指针常量 0 存32位（mov eax,0）64位比较读到高位残留判空失败
  - **原因**: 各 Task 独立测试未覆盖"组合场景"——结构体数组+指针遍历+函数指针回调+字符串字段+枚举+递归返回浮点同时出现时，IR/codegen 对复合类型的假设（8字节步进/单值传递/单行db/32位常量）逐一暴露
  - **解决**: ① genVarDecl 数组初始化列表遇 StructInitExpr 改 emitStructInitTo 逐字段写入、步进按元素大小；② registerVarSlots 数组槽数 = 长度×元素槽数；③ 新增 ptrElemStride（结构体指针按 typeSizeOf）+ visitBinaryExpr/visitUnaryExpr/visitIndexExpr/lvalueAddress 统一使用；④⑤ parseParamDecl/parseFuncPtrType 参数类型改用 parseTypeNameEx；⑥ 语义层 visitBinaryExpr 数组名退化（学生[5]→学生*）；⑦ 长字符串 db 按24字节/行拆分且行尾不加逗号；⑧ IR 调用结果类型查 funcReturnTypeOf；⑨ ConstInt ptr 类型强制 mov rax 64位
  - **预防**: 综合用例必须覆盖"组合场景"（结构体数组+指针遍历+函数指针+字符串字段+递归浮点返回）；结构体相关 IR/codegen 一律按 typeSizeOf 计算步进/槽数；函数调用结果类型须查语义层真实返回类型；长字符串拆行 db；ptr 常量恒64位
  - **权重**: 22.75（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 02:30] **问题类型**: 逻辑错误（权重 20.8）
  - **描述**: Task 2.6 优化器 -O1 下 05/07 E2E 崩溃（0xC0000005）——① 常量折叠把指令替换为 ConstInt/ConstBool 后结果寄存器仍保留，后续不可折叠指令（ptr 加法）引用该寄存器 → codegen 读未初始化的栈槽；② Branch 条件寄存器挂在块内最后一条指令 operands 尾部，被折叠为 ConstBool 后尾部仍是 %vN → emitTerminator 读槽垃圾
  - **原因**: 折叠只改指令本身不替换引用点；非 SSA IR 中"寄存器=常量"的传播必须同步替换所有使用处
  - **解决**: run() 第一步做常量传播（命中常量表的寄存器操作数原地替换为常量值，含尾部 Branch 条件）；只向纯运算指令传播（Load/Store/Call 等保持寄存器引用）
  - **预防**: 非 SSA IR 做折叠必须带"寄存器→常量"跟踪 + 引用点同步替换；Branch 条件尾部操作数是 codegen 的特殊契约（last.operands.back()），折叠后必须同步替换
  - **权重**: 20.8（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 02:30] **问题类型**: 逻辑错误（权重 16.8）
  - **描述**: Task 2.6 DCE 纯块内引用分析误删跨块引用——越界检查的边界常量在 bb0（`常量整数 %v25 (i64) [2]`），被 bb2 的 `转换 %v34 %v25` 引用；块内 DCE 看不到跨块引用 → 删除 → 运行崩溃
  - **原因**: IR 寄存器可被其他基本块引用（CFG 数据流），块内引用集合不完整
  - **解决**: DCE 改为函数级引用收集（先遍历函数所有块的指令操作数+终止信息，再逐块反向扫描删除）
  - **预防**: DCE/活性分析必须函数级（或跨块）引用集合，禁止纯块内分析；任何删除前先确认无跨块引用
  - **权重**: 16.8（逻辑错误8 × 详细分析1.5 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 02:30] **问题类型**: 集成问题（权重 14）
  - **描述**: Task 2.6 常量传播到操作数位置破坏 codegen 假设：① 浮点常量 `movsd xmm0, qword ptr 3.5`（A2050，MASM 无浮点立即数）；② i128 常量 `cmp rax, 100000000000000001`（A2084 超 32 位）；③ 小位宽 Store `movsx eax, byte ptr 100`（A2070 大小前缀+立即数非法）；④ 0b/0x 前缀 `mov eax, 0b1100`（A2048）
  - **原因**: codegen operandText 对 isConstant 直接输出文本，未区分操作数上下文（浮点需 @fpN 池标签、i128 拆双槽、小位宽 Store 需 movsx 语义、MASM 无 0b/0x 前缀）
  - **解决**: 常量传播加白名单——浮点/i128/u128 不传播；只向纯运算指令传播（Load/Store/StorePtr/Call/LoadPtr/FieldAddr/AddrOf 不传播）；传播时 normalizeIntText 规范化 0b/0x/0o 为十进制
  - **预防**: 优化器传播常量前必须核对 codegen 各指令对"常量操作数"的支持（浮点池/大值/大小前缀/进制前缀）；保守策略：只向纯运算指令传播
  - **权重**: 14（集成问题7 × 详细分析2.0 × 解决方案1.0 × 预防措施1.0 × 已解决1.0）

- [2026-08-13 01:30] **问题类型**: 逻辑错误（权重 20.8）
  - **描述**: Task 2.7 `r.左上.x`（嵌套成员）与 `点数组[1].x`（数组元素成员）字段访问输出 0——IR 层 `visitMemberExpr`/`lvalueAddress` 对 object 是 MemberExpr/IndexExpr 时查不到结构体类型（lookupSrcType 只认 IdentifierExpr），导致 decl==nullptr fallback 返回 ConstInt 0（地址未加偏移）
  - **原因**: IR 层无类型推导，嵌套/数组元素成员的类型需从 AST 递归推导（外层结构体字段类型/数组元素类型），遗漏即静默错误
  - **解决**: visitMemberExpr 与 lvalueAddress 的 MemberExpr 分支补充 object 为 MemberExpr（查外层字段类型）与 IndexExpr（查数组元素类型）的类型推导；字段地址统一走 lvalueAddress 递归
  - **预防**: 凡 IR 层访问自定义类型（结构体）信息，必须处理 object 的三种形态（IdentifierExpr/MemberExpr/IndexExpr）；输出异常先查 IR（cn.exe ir 命令）定位哪条指令错误
  - **权重**: 20.8（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 01:30] **问题类型**: 逻辑错误（权重 16.8）
  - **描述**: Task 2.7 自定义类型名变量声明探测歧义——`点[3] 点数组` 是数组声明（`]` 后跟变量名），但 `点数组[0] = v` 下标赋值也被误判为数组类型声明（peek(1)==[ && peek(2)==数字 命中），报"预期变量名，实际为 '='"
  - **原因**: parseStmt 探测自定义类型声明只看前3个 token，未区分"声明"（`]` 后跟变量名）与"赋值"（`]` 后跟 `=`）
  - **解决**: 数组类型声明探测加 peek(3)==RightBracket && peek(4)==Identifier 条件
  - **预防**: 语法探测必须看完整模式（含结尾界定符）；声明 vs 表达式歧义用"后随 token 类别"区分
  - **权重**: 16.8（逻辑错误8 × 详细分析1.5 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 01:30] **问题类型**: 逻辑错误（权重 12.5）
  - **描述**: 初始值 `{` 双重语义——`整32[5] 数据 = {1,2,3}` 是数组 InitListExpr，`点 p = 点{ x = 1 }` 是 StructInitExpr；parseTypePrefixVarDecl 一律走 parseInitList 导致结构体初始化被当数组列表（嵌套/数组赋值输出 0）
  - **原因**: `=` 后 `{` 的处理未区分目标类型（数组 vs 结构体）
  - **解决**: `if (check(LeftBrace) && types::isArray(decl->typeName)) parseInitList() else parseExpr()`（parseExpr 的 parsePrimary 识别 类型名{...} 为 StructInitExpr）
  - **预防**: 语法歧义场景先查规格书示例与既有类型处理；`{` 既是数组初始化列表也是结构体命名式初始化
  - **权重**: 12.5（逻辑错误8 × 详细分析1.5 × 解决方案1.3 × 预防措施1.0 × 已解决1.0）

- [2026-08-13 01:30] **问题类型**: 逻辑错误（权重 12.5）
  - **描述**: 枚举↔整数隐式转换失败——`打印行整数(颜色.蓝)` 报"无法将 '颜色' 隐式转换为 '整64'"，因为静态 types::canConvert 不认识枚举类型（无枚举表访问）
  - **原因**: 语义层 canConvert 是 type_system 静态函数，无法感知自定义枚举类型；枚举本质为整32，需特判
  - **解决**: 语义层新增非静态 `canConvertType`（枚举↔整数双向、枚举间须同名、结构体须同名），替换 visitVarDecl/visitCallExpr/visitAssignmentExpr/visitReturnStmt/visitStructInitExpr 的 canConvert 调用点
  - **预防**: 凡涉及自定义类型（枚举/结构体）的隐式转换必须走语义层 canConvertType，禁止直接调 type_system 静态 canConvert
  - **权重**: 12.5（逻辑错误8 × 详细分析1.5 × 解决方案1.3 × 预防措施1.0 × 已解决1.0）

- [2026-08-13 00:47] **问题类型**: 逻辑错误（权重 20.8）
  - **描述**: IR `decodeString` 用 `raw.compare(0, 4, "原始多行")` 判断前缀失败——`compare` 的第2参是**字节数**，而"原始"是 UTF-8 多字节（6字节），4 只覆盖了前4字节，导致原始/多行前缀无法识别（测试失败：字符串常量保留前缀未剥离）
  - **原因**: 中文前缀为 UTF-8 多字节编码，compare/substr 的偏移与长度参数都是字节计数，不能按"字符数"写
  - **解决**: 改用 `raw.rfind("原始多行", 0) == 0` 前缀匹配 + `start = 12`（"原始多行"=12字节）/`start = 6`（"原始"/"多行"=6字节）字节偏移
  - **预防**: 处理中文前缀/子串时，偏移量必须用 `prefix.size()`（字节数），禁止按字符数硬编码；凡 UTF-8 字符串索引一律字节语义
  - **权重**: 20.8（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 17:59] **问题类型**: 集成问题（权重 27.3）
  - **描述**: 换电脑（Linux GCC7 → Windows MSVC）后 Task 1.7 构建失败：1) `ir.hpp` 的 `IRFunction`/`IRModule` 含 `vector<unique_ptr>`，MSVC traits 探测实例化隐式拷贝构造触发 C2280；2) `test_lexer.cpp` raw string 内 `\"` 序列 MSVC 报 C2017 非法转义；3) 测试变量名 `asm` 是 MSVC 关键字报 C2059；4) cmake 不在 PATH，需用 VS 自带完整路径；5) CMakeLists gtest 本地源码回退路径指向不存在的 `~/third-party`，实际缓存在 `target/build/_deps`
  - **原因**: 项目原在 Linux GCC7 开发，未覆盖 MSVC 编译差异（unique_ptr容器拷贝/raw string转义/MSVC关键字/工具链路径）
  - **解决**: 1) `ir.hpp` 显式移动语义（拷贝删除）；2) raw string 含 `\"` 改普通字符串；3) 变量避免用 `asm` 等 MSVC 关键字；4) 用 VS 自带 cmake 完整路径；5) gtest 回退路径增加 `target/build/_deps/googletest-src/googletest` 检查
  - **预防**: 新代码避免 `asm`/`cl` 作标识符；raw string 含引号转义用普通字符串；含 unique_ptr 的容器成员类显式移动语义；构建前先确认工具链与依赖缓存路径
  - **权重**: 27.3（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 16:44] **问题类型**: 工具执行错误（权重 15）
  - **描述**: `TEST(SemanticTest, 递归函数)` 中文测试名导致编译失败：`expected initializer before ':' token`（GCC 7 不支持中文标识符）
  - **原因**: GCC 7 的 gtest 宏 `GTEST_TEST_` 用测试名生成 C++ 类名/函数名，中文标识符不合法；注释中的中文虽可编译，但宏展开的标识符不行
  - **解决**: 全部测试名改为英文（如 `TEST(SemanticTest, RecursiveFunction)`），中文仅用于注释/字符串
  - **预防**: 新建测试文件时测试名一律英文；标识符（变量/函数/类/枚举）一律英文

- [2026-08-12 16:44] **问题类型**: 工具执行错误（权重 12）
  - **描述**: `from[0] == '正'` 触发 `-Werror=multichar` 和 `-Werror=type-limits` 编译失败
  - **原因**: '正' 是 UTF-8 多字节字符（3字节），不能与单字节 char 比较；GCC 将多字符常量视为警告并在 -Werror 下报错
  - **解决**: 改用 `from.compare(0, 3, "正") == 0` 字符串前缀比较
  - **预防**: 任何中文字符比较必须用 std::string 的 compare/substr，禁止 `str[i] == '中文字符'`

- [2026-08-12 16:44] **问题类型**: 集成问题（权重 10）
  - **描述**: IRGenerator 继承 AstVisitor 后未实现 `visitIfStmt/visitWhileStmt/visitForStmt`，链接报 `undefined reference to ... vtable`
  - **原因**: 三个虚函数在头文件声明但实现遗漏，vtable 引用缺失
  - **解决**: 补充实现，转发到 genIf/genWhile/genFor
  - **预防**: 继承 AstVisitor 后逐一核对所有纯虚函数是否实现；链接错误先看 undefined reference 到哪个类

- [2026-08-12 16:44] **问题类型**: 逻辑错误（权重 8）
  - **描述**: 测试中 `变量 结果 = 阶乘(5)` 解析失败："预期变量名，实际为 '结果'"
  - **原因**: `结果`（Kw_Result）是 CN 语言53个保留关键字之一（结果<T,E>类型），不能作标识符
  - **解决**: 测试源码改用 `答案` 等非关键字变量名
  - **预防**: 编写 CN 测试代码时避免使用53个关键字作变量/函数名（结果/变量/选择/情况/默认等）

## 高权重问题（Task 1.9 新增）

- [2026-08-12 18:22] **问题类型**: 集成问题（权重 24.6）
  - **描述**: Task 1.9 `cn build` 链接阶段 LNK2019 无法解析 CRT 符号：1) `/ENTRY:WinMain` 时 link 不自动注入 CRT 默认库，puts/printf/malloc 等全部未解析；2) 改显式 `/DEFAULTLIB:ucrt.lib` 后仍缺 `memcpy/memset`（它们在静态 `libucrt.lib`，导入库 `ucrt.lib` 无定义）；3) 即使链接成功，`/ENTRY:WinMain` 跳过 CRT 启动代码，stdout 未初始化导致 `printLine`（puts）输出为空
  - **原因**: 自定义 `/ENTRY` 绕过了 CRT 启动（mainCRTStartup/WinMainCRTStartup），需自行注入默认库；puts 依赖 CRT 初始化 stdout
  - **解决**: 链接命令改为 `link /ENTRY:WinMainCRTStartup /SUBSYSTEM:CONSOLE /DEFAULTLIB:libcmt.lib /DEFAULTLIB:libucrt.lib /DEFAULTLIB:kernel32.lib`（CRT 初始化 stdout 后调用用户 WinMain）
  - **预防**: 自定义入口时必须用 `WinMainCRTStartup`（而非 WinMain）；静态 CRT 配套 `libucrt.lib` 而非 `ucrt.lib`；显式 `/DEFAULTLIB` 注入 CRT/内核库
  - **权重**: 24.6（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 18:22] **问题类型**: 配置错误（权重 15.6）
  - **描述**: ml64 汇编 UTF-8 文件失败 A2044（invalid character in file）：1) 带 UTF-8 BOM 的 asm 文件从第1行起全部 A2044；2) `db "你好，世界"` 中文原始 UTF-8 字节触发 A2044；3) 中文块标签 `块0:` 也是 A2044
  - **原因**: 此 ml64 版本不识别 UTF-8 BOM，且把原始 UTF-8 中文字节当非法字符；仅 UTF-8 无 BOM 的纯 ASCII 代码 + 中文注释可接受
  - **解决**: 1) asm 写 UTF-8 无 BOM；2) 字符串常量改用十六进制字节 `db 0E4h,0BDh,0A0h,0`（代码生成器 `hexBytesString()`）；3) IR 块标签 块N→bbN（ml64 不识别中文标识符）
  - **预防**: 生成汇编一律 UTF-8 无 BOM；中文内容只出现在注释和 `db` 十六进制中；符号/标签必须 ASCII
  - **权重**: 15.6（配置错误6 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 18:22] **问题类型**: 集成问题（权重 13.7）
  - **描述**: 语义分析器报"未声明的函数 '打印行'"——`打印行` 是 CN 内置函数，但 SemanticAnalyzer 未注册其符号，visitCallExpr 找不到
  - **原因**: 阶段一语义分析器只注册源码中的函数声明，未内置 IO 函数符号表
  - **解决**: semantic.cpp 新增 `registerBuiltins()`，在 visitProgram 第零趟注册 `打印行(字符串)->空类型`/`打印行整数(整64)`/`打印行浮点(浮64)`；代码生成器 `symbolName()` 映射到运行时 `printLine` 系列
  - **预防**: 新增内置函数必须同时注册语义符号表 + 代码生成器符号映射 + 运行时 extern "C" 导出三处
  - **权重**: 13.7（集成问题7 × 详细分析2.0 × 解决方案1.5 × 已解决1.0）

- [2026-08-12 18:22] **问题类型**: 工具执行错误（权重 3.9）
  - **描述**: MSVC 将 `getenv` 视为不安全函数，C4996 警告在 /WX 下报 C2220 错误
  - **原因**: MSVC 建议用 `_dupenv_s`，/W4 /WX 把 C4996 提升为错误
  - **解决**: 封装 `getEnvVar()` 用 `_dupenv_s` 实现
  - **预防**: Windows 代码避免直接 getenv/strcpy 等弃用函数，用 _s 安全版
  - **权重**: 3.9（工具执行错误2 × 详细分析2.0 × 解决方案1.5 × 已解决1.0）

## 高权重问题（生产环境审查 Task 1.10 新增，2026-08-12 19:04）

- [2026-08-12 19:04] **问题类型**: 设计缺陷（权重 31.9）
  - **描述**: 生产环境模拟发现编译器8个BUG：1)语义层 `整数`/`小数` 类型别名未规范化，`整数 x = 10` 报"无法将 '整32' 隐式转换为 '整数'"；2) x64_codegen 未登记局部变量槽，全部变量访问生成 `[rbp0]`（A2006 undefined symbol: rbp0）；3) `idiv [rbp-X]` 内存操作数缺 size 前缀（A2023）；4) Win x64 调用约定违反：调用侧未预留32字节影子空间、接收侧栈参偏移 `[rbp+40]` 错位（应 `[rbp+48]`）；5) IR 层变量遮蔽失效：`varRegs_` 平铺映射被内层同名覆盖，块外读错值；6) i32 参数传 `printLineInt(long long)` 未符号扩展，负数变巨大正数（4294967291）；7) `--output` 深层目录不自动创建（LNK1104）；8) driver 流水线 IR/codegen 阶段未检查 `diagnostics.hasErrors()` 仍返回0
  - **原因**: 类型别名用字符串相等比较、槽登记遗漏 Alloca、MASM 内存操作数需显式 size、调用约定实现未按 MS ABI（影子空间+栈参布局）、作用域用平铺 map、C ABI 整型参数需64位+符号扩展、路径处理未建目录、错误检查遗漏
  - **解决**: 1) `canonicalType()` 规范化（整数→整32/小数→浮64），declareVar/registerFunction/canConvert/commonNumericType 统一使用；2) `generateFunctionAssembly` 扫描全部 Alloca 登记变量槽；3) `idiv dword ptr/qword ptr [rbp-X]`；4) `emitCall` 一次性 `sub rsp,(32+栈参+对齐)` 预留影子空间，接收侧 `[rbp+48+(i-4)*8]`；5) IR 作用域栈 + 唯一内部名 `name$N`，func.params 保留源码名 + paramUniques 存唯一名；6) 传参前 `movsxd` 符号扩展 i32→i64；7) `ensureDirExists()` 逐级创建目录，中间文件与 exe 同目录；8) runPipeline 补 IR/codegen 后 hasErrors 检查
  - **预防**: 类型别名必须统一规范化后再比较；Alloca 必须登记槽；MASM 内存操作数带 size 前缀；Win x64 调用必须预留影子空间+16对齐+符号扩展；作用域必须用栈结构；`--output` 路径必须自动建目录；流水线每个阶段后检查错误
  - **权重**: 31.9（设计缺陷10 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

## 既有经验（继承自 HANDOFF，权重 10）

- **GCC 7 不支持中文标识符**：所有标识符（含 gtest 测试名）必须英文，中文仅用于注释/字符串
- **无外网**：googletest 用本地 `~/third-party/unittest/googletest`（LLVM定制版，需 `-DGTEST_NO_LLVM_SUPPORT`）
- **本机构建命令**：`export PATH=$HOME/gcc7/usr/bin:$PATH && cmake -B target/Debug -S . -DCMAKE_CXX_COMPILER=$HOME/gcc7/usr/bin/g++`
- **编译产物在 target/**：测试二进制在 `target/cn_unit_tests`
- **pkill 自匹配**：`pkill -f` 会匹配自身 shell，先 `pgrep -af` 再精确 kill
- **synchronize() 必须前进**：错误恢复停在边界字符会死循环
- **parseStmt() 必须分发 LeftBrace**：嵌套代码块是合法语句
- **`0 ++i` 词法歧义**：字面量后紧跟 ++ 解析为后缀自增，测试需分号分隔
- **peekNext(n) 先跳过当前位置**（`i <= n`）
- **运算符贪婪匹配**：三字符 > 双字符 > 单字符
- **字符串Token保留完整原始文本**

## 实施规范（用户规则）

- 中文回复用户；代码添加中文注释但标识符英文
- 文件 ≤1000行、函数 ≤100行
- E2E先行；禁止 skip()；产物在 target/
- 编译零警告（-Wall -Wextra -Werror）
- plans 文档完成后打勾
- 提交前更新"更新日志.md"（覆盖写入本次内容）
- 会话结束前写 HANDOFF.md 交接
- 实施计划示例代码用中文API命名，但**用户明确要求英文API命名**（指令优先）

## 任务经验（2026-08-12 实施计划文档拆分）

- [2026-08-12 20:36] **问题类型**: 任务经验（设计缺陷，权重 6.0）
  - **描述**: 大型计划文档（3968行）拆分重构的正确做法沉淀：按阶段边界切割（`### Task` 标题行处），拆分后内容零丢失（88个`[x]`勾选框、全部代码块、"已实现/已修复"历史备注、Task 1.1a 特殊任务均保留），交叉引用双向成对，原文件改写为导航索引而非删除
  - **原因**: 拆分大型文档时，若直接复制粘贴整段内容，勾选框与历史备注块容易被误删，导致后续 AI 误判任务状态；单文件超 1000 行违反项目文件约束
  - **解决**: 1) 在 `### Task` 标题行处按阶段边界切割；2) 拆分后逐段核对勾选框/历史备注完整性；3) 每个子文件头部带"前序文档/后续文档"双向链接；4) 原文件改写为索引文件（≤200行），保留既有引用
  - **预防**: 后续任何大型计划文档拆分，先识别 `### Task` 边界，再逐段搬移并保留全部 `[x]` 与"已实现/已修复"备注；拆分完成后运行链接校验（双向成对 + 索引可达）
  - **权重**: 6.0（设计缺陷10 × 低影响0.6）

## 任务经验（Task 2.1 控制流增量实施 switch，2026-08-12）

- [2026-08-12 21:30] **问题类型**: 逻辑错误（权重 31.2）
  - **描述**: IR 控制流生成标签 bug 链：1) genIf/genWhile/genFor 用 `"bb" + (blockCounter_ + N)` **预计算标签但不递增 blockCounter_**，嵌套控制流（否则-如果链/体内嵌套）会从旧编号开始分配块，导致标签重叠（ml64 A2005 symbol redefinition）与跳转错位；2) genSwitch 首版按"预留 caseCount*2+1 个编号"计算绝对标签，case 体块内嵌套控制流新增块后编号错位；3) 修复方案统一为**标签随建随取**（每个标签 `blockCounter_++` 预消费），保证嵌套任意深度标签唯一
  - **原因**: 原实现假设"块编号连续且无嵌套"，未考虑控制流语句体内还会递归生成新块
  - **解决**: genIf/genWhile/genFor/genSwitch 全部改为先 `blockCounter_++` 预消费标签再引用；genSwitch 改为两阶段（先预分配全部标签字符串，再逐个 newBlock 填充），杜绝与嵌套控制流编号冲突
  - **预防**: 生成基本块标签时必须"预消费计数器"（取完即 ++），严禁基于 `blockCounter_ + N` 预计算未来编号后不推进；新增控制流结构后必须跑 E2E 覆盖嵌套场景
  - **权重**: 31.2（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 21:30] **问题类型**: 逻辑错误（权重 15.6）
  - **描述**: IR `visitCharLiteral` 对字符字面量 `'B'` 用 `decodeString()` 解码，但该函数只剥离**双引号**，字符是**单引号**，结果 `text="'B'"` 取 `text[0]='`（39），switch 字符匹配全部落默认分支；parser `parseCaseValue` 正确取 `text[1]`（66）导致两侧不一致
  - **原因**: 字符字面量复用字符串解码函数，未单独处理单引号剥离；阶段一 IR 层只取首字节简化实现
  - **解决**: `visitCharLiteral` 单独剥离单引号（`raw.front()=='\'' && raw.back()=='\''`），与 parser 的 case 值解析保持一致
  - **预防**: 字符字面量（单引号）与字符串字面量（双引号）必须分开处理；新增字面量解析时两端（parser 常量求值 / IR 表达式求值）必须使用同一解码规则
  - **权重**: 15.6（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 已解决1.0）

- [2026-08-12 21:30] **问题类型**: 工具执行错误（权重 3.9）
  - **描述**: `Token::getText()` 不存在——Token 的 getter 是 `getValue()`（parser 里 `current().getValue()` 已在既有代码使用），新写的 parseCaseValue 误用 getText 导致 C2039 编译失败
  - **原因**: 未先确认 Token 类的实际 API 就使用推测方法名
  - **解决**: 改为 `current().getValue()`
  - **预防**: 调用既有类方法前先 grep 该类头文件确认成员名；新代码与既有代码风格保持一致
  - **权重**: 3.9（工具执行错误2 × 详细分析2.0 × 已解决1.0）

- [2026-08-12 21:30] **问题类型**: 逻辑错误（权重 7.8）
  - **描述**: parser `parseCaseValue` 解析成功（读整型/字符字面量）后**未 advance() 消费 Token**，调用方随后 `consume(Colon)` 时 current 仍是字面量，报"预期 ':'，实际为 '1'"
  - **原因**: 解析函数只读不改指针，调用方误以为已消费
  - **解决**: parseCaseValue 成功分支补充 `advance()`
  - **预防**: 解析"取值并推进"的函数必须消费 Token；调用 `consume()` 前确认 current 已推进到期望 Token
  - **权重**: 7.8（逻辑错误8 × 详细分析2.0 × 已解决1.0）

## 任务经验（Task 2.2 函数完整支持，2026-08-12）

- [2026-08-12 21:57] **问题类型**: 逻辑错误（权重 31.2）
  - **描述**: E2E 03_function 运行崩溃（0xC0000005 访问冲突）：emitCall 只在 `argCount > 4` 时预留影子空间，参数≤4 的直接调用/间接调用完全不留 32 字节影子空间。Win x64 ABI 要求**所有调用方必须在 call 前预留 32 字节影子空间**（无论参数多少），否则被调 C 函数（如 printLine）将参数写入栈顶踩坏调用方栈帧；间接调用 `call r11` 后该问题必然触发
  - **原因**: 阶段一 emitCall 简化为"仅栈参数时分配"，未遵循 Win x64 影子空间硬性要求；直接调用 printLine(1参) 此前侥幸通过（printLine 未写影子空间），函数指针间接调用打破侥幸
  - **解决**: emitCall 重构——参数≤4 也 `sub rsp, 32`（call 后 `add rsp, 32`）；argCount>4 时额外分配栈参数区；分配/恢复严格对称
  - **预防**: 任何 call（直接/间接）前必须预留影子空间 32 字节并保持 16 对齐；新增调用形式（间接调用/虚调用）后必须跑 E2E 验证运行期不崩溃
  - **权重**: 31.2（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 21:57] **问题类型**: 逻辑错误（权重 15.6）
  - **描述**: 语义单元测试 `整32 结果 = 回调(10, 20)` 报"预期变量名，实际为 '结果'"：`结果`（Kw_Result）是 CN 语言 53 个保留关键字之一（结果<T,E>类型），lexer 将其识别为关键字而非标识符，parser 的 `check(Identifier)` 失败
  - **原因**: 编写测试源码时未查阅关键字表，`结果`/`错误`/`某些` 等内置构造器相关词与错误处理关键字不能作变量名
  - **解决**: 测试源码变量名改用非关键字 `r`
  - **预防**: 编写 CN 测试/示例代码时避开 53 关键字（结果/错误/某些/变量/选择/情况/默认等）；报"预期变量名"先查该 token 是否被 lexer 识别为关键字
  - **权重**: 15.6（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 已解决1.0）

- [2026-08-12 21:57] **问题类型**: 工具执行错误（权重 3.9）
  - **描述**: `check(TokenType::LeftParen, /*unused*/ 0)` 误写——check 只接受 1 个参数，编译报错；这是编写 parseParamDecl 函数指针探测时残留的占位代码
  - **原因**: 编辑中途插入占位未清理，编译才暴露
  - **解决**: 删除占位，直接使用完整探测条件（peek(1)..peek(5)）
  - **预防**: apply_diff 修改后立即检查是否有残留占位/半成品代码；编译错误 C2059 等先看是否为编辑残留
  - **权重**: 3.9（工具执行错误2 × 详细分析2.0 × 已解决1.0）

## 高权重问题（Task 2.3 类型系统新增）

- [2026-08-12 22:59] **问题类型**: 集成问题（权重 31.2）
  - **描述**: ml64 汇编 SSE/8位指令大量 A2070/A2022/A2048：1) `movss xmm0, @fp0`（.data 段标签）报 A2070 invalid instruction operands——MASM 无法从标签推断内存宽度，需 `movss xmm0, dword ptr @fp0`；2) `movsx eax, [rbp-X]`（8/16位内存操作数）报 A2070——需 `movsx eax, word ptr [rbp-X]`；3) `movss [rbp-X], xmm0`（SSE 内存目标）同样需 `movss dword ptr [rbp-X], xmm0`；4) `mov eax, 0b1100`/`0xFF` 报 A2048/A2206——MASM 不支持 0b/0x 前缀，需转十进制；5) `@fp0 dd 0x3FF8...` 报 A2206——MASM 十六进制必须 `3FF8...h` 格式
  - **原因**: MASM（ml64）对内存操作数**从不推断宽度**，所有 movsx/movzx/movss/movsd/ucomisd/cvttsd2si 的 [rbp-X] 或标签操作数必须显式 byte/word/dword/qword ptr；MASM 数字字面量语法与 C 不同
  - **解决**: codegen 新增 `memSizePtr()` 辅助（byte/word ptr）；所有 SSE 指令内存操作数加 dword/qword ptr；浮点常量位模式改 `%016llXh`；0b/0x 前缀统一转十进制
  - **预防**: 生成汇编时凡涉及 [rbp-X]/标签的 movsx/movzx/SSE 指令必须加大小前缀；数字字面量只用十进制；浮点位模式用 `xxxh` 格式
  - **权重**: 31.2（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0 × 发生频率2）

- [2026-08-12 22:59] **问题类型**: 逻辑错误（权重 16.8）
  - **描述**: 8/16/32位值（u32/i32/u8 等）存入 8 字节栈槽后，以整64（打印行整数参数）读取时读到槽中高位栈残留垃圾（如 123U 打印成 549755814011、4000000000U 打印成 33214485043554305）
  - **原因**: 编译器所有虚拟寄存器/变量槽固定 8 字节，小宽度 Store 只写低 4/2/1 字节，高字节是栈残留；Win x64 ABI 整参按 64 位传递，emitCall 直接 mov rcx, [槽] 读满 8 字节
  - **解决**: 1) Store 时小宽度值先符号/零扩展存满 8 字节（movsx/movsxd 或 movzx）；2) emitCall 实参统一 Cast 到 i64（IR 层 visitCallExpr 对所有 <64位 整数实参生成 Cast）
  - **预防**: 涉及多宽度共存的栈槽必须"存满 8 字节"（扩展后再存）；实参传递统一 Cast 到 ABI 宽度；出现巨大/负数打印值先怀疑槽高位垃圾
  - **权重**: 16.8（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 22:59] **问题类型**: 逻辑错误（权重 12.6）
  - **描述**: 无后缀大整数字面量 `5000000000` 默认整32 导致截断（mov eax 溢出成 705032704）；需按值自适应提升为整64
  - **原因**: 规格书默认无后缀字面量为整32，但值超出 int32 范围时应自动提升（C++ 字面量规则）
  - **解决**: IR 层 visitIntegerLiteral 对无后缀且值>int32 范围的提升为整64；emitConstLoad 对超32位值用 mov rax
  - **预防**: 字面量类型 = 后缀决定 + 值自适应（超范围自动提升）；codegen 常量加载按值大小选寄存器宽度
  - **权重**: 12.6（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 已解决1.0）

## 高权重问题（Task 2.4 数组与指针新增）

- [2026-08-12 23:00] **问题类型**: 逻辑错误（权重 20.8）
  - **描述**: StorePtr（指针存储 `*p = v`、`数据[i] = v`）运行时崩溃 0xC0000005：`emitPtrLoadStore` 先用 `mov rax, addr` 把目标地址放 rax，随后 i32 分支用 `mov eax, value` 加载值——**写 32 位寄存器 eax 会清零 rax 高 32 位**，最终 `mov [rax], eax` 写入地址 0x000000000000000A（值本身）而非真实地址，访问冲突。i8/i16（movsx eax）、u8/u16（movzx eax）分支同样存在
  - **原因**: x86-64 经典陷阱：32 位写清零高 32 位。LoadPtr 不崩是因为读完后地址不再需要；StorePtr 必须地址与值分寄存器保存
  - **解决**: StorePtr 值统一经 `rcx/ecx/cl/cx` 加载（`mov ecx, value` + `mov [rax], ecx`），rax 保留地址
  - **预防**: 代码生成中"地址与值共存"时，值加载禁止用 eax/rax（会破坏地址），用 rcx 系列；凡 `mov [rax], e` 前出现 `mov e`（32位）都要警惕清零
  - **权重**: 20.8（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 23:00] **问题类型**: 设计缺陷（权重 26）
  - **描述**: 数组元素布局方向与指针算术方向不一致导致 E2E 失败：局部数组曾用 Sub（元素 i = 基址 - i*8），而指针算术用 Add（p+1 = p+8）。`&数据3[0]` 基址上 `q = q+1` 指向未分配的栈区（打印垃圾 1044304），`r < s`（r=base, s=base-16）比较方向反转
  - **原因**: 数组槽登记（顺序/逆序）与 IR 地址计算（Add/Sub）需联动一致；曾尝试 4 种组合（Add/Sub × 顺序/逆序），方向与 C 语义（数组向上增长、p+1 指向下一元素）不符
  - **解决**: 统一为 C 语义——IR 元素地址 = base + i*8（Add），codegen 数组槽**逆序登记**（元素 N-1 先、基址槽最后=最深），与参数数组登记方式一致
  - **预防**: 数组布局是跨 IR/codegen 的全局决策，必须先定方向（C 语义 Add + 逆序登记）再实现；改方向必须同步 IR 全部地址计算（下标/lvalueAddress/初始化列表/补零）与 codegen 登记顺序；验证用 `&a[0]` 基址上 p+1 指向 a[1]
  - **权重**: 26（设计缺陷10 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 23:00] **问题类型**: 逻辑错误（权重 15.6）
  - **描述**: emitBoundsCheck 生成 CFG 时 okLabel 块被跳过：手动预取 `okLabel = bb(blockCounter_+1)` 后调用 `newBlock(errLabel)`（内部 blockCounter_++），再 `newBlock(okLabel)`（内部再 ++），导致 okLabel 实际 = bb(N+2) 而非 bb(N+1)，jmp 到错误块地址崩溃
  - **原因**: newBlock 内部递增 blockCounter_ 与手动预取标签双重递增，标签错位
  - **解决**: 先 `endBranch(bad.toString(), errLabel, okLabel)`（用预取标签）再 `setCurrentBlock(newBlock(errLabel))`、`setCurrentBlock(newBlock(okLabel))`——newBlock 内部递增推进计数器，与预取标签对齐
  - **预防**: 涉及"预取标签 + newBlock"的模式必须确认计数器递增次数与标签数一致；CFG 生成后可用 `cn ir` 检查块号是否连续
  - **权重**: 15.6（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

## 高权重问题（优化器增强完善C 新增，2026-08-13）

- [2026-08-13 09:30] **问题类型**: 逻辑错误（权重 16.8）
  - **描述**: 优化器新 Pass 单元测试构造的 IR 基本块缺少终止信息（terminated=false），导致：① DCE 认为返回寄存器 %v9 无人引用 → 级联删除全部 Add/Mul（O1BasicPasses 失败）；② 跨块 DCE 可达性分析从入口沿跳转目标遍历"断链" → 误删可达块（O2CSEAndCrossBlockDCE 失败，块数剩1）
  - **原因**: 真实 IR 契约要求每块 terminated=true + termKind（跳转/条件跳转/返回）；测试构造图省事省略，违反契约
  - **解决**: 测试 makeBlock 后补 endReturn/endJump 终止信息（块0→跳转块1、块1→返回 %v9、块2→返回）
  - **预防**: 凡手工构造 IRModule 的测试，每个块必须带终止信息；优化 Pass 依赖块终止（DCE 引用收集 / 跨块可达性）——无终止的块行为未定义
  - **权重**: 16.8（逻辑错误8 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-13 09:30] **问题类型**: 逻辑错误（权重 12.5）
  - **描述**: CopyPropagation/GlobalValue Pass 的 Load 命中槽值表登记替换时**无条件** changed=true，导致 fixpoint 永不收敛：第二次运行 Load 结果已无引用，但仍报告"修改"→ PassManager 无限迭代（O3RunTwiceStable 失败）
  - **原因**: 登记映射与"实际替换引用点"混淆——登记本身不是修改，replaceUses 实际替换才是
  - **解决**: 登记映射不设 changed；仅 replaceUses（替换引用点）返回 true 才置 changed
  - **预防**: fixpoint Pass 的 changed 语义 = "模块实际被修改"（指令/操作数变化），登记中间数据结构不算修改；新 Pass 必须验证"运行两次第二次返回 false"（收敛性测试）
  - **权重**: 12.5（逻辑错误8 × 详细分析1.5 × 解决方案1.3 × 预防措施1.0 × 已解决1.0）

- [2026-08-13 09:30] **问题类型**: 工具执行错误（权重 4）
  - **描述**: apply_diff 修改 copy_propagation.hpp 的 include 块时误删类声明起始行（class CopyPropagationPass : public Pass），导致 C2059/C2653/C2146 一串语法错误；cse.hpp 静态成员函数 patternOf 访问非静态成员 allowFloat_ 报 C2597
  - **原因**: ① include 附近插入点与类定义行相邻，diff 匹配边界误伤；② patternOf 声明为 static 却需访问成员变量
  - **解决**: ① read_file 复核后补全类定义；② patternOf 改为成员函数（去掉 static）
  - **预防**: apply_diff 修改头文件后立即 read_file 复核结构完整性；静态成员函数不能访问非静态成员（需改成员函数或传参）
  - **权重**: 4（工具执行错误2 × 详细分析1.5 × 解决方案1.3 × 预防措施1.0 × 已解决1.0）
