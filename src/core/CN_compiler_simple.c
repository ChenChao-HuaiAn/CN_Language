/**
 * @file CN_compiler_simple.c
 * @brief CN_Language 编译器简化实现
 * 
 * 实现编译器的基本功能，协调词法分析、语法分析、语义分析和代码生成。
 * 这是一个简化版本，用于快速验证编译器架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_compiler_interface.h"
#include "lexer/CN_lexer_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 简化编译器实现 ==================== */

/**
 * @brief 编译器内部状态结构体（简化版）
 */
typedef struct {
    Stru_LexerInterface_t* lexer;          /**< 词法分析器实例 */
    char* current_source;                  /**< 当前源代码 */
    char* current_source_name;             /**< 当前源文件名 */
    int error_count;                       /**< 错误数量 */
    int warning_count;                     /**< 警告数量 */
} Stru_SimpleCompilerState_t;

/**
 * @brief 编译器实现结构体（简化版）
 */
typedef struct {
    Stru_CompilerInterface_t interface;    /**< 公共接口 */
    Stru_SimpleCompilerState_t* state;     /**< 内部状态 */
} Stru_SimpleCompilerImpl_t;

/* ==================== 辅助函数 ==================== */

/**
 * @brief 创建编译器内部状态
 */
static Stru_SimpleCompilerState_t* F_create_simple_compiler_state(void)
{
    Stru_SimpleCompilerState_t* state = (Stru_SimpleCompilerState_t*)malloc(sizeof(Stru_SimpleCompilerState_t));
    if (state == NULL) {
        return NULL;
    }
    
    state->lexer = NULL;
    state->current_source = NULL;
    state->current_source_name = NULL;
    state->error_count = 0;
    state->warning_count = 0;
    
    return state;
}

/**
 * @brief 销毁编译器内部状态
 */
static void F_destroy_simple_compiler_state(Stru_SimpleCompilerState_t* state)
{
    if (state == NULL) {
        return;
    }
    
    if (state->lexer != NULL) {
        state->lexer->destroy(state->lexer);
    }
    
    if (state->current_source != NULL) {
        free(state->current_source);
    }
    
    if (state->current_source_name != NULL) {
        free(state->current_source_name);
    }
    
    free(state);
}

/**
 * @brief 读取源代码文件
 */
static bool F_read_source_file_simple(const char* filename, char** buffer)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return false;
    }
    
    *buffer = (char*)malloc(file_size + 1);
    if (*buffer == NULL) {
        fclose(file);
        return false;
    }
    
    size_t bytes_read = fread(*buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(*buffer);
        *buffer = NULL;
        return false;
    }
    
    (*buffer)[file_size] = '\0';
    return true;
}

/* ==================== 接口方法实现 ==================== */

/**
 * @brief 初始化编译器（简化版）
 */
static bool F_initialize_simple_impl(Stru_CompilerInterface_t* self)
{
    Stru_SimpleCompilerImpl_t* impl = (Stru_SimpleCompilerImpl_t*)self;
    Stru_SimpleCompilerState_t* state = impl->state;
    
    if (state->lexer != NULL) {
        return true; /* 已经初始化 */
    }
    
    state->lexer = F_create_lexer_interface();
    if (state->lexer == NULL) {
        return false;
    }
    
    return true;
}

/**
 * @brief 编译源代码文件（简化版）
 */
