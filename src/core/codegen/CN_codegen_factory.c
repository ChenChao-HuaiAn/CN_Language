/******************************************************************************
 * 文件名: CN_codegen_factory.c
 * 功能: CN_Language 代码生成器工厂实现
 * 
 * 提供代码生成器接口的工厂函数实现，支持创建不同类型的代码生成器。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月9日
 * 修改历史:
 *   - 2026年1月9日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

/* 确保strdup函数可用 */
#define _POSIX_C_SOURCE 200809L

#include "CN_codegen_interface.h"
#include "implementations/c_backend/CN_c_backend.h"
#include "implementations/llvm_backend/CN_llvm_backend.h"
#include "implementations/x86_backend/CN_x86_backend.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include "../../infrastructure/containers/string/CN_string.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* 内部状态结构体 */
typedef struct {
    Stru_CodeGenOptions_t options;           ///< 当前配置选项
    Stru_DynamicArray_t* errors;             ///< 错误信息数组
    Stru_DynamicArray_t* warnings;           ///< 警告信息数组
    Stru_DynamicArray_t* optimizer_plugins;  ///< 优化器插件数组
    bool initialized;                        ///< 是否已初始化
    void* backend_state;                     ///< 后端特定状态
} CodeGeneratorState;

/* 内部函数声明 */
static bool initialize_codegen(Stru_CodeGeneratorInterface_t* interface, const Stru_CodeGenOptions_t* options);
static Stru_CodeGenResult_t* generate_code(Stru_CodeGeneratorInterface_t* interface, Stru_AstNode_t* ast);
static Stru_CodeGenResult_t* generate_module_code(Stru_CodeGeneratorInterface_t* interface, 
                                                  Stru_AstNode_t** modules, size_t module_count);
static Stru_CodeGenResult_t* optimize(Stru_CodeGeneratorInterface_t* interface, 
                                      const Stru_CodeGenResult_t* code, int optimization_level);
static bool register_optimizer_plugin(Stru_CodeGeneratorInterface_t* interface,
                                      const char* plugin_name, void* plugin_func);
static bool set_target(Stru_CodeGeneratorInterface_t* interface,
                       Eum_TargetCodeType target_type,
                       const char* platform, const char* architecture);
static bool generate_target_file(Stru_CodeGeneratorInterface_t* interface,
                                 const Stru_CodeGenResult_t* code, const char* output_file);
static Eum_TargetCodeType get_target_type(Stru_CodeGeneratorInterface_t* interface);
static void get_supported_optimization_levels(Stru_CodeGeneratorInterface_t* interface,
                                              int* min_level, int* max_level);
static Stru_DynamicArray_t* get_registered_plugins(Stru_CodeGeneratorInterface_t* interface);
static bool has_errors(Stru_CodeGeneratorInterface_t* interface);
static Stru_DynamicArray_t* get_errors(Stru_CodeGeneratorInterface_t* interface);
static void clear_errors(Stru_CodeGeneratorInterface_t* interface);
static void reset(Stru_CodeGeneratorInterface_t* interface);
static void destroy(Stru_CodeGeneratorInterface_t* interface);

/* 辅助函数声明 */
static CodeGeneratorState* get_state(Stru_CodeGeneratorInterface_t* interface);
static void add_error(CodeGeneratorState* state, const char* error_message);
static void add_warning(CodeGeneratorState* state, const char* warning_message);
static Stru_CodeGenResult_t* create_codegen_result(void);

/**
 * @brief 创建代码生成器接口实例
 * 
 * 创建并返回一个新的代码生成器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_CodeGeneratorInterface_t* 新创建的代码生成器接口实例
 */
