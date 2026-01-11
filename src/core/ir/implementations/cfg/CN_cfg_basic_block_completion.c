// 这是CN_cfg_basic_block.c文件的剩余部分
// 需要将这些内容添加到原文件的末尾

bool F_basic_block_remove_dominated(Stru_BasicBlock_t* block, 
                                    Stru_BasicBlock_t* dominated)
{
    if (!block || !dominated)
    {
        return false;
    }
    
    // 查找被支配者索引
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < block->dominated_count; i++)
    {
        if (block->dominated[i] == dominated)
        {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return false; // 不是被支配者
    }
    
    // 移动后续元素
    for (size_t i = index; i < block->dominated_count - 1; i++)
    {
        block->dominated[i] = block->dominated[i + 1];
    }
    
    block->dominated_count--;
    
    return true;
}

bool F_basic_block_dominates(const Stru_BasicBlock_t* dominator, 
                             const Stru_BasicBlock_t* dominated)
{
    if (!dominator || !dominated)
    {
        return false;
    }
    
    // 检查支配者是否在支配者集合中
    for (size_t i = 0; i < dominated->dominator_count; i++)
    {
        if (dominated->dominators[i] == dominator)
        {
            return true;
        }
    }
    
    return false;
}

bool F_basic_block_strictly_dominates(const Stru_BasicBlock_t* dominator, 
                                      const Stru_BasicBlock_t* dominated)
{
    if (!dominator || !dominated || dominator == dominated)
    {
        return false;
    }
    
    return F_basic_block_dominates(dominator, dominated);
}

// ============================================================================
// 查询函数实现
// ============================================================================

const char* F_basic_block_get_name(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return NULL;
    }
    
    return block->name;
}

bool F_basic_block_set_name(Stru_BasicBlock_t* block, const char* name)
{
    if (!block || !name)
    {
        return false;
    }
    
    // 释放旧名称
    if (block->name)
    {
        cn_free(block->name);
    }
    
    // 分配新名称
    block->name = cn_strdup(name);
    return block->name != NULL;
}

Eum_BasicBlockType F_basic_block_get_type(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return Eum_BASIC_BLOCK_NORMAL;
    }
    
    return block->type;
}

void F_basic_block_set_type(Stru_BasicBlock_t* block, Eum_BasicBlockType type)
{
    if (!block)
    {
        return;
    }
    
    block->type = type;
}

size_t F_basic_block_get_id(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return 0;
    }
    
    return block->id;
}

bool F_basic_block_is_empty(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return true;
    }
    
    return block->instruction_count == 0;
}

bool F_basic_block_is_jump_only(const Stru_BasicBlock_t* block)
{
    if (!block || block->instruction_count == 0)
    {
        return false;
    }
    
    // 检查最后一条指令是否为跳转指令
    Stru_IrInstruction_t* last_instr = block->instructions[block->instruction_count - 1];
    if (!last_instr)
    {
        return false;
    }
    
    // 检查是否为跳转或分支指令
    return (last_instr->type == Eum_IR_INSTR_BRANCH || 
            last_instr->type == Eum_IR_INSTR_JUMP ||
            last_instr->type == Eum_IR_INSTR_RETURN);
}

// ============================================================================
// 工具函数实现
// ============================================================================

const char* F_basic_block_type_to_string(Eum_BasicBlockType type)
{
    switch (type)
    {
        case Eum_BASIC_BLOCK_NORMAL:
            return "NORMAL";
        case Eum_BASIC_BLOCK_ENTRY:
            return "ENTRY";
        case Eum_BASIC_BLOCK_EXIT:
            return "EXIT";
        case Eum_BASIC_BLOCK_LOOP_HEADER:
            return "LOOP_HEADER";
        case Eum_BASIC_BLOCK_LOOP_BODY:
            return "LOOP_BODY";
        case Eum_BASIC_BLOCK_LOOP_EXIT:
            return "LOOP_EXIT";
        case Eum_BASIC_BLOCK_IF_THEN:
            return "IF_THEN";
        case Eum_BASIC_BLOCK_IF_ELSE:
            return "IF_ELSE";
        case Eum_BASIC_BLOCK_SWITCH_CASE:
            return "SWITCH_CASE";
        case Eum_BASIC_BLOCK_HANDLER:
            return "HANDLER";
        default:
            return "UNKNOWN";
    }
}

char* F_generate_basic_block_name(size_t id)
{
    // 分配足够的内存：前缀"BB_" + 数字 + 空字符
    size_t buffer_size = 32; // 足够存储"BB_4294967295\0"
    char* name = (char*)cn_malloc(buffer_size);
    if (!name)
    {
        return NULL;
    }
    
    snprintf(name, buffer_size, "BB_%zu", id);
    return name;
}

void F_basic_block_reset_visited(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return;
    }
    
    block->visited = false;
}

