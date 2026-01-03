/******************************************************************************
 * 文件名: CN_platform_windows_time.c
 * 功能: CN_Language Windows平台时间和日期子系统
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows时间和日期功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows.h"
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 将Windows FILETIME转换为Unix时间戳（毫秒）
 */
static uint64_t filetime_to_timestamp(const FILETIME* ft)
{
    ULARGE_INTEGER uli;
    uli.LowPart = ft->dwLowDateTime;
    uli.HighPart = ft->dwHighDateTime;
    
    // FILETIME是100纳秒间隔，从1601-01-01开始
    // 转换为Unix时间戳（毫秒，从1970-01-01开始）
    return (uli.QuadPart / 10000) - 11644473600000ULL;
}

/**
 * @brief 将Unix时间戳（毫秒）转换为Windows FILETIME
 */
static void timestamp_to_filetime(uint64_t timestamp, FILETIME* ft)
{
    ULARGE_INTEGER uli;
    
    // Unix时间戳（毫秒）转换为FILETIME（100纳秒间隔）
    uli.QuadPart = (timestamp + 11644473600000ULL) * 10000;
    
    ft->dwLowDateTime = uli.LowPart;
    ft->dwHighDateTime = uli.HighPart;
}

/**
 * @brief 将SYSTEMTIME转换为CN时间结构
 */
static void systemtime_to_cn_time(const SYSTEMTIME* st, Stru_CN_Time_t* cn_time)
{
    cn_time->year = st->wYear;
    cn_time->month = (uint8_t)st->wMonth;
    cn_time->day = (uint8_t)st->wDay;
    cn_time->hour = (uint8_t)st->wHour;
    cn_time->minute = (uint8_t)st->wMinute;
    cn_time->second = (uint8_t)st->wSecond;
    cn_time->millisecond = st->wMilliseconds;
}

/**
 * @brief 将CN时间结构转换为SYSTEMTIME
 */
static void cn_time_to_systemtime(const Stru_CN_Time_t* cn_time, SYSTEMTIME* st)
{
    st->wYear = cn_time->year;
    st->wMonth = cn_time->month;
    st->wDay = cn_time->day;
    st->wHour = cn_time->hour;
    st->wMinute = cn_time->minute;
    st->wSecond = cn_time->second;
    st->wMilliseconds = cn_time->millisecond;
    st->wDayOfWeek = 0; // 由系统计算
}

// ============================================================================
// 时间接口实现
// ============================================================================

// 获取当前时间戳（毫秒）
uint64_t windows_get_current_time(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return filetime_to_timestamp(&ft);
}

// 获取当前时间戳（纳秒）
uint64_t windows_get_current_time_nanos(void)
{
    LARGE_INTEGER frequency, counter;
    
    // 获取性能计数器频率
    if (!QueryPerformanceFrequency(&frequency))
    {
        // 如果性能计数器不可用，回退到毫秒精度
        return windows_get_current_time() * 1000000ULL;
    }
    
    // 获取性能计数器值
    QueryPerformanceCounter(&counter);
    
    // 转换为纳秒
    return (counter.QuadPart * 1000000000ULL) / frequency.QuadPart;
}

// 将时间戳转换为本地时间
bool windows_time_to_local(uint64_t timestamp, Stru_CN_Time_t* local_time)
{
    if (!local_time) return false;
    
    FILETIME ft_utc, ft_local;
    SYSTEMTIME st_local;
    
    // 将时间戳转换为FILETIME
    timestamp_to_filetime(timestamp, &ft_utc);
    
    // 转换为本地时间
    if (!FileTimeToLocalFileTime(&ft_utc, &ft_local))
    {
        return false;
    }
    
    // 转换为SYSTEMTIME
    if (!FileTimeToSystemTime(&ft_local, &st_local))
    {
        return false;
    }
    
    // 转换为CN时间结构
    systemtime_to_cn_time(&st_local, local_time);
    return true;
}

