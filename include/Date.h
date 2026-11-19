#ifndef DATE_H
#define DATE_H

#include <string>

// 日期工具类
// 内部通过统计距 1900-01-01 的天数实现日期加减与天数差计算
class Date {
    int year;
    int month;
    int day;

public:
    // 构造日期（若年月日不合法，提示并自动重置为 1900-01-01）
    Date(int y, int m, int d);

    // 获取当前系统日期
    static Date today();

    // 解析 "YYYY-MM-DD" 格式字符串为日期对象（格式有误则重置为 1900-01-01）
    static Date fromString(const std::string &s);

    // 获取当前日期时间字符串 "YYYY-MM-DD HH:MM:SS"（流水和日志时间戳用）
    static std::string nowDateTime();

    // 判断年份 y 是否为闰年
    static bool isLeap(int y);

    // 返回某年某月共有多少天（月份非法返回 0）
    static int daysInMonth(int y, int m);

    // 统计从 1900-01-01 到当前日期经过的天数
    long long toSerial() const;

    // 计算从当前日期(this)到 other 相隔几天（other 更早时返回负数）
    long long daysBetween(const Date &other) const;

    // 返回加上或减去 n 天后的新日期（支持负数）
    Date addDays(long long n) const;

    // 加 n 个月，月末自动收敛：2026-01-31 加 1 个月 → 2026-02-28（定期到期日用）
    Date addMonths(int n) const;

    // 格式化为 "2026-10-29"
    std::string toString() const;

    int getYear() const { return year; }

    int getMonth() const { return month; }

    int getDay() const { return day; }

    bool operator==(const Date &o) const { return year == o.year && month == o.month && day == o.day; }

    bool operator!=(const Date &o) const { return !(*this == o); }

    bool operator<(const Date &o) const { return toSerial() < o.toSerial(); }

    bool operator<=(const Date &o) const { return toSerial() <= o.toSerial(); }

    bool operator>(const Date &o) const { return o < *this; }

    bool operator>=(const Date &o) const { return o <= *this; }
};

#endif //DATE_H