void F_basic_block_set_visited(Stru_BasicBlock_t* block, bool visited)
{
    if (!block)
    {
        return;
    }
    
    block->visited = visited;
}

bool F_basic_block_is_visited(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    return block->visited;
}

// ============================================================================
// 验证函数实现
// ============================================================================

bool F_validate_basic_block(const Stru_BasicBlock_t* block, char** error_message)
{
    if (!block)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块指针为NULL");
        }
        return false;
    }
    
    // 检查名称
    if (!block->name)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块名称为NULL");
        }
        return false;
    }
    
    // 检查ID
    if (block->id == 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块ID为0");
        }
        return false;
    }
    
    // 检查指令数组
    if (!block->instructions && block->instruction_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("指令数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查指令计数
    if (block->instruction_count > block->instruction_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("指令计数超过容量");
        }
        return false;
    }
    
    // 检查前驱数组
    if (!block->predecessors && block->predecessor_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("前驱数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查前驱计数
    if (block->predecessor_count > block->predecessor_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("前驱计数超过容量");
        }
        return false;
    }
    
    // 检查后继数组
    if (!block->successors && block->successor_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("后继数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查后继计数
    if (block->successor_count > block->successor_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("后继计数超过容量");
        }
        return false;
    }
    
    // 检查支配者数组
    if (!block->dominators && block->dominator_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("支配者数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查支配者计数
    if (block->dominator_count > block->dominator_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("支配者计数超过容量");
        }
        return false;
    }
    
    // 检查被支配者数组
    if (!block->dominated && block->dominated_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("被支配者数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查被支配者计数
    if (block->dominated_count > block->dominated_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("被支配者计数超过容量");
        }
        return false;
    }
    
    // 检查指令指针
    for (size_t i = 0; i < block->instruction_count; i++)
    {
        if (!block->instructions[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("指令数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查前驱指针
    for (size_t i = 0; i < block->predecessor_count; i++)
    {
        if (!block->predecessors[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("前驱数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查后继指针
    for (size_t i = 0; i < block->successor_count; i++)
    {
        if (!block->successors[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("后继数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查支配者指针
    for (size_t i = 0; i < block->dominator_count; i++)
    {
        if (!block->dominators[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("支配者数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查被支配者指针
    for (size_t i = 0; i < block->dominated_count; i++)
    {
        if (!block->dominated[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("被支配者数组中存在NULL指针");
            }
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// 静态函数实现
// ============================================================================

static bool ensure_instruction_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->instruction_count < block->instruction_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->instruction_capacity * 2;
    if (new_capacity < INITIAL_INSTRUCTION_CAPACITY)
    {
        new_capacity = INITIAL_INSTRUCTION_CAPACITY;
    }
    
    // 重新分配数组
    Stru_IrInstruction_t** new_array = (Stru_IrInstruction_t**)cn_realloc(
        block->instructions, sizeof(Stru_IrInstruction_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->instructions = new_array;
    block->instruction_capacity = new_capacity;
    
    return true;
}

static bool ensure_predecessor_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->predecessor_count < block->predecessor_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->predecessor_capacity * 2;
    if (new_capacity < INITIAL_PREDECESSOR_CAPACITY)
    {
        new_capacity = INITIAL_PREDECESSOR_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->predecessors, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->predecessors = new_array;
    block->predecessor_capacity = new_capacity;
    
    return true;
}

static bool ensure_successor_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->successor_count < block->successor_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->successor_capacity * 2;
    if (new_capacity < INITIAL_SUCCESSOR_CAPACITY)
    {
        new_capacity = INITIAL_SUCCESSOR_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->successors, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->successors = new_array;
    block->successor_capacity = new_capacity;
    
    return true;
}

static bool ensure_dominator_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->dominator_count < block->dominator_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->dominator_capacity * 2;
    if (new_capacity < INITIAL_DOMINATOR_CAPACITY)
    {
        new_capacity = INITIAL_DOMINATOR_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->dominators, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->dominators = new_array;
    block->dominator_capacity = new_capacity;
    
    return true;
}

static bool ensure_dominated_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->dominated_count < block->dominated_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->dominated_capacity * 2;
    if (new_capacity < INITIAL_DOMINATED_CAPACITY)
    {
        new_capacity = INITIAL_DOMINATED_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->dominated, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->dominated = new_array;
    block->dominated_capacity = new_capacity;
    
    return true;
}

static void free_instruction_array(Stru_IrInstruction_t** instructions, size_t count)
{
    if (!instructions)
    {
        return;
    }
    
    // 注意：不释放指令本身，由调用者负责
    cn_free(instructions);
}

static void free_basic_block_array(Stru_BasicBlock_t** blocks, size_t count)
{
    if (!blocks)
    {
        return;
    }
    
    // 注意：不释放基本块本身，由调用者负责
    cn_free(blocks);
}
