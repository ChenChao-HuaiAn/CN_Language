// IR·循环累积拼接提升族声明片段（#1769 轮拆分 自 ir_generator.hpp 类内——
//   该头触 1000 冻结线「先拆再加」；类内 .inc 包含形态（X-Macro 同族先例）。
//   750 判据/铁律/序列见 ir_string_accum.cpp 文件头·001 §10.3 构建器模型）。

    // ---- 750：循环累积拼接提升（判据/铁律/序列见 ir_string_accum.cpp 文件头·001 §10.3 构建器模型）----
    //   三者成对：tryBegin（循环前 new+append 初值·成功才接管）/ emitStringAccumAssign
    //   （循环体内累积赋值改写 append）/ finish（出口块 finish+free+回写）；识别失败=零发射原路径。
    bool tryBeginStringAccum(const Stmt* body, const Expr* cond,
                             const Stmt* init, const Expr* update,
                             const SourceLocation& loc);
    bool beginStringAccumScan(const Stmt* body, const Expr* cond,
                              const Stmt* init, const Expr* update,
                              const SourceLocation& loc);
    bool emitStringAccumAssign(AssignmentExpr* node);
    bool emitStringAccumAssignInner(AssignmentExpr* node);
    void finishStringAccum(const SourceLocation& loc);
    // 750 复审返工（P0-1/P0-2/P0-3）：函数级别名逃逸/lambda 预扫描——自由函数/类方法/泛型实例/lambda 体入口调用
    void beginAccumScope(Stmt* body);
    // 提升上下文栈（条目 StringAccumCtx·types 头外迁）；遍历降级循环不优化（v2 同口径）
