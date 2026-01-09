/**
 * @file CN_compiler_impl.c
 * @brief CN_Language 编译器实现
 * 
 * 实现编译器的具体功能，协调词法分析、语法分析、语义分析和代码生成。
 * 遵循项目架构规范，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_compiler_interface.h"
#include "lexer/CN_lexer_interface.h"
#include "parser/CN_parser_interface.h"
#include "semantic/CN_semantic_interface.h"
#include "codegen/CN_codegen_interface.h"
#include "ast/CN_ast.h"
#include "../infrastructure/containers/array/CN_dynamic_array.h"
#include "../infrastructure/utils/CN_utils_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 内部数据结构 ==================== */

/**
 * @brief 编译器内部状态结构体
 */
typedef struct {
    Stru_LexerInterface_t* lexer;          /**< 词法分析器实例 */
    Stru_ParserInterface_t* parser;        /**< 语法分析器实例 */
    Stru_SemanticAnalyzerInterface_t* semantic;    /**< 语义分析器实例 */
    Stru_CodeGeneratorInterface_t* codegen; /**< 代码生成器实例 */
    
    Stru_DynamicArray_t* tokens;           /**< 词法分析结果：token数组 */
    Stru_AstNode_t* ast_root;              /**< 语法分析结果：AST根节点 */
    
    Stru_DynamicArray_t* errors;           /**< 错误信息数组 */
    Stru_DynamicArray_t* warnings;         /**< 警告信息数组 */
    
    bool initialized;                      /**< 是否已初始化 */
    bool lexer_done;                       /**< 词法分析是否完成 */
    bool parser_done;                      /**< 语法分析是否完成 */
    bool semantic_done;                    /**< 语义分析是否完成 */
    
    char* current_source;                  /**< 当前源代码 */
    char* current_source_name;             /**< 当前源文件名 */
} Stru_CompilerState_t;

/**
 * @brief 编译器实现结构体
 */
typedef struct {
    Stru_CompilerInterface_t interface;    /**< 公共接口 */
    Stru_CompilerState_t* state;           /**< 内部状态 */
} Stru_CompilerImpl_t;

/* ==================== 内部函数声明 ==================== */

/* 接口方法实现 */
static bool F_initialize_impl(Stru_CompilerInterface_t* self);
static Stru_CompilationResult_t* F_compile_file_impl(Stru_CompilerInterface_t* self, 
                                                    const Stru_CompileOptions_t* options);
static Stru_CompilationResult_t* F_compile_source_impl(Stru_CompilerInterface_t* self,
                                                      const char* source,
                                                      const char* source_name,
                                                      const Stru_CompileOptions_t* options);
static bool F_lexical_analysis_impl(Stru_CompilerInterface_t* self,
                                   const char* source,
                                   const char* source_name);
static bool F_syntax_analysis_impl(Stru_CompilerInterface_t* self);
static bool F_semantic_analysis_impl(Stru_CompilerInterface_t* self);
static bool F_code_generation_impl(Stru_CompilerInterface_t* self,
                                  const char* output_file);
static int F_get_error_count_impl(Stru_CompilerInterface_t* self);
static int F_get_warning_count_impl(Stru_CompilerInterface_t* self);
static const char* F_get_error_message_impl(Stru_CompilerInterface_t* self, int index);
static const char* F_get_warning_message_impl(Stru_CompilerInterface_t* self, int index);
static void F_reset_impl(Stru_CompilerInterface_t* self);
static void F_destroy_impl(Stru_CompilerInterface_t* self);

/* 辅助函数 */
static Stru_CompilerState_t* F_create_compiler_state(void);
static void F_destroy_compiler_state(Stru_CompilerState_t* state);
static bool F_read_source_file(const char* filename, char** buffer, size_t* size);
static Stru_CompilationResult_t* F_create_compilation_result(void);
static void F_add_error(Stru_CompilerState_t* state, const char* message);
static void F_add_warning(Stru_CompilerState_t* state, const char* message);
static void F_clear_errors_and_warnings(Stru_CompilerState_t* state);