static Stru_CompilationResult_t* F_compile_file_simple_impl(Stru_CompilerInterface_t* self, 
                                                           const Stru_CompileOptions_t* options)
{
    Stru_SimpleCompilerImpl_t* impl = (Stru_SimpleCompilerImpl_t*)self;
    Stru_SimpleCompilerState_t* state = impl->state;
    
    /* 记录开始时间 */
    clock_t start_time = clock();
    
    /* 创建编译结果 */
    Stru_CompilationResult_t* result = (Stru_CompilationResult_t*)malloc(sizeof(Stru_CompilationResult_t));
    if (result == NULL) {
        return NULL;
    }
    
    result->success = false;
    result->error_count = 0;
    result->warning_count = 0;
    result->error_message = NULL;
    result->output_path = NULL;
    result->compile_time_ms = 0;
    
    /* 检查输入文件 */
    if (options == NULL || options->input_file == NULL) {
        result->error_message = strdup("未指定输入文件");
        return result;
    }
    
    /* 读取源代码文件 */
    char* source = NULL;
    if (!F_read_source_file_simple(options->input_file, &source)) {
        result->error_message = strdup("无法读取源文件");
        return result;
    }
    
    /* 初始化编译器 */
    if (!F_initialize_simple_impl(self)) {
        free(source);
        result->error_message = strdup("编译器初始化失败");
        return result;
    }
    
    /* 执行词法分析 */
    if (!state->lexer->initialize(state->lexer, source, strlen(source), options->input_file)) {
        free(source);
        result->error_message = strdup("词法分析器初始化失败");
        return result;
    }
    
    /* 执行词法分析 */
    Stru_DynamicArray_t* tokens = state->lexer->tokenize_all(state->lexer);
    if (tokens == NULL) {
        free(source);
        result->error_message = strdup("词法分析失败");
        return result;
    }
    
    /* 检查词法分析错误 */
    if (state->lexer->has_errors(state->lexer)) {
        const char* error = state->lexer->get_last_error(state->lexer);
        free(source);
        result->error_message = strdup(error);
        return result;
    }
    
    /* 输出成功信息 */
    printf("编译成功！\n");
    printf("输入文件: %s\n", options->input_file);
    printf("词法分析完成，生成 %zu 个token\n", tokens->length);
    
    if (options->verbose) {
        printf("详细输出:\n");
        for (size_t i = 0; i < tokens->length && i < 10; i++) {
            Stru_Token_t* token = (Stru_Token_t*)F_dynamic_array_get(tokens, i);
            if (token != NULL) {
                printf("  Token[%zu]: ", i);
                /* TODO: 打印token详细信息 */
            }
        }
        if (tokens->length > 10) {
            printf("  ... 还有 %zu 个token\n", tokens->length - 10);
        }
    }
    
    /* 设置输出路径 */
    const char* output_file = (options != NULL && options->output_file != NULL) ? 
                              options->output_file : "a.out";
    result->output_path = strdup(output_file);
    
    /* 计算编译时间 */
    clock_t end_time = clock();
    result->compile_time_ms = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;
    
    /* 设置成功标志 */
    result->success = true;
    
    /* 清理资源 */
    free(source);
    /* TODO: 释放tokens数组和其中的token */
    
    return result;
}

/**
 * @brief 编译源代码字符串（简化版）
 */
static Stru_CompilationResult_t* F_compile_source_simple_impl(Stru_CompilerInterface_t* self,
                                                             const char* source,
                                                             const char* source_name,
                                                             const Stru_CompileOptions_t* options)
{
    /* 简化实现：将源代码写入临时文件，然后调用文件编译 */
    const char* temp_filename = "temp.cn";
    FILE* temp_file = fopen(temp_filename, "w");
    if (temp_file == NULL) {
        Stru_CompilationResult_t* result = (Stru_CompilationResult_t*)malloc(sizeof(Stru_CompilationResult_t));
        if (result != NULL) {
            result->success = false;
            result->error_message = strdup("无法创建临时文件");
        }
        return result;
    }
    
    fwrite(source, 1, strlen(source), temp_file);
    fclose(temp_file);
    
    /* 创建编译选项 */
    Stru_CompileOptions_t temp_options;
    if (options != NULL) {
        temp_options = *options;
    } else {
        temp_options = F_create_default_compile_options();
    }
    temp_options.input_file = temp_filename;
    
    /* 调用文件编译 */
    Stru_CompilationResult_t* result = F_compile_file_simple_impl(self, &temp_options);
    
    /* 删除临时文件 */
    remove(temp_filename);
    
    return result;
}

