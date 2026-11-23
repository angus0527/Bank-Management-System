#ifndef DATE_H
#define DATE_H

#include <string>

// 日期类
class Date {
    int year;
    int month;
    int day;

public:
    // 构造函数，日期不合法时重置为 1900-01-01
    Date(int y, int m, int d);

    // 获取今天日期
    static Date today();

    // 字符串转日期，如 "2026-10-29"
    static Date fromString(const std::string &s);

    // 获取当前日期和时间
    static std::string nowDateTime();

    // 判断闰年
    static bool isLeap(int y);

    // 获取某月天数
    static int daysInMonth(int y, int m);

    // 计算从 1900-01-01 到现在的天数
    long long toSerial() const;

    // 计算两个日期相差几天
    long long daysBetween(const Date &other) const;

    // 日期加减天数，n可以是负数
    Date addDays(long long n) const;

    // 加n个月，月末天数不够时取当月最后一天
    Date addMonths(int n) const;

    // 转为字符串，格式如 "2026-10-29"
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
