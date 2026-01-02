/******************************************************************************
 * 文件名: CN_keyword_utils.c
 * 功能: CN_Language关键字工具函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现关键字工具函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_keywords.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 工具函数实现
// ============================================================================

void F_print_keyword_info(const Stru_CN_KeywordInfo_t* keyword_info)
{
    if (keyword_info == NULL)
    {
        printf("关键字信息为空\n");
        return;
    }

    printf("关键字: %s\n", keyword_info->keyword);
    printf("  长度: %zu 字节\n", keyword_info->length);
    
    // 分类名称
    const char* category_name = "未知";
    switch (keyword_info->category)
    {
        case Eum_KEYWORD_CATEGORY_VARIABLE:
            category_name = "变量和类型声明";
            break;
        case Eum_KEYWORD_CATEGORY_CONTROL:
            category_name = "控制结构";
            break;
        case Eum_KEYWORD_CATEGORY_FUNCTION:
            category_name = "函数相关";
            break;
        case Eum_KEYWORD_CATEGORY_LOGICAL:
            category_name = "逻辑运算符";
            break;
        case Eum_KEYWORD_CATEGORY_LITERAL:
            category_name = "字面量";
            break;
        case Eum_KEYWORD_CATEGORY_MODULE:
            category_name = "模块系统";
            break;
        case Eum_KEYWORD_CATEGORY_OPERATOR:
            category_name = "运算符";
            break;
        case Eum_KEYWORD_CATEGORY_TYPE:
            category_name = "类型";
            break;
        case Eum_KEYWORD_CATEGORY_OTHER:
            category_name = "其他";
            break;
    }
    printf("  分类: %s\n", category_name);
    
    if (keyword_info->description != NULL)
    {
        printf("  描述: %s\n", keyword_info->description);
    }
    
    if (keyword_info->english_equivalent != NULL)
    {
        printf("  英文等价: %s\n", keyword_info->english_equivalent);
    }
    
    printf("  保留关键字: %s\n", keyword_info->is_reserved ? "是" : "否");
    
    if (keyword_info->category == Eum_KEYWORD_CATEGORY_OPERATOR)
    {
        printf("  优先级: %d\n", keyword_info->precedence);
    }
    
    printf("\n");
}

void F_print_keyword_table(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL)
    {
        printf("关键字表为空\n");
        return;
    }

    printf("========================================\n");
    printf("CN_Language 关键字表 (共 %zu 个关键字)\n", table->count);
    printf("========================================\n\n");

    for (size_t i = 0; i < table->count; i++)
    {
        printf("%3zu. ", i + 1);
        F_print_keyword_info(&table->keywords[i]);
    }

    printf("========================================\n");
}

void F_print_keyword_table_by_category(Stru_CN_KeywordTable_t* table)
{
    if (table == NULL)
    {
        printf("关键字表为空\n");
        return;
    }

    printf("========================================\n");
    printf("CN_Language 关键字表 (按分类)\n");
    printf("========================================\n\n");

    // 定义分类名称数组
    const char* category_names[] = {
        "变量和类型声明",
        "控制结构", 
        "函数相关",
        "逻辑运算符",
        "字面量",
        "模块系统",
        "运算符",
        "类型",
        "其他"
    };

    // 遍历所有分类
    for (int category = 0; category <= Eum_KEYWORD_CATEGORY_OTHER; category++)
    {
        size_t count = F_get_keyword_count_by_category(table, (Eum_CN_KeywordCategory_t)category);
        if (count == 0)
        {
            continue;
        }

        printf("\n%s 关键字 (共 %zu 个):\n", category_names[category], count);
        printf("----------------------------------------\n");

        // 获取该分类的关键字
        size_t category_count = 0;
        Stru_CN_KeywordInfo_t** keywords = F_get_keywords_by_category(
            table, (Eum_CN_KeywordCategory_t)category, &category_count);
        
        if (keywords != NULL)
        {
            for (size_t i = 0; i < category_count; i++)
            {
                printf("  • %s", keywords[i]->keyword);
                if (keywords[i]->english_equivalent != NULL)
                {
                    printf(" (%s)", keywords[i]->english_equivalent);
                }
                if (keywords[i]->description != NULL)
                {
                    printf(" - %s", keywords[i]->description);
                }
                printf("\n");
            }
            
            // 释放内存
            free(keywords);
        }
    }

    printf("\n========================================\n");
    printf("总计: %zu 个关键字\n", table->count);
    printf("========================================\n");
}

// ============================================================================
// 关键字表导入导出函数实现
// ============================================================================

Stru_CN_KeywordTable_t* F_load_keyword_table_from_file(const char* filename)
{
    // TODO: 实现从文件加载关键字表
    // 当前版本暂不支持，返回NULL
    (void)filename; // 避免未使用参数警告
    return NULL;
}

bool F_save_keyword_table_to_file(Stru_CN_KeywordTable_t* table,
                                  const char* filename)
{
    if (table == NULL || filename == NULL)
    {
        return false;
    }

    // TODO: 实现保存关键字表到文件
    // 当前版本暂不支持，返回false
    (void)table; // 避免未使用参数警告
    (void)filename; // 避免未使用参数警告
    return false;
}

bool F_export_keyword_table_to_c_code(Stru_CN_KeywordTable_t* table,
                                      const char* filename)
{
    if (table == NULL || filename == NULL)
    {
        return false;
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL)
    {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return false;
    }

    // 写入文件头
    fprintf(file, "/******************************************************************************\n");
    fprintf(file, " * 文件名: %s\n", filename);
    fprintf(file, " * 功能: 自动生成的CN_Language关键字表C代码\n");
    fprintf(file, " * 作者: CN_Language关键字模块\n");
    fprintf(file, " * 生成日期: 2026年1月\n");
    fprintf(file, " * 注意: 此文件为自动生成，请勿手动修改\n");
    fprintf(file, " * 版权: MIT许可证\n");
    fprintf(file, " ******************************************************************************/\n\n");
    
    fprintf(file, "#include <stddef.h>\n\n");
    
    // 写入关键字数据数组
    fprintf(file, "// 关键字数据数组\n");
    fprintf(file, "static const struct {\n");
    fprintf(file, "    const char* keyword;\n");
    fprintf(file, "    size_t length;\n");
    fprintf(file, "    int category;\n");
    fprintf(file, "    const char* description;\n");
    fprintf(file, "    const char* english_equivalent;\n");
    fprintf(file, "    int is_reserved;\n");
    fprintf(file, "    int precedence;\n");
    fprintf(file, "} g_keyword_data[] = {\n");
    
    for (size_t i = 0; i < table->count; i++)
    {
        const Stru_CN_KeywordInfo_t* info = &table->keywords[i];
        
        fprintf(file, "    {\n");
        fprintf(file, "        .keyword = \"%s\",\n", info->keyword);
        fprintf(file, "        .length = %zu,\n", info->length);
        fprintf(file, "        .category = %d,\n", info->category);
        
        if (info->description != NULL)
        {
            fprintf(file, "        .description = \"%s\",\n", info->description);
        }
        else
        {
            fprintf(file, "        .description = NULL,\n");
        }
        
        if (info->english_equivalent != NULL)
        {
            fprintf(file, "        .english_equivalent = \"%s\",\n", info->english_equivalent);
        }
        else
        {
            fprintf(file, "        .english_equivalent = NULL,\n");
        }
        
        fprintf(file, "        .is_reserved = %d,\n", info->is_reserved ? 1 : 0);
        fprintf(file, "        .precedence = %d\n", info->precedence);
        
        if (i == table->count - 1)
        {
            fprintf(file, "    }\n");
        }
        else
        {
            fprintf(file, "    },\n");
        }
    }
    
    fprintf(file, "};\n\n");
    
    // 写入关键字数量常量
    fprintf(file, "// 关键字数量\n");
    fprintf(file, "#define KEYWORD_COUNT %zu\n\n", table->count);
    
    // 写入分类枚举
    fprintf(file, "// 关键字分类枚举\n");
    fprintf(file, "enum KeywordCategory {\n");
    fprintf(file, "    CATEGORY_VARIABLE = %d,\n", Eum_KEYWORD_CATEGORY_VARIABLE);
    fprintf(file, "    CATEGORY_CONTROL = %d,\n", Eum_KEYWORD_CATEGORY_CONTROL);
    fprintf(file, "    CATEGORY_FUNCTION = %d,\n", Eum_KEYWORD_CATEGORY_FUNCTION);
    fprintf(file, "    CATEGORY_LOGICAL = %d,\n", Eum_KEYWORD_CATEGORY_LOGICAL);
    fprintf(file, "    CATEGORY_LITERAL = %d,\n", Eum_KEYWORD_CATEGORY_LITERAL);
    fprintf(file, "    CATEGORY_MODULE = %d,\n", Eum_KEYWORD_CATEGORY_MODULE);
    fprintf(file, "    CATEGORY_OPERATOR = %d,\n", Eum_KEYWORD_CATEGORY_OPERATOR);
    fprintf(file, "    CATEGORY_TYPE = %d,\n", Eum_KEYWORD_CATEGORY_TYPE);
    fprintf(file, "    CATEGORY_OTHER = %d\n", Eum_KEYWORD_CATEGORY_OTHER);
    fprintf(file, "};\n");
    
    fclose(file);
    return true;
}
