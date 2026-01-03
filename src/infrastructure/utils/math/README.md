# CN_Language 数学模块

## 概述

CN_Language数学模块是基础设施层的一部分，提供安全、可靠的数学运算功能。该模块采用模块化设计，遵循单一职责原则，每个子模块负责特定的功能领域。

## 模块化架构

### 核心模块
- **CN_math_core.h/c** - 数学核心结构、常量和基础函数
- **CN_math_safe_arithmetic.h/c** - 安全算术运算（溢出检查）
- **CN_math_random.h/c** - 随机数生成器
- **CN_math_basic.h/c** - 常用数学运算
- **CN_math.h** - 主头文件，包含所有模块

### 文件组织
- `CN_math_core.c` - 核心函数实现
- `CN_math_safe_arithmetic.c` - 整数安全运算
- `CN_math_safe_arithmetic_float.c` - 浮点数安全运算
- `CN_math_safe_arithmetic_batch.c` - 批量安全运算
- `CN_math_random.c` - 随机数生成器管理
- `CN_math_random_algorithms.c` - 随机数算法实现
- `CN_math_random_advanced.c` - 高级随机数功能
- `CN_math_basic_arithmetic.c` - 基本算术运算
- `CN_math_basic_trigonometric.c` - 三角函数和高级函数
- `CN_math.c` - 模块初始化和全局函数

## 功能特性

### 1. 安全算术运算（已实现）
- **整数溢出检查**：32位和64位有符号/无符号整数
- **浮点数异常检查**：NaN、无穷大、下溢检测
- **批量安全运算**：数组求和、乘积（防溢出）
- **高级安全运算**：幂运算、绝对值、取负、平均值

### 2. 随机数生成器（已实现）
- **多种算法支持**：
  - Xorshift算法（默认）
  - Mersenne Twister 19937算法
  - 线性同余生成器
  - 系统随机数生成器
- **范围限制生成**：指定范围内的均匀分布随机数
- **浮点随机数**：[0.0, 1.0)范围内的均匀分布
- **高级功能**：
  - 随机布尔值（可调概率）
  - 数组随机选择
  - Fisher-Yates洗牌算法
  - 随机字节序列生成

### 3. 基本数学运算（已实现）
- **算术运算**：绝对值、最大值、最小值、限制、线性插值
- **幂运算**：平方、立方、幂运算、平方根、立方根
- **对数运算**：自然对数、以10为底对数、以2为底对数
- **三角函数**：正弦、余弦、正切、反三角函数
- **双曲函数**：双曲正弦、双曲余弦、双曲正切
- **角度转换**：度转弧度、弧度转度
- **舍入函数**：向上取整、向下取整、四舍五入、截断
- **模运算**：浮点数取模、带符号取余

## 架构设计

### 设计原则
1. **单一职责原则**：每个模块只负责一个明确的功能领域
2. **接口隔离原则**：提供最小化的公共接口，隐藏实现细节
3. **开闭原则**：模块对修改封闭，对扩展开放
4. **内存安全**：使用安全的内存管理，防止内存泄漏
5. **错误处理**：全面的错误检查和报告机制

### 数据结构
```c
// 数学运算结果结构体
typedef struct Stru_CN_MathResult_t
{
    union
    {
        int32_t i32_result;     // 32位整数结果
        int64_t i64_result;     // 64位整数结果
        float   f32_result;     // 32位浮点数结果
        double  f64_result;     // 64位浮点数结果
    } value;                    // 运算结果值
    
    Eum_CN_MathError_t error;   // 错误码
    bool has_error;             // 是否有错误
} Stru_CN_MathResult_t;

// 随机数生成器结构体
struct Stru_CN_RandomGenerator_t
{
    Eum_CN_RandomAlgorithm_t algorithm;  // 算法类型
    uint64_t seed;                       // 当前种子
    void* state;                         // 算法特定状态
    size_t state_size;                   // 状态大小
};
```

## API使用示例

### 基本使用
```c
#include "src/infrastructure/utils/math/CN_math.h"

// 初始化数学模块
CN_math_init();

// 安全算术运算
int32_t result;
if (CN_math_safe_add_i32(1000000000, 2000000000, &result))
{
    printf("安全加法结果: %d\n", result);
}
else
{
    printf("加法溢出!\n");
}

// 随机数生成
Stru_CN_RandomGenerator_t* rng = CN_math_random_create(Eum_RANDOM_XORSHIFT, 12345);
int32_t random_num = CN_math_random_range_i32(rng, 1, 100);
float random_float = CN_math_random_float(rng);
CN_math_random_destroy(rng);

// 基本数学运算
float sqrt_value = CN_math_sqrt_f(25.0f);
float sin_value = CN_math_sin_f(CN_MATH_PI / 2.0f);
float clamped = CN_math_clamp_f(150.0f, 0.0f, 100.0f);

// 关闭数学模块
CN_math_shutdown();
```