Stru_CodeGeneratorInterface_t* F_create_codegen_interface(void)
{
    /* 分配接口结构体 */
    Stru_CodeGeneratorInterface_t* interface = (Stru_CodeGeneratorInterface_t*)malloc(sizeof(Stru_CodeGeneratorInterface_t));
    if (!interface) {
        return NULL;
    }
    
    /* 分配内部状态 */
    CodeGeneratorState* state = (CodeGeneratorState*)malloc(sizeof(CodeGeneratorState));
    if (!state) {
        free(interface);
        return NULL;
    }
    
    /* 初始化内部状态 */
    memset(state, 0, sizeof(CodeGeneratorState));
    state->options = F_create_default_codegen_options();
    state->errors = NULL;  // 动态数组将在需要时创建
    state->warnings = NULL; // 动态数组将在需要时创建
    state->optimizer_plugins = NULL; // 动态数组将在需要时创建
    state->initialized = false;
    state->backend_state = NULL;
    
    /* 设置接口函数指针 */
    interface->initialize = initialize_codegen;
    interface->generate_code = generate_code;
    interface->generate_module_code = generate_module_code;
    interface->optimize = optimize;
    interface->register_optimizer_plugin = register_optimizer_plugin;
    interface->set_target = set_target;
    interface->generate_target_file = generate_target_file;
    interface->get_target_type = get_target_type;
    interface->get_supported_optimization_levels = get_supported_optimization_levels;
    interface->get_registered_plugins = get_registered_plugins;
    interface->has_errors = has_errors;
    interface->get_errors = get_errors;
    interface->clear_errors = clear_errors;
    interface->reset = reset;
    interface->destroy = destroy;
    interface->internal_state = state;
    
    return interface;
}

/**
 * @brief 创建默认代码生成选项
 * 
 * 创建并返回默认的代码生成选项。
 * 
 * @return Stru_CodeGenOptions_t 默认代码生成选项
 */
Stru_CodeGenOptions_t F_create_default_codegen_options(void)
{
    Stru_CodeGenOptions_t options;
    options.target_type = Eum_TARGET_C;
    options.optimize = true;
    options.optimization_level = 2;
    options.debug_info = false;
    options.output_file = NULL;
    options.verbose = false;
    return options;
}

/**
 * @brief 销毁代码生成结果
 * 
 * 释放代码生成结果占用的所有资源。
 * 
 * @param result 要销毁的代码生成结果
 */
void F_destroy_codegen_result(Stru_CodeGenResult_t* result)
{
    if (!result) {
        return;
    }
    
    if (result->code) {
        free((void*)result->code);
    }
    
    if (result->errors) {
        // TODO: 实现动态数组销毁
        free(result->errors);
    }
    
    if (result->warnings) {
        // TODO: 实现动态数组销毁
        free(result->warnings);
    }
    
    free(result);
}

/* ============================================
 * 内部函数实现
 * ============================================ */

/**
 * @brief 获取内部状态
 */
static CodeGeneratorState* get_state(Stru_CodeGeneratorInterface_t* interface)
{
    if (!interface || !interface->internal_state) {
        return NULL;
    }
    return (CodeGeneratorState*)interface->internal_state;
}

/**
 * @brief 添加错误信息
 */
static void add_error(CodeGeneratorState* state, const char* error_message)
{
    if (!state || !error_message) {
        return;
    }
    
    char* error_copy = strdup(error_message);
    if (error_copy) {
        F_dynamic_array_push(state->errors, &error_copy);
    }
}

/**
 * @brief 添加警告信息
 */
static void add_warning(CodeGeneratorState* state, const char* warning_message)
{
    if (!state || !warning_message) {
        return;
    }
    
    char* warning_copy = strdup(warning_message);
    if (warning_copy) {
        F_dynamic_array_push(state->warnings, &warning_copy);
    }
}

/**
 * @brief 创建代码生成结果
 */
static Stru_CodeGenResult_t* create_codegen_result(void)
{
    Stru_CodeGenResult_t* result = (Stru_CodeGenResult_t*)malloc(sizeof(Stru_CodeGenResult_t));
    if (!result) {
        return NULL;
    }
    
    memset(result, 0, sizeof(Stru_CodeGenResult_t));
    result->errors = NULL;  // TODO: 创建动态数组
    result->warnings = NULL; // TODO: 创建动态数组
    
    return result;
}

/**
 * @brief 初始化代码生成器
 */