/* ==================== 工厂函数实现 ==================== */

/**
 * @brief 创建编译器接口实例
 */
Stru_CompilerInterface_t* F_create_compiler_interface(void)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)malloc(sizeof(Stru_CompilerImpl_t));
    if (impl == NULL) {
        return NULL;
    }
    
    /* 初始化接口方法 */
    impl->interface.initialize = F_initialize_impl;
    impl->interface.compile_file = F_compile_file_impl;
    impl->interface.compile_source = F_compile_source_impl;
    impl->interface.lexical_analysis = F_lexical_analysis_impl;
    impl->interface.syntax_analysis = F_syntax_analysis_impl;
    impl->interface.semantic_analysis = F_semantic_analysis_impl;
    impl->interface.code_generation = F_code_generation_impl;
    impl->interface.get_error_count = F_get_error_count_impl;
    impl->interface.get_warning_count = F_get_warning_count_impl;
    impl->interface.get_error_message = F_get_error_message_impl;
    impl->interface.get_warning_message = F_get_warning_message_impl;
    impl->interface.reset = F_reset_impl;
    impl->interface.destroy = F_destroy_impl;
    impl->interface.internal_state = NULL;
    
    /* 创建内部状态 */
    impl->state = F_create_compiler_state();
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

/* ==================== 辅助函数实现 ==================== */

/**
 * @brief 创建编译器内部状态
 */
static Stru_CompilerState_t* F_create_compiler_state(void)
{
    Stru_CompilerState_t* state = (Stru_CompilerState_t*)malloc(sizeof(Stru_CompilerState_t));
    if (state == NULL) {
        return NULL;
    }
    
    /* 初始化所有字段 */
    state->lexer = NULL;
    state->parser = NULL;
    state->semantic = NULL;
    state->codegen = NULL;
    state->tokens = NULL;
    state->ast_root = NULL;
    state->errors = NULL;
    state->warnings = NULL;
    state->initialized = false;
    state->lexer_done = false;
    state->parser_done = false;
    state->semantic_done = false;
    state->current_source = NULL;
    state->current_source_name = NULL;
    
    return state;
}

/**
 * @brief 销毁编译器内部状态
 */
static void F_destroy_compiler_state(Stru_CompilerState_t* state)
{
    if (state == NULL) {
        return;
    }
    
    /* 销毁各个模块实例 */
    if (state->lexer != NULL) {
        state->lexer->destroy(state->lexer);
    }
    
    /* 销毁动态数组 */
    if (state->tokens != NULL) {
        F_destroy_dynamic_array(state->tokens);
    }
    
    if (state->errors != NULL) {
        F_destroy_dynamic_array(state->errors);
    }
    
    if (state->warnings != NULL) {
        F_destroy_dynamic_array(state->warnings);
    }
    
    /* 释放字符串 */
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
static bool F_read_source_file(const char* filename, char** buffer, size_t* size)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return false;
    }
    
    /* 获取文件大小 */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return false;
    }
    
    /* 分配缓冲区 */
    *buffer = (char*)malloc(file_size + 1);
    if (*buffer == NULL) {
        fclose(file);
        return false;
    }
    
    /* 读取文件内容 */
    size_t bytes_read = fread(*buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(*buffer);
        *buffer = NULL;
        return false;
    }
    
    /* 添加终止符 */
    (*buffer)[file_size] = '\0';
    
    if (size != NULL) {
        *size = (size_t)file_size;
    }
    
    return true;
}

/**
 * @brief 创建编译结果
 */
static Stru_CompilationResult_t* F_create_compilation_result(void)
{
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
    
    return result;
}

/* ==================== 其他接口方法实现 ==================== */

/**
 * @brief 执行词法分析
 */