// 将时间戳转换为UTC时间
bool windows_time_to_utc(uint64_t timestamp, Stru_CN_Time_t* utc_time)
{
    if (!utc_time) return false;
    
    FILETIME ft_utc;
    SYSTEMTIME st_utc;
    
    // 将时间戳转换为FILETIME
    timestamp_to_filetime(timestamp, &ft_utc);
    
    // 转换为SYSTEMTIME
    if (!FileTimeToSystemTime(&ft_utc, &st_utc))
    {
        return false;
    }
    
    // 转换为CN时间结构
    systemtime_to_cn_time(&st_utc, utc_time);
    return true;
}

// 从本地时间创建时间戳
uint64_t windows_time_from_local(const Stru_CN_Time_t* local_time)
{
    if (!local_time) return 0;
    
    SYSTEMTIME st_local;
    FILETIME ft_local, ft_utc;
    
    // 将CN时间结构转换为SYSTEMTIME
    cn_time_to_systemtime(local_time, &st_local);
    
    // 转换为FILETIME
    if (!SystemTimeToFileTime(&st_local, &ft_local))
    {
        return 0;
    }
    
    // 转换为UTC时间
    if (!LocalFileTimeToFileTime(&ft_local, &ft_utc))
    {
        return 0;
    }
    
    // 转换为时间戳
    return filetime_to_timestamp(&ft_utc);
}

// 从UTC时间创建时间戳
uint64_t windows_time_from_utc(const Stru_CN_Time_t* utc_time)
{
    if (!utc_time) return 0;
    
    SYSTEMTIME st_utc;
    FILETIME ft_utc;
    
    // 将CN时间结构转换为SYSTEMTIME
    cn_time_to_systemtime(utc_time, &st_utc);
    
    // 转换为FILETIME
    if (!SystemTimeToFileTime(&st_utc, &ft_utc))
    {
        return 0;
    }
    
    // 转换为时间戳
    return filetime_to_timestamp(&ft_utc);
}

// 格式化时间
char* windows_format_time(uint64_t timestamp, const char* format)
{
    if (!format) return NULL;
    
    // 将时间戳转换为本地时间
    Stru_CN_Time_t local_time;
    if (!windows_time_to_local(timestamp, &local_time))
    {
        return NULL;
    }
    
    // 构建格式字符串
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
                      "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                      local_time.year, local_time.month, local_time.day,
                      local_time.hour, local_time.minute, local_time.second,
                      local_time.millisecond);
    
    if (len <= 0 || len >= (int)sizeof(buffer))
    {
        return NULL;
    }
    
    // 复制到动态分配的内存
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    
    strcpy(result, buffer);
    return result;
}

// 解析时间字符串
bool windows_parse_time(const char* str, const char* format, uint64_t* timestamp)
{
    if (!str || !format || !timestamp) return false;
    
    // 简化实现：支持常见格式
    // 这里实现一个简单的解析器，实际项目中可能需要更复杂的解析
    
    Stru_CN_Time_t time = {0};
    int parsed = sscanf(str, "%hu-%hhu-%hhu %hhu:%hhu:%hhu",
                       &time.year, &time.month, &time.day,
                       &time.hour, &time.minute, &time.second);
    
    if (parsed >= 3)
    {
        // 如果解析成功，创建时间戳
        *timestamp = windows_time_from_local(&time);
        return true;
    }
    
    return false;
}

// 获取性能计数器值
uint64_t windows_get_performance_counter(void)
{
    LARGE_INTEGER counter;
    if (QueryPerformanceCounter(&counter))
    {
        return counter.QuadPart;
    }
    return 0;
}

// 获取性能计数器频率
uint64_t windows_get_performance_frequency(void)
{
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency))
    {
        return frequency.QuadPart;
    }
    return 1000; // 默认1kHz
}

