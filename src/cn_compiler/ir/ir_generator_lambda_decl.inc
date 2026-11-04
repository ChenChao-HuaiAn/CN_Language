// 闭包（lambda/方法值）生成族声明片段（#1927 拆分件·自 ir_generator.hpp 类内——该头触
//   1000 冻结线「先拆再加」：994 行只剩 6 行余量，1571/1578/1920/1963 每轮被迫压缩注释）。
//   类内 .inc 包含形态（先例 ir_string_accum_decl.inc / ir_string_temp_trace_decl.inc）。
//   范围=原 hpp 私有段同一连续块·逐字搬移零语义变更（实现全在 ir_lambda.cpp 等·本片段只放声明）：
//     ① lastLambda* 一次性通道 + 1571 捕获名帧栈（定义处捕获登记·genVarDecl/闭包调用消费）；
//     ② 432 蹦床闭包模型私辅（buildLambdaUserParams/buildClosureParams943/
//        buildClosureTrampoline943/buildLambdaTrampoline/emitLambdaSnapshotStores/
//        lambdaSlotKey/isByRefCapture——[&] 捕获读取解引用语义）；
//     ③ 031 绑定方法值两槽值族（方法值<类,签名> = 栈上 {this, code} 两槽·arity 映射单点收口）；
//     ④ 482 载体快照槽族 + 940 快照值求值单一归属（捕获值=定义处深拷独立副本）；
//     ⑤ 闭包关联登记表（closureInfo_/lambdaCaptureMeta482_/lambdaVarNames_ 等·
//        655 返工「该名是 lambda 变量」全集与 clear 纪律解耦）。
//   包含位=原声明处：成员声明顺序保持不变（默认成员初始化器按声明顺序执行）。

    // 最近一次 lambda 生成的匿名函数名与捕获变量列表（Task 2.10， genVarDecl 遇 LambdaExpr 初始值时登记闭包关联）
    std::string lastLambdaName_;
    std::vector<std::string> lastLambdaCaptures_;
    std::string lastLambdaReturnIrType_;  // 最近 lambda 的返回 IR 类型（闭包调用结果类型）
    std::string lastLambdaSrcReturnType_;  // 485：最近 lambda 的源码返回类型（sret 判定）
    // 337-a（T53 家系·闭包调用路径）：最近一次闭包（lambda / 方法作值）的**用户 形参源码类型列表**——闭包调用 `cb(实参)` 展开为 `Call(捕获实参..., 用户实参...)`
    std::vector<std::string> lastLambdaParamTypes_;
    // 最近 lambda 各捕获是否引用捕获（缺陷修复：决定定义处捕获实参是 值快照（[=]/[变量]）还是变量地址指针（[&]），genVarDecl 登记闭包时使用）
    std::vector<char> lastLambdaCaptureRefs_;
    // #1578：最近 lambda 体返回位串拥有性（定义处按返回形态判定·与 lastLambda* 同生命周期一次性通道）——genVarDecl 登记 ClosureInfo/载体静态元数据共用。
    bool lastLambdaReturnBodyOwned_ = false;
    // 1571：lambda 体捕获名帧栈（返回位拥有契约：捕获槽=只读快照/外层地址，返回 字符串 须复制落堆）
    std::vector<std::unordered_set<std::string>> lambdaCaptureFrames_;
    // 查询变量是否为"引用捕获参数"（lambda 匿名函数内 [&] 捕获的参数槽存 被捕获变量地址，读取须解引用、赋值须经指针——规格书04-一D 引用语义）
    bool isByRefCapture(const std::string& name) const;
    // ---- 432（#276 波3·蹦床闭包模型）lambda 生成族私辅（实现在 ir_lambda.cpp）----
    // lambda 用户参数段构建（?lambdaN 与蹦床共用——纪律二当场归纳）：参数槽/ varStack 登记，引用参数（T&）按 ptr+byRef 形态（A-1 同款）
    void buildLambdaUserParams(ir::IRFunction& func, LambdaExpr* node);
    // 943：lambda/方法值蹦床单点（031 起方法值蹦床已废=仅 lambda 用）
    //   （1013 排版压缩 1006→997·903/943 收口先例纯排版零语义：参数合行不动 token 序）
    void buildClosureParams943(ir::IRFunction& func, const std::vector<const ParamDecl*>& params);
    void buildClosureTrampoline943(const std::string& trapName, const std::string& calleeName,
                                   const std::string& closureId, const std::string& returnSrcType,
                                   const std::string& returnIrType,
                                   const std::vector<const ParamDecl*>& params,
                                   const std::vector<std::string>& capturedNames,
                                   const std::vector<char>& captureRefs, const SourceLocation& loc);
    // ---- 031（#1075·绑定方法值一等公民化·两槽值族·实现在 ir_method_value.cpp）----
    // 值模型：`方法值<类,签名>` = 栈上两槽 {this, code}（unique / unique$s1——
    //   与 i128 双槽命名同规·varSlots=2 登记）。「一值=两位」映射单点收口本族
    //   （031 风险表第一条：禁散点 arity 映射）。
    // 判定：表达式是否为绑定方法值求值（MemberExpr 的 isMethodValue 标记）
    static bool isMethodValueExpr(const Expr* e);
    // 判定：变量名是否为方法值变量（lookupSrcType=方法值串·类型驱动）
    bool isMethodValueVar(const std::string& name) const;
    // 两槽物化：Alloca 临时（2 槽）+ Store this/code → 返回临时基名
    std::string emitMethodValuePair(const ir::IRValue& thisVal,
                                    const ir::IRValue& codeVal,
                                    const SourceLocation& loc);
    // 读取两槽（Load×2——基名/基名$s1）
    void loadMethodValuePair(const std::string& unique, ir::IRValue& thisOut,
                             ir::IRValue& codeOut, const SourceLocation& loc);
    // 声明位/赋值位统一：源表达式（方法值表达式/方法值变量）→ 目标变量两槽。
    //   true=已处理（两槽整拷）；false=源非方法值（调用方回原路）
    bool tryMethodValueAssign(const std::string& targetUnique, const Expr* src,
                              const SourceLocation& loc);
    // 声明位钩子（genExprInitializer 消费）：方法值初始化两槽整拷
    bool tryGenMethodValueInit(VarDecl* node, const DeclGenCtx& ctx);
    // 赋值位钩子（assignToIdentifierTarget 消费）：方法值变量赋值两槽整拷
    bool identifierMethodValueAssign(AssignmentExpr* node, const IdentifierExpr* ident,
                                     const std::string& unique);
    // 实参位判定（方法值表达式 or 方法值变量）
    bool isMethodValueArg(const Expr* arg) const;
    // 实参位单点展开：单个实参若为方法值 → 追加两个物理实参（this、code）
    //   （buildCallArgs 与 genMethodValueCall 位共用·per-arg 语义）
    void expandMethodValueArg(Expr* arg, std::vector<ir::IRValue>& args,
                              const SourceLocation& loc);
    // 快照静态槽注册 + 定义处固化（外层块发射）：值捕获=StorePtr 定义处值； [&]=StorePtr 变量地址；结构体值捕获=CopyStruct 槽（与 genVarDecl captureArgs 固化同构——数据流终点不同：captureArgs=局部值列表，本处=.data 静态槽·第 2 处重复留标记：与 ir_stmt_decl_init 固化段互为镜像）
    void emitLambdaSnapshotStores(const std::string& lambdaName,
        const std::vector<std::string>& capturedNames,
        const std::vector<char>& captureRefs,
        const SourceLocation& loc);
    // 蹦床合成函数 ?lambdaN$trap 构建（签名=用户签名）：体=LoadPtr 快照槽
    //   还原捕获实参 + Call ?lambdaN。lambda 值=蹦床地址（可安全作 fnptr 返回/传参——值捕获快照随槽全局存活=逃逸安全；[&] 逃逸=悬垂域挂
    //   #435/#468 逃逸检查面，本波只做值捕获高阶形态）
    void buildLambdaTrampoline(const std::string& lambdaName, const std::string& returnIrType,
        LambdaExpr* node,
        const std::vector<std::string>& capturedNames,
        const std::vector<char>& captureRefs,
        const SourceLocation& loc);
    // lambda 快照静态槽键（?gstatic_ 前缀外的注册键；lambdaName 唯一保证不撞）
    std::string lambdaSlotKey(const std::string& lambdaName, const std::string& cap) const;
    // ---- 482 载体快照槽族私辅（实现/数据/语义见 ir_lambda.cpp 与 ir_generator_types.hpp）----
    // 1360 结果化：无值=捕获源类型查表 miss（原哨兵 8 与合法槽宽 8 重叠·退役；
    //   消费点 value_or(8) 显形默认——8=缺省槽宽，语义在调用点写出）
    cn::Optional<int> captureBytes482(const std::string& capSrc);
    void emitSharedToCarrier482(const std::string& lambdaName, const std::vector<ClosureInfo::CarrierCapture>& slots, const SourceLocation& loc);
    void emitCarrierToShared482(const std::string& lambdaName, const std::vector<ClosureInfo::CarrierCapture>& slots, const SourceLocation& loc);
    void emitCarrierToCarrier482(const std::vector<ClosureInfo::CarrierCapture>& srcSlots, const std::vector<ClosureInfo::CarrierCapture>& dstSlots, const SourceLocation& loc);
    // 1360 结果化：无值=rhs 非 lambda 来源（原空串哨兵退役）
    cn::Optional<std::string> resolveLambdaOrigin482(Expr* rhs, const std::string& literalName = "");
    void updateCarrierOnAssign482(const std::string& varName, Expr* rhs, const SourceLocation& loc, const std::string& literalName = "", bool fromPlainAssign = false);
    std::vector<ClosureInfo::CarrierCapture> makeCarrierSlots482(const std::string& varName, const std::string& lambdaName, const SourceLocation& loc);
    void emitReturnOriginSnapshot482(Expr* rhs);
    void recordLambdaCaptureMeta482(const std::string& lambdaName, const std::vector<std::string>& capturedNames, const std::vector<char>& captureRefs, const std::vector<std::string>& srcTypes);
    // ---- 940 快照值求值单一归属（字符串/容器/类=定义处深拷独立副本·静态固化与同帧快径共用）---- #1880 值捕获数组元素级递归深拷（v2 同构·940 先例数组版）+ 数组容器/类元素内联壳拷贝构造（1367·v2 同构）----
    ir::IRValue evalValueCaptureCopy(const std::string& capName, const std::string& capSrc, const SourceLocation& loc); void emitValueCaptureArrayDeepCopy(const ir::IRValue&, const ir::IRValue&, const std::string&, const SourceLocation&, int); void emitValueCaptureElemClassCopy(const ir::IRValue&, const ir::IRValue&, const std::string&, const SourceLocation&);
    // 闭包关联：变量源码名 -> {匿名函数名, 捕获变量列表, 定义处捕获实参, 返回 IR 类型}（ClosureInfo 见 ir_generator_types.hpp）
    std::unordered_map<std::string, ClosureInfo> closureInfo_;
    std::unordered_map<std::string, LambdaCaptureMeta482> lambdaCaptureMeta482_;
    std::unordered_map<std::string, LambdaStaticInfo482> lambdaStaticInfo482_;
    std::unordered_map<std::string, std::string> funcReturnLambda482_;
    std::string currentReturnOrigin482_;
    std::string currentSigKey482_;
    std::unordered_set<std::string> carrierAssigned482_;
    // 655 返工：曾以 lambda/方法值登记过闭包关联的变量名全集。closureInfo_ 快径 受 visitLambdaExpr 入口 clear 纪律（485）影响（720 实录：同函数后定义的
    //   lambda 清掉先前登记=首个调用退化 432 蹦床通道·语义等价合法面），本集与 clear 无关地记住「该名是 lambda 变量」——间接调用尾据此区分「蹦床通道
    //   合法面」（不报）与「真全域 miss」（报 #655 诊断）。816 扩=拷贝传递登记。
    std::unordered_set<std::string> lambdaVarNames_;