/**
 * @brief 获取错误数量（简化版）
 */
static int F_get_error_count_simple_impl(Stru_CompilerInterface_t* self)
{
    Stru_SimpleCompilerImpl_t* impl = (Stru_SimpleCompilerImpl_t*)self;
    return impl->state->error_count;
}

/**
 * @brief 获取警告数量（简化版）
 */
static int F_get_warning_count_simple_impl(Stru_CompilerInterface_t* self)
{
    Stru_SimpleCompilerImpl_t* impl = (Stru_SimpleCompilerImpl_t*)self;
    return impl->state->warning_count;
}

/**
 * @brief 重置编译器状态（简化版）
 */
static void F_reset_simple_impl(Stru_CompilerInterface_t* self)
{
    Stru_SimpleCompilerImpl_t* impl = (Stru_SimpleCompilerImpl_t*)self;
    Stru_SimpleCompilerState_t* state = impl->state;
    
    if (state->lexer != NULL) {
        state->lexer->reset(state->lexer);
    }
    
    state->error_count = 0;
    state->warning_count = 0;
    
    if (state->current_source != NULL) {
        free(state->current_source);
        state->current_source = NULL;
    }
    
    if (state->current_source_name != NULL) {
        free(state->current_source_name);
        state->current_source_name = NULL;
    }
}

/**
 * @brief 销毁编译器（简化版）
 */
static void F_destroy_simple_impl(Stru_CompilerInterface_t* self)
{
    Stru_SimpleCompilerImpl_t* impl = (Stru_SimpleCompilerImpl_t*)self;
    if (impl == NULL) {
        return;
    }
    
    F_destroy_simple_compiler_state(impl->state);
    free(impl);
}

/* ==================== 工厂函数 ==================== */

/**
 * @brief 创建简化编译器接口实例
 */
Stru_CompilerInterface_t* F_create_simple_compiler_interface(void)
{
    Stru_SimpleCompilerImpl_t* impl = (Stru_SimpleCompilerImpl_t*)malloc(sizeof(Stru_SimpleCompilerImpl_t));
    if (impl == NULL) {
        return NULL;
    }
    
    /* 初始化接口方法 */
    impl->interface.initialize = F_initialize_simple_impl;
    impl->interface.compile_file = F_compile_file_simple_impl;
    impl->interface.compile_source = F_compile_source_simple_impl;
    impl->interface.lexical_analysis = NULL; /* 简化版不支持单独阶段 */
    impl->interface.syntax_analysis = NULL;
    impl->interface.semantic_analysis = NULL;
    impl->interface.code_generation = NULL;
    impl->interface.get_error_count = F_get_error_count_simple_impl;
    impl->interface.get_warning_count = F_get_warning_count_simple_impl;
    impl->interface.get_error_message = NULL; /* 简化版不支持获取具体错误信息 */
    impl->interface.get_warning_message = NULL;
    impl->interface.reset = F_reset_simple_impl;
    impl->interface.destroy = F_destroy_simple_impl;
    impl->interface.internal_state = NULL;
    
    /* 创建内部状态 */
    impl->state = F_create_simple_compiler_state();
    if (impl->state == NULL) {
        free(impl);
        return NULL;
    }
    
    return (Stru_CompilerInterface_t*)impl;
}

/**
 * @brief 创建默认编译选项
 */
Stru_CompileOptions_t F_create_default_compile_options(void)
{
    Stru_CompileOptions_t options = {
        .input_file = NULL,
        .output_file = NULL,
        .verbose = false,
        .optimize = false,
        .debug_info = true,
        .target = "native"
    };
    return options;
}

/**
 * @brief 释放编译结果
 */
void F_free_compilation_result(Stru_CompilationResult_t* result)
{
    if (result == NULL) {
        return;
    }
    
    if (result->error_message != NULL) {
        free((void*)result->error_message);
    }
    
    if (result->output_path != NULL) {
        free((void*)result->output_path);
    }
    
    free(result);
}

#ifdef __cplusplus
}
#endif