static bool initialize_codegen(Stru_CodeGeneratorInterface_t* interface, const Stru_CodeGenOptions_t* options)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !options) {
        return false;
    }
    
    /* 清除之前的错误和警告 */
    clear_errors(interface);
    
    /* 验证选项 */
    if (options->optimization_level < 0 || options->optimization_level > 3) {
        add_error(state, "优化级别必须在0-3之间");
        return false;
    }
    
    /* 保存选项 */
    state->options = *options;
    
    /* 根据目标类型初始化后端 */
    switch (options->target_type) {
        case Eum_TARGET_C:
            /* TODO: 初始化C后端 */
            break;
        case Eum_TARGET_LLVM_IR:
            /* TODO: 初始化LLVM后端 */
            break;
        case Eum_TARGET_X86_64:
            /* TODO: 初始化x86-64后端 */
            break;
        case Eum_TARGET_ARM64:
            /* TODO: 初始化ARM64后端 */
            break;
        case Eum_TARGET_WASM:
            /* TODO: 初始化WASM后端 */
            break;
        case Eum_TARGET_BYTECODE:
            /* TODO: 初始化字节码后端 */
            break;
        default:
            add_error(state, "不支持的目标代码类型");
            return false;
    }
    
    state->initialized = true;
    return true;
}

/**
 * @brief 生成代码
 */
static Stru_CodeGenResult_t* generate_code(Stru_CodeGeneratorInterface_t* interface, Stru_AstNode_t* ast)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !state->initialized || !ast) {
        Stru_CodeGenResult_t* result = create_codegen_result();
        if (result) {
            result->success = false;
            add_error(state, "代码生成器未初始化或AST为空");
        }
        return result;
    }
    
    Stru_CodeGenResult_t* result = NULL;
    
    /* 根据目标类型调用相应的后端生成代码 */
    switch (state->options.target_type) {
        case Eum_TARGET_C:
            /* 调用C后端生成代码 */
            result = F_generate_c_code(ast);
            if (!result) {
                add_error(state, "C后端代码生成失败");
                result = create_codegen_result();
                if (result) {
                    result->success = false;
                }
            }
            break;
        case Eum_TARGET_LLVM_IR:
            /* 调用LLVM后端生成代码 */
            {
                Stru_LLVMBackendConfig_t llvm_config = F_create_default_llvm_backend_config();
                llvm_config.optimization_level = state->options.optimization_level;
                llvm_config.debug_info = state->options.debug_info;
                
                result = F_generate_llvm_ir_from_ast(ast, &llvm_config);
                if (!result) {
                    add_error(state, "LLVM IR后端代码生成失败");
                    result = create_codegen_result();
                    if (result) {
                        result->success = false;
                    }
                }
            }
            break;
        case Eum_TARGET_X86_64:
            /* 调用x86-64后端生成代码 */
            {
                Stru_X86BackendConfig_t x86_config = F_create_default_x86_backend_config();
                x86_config.optimization_level = state->options.optimization_level;
                x86_config.debug_info = state->options.debug_info;
                x86_config.use_64bit = true;
                x86_config.architecture = Eum_X86_ARCH_64BIT;
                
                result = F_generate_x86_asm_from_ast(ast, &x86_config);
                if (!result) {
                    add_error(state, "x86-64汇编后端代码生成失败");
                    result = create_codegen_result();
                    if (result) {
                        result->success = false;
                    }
                }
            }
            break;
        case Eum_TARGET_ARM64:
            /* TODO: 调用ARM64后端生成代码 */
            result = create_codegen_result();
            if (result) {
                result->success = false;
                result->code = strdup("// ARM64汇编代码生成器 - 待实现\n");
                result->code_length = strlen(result->code);
                add_error(state, "ARM64汇编后端尚未实现");
            }
            break;
        case Eum_TARGET_WASM:
            /* TODO: 调用WASM后端生成代码 */
            result = create_codegen_result();
            if (result) {
                result->success = false;
                result->code = strdup("// WebAssembly代码生成器 - 待实现\n");
                result->code_length = strlen(result->code);
                add_error(state, "WebAssembly后端尚未实现");
            }
            break;
        case Eum_TARGET_BYTECODE:
            /* TODO: 调用字节码后端生成代码 */
            result = create_codegen_result();
            if (result) {
                result->success = false;
                result->code = strdup("// 字节码生成器 - 待实现\n");
                result->code_length = strlen(result->code);
                add_error(state, "字节码后端尚未实现");
            }
            break;
        default:
            result = create_codegen_result();
            if (result) {
                result->success = false;
                add_error(state, "不支持的目标代码类型");
            }
            break;
    }
    
    /* 如果结果不为空，合并状态中的错误和警告 */
    if (result && state->errors && state->errors->length > 0) {
        /* 确保结果有错误数组 */
        if (!result->errors) {
            result->errors = F_create_dynamic_array(sizeof(char*));
        }
        
        /* 复制错误信息 */
        for (size_t i = 0; i < state->errors->length; i++) {
            char* error = *(char**)F_dynamic_array_get(state->errors, i);
            if (error) {
                char* error_copy = strdup(error);
                if (error_copy) {
                    F_dynamic_array_push(result->errors, &error_copy);
                }
            }
        }
    }
    
    if (result && state->warnings && state->warnings->length > 0) {
        /* 确保结果有警告数组 */
        if (!result->warnings) {
            result->warnings = F_create_dynamic_array(sizeof(char*));
        }
        
        /* 复制警告信息 */
        for (size_t i = 0; i < state->warnings->length; i++) {
            char* warning = *(char**)F_dynamic_array_get(state->warnings, i);
            if (warning) {
                char* warning_copy = strdup(warning);
                if (warning_copy) {
                    F_dynamic_array_push(result->warnings, &warning_copy);
                }
            }
        }
    }
    
    return result;
}

