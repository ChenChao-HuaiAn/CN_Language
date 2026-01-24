# Arena 分配器集成指南 - IR 模块

## 快速集成步骤

### 第一步：修改 IR 模块数据结构

**文件**：`include/cnlang/ir/ir.h`

```c
#include "cnlang/support/memory/arena.h"

// IR 模块（一个编译单元）
typedef struct CnIrModule {
    CnArena *arena;             // 新增：Arena 分配器
    CnIrFunction *first_func;
    CnIrFunction *last_func;
    CnTargetTriple target;
    CnCompileMode compile_mode;
} CnIrModule;
```

### 第二步：修改模块创建和销毁

**文件**：`src/ir/core/ir.c`

```c
CnIrModule *cn_ir_module_new() {
    CnIrModule *module = (CnIrModule *)malloc(sizeof(CnIrModule));
    if (module) {
        // 创建 Arena
        module->arena = cn_arena_new(0);  // 使用默认 64KB 块大小
        if (!module->arena) {
            free(module);
            return NULL;
        }
        
        module->first_func = NULL;
        module->last_func = NULL;
        memset(&module->target, 0, sizeof(module->target));
        module->compile_mode = CN_COMPILE_MODE_HOSTED;
    }
    return module;
}

void cn_ir_module_free(CnIrModule *module) {
    if (!module) return;
    
    // Arena 会统一释放所有 IR 对象，无需逐个释放
    cn_arena_free(module->arena);
    
    // 注意：现在不需要遍历释放函数/块/指令了
    free(module);
}
```

### 第三步：修改对象创建函数

**文件**：`src/ir/core/ir.c`

#### 3.1 函数创建

```c
CnIrFunction *cn_ir_function_new(CnIrModule *module, const char *name, CnType *return_type) {
    // 使用 Arena 分配
    CnIrFunction *func = CN_ARENA_ALLOC(module->arena, CnIrFunction);
    if (func) {
        // name 可以直接使用传入指针，或从 Arena 分配拷贝
        func->name = name;  // 如果 name 生命周期足够长
        // 或者：
        // size_t len = strlen(name);
        // char *name_copy = CN_ARENA_ALLOC_ARRAY(module->arena, char, len + 1);
        // strcpy(name_copy, name);
        // func->name = name_copy;
        
        func->return_type = return_type;
        func->params = NULL;
        func->param_count = 0;
        func->locals = NULL;
        func->local_count = 0;
        func->first_block = NULL;
        func->last_block = NULL;
        func->next_reg_id = 0;
        func->next = NULL;
    }
    return func;
}
```

#### 3.2 基本块创建

```c
CnIrBasicBlock *cn_ir_basic_block_new(CnIrModule *module, const char *name_hint) {
    CnIrBasicBlock *block = CN_ARENA_ALLOC(module->arena, CnIrBasicBlock);
    if (block) {
        block->name = name_hint;  // 直接使用，或从 Arena 拷贝
        block->first_inst = NULL;
        block->last_inst = NULL;
        block->preds = NULL;
        block->succs = NULL;
        block->next = NULL;
        block->prev = NULL;
    }
    return block;
}
```

#### 3.3 指令创建

```c
CnIrInst *cn_ir_inst_new(CnIrModule *module, CnIrInstKind kind, 
                         CnIrOperand dest, CnIrOperand src1, CnIrOperand src2) {
    CnIrInst *inst = CN_ARENA_ALLOC(module->arena, CnIrInst);
    if (inst) {
        inst->kind = kind;
        inst->dest = dest;
        inst->src1 = src1;
        inst->src2 = src2;
        inst->extra_args = NULL;
        inst->extra_args_count = 0;
        inst->next = NULL;
        inst->prev = NULL;
    }
    return inst;
}
```

#### 3.4 额外参数分配

```c
// 在需要分配 extra_args 的地方
CnIrInst *inst = cn_ir_inst_new(module, ...);
inst->extra_args_count = count;
inst->extra_args = CN_ARENA_ALLOC_ARRAY(module->arena, CnIrOperand, count);
```

### 第四步：修改 irgen 调用

**文件**：`src/ir/gen/irgen.c`

#### 4.1 修改上下文

