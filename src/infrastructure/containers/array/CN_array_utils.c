/******************************************************************************
 * 文件名: CN_array_utils.c
 * 功能: CN_Language动态数组工具函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现数组工具函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_array_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// 工具函数实现
// ============================================================================

bool CN_array_swap(Stru_CN_Array_t* array, size_t index1, size_t index2)
{
    if (array == NULL || index1 >= array->length || index2 >= array->length)
    {
        return false;
    }
    
    if (index1 == index2)
    {
        return true; // 相同索引，不需要交换
    }
    
    // 获取元素指针
    void* elem1 = CN_array_internal_get_element_ptr(array, index1);
    void* elem2 = CN_array_internal_get_element_ptr(array, index2);
    
    // 交换元素
    char* bytes1 = (char*)elem1;
    char* bytes2 = (char*)elem2;
    
    for (size_t i = 0; i < array->element_size; i++)
    {
        char temp = bytes1[i];
        bytes1[i] = bytes2[i];
        bytes2[i] = temp;
    }
    
    return true;
}

bool CN_array_fill(Stru_CN_Array_t* array, const void* element, size_t count)
{
    if (array == NULL || element == NULL || count == 0)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!CN_array_internal_ensure_capacity(array, count))
    {
        return false;
    }
    
    // 填充元素
    for (size_t i = 0; i < count; i++)
    {
        void* dest = CN_array_internal_get_element_ptr(array, i);
        memcpy(dest, element, array->element_size);
    }
    
    // 更新长度
    if (count > array->length)
    {
        array->length = count;
    }
    
    return true;
}

bool CN_array_equal(const Stru_CN_Array_t* array1, const Stru_CN_Array_t* array2)
{
    if (array1 == NULL && array2 == NULL)
    {
        return true;
    }
    
    if (array1 == NULL || array2 == NULL)
    {
        return false;
    }
    
    // 检查基本属性
    if (array1->element_type != array2->element_type ||
        array1->element_size != array2->element_size ||
        array1->length != array2->length)
    {
        return false;
    }
    
    // 检查自定义函数
    if (array1->compare_func != array2->compare_func ||
        array1->free_func != array2->free_func ||
        array1->copy_func != array2->copy_func)
    {
        return false;
    }
    
    // 检查数据内容
    if (array1->length == 0)
    {
        return true; // 两个空数组相等
    }
    
    CN_ArrayCompareFunc compare_func = CN_array_internal_get_compare_func(array1);
    
    if (compare_func != NULL)
    {
        // 使用自定义比较函数
        for (size_t i = 0; i < array1->length; i++)
        {
            void* elem1 = CN_array_internal_get_element_ptr(array1, i);
            void* elem2 = CN_array_internal_get_element_ptr(array2, i);
            
            if (compare_func(elem1, elem2) != 0)
            {
                return false;
            }
        }
    }
    else
    {
        // 使用内存比较
        void* data1 = CN_array_internal_get_data(array1);
        void* data2 = CN_array_internal_get_data(array2);
        
        if (memcmp(data1, data2, array1->length * array1->element_size) != 0)
        {
            return false;
        }
    }
    
    return true;
}

void CN_array_dump(const Stru_CN_Array_t* array)
{
    if (array == NULL)
    {
        printf("Array: NULL\n");
        return;
    }
    
    printf("=== Array Information ===\n");
    printf("Address: %p\n", (void*)array);
    printf("Length: %lu\n", (unsigned long)array->length);
    printf("Capacity: %lu\n", (unsigned long)array->capacity);
    printf("Element size: %lu bytes\n", (unsigned long)array->element_size);
    printf("Element type: %d\n", array->element_type);
    printf("Data pointer: %p\n", array->data);
    printf("Compare function: %p\n", (void*)array->compare_func);
    printf("Free function: %p\n", (void*)array->free_func);
    printf("Copy function: %p\n", (void*)array->copy_func);
    
    // 打印前几个元素（如果存在）
    if (array->length > 0)
    {
        printf("\nFirst %d elements:\n", (array->length > 5) ? 5 : (int)array->length);
        int elements_to_print = (array->length > 5) ? 5 : (int)array->length;
        
        for (int i = 0; i < elements_to_print; i++)
        {
            void* element = CN_array_internal_get_element_ptr(array, i);
            printf("  [%d]: ", i);
            
            // 根据元素类型打印
            switch (array->element_type)
            {
                case Eum_ARRAY_ELEMENT_INT8:
                    printf("%d", *(int8_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_UINT8:
                    printf("%u", *(uint8_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_INT16:
                    printf("%d", *(int16_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_UINT16:
                    printf("%u", *(uint16_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_INT32:
                    printf("%d", *(int32_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_UINT32:
                    printf("%u", *(uint32_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_INT64:
                    printf("%lld", *(int64_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_UINT64:
                    printf("%llu", *(uint64_t*)element);
                    break;
                case Eum_ARRAY_ELEMENT_FLOAT:
                    printf("%f", *(float*)element);
                    break;
                case Eum_ARRAY_ELEMENT_DOUBLE:
                    printf("%lf", *(double*)element);
                    break;
                case Eum_ARRAY_ELEMENT_BOOL:
                    printf("%s", *(bool*)element ? "true" : "false");
                    break;
                case Eum_ARRAY_ELEMENT_CHAR:
                    printf("'%c'", *(char*)element);
                    break;
                case Eum_ARRAY_ELEMENT_POINTER:
                    printf("%p", *(void**)element);
                    break;
                case Eum_ARRAY_ELEMENT_CUSTOM:
                    printf("[custom data at %p]", element);
                    break;
                default:
                    printf("[unknown type]");
                    break;
            }
            printf("\n");
        }
        
        if (array->length > 5)
        {
            printf("  ... and %lu more elements\n", (unsigned long)(array->length - 5));
        }
    }
    
    printf("=========================\n");
}