static bool F_lexical_analysis_impl(Stru_CompilerInterface_t* self,
                                   const char* source,
                                   const char* source_name)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    /* 保存源代码 */
    if (state->current_source != NULL) {
        free(state->current_source);
    }
    state->current_source = strdup(source);
    
    if (state->current_source_name != NULL) {
        free(state->current_source_name);
    }
    state->current_source_name = strdup(source_name);
    
    /* 初始化词法分析器 */
    if (!state->lexer->initialize(state->lexer, source, strlen(source), source_name)) {
        F_add_error(state, "词法分析器初始化失败");
        return false;
    }
    
    /* 执行词法分析 */
    state->tokens = F_create_dynamic_array(sizeof(Stru_Token_t*));
    if (state->tokens == NULL) {
        F_add_error(state, "无法创建token数组");
        return false;
    }
    
    /* 获取所有token */
    while (state->lexer->has_more_tokens(state->lexer)) {
        Stru_Token_t* token = state->lexer->next_token(state->lexer);
        if (token == NULL) {
            break;
        }
        
        F_dynamic_array_push(state->tokens, &token);
        
        /* 检查是否到达文件结束 */
        if (token->type == Eum_TOKEN_EOF) {
            break;
        }
    }
    
    state->lexer_done = true;
    return true;
}

/**
 * @brief 执行语法分析
 */
static bool F_syntax_analysis_impl(Stru_CompilerInterface_t* self)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (!state->lexer_done) {
        F_add_error(state, "需要先执行词法分析");
        return false;
    }
    
    /* TODO: 实现语法分析 */
    /* 暂时返回成功，实际功能将在后续实现 */
    
    state->parser_done = true;
    return true;
}

/**
 * @brief 执行语义分析
 */
static bool F_semantic_analysis_impl(Stru_CompilerInterface_t* self)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (!state->parser_done) {
        F_add_error(state, "需要先执行语法分析");
        return false;
    }
    
    /* TODO: 实现语义分析 */
    /* 暂时返回成功，实际功能将在后续实现 */
    
    state->semantic_done = true;
    return true;
}

/**
 * @brief 执行代码生成
 */
static bool F_code_generation_impl(Stru_CompilerInterface_t* self,
                                  const char* output_file)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (!state->semantic_done) {
        F_add_error(state, "需要先执行语义分析");
        return false;
    }
    
    /* TODO: 实现代码生成 */
    /* 暂时返回成功，实际功能将在后续实现 */
    
    return true;
}

/**
 * @brief 获取错误数量
 */
static int F_get_error_count_impl(Stru_CompilerInterface_t* self)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (state->errors == NULL) {
        return 0;
    }
    
    return (int)state->errors->length;
}

/**
 * @brief 获取警告数量
 */
static int F_get_warning_count_impl(Stru_CompilerInterface_t* self)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (state->warnings == NULL) {
        return 0;
    }
    
    return (int)state->warnings->length;
}

/**
 * @brief 获取错误信息
 */
static const char* F_get_error_message_impl(Stru_CompilerInterface_t* self, int index)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (state->errors == NULL || index < 0 || index >= (int)state->errors->length) {
        return NULL;
    }
    
    char** error_ptr = (char**)F_dynamic_array_get(state->errors, index);
    if (error_ptr == NULL) {
        return NULL;
    }
    
    return *error_ptr;
}

/**
 * @brief 获取警告信息
 */
static const char* F_get_warning_message_impl(Stru_CompilerInterface_t* self, int index)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (state->warnings == NULL || index < 0 || index >= (int)state->warnings->length) {
        return NULL;
    }
    
    char** warning_ptr = (char**)F_dynamic_array_get(state->warnings, index);
    if (warning_ptr == NULL) {
        return NULL;
    }
    
    return *warning_ptr;
}

/**
 * @brief 重置编译器状态
 */