```c
typedef struct CnIrGenContext {
    CnIrModule *module;
    CnIrFunction *current_func;
    CnIrBasicBlock *current_block;
    CnIrBasicBlock *loop_exit;
    CnIrBasicBlock *loop_continue;
} CnIrGenContext;

CnIrGenContext *cn_ir_gen_context_new() {
    CnIrGenContext *ctx = malloc(sizeof(CnIrGenContext));
    ctx->module = cn_ir_module_new();  // 已包含 Arena
    ctx->current_func = NULL;
    ctx->current_block = NULL;
    ctx->loop_exit = NULL;
    ctx->loop_continue = NULL;
    return ctx;
}
```

#### 4.2 修改对象创建调用

```c
// 所有创建 IR 对象的地方，传入 module
CnIrFunction *func = cn_ir_function_new(ctx->module, name, return_type);
CnIrBasicBlock *block = cn_ir_basic_block_new(ctx->module, "entry");
CnIrInst *inst = cn_ir_inst_new(ctx->module, CN_IR_INST_MOV, dest, src1, src2);
```

### 第五步：清理不必要的 free 调用

**关键点**：

1. **删除 `cn_ir_module_free` 中的遍历释放代码**
   - Arena 会统一释放，不需要逐个释放函数/块/指令

2. **删除 extra_args 的 free**
   ```c
   // 删除这样的代码：
   // if (inst->extra_args) free(inst->extra_args);
   ```

3. **保留字符串的生命周期考虑**
   - 如果字符串来自 AST，可以直接使用指针
   - 如果需要拷贝，使用 Arena 分配

### 第六步：更新 API 签名

**文件**：`include/cnlang/ir/ir.h`

```c
// 更新函数签名，添加 module 参数
CnIrFunction *cn_ir_function_new(CnIrModule *module, const char *name, CnType *return_type);
CnIrBasicBlock *cn_ir_basic_block_new(CnIrModule *module, const char *name_hint);
CnIrInst *cn_ir_inst_new(CnIrModule *module, CnIrInstKind kind, CnIrOperand dest, CnIrOperand src1, CnIrOperand src2);
```

---

## 注意事项

### ✅ 推荐做法

1. **统一从 Arena 分配**
   - 所有 IR 对象（函数、块、指令）都用 Arena
   - 减少内存碎片，提升缓存友好性

2. **字符串策略**
   - 如果来自 AST 且生命周期足够：直接使用指针
   - 如果需要独立拷贝：用 `CN_ARENA_ALLOC_ARRAY` 分配

3. **参数数组**
   - extra_args 也从 Arena 分配
   - 简化内存管理

### ⚠️ 注意事项

1. **生命周期**
   - Arena 统一释放，不支持单独释放对象
   - 适合 IR 这种"一次性生成，统一销毁"的场景

2. **线程安全**
   - Arena 本身不是线程安全的
   - 每个编译单元用独立 Arena（当前架构已满足）

3. **错误处理**
   - Arena 分配失败仍需检查返回值
   - 但实际失败概率极低（预分配块）

---

## 预期效果

| 指标 | 优化前 | 优化后 | 改善 |
|-----|-------|--------|-----|
| malloc 调用次数（hello_world.cn） | ~100 | ~2 | 减少 98% |
| IR 生成耗时 | 0.046 ms | ~0.030 ms | 减少 35% |
| 内存峰值 | 基准 | 减少 15-20% | 显著改善 |

---

## 测试验证

### 回归测试

```bash
# 运行所有 IR 相关测试
ctest -C Debug -R "ir"

# 运行集成测试
ctest -C Debug -R "integration"

# 内存测试
ctest -C Debug -R "memory"
```

### 性能测试

```bash
# 使用 cnperf 测试优化效果
cnperf examples/ --output=after_arena.csv

# 对比优化前后
# 关注 "IR 生成" 阶段的耗时变化
```

---

## 进度追踪

- [ ] 修改 `CnIrModule` 结构
- [ ] 修改 `cn_ir_module_new/free`
- [ ] 修改 `cn_ir_function_new`
- [ ] 修改 `cn_ir_basic_block_new`
- [ ] 修改 `cn_ir_inst_new`
- [ ] 修改 irgen.c 中的调用
- [ ] 更新 API 签名
- [ ] 清理不必要的 free
- [ ] 运行回归测试
- [ ] 性能对比验证

---

**预计工时**：2-4 小时  
**风险等级**：低（修改集中，影响范围明确）  
**建议时机**：立即实施