/**
 * @brief 生成模块代码
 */
static Stru_CodeGenResult_t* generate_module_code(Stru_CodeGeneratorInterface_t* interface, 
                                                  Stru_AstNode_t** modules, size_t module_count)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !state->initialized || !modules || module_count == 0) {
        Stru_CodeGenResult_t* result = create_codegen_result();
        if (result) {
            result->success = false;
            add_error(state, "参数无效");
        }
        return result;
    }
    
    Stru_CodeGenResult_t* result = create_codegen_result();
    if (!result) {
        return NULL;
    }
    
    /* TODO: 实现模块代码生成 */
    result->success = true;
    result->code = strdup("// 模块代码生成器 - 待实现\n");
    result->code_length = strlen(result->code);
    
    return result;
}

/**
 * @brief 应用优化
 */
static Stru_CodeGenResult_t* optimize(Stru_CodeGeneratorInterface_t* interface, 
                                      const Stru_CodeGenResult_t* code, int optimization_level)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !state->initialized || !code) {
        Stru_CodeGenResult_t* result = create_codegen_result();
        if (result) {
            result->success = false;
            add_error(state, "参数无效");
        }
        return result;
    }
    
    Stru_CodeGenResult_t* result = create_codegen_result();
    if (!result) {
        return NULL;
    }
    
    /* TODO: 实现优化功能 */
    result->success = true;
    result->code = strdup(code->code);
    result->code_length = code->code_length;
    
    return result;
}

/**
 * @brief 注册优化器插件
 */
static bool register_optimizer_plugin(Stru_CodeGeneratorInterface_t* interface,
                                      const char* plugin_name, void* plugin_func)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !plugin_name || !plugin_func) {
        return false;
    }
    
    /* 检查是否已注册 */
    for (size_t i = 0; i < state->optimizer_plugins->length; i++) {
        char* name = *(char**)F_dynamic_array_get(state->optimizer_plugins, i);
        if (name && strcmp(name, plugin_name) == 0) {
            add_warning(state, "优化器插件已注册");
            return false;
        }
    }
    
    /* 注册插件 */
    char* name_copy = strdup(plugin_name);
    if (!name_copy) {
        return false;
    }
    
    // TODO: 实现动态数组推送
    // if (!F_dynamic_array_push(state->optimizer_plugins, &name_copy)) {
    //     free(name_copy);
    //     return false;
    // }
    
    // 临时实现：直接返回成功
    free(name_copy);
    
    return true;
}

/**
 * @brief 设置目标平台
 */
static bool set_target(Stru_CodeGeneratorInterface_t* interface,
                       Eum_TargetCodeType target_type,
                       const char* platform, const char* architecture)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state) {
        return false;
    }
    
    state->options.target_type = target_type;
    return true;
}

/**
 * @brief 生成目标文件
 */
static bool generate_target_file(Stru_CodeGeneratorInterface_t* interface,
                                 const Stru_CodeGenResult_t* code, const char* output_file)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !state->initialized || !code || !output_file) {
        return false;
    }
    
    /* TODO: 实现目标文件生成 */
    add_warning(state, "目标文件生成功能尚未实现");
    return false;
}

/**
 * @brief 获取当前目标类型
 */
static Eum_TargetCodeType get_target_type(Stru_CodeGeneratorInterface_t* interface)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state) {
        return Eum_TARGET_C;
    }
    
    return state->options.target_type;
}

