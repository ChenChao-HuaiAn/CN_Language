#ifndef CN_FRONTEND_PREPROCESSOR_H
#define CN_FRONTEND_PREPROCESSOR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CnDiagnostics;

/*
 * 预处理器宏定义
 * 支持对象宏和函数宏
 */
typedef struct CnMacro {
    char *name;                     // 宏名称
    size_t name_length;             // 宏名称长度
    char *replacement;              // 替换文本
    size_t replacement_length;      // 替换文本长度
    char **params;                  // 参数列表 (NULL 表示对象宏)
    size_t param_count;             // 参数数量
    bool is_function_like;          // 是否是函数宏
    int defined_line;               // 定义位置 (用于诊断)
} CnMacro;

/*
 * 条件编译栈项
 */
typedef struct CnConditionFrame {
    bool active;                    // 当前条件块是否激活
    bool has_executed;              // 是否已有分支执行过
    int start_line;                 // 条件起始行
} CnConditionFrame;

/*
 * 预处理器上下文
 */
typedef struct CnPreprocessor {
    const char *source;             // 输入源代码
    size_t source_length;           // 源代码长度
    const char *filename;           // 文件名
    
    char *output;                   // 预处理后的输出
    size_t output_length;           // 输出长度
    size_t output_capacity;         // 输出缓冲区容量
    
    CnMacro *macros;                // 宏定义表
    size_t macro_count;             // 宏数量
    size_t macro_capacity;          // 宏表容量
    
    CnConditionFrame *condition_stack;  // 条件编译栈
    size_t condition_depth;         // 条件栈深度
    size_t condition_capacity;      // 条件栈容量
    
    size_t current_offset;          // 当前扫描位置
    int current_line;               // 当前行号
    int current_column;             // 当前列号
    
    struct CnDiagnostics *diagnostics; // 诊断信息
} CnPreprocessor;

/*
 * 初始化预处理器
 * 
 * @param preprocessor 预处理器上下文
 * @param source 输入源代码
 * @param source_length 源代码长度
 * @param filename 文件名 (可为 NULL)
 */
void cn_frontend_preprocessor_init(
    CnPreprocessor *preprocessor,
    const char *source,
    size_t source_length,
    const char *filename
);

/*
 * 设置诊断接收器
 */
void cn_frontend_preprocessor_set_diagnostics(
    CnPreprocessor *preprocessor,
    struct CnDiagnostics *diagnostics
);

/*
 * 执行预处理
 * 
 * @param preprocessor 预处理器上下文
 * @return true 如果预处理成功，false 如果遇到错误
 * 
 * 成功后，预处理结果存储在 preprocessor->output 中，
 * 调用者负责在使用完成后调用 cn_frontend_preprocessor_free
 */
bool cn_frontend_preprocessor_process(CnPreprocessor *preprocessor);

/*
 * 释放预处理器资源
 * 
 * 注意: 调用此函数后，preprocessor->output 将被释放
 */
void cn_frontend_preprocessor_free(CnPreprocessor *preprocessor);

/*
 * 预定义宏
 * 
 * @param preprocessor 预处理器上下文
 * @param name 宏名称
 * @param value 宏值
 * @return true 如果成功，false 如果失败 (如内存不足)
 */
bool cn_frontend_preprocessor_define_macro(
    CnPreprocessor *preprocessor,
    const char *name,
    const char *value
);

/*
 * 取消定义宏
 * 
 * @param preprocessor 预处理器上下文
 * @param name 宏名称
 * @return true 如果宏存在并被删除，false 如果宏不存在
 */
bool cn_frontend_preprocessor_undefine_macro(
    CnPreprocessor *preprocessor,
    const char *name
);

/*
 * 检查宏是否已定义
 * 
 * @param preprocessor 预处理器上下文
 * @param name 宏名称
 * @param name_length 宏名称长度
 * @return true 如果宏已定义，false 否则
 */
bool cn_frontend_preprocessor_is_defined(
    const CnPreprocessor *preprocessor,
    const char *name,
    size_t name_length
);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_PREPROCESSOR_H */