// 睡眠指定毫秒数
void windows_sleep(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

// 睡眠指定纳秒数
void windows_sleep_nanos(uint64_t nanoseconds)
{
    // Windows Sleep函数精度为毫秒，所以将纳秒转换为毫秒
    uint32_t milliseconds = (uint32_t)(nanoseconds / 1000000ULL);
    if (milliseconds == 0 && nanoseconds > 0)
    {
        milliseconds = 1; // 至少睡眠1毫秒
    }
    Sleep(milliseconds);
}

// 获取时区偏移（分钟）
int32_t windows_get_timezone_offset(void)
{
    TIME_ZONE_INFORMATION tz_info;
    DWORD result = GetTimeZoneInformation(&tz_info);
    
    if (result == TIME_ZONE_ID_INVALID)
    {
        return 0;
    }
    
    // 返回以分钟为单位的偏移量（UTC+时区偏移）
    // Windows的Bias是以分钟为单位的，但符号与标准相反
    // 标准：UTC+8表示东八区，Windows：Bias = -480表示东八区
    return -tz_info.Bias;
}

// 检查是否夏令时
bool windows_is_daylight_saving_time(void)
{
    TIME_ZONE_INFORMATION tz_info;
    DWORD result = GetTimeZoneInformation(&tz_info);
    
    if (result == TIME_ZONE_ID_INVALID || result == TIME_ZONE_ID_UNKNOWN)
    {
        return false;
    }
    
    return (result == TIME_ZONE_ID_DAYLIGHT);
}

// 获取星期几（0=周日，1=周一，...，6=周六）
uint8_t windows_get_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 1 || month > 12 || day < 1 || day > 31)
    {
        return 0;
    }
    
    SYSTEMTIME st;
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    
    // 计算星期几
    FILETIME ft;
    if (!SystemTimeToFileTime(&st, &ft))
    {
        return 0;
    }
    
    SYSTEMTIME st_with_dow;
    if (!FileTimeToSystemTime(&ft, &st_with_dow))
    {
        return 0;
    }
    
    // 转换为0=周日，1=周一，...，6=周六
    return (uint8_t)(st_with_dow.wDayOfWeek % 7);
}

// 获取一年中的第几天（1-366）
uint16_t windows_get_day_of_year(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 1 || month > 12 || day < 1 || day > 31)
    {
        return 0;
    }
    
    // 构建年初时间
    Stru_CN_Time_t start_of_year = {year, 1, 1, 0, 0, 0, 0};
    Stru_CN_Time_t target_date = {year, month, day, 0, 0, 0, 0};
    
    // 计算时间戳
    uint64_t start_ts = windows_time_from_local(&start_of_year);
    uint64_t target_ts = windows_time_from_local(&target_date);
    
    if (start_ts == 0 || target_ts == 0)
    {
        return 0;
    }
    
    // 计算天数差
    uint64_t diff_ms = target_ts - start_ts;
    uint64_t diff_days = diff_ms / (1000ULL * 60 * 60 * 24);
    
    // 加1因为第一天是第1天
    return (uint16_t)(diff_days + 1);
}

// 检查是否为闰年
bool windows_is_leap_year(uint16_t year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

// ============================================================================
// 时间接口结构体
// ============================================================================

Stru_CN_TimeInterface_t g_windows_time_interface = {
    .get_current_time = windows_get_current_time,
    .get_current_time_nanos = windows_get_current_time_nanos,
    .time_to_local = windows_time_to_local,
    .time_to_utc = windows_time_to_utc,
    .time_from_local = windows_time_from_local,
    .time_from_utc = windows_time_from_utc,
    .format_time = windows_format_time,
    .parse_time = windows_parse_time,
    .get_performance_counter = windows_get_performance_counter,
    .get_performance_frequency = windows_get_performance_frequency,
    .sleep = windows_sleep,
    .sleep_nanos = windows_sleep_nanos,
    .get_timezone_offset = windows_get_timezone_offset,
    .is_daylight_saving_time = windows_is_daylight_saving_time,
    .get_day_of_week = windows_get_day_of_week,
    .get_day_of_year = windows_get_day_of_year,
    .is_leap_year = windows_is_leap_year
};

// ============================================================================
// 公共接口函数
// ============================================================================

/**
 * @brief 获取Windows平台时间接口
 * 
 * @return Windows平台时间接口指针
 */
Stru_CN_TimeInterface_t* CN_platform_windows_get_time_impl(void)
{
    return &g_windows_time_interface;
}
