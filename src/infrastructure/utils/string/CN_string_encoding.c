/******************************************************************************
 * 文件名: CN_string_encoding.c
 * 功能: CN_Language字符串编码转换模块实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现编码转换功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_string_encoding.h"
#include "CN_string_internal.h"
#include "simple_memory.h"
#include <string.h>
#include <ctype.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 检查UTF-8字节序列的有效性
 * 
 * @param data 数据指针
 * @param length 数据长度
 * @param pos 当前位置
 * @param expected_len 期望的序列长度
 * @return 如果有效返回true，否则返回false
 */
static bool is_valid_utf8_sequence(const char* data, size_t length, 
                                   size_t pos, int expected_len)
{
    if (pos + expected_len > length)
    {
        return false;
    }
    
    for (int i = 1; i < expected_len; i++)
    {
        if ((data[pos + i] & 0xC0) != 0x80)
        {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 获取UTF-8字符的码点
 * 
 * @param data UTF-8数据
 * @param length 数据长度
 * @param pos 当前位置
 * @param codepoint 输出参数：码点
 * @return 读取的字节数，0表示错误
 */
static int get_utf8_codepoint(const char* data, size_t length, 
                              size_t pos, uint32_t* codepoint)
{
    if (pos >= length)
    {
        return 0;
    }
    
    unsigned char first_byte = (unsigned char)data[pos];
    
    // 单字节字符 (0xxxxxxx)
    if (first_byte < 0x80)
    {
        *codepoint = first_byte;
        return 1;
    }
    // 两字节字符 (110xxxxx 10xxxxxx)
    else if ((first_byte & 0xE0) == 0xC0)
    {
        if (!is_valid_utf8_sequence(data, length, pos, 2))
        {
            return 0;
        }
        
        *codepoint = ((first_byte & 0x1F) << 6) | 
                     (data[pos + 1] & 0x3F);
        return 2;
    }
    // 三字节字符 (1110xxxx 10xxxxxx 10xxxxxx)
    else if ((first_byte & 0xF0) == 0xE0)
    {
        if (!is_valid_utf8_sequence(data, length, pos, 3))
        {
            return 0;
        }
        
        *codepoint = ((first_byte & 0x0F) << 12) | 
                     ((data[pos + 1] & 0x3F) << 6) | 
                     (data[pos + 2] & 0x3F);
        return 3;
    }
    // 四字节字符 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
    else if ((first_byte & 0xF8) == 0xF0)
    {
        if (!is_valid_utf8_sequence(data, length, pos, 4))
        {
            return 0;
        }
        
        *codepoint = ((first_byte & 0x07) << 18) | 
                     ((data[pos + 1] & 0x3F) << 12) | 
                     ((data[pos + 2] & 0x3F) << 6) | 
                     (data[pos + 3] & 0x3F);
        return 4;
    }
    
    return 0;
}

/**
 * @brief 将码点编码为UTF-8
 * 
 * @param codepoint 码点
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 写入的字节数，0表示错误
 */
static int encode_utf8_codepoint(uint32_t codepoint, char* buffer, size_t buffer_size)
{
    if (codepoint <= 0x7F)
    {
        if (buffer_size < 1) return 0;
        buffer[0] = (char)codepoint;
        return 1;
    }
    else if (codepoint <= 0x7FF)
    {
        if (buffer_size < 2) return 0;
        buffer[0] = (char)(0xC0 | (codepoint >> 6));
        buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    }
    else if (codepoint <= 0xFFFF)
    {
        if (buffer_size < 3) return 0;
        buffer[0] = (char)(0xE0 | (codepoint >> 12));
        buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    }
    else if (codepoint <= 0x10FFFF)
    {
        if (buffer_size < 4) return 0;
        buffer[0] = (char)(0xF0 | (codepoint >> 18));
        buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    
    return 0;
}

// ============================================================================
// 公共API实现
// ============================================================================

Stru_CN_String_t* CN_string_convert_to_encoding(const Stru_CN_String_t* str,
                                                Eum_CN_StringEncoding_t target_encoding)
{
    if (!str)
    {
        return NULL;
    }
    
    Eum_CN_StringEncoding_t source_encoding = CN_string_encoding(str);
    
    // 如果源编码和目标编码相同，直接复制
    if (source_encoding == target_encoding)
    {
        return CN_string_copy(str);
    }
    
    // 目前只支持UTF-8到其他编码的转换
    if (source_encoding != Eum_STRING_ENCODING_UTF8)
    {
        // 未来可以添加其他源编码的支持
        return NULL;
    }
    
    const char* source_data = CN_string_cstr(str);
    size_t source_len = CN_string_length(str);
    
    // 创建目标字符串
    Stru_CN_String_t* result = CN_string_create_empty(source_len * 2, target_encoding);
    if (!result)
    {
        return NULL;
    }
    
    // 根据目标编码进行转换
    switch (target_encoding)
    {
        case Eum_STRING_ENCODING_ASCII:
        {
            // UTF-8到ASCII：只保留ASCII字符，其他替换为'?'
            for (size_t i = 0; i < source_len; i++)
            {
                char ch = source_data[i];
                if ((unsigned char)ch <= 0x7F)
                {
                    CN_string_append_char(result, ch);
                }
                else
                {
                    // 跳过多字节UTF-8字符的后续字节
                    if ((ch & 0xC0) == 0x80)
                    {
                        continue;
                    }
                    // 多字节字符替换为'?'
                    CN_string_append_char(result, '?');
                }
            }
            break;
        }
        
        case Eum_STRING_ENCODING_UTF8:
            // 已经是UTF-8，直接复制（虽然前面已经处理了这种情况）
            CN_string_append_cstr(result, source_data, source_len);
            break;
            
        case Eum_STRING_ENCODING_UTF16:
        case Eum_STRING_ENCODING_UTF32:
        case Eum_STRING_ENCODING_GB2312:
        case Eum_STRING_ENCODING_GBK:
        case Eum_STRING_ENCODING_GB18030:
            // 这些编码转换需要更复杂的实现
            // 暂时只复制原始数据，标记为UTF-8（实际应该转换）
            // TODO: 实现真正的编码转换
            CN_string_append_cstr(result, source_data, source_len);
            // 注意：这里实际上没有转换编码，只是复制了数据
            // 在实际使用中，需要调用相应的转换函数
            break;
            
        default:
            CN_string_destroy(result);
            return NULL;
    }
    
    return result;
}

bool CN_string_encoding_conversion_supported(Eum_CN_StringEncoding_t source_encoding,
                                             Eum_CN_StringEncoding_t target_encoding)
{
    // 目前只支持UTF-8作为源编码
    if (source_encoding != Eum_STRING_ENCODING_UTF8)
    {
        return false;
    }
    
    // 支持UTF-8到所有编码的转换（部分需要完整实现）
    switch (target_encoding)
    {
        case Eum_STRING_ENCODING_ASCII:
        case Eum_STRING_ENCODING_UTF8:
        case Eum_STRING_ENCODING_UTF16:
        case Eum_STRING_ENCODING_UTF32:
        case Eum_STRING_ENCODING_GB2312:
        case Eum_STRING_ENCODING_GBK:
        case Eum_STRING_ENCODING_GB18030:
            return true;
            
        default:
            return false;
    }
}

const char* CN_string_encoding_name(Eum_CN_StringEncoding_t encoding)
{
    switch (encoding)
    {
        case Eum_STRING_ENCODING_ASCII:    return "ASCII";
        case Eum_STRING_ENCODING_UTF8:     return "UTF-8";
        case Eum_STRING_ENCODING_UTF16:    return "UTF-16";
        case Eum_STRING_ENCODING_UTF32:    return "UTF-32";
        case Eum_STRING_ENCODING_GB2312:   return "GB2312";
        case Eum_STRING_ENCODING_GBK:      return "GBK";
        case Eum_STRING_ENCODING_GB18030:  return "GB18030";
        default:                           return "UNKNOWN";
    }
}

Eum_CN_StringEncoding_t CN_string_detect_encoding(const char* data, size_t length)
{
    if (!data || length == 0)
    {
        return Eum_STRING_ENCODING_ASCII;
    }
    
    bool is_ascii = true;
    bool is_utf8 = true;
    
    for (size_t i = 0; i < length; i++)
    {
        unsigned char ch = (unsigned char)data[i];
        
        // 检查是否ASCII
        if (ch > 0x7F)
        {
            is_ascii = false;
        }
        
        // 检查UTF-8有效性
        if (ch >= 0x80)
        {
            // 检查是否有效的UTF-8起始字节
            if ((ch & 0xE0) == 0xC0)
            {
                // 两字节字符
                if (i + 1 >= length || (data[i + 1] & 0xC0) != 0x80)
                {
                    is_utf8 = false;
                }
                i++; // 跳过下一个字节
            }
            else if ((ch & 0xF0) == 0xE0)
            {
                // 三字节字符
                if (i + 2 >= length || 
                    (data[i + 1] & 0xC0) != 0x80 || 
                    (data[i + 2] & 0xC0) != 0x80)
                {
                    is_utf8 = false;
                }
                i += 2; // 跳过下两个字节
            }
            else if ((ch & 0xF8) == 0xF0)
            {
                // 四字节字符
                if (i + 3 >= length || 
                    (data[i + 1] & 0xC0) != 0x80 || 
                    (data[i + 2] & 0xC0) != 0x80 || 
                    (data[i + 3] & 0xC0) != 0x80)
                {
                    is_utf8 = false;
                }
                i += 3; // 跳过下三个字节
            }
            else if ((ch & 0xC0) == 0x80)
            {
                // 意外的连续字节
                is_utf8 = false;
            }
            else
            {
                // 无效的UTF-8字节
                is_utf8 = false;
            }
        }
    }
    
    if (is_ascii)
    {
        return Eum_STRING_ENCODING_ASCII;
    }
    else if (is_utf8)
    {
        return Eum_STRING_ENCODING_UTF8;
    }
    
    // 默认返回UTF-8（最可能的情况）
    return Eum_STRING_ENCODING_UTF8;
}

size_t CN_string_utf8_to_utf16(const char* utf8_str, size_t utf8_len,
                               uint16_t* utf16_str, size_t utf16_buf_size)
{
    if (!utf8_str || !utf16_str || utf16_buf_size == 0)
    {
        return 0;
    }
    
    size_t utf16_pos = 0;
    size_t utf8_pos = 0;
    
    while (utf8_pos < utf8_len && utf16_pos < utf16_buf_size)
    {
        uint32_t codepoint;
        int bytes_read = get_utf8_codepoint(utf8_str, utf8_len, utf8_pos, &codepoint);
        
        if (bytes_read == 0)
        {
            // 无效的UTF-8序列
            break;
        }
        
        utf8_pos += bytes_read;
        
        // 将码点编码为UTF-16
        if (codepoint <= 0xFFFF)
        {
            // 基本多文种平面（BMP）
            if (utf16_pos >= utf16_buf_size)
            {
                break;
            }
            utf16_str[utf16_pos++] = (uint16_t)codepoint;
        }
        else if (codepoint <= 0x10FFFF)
        {
            // 辅助平面，需要代理对
            if (utf16_pos + 1 >= utf16_buf_size)
            {
                break;
            }
            
            // 计算代理对
            codepoint -= 0x10000;
            uint16_t high_surrogate = (uint16_t)(0xD800 | (codepoint >> 10));
            uint16_t low_surrogate = (uint16_t)(0xDC00 | (codepoint & 0x3FF));
            
            utf16_str[utf16_pos++] = high_surrogate;
            utf16_str[utf16_pos++] = low_surrogate;
        }
        else
        {
            // 无效的码点
            break;
        }
    }
    
    return utf16_pos;
}

size_t CN_string_utf16_to_utf8(const uint16_t* utf16_str, size_t utf16_len,
                               char* utf8_str, size_t utf8_buf_size)
{
    if (!utf16_str || !utf8_str || utf8_buf_size == 0)
    {
        return 0;
    }
    
    size_t utf8_pos = 0;
    size_t utf16_pos = 0;
    
    while (utf16_pos < utf16_len && utf8_pos < utf8_buf_size)
    {
        uint32_t codepoint;
        uint16_t ch = utf16_str[utf16_pos++];
        
        if (ch >= 0xD800 && ch <= 0xDBFF)
        {
            // 高代理项
            if (utf16_pos >= utf16_len)
            {
                // 不完整代理对
                break;
            }
            
            uint16_t low_surrogate = utf16_str[utf16_pos];
            if (low_surrogate < 0xDC00 || low_surrogate > 0xDFFF)
            {
                // 无效的低代理项
                break;
            }
            
            utf16_pos++; // 消耗低代理项
            codepoint = 0x10000 + ((ch - 0xD800) << 10) + (low_surrogate - 0xDC00);
        }
        else if (ch >= 0xDC00 && ch <= 0xDFFF)
        {
            // 孤立的低代理项
            break;
        }
        else
        {
            // 基本多文种平面字符
            codepoint = ch;
        }
        
        // 将码点编码为UTF-8
        int bytes_written = encode_utf8_codepoint(codepoint, 
                                                  utf8_str + utf8_pos, 
                                                  utf8_buf_size - utf8_pos);
        if (bytes_written == 0)
        {
            break;
        }
        
        utf8_pos += bytes_written;
    }
    
    return utf8_pos;
}

size_t CN_string_utf8_to_utf32(const char* utf8_str, size_t utf8_len,
                               uint32_t* utf32_str, size_t utf32_buf_size)
{
    if (!utf8_str || !utf32_str || utf32_buf_size == 0)
    {
        return 0;
    }
    
    size_t utf32_pos = 0;
    size_t utf8_pos = 0;
    
    while (utf8_pos < utf8_len && utf32_pos < utf32_buf_size)
    {
        uint32_t codepoint;
        int bytes_read = get_utf8_codepoint(utf8_str, utf8_len, utf8_pos, &codepoint);
        
        if (bytes_read == 0)
        {
            // 无效的UTF-8序列
            break;
        }
        
        utf8_pos += bytes_read;
        utf32_str[utf32_pos++] = codepoint;
    }
    
    return utf32_pos;
}

size_t CN_string_utf32_to_utf8(const uint32_t* utf32_str, size_t utf32_len,
                               char* utf8_str, size_t utf8_buf_size)
{
    if (!utf32_str || !utf8_str || utf8_buf_size == 0)
    {
        return 0;
    }
    
    size_t utf8_pos = 0;
    
    for (size_t i = 0; i < utf32_len && utf8_pos < utf8_buf_size; i++)
    {
        uint32_t codepoint = utf32_str[i];
        
        // 将码点编码为UTF-8
        int bytes_written = encode_utf8_codepoint(codepoint,
                                                  utf8_str + utf8_pos,
                                                  utf8_buf_size - utf8_pos);
        if (bytes_written == 0)
        {
            break;
        }
        
        utf8_pos += bytes_written;
    }
    
    return utf8_pos;
}

size_t CN_string_gb2312_to_utf8(const char* gb2312_str, size_t gb2312_len,
                                char* utf8_str, size_t utf8_buf_size)
{
    if (!gb2312_str || !utf8_str || utf8_buf_size == 0)
    {
        return 0;
    }
    
    // 简化的GB2312到UTF-8转换
    // 注意：这是一个简化实现，只处理ASCII和部分常见中文字符
    // 完整的GB2312转换表需要包含所有20902个字符
    
    size_t utf8_pos = 0;
    
    for (size_t i = 0; i < gb2312_len && utf8_pos < utf8_buf_size; i++)
    {
        unsigned char ch = (unsigned char)gb2312_str[i];
        
        if (ch <= 0x7F)
        {
            // ASCII字符
            if (utf8_pos >= utf8_buf_size)
            {
                break;
            }
            utf8_str[utf8_pos++] = ch;
        }
        else if (i + 1 < gb2312_len)
        {
            // GB2312双字节字符
            unsigned char high = ch;
            unsigned char low = (unsigned char)gb2312_str[i + 1];
            
            // 简化的转换：将GB2312字符映射到Unicode
            // 这里只是一个示例，实际需要完整的转换表
            uint32_t codepoint = 0x4E00; // 默认："一"字
            
            // 尝试简单的映射（仅示例）
            if (high >= 0xB0 && high <= 0xF7 && low >= 0xA1 && low <= 0xFE)
            {
                // GB2312一级汉字区
                int offset = ((high - 0xB0) * 94) + (low - 0xA1);
                codepoint = 0x4E00 + offset; // 从"一"开始
            }
            
            // 将码点编码为UTF-8
            int bytes_written = encode_utf8_codepoint(codepoint,
                                                      utf8_str + utf8_pos,
                                                      utf8_buf_size - utf8_pos);
            if (bytes_written == 0)
            {
                break;
            }
            
            utf8_pos += bytes_written;
            i++; // 消耗第二个字节
        }
        else
        {
            // 不完整的GB2312字符
            break;
        }
    }
    
    return utf8_pos;
}

size_t CN_string_utf8_to_gb2312(const char* utf8_str, size_t utf8_len,
                                char* gb2312_str, size_t gb2312_buf_size)
{
    if (!utf8_str || !gb2312_str || gb2312_buf_size == 0)
    {
        return 0;
    }
    
    // 简化的UTF-8到GB2312转换
    // 注意：这是一个简化实现，只处理ASCII和部分常见中文字符
    
    size_t gb2312_pos = 0;
    size_t utf8_pos = 0;
    
    while (utf8_pos < utf8_len && gb2312_pos < gb2312_buf_size)
    {
        uint32_t codepoint;
        int bytes_read = get_utf8_codepoint(utf8_str, utf8_len, utf8_pos, &codepoint);
        
        if (bytes_read == 0)
        {
            break;
        }
        
        utf8_pos += bytes_read;
        
        if (codepoint <= 0x7F)
        {
            // ASCII字符
            if (gb2312_pos >= gb2312_buf_size)
            {
                break;
            }
            gb2312_str[gb2312_pos++] = (char)codepoint;
        }
        else if (codepoint >= 0x4E00 && codepoint <= 0x9FA5)
        {
            // 基本汉字区，尝试映射到GB2312
            if (gb2312_pos + 1 >= gb2312_buf_size)
            {
                break;
            }
            
            // 简化的反向映射（仅示例）
            int offset = codepoint - 0x4E00;
            if (offset < 94 * 87) // GB2312一级汉字区大小
            {
                int high = 0xB0 + (offset / 94);
                int low = 0xA1 + (offset % 94);
                
                gb2312_str[gb2312_pos++] = (char)high;
                gb2312_str[gb2312_pos++] = (char)low;
            }
            else
            {
                // 超出GB2312范围，替换为'?'
                if (gb2312_pos >= gb2312_buf_size)
                {
                    break;
                }
                gb2312_str[gb2312_pos++] = '?';
            }
        }
        else
        {
            // 非ASCII且非基本汉字，替换为'?'
            if (gb2312_pos >= gb2312_buf_size)
            {
                break;
            }
            gb2312_str[gb2312_pos++] = '?';
        }
    }
    
    return gb2312_pos;
}