static void F_reset_impl(Stru_CompilerInterface_t* self)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    /* 清除错误和警告 */
    F_clear_errors_and_warnings(state);
    
    /* 重置状态标志 */
    state->lexer_done = false;
    state->parser_done = false;
    state->semantic_done = false;
    
    /* 释放token数组 */
    if (state->tokens != NULL) {
        F_destroy_dynamic_array(state->tokens);
        state->tokens = NULL;
    }
    
    /* 释放AST */
    if (state->ast_root != NULL) {
        /* TODO: 释放AST节点 */
        state->ast_root = NULL;
    }
    
    /* 释放源代码 */
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
 * @brief 销毁编译器
 */
static void F_destroy_impl(Stru_CompilerInterface_t* self)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    
    if (impl == NULL) {
        return;
    }
    
    /* 销毁内部状态 */
    if (impl->state != NULL) {
        F_destroy_compiler_state(impl->state);
        impl->state = NULL;
    }
    
    /* 销毁实现结构体 */
    free(impl);
}

#ifdef __cplusplus
}
#endif

/**
 * @brief 添加错误信息
 */
static void F_add_error(Stru_CompilerState_t* state, const char* message)
{
    if (state == NULL || message == NULL) {
        return;
    }
    
    /* 创建错误数组（如果需要） */
    if (state->errors == NULL) {
        state->errors = F_create_dynamic_array(sizeof(char*));
        if (state->errors == NULL) {
            return;
        }
    }
    
    /* 复制错误信息 */
    char* error_copy = strdup(message);
    if (error_copy != NULL) {
        F_dynamic_array_push(state->errors, &error_copy);
    }
}

/**
 * @brief 添加警告信息
 */
static void F_add_warning(Stru_CompilerState_t* state, const char* message)
{
    if (state == NULL || message == NULL) {
        return;
    }
    
    /* 创建警告数组（如果需要） */
    if (state->warnings == NULL) {
        state->warnings = F_create_dynamic_array(sizeof(char*));
        if (state->warnings == NULL) {
            return;
        }
    }
    
    /* 复制警告信息 */
    char* warning_copy = strdup(message);
    if (warning_copy != NULL) {
        F_dynamic_array_push(state->warnings, &warning_copy);
    }
}

/**
 * @brief 清除错误和警告
 */
static void F_clear_errors_and_warnings(Stru_CompilerState_t* state)
{
    if (state == NULL) {
        return;
    }
    
    /* 清除错误数组 */
    if (state->errors != NULL) {
        /* 释放每个错误字符串 */
        for (size_t i = 0; i < state->errors->length; i++) {
            char** error_ptr = (char**)F_dynamic_array_get(state->errors, i);
            if (error_ptr != NULL && *error_ptr != NULL) {
                free(*error_ptr);
            }
        }
        F_destroy_dynamic_array(state->errors);
        state->errors = NULL;
    }
    
    /* 清除警告数组 */
    if (state->warnings != NULL) {
        /* 释放每个警告字符串 */
        for (size_t i = 0; i < state->warnings->length; i++) {
            char** warning_ptr = (char**)F_dynamic_array_get(state->warnings, i);
            if (warning_ptr != NULL && *warning_ptr != NULL) {
                free(*warning_ptr);
            }
        }
        F_destroy_dynamic_array(state->warnings);
        state->warnings = NULL;
    }
}

/* ==================== 接口方法实现 ==================== */

/**
 * @brief 初始化编译器
 */
static bool F_initialize_impl(Stru_CompilerInterface_t* self)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    if (state->initialized) {
        return true; /* 已经初始化 */
    }
    
    /* 创建各个模块实例 */
    state->lexer = F_create_lexer_interface();
    if (state->lexer == NULL) {
        F_add_error(state, "无法创建词法分析器");
        return false;
    }
    
    /* TODO: 创建其他模块实例（语法分析器、语义分析器、代码生成器） */
    /* 暂时返回成功，实际功能将在后续实现 */
    
    state->initialized = true;
    return true;
}