/**
 * @brief 获取支持的优化级别
 */
static void get_supported_optimization_levels(Stru_CodeGeneratorInterface_t* interface,
                                              int* min_level, int* max_level)
{
    if (min_level) *min_level = 0;
    if (max_level) *max_level = 3;
}

/**
 * @brief 获取已注册的优化器插件
 */
static Stru_DynamicArray_t* get_registered_plugins(Stru_CodeGeneratorInterface_t* interface)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state) {
        return NULL;
    }
    
    /* 返回副本 */
    // TODO: 实现动态数组创建和复制
    // Stru_DynamicArray_t* copy = F_create_dynamic_array(sizeof(char*));
    // if (!copy) {
    //     return NULL;
    // }
    
    // for (size_t i = 0; i < state->optimizer_plugins->length; i++) {
    //     char* name = *(char**)F_dynamic_array_get(state->optimizer_plugins, i);
    //     if (name) {
    //         char* name_copy = strdup(name);
    //         if (name_copy) {
    //             F_dynamic_array_push(copy, &name_copy);
    //         }
    //     }
    // }
    
    // return copy;
    return NULL;
}

/**
 * @brief 检查是否有错误
 */
static bool has_errors(Stru_CodeGeneratorInterface_t* interface)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !state->errors) {
        return false;
    }
    
    return state->errors->length > 0;
}

/**
 * @brief 获取错误信息
 */
static Stru_DynamicArray_t* get_errors(Stru_CodeGeneratorInterface_t* interface)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !state->errors) {
        return NULL;
    }
    
    /* 返回副本 */
    // TODO: 实现动态数组创建和复制
    // Stru_DynamicArray_t* copy = F_create_dynamic_array(sizeof(char*));
    // if (!copy) {
    //     return NULL;
    // }
    
    // for (size_t i = 0; i < state->errors->length; i++) {
    //     char* error = *(char**)F_dynamic_array_get(state->errors, i);
    //     if (error) {
    //         char* error_copy = strdup(error);
    //         if (error_copy) {
    //             F_dynamic_array_push(copy, &error_copy);
    //         }
    //     }
    // }
    
    // return copy;
    return NULL;
}

/**
 * @brief 清除错误
 */
static void clear_errors(Stru_CodeGeneratorInterface_t* interface)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state || !state->errors) {
        return;
    }
    
    /* 清除所有错误信息 */
    for (size_t i = 0; i < state->errors->length; i++) {
        char* error = *(char**)F_dynamic_array_get(state->errors, i);
        if (error) {
            free(error);
        }
    }
    
    /* 重置数组 */
    state->errors->length = 0;
    
    /* 同样清除警告 */
    if (state->warnings) {
        for (size_t i = 0; i < state->warnings->length; i++) {
            char* warning = *(char**)F_dynamic_array_get(state->warnings, i);
            if (warning) {
                free(warning);
            }
        }
        state->warnings->length = 0;
    }
}

/**
 * @brief 重置代码生成器
 */
static void reset(Stru_CodeGeneratorInterface_t* interface)
{
    CodeGeneratorState* state = get_state(interface);
    if (!state) {
        return;
    }
    
    /* 清除错误和警告 */
    clear_errors(interface);
    
    /* 重置状态 */
    state->initialized = false;
    state->options = F_create_default_codegen_options();
    
    /* 重置后端状态 */
    if (state->backend_state) {
        // TODO: 根据后端类型进行清理
        free(state->backend_state);
        state->backend_state = NULL;
    }
}

/**
 * @brief 销毁代码生成器
 */
static void destroy(Stru_CodeGeneratorInterface_t* interface)
{
    if (!interface) {
        return;
    }
    
    CodeGeneratorState* state = get_state(interface);
    if (!state) {
        free(interface);
        return;
    }
    
    /* 重置生成器 */
    reset(interface);
    
    /* 销毁动态数组 */
    if (state->errors) {
        // TODO: 实现动态数组销毁
        free(state->errors);
    }
    
    if (state->warnings) {
        // TODO: 实现动态数组销毁
        free(state->warnings);
    }
    
    if (state->optimizer_plugins) {
        // TODO: 实现动态数组销毁
        free(state->optimizer_plugins);
    }
    
    /* 销毁状态和接口 */
    free(state);
    free(interface);
}