### 安全算术运算
```c
// 安全整数运算
int32_t a = 1000000000;
int32_t b = 2000000000;
int32_t sum;

if (CN_math_safe_add_i32(a, b, &sum))
{
    printf("安全加法成功: %d + %d = %d\n", a, b, sum);
}
else
{
    printf("加法溢出!\n");
}

// 安全浮点数运算
float x = 1e30f;
float y = 1e30f;
float product;

if (CN_math_safe_mul_f(x, y, &product))
{
    printf("安全乘法成功: %e * %e = %e\n", x, y, product);
}
else
{
    printf("乘法溢出!\n");
}

// 批量安全运算
int32_t array[] = {100, 200, 300, 400, 500};
int32_t total;
if (CN_math_safe_sum_i32(array, 5, &total))
{
    printf("数组求和: %d\n", total);
}
```

### 随机数生成
```c
// 创建随机数生成器
Stru_CN_RandomGenerator_t* rng = CN_math_random_create(Eum_RANDOM_MT19937, 0);

// 生成各种类型的随机数
uint32_t rand_u32 = CN_math_random_u32(rng);
uint64_t rand_u64 = CN_math_random_u64(rng);
float rand_float = CN_math_random_float(rng);
double rand_double = CN_math_random_double(rng);

// 生成指定范围内的随机数
int32_t dice_roll = CN_math_random_range_i32(rng, 1, 6);
float random_angle = CN_math_random_range_float(rng, 0.0f, CN_MATH_TWOPI);

// 高级功能
bool heads = CN_math_random_bool(rng, 0.5f);  // 50%概率为真

int numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
CN_math_random_shuffle(rng, numbers, 10, sizeof(int));

// 清理
CN_math_random_destroy(rng);
```

### 全局随机数函数
```c
// 使用全局随机数生成器（自动初始化）
uint32_t rand1 = CN_math_random_global_u32();
float rand2 = CN_math_random_global_float();
int32_t rand3 = CN_math_random_global_range_i32(1, 100);

// 自定义全局生成器
CN_math_random_global_init(Eum_RANDOM_MT19937, 12345);

// 使用后关闭（可选）
CN_math_random_global_shutdown();
```

## 内存管理

### 资源管理
- 随机数生成器需要显式创建和销毁
- 数学模块需要初始化和关闭
- 所有分配的内存都有相应的释放函数

### 错误处理
- 安全算术函数返回布尔值表示成功/失败
- 数学错误通过错误码和结果结构体报告
- 无效参数检查防止未定义行为

## 性能考虑

### 时间复杂度
- 安全算术运算：O(1)（常数时间）
- 随机数生成：O(1)（大多数算法）
- 批量运算：O(n)（线性时间）
- 数学函数：取决于具体实现

### 空间复杂度
- 随机数生成器：O(1)（固定大小状态）
- 数学运算：O(1)（栈分配）
- 模块状态：O(1)（全局变量）

## 限制和未来改进

### 当前限制
1. 部分高级数学函数（如伽马函数、误差函数）为占位符实现
2. 随机数分布函数（正态分布、泊松分布等）需要完善
3. 向量和矩阵运算尚未实现
4. 复数运算支持需要添加

### 计划改进
1. 添加完整的特殊函数实现
2. 完善随机数分布函数
3. 添加向量和矩阵运算模块
4. 添加复数运算支持
5. 性能优化和基准测试
6. SIMD指令优化支持

## 测试建议

### 单元测试
- 测试各种边界条件的安全算术运算
- 测试随机数生成器的均匀性和周期性
- 测试数学函数的精度和正确性
- 测试错误处理机制

### 集成测试
- 测试模块初始化和关闭
- 测试内存管理（无泄漏）
- 测试并发访问（如果支持）
- 测试性能基准

## 相关文档

- [API文档](../../../docs/api/infrastructure/utils/CN_math_API.md)（需要创建）
- [架构设计原则](../../../docs/architecture/架构设计原则.md)
- [编码标准](../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)
- [数学库参考](https://en.cppreference.com/w/c/numeric/math)

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本，实现核心数学功能
- 安全算术运算（整数和浮点数）
- 随机数生成器（多种算法）
- 基本数学运算（算术、三角、对数等）
- 模块化架构设计
- 完整的错误处理机制

### 计划版本
- v1.1.0: 添加向量和矩阵运算
- v1.2.0: 添加复数运算支持
- v2.0.0: 性能优化和SIMD支持

## 贡献指南

1. 遵循项目编码标准
2. 添加新功能时同时添加测试
3. 更新相关文档
4. 性能改进需要提供基准测试数据
5. 保持向后兼容性
6. 遵循模块化设计原则

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 技术支持

### 问题报告
- GitHub Issues: [项目Issues页面](https://github.com/ChenChao-HuaiAn/CN_Language/issues)
- 邮件支持: 项目维护团队

### 常见问题
1. **Q: 如何从标准数学函数迁移到安全算术函数？**
   A: 将直接算术运算替换为对应的安全函数，并检查返回值。

2. **Q: 随机数生成器的种子如何设置？**
   A: 创建生成器时传入种子参数，0表示使用当前时间。

3. **Q: 性能开销如何？**
   A: 安全算术运算有额外检查开销，但可防止未定义行为。随机数生成性能取决于算法。

4. **Q: 线程安全吗？**
   A: 当前版本不是线程安全的，需要外部同步。每个线程应使用独立的随机数生成器。

5. **Q: 如何检测数学错误？**
   A: 安全函数返回布尔值，核心函数使用错误码。检查返回值或错误标志。