/**
 * @brief 编译源代码文件
 */
static Stru_CompilationResult_t* F_compile_file_impl(Stru_CompilerInterface_t* self, 
                                                    const Stru_CompileOptions_t* options)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    /* 记录开始时间 */
    clock_t start_time = clock();
    
    /* 创建编译结果 */
    Stru_CompilationResult_t* result = F_create_compilation_result();
    if (result == NULL) {
        return NULL;
    }
    
    /* 检查输入文件 */
    if (options == NULL || options->input_file == NULL) {
        result->success = false;
        result->error_message = strdup("未指定输入文件");
        return result;
    }
    
    /* 读取源代码文件 */
    char* source = NULL;
    size_t source_size = 0;
    if (!F_read_source_file(options->input_file, &source, &source_size)) {
        result->success = false;
        result->error_message = strdup("无法读取源文件");
        return result;
    }
    
    /* 执行编译 */
    Stru_CompileOptions_t source_options = *options;
    source_options.input_file = options->input_file;
    
    Stru_CompilationResult_t* source_result = F_compile_source_impl(self, source, options->input_file, &source_options);
    
    /* 计算编译时间 */
    clock_t end_time = clock();
    source_result->compile_time_ms = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;
    
    /* 设置输出路径 */
    if (source_result->success && options->output_file != NULL) {
        source_result->output_path = strdup(options->output_file);
    }
    
    /* 清理源代码缓冲区 */
    free(source);
    
    return source_result;
}

/**
 * @brief 编译源代码字符串
 */
static Stru_CompilationResult_t* F_compile_source_impl(Stru_CompilerInterface_t* self,
                                                      const char* source,
                                                      const char* source_name,
                                                      const Stru_CompileOptions_t* options)
{
    Stru_CompilerImpl_t* impl = (Stru_CompilerImpl_t*)self;
    Stru_CompilerState_t* state = impl->state;
    
    /* 创建编译结果 */
    Stru_CompilationResult_t* result = F_create_compilation_result();
    if (result == NULL) {
        return NULL;
    }
    
    /* 重置编译器状态 */
    F_reset_impl(self);
    
    /* 初始化编译器（如果需要） */
    if (!state->initialized && !F_initialize_impl(self)) {
        result->success = false;
        result->error_message = strdup("编译器初始化失败");
        return result;
    }
    
    /* 执行词法分析 */
    if (!F_lexical_analysis_impl(self, source, source_name)) {
        result->success = false;
        result->error_count = F_get_error_count_impl(self);
        result->warning_count = F_get_warning_count_impl(self);
        result->error_message = strdup("词法分析失败");
        return result;
    }
    
    /* 执行语法分析 */
    if (!F_syntax_analysis_impl(self)) {
        result->success = false;
        result->error_count = F_get_error_count_impl(self);
        result->warning_count = F_get_warning_count_impl(self);
        result->error_message = strdup("语法分析失败");
        return result;
    }
    
    /* 执行语义分析 */
    if (!F_semantic_analysis_impl(self)) {
        result->success = false;
        result->error_count = F_get_error_count_impl(self);
        result->warning_count = F_get_warning_count_impl(self);
        result->error_message = strdup("语义分析失败");
        return result;
    }
    
    /* 执行代码生成 */
    const char* output_file = (options != NULL && options->output_file != NULL) ? 
                              options->output_file : "a.out";
    
    if (!F_code_generation_impl(self, output_file)) {
        result->success = false;
        result->error_count = F_get_error_count_impl(self);
        result->warning_count = F_get_warning_count_impl(self);
        result->error_message = strdup("代码生成失败");
        return result;
    }
    
    /* 编译成功 */
    result->success = true;
    result->error_count = F_get_error_count_impl(self);
    result->warning_count = F_get_warning_count_impl(self);
    result->output_path = strdup(output_file);
    
    return result;
}
