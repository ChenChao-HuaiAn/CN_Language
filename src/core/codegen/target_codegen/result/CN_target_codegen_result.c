/**
 * @file CN_target_codegen_result.c
 * @brief CN_Language 目标代码生成器结果管理模块
 * 
 * 提供目标代码生成器结果的管理和销毁功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 */

#include "../CN_target_codegen_interface.h"
#include <stdlib.h>

/**
 * @brief 销毁目标代码生成结果
 * 
 * 释放目标代码生成结果占用的所有资源。
 * 
 * @param result 要销毁的目标代码生成结果
 */
void F_destroy_target_codegen_result(Stru_TargetCodeGenResult_t* result)
{
    if (!result) {
        return;
    }
    
    if (result->assembly_code) {
        free((void*)result->assembly_code);
    }
    
    if (result->object_code) {
        free((void*)result->object_code);
    }
    
    if (result->symbols) {
        // TODO: 实现动态数组销毁
        free(result->symbols);
    }
    
    if (result->relocations) {
        // TODO: 实现动态数组销毁
        free(result->relocations);
    }
    
    if (result->sections) {
        // TODO: 实现动态数组销毁
        free(result->sections);
    }
    
    if (result->debug_info) {
        // TODO: 实现动态数组销毁
        free(result->debug_info);
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
